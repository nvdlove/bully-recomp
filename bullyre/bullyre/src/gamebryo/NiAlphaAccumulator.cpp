// NiAlphaAccumulator (vtable 0x961930, 0x34 bytes) on NiBackToFrontAccumulator (vtable 0x9653A0,
// 0x30 bytes, constructor 0x7F7E40) on NiAccumulator (0xC: the camera at +8): the geometry with
// alpha blending kept back (a list), sorted far to near along the camera's direction and drawn;
// the rest drawn at once. Also NiBackToFrontAccumulator's pivot choice (the median of three
// depths) and the lone jumps and wrappers its slots share.
//
// Slots past NiObject's: 19 StartAccumulating(camera), 20 RegisterObjectArray, 21 Sort (its own
// 0x7F7D50, NiAlphaAccumulator's below).

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
GAME_VAR(RTTI, NiAlphaAccumulator_RTTI, 0x020E3FB8);
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);
constexpr uintptr_t kNiAlphaAccumulatorVtable = 0x00961930;

struct NiBackToFrontAccumulator : NiRefObject {
    re::Ptr32<NiAVObject> camera;                      // +0x8 (NiAccumulator's)
    NiTPointerList<NiAVObject*> items;       // +0xC (vtable 0x952E10: slot 1 allocates a node)
    int32_t numItems;                        // +0x1C (Sort's count of the list)
    int32_t maxItems;                        // +0x20 (the two arrays' size)
    re::Ptr32<re::Ptr32<NiAVObject>> sorted;                     // +0x24 (NiMalloc)
    re::Ptr32<float> depths;                           // +0x28 (NiMalloc)
    int32_t currentItem;                     // +0x2C
};
static_assert(offsetof(NiBackToFrontAccumulator, items) == 0xC);
static_assert(offsetof(NiBackToFrontAccumulator, depths) == 0x28);
static_assert(sizeof(NiBackToFrontAccumulator) == 0x30);

struct NiAlphaAccumulator : NiBackToFrontAccumulator {
    uint8_t observeNoSortHint;   // +0x30 (1: an alpha property's "no sorter" flag respected)
    uint8_t sortByClosestPoint;  // +0x31 (the bound's radius taken off the depth)
    uint8_t pad32[2];
};
static_assert(sizeof(NiAlphaAccumulator) == 0x34);

// NiVisibleArray as RegisterObjectArray reads it
struct VisibleArray {
    re::Ptr32<re::Ptr32<NiAVObject>> base;   // +0x0
    uint32_t count;      // +0x4
};

GAME_FN(NiAlphaAccumulator*, __thiscall, BackToFront_Construct, 0x007F7E40, void* self);
GAME_FN(void, __thiscall, BackToFront_Destruct, 0x007F7AE0, void* self);
GAME_FN(void, __thiscall, BackToFront_CopyMembers, 0x008253E0, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, BackToFront_IsEqual, 0x007F7AC0, void* self, void* other);
GAME_FN(void, __thiscall, BackToFront_SortObjectsByDepth, 0x007F7C10, void* self, int32_t left, int32_t right);
GAME_FN(uint8_t, __thiscall, Accumulator_IsEqual, 0x007769C0, void* self, void* other);
GAME_FN(uint8_t, __thiscall, Object_IsEqual, 0x007552A0, void* self, void* other);

// ---- NiAlphaAccumulator

// 0x7E0960 CreateObject (the stream's factory)
NiAlphaAccumulator* __cdecl AlphaCreateObject() {
    NiAlphaAccumulator* self = static_cast<NiAlphaAccumulator*>(NiNew(0x34));
    if (self == nullptr) {
        return nullptr;
    }
    BackToFront_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiAlphaAccumulatorVtable);
    self->observeNoSortHint = 1;
    self->sortByClosestPoint = 0;
    return self;
}

// vf8 IsEqual
uint8_t __fastcall AlphaIsEqual(NiAlphaAccumulator* self, re::Edx, void* other) {
    return BackToFront_IsEqual(self, other) != 0 ? 1 : 0;
}

