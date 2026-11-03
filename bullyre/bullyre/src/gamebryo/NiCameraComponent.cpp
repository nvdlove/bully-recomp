// NiCameraComponent (vtables 0x98BDA4 / 0x98BC78 at +8, 0x20 bytes): an entity's camera, an
// NiCamera made by the default constructor (its "Scene Root Pointer"), kept at the entity's
// transformation by Update. "Field of View" (degrees, perspective) or "Orthographic Frustum
// Width" (orthographic) with the "Aspect Ratio" make the frustum's sides; "Near / Far Clipping
// Plane" and "Orthographic" are the camera's own frustum's. Its interface methods get the
// interface pointer (+8). No master, no inherited values.
//
// Also the folded MakePropertyUnique (NiTransformationComponent's slot 14 too).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Crt.h"
#include "gamebryo/NiCamera.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::ec {
namespace {

struct CameraComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;     // +0x8
    uint32_t unknownC;     // +0xC (never written)
    re::Ptr32<NiCamera> camera;      // +0x10 (a reference)
    float fieldOfView;     // +0x14 (degrees: 90)
    float aspectRatio;     // +0x18 (4 / 3)
    float orthoWidth;      // +0x1C (100)
};
static_assert(offsetof(CameraComponent, camera) == 0x10);
static_assert(offsetof(CameraComponent, fieldOfView) == 0x14);
static_assert(offsetof(CameraComponent, orthoWidth) == 0x1C);
static_assert(sizeof(CameraComponent) == 0x20);

constexpr uintptr_t CamComponent_vtable = 0x0098BDA4;
constexpr uintptr_t CamComponentIface_vtable = 0x0098BC78;
constexpr uintptr_t PropertyInterface_vtable = 0x00989CE8;

GAME_VAR(NiFixedString, PROP_FIELD_OF_VIEW, 0x020E5E98);
GAME_VAR(NiFixedString, PROP_NEAR_CLIPPING_PLANE, 0x020E5E9C);
GAME_VAR(NiFixedString, PROP_FAR_CLIPPING_PLANE, 0x020E5EA0);
GAME_VAR(NiFixedString, PROP_ORTHOGRAPHIC, 0x020E5EA4);
GAME_VAR(NiFixedString, PROP_ASPECT_RATIO, 0x020E5EA8);
GAME_VAR(NiFixedString, PROP_ORTHO_WIDTH, 0x020E5EAC);
GAME_VAR(NiFixedString, PROP_SCENE_ROOT_POINTER, 0x020E5EB0);
GAME_VAR(NiFixedString, ms_kClassName, 0x020E5EB4);       // "NiCameraComponent"
GAME_VAR(NiFixedString, ms_kComponentName, 0x020E5EB8);   // "Camera"
GAME_VAR(NiFixedString, PROP_TRANSLATION, 0x020E5EBC);
GAME_VAR(NiFixedString, PROP_ROTATION, 0x020E5EC0);
GAME_VAR(NiFixedString, PROP_SCALE, 0x020E5EC4);
GAME_VAR(NiUniqueID, ms_kTemplateID, 0x020E5E84);
GAME_VAR(uint32_t, ms_kTemplateIDGuard, 0x020E5E94);

GAME_VAR(float, kDefaultFieldOfView, 0x00912FE4);   // 90
GAME_VAR(float, kDefaultAspectRatio, 0x0098BC74);   // 1.3333334
GAME_VAR(float, kDefaultOrthoWidth, 0x00900490);    // 100
GAME_VAR(float, kDefaultFar, 0x008FF34C);           // 10000
GAME_VAR(double, kHalf, 0x008FF1F8);                // 0.5
GAME_VAR(float, kTwoPi, 0x00BC9348);
GAME_VAR(double, k360, 0x00918050);

