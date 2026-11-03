// 0x409520 to 0x4099F0: the renderer's frame (BeginFrame, EndFrame, DisplayFrame and the
// render target groups, driven by its +0x94 state: 0 none, 1 in a frame, 2 in an offscreen
// frame, 3 ended, 4 ...), as the game calls them through the global renderer (0x20E2380).

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
GAME_VAR(uint8_t*, ms_pRenderTargetAC, 0x00BCBBAC);
GAME_VAR(uint8_t*, ms_pRenderTargetB0, 0x00BCBBB0);
GAME_VAR(uint8_t, ms_bClearNothing_BCBB53, 0x00BCBB53);
GAME_FN(uint8_t, __thiscall, Renderer_BeginUsingRenderTargetGroup, 0x00407890, void* self, void* group, uint32_t flags);
GAME_FN(uint8_t, __thiscall, Renderer_EndUsingRenderTargetGroup, 0x00407990, void* self);
GAME_FN(void, __cdecl, Fn_5EE1D0, 0x005EE1D0);
GAME_FN(void, __cdecl, SetRenderState_519790, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, NiTArray_SetSize, 0x0084A110, void* self, uint32_t size);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* self, uint32_t index, void* item);
GAME_FN(void, __thiscall, PointerList_AddHead, 0x00407BD0, void* list, const re::Ptr32<uint8_t>* element);

int32_t& State(uint8_t* r) {
    return *reinterpret_cast<int32_t*>(r + 0x94);
}

bool InFrame(uint8_t* r) {
    int32_t s = State(r);
    return s == 1 || s == 2 || s == 4;
}

void AddRef(uint8_t* object) {
    if (object) {
        vc::Increment(object + 4);
    }
}

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

} // namespace

