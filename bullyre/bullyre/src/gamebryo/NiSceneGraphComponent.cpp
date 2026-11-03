// NiSceneGraphComponent (vtables 0x98A05C / 0x989F30 at +8, 0x1C bytes): an entity's scene
// graph, loaded from its "NIF File Path" through the external asset manager (NiParamsNIF) and
// held as its "Scene Root Pointer"; Update moves it to the entity's "Translation", "Rotation"
// and "Scale" and updates it (only the selected parts when nothing moved and it's animated).
// The path is inherited from the master component until it's made unique (flag bit 1). Its
// interface methods get the interface pointer (+8); the helpers the object. The default
// constructor and the destructor are the other pass's (0x83CA60, 0x83CAC0).
//
// Also the interfaces' AddReference (folded: every component's slot 0).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::ec {
namespace {

struct SceneGraphComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;               // +0x8
    uint8_t flags;                   // +0xC (bit 0 the NIF failed to load, 1 the path made
                                     //  unique, 2 animated)
    uint8_t padD[3];
    re::Ptr32<const char> nifFilePath;       // +0x10
    re::Ptr32<NiAVObject> sceneRoot;           // +0x14 (a reference)
    re::Ptr32<SceneGraphComponent> master;     // +0x18 (a reference)
};
static_assert(offsetof(SceneGraphComponent, nifFilePath) == 0x10);
static_assert(offsetof(SceneGraphComponent, sceneRoot) == 0x14);
static_assert(offsetof(SceneGraphComponent, master) == 0x18);
static_assert(sizeof(SceneGraphComponent) == 0x1C);

enum : uint8_t {
    kLoadFailed = 0x1,
    kUniquePath = 0x2,
    kAnimated = 0x4,
};

constexpr uintptr_t SceneGraphComponent_vtable = 0x0098A05C;
constexpr uintptr_t SceneGraphComponentIface_vtable = 0x00989F30;
const char* const kNifFilenameSemantic = reinterpret_cast<const char*>(0x0098B574);   // "NIF Filename"

GAME_VAR(NiFixedString, ERR_TRANSLATION_NOT_FOUND, 0x020E5D34);
GAME_VAR(NiFixedString, ERR_ROTATION_NOT_FOUND, 0x020E5D38);
GAME_VAR(NiFixedString, ERR_SCALE_NOT_FOUND, 0x020E5D3C);
GAME_VAR(NiFixedString, ERR_FILE_LOAD_FAILED, 0x020E5D40);
GAME_VAR(NiFixedString, ms_kClassName, 0x020E5D48);
GAME_VAR(NiFixedString, ms_kComponentName, 0x020E5D4C);   // "Scene Graph"
GAME_VAR(NiFixedString, PROP_NIF_FILE_PATH, 0x020E5D50);
GAME_VAR(NiFixedString, PROP_SCENE_ROOT_POINTER, 0x020E5D54);
GAME_VAR(NiFixedString, DESC_NIF_FILE_PATH, 0x020E5D58);
GAME_VAR(NiFixedString, PROP_TRANSLATION, 0x020E5D5C);
GAME_VAR(NiFixedString, PROP_ROTATION, 0x020E5D60);
GAME_VAR(NiFixedString, PROP_SCALE, 0x020E5D64);
GAME_VAR(NiUniqueID, ms_kTemplateID, 0x020E5D20);
GAME_VAR(uint32_t, ms_kTemplateIDGuard, 0x020E5D30);
GAME_VAR(uint32_t, NiNode_ClassIndex, 0x020E2550);

// NiParamsNIF (12 bytes: the vtable, the asset path, the scene root): the other pass's
struct ParamsNIF {
    re::Ptr32<void> vtable;
    re::Ptr32<const char> path;
    re::Ptr32<NiObject> sceneRoot;
};
GAME_FN(void, __thiscall, ParamsNIF_Construct, 0x00856B40, ParamsNIF* self);
GAME_FN(void, __thiscall, ParamsNIF_Destruct, 0x00856B90, ParamsNIF* self);
GAME_FN(uint8_t, __thiscall, ParamsNIF_SetAssetPath, 0x00856A00, ParamsNIF* self, const re::Ptr32<const char>* path);
GAME_FN(uint8_t, __thiscall, ParamsNIF_GetSceneRoot, 0x00856BF0, ParamsNIF* self, re::Ptr32<NiAVObject>* sceneRoot);

