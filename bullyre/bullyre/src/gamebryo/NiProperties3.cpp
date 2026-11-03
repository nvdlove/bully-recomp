// More NiProperty kinds (src/gamebryo/NiProperties.h), the slots the first passes left:
//   NiSpecularProperty         (vtable 0x961010, 0x1C bytes: its flags, bit 0 specular on)
//   NiDitherProperty           (vtable 0x961510, 0x1C bytes: its flags, bit 0 dither on)
//   NiRendererSpecificProperty (vtable 0x961278, 0x18 bytes: nothing of its own)
// Their factories, clones, viewer strings and _SDMInit (each class's default property, held in a
// static NiPointer), and the flag comparison NiShadeProperty, NiWireframeProperty, NiSpecular-
// Property and NiDitherProperty share (slot 8).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSpecularProperty_RTTI, 0x020E3C68);
GAME_VAR(RTTI, NiRendererSpecificProperty_RTTI, 0x020E3DA4);
GAME_VAR(RTTI, NiDitherProperty_RTTI, 0x020E3E7C);

GAME_VAR(NiRefObject*, NiSpecularProperty_ms_spDefault, 0x020E3C9C);
GAME_VAR(NiRefObject*, NiRendererSpecificProperty_ms_spDefault, 0x020E3DD8);
GAME_VAR(NiRefObject*, NiDitherProperty_ms_spDefault, 0x020E3EB0);

constexpr uintptr_t NiSpecularProperty_vtable = 0x00961010;
constexpr uintptr_t NiRendererSpecificProperty_vtable = 0x00961278;
constexpr uintptr_t NiDitherProperty_vtable = 0x00961510;

// a property with its flags (NiProperty's NiObjectNET, then the flags)
struct FlagProperty : NiObjectNET {
    uint16_t flags;   // +0x18
    uint16_t pad1A;
};
static_assert(sizeof(FlagProperty) == 0x1C);

GAME_FN(void, __thiscall, Property_CopyMembers, 0x007654C0, void* self, void* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, Property_IsEqual, 0x007654F0, void* self, void* other);
GAME_FN(void, __thiscall, Property_GetViewerStrings, 0x00765530, void* self, NiTArray<char*>* strings);

// a reference slot assigned through the imports: the old one let go, then the new one held
inline void SetReference(re::Ptr32<NiRefObject>& slot, NiRefObject* value) {
    NiRefObject* old = slot;
    if (old == value) {
        return;
    }
    if (old != nullptr && ni::sys::Decrement(&old->refCount) == 0) {
        Virtual<void>(old, 1);
    }
    slot = value;
    if (value != nullptr) {
        ni::sys::Increment(&value->refCount);
    }
}

