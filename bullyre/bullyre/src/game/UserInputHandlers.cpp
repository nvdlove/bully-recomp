#include "reversed.h"

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
// The input handlers the UserInputManager (0xA45A30) passes pad events to (camera,
// cheat codes, debug menu, demo mode, menu system, player: 0x50E5A0 makes them), the
// menu's pad/keyboard tests beside them, and the memory-chunk allocator bits that
// follow them in the exe (LDefaultAllocator, LMemoryChunkBase).

#define INPUTHANDLERS_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline void* __cdecl Memcpy(void* dst, const void* src, size_t size) { return crt::Memcpy(dst, src, size); }

// pad and keyboard state
GAME_FN(float, __cdecl, Pad_GetButton, 0x00738840, int button, int pad);
GAME_FN(float, __cdecl, Pad_GetAxis, 0x00738AD0, int axis, int pad);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustDown, 0x00738900, int button, int pad);
GAME_FN(uint8_t, __cdecl, Pad_ButtonJustUp, 0x00738940, int button, int pad);
GAME_FN(uint8_t, __cdecl, Key_JustDown, 0x00738880, int key, int a, int b);
GAME_FN(uint8_t, __cdecl, Key_JustUp, 0x007388C0, int key, int a, int b);
GAME_FN(int32_t, __cdecl, Pad_GetActivePad, 0x00738070);

GAME_FN(uint8_t, __thiscall, UserInputManager_IsEnabled, 0x0050DF50, void* self);
GAME_FN(int32_t, __thiscall, Camera_OnPadEvent, 0x004F2190, void* camera, void* e);
GAME_FN(uint8_t, __thiscall, CheatCodeManager_OnButton, 0x00425240, void* self, void* e);
GAME_FN(uint8_t, __thiscall, MenuSystem_OnPadEvent, 0x00609650, void* self, void* e);
GAME_FN(void, __thiscall, ActionController_Play, 0x005F3A90, void* self, void* node, int mode, float value);
GAME_FN(uint8_t*, __thiscall, ActionController_Get, 0x005F3720, void* self, void* node);
GAME_FN(void, __thiscall, ActionController_Clear, 0x005F3A40, void* self);

GAME_FN(void, __thiscall, UserInputHandler_Construct, 0x0050E560, void* self);
GAME_FN(void, __thiscall, UserInputHandler_Destruct, 0x0050E570, void* self);
GAME_FN(void*, __thiscall, CameraUserInputHandler_Construct, 0x0050E250, void* self);
GAME_FN(void*, __thiscall, CheatCodeUserInputHandler_Construct, 0x0050E2C0, void* self);
GAME_FN(void*, __thiscall, DebugMenuUserInputHandler_Construct, 0x0050E320, void* self);
GAME_FN(void*, __thiscall, DemoModeUserInputHandler_Construct, 0x0050E370, void* self);
GAME_FN(void*, __thiscall, MenuSystemUserInputHandler_Construct, 0x0050E3C0, void* self);
GAME_FN(void*, __thiscall, PlayerUserInputHandler_Construct, 0x0050E420, void* self);
GAME_FN(void, __thiscall, LMemoryChunkBase_Clear, 0x0050E6B0, void* self);

// the import slots
using CriticalSectionFn = void(__stdcall*)(void*);
GAME_VAR(CriticalSectionFn, EnterCriticalSection_, 0x008FC17C);
GAME_VAR(CriticalSectionFn, LeaveCriticalSection_, 0x008FC178);
GAME_VAR(CriticalSectionFn, DeleteCriticalSection_, 0x008FC088);

GAME_VAR(uint8_t*, g_C2B120, 0x00C2B120);            // (its +0x2E0: an action controller)
GAME_VAR(uint8_t, lockButtonInputs, 0x00C1AF8C);     // buttons locked but...
inline const uint8_t* const lockButtonExcept = reinterpret_cast<const uint8_t*>(0x00C1AF8D); // ...these ([button])
GAME_VAR(uint8_t*, g_20C88D0, 0x020C88D0);           // +0x18: a node per button

