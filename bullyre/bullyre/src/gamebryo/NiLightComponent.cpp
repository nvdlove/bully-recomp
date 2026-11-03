// NiLightComponent (vtables 0x98A424 / 0x98A2F8 at +8, 0x68 bytes): an entity's light, made
// by Update for its "Light Type" (Ambient, Directional, Point or Spot: NiAmbientLight,
// NiDirectionalLight, NiPointLight, NiSpotLight) and kept in step with the properties
// ("Dimmer", the three colours, the attenuations, the spot angles and exponent), the
// "Affected Entities" (their scene roots attached as the light's affected nodes) and the
// entity's transformation; the light is its "Scene Root Pointer", "Direction" its world
// direction. No master: nothing is inherited. Its interface methods get the interface pointer
// (+8); the helpers the object. The default constructor and the destructor are the other
// pass's (0x83CE80, 0x83CF80).
//
// Also the interfaces' RemoveReference and the folded `xor al, al; ret 8` (the defaults of
// the property interface's two-argument setters; BullyNiDX9Renderer's slots 33 and 35 too).

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <intrin.h>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDynamicEffect.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::ec {
namespace {

struct LightComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;              // +0x8
    uint8_t flags;                  // +0xC (bit 0 the type wasn't valid, 1 the light's values
                                    //  changed, 2 the affected entities changed)
    uint8_t padD[3];
    re::Ptr32<const char> lightType;        // +0x10
    float dimmer;                   // +0x14
    NiColor ambient;                // +0x18
    NiColor diffuse;                // +0x24
    NiColor specular;               // +0x30
    float constantAttenuation;      // +0x3C
    float linearAttenuation;        // +0x40
    float quadraticAttenuation;     // +0x44
    float outerSpotAngle;           // +0x48
    float innerSpotAngle;           // +0x4C
    float spotExponent;             // +0x50
    NiTArray<void*> affectedEntities;   // +0x54 (NiTPrimitiveArray<NiEntityInterface*>)
    re::Ptr32<NiLight> light;                 // +0x64 (a reference)
};
static_assert(offsetof(LightComponent, lightType) == 0x10);
static_assert(offsetof(LightComponent, ambient) == 0x18);
static_assert(offsetof(LightComponent, constantAttenuation) == 0x3C);
static_assert(offsetof(LightComponent, spotExponent) == 0x50);
static_assert(offsetof(LightComponent, affectedEntities) == 0x54);
static_assert(offsetof(LightComponent, light) == 0x64);
static_assert(sizeof(LightComponent) == 0x68);

enum : uint8_t {
    kInvalidType = 0x1,
    kValuesChanged = 0x2,
    kEntitiesChanged = 0x4,
};

constexpr uintptr_t LightComponent_vtable = 0x0098A424;
constexpr uintptr_t LightComponentIface_vtable = 0x0098A2F8;
const char* const kLightTypeSemantic = reinterpret_cast<const char*>(0x008FFF48);   // "Light Type"

GAME_VAR(NiFixedString, ERR_TRANSLATION_NOT_FOUND, 0x020E5D7C);
GAME_VAR(NiFixedString, ERR_ROTATION_NOT_FOUND, 0x020E5D80);
GAME_VAR(NiFixedString, ERR_SCALE_NOT_FOUND, 0x020E5D84);
GAME_VAR(NiFixedString, ERR_INVALID_LIGHT_TYPE, 0x020E5D88);
GAME_VAR(NiFixedString, LT_AMBIENT, 0x020E5D8C);
GAME_VAR(NiFixedString, LT_DIRECTIONAL, 0x020E5D90);
GAME_VAR(NiFixedString, LT_POINT, 0x020E5D94);
GAME_VAR(NiFixedString, LT_SPOT, 0x020E5D98);
GAME_VAR(NiFixedString, ms_kClassName, 0x020E5D9C);
GAME_VAR(NiFixedString, ms_kComponentName, 0x020E5DA0);   // "Light"
GAME_VAR(NiFixedString, PROP_LIGHT_TYPE, 0x020E5DA4);
GAME_VAR(NiFixedString, PROP_DIMMER, 0x020E5DA8);
GAME_VAR(NiFixedString, PROP_AMBIENT_COLOR, 0x020E5DAC);
GAME_VAR(NiFixedString, PROP_DIFFUSE_COLOR, 0x020E5DB0);
GAME_VAR(NiFixedString, PROP_SPECULAR_COLOR, 0x020E5DB4);
GAME_VAR(NiFixedString, PROP_CONSTANT_ATTENUATION, 0x020E5DB8);
GAME_VAR(NiFixedString, PROP_LINEAR_ATTENUATION, 0x020E5DBC);
GAME_VAR(NiFixedString, PROP_QUADRATIC_ATTENUATION, 0x020E5DC0);
GAME_VAR(NiFixedString, PROP_OUTER_SPOT_ANGLE, 0x020E5DC4);
GAME_VAR(NiFixedString, PROP_INNER_SPOT_ANGLE, 0x020E5DC8);
GAME_VAR(NiFixedString, PROP_SPOT_EXPONENT, 0x020E5DCC);
GAME_VAR(NiFixedString, PROP_AFFECTED_ENTITIES, 0x020E5DD0);
GAME_VAR(NiFixedString, PROP_DIRECTION, 0x020E5DD4);
GAME_VAR(NiFixedString, PROP_SCENE_ROOT_POINTER, 0x020E5DD8);
constexpr uintptr_t kDescriptions = 0x020E5DDC;   // (the 13 descriptions, in the properties' order)
GAME_VAR(NiFixedString, PROP_TRANSLATION, 0x020E5E10);
GAME_VAR(NiFixedString, PROP_ROTATION, 0x020E5E14);
GAME_VAR(NiFixedString, PROP_SCALE, 0x020E5E18);
GAME_VAR(NiFixedString, PROP_SPOT_ANGLE, 0x020E5E1C);   // (a set-only name: the outer angle, the inner 0)
GAME_VAR(NiUniqueID, ms_kTemplateID, 0x020E5D68);
GAME_VAR(uint32_t, ms_kTemplateIDGuard, 0x020E5D78);
GAME_VAR(uint32_t, NiNode_ClassIndex, 0x020E2550);

// the RTTIs NiDynamicCast walks
constexpr uintptr_t NiNode_RTTI = 0x020E2548;
constexpr uintptr_t NiPointLight_RTTI = 0x020E2CD0;
constexpr uintptr_t NiSpotLight_RTTI = 0x020E2C9C;

GAME_FN(NiObject*, __cdecl, NiDynamicCast, 0x00401FF0, uintptr_t rtti, NiObject* object);
GAME_FN(void*, __thiscall, AmbientLight_Construct, 0x00784180, void* self);
GAME_FN(void*, __thiscall, DirectionalLight_Construct, 0x007D5500, void* self);
GAME_FN(void*, __thiscall, PointLight_Construct, 0x007751A0, void* self);
GAME_FN(void*, __thiscall, SpotLight_Construct, 0x00774D60, void* self);
GAME_FN(void, __thiscall, SpotLight_SetSpotAngle, 0x0040DDF0, NiObject* self, float angle);
GAME_FN(void, __thiscall, SpotLight_SetInnerSpotAngle, 0x0040DE30, NiObject* self, float angle);
GAME_FN(void, __thiscall, DynamicEffect_AttachAffectedNode, 0x00771A10, NiLight* self, NiObject* node);
GAME_FN(void, __thiscall, DynamicEffect_DetachAllAffectedNodes, 0x00771A80, NiLight* self);
GAME_FN(void, __thiscall, DynamicEffect_DetachAffectedNode, 0x00771F10, NiLight* self, NiObject* node);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E240, void* self);
GAME_FN(uint8_t, __thiscall, Matrix3_Equal, 0x007686A0, const NiMatrix3* self, const NiMatrix3* other);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
// NiTPrimitiveSet<NiNode*>
GAME_FN(void, __thiscall, NodeSet_Construct, 0x0081E260, NiTSet<NiObject*>* self, uint32_t size);
GAME_FN(void, __thiscall, NodeSet_Realloc, 0x0083AD20, NiTSet<NiObject*>* self, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, ArrayUpdateSize, 0x008498F0, NiTArray<void*>* array);
GAME_FN(void, __thiscall, PrimitiveArray_Construct, 0x0082F290, void* self, uint32_t maxSize, uint32_t growBy);

