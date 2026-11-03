// NiDI8InputSystem (src/gamebryo/NiInput.h), 0x8D11E0..0x8D31E0: NiInput over DirectInput 8:
// creating it from its parameters, the game pad action format and enumerating devices by it,
// the device descriptions, the action data handed to the game, updating every device, and
// mapping the keyboard's actions. Also the action-data pool's blocks and arrays.

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

GAME_VAR(uint8_t, NiDI8InputSystem_RTTI, 0x020E9704);
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(void*, gDInput8Module, 0x020E9700);             // dinput8.dll, when the library loaded it
GAME_VAR(uint32_t, gCreateParam20, 0x020E96F8);
GAME_VAR(uint32_t, gCreateParam24, 0x020E96FC);
GAME_VAR(uint32_t, DI8CreateParams_ClassIndex, 0x020E9740);
GAME_VAR(uint32_t, gLastKeyboardModifiers, 0x020E97D8);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiDI8InputSystem_vtable = 0x009A1B78;
constexpr uintptr_t Description_vtable = 0x009A1CC4;
constexpr uintptr_t DI8Description_vtable = 0x009A1CCC;
constexpr uintptr_t ControlListVtable = 0x0095145C;
constexpr uintptr_t PointerListVtable = 0x00952E10;
constexpr uintptr_t PointerListBaseVtable = 0x0099C748;
constexpr uintptr_t ActionData_vtable = 0x008FF22C;      // NiActionData
constexpr uintptr_t kFreeLibrary = 0x008FC0C0;

GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, PointerList_AddTail, 0x007843D0, void* list, re::Ptr32<NiRefObject>* element);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, void* slot, void* value);
GAME_FN(void, __thiscall, PointerAllocator_FreeNode, 0x00401B30, void* allocator, void* node);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* construct);
GAME_FN(uint32_t, __cdecl, MapSemanticToControl, 0x00828AB0, uint32_t semantic);
GAME_FN(void, __thiscall, InputSystem_Construct, 0x008D35D0, NiInputSystem* self);
GAME_FN(void, __thiscall, InputSystem_Destruct, 0x008D3A10, NiInputSystem* self);
GAME_FN(uint32_t, __thiscall, InputSystem_MapGamePadActions, 0x008D3470, NiInputSystem* self, NiInputGamePad* pad);
GAME_FN(uint32_t, __thiscall, Device_ClearMappedActions, 0x008D3F80, NiInputDevice* self);
GAME_FN(NiAction*, __thiscall, ActionMap_GetAction, 0x008D5500, NiActionMap* self, uint32_t index);
GAME_FN(NiInputGamePad*, __thiscall, DI8GamePad_Construct, 0x008D5B70, void* self,
        NiInputDeviceDescription* description, void* device, int32_t low, int32_t high);
// this file's, called where modes apply
GAME_FN(NiDI8InputSystem*, __thiscall, System_Construct, 0x008D11E0, void* self);
GAME_FN(uint32_t, __thiscall, System_ConfigureDevice, 0x008D1370, NiDI8InputSystem* self, void* device);
GAME_FN(void, __thiscall, Description_SetProductName, 0x008D1460, NiInputDeviceDescription* self, const char* name);
GAME_FN(void, __thiscall, Description_SetInstanceName, 0x008D14C0, NiInputDeviceDescription* self, const char* name);
GAME_FN(void, __thiscall, System_Destruct, 0x008D1530, NiDI8InputSystem* self);
GAME_FN(uint8_t, __thiscall, System_SetActionName, 0x008D1650, NiDI8InputSystem* self, uint32_t index, const char* name);
GAME_FN(uint8_t, __thiscall, System_FillDescription, 0x008D16E0, NiDI8InputSystem* self, const uint8_t* instance,
        NiInputDeviceDescription* description);
GAME_FN(uint8_t, __thiscall, System_BuildActionFormat, 0x008D17C0, NiDI8InputSystem* self);
GAME_FN(void, __thiscall, Description_Destruct, 0x008D2660, NiInputDeviceDescription* self);
GAME_FN(NiInputDeviceDescription*, __thiscall, Description_Construct, 0x008D2780, void* self);
GAME_FN(NiInputDeviceDescription*, __thiscall, DI8Description_Construct, 0x008D2800, void* self);
GAME_FN(uint8_t, __thiscall, System_EnumGamePad, 0x008D2950, NiDI8InputSystem* self, const uint8_t* instance,
        void* device);
GAME_FN(uint32_t, __thiscall, System_EnumerateDevices, 0x008D2AC0, NiDI8InputSystem* self);
GAME_FN(void, __thiscall, System_SyncKeyboardModifiers, 0x008D2D30, NiDI8InputSystem* self);
GAME_FN(void, __thiscall, Block_Destruct, 0x008D2E70, Block* self);
GAME_FN(void, __thiscall, System_ReleaseDevices, 0x008D3130, NiDI8InputSystem* self);
constexpr uintptr_t kEnumDevicesCallback = 0x008D2850;
constexpr uintptr_t kEnumGamePadsCallback = 0x008D2AA0;

template <typename R = void, typename... A>
R Virtual(const void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(const void*, re::Arg<A>...)>(static_cast<void*>(VtableOf(object)[slot]))(object, args...);
}

