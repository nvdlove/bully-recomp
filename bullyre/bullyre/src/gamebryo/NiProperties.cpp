// NiMaterialProperty, NiFogProperty, NiStencilProperty, NiVertexColorProperty and NiShadeProperty
// (src/gamebryo/NiProperties.h): construction and cloning, streaming (the old formats' separate
// values packed into the flags), comparison, viewer strings (with the enum names).

#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiProperties.h"
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
GAME_VAR(RTTI, NiMaterialProperty_RTTI, 0x020E2D04);
GAME_VAR(RTTI, NiFogProperty_RTTI, 0x020E2EF8);
GAME_VAR(RTTI, NiShadeProperty_RTTI, 0x020E2F30);
GAME_VAR(RTTI, NiStencilProperty_RTTI, 0x020E2F68);
GAME_VAR(RTTI, NiVertexColorProperty_RTTI, 0x020E3008);

constexpr uintptr_t NiMaterialProperty_vtable = 0x009530B0;
constexpr uintptr_t NiFogProperty_vtable = 0x009179FC;
constexpr uintptr_t NiShadeProperty_vtable = 0x00917A54;
constexpr uintptr_t NiStencilProperty_vtable = 0x00917AAC;
constexpr uintptr_t NiVertexColorProperty_vtable = 0x0092D0D4;

GAME_VAR(int32_t, NiMaterialProperty_ms_iNextIndex, 0x00BC8A70);
GAME_VAR(NiPoint3, NiColor_BLACK, 0x020E25E8);

GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __cdecl, MaterialProperty_PurgeRendererData, 0x007556A0, NiMaterialProperty* self);
GAME_FN(void, __thiscall, Point3_SaveBinary, 0x00762550, const NiPoint3* self, void* stream);
GAME_FN(char*, __thiscall, Color_GetViewerString, 0x00762710, const NiPoint3* self, const char* prefix);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
// NiProperty's (another chunk's)
GAME_FN(void, __thiscall, Property_LoadBinary, 0x007548C0, void* self, void* stream);
GAME_FN(void, __thiscall, Property_CopyMembers, 0x007654C0, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Property_SaveBinary, 0x007654E0, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, Property_IsEqual, 0x007654F0, void* self, void* other);
GAME_FN(void, __thiscall, Property_GetViewerStrings, 0x00765530, void* self, NiTArray<char*>* strings);
// (this chunk's, called where modes apply)
GAME_FN(void, __thiscall, MaterialProperty_CopyMembers, 0x007755E0, NiMaterialProperty* self,
        NiMaterialProperty* dest, void* cloning);
GAME_FN(NiMaterialProperty*, __thiscall, MaterialProperty_Construct, 0x00775660, NiMaterialProperty* self);
GAME_FN(void, __thiscall, Point3_LoadBinary, 0x007768A0, NiPoint3* self, void* stream);
GAME_FN(char*, __cdecl, FogFunctionViewerString, 0x00778480, const char* prefix, uint32_t function);
GAME_FN(char*, __cdecl, TestFunctionViewerString, 0x00778C70, const char* prefix, uint32_t function);
GAME_FN(char*, __cdecl, ActionViewerString, 0x00778DA0, const char* prefix, uint32_t action);
GAME_FN(char*, __cdecl, DrawModeViewerString, 0x00778E90, const char* prefix, uint32_t mode);
GAME_FN(char*, __cdecl, SourceModeViewerString, 0x00779710, const char* prefix, uint32_t mode);
GAME_FN(char*, __cdecl, LightingModeViewerString, 0x007797B0, const char* prefix, uint32_t mode);

// "%s = NAME" in a NiMalloc'd buffer of the prefix's length plus `extra`
inline char* EnumString(const char* prefix, uint32_t extra, const char* format) {
    size_t size = std::strlen(prefix) + extra;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    NiSprintf(text, size, format, prefix);
    return text;
}

// ---- NiMaterialProperty

