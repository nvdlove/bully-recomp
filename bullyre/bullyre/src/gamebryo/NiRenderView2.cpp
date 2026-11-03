// NiRenderView's cached visible set and Ni3DRenderView's camera and culling (the layouts as
// src/gamebryo/NiRenderView.cpp has them): GetPVGeometry (the set, made again once per frame),
// Ni3DRenderView's SetCameraData (the renderer's camera data from its camera, or the screen
// space's without one) and CalculatePVGeometry (its scenes culled by its culling process, or all
// their geometry when it has no camera or process).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCamera.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_VAR(void*, ms_pkRenderer, 0x020E2380);
GAME_VAR(uint32_t, NiGeometry_ClassIndex, 0x020E25BC);   // (the RTTI class-set bits)
GAME_VAR(uint32_t, NiNode_ClassIndex, 0x020E2550);

struct NiVisibleArray {
    re::Ptr32<re::Ptr32<void>> base;          // +0x0
    uint32_t count;       // +0x4
    uint32_t allocated;   // +0x8
    uint32_t growBy;      // +0xC
};
static_assert(sizeof(NiVisibleArray) == 0x10);

struct NiRenderView {
    re::Ptr32<void> vtable;                    // +0x0
    uint32_t refCount;               // +0x4
    NiVisibleArray cachedGeometry;   // +0x8
    uint32_t lastFrameID;            // +0x18
    re::Ptr32<char> name;                      // +0x1C
};
static_assert(sizeof(NiRenderView) == 0x20);

struct Ni3DRenderView : NiRenderView {
    re::Ptr32<NiCamera> camera;                     // +0x20
    re::Ptr32<NiRefObject> cullingProcess;          // +0x24
    NiTPointerList<NiAVObject*> scenes;   // +0x28
    uint8_t alwaysUseCameraViewport;      // +0x38
};
static_assert(offsetof(Ni3DRenderView, scenes) == 0x28);
static_assert(sizeof(Ni3DRenderView) == 0x3C);

GAME_FN(void, __thiscall, VisibleArray_SetAllocatedSize, 0x00776B50, NiVisibleArray* self, uint32_t size);
GAME_FN(void, __thiscall, Ni3DRenderView_AddToPVGeometryArray, 0x007AB590, Ni3DRenderView* self, NiAVObject* object);

// whether an object's class is a kind of the one with this class index (GetRTTI's set of bits)
inline bool KindOf(const void* object, uint32_t index) {
    const uint8_t* rtti = Virtual<const uint8_t*>(object, 2);
    return (At<uint32_t>(rtti, 0xC + int(index >> 5) * 4) & (1u << (index & 31))) != 0;
}

// the renderer inside a frame where camera data can be set (its frame state 1, 2 or 4 and, but for
// 4, a render target group active)
inline bool CanSetCameraData(void* renderer) {
    uint32_t state = At<uint32_t>(renderer, 0x94);
    return (state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 1 || state == 4);
}

// NiRenderView::GetPVGeometry(frame): the cached set, made again (slot 4) for a new frame (and for
// every frame of -1)
NiVisibleArray* __fastcall GetPVGeometry(NiRenderView* self, re::Edx, uint32_t frameID) {
    if (frameID != self->lastFrameID || frameID == 0xFFFFFFFF) {
        self->cachedGeometry.count = 0;
        Virtual<void>(self, 4);   // CalculatePVGeometry
    }
    self->lastFrameID = frameID;
    return &self->cachedGeometry;
}

// slot 3 SetCameraData(viewport): the camera's position, direction, up and right (its world
// rotation's columns), frustum and viewport (its own when it says so), or the screen space's
// (the renderer's slot 87) without a camera
void __fastcall SetCameraData(Ni3DRenderView* self, re::Edx, const NiRectFloat* viewport) {
    NiCamera* camera = self->camera;
    void* renderer = ms_pkRenderer;
    if (camera == nullptr) {
        if (CanSetCameraData(renderer)) {
            Virtual<void>(renderer, 87, viewport);   // SetScreenSpaceCameraData
        }
        return;
    }
    const NiRectFloat* port = self->alwaysUseCameraViewport != 0 ? &camera->port : viewport;
    const NiMatrix3& m = camera->world.rotate;
    NiPoint3 right = {m.m[0][2], m.m[1][2], m.m[2][2]};
    NiPoint3 up = {m.m[0][1], m.m[1][1], m.m[2][1]};
    NiPoint3 direction = {m.m[0][0], m.m[1][0], m.m[2][0]};
    if (CanSetCameraData(renderer)) {
        Virtual<void>(renderer, 86, &camera->world.translate, &direction, &up, &right, &camera->frustum, port);
    }
}

// AddToPVGeometryArray(object): a geometry appended to the cached set; a node's children each
void __fastcall AddToPVGeometryArray(Ni3DRenderView* self, re::Edx, NiAVObject* object) {
    if (object == nullptr) {
        return;
    }
    if (KindOf(object, NiGeometry_ClassIndex)) {
        NiVisibleArray* array = &self->cachedGeometry;
        if (array->count == array->allocated) {
            VisibleArray_SetAllocatedSize(array, array->allocated + array->growBy);
        }
        array->base[array->count] = object;
        array->count++;
        return;
    }
    if (KindOf(object, NiNode_ClassIndex)) {
        NiNode* node = static_cast<NiNode*>(object);
        for (uint32_t i = 0; i < node->children.size; i++) {
            NiAVObject* child = node->children.base[i];
            if (child != nullptr) {
                Ni3DRenderView_AddToPVGeometryArray(self, child);
            }
        }
    }
}

// slot 4 CalculatePVGeometry: each scene culled into the set (the culling process's slot 3), or
// all its geometry
void __fastcall CalculatePVGeometry(Ni3DRenderView* self, re::Edx) {
    for (NiTListNode<NiAVObject*>* node = self->scenes.head; node != nullptr;) {
        NiAVObject* scene = node->element;
        NiCamera* camera = self->camera;
        node = node->next;
        if (camera != nullptr && self->cullingProcess != nullptr) {
            Virtual<void>(self->cullingProcess, 3, camera, scene, &self->cachedGeometry);   // Process
        } else {
            Ni3DRenderView_AddToPVGeometryArray(self, scene);
        }
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIRENDERVIEW2_REVERSED(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIRENDERVIEW2_REVERSED("NiRenderView::GetPVGeometry", 0x007AB470, &ni::GetPVGeometry);
NIRENDERVIEW2_REVERSED("Ni3DRenderView::SetCameraData", 0x007AB4A0, &ni::SetCameraData);
NIRENDERVIEW2_REVERSED("Ni3DRenderView::AddToPVGeometryArray", 0x007AB590, &ni::AddToPVGeometryArray);
NIRENDERVIEW2_REVERSED("Ni3DRenderView::CalculatePVGeometry", 0x007AB660, &ni::CalculatePVGeometry);
