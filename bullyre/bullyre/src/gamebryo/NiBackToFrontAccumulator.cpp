// NiBackToFrontAccumulator (vtable 0x9653A0, 0x30 bytes) on NiAccumulator (vtable 0x9174B4, 0xC:
// the camera at +8): the geometry registered goes on a list; FinishAccumulating sorts it far to
// near along the camera's direction (Sort, slot 21: a quicksort on the depths) and draws it back
// to front. NiAlphaAccumulator (NiAlphaAccumulator.cpp) builds on it. The pivot choice
// (0x7F7A20) and IsEqual are chunk 28's.
//
// Slots past NiObject's: 19 FinishAccumulating, 20 RegisterObjectArray, 21 Sort.

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
GAME_VAR(RTTI, NiBackToFrontAccumulator_RTTI, 0x020E42E0);
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);
constexpr uintptr_t kBackToFrontVtable = 0x009653A0;
constexpr uintptr_t kAccumulatorVtable = 0x009174B4;
constexpr uintptr_t kPointerListVtable = 0x00952E10;      // NiTPointerList<NiAVObject*>
constexpr uintptr_t kPointerListBaseVtable = 0x0099C748;  // its base's

struct NiBackToFrontAccumulator : NiRefObject {
    re::Ptr32<NiAVObject> camera;                      // +0x8 (NiAccumulator's)
    NiTPointerList<NiAVObject*> items;       // +0xC (vtable 0x952E10: slot 1 allocates a node, 2 frees one)
    int32_t numItems;                        // +0x1C (Sort's count of the list)
    int32_t maxItems;                        // +0x20 (the two arrays' size)
    re::Ptr32<re::Ptr32<NiAVObject>> sorted;                     // +0x24 (NiMalloc)
    re::Ptr32<float> depths;                           // +0x28 (NiMalloc)
    int32_t currentItem;                     // +0x2C
};
static_assert(offsetof(NiBackToFrontAccumulator, items) == 0xC);
static_assert(offsetof(NiBackToFrontAccumulator, numItems) == 0x1C);
static_assert(offsetof(NiBackToFrontAccumulator, depths) == 0x28);
static_assert(sizeof(NiBackToFrontAccumulator) == 0x30);

// NiVisibleArray as RegisterObjectArray reads it
struct VisibleArray {
    re::Ptr32<re::Ptr32<NiAVObject>> base;   // +0x0
    uint32_t count;      // +0x4
};

GAME_FN(void, __thiscall, Accumulator_Construct, 0x00776980, void* self);
GAME_FN(void, __thiscall, Accumulator_FinishAccumulating, 0x007769A0, void* self);
GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);   // ~NiObject
GAME_FN(void, __thiscall, PointerListBase_Destruct, 0x0075DB60, void* list);
GAME_FN(void, __thiscall, BackToFront_CopyMembers, 0x007F7990, void* self, void* dest, void* cloning);
GAME_FN(float, __thiscall, BackToFront_ChoosePivot, 0x007F7A20, void* self, int32_t left, int32_t right);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, BackToFront_Destruct, 0x007F7AE0, void* self);
GAME_FN(void, __thiscall, BackToFront_SortObjectsByDepth, 0x007F7C10, void* self, int32_t left, int32_t right);

// the list's head taken off and its node freed (the allocator's slot 2)
inline void RemoveHead(NiTPointerList<NiAVObject*>& list) {
    NiTListNode<NiAVObject*>* node = list.head;
    NiTListNode<NiAVObject*>* next = node->next;
    list.head = next;
    if (next != nullptr) {
        next->prev = nullptr;
    } else {
        list.tail = nullptr;
    }
    Virtual<void>(&list, 2, node);
    list.count--;
}

// the fields past NiAccumulator's (the constructor's, also inlined in CreateClone)
inline void InitFields(NiBackToFrontAccumulator* self) {
    self->vtable = reinterpret_cast<void*>(kBackToFrontVtable);
    self->items.count = 0;
    self->items.head = nullptr;
    self->items.tail = nullptr;
    self->items.vtable = reinterpret_cast<void*>(kPointerListVtable);
    self->numItems = 0;
    self->maxItems = 0;
    self->sorted = nullptr;
    self->depths = nullptr;
}

// 0x7F7AE0 ~NiBackToFrontAccumulator: the arrays freed, the list emptied, then NiAccumulator's
// (inlined) and NiObject's
void __fastcall Destruct(NiBackToFrontAccumulator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kBackToFrontVtable);
    NiFree(self->sorted);
    NiFree(self->depths);
    while (self->items.count != 0) {
        RemoveHead(self->items);
    }
    self->items.vtable = reinterpret_cast<void*>(kPointerListVtable);
    PointerListBase_Destruct(&self->items);
    self->items.vtable = reinterpret_cast<void*>(kPointerListBaseVtable);
    self->vtable = reinterpret_cast<void*>(kAccumulatorVtable);
    Object_Destruct(self);
}

// 0x7F7B60 GetRTTI
const RTTI* __fastcall GetRTTI(NiBackToFrontAccumulator*, re::Edx) {
    return &NiBackToFrontAccumulator_RTTI;
}

// 0x7F7B70 FinishAccumulating (slot 19): sorted (slot 21), drawn from the far end back
// (RenderImmediate, slot 34; a null entry ends it), the list emptied, then NiAccumulator's.
// Nothing at all without a renderer
void __fastcall FinishAccumulating(NiBackToFrontAccumulator* self, re::Edx) {
    void* renderer = ms_pkRenderer;
    if (renderer == nullptr) {
        return;
    }
    Virtual<void>(self, 21);
    self->currentItem = self->numItems;
    while (self->currentItem != 0) {
        self->currentItem--;
        NiAVObject* object = self->sorted[self->currentItem];
        if (object == nullptr) {
            break;
        }
        Virtual<void>(object, 34, renderer);
    }
    while (self->items.count != 0) {
        RemoveHead(self->items);
    }
    Accumulator_FinishAccumulating(self);
}

