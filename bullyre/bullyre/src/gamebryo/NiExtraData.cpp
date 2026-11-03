// NiExtraData and NiBooleanExtraData (src/gamebryo/NiExtraData.h): construction, the name's
// streaming, cloning and comparison.

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiExtraData.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiExtraData_RTTI, 0x020E2854);
GAME_VAR(RTTI, NiBooleanExtraData_RTTI, 0x020E2888);

constexpr uintptr_t NiExtraData_vtable = 0x00952010;
constexpr uintptr_t NiBooleanExtraData_vtable = 0x008FFA24;

GAME_ARRAY(uint32_t, NiTexturingProperty_ms_auiStatic, 0x020E28F4);

GAME_FN(void, __thiscall, Object_DoNothing, 0x008A20A0, void* self, void* stream);
// (this chunk's, called where modes apply)
GAME_FN(NiExtraData*, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_Destructor, 0x00765670, NiExtraData* self);
GAME_FN(void, __thiscall, ExtraData_CopyMembers, 0x007656C0, NiExtraData* self, NiExtraData* dest, void* cloning);
GAME_FN(void, __thiscall, ExtraData_LoadBinary, 0x007655D0, NiExtraData* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_SaveBinary, 0x007655B0, NiExtraData* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_GetViewerStrings, 0x00765780, NiExtraData* self, NiTArray<char*>* strings);

// ---- NiExtraData

// vf6: the name registered
uint8_t __fastcall RegisterStreamables(NiExtraData* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    return Virtual<uint8_t>(stream, 8, &self->name) ? 1 : 0;   // RegisterFixedString
}

// vf7
void __fastcall SaveBinary(NiExtraData* self, re::Edx, void* stream) {
    Object_DoNothing(self, stream);
    Stream_SaveFixedString(stream, &self->name);
}

// vf4
void __fastcall LoadBinary(NiExtraData* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    if (StreamVersion(stream) < 0x14010001) {
        Stream_LoadInlineFixedString(stream, &self->name);
    } else {
        Stream_LoadFixedString(stream, &self->name);
    }
}

// vf8
uint8_t __fastcall IsEqual(NiExtraData* self, re::Edx, NiExtraData* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    return self->name == other->name ? 1 : 0;
}

NiExtraData* __fastcall Construct(NiExtraData* self, re::Edx) {
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiExtraData_vtable);
    self->name = NullFixedString;
    return self;
}

const RTTI* __fastcall GetRTTI(NiExtraData*, re::Edx) {
    return &NiExtraData_RTTI;
}

void __fastcall Destruct(NiExtraData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiExtraData_vtable);
    FixedStringRelease(&self->name);
    Object_Destruct(self);
}

void __fastcall CopyMembers(NiExtraData* self, re::Edx, NiExtraData* dest, void* cloning) {
    Object_CopyMembers(self, dest, cloning);
    FixedString_Assign(&dest->name, &self->name);
}

NiExtraData* __fastcall DeletingDestructor(NiExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiExtraData_vtable);
    FixedStringRelease(&self->name);
    Object_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// vf9 (NiObject's strings not included)
void __fastcall GetViewerStrings(NiExtraData* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString(NiExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringString("m_pcName", self->name));
}

// ---- NiBooleanExtraData

// vf3 CreateClone
NiBooleanExtraData* __fastcall BooleanCreateClone(NiBooleanExtraData* self, re::Edx, void* cloning) {
    NiBooleanExtraData* clone = static_cast<NiBooleanExtraData*>(NiNew(0x10));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiBooleanExtraData_vtable);
        clone->value = 0;
    } else {
        clone = nullptr;
    }
    ExtraData_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    return clone;
}

// vf4
void __fastcall BooleanLoadBinary(NiBooleanExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    uint8_t value;
    LoadBytes(stream, &value, 1);
    self->value = value != 0 ? 1 : 0;
}

// vf7
void __fastcall BooleanSaveBinary(NiBooleanExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    uint8_t value = self->value;
    SaveBytes(stream, &value, 1);
}

// vf8: the same class and value (the name not compared)
uint8_t __fastcall BooleanIsEqual(NiBooleanExtraData* self, re::Edx, NiBooleanExtraData* other) {
    if (other == nullptr) {
        return 0;
    }
    if (Virtual<const RTTI*>(other, 2) != &NiBooleanExtraData_RTTI) {
        return 0;
    }
    return self->value == other->value ? 1 : 0;
}

// vf9
void __fastcall BooleanGetViewerStrings(NiBooleanExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiBooleanExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bValue", self->value));
}

// a static array's entry by index (just before NiTexturingProperty's code: the address of
// 0x20E28F4[i])
uint32_t* __cdecl StaticEntry(int index) {
    return &NiTexturingProperty_ms_auiStatic[index];
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIEXTRA_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIEXTRA_REVERSED("NiExtraData::RegisterStreamables", 0x00765580, &ni::RegisterStreamables);
NIEXTRA_REVERSED("NiExtraData::SaveBinary", 0x007655B0, &ni::SaveBinary);
NIEXTRA_REVERSED("NiExtraData::LoadBinary", 0x007655D0, &ni::LoadBinary);
NIEXTRA_REVERSED("NiExtraData::IsEqual", 0x00765610, &ni::IsEqual);
NIEXTRA_REVERSED("NiExtraData::NiExtraData", 0x00765640, &ni::Construct);
NIEXTRA_REVERSED("NiExtraData::GetRTTI", 0x00765660, &ni::GetRTTI);
NIEXTRA_REVERSED("NiExtraData::~NiExtraData", 0x00765670, &ni::Destruct);
NIEXTRA_REVERSED("NiExtraData::CopyMembers", 0x007656C0, &ni::CopyMembers);
NIEXTRA_REVERSED("NiExtraData::DeletingDestructor", 0x00765720, &ni::DeletingDestructor);
NIEXTRA_REVERSED("NiExtraData::GetViewerStrings", 0x00765780, &ni::GetViewerStrings);
NIEXTRA_REVERSED("NiBooleanExtraData::CreateClone", 0x00765810, &ni::BooleanCreateClone);
NIEXTRA_REVERSED("NiBooleanExtraData::LoadBinary", 0x00765890, &ni::BooleanLoadBinary);
NIEXTRA_REVERSED("NiBooleanExtraData::SaveBinary", 0x007658E0, &ni::BooleanSaveBinary);
NIEXTRA_REVERSED("NiBooleanExtraData::IsEqual", 0x00765930, &ni::BooleanIsEqual);
NIEXTRA_REVERSED("NiBooleanExtraData::GetViewerStrings", 0x00765970, &ni::BooleanGetViewerStrings);
NIEXTRA_REVERSED("NiTexturingProperty::StaticEntry", 0x00765A00, &ni::StaticEntry);
