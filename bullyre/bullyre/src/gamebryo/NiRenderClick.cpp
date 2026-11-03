// NiRenderClick::Render (the render target group made current, cleared as the click says, then
// its PerformRendering between the callbacks), NiViewRenderClick (each render view culled into its
// visible set, the list processor's pre/post passes around the geometry's RenderImmediate, the
// statistics), NiShadowRenderClick::PerformRendering and NiMaterialSwapProcessor's pre-render pass.
// Layouts from the code (Bully's NiRenderClick is 0x4C bytes: src/game/MdBullyRenderClick.cpp).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiGeometry.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiColorA {
    float r, g, b, a;
};

// NiRenderClick's callbacks: (click, data) -> carry on
using RenderClickCallback = uint8_t(__cdecl*)(void* click, void* data);

struct NiRenderClick : NiRefObject {
    uint16_t flags;                         // +0x8 (bit 1 the renderer's background color, bit 2
                                            //  the click's made the renderer's)
    uint16_t unknownA;
    re::Ptr32<NiObject> unknownC;                     // +0xC
    re::Ptr32<NiObject> renderTargetGroup;            // +0x10 (none: the renderer's default)
    float viewport[4];                      // +0x14 (NiRect<float>)
    re::Ptr32<uint8_t __cdecl(void* click, void* data)> preProcessing;      // +0x24
    re::Ptr32<uint8_t __cdecl(void* click, void* data)> postProcessing;     // +0x28
    re::Ptr32<void> preProcessingData;                // +0x2C
    re::Ptr32<void> postProcessingData;               // +0x30
    uint32_t clearMode;                     // +0x34
    NiColorA backgroundColor;               // +0x38
    re::Ptr32<const char> name;                     // +0x48
};
static_assert(offsetof(NiRenderClick, renderTargetGroup) == 0x10);
static_assert(offsetof(NiRenderClick, preProcessing) == 0x24);
static_assert(offsetof(NiRenderClick, clearMode) == 0x34);
static_assert(sizeof(NiRenderClick) == 0x4C);

struct NiVisibleArray {
    re::Ptr32<re::Ptr32<void>> base;          // +0x0
    uint32_t count;       // +0x4
    uint32_t allocated;   // +0x8
    uint32_t growBy;      // +0xC
};

struct NiViewRenderClick : NiRenderClick {
    NiTPointerList<NiObject*> renderViews;  // +0x4C
    re::Ptr32<NiObject> processor;                    // +0x5C (NiRenderListProcessor; none: the default)
    re::Ptr32<void> processorData;                    // +0x60
    NiVisibleArray processed;               // +0x64
    uint32_t numObjectsDrawn;               // +0x74
    float cullTime;                         // +0x78
    float renderTime;                       // +0x7C
};
static_assert(offsetof(NiViewRenderClick, renderViews) == 0x4C);
static_assert(offsetof(NiViewRenderClick, processed) == 0x64);
static_assert(offsetof(NiViewRenderClick, renderTime) == 0x7C);
static_assert(sizeof(NiViewRenderClick) == 0x80);

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
GAME_VAR(NiObject*, NiViewRenderClick_ms_spDefaultProcessor, 0x020E2DF0);

GAME_FN(void, __thiscall, Renderer_ClearIfUsingGroup, 0x00407860, void* self, const float* viewport,
        uint32_t clearMode);
GAME_FN(uint8_t, __thiscall, Renderer_BeginUsingRenderTargetGroup, 0x00407890, void* self, NiObject* group,
        uint32_t clearMode);
GAME_FN(uint8_t, __thiscall, Renderer_EndUsingRenderTargetGroup, 0x00407990, void* self);
GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);
GAME_FN(NiVisibleArray*, __thiscall, RenderView_GetPVGeometry, 0x007AB470, NiObject* self, uint32_t frameID);
GAME_FN(uint8_t, __thiscall, Geometry_SetActiveMaterial, 0x00402880, NiGeometry* self, NiObject* material);
GAME_FN(uint8_t, __thiscall, Geometry_ApplyAndSetActiveMaterial, 0x00761F70, NiGeometry* self,
        NiObject* material, uint32_t extraData);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, VisibleArray_SetAllocatedSize, 0x00776B50, NiVisibleArray* self, uint32_t size);
