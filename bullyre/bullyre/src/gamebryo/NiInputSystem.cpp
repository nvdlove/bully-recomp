// NiInputSystem's half of NiInput (src/gamebryo/NiInput.h), 0x8D3300..0x8D5CC0: mapping the
// action map's actions to the devices, the action data reported to the game and its pool, the
// base device (its mapped actions), the game pad axis' dead zone, and NiInputDI8GamePad over
// DirectInput's buffered data. (NiDI8InputSystem and the base game pad sit just before.)

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInput.h"
#include "reversed.h"

namespace ni::input {
namespace {

GAME_VAR(uint8_t, NiInputSystem_RTTI, 0x020E9770);
GAME_VAR(uint8_t, NiInputDevice_RTTI, 0x020E97DC);
GAME_VAR(uint8_t, NiInputDI8GamePad_RTTI, 0x020E987C);
GAME_VAR(uint32_t, gActionSequence, 0x020E976C);           // the next action data's sequence
GAME_VAR(ActionDataPool*, gSharedActionDataPool, 0x020E9878);
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiInputSystem_vtable = 0x009A1D10;
constexpr uintptr_t NiInputDevice_vtable = 0x009A1DB8;
constexpr uintptr_t NiInputDI8GamePad_vtable = 0x009A1E70;
constexpr uintptr_t DeviceListVtable = 0x0095127C;       // NiTPointerList<Description*>
constexpr uintptr_t ActionDataListVtable = 0x00952E10;   // NiTPointerList<ActionData*>
constexpr uintptr_t MappedActionListVtable = 0x0095145C; // NiTPointerList<NiAction*>
constexpr uintptr_t PointerListBaseVtable = 0x0099C748;  // what a list's destructor leaves

GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, NiTPointerList_Remove, 0x0075E6B0, void* list, re::Ptr32<NiRefObject>* result,
        re::Ptr32<NiRefObject>* element);   // NiTPointerList<NiPointer<T>>: the element's node let go
GAME_FN(void, __thiscall, PrimitiveSet_Realloc, 0x0083AD20, ActionDataPool* set, uint32_t alloced);
GAME_FN(void, __thiscall, ActionDataPool_Purge, 0x00800050, ActionDataPool* pool);
GAME_FN(ActionData*, __cdecl, ActionData_NewArray, 0x008D2EB0, uint32_t count);
GAME_FN(uint32_t, __cdecl, MapSemanticToControl, 0x00828AB0, uint32_t semantic);
GAME_FN(void, __thiscall, GamePad_BeginUpdate, 0x008D0860, NiInputGamePad* self);
GAME_FN(void, __thiscall, GamePad_SetAxis, 0x008D0870, NiInputGamePad* self, uint32_t axis, int32_t raw);
GAME_FN(void, __thiscall, GamePad_PressButton, 0x008D10D0, NiInputGamePad* self, uint32_t button,
        uint32_t value);
GAME_FN(void, __thiscall, GamePad_ReleaseButton, 0x008D1150, NiInputGamePad* self, uint32_t button);
GAME_FN(void, __thiscall, GamePad_Construct, 0x008D0900, NiInputGamePad* self,
        NiInputDeviceDescription* description, int32_t low, int32_t high);
GAME_FN(void, __thiscall, GamePad_Destruct, 0x008D0A00, NiInputGamePad* self);
// this file's, called where modes apply
GAME_FN(uint32_t, __thiscall, InputSystem_MapKeyboardActions, 0x008D31E0, NiInputSystem* self);
GAME_FN(uint32_t, __thiscall, InputSystem_MapMouseActions, 0x008D3300, NiInputSystem* self);
GAME_FN(uint32_t, __thiscall, InputSystem_MapGamePadActions, 0x008D3470, NiInputSystem* self,
        NiInputGamePad* pad);
GAME_FN(uint32_t, __thiscall, InputSystem_MapAllGamePadActions, 0x008D3720, NiInputSystem* self);
GAME_FN(void, __thiscall, ActionDataPool_Grow, 0x008D3760, ActionDataPool* pool, uint32_t count);
GAME_FN(uint32_t, __thiscall, InputSystem_MapActionsToDevices, 0x008D3910, NiInputSystem* self);
GAME_FN(void, __thiscall, InputSystem_Destruct, 0x008D3A10, NiInputSystem* self);
GAME_FN(uint32_t, __thiscall, Device_ClearMappedActions, 0x008D3F80, NiInputDevice* self);
GAME_FN(void, __thiscall, Device_Destruct, 0x008D4110, NiInputDevice* self);
GAME_FN(NiAction*, __thiscall, ActionMap_GetAction, 0x008D5500, NiActionMap* self, uint32_t index);
GAME_FN(void, __cdecl, InputSystem_DeleteSharedPool, 0x008D5520);
GAME_FN(void, __thiscall, DI8GamePad_HandleEvent, 0x008D55B0, NiInputDI8GamePad* self, uint32_t object,
        int32_t value);

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
    kDISetProperty = 6,
    kDIAcquire = 7,
    kDIUnacquire = 8,
    kDIGetDeviceData = 10,
    kDIPoll = 25,
};