GAME_FN(void, __thiscall, Frustum_Construct, 0x00771890, NiFrustum* self, int ortho);
GAME_FN(void, __thiscall, Camera_SetViewFrustum, 0x00762D60, NiCamera* self, const NiFrustum* frustum);
GAME_FN(NiCamera*, __thiscall, Camera_Construct, 0x00763C20, void* self);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
GAME_FN(uint8_t, __thiscall, Matrix3_Equal, 0x007686A0, const NiMatrix3* self, const NiMatrix3* other);
GAME_FN(void, __thiscall, CloningProcess_Construct, 0x006C7000, void* self, uint32_t hashSize);
GAME_FN(NiObject*, __thiscall, Object_CloneWith, 0x007551F0, void* self, void* cloning);
GAME_FN(void, __cdecl, NiDelete_, 0x00752540, void* p, uint32_t size);

// this file's, through their addresses
GAME_FN(void, __thiscall, UpdateFrustum_, 0x00850710, CameraComponent* self);
GAME_FN(CameraComponent*, __thiscall, ConstructWith_, 0x00851010, void* self, NiCamera* camera, float fieldOfView,
        float aspectRatio, float orthoWidth);
GAME_FN(void*, __thiscall, DeletingDestructor_, 0x00850FB0, CameraComponent* self, int flags);

inline CameraComponent* Self(void* iface) {
    return FromIface<CameraComponent>(iface);
}

// its 7 properties, in its order
inline bool IsProperty(NiFixedString n) {
    return n == PROP_FIELD_OF_VIEW || n == PROP_ASPECT_RATIO || n == PROP_ORTHO_WIDTH ||
           n == PROP_NEAR_CLIPPING_PLANE || n == PROP_FAR_CLIPPING_PLANE || n == PROP_ORTHOGRAPHIC ||
           n == PROP_SCENE_ROOT_POINTER;
}

inline re::Ptr32<const char>* ReturnStatic(re::Ptr32<const char>* result, const re::Ptr32<const char>& string) {
    NiFixedString s = string;
    if (s != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(s) - 8);
        s = string;
    }
    *result = s;
    return result;
}

inline void ConstructBases_(CameraComponent* self) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    ni::sys::Increment(const_cast<long*>(&RefObject_ms_uiObjects));
    self->ifaceVtable = reinterpret_cast<void*>(ComponentInterface_vtable);
}

// NiPointer's release through the import (DeleteThis on the last)
inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && ni::sys::Decrement(&object->refCount) == 0) {
        re::PtrCast<void(__thiscall*)(NiRefObject*)>(VtableOf(object)[1])(object);
    }
}

// ---- the static names

constexpr StaticString kNames[] = {
    {0x020E5E98, 0x0098BC64},   // "Field of View"
    {0x020E5EA8, 0x0098BC54},   // "Aspect Ratio"
    {0x020E5EAC, 0x0098BC38},   // "Orthographic Frustum Width"
    {0x020E5E9C, 0x0098BC24},   // "Near Clipping Plane"
    {0x020E5EA0, 0x0098BC10},   // "Far Clipping Plane"
    {0x020E5EA4, 0x0098BC00},   // "Orthographic"
    {0x020E5EB0, 0x0098AF48},   // "Scene Root Pointer"
    {0x020E5EB4, 0x008FFE64},   // "NiCameraComponent"
    {0x020E5EB8, 0x0093231C},   // "Camera"
    {0x020E5EBC, 0x008FFFBC},   // "Translation"
    {0x020E5EC0, 0x008FFF9C},   // "Rotation"
    {0x020E5EC4, 0x008FFFA8},   // "Scale"
};

void __cdecl SDMInit() {
    SetStaticStrings(kNames);
}

void __cdecl SDMShutdown() {
    ClearStaticStrings(kNames);
}

// ---- the folded MakePropertyUnique: resettable ones are already unique (none are)

uint8_t __fastcall MakePropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* madeUnique) {
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));   // (the game's: the name's slot)
    uint8_t found = V(iface, slot::CanResetProperty, name, &canReset);
    if (!found) {
        return found;
    }
    *madeUnique = 0;
    return 1;
}

