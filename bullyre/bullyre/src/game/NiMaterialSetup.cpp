// 0x409A40 and 0x409B80: the z-buffer and alpha properties the game sets up on a loaded
// NIF object from the renderer's states and the object's extra data (fade, flags).

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

GAME_FN(void*, __thiscall, NiAVObject_GetProperty, 0x0075DC10, void* object, int type);
GAME_FN(void, __thiscall, NiAVObject_AttachProperty, 0x004099F0, void* object, void* property);
GAME_FN(void, __thiscall, NiAVObject_Fn75E040, 0x0075E040, void* object);
GAME_FN(void, __thiscall, NiProperty_Construct, 0x00764550, void* self);
GAME_FN(void*, __thiscall, ZBufferProperty_Construct, 0x00407B40, void* self);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, GetRenderState_519290, 0x00519290, int state, int32_t* value);
GAME_FN(void*, __thiscall, NiObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, NiFloatsExtraData_SetValue, 0x0076C9B0, void* self, uint32_t index, float value);
GAME_FN(void, __cdecl, ReadModelFlags, 0x00406A30, void* model, void* object);
GAME_FN(uint8_t*, __cdecl, GetDisplaySettings, 0x00405BA0, void* out);   // (returns `out`)
GAME_VAR(uint32_t, ms_vendor, 0x00BCBC20);   // 2: ATI
GAME_ARRAY(char*, ms_extraDataNames, 0x009D3BB4);   // pointers to NiFixedStrings
GAME_VAR(re::Ptr32<char>*, ms_pFadeName, 0x009D3BB0);
inline re::Ptr32<char>* const ms_floatsName_BCC518 = reinterpret_cast<re::Ptr32<char>*>(0x00BCC518);
inline re::Ptr32<char>* const ms_boolName_BCC52C = reinterpret_cast<re::Ptr32<char>*>(0x00BCC52C);
inline re::Ptr32<char>* const ms_intName_BCC530 = reinterpret_cast<re::Ptr32<char>*>(0x00BCC530);
GAME_VAR(uint32_t, ms_NiZBufferPropertyBit, 0x020E2B30);
GAME_VAR(uint32_t, ms_NiAlphaPropertyBit, 0x020E2AF8);
GAME_VAR(uint32_t, ms_NiBooleanExtraDataBit, 0x020E2890);
GAME_VAR(uint32_t, ms_NiFloatExtraDataBit, 0x020E2924);
GAME_VAR(uint32_t, ms_NiIntegerExtraDataBit, 0x020E2958);
GAME_VAR(uint32_t, ms_NiFloatsExtraDataBit, 0x020E2AC4);
GAME_VAR(uint32_t, ms_NiTexturingPropertyBit, 0x020E28C4);
GAME_FN(void*, __thiscall, NiTexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void, __thiscall, TexturingProperty_SetBaseTexture, 0x00407A60, void* self, void* texture);
GAME_FN(void*, __cdecl, CreateTestTexture, 0x00406E80);   // (returns the texture in eax)
GAME_FN(void, __thiscall, NiExtraData_Construct, 0x00765640, void* self);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(uint8_t, __thiscall, NiObjectNET_AddExtraData, 0x00764600, void* object, const re::Ptr32<char>* name, void* data);
GAME_FN(void, __cdecl, SetupAlpha_, 0x00409B80, void* object, float fade);
GAME_FN(void, __cdecl, SetupZBuffer_, 0x00409A40, void* object);
GAME_FN(void, __cdecl, SetupTexturing_, 0x0040A100, void* object);
template <typename... A>
inline int __cdecl sprintf_(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
inline void* const ms_NiGeometryRTTI = reinterpret_cast<void*>(0x020E25B4);
inline void* const ms_NiNodeRTTI = reinterpret_cast<void*>(0x020E2548);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);

uint8_t* IsKindOfBit(void* object, uint32_t bit) {
    if (!object) {
        return nullptr;
    }
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? static_cast<uint8_t*>(object) : nullptr;
}

uint16_t& W(void* p, int offset) {
    return *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + offset);
}
uint32_t& U(void* p, int offset) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + offset);
}