// this file's, through their addresses
GAME_FN(void, __thiscall, SetAmbientColor, 0x0084D7B0, LightComponent* self, const NiColor* color);
GAME_FN(void, __thiscall, SetDiffuseColor, 0x0084D800, LightComponent* self, const NiColor* color);
GAME_FN(void, __thiscall, SetSpecularColor, 0x0084D850, LightComponent* self, const NiColor* color);
GAME_FN(void, __thiscall, SetLinearAttenuation, 0x0084D8A0, LightComponent* self, float value);
GAME_FN(void, __thiscall, SetQuadraticAttenuation, 0x0084D8D0, LightComponent* self, float value);
GAME_FN(void, __thiscall, SetOuterSpotAngle, 0x0084D900, LightComponent* self, float value);
GAME_FN(void, __thiscall, SetInnerSpotAngle, 0x0084D930, LightComponent* self, float value);
GAME_FN(void, __thiscall, SetSpotExponent, 0x0084D960, LightComponent* self, float value);
GAME_FN(LightComponent*, __thiscall, Component_Construct, 0x0084EDA0, void* self, const re::Ptr32<const char>* lightType,
        float dimmer, const NiColor* ambient, const NiColor* diffuse, const NiColor* specular, float constant,
        float linear, float quadratic, float outerSpotAngle, float innerSpotAngle, float spotExponent,
        uint32_t entityCount, uint32_t entityGrowBy);
GAME_FN(void, __thiscall, ReleaseLight, 0x0084EE90, LightComponent* self);

inline LightComponent* Self(void* iface) {
    return FromIface<LightComponent>(iface);
}

// its 14 properties, in its order (the names' addresses, 0x20E5DA4..0x20E5DD8)
inline const re::Ptr32<const char>* Property(int index) {
    return reinterpret_cast<const re::Ptr32<const char>*>(0x020E5DA4 + 4 * index);
}

// which of them (-1: none)
inline int PropertyIndex(const re::Ptr32<const char>* name) {
    NiFixedString n = *name;
    for (int i = 0; i < 14; i++) {
        if (n == *Property(i)) {
            return i;
        }
    }
    return -1;
}

enum : int {
    kLightType = 0,
    kDimmer,
    kAmbient,
    kDiffuse,
    kSpecular,
    kConstant,
    kLinear,
    kQuadratic,
    kOuterSpot,
    kInnerSpot,
    kExponent,
    kAffectedEntities,
    kDirection,
    kSceneRoot,
};

inline bool IsValidType(NiFixedString type) {
    return type == LT_AMBIENT || type == LT_DIRECTIONAL || type == LT_POINT || type == LT_SPOT;
}

// ---- the interfaces' RemoveReference (folded: every component's slot 1) and the folded
// two-argument `return false`