// a COM (DirectInput) call by slot
template <typename... A>
int32_t Com(void* object, int slot, A... args) {
    return re::ComCall<int32_t>(object, slot, args...);
}
enum : int {
    kComAddRef = 1,
    kComRelease = 2,
    kDICreateDevice = 3,             // IDirectInput8A
    kDIEnumDevices = 4,
    kDIEnumDevicesBySemantics = 9,
    kDIDevAcquire = 7,               // IDirectInputDevice8A
    kDIDevUnacquire = 8,
    kDIDevBuildActionMap = 29,
    kDIDevSetActionMap = 30,
};

void RefObjectDestruct(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
}

// a string copied into the library's heap (empty or none: none)
char* CopyName(const char* name) {
    if (name == nullptr || *name == 0) {
        return nullptr;
    }
    uint32_t size = uint32_t(std::strlen(name)) + 1;
    auto* copy = static_cast<char*>(NiMalloc(size));
    crt::StrcpyS(copy, size, name);
    return copy;
}

// ---- NiDI8InputSystem

// the constructor: no DirectInput, 21 actions with no names
NiDI8InputSystem* __fastcall Construct(NiDI8InputSystem* self, re::Edx) {
    InputSystem_Construct(self);
    self->directInput = nullptr;
    self->actions = nullptr;
    self->field1DC = 0;
    self->vtable = reinterpret_cast<void*>(NiDI8InputSystem_vtable);
    self->actionCount = 21;
    for (re::Ptr32<char>& name : self->actionNames) {
        name = nullptr;
    }
    return self;
}

// a game pad's device given the action format (unacquired, mapped, acquired again; failing:
// 0x8000000C)
uint32_t __fastcall ConfigureDevice(NiDI8InputSystem* self, re::Edx, void* device) {
    if (device == nullptr) {
        self->lastError = kErrInvalidParams;
        return self->lastError;
    }
    Com(device, kDIDevUnacquire);
    if (Com(device, kDIDevBuildActionMap, self->actionFormat, 0u, 0u) >= 0 &&
        Com(device, kDIDevSetActionMap, self->actionFormat, 0u, 0u) >= 0) {
        Com(device, kDIDevAcquire);
        return self->lastError;
    }
    self->lastError = 0x8000000C;
    return self->lastError;
}

void __fastcall SetProductName(NiInputDeviceDescription* self, re::Edx, const char* name) {
    NiFree(self->productName);
    self->productName = nullptr;
    self->productName = CopyName(name);
}

void __fastcall SetInstanceName(NiInputDeviceDescription* self, re::Edx, const char* name) {
    NiFree(self->instanceName);
    self->instanceName = nullptr;
    self->instanceName = CopyName(name);
}

void* __fastcall GetRTTI(NiDI8InputSystem*, re::Edx) {
    return &NiDI8InputSystem_RTTI;
}

// the destructor: the names and actions freed, the devices let go, DirectInput released and
// its library unloaded
void __fastcall Destruct(NiDI8InputSystem* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiDI8InputSystem_vtable);
    for (char* name : self->actionNames) {
        if (name != nullptr) {
            NiFree(name);
        }
    }
    OperatorDelete(self->actions);
    System_ReleaseDevices(self);
    if (self->directInput != nullptr) {
        Com(self->directInput, kComRelease);
    }
    self->directInput = nullptr;
    self->field84 = 0;
    if (gDInput8Module != nullptr) {
        re::PtrCast<int(__stdcall*)(void*)>(*reinterpret_cast<re::Ptr32<void>*>(kFreeLibrary))(gDInput8Module);
        gDInput8Module = nullptr;
    }
    InputSystem_Destruct(self);
}

// an action's name (index up to 20): copied with two more zero bytes after it
uint8_t __fastcall SetActionName(NiDI8InputSystem* self, re::Edx, uint32_t index, const char* name) {
    if (int32_t(index) > 20) {
        return 0;
    }
    if (self->actionNames[index] != nullptr) {
        NiFree(self->actionNames[index]);
        self->actionNames[index] = nullptr;
    }
    if (name != nullptr) {
        uint32_t length = uint32_t(std::strlen(name));
        auto* copy = static_cast<char*>(NiMalloc(length + 3));
        self->actionNames[index] = copy;
        crt::StrcpyS(copy, length + 1, name);
        copy[length + 1] = 0;
        copy[length + 2] = 0;
    }
    return 1;
}

// a description filled from a DIDEVICEINSTANCEA: the next port, the instance GUID, the names,
// the kind (mouse and screen pointer 2, keyboard 1, sticks and pads 3)
uint8_t __fastcall FillDescription(NiDI8InputSystem* self, re::Edx, const uint8_t* instance,
                                   NiInputDeviceDescription* description) {
    if (instance == nullptr || description == nullptr) {
        return 0;
    }
    description->port = Virtual<uint32_t>(self, 27);
    crt::MemcpyS(reinterpret_cast<uint8_t*>(description) + 0x40, 0x10, instance + 4, 0x10);
    Description_SetProductName(description, reinterpret_cast<const char*>(instance + 0x12C));
    Description_SetInstanceName(description, reinterpret_cast<const char*>(instance + 0x28));
    switch (instance[0x24]) {
    case 0x12:   // DI8DEVTYPE_MOUSE
    case 0x1A:   // DI8DEVTYPE_SCREENPOINTER
        description->type = 2;
        break;
    case 0x13:   // DI8DEVTYPE_KEYBOARD
        description->type = 1;
        break;
    case 0x14:   // joystick, game pad, driving, flight, first person, supplemental
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x1C:
        description->type = 3;
        break;
    default:
        break;
    }
    return 1;
}