// NiAVObject's, NiPoint3's, NiMatrix3's, NiPointer's
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, NiAVObject* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateSelected, 0x0075DB30, NiAVObject* self, float time);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, NiAVObject* self);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E240, NiAVObject* self);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
GAME_FN(uint8_t, __thiscall, Matrix3_Equal, 0x007686A0, const NiMatrix3* self, const NiMatrix3* other);
GAME_FN(re::Ptr32<NiObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);

// this file's, through their addresses
GAME_FN(void, __thiscall, SetAnimated, 0x0084BF60, SceneGraphComponent* self, int animated);
GAME_FN(const re::Ptr32<const char>*, __thiscall, GetNifFilePath, 0x0084C440, SceneGraphComponent* self);
GAME_FN(void, __thiscall, SetNifFilePath, 0x0084C460, SceneGraphComponent* self, const re::Ptr32<const char>* path);
GAME_FN(uint8_t, __thiscall, HasControllers, 0x0084C640, SceneGraphComponent* self, NiObject* object);
GAME_FN(SceneGraphComponent*, __thiscall, Component_Construct, 0x0084C3E0, void* self, SceneGraphComponent* master);

inline SceneGraphComponent* Self(void* iface) {
    return FromIface<SceneGraphComponent>(iface);
}

// the path as the methods inline GetNifFilePath: its own unless inherited
inline const re::Ptr32<const char>* EffectivePath(SceneGraphComponent* self) {
    return ((self->flags & kUniquePath) == 0 && self->master != nullptr) ? GetNifFilePath(self->master)
                                                                         : &self->nifFilePath;
}

inline bool IsProperty(const re::Ptr32<const char>* name) {
    return *name == PROP_NIF_FILE_PATH || *name == PROP_SCENE_ROOT_POINTER;
}

// ---- the interfaces' AddReference (folded: the count is at +4 of the object, -4 of the interface)

void __fastcall AddReference_(void* iface, re::Edx) {
    _InterlockedIncrement(reinterpret_cast<volatile long*>(reinterpret_cast<uintptr_t>(iface) - 4));
}

// ---- the object's own

void __fastcall SetAnimated_(SceneGraphComponent* self, re::Edx, int animated) {
    if (re::Flag(animated)) {
        self->flags |= kAnimated;
    } else {
        self->flags &= ~kAnimated;
    }
}

const re::Ptr32<const char>* __fastcall GetNifFilePath_(SceneGraphComponent* self, re::Edx) {
    while ((self->flags & kUniquePath) == 0 && self->master != nullptr) {
        self = self->master;
    }
    return &self->nifFilePath;
}

// a new path made its own: the scene root let go (Update loads the new one)
void __fastcall SetNifFilePath_(SceneGraphComponent* self, re::Edx, const re::Ptr32<const char>* path) {
    const re::Ptr32<const char>* current = EffectivePath(self);
    if (*current != *path) {
        FixedString_Assign(&self->nifFilePath, path);
        NiAVObject* root = self->sceneRoot;
        if (root != nullptr) {
            Release(root);
            self->sceneRoot = nullptr;
        }
        self->flags = uint8_t((self->flags & ~kLoadFailed) | kUniquePath);
    }
}

