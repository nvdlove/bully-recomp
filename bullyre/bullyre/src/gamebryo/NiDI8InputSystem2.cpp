// NiDI8InputSystem's slots the first pass missed (src/gamebryo/NiInput.h), 0x8D1290..0x8D2CF0:
// DirectInput created and let go, the mouse and keyboard opened, every device acquired or
// configured through DirectInput's dialog, the device descriptions walked and found, and each
// device's controls enumerated into its description (NiInputDevice::ControlDesc).
//
// The system's vtable (0x9A1B78): 3 OpenDevice, 4 CloseDevice, 5 OpenDeviceFor, 6 GetGamePad,
// 7 OpenMouse, 8 OpenKeyboard, 9 UpdateAllDevices, 10 UpdateActionMap, 12 SetActionMap, 13..16
// the action data, 17 ReleaseActionData, 18 AcquireAllDevices, 21 CheckEnumerationStatus,
// 22 ConfigureDevices, 23 Initialize, 24 Shutdown, 25 AddDescription, 26 RemoveDescription,
// 27 the next port, 28/29 GetFirst/NextDeviceDesc, 30 GetDeviceDesc, 31 EnumerateControls.

#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInput.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::input {
namespace {

using ni::sys::At;
using ni::sys::Import;

GAME_VAR(void*, gDInput8Module, 0x020E9700);     // dinput8.dll, when the library loaded it
GAME_VAR(void*, gInstance, 0x020E96F8);          // the application's HINSTANCE (create parameters)
GAME_VAR(void*, gWindow, 0x020E96FC);            // its window (create parameters)
GAME_VAR(uint32_t, NiDX9Renderer_ClassIndex, 0x020E6D20);            // (IsKindOf's bits)
GAME_VAR(uint32_t, NiDX9OnscreenBufferData_ClassIndex, 0x020E7238);
GAME_VAR(uint32_t, NiD3D10Renderer_ClassIndex, 0x020E8F38);

constexpr uintptr_t kFreeLibrary = 0x008FC0C0;
constexpr uintptr_t kDirectInput8Create = 0x008FC02C;   // (the exe's thunk 0x8D9A80 jumps through it)
constexpr uintptr_t IID_IDirectInput8A = 0x009A2004;
constexpr uintptr_t kEnumObjectsCallback = 0x008D2230;
// the object kinds DirectInput reports (GUID_XAxis..GUID_Slider, GUID_Button, GUID_Key, GUID_POV)
constexpr uintptr_t GUID_XAxis = 0x009A2014;
constexpr uintptr_t GUID_YAxis = 0x009A2024;
constexpr uintptr_t GUID_ZAxis = 0x009A2034;
constexpr uintptr_t GUID_RxAxis = 0x009A2044;
constexpr uintptr_t GUID_RyAxis = 0x009A2054;
constexpr uintptr_t GUID_RzAxis = 0x009A2064;
constexpr uintptr_t GUID_Slider = 0x009A2074;
constexpr uintptr_t GUID_Button = 0x009A2084;
constexpr uintptr_t GUID_Key = 0x009A2094;
constexpr uintptr_t GUID_POV = 0x009A20A4;

GAME_FN(int, __cdecl, IsEqualGUID, 0x0074D200, const void* a, const void* b);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* value);
GAME_FN(int, __thiscall, NiDX9Renderer_Recreate, 0x00880E60, void* renderer, uint32_t width, uint32_t height,
        uint32_t flags, void* window, void* focus, uint32_t adapter, uint32_t deviceDesc, uint32_t frameBuffer,
        uint32_t depthStencil, uint32_t presentInterval, uint32_t swapEffect);
