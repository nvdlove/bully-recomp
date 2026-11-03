#include "game/Camera.h"

#include <cfloat>
#include <cmath>
#include <cstring>

#include "game/Crt.h"
#include "reversed.h"
#include "runflags.h"
#include "sysabi.h"
// PlayerUserInputHandler's pad reading (0x50B5B0..0x50DE30; its small methods are in
// Camera2.cpp): the pad through XInput or the DirectInput devices (keyboard, mouse,
// up to five game controllers), the controls read from and written to a state block
// by code (keys below 0x100, the mouse's buttons and axes 0x100..0x109, then each
// controller's axes, hat directions and buttons from 0x10A), the frame's update (the
// state kept as last frame's, the devices read, the sticks' dead zones, keyboard
// movement eased, the controls recorded), rumble, and the UserInputManager's set-up.
// Thiscalls are __fastcall wrappers with the unused edx.
//
// Layout (PlayerUserInputHandler):
//   +0 uses XInput, +4/+8 the keyboard and mouse devices, +0xC the controllers (0x30
//   each: +0 axes, +4 buttons, +8 hats, +0xC the device, +0x10 its DIDATAFORMAT, +0x28
//   force feedback, +0x2C the dead zone), +0xFC how many, +0x100/+0x104 when the
//   keyboard's x/y easing started, +0x108..+0x114 its from/to values, +0x138/+0x13C
//   the XInput user, +0x140 when there was input, +0x144 the state (0x3B4 bytes),
//   +0x4F8 last frame's, +0x8AC/+0x8B0 XInput buttons released/pressed this frame,
//   +0x8B4, +0x8BA can rumble, +0x8BC the rumble's countdown, +0x8BE its strength,
//   +0x8C0 the recorded controls (30 words, +0x8FC the next), +0x900 the XInput
//   capabilities.
// The state: +0x40 the sticks as used (left x/y, right x/y), +0x90 as read, +0xA0
// the XINPUT_STATE, +0xB0 the mouse (DIMOUSESTATE), +0xC0 the keys (256), +0x1C0
// the controllers' states (100 bytes each).

#define PLAYERINPUT_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

constexpr size_t kState = 0x144;
constexpr size_t kLastState = 0x4F8;
constexpr size_t kStateSize = 0x3B4;
constexpr int32_t kFirstControllerCode = 0x10A;
constexpr long kNotAcquired = long(0x8007000C);   // DIERR_NOTACQUIRED
constexpr long kInputLost = long(0x8007001E);     // DIERR_INPUTLOST

// DIEFFECT (the constant force the rumble sends; the system's layout: native pointers)
struct DIEffect {
    uint32_t size;               // +0
    uint32_t flags;              // +4
    int32_t duration;            // +8
    uint32_t samplePeriod;       // +0xC
    uint32_t gain;               // +0x10
    uint32_t triggerButton;      // +0x14
    uint32_t triggerRepeat;      // +0x18
    uint32_t axisCount;          // +0x1C
    uint32_t* axes;              // +0x20
    int32_t* directions;         // +0x24
    void* envelope;              // +0x28
    uint32_t typeSpecificSize;   // +0x2C
    void* typeSpecific;          // +0x30
    uint32_t startDelay;         // +0x34
};
static_assert(sizeof(void*) != 4 || sizeof(DIEffect) == 0x38);

// DIOBJECTDATAFORMAT
struct DIObjectFormat {
    re::Ptr32<const void> guid;   // +0
    uint32_t offset;    // +4
    uint32_t type;      // +8
    uint32_t flags;     // +0xC
};
static_assert(sizeof(DIObjectFormat) == 0x10);

using MouseEventFn = void(__stdcall*)(uint32_t flags, uint32_t dx, uint32_t dy, uint32_t data, uintptr_t extra);
GAME_VAR(MouseEventFn, mouseEvent_, 0x008FC238);   // user32's mouse_event (the import slot)

inline void* const buttonMap = reinterpret_cast<void*>(0x00A14810);   // GsButtonMap
inline const int32_t* const buttonMasks = reinterpret_cast<const int32_t*>(0x00A14810);
inline const int32_t* const keysA = reinterpret_cast<const int32_t*>(0x00A14850);   // the button map's keys
inline const int32_t* const keysB = reinterpret_cast<const int32_t*>(0x00A148B4);   // its saved keys
inline const void* const guidConstantForce = reinterpret_cast<const void*>(0x009A20D4);
inline const void* const dfKeyboard = reinterpret_cast<const void*>(0x009A2304);   // c_dfDIKeyboard
inline const void* const dfMouse = reinterpret_cast<const void*>(0x009A250C);      // c_dfDIMouse
inline const void* const iidDirectInput8 = reinterpret_cast<const void*>(0x009A2004);
inline void* const enumDevicesCallback = reinterpret_cast<void*>(0x007383D0);

GAME_VAR(float, mouseSensitivity, 0x00A1491C);      // the button map's +0x10C
GAME_VAR(uint8_t, userStart_20C32E9, 0x020C32E9);
GAME_VAR(uint8_t, userStart_20C32EA, 0x020C32EA);
GAME_VAR(uint8_t*, gameMode, 0x020C8704);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);          // CWorld::Players[0].m_pPed
GAME_VAR(uint8_t, m_OtherPause, 0x00C1A998);
GAME_VAR(uint8_t, m_CodePause, 0x00C1A999);
GAME_VAR(uint8_t, m_UserPause, 0x00C1A99A);
GAME_VAR(float, timeStep, 0x00C1A9A4);              // CTimer::ms_fTimeStep
GAME_VAR(void*, ms_hWnd, 0x00BD77FC);               // RsGlobal's window
GAME_VAR(void*, ms_hInstance, 0x00BD77B4);
GAME_ARRAY(uint8_t, xinputUserTaken, 0x00C3D2B8);   // [4]
GAME_VAR(int32_t, keyboardEasing, 0x00A45A00);      // 0 off, 1..5 the curve
GAME_VAR(uint8_t, clampRightStick, 0x00A459FC);
GAME_VAR(float, rumbleRate, 0x00A45A04);            // 0.02

inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
inline void* __cdecl CrtMalloc(size_t size) { return crt::Malloc(size); }
GAME_FN(uint32_t, __stdcall, XInputSetState_, 0x0085AB8A, uint32_t user, void* vibration);
GAME_FN(uint32_t, __stdcall, XInputGetCapabilities_, 0x0085AB90, uint32_t user, uint32_t flags, void* caps);
GAME_FN(uint32_t, __stdcall, XInputGetState_, 0x0085AB96, uint32_t user, void* state);
GAME_FN(long, __stdcall, DirectInput8Create_, 0x008D9A80, void* instance, uint32_t version, const void* iid, void** out, void* outer);
GAME_FN(const int32_t*, __thiscall, ButtonMap_GetKeys, 0x00454CD0, void* map, int saved);
GAME_FN(void, __thiscall, ButtonMap_SetDefaultKeys, 0x00454E00, void* map);
GAME_FN(void, __thiscall, ButtonMap_Save, 0x00454B70, void* map);
GAME_FN(uint32_t, __cdecl, GetTimeMs, 0x0045BCA0);
GAME_FN(float, __cdecl, RecipSqrt, 0x00414C20, float numerator, float value);   // (a / sqrt |b|)
GAME_FN(uint8_t, __thiscall, Ped_HasWeapon, 0x00474BF0, void* ped);
GAME_FN(void*, __cdecl, MakeInputHandler, 0x0050E5A0, int index);
GAME_FN(void, __cdecl, Input_738470, 0x00738470);
GAME_FN(int32_t, __cdecl, GetActivePad, 0x00738070);
GAME_FN(uint8_t, __cdecl, IsKeyUnbindable, 0x0050BCC0, int key);
// PlayerUserInputHandler (through the game's addresses, so modes apply)
GAME_FN(void, __thiscall, PlayerInput_UpdateRumble, 0x0050B900, void* self);
GAME_FN(int32_t, __thiscall, PlayerInput_MapCode, 0x0050BDD0, void* self, int code, int32_t* device, int32_t* index);
GAME_FN(float, __thiscall, PlayerInput_GetValue, 0x0050C030, void* self, int code, uint8_t* state);
GAME_FN(void, __thiscall, PlayerInput_SetValue, 0x0050C3C0, void* self, int code, float value, uint8_t* state);
GAME_FN(float, __thiscall, PlayerInput_GetControl, 0x0050C880, void* self, int control);
GAME_FN(uint8_t, __thiscall, PlayerInput_JustPressed, 0x0050C920, void* self, int control);

using re::At;

// a COM method (stdcall, the object first)
template <typename R, typename... A>
R Com(void* object, int index, A... args) {
    return re::ComCall<R>(object, index, args...);
}

template <typename R, typename... A>
R VCall(void* object, int index, A... args) {
    auto fn = reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[index]));
    return fn(object, args...);
}

const int32_t* Keys() {
    return ButtonMap_GetKeys(buttonMap, 0);
}

uint8_t* Controller(uint8_t* self, int i) {
    return self + 0xC + i * 0x30;
}

// no menu or cutscene in the way of the player's controls (the game also tests the
// game mode's +0x50 member's address)
bool PlayerControlsFree() {
    if (userStart_20C32E9 || userStart_20C32EA) {
        return false;
    }
    uint8_t* mode = gameMode;
    return !mode || uintptr_t(mode) + 0x50 == 0 || !mode[0xA0];
}

// IDirectInputDevice8::GetDeviceState, acquired again once when lost
long ReadDevice(const re::Ptr32<void>* slot, uint32_t size, void* buffer) {
    long hr = Com<long>(*slot, 9, size, buffer);
    if (hr == kNotAcquired || hr == kInputLost) {
        Com<long>(*slot, 7);
        hr = Com<long>(*slot, 9, size, buffer);
    }
    re::NoteInput(size == 0x100 ? "PlayerInput keyboard" : size == 0x10 ? "PlayerInput mouse" : "PlayerInput pad", hr,
                  static_cast<const uint8_t*>(buffer), size);
    if (size == 0x100 && re::ScriptedKeysActive()) {   // (a test run's keys: runflags.h)
        // (only the run's own: a test game in the background reads the keyboard however it's
        // focused (--keep-active), and keys typed into other windows mustn't move it)
        std::memset(buffer, 0, size);
        hr = 0;
        re::ApplyScriptedKeys(static_cast<uint8_t*>(buffer));
    }
    if (!re::GameInputAllowed()) {
        // (bullyre_x64 played with another window in front: nothing pressed, nothing moved)
        std::memset(buffer, 0, size);
        hr = 0;
    }
    if (size == 0x10 && re::ScriptedKeysActive()) {   // (the mouse: a test run's buttons)
        // (only the run's own, as the keyboard's)
        std::memset(buffer, 0, size);
        hr = 0;
        re::ApplyScriptedMouse(static_cast<uint8_t*>(buffer));
    }
    return hr;
}

// the x87 control word (the rumble below sends it by mistake)
uint16_t FpuControlWord() {
#if defined(_M_IX86)
    uint16_t controlWord;
    __asm { fnstcw controlWord }
    return controlWord;
#else
    return 0x027F;
#endif
}

