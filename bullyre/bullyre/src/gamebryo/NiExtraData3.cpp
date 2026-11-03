// More kinds of NiExtraData (src/gamebryo/NiExtraData.h), the slots the first passes left:
//   NiVertWeightsExtraData  (vtable 0x960848, 0xC bytes: nothing of its own)
//   NiVectorExtraData       (vtable 0x960C88, 0x1C bytes: four floats)
//   NiStringsExtraData      (vtable 0x960FC0, 0x14 bytes: NiMalloc'd strings)
//   NiSwitchStringExtraData (vtable 0x960EA0, 0x18 bytes: the strings and an index, -1)
// Their factories (CreateObject, the stream's loaders), destructors, cloning, streaming,
// comparison (the class and the values: the name isn't compared) and viewer strings.

#include <cstddef>
#include <cstdint>

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
GAME_VAR(RTTI, NiVertWeightsExtraData_RTTI, 0x020E38D4);
GAME_VAR(RTTI, NiVectorExtraData_RTTI, 0x020E3AFC);
GAME_VAR(RTTI, NiSwitchStringExtraData_RTTI, 0x020E3BCC);
GAME_VAR(RTTI, NiStringsExtraData_RTTI, 0x020E3C34);

constexpr uintptr_t NiVertWeightsExtraData_vtable = 0x00960848;
constexpr uintptr_t NiVectorExtraData_vtable = 0x00960C88;
constexpr uintptr_t NiSwitchStringExtraData_vtable = 0x00960EA0;
constexpr uintptr_t NiStringsExtraData_vtable = 0x00960FC0;

struct NiVectorExtraData : NiExtraData {
    float vector[4];      // +0xC
};
static_assert(sizeof(NiVectorExtraData) == 0x1C);

struct NiStringsExtraData : NiExtraData {
    uint32_t size;        // +0xC
    re::Ptr32<re::Ptr32<char>> values;        // +0x10 (NiMalloc'd, each string too)
};
static_assert(sizeof(NiStringsExtraData) == 0x14);

struct NiSwitchStringExtraData : NiExtraData {
    uint32_t size;        // +0xC
    re::Ptr32<re::Ptr32<char>> values;        // +0x10 (NiMalloc'd, each string too)
    int32_t index;        // +0x14 (-1)
};
static_assert(sizeof(NiSwitchStringExtraData) == 0x18);

GAME_FN(void, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_Destruct, 0x00765670, void* self);
GAME_FN(void, __thiscall, ExtraData_CopyMembers, 0x007656C0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, ExtraData_LoadBinary, 0x007655D0, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_SaveBinary, 0x007655B0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, ExtraData_IsEqual, 0x00765610, void* self, void* other);
GAME_FN(void, __thiscall, ExtraData_GetViewerStrings, 0x00765780, void* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, Stream_LoadCString, 0x0075AEA0, void* stream, re::Ptr32<char>* string);   // NiMalloc'd
GAME_FN(void, __thiscall, Stream_SaveCString, 0x0075AFB0, void* stream, const char* string);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, SwitchStringExtraData_Destruct, 0x007D8E00, void* self);
GAME_FN(void, __thiscall, StringsExtraData_Destruct, 0x007D9DC0, void* self);

inline bool IsClass(void* object, const RTTI& rtti) {
    return Virtual<const RTTI*>(object, 2) == &rtti;
}

// ---- NiVertWeightsExtraData

// vf8: NiExtraData's comparison
uint8_t __fastcall VertWeightsIsEqual(NiExtraData* self, re::Edx, void* other) {
    return ExtraData_IsEqual(self, other) != 0 ? 1 : 0;
}

const RTTI* __fastcall VertWeightsGetRTTI(NiExtraData*, re::Edx) {
    return &NiVertWeightsExtraData_RTTI;
}