inline void* const theUserInputManager = reinterpret_cast<void*>(0x00A45A30);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const theCheatCodeManager = reinterpret_cast<void*>(0x009E5158);
inline void* const theMenuSystem = reinterpret_cast<void*>(0x00BA4FF8);

constexpr uintptr_t kUserInputManagerVtable = 0x00914800;
constexpr uintptr_t kUserInputHandlerVtable = 0x00914860;
constexpr uintptr_t kCameraUserInputHandlerVtable = 0x0091480C;
constexpr uintptr_t kCheatCodeUserInputHandlerVtable = 0x00914818;
constexpr uintptr_t kDebugMenuUserInputHandlerVtable = 0x00914824;
constexpr uintptr_t kDemoModeUserInputHandlerVtable = 0x00914830;
constexpr uintptr_t kMenuSystemUserInputHandlerVtable = 0x0091483C;
constexpr uintptr_t kPlayerUserInputHandlerVtable = 0x00914854;
constexpr uintptr_t kLMemoryChunkBaseVtable = 0x00914964;

// a pad event (as in Cheats.cpp)
struct PadEvent {
    int32_t unknown0;  // +0
    int32_t pad;       // +4
    float value;       // +8
    int32_t button;    // +0xC
};
static_assert(sizeof(PadEvent) == 0x10);

struct UserInputHandler {
    uint32_t vtable;  // +0
};

struct PlayerUserInputHandler {
    uint32_t vtable;    // +0
    uint8_t unknown4;    // +4
    uint8_t pad5[3];
};
static_assert(sizeof(PlayerUserInputHandler) == 8);

// A list of allocated chunks (each starts with the next one's pointer)
struct LMemoryChunkBase {
    uint32_t vtable;    // +0
    re::Ptr32<re::Ptr32<void>> first;        // +4
    re::Ptr32<re::Ptr32<void>> current;      // +8
    int32_t unknownC;    // +0xC
    int32_t unknown10;   // +0x10
    uint8_t keepFirst;   // +0x14 clearing keeps the first chunk
    uint8_t unknown15[3];
    uint8_t unknown18;   // +0x18
    uint8_t unknown19[3];
    int32_t unknown1C;   // +0x1C
    uint8_t lock[0x18];  // +0x20 a CRITICAL_SECTION (in the locked kind)
};
static_assert(offsetof(LMemoryChunkBase, keepFirst) == 0x14);
static_assert(offsetof(LMemoryChunkBase, unknown1C) == 0x1C);
static_assert(offsetof(LMemoryChunkBase, lock) == 0x20);

} // namespace

// ---- the menus' pad/keyboard tests (stdcall, the pad)

// 0x50E040: button 1, the axis 0x10 below 0, or the right arrow key
uint8_t __fastcall MenuInput_Right(void*, re::Edx, int pad) {
    if (Pad_GetButton(1, pad) != 0.0f) {
        return 1;
    }
    if (Pad_GetAxis(0x10, pad) < -0.0f) {
        return 1;
    }
    return Key_JustDown(0xCD, 0, 1) != 0 ? 1 : 0;
}
INPUTHANDLERS_REVERSED("MenuInput::Right_50E040", 0x0050E040, RE_BODY(&MenuInput_Right), nullptr, nullptr, re::Mode::Original);

// 0x50E0A0: buttons 2 or 0xB, the axis 0x11 above 0, or the up arrow key
uint8_t __fastcall MenuInput_Up(void*, re::Edx, int pad) {
    if (Pad_GetButton(2, pad) != 0.0f) {
        return 1;
    }
    if (Pad_GetButton(0xB, pad) != 0.0f) {
        return 1;
    }
    if (Pad_GetAxis(0x11, pad) > 0.0f) {
        return 1;
    }
    return Key_JustDown(0xC8, 0, 1) != 0 ? 1 : 0;
}
INPUTHANDLERS_REVERSED("MenuInput::Up_50E0A0", 0x0050E0A0, RE_BODY(&MenuInput_Up), nullptr, nullptr, re::Mode::Original);

