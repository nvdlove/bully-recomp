// NiInheritedTransformationComponent, the rest (the first methods are in
// NiInheritedTransformationComponent.cpp): the translation / rotation getters and setters, the
// "Source Entity" and "Attachment Point Name", Update (the attachment point looked up in the
// source's scene root), the names and types, the constructors, the destructor, Clone and the
// static names. Vtables 0x98BBF4 (the object: 0 the deleting destructor, 1 DeleteThis) and
// 0x98BAC8 (the interface at +8); 0x54 bytes. Its interface methods get the interface pointer.
//
// The source entity is not a reference (nothing counts it); the node found at the attachment
// point (+0x4C) is kept until the source or the name changes. Flag 0x8 (+0xC): the source had
// no NiAVObject scene root, so Update doesn't look again until then.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::ec {
namespace {

struct InheritedTransformationComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;             // +0x8
    uint8_t flags;                 // +0xC (bits 0..2: the translation, rotation, scale inherited;
                                   //  bit 3: the source had no scene root to search)
    uint8_t padD[3];
    re::Ptr32<void> sourceEntity;            // +0x10 (NiEntityInterface; not a reference)
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
static_assert(offsetof(InheritedTransformationComponent, attachmentPoint) == 0x14);
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
    kNoSceneRoot = 0x8,
};

constexpr uintptr_t InhComponent_vtable = 0x0098BBF4;
constexpr uintptr_t InhComponentIface_vtable = 0x0098BAC8;
constexpr uintptr_t PropertyInterface_vtable = 0x00989CE8;   // (what the destructor leaves at +8)
constexpr uintptr_t NiAVObject_RTTI = 0x020E2514;

GAME_VAR(NiFixedString, ERR_ATTACHMENT_POINT_NOT_FOUND, 0x020E5E34);
GAME_VAR(NiFixedString, ms_kClassName, 0x020E5E38);       // "NiInheritedTransformationComponent"
GAME_VAR(NiFixedString, ms_kComponentName, 0x020E5E3C);   // "Inherited Transformation"
GAME_VAR(NiFixedString, PROP_INHERIT_TRANSLATION, 0x020E5E40);
GAME_VAR(NiFixedString, PROP_INHERIT_ROTATION, 0x020E5E44);
GAME_VAR(NiFixedString, PROP_INHERIT_SCALE, 0x020E5E48);
GAME_VAR(NiFixedString, PROP_TRANSLATION, 0x020E5E4C);
GAME_VAR(NiFixedString, PROP_ROTATION, 0x020E5E50);
GAME_VAR(NiFixedString, PROP_SCALE, 0x020E5E54);
GAME_VAR(NiFixedString, PROP_SOURCE_ENTITY, 0x020E5E58);
GAME_VAR(NiFixedString, PROP_ATTACHMENT_POINT_NAME, 0x020E5E5C);
GAME_VAR(NiFixedString, DESC_INHERIT_TRANSLATION, 0x020E5E60);
GAME_VAR(NiFixedString, DESC_INHERIT_ROTATION, 0x020E5E64);
GAME_VAR(NiFixedString, DESC_INHERIT_SCALE, 0x020E5E68);
GAME_VAR(NiFixedString, DESC_TRANSLATION, 0x020E5E6C);
GAME_VAR(NiFixedString, DESC_ROTATION, 0x020E5E70);
GAME_VAR(NiFixedString, DESC_SCALE, 0x020E5E74);
GAME_VAR(NiFixedString, DESC_SOURCE_ENTITY, 0x020E5E78);
GAME_VAR(NiFixedString, DESC_ATTACHMENT_POINT_NAME, 0x020E5E7C);
GAME_VAR(NiFixedString, PROP_SCENE_ROOT_POINTER, 0x020E5E80);   // (asked of the source)

GAME_FN(NiObject*, __cdecl, NiDynamicCast, 0x00401FF0, uintptr_t rtti, void* object);
GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);

