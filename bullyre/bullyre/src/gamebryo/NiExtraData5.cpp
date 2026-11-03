// The slots of NiSwitchStringExtraData (vtable 0x960EA0, 0x18 bytes: NiMalloc'd strings and an
// index, -1 for none) and NiStringsExtraData (vtable 0x960FC0, 0x14 bytes) the first passes left:
// copying (each string copied; an index out of range -1), cloning, streaming, comparison (the
// class, the count, the index, the strings: the name isn't compared) and viewer strings.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
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
GAME_VAR(RTTI, NiSwitchStringExtraData_RTTI, 0x020E3BCC);

constexpr uintptr_t NiSwitchStringExtraData_vtable = 0x00960EA0;
constexpr uintptr_t NiStringsExtraData_vtable = 0x00960FC0;

struct NiStringsExtraData : NiExtraData {
    uint32_t size;        // +0xC
    re::Ptr32<re::Ptr32<char>> values;        // +0x10 (NiMalloc'd, each string too)
};
static_assert(sizeof(NiStringsExtraData) == 0x14);

struct NiSwitchStringExtraData : NiStringsExtraData {
    int32_t index;        // +0x14 (-1)
};
static_assert(sizeof(NiSwitchStringExtraData) == 0x18);

GAME_FN(void*, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_CopyMembers, 0x007656C0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, ExtraData_SaveBinary, 0x007655B0, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_LoadBinary, 0x007655D0, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_GetViewerStrings, 0x00765780, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Stream_LoadCString, 0x0075AEA0, void* stream, re::Ptr32<char>* string);   // NiMalloc'd
GAME_FN(void, __thiscall, Stream_SaveCString, 0x0075AFB0, void* stream, const char* string);
GAME_FN(char*, __cdecl, GetViewerStringInt, 0x007856A0, const char* prefix, int32_t value);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, SwitchString_CopyMembers, 0x007D8E50, NiSwitchStringExtraData* self,
        NiSwitchStringExtraData* dest, void* cloning);
GAME_FN(void, __thiscall, Strings_CopyMembers, 0x007D9E10, NiStringsExtraData* self, NiStringsExtraData* dest,
        void* cloning);

// each string copied (NiMalloc'd; none stays none)
inline void CopyStrings(const NiStringsExtraData* self, NiStringsExtraData* dest) {
    for (uint32_t i = 0; i < self->size; i++) {
        const char* value = self->values[i];
        if (value == nullptr) {
            dest->values[i] = nullptr;
            continue;
        }
        uint32_t size = uint32_t(std::strlen(value)) + 1;
        dest->values[i] = static_cast<char*>(NiMalloc(size));
        crt::StrcpyS(dest->values[i], size, self->values[i]);
    }
}

} // namespace

// ---- NiSwitchStringExtraData

// 0x7D8E50: CopyMembers
void __fastcall NiSwitchStringExtraData_CopyMembers(NiSwitchStringExtraData* self, re::Edx,
                                                    NiSwitchStringExtraData* dest, void* cloning) {
    ExtraData_CopyMembers(self, dest, cloning);
    if (self->values == nullptr || self->size == 0) {
        dest->values = nullptr;
        dest->size = 0;
        return;
    }
    dest->size = self->size;
    int32_t index = self->index;
    dest->index = index > -1 && index < int32_t(self->size) ? index : -1;
    dest->values = static_cast<re::Ptr32<char>*>(NiMalloc(self->size * 4));
    CopyStrings(self, dest);
}

// 0x7D8F80: CreateClone (slot 3); a failed NiNew is copied into anyway, as the game has it
NiSwitchStringExtraData* __fastcall NiSwitchStringExtraData_CreateClone(NiSwitchStringExtraData* self, re::Edx,
                                                                       void* cloning) {
    NiSwitchStringExtraData* clone = static_cast<NiSwitchStringExtraData*>(NiNew(sizeof(NiSwitchStringExtraData)));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiSwitchStringExtraData_vtable);
        clone->values = nullptr;
        clone->size = 0;
        clone->index = -1;
        SwitchString_CopyMembers(self, clone, cloning);
        return clone;
    }
    SwitchString_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

// 0x7D8FE0: LoadBinary (slot 4): the count, the strings, the index
void __fastcall NiSwitchStringExtraData_LoadBinary(NiSwitchStringExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    LoadBytes(stream, &self->size, 4);
    if (self->size == 0) {
        self->values = nullptr;
    } else {
        self->values = static_cast<re::Ptr32<char>*>(NiMalloc(self->size * 4));
        for (uint32_t i = 0; i < self->size; i++) {
            Stream_LoadCString(stream, &self->values[i]);
        }
    }
    LoadBytes(stream, &self->index, 4);
}

