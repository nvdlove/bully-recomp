// The value-holding kinds of extra data (src/gamebryo/NiExtraDataTypes.h): NiFloatExtraData,
// NiIntegerExtraData, NiColorExtraData, NiFloatsExtraData, NiIntegersExtraData. Their
// streaming, cloning, comparison (the class and the value: the name isn't compared) and
// viewer strings. (NiFloatExtraData's and NiIntegerExtraData's destructors and GetRTTI are in
// the game's code, 0x406610..0x407B10.)

#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiExtraData.h"
#include "gamebryo/NiExtraDataTypes.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiFloatExtraData_RTTI, 0x020E291C);
GAME_VAR(RTTI, NiIntegerExtraData_RTTI, 0x020E2950);
GAME_VAR(RTTI, NiColorExtraData_RTTI, 0x020E29DC);
GAME_VAR(RTTI, NiFloatsExtraData_RTTI, 0x020E2ABC);
GAME_VAR(RTTI, NiIntegersExtraData_RTTI, 0x020E2B60);

constexpr uintptr_t NiFloatExtraData_vtable = 0x008FFA8C;
constexpr uintptr_t NiIntegerExtraData_vtable = 0x008FFAE4;
constexpr uintptr_t NiColorExtraData_vtable = 0x009524F0;
constexpr uintptr_t NiFloatsExtraData_vtable = 0x00952860;
constexpr uintptr_t NiIntegersExtraData_vtable = 0x00952B88;

// NiColorA's default (0, 0, 0, 1), the constructor's default argument
GAME_ARRAY(float, ColorA_Default, 0x00BC75D4);

GAME_FN(NiExtraData*, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_Destruct, 0x00765670, NiExtraData* self);
GAME_FN(void, __thiscall, ExtraData_CopyMembers, 0x007656C0, NiExtraData* self, NiExtraData* dest, void* cloning);
GAME_FN(void, __thiscall, ExtraData_LoadBinary, 0x007655D0, NiExtraData* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_SaveBinary, 0x007655B0, NiExtraData* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_GetViewerStrings, 0x00765780, NiExtraData* self, NiTArray<char*>* strings);
GAME_FN(void, __thiscall, ColorA_LoadBinary, 0x007625D0, float* self, void* stream);
GAME_FN(void, __thiscall, ColorA_SaveBinary, 0x00762670, float* self, void* stream);
GAME_FN(char*, __thiscall, ColorA_GetViewerString, 0x00762770, const float* self, const char* prefix);
GAME_FN(char*, __cdecl, GetViewerStringInt, 0x007856A0, const char* prefix, int32_t value);
// (this chunk's, called where modes apply)
GAME_FN(uint8_t, __thiscall, ColorA_NotEqual, 0x00769390, const float* self, const float* other);
GAME_FN(void, __thiscall, FloatsExtraData_SetArray, 0x0076C8B0, NiFloatsExtraData* self, uint32_t size,
        const float* values);
GAME_FN(void, __thiscall, FloatsExtraData_CopyMembers, 0x0076C9F0, NiFloatsExtraData* self,
        NiFloatsExtraData* dest, void* cloning);
GAME_FN(void, __thiscall, IntegersExtraData_SetArray, 0x0076D960, NiIntegersExtraData* self, uint32_t size,
        const int32_t* values);
GAME_FN(void, __thiscall, IntegersExtraData_CopyMembers, 0x0076D9D0, NiIntegersExtraData* self,
        NiIntegersExtraData* dest, void* cloning);

// an array streamed in one read or write of `bytes` (one component of 4 bytes, as the
// library's NiStreamLoadBinary(stream, array, count) passes it)
inline void LoadArray(void* stream, void* buffer, uint32_t bytes) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = 4;
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}
inline void SaveArray(void* stream, const void* buffer, uint32_t bytes) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = 4;
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

inline bool IsClass(NiObject* object, const RTTI& rtti) {
    return Virtual<const RTTI*>(object, 2) == &rtti;
}

// ---- NiFloatExtraData, NiIntegerExtraData

// vf3 CreateClone
NiFloatExtraData* __fastcall FloatCreateClone(NiFloatExtraData* self, re::Edx, void* cloning) {
    NiFloatExtraData* clone = static_cast<NiFloatExtraData*>(NiNew(0x10));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->value = 0.0f;
        clone->vtable = reinterpret_cast<void*>(NiFloatExtraData_vtable);
    } else {
        clone = nullptr;
    }
    ExtraData_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    return clone;
}

