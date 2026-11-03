// The pads: the four PlayerUserInputHandlers (0x20CECF0, 0x914 bytes each), the pad the
// player uses (0x20CECE8) and the second one (0xBC7490), the DirectInput joystick
// enumeration (with Microsoft's WMI test for XInput pads) and the game's pad queries by
// player (0 = the main pad, 1 = the second, else the handler itself). 0x737FC0..0x738AD0.

#include <cmath>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

constexpr int kHandlerSize = 0x914;

inline uint8_t* const piHandlers = reinterpret_cast<uint8_t*>(0x020CECF0);   // 4
inline uint8_t* const piHandlersEnd = reinterpret_cast<uint8_t*>(0x020D1140);
inline void* const piInputManager = reinterpret_cast<void*>(0x00A45A30);
inline void* const piButtonMap = reinterpret_cast<void*>(0x00A14810);
inline const void* const piGuidSysKeyboard = reinterpret_cast<const void*>(0x009A20C4);
inline const void* const piGuidSysMouse = reinterpret_cast<const void*>(0x009A20B4);
inline const void* const piClsidWbemLocator = reinterpret_cast<const void*>(0x0094D774);
inline const void* const piIidWbemLocator = reinterpret_cast<const void*>(0x0094D784);
inline void* const piEventVtable = reinterpret_cast<void*>(0x0094D6DC);
GAME_VAR(int32_t, piMainPad, 0x020CECE8);
GAME_VAR(int32_t, piSecondPad, 0x00BC7490);
GAME_VAR(int32_t, piJoystickCount, 0x020D1140);
GAME_VAR(uint8_t, piPreferJoystick, 0x00A14924);
GAME_VAR(uint8_t, piRumbleOn, 0x00C18700);
GAME_VAR(uint8_t*, piHud, 0x009E4F4C);
GAME_VAR(uint8_t, piPaused1, 0x00C1A99A);
GAME_VAR(uint8_t, piPaused2, 0x00C1A999);
GAME_VAR(uint8_t, piPaused3, 0x00C1A998);

constexpr uintptr_t kCoInitialize = 0x008FC338;
constexpr uintptr_t kCoCreateInstance = 0x008FC32C;
constexpr uintptr_t kCoSetProxyBlanket = 0x008FC330;
constexpr uintptr_t kCoUninitialize = 0x008FC33C;
constexpr uintptr_t kSysAllocString = 0x008FC1F8;
constexpr uintptr_t kSysFreeString = 0x008FC1FC;

