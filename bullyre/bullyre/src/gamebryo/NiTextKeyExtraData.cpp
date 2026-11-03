// NiTextKeyExtraData (vtable 0x965DC0, 0x14 bytes: an array of text keys, a time and a fixed
// string each, NiNew[]'d with the count before them) and NiStringPalette (vtable 0x965E38, 0x14:
// a buffer of strings with its allocated size and its used end). Construction, cloning,
// streaming, comparison, viewer strings.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {

namespace {

using scene::AddViewerString;
using scene::AddViewerStringInline;
using scene::BinaryStream;
using scene::GetViewerString;
using scene::GetViewerStringFloat;
using scene::GetViewerStringString;
using scene::GetViewerStringUint;
using scene::LoadBytes;
using scene::NullFixedString;
using scene::Object_IsEqual;
using scene::Object_LoadBinary;
using scene::RefObject_Construct;
using scene::SaveBytes;
using scene::At;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiTextKeyExtraData_RTTI, 0x020E4B74);
GAME_VAR(RTTI, NiStringPalette_RTTI, 0x020E4BA8);

constexpr uintptr_t NiTextKeyExtraData_vtable = 0x00965DC0;
constexpr uintptr_t NiStringPalette_vtable = 0x00965E38;
constexpr uintptr_t TextKey_Construct_address = 0x00806B70;   // (this file's, the array's constructor)

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void*, __cdecl, NiAllocArray, 0x007524D0, uint32_t size);              // NiNew[]'s
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __thiscall, Object_Destruct, 0x007769D0, void* self);
GAME_FN(void, __thiscall, Object_SaveBinaryNone, 0x008A20A0, void* self, void* stream);
// NiExtraData's
GAME_FN(void, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, ExtraData_Destruct, 0x00765670, void* self);
GAME_FN(uint8_t, __thiscall, ExtraData_RegisterStreamables, 0x00765580, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_SaveBinary, 0x007655B0, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_LoadBinary, 0x007655D0, void* self, void* stream);
GAME_FN(void, __thiscall, ExtraData_CopyMembers, 0x007656C0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, ExtraData_GetViewerStrings, 0x00765780, void* self, NiTArray<char*>* strings);
// NiTextKey's (the next chunk's)
GAME_FN(void, __thiscall, TextKey_RegisterStreamables, 0x00826060, NiTextKey* self, void* stream);
GAME_FN(void, __thiscall, TextKey_Destruct, 0x00826080, NiTextKey* self);
GAME_FN(void, __thiscall, TextKey_SetText, 0x008260C0, NiTextKey* self, const re::Ptr32<const char>* text);
GAME_FN(void, __thiscall, TextKey_LoadBinary, 0x008260D0, NiTextKey* self, void* stream);
GAME_FN(void, __thiscall, TextKey_SaveBinary, 0x00826130, NiTextKey* self, void* stream);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, TextKeyExtraData_Destruct, 0x00806B80, NiTextKeyExtraData* self);
GAME_FN(void, __thiscall, TextKeyExtraData_CopyMembers, 0x00806BD0, NiTextKeyExtraData* self,
        NiTextKeyExtraData* dest, void* cloning);

// a key array NiNew[]'d (the count before the keys), each key constructed
inline NiTextKey* NewKeys(uint32_t count) {
    uint32_t* block = static_cast<uint32_t*>(NiAllocArray(KeyArraySize(count, 8)));
    if (block == nullptr) {
        return nullptr;
    }
    block[0] = count;
    NiTextKey* keys = reinterpret_cast<NiTextKey*>(block + 1);
    VectorConstruct(keys, 8, count, reinterpret_cast<void*>(TextKey_Construct_address));
    return keys;
}

// ---- NiTextKeyExtraData

// 0x806A60: RegisterStreamables (each key's text too)
uint8_t __fastcall TextKeys_RegisterStreamables(NiTextKeyExtraData* self, re::Edx, void* stream) {
    if (!ExtraData_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (uint32_t i = 0; i < self->numKeys; i++) {
        TextKey_RegisterStreamables(&self->keys[i], stream);
    }
    return 1;
}

const RTTI* __fastcall TextKeys_GetRTTI(NiTextKeyExtraData*, re::Edx) {
    return &NiTextKeyExtraData_RTTI;
}

// 0x806AB0: CreateObject
NiObject* __cdecl TextKeys_CreateObject() {
    void* memory = NiNew(0x14);
    if (memory == nullptr) {
        return nullptr;
    }
    NiTextKeyExtraData* self = static_cast<NiTextKeyExtraData*>(memory);
    ExtraData_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiTextKeyExtraData_vtable);
    self->numKeys = 0;
    self->keys = nullptr;
    return self;
}

// 0x806AF0: IsEqual (the keys' times and text handles)
uint8_t __fastcall TextKeys_IsEqual(NiTextKeyExtraData* self, re::Edx, NiTextKeyExtraData* other) {
    if (other == nullptr || !Object_IsEqual(self, other)) {
        return 0;
    }
    uint32_t count = self->numKeys;
    if (count != other->numKeys) {
        return 0;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (other->keys[i].time != self->keys[i].time) {
            return 0;
        }
        if (self->keys[i].text != other->keys[i].text) {
            return 0;
        }
    }
    return 1;
}

