// NiInheritedTransformationComponent (vtables 0x98BBF4 / 0x98BAC8 at +8, 0x54 bytes): a
// transformation taken from a "Source Entity" (or the node at its "Attachment Point Name"):
// "Inherit Translation" / "Inherit Rotation" / "Inherit Scale" say which of its "Translation",
// "Rotation", "Scale" come from there (read-only while inherited) rather than its own values.
// This part of the exe has its first methods (the rest, the constructors and the static names
// are further on: 0x84F9A0.., 0x84FE20, 0x84FBF0). Its interface methods get the interface
// pointer (+8).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "reversed.h"

namespace ni::ec {
namespace {

struct InheritedTransformationComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;             // +0x8
    uint8_t flags;                 // +0xC (bits 0..2: the translation, rotation, scale inherited;
                                   //  all three set by the constructor)
    uint8_t padD[3];
    re::Ptr32<void> sourceEntity;            // +0x10 (NiEntityInterface)
    re::Ptr32<const char> attachmentPoint; // +0x14
    NiPoint3 translate;            // +0x18
    NiMatrix3 rotate;              // +0x24
    float scale;                   // +0x48
    re::Ptr32<NiAVObject> attachedObject;    // +0x4C (the source's node whose world transform is used)
    uint8_t inTranslation;         // +0x50 (reentrancy guards while asking the source)
    uint8_t inRotation;            // +0x51
    uint8_t inScale;               // +0x52
    uint8_t pad53;
};
static_assert(offsetof(InheritedTransformationComponent, sourceEntity) == 0x10);
static_assert(offsetof(InheritedTransformationComponent, translate) == 0x18);
static_assert(offsetof(InheritedTransformationComponent, rotate) == 0x24);
static_assert(offsetof(InheritedTransformationComponent, scale) == 0x48);
static_assert(offsetof(InheritedTransformationComponent, attachedObject) == 0x4C);
static_assert(offsetof(InheritedTransformationComponent, inScale) == 0x52);
static_assert(sizeof(InheritedTransformationComponent) == 0x54);

enum : uint8_t {
    kInheritTranslation = 0x1,
    kInheritRotation = 0x2,
    kInheritScale = 0x4,
};

GAME_VAR(NiFixedString, PROP_INHERIT_TRANSLATION, 0x020E5E40);
GAME_VAR(NiFixedString, PROP_INHERIT_ROTATION, 0x020E5E44);
GAME_VAR(NiFixedString, PROP_INHERIT_SCALE, 0x020E5E48);
GAME_VAR(NiFixedString, PROP_TRANSLATION, 0x020E5E4C);
GAME_VAR(NiFixedString, PROP_ROTATION, 0x020E5E50);
GAME_VAR(NiFixedString, PROP_SCALE, 0x020E5E54);
GAME_VAR(NiFixedString, PROP_SOURCE_ENTITY, 0x020E5E58);
GAME_VAR(NiFixedString, PROP_ATTACHMENT_POINT_NAME, 0x020E5E5C);
GAME_VAR(NiUniqueID, ms_kTemplateID, 0x020E5E20);
GAME_VAR(uint32_t, ms_kTemplateIDGuard, 0x020E5E30);

inline InheritedTransformationComponent* Self(void* iface) {
    return FromIface<InheritedTransformationComponent>(iface);
}

// its 8 properties (0x20E5E40..0x20E5E5C)
inline bool IsProperty(const re::Ptr32<const char>* name) {
    NiFixedString n = *name;
    return n == PROP_INHERIT_TRANSLATION || n == PROP_INHERIT_ROTATION || n == PROP_INHERIT_SCALE ||
           n == PROP_TRANSLATION || n == PROP_ROTATION || n == PROP_SCALE || n == PROP_SOURCE_ENTITY ||
           n == PROP_ATTACHMENT_POINT_NAME;
}

NiUniqueID* __fastcall GetTemplateID_(void*, re::Edx, NiUniqueID* result) {
    return ReturnTemplateID(result, ms_kTemplateIDGuard, ms_kTemplateID, 0x17C5DB11, 0x4490DC9B, 0xCD7DFAA3,
                            0xEE7D5D6A);
}

// (CanResetProperty, MakePropertyUnique, IsPropertyInheritable, IsCollection: false)
uint8_t __fastcall PropertyFlagFalse(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* value) {
    if (!IsProperty(name)) {
        return 0;
    }
    *value = 0;
    return 1;
}

// read-only: a transformation while it's inherited
uint8_t __fastcall IsPropertyReadOnly(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* readOnly) {
    uint8_t flags = Self(iface)->flags;
    NiFixedString n = *name;
    if (n == PROP_TRANSLATION) {
        *readOnly = flags & 1;
        return 1;
    }
    if (n == PROP_ROTATION) {
        *readOnly = (flags >> 1) & 1;
        return 1;
    }
    if (n == PROP_SCALE) {
        *readOnly = (flags >> 2) & 1;
        return 1;
    }
    if (!IsProperty(name)) {
        return 0;
    }
    *readOnly = 0;
    return 1;
}

uint8_t __fastcall IsPropertyUnique(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* unique) {
    if (!IsProperty(name)) {
        return 0;
    }
    *unique = 1;
    return 1;
}

// (IsExternalAssetPath, SetElementCount: false through the third argument)
uint8_t __fastcall PropertyFlagFalse3(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* value) {
    if (!IsProperty(name)) {
        return 0;
    }
    *value = 0;
    return 1;
}

uint8_t __fastcall GetElementCount(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    if (!IsProperty(name)) {
        return 0;
    }
    *count = 1;
    return 1;
}

// the scale: inherited (unless asked again from within) from the attached node's world scale,
// else from the source entity, else its own
uint8_t __fastcall GetScale(void* iface, re::Edx, const re::Ptr32<const char>* name, float* value, uint32_t index) {
    if (index != 0 || *name != PROP_SCALE) {
        return 0;
    }
    InheritedTransformationComponent* self = Self(iface);
    if ((self->flags & kInheritScale) == 0 || self->inScale != 0) {
        *value = self->scale;
        return 1;
    }
    self->inScale = 1;
    if (self->attachedObject != nullptr) {
        *value = self->attachedObject->world.scale;
        self->inScale = 0;
        return 1;
    }
    if (self->sourceEntity == nullptr || !V(self->sourceEntity, slot::GetFloat, name, value, 0u)) {
        *value = self->scale;
    }
    self->inScale = 0;
    return 1;
}

// its own scale (not while inherited)
uint8_t __fastcall SetScale(void* iface, re::Edx, const re::Ptr32<const char>* name, float value, uint32_t index) {
    InheritedTransformationComponent* self = Self(iface);
    if (index == 0 && *name == PROP_SCALE && (self->flags & kInheritScale) == 0) {
        self->scale = value;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    uint8_t flags = Self(iface)->flags;
    NiFixedString n = *name;
    if (n == PROP_INHERIT_TRANSLATION) {
        *value = flags & 1;
        return 1;
    }
    if (n == PROP_INHERIT_ROTATION) {
        *value = (flags >> 1) & 1;
        return 1;
    }
    if (n == PROP_INHERIT_SCALE) {
        *value = (flags >> 2) & 1;
        return 1;
    }
    return 0;
}

// inheriting turned on; or off, the inherited value kept as its own first (through its own
// getter)
uint8_t __fastcall SetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, int value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    InheritedTransformationComponent* self = Self(iface);
    NiFixedString n = *name;
    bool on = re::Flag(value);
    if (n == PROP_INHERIT_TRANSLATION) {
        if (on) {
            self->flags |= kInheritTranslation;
        } else {
            V(iface, slot::GetPoint3, &PROP_TRANSLATION, &self->translate, 0u);
            self->flags &= ~kInheritTranslation;
        }
        return 1;
    }
    if (n == PROP_INHERIT_ROTATION) {
        if (on) {
            self->flags |= kInheritRotation;
        } else {
            V(iface, slot::GetMatrix3, &PROP_ROTATION, &self->rotate, 0u);
            self->flags &= ~kInheritRotation;
        }
        return 1;
    }
    if (n == PROP_INHERIT_SCALE) {
        if (on) {
            self->flags |= kInheritScale;
        } else {
            V(iface, slot::GetFloat, &PROP_SCALE, &self->scale, 0u);
            self->flags &= ~kInheritScale;
        }
        return 1;
    }
    return 0;
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NIINHTRANSCOMP_REVERSED(name, address, fn)                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace itc = ni::ec;

NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::GetTemplateID", 0x0084F4D0, &itc::GetTemplateID_);
// (folded: MakePropertyUnique, IsPropertyInheritable, IsCollection)
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::CanResetProperty", 0x0084F580, &itc::PropertyFlagFalse);
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::IsPropertyReadOnly", 0x0084F5E0,
                        &itc::IsPropertyReadOnly);
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::IsPropertyUnique", 0x0084F670, &itc::IsPropertyUnique);
// (folded: SetElementCount)
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::IsExternalAssetPath", 0x0084F6D0,
                        &itc::PropertyFlagFalse3);
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::GetElementCount", 0x0084F730, &itc::GetElementCount);
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::GetPropertyData(float)", 0x0084F790, &itc::GetScale);
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::SetPropertyData(float)", 0x0084F820, &itc::SetScale);
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::GetPropertyData(bool)", 0x0084F850, &itc::GetBool);
NIINHTRANSCOMP_REVERSED("NiInheritedTransformationComponent::SetPropertyData(bool)", 0x0084F8C0, &itc::SetBool);