// a reference slot as the library inlines NiPointer's assignment here: the old one let go first
template <typename T>
void SetReference(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    if (slot != value) {
        Release(slot);
        slot = value;
        AddRef(value);
    }
}

// a reference let go and the slot cleared, only when it held one
template <typename T>
void ClearReference(re::Ptr32<T>& slot) {
    if (slot != nullptr) {
        Release(slot);
        slot = nullptr;
    }
}

// NiTPointerList's AddTail, inlined: a node from its allocator (slot 1)
template <typename L, typename T>
void ListAddTail(L list, T element) {
    auto* node = Virtual<NiTListNode<T>*>(list, 1);
    node->element = element;
    node->next = nullptr;
    node->prev = list->tail;
    if (list->tail != nullptr) {
        list->tail->next = node;
        list->count++;
        list->tail = node;
    } else {
        list->count++;
        list->head = node;
        list->tail = node;
    }
}

// NiTPointerList's RemoveAll, inlined: each node to its allocator (slot 2)
template <typename T>
void ListRemoveAll(NiTPointerList<T>* list) {
    for (NiTListNode<T>* node = list->head; node != nullptr;) {
        NiTListNode<T>* next = node->next;
        Virtual(list, 2, node);
        node = next;
    }
    list->count = 0;
    list->head = nullptr;
    list->tail = nullptr;
}

// the pool's free set: AddUnique
void PoolFree(ActionDataPool* pool, ActionData* data) {
    uint32_t count = pool->freeCount;
    for (uint32_t i = 0; i < count; i++) {
        if (pool->freeData[i] == data) {
            return;
        }
    }
    if (count == pool->freeAlloced) {
        PrimitiveSet_Realloc(pool, pool->freeAlloced != 0 ? pool->freeAlloced * 2 : 1);
    }
    pool->freeData[pool->freeCount] = data;
    pool->freeCount++;
}

ActionDataPool* NewPool() {
    auto* pool = static_cast<ActionDataPool*>(NiNew(sizeof(ActionDataPool)));
    if (pool != nullptr) {
        pool->freeData = nullptr;
        pool->freeAlloced = 0;
        pool->freeCount = 0;
        pool->blocks = nullptr;
        pool->growBy = 8;
        pool->initSize = 8;
    }
    return pool;
}

void DeletePool(ActionDataPool* pool) {
    if (pool != nullptr) {
        ActionDataPool_Purge(pool);
        NiDelete(pool, sizeof(ActionDataPool));
    }
}

uint32_t DeviceID(const NiInputDevice* device) {
    return device->description->slot << 16 | device->description->port;
}

// ---- NiInputSystem

// the mouse's actions: each mouse semantic of the map naming an axis (0x40000001..3) or a
// button (0x40001000..7) mapped, unless the mouse (slot 8) says it's taken; the modifier
// semantics marked not applicable
uint32_t __fastcall MapMouseActions(NiInputSystem* self, re::Edx) {
    NiInputDevice* mouse = self->mouseDevice;
    if (mouse == nullptr) {
        return kOk;
    }
    if (self->actionMap == nullptr) {
        return kErrNoActionMap;
    }
    Device_ClearMappedActions(mouse);
    uint32_t count = self->actionMap->actions.esize;
    if (count == 0) {
        return kOk;
    }
    for (uint32_t i = 0; i < count; i++) {
        NiAction* action = ActionMap_GetAction(self->actionMap, i);
        if (action == nullptr || (action->semantic & 0x40000000) == 0) {
            continue;
        }
        uint32_t semantic = action->semantic;
        if (semantic <= 0x40001007 && (semantic >= 0x40001000 || (semantic >= 0x40000001 && semantic <= 0x40000003))) {
            uint32_t control = MapSemanticToControl(semantic);
            if (Virtual<uint8_t>(self->mouseDevice, 8, control, semantic, (action->flags >> 9) & 0xFFFFFF01,
                                 action->mouseModifiers, 0u, 0u, 0u) != 0) {
                self->lastError = kErrActionMapped;
            } else {
                uint32_t error = Virtual<uint32_t>(self->mouseDevice, 9, action);
                if (error != kOk) {
                    self->lastError = error;
                    action->status = 4;
                } else {
                    action->status = 2;
                    action->control = MapSemanticToControl(semantic);
                }
            }
        } else if ((semantic & 0x40010100) == 0x40010100 || (semantic & 0x40010001) == 0x40010001 ||
                   (semantic & 0x40010002) == 0x40010002 || (semantic & 0x40010003) == 0x40010003) {
            action->status = 6;
        }
    }
    return self->lastError;
}