NiMaterialProperty* __fastcall MaterialConstruct(NiMaterialProperty* self, re::Edx) {
    ObjectNET_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiMaterialProperty_vtable);
    self->ambient = {0.5f, 0.5f, 0.5f};
    self->diffuse = {0.5f, 0.5f, 0.5f};
    self->specular = {0.0f, 0.0f, 0.0f};
    self->emissive = {0.0f, 0.0f, 0.0f};
    self->shine = 4.0f;
    self->alpha = 1.0f;
    self->index = ni::sys::Increment(&NiMaterialProperty_ms_iNextIndex);
    self->revisionID = 1;
    self->rendererData = nullptr;
    return self;
}

// CopyMembers: the colors, shine and alpha (not the index or the revision)
void __fastcall MaterialCopyMembers(NiMaterialProperty* self, re::Edx, NiMaterialProperty* dest, void* cloning) {
    Property_CopyMembers(self, dest, cloning);
    dest->ambient = self->ambient;
    dest->diffuse = self->diffuse;
    dest->specular = self->specular;
    dest->emissive = self->emissive;
    dest->shine = self->shine;
    dest->alpha = self->alpha;
}

const RTTI* __fastcall MaterialGetRTTI(NiMaterialProperty*, re::Edx) {
    return &NiMaterialProperty_RTTI;
}

// vf3 CreateClone (CopyMembers runs with a null clone when the allocation failed)
NiMaterialProperty* __fastcall MaterialCreateClone(NiMaterialProperty* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x5C);
    NiMaterialProperty* clone =
        memory != nullptr ? MaterialProperty_Construct(static_cast<NiMaterialProperty*>(memory)) : nullptr;
    MaterialProperty_CopyMembers(self, clone, cloning);
    return clone;
}

// the deleting destructor: the renderer's data let go first
NiMaterialProperty* __fastcall MaterialDestruct(NiMaterialProperty* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiMaterialProperty_vtable);
    MaterialProperty_PurgeRendererData(self);
    ObjectNET_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x5C);
    }
    return self;
}

void __fastcall MaterialLoadBinary(NiMaterialProperty* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    Point3_LoadBinary(&self->ambient, stream);
    Point3_LoadBinary(&self->diffuse, stream);
    Point3_LoadBinary(&self->specular, stream);
    Point3_LoadBinary(&self->emissive, stream);
    LoadBytes(stream, &self->shine, 4);
    LoadBytes(stream, &self->alpha, 4);
}

void __fastcall MaterialSaveBinary(NiMaterialProperty* self, re::Edx, void* stream) {
    Property_SaveBinary(self, stream);
    Point3_SaveBinary(&self->ambient, stream);
    Point3_SaveBinary(&self->diffuse, stream);
    Point3_SaveBinary(&self->specular, stream);
    Point3_SaveBinary(&self->emissive, stream);
    SaveBytes(stream, &self->shine, 4);
    SaveBytes(stream, &self->alpha, 4);
}

uint8_t __fastcall MaterialIsEqual(NiMaterialProperty* self, re::Edx, NiMaterialProperty* other) {
    if (!Property_IsEqual(self, other)) {
        return 0;
    }
    if (Point3_NotEqual(&self->ambient, &other->ambient) || Point3_NotEqual(&self->diffuse, &other->diffuse) ||
        Point3_NotEqual(&self->specular, &other->specular) || Point3_NotEqual(&self->emissive, &other->emissive)) {
        return 0;
    }
    if (self->shine != other->shine || self->alpha != other->alpha) {
        return 0;
    }
    return 1;
}

void __fastcall MaterialGetViewerStrings(NiMaterialProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiMaterialProperty_RTTI.name));
    AddViewerString(strings, Color_GetViewerString(&self->ambient, "m_amb"));
    AddViewerString(strings, Color_GetViewerString(&self->diffuse, "m_diff"));
    AddViewerString(strings, Color_GetViewerString(&self->specular, "m_spec"));
    AddViewerString(strings, Color_GetViewerString(&self->emissive, "m_emit"));
    AddViewerString(strings, GetViewerStringFloat("m_fShine", self->shine));
    AddViewerString(strings, GetViewerStringFloat("m_fAlpha", self->alpha));
}