void __fastcall RemoveReference_(void* iface, re::Edx) {
    NiRefObject* object = FromIface<NiRefObject>(iface);
    if (_InterlockedDecrement(reinterpret_cast<volatile long*>(&object->refCount)) == 0) {
        re::PtrCast<void(__thiscall*)(NiRefObject*)>(VtableOf(object)[1])(object);
    }
}

uint8_t __fastcall ReturnFalse2(void*, re::Edx, void*, void*) {
    return 0;
}

// ---- the static names (the exe's texts)

constexpr StaticString kNames[] = {
    {0x020E5D7C, 0x0098B554},   // "Translation property not found."
    {0x020E5D80, 0x0098B534},   // "Rotation property not found."
    {0x020E5D84, 0x0098B518},   // "Scale property not found."
    {0x020E5D88, 0x0098B85C},   // "The specified light type is not valid."
    {0x020E5D8C, 0x008FFF40},   // "Ambient"
    {0x020E5D90, 0x008FFF24},   // "Directional"
    {0x020E5D94, 0x008FFF38},   // "Point"
    {0x020E5D98, 0x008FFF30},   // "Spot"
    {0x020E5D9C, 0x008FFE50},   // "NiLightComponent"
    {0x020E5DA0, 0x0093D070},   // "Light"
    {0x020E5DA4, 0x008FFF48},   // "Light Type"
    {0x020E5DA8, 0x009000CC},   // "Dimmer"
    {0x020E5DAC, 0x009000BC},   // "Color (Ambient)"
    {0x020E5DB0, 0x009000AC},   // "Color (Diffuse)"
    {0x020E5DB4, 0x00900098},   // "Color (Specular)"
    {0x020E5DB8, 0x00900080},   // "Attenuation (Constant)"
    {0x020E5DBC, 0x00900068},   // "Attenuation (Linear)"
    {0x020E5DC0, 0x00900050},   // "Attenuation (Quadratic)"
    {0x020E5DC4, 0x0090003C},   // "Spot Angle (Outer)"
    {0x020E5DC8, 0x00900028},   // "Spot Angle (Inner)"
    {0x020E5DCC, 0x00900018},   // "Spot Exponent"
    {0x020E5DD0, 0x009000D4},   // "Affected Entities"
    {0x020E5DD4, 0x00950F20},   // "Direction"
    {0x020E5DD8, 0x0098AF48},   // "Scene Root Pointer"
    {0x020E5DDC, 0x0098B848},   // "The type of light."
    {0x020E5DE0, 0x0098B820},   // "The relative intensity of the light."
    {0x020E5DE4, 0x0098B800},   // "The ambient color of the light."
    {0x020E5DE8, 0x0098B7E0},   // "The diffuse color of the light."
    {0x020E5DEC, 0x0098B7BC},   // "The specular color of the light."
    {0x020E5DF0, 0x0098B784},   // "The constant distance attenuation factor for the light."
    {0x020E5DF4, 0x0098B74C},   // "The linear distance attenuation factor for the light."
    {0x020E5DF8, 0x0098B710},   // "The quadratic distance attenuation factor for the light."
    {0x020E5DFC, 0x0098B698},   // "The angle of the spot light's outer cone..."
    {0x020E5E00, 0x0098B618},   // "The angle of the spot light's inner cone..."
    {0x020E5E04, 0x0098B5E4},   // "The angular attenuation factor for the spot light."
    {0x020E5E08, 0x0098B5BC},   // "Entities that the light will affect."
    {0x020E5E0C, 0x0098B594},   // "The wold-space direction of the light."
    {0x020E5E10, 0x008FFFBC},   // "Translation"
    {0x020E5E14, 0x008FFF9C},   // "Rotation"
    {0x020E5E18, 0x008FFFA8},   // "Scale"
    {0x020E5E1C, 0x0098B588},   // "Spot Angle"
};

void __cdecl SDMInit() {
    SetStaticStrings(kNames);
}

void __cdecl SDMShutdown() {
    ClearStaticStrings(kNames);
}

// ---- the interface's methods

NiUniqueID* __fastcall GetTemplateID_(void*, re::Edx, NiUniqueID* result) {
    return ReturnTemplateID(result, ms_kTemplateIDGuard, ms_kTemplateID, 0xD90F2618, 0x4D8AE1F0, 0xE847EF91,
                            0xFE7EE820);
}

// read-only: the direction and the light
uint8_t __fastcall IsPropertyReadOnly(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* readOnly) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    *readOnly = (i == kDirection || i == kSceneRoot) ? 1 : 0;
    return 1;
}

uint8_t __fastcall IsPropertyUnique(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* unique) {
    if (PropertyIndex(name) < 0) {
        return 0;
    }
    *unique = 1;
    return 1;
}

// serializable: unique and not read-only (the light never). The read-only byte starts as the
// high byte of the pushed `this`
uint8_t __fastcall IsPropertySerializable(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* serializable) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    if (i == kSceneRoot) {
        *serializable = 0;
        return 1;
    }
    uint8_t unique = uint8_t(reinterpret_cast<uintptr_t>(name));
    V(iface, slot::IsPropertyUnique, name, &unique);
    uint8_t readOnly = uint8_t(reinterpret_cast<uintptr_t>(iface) >> 24);
    V(iface, slot::IsPropertyReadOnly, name, &readOnly);
    *serializable = (unique != 0 && readOnly == 0) ? 1 : 0;
    return 1;
}

// (CanResetProperty, MakePropertyUnique, IsPropertyInheritable: false for its properties)
uint8_t __fastcall PropertyFlagFalse(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* value) {
    if (PropertyIndex(name) < 0) {
        return 0;
    }
    *value = 0;
    return 1;
}