// the object's z-buffer property (NiProperty type 0xB), made and attached when missing
uint8_t* GetZBufferProperty(void* object, bool inlineConstructor) {
    uint8_t* z = IsKindOfBit(NiAVObject_GetProperty(object, 0xB), ms_NiZBufferPropertyBit);
    if (z) {
        return z;
    }
    uint8_t* p = static_cast<uint8_t*>(NiNew(0x30));
    if (p) {
        if (inlineConstructor) {
            NiProperty_Construct(p);
            *reinterpret_cast<float*>(p + 0x1C) = 0.0f;
            U(p, 0) = 0x008FFCA4;
            *reinterpret_cast<float*>(p + 0x20) = 0.0f;
            U(p, 0x24) = 0;
            W(p, 0x18) = 0xF;
            *reinterpret_cast<float*>(p + 0x28) = 1.0f;
            *reinterpret_cast<float*>(p + 0x2C) = 1.0f;
        } else {
            p = static_cast<uint8_t*>(ZBufferProperty_Construct(p));
        }
    }
    NiAVObject_AttachProperty(object, p);
    NiAVObject_Fn75E040(object);
    return p;
}

// the walk over a tree's geometry (its cached list at +0xB8, itself, or its children)
template <typename Geometry, typename Recurse>
void Walk(uint8_t* object, Geometry geometry, Recurse recurse) {
    if (!object || (object[0xA8] & 1)) {
        return;
    }
    if (W(object, 0xBE) != 0) {
        for (uint32_t i = 0; i < W(object, 0xBE); i++) {
            geometry((*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(object + 0xB8))[i]);
        }
        return;
    }
    uint8_t* g = IsKindOf(ms_NiGeometryRTTI, object);
    if (g) {
        geometry(g);
        return;
    }
    uint8_t* node = IsKindOf(ms_NiNodeRTTI, object);
    if (!node) {
        return;
    }
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        recurse((*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(node + 0xC8))[i]);
    }
}

// the object's texturing property (type 8), made and attached when missing
uint8_t* GetTexturingProperty(void* object) {
    uint8_t* t = IsKindOfBit(NiAVObject_GetProperty(object, 8), ms_NiTexturingPropertyBit);
    if (t) {
        return t;
    }
    void* p = NiNew(0x34);
    t = static_cast<uint8_t*>(p ? NiTexturingProperty_Construct(p) : nullptr);
    NiAVObject_AttachProperty(object, t);
    return t;
}

// a boolean / integer extra data added under a name
void AddBool(void* object, const re::Ptr32<char>* name, uint8_t value) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x10));
    if (data) {
        NiExtraData_Construct(data);
        U(data, 0) = 0x008FFA24;
        data[0xC] = value;
    }
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(data) + 8), name);
    NiObjectNET_AddExtraData(object, name, data);
}
void AddInt(void* object, const re::Ptr32<char>* name, uint32_t value) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x10));
    if (data) {
        NiExtraData_Construct(data);
        U(data, 0) = 0x008FFAE4;
        U(data, 0xC) = value;
    }
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(reinterpret_cast<uintptr_t>(data) + 8), name);
    NiObjectNET_AddExtraData(object, name, data);
}

} // namespace

namespace NiMaterialSetup {

// 0x409A40: the z-buffer property's test and write flags from render states 6 and 8,
// and the floats extra data (0xBCC518) set to 0.5, 100
static void __cdecl SetupZBuffer(void* object) {
    uint8_t* z = GetZBufferProperty(object, true);
    int32_t value;
    GetRenderState_519290(6, &value);
    if (value) {
        W(z, 0x18) |= 1;
    } else {
        W(z, 0x18) &= 0xFFFE;
    }
    GetRenderState_519290(8, &value);
    if (value) {
        W(z, 0x18) |= 2;
    } else {
        W(z, 0x18) &= 0xFFFD;
    }
    uint8_t* floats = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_floatsName_BCC518), ms_NiFloatsExtraDataBit);
    if (floats) {
        NiFloatsExtraData_SetValue(floats, 0, 0.5f);
        NiFloatsExtraData_SetValue(floats, 1, 100.0f);
    }
}