GAME_FN(uint32_t, __thiscall, InputSystem_Initialize, 0x008D3810, NiInputSystem* self, uint8_t* params);
GAME_FN(uint32_t, __thiscall, System_ConfigureDevice, 0x008D1370, NiDI8InputSystem* self, void* device);
GAME_FN(void, __thiscall, System_ReleaseDevices, 0x008D3130, NiDI8InputSystem* self);
// (named the other way round by the first pass: 0x8D31E0 maps the keyboard's actions, 0x8D3300
//  the mouse's)
GAME_FN(uint32_t, __thiscall, System_MapKeyboardActions, 0x008D31E0, NiInputSystem* self);
GAME_FN(uint32_t, __thiscall, System_MapMouseActions, 0x008D3300, NiInputSystem* self);
GAME_FN(NiInputDevice*, __thiscall, DI8Mouse_Construct, 0x008D4350, void* self,
        NiInputDeviceDescription* description, void* device, uint32_t cooperation);
GAME_FN(NiInputDevice*, __thiscall, DI8Keyboard_Construct, 0x008D5390, void* self,
        NiInputDeviceDescription* description, void* device, uint32_t cooperation);
// this file's, called where modes apply
GAME_FN(uint8_t, __thiscall, System_CreateDirectInput, 0x008D12B0, NiDI8InputSystem* self);
GAME_FN(void, __thiscall, ControlDesc_SetName, 0x008D1400, ControlDesc* self, const char* name);
GAME_FN(ControlDesc*, __thiscall, ControlDesc_Construct, 0x008D1E90, void* self, uint32_t index, const char* name,
        uint32_t type, uint32_t flags);
GAME_FN(int32_t, __thiscall, Description_AddControl, 0x008D3FA0, NiInputDeviceDescription* self,
        ControlDesc* control);

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
    kDICreateDevice = 3,             // IDirectInput8A
    kDIConfigureDevices = 10,
    kDIDevEnumObjects = 4,           // IDirectInputDevice8A
};

// NiObject::IsKindOf through the RTTI's class bits
bool IsKindOf(const void* object, uint32_t index) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (At<uint32_t>(rtti, 0xC + (index >> 5) * 4) & (1u << (index & 31))) != 0;
}

// ---- NiDI8InputSystem

// DirectInput 8 created over the instance and window the create parameters gave (either
// missing: 0x80000002; failing: 0x80000003)
uint8_t __fastcall CreateDirectInput(NiDI8InputSystem* self, re::Edx) {
    void* instance = gInstance;
    if (instance == nullptr || gWindow == nullptr) {
        self->lastError = kErrInvalidParams;
        return 0;
    }
    if (Import<int32_t>(kDirectInput8Create, instance, 0x800u, IID_IDirectInput8A, &self->directInput,
                        static_cast<void*>(nullptr)) < 0) {
        self->lastError = 0x80000003;
        return 0;
    }
    return 1;
}

// vf23 Initialize: the base's, then DirectInput when that went well
uint32_t __fastcall Initialize(NiDI8InputSystem* self, re::Edx, uint8_t* params) {
    if (InputSystem_Initialize(self, params) == kOk) {
        System_CreateDirectInput(self);
    }
    return self->lastError;
}

// vf24 Shutdown: the devices let go, DirectInput released, its library unloaded
uint32_t __fastcall Shutdown(NiDI8InputSystem* self, re::Edx) {
    System_ReleaseDevices(self);
    if (self->directInput != nullptr) {
        Com(self->directInput, kComRelease);
    }
    self->directInput = nullptr;
    self->field84 = 0;
    if (gDInput8Module != nullptr) {
        Import<int>(kFreeLibrary, gDInput8Module);
        gDInput8Module = nullptr;
    }
    return kOk;
}

// vf21 CheckEnumerationStatus: not enumerated yet: 0x80000030; else the flag cleared (so the
// next EnumerateDevices runs again)
uint32_t __fastcall CheckEnumerationStatus(NiDI8InputSystem* self, re::Edx) {
    if (self->field28 == 0) {
        return 0x80000030;
    }
    self->field28 = 0;
    return kOk;
}