// this file's, through their addresses
GAME_FN(void, __thiscall, Component_Destruct, 0x0084FEC0, InheritedTransformationComponent* self);
GAME_FN(InheritedTransformationComponent*, __thiscall, Component_ConstructWith, 0x0084FF20, void* self,
        void* sourceEntity, const re::Ptr32<const char>* attachmentPoint, int inheritTranslation, int inheritRotation,
        int inheritScale, const NiPoint3* translate, const NiMatrix3* rotate, float scale);
GAME_FN(void*, __thiscall, Component_DeletingDestructor, 0x00850500, InheritedTransformationComponent* self,
        int flags);

inline InheritedTransformationComponent* Self(void* iface) {
    return FromIface<InheritedTransformationComponent>(iface);
}

// a static name returned (NiFixedString's copy constructor: the count through the import, the
// handle read again)
inline re::Ptr32<const char>* ReturnStatic(re::Ptr32<const char>* result, const re::Ptr32<const char>& string) {
    NiFixedString s = string;
    if (s != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(s) - 8);
        s = string;
    }
    *result = s;
    return result;
}

// NiRefObject's constructor, then the interface's vtable (the counts through the imports)
inline void ConstructBases_(InheritedTransformationComponent* self) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    ni::sys::Increment(const_cast<long*>(&RefObject_ms_uiObjects));
    self->ifaceVtable = reinterpret_cast<void*>(ComponentInterface_vtable);
}

// ---- the static names (the exe's texts)

constexpr StaticString kNames[] = {
    {0x020E5E34, 0x0098BA70},   // "The specified attachment point was not found; the base scene root..."
    {0x020E5E38, 0x0098A19C},   // "NiInheritedTransformationComponent"
    {0x020E5E3C, 0x0098BA50},   // "Inherited Transformation"
    {0x020E5E40, 0x0098BA3C},   // "Inherit Translation"
    {0x020E5E44, 0x0098BA28},   // "Inherit Rotation"
    {0x020E5E48, 0x0098BA18},   // "Inherit Scale"
    {0x020E5E4C, 0x008FFFBC},   // "Translation"
    {0x020E5E50, 0x008FFF9C},   // "Rotation"
    {0x020E5E54, 0x008FFFA8},   // "Scale"
    {0x020E5E58, 0x0098BA08},   // "Source Entity"
    {0x020E5E5C, 0x0098B9F0},   // "Attachment Point Name"
    {0x020E5E60, 0x0098B9B0},   // "Whether or not to inherit the translation of the source entity."
    {0x020E5E64, 0x0098B96C},   // "Whether or not to inherit the rotation of the source entity."
    {0x020E5E68, 0x0098B930},   // "Whether or not to inherit the scale of the source entity."
    {0x020E5E6C, 0x0098AF10},   // "The world-space position of the entity."
    {0x020E5E70, 0x0098AEE4},   // "The world-space orientation of the entity."
    {0x020E5E74, 0x0098AEBC},   // "The world-space scale of the entity."
    {0x020E5E78, 0x0098B8D8},   // "The entity from which the translation, rotation, and scale ..."
    {0x020E5E7C, 0x0098B888},   // "The attachment point in the source entity whose transforms are inherited."
    {0x020E5E80, 0x0098AF48},   // "Scene Root Pointer"
};

void __cdecl SDMInit() {
    SetStaticStrings(kNames);
}

void __cdecl SDMShutdown() {
    ClearStaticStrings(kNames);
}

// ---- the transformation's getters and setters