GAME_FN(void, __cdecl, Pi_Free, 0x005EEAB0, void* p);
GAME_FN(int, __cdecl, Pi_IsXInputDevice, 0x007380C0, const void* productGuid);
GAME_FN(void, __cdecl, Pi_UpdateHandler, 0x00738570, uint8_t* handler);
GAME_FN(float, __thiscall, Pi_ApplyDeadzone, 0x00737FE0, uint8_t* handler, int axis, float deadzone);
GAME_FN(float, __thiscall, Pi_DominantAxis, 0x007389C0, uint8_t* handler, int control);
inline wchar_t* __cdecl Pi_Wcsstr(const wchar_t* s, const wchar_t* find) { return crt::Wcsstr(s, find); }
template <typename... A>
inline int __cdecl Pi_Swscanf(const wchar_t* s, const wchar_t* format, A... rest) { return crt::Swscanf(s, format, rest...); }
GAME_FN(void*, __thiscall, Pi_CreateDevice, 0x0050DEE0, void* manager, const void* guid);
GAME_FN(uint8_t, __thiscall, Pi_Dispatch, 0x0050DF20, void* manager, void* event);
GAME_FN(void, __thiscall, Pi_AddController, 0x0050B9A0, uint8_t* handler, void* device, int xinput);
GAME_FN(void, __thiscall, Pi_SetKeyboard, 0x0050BC40, uint8_t* handler, void* device);
GAME_FN(void, __thiscall, Pi_SetMouse, 0x0050BC80, uint8_t* handler, void* device);
GAME_FN(void, __thiscall, Pi_ButtonMapLoad, 0x00454A50, void* map);
GAME_FN(void, __thiscall, Pi_SetPlayer, 0x0050B590, uint8_t* handler, int player);
GAME_FN(int32_t, __thiscall, Pi_FindReleased, 0x0050C6A0, uint8_t* handler);
GAME_FN(int32_t, __thiscall, Pi_MapCode, 0x0050BDD0, uint8_t* handler, int code, int32_t* device, int32_t* index);
GAME_FN(uint8_t, __thiscall, Pi_CanSwapLook, 0x0050B5B0, uint8_t* handler, int force);
GAME_FN(void, __thiscall, Pi_SwapKeys, 0x0050C7D0, uint8_t* handler, int a, int b);
GAME_FN(void, __thiscall, Pi_CheckKeys, 0x0050BF10, uint8_t* handler);
GAME_FN(uint8_t, __thiscall, Pi_HandlerUpdate, 0x0050CD30, uint8_t* handler);
GAME_FN(uint8_t, __thiscall, Pi_JustPressed, 0x0050C920, uint8_t* handler, int control);
GAME_FN(uint8_t, __thiscall, Pi_JustReleased, 0x0050CB70, uint8_t* handler, int control);
GAME_FN(void, __thiscall, Pi_Clear, 0x0050B630, uint8_t* handler);
GAME_FN(void, __thiscall, Pi_MoveKeyValues, 0x0050CC80, uint8_t* handler, int toSaved);
GAME_FN(void, __thiscall, Pi_Clear140, 0x0050B6F0, uint8_t* handler);
GAME_FN(uint8_t, __thiscall, Pi_HasRumble, 0x0050B700, uint8_t* handler);
GAME_FN(void, __thiscall, Pi_Rumble, 0x0050B710, uint8_t* handler, int duration, int strength, int rightOnly);
GAME_FN(void, __thiscall, Pi_UpdateRumble, 0x0050B900, uint8_t* handler);
GAME_FN(float, __thiscall, Pi_GetControl, 0x0050C880, uint8_t* handler, int control);

uint8_t* Handler(int i) {
    return piHandlers + i * kHandlerSize;
}

// the player's pad: 0 the main one, 1 the second one, else the handler
int Resolve(int pad) {
    if (pad == 0) {
        return piMainPad;
    }
    if (pad == 1) {
        return piSecondPad;
    }
    return pad;
}

// a COM method (stdcall, this first) by vtable index
template <typename R, typename... A>
R Com(void* obj, int index, A... args) {
    return re::ComCall<R>(obj, index, args...);
}

void Release(void*& obj) {   // (a system COM object: IUnknown::Release)
    if (obj != nullptr) {
        Com<unsigned long>(obj, 2);
        obj = nullptr;
    }
}

// a VARIANT (only its BSTR read)
struct PiVariant {
    uint16_t vt;
    uint16_t reserved[3];
    re::Ptr32<wchar_t> bstrVal;
    uint32_t pad;
};
static_assert(sizeof(PiVariant) == 0x10);

// the input event UserInputManager::Dispatch hands to the listeners (vtable 0x94D6DC)
struct PiEvent {
    re::Ptr32<void> vtable;    // +0x0
    int32_t player;  // +0x4
    float value;     // +0x8
    int32_t control; // +0xC
};

void DispatchControl(uint8_t* handler, float value, int control) {
    PiEvent e;
    e.vtable = piEventVtable;
    e.player = *reinterpret_cast<int32_t*>(handler + 0x138);
    e.value = value;
    e.control = control;
    Pi_Dispatch(piInputManager, &e);
    e.vtable = piEventVtable;   // (its destructor)
}

} // namespace

