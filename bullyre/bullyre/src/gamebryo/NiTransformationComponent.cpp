// NiTransformationComponent (vtables 0x98A194 / 0x98A068 at +8, 0x40 bytes): an entity's
// world-space translation, rotation and scale, as the properties "Translation" (Point3),
// "Rotation" (Matrix3) and "Scale" (Float). All three are its own (unique), never read-only,
// never collections; no master. Its interface methods get the interface pointer (+8). The
// default constructor and the destructor are the other pass's (0x83CB50, 0x83CBD0).
//
// Also NiTArray<T*>::UpdateSize (the light and shadow components') and
// IsPropertySerializable as the general and transformation components share it.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "reversed.h"

namespace ni::ec {
namespace {

struct TransformationComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;    // +0x8
    NiPoint3 translate;   // +0xC
    NiMatrix3 rotate;     // +0x18
    float scale;          // +0x3C
};
static_assert(offsetof(TransformationComponent, translate) == 0xC);
static_assert(offsetof(TransformationComponent, rotate) == 0x18);
static_assert(offsetof(TransformationComponent, scale) == 0x3C);
static_assert(sizeof(TransformationComponent) == 0x40);

constexpr uintptr_t TransformationComponent_vtable = 0x0098A194;
constexpr uintptr_t TransformationComponentIface_vtable = 0x0098A068;

GAME_VAR(NiFixedString, ms_kClassName, 0x020E5C78);
GAME_VAR(NiFixedString, ms_kComponentName, 0x020E5C7C);       // "Transformation"
GAME_VAR(NiFixedString, ms_kTranslationName, 0x020E5C80);
GAME_VAR(NiFixedString, ms_kRotationName, 0x020E5C84);
GAME_VAR(NiFixedString, ms_kScaleName, 0x020E5C88);
GAME_VAR(NiFixedString, ms_kTranslationDescription, 0x020E5C8C);
GAME_VAR(NiFixedString, ms_kRotationDescription, 0x020E5C90);
GAME_VAR(NiFixedString, ms_kScaleDescription, 0x020E5C94);
GAME_VAR(NiUniqueID, ms_kTemplateID, 0x020E5C64);   // (GetTemplateID's static, guarded by 0x20E5C74)
GAME_VAR(uint32_t, ms_kTemplateIDGuard, 0x020E5C74);

GAME_FN(TransformationComponent*, __thiscall, Component_Construct, 0x008494E0, void* self,
        const NiPoint3* translate, const NiMatrix3* rotate, float scale);

inline TransformationComponent* Self(void* iface) {
    return FromIface<TransformationComponent>(iface);
}

inline bool IsOwnProperty(const re::Ptr32<const char>* name) {
    NiFixedString n = *name;
    return n == ms_kTranslationName || n == ms_kRotationName || n == ms_kScaleName;
}

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kClassName);
}

NiUniqueID* __fastcall GetTemplateID_(void*, re::Edx, NiUniqueID* result) {
    return ReturnTemplateID(result, ms_kTemplateIDGuard, ms_kTemplateID, 0x143E7670, 0x4D02832B, 0x9976E1AE,
                            0xF2229364);
}

re::Ptr32<const char>* __fastcall GetName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kComponentName);
}

// (CanResetProperty, IsPropertyReadOnly, IsPropertyInheritable and IsCollection: false for
// its properties)
uint8_t __fastcall PropertyFlagFalse(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* value) {
    if (!IsOwnProperty(name)) {
        return 0;
    }
    *value = 0;
    return 1;
}

// (IsPropertyUnique: true)
uint8_t __fastcall PropertyFlagTrue(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* value) {
    if (!IsOwnProperty(name)) {
        return 0;
    }
    *value = 1;
    return 1;
}

// (GetElementCount: one)
uint8_t __fastcall GetElementCount(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    if (!IsOwnProperty(name)) {
        return 0;
    }
    *count = 1;
    return 1;
}

// (IsExternalAssetPath, SetElementCount: false through the third argument)
uint8_t __fastcall PropertyFlagFalse3(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* value) {
    if (!IsOwnProperty(name)) {
        return 0;
    }
    *value = 0;
    return 1;
}