// the translation: inherited (unless asked again from within) from the attached node's world
// translation, else from the source entity, else its own
uint8_t __fastcall GetTranslation(void* iface, re::Edx, const re::Ptr32<const char>* name, NiPoint3* value, uint32_t index) {
    if (index != 0 || *name != PROP_TRANSLATION) {
        return 0;
    }
    InheritedTransformationComponent* self = Self(iface);
    if ((self->flags & kInheritTranslation) == 0 || self->inTranslation != 0) {
        *value = self->translate;
        return 1;
    }
    self->inTranslation = 1;
    if (self->attachedObject != nullptr) {
        *value = self->attachedObject->world.translate;
        self->inTranslation = 0;
        return 1;
    }
    if (self->sourceEntity == nullptr || !V(self->sourceEntity, slot::GetPoint3, name, value, 0u)) {
        *value = self->translate;
    }
    self->inTranslation = 0;
    return 1;
}

// its own translation (not while inherited)
uint8_t __fastcall SetTranslation(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiPoint3* value,
                                  uint32_t index) {
    InheritedTransformationComponent* self = Self(iface);
    if (index == 0 && *name == PROP_TRANSLATION && (self->flags & kInheritTranslation) == 0) {
        self->translate = *value;
        return 1;
    }
    return 0;
}

// the rotation, as the translation
uint8_t __fastcall GetRotation(void* iface, re::Edx, const re::Ptr32<const char>* name, NiMatrix3* value, uint32_t index) {
    if (index != 0 || *name != PROP_ROTATION) {
        return 0;
    }
    InheritedTransformationComponent* self = Self(iface);
    if ((self->flags & kInheritRotation) == 0 || self->inRotation != 0) {
        *value = self->rotate;
        return 1;
    }
    self->inRotation = 1;
    if (self->attachedObject != nullptr) {
        *value = self->attachedObject->world.rotate;
        self->inRotation = 0;
        return 1;
    }
    if (self->sourceEntity == nullptr || !V(self->sourceEntity, slot::GetMatrix3, name, value, 0u)) {
        *value = self->rotate;
    }
    self->inRotation = 0;
    return 1;
}