// a new flag property of that class (flags off), or null
inline FlagProperty* NewFlagProperty(uintptr_t vtable) {
    FlagProperty* property = static_cast<FlagProperty*>(NiNew(0x1C));
    if (property == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(property);
    property->vtable = reinterpret_cast<void*>(vtable);
    property->flags = 0;
    return property;
}

// a clone: NiProperty's members and the flags (the clone isn't checked: a failed allocation
// crashes, as in the game)
inline FlagProperty* CloneFlagProperty(FlagProperty* self, void* cloning, uintptr_t vtable) {
    FlagProperty* clone = NewFlagProperty(vtable);
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    return clone;
}

// ---- the shared comparison

// vf8: NiProperty's, then the same bit 0 of the flags
uint8_t __fastcall FlagPropertyIsEqual(FlagProperty* self, re::Edx, FlagProperty* other) {
    if (!Property_IsEqual(self, other)) {
        return 0;
    }
    return uint8_t(~uint8_t(other->flags) ^ uint8_t(self->flags)) & 1;
}

// ---- NiSpecularProperty

const RTTI* __fastcall SpecularGetRTTI(void*, re::Edx) {
    return &NiSpecularProperty_RTTI;
}

// slot 18 Type: SPECULAR
uint32_t __fastcall SpecularType(void*, re::Edx) {
    return 6;
}

FlagProperty* __fastcall SpecularCreateClone(FlagProperty* self, re::Edx, void* cloning) {
    return CloneFlagProperty(self, cloning, NiSpecularProperty_vtable);
}

FlagProperty* __cdecl SpecularCreateObject() {
    return NewFlagProperty(NiSpecularProperty_vtable);
}

void __cdecl SpecularSDMInit() {
    SetReference(NiSpecularProperty_ms_spDefault, NewFlagProperty(NiSpecularProperty_vtable));
}

// _SDMShutdown (NiMainSDM::Shutdown's): the default let go, the slot emptied
void __cdecl SpecularSDMShutdown() {
    SetReference(NiSpecularProperty_ms_spDefault, nullptr);
}

void __fastcall SpecularGetViewerStrings(FlagProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSpecularProperty_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bSpec", self->flags & 1));
}

// ---- NiRendererSpecificProperty

const RTTI* __fastcall RendererSpecificGetRTTI(void*, re::Edx) {
    return &NiRendererSpecificProperty_RTTI;
}

inline NiObjectNET* NewRendererSpecific() {
    NiObjectNET* property = static_cast<NiObjectNET*>(NiNew(0x18));
    if (property == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(property);
    property->vtable = reinterpret_cast<void*>(NiRendererSpecificProperty_vtable);
    return property;
}

// vf3 CreateClone (copied into a null clone when the allocation fails, as the game has it)
NiObjectNET* __fastcall RendererSpecificCreateClone(NiObjectNET* self, re::Edx, void* cloning) {
    NiObjectNET* clone = NewRendererSpecific();
    Property_CopyMembers(self, clone, cloning);
    return clone;
}

NiObjectNET* __cdecl RendererSpecificCreateObject() {
    return NewRendererSpecific();
}

NiObjectNET* __fastcall RendererSpecificDeletingDestructor(NiObjectNET* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiRendererSpecificProperty_vtable);
    ObjectNET_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

void __cdecl RendererSpecificSDMInit() {
    SetReference(NiRendererSpecificProperty_ms_spDefault, NewRendererSpecific());
}

// _SDMShutdown (NiMainSDM::Shutdown's): the default let go, the slot emptied
void __cdecl RendererSpecificSDMShutdown() {
    SetReference(NiRendererSpecificProperty_ms_spDefault, nullptr);
}

// ---- NiDitherProperty

const RTTI* __fastcall DitherGetRTTI(void*, re::Edx) {
    return &NiDitherProperty_RTTI;
}

FlagProperty* __fastcall DitherCreateClone(FlagProperty* self, re::Edx, void* cloning) {
    return CloneFlagProperty(self, cloning, NiDitherProperty_vtable);
}

FlagProperty* __cdecl DitherCreateObject() {
    return NewFlagProperty(NiDitherProperty_vtable);
}

void __cdecl DitherSDMInit() {
    SetReference(NiDitherProperty_ms_spDefault, NewFlagProperty(NiDitherProperty_vtable));
}

void __cdecl DitherSDMShutdown() {
    SetReference(NiDitherProperty_ms_spDefault, nullptr);
}

void __fastcall DitherGetViewerStrings(FlagProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiDitherProperty_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bDither", self->flags & 1));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPROPS3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIPROPS3_REVERSED("NiSpecularProperty::GetRTTI", 0x007DA280, ni::SpecularGetRTTI);
NIPROPS3_REVERSED("NiSpecularProperty::Type", 0x007DA290, ni::SpecularType);
NIPROPS3_REVERSED("NiSpecularProperty::CreateClone", 0x007DA2A0, ni::SpecularCreateClone);
NIPROPS3_REVERSED("NiSpecularProperty::CreateObject", 0x007DA2F0, ni::SpecularCreateObject);
NIPROPS3_REVERSED("NiSpecularProperty::_SDMInit", 0x007DA360, ni::SpecularSDMInit);
NIPROPS3_REVERSED("NiSpecularProperty::GetViewerStrings", 0x007DA400, ni::SpecularGetViewerStrings);
NIPROPS3_REVERSED("NiRendererSpecificProperty::GetRTTI", 0x007DC7C0, ni::RendererSpecificGetRTTI);
NIPROPS3_REVERSED("NiRendererSpecificProperty::CreateClone", 0x007DC7E0, ni::RendererSpecificCreateClone);
NIPROPS3_REVERSED("NiRendererSpecificProperty::CreateObject", 0x007DC830, ni::RendererSpecificCreateObject);
NIPROPS3_REVERSED("NiRendererSpecificProperty::DeletingDestructor", 0x007DC860,
                  ni::RendererSpecificDeletingDestructor);
NIPROPS3_REVERSED("NiRendererSpecificProperty::_SDMInit", 0x007DC890, ni::RendererSpecificSDMInit);
NIPROPS3_REVERSED("NiShadeProperty::IsEqual", 0x007DE210, ni::FlagPropertyIsEqual);
NIPROPS3_REVERSED("NiDitherProperty::GetRTTI", 0x007DE240, ni::DitherGetRTTI);
NIPROPS3_REVERSED("NiDitherProperty::CreateClone", 0x007DE250, ni::DitherCreateClone);
NIPROPS3_REVERSED("NiDitherProperty::CreateObject", 0x007DE2A0, ni::DitherCreateObject);
NIPROPS3_REVERSED("NiDitherProperty::_SDMInit", 0x007DE2D0, ni::DitherSDMInit);
NIPROPS3_REVERSED("NiDitherProperty::GetViewerStrings", 0x007DE370, ni::DitherGetViewerStrings);
// the _SDMShutdowns: never hooked in the 32-bit game, bodies at their addresses for the 64-bit build
RE_UNHOOKED_FN("NiSpecularProperty::_SDMShutdown", 0x007DA3D0, &ni::SpecularSDMShutdown);
RE_UNHOOKED_FN("NiRendererSpecificProperty::_SDMShutdown", 0x007DC8F0, &ni::RendererSpecificSDMShutdown);
RE_UNHOOKED_FN("NiDitherProperty::_SDMShutdown", 0x007DE340, &ni::DitherSDMShutdown);
