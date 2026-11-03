// NiScreenFillingRenderView (vtable 0x962708, 0x3C bytes) on NiRenderView (0x20): a view of one
// screen-filling quad (an NiScreenElements over screen elements data every view shares, made by
// the first view and forgotten when the last goes: a count and the data under a critical section),
// with its own properties and dynamic effects applied when they change.
//
// Slots: 3 SetCameraData(viewport) (the screen space's), 4 CalculatePVGeometry.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiScreenFillingRenderView_RTTI, 0x020E41F0);
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);
GAME_VAR(long, ms_uiObjects, 0x020E2168);
// the shared quad's data (NiScreenElementsData*, not a reference) and the views using it
GAME_VAR(void*, ms_pkScreenFillingData, 0x020E41E8);
GAME_VAR(long, ms_iScreenFillingViews, 0x020E41EC);
inline NiCriticalSection& ms_kScreenFillingLock = *reinterpret_cast<NiCriticalSection*>(0x020E4224);

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kViewVtable = 0x00962708;
constexpr uintptr_t kEffectListVtable = 0x00952E10;      // NiTPointerList<NiDynamicEffect*>
constexpr uintptr_t kPointerListBaseVtable = 0x0099C748;
constexpr uintptr_t kDynamicEffectStateVtable = 0x00951BEC;

struct NiScreenFillingRenderView {
    re::Ptr32<void> vtable;                              // +0x0
    uint32_t refCount;                         // +0x4
    re::Ptr32<re::Ptr32<NiAVObject>> cachedGeometry;               // +0x8 (NiRenderView's visible array: base, size,
    uint32_t cachedSize;                       // +0xC  allocated, grow by)
    uint32_t cachedAllocated;                  // +0x10
    uint32_t cachedGrowBy;                     // +0x14
    uint32_t lastFrameID;                      // +0x18
    re::Ptr32<char> name;                                // +0x1C
    re::Ptr32<NiAVObject> quad;                          // +0x20 (NiScreenElementsPtr)
    NiTPointerList<NiRefObject*> effects;      // +0x24 (the dynamic effects)
    re::Ptr32<NiRefObject> effectState;                  // +0x34 (NiDynamicEffectStatePtr)
    uint8_t propertiesChanged;                 // +0x38
    uint8_t effectsChanged;                    // +0x39
    uint8_t pad3A[2];
};
static_assert(offsetof(NiScreenFillingRenderView, quad) == 0x20);
static_assert(offsetof(NiScreenFillingRenderView, effectState) == 0x34);
static_assert(sizeof(NiScreenFillingRenderView) == 0x3C);

GAME_FN(void, __thiscall, RenderView_Construct, 0x00755F90, void* self);
GAME_FN(void, __thiscall, RenderView_Destruct, 0x00756010, void* self);
GAME_FN(void, __thiscall, VisibleArray_SetAllocatedSize, 0x00776B50, void* array, uint32_t size);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, EffectState_AddEffect, 0x007D2700, void* self, void* effect);
GAME_FN(void*, __thiscall, ScreenData_Construct, 0x007893A0, void* self, int wantNormals, int wantColors,
        uint32_t textureSets, int maxPolygons, int polygonGrowBy, int maxVertices, int vertexGrowBy,
        int maxTriangles, int triangleGrowBy);
GAME_FN(int32_t, __thiscall, ScreenData_Insert, 0x00788AA0, void* self, uint32_t vertices, uint32_t triangles,
        const uint16_t* triangleList);
GAME_FN(uint8_t, __thiscall, ScreenData_SetRectangle, 0x00789060, void* self, int32_t polygon, float left, float top,
        float width, float height);
GAME_FN(uint8_t, __thiscall, ScreenData_SetTexture, 0x00789270, void* self, int32_t polygon, uint32_t set,
        const float* uvs);
GAME_FN(NiAVObject*, __thiscall, ScreenElements_Construct, 0x007AB380, void* self, void* data);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, View_Destruct, 0x007EADB0, NiScreenFillingRenderView* self);

inline void LetGo(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiPointer's assignment inlined (the old one let go first)
template <typename T>
void AssignPointer(re::Ptr32<T>& slot, std::type_identity_t<T>* value) {
    T* old = slot;
    if (old != value) {
        LetGo(old);
        slot = value;
        if (value != nullptr) {
            Increment(&value->refCount);
        }
    }
}

// 0x7EAD30 CalculatePVGeometry: the quad's properties updated and its effects applied when they
// changed (the state with every effect, or none when there are none), then the quad the only
// geometry
void __fastcall ViewCalculatePVGeometry(NiScreenFillingRenderView* self, re::Edx) {
    if (self->propertiesChanged != 0) {
        AVObject_UpdateProperties(self->quad);
        self->propertiesChanged = 0;
    }
    if (self->effectsChanged != 0) {
        NiRefObject* state = nullptr;
        if (self->effects.count != 0) {
            NiTListNode<NiRefObject*>* node = self->effects.head;
            state = self->effectState;
            while (node != nullptr) {
                NiRefObject* effect = node->element;
                node = node->next;
                EffectState_AddEffect(state, effect);
            }
        }
        Virtual<void>(self->quad, 29, state);   // UpdateEffectsDownward
        self->effectsChanged = 0;
    }
    NiAVObject* quad = self->quad;
    if (self->cachedSize == self->cachedAllocated) {
        VisibleArray_SetAllocatedSize(&self->cachedGeometry, self->cachedGrowBy + self->cachedAllocated);
    }
    self->cachedGeometry[self->cachedSize] = quad;
    self->cachedSize++;
}

// 0x7EADB0 ~NiScreenFillingRenderView (a tail call into ~NiRenderView)
void __fastcall ViewDestruct(NiScreenFillingRenderView* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kViewVtable);
    Decrement(&ms_iScreenFillingViews);
    Lock(ms_kScreenFillingLock);
    if (ms_iScreenFillingViews <= 0) {
        ms_pkScreenFillingData = nullptr;
        ms_iScreenFillingViews = 0;
    }
    Unlock(ms_kScreenFillingLock);
    LetGo(self->effectState);
    self->effects.vtable = reinterpret_cast<void*>(kEffectListVtable);
    PointerList_RemoveAll(&self->effects);
    self->effects.vtable = reinterpret_cast<void*>(kPointerListBaseVtable);
    LetGo(re::PtrCast<NiRefObject*>(self->quad));
    RenderView_Destruct(self);
}