// vf18 AcquireAllDevices: each open device acquired (vf6; its result kept as its last
// error); any another application has: 0x80000006
uint32_t __fastcall AcquireAllDevices(NiDI8InputSystem* self, re::Edx) {
    bool lost = false;
    self->lastError = kOk;
    if (self->keyboardDevice != nullptr) {
        uint32_t result = Virtual<uint32_t>(self->keyboardDevice, 6);
        if (result == kErrAcquireFailed) {
            lost = true;
        }
        self->keyboardDevice->lastError = result;
    }
    if (self->mouseDevice != nullptr) {
        uint32_t result = Virtual<uint32_t>(self->mouseDevice, 6);
        if (result == kErrAcquireFailed) {
            lost = true;
        }
        self->mouseDevice->lastError = result;
    }
    for (NiInputGamePad* pad : self->gamePads) {
        if (pad != nullptr) {
            uint32_t result = Virtual<uint32_t>(pad, 6);
            if (result == kErrAcquireFailed) {
                lost = true;
            }
            pad->lastError = result;
        }
    }
    if (lost) {
        self->lastError = kErrAcquireFailed;
    }
    return self->lastError;
}

// vf7 OpenMouse / vf8 OpenKeyboard: the one open, else the first description of its kind
// (none: 0x80000005) made a DirectInput device (failing: 0x80000007), the library's device
// over it (the cooperative level from the create parameters), held, and the action map's
// actions mapped to it. (The new device's own error isn't looked at: the system's is.)
NiInputDevice* OpenDevice(NiDI8InputSystem* self, re::Ptr32<NiInputDevice>& slot, uint32_t type, uint32_t size,
                          NiInputDevice*(__thiscall* construct)(void*, NiInputDeviceDescription*, void*, uint32_t),
                          uint32_t cooperation, uint32_t(__thiscall* mapActions)(NiInputSystem*)) {
    if (self->directInput == nullptr) {
        self->lastError = kErrInvalidParams;
        return nullptr;
    }
    if (slot != nullptr) {
        return slot;
    }
    auto* description = Virtual<NiInputDeviceDescription*>(self, 28);
    while (true) {
        if (description == nullptr) {
            self->lastError = 0x80000005;
            return nullptr;
        }
        if (description->type == type) {
            break;
        }
        description = Virtual<NiInputDeviceDescription*>(self, 29);
    }
    void* device = nullptr;
    if (Com(self->directInput, kDICreateDevice, &static_cast<NiDI8InputDeviceDescription*>(description)->guidInstance,
            &device, 0u) < 0) {
        self->lastError = 0x80000007;
        return nullptr;
    }
    void* p = NiNew(size);
    NiInputDevice* created = p != nullptr ? construct(p, description, device, cooperation) : nullptr;
    if (created != nullptr && self->lastError == kOk) {
        created->status = 0;
        NiPointer_Assign(&slot, created);
        mapActions(self);
        return slot;
    }
    self->lastError = 0x80000007;
    if (created != nullptr) {
        Virtual(created, 0, 1u);
    }
    return nullptr;
}

NiInputDevice* __fastcall OpenMouse(NiDI8InputSystem* self, re::Edx) {
    return OpenDevice(self, self->mouseDevice, 2, sizeof(NiInputDI8Mouse), DI8Mouse_Construct, self->createParams[1],
                      System_MapMouseActions);
}

NiInputDevice* __fastcall OpenKeyboard(NiDI8InputSystem* self, re::Edx) {
    return OpenDevice(self, self->keyboardDevice, 1, sizeof(NiInputDI8Keyboard), DI8Keyboard_Construct,
                      self->createParams[0], System_MapKeyboardActions);
}