// the game pad action format: 21 DIACTIONs (each action's name from the create parameters,
// else the library's), then the DIACTIONFORMAT over them ("NiInput Default", genre 0x9000000)
uint8_t __fastcall BuildActionFormat(NiDI8InputSystem* self, re::Edx) {
    struct Action {
        uint32_t appData;
        uint32_t semantic;
        uintptr_t defaultName;
    };
    static const Action kActions[21] = {
        {1, 0xFF014201, 0x009A1CB4},  {0, 0xFF00C201, 0x009A1CA4},  {3, 0xFF004201, 0x009A1C94},
        {2, 0xFF004202, 0x009A1C84},  {5, 0xFF004203, 0x009A1C74},  {4, 0xFF004204, 0x009A1C64},
        {7, 0xFF014202, 0x009A1C54},  {6, 0xFF00C202, 0x009A1C44},  {8, 0x09004601, 0x009A1C28},
        {10, 0xFF004400, 0x009A1C20}, {12, 0xFF004401, 0x009A1C18}, {11, 0xFF004402, 0x009A1C10},
        {9, 0xFF004403, 0x009A1C08},  {13, 0xFF004404, 0x0092BE44}, {14, 0xFF004405, 0x0092BE3C},
        {16, 0xFF004406, 0x0092BE70}, {15, 0xFF004407, 0x0092BE68}, {19, 0xFF004408, 0x0090DA78},
        {20, 0xFF004409, 0x0090DA7C}, {17, 0xFF00440A, 0x0092BE40}, {18, 0xFF00440B, 0x0092BE38},
    };   // ("Stick 1 Vert".."Stick 4 Horz", the D-pad, R DOWN/RIGHT/LEFT/UP, L1, R1, SELECT,
         //  START, A, B, L2, R2)
    OperatorDelete(self->actions);
    uint64_t bytes = uint64_t(self->actionCount) * 0x28;
    self->actions = static_cast<uint8_t*>(OperatorNew(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes)));
    if (self->actions == nullptr) {
        return 0;
    }
    std::memset(self->actions, 0, self->actionCount * 0x28);
    for (int i = 0; i < 21; i++) {
        uint8_t* action = self->actions + i * 0x28;
        At<uint32_t>(action, 0x0) = kActions[i].appData;
        At<uint32_t>(action, 0x4) = kActions[i].semantic;
        const char* name = self->actionNames[kActions[i].appData];
        At<const char*>(action, 0xC) = name != nullptr ? name : reinterpret_cast<const char*>(kActions[i].defaultName);
    }
    uint8_t* format = self->actionFormat;
    std::memset(format, 0, 0x148);
    uint32_t count = self->actionCount;
    At<uint32_t>(format, 0xC) = count;                 // dwNumActions
    At<uint32_t>(format, 0x4) = 0x28;                  // dwActionSize
    At<uint32_t>(format, 0x8) = count * 4;             // dwDataSize
    At<uint32_t>(format, 0x0) = 0x148;                 // dwSize
    At<uint32_t>(format, 0x14) = 0x0CCC7545;           // guidActionMap
    At<uint32_t>(format, 0x18) = 0x406D2FF1;
    At<uint32_t>(format, 0x1C) = 0xE2E84BB1;
    At<uint32_t>(format, 0x20) = 0x67B91798;
    At<uint32_t>(format, 0x28) = count;                // dwBufferSize
    At<uint32_t>(format, 0x24) = 0x09000000;           // dwGenre
    At<uint8_t*>(format, 0x10) = self->actions;        // rgoAction
    At<int32_t>(format, 0x2C) = self->axisLow;         // lAxisMin
    At<int32_t>(format, 0x30) = self->axisHigh;        // lAxisMax
    crt::StrcpyS(reinterpret_cast<char*>(format + 0x44), 0x104, "NiInput Default");   // tszActionMap
    return 1;
}

// the action data the game hasn't taken: how many, the first (taken off the list), and
// walking them
uint32_t __fastcall GetActionDataCount(NiDI8InputSystem* self, re::Edx) {
    return self->actionData.count;
}

ActionData* __fastcall PopActionData(NiDI8InputSystem* self, re::Edx) {
    if (self->actionData.count == 0) {
        return nullptr;
    }
    NiTListNode<ActionData*>* node = self->actionData.head;
    self->actionData.head = node->next;
    if (node->next == nullptr) {
        self->actionData.tail = nullptr;
    } else {
        node->next->prev = nullptr;
    }
    ActionData* data = node->element;
    Virtual(&self->actionData, 2, node);
    self->actionData.count--;
    return data;
}

ActionData* __fastcall GetFirstActionData(NiDI8InputSystem* self, re::Edx) {
    auto& iter = reinterpret_cast<re::Ptr32<NiTListNode<ActionData*>>&>(self->actionDataIter);
    NiTListNode<ActionData*>* node = self->actionData.head;
    iter = node;
    if (node != nullptr) {
        iter = node->next;
        return node->element;
    }
    return nullptr;
}