// vf2
const RTTI* __fastcall ViewGetRTTI(NiScreenFillingRenderView*, re::Edx) {
    return &NiScreenFillingRenderView_RTTI;
}

// vf3 SetCameraData(viewport): the renderer's screen space camera (slot 87) when it's in a frame
// (state 1 or 2 with a render target group active, or 4); shared with Ni2DRenderView
void __fastcall ViewSetCameraData(void*, re::Edx, const void* viewport) {
    void* renderer = ms_pkRenderer;
    uint32_t state = At<uint32_t>(renderer, 0x94);
    if ((state == 1 || state == 2 || state == 4) && (At<uint8_t>(renderer, 0x90) == 1 || state == 4)) {
        Virtual<void>(renderer, 87, viewport);
    }
}

// 0x7EAED0 NiScreenFillingRenderView(): the shared data made by the first view (one polygon over
// the screen, textured 0..1), a quad over it updated once, an empty effect state
NiScreenFillingRenderView* __fastcall ViewConstruct(NiScreenFillingRenderView* self, re::Edx) {
    RenderView_Construct(self);
    self->vtable = reinterpret_cast<void*>(kViewVtable);
    self->quad = nullptr;
    self->effects.count = 0;
    self->effects.head = nullptr;
    self->effects.tail = nullptr;
    self->effects.vtable = reinterpret_cast<void*>(kEffectListVtable);
    self->effectState = nullptr;
    self->propertiesChanged = 0;
    self->effectsChanged = 0;
    NiCriticalSection& cs = ms_kScreenFillingLock;
    Import<void>(kEnterCriticalSection, &cs.section);
    uint32_t thread = Import<uint32_t>(kGetCurrentThreadId);
    cs.lockCount++;
    cs.owner = thread;
    if (ms_pkScreenFillingData == nullptr) {
        void* memory = NiNew(0x70);
        void* data = memory != nullptr ? ScreenData_Construct(memory, 0, 0, 1, 1, 1, 4, 4, 2, 2) : nullptr;
        ms_pkScreenFillingData = data;
        int32_t polygon = ScreenData_Insert(data, 4, 0, nullptr);
        ScreenData_SetRectangle(ms_pkScreenFillingData, polygon, 0.0f, 0.0f, 1.0f, 1.0f);
        const float uvs[8] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f};
        ScreenData_SetTexture(ms_pkScreenFillingData, polygon, 0, uvs);
    }
    Unlock(cs);
    void* memory = NiNew(0xEC);
    NiAVObject* quad = memory != nullptr ? ScreenElements_Construct(memory, ms_pkScreenFillingData) : nullptr;
    AssignPointer(self->quad, quad);
    Increment(&ms_iScreenFillingViews);
    AVObject_Update(self->quad, 0.0f, 1);
    NiRefObject* state = static_cast<NiRefObject*>(NiNew(0x20));
    if (state != nullptr) {
        state->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
        state->refCount = 0;
        Increment(&ms_uiObjects);
        state->vtable = reinterpret_cast<void*>(kDynamicEffectStateVtable);
        At<uint8_t>(state, 0x8) = 0;
        At<uint32_t>(state, 0xC) = 0;
        At<uint32_t>(state, 0x10) = 0;
        At<uint32_t>(state, 0x14) = 0;
        At<uint32_t>(state, 0x18) = 0;
        At<uint32_t>(state, 0x1C) = 0;
    }
    AssignPointer(self->effectState, state);
    return self;
}

// vf0 the deleting destructor
NiScreenFillingRenderView* __fastcall ViewDeletingDestructor(NiScreenFillingRenderView* self, re::Edx,
                                                             uint32_t flags) {
    View_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x3C);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISCREENFILL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISCREENFILL_REVERSED("NiScreenFillingRenderView::CalculatePVGeometry", 0x007EAD30, ni::ViewCalculatePVGeometry);
NISCREENFILL_REVERSED("NiScreenFillingRenderView::~NiScreenFillingRenderView", 0x007EADB0, ni::ViewDestruct);
NISCREENFILL_REVERSED("NiScreenFillingRenderView::GetRTTI", 0x007EAE80, ni::ViewGetRTTI);
NISCREENFILL_REVERSED("NiScreenFillingRenderView::SetCameraData", 0x007EAE90, ni::ViewSetCameraData);
NISCREENFILL_REVERSED("NiScreenFillingRenderView::NiScreenFillingRenderView", 0x007EAED0, ni::ViewConstruct);
NISCREENFILL_REVERSED("NiScreenFillingRenderView::DeletingDestructor", 0x007EB0D0, ni::ViewDeletingDestructor);