uint8_t __fastcall IsExternalAssetPath(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* isPath) {
    if (PropertyIndex(name) < 0) {
        return 0;
    }
    *isPath = 0;
    return 1;
}

// one, or the affected entities' count
uint8_t __fastcall GetElementCount(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    *count = i == kAffectedEntities ? Self(iface)->affectedEntities.size : 1;
    return 1;
}

uint8_t __fastcall IsCollection_(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* isCollection) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    *isCollection = i == kAffectedEntities ? 1 : 0;
    return 1;
}

uint8_t __fastcall GetFloat(void* iface, re::Edx, const re::Ptr32<const char>* name, float* value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    LightComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_DIMMER) {
        *value = self->dimmer;
    } else if (n == PROP_CONSTANT_ATTENUATION) {
        *value = self->constantAttenuation;
    } else if (n == PROP_LINEAR_ATTENUATION) {
        *value = self->linearAttenuation;
    } else if (n == PROP_QUADRATIC_ATTENUATION) {
        *value = self->quadraticAttenuation;
    } else if (n == PROP_OUTER_SPOT_ANGLE) {
        *value = self->outerSpotAngle;
    } else if (n == PROP_INNER_SPOT_ANGLE) {
        *value = self->innerSpotAngle;
    } else if (n == PROP_SPOT_EXPONENT) {
        *value = self->spotExponent;
    } else {
        return 0;
    }
    return 1;
}

uint8_t __fastcall GetColor(void* iface, re::Edx, const re::Ptr32<const char>* name, NiColor* value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    LightComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_AMBIENT_COLOR) {
        *value = self->ambient;
    } else if (n == PROP_DIFFUSE_COLOR) {
        *value = self->diffuse;
    } else if (n == PROP_SPECULAR_COLOR) {
        *value = self->specular;
    } else {
        return 0;
    }
    return 1;
}

// the light's world direction (its rotation's first column; zero without a light)
uint8_t __fastcall GetDirection(void* iface, re::Edx, const re::Ptr32<const char>* name, NiPoint3* value, uint32_t index) {
    if (index != 0 || *name != PROP_DIRECTION) {
        return 0;
    }
    NiLight* light = Self(iface)->light;
    if (light != nullptr) {
        value->x = light->world.rotate.m[0][0];
        value->y = light->world.rotate.m[1][0];
        value->z = light->world.rotate.m[2][0];
        return 1;
    }
    *value = Point3_ZERO;
    return 1;
}

uint8_t __fastcall GetObject_(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<NiObject>* value, uint32_t index) {
    if (index == 0 && *name == PROP_SCENE_ROOT_POINTER) {
        *value = Self(iface)->light;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetEntity(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<void>* value, uint32_t index) {
    if (*name != PROP_AFFECTED_ENTITIES) {
        return 0;
    }
    NiTArray<void*>& entities = Self(iface)->affectedEntities;
    *value = index < entities.size ? entities.base[index] : nullptr;
    return 1;
}

// the setters: a change marks the light's values stale
void __fastcall SetColorAt(LightComponent* self, NiColor* field, const NiColor* color) {
    if (!(color->r == field->r) || !(color->g == field->g) || !(color->b == field->b)) {
        *field = *color;
        self->flags |= kValuesChanged;
    }
}

void __fastcall SetAmbientColor_(LightComponent* self, re::Edx, const NiColor* color) {
    SetColorAt(self, &self->ambient, color);
}
void __fastcall SetDiffuseColor_(LightComponent* self, re::Edx, const NiColor* color) {
    SetColorAt(self, &self->diffuse, color);
}
void __fastcall SetSpecularColor_(LightComponent* self, re::Edx, const NiColor* color) {
    SetColorAt(self, &self->specular, color);
}

inline void SetFloatAt(LightComponent* self, float& field, float value) {
    if (value != field) {
        field = value;
        self->flags |= kValuesChanged;
    }
}

void __fastcall SetLinearAttenuation_(LightComponent* self, re::Edx, float value) {
    SetFloatAt(self, self->linearAttenuation, value);
}
void __fastcall SetQuadraticAttenuation_(LightComponent* self, re::Edx, float value) {
    SetFloatAt(self, self->quadraticAttenuation, value);
}
void __fastcall SetOuterSpotAngle_(LightComponent* self, re::Edx, float value) {
    SetFloatAt(self, self->outerSpotAngle, value);
}
void __fastcall SetInnerSpotAngle_(LightComponent* self, re::Edx, float value) {
    SetFloatAt(self, self->innerSpotAngle, value);
}
void __fastcall SetSpotExponent_(LightComponent* self, re::Edx, float value) {
    SetFloatAt(self, self->spotExponent, value);
}

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kClassName);
}

re::Ptr32<const char>* __fastcall GetName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kComponentName);
}

// the property's own name (the light: none)
uint8_t __fastcall GetDisplayName(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    if (i == kSceneRoot) {
        FixedString_SetString(value, nullptr);
    } else {
        FixedString_Assign(value, name);
    }
    return 1;
}

// the primitive types (and the semantic ones: the light type's is "Light Type")
template <bool Semantic>
uint8_t __fastcall GetType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    if (i == kLightType) {
        if (Semantic) {
            FixedString_SetString(value, kLightTypeSemantic);
        } else {
            FixedString_Assign(value, &PT_STRING);
        }
        return 1;
    }
    const re::Ptr32<const char>* type;
    if (i == kAmbient || i == kDiffuse || i == kSpecular) {
        type = &PT_COLOR;
    } else if (i == kAffectedEntities) {
        type = &PT_ENTITYPOINTER;
    } else if (i == kDirection) {
        type = &PT_POINT3;
    } else if (i == kSceneRoot) {
        type = &PT_NIOBJECTPOINTER;
    } else {
        type = &PT_FLOAT;
    }
    FixedString_Assign(value, type);
    return 1;
}

