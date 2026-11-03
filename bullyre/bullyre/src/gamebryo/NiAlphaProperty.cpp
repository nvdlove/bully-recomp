// NiAlphaProperty and NiZBufferProperty (src/gamebryo/NiAlphaProperty.h): cloning, streaming,
// comparison, viewer strings. Also NiTexturingProperty::CreateClone, which sits just before
// this code, and the shared NiProperty::LinkObject thunk.

#include <cstdint>
#include <cstring>

#include "gamebryo/NiAlphaProperty.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "gamebryo/NiTexturingProperty.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiAlphaProperty_RTTI, 0x020E2AF0);
GAME_VAR(RTTI, NiZBufferProperty_RTTI, 0x020E2B28);

constexpr uintptr_t NiAlphaProperty_vtable = 0x008FFB9C;
constexpr uintptr_t NiZBufferProperty_vtable = 0x008FFCA4;

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(char*, __cdecl, GetViewerStringByte, 0x00785600, const char* prefix, uint32_t value);
GAME_FN(void, __thiscall, Property_LoadBinary, 0x007548C0, void* self, void* stream);
GAME_FN(void, __thiscall, Property_CopyMembers, 0x007654C0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Property_SaveBinary, 0x007654E0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Property_IsEqual, 0x007654F0, void* self, void* other);
GAME_FN(void, __thiscall, Property_GetViewerStrings, 0x00765530, void* self, NiTArray<char*>* strings);
GAME_FN(NiTexturingProperty*, __thiscall, TexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void, __thiscall, TexturingProperty_CopyMembers, 0x00767620, NiTexturingProperty* self,
        NiTexturingProperty* dest, void* cloning);
// (this chunk's, called where modes apply)
GAME_FN(char*, __cdecl, AlphaFunctionViewerString, 0x0076D050, const char* prefix, uint32_t function);
GAME_FN(char*, __cdecl, TestFunctionViewerString, 0x0076D1D0, const char* prefix, uint32_t function);

// ---- NiTexturingProperty

// vf3 CreateClone
NiTexturingProperty* __fastcall TexturingCreateClone(NiTexturingProperty* self, re::Edx, void* cloning) {
    void* p = NiNew(0x34);
    if (p != nullptr) {
        NiTexturingProperty* clone = TexturingProperty_Construct(p);
        TexturingProperty_CopyMembers(self, clone, cloning);
        return clone;
    }
    TexturingProperty_CopyMembers(self, nullptr, cloning);
    return nullptr;
}

// ---- NiAlphaProperty

// vf8: every field of the flags and the test reference
uint8_t __fastcall AlphaIsEqual(NiAlphaProperty* self, re::Edx, NiAlphaProperty* other) {
    uint8_t equal = Property_IsEqual(self, other);
    if (!equal) {
        return equal;
    }
    uint32_t a = self->flags;
    uint32_t b = other->flags;
    uint32_t diff = a ^ b;
    if ((diff & 1) || (diff & 0x1E) || (diff & 0x1E0) || ((diff >> 9) & 1) || (diff & 0x1C00) ||
        self->testRef != other->testRef || ((diff >> 13) & 1) || ((diff >> 14) & 1) || ((diff >> 15) & 1)) {
        return 0;
    }
    return 1;
}

// vf3 CreateClone
NiAlphaProperty* __fastcall AlphaCreateClone(NiAlphaProperty* self, re::Edx, void* cloning) {
    NiAlphaProperty* clone = static_cast<NiAlphaProperty*>(NiNew(0x1C));
    if (clone != nullptr) {
        ObjectNET_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiAlphaProperty_vtable);
        clone->testRef = 0;
        clone->flags = 0xEC;
    } else {
        clone = nullptr;
    }
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    clone->testRef = self->testRef;
    return clone;
}

void __fastcall AlphaLoadBinary(NiAlphaProperty* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    LoadBytes(stream, &self->testRef, 1);
}

void __fastcall AlphaSaveBinary(NiAlphaProperty* self, re::Edx, void* stream) {
    Property_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
    SaveBytes(stream, &self->testRef, 1);
}

// "prefix = ALPHA_..." (a blend function)
char* __cdecl AlphaFunctionString(const char* prefix, uint32_t function) {
    static const char* const kFormats[] = {
        "%s = ALPHA_ONE",         "%s = ALPHA_ZERO",          "%s = ALPHA_SRCCOLOR",
        "%s = ALPHA_INVSRCCOLOR", "%s = ALPHA_DESTCOLOR",     "%s = ALPHA_INVDESTCOLOR",
        "%s = ALPHA_SRCALPHA",    "%s = ALPHA_INVSRCALPHA",   "%s = ALPHA_DESTALPHA",
        "%s = ALPHA_INVDESTALPHA", "%s = ALPHA_SRCALPHASAT",
    };
    size_t size = std::strlen(prefix) + 0x16;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, function <= 10 ? kFormats[function] : "%s = UNKNOWN!!!", prefix);
    return text;
}

// "prefix = TEST_..." (an alpha test function)
char* __cdecl TestFunctionString(const char* prefix, uint32_t function) {
    static const char* const kFormats[] = {
        "%s = TEST_ALWAYS",  "%s = TEST_LESS",     "%s = TEST_EQUAL",        "%s = TEST_LESSEQUAL",
        "%s = TEST_GREATER", "%s = TEST_NOTEQUAL", "%s = TEST_GREATEREQUAL", "%s = TEST_NEVER",
    };
    size_t size = std::strlen(prefix) + 0x16;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, function <= 7 ? kFormats[function] : "%s = UNKNOWN!!!", prefix);
    return text;
}