// 0x409B80: the alpha property (made when missing; type 0) set up for a fade: opaque
// (fade >= 0.99, or a blend that isn't additive-like) reads the model flags and, when
// blending, turns z writes and alpha testing off; otherwise alpha blended (src alpha,
// inverse src alpha) with object flags 0x8000 when the extra data asks. Then the
// sorting flags, alpha to coverage (multisampling; ATI also alpha blends), and two
// extra data values that record the result
static void __cdecl SetupAlpha(uint8_t* object, float fade) {
    uint8_t* alpha = IsKindOfBit(NiAVObject_GetProperty(object, 0), ms_NiAlphaPropertyBit);
    if (!alpha) {
        alpha = static_cast<uint8_t*>(NiNew(0x1C));
        if (alpha) {
            NiProperty_Construct(alpha);
            U(alpha, 0) = 0x008FFB9C;
            alpha[0x1A] = 0;
            W(alpha, 0x18) = 0xEC;
        }
        NiAVObject_AttachProperty(object, alpha);
        NiAVObject_Fn75E040(object);
    }
    void* first = NiObjectNET_GetExtraData(object, re::PtrCast<const re::Ptr32<char>*>(ms_extraDataNames[0]));
    if (first) {
        vc::Call<void*>(first, 0x08);   // (its RTTI, unused)
    }
    uint8_t flagA = 0;
    uint8_t* b = IsKindOfBit(NiObjectNET_GetExtraData(object, re::PtrCast<const re::Ptr32<char>*>(ms_extraDataNames[1])),
                             ms_NiBooleanExtraDataBit);
    if (b) {
        flagA = b[0xC];
    }
    uint8_t flagB = 0;
    b = IsKindOfBit(NiObjectNET_GetExtraData(object, re::PtrCast<const re::Ptr32<char>*>(ms_extraDataNames[4])),
                    ms_NiBooleanExtraDataBit);
    if (b) {
        flagB = b[0xC];
    }
    uint8_t* fadeData = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_pFadeName), ms_NiFloatExtraDataBit);
    if (fadeData) {
        *reinterpret_cast<float*>(fadeData + 0xC) = fade;
    }
    uint16_t flags = W(alpha, 0x18);
    bool blended = (flags & 1) && ((flags & 0x1E) < 0xC || (flags & 0x1E0) < 0xC0);
    W(object, 0xA8) &= 0x7FFF;
    if (blended || !(fade < double(0.99f))) {
        ReadModelFlags(alpha, object);
        if (blended) {
            U(object, 0xB0) |= 1;
            uint8_t* z = GetZBufferProperty(object, false);
            W(z, 0x18) &= 0xFFFD;
            W(alpha, 0x18) &= 0xFDFF;
        }
    } else {
        if (flagA || flagB) {
            W(object, 0xA8) |= 0x8000;
        }
        W(alpha, 0x18) = uint16_t((W(alpha, 0x18) & 0xF2ED) | 0x12ED);
    }
    if (W(alpha, 0x18) & 1) {
        W(object, 0xA8) |= 0x1000;
    } else {
        W(object, 0xA8) &= 0xEFFF;
    }
    if (flagB) {
        U(object, 0xB0) |= 2;
    }
    uint16_t a = W(alpha, 0x18);
    if (((a >> 9) & 1) && !(W(alpha, 0x18) & 1)) {
        uint8_t mode[0x18];
        if (*reinterpret_cast<int32_t*>(GetDisplaySettings(mode) + 8) > 1) {
            a = uint16_t((W(alpha, 0x18) & 0x3FFF) | 0x4000);
            W(alpha, 0x18) = a;
            if (ms_vendor == 2) {
                W(alpha, 0x18) = uint16_t((a & 0xFDFF) | 1);
            }
        }
    } else {
        W(alpha, 0x18) = uint16_t(a & 0xBFFF);
    }
    if (((W(alpha, 0x18) >> 9) & 1) && !alpha[0x1A]) {
        alpha[0x1A] = 1;
    }
    bool transparent = (W(alpha, 0x18) & 1) || ((W(alpha, 0x18) >> 9) & 1);
    uint8_t* flag = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_boolName_BCC52C), ms_NiBooleanExtraDataBit);
    if (flag) {
        flag[0xC] = transparent ? 1 : 0;
    }
    uint8_t* mask = IsKindOfBit(NiObjectNET_GetExtraData(object, ms_intName_BCC530), ms_NiIntegerExtraDataBit);
    if (mask) {
        U(mask, 0xC) = transparent ? 7 : 0xF;
    }
}

// 0x409F60: SetupAlpha over a tree
static void __cdecl SetupTreeAlpha(uint8_t* object, float fade) {
    Walk(
        object, [&](uint8_t* g) { SetupAlpha_(g, fade); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*, float)>(0x00409F60)(child, fade); });
}

// 0x40A040: SetupZBuffer over a tree
static void __cdecl SetupTreeZBuffer(uint8_t* object) {
    Walk(
        object, [&](uint8_t* g) { SetupZBuffer_(g); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*)>(0x0040A040)(child); });
}

// 0x40A100: the texturing property (made when missing) given render state 1's texture
// (its +0x34) as the base map
static void __cdecl SetupTexturing(void* object) {
    uint8_t* texturing = GetTexturingProperty(object);
    int32_t value = 0;
    GetRenderState_519290(1, &value);
    uint8_t* state = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(value));
    if (state && U(state, 0x34) != 0) {
        TexturingProperty_SetBaseTexture(texturing, reinterpret_cast<void*>(static_cast<uintptr_t>(U(state, 0x34))));
    }
}