namespace padinput {

// 0x737FC0: the input event's (scalar deleting) destructor
PiEvent* __fastcall Event_Destruct(PiEvent* self, re::Edx, uint32_t flags) {
    self->vtable = piEventVtable;
    if (flags & 1) {
        Pi_Free(self);
    }
    return self;
}

// 0x737FE0: an axis past the dead zone, rescaled (|axis| over 1 gives 1, under the dead
// zone 0); the dead zone 0 gives the axis as it is
float __fastcall ApplyDeadzone(uint8_t* self, re::Edx, int axis, float deadzone) {
    float a = *reinterpret_cast<float*>(self + 0x194 + axis * 4);
    if (deadzone == 0.0f) {
        return a;
    }
    bool negative;
    if (a < 0.0f) {
        a = -a;
        negative = true;
    } else {
        negative = false;
    }
    float r;
    if (1.0f < a) {
        r = 1.0f;
    } else if (a < deadzone) {
        r = 0.0f;
    } else {
        r = (a - deadzone) / (a * (1.0f - deadzone));
    }
    return negative ? -r : r;
}

// 0x738070
int32_t __cdecl GetMainPad() {
    return piMainPad;
}

// 0x738080
int32_t __cdecl GetSecondPad() {
    return piSecondPad;
}

// 0x738090
void __cdecl SetPlayerIndices() {
    int i = 0;
    for (uint8_t* h = piHandlers; h < piHandlersEnd; h += kHandlerSize) {
        Pi_SetPlayer(h, i);
        i++;
    }
}

// 0x7380C0: whether the DirectInput product is an XInput pad (its PnP device id has
// "IG_" and the same VID/PID), through WMI (Microsoft's sample)
int __cdecl IsXInputDevice(const uint32_t* productGuid) {
    // (WMI's objects: the system writes their pointers, native ones on x64)
    void* devices[20] = {};
    void* enumDevices = nullptr;
    void* locator = nullptr;
    void* services = nullptr;
    wchar_t* deviceId = nullptr;
    uint8_t isXInput = 0;
    long hr = vc::Import<long>(kCoInitialize, static_cast<void*>(nullptr));
    uint8_t cleanupCom = hr >= 0;
    hr = vc::Import<long>(kCoCreateInstance, piClsidWbemLocator, static_cast<void*>(nullptr), 1u, piIidWbemLocator,
                          &locator);
    if (hr >= 0 && locator != nullptr) {
        wchar_t* ns = vc::Import<wchar_t*>(kSysAllocString, L"\\\\.\\root\\cimv2");
        if (ns != nullptr) {
            wchar_t* className = vc::Import<wchar_t*>(kSysAllocString, L"Win32_PNPEntity");
            if (className != nullptr) {
                deviceId = vc::Import<wchar_t*>(kSysAllocString, L"DeviceID");
            }
            if (className != nullptr && deviceId != nullptr) {
                // IWbemLocator::ConnectServer
                hr = Com<long>(locator, 3, ns, static_cast<void*>(nullptr), static_cast<void*>(nullptr),
                               static_cast<void*>(nullptr), 0L, static_cast<void*>(nullptr), static_cast<void*>(nullptr),
                               &services);
                if (hr >= 0 && services != nullptr) {
                    vc::Import<long>(kCoSetProxyBlanket, services, 10u, 0u, static_cast<void*>(nullptr), 3u, 3u,
                                     static_cast<void*>(nullptr), 0u);
                    // IWbemServices::CreateInstanceEnum
                    hr = Com<long>(services, 18, className, 0L, static_cast<void*>(nullptr), &enumDevices);
                    if (hr >= 0 && enumDevices != nullptr) {
                        uint32_t returned = 0;
                        // IEnumWbemClassObject::Next
                        while (Com<long>(enumDevices, 4, 10000L, 20u, devices, &returned) >= 0 && returned != 0) {
                            for (uint32_t i = 0; i < returned; i++) {
                                PiVariant var;
                                // IWbemClassObject::Get
                                hr = Com<long>(devices[i], 4, static_cast<const wchar_t*>(deviceId), 0L, &var,
                                               static_cast<void*>(nullptr), static_cast<void*>(nullptr));
                                if (hr >= 0 && var.vt == 8 && var.bstrVal != nullptr &&
                                    Pi_Wcsstr(var.bstrVal, L"IG_") != nullptr) {
                                    uint32_t vid = 0;
                                    uint32_t pid = 0;
                                    wchar_t* s = Pi_Wcsstr(var.bstrVal, L"VID_");
                                    if (s != nullptr && Pi_Swscanf(s, L"VID_%4X", &vid) != 1) {
                                        vid = 0;
                                    }
                                    s = Pi_Wcsstr(var.bstrVal, L"PID_");
                                    if (s != nullptr && Pi_Swscanf(s, L"PID_%4X", &pid) != 1) {
                                        pid = 0;
                                    }
                                    uint32_t vidPid = ((pid & 0xFFFF) << 16) | (vid & 0xFFFF);
                                    if (vidPid == productGuid[0]) {
                                        isXInput = 1;
                                        goto done;
                                    }
                                }
                                Release(devices[i]);
                            }
                        }
                    }
                }
            }
        done:
            vc::Import<void>(kSysFreeString, ns);
            if (deviceId != nullptr) {
                vc::Import<void>(kSysFreeString, deviceId);
            }
            if (className != nullptr) {
                vc::Import<void>(kSysFreeString, className);
            }
        }
    }
    for (void*& d : devices) {
        Release(d);
    }
    Release(enumDevices);
    Release(locator);
    Release(services);
    if (cleanupCom) {
        vc::Import<void>(kCoUninitialize);
    }
    return isXInput;
}

// 0x7383D0: IDirectInput8::EnumDevices' callback for the joysticks (XInput pads go to
// handlers 1..3, the others share handler 0)
int __stdcall EnumJoystick(const uint8_t* instance, void*) {
    int pad = piJoystickCount + 1;
    if (pad >= 4) {
        return 0;   // DIENUM_STOP
    }
    uint8_t xinput = Pi_IsXInputDevice(instance + 0x14) != 0;   // guidProduct
    if (!xinput) {
        pad = 0;
    }
    void* device = Pi_CreateDevice(piInputManager, instance + 4);   // guidInstance
    Pi_AddController(Handler(pad), device, xinput);
    if (piMainPad == 0 && pad != 0 && piPreferJoystick != 0) {
        if (piSecondPad == pad) {
            piSecondPad = 0;
        }
        piMainPad = pad;
    }
    if (xinput) {
        piJoystickCount = pad;
    }
    return 1;   // DIENUM_CONTINUE
}

// 0x738470
void __cdecl InitKeyboardAndMouse() {
    Pi_SetKeyboard(piHandlers, Pi_CreateDevice(piInputManager, piGuidSysKeyboard));
    Pi_SetMouse(piHandlers, Pi_CreateDevice(piInputManager, piGuidSysMouse));
    Pi_ButtonMapLoad(piButtonMap);
}

// 0x7384B0
int32_t __cdecl FindReleased(int pad) {
    return Pi_FindReleased(Handler(pad));
}

// 0x7384D0
int32_t __cdecl MapCode(int pad, int code, int32_t* device, int32_t* index) {
    return Pi_MapCode(Handler(pad), code, device, index);
}

// 0x738500
uint8_t __cdecl IsXInput(int pad) {
    return *Handler(pad);
}

// 0x738520
uint8_t __cdecl CanSwapLook(int pad, int force) {
    return Pi_CanSwapLook(Handler(pad), force);
}

// 0x738540
void __cdecl SwapKeys(int pad, int a, int b) {
    Pi_SwapKeys(Handler(pad), a, b);
}

// 0x738560
void __cdecl CheckKeys() {
    Pi_CheckKeys(piHandlers);
}

// 0x738570: a handler's frame (the controls' presses and releases to the listeners)
void __cdecl UpdateHandler(uint8_t* handler) {
    if (!Pi_HandlerUpdate(handler)) {
        return;
    }
    for (int i = 0; i < 20; i++) {
        if (Pi_JustPressed(handler, i)) {
            DispatchControl(handler, 1.0f, i);
        } else if (Pi_JustReleased(handler, i)) {
            DispatchControl(handler, 0.0f, i);
        }
    }
}

// 0x738610
void __cdecl UpdateAll() {
    for (uint8_t* h = piHandlers; h < piHandlersEnd; h += kHandlerSize) {
        Pi_UpdateHandler(h);
    }
}

// 0x738630
void __cdecl ClearAll() {
    for (uint8_t* h = piHandlers; h < piHandlersEnd; h += kHandlerSize) {
        Pi_Clear(h);
    }
}

// 0x738650
void __cdecl MoveKeyValues(int toSaved) {
    Pi_MoveKeyValues(piHandlers, toSaved);
}

// 0x738660
void __cdecl Clear140All() {
    for (uint8_t* h = piHandlers; h < piHandlersEnd; h += kHandlerSize) {
        Pi_Clear140(h);
    }
}

// 0x738680: rumble the player's pad (when rumble is on and the game isn't paused)
void __cdecl Rumble(int pad, int duration, int strength, int rightOnly) {
    uint8_t* h = Handler(Resolve(pad));
    if (Pi_HasRumble(h) && piRumbleOn != 0 && piHud[0x85D] == 0 && piPaused1 == 0 && piPaused2 == 0 &&
        piPaused3 == 0) {
        Pi_Rumble(h, duration, strength, rightOnly);
    }
}

// 0x738700
void __cdecl StopRumble(int pad) {
    Handler(Resolve(pad))[0x8BF] = 1;
}

// 0x738740
void __cdecl UpdateRumble(int pad) {
    uint8_t* h = Handler(Resolve(pad));
    if (Pi_HasRumble(h)) {
        Pi_UpdateRumble(h);
    }
}

// 0x738780
void __cdecl UpdateRumbleAll() {
    for (int i = 0; i < 4; i++) {
        uint8_t* h = Handler(Resolve(i));
        if (Pi_HasRumble(h)) {
            Pi_UpdateRumble(h);
        }
    }
}

// 0x7387D0 (eax is 0 or 1)
int __cdecl Handler0Word8BCPositive() {
    return *reinterpret_cast<int16_t*>(piHandlers + 0x8BC) > 0;
}

// 0x7387E0, 0x738800, 0x738820: a flag of the main pad's handler
template <int Offset>
uint8_t __cdecl MainPadFlag() {
    return Handler(piMainPad)[Offset];
}

// 0x738840
float __cdecl GetControl(int control, int pad) {
    return Pi_GetControl(Handler(Resolve(pad)), control);
}

// 0x738880, 0x7388C0: a key code's press/release (raw: pad is the handler)
template <uintptr_t Query>
uint8_t __cdecl CodeQuery(int code, int pad, uint8_t raw) {
    if (!raw) {
        pad = Resolve(pad);
    }
    return reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, int)>(Query)(Handler(pad), code);
}