// ---- 0x50B5B0: the look-back controls can be swapped in (not XInput; forced, or
// the player in a vehicle with its controls free), both keys not a controller's
uint8_t __fastcall PlayerInput_CanSwapLookOurs(uint8_t* self, re::Edx, int force) {
    if (At<uint8_t>(self, 0)) {
        return 0;
    }
    if (!uint8_t(force)) {
        uint8_t* player = playerPed;
        if (!player || !At<void*>(player, 0x1554) || !PlayerControlsFree()) {
            return 0;
        }
    }
    if (Keys()[17] < kFirstControllerCode && Keys()[21] < kFirstControllerCode) {
        return 1;
    }
    return 0;
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::CanSwapLook_50B5B0", 0x0050B5B0, RE_BODY(&PlayerInput_CanSwapLookOurs), nullptr, nullptr, re::Mode::Original);

// 0x50B630: the state cleared (both frames' buttons, keys, mouse and controllers)
void __fastcall PlayerInput_ClearOurs(uint8_t* self, re::Edx) {
    At<uint16_t>(self, 0x1E8) = 0;
    At<uint16_t>(self, 0x59C) = 0;
    At<uint32_t>(self, 0x8AC) = 0;
    At<uint32_t>(self, 0x8B0) = 0;
    Memset(self + 0x5B8, 0, 0x100);
    At<uint32_t>(self, 0x5A8) = 0;
    At<uint32_t>(self, 0x5AC) = 0;
    At<uint32_t>(self, 0x5B0) = 0;
    At<uint32_t>(self, 0x5B4) = 0;
    for (int i = 0; i < At<int32_t>(self, 0xFC); i++) {
        uint8_t* state = self + 0x304 + i * 100;
        Memset(state + kStateSize, 0, 100);
        Memset(state, 0, 100);
    }
    Memset(self + 0x204, 0, 0x100);
    At<uint32_t>(self, 0x1F4) = 0;
    At<uint32_t>(self, 0x1F8) = 0;
    At<uint32_t>(self, 0x1FC) = 0;
    At<uint32_t>(self, 0x200) = 0;
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::Clear_50B630", 0x0050B630, RE_BODY(&PlayerInput_ClearOurs), nullptr, nullptr, re::Mode::Original);

// 0x50B710: rumble for `duration` at `strength` (0..1 as a byte): XInput (one motor or
// the other; the left one otherwise gets the x87 control word, the game's slip),
// retried 5 times (32000 while paused), or a constant force effect made on each
// controller (its DIEFFECT's size left unset, as in the game)
void __fastcall PlayerInput_RumbleOurs(uint8_t* self, re::Edx, int duration, int strength, int rightOnly) {
    if (!At<uint8_t>(self, 0x8BA)) {
        return;
    }
    int16_t length = int16_t(duration);
    uint8_t amount = uint8_t(strength);
    At<int16_t>(self, 0x8BC) = length;
    At<uint8_t>(self, 0x8BE) = amount;
    if (At<uint8_t>(self, 0)) {
        uint16_t speed = uint16_t(int32_t(double(amount) * 255.99609375));
        uint16_t vibration[2];
        if (uint8_t(rightOnly)) {
            vibration[0] = speed;
            vibration[1] = 0;
        } else {
            vibration[0] = FpuControlWord();
            vibration[1] = speed;
        }
        int tries = 5;
        if (m_UserPause || m_CodePause || m_OtherPause) {
            tries = 32000;
        }
        if (XInputSetState_(At<uint32_t>(self, 0x13C), vibration) != 0) {
            for (int i = 0; i < tries; i++) {
                if (XInputSetState_(At<uint32_t>(self, 0x13C), vibration) == 0) {
                    return;
                }
            }
        }
        return;
    }
    uint32_t axes[2] = {0, 4};   // DIJOFS_X, DIJOFS_Y
    int32_t directions[2] = {int32_t(double(amount) * 255.99609375), 0};
    int32_t magnitude = 10000;
    DIEffect effect;   // (size and start delay unset)
    effect.flags = 0x20;   // DIEFF_POLAR
    effect.duration = length;
    effect.samplePeriod = 0;
    effect.gain = 10000;
    effect.triggerButton = 0xFFFFFFFF;
    effect.triggerRepeat = 0;
    effect.axisCount = 2;
    effect.axes = axes;
    effect.directions = directions;
    effect.envelope = nullptr;
    effect.typeSpecificSize = 4;
    effect.typeSpecific = &magnitude;
    void* created = nullptr;
    for (int i = 0; i < At<int32_t>(self, 0xFC); i++) {
        void* device = At<void*>(self, 0x18 + i * 0x30);
        Com<long>(device, 18, guidConstantForce, &effect, &created, static_cast<void*>(nullptr));   // CreateEffect
    }
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::Rumble_50B710", 0x0050B710, RE_BODY(&PlayerInput_RumbleOurs), nullptr, nullptr, re::Mode::Original);

// 0x50B9A0: a controller added: with XInput the first free user that answers (its
// rumble from the capabilities); otherwise the DirectInput device (up to five): its
// capabilities, a data format for its axes, hats and buttons, exclusive in the
// foreground, axes -1000..1000, acquired, its dead zone (0x55 without one)
void __fastcall PlayerInput_AddControllerOurs(uint8_t* self, re::Edx, void* device, int xinput) {
    At<uint8_t>(self, 0) = uint8_t(xinput);
    if (uint8_t(xinput)) {
        for (uint32_t user = 0; user < 4; user++) {
            uint8_t state[0x10];
            if (!xinputUserTaken[user] && XInputGetState_(user, state) == 0) {
                At<uint32_t>(self, 0x13C) = user;
                xinputUserTaken[user] = 1;
                break;
            }
        }
        XInputGetCapabilities_(At<uint32_t>(self, 0x13C), 1, self + 0x900);
        At<uint8_t>(self, 0x8BA) = uint32_t(At<uint16_t>(self, 0x912)) + At<uint16_t>(self, 0x910) != 0 ? 1 : 0;
        return;
    }
    if (At<int32_t>(self, 0xFC) >= 5) {
        return;
    }
    uint8_t* entry = Controller(self, At<int32_t>(self, 0xFC));
    At<void*>(entry, 0xC) = device;
    uint32_t caps[11];   // DIDEVCAPS
    caps[0] = 0x2C;
    Com<long>(device, 3, caps);   // GetCapabilities
    uint8_t feedback = uint8_t((caps[1] >> 8) & 1);   // DIDC_FORCEFEEDBACK
    At<uint8_t>(entry, 0x28) = feedback;
    At<uint8_t>(self, 0x8BA) |= feedback;
    int32_t axisCount = int32_t(caps[3]);
    int32_t buttonCount = int32_t(caps[4]);
    int32_t hatCount = int32_t(caps[5]);
    At<int32_t>(entry, 0) = axisCount;
    At<int32_t>(entry, 4) = buttonCount;
    At<int32_t>(entry, 8) = hatCount;
    int32_t dataSize = buttonCount + (axisCount + hatCount * 2) * 4;
    At<uint32_t>(entry, 0x10) = 0x18;   // DIDATAFORMAT: its size, the object size, DIDF_ABSAXIS
    At<uint32_t>(entry, 0x14) = 0x10;
    At<uint32_t>(entry, 0x18) = 1;
    int32_t rem = dataSize % 4;
    At<int32_t>(entry, 0x1C) = (rem == 0 ? 0 : 4 - rem) + dataSize;
    uint32_t objectCount = uint32_t(buttonCount + axisCount + hatCount);
    At<uint32_t>(entry, 0x20) = objectCount;
    auto* objects = static_cast<DIObjectFormat*>(CrtMalloc(objectCount << 4));
    At<DIObjectFormat*>(entry, 0x24) = objects;
    for (uint32_t i = 0; i < At<uint32_t>(entry, 0x20); i++) {
        int32_t axes = At<int32_t>(entry, 0);
        DIObjectFormat& o = objects[i];
        o.guid = nullptr;
        if (int32_t(i) < axes) {
            o.offset = i * 4;
            o.type = 0xFFFF03;   // DIDFT_AXIS
        } else if (int32_t(i) < At<int32_t>(entry, 8) + axes) {
            o.offset = uint32_t(int32_t(i * 8) - axes * 4);
            o.type = 0xFFFF10;   // DIDFT_POV
        } else {
            o.offset = uint32_t(At<int32_t>(entry, 8) * 7 + axes * 3 + int32_t(i));
            o.type = 0xFFFF0C;   // DIDFT_BUTTON
        }
        o.flags = 0;
    }
    Com<long>(At<void*>(entry, 0xC), 11, re::DIDataFormat(entry + 0x10).get());   // SetDataFormat
    Com<long>(At<void*>(entry, 0xC), 13, ms_hWnd, uint32_t(5));      // SetCooperativeLevel (exclusive, foreground)
    int32_t range[6] = {0x18, 0x10, 0, 0, -1000, 1000};             // DIPROPRANGE
    Com<long>(At<void*>(entry, 0xC), 6, reinterpret_cast<const void*>(4), range);   // SetProperty(DIPROP_RANGE)
    Com<long>(At<void*>(entry, 0xC), 7);                             // Acquire
    At<int32_t>(entry, 0x2C) = 0x55;
    int32_t deadZone[5] = {0x14, 0x10, 0, 0, 0};                     // DIPROPDWORD (the value unset in the game)
    if (Com<long>(At<void*>(entry, 0xC), 5, reinterpret_cast<const void*>(5), deadZone) == 0) {   // GetProperty(DIPROP_DEADZONE)
        At<int32_t>(entry, 0x2C) = deadZone[4];
    }
    At<int32_t>(self, 0xFC) += 1;
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::AddController_50B9A0", 0x0050B9A0, RE_BODY(&PlayerInput_AddControllerOurs), nullptr, nullptr, re::Mode::Original);

// 0x50BC40: the keyboard device (keyboard format, exclusive in the foreground, acquired)
void __fastcall PlayerInput_SetKeyboardOurs(uint8_t* self, re::Edx, void* device) {
    At<void*>(self, 4) = device;
    long format = Com<long>(device, 11, re::DIDataFormat(dfKeyboard).get());
    // (exclusive in the foreground; bullyre_x64 played: shared, the Windows key working: runflags.h)
    long level = Com<long>(At<void*>(self, 4), 13, ms_hWnd, uint32_t(re::PlayWindow() ? 6 : 5));
    long acquire = Com<long>(At<void*>(self, 4), 7);
    char note[160];
    std::snprintf(note, sizeof note, "keyboard setup: device %p window %p format %08lX level %08lX acquire", device,
                  static_cast<void*>(ms_hWnd), static_cast<unsigned long>(format), static_cast<unsigned long>(level));
    re::NoteInput(note, acquire, nullptr, 0);
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::SetKeyboard_50BC40", 0x0050BC40, RE_BODY(&PlayerInput_SetKeyboardOurs), nullptr, nullptr, re::Mode::Original);

// 0x50BC80: the mouse device (likewise)
void __fastcall PlayerInput_SetMouseOurs(uint8_t* self, re::Edx, void* device) {
    At<void*>(self, 8) = device;
    Com<long>(device, 11, re::DIDataFormat(dfMouse).get());
    Com<long>(At<void*>(self, 8), 13, ms_hWnd, uint32_t(re::PlayWindow() ? 6 : 5));   // (likewise: the cursor not trapped)
    Com<long>(At<void*>(self, 8), 7);
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::SetMouse_50BC80", 0x0050BC80, RE_BODY(&PlayerInput_SetMouseOurs), nullptr, nullptr, re::Mode::Original);

// 0x50BDD0: a controller code to its controller, index and kind (0/1 an axis's
// negative/positive half, 2..5 a hat up/right/down/left... as the game orders them,
// 6 a button; -1 none). (The next controller's codes start at its axes and buttons
// counted alone, not after the ones before: the game's slip)
int32_t __fastcall PlayerInput_MapCodeOurs(uint8_t* self, re::Edx, int code, int32_t* device, int32_t* index) {
    int32_t base = kFirstControllerCode;
    for (int32_t i = 0; i < At<int32_t>(self, 0xFC); i++) {
        uint8_t* entry = Controller(self, i);
        int32_t axes = At<int32_t>(entry, 0);
        int32_t buttons = At<int32_t>(entry, 4);
        int32_t hats = At<int32_t>(entry, 8);
        if (code < buttons + (axes + hats * 2) * 2 + base) {
            *device = i;
            int32_t r = code - base;
            if (r < axes * 2) {
                bool even = r % 2 == 0;
                if (!even) {
                    r -= 1;
                }
                *index = r / 2;
                return even ? 1 : 0;
            }
            if (r < (axes + hats * 2) * 2) {
                int32_t s = r - axes * 2;
                int32_t m = s % 4;
                *index = (s - m) / 4;
                switch (m) {
                case 0:
                    return 3;
                case 1:
                    return 2;
                case 2:
                    return 4;
                case 3:
                    return 5;
                default:
                    return -1;
                }
            }
            *index = r - (axes + hats * 2) * 2;
            return 6;
        }
        base = buttons + axes * 2;
    }
    return -1;
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::MapCode_50BDD0", 0x0050BDD0, RE_BODY(&PlayerInput_MapCodeOurs), nullptr, nullptr, re::Mode::Original);

// 0x50BF10: the keys back to the defaults (and saved) when one names a controller
// input that isn't there
void __fastcall PlayerInput_CheckKeysOurs(uint8_t* self, re::Edx) {
    for (int i = 0; i < 25; i++) {
        int32_t key = Keys()[i];
        if (key >= kFirstControllerCode) {
            int32_t device, index;
            if (PlayerInput_MapCode(self, key, &device, &index) == -1) {
                ButtonMap_SetDefaultKeys(buttonMap);
                ButtonMap_Save(buttonMap);
                return;
            }
        }
    }
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::CheckKeys_50BF10", 0x0050BF10, RE_BODY(&PlayerInput_CheckKeysOurs), nullptr, nullptr, re::Mode::Original);

// 0x50BF80: init (no devices, no XInput user taken)
void __fastcall PlayerInput_InitOurs(uint8_t* self, re::Edx) {
    At<float>(self, 0x108) = 0.0f;
    At<float>(self, 0x10C) = 0.0f;
    At<float>(self, 0x110) = 0.0f;
    At<int32_t>(self, 0x138) = -1;
    At<int32_t>(self, 0x13C) = -1;
    At<float>(self, 0x114) = 0.0f;
    At<uint8_t>(self, 0) = 0;
    At<void*>(self, 4) = nullptr;
    At<void*>(self, 8) = nullptr;
    At<int32_t>(self, 0xFC) = 0;
    At<int32_t>(self, 0x100) = 0;
    At<int32_t>(self, 0x104) = 0;
    At<uint32_t>(self, 0x140) = *reinterpret_cast<uint32_t*>(0x00C1A9B4);   // CTimer::m_snTimeInMilliseconds
    At<uint8_t>(self, 0x8B4) = 0;
    At<uint8_t>(self, 0x8B5) = 0;
    At<uint8_t>(self, 0x8B6) = 1;
    At<uint8_t>(self, 0x8BA) = 0;
    At<int16_t>(self, 0x8BC) = 0;
    At<uint8_t>(self, 0x8BE) = 0;
    At<uint32_t>(self, 0x8AC) = 0;
    At<uint32_t>(self, 0x8B0) = 0;
    At<uint32_t>(xinputUserTaken, 0) = 0;
    for (int i = 0; i < 5; i++) {
        uint8_t* entry = Controller(self, i);
        At<uint8_t>(entry, 0x28) = 0;
        At<int32_t>(entry, 0) = 0;
        At<int32_t>(entry, 4) = 0;
        At<void*>(entry, 0xC) = nullptr;
    }
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::Init_50BF80", 0x0050BF80, RE_BODY(&PlayerInput_InitOurs), nullptr, nullptr, re::Mode::Original);

// 0x50C030: a code's value in a state: keys and mouse buttons 1 or 0, the wheel as
// is, the mouse's movement scaled by the sensitivity, a controller's axis half (/1000,
// signed), a hat direction (by the angle's high byte) or a button's byte
float __fastcall PlayerInput_GetValueOurs(uint8_t* self, re::Edx, int code, uint8_t* state) {
    switch (code) {
    case 0x100:
    case 0x101:
    case 0x102:
    case 0x103:
        return state[0xBC + (code - 0x100)] ? 1.0f : 0.0f;
    case 0x104:
        return float(At<int32_t>(state, 0xB8));
    case 0x105:
        return float(-At<int32_t>(state, 0xB8));
    case 0x106:
        return float(double(-At<int32_t>(state, 0xB4)) * 0.0078125 * mouseSensitivity);
    case 0x107:
        return float(double(At<int32_t>(state, 0xB4)) * 0.0078125 * mouseSensitivity);
    case 0x108:
        return float(double(-At<int32_t>(state, 0xB0)) * 0.0078125 * mouseSensitivity);
    case 0x109:
        return float(double(At<int32_t>(state, 0xB0)) * 0.0078125 * mouseSensitivity);
    default:
        break;
    }
    if (code < 0x100) {
        return state[code + 0xC0] ? 1.0f : 0.0f;
    }
    int32_t device = 0;
    int32_t index = 0;
    int32_t kind = PlayerInput_MapCode(self, code, &device, &index);
    uint8_t* entry = Controller(self, device);
    uint8_t* controller = state + 0x1C0 + device * 100;
    uint8_t hat = 0;
    if (kind >= 2 && kind <= 5) {
        hat = controller[(index * 2 + At<int32_t>(entry, 0)) * 4 + 1];
    }
    switch (kind) {
    case 0:
    case 1: {
        float sign = kind == 1 ? 1.0f : -1.0f;
        return float(double(At<int32_t>(controller, index * 4)) * sign * 0.0010000000474974513);
    }
    case 2:
        return (hat >= 0x7B && hat <= 0x8C) || hat <= 0x11 ? 1.0f : 0.0f;
    case 3:
        return uint8_t(hat - 0x34) <= 0x23 ? 1.0f : 0.0f;
    case 4:
        return uint8_t(hat - 0x57) <= 0x24 ? 1.0f : 0.0f;
    case 5:
        return uint8_t(hat - 0x11) <= 0x23 ? 1.0f : 0.0f;
    case 6:
        return float(controller[index + (At<int32_t>(entry, 8) * 2 + At<int32_t>(entry, 0)) * 4]);
    default:
        return 0.0f;
    }
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::GetValue_50C030", 0x0050C030, RE_BODY(&PlayerInput_GetValueOurs), nullptr, nullptr, re::Mode::Original);

// 0x50C3C0: a code's value set in a state (GetValue's reverse; hats aren't set)
void __fastcall PlayerInput_SetValueOurs(uint8_t* self, re::Edx, int code, float value, uint8_t* state) {
    double scale = double(mouseSensitivity) * 0.0078125;
    switch (code) {
    case 0x100:
    case 0x101:
    case 0x102:
    case 0x103:
        state[0xBC + (code - 0x100)] = 0.0f < value ? 1 : 0;
        return;
    case 0x104:
        At<int32_t>(state, 0xB8) = int32_t(value);
        return;
    case 0x105:
        At<int32_t>(state, 0xB8) = int32_t(-value);
        return;
    case 0x106:
        At<int32_t>(state, 0xB4) = int32_t(double(-value) / scale);
        return;
    case 0x107:
        At<int32_t>(state, 0xB4) = int32_t(double(value) / scale);
        return;
    case 0x108:
        At<int32_t>(state, 0xB0) = int32_t(double(-value) / scale);
        return;
    case 0x109:
        At<int32_t>(state, 0xB0) = int32_t(double(value) / scale);
        return;
    default:
        break;
    }
    if (code < 0x100) {
        state[code + 0xC0] = 0.0f < value ? 1 : 0;
        return;
    }
    int32_t device = 0;
    int32_t index = 0;
    int32_t kind = PlayerInput_MapCode(self, code, &device, &index);
    uint8_t* entry = Controller(self, device);
    if (kind < 0) {
        return;
    }
    if (kind <= 1) {
        float sign = kind == 1 ? 1.0f : -1.0f;
        At<int32_t>(state, 0x1C0 + (device * 25 + index) * 4) = int32_t(double(value) / (double(sign) * 0.0010000000474974513));
    } else if (kind == 6) {
        state[index + (device * 25 + 0x70 + At<int32_t>(entry, 8) * 2 + At<int32_t>(entry, 0)) * 4] = uint8_t(int32_t(value));
    }
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::SetValue_50C3C0", 0x0050C3C0, RE_BODY(&PlayerInput_SetValueOurs), nullptr, nullptr, re::Mode::Original);

// 0x50C6A0: the first code let go this frame (XInput: the lowest button bit
// released; otherwise on last frame and off now, or an axis or button code from
// 0x106 that changed by more than 0.1), skipping unbindable keys; -1 none
int32_t __fastcall PlayerInput_FindReleasedOurs(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0)) {
        uint32_t released = At<uint32_t>(self, 0x8AC);
        for (int i = 0; i < 16; i++) {
            if ((1u << i) & released) {
                return int32_t(1u << i);
            }
        }
        return -1;
    }
    int32_t count = kFirstControllerCode;
    for (int32_t i = At<int32_t>(self, 0xFC), j = 0; i > 0; i--, j++) {
        uint8_t* entry = Controller(self, j);
        count += At<int32_t>(entry, 4) + (At<int32_t>(entry, 0) + At<int32_t>(entry, 8) * 2) * 2;
    }
    for (int32_t code = 0; code < count; code++) {
        if (IsKeyUnbindable(code)) {
            continue;
        }
        float last = PlayerInput_GetValue(self, code, self + kLastState);
        float now = PlayerInput_GetValue(self, code, self + kState);
        if (last > 0.0f && (now < 0.0f || now == 0.0f)) {
            return code;
        }
        if (code >= 0x106 && last > 0.0f && now > 0.0f) {
            float change = std::fabs(float(now - last));
            if (change > 0.10000000149011612) {
                return code;
            }
        }
    }
    return -1;
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::FindReleased_50C6A0", 0x0050C6A0, RE_BODY(&PlayerInput_FindReleasedOurs), nullptr, nullptr, re::Mode::Original);

// whether a code went on (off: `released`) this frame
bool Changed(uint8_t* self, int code, bool released) {
    float now = PlayerInput_GetValue(self, code, self + kState);
    if (released ? (now > 0.0f || std::isnan(now)) : !(now > 0.0f)) {
        return false;
    }
    float last = PlayerInput_GetValue(self, code, self + kLastState);
    return released ? last > 0.0f : (last < 0.0f || last == 0.0f);
}

// a control (0..15, and 24 without XInput) went on or off this frame
uint8_t ControlChanged(uint8_t* self, int control, bool released) {
    if (At<uint8_t>(self, 0)) {
        if (uint32_t(control) > 15) {
            return 0;
        }
        return (At<uint32_t>(self, released ? 0x8AC : 0x8B0) & uint32_t(buttonMasks[control])) != 0 ? 1 : 0;
    }
    if (uint32_t(control) > 24 || (control >= 16 && control <= 23)) {
        return 0;
    }
    return Changed(self, Keys()[control], released) ? 1 : 0;
}

// 0x50C920: a control pressed this frame
uint8_t __fastcall PlayerInput_JustPressedOurs(uint8_t* self, re::Edx, int control) {
    return ControlChanged(self, control, false);
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::JustPressed_50C920", 0x0050C920, RE_BODY(&PlayerInput_JustPressedOurs), nullptr, nullptr, re::Mode::Original);

// a code went on or off this frame (never with XInput, nor for one of the keys in
// use while this is the active pad)
uint8_t CodeChanged(uint8_t* self, int code, bool released) {
    if (At<uint8_t>(self, 0)) {
        return 0;
    }
    if (GetActivePad() == At<int32_t>(self, 0x138)) {
        for (int i = 0; i < 25; i++) {
            if (Keys()[i] == code) {
                return 0;
            }
        }
    }
    return Changed(self, code, released) ? 1 : 0;
}

// 0x50CA30: a code pressed this frame
uint8_t __fastcall PlayerInput_CodeJustPressedOurs(uint8_t* self, re::Edx, int code) {
    return CodeChanged(self, code, false);
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::CodeJustPressed_50CA30", 0x0050CA30, RE_BODY(&PlayerInput_CodeJustPressedOurs), nullptr, nullptr, re::Mode::Original);

// 0x50CAD0: a code let go this frame
uint8_t __fastcall PlayerInput_CodeJustReleasedOurs(uint8_t* self, re::Edx, int code) {
    return CodeChanged(self, code, true);
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::CodeJustReleased_50CAD0", 0x0050CAD0, RE_BODY(&PlayerInput_CodeJustReleasedOurs), nullptr, nullptr, re::Mode::Original);

// 0x50CB70: a control let go this frame
uint8_t __fastcall PlayerInput_JustReleasedOurs(uint8_t* self, re::Edx, int control) {
    return ControlChanged(self, control, true);
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::JustReleased_50CB70", 0x0050CB70, RE_BODY(&PlayerInput_JustReleasedOurs), nullptr, nullptr, re::Mode::Original);

// 0x50CC80: the controls' values moved from one key set to the other (the saved set's
// to the current one's with `toSaved` 0)
void __fastcall PlayerInput_MoveKeyValuesOurs(uint8_t* self, re::Edx, int toSaved) {
    uint8_t* state = self + kState;
    float values[25];
    const int32_t* from = uint8_t(toSaved) ? keysA : keysB;
    const int32_t* to = uint8_t(toSaved) ? keysB : keysA;
    for (int i = 0; i < 25; i++) {
        values[i] = PlayerInput_GetValue(self, from[i], state);
    }
    for (int i = 0; i < 25; i++) {
        PlayerInput_SetValue(self, to[i], values[i], state);
    }
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::MoveKeyValues_50CC80", 0x0050CC80, RE_BODY(&PlayerInput_MoveKeyValuesOurs), nullptr, nullptr, re::Mode::Original);

// a stick's dead zone (0.4 of its travel) taken out, and beyond full travel brought back
void StickDeadZone(float& x, float& y) {
    float m = float(double(x) * x + double(y) * y);
    float s;
    if (m > 1.0f) {
        s = RecipSqrt(1.0f, m);
    } else if (0.16f > m) {
        x = x * 0.0f;
        y = y * 0.0f;
        return;
    } else {
        float r = float(std::sqrt(double(std::fabs(m))));
        s = float((double(r) - 0.4000000059604645) / (double(r) * 0.5999999940395355));
    }
    x = s * x;
    y = s * y;
}

// keyboard movement eased (0x50CD30): a change after 200 ms starts a new ease from
// last frame's value, a change back within 200 ms swaps the ends; `now` moves on a
// millisecond when one starts
float EaseKeys(uint8_t* self, uint32_t& now, size_t startOffset, size_t fromOffset, size_t toOffset, float last,
               float value, float raw) {
    if (now - At<uint32_t>(self, startOffset) >= 200) {
        float change = std::fabs(float(last - value));
        if (!(change <= 1e-05f)) {
            At<uint32_t>(self, startOffset) = now;
            At<float>(self, fromOffset) = last;
            now += 1;
            At<float>(self, toOffset) = value;
        }
    }
    if (now - At<uint32_t>(self, startOffset) >= 200) {
        return value;
    }
    float back = std::fabs(float(At<float>(self, fromOffset) - value));
    if (back <= 1e-05f) {
        At<float>(self, fromOffset) = At<float>(self, toOffset);
        At<uint32_t>(self, startOffset) = now - 200;
        At<float>(self, toOffset) = value;
    }
    double t = (double(now) - double(At<uint32_t>(self, startOffset))) / 200.0;
    float s = 0.0f;
    switch (keyboardEasing) {
    case 1:
        s = float(std::pow(double(float(t)), 0.3333333432674408));
        break;
    case 2:
        s = float(std::sqrt(double(std::fabs(float(t)))));
        break;
    case 3:
        s = float(t);
        break;
    case 4: {
        float f = float(t);
        s = f * f;
        break;
    }
    case 5:
        s = float(std::pow(double(float(t)), 3.0));
        break;
    default:
        break;
    }
    float eased = float((double(At<float>(self, toOffset)) - At<float>(self, fromOffset)) * s + At<float>(self, fromOffset));
    float moved = std::fabs(float(At<float>(self, toOffset) - raw));
    if (!(moved <= 1e-05f)) {
        At<uint32_t>(self, startOffset) = now;
        At<float>(self, fromOffset) = eased;
        At<float>(self, toOffset) = raw;
    }
    return eased;
}

// 0x50CD30: the frame's update; whether a device was read
uint8_t __fastcall PlayerInput_UpdateOurs(uint8_t* self, re::Edx) {
    uint8_t read = 0;
    uint8_t any = 0;
    uint8_t result;
    std::memcpy(self + kLastState, self + kState, kStateSize);
    bool active;
    if (At<uint8_t>(self, 0)) {
        uint32_t hr = XInputGetState_(At<uint32_t>(self, 0x13C), self + 0x1E4);
        if (At<uint8_t>(self, 0x1EA) >= 0x1E) {
            At<uint16_t>(self, 0x1E8) |= 0x400;
        }
        if (At<uint8_t>(self, 0x1EB) >= 0x1E) {
            At<uint16_t>(self, 0x1E8) |= 0x800;
        }
        uint32_t buttons = At<uint16_t>(self, 0x1E8);
        uint32_t last = At<uint16_t>(self, 0x59C);
        uint32_t changed = buttons ^ last;
        At<uint32_t>(self, 0x8AC) = changed & last;
        At<uint32_t>(self, 0x8B0) = changed & buttons;
        if (hr == 0) {
            result = 1;
            constexpr double k = 3.0517578125e-05;
            float rx = float(double(At<int16_t>(self, 0x1F0)) * k);
            float lx = float(double(At<int16_t>(self, 0x1EC)) * k);
            float ly = float(double(-int32_t(At<int16_t>(self, 0x1EE))) * k);
            float ry = float(k * double(-int32_t(At<int16_t>(self, 0x1F2))));
            At<float>(self, 0x1DC) = -rx;
            At<float>(self, 0x1E0) = -ry;
            At<float>(self, 0x1D4) = -lx;
            At<float>(self, 0x1D8) = -ly;
            StickDeadZone(lx, ly);
            StickDeadZone(rx, ry);
            At<float>(self, 0x18C) = -rx;
            At<float>(self, 0x190) = -ry;
            At<float>(self, 0x184) = -lx;
            At<float>(self, 0x188) = -ly;
        } else {
            result = read;
        }
        active = buttons != 0;
        for (int i = 0; !active && i < 20; i++) {
            active = 0.0f != At<float>(self, kState + i * 4);
        }
    } else {
        if (At<void*>(self, 4)) {
            uint8_t* keys = self + 0x204;
            if (ReadDevice(&At<void*>(self, 4), 0x100, keys) == 0) {
                read = 1;
                for (int i = 0; i < 0x100; i++) {
                    if (keys[i]) {
                        any = 1;
                        break;
                    }
                }
            }
        }
        if (At<void*>(self, 8)) {
            int32_t* mouse = &At<int32_t>(self, 0x1F4);
            if (ReadDevice(&At<void*>(self, 8), 0x10, mouse) == 0) {
                read = 1;
                if (mouse[0] != 0) {
                    mouse[0] -= 1;
                }
                // (never true)
                if (0.0f > std::fabs(float(mouse[0]))) {
                    mouse[0] = 0;
                }
                if (std::fabs(float(mouse[1])) < 0.0f) {
                    mouse[1] = 0;
                }
                if (0.0f > std::fabs(float(mouse[2]))) {
                    mouse[2] = 0;
                }
                if (!any && (mouse[0] || mouse[1] || mouse[2] || At<uint8_t>(self, 0x200) || At<uint8_t>(self, 0x201) ||
                             At<uint8_t>(self, 0x202) || At<uint8_t>(self, 0x203))) {
                    any = 1;
                }
            }
        }
        for (int32_t d = 0; d < At<int32_t>(self, 0xFC); d++) {
            uint8_t* entry = Controller(self, d);
            uint8_t* state = self + 0x304 + d * 100;
            if (!At<void*>(entry, 0xC)) {
                continue;
            }
            if (ReadDevice(&At<void*>(entry, 0xC), At<uint32_t>(entry, 0x1C), state) != 0) {
                continue;
            }
            read = 1;
            for (int32_t i = 0; i < At<int32_t>(entry, 0); i++) {
                // (the axes read 2 bytes apart: the game's slip)
                int32_t v;
                std::memcpy(&v, state + i * 2, 4);
                if (double(At<int32_t>(entry, 0x2C)) > double(std::fabs(float(v)))) {
                    v = 0;
                    std::memcpy(state + i * 2, &v, 4);
                }
                if (!any && v != 0) {
                    any = 1;
                }
            }
            if (!any) {
                const uint8_t* buttons = state + At<int32_t>(entry, 0);
                for (int32_t i = 0; i < At<int32_t>(entry, 4); i++) {
                    if (buttons[i]) {
                        any = 1;
                        break;
                    }
                }
            }
        }
        uint8_t* state = self + kState;
        float a = PlayerInput_GetValue(self, Keys()[20], state);
        float x1 = float(double(a) - PlayerInput_GetValue(self, Keys()[16], state));
        a = PlayerInput_GetValue(self, Keys()[17], state);
        float y1 = float(double(a) - PlayerInput_GetValue(self, Keys()[21], state));
        a = PlayerInput_GetValue(self, Keys()[22], state);
        float x2 = float(double(a) - PlayerInput_GetValue(self, Keys()[18], state));
        a = PlayerInput_GetValue(self, Keys()[19], state);
        float y2 = float(double(a) - PlayerInput_GetValue(self, Keys()[23], state));
        At<float>(self, 0x1DC) = -x2;
        At<float>(self, 0x1E0) = y2;
        bool normalise = true;
        At<float>(self, 0x1D4) = -x1;
        At<float>(self, 0x1D8) = y1;
        uint8_t* player = playerPed;
        if (player) {
            // on a vehicle: the look-back keys drive (when both are keyboard keys)
            if (At<uint8_t*>(player, 0x1554) && PlayerControlsFree() && Keys()[17] < kFirstControllerCode &&
                Keys()[21] < kFirstControllerCode) {
                float v = PlayerInput_GetValue(self, Keys()[17], state);
                PlayerInput_SetValue(self, Keys()[7], v, state);
                v = PlayerInput_GetValue(self, Keys()[21], state);
                PlayerInput_SetValue(self, Keys()[6], v, state);
                // (the vehicle's +0x2A4 points at its data, whose first int is the kind: one
                // dereference, `cmp dword ptr [edx], 60h` at 0x50D4AB)
                if (*At<re::Ptr32<int32_t>>(At<uint8_t*>(playerPed, 0x1554), 0x2A4) == 0x60) {
                    normalise = false;
                }
            }
            player = playerPed;
            uint8_t* vehicle = player ? At<uint8_t*>(player, 0x1554) : nullptr;
            // in a vehicle that says so, unarmed (or with model 300): keys 10 and 12 swapped
            if (vehicle && VCall<uint8_t>(vehicle, 4) &&
                (!Ped_HasWeapon(playerPed) || At<int16_t>(At<uint8_t*>(playerPed, 0x1D0), 0x10E) == 300) &&
                PlayerControlsFree()) {
                float v10 = PlayerInput_GetValue(self, Keys()[10], state);
                float v12 = PlayerInput_GetValue(self, Keys()[12], state);
                PlayerInput_SetValue(self, Keys()[12], v10, state);
                PlayerInput_SetValue(self, Keys()[10], v12, state);
            }
        }
        if (keyboardEasing != 0) {
            uint32_t now = GetTimeMs();
            if (Keys()[16] < 0x100 || Keys()[20] < 0x100) {
                x1 = EaseKeys(self, now, 0x100, 0x108, 0x10C, -At<float>(self, 0x588), x1, -At<float>(self, 0x1D4));
            }
            if (Keys()[17] < 0x100 || Keys()[21] < 0x100) {
                y1 = EaseKeys(self, now, 0x104, 0x110, 0x114, At<float>(self, 0x58C), y1, At<float>(self, 0x1D8));
            }
        }
        if (normalise) {
            float m = float(double(x1) * x1 + double(y1) * y1);
            if (m > 1.0f) {
                float r = float(std::sqrt(double(std::fabs(m))));
                float inv = r == 0.0f ? FLT_MAX : float(1.0 / r);
                x1 = inv * x1;
                y1 = inv * y1;
            }
        }
        if (clampRightStick) {
            float m = float(double(x2) * x2 + double(y2) * y2);
            float most = mouseSensitivity;
            if (most < m) {
                float k = RecipSqrt(most, m);
                x2 = k * x2;
                y2 = k * y2;
            }
        }
        At<float>(self, 0x18C) = -x2;
        At<float>(self, 0x190) = y2;
        At<float>(self, 0x184) = -x1;
        At<float>(self, 0x188) = y1;
        result = read;
        active = any != 0;
    }
    if (active) {
        At<uint32_t>(self, 0x140) = GetTimeMs();
        bool record = false;
        if (At<uint8_t>(self, 0)) {
            record = At<uint32_t>(self, 0x8B0) != 0;
        } else {
            for (int i = 0; i < 16; i++) {
                if (PlayerInput_JustPressed(self, i)) {
                    record = true;
                    break;
                }
            }
        }
        if (record) {
            uint32_t bits = 0;
            for (int i = 0; i < 16; i++) {
                if (!(PlayerInput_GetControl(self, i) == 0.0f)) {
                    bits |= 1u << i;
                }
            }
            At<uint16_t>(self, 0x8C0 + At<int32_t>(self, 0x8FC) * 2) = uint16_t(bits);
            if (At<int32_t>(self, 0x8FC) < 0x1D) {
                At<int32_t>(self, 0x8FC) += 1;
            } else {
                At<int32_t>(self, 0x8FC) = 0;
            }
        }
        mouseEvent_(1, 1, 0, 0, 0);   // (MOUSEEVENTF_MOVE: keeps the screen saver off)
    }
    if (At<uint8_t>(self, 0x8BA)) {
        int64_t spent = int64_t(double(rumbleRate) * timeStep * 1000.0);
        int32_t left = int32_t(At<int16_t>(self, 0x8BC)) - int32_t(spent);
        float f = float(left);
        if (f < 0.0f) {
            f = 0.0f;
        }
        int16_t countdown = int16_t(int32_t(f));
        At<int16_t>(self, 0x8BC) = countdown;
        if (countdown == 0) {
            PlayerInput_UpdateRumble(self);
        }
    } else {
        PlayerInput_UpdateRumble(self);
    }
    At<uint8_t>(self, 0x8B4) = 0;
    return result;
}
PLAYERINPUT_REVERSED("PlayerUserInputHandler::Update_50CD30", 0x0050CD30, RE_BODY(&PlayerInput_UpdateOurs), nullptr, nullptr, re::Mode::Original);

// 0x50DE30: UserInputManager's init: all input on, the six handlers made, DirectInput
// created and its game controllers enumerated (0x7383D0 per device)
void __fastcall UserInputManager_InitOurs(UserInputManager* self, re::Edx) {
    for (int i = 0; i < 4; i++) {
        self->enabled[i] = 1;
    }
    for (int i = 0; i < 6; i++) {
        self->handlers[i] = MakeInputHandler(i);
    }
    void* directInput = nullptr;   // (the system writes a native pointer: then into the game's 4-byte field)
    DirectInput8Create_(ms_hInstance, 0x800, iidDirectInput8, &directInput, nullptr);
    self->directInput = directInput;
    Input_738470();
    if (self->directInput) {
        // IDirectInput8::EnumDevices(DI8DEVCLASS_GAMECTRL, ..., DIEDFL_ATTACHEDONLY)
        Com<long>(self->directInput, 4, uint32_t(4), enumDevicesCallback, static_cast<void*>(nullptr), uint32_t(1));
    }
}
PLAYERINPUT_REVERSED("UserInputManager::Init_50DE30", 0x0050DE30, RE_BODY(&UserInputManager_InitOurs), nullptr, nullptr, re::Mode::Original);

} // namespace