// vf2
const RTTI* __fastcall AlphaGetRTTI(NiAlphaAccumulator*, re::Edx) {
    return &NiAlphaAccumulator_RTTI;
}

// vf0 the deleting destructor
NiAlphaAccumulator* __fastcall AlphaDeletingDestructor(NiAlphaAccumulator* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kNiAlphaAccumulatorVtable);
    BackToFront_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x34);
    }
    return self;
}

// vf21 Sort: the list copied into the arrays (grown to fit, their contents not kept), each depth
// the bound's center along the camera's direction (its world rotation's first column), less the
// radius when sorting by the closest point; then sorted
void __fastcall AlphaSort(NiAlphaAccumulator* self, re::Edx) {
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
    if (self->sortByClosestPoint != 0) {
        for (int32_t i = 0; i < self->numItems; i++) {
            NiAVObject* object = node->element;
            node = node->next;
            self->sorted[i] = object;
            const NiBound& bound = self->sorted[i]->worldBound;
            float depth = bound.center.y * y + bound.center.x * x + bound.center.z * z;
            depth = depth - bound.radius;
            self->depths[i] = depth;
        }
    } else {
        for (int32_t i = 0; i < self->numItems; i++) {
            NiAVObject* object = node->element;
            node = node->next;
            self->sorted[i] = object;
            const NiBound& bound = self->sorted[i]->worldBound;
            self->depths[i] = bound.center.y * y + x * bound.center.x + bound.center.z * z;
        }
    }
    BackToFront_SortObjectsByDepth(self, 0, self->numItems - 1);
}

// vf3 CreateClone(cloning): the flags copied (after CopyMembers, which runs on a failed
// allocation too)
NiAlphaAccumulator* __fastcall AlphaCreateClone(NiAlphaAccumulator* self, re::Edx, void* cloning) {
    NiAlphaAccumulator* clone = static_cast<NiAlphaAccumulator*>(NiNew(0x34));
    if (clone != nullptr) {
        BackToFront_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(kNiAlphaAccumulatorVtable);
        clone->observeNoSortHint = 1;
        clone->sortByClosestPoint = 0;
    } else {
        clone = nullptr;
    }
    BackToFront_CopyMembers(self, clone, cloning);
    clone->observeNoSortHint = self->observeNoSortHint;
    clone->sortByClosestPoint = self->sortByClosestPoint;
    return clone;
}

// vf20 RegisterObjectArray(visible array): each geometry whose alpha property blends (and isn't
// marked "no sorter" when that's observed) and that's marked to sort (flags bit 6 clear) goes on
// the list; the rest draw at once (RenderImmediate, slot 34). The property state is held and let
// go around the read (the original reads it after the release, and crashes on a geometry
// without one)
void __fastcall AlphaRegisterObjectArray(NiAlphaAccumulator* self, re::Edx, VisibleArray* array) {
    void* renderer = ms_pkRenderer;
    uint32_t count = array->count;
    for (uint32_t i = 0; i < count; i++) {
        NiAVObject* geometry = array->base[i];
        NiRefObject* state = At<NiRefObject*>(geometry, 0xC4);
        if (state != nullptr) {
            Increment(&state->refCount);
            if (Decrement(&state->refCount) == 0) {
                Virtual<void>(state, 1);
            }
        }
        void* alpha = At<void*>(state, 0x8);
        uint16_t alphaFlags = At<uint16_t>(alpha, 0x18);
        if ((alphaFlags & 1) == 0 || (self->observeNoSortHint != 0 && ((alphaFlags >> 13) & 1) != 0) ||
            ((~(uint8_t(geometry->flags) >> 6)) & 1) == 0) {
            Virtual<void>(geometry, 34, renderer);
            continue;
        }
        NiTPointerList<NiAVObject*>& list = self->items;
        auto* node = Virtual<NiTListNode<NiAVObject*>*>(&list, 1);
        node->element = geometry;
        node->next = nullptr;
        node->prev = list.tail;
        if (list.tail != nullptr) {
            list.tail->next = node;
            list.count++;
            list.tail = node;
        } else {
            list.count++;
            list.head = node;
            list.tail = node;
        }
    }
}