// 0x806B70: NiTextKey() (the text the empty handle)
NiTextKey* __fastcall TextKey_Construct(NiTextKey* self, re::Edx) {
    self->text = NullFixedString;
    return self;
}

// 0x806B80: ~NiTextKeyExtraData (the keys destroyed last to first, the array freed)
void __fastcall TextKeys_Destruct(NiTextKeyExtraData* self, re::Edx) {
    NiTextKey* keys = self->keys;
    self->vtable = reinterpret_cast<void*>(NiTextKeyExtraData_vtable);
    if (keys != nullptr) {
        uint32_t* block = reinterpret_cast<uint32_t*>(keys) - 1;
        for (int32_t i = int32_t(block[0]) - 1; i >= 0; i--) {
            TextKey_Destruct(&keys[i]);
        }
        NiFreeArray(block, 8);
    }
    ExtraData_Destruct(self);
}

// 0x806BD0: CopyMembers (the keys copied only for this exact class: a derived one's clone gets
// none)
void __fastcall TextKeys_CopyMembers(NiTextKeyExtraData* self, re::Edx, NiTextKeyExtraData* dest, void* cloning) {
    ExtraData_CopyMembers(self, dest, cloning);
    dest->numKeys = self->numKeys;
    if (self->numKeys == 0 || Virtual<const RTTI*>(self, 2) != &NiTextKeyExtraData_RTTI) {
        dest->keys = nullptr;
        return;
    }
    dest->keys = NewKeys(self->numKeys);
    for (uint32_t i = 0; i < self->numKeys; i++) {
        dest->keys[i].time = self->keys[i].time;
        TextKey_SetText(&dest->keys[i], &self->keys[i].text);
    }
}

// 0x806CA0: LoadBinary (the keys, when there are any)
void __fastcall TextKeys_LoadBinary(NiTextKeyExtraData* self, re::Edx, void* stream) {
    ExtraData_LoadBinary(self, stream);
    uint32_t count;
    LoadBytes(stream, &count, 4);
    if (count == 0) {
        return;
    }
    NiTextKey* keys = NewKeys(count);
    for (uint32_t i = 0; i < count; i++) {
        TextKey_LoadBinary(&keys[i], stream);
    }
    self->numKeys = count;
    self->keys = keys;
}

// 0x806D70: SaveBinary
void __fastcall TextKeys_SaveBinary(NiTextKeyExtraData* self, re::Edx, void* stream) {
    ExtraData_SaveBinary(self, stream);
    SaveBytes(stream, &self->numKeys, 4);
    for (uint32_t i = 0; i < self->numKeys; i++) {
        TextKey_SaveBinary(&self->keys[i], stream);
    }
}

// 0x806DD0: the deleting destructor
NiTextKeyExtraData* __fastcall TextKeys_DeletingDestructor(NiTextKeyExtraData* self, re::Edx, uint32_t flags) {
    TextKeyExtraData_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// 0x806DF0: CreateClone (a failed allocation still copied onto null)
NiTextKeyExtraData* __fastcall TextKeys_CreateClone(NiTextKeyExtraData* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x14);
    if (memory == nullptr) {
        TextKeyExtraData_CopyMembers(self, nullptr, cloning);
        return nullptr;
    }
    NiTextKeyExtraData* clone = static_cast<NiTextKeyExtraData*>(memory);
    ExtraData_Construct(clone);
    clone->vtable = reinterpret_cast<void*>(NiTextKeyExtraData_vtable);
    clone->numKeys = 0;
    clone->keys = nullptr;
    TextKeyExtraData_CopyMembers(self, clone, cloning);
    return clone;
}

// 0x806E50: GetViewerStrings ("Time = ..." and "Text = ..." joined per key)
void __fastcall TextKeys_GetViewerStrings(NiTextKeyExtraData* self, re::Edx, NiTArray<char*>* strings) {
    ExtraData_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiTextKeyExtraData_RTTI.name));
    AddViewerString(strings, GetViewerStringUint("m_uiNumKeys", self->numKeys));
    for (uint32_t i = 0; i < self->numKeys; i++) {
        char* time = GetViewerStringFloat("Time", self->keys[i].time);
        char* text = GetViewerStringString("Text", self->keys[i].text);
        size_t size = std::strlen(time) + std::strlen(text) + 4;
        char* joined = static_cast<char*>(NiMalloc(uint32_t(size)));
        NiSprintf(joined, size, "%s : %s", time, text);
        AddViewerStringInline(strings, GetViewerStringString("TextKey", joined));
        NiFree(time);
        NiFree(text);
        NiFree(joined);
    }
}

// ---- NiStringPalette