// a game pad's actions: each game pad semantic naming one control (1..8 or 0x100..0x10F)
// added to the pad; the pad's modifier semantics marked not applicable. (0, not the last
// error, when done.)
uint32_t __fastcall MapGamePadActions(NiInputSystem* self, re::Edx, NiInputGamePad* pad) {
    if (pad == nullptr) {
        return kErrInvalidParams;
    }
    if (self->actionMap == nullptr) {
        return kErrNoActionMap;
    }
    Device_ClearMappedActions(pad);
    uint32_t count = self->actionMap->actions.esize;
    if (count == 0) {
        return kOk;
    }
    for (uint32_t i = 0; i < count; i++) {
        NiAction* action = ActionMap_GetAction(self->actionMap, i);
        if (action == nullptr || (action->semantic & 0x10000000) == 0) {
            continue;
        }
        uint32_t semantic = action->semantic;
        if (semantic < 0x10000110 && (semantic > 0x100000FF || (semantic > 0x10000000 && semantic < 0x10000009))) {
            action->control = MapSemanticToControl(semantic);
            uint32_t error = Virtual<uint32_t>(pad, 9, action);
            if (error != kOk) {
                self->lastError = error;
            }
        } else if ((semantic & 0x10010100) == 0x10010100 || (semantic & 0x10010000) == 0x10010000 ||
                   (semantic & 0x10010001) == 0x10010001 || (semantic & 0x10010002) == 0x10010002 ||
                   (semantic & 0x10010003) == 0x10010003 || (semantic & 0x10001004) == 0x10001004 ||
                   (semantic & 0x10001005) == 0x10001005 || (semantic & 0x10001006) == 0x10001006 ||
                   (semantic & 0x10001007) == 0x10001007) {
            action->status = 6;
        }
    }
    return kOk;
}

// the constructor: no devices, no map; the axis range -100..100. (The lists it has just
// cleared are emptied once more, as the inlined helpers do.)
NiInputSystem* __fastcall Construct(NiInputSystem* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
    self->vtable = reinterpret_cast<void*>(NiInputSystem_vtable);
    self->lastError = kOk;
    self->renderer = nullptr;
    self->mode = 0;
    self->createParams[0] = 0;
    self->createParams[1] = 0;
    self->createParams[2] = 0;
    self->axisLow = -100;
    self->axisHigh = 100;
    self->field28 = 0;
    self->devices.count = 0;
    self->devices.head = nullptr;
    self->devices.tail = nullptr;
    self->devices.vtable = reinterpret_cast<void*>(DeviceListVtable);
    self->keyboardDevice = nullptr;
    self->mouseDevice = nullptr;
    for (re::Ptr32<NiInputGamePad>& pad : self->gamePads) {
        pad = nullptr;
    }
    self->actionMap = nullptr;
    self->actionData.count = 0;
    self->actionData.head = nullptr;
    self->actionData.tail = nullptr;
    self->actionData.vtable = reinterpret_cast<void*>(ActionDataListVtable);
    self->actionDataPool = nullptr;
    for (re::Ptr32<NiInputGamePad>& pad : self->gamePads) {
        ClearReference(pad);
    }
    ListRemoveAll(&self->actionData);
    ListRemoveAll(&self->devices);
    return self;
}

void* __fastcall GetRTTI(NiInputSystem*, re::Edx) {
    return &NiInputSystem_RTTI;
}

// every open game pad's actions; the last failure kept
uint32_t __fastcall MapAllGamePadActions(NiInputSystem* self, re::Edx) {
    for (NiInputGamePad* pad : self->gamePads) {
        if (pad != nullptr) {
            uint32_t error = InputSystem_MapGamePadActions(self, pad);
            if (error != kOk) {
                self->lastError = error;
            }
        }
    }
    return self->lastError;
}

// vf23 Initialize: the create parameters' renderer, ranges and action map (a pool for the
// action data when there is a map)
uint32_t __fastcall Initialize(NiInputSystem* self, re::Edx, uint8_t* params) {
    if (params == nullptr) {
        self->lastError = kErrInvalidParams;
        return self->lastError;
    }
    SetReference(self->renderer, *reinterpret_cast<re::Ptr32<NiRefObject>*>(params + 0x1C));
    self->createParams[0] = *reinterpret_cast<uint32_t*>(params + 0x8);
    self->createParams[1] = *reinterpret_cast<uint32_t*>(params + 0xC);
    self->createParams[2] = *reinterpret_cast<uint32_t*>(params + 0x10);
    self->axisLow = *reinterpret_cast<int32_t*>(params + 0x14);
    self->axisHigh = *reinterpret_cast<int32_t*>(params + 0x18);
    SetReference(self->actionMap, *reinterpret_cast<re::Ptr32<NiActionMap>*>(params + 0x4));
    if (self->actionMap != nullptr) {
        self->mode = 1;
        self->actionDataPool = NewPool();
    }
    return self->lastError;
}