GAME_FN(void, __thiscall, ViewRenderClick_PerformRendering, 0x00776D80, NiViewRenderClick* self,
        uint32_t frameID);

// Render(frameID): the click's render target group made current (or, when it is already, the
// viewport cleared), then PerformRendering (slot 6) between the callbacks
void __fastcall Render(NiRenderClick* self, re::Edx, uint32_t frameID) {
    if (self->preProcessing != nullptr && !self->preProcessing(self, self->preProcessingData)) {
        return;
    }
    void* renderer = NiRenderer_ms_pkRenderer;
    NiObject* group = self->renderTargetGroup;
    if (group == nullptr) {
        group = Virtual<NiObject*>(renderer, 29);   // GetDefaultRenderTargetGroup
    }
    NiObject* current = Virtual<NiObject*>(renderer, 30);   // GetCurrentRenderTargetGroup
    if (current == group) {
        if (self->clearMode != 0) {
            uint8_t flags = uint8_t(self->flags);
            if (((flags >> 1) & 1) != 0) {
                Renderer_ClearIfUsingGroup(renderer, self->viewport, self->clearMode);
            } else if (((flags >> 2) & 1) != 0) {
                Virtual<void>(renderer, 22, &self->backgroundColor);   // SetBackgroundColor
                Renderer_ClearIfUsingGroup(renderer, self->viewport, self->clearMode);
            } else {
                NiColorA saved = {0.0f, 0.0f, 0.0f, 0.0f};
                Virtual<void>(renderer, 24, &saved);                   // GetBackgroundColor
                Virtual<void>(renderer, 22, &self->backgroundColor);
                Renderer_ClearIfUsingGroup(renderer, self->viewport, self->clearMode);
                Virtual<void>(renderer, 22, &saved);
            }
        }
    } else {
        if (current != nullptr) {
            Renderer_EndUsingRenderTargetGroup(renderer);
        }
        uint8_t flags = uint8_t(self->flags);
        if (((flags >> 1) & 1) != 0) {
            Renderer_BeginUsingRenderTargetGroup(renderer, group, self->clearMode);
        } else if (((flags >> 2) & 1) != 0) {
            Virtual<void>(renderer, 22, &self->backgroundColor);
            Renderer_BeginUsingRenderTargetGroup(renderer, group, self->clearMode);
        } else {
            NiColorA saved = {0.0f, 0.0f, 0.0f, 0.0f};
            Virtual<void>(renderer, 24, &saved);
            Virtual<void>(renderer, 22, &self->backgroundColor);
            Renderer_BeginUsingRenderTargetGroup(renderer, group, self->clearMode);
            Virtual<void>(renderer, 22, &saved);
        }
    }
    Virtual<void>(self, 6, frameID);   // PerformRendering
    if (self->postProcessing != nullptr) {
        self->postProcessing(self, self->postProcessingData);
    }
}

// vf3 GetNumObjectsDrawn, vf4 GetCullTime, vf5 GetRenderTime (4 bytes each: unhooked)
uint32_t __fastcall GetNumObjectsDrawn(NiViewRenderClick* self, re::Edx) {
    return self->numObjectsDrawn;
}
float __fastcall GetCullTime(NiViewRenderClick* self, re::Edx) {
    return self->cullTime;
}
float __fastcall GetRenderTime(NiViewRenderClick* self, re::Edx) {
    return self->renderTime;
}

// vf6 PerformRendering(frameID): per render view, its viewport set and its geometry culled (timed:
// the cull time), then the processor's pre pass, each one's RenderImmediate, the post pass
// (timed: the render time)
void __fastcall PerformRendering(NiViewRenderClick* self, re::Edx, uint32_t frameID) {
    if (self->renderViews.count == 0) {
        return;
    }
    void* renderer = NiRenderer_ms_pkRenderer;
    NiObject* processor = self->processor;
    if (processor == nullptr) {
        processor = NiViewRenderClick_ms_spDefaultProcessor;
    }
    self->renderTime = 0.0f;
    self->cullTime = 0.0f;
    self->numObjectsDrawn = 0;
    for (NiTListNode<NiObject*>* node = self->renderViews.head; node != nullptr;) {
        NiTListNode<NiObject*>* next = node->next;
        NiObject* view = node->element;
        Virtual<void>(view, 3, self->viewport);   // SetViewport
        float start = NiGetCurrentTimeInSec();
        NiVisibleArray* geometry = RenderView_GetPVGeometry(view, frameID);
        float end = NiGetCurrentTimeInSec();
        float elapsed = end - start;
        self->cullTime = elapsed + self->cullTime;
        self->numObjectsDrawn += geometry->count;
        if (self->processed.allocated < geometry->count) {
            VisibleArray_SetAllocatedSize(&self->processed, geometry->count);
        }
        start = NiGetCurrentTimeInSec();
        Virtual<void>(processor, 3, geometry, &self->processed, self->processorData);   // PreRenderProcessList
        uint32_t count = self->processed.count;
        for (uint32_t i = 0; i < count; i++) {
            Virtual<void>(self->processed.base[i], 34, renderer);   // RenderImmediate
        }
        Virtual<void>(processor, 4, &self->processed, self->processorData);   // PostRenderProcessList
        end = NiGetCurrentTimeInSec();
        elapsed = end - start;
        self->processed.count = 0;
        self->renderTime = elapsed + self->renderTime;
        node = next;
    }
}

