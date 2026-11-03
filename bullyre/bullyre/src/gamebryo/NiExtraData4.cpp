// NiStringExtraData (vtable 0x952450, 0x10 bytes: NiExtraData and an NiFixedString value): its
// slots (streaming, comparison by the value's handle, cloning, viewer strings, destruction);
// and NiFloatsExtraData::GetValue.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiExtraData.h"
#include "gamebryo/NiExtraDataTypes.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiStringExtraData : NiExtraData {
    re::Ptr32<const char> string;   // +0xC
};
static_assert(sizeof(NiStringExtraData) == 0x10);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiStringExtraData_RTTI, 0x020E2984);

constexpr uintptr_t NiStringExtraData_vtable = 0x00952450;

GAME_FN(NiExtraData*, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_Destruct, 0x00765670, NiExtraData* self);
GAME_FN(void, __thiscall, ExtraData_CopyMembers, 0x007656C0, NiExtraData* self, NiExtraData* dest, void* cloning);
GAME_FN(uint8_t, __thiscall, ExtraData_RegisterStreamables, 0x00765580, NiExtraData* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_SaveBinary, 0x007655B0, NiExtraData* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_LoadBinary, 0x007655D0, NiExtraData* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_GetViewerStrings, 0x00765780, NiExtraData* self, NiTArray<char*>* strings);

} // namespace

// 0x768430: RegisterStreamables (slot 6): the value registered with the stream (its slot 8)
uint8_t __fastcall NiStringExtraData_RegisterStreamables(NiStringExtraData* self, re::Edx, void* stream) {
    if (!ExtraData_RegisterStreamables(self, stream)) {
        return 0;
    }
    Virtual<uint8_t>(stream, 8, &self->string);
    return 1;
}

// 0x768460: SaveBinary (slot 7)
void __fastcall NiStringExtraData_SaveBinary(NiStringExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    Stream_SaveFixedString(stream, &self->string);
}

// 0x768480: LoadBinary (slot 4): the value inline before 20.1.0.1, else from the string table
void __fastcall NiStringExtraData_LoadBinary(NiStringExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    if (StreamVersion(stream) < 0x14010001) {
        Stream_LoadInlineFixedString(stream, &self->string);
        return;
    }
    Stream_LoadFixedString(stream, &self->string);
}

// 0x7684C0: GetRTTI (slot 2)
const RTTI* __fastcall NiStringExtraData_GetRTTI(NiStringExtraData*, re::Edx) {
    return &NiStringExtraData_RTTI;
}

// 0x768500: IsEqual (slot 8): the same class and the same handle (the name isn't compared)
uint8_t __fastcall NiStringExtraData_IsEqual(NiStringExtraData* self, re::Edx, NiStringExtraData* other) {
    if (other == nullptr) {
        return 0;
    }
    if (Virtual<const RTTI*>(other, 2) != &NiStringExtraData_RTTI) {
        return 0;
    }
    return self->string == other->string ? 1 : 0;
}

// 0x768540: the deleting destructor (slot 0)
NiStringExtraData* __fastcall NiStringExtraData_DeletingDestructor(NiStringExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiStringExtraData_vtable);
    FixedStringRelease(&self->string);
    ExtraData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiStringExtraData));
    }
    return self;
}

// 0x7685A0: CreateClone (slot 3); a failed NiNew is cloned into anyway, as the game has it
NiStringExtraData* __fastcall NiStringExtraData_CreateClone(NiStringExtraData* self, re::Edx, void* cloning) {
    NiStringExtraData* clone = static_cast<NiStringExtraData*>(NiNew(sizeof(NiStringExtraData)));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiStringExtraData_vtable);
        clone->string = NullFixedString;
    } else {
        clone = nullptr;
    }
    ExtraData_CopyMembers(self, clone, cloning);
    FixedString_Assign(&clone->string, &self->string);
    return clone;
}

// 0x7685F0: GetViewerStrings (slot 9)
void __fastcall NiStringExtraData_GetViewerStrings(NiStringExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiStringExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringString("m_kString", self->string));
}

// 0x76C9D0: NiFloatsExtraData::GetValue(index) (0 past the end)
float __fastcall NiFloatsExtraData_GetValue(NiFloatsExtraData* self, re::Edx, uint32_t index) {
    if (index < self->size) {
        return self->values[index];
    }
    return 0.0f;
}

#define NIEXTRA4_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIEXTRA4_REVERSED("NiStringExtraData::RegisterStreamables", 0x00768430, NiStringExtraData_RegisterStreamables);
NIEXTRA4_REVERSED("NiStringExtraData::SaveBinary", 0x00768460, NiStringExtraData_SaveBinary);
NIEXTRA4_REVERSED("NiStringExtraData::LoadBinary", 0x00768480, NiStringExtraData_LoadBinary);
NIEXTRA4_REVERSED("NiStringExtraData::GetRTTI", 0x007684C0, NiStringExtraData_GetRTTI);
NIEXTRA4_REVERSED("NiStringExtraData::IsEqual", 0x00768500, NiStringExtraData_IsEqual);
NIEXTRA4_REVERSED("NiStringExtraData::DeletingDestructor", 0x00768540, NiStringExtraData_DeletingDestructor);
NIEXTRA4_REVERSED("NiStringExtraData::CreateClone", 0x007685A0, NiStringExtraData_CreateClone);
NIEXTRA4_REVERSED("NiStringExtraData::GetViewerStrings", 0x007685F0, NiStringExtraData_GetViewerStrings);
NIEXTRA4_REVERSED("NiFloatsExtraData::GetValue", 0x0076C9D0, NiFloatsExtraData_GetValue);

} // namespace ni