// 0x7D9080: SaveBinary (slot 7)
void __fastcall NiSwitchStringExtraData_SaveBinary(NiSwitchStringExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    SaveBytes(stream, &self->size, 4);
    for (uint32_t i = 0; i < self->size; i++) {
        Stream_SaveCString(stream, self->values[i]);
    }
    SaveBytes(stream, &self->index, 4);
}

// 0x7D9110: IsEqual (slot 8)
uint8_t __fastcall NiSwitchStringExtraData_IsEqual(NiSwitchStringExtraData* self, re::Edx,
                                                   NiSwitchStringExtraData* other) {
    if (other == nullptr || Virtual<const RTTI*>(other, 2) != &NiSwitchStringExtraData_RTTI) {
        return 0;
    }
    if (self->size != other->size || self->index != other->index) {
        return 0;
    }
    re::Ptr32<char>* values = self->values;
    if ((values == nullptr) != (other->values == nullptr)) {
        return 0;
    }
    if (values == nullptr) {
        return 1;
    }
    for (uint32_t i = 0; i < self->size; i++) {
        const char* a = values[i];
        const char* b = other->values[i];
        if ((a == nullptr) != (b == nullptr)) {
            return 0;
        }
        if (a != nullptr && std::strcmp(a, b) != 0) {
            return 0;
        }
    }
    return 1;
}

// 0x7D91F0: GetViewerStrings (slot 9)
void __fastcall NiSwitchStringExtraData_GetViewerStrings(NiSwitchStringExtraData* self, re::Edx,
                                                         NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSwitchStringExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiSize", self->size));
    for (uint32_t i = 0; i < self->size; i++) {
        AddViewerStringInline(strings, GetViewerStringString("m_ppcValue[i]", self->values[i]));
    }
    AddViewerStringInline(strings, GetViewerStringInt("m_iIndex", self->index));
}

// ---- NiStringsExtraData

// 0x7D9E10: CopyMembers
void __fastcall NiStringsExtraData_CopyMembers(NiStringsExtraData* self, re::Edx, NiStringsExtraData* dest,
                                               void* cloning) {
    ExtraData_CopyMembers(self, dest, cloning);
    if (self->values == nullptr || self->size == 0) {
        dest->values = nullptr;
        dest->size = 0;
        return;
    }
    dest->size = self->size;
    dest->values = static_cast<re::Ptr32<char>*>(NiMalloc(self->size * 4));
    CopyStrings(self, dest);
}

// 0x7D9F20: CreateClone (slot 3)
NiStringsExtraData* __fastcall NiStringsExtraData_CreateClone(NiStringsExtraData* self, re::Edx, void* cloning) {
    NiStringsExtraData* clone = static_cast<NiStringsExtraData*>(NiNew(sizeof(NiStringsExtraData)));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiStringsExtraData_vtable);
        clone->values = nullptr;
        clone->size = 0;
        Strings_CopyMembers(self, clone, cloning);
        return clone;
    }
    Strings_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

#define NIEXTRA5_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIEXTRA5_REVERSED("NiSwitchStringExtraData::CopyMembers", 0x007D8E50, NiSwitchStringExtraData_CopyMembers);
NIEXTRA5_REVERSED("NiSwitchStringExtraData::CreateClone", 0x007D8F80, NiSwitchStringExtraData_CreateClone);
NIEXTRA5_REVERSED("NiSwitchStringExtraData::LoadBinary", 0x007D8FE0, NiSwitchStringExtraData_LoadBinary);
NIEXTRA5_REVERSED("NiSwitchStringExtraData::SaveBinary", 0x007D9080, NiSwitchStringExtraData_SaveBinary);
NIEXTRA5_REVERSED("NiSwitchStringExtraData::IsEqual", 0x007D9110, NiSwitchStringExtraData_IsEqual);
NIEXTRA5_REVERSED("NiSwitchStringExtraData::GetViewerStrings", 0x007D91F0, NiSwitchStringExtraData_GetViewerStrings);
NIEXTRA5_REVERSED("NiStringsExtraData::CopyMembers", 0x007D9E10, NiStringsExtraData_CopyMembers);
NIEXTRA5_REVERSED("NiStringsExtraData::CreateClone", 0x007D9F20, NiStringsExtraData_CreateClone);

} // namespace ni