// vf3 CreateClone (copied into a null clone when the allocation fails, as the game has it)
NiExtraData* __fastcall VertWeightsCreateClone(NiExtraData* self, re::Edx, void* cloning) {
    NiExtraData* clone = static_cast<NiExtraData*>(NiNew(0xC));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiVertWeightsExtraData_vtable);
    }
    ExtraData_CopyMembers(self, clone, cloning);
    return clone;
}

NiExtraData* __cdecl VertWeightsCreateObject() {
    NiExtraData* object = static_cast<NiExtraData*>(NiNew(0xC));
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiVertWeightsExtraData_vtable);
    return object;
}

NiExtraData* __fastcall VertWeightsDeletingDestructor(NiExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiVertWeightsExtraData_vtable);
    ExtraData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0xC);
    }
    return self;
}

void __fastcall VertWeightsGetViewerStrings(NiExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiVertWeightsExtraData_RTTI.name));
}

// ---- NiVectorExtraData

inline void VectorInit(NiVectorExtraData* object) {
    ExtraData_Construct(object);
    object->vector[3] = 0.0f;
    object->vtable = reinterpret_cast<void*>(NiVectorExtraData_vtable);
    object->vector[2] = 0.0f;
    object->vector[1] = 0.0f;
    object->vector[0] = 0.0f;
}

NiVectorExtraData* __cdecl VectorCreateObject() {
    NiVectorExtraData* object = static_cast<NiVectorExtraData*>(NiNew(0x1C));
    if (object == nullptr) {
        return nullptr;
    }
    VectorInit(object);
    return object;
}

const RTTI* __fastcall VectorGetRTTI(NiVectorExtraData*, re::Edx) {
    return &NiVectorExtraData_RTTI;
}

NiVectorExtraData* __fastcall VectorDeletingDestructor(NiVectorExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiVectorExtraData_vtable);
    ExtraData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// vf3 CreateClone (the clone isn't checked: a failed allocation crashes, as in the game)
NiVectorExtraData* __fastcall VectorCreateClone(NiVectorExtraData* self, re::Edx, void* cloning) {
    NiVectorExtraData* clone = static_cast<NiVectorExtraData*>(NiNew(0x1C));
    if (clone != nullptr) {
        VectorInit(clone);
    }
    ExtraData_CopyMembers(self, clone, cloning);
    clone->vector[0] = self->vector[0];
    clone->vector[1] = self->vector[1];
    clone->vector[2] = self->vector[2];
    clone->vector[3] = self->vector[3];
    return clone;
}

void __fastcall VectorLoadBinary(NiVectorExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    LoadBytes(stream, &self->vector[0], 4);
    LoadBytes(stream, &self->vector[1], 4);
    LoadBytes(stream, &self->vector[2], 4);
    LoadBytes(stream, &self->vector[3], 4);
}

void __fastcall VectorSaveBinary(NiVectorExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    SaveBytes(stream, &self->vector[0], 4);
    SaveBytes(stream, &self->vector[1], 4);
    SaveBytes(stream, &self->vector[2], 4);
    SaveBytes(stream, &self->vector[3], 4);
}

uint8_t __fastcall VectorIsEqual(NiVectorExtraData* self, re::Edx, NiVectorExtraData* other) {
    if (other == nullptr || !IsClass(other, NiVectorExtraData_RTTI)) {
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (!(other->vector[i] == self->vector[i])) {
            return 0;
        }
    }
    return 1;
}

void __fastcall VectorGetViewerStrings(NiVectorExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiVectorExtraData_RTTI.name));
    char* text = static_cast<char*>(NiMalloc(0x37));
    NiSprintf(text, 0x37, "Vector = (%5.3f,%5.3f,%5.3f,%5.3f)", double(self->vector[0]), double(self->vector[1]),
              double(self->vector[2]), double(self->vector[3]));
    AddViewerString(strings, text);
}

// ---- NiSwitchStringExtraData