ActionData* __fastcall GetNextActionData(NiDI8InputSystem* self, re::Edx) {
    auto& iter = reinterpret_cast<re::Ptr32<NiTListNode<ActionData*>>&>(self->actionDataIter);
    if (iter != nullptr) {
        NiTListNode<ActionData*>* node = iter;
        iter = node->next;
        return node->element;
    }
    return nullptr;
}

NiDI8InputSystem* __fastcall DeletingDestructor(NiDI8InputSystem* self, re::Edx, uint32_t flags) {
    System_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiDI8InputSystem));
    }
    return self;
}

// vf6 GetGamePad: an open pad at a port (when one has it, the pad in the slot numbered as the
// port is returned: the game's quirk); a port of 8 or more, the first open pad
NiInputGamePad* __fastcall GetGamePad(NiDI8InputSystem* self, re::Edx, uint32_t port, uint32_t) {
    if (self->directInput == nullptr) {
        self->lastError = kErrInvalidParams;
        return nullptr;
    }
    if (port < 8) {
        for (uint32_t i = 0; i < 8; i++) {
            NiInputGamePad* pad = self->gamePads[i];
            if (pad != nullptr && pad->description->port == port) {
                return self->gamePads[port];
            }
        }
        return nullptr;
    }
    for (NiInputGamePad* pad : self->gamePads) {
        if (pad != nullptr) {
            return pad;
        }
    }
    return nullptr;
}

// ---- NiInputDevice::Description

// the destructor: the names and the control descriptions freed (up to the first empty one)
void __fastcall DescriptionDestruct(NiInputDeviceDescription* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(Description_vtable);
    NiFree(self->instanceName);
    NiFree(self->productName);
    auto& iter = reinterpret_cast<re::Ptr32<NiTListNode<void*>>&>(self->controlsIter);
    NiTListNode<void*>* node = self->controls.head;
    iter = node;
    if (node != nullptr) {
        iter = node->next;
        void* control = node->element;
        while (control != nullptr) {
            NiFree(At<void*>(control, 4));
            NiDelete(control, 0x14);
            if (iter == nullptr) {
                break;
            }
            node = iter;
            iter = node->next;
            control = node->element;
        }
    }
    NiTPointerList_RemoveAll(&self->controls);
    self->controls.vtable = reinterpret_cast<void*>(PointerListVtable);
    NiTPointerList_RemoveAll(&self->controls);
    self->controls.vtable = reinterpret_cast<void*>(PointerListBaseVtable);
    RefObjectDestruct(self);
}

// vf25 AddDescription: listed (a reference passed in, let go); how many there are now
uint32_t __fastcall AddDescription(NiDI8InputSystem* self, re::Edx, NiInputDeviceDescription* description) {
    re::Ptr32<NiRefObject> element = description;
    AddRef(element);
    PointerList_AddTail(&self->devices, &element);
    Release(description);
    return self->devices.count;
}

NiInputDeviceDescription* __fastcall DescriptionConstruct(NiInputDeviceDescription* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
    self->vtable = reinterpret_cast<void*>(Description_vtable);
    self->type = 0;
    self->port = 0;
    self->slot = 0;
    self->productName = nullptr;
    self->instanceName = nullptr;
    self->opened = 0;
    self->controls.count = 0;
    self->controls.head = nullptr;
    self->controls.tail = nullptr;
    self->controls.vtable = reinterpret_cast<void*>(ControlListVtable);
    self->buttonCount = 0;
    self->axisCount = 0;
    self->povCount = 0;
    NiTPointerList_RemoveAll(&self->controls);
    return self;
}

NiInputDeviceDescription* __fastcall DescriptionDeletingDestructor(NiInputDeviceDescription* self, re::Edx,
                                                                  uint32_t flags) {
    Description_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputDeviceDescription));
    }
    return self;
}

NiInputDeviceDescription* __fastcall DI8DescriptionConstruct(NiDI8InputDeviceDescription* self, re::Edx) {
    Description_Construct(self);
    self->vtable = reinterpret_cast<void*>(DI8Description_vtable);
    self->guidInstance[0] = 0;
    self->guidInstance[1] = 0;
    self->guidInstance[2] = 0;
    self->guidInstance[3] = 0;
    return self;
}

NiInputDeviceDescription* __fastcall DI8DescriptionDeletingDestructor(NiDI8InputDeviceDescription* self, re::Edx,
                                                                     uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(DI8Description_vtable);
    Description_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiDI8InputDeviceDescription));
    }
    return self;
}

// ---- enumerating devices

// DIEnumDevicesCallback: each attached device made and described when it's a keyboard, mouse
// or screen pointer (listed); its interface released again either way
int __stdcall EnumDevicesCallback(const uint8_t* instance, NiDI8InputSystem* self) {
    if (self == nullptr || self->directInput == nullptr) {
        return 0;
    }
    void* device = nullptr;
    if (Com(self->directInput, kDICreateDevice, instance + 4, &device, 0u) < 0) {
        return 1;
    }
    void* p = NiNew(sizeof(NiDI8InputDeviceDescription));
    NiInputDeviceDescription* description = p != nullptr ? DI8Description_Construct(p) : nullptr;
    // (the game compares the GUID with the system keyboard's and mouse's and drops the result)
    uint8_t type = instance[0x24];
    if (type == 0x12 || type == 0x13 || type == 0x1A) {
        System_FillDescription(self, instance, description);
        Virtual(self, 31, description, device);
        Virtual<uint32_t>(self, 25, description);
    } else if (description != nullptr) {
        Virtual(description, 0, 1u);
    }
    Com(device, kComRelease);
    return 1;
}