uint8_t __fastcall SetRotation(void* iface, re::Edx, const re::Ptr32<const char>* name, const NiMatrix3* value,
                               uint32_t index) {
    InheritedTransformationComponent* self = Self(iface);
    if (index == 0 && *name == PROP_ROTATION && (self->flags & kInheritRotation) == 0) {
        self->rotate = *value;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetSourceEntity(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<void>* value, uint32_t index) {
    if (index == 0 && *name == PROP_SOURCE_ENTITY) {
        *value = Self(iface)->sourceEntity;
        return 1;
    }
    return 0;
}

// a new source: the attachment point looked up again
uint8_t __fastcall SetSourceEntity(void* iface, re::Edx, const re::Ptr32<const char>* name, void* value, uint32_t index) {
    if (index == 0 && *name == PROP_SOURCE_ENTITY) {
        InheritedTransformationComponent* self = Self(iface);
        self->sourceEntity = value;
        self->attachedObject = nullptr;
        self->flags &= ~kNoSceneRoot;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetAttachmentPoint(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value,
                                      uint32_t index) {
    if (index == 0 && *name == PROP_ATTACHMENT_POINT_NAME) {
        FixedString_Assign(value, &Self(iface)->attachmentPoint);
        return 1;
    }
    return 0;
}

// a new name: looked up again
uint8_t __fastcall SetAttachmentPoint(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* value,
                                      uint32_t index) {
    if (index == 0 && *name == PROP_ATTACHMENT_POINT_NAME) {
        InheritedTransformationComponent* self = Self(iface);
        FixedString_Assign(&self->attachmentPoint, value);
        self->attachedObject = nullptr;
        self->flags &= ~kNoSceneRoot;
        return 1;
    }
    return 0;
}

// ---- construction and destruction

// the defaults: everything inherited, the identity transformation, no source
InheritedTransformationComponent* __fastcall Construct(InheritedTransformationComponent* self, re::Edx) {
    ConstructBases_(self);
    self->flags = 0;
    self->sourceEntity = nullptr;
    self->vtable = reinterpret_cast<void*>(InhComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(InhComponentIface_vtable);
    self->attachmentPoint = NullFixedString;
    self->translate = Point3_ZERO;
    self->rotate = Matrix3_IDENTITY;
    self->scale = 1.0f;
    self->flags |= kInheritTranslation | kInheritRotation | kInheritScale;
    self->attachedObject = nullptr;
    self->inTranslation = 0;
    self->inRotation = 0;
    self->inScale = 0;
    return self;
}

// (Clone's: the source, the name, which are inherited and its own values)
InheritedTransformationComponent* __fastcall ConstructWith(InheritedTransformationComponent* self, re::Edx,
                                                           void* sourceEntity, const re::Ptr32<const char>* attachmentPoint,
                                                           int inheritTranslation, int inheritRotation,
                                                           int inheritScale, const NiPoint3* translate,
                                                           const NiMatrix3* rotate, float scale) {
    ConstructBases_(self);
    self->vtable = reinterpret_cast<void*>(InhComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(InhComponentIface_vtable);
    self->flags = 0;
    self->sourceEntity = sourceEntity;
    NiFixedString s = *attachmentPoint;
    if (s != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(s) - 8);
    }
    self->attachmentPoint = *attachmentPoint;
    self->translate = *translate;
    self->rotate = *rotate;
    self->scale = scale;
    self->attachedObject = nullptr;
    self->inTranslation = 0;
    self->inRotation = 0;
    self->inScale = 0;
    if (re::Flag(inheritTranslation)) {
        self->flags |= kInheritTranslation;
    } else {
        self->flags &= ~kInheritTranslation;
    }
    if (re::Flag(inheritRotation)) {
        self->flags |= kInheritRotation;
    } else {
        self->flags &= ~kInheritRotation;
    }
    if (re::Flag(inheritScale)) {
        self->flags |= kInheritScale;
    } else {
        self->flags &= ~kInheritScale;
    }
    return self;
}

// the name let go, the bases' vtables back
void __fastcall Destruct(InheritedTransformationComponent* self, re::Edx) {
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->attachmentPoint));
    self->ifaceVtable = reinterpret_cast<void*>(PropertyInterface_vtable);
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    ni::sys::Decrement(const_cast<long*>(&RefObject_ms_uiObjects));
}

InheritedTransformationComponent* __fastcall DeletingDestructor(InheritedTransformationComponent* self, re::Edx,
                                                                int flags) {
    Component_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete_(self, sizeof(InheritedTransformationComponent));
    }
    return self;
}

// the interface's slot 68 (the adjustor thunk)
void* __fastcall InterfaceDeletingDestructor(void* iface, re::Edx, int flags) {
    return Component_DeletingDestructor(Self(iface), flags);
}

// a copy of its settings (the argument not looked at)
void* __fastcall Clone(void* iface, re::Edx, int) {
    InheritedTransformationComponent* self = Self(iface);
    void* memory = NiNew(sizeof(InheritedTransformationComponent));
    if (memory == nullptr) {
        return nullptr;
    }
    uint8_t flags = self->flags;
    InheritedTransformationComponent* clone =
        Component_ConstructWith(memory, self->sourceEntity, &self->attachmentPoint, (flags & 1) != 0 ? 1 : 0,
                                ((flags >> 1) & 1) != 0 ? 1 : 0, ((flags >> 2) & 1) != 0 ? 1 : 0, &self->translate,
                                &self->rotate, self->scale);
    return clone != nullptr ? reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(clone) + 8) : nullptr;
}

// ---- the property interface

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnStatic(result, ms_kClassName);
}

re::Ptr32<const char>* __fastcall GetName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnStatic(result, ms_kComponentName);
}