// 0x50E110: buttons 3 or 0xD, the axis 0x11 below 0, or the down arrow key
uint8_t __fastcall MenuInput_Down(void*, re::Edx, int pad) {
    if (Pad_GetButton(3, pad) != 0.0f) {
        return 1;
    }
    if (Pad_GetButton(0xD, pad) != 0.0f) {
        return 1;
    }
    if (Pad_GetAxis(0x11, pad) < -0.0f) {
        return 1;
    }
    return Key_JustDown(0xD0, 0, 1) != 0 ? 1 : 0;
}
INPUTHANDLERS_REVERSED("MenuInput::Down_50E110", 0x0050E110, RE_BODY(&MenuInput_Down), nullptr, nullptr, re::Mode::Original);

// 0x50E180: buttons 7 or 0xC just pressed, or enter
uint8_t __fastcall MenuInput_AcceptDown(void*, re::Edx, int pad) {
    if (Pad_ButtonJustDown(7, pad) || Pad_ButtonJustDown(0xC, pad)) {
        return 1;
    }
    return Key_JustDown(0x1C, 0, 1) != 0 ? 1 : 0;
}
INPUTHANDLERS_REVERSED("MenuInput::AcceptDown_50E180", 0x0050E180, RE_BODY(&MenuInput_AcceptDown), nullptr, nullptr, re::Mode::Original);

// 0x50E1C0: the same, released
uint8_t __fastcall MenuInput_AcceptUp(void*, re::Edx, int pad) {
    if (Pad_ButtonJustUp(7, pad) || Pad_ButtonJustUp(0xC, pad)) {
        return 1;
    }
    return Key_JustUp(0x1C, 0, 1) != 0 ? 1 : 0;
}
INPUTHANDLERS_REVERSED("MenuInput::AcceptUp_50E1C0", 0x0050E1C0, RE_BODY(&MenuInput_AcceptUp), nullptr, nullptr, re::Mode::Original);

// 0x50E200: buttons 8 or 0xA just pressed
uint8_t __fastcall MenuInput_BackDown(void*, re::Edx, int pad) {
    if (Pad_ButtonJustDown(8, pad) || Pad_ButtonJustDown(0xA, pad)) {
        return 1;
    }
    return 0;
}
INPUTHANDLERS_REVERSED("MenuInput::BackDown_50E200", 0x0050E200, RE_BODY(&MenuInput_BackDown), nullptr, nullptr, re::Mode::Original);

// ---- the handlers