// ---- NiFogProperty

// vf3 CreateClone: the constructor inlined (a null clone isn't checked past it)
NiFogProperty* __fastcall FogCreateClone(NiFogProperty* self, re::Edx, void* cloning) {
    NiFogProperty* clone = static_cast<NiFogProperty*>(NiNew(0x2C));
    if (clone != nullptr) {
        ObjectNET_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiFogProperty_vtable);
        clone->color = {0.0f, 0.0f, 0.0f};
        clone->flags = 0;
        clone->depth = 1.0f;
        clone->color = NiColor_BLACK;
    } else {
        clone = nullptr;
    }
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    clone->depth = self->depth;
    clone->color = self->color;
    return clone;
}

void __fastcall FogLoadBinary(NiFogProperty* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    LoadBytes(stream, &self->depth, 4);
    Point3_LoadBinary(&self->color, stream);
}

void __fastcall FogSaveBinary(NiFogProperty* self, re::Edx, void* stream) {
    Property_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
    SaveBytes(stream, &self->depth, 4);
    Point3_SaveBinary(&self->color, stream);
}

// vf8: on/off, depth, function, color
uint8_t __fastcall FogIsEqual(NiFogProperty* self, re::Edx, NiFogProperty* other) {
    if (!Property_IsEqual(self, other)) {
        return 0;
    }
    if (((uint8_t(self->flags) ^ uint8_t(other->flags)) & 1) != 0) {
        return 0;
    }
    if (self->depth != other->depth) {
        return 0;
    }
    if (((uint8_t(self->flags) ^ uint8_t(other->flags)) & 6) != 0) {
        return 0;
    }
    if (Point3_NotEqual(&self->color, &other->color)) {
        return 0;
    }
    return 1;
}

// the fog function's name (an unknown one: the buffer as allocated)
char* __cdecl FogFunctionString(const char* prefix, uint32_t function) {
    size_t size = std::strlen(prefix) + 0x14;
    char* text = static_cast<char*>(NiMalloc(uint32_t(size)));
    if (function == 0) {
        NiSprintf(text, size, "%s = FOG_Z_LINEAR", prefix);
    } else if (function == 1) {
        NiSprintf(text, size, "%s = FOG_RANGE_SQ", prefix);
    } else if (function == 2) {
        NiSprintf(text, size, "%s = FOG_VERTEX_ALPHA", prefix);
    }
    return text;
}

void __fastcall FogGetViewerStrings(NiFogProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiFogProperty_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("Enable", self->flags & 1));
    AddViewerString(strings, GetViewerStringFloat("Depth", self->depth));
    AddViewerString(strings, FogFunctionViewerString("Function", (uint8_t(self->flags) >> 1) & 3));
    AddViewerString(strings, Color_GetViewerString(&self->color, "Color"));
}

// ---- NiShadeProperty

NiFlagsProperty* __fastcall ShadeCreateClone(NiFlagsProperty* self, re::Edx, void* cloning) {
    NiFlagsProperty* clone = static_cast<NiFlagsProperty*>(NiNew(0x1C));
    if (clone != nullptr) {
        ObjectNET_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiShadeProperty_vtable);
        clone->flags = 1;
    } else {
        clone = nullptr;
    }
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    return clone;
}

void __fastcall ShadeGetViewerStrings(NiFlagsProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiShadeProperty_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("m_bSmooth", self->flags & 1));
}

// SaveBinary of the properties that are a flags word (z-buffer, shade, vertex colors, dither)
void __fastcall FlagsSaveBinary(NiFlagsProperty* self, re::Edx, void* stream) {
    Property_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
}

// ---- NiStencilProperty

uint8_t __fastcall StencilIsEqual(NiStencilProperty* self, re::Edx, NiStencilProperty* other) {
    if (!Property_IsEqual(self, other)) {
        return 0;
    }
    if (other->flags != self->flags || other->reference != self->reference || other->mask != self->mask) {
        return 0;
    }
    return 1;
}

