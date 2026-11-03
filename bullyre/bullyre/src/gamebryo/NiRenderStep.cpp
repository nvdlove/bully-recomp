// NiRenderStep (vtable 0x951F80), NiClickRenderStep (vtable 0x951FA4, 0x2C bytes) with
// NiDefaultClickRenderStep's slots, and NiRenderFrame's Draw / Display.
//
// NiRenderStep: NiRefObject (8), the pre- and post-processing callbacks (+0x8, +0xC) and their
// data (+0x10, +0x14), the name (+0x18, a fixed string: the default name at 0x20E393C), active
// (+0x1C). NiClickRenderStep: objects drawn (+0x20), cull time (+0x24), render time (+0x28).
// Its slots: 3 SetOutputRenderTargetGroup, 4 GetOutputRenderTargetGroup, 7 GetRenderTime (the
// others' sums), 8 Render, 9 GetRenderClicks, 10 PerformRendering.
//
// NiRenderFrame: its render steps' list (+0x8; head +0xC), the pre/post callbacks (+0x18, +0x1C)
// and their data (+0x20, +0x24). The renderer's frame state (+0x94: 1 in a frame, 3 ended,
// 0 displayed) and frame ID (+0x9C).

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiRenderStep_RTTI, 0x020E3908);
GAME_VAR(RTTI, NiClickRenderStep_RTTI, 0x020E27A8);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiRenderStep_vtable = 0x00951F80;
constexpr uintptr_t NiClickRenderStep_vtable = 0x00951FA4;

GAME_VAR(volatile long, NiRefObject_ms_uiObjects, 0x020E2168);
GAME_VAR(NiFixedString, NiRenderStep_ms_kDefaultName, 0x020E393C);
GAME_VAR(uint8_t*, NiRenderer_ms_pkRenderer, 0x020E2380);

using Callback = uint8_t(__cdecl*)(void* owner, void* data);

GAME_FN(void, __thiscall, RenderClick_Render, 0x00776BC0, void* click, uint32_t frameID);
GAME_FN(uint8_t, __thiscall, Renderer_EndUsingRenderTargetGroup, 0x00407990, void* renderer);
// (this chunk's, called where modes apply)
GAME_FN(void*, __thiscall, RenderStep_Construct, 0x00765060, void* self);

struct NiRenderStep : NiRefObject {
    re::Ptr32<uint8_t __cdecl(void* owner, void* data)> preProcessing;   // +0x8
    re::Ptr32<uint8_t __cdecl(void* owner, void* data)> postProcessing;  // +0xC
    re::Ptr32<void> preData;            // +0x10
    re::Ptr32<void> postData;           // +0x14
    re::Ptr32<const char> name;       // +0x18
    uint8_t active;           // +0x1C
    uint8_t pad1D[3];
    uint32_t objectsDrawn;    // +0x20 (NiClickRenderStep's)
    float cullTime;           // +0x24
    float renderTime;         // +0x28
};
static_assert(offsetof(NiRenderStep, name) == 0x18);
static_assert(sizeof(NiRenderStep) == 0x2C);

using Node = NiTListNode<NiObject*>;
using List = NiTPointerList<NiObject*>;

NiRenderStep* __fastcall RenderStepConstruct(NiRenderStep* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    _InterlockedIncrement(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiRenderStep_vtable);
    self->preProcessing = nullptr;
    self->postProcessing = nullptr;
    self->preData = nullptr;
    self->postData = nullptr;
    NiFixedString name = NiRenderStep_ms_kDefaultName;
    if (name != NullFixedString) {
        _InterlockedIncrement(reinterpret_cast<volatile long*>(re::PtrCast<char*>(name) - 8));
        name = NiRenderStep_ms_kDefaultName;
    }
    self->name = name;
    self->active = 1;
    return self;
}

NiRenderStep* __fastcall ClickRenderStepConstruct(NiRenderStep* self, re::Edx) {
    RenderStep_Construct(self);
    self->cullTime = 0.0f;
    self->vtable = reinterpret_cast<void*>(NiClickRenderStep_vtable);
    self->renderTime = 0.0f;
    self->objectsDrawn = 0;
    return self;
}

const RTTI* __fastcall RenderStepGetRTTI(NiRenderStep*, re::Edx) {
    return &NiRenderStep_RTTI;
}

const RTTI* __fastcall ClickRenderStepGetRTTI(NiRenderStep*, re::Edx) {
    return &NiClickRenderStep_RTTI;
}

// the last render click's render target group
uint8_t __fastcall SetOutputRenderTargetGroup(NiRenderStep* self, re::Edx, NiObject* group) {
    List* clicks = Virtual<List*>(self, 9);
    if (clicks->count == 0) {
        return 0;
    }
    re::Ptr32<NiObject>& slot = At<NiObject*>(clicks->tail->element, 0x10);
    if (slot != group) {
        Release(slot);
        slot = group;
        AddRef(group);
    }
    return 1;
}

// ... or the renderer's default one
NiObject* __fastcall GetOutputRenderTargetGroup(NiRenderStep* self, re::Edx) {
    List* clicks = Virtual<List*>(self, 9);
    if (clicks->count == 0) {
        return nullptr;
    }
    NiObject* group = At<NiObject*>(clicks->tail->element, 0x10);
    if (group == nullptr) {
        return Virtual<NiObject*>(NiRenderer_ms_pkRenderer, 29);   // GetDefaultRenderTargetGroup
    }
    return group;
}

// slot 7 (4 bytes: not hooked)
float __fastcall GetRenderTime(NiRenderStep* self, re::Edx) {
    return self->renderTime;
}