// 0x738900, 0x738940: a control's press/release
template <uintptr_t Query>
uint8_t __cdecl ControlQuery(int control, int pad) {
    return reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, int)>(Query)(Handler(Resolve(pad)), control);
}

// 0x738980
float __cdecl Deadzone(int axis, int pad, float deadzone) {
    return Pi_ApplyDeadzone(Handler(Resolve(pad)), axis, deadzone);
}

// 0x7389C0: a stick axis, 0 when the stick's other axis is pushed further
float __fastcall DominantAxis(uint8_t* self, re::Edx, int control) {
    float v = Pi_GetControl(self, control);
    int other;
    switch (control) {
    case 0x10: other = 0x11; break;
    case 0x11: other = 0x10; break;
    case 0x12: other = 0x13; break;
    case 0x13: other = 0x12; break;
    default: return v;
    }
    float o = std::fabs(Pi_GetControl(self, other));
    if (std::fabs(v) < o) {
        return 0.0f;
    }
    return v;
}

// 0x738AD0
float __cdecl GetDominantAxis(int control, int pad) {
    return Pi_DominantAxis(Handler(Resolve(pad)), control);
}

} // namespace padinput

#define PADINPUT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PADINPUT_REVERSED("UserInputEvent::Destruct", 0x00737FC0, &padinput::Event_Destruct);
PADINPUT_REVERSED("Pads::ApplyDeadzone", 0x00737FE0, &padinput::ApplyDeadzone);
PADINPUT_REVERSED("Pads::GetMainPad", 0x00738070, &padinput::GetMainPad);
PADINPUT_REVERSED("Pads::GetSecondPad", 0x00738080, &padinput::GetSecondPad);
PADINPUT_REVERSED("Pads::SetPlayerIndices", 0x00738090, &padinput::SetPlayerIndices);
PADINPUT_REVERSED("Pads::IsXInputDevice", 0x007380C0, &padinput::IsXInputDevice);
PADINPUT_REVERSED("Pads::EnumJoystick", 0x007383D0, &padinput::EnumJoystick);
PADINPUT_REVERSED("Pads::InitKeyboardAndMouse", 0x00738470, &padinput::InitKeyboardAndMouse);
PADINPUT_REVERSED("Pads::FindReleased", 0x007384B0, &padinput::FindReleased);
PADINPUT_REVERSED("Pads::MapCode", 0x007384D0, &padinput::MapCode);
PADINPUT_REVERSED("Pads::IsXInput", 0x00738500, &padinput::IsXInput);
PADINPUT_REVERSED("Pads::CanSwapLook", 0x00738520, &padinput::CanSwapLook);
PADINPUT_REVERSED("Pads::SwapKeys", 0x00738540, &padinput::SwapKeys);
PADINPUT_REVERSED("Pads::CheckKeys", 0x00738560, &padinput::CheckKeys);
PADINPUT_REVERSED("Pads::UpdateHandler", 0x00738570, &padinput::UpdateHandler);
PADINPUT_REVERSED("Pads::UpdateAll", 0x00738610, &padinput::UpdateAll);
PADINPUT_REVERSED("Pads::ClearAll", 0x00738630, &padinput::ClearAll);
PADINPUT_REVERSED("Pads::MoveKeyValues", 0x00738650, &padinput::MoveKeyValues);
PADINPUT_REVERSED("Pads::Clear140All", 0x00738660, &padinput::Clear140All);
PADINPUT_REVERSED("Pads::Rumble", 0x00738680, &padinput::Rumble);
PADINPUT_REVERSED("Pads::StopRumble", 0x00738700, &padinput::StopRumble);
PADINPUT_REVERSED("Pads::UpdateRumble", 0x00738740, &padinput::UpdateRumble);
PADINPUT_REVERSED("Pads::UpdateRumbleAll", 0x00738780, &padinput::UpdateRumbleAll);
PADINPUT_REVERSED("Pads::Handler0Word8BCPositive", 0x007387D0, &padinput::Handler0Word8BCPositive);
PADINPUT_REVERSED("Pads::MainPadFlag8B4", 0x007387E0, &padinput::MainPadFlag<0x8B4>);
PADINPUT_REVERSED("Pads::MainPadFlag8B5", 0x00738800, &padinput::MainPadFlag<0x8B5>);
PADINPUT_REVERSED("Pads::MainPadFlag8B6", 0x00738820, &padinput::MainPadFlag<0x8B6>);
PADINPUT_REVERSED("Pads::GetControl", 0x00738840, &padinput::GetControl);
PADINPUT_REVERSED("Pads::CodeJustPressed", 0x00738880, &padinput::CodeQuery<0x0050CA30>);
PADINPUT_REVERSED("Pads::CodeJustReleased", 0x007388C0, &padinput::CodeQuery<0x0050CAD0>);
PADINPUT_REVERSED("Pads::JustPressed", 0x00738900, &padinput::ControlQuery<0x0050C920>);
PADINPUT_REVERSED("Pads::JustReleased", 0x00738940, &padinput::ControlQuery<0x0050CB70>);
PADINPUT_REVERSED("Pads::Deadzone", 0x00738980, &padinput::Deadzone);
PADINPUT_REVERSED("PlayerUserInputHandler::DominantAxis", 0x007389C0, &padinput::DominantAxis);
PADINPUT_REVERSED("Pads::GetDominantAxis", 0x00738AD0, &padinput::GetDominantAxis);