const RTTI* __fastcall SwitchStringGetRTTI(NiSwitchStringExtraData*, re::Edx) {
    return &NiSwitchStringExtraData_RTTI;
}

// the strings freed, then the array (left null)
void __fastcall SwitchStringDestruct(NiSwitchStringExtraData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiSwitchStringExtraData_vtable);
    for (uint32_t i = 0; i < self->size; i++) {
        NiFree(self->values[i]);
    }
    NiFree(self->values);
    self->values = nullptr;
    ExtraData_Destruct(self);
}

NiSwitchStringExtraData* __cdecl SwitchStringCreateObject() {
    NiSwitchStringExtraData* object = static_cast<NiSwitchStringExtraData*>(NiNew(0x18));
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiSwitchStringExtraData_vtable);
    object->values = nullptr;
    object->size = 0;
    object->index = -1;
    return object;
}

NiSwitchStringExtraData* __fastcall SwitchStringDeletingDestructor(NiSwitchStringExtraData* self, re::Edx,
                                                                   uint32_t flags) {
    SwitchStringExtraData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x18);
    }
    return self;
}

// ---- NiStringsExtraData

const RTTI* __fastcall StringsGetRTTI(NiStringsExtraData*, re::Edx) {
    return &NiStringsExtraData_RTTI;
}

void __fastcall StringsDestruct(NiStringsExtraData* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(NiStringsExtraData_vtable);
    for (uint32_t i = 0; i < self->size; i++) {
        NiFree(self->values[i]);
    }
    NiFree(self->values);
    self->values = nullptr;
    ExtraData_Destruct(self);
}

NiStringsExtraData* __cdecl StringsCreateObject() {
    NiStringsExtraData* object = static_cast<NiStringsExtraData*>(NiNew(0x14));
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    object->vtable = reinterpret_cast<void*>(NiStringsExtraData_vtable);
    object->values = nullptr;
    object->size = 0;
    return object;
}