// vf22 ConfigureDevices: DirectInput's device configuration dialog over the game pad action
// format (DICD_EDIT), in the window of the create parameters. A full-screen DX9 renderer is
// recreated windowed at 640x480 around it and back at its size after; a D3D10 renderer is
// left as it is. When it went well, the pads are given the action format again and every
// device is acquired, then all updated (vf9).
uint32_t __fastcall ConfigureDevices(NiDI8InputSystem* self, re::Edx) {
    struct {   // DICONFIGUREDEVICESPARAMSA
        uint32_t size;
        uint32_t users;
        const char* userNames;
        uint32_t formats;
        void* formatList;
        void* window;
        uint32_t colors[9];   // DICOLORSET
        void* target;
    } params;
#if !defined(_WIN64)
    static_assert(sizeof(params) == 0x40);   // (DirectInput's own struct: the 64-bit one differs)
#endif
    crt::Memset(&params, 0, sizeof(params));
    params.size = 0x40;
    params.users = 1;
    params.userNames = nullptr;
    params.formats = 1;
    params.formatList = self->actionFormat;
    params.window = gWindow;
    // (with no renderer, or one neither DX9 nor D3D10, the game tests a result it never set:
    //  uninitialised stack. Taken as a failure here: nothing configured)
    int32_t hr = -1;
    void* renderer = self->renderer;
    if (renderer != nullptr) {
        if (IsKindOf(renderer, NiDX9Renderer_ClassIndex)) {
            // the default target group's first buffer's data: windowed (+0x38) or not
            void* buffer = Virtual<void*>(At<void*>(renderer, 0x680), 27, 0u);
            void* data = At<void*>(buffer, 0x10);
            void* onscreen = data != nullptr && IsKindOf(data, NiDX9OnscreenBufferData_ClassIndex) ? data : nullptr;
            bool fullScreen = At<uint32_t>(onscreen, 0x38) == 0;
            uint32_t width = 0;
            uint32_t height = 0;
            if (fullScreen) {
                void* group = Virtual<void*>(renderer, 29);
                width = Virtual<uint32_t>(group, 18, 0u);
                height = Virtual<uint32_t>(group, 19, 0u);
                NiDX9Renderer_Recreate(renderer, 640, 480, 0, gWindow, nullptr, 0, 1, 0, 0, 1, 0);
            }
            params.target = nullptr;
            hr = Com(self->directInput, kDIConfigureDevices, static_cast<void*>(nullptr), &params, 1u, self);
            if (fullScreen) {
                NiDX9Renderer_Recreate(renderer, width, height, 4, gWindow, nullptr, 0, 1, 0, 0, 1, 0);
            }
        } else if (IsKindOf(renderer, NiD3D10Renderer_ClassIndex)) {
            params.target = nullptr;
            hr = Com(self->directInput, kDIConfigureDevices, static_cast<void*>(nullptr), &params, 1u, self);
        }
    }
    if (hr < 0) {
        return kOk;
    }
    for (NiInputGamePad* pad : self->gamePads) {
        if (pad != nullptr) {
            System_ConfigureDevice(self, static_cast<NiInputDI8GamePad*>(pad)->device);
        }
    }
    for (NiInputGamePad* pad : self->gamePads) {
        if (pad != nullptr) {
            Virtual<uint32_t>(pad, 6);
        }
    }
    if (self->keyboardDevice != nullptr) {
        Virtual<uint32_t>(self->keyboardDevice, 6);
    }
    if (self->mouseDevice != nullptr) {
        Virtual<uint32_t>(self->mouseDevice, 6);
    }
    Virtual<uint32_t>(self, 9);
    return kOk;
}

// vf31 EnumerateControls: a device's objects enumerated into its description (0x8D2230)
uint32_t __fastcall EnumerateControls(NiDI8InputSystem* self, re::Edx, NiInputDeviceDescription* description,
                                      void* device) {
    if (self->directInput == nullptr || device == nullptr) {
        return kErrInvalidParams;
    }
    Com(device, kDIDevEnumObjects, kEnumObjectsCallback, description, 0u);   // DIDFT_ALL
    return kOk;
}

// ---- NiInputSystem: the device descriptions

