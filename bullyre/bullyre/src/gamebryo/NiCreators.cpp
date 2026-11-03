// The classes' static CreateObject functions, the loaders NiMainSDM::Init registers with
// NiStream by class name: NiNew of the class's size, then its constructor (some inline the
// default constructor's stores instead of calling it: those are written out as the game has
// them, the base's constructor called, then the class's own fields and vtable). Null when NiNew
// fails.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

using re::At;

GAME_VAR(const char*, NullFixedString, 0x00951028);
GAME_ARRAY(const uint32_t, ColorA_Default, 0x00BC75D4);    // NiColorA's default (0, 0, 0, 1)
GAME_ARRAY(const uint32_t, Color_FogDefault, 0x020E25E8);  // the fog colour's default (NiColor)

// the bases' constructors
GAME_FN(void*, __thiscall, RefObject_Construct, 0x00755260, void* self);   // (NiObject's)
GAME_FN(void*, __thiscall, ObjectNET_Construct, 0x00764550, void* self);
GAME_FN(void*, __thiscall, ExtraData_Construct, 0x00765640, void* self);
GAME_FN(void*, __thiscall, Light_Construct, 0x00773900, void* self);
GAME_FN(void*, __thiscall, Geometry_Construct, 0x00761CA0, void* self);
GAME_FN(void*, __thiscall, TriBasedGeom_Construct, 0x007793A0, void* self);
GAME_FN(void*, __thiscall, GeometryData_ConstructDefault, 0x0077A5A0, void* self);
GAME_FN(void*, __thiscall, TriBasedGeomData_Construct, 0x007706F0, void* self);
GAME_FN(void*, __thiscall, TriShapeData_Construct, 0x0077BA40, void* self);
GAME_FN(void*, __thiscall, PixelFormat_Construct, 0x0076B280, void* self);
// the classes' own
GAME_FN(void*, __thiscall, Node_Construct, 0x00760780, void* self, uint32_t childCount);
GAME_FN(void*, __thiscall, Camera_Construct, 0x00763C20, void* self);
GAME_FN(void*, __thiscall, TexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void*, __thiscall, SourceTexture_Construct, 0x007697A0, void* self);
GAME_FN(void*, __thiscall, TextureEffect_Construct, 0x0076BA80, void* self);
GAME_FN(void*, __thiscall, ShadowGenerator_ConstructDefault, 0x00773070, void* self);
GAME_FN(void*, __thiscall, SpotLight_Construct, 0x00774D60, void* self);
GAME_FN(void*, __thiscall, MaterialProperty_Construct, 0x00775660, void* self);

constexpr uintptr_t NiExtraData_vtable = 0x00952010;
constexpr uintptr_t NiBooleanExtraData_vtable = 0x008FFA24;
constexpr uintptr_t NiFloatExtraData_vtable = 0x008FFA8C;
constexpr uintptr_t NiIntegerExtraData_vtable = 0x008FFAE4;
constexpr uintptr_t NiStringExtraData_vtable = 0x00952450;
constexpr uintptr_t NiColorExtraData_vtable = 0x009524F0;
constexpr uintptr_t NiFloatsExtraData_vtable = 0x00952860;
constexpr uintptr_t NiIntegersExtraData_vtable = 0x00952B88;
constexpr uintptr_t NiBinaryExtraData_vtable = 0x009179A4;
constexpr uintptr_t NiPixelData_vtable = 0x009525D0;
constexpr uintptr_t NiAlphaProperty_vtable = 0x008FFB9C;
constexpr uintptr_t NiZBufferProperty_vtable = 0x008FFCA4;
constexpr uintptr_t NiFogProperty_vtable = 0x009179FC;
constexpr uintptr_t NiShadeProperty_vtable = 0x00917A54;
constexpr uintptr_t NiStencilProperty_vtable = 0x00917AAC;
constexpr uintptr_t NiVertexColorProperty_vtable = 0x0092D0D4;
constexpr uintptr_t NiSkinPartition_vtable = 0x00952C00;
constexpr uintptr_t NiSkinInstance_vtable = 0x00953200;
constexpr uintptr_t NiPointLight_vtable = 0x00952FF0;
constexpr uintptr_t NiAmbientLight_vtable = 0x00954C68;
constexpr uintptr_t NiLines_vtable = 0x009538D0;
constexpr uintptr_t NiTriShape_vtable = 0x00953B20;
constexpr uintptr_t NiLinesData_vtable = 0x00953970;
constexpr uintptr_t NiTriShapeData_vtable = 0x00953BC8;
constexpr uintptr_t NiScreenElementsData_vtable = 0x00955260;