// a game pad found by the action format: into the first free pad slot (none: 0x8000000B),
// configured, described and listed; when it's the newest description, a DI8 game pad made
// over it (the device held), put in the slot, and the action map's actions mapped to it
uint8_t __fastcall EnumGamePad(NiDI8InputSystem* self, re::Edx, const uint8_t* instance, void* device) {
    uint8_t type = instance[0x24];
    if (type == 0x13 || type == 0x12 || type == 0x1A) {
        return 1;
    }
    if (type != 0x18 && type != 0x16 && type != 0x17 && type != 0x15 && type != 0x14 && type != 0x1C) {
        return 1;
    }
    uint32_t slot = 0;
    while (self->gamePads[slot] != nullptr) {
        slot++;
        if (slot >= 8) {
            self->lastError = 0x8000000B;
            return 1;
        }
    }
    System_ConfigureDevice(self, device);
    if (self->lastError != kOk) {
        return 1;
    }
    void* p = NiNew(sizeof(NiDI8InputDeviceDescription));
    NiInputDeviceDescription* description = nullptr;
    if (p != nullptr) {
        description = Description_Construct(p);
        description->vtable = reinterpret_cast<void*>(DI8Description_vtable);
        auto* di8 = static_cast<NiDI8InputDeviceDescription*>(description);
        di8->guidInstance[0] = 0;
        di8->guidInstance[1] = 0;
        di8->guidInstance[2] = 0;
        di8->guidInstance[3] = 0;
    }
    System_FillDescription(self, instance, description);
    Virtual(self, 31, description, device);
    if (Virtual<uint32_t>(self, 25, description) == description->port + 1) {
        void* q = NiNew(sizeof(NiInputDI8GamePad));
        if (q != nullptr) {
            NiInputGamePad* pad = DI8GamePad_Construct(q, description, device, self->axisLow, self->axisHigh);
            if (pad != nullptr) {
                Com(device, kComAddRef);
                pad->index = slot;
                pad->field14 = 1;
                pad->status = 0;
                NiPointer_Assign(&self->gamePads[slot], pad);
                InputSystem_MapGamePadActions(self, pad);
            }
        }
    }
    return 1;
}

// DIEnumDevicesBySemanticsCallback: the game pad handler (the system as the context)
int __stdcall EnumGamePadsCallback(const uint8_t* instance, void* device, uint32_t, uint32_t,
                                   NiDI8InputSystem* self) {
    return System_EnumGamePad(self, instance, device);
}

// the devices enumerated once: game pads by the action format, then every attached device
// (none yet: 0x80000002; done before: 0x80000031; failing: 0x80000003)
uint32_t __fastcall EnumerateDevices(NiDI8InputSystem* self, re::Edx) {
    if (self->directInput == nullptr) {
        self->lastError = kErrInvalidParams;
        return kErrInvalidParams;
    }
    if (self->field28 != 0) {
        return 0x80000031;
    }
    if (System_BuildActionFormat(self) != 0 &&
        Com(self->directInput, kDIEnumDevicesBySemantics, uintptr_t(0), self->actionFormat, kEnumGamePadsCallback,
            self, 0u) >= 0 &&
        Com(self->directInput, kDIEnumDevices, 0u, kEnumDevicesCallback, self, 1u) >= 0) {
        self->field28 = 1;
        return kOk;
    }
    self->lastError = 0x80000003;
    return 0x80000003;
}

// NiDI8InputSystem::Create: for DI8 create parameters only: the system, the actions' names,
// two parameters kept globally, initialised (failing: deleted) and its devices enumerated
NiDI8InputSystem* __cdecl Create(uint8_t* params) {
    if (params == nullptr) {
        return nullptr;
    }
    auto* rtti = Virtual<uint8_t*>(params, 0);
    uint32_t index = DI8CreateParams_ClassIndex;
    if ((At<uint32_t>(rtti, 0xC + (index >> 5) * 4) & (1u << (index & 31))) == 0) {
        return nullptr;
    }
    void* p = NiNew(sizeof(NiDI8InputSystem));
    if (p == nullptr) {
        return nullptr;
    }
    NiDI8InputSystem* system = System_Construct(p);
    if (system == nullptr) {
        return nullptr;
    }
    for (uint32_t i = 0; i < 21; i++) {
        System_SetActionName(system, i, At<const char*>(params, 0x28 + i * 4));
    }
    gCreateParam20 = At<uint32_t>(params, 0x20);
    gCreateParam24 = At<uint32_t>(params, 0x24);
    if (Virtual<uint32_t>(system, 23, params) != 0) {
        Virtual(system, 0, 1u);
        return nullptr;
    }
    System_EnumerateDevices(system);
    return system;
}