// the camera's frustum sides from the field of view (perspective: tan of half of it, in
// radians) or half the width (orthographic), the top from the aspect ratio
void __fastcall UpdateFrustum(CameraComponent* self, re::Edx) {
    NiFrustum frustum;
    Frustum_Construct(&frustum, 0);
    NiCamera* camera = self->camera;
    frustum = camera->frustum;
    float right;
    if (camera->frustum.ortho != 0) {
        right = float(self->orthoWidth * kHalf);
    } else {
        float angle = float(self->fieldOfView * kHalf);
        angle = angle * kTwoPi;
        angle = float(angle / k360);
        right = crt::Tanf(angle);
    }
    frustum.right = right;
    frustum.left = -right;
    float top = right / self->aspectRatio;
    frustum.top = top;
    frustum.bottom = -top;
    Camera_SetViewFrustum(camera, &frustum);
}

NiUniqueID* __fastcall GetTemplateID_(void*, re::Edx, NiUniqueID* result) {
    return ReturnTemplateID(result, ms_kTemplateIDGuard, ms_kTemplateID, 0xB642ABFD, 0x4CEA4B60, 0x7E6C068D,
                            0xFF990FB0);
}

// read-only: the camera; the field of view while orthographic, the width while perspective
uint8_t __fastcall IsPropertyReadOnly(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* readOnly) {
    NiFixedString n = *name;
    if (n == PROP_ASPECT_RATIO || n == PROP_NEAR_CLIPPING_PLANE || n == PROP_FAR_CLIPPING_PLANE ||
        n == PROP_ORTHOGRAPHIC) {
        *readOnly = 0;
        return 1;
    }
    if (n == PROP_SCENE_ROOT_POINTER) {
        *readOnly = 1;
        return 1;
    }
    if (Self(iface)->camera->frustum.ortho != 0) {
        if (n == PROP_ORTHO_WIDTH) {
            *readOnly = 0;
            return 1;
        }
        if (n == PROP_FIELD_OF_VIEW) {
            *readOnly = 1;
            return 1;
        }
    } else {
        if (n == PROP_ORTHO_WIDTH) {
            *readOnly = 1;
            return 1;
        }
        if (n == PROP_FIELD_OF_VIEW) {
            *readOnly = 0;
            return 1;
        }
    }
    return 0;
}

uint8_t __fastcall IsPropertyUnique(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* unique) {
    if (!IsProperty(*name)) {
        return 0;
    }
    *unique = 1;
    return 1;
}

// serializable: unique and not read-only (the camera never). The read-only byte starts as the
// high byte of the pushed `this`, the unique one as the name's slot
uint8_t __fastcall IsPropertySerializable(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* serializable) {
    NiFixedString n = *name;
    if (n == PROP_FIELD_OF_VIEW || n == PROP_ASPECT_RATIO || n == PROP_ORTHO_WIDTH ||
        n == PROP_NEAR_CLIPPING_PLANE || n == PROP_FAR_CLIPPING_PLANE || n == PROP_ORTHOGRAPHIC) {
        uint8_t unique = uint8_t(reinterpret_cast<uintptr_t>(name));
        V(iface, slot::IsPropertyUnique, name, &unique);
        uint8_t readOnly = uint8_t(reinterpret_cast<uintptr_t>(iface) >> 24);
        V(iface, slot::IsPropertyReadOnly, name, &readOnly);
        *serializable = (unique != 0 && readOnly == 0) ? 1 : 0;
        return 1;
    }
    if (n == PROP_SCENE_ROOT_POINTER) {
        *serializable = 0;
        return 1;
    }
    return 0;
}

// (folded: SetElementCount)
uint8_t __fastcall IsExternalAssetPath(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* value) {
    if (!IsProperty(*name)) {
        return 0;
    }
    *value = 0;
    return 1;
}

