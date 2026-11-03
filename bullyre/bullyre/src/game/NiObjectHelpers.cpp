// 0x406460 to 0x406BB0: NiFixedString's comparisons, small NiObject helpers (extra data
// RTTI, render state cache, transforms), and the model flags read from a NIF's extra data.

#include <cmath>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_FN(char*, __cdecl, StringTable_AddString, 0x00754760, const char* s);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(int, __cdecl, stricmp_, 0x00753820, const char* a, const char* b);
GAME_FN(int, __cdecl, strnicmp_, 0x00753830, const char* a, const char* b, size_t n);
GAME_FN(uint32_t, __thiscall, FixedString_GetLength, 0x00406460, const re::Ptr32<char>* self);
GAME_FN(uint8_t, __thiscall, FixedString_Contains, 0x00406510, const re::Ptr32<char>* self, const char* s);
GAME_FN(uint8_t, __thiscall, FixedString_EqualsNoCase, 0x004064E0, const re::Ptr32<char>* self, const char* s);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);

GAME_FN(void*, __thiscall, NiObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __thiscall, NiObjectNET_AddExtraData, 0x00764600, void* object, const re::Ptr32<char>* name, void* data);
GAME_FN(void, __thiscall, NiObjectNET_Fn762080, 0x00762080, void* object, void* name, int);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __thiscall, NiColorExtraData_Construct, 0x00769260, void* self, float r, float g, float b, float a);
GAME_FN(void, __thiscall, NiColorExtraData_SetValue, 0x007691F0, void* self, float r, float g, float b, float a);
GAME_FN(float*, __thiscall, NiMatrix3_Multiply, 0x00768920, const float* self, float* out, const float* other);
GAME_FN(float*, __thiscall, NiMatrix3_MultiplyVector, 0x00768A70, const float* self, float* out, const float* v);
GAME_FN(uint8_t*, __cdecl, Fn_6C7370, 0x006C7370, void* object);

GAME_ARRAY(char*, ms_extraDataNames, 0x009D3BB4);   // pointers to NiFixedStrings (0xBCC4D4...)
GAME_ARRAY(const char*, ms_nodeNames, 0x009D3CE8);  // remove_geometry, Dummy, Biped Object, BipH, Bip0
inline re::Ptr32<char>* const ms_nameBCC500 = reinterpret_cast<re::Ptr32<char>*>(0x00BCC500);
inline re::Ptr32<char>* const ms_colourExtraName = reinterpret_cast<re::Ptr32<char>*>(0x00BCC504);
inline void* const ms_nameBCC508 = reinterpret_cast<void*>(0x00BCC508);
GAME_VAR(uint32_t, ms_NiColorExtraDataBit, 0x020E29E4);
GAME_VAR(uint32_t, ms_NiBooleanExtraDataBit, 0x020E2890);
inline void* const ms_NiBooleanExtraDataRTTI = reinterpret_cast<void*>(0x020E2888);
inline void* const ms_NiIntegerExtraDataRTTI = reinterpret_cast<void*>(0x020E2950);
inline void* const ms_NiStringExtraDataRTTI = reinterpret_cast<void*>(0x020E2984);

uint8_t* IsKindOfBit(uint8_t* object, uint32_t bit) {
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? object : nullptr;
}

// the 0x34 bytes of an NiTransform (rotation, translation, scale)
void CopyTransform(uint8_t* to, const void* from) {
    uint32_t* d = reinterpret_cast<uint32_t*>(to);
    const uint32_t* s = static_cast<const uint32_t*>(from);
    for (int i = 0; i < 13; i++) {
        d[i] = s[i];
    }
}

// an NiAVObject's world transform (+0x2C) set, its scale made positive
void SetWorldTransform(uint8_t* object, const void* transform) {
    CopyTransform(object + 0x2C, transform);
    float& scale = *reinterpret_cast<float*>(object + 0x5C);
    scale = std::fabs(scale);
}

} // namespace

