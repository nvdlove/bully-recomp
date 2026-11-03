// NiScreenElements (vtable 0x95F6E0, 0xEC bytes: NiTriShape's, nothing of its own): the geometry of
// screen-space polygons over an NiScreenElementsData (src/gamebryo/NiScreenElementsData.h). Its
// construction, cloning, RTTI and viewer strings; the rest of its slots are NiTriShape's or jumps
// to them.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiGeometry.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiScreenElements_RTTI, 0x020E355C);

constexpr uintptr_t NiScreenElements_vtable = 0x0095F6E0;

GAME_FN(NiGeometry*, __thiscall, TriShape_ConstructWithData, 0x0077B860, NiGeometry* self, NiObject* data);
GAME_FN(NiGeometry*, __thiscall, TriShape_Construct, 0x0077B880, NiGeometry* self);
GAME_FN(void, __thiscall, TriShape_CopyMembers, 0x0077B8A0, NiGeometry* self, NiGeometry* dest, void* cloning);
GAME_FN(void, __thiscall, TriShape_GetViewerStrings, 0x0077B9F0, NiGeometry* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, TriBasedGeom_Destruct, 0x007793C0, NiGeometry* self);

// the stream's CreateObject
NiGeometry* __cdecl CreateObject() {
    NiGeometry* self = static_cast<NiGeometry*>(NiNew(0xEC));
    if (self == nullptr) {
        return nullptr;
    }
    TriShape_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiScreenElements_vtable);
    return self;
}

// NiScreenElements(data)
NiGeometry* __fastcall Construct(NiGeometry* self, re::Edx, NiObject* data) {
    TriShape_ConstructWithData(self, data);
    self->vtable = reinterpret_cast<void*>(NiScreenElements_vtable);
    return self;
}

const RTTI* __fastcall GetRTTI(NiGeometry*, re::Edx) {
    return &NiScreenElements_RTTI;
}

NiGeometry* __fastcall DeletingDestructor(NiGeometry* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiScreenElements_vtable);
    TriBasedGeom_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xEC);
    }
    return self;
}

// slot 3 CreateClone(cloning): a new one with the members copied (copied into none when the
// allocation fails, as the game has it)
NiGeometry* __fastcall CreateClone(NiGeometry* self, re::Edx, void* cloning) {
    NiGeometry* clone = static_cast<NiGeometry*>(NiNew(0xEC));
    if (clone != nullptr) {
        TriShape_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiScreenElements_vtable);
        TriShape_CopyMembers(self, clone, cloning);
        return clone;
    }
    TriShape_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

void __fastcall GetViewerStrings(NiGeometry* self, re::Edx, NiTArray<char*>* strings) {
    TriShape_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiScreenElements_RTTI.name));
}

} // namespace

// lone jumps
RE_UNHOOKED("NiScreenElements::RegisterStreamables (jump)", 0x007AB350);   // jmp 0x7D8A30
RE_UNHOOKED("NiScreenElements::SaveBinary (jump)", 0x007AB360);            // jmp 0x7D8A40
// a virtual call's thunk (slot 34, its arguments left on the stack: jmp [vtable+0x88]); one copy the
// compiler shares between classes, its callers pass one argument or three (here passed on as
// they came: the 64-bit build's calls by address)
static uintptr_t __fastcall CallSlot34Thunk(void* self, re::Edx, uintptr_t a, uintptr_t b, uintptr_t c) {
    using Fn = uintptr_t(__thiscall*)(void*, uintptr_t, uintptr_t, uintptr_t);
    const uint8_t* vtable = *static_cast<const re::Ptr32<const uint8_t>*>(self);
    return re::GameFnAt<Fn>(vtable + 0x88)(self, a, b, c);
}
RE_UNHOOKED_FN("NiAVObject::CallSlot34 (thunk)", 0x007AB310, &CallSlot34Thunk);

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISCREENELEM_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISCREENELEM_REVERSED("NiScreenElements::CreateObject", 0x007AB320, &ni::CreateObject);
NISCREENELEM_REVERSED("NiScreenElements::GetRTTI", 0x007AB370, &ni::GetRTTI);
NISCREENELEM_REVERSED("NiScreenElements::NiScreenElements", 0x007AB380, &ni::Construct);
NISCREENELEM_REVERSED("NiScreenElements::DeletingDestructor", 0x007AB3A0, &ni::DeletingDestructor);
NISCREENELEM_REVERSED("NiScreenElements::CreateClone", 0x007AB3D0, &ni::CreateClone);
NISCREENELEM_REVERSED("NiScreenElements::GetViewerStrings", 0x007AB420, &ni::GetViewerStrings);