NiStencilProperty* __fastcall StencilCreateClone(NiStencilProperty* self, re::Edx, void* cloning) {
    NiStencilProperty* clone = static_cast<NiStencilProperty*>(NiNew(0x24));
    if (clone != nullptr) {
        ObjectNET_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiStencilProperty_vtable);
        clone->reference = 0;
        clone->mask = 0xFFFFFFFF;
        clone->flags = 0x4180;
    } else {
        clone = nullptr;
    }
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    clone->reference = self->reference;
    clone->mask = self->mask;
    return clone;
}

// vf4: before 20.1.0.2 each setting was a value of its own (packed into the flags here, unmasked)
void __fastcall StencilLoadBinary(NiStencilProperty* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    if (StreamVersion(stream) < 0x14010002) {
        uint8_t enabled;
        LoadBytes(stream, &enabled, 1);
        if (enabled != 0) {
            self->flags |= 1;
        } else {
            self->flags &= 0xFFFE;
        }
        uint32_t value;
        LoadBytes(stream, &value, 4);
        self->flags = uint16_t((self->flags & 0x0FFF) | (value << 12));
        LoadBytes(stream, &self->reference, 4);
        LoadBytes(stream, &self->mask, 4);
        LoadBytes(stream, &value, 4);
        self->flags = uint16_t((self->flags & 0xFFF1) | (value * 2));
        LoadBytes(stream, &value, 4);
        self->flags = uint16_t((self->flags & 0xFF8F) | (value << 4));
        LoadBytes(stream, &value, 4);
        self->flags = uint16_t((self->flags & 0xFC7F) | (value << 7));
        LoadBytes(stream, &value, 4);
        self->flags = uint16_t((self->flags & 0xF3FF) | (value << 10));
        return;
    }
    LoadBytes(stream, &self->flags, 2);
    LoadBytes(stream, &self->reference, 4);
    LoadBytes(stream, &self->mask, 4);
}

void __fastcall StencilSaveBinary(NiStencilProperty* self, re::Edx, void* stream) {
    Property_SaveBinary(self, stream);
    SaveBytes(stream, &self->flags, 2);
    SaveBytes(stream, &self->reference, 4);
    SaveBytes(stream, &self->mask, 4);
}

char* __cdecl TestFunctionString(const char* prefix, uint32_t function) {
    static const char* const kNames[] = {"%s = TEST_NEVER",   "%s = TEST_LESS",     "%s = TEST_EQUAL",
                                         "%s = TEST_LESSEQUAL", "%s = TEST_GREATER", "%s = TEST_NOTEQUAL",
                                         "%s = TEST_GREATEREQUAL", "%s = TEST_ALWAYS"};
    return EnumString(prefix, 0x16, function < 8 ? kNames[function] : "%s = UNKNOWN!!!");
}

char* __cdecl ActionString(const char* prefix, uint32_t action) {
    static const char* const kNames[] = {"%s = ACTION_KEEP",      "%s = ACTION_ZERO",
                                         "%s = ACTION_REPLACE",   "%s = ACTION_INCREMENT",
                                         "%s = ACTION_DECREMENT", "%s = ACTION_INVERT"};
    return EnumString(prefix, 0x16, action < 6 ? kNames[action] : "%s = UNKNOWN!!!");
}

char* __cdecl DrawModeString(const char* prefix, uint32_t mode) {
    static const char* const kNames[] = {"%s = DRAW_CCW_OR_BOTH", "%s = DRAW_CCW", "%s = DRAW_CW",
                                         "%s = DRAW_BOTH"};
    return EnumString(prefix, 0x16, mode < 4 ? kNames[mode] : "%s = UNKNOWN!!!");
}