uint8_t __fastcall GetDescription(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    if (i == kSceneRoot) {
        FixedString_SetString(value, nullptr);
    } else {
        FixedString_Assign(value, reinterpret_cast<re::Ptr32<const char>*>(kDescriptions + 4 * i));
    }
    return 1;
}

uint8_t __fastcall GetCategory(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    if (PropertyIndex(name) < 0) {
        return 0;
    }
    FixedString_Assign(category, &ms_kComponentName);
    return 1;
}

// the affected entities cut to `count` (the ones past it cleared through SetPropertyData)
uint8_t __fastcall SetElementCount(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t count, uint8_t* countSet) {
    int i = PropertyIndex(name);
    if (i < 0) {
        return 0;
    }
    if (i != kAffectedEntities) {
        *countSet = 0;
        return 1;
    }
    NiTArray<void*>& entities = Self(iface)->affectedEntities;
    uint32_t size = entities.size;
    for (uint32_t j = count; j < size; j++) {
        V(iface, slot::SetEntity, name, static_cast<void*>(nullptr), j);
    }
    PointerArray_SetSize(&entities, count);
    *countSet = 1;
    return 1;
}

// the dimmer and constant attenuation set here; the rest through their setters ("Spot
// Angle": the outer angle, and the inner 0)
uint8_t __fastcall SetFloat(void* iface, re::Edx, const re::Ptr32<const char>* name, float value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    LightComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_DIMMER) {
        SetFloatAt(self, self->dimmer, value);
        return 1;
    }
    if (n == PROP_CONSTANT_ATTENUATION) {
        SetFloatAt(self, self->constantAttenuation, value);
        return 1;
    }
    if (n == PROP_LINEAR_ATTENUATION) {
        SetLinearAttenuation(self, value);
        return 1;
    }
    if (n == PROP_QUADRATIC_ATTENUATION) {
        SetQuadraticAttenuation(self, value);
        return 1;
    }
    if (n == PROP_OUTER_SPOT_ANGLE) {
        SetOuterSpotAngle(self, value);
        return 1;
    }
    if (n == PROP_INNER_SPOT_ANGLE) {
        SetInnerSpotAngle(self, value);
        return 1;
    }
    if (n == PROP_SPOT_EXPONENT) {
        SetSpotExponent(self, value);
        return 1;
    }
    if (n == PROP_SPOT_ANGLE) {
        SetOuterSpotAngle(self, value);
        SetInnerSpotAngle(self, 0.0f);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetString(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value, uint32_t index) {
    if (index == 0 && *name == PROP_LIGHT_TYPE) {
        FixedString_Assign(value, &Self(iface)->lightType);
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetColor(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiColor* value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    LightComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_AMBIENT_COLOR) {
        SetAmbientColor(self, value);
        return 1;
    }
    if (n == PROP_DIFFUSE_COLOR) {
        SetDiffuseColor(self, value);
        return 1;
    }
    if (n == PROP_SPECULAR_COLOR) {
        SetSpecularColor(self, value);
        return 1;
    }
    return 0;
}

// an affected entity set: one cleared has its scene roots (NiNodes) detached from the light
// first; a new one marks the entities changed (Update attaches them). The count and the object
// the entity hands back start as what the game's reused stack slots hold (the name's, the
// interface's) when the calls don't write them
uint8_t __fastcall SetEntity(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<void> value, uint32_t index) {
    if (*name != PROP_AFFECTED_ENTITIES) {
        return 0;
    }
    LightComponent* self = Self(iface);
    NiTArray<void*>& entities = self->affectedEntities;
    if (self->light != nullptr && value == nullptr && index < entities.size) {
        void* entity = entities.base[index];
        uint32_t count = uint32_t(reinterpret_cast<uintptr_t>(name));
        if (entity != nullptr && V(entity, slot::GetElementCount, &PROP_SCENE_ROOT_POINTER, &count)) {
            NiObject* object = static_cast<NiObject*>(iface);
            for (uint32_t j = 0; j < count; j++) {
                V(entity, slot::GetObject, &PROP_SCENE_ROOT_POINTER, &object, j);
                NiObject* node = NiDynamicCast(NiNode_RTTI, object);
                if (node != nullptr) {
                    DynamicEffect_DetachAffectedNode(self->light, node);
                    AVObject_UpdateEffects(node);
                }
            }
        }
    }
    if (entities.maxSize <= index) {
        PointerArray_SetSize(&entities, entities.growBy + index);
    }
    PointerArray_SetAt(&entities, index, &value);
    ArrayUpdateSize(&entities);
    if (value != nullptr) {
        self->flags |= kEntitiesChanged;
    }
    return 1;
}

LightComponent* __fastcall ComponentConstruct(LightComponent* self, re::Edx, const re::Ptr32<const char>* lightType,
                                              float dimmer, const NiColor* ambient, const NiColor* diffuse,
                                              const NiColor* specular, float constant, float linear,
                                              float quadratic, float outerSpotAngle, float innerSpotAngle,
                                              float spotExponent, uint32_t entityCount, uint32_t entityGrowBy) {
    ConstructBases(self);
    self->vtable = reinterpret_cast<void*>(LightComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(LightComponentIface_vtable);
    self->flags = 0;
    FixedStringCopy(&self->lightType, lightType);
    self->dimmer = dimmer;
    self->ambient = *ambient;
    self->diffuse = *diffuse;
    self->specular = *specular;
    self->constantAttenuation = constant;
    self->linearAttenuation = linear;
    self->quadraticAttenuation = quadratic;
    self->outerSpotAngle = outerSpotAngle;
    self->innerSpotAngle = innerSpotAngle;
    self->spotExponent = spotExponent;
    PrimitiveArray_Construct(&self->affectedEntities, entityCount, entityGrowBy);
    self->light = nullptr;
    return self;
}

// the light let go: its affected nodes detached (and their effects updated) first
void __fastcall ReleaseLight_(LightComponent* self, re::Edx) {
    NiLight* light = self->light;
    if (light == nullptr) {
        return;
    }
    uint32_t count = light->affectedNodes.count;
    NiTSet<NiObject*> nodes{count != 0 ? static_cast<re::Ptr32<NiObject>*>(NiMalloc(count * 4)) : nullptr, count, 0};
    for (NiTListNode<NiNode*>* n = light->affectedNodes.head; n != nullptr;) {
        NiObject* node = n->element;
        n = n->next;
        if (nodes.used == nodes.alloced) {
            NodeSet_Realloc(&nodes, nodes.alloced == 0 ? 1 : nodes.alloced * 2);
        }
        nodes.base[nodes.used++] = node;
    }
    DynamicEffect_DetachAllAffectedNodes(self->light);
    for (uint32_t i = 0; i < nodes.used; i++) {
        AVObject_UpdateEffects(nodes.base[i]);
    }
    NiLight* old = self->light;
    if (old != nullptr) {
        Release(old);
        self->light = nullptr;
    }
    self->flags &= ~kInvalidType;
    NiFree(nodes.base);
}

// a copy of its values and affected entities (the argument not looked at: nothing to inherit)
void* __fastcall Clone_(void* iface, re::Edx, int) {
    LightComponent* self = Self(iface);
    void* memory = NiNew(sizeof(LightComponent));
    LightComponent* clone =
        memory == nullptr
            ? nullptr
            : Component_Construct(memory, &self->lightType, self->dimmer, &self->ambient, &self->diffuse,
                                  &self->specular, self->constantAttenuation, self->linearAttenuation,
                                  self->quadraticAttenuation, self->outerSpotAngle, self->innerSpotAngle,
                                  self->spotExponent, self->affectedEntities.maxSize, self->affectedEntities.growBy);
    for (uint32_t i = 0; i < self->affectedEntities.size; i++) {
        void* value = self->affectedEntities.base[i];
        NiTArray<void*>& to = clone->affectedEntities;
        if (i < to.size) {
            if (value == nullptr) {
                if (to.base[i] != nullptr) {
                    to.esize--;
                }
            } else if (to.base[i] == nullptr) {
                to.esize++;
            }
        } else {
            to.size = uint16_t(i + 1);
            if (value != nullptr) {
                to.esize++;
            }
        }
        to.base[i] = value;
    }
    return clone != nullptr ? reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(clone) + 8) : nullptr;
}

// a valid type made the light's (the old light let go)
uint8_t __fastcall SetString(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* value,
                             uint32_t index) {
    if (index != 0 || *name != PROP_LIGHT_TYPE || !IsValidType(*value)) {
        return 0;
    }
    LightComponent* self = Self(iface);
    if (self->lightType != *value) {
        FixedString_Assign(&self->lightType, value);
        ReleaseLight(self);
    }
    return 1;
}

// the transformation it follows
void __fastcall GetDependentPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_TRANSLATION);
    StringSetAdd(names, &PROP_ROTATION);
    StringSetAdd(names, &PROP_SCALE);
}