// 0x807030: IsEqual (the buffers compared over the allocated size)
uint8_t __fastcall Palette_IsEqual(NiStringPalette* self, re::Edx, NiStringPalette* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    uint32_t size = self->allocated;
    if (other->allocated != size || other->end != self->end) {
        return 0;
    }
    if (size == 0) {
        return 1;
    }
    return std::memcmp(other->buffer, self->buffer, size) == 0 ? 1 : 0;
}

const RTTI* __fastcall Palette_GetRTTI(NiStringPalette*, re::Edx) {
    return &NiStringPalette_RTTI;
}

// 0x807110: CreateObject
NiObject* __cdecl Palette_CreateObject() {
    void* memory = NiNew(0x14);
    if (memory == nullptr) {
        return nullptr;
    }
    NiStringPalette* self = static_cast<NiStringPalette*>(memory);
    RefObject_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiStringPalette_vtable);
    self->buffer = nullptr;
    self->allocated = 0;
    self->end = 0;
    return self;
}

// 0x807150: the deleting destructor
NiStringPalette* __fastcall Palette_DeletingDestructor(NiStringPalette* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiStringPalette_vtable);
    NiFree(self->buffer);
    Object_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x14);
    }
    return self;
}

// the stream's raw bytes, one byte a component
inline void LoadRaw(void* stream, void* buffer, uint32_t bytes) {
    BinaryStream* in = At<BinaryStream*>(stream, 0x188);
    uint32_t componentSize = 1;
    in->position += in->read(in, buffer, bytes, &componentSize, 1);
}

inline void SaveRaw(void* stream, const void* buffer, uint32_t bytes) {
    BinaryStream* out = At<BinaryStream*>(stream, 0x18C);
    uint32_t componentSize = 1;
    out->position += out->write(out, buffer, bytes, &componentSize, 1);
}

// 0x807190: SaveBinary (the used bytes saved as the size, then the end)
void __fastcall Palette_SaveBinary(NiStringPalette* self, re::Edx, void* stream) {
    Object_SaveBinaryNone(self, stream);
    uint32_t size = self->end;
    SaveBytes(stream, &size, 4);
    if (size != 0) {
        SaveRaw(stream, self->buffer, size);
    }
    SaveBytes(stream, &self->end, 4);
}

// 0x807230: LoadBinary
void __fastcall Palette_LoadBinary(NiStringPalette* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    LoadBytes(stream, &self->allocated, 4);
    if (self->allocated != 0) {
        self->buffer = static_cast<char*>(NiMalloc(self->allocated));
        LoadRaw(stream, self->buffer, self->allocated);
    }
    LoadBytes(stream, &self->end, 4);
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC30_TEXTKEY_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

// (0x806A50: a lone jmp to NiObject's empty SaveBinary/LinkObject 0x8A20A0, the slot many
// classes share)
RE_UNHOOKED("NiObject::LinkObject (thunk 0x806A50)", 0x00806A50);

NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::RegisterStreamables", 0x00806A60,
                       &ni::anim::TextKeys_RegisterStreamables);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::GetRTTI", 0x00806AA0, &ni::anim::TextKeys_GetRTTI);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::CreateObject", 0x00806AB0, &ni::anim::TextKeys_CreateObject);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::IsEqual", 0x00806AF0, &ni::anim::TextKeys_IsEqual);
NIC30_TEXTKEY_REVERSED("NiTextKey::NiTextKey", 0x00806B70, &ni::anim::TextKey_Construct);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::~NiTextKeyExtraData", 0x00806B80, &ni::anim::TextKeys_Destruct);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::CopyMembers", 0x00806BD0, &ni::anim::TextKeys_CopyMembers);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::LoadBinary", 0x00806CA0, &ni::anim::TextKeys_LoadBinary);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::SaveBinary", 0x00806D70, &ni::anim::TextKeys_SaveBinary);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::DeletingDestructor", 0x00806DD0, &ni::anim::TextKeys_DeletingDestructor);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::CreateClone", 0x00806DF0, &ni::anim::TextKeys_CreateClone);
NIC30_TEXTKEY_REVERSED("NiTextKeyExtraData::GetViewerStrings", 0x00806E50, &ni::anim::TextKeys_GetViewerStrings);
NIC30_TEXTKEY_REVERSED("NiStringPalette::IsEqual", 0x00807030, &ni::anim::Palette_IsEqual);
NIC30_TEXTKEY_REVERSED("NiStringPalette::GetRTTI", 0x00807100, &ni::anim::Palette_GetRTTI);
NIC30_TEXTKEY_REVERSED("NiStringPalette::CreateObject", 0x00807110, &ni::anim::Palette_CreateObject);
NIC30_TEXTKEY_REVERSED("NiStringPalette::DeletingDestructor", 0x00807150, &ni::anim::Palette_DeletingDestructor);
NIC30_TEXTKEY_REVERSED("NiStringPalette::SaveBinary", 0x00807190, &ni::anim::Palette_SaveBinary);
NIC30_TEXTKEY_REVERSED("NiStringPalette::LoadBinary", 0x00807230, &ni::anim::Palette_LoadBinary);
