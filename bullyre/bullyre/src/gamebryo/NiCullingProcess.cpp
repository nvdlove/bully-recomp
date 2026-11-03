// NiCullingProcess (vtable 0x9531C8, 0x94 bytes): the camera's frustum and its world-space
// planes, the visible set the scene's Cull appends to. NiVisibleArray::SetAllocatedSize (the
// visible set's growth), NiAccumulator's own few, and ~NiObject (which is NiRefObject's).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCamera.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using ni::scene::RefObject_Construct;
using ni::sys::Decrement;
using ni::sys::Increment;

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiAccumulator_vtable = 0x009174B4;
constexpr uintptr_t NiCullingProcess_vtable = 0x009531C8;

GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);

struct NiPlane {
    NiPoint3 normal;   // +0x0
    float constant;    // +0xC
};
static_assert(sizeof(NiPlane) == 0x10);

struct NiCullingProcess : NiRefObject {
    uint8_t useVirtualAppend;   // +0x8 (the culled geometry handed to Append, slot 4)
    uint8_t pad9[3];
    re::Ptr32<void> visibleSet;           // +0xC (NiVisibleArray)
    re::Ptr32<NiCamera> camera;           // +0x10 (while culling)
    NiFrustum frustum;          // +0x14
    NiPlane planes[6];          // +0x30 (NiFrustumPlanes, in world space)
    uint32_t activePlanes;      // +0x90 (0x3F: all six)
};
static_assert(offsetof(NiCullingProcess, frustum) == 0x14);
static_assert(offsetof(NiCullingProcess, planes) == 0x30);
static_assert(offsetof(NiCullingProcess, activePlanes) == 0x90);
static_assert(sizeof(NiCullingProcess) == 0x94);

// NiVisibleArray: the geometry pointers, the count, the allocated size, the growth
struct NiVisibleArray {
    re::Ptr32<re::Ptr32<void>> base;          // +0x0
    uint32_t count;       // +0x4
    uint32_t allocated;   // +0x8
    uint32_t growBy;      // +0xC
};
static_assert(sizeof(NiVisibleArray) == 0x10);

GAME_FN(void, __thiscall, Frustum_Construct, 0x007718A0, NiFrustum* self, float left, float right, float top,
        float bottom, float nearDist, float farDist, int ortho);
GAME_FN(void, __thiscall, Plane_Construct, 0x00770FE0, NiPlane* self);
GAME_FN(void, __thiscall, FrustumPlanes_Set, 0x007710D0, NiPlane* self, const NiFrustum* frustum,
        const NiTransform* transform);
GAME_FN(void, __thiscall, AVObject_Cull, 0x0075DC50, void* self, NiCullingProcess* culler);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, CullingProcess_SetFrustum, 0x007769F0, NiCullingProcess* self, const NiFrustum* frustum);

// ---- NiAccumulator

void* __fastcall AccumulatorConstruct(NiRefObject* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiAccumulator_vtable);
    *reinterpret_cast<re::Ptr32<void>*>(reinterpret_cast<uint8_t*>(self) + 8) = nullptr;   // the camera
    return self;
}

// FinishAccumulating: the camera let go
void __fastcall AccumulatorFinishAccumulating(NiRefObject* self, re::Edx) {
    *reinterpret_cast<re::Ptr32<void>*>(reinterpret_cast<uint8_t*>(self) + 8) = nullptr;
}

// whether there's a camera (accumulating)
uint8_t __fastcall AccumulatorHasCamera(NiRefObject* self, re::Edx) {
    return *reinterpret_cast<re::Ptr32<void>*>(reinterpret_cast<uint8_t*>(self) + 8) != nullptr ? 1 : 0;
}

// ~NiObject (NiRefObject's: the object count down)
void __fastcall ObjectDestruct(NiRefObject* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&NiRefObject_ms_uiObjects);
}

// ---- NiCullingProcess