uint8_t __fastcall GetScale(void* iface, re::Edx, const re::Ptr32<const char>* name, float* value, uint32_t index) {
    if (index == 0 && *name == ms_kScaleName) {
        *value = Self(iface)->scale;
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetScale(void* iface, re::Edx, const re::Ptr32<const char>* name, float value, uint32_t index) {
    if (index == 0 && *name == ms_kScaleName) {
        Self(iface)->scale = value;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetTranslation(void* iface, re::Edx, const re::Ptr32<const char>* name, NiPoint3* value, uint32_t index) {
    if (index == 0 && *name == ms_kTranslationName) {
        *value = Self(iface)->translate;
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetTranslation(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiPoint3* value,
                                  uint32_t index) {
    if (index == 0 && *name == ms_kTranslationName) {
        Self(iface)->translate = *value;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetRotation(void* iface, re::Edx, const re::Ptr32<const char>* name, NiMatrix3* value, uint32_t index) {
    if (index == 0 && *name == ms_kRotationName) {
        *value = Self(iface)->rotate;
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetRotation(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiMatrix3* value,
                               uint32_t index) {
    if (index == 0 && *name == ms_kRotationName) {
        Self(iface)->rotate = *value;
        return 1;
    }
    return 0;
}

TransformationComponent* __fastcall ComponentConstruct(TransformationComponent* self, re::Edx,
                                                       const NiPoint3* translate, const NiMatrix3* rotate,
                                                       float scale) {
    ConstructBases(self);
    self->vtable = reinterpret_cast<void*>(TransformationComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(TransformationComponentIface_vtable);
    self->translate = *translate;
    self->rotate = *rotate;
    self->scale = scale;
    return self;
}

constexpr StaticString kNames[] = {
    {0x020E5C78, 0x008FFE98},   // "NiTransformationComponent"
    {0x020E5C7C, 0x0098AF38},   // "Transformation"
    {0x020E5C80, 0x008FFFBC},   // "Translation"
    {0x020E5C84, 0x008FFF9C},   // "Rotation"
    {0x020E5C88, 0x008FFFA8},   // "Scale"
    {0x020E5C8C, 0x0098AF10},   // "The world-space position of the entity."
    {0x020E5C90, 0x0098AEE4},   // "The world-space orientation of the entity."
    {0x020E5C94, 0x0098AEBC},   // "The world-space scale of the entity."
};

void __cdecl SDMInit() {
    SetStaticStrings(kNames);
}

void __cdecl SDMShutdown() {
    ClearStaticStrings(kNames);
}

// a copy (whatever the argument: no master to inherit from)
void* __fastcall Clone_(void* iface, re::Edx, int) {
    TransformationComponent* self = Self(iface);
    void* memory = NiNew(sizeof(TransformationComponent));
    if (memory == nullptr) {
        return nullptr;
    }
    TransformationComponent* clone = Component_Construct(memory, &self->translate, &self->rotate, self->scale);
    return IfaceOf(clone);
}

uint8_t __fastcall GetDisplayName(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    NiFixedString n = *name;
    if (n == ms_kTranslationName) {
        FixedString_Assign(value, &ms_kTranslationName);
        return 1;
    }
    if (n == ms_kRotationName) {
        FixedString_Assign(value, &ms_kRotationName);
        return 1;
    }
    if (n == ms_kScaleName) {
        FixedString_Assign(value, &ms_kScaleName);
        return 1;
    }
    return 0;
}

// (GetPrimitiveType and GetSemanticType alike)
uint8_t __fastcall GetPrimitiveType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    NiFixedString n = *name;
    if (n == ms_kTranslationName) {
        FixedString_Assign(value, &PT_POINT3);
        return 1;
    }
    if (n == ms_kRotationName) {
        FixedString_Assign(value, &PT_MATRIX3);
        return 1;
    }
    if (n == ms_kScaleName) {
        FixedString_Assign(value, &PT_FLOAT);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetDescription(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    NiFixedString n = *name;
    if (n == ms_kTranslationName) {
        FixedString_Assign(value, &ms_kTranslationDescription);
        return 1;
    }
    if (n == ms_kRotationName) {
        FixedString_Assign(value, &ms_kRotationDescription);
        return 1;
    }
    if (n == ms_kScaleName) {
        FixedString_Assign(value, &ms_kScaleDescription);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetCategory(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    if (!IsOwnProperty(name)) {
        return 0;
    }
    FixedString_Assign(category, &ms_kComponentName);
    return 1;
}

void __fastcall GetPropertyNames_(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &ms_kTranslationName);
    StringSetAdd(names, &ms_kRotationName);
    StringSetAdd(names, &ms_kScaleName);
}

// NiTArray<T*>::UpdateSize: the trailing empty slots dropped from the count
void __fastcall ArrayUpdateSize(NiTArray<void*>* self, re::Edx) {
    while (self->size != 0) {
        if (self->base[self->size - 1] != nullptr) {
            return;
        }
        self->size--;
    }
}

// serializable: unique and not read-only. The read-only byte starts as the high byte of the
// word the function pushed (its `this`), what the game has there when the call doesn't write it
uint8_t __fastcall IsPropertySerializable(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* serializable) {
    uint8_t unique = uint8_t(reinterpret_cast<uintptr_t>(name));
    uint8_t result = V(iface, slot::IsPropertyUnique, name, &unique);
    if (result) {
        uint8_t readOnly = uint8_t(reinterpret_cast<uintptr_t>(iface) >> 24);
        result = V(iface, slot::IsPropertyReadOnly, name, &readOnly);
        *serializable = (unique != 0 && readOnly == 0) ? 1 : 0;
    }
    return result;
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NITRANSCOMP_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace tc = ni::ec;

NITRANSCOMP_REVERSED("NiTransformationComponent::GetTemplateID", 0x00849200, &tc::GetTemplateID_);
// (folded: IsPropertyReadOnly, IsPropertyInheritable, IsCollection)
NITRANSCOMP_REVERSED("NiTransformationComponent::CanResetProperty", 0x008492B0, &tc::PropertyFlagFalse);
NITRANSCOMP_REVERSED("NiTransformationComponent::IsPropertyUnique", 0x008492E0, &tc::PropertyFlagTrue);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetElementCount", 0x00849310, &tc::GetElementCount);
// (folded: SetElementCount)
NITRANSCOMP_REVERSED("NiTransformationComponent::IsExternalAssetPath", 0x00849350, &tc::PropertyFlagFalse3);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetPropertyData(float)", 0x00849380, &tc::GetScale);
NITRANSCOMP_REVERSED("NiTransformationComponent::SetPropertyData(float)", 0x008493B0, &tc::SetScale);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetPropertyData(NiPoint3)", 0x008493E0, &tc::GetTranslation);
NITRANSCOMP_REVERSED("NiTransformationComponent::SetPropertyData(NiPoint3)", 0x00849420, &tc::SetTranslation);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetPropertyData(NiMatrix3)", 0x00849460, &tc::GetRotation);
NITRANSCOMP_REVERSED("NiTransformationComponent::SetPropertyData(NiMatrix3)", 0x008494A0, &tc::SetRotation);
NITRANSCOMP_REVERSED("NiTransformationComponent::NiTransformationComponent", 0x008494E0, &tc::ComponentConstruct);
NITRANSCOMP_REVERSED("NiTransformationComponent::_SDMInit", 0x00849550, &tc::SDMInit);
NITRANSCOMP_REVERSED("NiTransformationComponent::_SDMShutdown", 0x008495D0, &tc::SDMShutdown);
NITRANSCOMP_REVERSED("NiTransformationComponent::Clone", 0x00849640, &tc::Clone_);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetClassName", 0x00849680, &tc::GetClassName_);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetName", 0x008496B0, &tc::GetName_);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetDisplayName", 0x008496E0, &tc::GetDisplayName);
// (folded: GetSemanticType)
NITRANSCOMP_REVERSED("NiTransformationComponent::GetPrimitiveType", 0x00849740, &tc::GetPrimitiveType);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetDescription", 0x008497A0, &tc::GetDescription);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetCategory", 0x00849800, &tc::GetCategory);
NITRANSCOMP_REVERSED("NiTransformationComponent::GetPropertyNames", 0x00849840, &tc::GetPropertyNames_);
NITRANSCOMP_REVERSED("NiTArray<NiEntityInterface*>::UpdateSize", 0x008498F0, &tc::ArrayUpdateSize);
// (folded: the general and inherited transformation components')
NITRANSCOMP_REVERSED("NiTransformationComponent::IsPropertySerializable", 0x0084F470, &tc::IsPropertySerializable);