// all three kinds of device; the first kind that failed named
uint32_t __fastcall MapActionsToDevices(NiInputSystem* self, re::Edx) {
    uint32_t keyboard = InputSystem_MapKeyboardActions(self);
    uint32_t mouse = InputSystem_MapMouseActions(self);
    uint32_t pads = InputSystem_MapAllGamePadActions(self);
    if (keyboard != kOk) {
        self->lastError = kErrKeyboardMapping;
    } else if (mouse != kOk) {
        self->lastError = kErrMouseMapping;
    } else if (pads != kOk) {
        self->lastError = kErrGamePadMapping;
    } else {
        self->lastError = kOk;
    }
    return self->lastError;
}

// vf26: the enumerated device description at a port and slot removed
void __fastcall RemoveDeviceDescription(NiInputSystem* self, re::Edx, uint32_t port, uint32_t slot) {
    for (NiTListNode<NiInputDeviceDescription*>* node = self->devices.head; node != nullptr;) {
        NiInputDeviceDescription* description = node->element;
        node = node->next;
        if (description != nullptr && description->port == port && description->slot == slot) {
            re::Ptr32<NiRefObject> element = description;
            AddRef(element);
            re::Ptr32<NiRefObject> result;
            NiTPointerList_Remove(&self->devices, &result, &element);
            Release(result);
            Release(element);
            return;
        }
    }
}

// the destructor: the action data still listed back to the pool (used without checking it's
// there), the pool and the shared one deleted, every reference let go
void __fastcall Destruct(NiInputSystem* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiInputSystem_vtable);
    ClearReference(self->actionMap);
    for (NiTListNode<ActionData*>* node = self->actionData.head; node != nullptr;) {
        ActionData* data = node->element;
        node = node->next;
        if (data != nullptr) {
            PoolFree(self->actionDataPool, data);
        }
    }
    DeletePool(self->actionDataPool);
    InputSystem_DeleteSharedPool();
    ClearReference(self->renderer);
    self->actionData.vtable = reinterpret_cast<void*>(ActionDataListVtable);
    NiTPointerList_RemoveAll(&self->actionData);
    self->actionData.vtable = reinterpret_cast<void*>(PointerListBaseVtable);
    Release(self->actionMap);
    for (int i = 7; i >= 0; i--) {
        Release(self->gamePads[i]);
    }
    Release(self->mouseDevice);
    Release(self->keyboardDevice);
    self->devices.vtable = reinterpret_cast<void*>(DeviceListVtable);
    NiTPointerList_RemoveAll(&self->devices);
    self->devices.vtable = reinterpret_cast<void*>(PointerListBaseVtable);
    Release(self->renderer);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
}

// vf12 SetActionMap: a new map mapped onto the devices; none, out of mapped mode (the action
// data released, every device's mapped actions cleared, the pool gone)
uint32_t __fastcall SetActionMap(NiInputSystem* self, re::Edx, NiActionMap* map) {
    if (map == self->actionMap) {
        return kOk;
    }
    SetReference(self->actionMap, map);
    if (self->actionMap != nullptr) {
        self->mode = 1;
        if (self->actionDataPool == nullptr) {
            self->actionDataPool = NewPool();
        }
        InputSystem_MapActionsToDevices(self);
        return self->lastError;
    }
    if (self->mode == 1) {
        Virtual(self, 17);
        if (self->keyboardDevice != nullptr) {
            Device_ClearMappedActions(self->keyboardDevice);
        }
        if (self->mouseDevice != nullptr) {
            Device_ClearMappedActions(self->mouseDevice);
        }
        for (NiInputGamePad* pad : self->gamePads) {
            if (pad != nullptr) {
                Device_ClearMappedActions(pad);
            }
        }
        DeletePool(self->actionDataPool);
        self->actionDataPool = nullptr;
        self->mode = 0;
    }
    return self->lastError;
}

// vf17: this frame's action data back to the pool (used without checking it's there)
uint32_t __fastcall ReleaseActionData(NiInputSystem* self, re::Edx) {
    auto& iter = reinterpret_cast<re::Ptr32<NiTListNode<ActionData*>>&>(self->actionDataIter);
    iter = self->actionData.head;
    while (iter != nullptr) {
        NiTListNode<ActionData*>* node = iter;
        iter = node->next;
        if (node->element != nullptr) {
            PoolFree(self->actionDataPool, node->element);
        }
    }
    NiTPointerList_RemoveAll(&self->actionData);
    return kOk;
}

// an action's data from the pool (grown by its step, which doubles, when empty; with no pool
// the game writes through null) listed for the game
uint32_t __fastcall ReportAction(NiInputSystem* self, re::Edx, NiInputDevice* device, uint32_t appData,
                                 uint32_t sequence, uint32_t data, uint32_t context) {
    ActionData* entry = nullptr;
    if (ActionDataPool* pool = self->actionDataPool) {
        if (pool->freeCount == 0) {
            ActionDataPool_Grow(pool, pool->growBy);
            pool->growBy *= 2;
        }
        re::Ptr32<ActionData>* free = pool->freeData;
        entry = free[0];
        pool->freeCount--;
        free[0] = free[pool->freeCount];
    }
    entry->appData = appData;
    entry->data = data;
    entry->sequence = sequence != 0 ? sequence : gActionSequence++;
    entry->context = context;
    entry->deviceID = DeviceID(device);
    ListAddTail(&self->actionData, entry);
    return kOk;
}