uint8_t __fastcall GetElementCount(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    if (!IsProperty(*name)) {
        return 0;
    }
    *count = 1;
    return 1;
}

// (folded: IsPropertyInheritable, IsCollection)
uint8_t __fastcall CanResetProperty(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* value) {
    if (!IsProperty(*name)) {
        return 0;
    }
    *value = 0;
    return 1;
}

uint8_t __fastcall GetFloat(void* iface, re::Edx, const re::Ptr32<const char>* name, float* value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    CameraComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_FIELD_OF_VIEW) {
        *value = self->fieldOfView;
    } else if (n == PROP_ASPECT_RATIO) {
        *value = self->aspectRatio;
    } else if (n == PROP_ORTHO_WIDTH) {
        *value = self->orthoWidth;
    } else if (n == PROP_NEAR_CLIPPING_PLANE) {
        *value = self->camera->frustum.nearDist;
    } else if (n == PROP_FAR_CLIPPING_PLANE) {
        *value = self->camera->frustum.farDist;
    } else {
        return 0;
    }
    return 1;
}

// the three that make the sides update the frustum; the planes go to the camera's directly
uint8_t __fastcall SetFloat(void* iface, re::Edx, const re::Ptr32<const char>* name, float value, uint32_t index) {
    if (index != 0) {
        return 0;
    }
    CameraComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_FIELD_OF_VIEW) {
        self->fieldOfView = value;
        UpdateFrustum_(self);
        return 1;
    }
    if (n == PROP_ASPECT_RATIO) {
        self->aspectRatio = value;
        UpdateFrustum_(self);
        return 1;
    }
    if (n == PROP_ORTHO_WIDTH) {
        self->orthoWidth = value;
        UpdateFrustum_(self);
        return 1;
    }
    if (n == PROP_NEAR_CLIPPING_PLANE) {
        NiCamera* camera = self->camera;
        NiFrustum frustum = camera->frustum;
        frustum.nearDist = value;
        Camera_SetViewFrustum(camera, &frustum);
        return 1;
    }
    if (n == PROP_FAR_CLIPPING_PLANE) {
        NiCamera* camera = self->camera;
        NiFrustum frustum = camera->frustum;
        frustum.farDist = value;
        Camera_SetViewFrustum(camera, &frustum);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* value, uint32_t index) {
    if (index == 0 && *name == PROP_ORTHOGRAPHIC) {
        *value = Self(iface)->camera->frustum.ortho;
        return 1;
    }
    return 0;
}

// orthographic or not: the camera's frustum, then its sides again
uint8_t __fastcall SetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, int value, uint32_t index) {
    if (index != 0 || *name != PROP_ORTHOGRAPHIC) {
        return 0;
    }
    CameraComponent* self = Self(iface);
    NiCamera* camera = self->camera;
    NiFrustum frustum = camera->frustum;
    frustum.ortho = uint8_t(value);
    Camera_SetViewFrustum(camera, &frustum);
    UpdateFrustum_(self);
    return 1;
}

// the camera (no reference taken)
uint8_t __fastcall GetObject_(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<NiObject>* value, uint32_t index) {
    if (index == 0 && *name == PROP_SCENE_ROOT_POINTER) {
        *value = Self(iface)->camera;
        return 1;
    }
    return 0;
}

// ---- construction, destruction, cloning