inline void SetVtable(void* object, uintptr_t vtable) {
    *static_cast<re::Ptr32<void>*>(object) = reinterpret_cast<void*>(vtable);
}

// an extra data kind of 0x10 bytes with a 32-bit value
inline void* NewExtraData16(uintptr_t vtable) {
    void* object = NiNew(0x10);
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    SetVtable(object, vtable);
    At<uint32_t>(object, 0xC) = 0;
    return object;
}

// an extra data kind with an array: the pointer, then the size
inline void* NewArrayExtraData(uintptr_t vtable) {
    void* object = NiNew(0x14);
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    SetVtable(object, vtable);
    At<void*>(object, 0x10) = nullptr;
    At<uint32_t>(object, 0xC) = 0;
    return object;
}

// a property of 0x1C bytes: NiObjectNET, then its flags
inline void* NewFlagsProperty(uintptr_t vtable, uint16_t flags) {
    void* object = NiNew(0x1C);
    if (object == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(object);
    SetVtable(object, vtable);
    At<uint16_t>(object, 0x18) = flags;
    return object;
}

} // namespace

// 0x760A60: NiNode
void* __cdecl NiNode_CreateObject() {
    void* object = NiNew(0xF4);
    return object != nullptr ? Node_Construct(object, 0) : nullptr;
}

// 0x763D50: NiCamera
void* __cdecl NiCamera_CreateObject() {
    void* object = NiNew(0x13C);
    return object != nullptr ? Camera_Construct(object) : nullptr;
}

// 0x7656F0: NiExtraData (the base: NiObject's constructor, the name the null handle)
void* __cdecl NiExtraData_CreateObject() {
    void* object = NiNew(0xC);
    if (object == nullptr) {
        return nullptr;
    }
    RefObject_Construct(object);
    SetVtable(object, NiExtraData_vtable);
    At<const char*>(object, 8) = NullFixedString;
    return object;
}

// 0x765860: NiBooleanExtraData (false)
void* __cdecl NiBooleanExtraData_CreateObject() {
    void* object = NiNew(0x10);
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    SetVtable(object, NiBooleanExtraData_vtable);
    At<uint8_t>(object, 0xC) = 0;
    return object;
}

// 0x766180: NiTexturingProperty
void* __cdecl NiTexturingProperty_CreateObject() {
    void* object = NiNew(0x34);
    return object != nullptr ? TexturingProperty_Construct(object) : nullptr;
}

// 0x768160: NiFloatExtraData (0.0)
void* __cdecl NiFloatExtraData_CreateObject() {
    void* object = NiNew(0x10);
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    At<float>(object, 0xC) = 0.0f;
    SetVtable(object, NiFloatExtraData_vtable);
    return object;
}

// 0x7682F0: NiIntegerExtraData (0)
void* __cdecl NiIntegerExtraData_CreateObject() {
    return NewExtraData16(NiIntegerExtraData_vtable);
}

// 0x7684D0: NiStringExtraData (the null handle)
void* __cdecl NiStringExtraData_CreateObject() {
    void* object = NiNew(0x10);
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    SetVtable(object, NiStringExtraData_vtable);
    At<const char*>(object, 0xC) = NullFixedString;
    return object;
}