NiInputSystem* __fastcall DeletingDestructor(NiInputSystem* self, re::Edx, uint32_t flags) {
    InputSystem_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputSystem));
    }
    return self;
}

// a block of `count` action data (made as an array) added to the pool's free set
void __fastcall PoolGrow(ActionDataPool* pool, re::Edx, uint32_t count) {
    auto* block = static_cast<Block*>(NiNew(sizeof(Block)));
    if (block != nullptr) {
        block->count = count;
        block->next = nullptr;
        block->data = count != 0 ? ActionData_NewArray(count) : nullptr;
    }
    for (uint32_t i = 0; i < count; i++) {
        ActionData* data = i < block->count ? &block->data[i] : nullptr;
        if (pool->freeCount == pool->freeAlloced) {
            PrimitiveSet_Realloc(pool, pool->freeAlloced != 0 ? pool->freeAlloced * 2 : 1);
        }
        pool->freeData[pool->freeCount] = data;
        pool->freeCount++;
    }
    block->next = pool->blocks;
    pool->blocks = block;
}

// the shared pool (0x20E9878) purged and deleted
void __cdecl DeleteSharedPool() {
    ActionDataPool* pool = gSharedActionDataPool;
    DeletePool(pool);
    gSharedActionDataPool = nullptr;
}

// ---- NiInputDevice

// vf8 (the device's "already mapped?" test, 7 arguments): no
uint8_t __fastcall NotMapped(NiInputDevice*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                             uint32_t) {
    return 0;
}

uint32_t __fastcall ClearMappedActions(NiInputDevice* self, re::Edx) {
    if (self->mappedActions == nullptr) {
        self->lastError = kErrInvalidParams;
        return kErrInvalidParams;
    }
    NiTPointerList_RemoveAll(self->mappedActions);
    return kOk;
}

// the constructor: the description held and marked open
NiInputDevice* __fastcall DeviceConstruct(NiInputDevice* self, re::Edx, NiInputDeviceDescription* description) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
    self->vtable = reinterpret_cast<void*>(NiInputDevice_vtable);
    self->description = nullptr;
    self->status = -1;
    self->modifiers = 0;
    self->field14 = 0;
    self->mappedActions = nullptr;
    SetReference(self->description, description);
    description->opened = 1;
    return self;
}

void* __fastcall DeviceGetRTTI(NiInputDevice*, re::Edx) {
    return &NiInputDevice_RTTI;
}

// the destructor: the mapped-action list deleted, the description marked closed and let go
void __fastcall DeviceDestruct(NiInputDevice* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiInputDevice_vtable);
    if (self->mappedActions != nullptr) {
        NiTPointerList_RemoveAll(self->mappedActions);
    }
    if (self->mappedActions != nullptr) {
        Virtual(self->mappedActions, 0, 1u);
    }
    self->description->opened = 0;
    ClearReference(self->description);
    Release(self->description);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(reinterpret_cast<volatile long*>(&NiRefObject_ms_uiObjects));
}

// NiTPointerList<ActionData*>'s deleting destructor (the vtable 0x952E10 lists')
void* __fastcall PointerListDeletingDestructor(NiTPointerList<void*>* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(ActionDataListVtable);
    NiTPointerList_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(PointerListBaseVtable);
    if (flags & 1) {
        NiDelete(self, sizeof(NiTPointerList<void*>));
    }
    return self;
}

// whether an action applies to this device: its device, when it names one, and its modifiers
// (with the control's own bit, or without), when they must match
uint8_t __fastcall ActionApplies(NiInputDevice* self, re::Edx, NiAction* action, uint32_t modifiers) {
    if ((action->flags & 0x400) != 0 && action->deviceID != DeviceID(self)) {
        return 0;
    }
    if ((action->flags & 0x200) != 0 && ((1u << (action->control & 31)) | modifiers) != self->modifiers &&
        modifiers != self->modifiers) {
        return 0;
    }
    return 1;
}

NiInputDevice* __fastcall DeviceDeletingDestructor(NiInputDevice* self, re::Edx, uint32_t flags) {
    Device_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputDevice));
    }
    return self;
}