// 0x50E230 (vtable[0]): (only the vtable: the handlers aren't deleted here)
void* __fastcall UserInputManager_Delete(UserInputHandler* self, re::Edx, int flags) {
    self->vtable = kUserInputManagerVtable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
INPUTHANDLERS_REVERSED("UserInputManager::vf0_50E230", 0x0050E230, RE_BODY(&UserInputManager_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50E560
void* __fastcall UserInputHandler_ConstructOurs(UserInputHandler* self, re::Edx) {
    self->vtable = kUserInputHandlerVtable;
    return self;
}
INPUTHANDLERS_REVERSED("UserInputHandler::UserInputHandler_50E560", 0x0050E560, RE_BODY(&UserInputHandler_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E570
void __fastcall UserInputHandler_DestructOurs(UserInputHandler* self, re::Edx) {
    self->vtable = kUserInputHandlerVtable;
}
INPUTHANDLERS_REVERSED("UserInputHandler::~UserInputHandler_50E570", 0x0050E570, RE_BODY(&UserInputHandler_DestructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E580 (vtable[0])
void* __fastcall UserInputHandler_Delete(UserInputHandler* self, re::Edx, int flags) {
    self->vtable = kUserInputHandlerVtable;
    if (flags & 1) {
        Free(self);
    }
    return self;
}
INPUTHANDLERS_REVERSED("UserInputHandler::vf0_50E580", 0x0050E580, RE_BODY(&UserInputHandler_Delete), nullptr, nullptr, re::Mode::Original);

namespace {

void* ConstructHandler(UserInputHandler* self, uintptr_t vtable) {
    UserInputHandler_Construct(self);
    self->vtable = vtable;
    return self;
}

void* DeleteHandler(UserInputHandler* self, uintptr_t vtable, int flags) {
    self->vtable = vtable;
    UserInputHandler_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

} // namespace

// 0x50E250
void* __fastcall CameraUserInputHandler_ConstructOurs(UserInputHandler* self, re::Edx) {
    return ConstructHandler(self, kCameraUserInputHandlerVtable);
}
INPUTHANDLERS_REVERSED("CameraUserInputHandler::CameraUserInputHandler_50E250", 0x0050E250, RE_BODY(&CameraUserInputHandler_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E270 (vtable[1]): the event to the camera
uint8_t __fastcall CameraUserInputHandler_OnEvent(UserInputHandler*, re::Edx, PadEvent* e) {
    return Camera_OnPadEvent(theCamera, e) != 0 ? 1 : 0;
}
INPUTHANDLERS_REVERSED("CameraUserInputHandler::vf1_50E270", 0x0050E270, RE_BODY(&CameraUserInputHandler_OnEvent), nullptr, nullptr, re::Mode::Original);

// 0x50E290 (vtable[0])
void* __fastcall CameraUserInputHandler_Delete(UserInputHandler* self, re::Edx, int flags) {
    return DeleteHandler(self, kCameraUserInputHandlerVtable, flags);
}
INPUTHANDLERS_REVERSED("CameraUserInputHandler::vf0_50E290", 0x0050E290, RE_BODY(&CameraUserInputHandler_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50E2C0
void* __fastcall CheatCodeUserInputHandler_ConstructOurs(UserInputHandler* self, re::Edx) {
    return ConstructHandler(self, kCheatCodeUserInputHandlerVtable);
}
INPUTHANDLERS_REVERSED("CheatCodeUserInputHandler::CheatCodeUserInputHandler_50E2C0", 0x0050E2C0, RE_BODY(&CheatCodeUserInputHandler_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E2E0 (vtable[1]): the event to the cheat code manager
uint8_t __fastcall CheatCodeUserInputHandler_OnEvent(UserInputHandler*, re::Edx, PadEvent* e) {
    return CheatCodeManager_OnButton(theCheatCodeManager, e);
}
INPUTHANDLERS_REVERSED("CheatCodeUserInputHandler::vf1_50E2E0", 0x0050E2E0, RE_BODY(&CheatCodeUserInputHandler_OnEvent), nullptr, nullptr, re::Mode::Original);

// 0x50E2F0 (vtable[0])
void* __fastcall CheatCodeUserInputHandler_Delete(UserInputHandler* self, re::Edx, int flags) {
    return DeleteHandler(self, kCheatCodeUserInputHandlerVtable, flags);
}
INPUTHANDLERS_REVERSED("CheatCodeUserInputHandler::vf0_50E2F0", 0x0050E2F0, RE_BODY(&CheatCodeUserInputHandler_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50E320
void* __fastcall DebugMenuUserInputHandler_ConstructOurs(UserInputHandler* self, re::Edx) {
    return ConstructHandler(self, kDebugMenuUserInputHandlerVtable);
}
INPUTHANDLERS_REVERSED("DebugMenuUserInputHandler::DebugMenuUserInputHandler_50E320", 0x0050E320, RE_BODY(&DebugMenuUserInputHandler_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E340 (vtable[0])
void* __fastcall DebugMenuUserInputHandler_Delete(UserInputHandler* self, re::Edx, int flags) {
    return DeleteHandler(self, kDebugMenuUserInputHandlerVtable, flags);
}
INPUTHANDLERS_REVERSED("DebugMenuUserInputHandler::vf0_50E340", 0x0050E340, RE_BODY(&DebugMenuUserInputHandler_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50E370
void* __fastcall DemoModeUserInputHandler_ConstructOurs(UserInputHandler* self, re::Edx) {
    return ConstructHandler(self, kDemoModeUserInputHandlerVtable);
}
INPUTHANDLERS_REVERSED("DemoModeUserInputHandler::DemoModeUserInputHandler_50E370", 0x0050E370, RE_BODY(&DemoModeUserInputHandler_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E390 (vtable[0])
void* __fastcall DemoModeUserInputHandler_Delete(UserInputHandler* self, re::Edx, int flags) {
    return DeleteHandler(self, kDemoModeUserInputHandlerVtable, flags);
}
INPUTHANDLERS_REVERSED("DemoModeUserInputHandler::vf0_50E390", 0x0050E390, RE_BODY(&DemoModeUserInputHandler_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50E3C0
void* __fastcall MenuSystemUserInputHandler_ConstructOurs(UserInputHandler* self, re::Edx) {
    return ConstructHandler(self, kMenuSystemUserInputHandlerVtable);
}
INPUTHANDLERS_REVERSED("MenuSystemUserInputHandler::MenuSystemUserInputHandler_50E3C0", 0x0050E3C0, RE_BODY(&MenuSystemUserInputHandler_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E3E0 (vtable[1]): the event to the menu system (whose handler takes none: 0)
uint8_t __fastcall MenuSystemUserInputHandler_OnEvent(UserInputHandler*, re::Edx, PadEvent* e) {
    return MenuSystem_OnPadEvent(theMenuSystem, e);
}
INPUTHANDLERS_REVERSED("MenuSystemUserInputHandler::vf1_50E3E0", 0x0050E3E0, RE_BODY(&MenuSystemUserInputHandler_OnEvent), nullptr, nullptr, re::Mode::Original);

// 0x50E3F0 (vtable[0])
void* __fastcall MenuSystemUserInputHandler_Delete(UserInputHandler* self, re::Edx, int flags) {
    return DeleteHandler(self, kMenuSystemUserInputHandlerVtable, flags);
}
INPUTHANDLERS_REVERSED("MenuSystemUserInputHandler::vf0_50E3F0", 0x0050E3F0, RE_BODY(&MenuSystemUserInputHandler_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50E420
void* __fastcall PlayerUserInputHandler_ConstructOurs(PlayerUserInputHandler* self, re::Edx) {
    UserInputHandler_Construct(self);
    self->vtable = kPlayerUserInputHandlerVtable;
    self->unknown4 = 0;
    return self;
}
INPUTHANDLERS_REVERSED("PlayerUserInputHandler::PlayerUserInputHandler_50E420", 0x0050E420, RE_BODY(&PlayerUserInputHandler_ConstructOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E440 (vtable[1]): the active pad's buttons to the action controller at
// g_C2B120+0x2E0 (mode 3 on a release, 2 on a press; a locked button's node is
// played in mode 3 with its values at FLT_MAX). Never takes the event (0)
uint8_t __fastcall PlayerUserInputHandler_OnEvent(PlayerUserInputHandler*, re::Edx, PadEvent* e) {
    if (e->pad != Pad_GetActivePad()) {
        return 0;
    }
    uint8_t* object = g_C2B120;
    if (object == nullptr) {
        return 0;
    }
    auto controller = [object] { return *reinterpret_cast<re::Ptr32<void>*>(object + 0x2E0); };
    if (!UserInputManager_IsEnabled(theUserInputManager)) {
        ActionController_Clear(controller());
        return 0;
    }
    re::Ptr32<void>* nodes = reinterpret_cast<re::Ptr32<void>*>(g_20C88D0 + 0x18);
    if (lockButtonInputs != 0 && lockButtonExcept[e->button] == 0) {
        ActionController_Play(controller(), nodes[e->button], 3, 0.0f);
        uint8_t* entry = ActionController_Get(controller(), nodes[e->button]);
        *reinterpret_cast<float*>(entry + 4) = 3.4028234663852886e+38f;
        *reinterpret_cast<float*>(entry + 8) = 3.4028234663852886e+38f;
        return 0;
    }
    int mode = 2;
    if (0.0f == e->value) {
        mode = 3;
    }
    ActionController_Play(controller(), nodes[e->button], mode, 0.0f);
    return 0;
}
INPUTHANDLERS_REVERSED("PlayerUserInputHandler::vf1_50E440", 0x0050E440, RE_BODY(&PlayerUserInputHandler_OnEvent), nullptr, nullptr, re::Mode::Original);

// 0x50E530 (vtable[0])
void* __fastcall PlayerUserInputHandler_Delete(UserInputHandler* self, re::Edx, int flags) {
    return DeleteHandler(self, kPlayerUserInputHandlerVtable, flags);
}
INPUTHANDLERS_REVERSED("PlayerUserInputHandler::vf0_50E530", 0x0050E530, RE_BODY(&PlayerUserInputHandler_Delete), nullptr, nullptr, re::Mode::Original);

// 0x50E5A0: makes a handler by kind (0 demo mode, 1 debug menu, 2 cheat codes, 3 menu
// system, 4 player, 5 camera; nullptr otherwise or when out of memory)
void* __cdecl UserInputHandler_Create(int kind) {
    void* p;
    switch (kind) {
    case 0:
        p = Malloc(4);
        return p ? DemoModeUserInputHandler_Construct(p) : nullptr;
    case 1:
        p = Malloc(4);
        return p ? DebugMenuUserInputHandler_Construct(p) : nullptr;
    case 2:
        p = Malloc(4);
        return p ? CheatCodeUserInputHandler_Construct(p) : nullptr;
    case 3:
        p = Malloc(4);
        return p ? MenuSystemUserInputHandler_Construct(p) : nullptr;
    case 4:
        p = Malloc(8);
        return p ? PlayerUserInputHandler_Construct(p) : nullptr;
    case 5:
        p = Malloc(4);
        return p ? CameraUserInputHandler_Construct(p) : nullptr;
    default:
        return nullptr;
    }
}
INPUTHANDLERS_REVERSED("UserInputHandler::Create_50E5A0", 0x0050E5A0, &UserInputHandler_Create, nullptr, nullptr, re::Mode::Original);

// ---- LDefaultAllocator (thiscall, `this` unused)

// 0x50E650 (vtable[0])
void* __fastcall LDefaultAllocator_Alloc(void*, re::Edx, size_t size) {
    return Malloc(size);
}
INPUTHANDLERS_REVERSED("LDefaultAllocator::vf0_50E650", 0x0050E650, RE_BODY(&LDefaultAllocator_Alloc), nullptr, nullptr, re::Mode::Original);

// 0x50E660 (vtable[2])
void __fastcall LDefaultAllocator_Free(void*, re::Edx, void* p) {
    Free(p);
}
INPUTHANDLERS_REVERSED("LDefaultAllocator::vf2_50E660", 0x0050E660, RE_BODY(&LDefaultAllocator_Free), nullptr, nullptr, re::Mode::Original);

// 0x50E670 (vtable[1]): realloc (no check that the allocation worked)
void* __fastcall LDefaultAllocator_Realloc(void*, re::Edx, void* p, int oldSize, int newSize) {
    void* q = Malloc(size_t(newSize));
    int size = oldSize < newSize ? oldSize : newSize;
    Memcpy(q, p, size_t(size));
    Free(p);
    return q;
}
INPUTHANDLERS_REVERSED("LDefaultAllocator::vf1_50E670", 0x0050E670, RE_BODY(&LDefaultAllocator_Realloc), nullptr, nullptr, re::Mode::Original);

// ---- LMemoryChunkBase<LMemoryChunkDescMemAllocatorDefault>

// 0x50E6B0: frees the chunks (all, or all but the first when keepFirst is set)
void __fastcall LMemoryChunkBase_ClearOurs(LMemoryChunkBase* self, re::Edx) {
    re::Ptr32<void>* chunk;
    if (self->keepFirst != 0 && self->first != nullptr) {
        chunk = static_cast<re::Ptr32<void>*>(*self->first);
        *self->first = nullptr;
        self->current = self->first;
        self->unknown10 = self->unknownC;
    } else {
        chunk = self->first;
        self->current = nullptr;
        self->first = nullptr;
        self->unknown10 = 0;
    }
    while (chunk != nullptr) {
        re::Ptr32<void>* next = static_cast<re::Ptr32<void>*>(*chunk);
        Free(chunk);
        chunk = next;
    }
}
INPUTHANDLERS_REVERSED("LMemoryChunkBase::Clear_50E6B0", 0x0050E6B0, RE_BODY(&LMemoryChunkBase_ClearOurs), nullptr, nullptr, re::Mode::Original);

// 0x50E710: the destructor of something holding a locked chunk list at +4 (cleared
// twice under the lock, then as the base class again)
void __fastcall LockedMemoryChunks_Destruct(uint8_t* self, re::Edx) {
    LMemoryChunkBase* chunks = reinterpret_cast<LMemoryChunkBase*>(self + 4);
    EnterCriticalSection_(chunks->lock);
    LMemoryChunkBase_Clear(chunks);
    chunks->unknown1C = 0;
    LeaveCriticalSection_(chunks->lock);
    EnterCriticalSection_(chunks->lock);
    LMemoryChunkBase_Clear(chunks);
    chunks->unknown1C = 0;
    LeaveCriticalSection_(chunks->lock);
    DeleteCriticalSection_(chunks->lock);
    chunks->vtable = kLMemoryChunkBaseVtable;
    chunks->keepFirst = 0;
    LMemoryChunkBase_Clear(chunks);
}
INPUTHANDLERS_REVERSED("LockedMemoryChunks::Destruct_50E710", 0x0050E710, RE_BODY(&LockedMemoryChunks_Destruct), nullptr, nullptr, re::Mode::Original);

// 0x50E770 (vtable[0])
void* __fastcall LMemoryChunkBase_Delete(LMemoryChunkBase* self, re::Edx, int flags) {
    self->vtable = kLMemoryChunkBaseVtable;
    self->keepFirst = 0;
    LMemoryChunkBase_Clear(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
INPUTHANDLERS_REVERSED("LMemoryChunkBase::vf0_50E770", 0x0050E770, RE_BODY(&LMemoryChunkBase_Delete), nullptr, nullptr, re::Mode::Original);

// ---- CBaseModelInfo's defaults

// 0x50EB50 (vtable[10]): -1 (4 bytes: too short to hook)
int32_t __fastcall CBaseModelInfo_Vf10(void*, re::Edx) {
    return -1;
}
RE_UNHOOKED_FN("CBaseModelInfo::vf10_50EB50", 0x0050EB50, &CBaseModelInfo_Vf10);

// 0x50EB60 (vtable[11]): -1 as a 16-bit value (the game's "or ax, 0xFFFF" leaves
// eax's high half as it was)
int16_t __fastcall CBaseModelInfo_Vf11(void*, re::Edx) {
    return -1;
}
INPUTHANDLERS_REVERSED("CBaseModelInfo::vf11_50EB60", 0x0050EB60, RE_BODY(&CBaseModelInfo_Vf11), nullptr, nullptr, re::Mode::Original);