// a new NiCamera, its planes at 1 and 10000 (its sides the camera's defaults until a property
// changes)
CameraComponent* __fastcall Construct(CameraComponent* self, re::Edx) {
    ConstructBases_(self);
    self->vtable = reinterpret_cast<void*>(CamComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(CamComponentIface_vtable);
    self->camera = nullptr;
    self->fieldOfView = kDefaultFieldOfView;
    self->aspectRatio = kDefaultAspectRatio;
    self->orthoWidth = kDefaultOrthoWidth;
    void* memory = NiNew(sizeof(NiCamera));
    NiCamera* camera = memory != nullptr ? Camera_Construct(memory) : nullptr;
    NiCamera* old = self->camera;
    if (old != camera) {
        ReleaseRef(old);   // (the old one let go first, then the new one held)
        self->camera = camera;
        if (camera != nullptr) {
            ni::sys::Increment(&camera->refCount);
        }
    }
    NiFrustum frustum = self->camera->frustum;
    frustum.nearDist = 1.0f;
    frustum.farDist = kDefaultFar;
    Camera_SetViewFrustum(self->camera, &frustum);
    AVObject_Update(self->camera, 0.0f, 1);
    return self;
}

// (Clone's: the camera given, the values)
CameraComponent* __fastcall ConstructWith(CameraComponent* self, re::Edx, NiCamera* camera, float fieldOfView,
                                          float aspectRatio, float orthoWidth) {
    ConstructBases_(self);
    self->vtable = reinterpret_cast<void*>(CamComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(CamComponentIface_vtable);
    self->camera = camera;
    if (camera != nullptr) {
        ni::sys::Increment(&camera->refCount);
    }
    self->fieldOfView = fieldOfView;
    self->aspectRatio = aspectRatio;
    self->orthoWidth = orthoWidth;
    AVObject_Update(self->camera, 0.0f, 1);
    return self;
}

// the camera let go, the bases' vtables back (the destructor inlined)
CameraComponent* __fastcall DeletingDestructor(CameraComponent* self, re::Edx, int flags) {
    ReleaseRef(self->camera);
    self->ifaceVtable = reinterpret_cast<void*>(PropertyInterface_vtable);
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    ni::sys::Decrement(const_cast<long*>(&RefObject_ms_uiObjects));
    if ((flags & 1) != 0) {
        NiDelete_(self, sizeof(CameraComponent));
    }
    return self;
}

void* __fastcall InterfaceDeletingDestructor(void* iface, re::Edx, int flags) {
    return DeletingDestructor_(Self(iface), flags);
}

// a copy with a clone of the camera (an exact copy: the cloning process's copy type 1); the
// argument not looked at
void* __fastcall Clone(void* iface, re::Edx, int) {
    CameraComponent* self = Self(iface);
    void* cloning[6];
    CloningProcess_Construct(cloning, 0x25);
    cloning[2] = reinterpret_cast<void*>(1);
    void* memory = NiNew(sizeof(CameraComponent));
    void* result = nullptr;
    if (memory != nullptr) {
        float orthoWidth = self->orthoWidth;
        float aspectRatio = self->aspectRatio;
        float fieldOfView = self->fieldOfView;
        NiObject* camera = Object_CloneWith(self->camera, cloning);
        CameraComponent* clone =
            ConstructWith_(memory, static_cast<NiCamera*>(static_cast<NiObjectNET*>(camera)), fieldOfView,
                           aspectRatio, orthoWidth);
        if (clone != nullptr) {
            result = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(clone) + 8);
        }
    }
    // the cloning process's destructor: its two maps deleted
    if (cloning[0] != nullptr) {
        re::PtrCast<void(__thiscall*)(void*, int)>(VtableOf(cloning[0])[0])(cloning[0], 1);
    }
    if (cloning[1] != nullptr) {
        re::PtrCast<void(__thiscall*)(void*, int)>(VtableOf(cloning[1])[0])(cloning[1], 1);
    }
    return result;
}

// ---- the interface's other methods

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnStatic(result, ms_kClassName);
}

re::Ptr32<const char>* __fastcall GetName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnStatic(result, ms_kComponentName);
}