// 0x769330: NiColorExtraData (zeroed, then NiColorA's default)
void* __cdecl NiColorExtraData_CreateObject() {
    void* object = NiNew(0x1C);
    if (object == nullptr) {
        return nullptr;
    }
    ExtraData_Construct(object);
    SetVtable(object, NiColorExtraData_vtable);
    At<float>(object, 0xC) = 0.0f;
    At<float>(object, 0x10) = 0.0f;
    At<float>(object, 0x14) = 0.0f;
    At<float>(object, 0x18) = 0.0f;
    At<uint32_t>(object, 0xC) = ColorA_Default[0];
    At<uint32_t>(object, 0x10) = ColorA_Default[1];
    At<uint32_t>(object, 0x14) = ColorA_Default[2];
    At<uint32_t>(object, 0x18) = ColorA_Default[3];
    return object;
}

// 0x769AD0: NiSourceTexture
void* __cdecl NiSourceTexture_CreateObject() {
    void* object = NiNew(0x44);
    return object != nullptr ? SourceTexture_Construct(object) : nullptr;
}

// 0x76A880: NiPixelData (no pixels, one revision)
void* __cdecl NiPixelData_CreateObject() {
    void* object = NiNew(0x70);
    if (object == nullptr) {
        return nullptr;
    }
    RefObject_Construct(object);
    SetVtable(object, NiPixelData_vtable);
    PixelFormat_Construct(static_cast<uint8_t*>(object) + 8);
    At<void*>(object, 0x4C) = nullptr;   // the palette
    At<void*>(object, 0x50) = nullptr;   // the pixels
    At<void*>(object, 0x54) = nullptr;   // the widths
    At<void*>(object, 0x58) = nullptr;   // the heights
    At<void*>(object, 0x5C) = nullptr;   // the offsets
    At<uint32_t>(object, 0x60) = 0;      // the mipmap levels
    At<uint32_t>(object, 0x6C) = 0;      // the faces
    At<uint32_t>(object, 0x64) = 0;      // the pixel stride
    At<uint32_t>(object, 0x68) = 1;      // the revision
    return object;
}

// 0x76BE00: NiTextureEffect
void* __cdecl NiTextureEffect_CreateObject() {
    void* object = NiNew(0x190);
    return object != nullptr ? TextureEffect_Construct(object) : nullptr;
}

// 0x76CB00: NiFloatsExtraData (none)
void* __cdecl NiFloatsExtraData_CreateObject() {
    return NewArrayExtraData(NiFloatsExtraData_vtable);
}