// SetFrustum: the planes made from the camera's world transform (only while there's a camera)
void __fastcall SetFrustum(NiCullingProcess* self, re::Edx, const NiFrustum* frustum) {
    NiCamera* camera = self->camera;
    if (camera == nullptr) {
        return;
    }
    self->frustum = *frustum;
    FrustumPlanes_Set(self->planes, &self->frustum, &camera->world);
    self->activePlanes = 0x3F;
}

NiCullingProcess* __fastcall Construct(NiCullingProcess* self, re::Edx, void* visibleSet) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    self->refCount = 0;
    Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(NiCullingProcess_vtable);
    self->useVirtualAppend = 0;
    Frustum_Construct(&self->frustum, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    for (int i = 0; i < 6; i++) {
        Plane_Construct(&self->planes[i]);
    }
    self->activePlanes = 0x3F;
    self->visibleSet = visibleSet;
    self->camera = nullptr;
    return self;
}

NiCullingProcess* __fastcall DeletingDestructor(NiCullingProcess* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&NiRefObject_ms_uiObjects);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x94);
    }
    return self;
}

// vf3 Process(camera, scene, visibleSet): the scene culled into the given set (the process's own
// when none is given)
void __fastcall Process(NiCullingProcess* self, re::Edx, NiCamera* camera, void* scene, void* visibleSet) {
    if (camera == nullptr || scene == nullptr) {
        return;
    }
    self->camera = camera;
    CullingProcess_SetFrustum(self, &camera->frustum);
    void* saved = nullptr;
    if (visibleSet != nullptr) {
        saved = self->visibleSet;
        self->visibleSet = visibleSet;
    }
    if (self->visibleSet != nullptr) {
        AVObject_Cull(scene, self);
    }
    if (visibleSet != nullptr) {
        self->visibleSet = saved;
    }
    self->camera = nullptr;
}

// ---- NiVisibleArray

// SetAllocatedSize(size): the pointers kept up to the new size
void __fastcall VisibleArraySetAllocatedSize(NiVisibleArray* self, re::Edx, uint32_t size) {
    if (size == self->allocated) {
        return;
    }
    if (size < self->count) {
        self->count = size;
    }
    re::Ptr32<void>* old = self->base;
    self->allocated = size;
    if (size == 0) {
        self->base = nullptr;
    } else {
        self->base = static_cast<re::Ptr32<void>*>(NiMalloc(size * 4));
        if (self->count != 0) {
            uint32_t bytes = self->count * 4;
            crt::MemcpyS(self->base, bytes, old, bytes);
            NiFree(old);
            return;
        }
    }
    NiFree(old);
}

} // namespace

RE_UNHOOKED("NiAccumulator::IsEqual", 0x007769C0);   // jmp 0x7552A0 (NiObject's)

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NICULL_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NICULL_REVERSED("NiAccumulator::NiAccumulator", 0x00776980, &ni::AccumulatorConstruct);
NICULL_REVERSED("NiAccumulator::FinishAccumulating", 0x007769A0, &ni::AccumulatorFinishAccumulating);
NICULL_REVERSED("NiAccumulator::HasCamera", 0x007769B0, &ni::AccumulatorHasCamera);
NICULL_REVERSED("NiObject::~NiObject", 0x007769D0, &ni::ObjectDestruct);
NICULL_REVERSED("NiCullingProcess::SetFrustum", 0x007769F0, &ni::SetFrustum);
NICULL_REVERSED("NiCullingProcess::NiCullingProcess", 0x00776A30, &ni::Construct);
NICULL_REVERSED("NiCullingProcess::DeletingDestructor", 0x00776AC0, &ni::DeletingDestructor);
NICULL_REVERSED("NiCullingProcess::Process", 0x00776AF0, &ni::Process);
NICULL_REVERSED("NiVisibleArray::SetAllocatedSize", 0x00776B50, &ni::VisibleArraySetAllocatedSize);