// NiMaterialSwapProcessor::PreRenderProcessList(input, output, data): without a material the
// geometry is passed on; with one, each is drawn here with it swapped in (and its own back after)
void __fastcall SwapPreRenderProcessList(void* self, re::Edx, NiVisibleArray* input, NiVisibleArray* output,
                                         void*) {
    if (input == nullptr) {
        return;
    }
    uint32_t count = input->count;
    NiObject* material = At<NiObject*>(self, 8);
    if (material == nullptr) {
        for (uint32_t i = 0; i < count; i++) {
            void* geometry = input->base[i];
            if (output->count == output->allocated) {
                VisibleArray_SetAllocatedSize(output, output->growBy + output->allocated);
            }
            output->base[output->count] = geometry;
            output->count++;
        }
        return;
    }
    void* renderer = NiRenderer_ms_pkRenderer;
    for (uint32_t i = 0; i < count; i++) {
        NiGeometry* geometry = static_cast<NiGeometry*>(input->base[i]);
        NiObject* previous = geometry->activeMaterial < geometry->materialCount
                                 ? geometry->materials[geometry->activeMaterial].material
                                 : nullptr;
        Geometry_ApplyAndSetActiveMaterial(geometry, At<NiObject*>(self, 8), At<uint32_t>(self, 0xC));
        Virtual<void>(geometry, 34, renderer);   // RenderImmediate
        Geometry_SetActiveMaterial(geometry, previous);
    }
}

// NiShadowRenderClick::PerformRendering: the renderer told whether the generator's casters render
// (its flags' bit 4, slot 79), the view's rendering, then the click's bit 4 cleared
void __fastcall ShadowPerformRendering(NiViewRenderClick* self, re::Edx, uint32_t frameID) {
    uint8_t generatorFlags = At<uint8_t>(At<void*>(self, 0x80), 8);
    Virtual<void>(NiRenderer_ms_pkRenderer, 79, uint32_t((generatorFlags >> 4) & 1));
    ViewRenderClick_PerformRendering(self, frameID);
    At<uint8_t>(self, 0x84) &= 0xEF;
}

} // namespace

// 0x00776D50: [8b 41 74 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_00776D50(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x74);
}
RE_UNHOOKED_FN("NiViewRenderClick::GetNumObjectsDrawn", 0x00776D50, &Tiny_00776D50);   // 4 bytes
// 0x00776D60: [d9 41 78 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_00776D60(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x78);
}
RE_UNHOOKED_FN("NiViewRenderClick::GetCullTime", 0x00776D60, &Tiny_00776D60);          // 4 bytes
// 0x00776D70: [d9 41 7c c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_00776D70(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x7C);
}
RE_UNHOOKED_FN("NiViewRenderClick::GetRenderTime", 0x00776D70, &Tiny_00776D70);        // 4 bytes

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NICLICK_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NICLICK_REVERSED("NiRenderClick::Render", 0x00776BC0, &ni::Render);
NICLICK_REVERSED("NiViewRenderClick::PerformRendering", 0x00776D80, &ni::PerformRendering);
NICLICK_REVERSED("NiMaterialSwapProcessor::PreRenderProcessList", 0x00777D50, &ni::SwapPreRenderProcessList);
NICLICK_REVERSED("NiShadowRenderClick::PerformRendering", 0x00777E40, &ni::ShadowPerformRendering);