// 0x76CEC0: NiAlphaProperty (flags 0xEC, test reference 0)
void* __cdecl NiAlphaProperty_CreateObject() {
    void* object = NiNew(0x1C);
    if (object == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(object);
    SetVtable(object, NiAlphaProperty_vtable);
    At<uint8_t>(object, 0x1A) = 0;
    At<uint16_t>(object, 0x18) = 0xEC;
    return object;
}

// 0x76D660: NiZBufferProperty (flags 0xF; Bully's depth bias fields 0, 0, 0, 1, 1)
void* __cdecl NiZBufferProperty_CreateObject() {
    void* object = NiNew(0x30);
    if (object == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(object);
    At<float>(object, 0x1C) = 0.0f;
    SetVtable(object, NiZBufferProperty_vtable);
    At<float>(object, 0x20) = 0.0f;
    At<uint32_t>(object, 0x24) = 0;
    At<uint16_t>(object, 0x18) = 0xF;
    At<float>(object, 0x28) = 1.0f;
    At<float>(object, 0x2C) = 1.0f;
    return object;
}

// 0x76DAE0: NiIntegersExtraData (none)
void* __cdecl NiIntegersExtraData_CreateObject() {
    return NewArrayExtraData(NiIntegersExtraData_vtable);
}

// 0x76E380: NiSkinPartition (no partitions; the count left)
void* __cdecl NiSkinPartition_CreateObject() {
    void* object = NiNew(0x10);
    if (object == nullptr) {
        return nullptr;
    }
    RefObject_Construct(object);
    SetVtable(object, NiSkinPartition_vtable);
    At<void*>(object, 0xC) = nullptr;
    return object;
}

// 0x773340: NiShadowGenerator (without a light)
void* __cdecl NiShadowGenerator_CreateObject() {
    void* object = NiNew(0x68);
    return object != nullptr ? ShadowGenerator_ConstructDefault(object) : nullptr;
}

// 0x774E90: NiSpotLight
void* __cdecl NiSpotLight_CreateObject() {
    void* object = NiNew(0x150);
    return object != nullptr ? SpotLight_Construct(object) : nullptr;
}

// 0x775210: NiPointLight (light type 2; attenuation 0, 1, 0)
void* __cdecl NiPointLight_CreateObject() {
    void* object = NiNew(0x130);
    if (object == nullptr) {
        return nullptr;
    }
    Light_Construct(object);
    At<float>(object, 0x124) = 0.0f;
    SetVtable(object, NiPointLight_vtable);
    At<uint8_t>(object, 0xC5) = 2;
    At<float>(object, 0x128) = 1.0f;
    At<float>(object, 0x12C) = 0.0f;
    return object;
}

// 0x775750: NiMaterialProperty
void* __cdecl NiMaterialProperty_CreateObject() {
    void* object = NiNew(0x5C);
    return object != nullptr ? MaterialProperty_Construct(object) : nullptr;
}

// 0x777220: NiSkinInstance (nothing bound; the frame -1)
void* __cdecl NiSkinInstance_CreateObject() {
    void* object = NiNew(0x30);
    if (object == nullptr) {
        return nullptr;
    }
    RefObject_Construct(object);
    SetVtable(object, NiSkinInstance_vtable);
    At<void*>(object, 0x8) = nullptr;
    At<void*>(object, 0xC) = nullptr;
    At<void*>(object, 0x10) = nullptr;
    At<void*>(object, 0x14) = nullptr;
    At<uint32_t>(object, 0x1C) = 0;
    At<uint32_t>(object, 0x20) = 0;
    At<uint32_t>(object, 0x24) = 0;
    At<uint32_t>(object, 0x28) = 0;
    At<uint32_t>(object, 0x2C) = 0;
    At<int32_t>(object, 0x18) = -1;
    return object;
}

// 0x777E80: NiBinaryExtraData (none)
void* __cdecl NiBinaryExtraData_CreateObject() {
    return NewArrayExtraData(NiBinaryExtraData_vtable);
}

// 0x778220: NiFogProperty (flags 0, depth 1, the colour zeroed then its default)
void* __cdecl NiFogProperty_CreateObject() {
    void* object = NiNew(0x2C);
    if (object == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(object);
    SetVtable(object, NiFogProperty_vtable);
    At<float>(object, 0x20) = 0.0f;
    At<float>(object, 0x24) = 0.0f;
    At<float>(object, 0x28) = 0.0f;
    At<uint16_t>(object, 0x18) = 0;
    At<float>(object, 0x1C) = 1.0f;
    At<uint32_t>(object, 0x20) = Color_FogDefault[0];
    At<uint32_t>(object, 0x24) = Color_FogDefault[1];
    At<uint32_t>(object, 0x28) = Color_FogDefault[2];
    return object;
}

// 0x7786A0: NiShadeProperty (flags 1: smooth)
void* __cdecl NiShadeProperty_CreateObject() {
    return NewFlagsProperty(NiShadeProperty_vtable, 1);
}

// 0x7788C0: NiStencilProperty (flags 0x4180, reference 0, mask all)
void* __cdecl NiStencilProperty_CreateObject() {
    void* object = NiNew(0x24);
    if (object == nullptr) {
        return nullptr;
    }
    ObjectNET_Construct(object);
    SetVtable(object, NiStencilProperty_vtable);
    At<uint32_t>(object, 0x1C) = 0;
    At<uint32_t>(object, 0x20) = 0xFFFFFFFF;
    At<uint16_t>(object, 0x18) = 0x4180;
    return object;
}

// 0x779530: NiVertexColorProperty (flags 8)
void* __cdecl NiVertexColorProperty_CreateObject() {
    return NewFlagsProperty(NiVertexColorProperty_vtable, 8);
}

// 0x7799A0: NiLines (NiGeometry's constructor)
void* __cdecl NiLines_CreateObject() {
    void* object = NiNew(0xEC);
    if (object == nullptr) {
        return nullptr;
    }
    Geometry_Construct(object);
    SetVtable(object, NiLines_vtable);
    return object;
}

// 0x779C80: NiLinesData (no connection flags)
void* __cdecl NiLinesData_CreateObject() {
    void* object = NiNew(0x44);
    if (object == nullptr) {
        return nullptr;
    }
    GeometryData_ConstructDefault(object);
    SetVtable(object, NiLinesData_vtable);
    At<void*>(object, 0x40) = nullptr;
    return object;
}

// 0x77B8B0: NiTriShape (NiTriBasedGeom's constructor)
void* __cdecl NiTriShape_CreateObject() {
    void* object = NiNew(0xEC);
    if (object == nullptr) {
        return nullptr;
    }
    TriBasedGeom_Construct(object);
    SetVtable(object, NiTriShape_vtable);
    return object;
}

// 0x77BB10: NiTriShapeData (NiTriBasedGeomData's constructor, no triangles or shared normals)
void* __cdecl NiTriShapeData_CreateObject() {
    void* object = NiNew(0x58);
    if (object == nullptr) {
        return nullptr;
    }
    TriBasedGeomData_Construct(object);
    At<uint32_t>(object, 0x44) = 0;
    At<void*>(object, 0x48) = nullptr;
    At<void*>(object, 0x4C) = nullptr;
    At<uint16_t>(object, 0x50) = 0;
    At<void*>(object, 0x54) = nullptr;
    SetVtable(object, NiTriShapeData_vtable);
    return object;
}

// 0x7841A0: NiAmbientLight (light type 0)
void* __cdecl NiAmbientLight_CreateObject() {
    void* object = NiNew(0x124);
    if (object == nullptr) {
        return nullptr;
    }
    Light_Construct(object);
    SetVtable(object, NiAmbientLight_vtable);
    At<uint8_t>(object, 0xC5) = 0;
    return object;
}

// 0x788850: NiScreenElementsData (NiTriShapeData's constructor, no polygons)
void* __cdecl NiScreenElementsData_CreateObject() {
    void* object = NiNew(0x70);
    if (object == nullptr) {
        return nullptr;
    }
    TriShapeData_Construct(object);
    At<void*>(object, 0x58) = nullptr;
    At<void*>(object, 0x5C) = nullptr;
    At<uint16_t>(object, 0x60) = 0;
    At<uint16_t>(object, 0x62) = 0;
    At<uint16_t>(object, 0x64) = 0;
    At<uint16_t>(object, 0x66) = 0;
    At<uint16_t>(object, 0x68) = 0;
    At<uint16_t>(object, 0x6A) = 0;
    At<uint16_t>(object, 0x6C) = 0;
    At<uint8_t>(object, 0x6E) = 0;
    SetVtable(object, NiScreenElementsData_vtable);
    return object;
}

#define NICREATORS_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NICREATORS_REVERSED("NiNode::CreateObject", 0x00760A60, NiNode_CreateObject);
NICREATORS_REVERSED("NiCamera::CreateObject", 0x00763D50, NiCamera_CreateObject);
NICREATORS_REVERSED("NiExtraData::CreateObject", 0x007656F0, NiExtraData_CreateObject);
NICREATORS_REVERSED("NiBooleanExtraData::CreateObject", 0x00765860, NiBooleanExtraData_CreateObject);
NICREATORS_REVERSED("NiTexturingProperty::CreateObject", 0x00766180, NiTexturingProperty_CreateObject);
NICREATORS_REVERSED("NiFloatExtraData::CreateObject", 0x00768160, NiFloatExtraData_CreateObject);
NICREATORS_REVERSED("NiIntegerExtraData::CreateObject", 0x007682F0, NiIntegerExtraData_CreateObject);
NICREATORS_REVERSED("NiStringExtraData::CreateObject", 0x007684D0, NiStringExtraData_CreateObject);
NICREATORS_REVERSED("NiColorExtraData::CreateObject", 0x00769330, NiColorExtraData_CreateObject);
NICREATORS_REVERSED("NiSourceTexture::CreateObject", 0x00769AD0, NiSourceTexture_CreateObject);
NICREATORS_REVERSED("NiPixelData::CreateObject", 0x0076A880, NiPixelData_CreateObject);
NICREATORS_REVERSED("NiTextureEffect::CreateObject", 0x0076BE00, NiTextureEffect_CreateObject);
NICREATORS_REVERSED("NiFloatsExtraData::CreateObject", 0x0076CB00, NiFloatsExtraData_CreateObject);
NICREATORS_REVERSED("NiAlphaProperty::CreateObject", 0x0076CEC0, NiAlphaProperty_CreateObject);
NICREATORS_REVERSED("NiZBufferProperty::CreateObject", 0x0076D660, NiZBufferProperty_CreateObject);
NICREATORS_REVERSED("NiIntegersExtraData::CreateObject", 0x0076DAE0, NiIntegersExtraData_CreateObject);
NICREATORS_REVERSED("NiSkinPartition::CreateObject", 0x0076E380, NiSkinPartition_CreateObject);
NICREATORS_REVERSED("NiShadowGenerator::CreateObject", 0x00773340, NiShadowGenerator_CreateObject);
NICREATORS_REVERSED("NiSpotLight::CreateObject", 0x00774E90, NiSpotLight_CreateObject);
NICREATORS_REVERSED("NiPointLight::CreateObject", 0x00775210, NiPointLight_CreateObject);
NICREATORS_REVERSED("NiMaterialProperty::CreateObject", 0x00775750, NiMaterialProperty_CreateObject);
NICREATORS_REVERSED("NiSkinInstance::CreateObject", 0x00777220, NiSkinInstance_CreateObject);
NICREATORS_REVERSED("NiBinaryExtraData::CreateObject", 0x00777E80, NiBinaryExtraData_CreateObject);
NICREATORS_REVERSED("NiFogProperty::CreateObject", 0x00778220, NiFogProperty_CreateObject);
NICREATORS_REVERSED("NiShadeProperty::CreateObject", 0x007786A0, NiShadeProperty_CreateObject);
NICREATORS_REVERSED("NiStencilProperty::CreateObject", 0x007788C0, NiStencilProperty_CreateObject);
NICREATORS_REVERSED("NiVertexColorProperty::CreateObject", 0x00779530, NiVertexColorProperty_CreateObject);
NICREATORS_REVERSED("NiLines::CreateObject", 0x007799A0, NiLines_CreateObject);
NICREATORS_REVERSED("NiLinesData::CreateObject", 0x00779C80, NiLinesData_CreateObject);
NICREATORS_REVERSED("NiTriShape::CreateObject", 0x0077B8B0, NiTriShape_CreateObject);
NICREATORS_REVERSED("NiTriShapeData::CreateObject", 0x0077BB10, NiTriShapeData_CreateObject);
NICREATORS_REVERSED("NiAmbientLight::CreateObject", 0x007841A0, NiAmbientLight_CreateObject);
NICREATORS_REVERSED("NiScreenElementsData::CreateObject", 0x00788850, NiScreenElementsData_CreateObject);

} // namespace ni