// vf4 (NiIntegerExtraData's too: the 4-byte value)
void __fastcall FloatLoadBinary(NiFloatExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    LoadBytes(stream, &self->value, 4);
}

// vf7 (NiIntegerExtraData's too)
void __fastcall FloatSaveBinary(NiFloatExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    SaveBytes(stream, &self->value, 4);
}

// vf8: the same class and value
uint8_t __fastcall FloatIsEqual(NiFloatExtraData* self, re::Edx, NiFloatExtraData* other) {
    if (other == nullptr || !IsClass(other, NiFloatExtraData_RTTI)) {
        return 0;
    }
    return other->value == self->value ? 1 : 0;
}

// vf9
void __fastcall FloatGetViewerStrings(NiFloatExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFloatExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringFloat("m_fValue", self->value));
}

NiIntegerExtraData* __fastcall IntegerCreateClone(NiIntegerExtraData* self, re::Edx, void* cloning) {
    NiIntegerExtraData* clone = static_cast<NiIntegerExtraData*>(NiNew(0x10));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiIntegerExtraData_vtable);
        clone->value = 0;
    } else {
        clone = nullptr;
    }
    ExtraData_CopyMembers(self, clone, cloning);
    clone->value = self->value;
    return clone;
}

uint8_t __fastcall IntegerIsEqual(NiIntegerExtraData* self, re::Edx, NiIntegerExtraData* other) {
    if (other == nullptr || !IsClass(other, NiIntegerExtraData_RTTI)) {
        return 0;
    }
    return self->value == other->value ? 1 : 0;
}

void __fastcall IntegerGetViewerStrings(NiIntegerExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiIntegerExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringInt("m_iValue", self->value));
}

// ---- NiColorExtraData

// SetValue(kColor) (the colour by value)
void __fastcall ColorSetValue(NiColorExtraData* self, re::Edx, float r, float g, float b, float a) {
    self->color[0] = r;
    self->color[1] = g;
    self->color[2] = b;
    self->color[3] = a;
}

void __fastcall ColorLoadBinary(NiColorExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    ColorA_LoadBinary(self->color, stream);
}

void __fastcall ColorSaveBinary(NiColorExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    ColorA_SaveBinary(self->color, stream);
}

const RTTI* __fastcall ColorGetRTTI(NiColorExtraData*, re::Edx) {
    return &NiColorExtraData_RTTI;
}

// NiColorExtraData(kColor)
NiColorExtraData* __fastcall ColorConstruct(NiColorExtraData* self, re::Edx, float r, float g, float b, float a) {
    ExtraData_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiColorExtraData_vtable);
    self->color[0] = r;
    self->color[1] = g;
    self->color[2] = b;
    self->color[3] = a;
    return self;
}

NiColorExtraData* __fastcall ColorCreateClone(NiColorExtraData* self, re::Edx, void* cloning) {
    NiColorExtraData* clone = static_cast<NiColorExtraData*>(NiNew(0x1C));
    if (clone != nullptr) {
        ExtraData_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiColorExtraData_vtable);
        for (int i = 0; i < 4; i++) {
            clone->color[i] = ColorA_Default[i];
        }
    } else {
        clone = nullptr;
    }
    ExtraData_CopyMembers(self, clone, cloning);
    for (int i = 0; i < 4; i++) {
        clone->color[i] = self->color[i];
    }
    return clone;
}

NiColorExtraData* __fastcall ColorDeletingDestructor(NiColorExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiColorExtraData_vtable);
    ExtraData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

uint8_t __fastcall ColorIsEqual(NiColorExtraData* self, re::Edx, NiColorExtraData* other) {
    if (other == nullptr || !IsClass(other, NiColorExtraData_RTTI)) {
        return 0;
    }
    return ColorA_NotEqual(self->color, other->color) == 0 ? 1 : 0;
}

void __fastcall ColorGetViewerStrings(NiColorExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiColorExtraData_RTTI.name));
    AddViewerString(strings, ColorA_GetViewerString(self->color, "Color = "));
}

// ---- NiFloatsExtraData