void __fastcall GetPropertyNames_(void*, re::Edx, StringSet* names) {
    for (int i = 0; i < 14; i++) {
        StringSetAdd(names, Property(i));
    }
}

// the error handler told (slot 2: message, description, the entity's name, the property's)
void ReportMissing(void* errors, void* entity, const re::Ptr32<const char>* message, const re::Ptr32<const char>* property) {
    NiFixedString description = NullFixedString;
    NiFixedString entityName;
    V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
    V<void>(errors, 2, message, &description, &entityName, property);
    FixedStringRelease(&entityName);
    FixedStringRelease(&description);
}

// the light made for a valid type (an invalid one reported once), its values and affected
// nodes brought up to date when they changed, then moved to the entity's transformation
void __fastcall Update(void* iface, re::Edx, void* entity, float time, void* errors, void*) {
    LightComponent* self = Self(iface);
    if (self->light == nullptr) {
        if ((self->flags & kInvalidType) == 0) {
            NiFixedString type = self->lightType;
            if (!IsValidType(type)) {
                NiFixedString entityName;
                V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
                V<void>(errors, 2, &ERR_INVALID_LIGHT_TYPE, &self->lightType, &entityName, &PROP_LIGHT_TYPE);
                FixedString_Destruct(&entityName);
                self->flags |= kInvalidType;
                return;
            }
            void* memory;
            void* light = nullptr;
            bool made = true;
            if (type == LT_AMBIENT) {
                memory = NiNew(0x124);
                light = memory != nullptr ? AmbientLight_Construct(memory) : nullptr;
            } else if (type == LT_DIRECTIONAL) {
                memory = NiNew(0x130);
                light = memory != nullptr ? DirectionalLight_Construct(memory) : nullptr;
            } else if (type == LT_POINT) {
                memory = NiNew(0x130);
                light = memory != nullptr ? PointLight_Construct(memory) : nullptr;
            } else if (type == LT_SPOT) {
                memory = NiNew(0x150);
                light = memory != nullptr ? SpotLight_Construct(memory) : nullptr;
            } else {
                made = false;
            }
            if (made) {
                Pointer_Assign(reinterpret_cast<re::Ptr32<NiObject>*>(&self->light), static_cast<NiObject*>(light));
            }
            self->flags |= kValuesChanged | kEntitiesChanged;
            AVObject_Update(self->light, 0.0f, 1);
        }
        if (self->light == nullptr) {
            return;
        }
    }

    if ((self->flags & kValuesChanged) != 0) {
        NiLight* light = self->light;
        light->revisionID++;
        light->dimmer = self->dimmer;
        light = self->light;
        light->ambient = {self->ambient.r, self->ambient.g, self->ambient.b};
        light->revisionID++;
        light = self->light;
        light->diffuse = {self->diffuse.r, self->diffuse.g, self->diffuse.b};
        light->revisionID++;
        light = self->light;
        light->specular = {self->specular.r, self->specular.g, self->specular.b};
        light->revisionID++;
        NiObject* point = NiDynamicCast(NiPointLight_RTTI, self->light);
        if (point != nullptr) {
            uint32_t& revision = At<uint32_t>(point, 0xD0);
            revision++;
            At<float>(point, 0x124) = self->constantAttenuation;
            revision++;
            At<float>(point, 0x128) = self->linearAttenuation;
            At<float>(point, 0x12C) = self->quadraticAttenuation;
            revision++;
            NiObject* spot = NiDynamicCast(NiSpotLight_RTTI, point);
            if (spot != nullptr) {
                SpotLight_SetSpotAngle(spot, self->outerSpotAngle);
                SpotLight_SetInnerSpotAngle(spot, self->innerSpotAngle);
                At<uint32_t>(spot, 0xD0)++;
                At<float>(spot, 0x144) = self->spotExponent;
            }
        }
        self->flags &= ~kValuesChanged;
    }

    if ((self->flags & kEntitiesChanged) != 0) {
        // the nodes it affected detached (their effects updated), then the entities' scene
        // roots (NiNodes) attached; the change done with when every one was there. The count
        // and the object an entity hands back stay from the call before when one isn't
        // written (the game's stack, uninitialised the first time: ours start at 0)
        NiLight* light = self->light;
        NiTSet<NiObject*> nodes;
        NodeSet_Construct(&nodes, light->affectedNodes.count);
        for (NiTListNode<NiNode*>* n = light->affectedNodes.head; n != nullptr;) {
            const re::Ptr32<NiNode>* element = &n->element;
            n = n->next;
            if (nodes.used == nodes.alloced) {
                NodeSet_Realloc(&nodes, nodes.alloced == 0 ? 1 : nodes.alloced * 2);
            }
            nodes.base[nodes.used] = *element;
            nodes.used++;
        }
        DynamicEffect_DetachAllAffectedNodes(self->light);
        for (uint32_t i = 0; i < nodes.used; i++) {
            AVObject_UpdateEffects(nodes.base[i]);
        }
        uint8_t allFound = 1;
        uint32_t count = 0;
        NiObject* object = nullptr;
        for (uint32_t i = 0; i < self->affectedEntities.size; i++) {
            void* e = self->affectedEntities.base[i];
            if (e == nullptr || !V(e, slot::GetElementCount, &PROP_SCENE_ROOT_POINTER, &count)) {
                continue;
            }
            for (uint32_t j = 0; j < count; j++) {
                V(e, slot::GetObject, &PROP_SCENE_ROOT_POINTER, &object, j);
                if (object == nullptr) {
                    allFound = 0;
                    continue;
                }
                const uint32_t* rtti =
                    re::PtrCast<const uint32_t*(__thiscall*)(NiObject*)>(VtableOf(object)[2])(object);
                if (((1u << (NiNode_ClassIndex & 31)) & rtti[3 + (NiNode_ClassIndex >> 5)]) != 0) {
                    NiObject* node = object;
                    DynamicEffect_AttachAffectedNode(self->light, node);
                    AVObject_UpdateEffects(node);
                }
            }
        }
        if (allFound) {
            self->flags &= ~kEntitiesChanged;
        }
        NiFree(nodes.base);
    }

    bool allFound = true;
    NiPoint3 translate;
    NiMatrix3 rotate;
    float scale;
    if (!V(entity, slot::GetPoint3, &PROP_TRANSLATION, &translate, 0u)) {
        allFound = false;
        ReportMissing(errors, entity, &ERR_TRANSLATION_NOT_FOUND, &PROP_TRANSLATION);
    }
    if (!V(entity, slot::GetMatrix3, &PROP_ROTATION, &rotate, 0u)) {
        allFound = false;
        ReportMissing(errors, entity, &ERR_ROTATION_NOT_FOUND, &PROP_ROTATION);
    }
    if (!V(entity, slot::GetFloat, &PROP_SCALE, &scale, 0u)) {
        allFound = false;
        ReportMissing(errors, entity, &ERR_SCALE_NOT_FOUND, &PROP_SCALE);
    }
    if (!allFound) {
        return;
    }
    bool moved = false;
    NiLight* light = self->light;
    if (!(translate.x == light->local.translate.x) || !(translate.y == light->local.translate.y) ||
        !(translate.z == light->local.translate.z)) {
        light->local.translate = translate;
        moved = true;
    }
    if (!Matrix3_Equal(&self->light->local.rotate, &rotate)) {
        self->light->local.rotate = rotate;
        moved = true;
    }
    light = self->light;
    if (!(scale == light->local.scale)) {
        uint32_t bits;   // (fabs: the sign bit cleared)
        std::memcpy(&bits, &scale, 4);
        bits &= 0x7FFFFFFF;
        std::memcpy(&light->local.scale, &bits, 4);
    } else if (!moved) {
        return;
    }
    AVObject_Update(self->light, time, 1);
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NILIGHTCOMP_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace lc = ni::ec;

// (folded: the property interface's two-argument setters' default; BullyNiDX9Renderer's
// slots 33 and 35)
NILIGHTCOMP_REVERSED("NiEntityPropertyInterface::ReturnFalse2_84D060", 0x0084D060, &lc::ReturnFalse2);
NILIGHTCOMP_REVERSED("NiLightComponent::GetTemplateID", 0x0084D070, &lc::GetTemplateID_);
// (folded: every component's slot 1)
NILIGHTCOMP_REVERSED("NiEntityComponentInterface::RemoveReference", 0x0084D120, &lc::RemoveReference_);
NILIGHTCOMP_REVERSED("NiLightComponent::IsPropertyReadOnly", 0x0084D140, &lc::IsPropertyReadOnly);
NILIGHTCOMP_REVERSED("NiLightComponent::IsPropertyUnique", 0x0084D1E0, &lc::IsPropertyUnique);
NILIGHTCOMP_REVERSED("NiLightComponent::IsPropertySerializable", 0x0084D270, &lc::IsPropertySerializable);
// (folded: MakePropertyUnique, IsPropertyInheritable)
NILIGHTCOMP_REVERSED("NiLightComponent::CanResetProperty", 0x0084D350, &lc::PropertyFlagFalse);
NILIGHTCOMP_REVERSED("NiLightComponent::IsExternalAssetPath", 0x0084D3E0, &lc::IsExternalAssetPath);
NILIGHTCOMP_REVERSED("NiLightComponent::GetElementCount", 0x0084D470, &lc::GetElementCount);
NILIGHTCOMP_REVERSED("NiLightComponent::IsCollection", 0x0084D510, &lc::IsCollection_);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPropertyData(float)", 0x0084D5B0, &lc::GetFloat);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPropertyData(NiColor)", 0x0084D660, &lc::GetColor);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPropertyData(NiPoint3)", 0x0084D6E0, &lc::GetDirection);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPropertyData(NiObject*)", 0x0084D740, &lc::GetObject_);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPropertyData(NiEntityInterface*)", 0x0084D770, &lc::GetEntity);
NILIGHTCOMP_REVERSED("NiLightComponent::SetAmbientColor", 0x0084D7B0, &lc::SetAmbientColor_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetDiffuseColor", 0x0084D800, &lc::SetDiffuseColor_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetSpecularColor", 0x0084D850, &lc::SetSpecularColor_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetLinearAttenuation", 0x0084D8A0, &lc::SetLinearAttenuation_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetQuadraticAttenuation", 0x0084D8D0, &lc::SetQuadraticAttenuation_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetOuterSpotAngle", 0x0084D900, &lc::SetOuterSpotAngle_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetInnerSpotAngle", 0x0084D930, &lc::SetInnerSpotAngle_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetSpotExponent", 0x0084D960, &lc::SetSpotExponent_);
NILIGHTCOMP_REVERSED("NiLightComponent::_SDMInit", 0x0084D990, &lc::SDMInit);
NILIGHTCOMP_REVERSED("NiLightComponent::_SDMShutdown", 0x0084DC00, &lc::SDMShutdown);
NILIGHTCOMP_REVERSED("NiLightComponent::GetClassName", 0x0084DDF0, &lc::GetClassName_);
NILIGHTCOMP_REVERSED("NiLightComponent::GetName", 0x0084DE20, &lc::GetName_);
NILIGHTCOMP_REVERSED("NiLightComponent::Update", 0x0084DE50, &lc::Update);
NILIGHTCOMP_REVERSED("NiLightComponent::GetDisplayName", 0x0084E530, &lc::GetDisplayName);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPrimitiveType", 0x0084E5D0, &lc::GetType<false>);
NILIGHTCOMP_REVERSED("NiLightComponent::GetSemanticType", 0x0084E6C0, &lc::GetType<true>);
NILIGHTCOMP_REVERSED("NiLightComponent::GetDescription", 0x0084E7B0, &lc::GetDescription);
NILIGHTCOMP_REVERSED("NiLightComponent::GetCategory", 0x0084E940, &lc::GetCategory);
NILIGHTCOMP_REVERSED("NiLightComponent::SetElementCount", 0x0084E9D0, &lc::SetElementCount);
NILIGHTCOMP_REVERSED("NiLightComponent::SetPropertyData(float)", 0x0084EAD0, &lc::SetFloat);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPropertyData(NiFixedString)", 0x0084EC10, &lc::GetString);
NILIGHTCOMP_REVERSED("NiLightComponent::SetPropertyData(NiColor)", 0x0084EC40, &lc::SetColor);
NILIGHTCOMP_REVERSED("NiLightComponent::SetPropertyData(NiEntityInterface*)", 0x0084ECA0, &lc::SetEntity);
NILIGHTCOMP_REVERSED("NiLightComponent::NiLightComponent", 0x0084EDA0, &lc::ComponentConstruct);
NILIGHTCOMP_REVERSED("NiLightComponent::ReleaseLight", 0x0084EE90, &lc::ReleaseLight_);
NILIGHTCOMP_REVERSED("NiLightComponent::Clone", 0x0084EF80, &lc::Clone_);
NILIGHTCOMP_REVERSED("NiLightComponent::SetPropertyData(NiFixedString)", 0x0084F070, &lc::SetString);
NILIGHTCOMP_REVERSED("NiLightComponent::GetDependentPropertyNames", 0x0084F0D0, &lc::GetDependentPropertyNames);
NILIGHTCOMP_REVERSED("NiLightComponent::GetPropertyNames", 0x0084F180, &lc::GetPropertyNames_);