// the camera moved to the entity's transformation (all three found), updated when it changed
// (a scale that differs is taken as its absolute value)
void __fastcall Update(void* iface, re::Edx, void* entity, float time, void*, void*) {
    CameraComponent* self = Self(iface);
    NiPoint3 translate;
    NiMatrix3 rotate;
    float scale;
    bool found = true;
    if (!V(entity, slot::GetPoint3, &PROP_TRANSLATION, &translate, 0u)) {
        found = false;
    }
    if (!V(entity, slot::GetMatrix3, &PROP_ROTATION, &rotate, 0u)) {
        found = false;
    }
    if (!V(entity, slot::GetFloat, &PROP_SCALE, &scale, 0u)) {
        found = false;
    }
    bool changed = false;
    if (!found) {
        return;
    }
    NiCamera* camera = self->camera;
    if (Point3_NotEqual(&camera->local.translate, &translate)) {
        camera->local.translate = translate;
        changed = true;
    }
    if (!Matrix3_Equal(&self->camera->local.rotate, &rotate)) {
        self->camera->local.rotate = rotate;
        changed = true;
    }
    camera = self->camera;
    if (!(scale == camera->local.scale)) {
        uint32_t bits;   // (fabs: the sign bit cleared)
        std::memcpy(&bits, &scale, 4);
        bits &= 0x7FFFFFFF;
        std::memcpy(&camera->local.scale, &bits, 4);
    } else if (!changed) {
        return;
    }
    AVObject_Update(camera, time, 1);
}

// the display name: the property's own (none for the camera). (Folded: GetDescription)
uint8_t __fastcall GetDisplayName(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* displayName) {
    NiFixedString n = *name;
    if (n == PROP_FIELD_OF_VIEW || n == PROP_ASPECT_RATIO || n == PROP_ORTHO_WIDTH ||
        n == PROP_NEAR_CLIPPING_PLANE || n == PROP_FAR_CLIPPING_PLANE || n == PROP_ORTHOGRAPHIC) {
        FixedString_Assign(displayName, name);
        return 1;
    }
    if (n == PROP_SCENE_ROOT_POINTER) {
        FixedString_SetString(displayName, nullptr);
        return 1;
    }
    return 0;
}

// (folded: GetSemanticType)
uint8_t __fastcall GetPrimitiveType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* type) {
    NiFixedString n = *name;
    if (n == PROP_FIELD_OF_VIEW || n == PROP_ASPECT_RATIO || n == PROP_ORTHO_WIDTH ||
        n == PROP_NEAR_CLIPPING_PLANE || n == PROP_FAR_CLIPPING_PLANE) {
        FixedString_Assign(type, &PT_FLOAT);
        return 1;
    }
    if (n == PROP_ORTHOGRAPHIC) {
        FixedString_Assign(type, &PT_BOOL);
        return 1;
    }
    if (n == PROP_SCENE_ROOT_POINTER) {
        FixedString_Assign(type, &PT_NIOBJECTPOINTER);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetCategory(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    if (!IsProperty(*name)) {
        return 0;
    }
    FixedString_Assign(category, &ms_kComponentName);
    return 1;
}

// the transformation it follows
void __fastcall GetDependentPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_TRANSLATION);
    StringSetAdd(names, &PROP_ROTATION);
    StringSetAdd(names, &PROP_SCALE);
}

void __fastcall GetPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_FIELD_OF_VIEW);
    StringSetAdd(names, &PROP_ASPECT_RATIO);
    StringSetAdd(names, &PROP_ORTHO_WIDTH);
    StringSetAdd(names, &PROP_NEAR_CLIPPING_PLANE);
    StringSetAdd(names, &PROP_FAR_CLIPPING_PLANE);
    StringSetAdd(names, &PROP_ORTHOGRAPHIC);
    StringSetAdd(names, &PROP_SCENE_ROOT_POINTER);
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NICAMCOMP_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace cc = ni::ec;