// vf9 AddMappedAction (the list made on the first)
uint32_t __fastcall AddMappedAction(NiInputDevice* self, re::Edx, NiAction* action) {
    if (self->mappedActions == nullptr) {
        auto* list = static_cast<NiTPointerList<NiAction*>*>(NiNew(sizeof(NiTPointerList<NiAction*>)));
        if (list != nullptr) {
            list->count = 0;
            list->head = nullptr;
            list->tail = nullptr;
            list->vtable = reinterpret_cast<void*>(MappedActionListVtable);
        }
        self->mappedActions = list;
        if (list == nullptr) {
            self->lastError = kErrAllocFailed;
            return kErrAllocFailed;
        }
    }
    ListAddTail(self->mappedActions, action);
    return kOk;
}

// ---- NiActionMap

// the action at an index (an index equal to the size gets through the check)
NiAction* __fastcall GetAction(NiActionMap* self, re::Edx, uint32_t index) {
    if (self->actions.size < index) {
        return nullptr;
    }
    return self->actions.base[index];
}

// ---- NiInputDI8GamePad

// vf5
uint32_t __fastcall PadUnacquire(NiInputDI8GamePad* self, re::Edx) {
    Com(self->device, kDIUnacquire);
    return kOk;
}

// vf6
uint32_t __fastcall PadAcquire(NiInputDI8GamePad* self, re::Edx) {
    int32_t hr = Com(self->device, kDIAcquire);
    if (hr == 0) {
        self->lastError = kOk;
    } else if (hr != int32_t(0x8007001E) && hr != int32_t(0x8007000C) && hr != int32_t(0x80070005)) {
        self->lastError = kErrNotAcquired;
    } else {
        self->lastError = kErrAcquireFailed;
    }
    return self->lastError;
}

// one buffered DirectInput event (the object's index is its application data): axes 0..7 set,
// the point of view as the four direction buttons 0..3, buttons 9..20 as the pad's buttons
void __fastcall HandleEvent(NiInputDI8GamePad* self, re::Edx, uint32_t object, int32_t value) {
    // the pad's button for objects 9..20
    static const uint8_t kButtons[12] = {8, 9, 10, 11, 4, 6, 14, 15, 5, 7, 12, 13};
    if (object > 20) {
        return;
    }
    if (object < 8) {
        GamePad_SetAxis(self, object, value);
        self->axesChanged |= 1u << object;
        return;
    }
    if (object > 8) {
        uint32_t button = kButtons[object - 9];
        if (value == 0) {
            GamePad_ReleaseButton(self, button);
        } else {
            GamePad_PressButton(self, button, 0xFF);
        }
        self->buttonsChanged |= 1u << button;
        return;
    }
    // the point of view, in hundredths of a degree, as buttons up 0, down 1, left 2, right 3
    switch (value) {
    case 13500:   // down and right
        GamePad_ReleaseButton(self, 0);
        GamePad_ReleaseButton(self, 2);
        GamePad_PressButton(self, 1, 0xFF);
        GamePad_PressButton(self, 3, 0xFF);
        break;
    case 4500:   // up and right
        GamePad_ReleaseButton(self, 1);
        GamePad_ReleaseButton(self, 2);
        GamePad_PressButton(self, 0, 0xFF);
        GamePad_PressButton(self, 3, 0xFF);
        break;
    case -1:   // centred
        GamePad_ReleaseButton(self, 1);
        GamePad_ReleaseButton(self, 2);
        GamePad_ReleaseButton(self, 3);
        GamePad_ReleaseButton(self, 0);
        break;
    case 0:   // up
        GamePad_ReleaseButton(self, 1);
        GamePad_ReleaseButton(self, 2);
        GamePad_ReleaseButton(self, 3);
        GamePad_PressButton(self, 0, 0xFF);
        break;
    case 9000:   // right
        GamePad_ReleaseButton(self, 0);
        GamePad_ReleaseButton(self, 1);
        GamePad_ReleaseButton(self, 2);
        GamePad_PressButton(self, 3, 0xFF);
        break;
    case 18000:   // down
        GamePad_ReleaseButton(self, 0);
        GamePad_ReleaseButton(self, 2);
        GamePad_ReleaseButton(self, 3);
        GamePad_PressButton(self, 1, 0xFF);
        break;
    case 27000:   // left
        GamePad_ReleaseButton(self, 0);
        GamePad_ReleaseButton(self, 1);
        GamePad_ReleaseButton(self, 3);
        GamePad_PressButton(self, 2, 0xFF);
        break;
    case 22500:   // down and left
        GamePad_ReleaseButton(self, 0);
        GamePad_ReleaseButton(self, 3);
        GamePad_PressButton(self, 1, 0xFF);
        GamePad_PressButton(self, 2, 0xFF);
        break;
    case 31500:   // up and left
        GamePad_ReleaseButton(self, 1);
        GamePad_ReleaseButton(self, 3);
        GamePad_PressButton(self, 0, 0xFF);
        GamePad_PressButton(self, 2, 0xFF);
        break;
    default:
        break;
    }
    self->buttonsChanged |= 0xF;
}

void* __fastcall PadGetRTTI(NiInputDI8GamePad*, re::Edx) {
    return &NiInputDI8GamePad_RTTI;
}

