// NiShadowClickGenerator (vtable 0x95F800, 0x1C bytes): the abstract base of the shadow click
// generators (NiDefaultShadowClickGenerator, src/gamebryo/NiDefaultShadowClickGenerator.cpp): an
// NiRefObject with its render clicks (an NiTObjectSet at +8) and its name (+0x18); slots 5, 6 and
// 7 prepare a spot, point and directional light's shadow generator. Its preparing by the light's
// type, its destruction and RTTI. Also NiShadowClickValidator::ValidateClick (the validator's
// layout as src/gamebryo/NiShadowManager.cpp has it).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiShadowClickGenerator_vtable = 0x0095F800;
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(uint8_t, NiShadowClickGenerator_RTTI, 0x020E3664);
GAME_VAR(uint32_t, NiShadowRenderClick_ClassIndex, 0x020E2E98);   // (the RTTI class-set bits)

struct NiShadowClickGenerator {
    re::Ptr32<void> vtable;                    // +0x0
    uint32_t refCount;               // +0x4
    NiTSet<NiRefObject*> clicks;     // +0x8 (NiTObjectSet<NiRenderClickPtr>)
    uint32_t unknown14;              // +0x14
    re::Ptr32<const char> name;              // +0x18
};
static_assert(offsetof(NiShadowClickGenerator, name) == 0x18);
static_assert(sizeof(NiShadowClickGenerator) == 0x1C);

struct NiVisibleArray {
    re::Ptr32<re::Ptr32<void>> base;          // +0x0
    uint32_t count;       // +0x4
    uint32_t allocated;   // +0x8
    uint32_t growBy;      // +0xC
};

struct NiShadowClickValidator {
    re::Ptr32<void> vtable;                  // +0x0
    uint32_t refCount;             // +0x4
    re::Ptr32<NiRefObject> camera;           // +0x8
    re::Ptr32<NiRefObject> cullingProcess;   // +0xC
    NiVisibleArray visible;        // +0x10
};
static_assert(sizeof(NiShadowClickValidator) == 0x20);

GAME_FN(void, __thiscall, ShadowClickGenerator_Destruct, 0x007ABB60, NiShadowClickGenerator* self);
GAME_FN(void, __thiscall, ObjectSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __cdecl, ObjectSet_DeleteArray, 0x00836F80, void* base);

// the slot that prepares a generator of the light's type (its light at +0x4C, the type at +0xC5):
// a point light's 6, a directional light's 7, a spot light's 5 (their shadowed kinds the same); -1
// for the others
int PrepareSlot(void* generator) {
    switch (At<uint8_t>(At<void*>(generator, 0x4C), 0xC5)) {
    case 2:
    case 5:
        return 6;
    case 3:
    case 6:
        return 7;
    case 4:
    case 7:
        return 5;
    default:
        return -1;
    }
}

// PrepareShadowGenerator(generator): the light type's slot (a tail call: what it returns; the
// generator left in eax for the other types)
uint32_t __fastcall PrepareShadowGenerator(NiShadowClickGenerator* self, re::Edx, void* generator) {
    int slot = PrepareSlot(generator);
    if (slot < 0) {
        return uint32_t(reinterpret_cast<uintptr_t>(generator));
    }
    return Virtual<uint32_t>(self, slot, generator);
}

// PrepareShadowGenerators(list): each generator of the list prepared
void __fastcall PrepareShadowGenerators(NiShadowClickGenerator* self, re::Edx, NiTPointerList<void*>* generators) {
    for (NiTListNode<void*>* node = generators->head; node != nullptr;) {
        void* generator = node->element;
        int slot = PrepareSlot(generator);
        node = node->next;
        if (slot >= 0) {
            Virtual<uint8_t>(self, slot, generator);
        }
    }
}

const void* __fastcall GetRTTI(NiShadowClickGenerator*, re::Edx) {
    return &NiShadowClickGenerator_RTTI;
}

// ~NiShadowClickGenerator: the name let go, the clicks let go
void __fastcall Destruct(NiShadowClickGenerator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiShadowClickGenerator_vtable);
    NiFixedString name = self->name;
    if (name != kNullFixedString) {
        uint32_t length = *reinterpret_cast<const uint16_t*>(name - 2);
        if (Decrement(re::PtrCast<char*>(name) - 8) == 1) {
            GlobalStringTable_RemoveString(kGlobalStringTable, re::PtrCast<re::Ptr32<char>*>(&self->name), length);
        }
    }
    ObjectSet_RemoveAll(&self->clicks);
    ObjectSet_DeleteArray(self->clicks.base);
    self->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
    Decrement(&ms_uiObjects);
}

NiShadowClickGenerator* __fastcall DeletingDestructor(NiShadowClickGenerator* self, re::Edx, uint32_t flags) {
    ShadowClickGenerator_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// NiShadowClickValidator::ValidateClick(click, frame): a shadow render click is rendered (its flag
// 0x10 set) when it always is (flag 8), when there's no camera or culling process, or when any of
// its light's affected nodes has something visible; false otherwise. Other clicks are valid
uint8_t __fastcall ValidateClick(NiShadowClickValidator* self, re::Edx, NiRefObject* click, uint32_t) {
    if (click == nullptr) {
        return 1;
    }
    const uint8_t* rtti = Virtual<const uint8_t*>(click, 2);
    uint32_t index = NiShadowRenderClick_ClassIndex;
    uint8_t* shadowClick = (At<uint32_t>(rtti, 0xC + int(index >> 5) * 4) & (1u << (index & 31))) != 0
                               ? reinterpret_cast<uint8_t*>(click)
                               : nullptr;
    if (shadowClick == nullptr) {
        return 1;
    }
    uint8_t flags = shadowClick[0x84];
    if ((flags & 8) != 0 || self->camera == nullptr || self->cullingProcess == nullptr) {
        shadowClick[0x84] = flags | 0x10;
        return 1;
    }
    self->visible.count = 0;
    void* light = At<void*>(At<void*>(shadowClick, 0x80), 0x4C);
    for (NiTListNode<void*>* node = At<NiTListNode<void*>*>(light, 0xDC); node != nullptr;) {
        void* affected = node->element;
        node = node->next;
        Virtual<void>(self->cullingProcess, 3, self->camera, affected, &self->visible);   // Process
        if (self->visible.count > 0) {
            shadowClick[0x84] |= 0x10;
            return 1;
        }
    }
    return 0;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISHADOWCLICKGEN_REVERSED(name, address, fn)                                           \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISHADOWCLICKGEN_REVERSED("NiShadowClickValidator::ValidateClick", 0x007AB960, &ni::ValidateClick);
NISHADOWCLICKGEN_REVERSED("NiShadowClickGenerator::PrepareShadowGenerator", 0x007ABA20, &ni::PrepareShadowGenerator);
NISHADOWCLICKGEN_REVERSED("NiShadowClickGenerator::PrepareShadowGenerators", 0x007ABA80,
                          &ni::PrepareShadowGenerators);
NISHADOWCLICKGEN_REVERSED("NiShadowClickGenerator::GetRTTI", 0x007ABB50, &ni::GetRTTI);
NISHADOWCLICKGEN_REVERSED("NiShadowClickGenerator::~NiShadowClickGenerator", 0x007ABB60, &ni::Destruct);
NISHADOWCLICKGEN_REVERSED("NiShadowClickGenerator::DeletingDestructor", 0x007ABDE0, &ni::DeletingDestructor);