// the attachment point looked up in the source's scene root (an NiAVObject) by name, once: not
// found, the scene root itself is used (and the error handler told); no scene root, not looked
// for again until the source or the name changes
void __fastcall Update(void* iface, re::Edx, void* entity, float, void* errors, void*) {
    InheritedTransformationComponent* self = Self(iface);
    if (self->sourceEntity == nullptr || self->attachedObject != nullptr) {
        return;
    }
    if (self->attachmentPoint == NullFixedString || (self->flags & kNoSceneRoot) != 0) {
        return;
    }
    void* object = nullptr;   // (the game's slot starts as the pushed ecx: written when found)
    if (!V(self->sourceEntity, slot::GetObject, &PROP_SCENE_ROOT_POINTER, &object, 0u) || object == nullptr) {
        return;
    }
    NiObject* root = NiDynamicCast(NiAVObject_RTTI, object);
    if (root != nullptr) {
        // NiAVObject::GetObjectByName (slot 23)
        NiAVObject* found =
            re::PtrCast<NiAVObject*(__thiscall*)(NiObject*, const re::Ptr32<const char>*)>(VtableOf(root)[23])(
                root, &self->attachmentPoint);
        self->attachedObject = found;
        if (found == nullptr) {
            NiFixedString entityName;
            re::Ptr32<const char>* namePtr = V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
            V<void>(errors, 2, &ERR_ATTACHMENT_POINT_NOT_FOUND, &self->attachmentPoint, namePtr,
                    &PROP_ATTACHMENT_POINT_NAME);
            FixedString_Destruct(&entityName);
            self->attachedObject = static_cast<NiAVObject*>(static_cast<NiObjectNET*>(root));
        }
    }
    if (self->attachedObject == nullptr) {
        self->flags |= kNoSceneRoot;
    }
}

void __fastcall GetPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_INHERIT_TRANSLATION);
    StringSetAdd(names, &PROP_INHERIT_ROTATION);
    StringSetAdd(names, &PROP_INHERIT_SCALE);
    StringSetAdd(names, &PROP_TRANSLATION);
    StringSetAdd(names, &PROP_ROTATION);
    StringSetAdd(names, &PROP_SCALE);
    StringSetAdd(names, &PROP_SOURCE_ENTITY);
    StringSetAdd(names, &PROP_ATTACHMENT_POINT_NAME);
}

// the display name: the property's own
uint8_t __fastcall GetDisplayName(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* displayName) {
    const re::Ptr32<const char>* names[] = {&PROP_INHERIT_TRANSLATION, &PROP_INHERIT_ROTATION, &PROP_INHERIT_SCALE,
                                    &PROP_TRANSLATION,         &PROP_ROTATION,         &PROP_SCALE,
                                    &PROP_SOURCE_ENTITY,       &PROP_ATTACHMENT_POINT_NAME};
    NiFixedString n = *name;
    for (const re::Ptr32<const char>* p : names) {
        if (n == *p) {
            FixedString_Assign(displayName, p);
            return 1;
        }
    }
    return 0;
}

// (folded: GetSemanticType, the same names)
uint8_t __fastcall GetPrimitiveType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* type) {
    NiFixedString n = *name;
    const re::Ptr32<const char>* t;
    if (n == PROP_INHERIT_TRANSLATION || n == PROP_INHERIT_ROTATION || n == PROP_INHERIT_SCALE) {
        t = &PT_BOOL;
    } else if (n == PROP_TRANSLATION) {
        t = &PT_POINT3;
    } else if (n == PROP_ROTATION) {
        t = &PT_MATRIX3;
    } else if (n == PROP_SCALE) {
        t = &PT_FLOAT;
    } else if (n == PROP_SOURCE_ENTITY) {
        t = &PT_ENTITYPOINTER;
    } else if (n == PROP_ATTACHMENT_POINT_NAME) {
        t = &PT_STRING;
    } else {
        return 0;
    }
    FixedString_Assign(type, t);
    return 1;
}