// 0x40A1A0: SetupTexturing over a tree
static void __cdecl SetupTreeTexturing(uint8_t* object) {
    Walk(
        object, [&](uint8_t* g) { SetupTexturing_(g); },
        [&](uint8_t* child) { reinterpret_cast<void(__cdecl*)(uint8_t*)>(0x0040A1A0)(child); });
}

// 0x40A260: the alpha property's state recorded as extra data (enabled; blend on, source
// and destination factors; test on, function, threshold; transparent; the colour mask),
// the property made (blending and testing off) when missing
static void __cdecl RecordAlphaState(void* object) {
    bool transparent = false;
    const re::Ptr32<char>* names[7];
    for (int i = 0; i < 7; i++) {
        names[i] = re::PtrCast<const re::Ptr32<char>*>(ms_extraDataNames[i]);
    }
    uint8_t* alpha = IsKindOfBit(NiAVObject_GetProperty(object, 0), ms_NiAlphaPropertyBit);
    if (alpha) {
        AddBool(object, names[0], 1);
        AddBool(object, names[1], alpha[0x18] & 1);
        if (alpha[0x18] & 1) {
            transparent = true;
        }
        AddInt(object, names[2], (alpha[0x18] >> 1) & 0xF);
        AddInt(object, names[3], (W(alpha, 0x18) >> 5) & 0xF);
        AddBool(object, names[4], (W(alpha, 0x18) >> 9) & 1);
    } else {
        AddBool(object, names[0], 1);
        alpha = static_cast<uint8_t*>(NiNew(0x1C));
        if (alpha) {
            NiProperty_Construct(alpha);
            U(alpha, 0) = 0x008FFB9C;
            alpha[0x1A] = 0;
            W(alpha, 0x18) = 0xEC;
        }
        NiAVObject_AttachProperty(object, alpha);
        W(alpha, 0x18) &= 0xFFFE;
        AddBool(object, names[1], alpha[0x18] & 1);
        AddInt(object, names[2], (alpha[0x18] >> 1) & 0xF);
        AddInt(object, names[3], (W(alpha, 0x18) >> 5) & 0xF);
        W(alpha, 0x18) &= 0xFDFF;
        AddBool(object, names[4], (W(alpha, 0x18) >> 9) & 1);
    }
    AddInt(object, names[5], (W(alpha, 0x18) >> 10) & 7);
    AddInt(object, names[6], alpha[0x1A]);
    AddBool(object, ms_boolName_BCC52C, transparent ? 1 : 0);
    AddInt(object, ms_intName_BCC530, transparent ? 7 : 0xF);
}

// 0x40A740: a geometry without a base map given the 2x2 test texture (the warning is
// formatted and not shown)
static void __cdecl EnsureBaseMap(uint8_t* geometry) {
    char message[256];
    uint8_t* texturing = GetTexturingProperty(geometry);
    uint8_t* map = **reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(texturing + 0x20);
    if (!map || U(map, 8) == 0) {
        sprintf_(message, "WARNING: No base map on geometry %s\n", *reinterpret_cast<re::Ptr32<const char>*>(geometry + 8));
        TexturingProperty_SetBaseTexture(texturing, CreateTestTexture());
    }
}

} // namespace NiMaterialSetup

// ---- registration ----------------------------------------------------------------------

#define C00_MAT_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace NiMaterialSetup {
C00_MAT_REVERSED("NiMaterialSetup::SetupZBuffer", SetupZBuffer, 0x00409A40);
C00_MAT_REVERSED("NiMaterialSetup::SetupAlpha", SetupAlpha, 0x00409B80);
C00_MAT_REVERSED("NiMaterialSetup::SetupTreeAlpha", SetupTreeAlpha, 0x00409F60);
C00_MAT_REVERSED("NiMaterialSetup::SetupTreeZBuffer", SetupTreeZBuffer, 0x0040A040);
C00_MAT_REVERSED("NiMaterialSetup::SetupTexturing", SetupTexturing, 0x0040A100);
C00_MAT_REVERSED("NiMaterialSetup::SetupTreeTexturing", SetupTreeTexturing, 0x0040A1A0);
C00_MAT_REVERSED("NiMaterialSetup::RecordAlphaState", RecordAlphaState, 0x0040A260);
C00_MAT_REVERSED("NiMaterialSetup::EnsureBaseMap", EnsureBaseMap, 0x0040A740);
} // namespace NiMaterialSetup
