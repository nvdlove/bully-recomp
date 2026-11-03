// NiRenderView (vtable 0x951244, 0x20 bytes) and Ni3DRenderView (vtable 0x951288, 0x3C bytes):
// a view's cached visible set, its last frame and name; the 3D one's camera, culling process
// and list of scenes.

#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiRenderViewVtable = 0x00951244;
constexpr uintptr_t kNi3DRenderViewVtable = 0x00951288;
constexpr uintptr_t kSceneListVtable = 0x0095127C;       // NiTPointerList<NiAVObjectPtr>
constexpr uintptr_t kPointerListBaseVtable = 0x0099C748;
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(char*, ms_kDefaultName, 0x020E35C4);   // the name a view starts with

struct NiRenderView {
    re::Ptr32<void> vtable;              // +0x0
    uint32_t refCount;         // +0x4
    re::Ptr32<void> cachedGeometry;      // +0x8 (NiVisibleArray: the array (NiMalloc), size, allocated, grow by)
    uint32_t cachedSize;       // +0xC
    uint32_t cachedAllocated;  // +0x10
    uint32_t cachedGrowBy;     // +0x14
    uint32_t lastFrameID;      // +0x18
    re::Ptr32<char> name;                // +0x1C (NiFixedString)
};
static_assert(sizeof(NiRenderView) == 0x20);

struct Ni3DRenderView : NiRenderView {
    re::Ptr32<NiRefObject> camera;             // +0x20
    re::Ptr32<NiRefObject> cullingProcess;     // +0x24
    NiTPointerList<void*> scenes;    // +0x28
    uint8_t alwaysUseCameraViewport; // +0x38
};
static_assert(offsetof(Ni3DRenderView, scenes) == 0x28);
static_assert(sizeof(Ni3DRenderView) == 0x3C);

GAME_FN(void, __thiscall, NiVisibleArray_SetAllocatedSize, 0x00776B50, void* self, uint32_t size);
GAME_FN(NiRenderView*, __thiscall, NiRenderView_Construct, 0x00755F90, NiRenderView* self);
GAME_FN(void, __thiscall, NiRenderView_Destruct, 0x00756010, NiRenderView* self);
GAME_FN(void, __thiscall, Ni3DRenderView_Destruct, 0x00756220, Ni3DRenderView* self);
GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, void* list);

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

} // namespace

// 0x755F90: NiRenderView's constructor
NiRenderView* __fastcall NiRenderView_ConstructImpl(NiRenderView* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiRenderViewVtable);
    self->cachedGeometry = nullptr;
    self->cachedSize = 0;
    self->cachedAllocated = 0;
    self->cachedGrowBy = 0x10;
    NiVisibleArray_SetAllocatedSize(&self->cachedGeometry, 0x10);
    self->lastFrameID = 0xFFFFFFFF;
    self->name = FixedStringAddRef(ms_kDefaultName);
    return self;
}

// 0x756000: NiRenderView::GetRTTI
void* __fastcall NiRenderView_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E3590);
}

// 0x756010: ~NiRenderView
void __fastcall NiRenderView_DestructImpl(NiRenderView* self, re::Edx) {
    FixedStringRelease(&self->name);
    NiFree(self->cachedGeometry);
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    Decrement(&ms_uiObjects);
}

// 0x756180: NiRenderView's deleting destructor
NiRenderView* __fastcall NiRenderView_Destroy(NiRenderView* self, re::Edx, int flags) {
    NiRenderView_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiRenderView));
    }
    return self;
}

// 0x7561A0: Ni3DRenderView(camera, cullingProcess, alwaysUseCameraViewport)
Ni3DRenderView* __fastcall Ni3DRenderView_Construct(Ni3DRenderView* self, re::Edx, NiRefObject* camera,
                                                   NiRefObject* cullingProcess, int alwaysUseCameraViewport) {
    NiRenderView_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNi3DRenderViewVtable);
    self->camera = camera;
    if (camera != nullptr) {
        Increment(&camera->refCount);
    }
    self->cullingProcess = cullingProcess;
    if (cullingProcess != nullptr) {
        Increment(&cullingProcess->refCount);
    }
    self->scenes.count = 0;
    self->scenes.head = nullptr;
    self->scenes.tail = nullptr;
    self->scenes.vtable = reinterpret_cast<void*>(kSceneListVtable);
    self->alwaysUseCameraViewport = uint8_t(alwaysUseCameraViewport);
    return self;
}

// 0x756210: Ni3DRenderView::GetRTTI
void* __fastcall Ni3DRenderView_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E35C8);
}

// 0x756220: ~Ni3DRenderView
void __fastcall Ni3DRenderView_DestructImpl(Ni3DRenderView* self, re::Edx) {
    self->scenes.vtable = reinterpret_cast<void*>(kSceneListVtable);
    NiTPointerList_RemoveAll(&self->scenes);
    self->scenes.vtable = reinterpret_cast<void*>(kPointerListBaseVtable);
    ReleaseRef(self->cullingProcess);
    ReleaseRef(self->camera);
    NiRenderView_Destruct(self);
}

// 0x756280: Ni3DRenderView's deleting destructor
Ni3DRenderView* __fastcall Ni3DRenderView_Destroy(Ni3DRenderView* self, re::Edx, int flags) {
    Ni3DRenderView_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(Ni3DRenderView));
    }
    return self;
}

#define NIRENDERVIEW_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIRENDERVIEW_REVERSED("NiRenderView::NiRenderView", 0x00755F90, NiRenderView_ConstructImpl);
NIRENDERVIEW_REVERSED("NiRenderView::GetRTTI", 0x00756000, NiRenderView_GetRTTI);
NIRENDERVIEW_REVERSED("NiRenderView::~NiRenderView", 0x00756010, NiRenderView_DestructImpl);
NIRENDERVIEW_REVERSED("NiRenderView::Destroy", 0x00756180, NiRenderView_Destroy);
NIRENDERVIEW_REVERSED("Ni3DRenderView::Ni3DRenderView", 0x007561A0, Ni3DRenderView_Construct);
NIRENDERVIEW_REVERSED("Ni3DRenderView::GetRTTI", 0x00756210, Ni3DRenderView_GetRTTI);
NIRENDERVIEW_REVERSED("Ni3DRenderView::~Ni3DRenderView", 0x00756220, Ni3DRenderView_DestructImpl);
NIRENDERVIEW_REVERSED("Ni3DRenderView::Destroy", 0x00756280, Ni3DRenderView_Destroy);

} // namespace ni