// SetArray(uiSize, pfValue): the old array freed, a copy kept (none when either is empty)
void __fastcall FloatsSetArray(NiFloatsExtraData* self, re::Edx, uint32_t size, const float* values) {
    NiFree(self->values);
    if (values == nullptr || size == 0) {
        self->size = 0;
        self->values = nullptr;
        return;
    }
    self->size = size;
    self->values = static_cast<float*>(NiMalloc(size * 4));
    for (uint32_t i = 0; i < size; i++) {
        self->values[i] = values[i];
    }
}

// SetValue(uiIndex, fValue): false out of range
uint8_t __fastcall FloatsSetValue(NiFloatsExtraData* self, re::Edx, uint32_t index, float value) {
    if (index >= self->size) {
        return 0;
    }
    self->values[index] = value;
    return 1;
}

// CopyMembers (the clone's own array not freed: a new one)
void __fastcall FloatsCopyMembers(NiFloatsExtraData* self, re::Edx, NiFloatsExtraData* dest, void* cloning) {
    ExtraData_CopyMembers(self, dest, cloning);
    if (self->values == nullptr || self->size == 0) {
        dest->values = nullptr;
        dest->size = 0;
        return;
    }
    dest->size = self->size;
    dest->values = static_cast<float*>(NiMalloc(self->size * 4));
    for (uint32_t i = 0; i < self->size; i++) {
        dest->values[i] = self->values[i];
    }
}

const RTTI* __fastcall FloatsGetRTTI(NiFloatsExtraData*, re::Edx) {
    return &NiFloatsExtraData_RTTI;
}

NiFloatsExtraData* __fastcall FloatsConstruct(NiFloatsExtraData* self, re::Edx, uint32_t size, const float* values) {
    ExtraData_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiFloatsExtraData_vtable);
    self->values = nullptr;
    FloatsExtraData_SetArray(self, size, values);
    return self;
}

NiFloatsExtraData* __fastcall FloatsCreateClone(NiFloatsExtraData* self, re::Edx, void* cloning) {
    NiFloatsExtraData* clone = static_cast<NiFloatsExtraData*>(NiNew(0x14));
    if (clone == nullptr) {
        FloatsExtraData_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    ExtraData_Construct(clone);
    clone->vtable = reinterpret_cast<void*>(NiFloatsExtraData_vtable);
    clone->values = nullptr;
    clone->size = 0;
    FloatsExtraData_CopyMembers(self, clone, cloning);
    return clone;
}

NiFloatsExtraData* __fastcall FloatsDeletingDestructor(NiFloatsExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiFloatsExtraData_vtable);
    NiFree(self->values);
    self->values = nullptr;
    ExtraData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x14);
    }
    return self;
}

// vf7 (NiIntegersExtraData's too): the size, then the array in one write
void __fastcall FloatsSaveBinary(NiFloatsExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    SaveBytes(stream, &self->size, 4);
    SaveArray(stream, self->values, self->size * 4);
}

// vf4 (NiIntegersExtraData's too): a new array (the old one not freed)
void __fastcall FloatsLoadBinary(NiFloatsExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    LoadBytes(stream, &self->size, 4);
    if (self->size == 0) {
        self->values = nullptr;
        return;
    }
    float* values = static_cast<float*>(NiMalloc(self->size * 4));
    self->values = values;
    LoadArray(stream, values, self->size * 4);
}

// vf8: the same class, size and values (both arrays there or both missing)
uint8_t __fastcall FloatsIsEqual(NiFloatsExtraData* self, re::Edx, NiFloatsExtraData* other) {
    if (other == nullptr || !IsClass(other, NiFloatsExtraData_RTTI)) {
        return 0;
    }
    uint32_t size = self->size;
    if (size != other->size) {
        return 0;
    }
    const float* a = self->values;
    if ((a == nullptr) != (other->values == nullptr)) {
        return 0;
    }
    if (a == nullptr) {
        return 1;
    }
    for (uint32_t i = 0; i < size; i++) {
        if (!(other->values[i] == a[i])) {
            return 0;
        }
    }
    return 1;
}

void __fastcall FloatsGetViewerStrings(NiFloatsExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFloatsExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiSize", self->size));
    for (uint32_t i = 0; i < self->size; i++) {
        AddViewerStringInline(strings, GetViewerStringFloat("m_pfValue[i]", self->values[i]));
    }
}