namespace RendererFrame {

// 0x409520: the scene's render targets: the back buffer with the depth buffer of 0xBCBBB0,
// then the group at 0xBCBBAC begun (cleared per `flags` unless 0xBCBB53 says not), and the
// render states 6, 8 (off) and 0x14 (on)
static void __cdecl BeginSceneTargets(uint32_t flags, int useDefault) {
    uint8_t* r = ms_pRenderer;
    if (!InFrame(r) || r[0x90]) {
        return;
    }
    uint8_t* held = nullptr;
    bool holding = false;
    if (re::Flag(useDefault)) {
        vc::Call<void*>(r, 0x74);   // (its result unused)
    } else {
        held = ms_pRenderTargetAC;
        holding = true;
        AddRef(held);
    }
    if (holding) {
        Release(held);
    }
    uint8_t* target = ms_pRenderTargetB0;
    AddRef(target);
    void* group = vc::Call<void*>(ms_pRenderer, 0x74);
    void* buffer = vc::Call<void*>(group, 0x6C, 0);
    void* depth = vc::Call<void*>(target, 0x50);
    vc::Call<void>(ms_pRenderer, 0xE8, buffer, depth, 0, 0, 0);
    if (vc::Decrement(target + 4) == 0) {   // (not checked for null)
        vc::Call<void>(target, 0x04);
    }
    uint8_t* scene = ms_pRenderTargetAC;
    AddRef(scene);
    Renderer_BeginUsingRenderTargetGroup(ms_pRenderer, scene, ms_bClearNothing_BCBB53 ? 0 : flags);
    Release(scene);
    Fn_5EE1D0();
    SetRenderState_519790(6, 0);
    SetRenderState_519790(8, 0);
    SetRenderState_519790(0x14, 1);
}

// 0x4096C0: EndUsingRenderTargetGroup, when one is in use
static void __cdecl EndTargets() {
    uint8_t* r = ms_pRenderer;
    if (InFrame(r) && r[0x90]) {
        Renderer_EndUsingRenderTargetGroup(r);
    }
}

// 0x4096F0: BeginFrame (vtable[82]) into state 1
static void __cdecl BeginFrame() {
    uint8_t* r = ms_pRenderer;
    if (State(r) == 0 && vc::Call<uint8_t>(r, 0x148)) {
        State(r) = 1;
    }
}

// 0x409720: BeginFrame into state 2 (offscreen)
static void __cdecl BeginOffscreenFrame() {
    uint8_t* r = ms_pRenderer;
    if (State(r) == 0 && vc::Call<uint8_t>(r, 0x148)) {
        State(r) = 2;
    }
}

// 0x409750: EndFrame (vtable[83]) from state 1: 3 when it worked, else 0
static void __cdecl EndFrame() {
    uint8_t* r = ms_pRenderer;
    if (State(r) == 1) {
        State(r) = vc::Call<uint8_t>(r, 0x14C) ? 3 : 0;
    }
}

// 0x409790: EndFrame from state 2 (offscreen: nothing to display)
static void __cdecl EndOffscreenFrame() {
    uint8_t* r = ms_pRenderer;
    if (State(r) == 2) {
        vc::Call<uint8_t>(r, 0x14C);
        State(r) = 0;
    }
}

// 0x4097D0: DisplayFrame (vtable[84]) from state 3; +0x9C counts the frames shown
static void __cdecl DisplayFrame() {
    uint8_t* r = ms_pRenderer;
    if (State(r) == 3) {
        uint8_t ok = vc::Call<uint8_t>(r, 0x150);
        State(r) = 0;
        if (ok) {
            (*reinterpret_cast<int32_t*>(r + 0x9C))++;
        }
    }
}

// 0x409810: ClearBuffer (vtable[85]: 0, flags) with a group in use
static void __cdecl ClearBuffers(uint32_t flags) {
    uint8_t* r = ms_pRenderer;
    if (InFrame(r) && r[0x90] && (r[0x90] == 1 || State(r) == 4)) {
        vc::Call<void>(r, 0x154, 0, flags);
    }
}

// 0x409860: BeginFrame, then the default target group begun (clearing all: 7)
static void __cdecl BeginFrameAndTargets() {
    uint8_t* r = ms_pRenderer;
    if (State(r) == 0 && vc::Call<uint8_t>(r, 0x148)) {
        State(r) = 1;
    }
    r = ms_pRenderer;
    if (!r[0x90] && InFrame(r)) {
        void* group = vc::Call<void*>(r, 0x74);
        Renderer_BeginUsingRenderTargetGroup(r, group, 7);
    }
}

// 0x4098D0: the frame ended: the group, EndFrame, DisplayFrame
static void __cdecl EndAndDisplayFrame() {
    uint8_t* r = ms_pRenderer;
    if (InFrame(r)) {
        if (r[0x90]) {
            Renderer_EndUsingRenderTargetGroup(r);
        }
        if (State(r) == 1) {
            State(r) = vc::Call<uint8_t>(r, 0x14C) ? 3 : 0;
        }
    }
    r = ms_pRenderer;
    if (State(r) == 3) {
        uint8_t ok = vc::Call<uint8_t>(r, 0x150);
        State(r) = 0;
        if (ok) {
            (*reinterpret_cast<int32_t*>(r + 0x9C))++;
        }
    }
}

// 0x409960: vtable[87](0) with a group in use
static void __cdecl Fn409960() {
    uint8_t* r = ms_pRenderer;
    if (InFrame(r) && r[0x90] && (r[0x90] == 1 || State(r) == 4)) {
        vc::Call<void>(r, 0x15C, 0);
    }
}

// 0x4099B0 (thiscall): NiTArray::Add: at the used size (grown by +0xE when full);
// returns the index
static uint32_t __fastcall NiTArray_Add(uint8_t* self, re::Edx, void* item) {
    uint16_t size = *reinterpret_cast<uint16_t*>(self + 0xA);
    if (*reinterpret_cast<uint16_t*>(self + 8) <= size) {
        NiTArray_SetSize(self, uint32_t(*reinterpret_cast<uint16_t*>(self + 0xE)) + size);
    }
    NiTArray_SetAt(self, size, item);
    return size;
}

// 0x4099F0 (thiscall): an object added to the head of the list at +0x94 (held by a
// temporary NiPointer)
static void __fastcall AddToList94(uint8_t* self, re::Edx, uint8_t* object) {
    re::Ptr32<uint8_t> held = object;
    AddRef(object);
    PointerList_AddHead(self + 0x94, &held);
    Release(object);
}

} // namespace RendererFrame

// ---- registration ----------------------------------------------------------------------

#define C00_FRAMES_REVERSED(name, fn, address)                                                      \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace RendererFrame {
C00_FRAMES_REVERSED("RendererFrame::BeginSceneTargets", BeginSceneTargets, 0x00409520);
C00_FRAMES_REVERSED("RendererFrame::EndTargets", EndTargets, 0x004096C0);
C00_FRAMES_REVERSED("RendererFrame::BeginFrame", BeginFrame, 0x004096F0);
C00_FRAMES_REVERSED("RendererFrame::BeginOffscreenFrame", BeginOffscreenFrame, 0x00409720);
C00_FRAMES_REVERSED("RendererFrame::EndFrame", EndFrame, 0x00409750);
C00_FRAMES_REVERSED("RendererFrame::EndOffscreenFrame", EndOffscreenFrame, 0x00409790);
C00_FRAMES_REVERSED("RendererFrame::DisplayFrame", DisplayFrame, 0x004097D0);
C00_FRAMES_REVERSED("RendererFrame::ClearBuffers", ClearBuffers, 0x00409810);
C00_FRAMES_REVERSED("RendererFrame::BeginFrameAndTargets", BeginFrameAndTargets, 0x00409860);
C00_FRAMES_REVERSED("RendererFrame::EndAndDisplayFrame", EndAndDisplayFrame, 0x004098D0);
C00_FRAMES_REVERSED("RendererFrame::Fn409960", Fn409960, 0x00409960);
C00_FRAMES_REVERSED("NiTArray::Add", NiTArray_Add, 0x004099B0);
C00_FRAMES_REVERSED("BullyMain::AddToList94", AddToList94, 0x004099F0);
} // namespace RendererFrame