// vf28/vf29: walking the descriptions (the system's iterator)
NiInputDeviceDescription* __fastcall GetFirstDeviceDesc(NiInputSystem* self, re::Edx) {
    auto& iter = reinterpret_cast<re::Ptr32<NiTListNode<NiInputDeviceDescription*>>&>(self->devicesIter);
    NiTListNode<NiInputDeviceDescription*>* node = self->devices.head;
    iter = node;
    if (node != nullptr) {
        iter = node->next;
        return node->element;
    }
    return nullptr;
}

NiInputDeviceDescription* __fastcall GetNextDeviceDesc(NiInputSystem* self, re::Edx) {
    auto& iter = reinterpret_cast<re::Ptr32<NiTListNode<NiInputDeviceDescription*>>&>(self->devicesIter);
    if (iter != nullptr) {
        NiTListNode<NiInputDeviceDescription*>* node = iter;
        iter = node->next;
        return node->element;
    }
    return nullptr;
}

// vf30 GetDeviceDesc: the description at a port and slot
NiInputDeviceDescription* __fastcall GetDeviceDesc(NiInputSystem* self, re::Edx, uint32_t port, uint32_t slot) {
    auto* description = Virtual<NiInputDeviceDescription*>(self, 28);
    while (description != nullptr && (description->port != port || description->slot != slot)) {
        description = Virtual<NiInputDeviceDescription*>(self, 29);
    }
    return description;
}

// ---- NiInputDevice::ControlDesc

// its name copied into the library's heap (empty or none: none)
void __fastcall ControlDescSetName(ControlDesc* self, re::Edx, const char* name) {
    NiFree(self->name);
    self->name = nullptr;
    if (name == nullptr || *name == 0) {
        self->name = nullptr;
        return;
    }
    uint32_t size = uint32_t(std::strlen(name)) + 1;
    auto* copy = static_cast<char*>(NiMalloc(size));
    self->name = copy;
    crt::StrcpyS(copy, size, name);
}

ControlDesc* __fastcall ControlDescConstruct(ControlDesc* self, re::Edx, uint32_t index, const char* name, uint32_t type,
                                             uint32_t flags) {
    self->index = index;
    self->type = type;
    self->name = nullptr;
    self->flags = flags;
    ControlDesc_SetName(self, name);
    return self;
}

// a control added to a description: one of the same name, kind and flags there already: -1;
// else counted by its kind and listed: how many there are now
int32_t __fastcall AddControl(NiInputDeviceDescription* self, re::Edx, ControlDesc* control) {
    for (NiTListNode<void*>* node = self->controls.head; node != nullptr;) {
        auto* other = static_cast<ControlDesc*>(node->element);
        node = node->next;
        if (other != nullptr && crt::Strcmp(control->name, other->name) == 0 && control->type == other->type &&
            control->flags == other->flags) {
            return -1;
        }
    }
    if (control->type == 0) {
        self->buttonCount++;
    }
    if (control->type == 1) {
        self->axisCount++;
    }
    if (control->type == 2) {
        self->povCount++;
    }
    // NiTPointerList's AddTail, inlined: a node from its allocator (slot 1)
    auto* node = Virtual<NiTListNode<void*>*>(&self->controls, 1);
    node->element = control;
    node->next = nullptr;
    node->prev = self->controls.tail;
    if (self->controls.tail != nullptr) {
        self->controls.tail->next = node;
        self->controls.count++;
        self->controls.tail = node;
    } else {
        self->controls.count++;
        self->controls.head = node;
        self->controls.tail = node;
    }
    return int32_t(self->controls.count);
}