// ---- NiIntegersExtraData

void __fastcall IntegersSetArray(NiIntegersExtraData* self, re::Edx, uint32_t size, const int32_t* values) {
    NiFree(self->values);
    if (values == nullptr || size == 0) {
        self->size = 0;
        self->values = nullptr;
        return;
    }
    self->size = size;
    self->values = static_cast<int32_t*>(NiMalloc(size * 4));
    for (uint32_t i = 0; i < size; i++) {
        self->values[i] = values[i];
    }
}

void __fastcall IntegersCopyMembers(NiIntegersExtraData* self, re::Edx, NiIntegersExtraData* dest, void* cloning) {
    ExtraData_CopyMembers(self, dest, cloning);
    if (self->values == nullptr || self->size == 0) {
        dest->values = nullptr;
        dest->size = 0;
        return;
    }
    dest->size = self->size;
    dest->values = static_cast<int32_t*>(NiMalloc(self->size * 4));
    for (uint32_t i = 0; i < self->size; i++) {
        dest->values[i] = self->values[i];
    }
}

const RTTI* __fastcall IntegersGetRTTI(NiIntegersExtraData*, re::Edx) {
    return &NiIntegersExtraData_RTTI;
}

NiIntegersExtraData* __fastcall IntegersConstruct(NiIntegersExtraData* self, re::Edx, uint32_t size,
                                                  const int32_t* values) {
    ExtraData_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiIntegersExtraData_vtable);
    self->values = nullptr;
    IntegersExtraData_SetArray(self, size, values);
    return self;
}

NiIntegersExtraData* __fastcall IntegersCreateClone(NiIntegersExtraData* self, re::Edx, void* cloning) {
    NiIntegersExtraData* clone = static_cast<NiIntegersExtraData*>(NiNew(0x14));
    if (clone == nullptr) {
        IntegersExtraData_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    ExtraData_Construct(clone);
    clone->vtable = reinterpret_cast<void*>(NiIntegersExtraData_vtable);
    clone->values = nullptr;
    clone->size = 0;
    IntegersExtraData_CopyMembers(self, clone, cloning);
    return clone;
}

NiIntegersExtraData* __fastcall IntegersDeletingDestructor(NiIntegersExtraData* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiIntegersExtraData_vtable);
    NiFree(self->values);
    self->values = nullptr;
    ExtraData_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x14);
    }
    return self;
}

uint8_t __fastcall IntegersIsEqual(NiIntegersExtraData* self, re::Edx, NiIntegersExtraData* other) {
    if (other == nullptr || !IsClass(other, NiIntegersExtraData_RTTI)) {
        return 0;
    }
    uint32_t size = self->size;
    if (size != other->size) {
        return 0;
    }
    const int32_t* a = self->values;
    if ((a == nullptr) != (other->values == nullptr)) {
        return 0;
    }
    if (a == nullptr) {
        return 1;
    }
    for (uint32_t i = 0; i < size; i++) {
        if (a[i] != other->values[i]) {
            return 0;
        }
    }
    return 1;
}