void __fastcall StencilGetViewerStrings(NiStencilProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiStencilProperty_RTTI.name));
    AddViewerString(strings, GetViewerStringBool("Enabled", self->flags & 1));
    AddViewerString(strings, GetViewerStringUint("m_uiRef", self->reference));
    AddViewerString(strings, GetViewerStringUint("m_uiMask", self->mask));
    AddViewerString(strings, TestFunctionViewerString("Test Funtion", self->flags >> 12));   // (sic)
    AddViewerString(strings, ActionViewerString("Test Fail Action", (uint8_t(self->flags) >> 1) & 7));
    AddViewerString(strings, ActionViewerString("Depth Fail Action", (uint8_t(self->flags) >> 4) & 7));
    AddViewerString(strings, ActionViewerString("Test Pass Action", (self->flags >> 7) & 7));
    AddViewerString(strings, DrawModeViewerString("Draw Mode", (self->flags >> 10) & 3));
}

// ---- NiVertexColorProperty

// vf8: the source and lighting modes
uint8_t __fastcall VertexColorIsEqual(NiFlagsProperty* self, re::Edx, NiFlagsProperty* other) {
    if (!Property_IsEqual(self, other)) {
        return 0;
    }
    uint8_t difference = uint8_t(other->flags) ^ uint8_t(self->flags);
    if ((difference & 0x30) != 0 || (difference & 8) != 0) {
        return 0;
    }
    return 1;
}

NiFlagsProperty* __fastcall VertexColorCreateClone(NiFlagsProperty* self, re::Edx, void* cloning) {
    NiFlagsProperty* clone = static_cast<NiFlagsProperty*>(NiNew(0x1C));
    if (clone != nullptr) {
        ObjectNET_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiVertexColorProperty_vtable);
        clone->flags = 8;
    } else {
        clone = nullptr;
    }
    Property_CopyMembers(self, clone, cloning);
    clone->flags = self->flags;
    return clone;
}

// vf4: before 20.1.0.2 the source and lighting modes followed the flags as values
void __fastcall VertexColorLoadBinary(NiFlagsProperty* self, re::Edx, void* stream) {
    Property_LoadBinary(self, stream);
    LoadBytes(stream, &self->flags, 2);
    if (StreamVersion(stream) < 0x14010002) {
        uint32_t value;
        LoadBytes(stream, &value, 4);
        self->flags = uint16_t((self->flags & 0xFFCF) | (value << 4));
        LoadBytes(stream, &value, 4);
        self->flags = uint16_t((value * 8) | (self->flags & 0xFFF7));
    }
}

char* __cdecl SourceModeString(const char* prefix, uint32_t mode) {
    static const char* const kNames[] = {"%s = SOURCE_IGNORE", "%s = SOURCE_EMISSIVE", "%s = SOURCE_AMB_DIFF"};
    return EnumString(prefix, 0x1A, mode < 3 ? kNames[mode] : "%s = UNKNOWN!!!");
}

char* __cdecl LightingModeString(const char* prefix, uint32_t mode) {
    static const char* const kNames[] = {"%s = LIGHTING_E", "%s = LIGHTING_E_A_D"};
    return EnumString(prefix, 0x1A, mode < 2 ? kNames[mode] : "%s = UNKNOWN!!!");
}

void __fastcall VertexColorGetViewerStrings(NiFlagsProperty* self, re::Edx, NiTArray<char*>* strings) {
    Property_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiVertexColorProperty_RTTI.name));
    AddViewerString(strings, SourceModeViewerString("Source Mode", (uint8_t(self->flags) >> 4) & 3));
    AddViewerString(strings, LightingModeViewerString("Lighting Mode", (uint8_t(self->flags) >> 3) & 1));
}

} // namespace