// slot 8 Render: PerformRendering(GetRenderClicks())
void __fastcall Render(NiRenderStep* self, re::Edx) {
    List* clicks = Virtual<List*>(self, 9);
    Virtual<void>(self, 10, clicks);
}

// slot 10 PerformRendering(kClicks): the active clicks their validators let through, rendered
// for this frame, their counts and times summed
void __fastcall PerformRendering(NiRenderStep* self, re::Edx, List* clicks) {
    uint32_t frameID = At<uint32_t>(NiRenderer_ms_pkRenderer, 0x9C);
    self->renderTime = 0.0f;
    self->cullTime = 0.0f;
    self->objectsDrawn = 0;
    for (Node* node = clicks->head; node != nullptr;) {
        NiObject* click = node->element;
        node = node->next;
        if ((At<uint8_t>(click, 8) & 1) == 0) {
            continue;
        }
        void* validator = At<void*>(click, 0xC);
        if (validator != nullptr && !Virtual<uint8_t>(validator, 3, click, frameID)) {
            continue;
        }
        RenderClick_Render(click, frameID);
        self->objectsDrawn += Virtual<uint32_t>(click, 3);
        self->cullTime = Virtual<float>(click, 4) + self->cullTime;
        self->renderTime = Virtual<float>(click, 5) + self->renderTime;
    }
}

// NiClickRenderStep's deleting destructor (~NiRenderStep inlined)
NiRenderStep* __fastcall ClickRenderStepDeletingDestructor(NiRenderStep* self, re::Edx, uint32_t flags) {
    FixedStringRelease(&self->name);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    _InterlockedDecrement(&NiRefObject_ms_uiObjects);
    if (flags & 1) {
        NiDelete(self, 0x2C);
    }
    return self;
}

// NiRenderFrame::Draw: the frame begun (unless it is), each active step rendered between its
// callbacks, the frame ended
void __fastcall RenderFrameDraw(void* self, re::Edx) {
    Callback pre = At<Callback>(self, 0x18);
    if (pre != nullptr && !pre(self, At<void*>(self, 0x20))) {
        return;
    }
    uint8_t* renderer = NiRenderer_ms_pkRenderer;
    if (At<uint32_t>(renderer, 0x94) == 0 && Virtual<uint8_t>(renderer, 82)) {   // BeginFrame
        At<uint32_t>(renderer, 0x94) = 1;
    }
    for (Node* node = At<Node*>(self, 0xC); node != nullptr;) {
        NiRenderStep* step = re::PtrCast<NiRenderStep*>(node->element);
        node = node->next;
        if (step->active == 0) {
            continue;
        }
        if (step->preProcessing != nullptr && !step->preProcessing(step, step->preData)) {
            continue;
        }
        Virtual<void>(step, 8);
        if (step->postProcessing != nullptr) {
            step->postProcessing(step, step->postData);
        }
    }
    if (At<uint8_t>(renderer, 0x90) != 0) {
        Renderer_EndUsingRenderTargetGroup(renderer);
    }
    if (At<uint32_t>(renderer, 0x94) == 1) {
        At<uint32_t>(renderer, 0x94) = Virtual<uint8_t>(renderer, 83) ? 3 : 0;   // EndFrame
    }
    Callback post = At<Callback>(self, 0x1C);
    if (post != nullptr) {
        post(self, At<void*>(self, 0x24));
    }
}

// NiRenderFrame::Display: an ended frame shown (the frame ID moves on when it is)
void __cdecl RenderFrameDisplay() {
    uint8_t* renderer = NiRenderer_ms_pkRenderer;
    if (At<uint32_t>(renderer, 0x94) == 3) {
        uint8_t shown = Virtual<uint8_t>(renderer, 84);   // DisplayFrame
        At<uint32_t>(renderer, 0x94) = 0;
        if (shown) {
            At<uint32_t>(renderer, 0x9C) += 1;
        }
    }
}

} // namespace

// 0x00765030: [d9 41 28 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_00765030(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x28);
}
RE_UNHOOKED_FN("NiClickRenderStep::GetRenderTime", 0x00765030, &Tiny_00765030);   // 4 bytes

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTEP_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTEP_REVERSED("NiClickRenderStep::Render", 0x00765040, &ni::Render);
NISTEP_REVERSED("NiRenderStep::NiRenderStep", 0x00765060, &ni::RenderStepConstruct);
NISTEP_REVERSED("NiRenderStep::GetRTTI", 0x007650C0, &ni::RenderStepGetRTTI);
NISTEP_REVERSED("NiClickRenderStep::NiClickRenderStep", 0x007650D0, &ni::ClickRenderStepConstruct);
NISTEP_REVERSED("NiClickRenderStep::GetRTTI", 0x00765100, &ni::ClickRenderStepGetRTTI);
NISTEP_REVERSED("NiClickRenderStep::SetOutputRenderTargetGroup", 0x00765110, &ni::SetOutputRenderTargetGroup);
NISTEP_REVERSED("NiClickRenderStep::GetOutputRenderTargetGroup", 0x00765170, &ni::GetOutputRenderTargetGroup);
NISTEP_REVERSED("NiDefaultClickRenderStep::PerformRendering", 0x007651A0, &ni::PerformRendering);
NISTEP_REVERSED("NiClickRenderStep::DeletingDestructor", 0x00765290, &ni::ClickRenderStepDeletingDestructor);
NISTEP_REVERSED("NiRenderFrame::Draw", 0x007652F0, &ni::RenderFrameDraw);
NISTEP_REVERSED("NiRenderFrame::Display", 0x007653D0, &ni::RenderFrameDisplay);