// vf3 (by kind: 1 the keyboard (vf8 OpenKeyboard), 2 the mouse (vf7 OpenMouse), 3 a game pad at a
// port and slot; else 0x80000004)
void* __fastcall OpenDevice(NiDI8InputSystem* self, re::Edx, uint32_t kind, uint32_t port, uint32_t slot) {
    switch (kind) {
    case 1:
        return Virtual<void*>(self, 8);
    case 2:
        return Virtual<void*>(self, 7);
    case 3:
        return Virtual<void*>(self, 6, port, slot);
    default:
        self->lastError = kErrDeviceLost;
        return nullptr;
    }
}

// two vtable slots this system doesn't support: 0x80000001
void* __fastcall Unsupported2(NiDI8InputSystem* self, re::Edx, uint32_t, uint32_t) {
    self->lastError = 0x80000001;
    return nullptr;
}
void* __fastcall Unsupported0(NiDI8InputSystem* self, re::Edx) {
    self->lastError = 0x80000001;
    return nullptr;
}

// NiTObjectPool<ActionData>'s block: its deleting destructor
Block* __fastcall BlockDeletingDestructor(Block* self, re::Edx, uint32_t flags) {
    Block_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(Block));
    }
    return self;
}

// NiActionData's constructor
ActionData* __fastcall ActionDataConstruct(ActionData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(ActionData_vtable);
    self->appData = 0;
    self->deviceID = 0xFFFFFFFF;
    self->data = 0;
    self->context = 0;
    self->field14 = 0;
    self->sequence = 0;
    return self;
}

// vf5 (a description's device opened: vf6 with its port and slot)
void __fastcall OpenDeviceFor(NiDI8InputSystem* self, re::Edx, NiInputDeviceDescription* description) {
    if (description == nullptr) {
        return;
    }
    Virtual<void*>(self, 6, description->port, description->slot);
}

// the keyboard's buttons as mouse modifiers (bits 8..16) when they changed since last time
void __fastcall SyncKeyboardModifiers(NiDI8InputSystem* self, re::Edx) {
    if (self->keyboardDevice == nullptr || self->mouseDevice == nullptr) {
        return;
    }
    uint32_t buttons = self->keyboardDevice->modifiers;
    if (buttons == gLastKeyboardModifiers) {
        return;
    }
    uint32_t changed = gLastKeyboardModifiers ^ buttons;
    for (int i = 0; i < 8; i++) {
        uint32_t bit = 1u << i;
        if (changed & bit) {
            if (buttons & bit) {
                self->mouseDevice->modifiers |= bit << 8;
            } else {
                self->mouseDevice->modifiers &= ~(bit << 8);
            }
        }
    }
    gLastKeyboardModifiers = buttons;
    if (changed & 0x100) {
        if (buttons & 0x100) {
            self->mouseDevice->modifiers |= 0x10000;
            return;
        }
        self->mouseDevice->modifiers &= ~0x10000u;
    }
}

// the block's destructor: its elements deleted (an empty array freed, else the elements'
// vector deleting destructor), then the blocks after it
void __fastcall BlockDestruct(Block* self, re::Edx) {
    ActionData* data = self->data;
    if (data != nullptr) {
        if (reinterpret_cast<uint32_t*>(data)[-1] == 0) {
            OperatorDelete(reinterpret_cast<uint32_t*>(data) - 1);
        } else {
            Virtual(data, 0, 3u);
        }
    }
    Block* next = self->next;
    if (next != nullptr) {
        Block_Destruct(next);
        NiDelete(next, sizeof(Block));
    }
}

// `count` action data constructed in a new[] with the count before them
ActionData* __cdecl ActionDataNewArray(uint32_t count) {
    uint32_t* block;
    if (count == 0) {
        block = static_cast<uint32_t*>(OperatorNew(4));
    } else {
        uint64_t bytes = uint64_t(count) * sizeof(ActionData);
        uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
        size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
        block = static_cast<uint32_t*>(NiNewArray(size));
    }
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    VectorConstruct(block + 1, sizeof(ActionData), count, reinterpret_cast<void*>(0x008D2CA0));
    return reinterpret_cast<ActionData*>(block + 1);
}

// NiTPointerMap<..., NiPointer<NiShader>>::DeleteItem: the value let go, the node freed
void __fastcall ShaderMapDeleteItem(uint8_t* self, re::Edx, uint8_t* item) {
    re::Ptr32<NiRefObject>& value = At<NiRefObject*>(item, 8);
    if (value != nullptr) {
        Release(value);
        value = nullptr;
    }
    PointerAllocator_FreeNode(self + 0xC, item);
}

// vf4 CloseDevice: the keyboard, the mouse or a game pad let go
uint32_t __fastcall CloseDevice(NiDI8InputSystem* self, re::Edx, NiInputDevice* device) {
    if (self->keyboardDevice == device) {
        if (self->keyboardDevice != nullptr) {
            Release(self->keyboardDevice);
            self->keyboardDevice = nullptr;
        }
        return 0;
    }
    if (self->mouseDevice == device) {
        if (self->mouseDevice != nullptr) {
            Release(self->mouseDevice);
            self->mouseDevice = nullptr;
        }
        return 0;
    }
    for (re::Ptr32<NiInputGamePad>& pad : self->gamePads) {
        if (pad == device && pad != nullptr) {
            Release(pad);
            pad = nullptr;
        }
    }
    return 0;
}