void __fastcall IntegersGetViewerStrings(NiIntegersExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiIntegersExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiSize", self->size));
    for (uint32_t i = 0; i < self->size; i++) {
        AddViewerStringInline(strings, GetViewerStringInt("m_piValue[i]", self->values[i]));
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIEXTRA2_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIEXTRA2_REVERSED("NiFloatExtraData::CreateClone", 0x00768110, &ni::FloatCreateClone);
NIEXTRA2_REVERSED("NiFloatExtraData::LoadBinary", 0x00768190, &ni::FloatLoadBinary);
NIEXTRA2_REVERSED("NiFloatExtraData::IsEqual", 0x007681D0, &ni::FloatIsEqual);
NIEXTRA2_REVERSED("NiFloatExtraData::GetViewerStrings", 0x00768210, &ni::FloatGetViewerStrings);
NIEXTRA2_REVERSED("NiIntegerExtraData::CreateClone", 0x007682A0, &ni::IntegerCreateClone);
NIEXTRA2_REVERSED("NiFloatExtraData::SaveBinary", 0x00768320, &ni::FloatSaveBinary);
NIEXTRA2_REVERSED("NiIntegerExtraData::IsEqual", 0x00768360, &ni::IntegerIsEqual);
NIEXTRA2_REVERSED("NiIntegerExtraData::GetViewerStrings", 0x007683A0, &ni::IntegerGetViewerStrings);
NIEXTRA2_REVERSED("NiColorExtraData::SetValue", 0x007691F0, &ni::ColorSetValue);
NIEXTRA2_REVERSED("NiColorExtraData::LoadBinary", 0x00769210, &ni::ColorLoadBinary);
NIEXTRA2_REVERSED("NiColorExtraData::SaveBinary", 0x00769230, &ni::ColorSaveBinary);
NIEXTRA2_REVERSED("NiColorExtraData::GetRTTI", 0x00769250, &ni::ColorGetRTTI);
NIEXTRA2_REVERSED("NiColorExtraData::NiColorExtraData", 0x00769260, &ni::ColorConstruct);
NIEXTRA2_REVERSED("NiColorExtraData::CreateClone", 0x007692A0, &ni::ColorCreateClone);
NIEXTRA2_REVERSED("NiColorExtraData::DeletingDestructor", 0x007693F0, &ni::ColorDeletingDestructor);
NIEXTRA2_REVERSED("NiColorExtraData::IsEqual", 0x00769420, &ni::ColorIsEqual);
NIEXTRA2_REVERSED("NiColorExtraData::GetViewerStrings", 0x00769460, &ni::ColorGetViewerStrings);
NIEXTRA2_REVERSED("NiFloatsExtraData::SetArray", 0x0076C8B0, &ni::FloatsSetArray);
NIEXTRA2_REVERSED("NiFloatsExtraData::SetValue", 0x0076C9B0, &ni::FloatsSetValue);
NIEXTRA2_REVERSED("NiFloatsExtraData::CopyMembers", 0x0076C9F0, &ni::FloatsCopyMembers);
NIEXTRA2_REVERSED("NiFloatsExtraData::GetRTTI", 0x0076CA60, &ni::FloatsGetRTTI);
NIEXTRA2_REVERSED("NiFloatsExtraData::NiFloatsExtraData", 0x0076CA70, &ni::FloatsConstruct);
NIEXTRA2_REVERSED("NiFloatsExtraData::CreateClone", 0x0076CAA0, &ni::FloatsCreateClone);
NIEXTRA2_REVERSED("NiFloatsExtraData::DeletingDestructor", 0x0076CB40, &ni::FloatsDeletingDestructor);
NIEXTRA2_REVERSED("NiFloatsExtraData::SaveBinary", 0x0076CB80, &ni::FloatsSaveBinary);
NIEXTRA2_REVERSED("NiFloatsExtraData::IsEqual", 0x0076CBF0, &ni::FloatsIsEqual);
NIEXTRA2_REVERSED("NiFloatsExtraData::GetViewerStrings", 0x0076CC80, &ni::FloatsGetViewerStrings);
NIEXTRA2_REVERSED("NiIntegersExtraData::SetArray", 0x0076D960, &ni::IntegersSetArray);
NIEXTRA2_REVERSED("NiIntegersExtraData::CopyMembers", 0x0076D9D0, &ni::IntegersCopyMembers);
NIEXTRA2_REVERSED("NiIntegersExtraData::GetRTTI", 0x0076DA40, &ni::IntegersGetRTTI);
NIEXTRA2_REVERSED("NiIntegersExtraData::NiIntegersExtraData", 0x0076DA50, &ni::IntegersConstruct);
NIEXTRA2_REVERSED("NiIntegersExtraData::CreateClone", 0x0076DA80, &ni::IntegersCreateClone);
NIEXTRA2_REVERSED("NiIntegersExtraData::DeletingDestructor", 0x0076DB20, &ni::IntegersDeletingDestructor);
NIEXTRA2_REVERSED("NiFloatsExtraData::LoadBinary", 0x0076DB60, &ni::FloatsLoadBinary);
NIEXTRA2_REVERSED("NiIntegersExtraData::IsEqual", 0x0076DBF0, &ni::IntegersIsEqual);
NIEXTRA2_REVERSED("NiIntegersExtraData::GetViewerStrings", 0x0076DC60, &ni::IntegersGetViewerStrings);
