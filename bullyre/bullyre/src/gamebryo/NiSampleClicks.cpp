// The sample framework's render clicks: NiCursorRenderClick (vtable 0x988FD4, 0x64: a render
// click drawing its list of cursors; NiRTTI 0x20E9344) and NiUIManagerRenderClick (vtable
// 0x9893F8, 0x50: draws the UI manager's groups and times it; NiRTTI 0x20E5A40). Both on
// NiRenderClick (0x4C, src/gamebryo/NiRenderClick.cpp). NiCursorRenderClick's constructor is
// inlined in NiSample's (0x832A20); its other slots are the shared NiViewRenderClick ones.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSampleFramework.h"
#include "reversed.h"

namespace ni::sample {
namespace {

GAME_FN(void, __thiscall, RenderClick_Construct, 0x00515C00, void* self);
GAME_FN(void, __thiscall, RenderClick_Destruct, 0x00515CA0, void* self);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

constexpr uint32_t kNiTPointerListCursorVtable = 0x0095127C;
constexpr uint32_t kNiTPointerListBaseVtable = 0x0099C748;
constexpr uint32_t kNiUIManagerRenderClickVtable = 0x009893F8;
constexpr uintptr_t kNiCursorRenderClickRTTI = 0x020E9344;
constexpr uintptr_t kNiUIManagerRenderClickRTTI = 0x020E5A40;

// NiCursorRenderClick::GetRTTI
const void* __fastcall CursorClick_GetRTTI(const void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiCursorRenderClickRTTI);
}

// NiCursorRenderClick's deleting destructor: the cursor list emptied, then NiRenderClick's
void* __fastcall CursorClick_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    uint8_t* list = self + 0x4C;
    At<uint32_t>(list, 0) = kNiTPointerListCursorVtable;
    PointerList_RemoveAll(list);
    At<uint32_t>(list, 0) = kNiTPointerListBaseVtable;
    RenderClick_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x64);
    }
    return self;
}

// NiUIManagerRenderClick::GetRenderTime (4 bytes: unhooked)
float __fastcall UIClick_GetRenderTime(const uint8_t* self, re::Edx) {
    return At<float>(self, 0x4C);
}
RE_UNHOOKED_FN("NiUIManagerRenderClick::GetRenderTime", 0x00838FC0, &UIClick_GetRenderTime);

// NiUIManagerRenderClick::PerformRendering(frameID): the UI manager's groups drawn, timed
void __fastcall UIClick_PerformRendering(uint8_t* self, re::Edx, uint32_t) {
    NiUIManager* manager = UIManager_Get();
    if (manager == nullptr) {
        return;
    }
    void* renderer = NiRenderer_ms_pkRenderer;
    float start = NiGetCurrentTimeInSec();
    UIManager_Draw(manager, renderer);
    At<float>(self, 0x4C) = NiGetCurrentTimeInSec() - start;
}

// NiUIManagerRenderClick's constructor
uint8_t* __fastcall UIClick_Construct(uint8_t* self, re::Edx) {
    RenderClick_Construct(self);
    At<float>(self, 0x4C) = 0.0f;
    At<uint32_t>(self, 0) = kNiUIManagerRenderClickVtable;
    return self;
}

// NiUIManagerRenderClick::GetRTTI
const void* __fastcall UIClick_GetRTTI(const void*, re::Edx) {
    return reinterpret_cast<const void*>(kNiUIManagerRenderClickRTTI);
}

// NiUIManagerRenderClick's deleting destructor
uint8_t* __fastcall UIClick_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    RenderClick_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x50);
    }
    return self;
}

} // namespace

#define NISAMPLECLICK_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISAMPLECLICK_REVERSED("NiCursorRenderClick::GetRTTI", 0x00832340, &CursorClick_GetRTTI);
NISAMPLECLICK_REVERSED("NiCursorRenderClick::DeletingDestructor", 0x00832350, &CursorClick_DeletingDestructor);
NISAMPLECLICK_REVERSED("NiUIManagerRenderClick::PerformRendering", 0x00838FD0, &UIClick_PerformRendering);
NISAMPLECLICK_REVERSED("NiUIManagerRenderClick::NiUIManagerRenderClick", 0x00839010, &UIClick_Construct);
NISAMPLECLICK_REVERSED("NiUIManagerRenderClick::GetRTTI", 0x00839030, &UIClick_GetRTTI);
NISAMPLECLICK_REVERSED("NiUIManagerRenderClick::DeletingDestructor", 0x00839040, &UIClick_DeletingDestructor);

} // namespace ni::sample