// 0x7F7C10 SortObjectsByDepth(left, right): quicksort, the pivot from ChoosePivot (stored as a
// float); the second half by recursion, the first by the loop (or the right end pulled in when
// the split lands on it). The x87 compares: an unordered depth stops both scans
void __fastcall SortObjectsByDepth(NiBackToFrontAccumulator* self, re::Edx, int32_t left, int32_t right) {
    while (left < right) {
        float pivot = BackToFront_ChoosePivot(self, left, right);
        int32_t i = right + 1;
        int32_t j = left - 1;
        for (;;) {
            do {
                i--;
            } while (self->depths[i] > pivot);
            do {
                j++;
            } while (self->depths[j] < pivot);
            if (j >= i) {
                break;
            }
            NiAVObject* object = self->sorted[j];
            self->sorted[j] = self->sorted[i];
            self->sorted[i] = object;
            float depth = self->depths[j];
            self->depths[j] = self->depths[i];
            self->depths[i] = depth;
        }
        if (i == right) {
            right = i - 1;
        } else {
            BackToFront_SortObjectsByDepth(self, left, i);
            left = i + 1;
        }
    }
}

// 0x7F7CD0 the deleting destructor
NiBackToFrontAccumulator* __fastcall DeletingDestructor(NiBackToFrontAccumulator* self, re::Edx, uint32_t flags) {
    BackToFront_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x30);
    }
    return self;
}

// 0x7F7CF0 RegisterObjectArray(visible array) (slot 20): every geometry on the list's tail
void __fastcall RegisterObjectArray(NiBackToFrontAccumulator* self, re::Edx, VisibleArray* array) {
    uint32_t count = array->count;
    NiTPointerList<NiAVObject*>& list = self->items;
    for (uint32_t i = 0; i < count; i++) {
        NiAVObject* geometry = array->base[i];
        auto* node = Virtual<NiTListNode<NiAVObject*>*>(&list, 1);
        node->element = geometry;
        node->next = nullptr;
        node->prev = list.tail;
        if (list.tail != nullptr) {
            list.tail->next = node;
        } else {
            list.head = node;
        }
        list.count++;
        list.tail = node;
    }
}

// 0x7F7D50 Sort (slot 21): the list copied into the arrays (grown to fit, their contents not
// kept), each depth the bound's center along the camera's direction (its world rotation's first
// column), then sorted
void __fastcall Sort(NiBackToFrontAccumulator* self, re::Edx) {
    int32_t count = static_cast<int32_t>(self->items.count);
    self->numItems = count;
    if (count == 0) {
        return;
    }
    if (count > self->maxItems) {
        NiFree(self->sorted);
        self->maxItems = self->numItems;
        self->sorted = static_cast<re::Ptr32<NiAVObject>*>(NiMalloc(uint32_t(self->numItems) * 4));
        NiFree(self->depths);
        self->depths = static_cast<float*>(NiMalloc(uint32_t(self->maxItems) * 4));
    }
    const float* rotate = &self->camera->world.rotate.m[0][0];
    float x = rotate[0];
    float y = rotate[3];
    float z = rotate[6];
    NiTListNode<NiAVObject*>* node = self->items.head;
    for (int32_t i = 0; i < self->numItems; i++) {
        NiAVObject* object = node->element;
        node = node->next;
        self->sorted[i] = object;
        const NiBound& bound = self->sorted[i]->worldBound;
        float depth = bound.center.y * y + bound.center.x * x;
        depth = depth + bound.center.z * z;
        self->depths[i] = depth;
    }
    BackToFront_SortObjectsByDepth(self, 0, self->numItems - 1);
}

// 0x7F7E40 NiBackToFrontAccumulator()
NiBackToFrontAccumulator* __fastcall Construct(NiBackToFrontAccumulator* self, re::Edx) {
    Accumulator_Construct(self);
    InitFields(self);
    return self;
}

// 0x7F7E70 CreateClone(cloning) (slot 3): CopyMembers runs on a failed allocation too
NiBackToFrontAccumulator* __fastcall CreateClone(NiBackToFrontAccumulator* self, re::Edx, void* cloning) {
    auto* clone = static_cast<NiBackToFrontAccumulator*>(NiNew(0x30));
    if (clone != nullptr) {
        Accumulator_Construct(clone);
        InitFields(clone);
    }
    BackToFront_CopyMembers(self, clone, cloning);
    return clone;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIBTFACCUM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::~NiBackToFrontAccumulator", 0x007F7AE0, ni::Destruct);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::GetRTTI", 0x007F7B60, ni::GetRTTI);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::FinishAccumulating", 0x007F7B70, ni::FinishAccumulating);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::SortObjectsByDepth", 0x007F7C10, ni::SortObjectsByDepth);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::DeletingDestructor", 0x007F7CD0, ni::DeletingDestructor);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::RegisterObjectArray", 0x007F7CF0, ni::RegisterObjectArray);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::Sort", 0x007F7D50, ni::Sort);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::NiBackToFrontAccumulator", 0x007F7E40, ni::Construct);
NIBTFACCUM_REVERSED("NiBackToFrontAccumulator::CreateClone", 0x007F7E70, ni::CreateClone);
