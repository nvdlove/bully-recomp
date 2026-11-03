// Code the reach found only as stored pointers (one-slot vtables, a factory), inside ranges the
// other passes finished: NiTObjectArray<NiD3DPassPtr>'s and NiUISignal0's deleting destructors,
// NiShadowRenderClickFactory::Create. And three "pointers" that are text (no code starts there).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

constexpr uintptr_t PassArray_vtable = 0x00950660;   // NiTObjectArray<NiD3DPassPtr> (one slot)

GAME_FN(void, __cdecl, PassPointerArray_Delete, 0x00747030, void* base);   // NiDelete[] of NiD3DPassPtrs
GAME_FN(void, __thiscall, UISignal0_Destruct, 0x0083A110, void* self);
GAME_FN(void*, __thiscall, ShadowRenderClick_Construct, 0x00518D90, void* self);

struct PassArray {
    re::Ptr32<void> vtable;   // +0x0
    re::Ptr32<void> base;     // +0x4
    uint16_t maxSize, size, esize, growBy;
};
static_assert(sizeof(PassArray) == 0x10);

// 0x747080: the array's elements let go (NiDelete[]), then the array itself when asked
PassArray* __fastcall PassArrayDeletingDestructor(PassArray* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(PassArray_vtable);
    PassPointerArray_Delete(self->base);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(PassArray));
    }
    return self;
}

// 0x7562A0 NiShadowRenderClickFactory::Create (vtable 0x8FF194's slot 0; `this` unused): a new
// NiShadowRenderClick (0x88 bytes)
void* __fastcall ShadowRenderClickFactoryCreate(void*, re::Edx) {
    void* memory = NiNew(0x88);
    if (memory == nullptr) {
        return nullptr;
    }
    return ShadowRenderClick_Construct(memory);
}

// 0x83A240 NiUISignal0's deleting destructor (vtable 0x989430's slot 0)
void* __fastcall UISignal0DeletingDestructor(void* self, re::Edx, int flags) {
    UISignal0_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x10);
    }
    return self;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTOREDCB_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTOREDCB_REVERSED("NiTObjectArray<NiD3DPassPtr>::DeletingDestructor", 0x00747080, &ni::PassArrayDeletingDestructor);
NISTOREDCB_REVERSED("NiShadowRenderClickFactory::Create", 0x007562A0, &ni::ShadowRenderClickFactoryCreate);
// Not functions: the reach took text for code pointers. 0x766D77 is "wmv\0" (in ".wmv" movie
// names, 0x901CFE..), the middle of an instruction of NiTexturingProperty's _SDMShutdown (0x766D60);
// 0x78616D is "max\0" (0x94FB6C, 0x950B6A), inside NiMainSDM's shutdown; 0x78662E is ".fx\0"
// (0x950958), the middle of an instruction. Nothing calls or jumps there.
RE_UNHOOKED("NotAFunction_766D77", 0x00766D77);
RE_UNHOOKED("NotAFunction_78616D", 0x0078616D);
RE_UNHOOKED("NotAFunction_78662E", 0x0078662E);
NISTOREDCB_REVERSED("NiUISignal0::DeletingDestructor", 0x0083A240, &ni::UISignal0DeletingDestructor);