// (folded: NiTransformationComponent's slot 14 too)
NICAMCOMP_REVERSED("NiCameraComponent::MakePropertyUnique", 0x008506E0, &cc::MakePropertyUnique);
NICAMCOMP_REVERSED("NiCameraComponent::UpdateFrustum", 0x00850710, &cc::UpdateFrustum);
NICAMCOMP_REVERSED("NiCameraComponent::GetTemplateID", 0x008507B0, &cc::GetTemplateID_);
NICAMCOMP_REVERSED("NiCameraComponent::IsPropertyReadOnly", 0x00850860, &cc::IsPropertyReadOnly);
NICAMCOMP_REVERSED("NiCameraComponent::IsPropertyUnique", 0x008508F0, &cc::IsPropertyUnique);
NICAMCOMP_REVERSED("NiCameraComponent::IsPropertySerializable", 0x00850940, &cc::IsPropertySerializable);
// (folded: SetElementCount)
NICAMCOMP_REVERSED("NiCameraComponent::IsExternalAssetPath", 0x008509F0, &cc::IsExternalAssetPath);
NICAMCOMP_REVERSED("NiCameraComponent::GetElementCount", 0x00850A40, &cc::GetElementCount);
// (folded: IsPropertyInheritable, IsCollection)
NICAMCOMP_REVERSED("NiCameraComponent::CanResetProperty", 0x00850AA0, &cc::CanResetProperty);
NICAMCOMP_REVERSED("NiCameraComponent::GetPropertyData(float)", 0x00850AF0, &cc::GetFloat);
NICAMCOMP_REVERSED("NiCameraComponent::SetPropertyData(float)", 0x00850B80, &cc::SetFloat);
NICAMCOMP_REVERSED("NiCameraComponent::GetPropertyData(bool)", 0x00850C80, &cc::GetBool);
NICAMCOMP_REVERSED("NiCameraComponent::SetPropertyData(bool)", 0x00850CB0, &cc::SetBool);
NICAMCOMP_REVERSED("NiCameraComponent::GetPropertyData(NiObject*)", 0x00850D20, &cc::GetObject_);
NICAMCOMP_REVERSED("NiCameraComponent::_SDMInit", 0x00850D50, &cc::SDMInit);
NICAMCOMP_REVERSED("NiCameraComponent::_SDMShutdown", 0x00850E10, &cc::SDMShutdown);
NICAMCOMP_REVERSED("NiCameraComponent::NiCameraComponent", 0x00850EB0, &cc::Construct);
NICAMCOMP_REVERSED("NiCameraComponent::InterfaceDeletingDestructor", 0x00850FA0, &cc::InterfaceDeletingDestructor);
NICAMCOMP_REVERSED("NiCameraComponent::DeletingDestructor", 0x00850FB0, &cc::DeletingDestructor);
NICAMCOMP_REVERSED("NiCameraComponent::NiCameraComponent(values)", 0x00851010, &cc::ConstructWith);
NICAMCOMP_REVERSED("NiCameraComponent::GetClassName", 0x00851080, &cc::GetClassName_);
NICAMCOMP_REVERSED("NiCameraComponent::GetName", 0x008510B0, &cc::GetName_);
NICAMCOMP_REVERSED("NiCameraComponent::Update", 0x008510E0, &cc::Update);
// (folded: GetDescription)
NICAMCOMP_REVERSED("NiCameraComponent::GetDisplayName", 0x00851200, &cc::GetDisplayName);
// (folded: GetSemanticType)
NICAMCOMP_REVERSED("NiCameraComponent::GetPrimitiveType", 0x00851270, &cc::GetPrimitiveType);
NICAMCOMP_REVERSED("NiCameraComponent::GetCategory", 0x008512F0, &cc::GetCategory);
NICAMCOMP_REVERSED("NiCameraComponent::Clone", 0x00851350, &cc::Clone);
NICAMCOMP_REVERSED("NiCameraComponent::GetDependentPropertyNames", 0x008513E0, &cc::GetDependentPropertyNames);
NICAMCOMP_REVERSED("NiCameraComponent::GetPropertyNames", 0x00851490, &cc::GetPropertyNames);