// every open device updated (vf3; the keyboard's buttons passed on as modifiers); any lost:
// 0x80000006
uint32_t __fastcall UpdateAllDevices(NiDI8InputSystem* self, re::Edx) {
    bool lost = false;
    self->lastError = kOk;
    if (self->keyboardDevice != nullptr) {
        if (Virtual<uint32_t>(self->keyboardDevice, 3) == kErrAcquireFailed) {
            lost = true;
        } else {
            System_SyncKeyboardModifiers(self);
        }
    }
    if (self->mouseDevice != nullptr && Virtual<uint32_t>(self->mouseDevice, 3) == kErrAcquireFailed) {
        lost = true;
    }
    for (NiInputGamePad* pad : self->gamePads) {
        if (pad != nullptr && Virtual<uint32_t>(pad, 3) == kErrAcquireFailed) {
            lost = true;
        }
    }
    if (lost) {
        self->lastError = kErrAcquireFailed;
    }
    return self->lastError;
}

// the same through each device's UpdateActionMap (vf4, reporting to this system)
uint32_t __fastcall UpdateActionMap(NiDI8InputSystem* self, re::Edx) {
    bool lost = false;
    self->lastError = kOk;
    if (self->keyboardDevice != nullptr) {
        if (Virtual<uint32_t>(self->keyboardDevice, 4, self) == kErrAcquireFailed) {
            lost = true;
        } else {
            System_SyncKeyboardModifiers(self);
        }
    }
    if (self->mouseDevice != nullptr && Virtual<uint32_t>(self->mouseDevice, 4, self) == kErrAcquireFailed) {
        lost = true;
    }
    for (NiInputGamePad* pad : self->gamePads) {
        if (pad != nullptr && Virtual<uint32_t>(pad, 4, self) == kErrAcquireFailed) {
            lost = true;
        }
    }
    if (lost) {
        self->lastError = kErrAcquireFailed;
    }
    return self->lastError;
}

// every device let go, the descriptions' list emptied
void __fastcall ReleaseDevices(NiDI8InputSystem* self, re::Edx) {
    if (self->keyboardDevice != nullptr) {
        Release(self->keyboardDevice);
        self->keyboardDevice = nullptr;
    }
    if (self->mouseDevice != nullptr) {
        Release(self->mouseDevice);
        self->mouseDevice = nullptr;
    }
    for (re::Ptr32<NiInputGamePad>& pad : self->gamePads) {
        if (pad != nullptr) {
            Release(pad);
            pad = nullptr;
        }
    }
    for (NiTListNode<NiInputDeviceDescription*>* node = self->devices.head; node != nullptr;) {
        NiTListNode<NiInputDeviceDescription*>* next = node->next;
        Virtual(&self->devices, 2, node);
        node = next;
    }
    self->devices.count = 0;
    self->devices.head = nullptr;
    self->devices.tail = nullptr;
}

// the keyboard's actions: each keyboard semantic (not a modifier one) mapped unless the keyboard
// (vf8) says the control's taken; the modifier ones marked not applicable
uint32_t __fastcall MapKeyboardActions(NiDI8InputSystem* self, re::Edx) {
    if (self->keyboardDevice == nullptr) {
        return kOk;
    }
    if (self->actionMap == nullptr) {
        return kErrNoActionMap;
    }
    Device_ClearMappedActions(self->keyboardDevice);
    uint32_t count = self->actionMap->actions.esize;
    if (count == 0) {
        return kOk;
    }
    for (uint32_t i = 0; i < count; i++) {
        NiAction* action = ActionMap_GetAction(self->actionMap, i);
        if (action == nullptr || (action->semantic & 0x20000000) == 0) {
            continue;
        }
        uint32_t semantic = action->semantic;
        if ((semantic & 0x20010000) == 0x20010000) {
            action->status = 6;
            continue;
        }
        uint32_t control = MapSemanticToControl(semantic);
        if (Virtual<uint8_t>(self->keyboardDevice, 8, control, semantic, (action->flags >> 9) & 0xFFFFFF01, action->keyboardModifiers,
                             0u, 0u, 0u) != 0) {
            self->lastError = kErrActionMapped;
        } else {
            uint32_t error = Virtual<uint32_t>(self->keyboardDevice, 9, action);
            if (error != kOk) {
                self->lastError = error;
                action->status = 4;
            } else {
                action->status = 2;
                action->control = MapSemanticToControl(semantic);
            }
        }
    }
    return self->lastError;
}

} // namespace
} // namespace ni::input

// ---- registration ---------------------------------------------------------------------

