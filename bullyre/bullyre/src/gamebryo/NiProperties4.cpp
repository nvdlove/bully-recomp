// The slots of two more NiProperty kinds the first passes left:
//   NiWireframeProperty        (vtable 0x960C28, 0x1C bytes: its flags, bit 0 wireframe on;
//                               type 10)
//   NiRendererSpecificProperty (vtable 0x961278, 0x18 bytes: nothing of its own; type 4)
// GetRTTI, Type, CreateClone, viewer strings; the renderer specific property's streaming and
// comparison slots are NiProperty's (through NiProperty's own jumps to NiObjectNET's).

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiWireframeProperty_RTTI, 0x020E3AC4);
GAME_VAR(RTTI, NiRendererSpecificProperty_RTTI, 0x020E3DA4);

constexpr uintptr_t NiWireframeProperty_vtable = 0x00960C28;

struct NiWireframeProperty : NiObjectNET {
    uint16_t flags;   // +0x18
    uint16_t pad1A;
};
static_assert(sizeof(NiWireframeProperty) == 0x1C);

// NiProperty's slots (lone jumps to NiObjectNET's)
GAME_FN(void, __thiscall, Property_CopyMembers, 0x007654C0, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, Property_RegisterStreamables, 0x007654D0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Property_IsEqual, 0x007654F0, void* self, void* other);
GAME_FN(void, __thiscall, Property_GetViewerStrings, 0x00765530, void* self, NiTArray<char*>* strings);

} // namespace

// ---- NiWireframeProperty

// 0x7D7E30: GetRTTI (slot 2)
const RTTI* __fastcall NiWireframeProperty_GetRTTI(void*, re::Edx) {
    return &NiWireframeProperty_RTTI;
}

// 0x7D7E40: Type (10)
uint32_t __fastcall NiWireframeProperty_Type(void*, re::Edx) {
    return 10;
}

// 0x7D7E50: CreateClone (slot 3): NiProperty's members, then the flags (a failed NiNew is cloned
// into anyway, as the game has it)
NiWireframeProperty* __fastcall NiWireframeProperty_CreateClone(NiWireframeProperty* self, re::Edx, void* cloning) {
    NiWireframeProperty* clone = static_cast<NiWireframeProperty*>(NiNew(sizeof(NiWireframeProperty)));
    if (clone != nullptr) {
        ObjectNET_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiWireframeProperty_vtable);
        clone->flags = 0;
    } else {
        clone = nullptr;
    }
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    return clone;
}

// 0x7D7F70: GetViewerStrings (slot 9)
void __fastcall NiWireframeProperty_GetViewerStrings(NiWireframeProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiWireframeProperty_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bWireframe", self->flags & 1));
}

// ---- NiRendererSpecificProperty

// 0x7DC760 and 0x7DC790: slots 4 and 7 (LoadBinary, SaveBinary), lone jumps to NiProperty's
RE_UNHOOKED("NiRendererSpecificProperty::LoadBinary", 0x007DC760);
RE_UNHOOKED("NiRendererSpecificProperty::SaveBinary", 0x007DC790);

// 0x7DC770: RegisterStreamables (slot 6)
uint8_t __fastcall NiRendererSpecificProperty_RegisterStreamables(void* self, re::Edx, void* stream) {
    return Property_RegisterStreamables(self, stream) != 0 ? 1 : 0;
}

// 0x7DC7A0: IsEqual (slot 8)
uint8_t __fastcall NiRendererSpecificProperty_IsEqual(void* self, re::Edx, void* other) {
    return Property_IsEqual(self, other) != 0 ? 1 : 0;
}

// 0x7DC7D0: Type (4)
uint32_t __fastcall NiRendererSpecificProperty_Type(void*, re::Edx) {
    return 4;
}

// 0x7DC920: GetViewerStrings (slot 9)
void __fastcall NiRendererSpecificProperty_GetViewerStrings(void* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiRendererSpecificProperty_RTTI.name));
}

#define NIPROPS4_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIPROPS4_REVERSED("NiWireframeProperty::GetRTTI", 0x007D7E30, NiWireframeProperty_GetRTTI);
NIPROPS4_REVERSED("NiWireframeProperty::Type", 0x007D7E40, NiWireframeProperty_Type);
NIPROPS4_REVERSED("NiWireframeProperty::CreateClone", 0x007D7E50, NiWireframeProperty_CreateClone);
NIPROPS4_REVERSED("NiWireframeProperty::GetViewerStrings", 0x007D7F70, NiWireframeProperty_GetViewerStrings);
NIPROPS4_REVERSED("NiRendererSpecificProperty::RegisterStreamables", 0x007DC770, NiRendererSpecificProperty_RegisterStreamables);
NIPROPS4_REVERSED("NiRendererSpecificProperty::IsEqual", 0x007DC7A0, NiRendererSpecificProperty_IsEqual);
NIPROPS4_REVERSED("NiRendererSpecificProperty::Type", 0x007DC7D0, NiRendererSpecificProperty_Type);
NIPROPS4_REVERSED("NiRendererSpecificProperty::GetViewerStrings", 0x007DC920, NiRendererSpecificProperty_GetViewerStrings);

} // namespace ni