// vf13: nothing to report
void __fastcall PadNoValue(NiInputDI8GamePad*, re::Edx, uint32_t, uint32_t* out) {
    *out = 0;
}

// the constructor: the device's dead zone 25% (DIPROP_DEADZONE) and acquired, every axis'
// raw range the given one
NiInputDI8GamePad* __fastcall PadConstruct(NiInputDI8GamePad* self, re::Edx, NiInputDeviceDescription* description,
                                           void* device, int32_t low, int32_t high) {
    GamePad_Construct(self, description, low, high);
    self->vtable = reinterpret_cast<void*>(NiInputDI8GamePad_vtable);
    self->device = device;
    if (device != nullptr) {
        struct {
            uint32_t size, headerSize, object, how, data;
        } deadZone = {0x14, 0x10, 0, 0, 2500};   // DIPROPDWORD, DIPH_DEVICE
        if (Com(device, kDISetProperty, uintptr_t(5), &deadZone) == 0 && Com(self->device, kDIAcquire) != 0) {
            self->lastError = kErrAcquireFailed;
        }
        for (int i = 0; i < 8; i++) {
            self->axes[i].rawLow = low;
            self->axes[i].rawHigh = high;
        }
    }
    return self;
}

NiInputDI8GamePad* __fastcall PadDeletingDestructor(NiInputDI8GamePad* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiInputDI8GamePad_vtable);
    if (self->device != nullptr) {
        Com(self->device, kDIUnacquire);
        Com(self->device, kComRelease);
    }
    self->device = nullptr;
    GamePad_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiInputDI8GamePad));
    }
    return self;
}

// vf3 UpdateDevice: acquired, polled, up to 17 buffered events handled; what they didn't
// change keeps its value as the last one
uint32_t __fastcall PadUpdate(NiInputDI8GamePad* self, re::Edx) {
    struct ObjectData {   // DIDEVICEOBJECTDATA (the system's: uAppData a UINT_PTR)
        uint32_t offset, data, timeStamp, sequence;
        uintptr_t appData;
    };
    self->lastError = kOk;
    GamePad_BeginUpdate(self);
    if (self->device == nullptr) {
        self->lastError = kErrDeviceLost;
        return kErrDeviceLost;
    }
    uint32_t count = 17;
    ObjectData events[17];
    int32_t hr = Com(self->device, kDIAcquire);
    if (hr != 0 && hr != 1) {
        if (hr != int32_t(0x8007001E) && hr != int32_t(0x8007000C) && hr != int32_t(0x80040209)) {
            self->lastError = kErrDeviceLost;
            return self->lastError;
        }
        self->lastError = kErrDeviceBusy;
        self->status = 2;
        return self->lastError;
    }
    self->status = 0;
    hr = Com(self->device, kDIPoll);
    if (hr != 0 && hr != 1) {
        self->lastError = kErrDeviceLost;
        return kErrDeviceLost;
    }
    if (Com(self->device, kDIGetDeviceData, uint32_t(sizeof(ObjectData)), events, &count, 0u) != 0) {
        return self->lastError;
    }
    if (count == 0) {
        Virtual(self, 7);
        return self->lastError;
    }
    self->buttonsChanged = 0;
    self->axesChanged = 0;
    for (uint32_t i = 0; i < count; i++) {
        DI8GamePad_HandleEvent(self, events[i].appData, int32_t(events[i].data));
    }
    for (int i = 0; i < 16; i++) {
        if ((self->buttonsChanged & (1u << i)) == 0) {
            GamePadButton& button = self->buttons[i];
            button.lastValue = button.value;
            button.lastPressed = button.pressed;
            button.lastModifiers = button.modifiers;
        }
    }
    for (int i = 0; i < 4; i++) {
        if ((self->axesChanged & (1u << i)) == 0) {
            GamePadAxis& axis = self->axes[i];
            axis.lastRaw = axis.raw;
            axis.lastValue = axis.value;
        }
    }
    return self->lastError;
}

// ---- the game pad's axis

// a raw reading (flipped within the raw range when asked) normalised, put through the dead
// zone around the middle, and mapped to the axis' range. In float as the game's 24-bit x87.
void __fastcall AxisSetRaw(GamePadAxis* self, re::Edx, int32_t raw, uint8_t flip, float deadZone) {
    self->lastRaw = self->raw;
    self->lastValue = self->value;
    self->raw = raw;
    int32_t v = flip != 0 ? self->rawHigh + self->rawLow - raw : raw;
    float norm = float(v - self->rawLow) / float(self->rawHigh - self->rawLow);
    float halfZone = deadZone * 0.5f;
    float upper = 0.5f + halfZone;
    float result;
    if (upper < norm) {
        result = 0.5f + (norm - upper) * 0.5f / (1.0f - upper);
    } else {
        float lower = 0.5f - halfZone;
        if (lower > norm) {
            result = norm / lower * 0.5f;
        } else {
            result = 0.5f;
        }
    }
    int32_t low = self->low;
    self->value = crt::FloatToInt(float(self->high - low) * result) + low;
}

} // namespace
} // namespace ni::input