uint8_t __fastcall GetDescription(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* description) {
    const re::Ptr32<const char>* names[] = {&PROP_INHERIT_TRANSLATION, &PROP_INHERIT_ROTATION, &PROP_INHERIT_SCALE,
                                    &PROP_TRANSLATION,         &PROP_ROTATION,         &PROP_SCALE,
                                    &PROP_SOURCE_ENTITY,       &PROP_ATTACHMENT_POINT_NAME};
    const re::Ptr32<const char>* descriptions[] = {&DESC_INHERIT_TRANSLATION, &DESC_INHERIT_ROTATION, &DESC_INHERIT_SCALE,
                                           &DESC_TRANSLATION,         &DESC_ROTATION,         &DESC_SCALE,
                                           &DESC_SOURCE_ENTITY,       &DESC_ATTACHMENT_POINT_NAME};
    NiFixedString n = *name;
    for (int i = 0; i < 8; i++) {
        if (n == *names[i]) {
            FixedString_Assign(description, descriptions[i]);
            return 1;
        }
    }
    return 0;
}

// the category: the component's name
uint8_t __fastcall GetCategory(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    NiFixedString n = *name;
    if (n == PROP_INHERIT_TRANSLATION || n == PROP_INHERIT_ROTATION || n == PROP_INHERIT_SCALE ||
        n == PROP_TRANSLATION || n == PROP_ROTATION || n == PROP_SCALE || n == PROP_SOURCE_ENTITY ||
        n == PROP_ATTACHMENT_POINT_NAME) {
        FixedString_Assign(category, &ms_kComponentName);
        return 1;
    }
    return 0;
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NIINHTRANS2_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace itc2 = ni::ec;

NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetPropertyData(NiPoint3)", 0x0084F9A0,
                     &itc2::GetTranslation);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::SetPropertyData(NiPoint3)", 0x0084FA60,
                     &itc2::SetTranslation);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetPropertyData(NiMatrix3)", 0x0084FAA0,
                     &itc2::GetRotation);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::SetPropertyData(NiMatrix3)", 0x0084FB40,
                     &itc2::SetRotation);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetPropertyData(NiEntityInterface*)", 0x0084FB80,
                     &itc2::GetSourceEntity);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::SetPropertyData(NiEntityInterface*)", 0x0084FBB0,
                     &itc2::SetSourceEntity);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::_SDMInit", 0x0084FBF0, &itc2::SDMInit);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::_SDMShutdown", 0x0084FD20, &itc2::SDMShutdown);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::NiInheritedTransformationComponent", 0x0084FE20,
                     &itc2::Construct);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::InterfaceDeletingDestructor", 0x0084FEB0,
                     &itc2::InterfaceDeletingDestructor);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::~NiInheritedTransformationComponent", 0x0084FEC0,
                     &itc2::Destruct);
// (not in the chunk's list: the reach missed it; Clone's only callee)
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::NiInheritedTransformationComponent(values)", 0x0084FF20,
                     &itc2::ConstructWith);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::Clone", 0x0084FFF0, &itc2::Clone);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetClassName", 0x00850060, &itc2::GetClassName_);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetName", 0x00850090, &itc2::GetName_);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::Update", 0x008500C0, &itc2::Update);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetDisplayName", 0x00850190, &itc2::GetDisplayName);
// (folded: GetSemanticType)
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetPrimitiveType", 0x00850280, &itc2::GetPrimitiveType);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetDescription", 0x00850340, &itc2::GetDescription);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetCategory", 0x00850430, &itc2::GetCategory);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetPropertyData(NiFixedString)", 0x00850490,
                     &itc2::GetAttachmentPoint);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::SetPropertyData(NiFixedString)", 0x008504C0,
                     &itc2::SetAttachmentPoint);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::DeletingDestructor", 0x00850500,
                     &itc2::DeletingDestructor);
NIINHTRANS2_REVERSED("NiInheritedTransformationComponent::GetPropertyNames", 0x00850520, &itc2::GetPropertyNames);