#define NIDI8INPUT_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIDI8INPUT_REVERSED("NiDI8InputSystem::NiDI8InputSystem", 0x008D11E0, &ni::input::Construct);
NIDI8INPUT_REVERSED("NiDI8InputSystem::ConfigureDevice", 0x008D1370, &ni::input::ConfigureDevice);
NIDI8INPUT_REVERSED("NiInputDevice::Description::SetProductName", 0x008D1460, &ni::input::SetProductName);
NIDI8INPUT_REVERSED("NiInputDevice::Description::SetInstanceName", 0x008D14C0, &ni::input::SetInstanceName);
NIDI8INPUT_REVERSED("NiDI8InputSystem::GetRTTI", 0x008D1520, &ni::input::GetRTTI);
NIDI8INPUT_REVERSED("NiDI8InputSystem::~NiDI8InputSystem", 0x008D1530, &ni::input::Destruct);
NIDI8INPUT_REVERSED("NiDI8InputSystem::SetActionName", 0x008D1650, &ni::input::SetActionName);
NIDI8INPUT_REVERSED("NiDI8InputSystem::FillDescription", 0x008D16E0, &ni::input::FillDescription);
NIDI8INPUT_REVERSED("NiDI8InputSystem::BuildActionFormat", 0x008D17C0, &ni::input::BuildActionFormat);
NIDI8INPUT_REVERSED("NiInputSystem::GetActionDataCount", 0x008D1F00, &ni::input::GetActionDataCount);
NIDI8INPUT_REVERSED("NiInputSystem::PopActionData", 0x008D1F10, &ni::input::PopActionData);
NIDI8INPUT_REVERSED("NiInputSystem::GetFirstActionData", 0x008D1F50, &ni::input::GetFirstActionData);
NIDI8INPUT_REVERSED("NiInputSystem::GetNextActionData", 0x008D1F70, &ni::input::GetNextActionData);
NIDI8INPUT_REVERSED("NiDI8InputSystem::DeletingDestructor", 0x008D1F90, &ni::input::DeletingDestructor);
NIDI8INPUT_REVERSED("NiDI8InputSystem::GetGamePad", 0x008D1FC0, &ni::input::GetGamePad);
NIDI8INPUT_REVERSED("NiInputDevice::Description::~Description", 0x008D2660, &ni::input::DescriptionDestruct);
NIDI8INPUT_REVERSED("NiInputSystem::AddDescription", 0x008D26F0, &ni::input::AddDescription);
NIDI8INPUT_REVERSED("NiInputDevice::Description::Description", 0x008D2780, &ni::input::DescriptionConstruct);
NIDI8INPUT_REVERSED("NiInputDevice::Description::DeletingDestructor", 0x008D27E0,
                    &ni::input::DescriptionDeletingDestructor);
NIDI8INPUT_REVERSED("NiDI8InputSystem::DI8Description::DI8Description", 0x008D2800,
                    &ni::input::DI8DescriptionConstruct);
NIDI8INPUT_REVERSED("NiDI8InputSystem::DI8Description::DeletingDestructor", 0x008D2820,
                    &ni::input::DI8DescriptionDeletingDestructor);
NIDI8INPUT_REVERSED("NiDI8InputSystem::EnumDevicesCallback", 0x008D2850, &ni::input::EnumDevicesCallback);
NIDI8INPUT_REVERSED("NiDI8InputSystem::EnumGamePad", 0x008D2950, &ni::input::EnumGamePad);
NIDI8INPUT_REVERSED("NiDI8InputSystem::EnumGamePadsCallback", 0x008D2AA0, &ni::input::EnumGamePadsCallback);
NIDI8INPUT_REVERSED("NiDI8InputSystem::EnumerateDevices", 0x008D2AC0, &ni::input::EnumerateDevices);
NIDI8INPUT_REVERSED("NiDI8InputSystem::Create", 0x008D2B40, &ni::input::Create);
NIDI8INPUT_REVERSED("NiDI8InputSystem::OpenDevice", 0x008D2C10, &ni::input::OpenDevice);
NIDI8INPUT_REVERSED("NiDI8InputSystem::Unsupported2", 0x008D2C60, &ni::input::Unsupported2);
NIDI8INPUT_REVERSED("NiDI8InputSystem::Unsupported0", 0x008D2C70, &ni::input::Unsupported0);
NIDI8INPUT_REVERSED("NiTObjectPool<ActionData>::Block::DeletingDestructor", 0x008D2C80,
                    &ni::input::BlockDeletingDestructor);
NIDI8INPUT_REVERSED("NiActionData::NiActionData", 0x008D2CA0, &ni::input::ActionDataConstruct);
NIDI8INPUT_REVERSED("NiDI8InputSystem::OpenDeviceFor", 0x008D2CD0, &ni::input::OpenDeviceFor);
NIDI8INPUT_REVERSED("NiDI8InputSystem::SyncKeyboardModifiers", 0x008D2D30, &ni::input::SyncKeyboardModifiers);
NIDI8INPUT_REVERSED("NiTObjectPool<ActionData>::Block::~Block", 0x008D2E70, &ni::input::BlockDestruct);
NIDI8INPUT_REVERSED("NiActionData::NewArray", 0x008D2EB0, &ni::input::ActionDataNewArray);
NIDI8INPUT_REVERSED("NiTPointerMap<NiShaderPtr>::DeleteItem", 0x008D2F10, &ni::input::ShaderMapDeleteItem);
NIDI8INPUT_REVERSED("NiDI8InputSystem::CloseDevice", 0x008D2F50, &ni::input::CloseDevice);
NIDI8INPUT_REVERSED("NiDI8InputSystem::UpdateAllDevices", 0x008D3010, &ni::input::UpdateAllDevices);
NIDI8INPUT_REVERSED("NiDI8InputSystem::UpdateActionMap", 0x008D30A0, &ni::input::UpdateActionMap);
NIDI8INPUT_REVERSED("NiDI8InputSystem::ReleaseDevices", 0x008D3130, &ni::input::ReleaseDevices);
NIDI8INPUT_REVERSED("NiDI8InputSystem::MapKeyboardActions", 0x008D31E0, &ni::input::MapKeyboardActions);