// DIEnumDeviceObjectsCallback (the description as the context): axes, buttons and points of
// view described (index: the controls listed so far; DirectInput's object type kept) and
// added (freed again when one like it is there); keys only counted as buttons
int __stdcall EnumObjectsCallback(const uint8_t* object, NiInputDeviceDescription* description) {
    if (description == nullptr) {
        return 0;
    }
    const uint8_t* guid = object + 4;                          // guidType
    const char* name = reinterpret_cast<const char*>(object + 0x20);   // tszName
    ControlDesc* control;
    if (IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_XAxis)) != 0 ||
        IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_YAxis)) != 0 ||
        IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_ZAxis)) != 0 ||
        IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_RxAxis)) != 0 ||
        IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_RyAxis)) != 0 ||
        IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_RzAxis)) != 0 ||
        IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_Slider)) != 0) {
        // (constructed inline; a failed allocation is written through)
        control = static_cast<ControlDesc*>(NiNew(sizeof(ControlDesc)));
        if (control != nullptr) {
            control->index = description->controls.count;
            control->name = nullptr;
            control->type = 1;
            control->flags = 0;
            ControlDesc_SetName(control, name);
        }
        control->flags |= 0x12;
    } else if (IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_Button)) != 0) {
        void* p = NiNew(sizeof(ControlDesc));
        control = p != nullptr ? ControlDesc_Construct(p, description->controls.count, name, 0, 0) : nullptr;
        control->flags = (control->flags & ~2u) | 0x11;
    } else if (IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_Key)) != 0) {
        description->buttonCount++;
        return 1;
    } else if (IsEqualGUID(guid, reinterpret_cast<const void*>(GUID_POV)) != 0) {
        void* p = NiNew(sizeof(ControlDesc));
        control = p != nullptr ? ControlDesc_Construct(p, description->controls.count, name, 2, 0) : nullptr;
        control->flags = (control->flags & ~2u) | 0x11;
    } else {
        return 1;
    }
    control->diType = At<uint32_t>(object, 0x18);   // dwType
    if (Description_AddControl(description, control) == -1) {
        NiFree(control->name);
        NiDelete(control, sizeof(ControlDesc));
    }
    return 1;
}

} // namespace
} // namespace ni::input

// ---- registration ---------------------------------------------------------------------

#define NIDI8INPUT2_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

RE_UNHOOKED("NiDI8InputSystem::UpdateAllDevices (thunk)", 0x008D1290);   // jmp 0x8D3010 (vf9)
RE_UNHOOKED("NiDI8InputSystem::UpdateActionMap (thunk)", 0x008D12A0);    // jmp 0x8D30A0 (vf10)
NIDI8INPUT2_REVERSED("NiDI8InputSystem::CreateDirectInput", 0x008D12B0, &ni::input::CreateDirectInput);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::Initialize", 0x008D1300, &ni::input::Initialize);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::Shutdown", 0x008D1320, &ni::input::Shutdown);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::CheckEnumerationStatus", 0x008D13E0, &ni::input::CheckEnumerationStatus);
NIDI8INPUT2_REVERSED("NiInputDevice::ControlDesc::SetName", 0x008D1400, &ni::input::ControlDescSetName);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::AcquireAllDevices", 0x008D15C0, &ni::input::AcquireAllDevices);
NIDI8INPUT2_REVERSED("NiInputDevice::ControlDesc::ControlDesc", 0x008D1E90, &ni::input::ControlDescConstruct);
NIDI8INPUT2_REVERSED("NiInputSystem::GetFirstDeviceDesc", 0x008D1EC0, &ni::input::GetFirstDeviceDesc);
NIDI8INPUT2_REVERSED("NiInputSystem::GetNextDeviceDesc", 0x008D1EE0, &ni::input::GetNextDeviceDesc);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::OpenMouse", 0x008D2030, &ni::input::OpenMouse);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::OpenKeyboard", 0x008D2130, &ni::input::OpenKeyboard);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::EnumObjectsCallback", 0x008D2230, &ni::input::EnumObjectsCallback);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::ConfigureDevices", 0x008D2420, &ni::input::ConfigureDevices);
NIDI8INPUT2_REVERSED("NiDI8InputSystem::EnumerateControls", 0x008D2740, &ni::input::EnumerateControls);
NIDI8INPUT2_REVERSED("NiInputSystem::GetDeviceDesc", 0x008D2CF0, &ni::input::GetDeviceDesc);
NIDI8INPUT2_REVERSED("NiInputDevice::Description::AddControl", 0x008D3FA0, &ni::input::AddControl);
