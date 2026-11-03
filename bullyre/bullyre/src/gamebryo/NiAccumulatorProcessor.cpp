// The render-list and render-click pieces in chunk 44: NiAccumulatorProcessor::PreRenderProcessList
// (vtable 0x9687A8, slot 3; +0x8 its accumulator, held; +0xC the camera; +0x10 whether it
// finishes the accumulation), Ni2DRenderView::CalculatePVGeometry (vtable 0x968A84, slot 4: its
// screen elements list at +0x20 into the cached array at +0x8) and
// NiScreenTextureRenderClick::PerformRendering (vtable 0x968A98, slot 6: its screen textures,
// the list at +0x4C, drawn). An NiVisibleArray: +0x0 the objects, +0x4 the count, +0x8 the
// allocated size, +0xC the growth.

#include "gamebryo/NiD3D10C44.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

GAME_VAR(void*, ms_pkRenderer, 0x020E2380);   // NiRenderer::ms_pkRenderer

GAME_FN(void, __thiscall, VisibleArray_SetAllocatedSize, 0x00776B50, void* self, uint32_t size);
GAME_FN(void, __thiscall, Renderer_SetCameraData_755D50, 0x00755D50, void* renderer, void* camera);
GAME_FN(uint8_t, __thiscall, Accumulator_IsAccumulating, 0x007769B0, void* self);
GAME_FN(void, __thiscall, ScreenTexture_Draw, 0x007DB6C0, void* self, void* renderer);
GAME_FN(float, __cdecl, NiGetCurrentTimeInSec, 0x007537B0);

// an object added to a visible array (grown by its growth when full)
inline void Add(uint8_t* array, void* object) {
    uint32_t allocated = At<uint32_t>(array, 8);
    if (At<uint32_t>(array, 4) == allocated) {
        VisibleArray_SetAllocatedSize(array, At<uint32_t>(array, 0xC) + allocated);
    }
    At<re::Ptr32<void>*>(array, 0)[At<uint32_t>(array, 4)] = object;
    At<uint32_t>(array, 4) += 1;
}

} // namespace

// 0x8C58C0 (thiscall, slot 3): NiAccumulatorProcessor::PreRenderProcessList (the input appended to
// the output; with an accumulator: the renderer's camera set, the accumulation started when it
// isn't, the output registered, finished when asked, the output emptied)
void __fastcall AccumulatorProcessor_PreRenderProcessList(uint8_t* self, re::Edx, uint8_t* input, uint8_t* output,
                                                          void*) {
    if (input == nullptr) {
        return;
    }
    uint32_t count = At<uint32_t>(input, 4);
    for (uint32_t i = 0; i < count; i++) {
        Add(output, At<re::Ptr32<void>*>(input, 0)[i]);
    }
    void* accumulator = At<void*>(self, 8);
    if (accumulator == nullptr) {
        return;
    }
    Renderer_SetCameraData_755D50(ms_pkRenderer, At<void*>(self, 0xC));
    if (!Accumulator_IsAccumulating(At<void*>(self, 8))) {
        Virtual<void>(At<void*>(self, 8), 18, At<void*>(self, 0xC));   // StartAccumulating
    }
    Virtual<void>(At<void*>(self, 8), 20, output);   // RegisterObjectArray
    if (At<uint8_t>(self, 0x10) != 0) {
        Virtual<void>(At<void*>(self, 8), 19);   // FinishAccumulating
    }
    At<uint32_t>(output, 4) = 0;
}

// 0x8C5980 (thiscall, slot 4): Ni2DRenderView::CalculatePVGeometry (its screen elements cached)
void __fastcall TwoDRenderView_CalculatePVGeometry(uint8_t* self, re::Edx) {
    for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(self, 0x24); node != nullptr;) {
        void* element = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        Add(self + 8, element);
    }
}

// 0x8C59D0 (thiscall, slot 6): NiScreenTextureRenderClick::PerformRendering (the screen-space
// camera set to its viewport when a frame is open; each screen texture drawn; the count and the
// time taken kept)
void __fastcall ScreenTextureClick_PerformRendering(uint8_t* self, re::Edx, uint32_t) {
    void* renderer = ms_pkRenderer;
    uint32_t state = At<uint32_t>(renderer, 0x94);
    if ((state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 1 || state == 4)) {
        Virtual<void>(renderer, 87, self + 0x14);   // SetScreenSpaceCameraData
    }
    float start = NiGetCurrentTimeInSec();
    for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(self, 0x50); node != nullptr;) {
        void* texture = node[2];
        node = static_cast<re::Ptr32<void>*>(node[0]);
        ScreenTexture_Draw(texture, renderer);
    }
    float end = NiGetCurrentTimeInSec();
    At<float>(self, 0x60) = end - start;
    At<uint32_t>(self, 0x5C) = At<uint32_t>(self, 0x58);
}

// 0x8C46E0 / 0x8C46F0 (4 bytes each: under a hook's size): folded getters the render clicks'
// vtables share (slot 3: the objects drawn, +0x5C; slot 5: the render time, the float at +0x60)
// 0x8C46E0 (thiscall): the objects the click drew (+0x5C)
uint32_t __fastcall RenderClick_GetNumObjectsDrawn(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x5C);
}
RE_UNHOOKED_FN("NiRenderClick::GetNumObjectsDrawn_8C46E0", 0x008C46E0, &RenderClick_GetNumObjectsDrawn);
// 0x008C46F0: [d9 41 60 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static float __fastcall Tiny_008C46F0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const float*>(self + 0x60);
}
RE_UNHOOKED_FN("NiRenderClick::GetRenderTime_8C46F0", 0x008C46F0, &Tiny_008C46F0);

#define GB44A_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44A_REVERSED("NiAccumulatorProcessor::PreRenderProcessList", 0x008C58C0, AccumulatorProcessor_PreRenderProcessList);
GB44A_REVERSED("Ni2DRenderView::CalculatePVGeometry", 0x008C5980, TwoDRenderView_CalculatePVGeometry);
GB44A_REVERSED("NiScreenTextureRenderClick::PerformRendering", 0x008C59D0, ScreenTextureClick_PerformRendering);

} // namespace ni::c44
