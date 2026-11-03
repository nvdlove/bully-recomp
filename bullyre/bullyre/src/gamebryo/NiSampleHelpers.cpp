// The NiSample framework's small functions at the end of NiApplication's range (0x830D40..
// 0x830F20, before NiSample::Initialize): NiSample's message box wrapper and its two slot
// helpers, the navigation controllers' up direction setters and three NiUIManager lookups.
// Types from src/gamebryo/NiSampleFramework.h (the chunk 34 pass's, used read-only).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::sample {
namespace {

constexpr uintptr_t kShowCursor = 0x008FC244;   // USER32 ShowCursor's import slot

// the message box function NiSample replaced (it returns the button pressed)
using PreviousMessageBoxFn = uint32_t(__cdecl*)(const char* text, const char* caption, uint32_t flags);
GAME_VAR(PreviousMessageBoxFn, ms_pfnPreviousMessageBox, 0x020E5A00);

GAME_FN(float, __thiscall, Point3_Unitize, 0x00415130, NiPoint3* self);

// 0x830D40 (not in the chunk's list: reached only through NiMessageBoxFunction, which NiSample
// sets while it makes the input system): the cursor shown around the previous message box when
// the mouse is exclusive (NiApplication's slot 30, IsExclusiveMouse)
uint32_t __cdecl SampleMessageBox(const char* text, const char* caption, uint32_t flags) {
    uint8_t exclusive = Virtual<uint8_t>(NiApplication_ms_pkApplication, 30);
    if (exclusive) {
        sys::Import<int>(kShowCursor, 1);
    }
    uint32_t result = ms_pfnPreviousMessageBox(text, caption, flags);
    if (exclusive) {
        sys::Import<int>(kShowCursor, 0);
    }
    return result;
}

// 0x830DA0: the member slot's function for &NiSample::CycleNavigationController (the
// compiler's vcall thunk: a jump through slot 58, so the virtual is called as it stands)
void __fastcall Sample_CycleNavigationThunk(NiSample* self, re::Edx) {
    Virtual<void>(self, 58);
}

// the up direction set (made unit length unless it's zero)
inline void SetUnitUp(NiPoint3* up, const NiPoint3* dir) {
    *up = *dir;
    float length = crt::Sqrtf(up->y * up->y + up->x * up->x + up->z * up->z);
    if (!(length == 0.0f)) {
        Point3_Unitize(up);
    }
}

// 0x830DB0: NiNavFlyController::SetUpDir(dir)
void __fastcall Fly_SetUpDir(NiNavFlyController* self, re::Edx, const NiPoint3* dir) {
    SetUnitUp(&self->upDir, dir);
}

// 0x830E20: NiNavOrbitController::SetUpDir(dir)
void __fastcall Orbit_SetUpDir(NiNavOrbitController* self, re::Edx, const NiPoint3* dir) {
    SetUnitUp(&self->upDir, dir);
}

// 0x830E90: NiUIManager::FindGamePadBinding(button, modifiers): the reserved button's index
// (the axes skipped), or -1
int32_t __fastcall UIManager_FindGamePadButton(NiUIManager* self, re::Edx, uint32_t button, uint32_t modifiers) {
    re::Ptr32<NiUIGamePadBinding>* bindings = self->gamePadBindings.base;
    for (uint32_t i = 0; i < self->gamePadBindings.used; i++) {
        NiUIGamePadBinding* binding = bindings[i];
        if (binding->isButton && binding->button == button && binding->modifiers == modifiers) {
            return int32_t(i);
        }
    }
    return -1;
}

// 0x830EE0: NiUIManager::FindKeyboardBinding(key, modifiers): the reserved key's index, or -1
int32_t __fastcall UIManager_FindKeyboardKey(NiUIManager* self, re::Edx, uint32_t key, uint32_t modifiers) {
    re::Ptr32<NiUIKeyBinding>* bindings = self->keyboardBindings.base;
    for (uint32_t i = 0; i < self->keyboardBindings.used; i++) {
        NiUIKeyBinding* binding = bindings[i];
        if (binding->key == key && binding->modifiers == modifiers) {
            return int32_t(i);
        }
    }
    return -1;
}

// 0x830F20: NiUIManager::SetFocusedGroup(index): the focus moved from the old group (slot 6,
// SetFocus) to that one (none checked: the index is trusted); the group
NiUIRenderGroup* __fastcall UIManager_SetFocusedGroup_(NiUIManager* self, re::Edx, uint32_t index) {
    int32_t current = self->focusedGroup;
    if (int32_t(index) == current) {
        return self->groups.base[current];
    }
    if (current != -1) {
        Virtual<uint8_t>(self->groups.base[current], 6, 0);
    }
    self->focusedGroup = int32_t(index);
    NiUIRenderGroup* group = self->groups.base[index];
    Virtual<uint8_t>(group, 6, 1);
    return group;
}

}   // namespace
}   // namespace ni::sample

// ---- registration ---------------------------------------------------------------------

#define C33_SAMPLEHELP_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

C33_SAMPLEHELP_REVERSED("NiSample::MessageBox", 0x00830D40, &ni::sample::SampleMessageBox);
// 0x830D90 (NiSample's slot 29, OnDefault): a lone 5-byte jmp to a folded "return false" (0x609650,
// ret 4): too small to hook past
RE_UNHOOKED("NiSample::OnDefault (thunk to 0x609650)", 0x00830D90);
C33_SAMPLEHELP_REVERSED("NiSample::CycleNavigationController (vcall thunk)", 0x00830DA0,
                        &ni::sample::Sample_CycleNavigationThunk);
C33_SAMPLEHELP_REVERSED("NiNavFlyController::SetUpDir", 0x00830DB0, &ni::sample::Fly_SetUpDir);
C33_SAMPLEHELP_REVERSED("NiNavOrbitController::SetUpDir", 0x00830E20, &ni::sample::Orbit_SetUpDir);
C33_SAMPLEHELP_REVERSED("NiUIManager::FindGamePadBinding", 0x00830E90, &ni::sample::UIManager_FindGamePadButton);
C33_SAMPLEHELP_REVERSED("NiUIManager::FindKeyboardBinding", 0x00830EE0, &ni::sample::UIManager_FindKeyboardKey);
C33_SAMPLEHELP_REVERSED("NiUIManager::SetFocusedGroup", 0x00830F20, &ni::sample::UIManager_SetFocusedGroup_);