SceneGraphComponent* __fastcall ComponentConstruct(SceneGraphComponent* self, re::Edx, SceneGraphComponent* master) {
    ConstructBases(self);
    self->vtable = reinterpret_cast<void*>(SceneGraphComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(SceneGraphComponentIface_vtable);
    self->flags = 0;
    self->nifFilePath = nullptr;
    self->sceneRoot = nullptr;
    self->master = master;
    AddRef(master);
    return self;
}

// a controller anywhere in the graph (NiNodes' children searched)
uint8_t __fastcall HasControllers_(SceneGraphComponent* self, re::Edx, NiObject* object) {
    if (static_cast<NiObjectNET*>(object)->controller != nullptr) {
        return 1;
    }
    const uint32_t* rtti = re::PtrCast<const uint32_t*(__thiscall*)(NiObject*)>(VtableOf(object)[2])(object);
    if (((1u << (NiNode_ClassIndex & 31)) & rtti[3 + (NiNode_ClassIndex >> 5)]) == 0) {
        return 0;
    }
    NiNode* node = static_cast<NiNode*>(object);
    for (uint32_t i = 0; i < node->children.size; i++) {
        NiAVObject* child = node->children.base[i];
        if (child != nullptr && HasControllers(self, child)) {
            return 1;
        }
    }
    return 0;
}

// ---- the static names (the exe's texts)

constexpr StaticString kNames[] = {
    {0x020E5D34, 0x0098B554},   // "Translation property not found."
    {0x020E5D38, 0x0098B534},   // "Rotation property not found."
    {0x020E5D3C, 0x0098B518},   // "Scale property not found."
    {0x020E5D40, 0x0098ADE4},   // "NIF file load failed."
    {0x020E5D44, 0x0098ADA8},   // "Root object at index 0 in NIF file is not an NiAVObject."
    {0x020E5D48, 0x008FFE80},   // "NiSceneGraphComponent"
    {0x020E5D4C, 0x0098B50C},   // "Scene Graph"
    {0x020E5D50, 0x0098B4FC},   // "NIF File Path"
    {0x020E5D54, 0x0098AF48},   // "Scene Root Pointer"
    {0x020E5D58, 0x0098B4CC},   // "The path to a NIF file to use for the entity."
    {0x020E5D5C, 0x008FFFBC},   // "Translation"
    {0x020E5D60, 0x008FFF9C},   // "Rotation"
    {0x020E5D64, 0x008FFFA8},   // "Scale"
};

void __cdecl SDMInit() {
    SetStaticStrings(kNames);
}

void __cdecl SDMShutdown() {
    ClearStaticStrings(kNames);
}

// ---- the interface's methods

NiUniqueID* __fastcall GetTemplateID_(void*, re::Edx, NiUniqueID* result) {
    return ReturnTemplateID(result, ms_kTemplateIDGuard, ms_kTemplateID, 0x294185A8, 0x47A624FD, 0xC40278A8,
                            0xBDCD7E3E);
}

uint8_t __fastcall IsAnimated_(void* iface, re::Edx) {
    return (Self(iface)->flags >> 2) & 1;
}

// the scene root through the rendering context's culler (its slot 3: Process(camera, scene,
// visible set))
void __fastcall BuildVisibleSet(void* iface, re::Edx, void* context, void*) {
    NiAVObject* root = Self(iface)->sceneRoot;
    if (root != nullptr) {
        void* culler = At<void*>(context, 0x10);
        re::PtrCast<void(__thiscall*)(void*, void*, NiAVObject*, void*)>(VtableOf(culler)[3])(
            culler, At<void*>(context, 0xC), root, At<void*>(culler, 0xC));
    }
}

uint8_t __fastcall CanResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* canReset) {
    SceneGraphComponent* self = Self(iface);
    if (*name == PROP_NIF_FILE_PATH) {
        *canReset = (self->master != nullptr && (self->flags & kUniquePath) != 0) ? 1 : 0;
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        *canReset = 0;
        return 1;
    }
    return 0;
}

// (the byte CanResetProperty writes is the name's slot)
uint8_t __fastcall ResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name) {
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));
    if (V(iface, slot::CanResetProperty, name, &canReset) && canReset != 0 && *name == PROP_NIF_FILE_PATH) {
        Self(iface)->flags &= ~kUniquePath;
        return 1;
    }
    return 0;
}

uint8_t __fastcall MakePropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* madeUnique) {
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));
    if (!V(iface, slot::CanResetProperty, name, &canReset)) {
        return 0;
    }
    SceneGraphComponent* self = Self(iface);
    if (canReset == 0 && self->master != nullptr && *name == PROP_NIF_FILE_PATH) {
        self->flags |= kUniquePath;
        *madeUnique = 1;
        return 1;
    }
    *madeUnique = 0;
    return 1;
}