NiStringsExtraData* __fastcall StringsDeletingDestructor(NiStringsExtraData* self, re::Edx, uint32_t flags) {
    StringsExtraData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// the count, then each string (none: a null array)
void __fastcall StringsLoadBinary(NiStringsExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    LoadBytes(stream, &self->size, 4);
    if (self->size == 0) {
        self->values = nullptr;
        return;
    }
    self->values = static_cast<re::Ptr32<char>*>(NiMalloc(self->size * 4));
    for (uint32_t i = 0; i < self->size; i++) {
        self->values[i] = nullptr;
        Stream_LoadCString(stream, &self->values[i]);
    }
}

void __fastcall StringsSaveBinary(NiStringsExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    SaveBytes(stream, &self->size, 4);
    for (uint32_t i = 0; i < self->size; i++) {
        Stream_SaveCString(stream, self->values[i]);
    }
}

// the same class and count, both arrays or neither, the same strings (or both null)
uint8_t __fastcall StringsIsEqual(NiStringsExtraData* self, re::Edx, NiStringsExtraData* other) {
    if (other == nullptr || !IsClass(other, NiStringsExtraData_RTTI)) {
        return 0;
    }
    uint32_t size = self->size;
    if (size != other->size) {
        return 0;
    }
    re::Ptr32<char>* values = self->values;
    if (values == nullptr) {
        return other->values == nullptr ? 1 : 0;
    }
    if (other->values == nullptr) {
        return 0;
    }
    for (uint32_t i = 0; i < size; i++) {
        char* a = values[i];
        char* b = other->values[i];
        if (a == nullptr) {
            if (b != nullptr) {
                return 0;
            }
            continue;
        }
        if (b == nullptr || crt::Strcmp(a, b) != 0) {
            return 0;
        }
    }
    return 1;
}

void __fastcall StringsGetViewerStrings(NiStringsExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiStringsExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiSize", self->size));
    for (uint32_t i = 0; i < self->size; i++) {
        AddViewerStringInline(strings, GetViewerStringString("m_ppcValue[i]", self->values[i]));
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIEXTRA3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIEXTRA3_REVERSED("NiVertWeightsExtraData::IsEqual", 0x007D38C0, ni::VertWeightsIsEqual);
NIEXTRA3_REVERSED("NiVertWeightsExtraData::GetRTTI", 0x007D38E0, ni::VertWeightsGetRTTI);
NIEXTRA3_REVERSED("NiVertWeightsExtraData::CreateClone", 0x007D38F0, ni::VertWeightsCreateClone);
NIEXTRA3_REVERSED("NiVertWeightsExtraData::CreateObject", 0x007D3940, ni::VertWeightsCreateObject);
NIEXTRA3_REVERSED("NiVertWeightsExtraData::DeletingDestructor", 0x007D3970, ni::VertWeightsDeletingDestructor);
NIEXTRA3_REVERSED("NiVertWeightsExtraData::GetViewerStrings", 0x007D39A0, ni::VertWeightsGetViewerStrings);
NIEXTRA3_REVERSED("NiVectorExtraData::CreateObject", 0x007D8010, ni::VectorCreateObject);
NIEXTRA3_REVERSED("NiVectorExtraData::GetRTTI", 0x007D8060, ni::VectorGetRTTI);
NIEXTRA3_REVERSED("NiVectorExtraData::DeletingDestructor", 0x007D8070, ni::VectorDeletingDestructor);
NIEXTRA3_REVERSED("NiVectorExtraData::CreateClone", 0x007D80A0, ni::VectorCreateClone);
NIEXTRA3_REVERSED("NiVectorExtraData::LoadBinary", 0x007D8100, ni::VectorLoadBinary);
NIEXTRA3_REVERSED("NiVectorExtraData::SaveBinary", 0x007D81A0, ni::VectorSaveBinary);
NIEXTRA3_REVERSED("NiVectorExtraData::IsEqual", 0x007D8240, ni::VectorIsEqual);
NIEXTRA3_REVERSED("NiVectorExtraData::GetViewerStrings", 0x007D82B0, ni::VectorGetViewerStrings);
NIEXTRA3_REVERSED("NiSwitchStringExtraData::GetRTTI", 0x007D8DF0, ni::SwitchStringGetRTTI);
NIEXTRA3_REVERSED("NiSwitchStringExtraData::~NiSwitchStringExtraData", 0x007D8E00, ni::SwitchStringDestruct);
NIEXTRA3_REVERSED("NiSwitchStringExtraData::CreateObject", 0x007D8F20, ni::SwitchStringCreateObject);
NIEXTRA3_REVERSED("NiSwitchStringExtraData::DeletingDestructor", 0x007D8F60, ni::SwitchStringDeletingDestructor);
NIEXTRA3_REVERSED("NiStringsExtraData::GetRTTI", 0x007D9DB0, ni::StringsGetRTTI);
NIEXTRA3_REVERSED("NiStringsExtraData::~NiStringsExtraData", 0x007D9DC0, ni::StringsDestruct);
NIEXTRA3_REVERSED("NiStringsExtraData::CreateObject", 0x007D9EC0, ni::StringsCreateObject);
NIEXTRA3_REVERSED("NiStringsExtraData::DeletingDestructor", 0x007D9F00, ni::StringsDeletingDestructor);
NIEXTRA3_REVERSED("NiStringsExtraData::LoadBinary", 0x007D9F80, ni::StringsLoadBinary);
NIEXTRA3_REVERSED("NiStringsExtraData::SaveBinary", 0x007DA010, ni::StringsSaveBinary);
NIEXTRA3_REVERSED("NiStringsExtraData::IsEqual", 0x007DA070, ni::StringsIsEqual);
NIEXTRA3_REVERSED("NiStringsExtraData::GetViewerStrings", 0x007DA150, ni::StringsGetViewerStrings);