// vf9
void __fastcall AlphaGetViewerStrings(NiAlphaProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiAlphaProperty_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bAlpha", self->flags & 1));
    AddViewerString(strings, AlphaFunctionViewerString("m_eSrcBlend", (self->flags >> 1) & 0xF));
    AddViewerString(strings, AlphaFunctionViewerString("m_eDestBlend", (self->flags >> 5) & 0xF));
    AddViewerString(strings, GetViewerStringBool("m_bAlphaTest", (self->flags >> 9) & 1));
    AddViewerString(strings, TestFunctionViewerString("m_eTestMode", (self->flags >> 10) & 7));
    AddViewerString(strings, GetViewerStringByte("m_ucTestRef", self->testRef));
    AddViewerString(strings, GetViewerStringBool("m_bNoSorter", (self->flags >> 13) & 1));
}

// ---- NiZBufferProperty

// vf8: the flags' fields and Bully's four floats (not the word at +0x24)
uint8_t __fastcall ZBufferIsEqual(NiZBufferProperty* self, re::Edx, NiZBufferProperty* other) {
    if (!Property_IsEqual(self, other)) {
        return 0;
    }
    uint32_t diff = uint32_t(self->flags ^ other->flags) & 0xFF;
    if ((diff & 1) || ((diff >> 1) & 1) || (diff & 0x3C)) {
        return 0;
    }
    if (!(other->unknown1C == self->unknown1C) || !(other->unknown20 == self->unknown20)) {
        return 0;
    }
    if (diff & 0xC0) {
        return 0;
    }
    if (!(other->unknown28 == self->unknown28) || !(other->unknown2C == self->unknown2C)) {
        return 0;
    }
    return 1;
}

NiZBufferProperty* __fastcall ZBufferCreateClone(NiZBufferProperty* self, re::Edx, void* cloning) {
    NiZBufferProperty* clone = static_cast<NiZBufferProperty*>(NiNew(0x30));
    if (clone != nullptr) {
        ObjectNET_Construct(clone);
        clone->unknown1C = 0.0f;
        clone->vtable = reinterpret_cast<void*>(NiZBufferProperty_vtable);
        clone->unknown20 = 0.0f;
        clone->unknown24 = 0;
        clone->flags = 0xF;
        clone->unknown28 = 1.0f;
        clone->unknown2C = 1.0f;
    } else {
        clone = nullptr;
    }
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    clone->unknown1C = self->unknown1C;
    clone->unknown20 = self->unknown20;
    clone->unknown24 = self->unknown24;
    clone->unknown28 = self->unknown28;
    clone->unknown2C = self->unknown2C;
    return clone;
}

// vf4: files before 20.1.0.2 have the test function as a word of its own
void __fastcall ZBufferLoadBinary(NiZBufferProperty* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    if (StreamVersion(stream) < 0x14010002) {
        uint32_t function;
        LoadBytes(stream, &function, 4);
        self->flags = uint16_t((self->flags & 0xFFC3) | uint16_t(function * 4));
    }
}

// vf9 (a test function over 7 reads past the game's table of 8 names: whatever its stack
// holds there; here, null)
void __fastcall ZBufferGetViewerStrings(NiZBufferProperty* self, re::Edx, NiTArray<char*>* strings) {
    static const char* const kTests[16] = {
        "TEST_ALWAYS",  "TEST_LESS",     "TEST_EQUAL",        "TEST_LESSEQUAL",
        "TEST_GREATER", "TEST_NOTEQUAL", "TEST_GREATEREQUAL", "TEST_NEVER",
    };
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiZBufferProperty_RTTI.name));
    uint8_t flags = uint8_t(self->flags);
    AddViewerString(strings, GetViewerStringBool("m_bZTest", flags & 1));
    AddViewerString(strings, GetViewerStringBool("m_bZWrite", (flags >> 1) & 1));
    AddViewerString(strings, GetViewerStringString("Test Function", kTests[(flags >> 2) & 0xF]));
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIALPHA_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIALPHA_REVERSED("NiTexturingProperty::CreateClone", 0x007680C0, &ni::TexturingCreateClone);
NIALPHA_REVERSED("NiAlphaProperty::IsEqual", 0x0076CDB0, &ni::AlphaIsEqual);
NIALPHA_REVERSED("NiAlphaProperty::CreateClone", 0x0076CE60, &ni::AlphaCreateClone);
NIALPHA_REVERSED("NiAlphaProperty::LoadBinary", 0x0076CEF0, &ni::AlphaLoadBinary);
NIALPHA_REVERSED("NiAlphaProperty::SaveBinary", 0x0076CF50, &ni::AlphaSaveBinary);
NIALPHA_REVERSED("NiAlphaProperty::AlphaFunctionViewerString", 0x0076D050, &ni::AlphaFunctionString);
NIALPHA_REVERSED("NiAlphaProperty::TestFunctionViewerString", 0x0076D1D0, &ni::TestFunctionString);
NIALPHA_REVERSED("NiAlphaProperty::GetViewerStrings", 0x0076D300, &ni::AlphaGetViewerStrings);
RE_UNHOOKED("NiProperty::LinkObject", 0x0076D540);   // a jmp to NiTexture::LinkObject (0x7548D0)
NIALPHA_REVERSED("NiZBufferProperty::IsEqual", 0x0076D550, &ni::ZBufferIsEqual);
NIALPHA_REVERSED("NiZBufferProperty::CreateClone", 0x0076D5E0, &ni::ZBufferCreateClone);
NIALPHA_REVERSED("NiZBufferProperty::LoadBinary", 0x0076D6B0, &ni::ZBufferLoadBinary);
NIALPHA_REVERSED("NiZBufferProperty::GetViewerStrings", 0x0076D7F0, &ni::ZBufferGetViewerStrings);