// ---- registration ---------------------------------------------------------------------

#define NIINPUT_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIINPUT_REVERSED("NiInputSystem::MapMouseActions", 0x008D3300, &ni::input::MapMouseActions);
NIINPUT_REVERSED("NiInputSystem::MapGamePadActions", 0x008D3470, &ni::input::MapGamePadActions);
NIINPUT_REVERSED("NiInputSystem::NiInputSystem", 0x008D35D0, &ni::input::Construct);
NIINPUT_REVERSED("NiInputSystem::GetRTTI", 0x008D3710, &ni::input::GetRTTI);
NIINPUT_REVERSED("NiInputSystem::MapAllGamePadActions", 0x008D3720, &ni::input::MapAllGamePadActions);
NIINPUT_REVERSED("NiTObjectPool<ActionData>::Grow", 0x008D3760, &ni::input::PoolGrow);
NIINPUT_REVERSED("NiInputSystem::Initialize", 0x008D3810, &ni::input::Initialize);
NIINPUT_REVERSED("NiInputSystem::MapActionsToDevices", 0x008D3910, &ni::input::MapActionsToDevices);
NIINPUT_REVERSED("NiInputSystem::RemoveDeviceDescription", 0x008D3970, &ni::input::RemoveDeviceDescription);
NIINPUT_REVERSED("NiInputSystem::~NiInputSystem", 0x008D3A10, &ni::input::Destruct);
NIINPUT_REVERSED("NiInputSystem::SetActionMap", 0x008D3BE0, &ni::input::SetActionMap);
NIINPUT_REVERSED("NiInputSystem::ReleaseActionData", 0x008D3D10, &ni::input::ReleaseActionData);
NIINPUT_REVERSED("NiInputSystem::ReportAction", 0x008D3DA0, &ni::input::ReportAction);
NIINPUT_REVERSED("NiInputSystem::DeletingDestructor", 0x008D3E70, &ni::input::DeletingDestructor);
NIINPUT_REVERSED("NiInputDevice::NotMapped", 0x008D3EA0, &ni::input::NotMapped);
NIINPUT_REVERSED("NiInputGamePad::Axis::SetRaw", 0x008D3EB0, &ni::input::AxisSetRaw);
NIINPUT_REVERSED("NiInputDevice::ClearMappedActions", 0x008D3F80, &ni::input::ClearMappedActions);
NIINPUT_REVERSED("NiInputDevice::NiInputDevice", 0x008D4080, &ni::input::DeviceConstruct);
NIINPUT_REVERSED("NiInputDevice::GetRTTI", 0x008D4100, &ni::input::DeviceGetRTTI);
NIINPUT_REVERSED("NiInputDevice::~NiInputDevice", 0x008D4110, &ni::input::DeviceDestruct);
NIINPUT_REVERSED("NiTPointerList<ActionData*>::DeletingDestructor", 0x008D4190,
                 &ni::input::PointerListDeletingDestructor);
NIINPUT_REVERSED("NiInputDevice::ActionApplies", 0x008D41C0, &ni::input::ActionApplies);
NIINPUT_REVERSED("NiInputDevice::DeletingDestructor", 0x008D4220, &ni::input::DeviceDeletingDestructor);
NIINPUT_REVERSED("NiInputDevice::AddMappedAction", 0x008D4240, &ni::input::AddMappedAction);
NIINPUT_REVERSED("NiActionMap::GetAction", 0x008D5500, &ni::input::GetAction);
NIINPUT_REVERSED("NiInputSystem::DeleteSharedPool", 0x008D5520, &ni::input::DeleteSharedPool);
NIINPUT_REVERSED("NiInputDI8GamePad::Unacquire", 0x008D5550, &ni::input::PadUnacquire);
NIINPUT_REVERSED("NiInputDI8GamePad::Acquire", 0x008D5560, &ni::input::PadAcquire);
NIINPUT_REVERSED("NiInputDI8GamePad::HandleEvent", 0x008D55B0, &ni::input::HandleEvent);
NIINPUT_REVERSED("NiInputDI8GamePad::GetRTTI", 0x008D5B50, &ni::input::PadGetRTTI);
NIINPUT_REVERSED("NiInputDI8GamePad::NoValue", 0x008D5B60, &ni::input::PadNoValue);
NIINPUT_REVERSED("NiInputDI8GamePad::NiInputDI8GamePad", 0x008D5B70, &ni::input::PadConstruct);
NIINPUT_REVERSED("NiInputDI8GamePad::DeletingDestructor", 0x008D5C70, &ni::input::PadDeletingDestructor);
NIINPUT_REVERSED("NiInputDI8GamePad::UpdateDevice", 0x008D5CC0, &ni::input::PadUpdate);