namespace NiFixedStringOps {

// 0x406460 (thiscall): the length (0 for the null string)
static uint32_t __fastcall GetLength(const re::Ptr32<char>* self, re::Edx) {
    char* s = *self;
    if (s == ms_nullString) {
        return 0;
    }
    return *reinterpret_cast<uint16_t*>(s - 4);
}

// 0x406480 (thiscall): equal to a C string (strcmp)
static uint8_t __fastcall Equals(const re::Ptr32<char>* self, re::Edx, const char* other) {
    const char* s = *self;
    if (s == other) {
        return 1;
    }
    if (!other || !s) {
        return 0;
    }
    while (true) {
        if (s[0] != other[0]) {
            return 0;
        }
        if (s[0] == 0) {
            return 1;
        }
        if (s[1] != other[1]) {
            return 0;
        }
        s += 2;
        other += 2;
        if (s[-1] == 0) {
            return 1;
        }
    }
}

// 0x4064E0 (thiscall): equal ignoring case
static uint8_t __fastcall EqualsNoCase(const re::Ptr32<char>* self, re::Edx, const char* other) {
    const char* s = *self;
    if (s == other) {
        return 1;
    }
    if (!other || !s) {
        return 0;
    }
    return stricmp_(s, other) == 0;
}

// 0x406510 (thiscall): contains `other` ignoring case (an empty one isn't contained)
static uint8_t __fastcall Contains(const re::Ptr32<char>* self, re::Edx, const char* other) {
    const char* s = *self;
    if (s == other) {
        return other != nullptr;
    }
    if (!other || !s || !*other) {
        return 0;
    }
    size_t length = 0;
    while (other[length]) {
        length++;
    }
    for (uint32_t i = 0; i < FixedString_GetLength(self); i++) {
        if (strnicmp_(s + i, other, length) == 0) {
            return 1;
        }
    }
    return 0;
}

// 0x406730 (thiscall): assigned from a C string
static re::Ptr32<char>* __fastcall AssignCString(re::Ptr32<char>* self, re::Edx, const char* s) {
    re::Ptr32<char> old = *self;
    if (old == s) {
        return self;
    }
    *self = StringTable_AddString(s);
    if (old != ms_nullString) {
        uint16_t length = *reinterpret_cast<uint16_t*>(old - 2);
        if (vc::Decrement(old - 8) == 1) {
            StringTable_Release(ms_pStringTable, &old, length);
        }
    }
    return self;
}

} // namespace NiFixedStringOps