// ---- NiBackToFrontAccumulator

// 0x7F7A20 ChoosePivot(left, right): the median of the depths at left, middle and right (the x87
// compares: an unordered pair counts as "not greater")
float __fastcall BackToFrontChoosePivot(NiBackToFrontAccumulator* self, re::Edx, int32_t left, int32_t right) {
    int32_t middle = (left + right) >> 1;
    const float* d = self->depths;
    float l = d[left];
    float m = d[middle];
    float r = d[right];
    if (!(m > l)) {
        if (r > l) {
            return l;
        }
        if (r > m) {
            return r;
        }
        return m;
    }
    if (r > m) {
        return m;
    }
    if (!(r > l)) {
        return l;
    }
    return r;
}

// vf8 IsEqual: NiAccumulator's
uint8_t __fastcall BackToFrontIsEqual(NiBackToFrontAccumulator* self, re::Edx, void* other) {
    return Accumulator_IsEqual(self, other) != 0 ? 1 : 0;
}

// NiObject::IsEqual for the classes on NiObject with nothing of their own to compare (slot 8 of
// NiRenderTargetGroup, NiDepthStencilBuffer, Ni2DBuffer, NiAVObjectPalette)
uint8_t __fastcall SharedObjectIsEqual(NiObject* self, re::Edx, void* other) {
    return Object_IsEqual(self, other) != 0 ? 1 : 0;
}

} // namespace

// the lone jumps (slots shared through the linker)
RE_UNHOOKED("NiAlphaAccumulator::LoadBinary", 0x007E0990);            // jmp 0x7F7AB0
RE_UNHOOKED("NiAlphaAccumulator::RegisterStreamables", 0x007E09A0);   // jmp 0x7DC970
RE_UNHOOKED("NiAccumulator::LoadBinary", 0x007F79A0);                 // jmp 0x7554E0 (NiObject's)
RE_UNHOOKED("NiBackToFrontAccumulator::LoadBinary", 0x007F7AB0);      // jmp 0x7F79A0
RE_UNHOOKED("NiBackToFrontAccumulator::CopyMembers", 0x007F7990);     // jmp 0x755410 (NiObject's)

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIALPHAACCUM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIALPHAACCUM_REVERSED("NiAlphaAccumulator::CreateObject", 0x007E0960, ni::AlphaCreateObject);
NIALPHAACCUM_REVERSED("NiAlphaAccumulator::IsEqual", 0x007E09B0, ni::AlphaIsEqual);
NIALPHAACCUM_REVERSED("NiAlphaAccumulator::GetRTTI", 0x007E09D0, ni::AlphaGetRTTI);
NIALPHAACCUM_REVERSED("NiAlphaAccumulator::DeletingDestructor", 0x007E09E0, ni::AlphaDeletingDestructor);
NIALPHAACCUM_REVERSED("NiAlphaAccumulator::Sort", 0x007E0A10, ni::AlphaSort);
NIALPHAACCUM_REVERSED("NiAlphaAccumulator::CreateClone", 0x007E0B60, ni::AlphaCreateClone);
NIALPHAACCUM_REVERSED("NiAlphaAccumulator::RegisterObjectArray", 0x007E0BB0, ni::AlphaRegisterObjectArray);
NIALPHAACCUM_REVERSED("NiBackToFrontAccumulator::ChoosePivot", 0x007F7A20, ni::BackToFrontChoosePivot);
NIALPHAACCUM_REVERSED("NiBackToFrontAccumulator::IsEqual", 0x007F7AC0, ni::BackToFrontIsEqual);
NIALPHAACCUM_REVERSED("NiAVObjectPalette::IsEqual", 0x007F79B0, ni::SharedObjectIsEqual);