// RegisterStreamables of the fog, shade, stencil, vertex color, z-buffer and alpha properties: a
// lone jmp to NiObjectNET's
RE_UNHOOKED("NiProperty::RegisterStreamables_775650", 0x00775650);

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIPROPS_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIPROPS_REVERSED("NiMaterialProperty::CopyMembers", 0x007755E0, &ni::MaterialCopyMembers);
NIPROPS_REVERSED("NiMaterialProperty::NiMaterialProperty", 0x00775660, &ni::MaterialConstruct);
NIPROPS_REVERSED("NiMaterialProperty::GetRTTI", 0x007756F0, &ni::MaterialGetRTTI);
NIPROPS_REVERSED("NiMaterialProperty::CreateClone", 0x00775700, &ni::MaterialCreateClone);
NIPROPS_REVERSED("NiMaterialProperty::~NiMaterialProperty", 0x00775770, &ni::MaterialDestruct);
NIPROPS_REVERSED("NiMaterialProperty::LoadBinary", 0x007757B0, &ni::MaterialLoadBinary);
NIPROPS_REVERSED("NiMaterialProperty::SaveBinary", 0x00775840, &ni::MaterialSaveBinary);
NIPROPS_REVERSED("NiMaterialProperty::IsEqual", 0x007758D0, &ni::MaterialIsEqual);
NIPROPS_REVERSED("NiMaterialProperty::GetViewerStrings", 0x007759E0, &ni::MaterialGetViewerStrings);
NIPROPS_REVERSED("NiFogProperty::CreateClone", 0x00778190, &ni::FogCreateClone);
NIPROPS_REVERSED("NiFogProperty::LoadBinary", 0x00778280, &ni::FogLoadBinary);
NIPROPS_REVERSED("NiFogProperty::SaveBinary", 0x007782F0, &ni::FogSaveBinary);
NIPROPS_REVERSED("NiFogProperty::IsEqual", 0x00778360, &ni::FogIsEqual);
NIPROPS_REVERSED("NiFogProperty::GetFunctionViewerString", 0x00778480, &ni::FogFunctionString);
NIPROPS_REVERSED("NiFogProperty::GetViewerStrings", 0x00778500, &ni::FogGetViewerStrings);
NIPROPS_REVERSED("NiShadeProperty::CreateClone", 0x00778650, &ni::ShadeCreateClone);
NIPROPS_REVERSED("NiShadeProperty::GetViewerStrings", 0x00778770, &ni::ShadeGetViewerStrings);
NIPROPS_REVERSED("NiStencilProperty::IsEqual", 0x00778810, &ni::StencilIsEqual);
NIPROPS_REVERSED("NiStencilProperty::CreateClone", 0x00778850, &ni::StencilCreateClone);
NIPROPS_REVERSED("NiStencilProperty::LoadBinary", 0x00778900, &ni::StencilLoadBinary);
NIPROPS_REVERSED("NiStencilProperty::SaveBinary", 0x00778B30, &ni::StencilSaveBinary);
NIPROPS_REVERSED("NiStencilProperty::GetTestFunctionViewerString", 0x00778C70, &ni::TestFunctionString);
NIPROPS_REVERSED("NiStencilProperty::GetActionViewerString", 0x00778DA0, &ni::ActionString);
NIPROPS_REVERSED("NiStencilProperty::GetDrawModeViewerString", 0x00778E90, &ni::DrawModeString);
NIPROPS_REVERSED("NiStencilProperty::GetViewerStrings", 0x00778F50, &ni::StencilGetViewerStrings);
NIPROPS_REVERSED("NiVertexColorProperty::IsEqual", 0x007794A0, &ni::VertexColorIsEqual);
NIPROPS_REVERSED("NiVertexColorProperty::CreateClone", 0x007794E0, &ni::VertexColorCreateClone);
NIPROPS_REVERSED("NiVertexColorProperty::LoadBinary", 0x00779560, &ni::VertexColorLoadBinary);
NIPROPS_REVERSED("NiShadeProperty::SaveBinary", 0x00779630, &ni::FlagsSaveBinary);
NIPROPS_REVERSED("NiVertexColorProperty::GetSourceModeViewerString", 0x00779710, &ni::SourceModeString);
NIPROPS_REVERSED("NiVertexColorProperty::GetLightingModeViewerString", 0x007797B0, &ni::LightingModeString);
NIPROPS_REVERSED("NiVertexColorProperty::GetViewerStrings", 0x00779830, &ni::VertexColorGetViewerStrings);