namespace NiHelpers {

// 0x4065A0 (thiscall): out = other + this (3 floats), out returned
static float* __fastcall AddVectors(const float* self, re::Edx, float* out, const float* other) {
    out[0] = other[0] + self[0];
    out[1] = other[1] + self[1];
    out[2] = other[2] + self[2];
    return out;
}

// 0x4065D0 (thiscall; a virtual): flag 0x800 of +0xA8
static void __fastcall SetFlag800(uint8_t* self, re::Edx, int on) {
    uint16_t& flags = *reinterpret_cast<uint16_t*>(self + 0xA8);
    if (re::Flag(on)) {
        flags |= 0x800;
    } else {
        flags &= 0xF7FF;
    }
}

// 0x4065F0, 0x406610, 0x406620: the extra data classes' RTTI; 0x406600: a ShaderMap's type
static void* __cdecl BooleanExtraData_GetRTTI() {
    return reinterpret_cast<void*>(0x020E2888);
}
static uint32_t __cdecl ShaderMap_GetType() {
    return 3;
}
static void* __cdecl FloatExtraData_GetRTTI() {
    return reinterpret_cast<void*>(0x020E291C);
}
static void* __cdecl IntegerExtraData_GetRTTI() {
    return reinterpret_cast<void*>(0x020E2950);
}

// 0x406630 (thiscall): a render state set through the cache (+0x120, 8 bytes a state)
static void __fastcall SetRenderState(uint8_t* self, re::Edx, uint32_t state, uint32_t value) {
    uint32_t& cached = *reinterpret_cast<uint32_t*>(self + 0x120 + state * 8);
    if (cached == value) {
        return;
    }
    void* device = *reinterpret_cast<re::Ptr32<void>*>(self + 0x2238);
    // IDirect3DDevice9::SetRenderState
    reinterpret_cast<long(__stdcall*)(void*, uint32_t, uint32_t)>(static_cast<void*>(re::ComVtable(device)[0xE4 / 4]))(device, state,
                                                                                                                  value);
    cached = value;
}

// 0x406670: a D3D matrix from a rotation (transposed, scaled) and a translation
static void __cdecl MakeD3DMatrix(float* out, const float* rotation, const float* translation, float scale) {
    out[0] = rotation[0] * scale;
    out[1] = rotation[3] * scale;
    out[2] = rotation[6] * scale;
    out[3] = 0.0f;
    out[4] = rotation[1] * scale;
    out[5] = rotation[4] * scale;
    out[6] = rotation[7] * scale;
    out[7] = 0.0f;
    out[8] = rotation[2] * scale;
    out[9] = rotation[5] * scale;
    out[10] = rotation[8] * scale;
    out[11] = 0.0f;
    out[12] = translation[0];
    out[13] = translation[1];
    out[14] = translation[2];
    out[15] = 1.0f;
}

// 0x4066F0: IsKindOf as a bool
static uint8_t __cdecl IsKindOfBool(uint8_t* rttiInfo, uint8_t* object) {
    if (!object) {
        return 0;
    }
    return IsKindOfBit(object, *reinterpret_cast<uint32_t*>(rttiInfo + 8)) != nullptr;
}

// 0x406790: a node to keep whole: Bip0, BipH, Biped Object or Dummy in its name, or
// "remove_geometry" in its string extra data (none: kept)
static uint8_t __cdecl IsSkeletonNode(uint8_t* object) {
    if (!object) {
        return 1;
    }
    const re::Ptr32<char>* name = reinterpret_cast<const re::Ptr32<char>*>(object + 8);
    if (FixedString_Contains(name, ms_nodeNames[4]) || FixedString_Contains(name, ms_nodeNames[3]) ||
        FixedString_Contains(name, ms_nodeNames[2]) || FixedString_EqualsNoCase(name, ms_nodeNames[1])) {
        return 1;
    }
    uint8_t* extra = IsKindOf(ms_NiStringExtraDataRTTI, NiObjectNET_GetExtraData(object, ms_nameBCC500));
    if (extra && FixedString_Contains(re::PtrCast<const re::Ptr32<char>*>(extra + 0xC), ms_nodeNames[0])) {
        return 1;
    }
    return 0;
}

// 0x406830 (thiscall): NiTransform's product: out = this * other
static float* __fastcall MultiplyTransforms(const float* self, re::Edx, float* out, const float* other) {
    out[12] = other[12] * self[12];
    float rotation[9];
    const float* product = NiMatrix3_Multiply(self, rotation, other);
    for (int i = 0; i < 9; i++) {
        out[i] = product[i];
    }
    float v[3];
    const float* rotated = NiMatrix3_MultiplyVector(self, v, other + 9);
    float scale = self[12];
    float x = rotated[0] * scale;
    float y = rotated[1] * scale;
    float z = scale * rotated[2];
    out[9] = x + self[9];
    out[10] = self[10] + y;
    out[11] = self[11] + z;
    return out;
}

// 0x4068E0: an object's (+0x14's) world transform set
static void __cdecl SetObjectWorldTransform(uint8_t* holder, const void* transform) {
    SetWorldTransform(*reinterpret_cast<re::Ptr32<uint8_t>*>(holder + 0x14), transform);
}

// 0x406910: the world transforms of a list of objects under a parent (+4's +0x14): the
// first given the parent's, the others the parent's combined with their own (0x6C7370's
// +0x14 rotation and +0x38 translation; the parent's scale). Objects flagged +0xA8 & 1
// are skipped. (List nodes sit at +0x4C of their objects; +8 is the list's head)
static void __cdecl UpdateAttachedTransforms(uint8_t* list) {
    uint8_t* head = list + 8;
    uint8_t* node = *reinterpret_cast<re::Ptr32<uint8_t>*>(head);
    uint8_t* object = *reinterpret_cast<re::Ptr32<uint8_t>*>(node - 0x38);
    if (!(object[0xA8] & 1)) {
        SetWorldTransform(object, *reinterpret_cast<re::Ptr32<uint8_t>*>(list + 4) + 0x14);
    }
    for (node = *reinterpret_cast<re::Ptr32<uint8_t>*>(node); node != head; node = *reinterpret_cast<re::Ptr32<uint8_t>*>(node)) {
        object = *reinterpret_cast<re::Ptr32<uint8_t>*>(node - 0x38);
        if (object[0xA8] & 1) {
            continue;
        }
        uint8_t* owner = node - 0x4C;
        float world[13];
        CopyTransform(reinterpret_cast<uint8_t*>(world), *reinterpret_cast<re::Ptr32<uint8_t>*>(list + 4) + 0x14);
        float v[3];
        NiMatrix3_MultiplyVector(world, v, reinterpret_cast<float*>(Fn_6C7370(owner) + 0x38));
        world[9] = v[0] + world[9];
        world[10] = world[10] + v[1];
        world[11] = world[11] + v[2];
        float rotation[9];
        const float* product = NiMatrix3_Multiply(world, rotation, reinterpret_cast<float*>(Fn_6C7370(owner) + 0x14));
        for (int i = 0; i < 9; i++) {
            world[i] = product[i];
        }
        SetWorldTransform(*reinterpret_cast<re::Ptr32<uint8_t>*>(owner + 0x14), world);
    }
}

// 0x406A30: a model's flags (+0x18 bits, +0x1A) from its NIF's extra data, when its
// first boolean is set: 1 (bool), 0x1E (int << 1), 0x1E0 (int << 5), 0x200 (bool),
// 0x1C00 (int << 10), +0x1A (int's low byte). (The others aren't checked for null)
static void __cdecl ReadModelFlags(uint8_t* model, void* object) {
    uint8_t* enabled = static_cast<uint8_t*>(NiObjectNET_GetExtraData(object, re::PtrCast<const re::Ptr32<char>*>(ms_extraDataNames[0])));
    if (!enabled || !IsKindOfBit(enabled, ms_NiBooleanExtraDataBit) || !enabled[0xC]) {
        return;
    }
    auto get = [&](int index, void* rtti) {
        return IsKindOf(rtti, NiObjectNET_GetExtraData(object, re::PtrCast<const re::Ptr32<char>*>(ms_extraDataNames[index])));
    };
    uint16_t& flags = *reinterpret_cast<uint16_t*>(model + 0x18);
    if (get(1, ms_NiBooleanExtraDataRTTI)[0xC]) {
        flags |= 1;
    } else {
        flags &= 0xFFFE;
    }
    flags = uint16_t((*reinterpret_cast<uint16_t*>(get(2, ms_NiIntegerExtraDataRTTI) + 0xC) * 2) | (flags & 0xFFE1));
    flags = uint16_t((*reinterpret_cast<uint16_t*>(get(3, ms_NiIntegerExtraDataRTTI) + 0xC) << 5) | (flags & 0xFE1F));
    if (get(4, ms_NiBooleanExtraDataRTTI)[0xC]) {
        flags |= 0x200;
    } else {
        flags &= 0xFDFF;
    }
    flags = uint16_t((*reinterpret_cast<uint16_t*>(get(5, ms_NiIntegerExtraDataRTTI) + 0xC) << 10) | (flags & 0xE3FF));
    model[0x1A] = get(6, ms_NiIntegerExtraDataRTTI)[0xC];
}

// 0x406BB0: an object's colour extra data (0xBCC504) set from an RGBA byte colour (made
// when missing), then 0x762080(0xBCC508)
static void __cdecl SetColourExtraData(void* object, uint32_t rgba) {
    float r = float(double(rgba & 0xFF) / 255.0);
    float g = float(double((rgba >> 8) & 0xFF) / 255.0);
    float b = float(double((rgba >> 16) & 0xFF) / 255.0);
    float a = float(double(rgba >> 24) / 255.0);
    uint8_t* extra = static_cast<uint8_t*>(NiObjectNET_GetExtraData(object, ms_colourExtraName));
    if (extra && IsKindOfBit(extra, ms_NiColorExtraDataBit)) {
        NiColorExtraData_SetValue(extra, r, g, b, a);
    } else {
        uint8_t* data = static_cast<uint8_t*>(NiNew(0x1C));
        if (data) {
            NiColorExtraData_Construct(data, r, g, b, a);
        }
        // (a failed allocation writes the name to address 8)
        FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(data + 8), ms_colourExtraName);
        NiObjectNET_AddExtraData(object, ms_colourExtraName, data);
    }
    NiObjectNET_Fn762080(object, ms_nameBCC508, -1);
}

} // namespace NiHelpers