uint8_t __fastcall GetDisplayName(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    if (*name == PROP_NIF_FILE_PATH) {
        FixedString_Assign(value, &PROP_NIF_FILE_PATH);
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        FixedString_SetString(value, nullptr);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetPrimitiveType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    if (*name == PROP_NIF_FILE_PATH) {
        FixedString_Assign(value, &PT_STRING);
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        FixedString_Assign(value, &PT_NIOBJECTPOINTER);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetSemanticType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    if (*name == PROP_NIF_FILE_PATH) {
        FixedString_SetString(value, kNifFilenameSemantic);
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        FixedString_Assign(value, &PT_NIOBJECTPOINTER);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetDescription(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value) {
    if (*name == PROP_NIF_FILE_PATH) {
        FixedString_Assign(value, &DESC_NIF_FILE_PATH);
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        FixedString_SetString(value, nullptr);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetCategory(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    if (!IsProperty(name)) {
        return 0;
    }
    FixedString_Assign(category, &ms_kComponentName);
    return 1;
}

// read-only: the pointer
uint8_t __fastcall IsPropertyReadOnly(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* readOnly) {
    if (*name == PROP_NIF_FILE_PATH) {
        *readOnly = 0;
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        *readOnly = 1;
        return 1;
    }
    return 0;
}

// unique: the path as CanResetProperty says when there's a master (its result not looked at)
uint8_t __fastcall IsPropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* unique) {
    if (*name == PROP_NIF_FILE_PATH) {
        if (Self(iface)->master != nullptr) {
            V(iface, slot::CanResetProperty, name, unique);
            return 1;
        }
        *unique = 1;
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        *unique = 1;
        return 1;
    }
    return 0;
}

// serializable: the path when unique and not read-only (the read-only byte starts as the high
// byte of the pushed `this`)
uint8_t __fastcall IsPropertySerializable(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* serializable) {
    if (*name != PROP_NIF_FILE_PATH) {
        if (*name == PROP_SCENE_ROOT_POINTER) {
            *serializable = 0;
            return 1;
        }
        return 0;
    }
    uint8_t unique = uint8_t(reinterpret_cast<uintptr_t>(name));
    V(iface, slot::IsPropertyUnique, name, &unique);
    uint8_t readOnly = uint8_t(reinterpret_cast<uintptr_t>(iface) >> 24);
    V(iface, slot::IsPropertyReadOnly, name, &readOnly);
    *serializable = (unique != 0 && readOnly == 0) ? 1 : 0;
    return 1;
}

// (IsPropertyInheritable and IsExternalAssetPath: the path yes, the pointer no)
uint8_t __fastcall IsPropertyInheritable(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* value) {
    if (*name == PROP_NIF_FILE_PATH) {
        *value = 1;
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        *value = 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall IsExternalAssetPath(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* value) {
    if (*name == PROP_NIF_FILE_PATH) {
        *value = 1;
        return 1;
    }
    if (*name == PROP_SCENE_ROOT_POINTER) {
        *value = 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetElementCount(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    if (!IsProperty(name)) {
        return 0;
    }
    *count = 1;
    return 1;
}

uint8_t __fastcall SetElementCount(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* countSet) {
    if (!IsProperty(name)) {
        return 0;
    }
    *countSet = 0;
    return 1;
}

uint8_t __fastcall IsCollection_(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* isCollection) {
    if (!IsProperty(name)) {
        return 0;
    }
    *isCollection = 0;
    return 1;
}

uint8_t __fastcall GetObject_(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<NiObject>* value, uint32_t index) {
    if (index == 0 && *name == PROP_SCENE_ROOT_POINTER) {
        *value = Self(iface)->sceneRoot;
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetString(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value, uint32_t index) {
    if (index != 0 || *name != PROP_NIF_FILE_PATH) {
        return 0;
    }
    FixedString_Assign(value, EffectivePath(Self(iface)));
    return 1;
}

uint8_t __fastcall SetString(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* value,
                             uint32_t index) {
    if (index == 0 && *name == PROP_NIF_FILE_PATH) {
        SetNifFilePath(Self(iface), value);
        return 1;
    }
    return 0;
}

// a copy: inheriting (this the master) or with its flags and path (and its master); the
// non-inheriting copy isn't checked for a failed allocation (as the game's)
void* __fastcall Clone_(void* iface, re::Edx, int inherit) {
    SceneGraphComponent* self = Self(iface);
    if (re::Flag(inherit)) {
        void* memory = NiNew(sizeof(SceneGraphComponent));
        if (memory == nullptr) {
            return nullptr;
        }
        return IfaceOf(Component_Construct(memory, self));
    }
    void* memory = NiNew(sizeof(SceneGraphComponent));
    SceneGraphComponent* clone = memory != nullptr ? Component_Construct(memory, self->master) : nullptr;
    clone->flags = self->flags;
    FixedString_Assign(&clone->nifFilePath, &self->nifFilePath);
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(clone) + 8);
}

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kClassName);
}

re::Ptr32<const char>* __fastcall GetName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnString(result, ms_kComponentName);
}

// none: every property made unique, the master let go; else the new master held
void __fastcall SetMasterComponent(void* iface, re::Edx, void* masterIface) {
    SceneGraphComponent* self = Self(iface);
    if (masterIface == nullptr) {
        StringSet names{FixedStringArray_New(10), 10, 0};
        V<void>(iface, slot::GetPropertyNames, &names);
        uint32_t count = names.used;
        for (uint32_t i = 0; i < count; i++) {
            uint8_t madeUnique = 0;   // (its slot: the argument's, the null master)
            V(iface, slot::MakePropertyUnique, &names.base[i], &madeUnique);
        }
        SceneGraphComponent* master = self->master;
        if (master != nullptr) {
            Release(master);
            self->master = nullptr;
        }
        if (names.base != nullptr) {
            FixedStringArray_Delete(names.base, 3);
        }
        return;
    }
    SceneGraphComponent* master = FromIface<SceneGraphComponent>(masterIface);
    if (self->master != master) {
        Release(self->master);
        self->master = master;
        AddRef(master);
    }
}

// the transformation it follows
void __fastcall GetDependentPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_TRANSLATION);
    StringSetAdd(names, &PROP_ROTATION);
    StringSetAdd(names, &PROP_SCALE);
}

void __fastcall GetPropertyNames_(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_NIF_FILE_PATH);
    StringSetAdd(names, &PROP_SCENE_ROOT_POINTER);
}

// the error handler told (slot 2: message, description, the entity's name, the property's);
// the empty description and the name live in the argument slots the game reuses
void ReportMissing(void* errors, void* entity, const re::Ptr32<const char>* message, const re::Ptr32<const char>* property) {
    NiFixedString description = NullFixedString;
    NiFixedString entityName;
    V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
    V<void>(errors, 2, message, &description, &entityName, property);
    FixedStringRelease(&entityName);
    FixedStringRelease(&description);
}

// the NIF (re)loaded when the path changed (or nothing's loaded and it hasn't failed), then the
// root moved to the entity's transformation and updated
void __fastcall Update(void* iface, re::Edx, void* entity, float time, void* errors, void* assets) {
    SceneGraphComponent* self = Self(iface);
    uint8_t flags = self->flags;
    const re::Ptr32<const char>* path = ((flags & kUniquePath) == 0 && self->master != nullptr)
                                    ? GetNifFilePath(self->master)
                                    : &self->nifFilePath;
    if (*path != self->nifFilePath || (self->sceneRoot == nullptr && (flags & kLoadFailed) == 0)) {
        FixedString_Assign(&self->nifFilePath, EffectivePath(self));
        NiAVObject* root = self->sceneRoot;
        if (root != nullptr) {
            Release(root);
            self->sceneRoot = nullptr;
        }
        if (*EffectivePath(self) != NullFixedString && assets != nullptr &&
            V<void*>(assets, 17) != nullptr) {
            ParamsNIF params;
            ParamsNIF_Construct(&params);
            // (the scene root's out slot is the asset manager's argument)
            re::Ptr32<NiAVObject> loaded = static_cast<NiAVObject*>(assets);
            bool ok = ParamsNIF_SetAssetPath(&params, GetNifFilePath(self)) && V(assets, 5, &params);
            if (ok) {
                V<void*>(assets, 7, &params);
                ok = ParamsNIF_GetSceneRoot(&params, &loaded) && loaded != nullptr;
            }
            if (!ok) {
                self->flags |= kLoadFailed;
                NiFixedString entityName;
                V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
                V<void>(errors, 2, &ERR_FILE_LOAD_FAILED, GetNifFilePath(self), &entityName, &PROP_NIF_FILE_PATH);
                FixedString_Destruct(&entityName);
                ParamsNIF_Destruct(&params);
                return;
            }
            Pointer_Assign(reinterpret_cast<re::Ptr32<NiObject>*>(&self->sceneRoot), loaded);
            uint8_t animated = HasControllers(self, self->sceneRoot);
            SetAnimated(self, animated != 0);
            AVObject_Update(self->sceneRoot, 0.0f, 1);
            V<void>(self->sceneRoot, 19);   // UpdateNodeBound
            AVObject_UpdateProperties(self->sceneRoot);
            AVObject_UpdateEffects(self->sceneRoot);
            ParamsNIF_Destruct(&params);
        }
    }

    if (self->sceneRoot == nullptr) {
        return;
    }
    // (all found: the byte of the asset manager's argument slot)
    uint8_t allFound = 1;
    NiPoint3 translate;
    NiMatrix3 rotate;
    float scale;
    if (!V(entity, slot::GetPoint3, &PROP_TRANSLATION, &translate, 0u)) {
        allFound = 0;
        ReportMissing(errors, entity, &ERR_TRANSLATION_NOT_FOUND, &PROP_TRANSLATION);
    }
    if (!V(entity, slot::GetMatrix3, &PROP_ROTATION, &rotate, 0u)) {
        allFound = 0;
        ReportMissing(errors, entity, &ERR_ROTATION_NOT_FOUND, &PROP_ROTATION);
    }
    if (!V(entity, slot::GetFloat, &PROP_SCALE, &scale, 0u)) {
        allFound = 0;
        ReportMissing(errors, entity, &ERR_SCALE_NOT_FOUND, &PROP_SCALE);
    }
    if (allFound != 0) {
        bool moved = false;
        NiAVObject* root = self->sceneRoot;
        if (Point3_NotEqual(&root->local.translate, &translate)) {
            root->local.translate = translate;
            moved = true;
        }
        if (!Matrix3_Equal(&self->sceneRoot->local.rotate, &rotate)) {
            self->sceneRoot->local.rotate = rotate;
            moved = true;
        }
        root = self->sceneRoot;
        if (!(scale == root->local.scale)) {
            uint32_t bits;   // (fabs: the sign bit cleared)
            std::memcpy(&bits, &scale, 4);
            bits &= 0x7FFFFFFF;
            std::memcpy(&root->local.scale, &bits, 4);
            AVObject_Update(self->sceneRoot, time, 1);
            return;
        }
        if (moved) {
            AVObject_Update(self->sceneRoot, time, 1);
            return;
        }
    }
    if ((self->flags & kAnimated) != 0) {
        AVObject_UpdateSelected(self->sceneRoot, time);
    }
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NISCENEGRAPHCOMP_REVERSED(name, address, fn)                                           \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace sg = ni::ec;

NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::SetAnimated", 0x0084BF60, &sg::SetAnimated_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetTemplateID", 0x0084BF90, &sg::GetTemplateID_);
// (folded: every component's slot 0)
NISCENEGRAPHCOMP_REVERSED("NiEntityComponentInterface::AddReference", 0x0084C040, &sg::AddReference_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::IsAnimated", 0x0084C050, &sg::IsAnimated_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::BuildVisibleSet", 0x0084C060, &sg::BuildVisibleSet);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::CanResetProperty", 0x0084C090, &sg::CanResetProperty);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::ResetProperty", 0x0084C0E0, &sg::ResetProperty);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::MakePropertyUnique", 0x0084C120, &sg::MakePropertyUnique);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::IsPropertyReadOnly", 0x0084C190, &sg::IsPropertyReadOnly);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::IsPropertyUnique", 0x0084C1D0, &sg::IsPropertyUnique);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::IsPropertySerializable", 0x0084C220, &sg::IsPropertySerializable);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::IsPropertyInheritable", 0x0084C2A0, &sg::IsPropertyInheritable);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::IsExternalAssetPath", 0x0084C2E0, &sg::IsExternalAssetPath);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetElementCount", 0x0084C320, &sg::GetElementCount);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::SetElementCount", 0x0084C350, &sg::SetElementCount);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::IsCollection", 0x0084C380, &sg::IsCollection_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetPropertyData(NiObject*)", 0x0084C3B0, &sg::GetObject_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::NiSceneGraphComponent", 0x0084C3E0, &sg::ComponentConstruct);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetNifFilePath", 0x0084C440, &sg::GetNifFilePath_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::SetNifFilePath", 0x0084C460, &sg::SetNifFilePath_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::_SDMInit", 0x0084C4D0, &sg::SDMInit);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::_SDMShutdown", 0x0084C5A0, &sg::SDMShutdown);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::HasControllers", 0x0084C640, &sg::HasControllers_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::Clone", 0x0084C6D0, &sg::Clone_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetClassName", 0x0084C740, &sg::GetClassName_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetName", 0x0084C770, &sg::GetName_);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::Update", 0x0084C7A0, &sg::Update);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetDisplayName", 0x0084CC70, &sg::GetDisplayName);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetPrimitiveType", 0x0084CCB0, &sg::GetPrimitiveType);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetSemanticType", 0x0084CD00, &sg::GetSemanticType);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetDescription", 0x0084CD50, &sg::GetDescription);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetCategory", 0x0084CD90, &sg::GetCategory);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetPropertyData(NiFixedString)", 0x0084CDC0, &sg::GetString);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::SetPropertyData(NiFixedString)", 0x0084CE10, &sg::SetString);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::SetMasterComponent", 0x0084CE40, &sg::SetMasterComponent);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetDependentPropertyNames", 0x0084CF30,
                          &sg::GetDependentPropertyNames);
NISCENEGRAPHCOMP_REVERSED("NiSceneGraphComponent::GetPropertyNames", 0x0084CFE0, &sg::GetPropertyNames_);