// ---- registration ----------------------------------------------------------------------

#define C00_NIH_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace NiFixedStringOps {
C00_NIH_REVERSED("NiFixedString::GetLength", GetLength, 0x00406460);
C00_NIH_REVERSED("NiFixedString::Equals", Equals, 0x00406480);
C00_NIH_REVERSED("NiFixedString::EqualsNoCase", EqualsNoCase, 0x004064E0);
C00_NIH_REVERSED("NiFixedString::Contains", Contains, 0x00406510);
C00_NIH_REVERSED("NiFixedString::AssignCString", AssignCString, 0x00406730);
} // namespace NiFixedStringOps
namespace NiHelpers {
C00_NIH_REVERSED("NiPoint3::Add", AddVectors, 0x004065A0);
C00_NIH_REVERSED("NiHelpers::SetFlag800", SetFlag800, 0x004065D0);
C00_NIH_REVERSED("NiBooleanExtraData::GetRTTI", BooleanExtraData_GetRTTI, 0x004065F0);
C00_NIH_REVERSED("NiTexturingProperty::ShaderMap::GetType", ShaderMap_GetType, 0x00406600);
C00_NIH_REVERSED("NiFloatExtraData::GetRTTI", FloatExtraData_GetRTTI, 0x00406610);
C00_NIH_REVERSED("NiIntegerExtraData::GetRTTI", IntegerExtraData_GetRTTI, 0x00406620);
C00_NIH_REVERSED("NiDX9RenderState::SetRenderState", SetRenderState, 0x00406630);
C00_NIH_REVERSED("NiHelpers::MakeD3DMatrix", MakeD3DMatrix, 0x00406670);
C00_NIH_REVERSED("NiHelpers::IsKindOfBool", IsKindOfBool, 0x004066F0);
C00_NIH_REVERSED("NiHelpers::IsSkeletonNode", IsSkeletonNode, 0x00406790);
C00_NIH_REVERSED("NiTransform::Multiply", MultiplyTransforms, 0x00406830);
C00_NIH_REVERSED("NiHelpers::SetObjectWorldTransform", SetObjectWorldTransform, 0x004068E0);
C00_NIH_REVERSED("NiHelpers::UpdateAttachedTransforms", UpdateAttachedTransforms, 0x00406910);
C00_NIH_REVERSED("NiHelpers::ReadModelFlags", ReadModelFlags, 0x00406A30);
C00_NIH_REVERSED("NiHelpers::SetColourExtraData", SetColourExtraData, 0x00406BB0);
} // namespace NiHelpers
