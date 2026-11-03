// The entity library's interfaces' own code (NiEntityPropertyInterface's destructor and default
// GetPropertyData / SetPropertyData, NiEntityComponentInterface's and NiEntityInterface's
// GetInterfaceType and static names, the component interface's CanAttachToEntity /
// CanDetachFromEntity) and the components' constructors and destructors in this part of the
// exe (their other methods are further on): NiSceneGraphComponent, NiTransformationComponent,
// NiActorComponent, NiLightComponent, NiShadowGeneratorComponent and NiGeneralComponent. Each is
// an NiRefObject with the component interface at +8; the 8-byte thunks are the interface's
// slot 68 (the deleting destructor through the interface pointer).
//
// Field names are Gamebryo 2.x's where the code shows what they hold (the defaults), otherwise
// unknownXX.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntity.h"
#include "reversed.h"

namespace ni::entity {
namespace {

GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);
constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kPropertyInterfaceVtable = 0x00989CE8;
constexpr uint32_t kComponentInterfaceVtable = 0x00989E00;
constexpr uint32_t kNiTPrimitiveArrayVtable = 0x00961988;

GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t elementSize);
GAME_FN(void, __thiscall, FixedStringArray_VectorDelete, 0x0075B1B0, re::Ptr32<const char>* base, uint32_t flags);
GAME_FN(void, __thiscall, PrimitiveArray_Construct, 0x0082F290, void* self, uint32_t size, uint32_t growBy);
GAME_FN(void, __thiscall, ObjectSet_RemoveAll, 0x00836940, void* set);
GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, void* base);
GAME_FN(void, __thiscall, PointerList_RemoveAll, 0x0075DB60, void* list);
GAME_FN(void, __cdecl, NiShadowManager_DeleteShadowGenerator, 0x00758710, NiRefObject* generator);

// the static names
GAME_VAR(NiFixedString, NiEntityComponentInterface_IT_COMPONENTINTERFACE, 0x020E5C1C);   // "Component Interface"
GAME_VAR(NiFixedString, NiEntityInterface_IT_ENTITYINTERFACE, 0x020E5C20);               // "Entity Interface"
const char* const kComponentInterface = reinterpret_cast<const char*>(0x0098AE34);
const char* const kEntityInterface = reinterpret_cast<const char*>(0x0098AE48);

// the defaults the constructors copy
GAME_ARRAY(float, NiPoint3_ZERO, 0x020E2D44);
GAME_ARRAY(float, NiMatrix3_IDENTITY, 0x00BC75FC);
GAME_ARRAY(float, NiColor_WHITE, 0x020E25E8);
GAME_ARRAY(float, NiColor_BLACK, 0x00BC75C8);
GAME_VAR(uint32_t, NiActorManager_INVALID_SEQUENCE_ID, 0x00965A40);
GAME_VAR(NiFixedString, NiLightComponent_LT_DEFAULT, 0x020E5D98);   // the default light type
GAME_VAR(float, kDefaultOuterSpotAngle, 0x0094B998);   // 22.5
GAME_VAR(float, kDefaultInnerSpotAngle, 0x0098A2F4);   // 21.5
const char* const kNiStandardShadowTechnique = reinterpret_cast<const char*>(0x009516C8);

// (this file's, called where modes apply)
GAME_FN(void, __thiscall, SceneGraphComponent_Destruct, 0x0083CAC0, void* self);
GAME_FN(void, __thiscall, ActorComponent_Destruct, 0x0083CDD0, void* self);
GAME_FN(void, __thiscall, LightComponent_Destruct, 0x0083CF80, void* self);
GAME_FN(void, __thiscall, ShadowGeneratorComponent_Destruct, 0x0083D160, void* self);
GAME_FN(void, __thiscall, ShadowGeneratorComponent_DestroyShadowGenerator, 0x0083D100, void* self);
GAME_FN(void, __thiscall, GeneralComponent_Destruct, 0x0083DB20, void* self);
GAME_FN(void*, __thiscall, SceneGraphComponent_DeletingDestructor, 0x0083D030, void* self, int flags);
GAME_FN(void*, __thiscall, TransformationComponent_DeletingDestructor, 0x0083CBD0, void* self, int flags);
GAME_FN(void*, __thiscall, ActorComponent_DeletingDestructor, 0x0083CE60, void* self, int flags);
GAME_FN(void*, __thiscall, LightComponent_DeletingDestructor, 0x0083D010, void* self, int flags);
GAME_FN(void*, __thiscall, ShadowGeneratorComponent_DeletingDestructor, 0x0083D630, void* self, int flags);
GAME_FN(void*, __thiscall, GeneralComponent_DeletingDestructor, 0x0083DBB0, void* self, int flags);
GAME_FN(uint8_t, __cdecl, ComponentInterface_CanAttach, 0x00846090, void* component, void* entity);
GAME_FN(uint8_t, __cdecl, ComponentInterface_CanDetach, 0x008462D0, void* component, void* entity);

// NiPointer<T> released (NiRefObject's count, the game's import slots)
inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && ni::sys::Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// NiRefObject's constructor and destructor as the derived ones inline them
inline void ConstructRefObject(NiRefObject* self) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
}
// the interface's vtable back to NiEntityPropertyInterface's, then NiRefObject's
inline void DestructBases(NiRefObject* self) {
    At<uint32_t>(self, 8) = kPropertyInterfaceVtable;
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

// NiTPrimitiveArray's destructor inlined
inline void DestructPrimitiveArray(uint8_t* array) {
    At<uint32_t>(array, 0) = kNiTPrimitiveArrayVtable;
    NiFree(At<void*>(array, 4));
}

// ---- layouts

// NiSceneGraphComponent (vtables 0x98A05C / 0x989F30, 0x1C)
struct NiSceneGraphComponent : NiRefObject {
    re::Ptr32<void> interfaceVtable;       // +0x8
    uint8_t flags;               // +0xC (bit 2 IsAnimated)
    uint8_t padD[3];
    re::Ptr32<const char> nifFilePath;   // +0x10
    re::Ptr32<NiRefObject> sceneRoot;      // +0x14 (a reference: the NIF's NiAVObject)
    re::Ptr32<NiRefObject> masterComponent;   // +0x18 (a reference)
};
static_assert(sizeof(NiSceneGraphComponent) == 0x1C);

// NiTransformationComponent (vtables 0x98A194 / 0x98A068, 0x40)
struct NiTransformationComponent : NiRefObject {
    re::Ptr32<void> interfaceVtable;   // +0x8
    float translate[3];      // +0xC
    float rotate[9];         // +0x18
    float scale;             // +0x3C
};
static_assert(sizeof(NiTransformationComponent) == 0x40);

// NiActorComponent (vtables 0x98A2EC / 0x98A1C0, 0x24)
struct NiActorComponent : NiRefObject {
    re::Ptr32<void> interfaceVtable;       // +0x8
    uint8_t flags;               // +0xC
    uint8_t padD[3];
    re::Ptr32<const char> kfmFilePath;   // +0x10
    uint32_t activeSequenceID;   // +0x14 (NiActorManager::INVALID_SEQUENCE_ID)
    uint8_t unknown18;
    uint8_t pad19[3];
    re::Ptr32<NiRefObject> actorManager;   // +0x1C (a reference)
    re::Ptr32<NiRefObject> masterComponent;   // +0x20 (a reference: GetMasterComponent reads it here)
};
static_assert(sizeof(NiActorComponent) == 0x24);

// NiLightComponent (vtables 0x98A424 / 0x98A2F8, 0x68)
struct NiLightComponent : NiRefObject {
    re::Ptr32<void> interfaceVtable;       // +0x8
    uint8_t flags;               // +0xC
    uint8_t padD[3];
    re::Ptr32<const char> lightType;     // +0x10
    float dimmer;                // +0x14
    float ambientColor[3];       // +0x18
    float diffuseColor[3];       // +0x24
    float specularColor[3];      // +0x30
    float constantAttenuation;   // +0x3C
    float linearAttenuation;     // +0x40
    float quadraticAttenuation;  // +0x44
    float outerSpotAngle;        // +0x48
    float innerSpotAngle;        // +0x4C
    float spotExponent;          // +0x50
    uint8_t affectedEntities[0x10];   // +0x54 (NiTPrimitiveArray<NiEntityInterface*>)
    re::Ptr32<NiRefObject> light;          // +0x64 (a reference: NiLight)
};
static_assert(offsetof(NiLightComponent, affectedEntities) == 0x54);
static_assert(sizeof(NiLightComponent) == 0x68);

// NiShadowGeneratorComponent (vtables 0x98A55C / 0x98A430, 0x4C)
struct NiShadowGeneratorComponent : NiRefObject {
    re::Ptr32<void> interfaceVtable;       // +0x8
    uint32_t unknownC;           // +0xC
    re::Ptr32<NiRefObject> shadowGenerator;   // +0x10 (a reference: NiShadowGenerator)
    re::Ptr32<NiRefObject> light;          // +0x14 (a reference: the NiDynamicEffect it shadows)
    re::Ptr32<NiRefObject> masterComponent;   // +0x18 (a reference)
    uint8_t unaffectedCasters[0x10];     // +0x1C (NiTPrimitiveArray)
    uint8_t unaffectedReceivers[0x10];   // +0x2C (NiTPrimitiveArray)
    re::Ptr32<const char> technique;     // +0x3C ("NiStandardShadowTechnique")
    float depthBias;             // +0x40
    uint16_t sizeHint;           // +0x44 (1024)
    uint8_t unknown46;           // +0x46 (1)
    uint8_t unknown47;
    uint8_t unknown48;
    uint8_t unknown49;           // +0x49 (1)
    uint8_t unknown4A;           // +0x4A (1)
    uint8_t pad4B;
};
static_assert(offsetof(NiShadowGeneratorComponent, technique) == 0x3C);
static_assert(offsetof(NiShadowGeneratorComponent, sizeHint) == 0x44);
static_assert(sizeof(NiShadowGeneratorComponent) == 0x4C);

// NiGeneralComponent (vtables 0x98A6C4 / 0x98A598, 0x30)
struct NiGeneralComponent : NiRefObject {
    re::Ptr32<void> interfaceVtable;        // +0x8
    re::Ptr32<const char> name;           // +0xC
    NiTSet<NiRefObject*> properties;   // +0x10 (NiTObjectSet of its properties)
    re::Ptr32<NiRefObject> masterComponent; // +0x1C (a reference)
    uint32_t templateID[4];       // +0x20 (NiUniqueID)
};
static_assert(offsetof(NiGeneralComponent, properties) == 0x10);
static_assert(sizeof(NiGeneralComponent) == 0x30);

// ---- NiEntityPropertyInterface

// 0x83CA40: the deleting destructor (the interface alone; the game's operator delete)
void* __fastcall PropertyInterface_DeletingDestructor(void* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kPropertyInterfaceVtable;
    if ((flags & 1) != 0) {
        OperatorDelete(self);
    }
    return self;
}

// 0x846600 / 0x846610 / 0x846620: GetPropertyData / SetPropertyData's defaults (no such
// property: false), folded with every other `return false` of 5, 3 and 4 arguments
uint8_t __fastcall ReturnFalse5(void*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    return 0;
}
uint8_t __fastcall ReturnFalse3(void*, re::Edx, uint32_t, uint32_t, uint32_t) {
    return 0;
}
uint8_t __fastcall ReturnFalse4(void*, re::Edx, uint32_t, uint32_t, uint32_t, uint32_t) {
    return 0;
}

// ---- NiEntityComponentInterface / NiEntityInterface

// 0x846040 / 0x846050 and 0x8465B0 / 0x8465C0: the interface type names set and cleared
void __cdecl ComponentInterface_SDMInit() {
    FixedString_AssignText(&NiEntityComponentInterface_IT_COMPONENTINTERFACE, kComponentInterface);
}
void __cdecl ComponentInterface_SDMShutdown() {
    FixedString_AssignText(&NiEntityComponentInterface_IT_COMPONENTINTERFACE, nullptr);
}
void __cdecl EntityInterface_SDMInit() {
    FixedString_AssignText(&NiEntityInterface_IT_ENTITYINTERFACE, kEntityInterface);
}
void __cdecl EntityInterface_SDMShutdown() {
    FixedString_AssignText(&NiEntityInterface_IT_ENTITYINTERFACE, nullptr);
}

// 0x846060 / 0x8465D0 GetInterfaceType (NiFixedString returned through the hidden pointer)
re::Ptr32<const char>* __fastcall ComponentInterface_GetInterfaceType(void*, re::Edx, re::Ptr32<const char>* result) {
    *result = CopyFixedString(NiEntityComponentInterface_IT_COMPONENTINTERFACE);
    return result;
}
re::Ptr32<const char>* __fastcall EntityInterface_GetInterfaceType(void*, re::Edx, re::Ptr32<const char>* result) {
    *result = CopyFixedString(NiEntityInterface_IT_ENTITYINTERFACE);
    return result;
}

// NiTObjectSet<NiFixedString>'s destructor as these inline it: the handles let go last first,
// the counted array deleted
inline void DeleteFixedStringArray(re::Ptr32<const char>* base) {
    uint32_t* header = reinterpret_cast<uint32_t*>(base) - 1;
    for (int32_t i = int32_t(*header) - 1; i >= 0; i--) {
        ReleaseFixedString(&base[i]);
    }
    NiDeleteArray(header, 4);
}

// the index of a handle in a set's first `used` (-1: none)
inline uint32_t FindName(const NiTSet<NiFixedString>& set, NiFixedString name) {
    for (uint32_t i = 0; i < set.used; i++) {
        if (set.base[i] == name) {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

// 0x846090 CanAttachToEntity (the component, the entity): none of the component's property
// names on the entity already, and every name it depends on there
uint8_t __cdecl CanAttach(void* component, void* entity) {
    NiTSet<NiFixedString> names = {nullptr, 0, 0};
    Virtual<void>(component, prop::GetPropertyNames, &names);
    NiTSet<NiFixedString> entityNames = {nullptr, 0, 0};
    Virtual<void>(entity, prop::GetPropertyNames, &entityNames);
    for (uint32_t i = 0; i < names.used; i++) {
        if (FindName(entityNames, names.base[i]) != 0xFFFFFFFF) {
            if (entityNames.base != nullptr) {
                FixedStringArray_VectorDelete(entityNames.base, 3);
            }
            if (names.base != nullptr) {
                FixedStringArray_VectorDelete(names.base, 3);
            }
            return 0;
        }
    }
    NiTSet<NiFixedString> dependencies = {nullptr, 0, 0};
    Virtual<void>(component, comp::GetDependentPropertyNames, &dependencies);
    for (uint32_t i = 0; i < dependencies.used; i++) {
        if (FindName(entityNames, dependencies.base[i]) == 0xFFFFFFFF) {
            if (dependencies.base != nullptr) {
                FixedStringArray_VectorDelete(dependencies.base, 3);
            }
            if (entityNames.base != nullptr) {
                FixedStringArray_VectorDelete(entityNames.base, 3);
            }
            if (names.base != nullptr) {
                FixedStringArray_VectorDelete(names.base, 3);
            }
            return 0;
        }
    }
    if (dependencies.base != nullptr) {
        DeleteFixedStringArray(dependencies.base);
    }
    if (entityNames.base != nullptr) {
        DeleteFixedStringArray(entityNames.base);
    }
    if (names.base != nullptr) {
        DeleteFixedStringArray(names.base);
    }
    return 1;
}

// 0x8462D0 CanDetachFromEntity (the component, the entity): it's one of the entity's
// components and no component of the entity (itself included) depends on its property names
uint8_t __cdecl CanDetach(void* component, void* entity) {
    uint32_t count = Virtual<uint32_t>(entity, ent::GetComponentCount);
    uint32_t i = 0;
    for (; i < count; i++) {
        if (Virtual<void*>(entity, ent::GetComponentAt, i) == component) {
            break;
        }
    }
    if (i >= count) {
        return 0;
    }
    NiTSet<NiFixedString> names = {nullptr, 0, 0};
    Virtual<void>(component, prop::GetPropertyNames, &names);
    uint32_t components = Virtual<uint32_t>(entity, ent::GetComponentCount);
    for (uint32_t k = 0; k < components; k++) {
        void* other = Virtual<void*>(entity, ent::GetComponentAt, k);
        NiTSet<NiFixedString> dependencies = {nullptr, 0, 0};
        Virtual<void>(other, comp::GetDependentPropertyNames, &dependencies);
        for (uint32_t d = 0; d < dependencies.used; d++) {
            if (FindName(names, dependencies.base[d]) != 0xFFFFFFFF) {
                if (dependencies.base != nullptr) {
                    DeleteFixedStringArray(dependencies.base);
                }
                if (names.base != nullptr) {
                    DeleteFixedStringArray(names.base);
                }
                return 0;
            }
        }
        if (dependencies.base != nullptr) {
            DeleteFixedStringArray(dependencies.base);
        }
    }
    if (names.base != nullptr) {
        DeleteFixedStringArray(names.base);
    }
    return 1;
}

// 0x846570 / 0x846590: the component interface's slots 73 / 74
uint8_t __fastcall CanAttachToEntity(void* self, re::Edx, void* entity) {
    return ComponentInterface_CanAttach(self, entity);
}
uint8_t __fastcall CanDetachFromEntity(void* self, re::Edx, void* entity) {
    return ComponentInterface_CanDetach(self, entity);
}

// ---- NiSceneGraphComponent

// 0x83CA60
NiSceneGraphComponent* __fastcall SceneGraph_Construct(NiSceneGraphComponent* self, re::Edx) {
    ConstructRefObject(self);
    self->interfaceVtable = reinterpret_cast<void*>(kComponentInterfaceVtable);
    self->flags = 0;
    self->vtable = reinterpret_cast<void*>(0x0098A05C);
    self->interfaceVtable = reinterpret_cast<void*>(0x00989F30);
    self->nifFilePath = NullFixedString();
    self->sceneRoot = nullptr;
    self->masterComponent = nullptr;
    return self;
}

// 0x83CAC0
void __fastcall SceneGraph_Destruct(NiSceneGraphComponent* self, re::Edx) {
    ReleaseRef(self->masterComponent);
    ReleaseRef(self->sceneRoot);
    ReleaseFixedString(&self->nifFilePath);
    DestructBases(self);
}

// 0x83D030
NiSceneGraphComponent* __fastcall SceneGraph_DeletingDestructor(NiSceneGraphComponent* self, re::Edx, int flags) {
    SceneGraphComponent_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiSceneGraphComponent));
    }
    return self;
}

// the interface's slot 68: the object's deleting destructor from the interface pointer
template <void*(__thiscall* const* DeletingDestructor)(void*, int)>
void* __fastcall InterfaceDeletingDestructor(uint8_t* self, re::Edx, int flags) {
    return (*DeletingDestructor)(self - 8, flags);
}

// ---- NiTransformationComponent

// 0x83CB50: at the origin, unrotated, scale 1
NiTransformationComponent* __fastcall Transformation_Construct(NiTransformationComponent* self, re::Edx) {
    ConstructRefObject(self);
    self->interfaceVtable = reinterpret_cast<void*>(kComponentInterfaceVtable);
    self->vtable = reinterpret_cast<void*>(0x0098A194);
    self->interfaceVtable = reinterpret_cast<void*>(0x0098A068);
    self->translate[0] = NiPoint3_ZERO[0];
    self->translate[1] = NiPoint3_ZERO[1];
    self->translate[2] = NiPoint3_ZERO[2];
    crt::Memcpy(self->rotate, NiMatrix3_IDENTITY, sizeof(self->rotate));
    self->scale = 1.0f;
    return self;
}

// 0x83CBD0 (the destructor inlined: nothing of its own to free)
NiTransformationComponent* __fastcall Transformation_DeletingDestructor(NiTransformationComponent* self, re::Edx,
                                                                        int flags) {
    DestructBases(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiTransformationComponent));
    }
    return self;
}

// ---- NiActorComponent

// 0x83CD70
NiActorComponent* __fastcall Actor_Construct(NiActorComponent* self, re::Edx) {
    ConstructRefObject(self);
    self->interfaceVtable = reinterpret_cast<void*>(kComponentInterfaceVtable);
    self->flags = 0;
    self->vtable = reinterpret_cast<void*>(0x0098A2EC);
    self->interfaceVtable = reinterpret_cast<void*>(0x0098A1C0);
    self->kfmFilePath = NullFixedString();
    self->unknown18 = 0;
    self->activeSequenceID = NiActorManager_INVALID_SEQUENCE_ID;
    self->actorManager = nullptr;
    self->masterComponent = nullptr;
    return self;
}

// 0x83CDD0
void __fastcall Actor_Destruct(NiActorComponent* self, re::Edx) {
    ReleaseRef(self->masterComponent);
    ReleaseRef(self->actorManager);
    ReleaseFixedString(&self->kfmFilePath);
    DestructBases(self);
}

// 0x83CE60
NiActorComponent* __fastcall Actor_DeletingDestructor(NiActorComponent* self, re::Edx, int flags) {
    ActorComponent_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiActorComponent));
    }
    return self;
}

// ---- NiLightComponent

// 0x83CE80 (the affected entities' array size and growth): the default type, white ambient,
// black diffuse and specular, attenuation (1, 0, 0), spot angles 22.5 / 21.5, exponent 1
NiLightComponent* __fastcall Light_Construct(NiLightComponent* self, re::Edx, uint32_t size, uint32_t growBy) {
    ConstructRefObject(self);
    self->interfaceVtable = reinterpret_cast<void*>(kComponentInterfaceVtable);
    self->vtable = reinterpret_cast<void*>(0x0098A424);
    self->interfaceVtable = reinterpret_cast<void*>(0x0098A2F8);
    self->flags = 0;
    self->lightType = CopyFixedString(NiLightComponent_LT_DEFAULT);
    self->dimmer = 1.0f;
    for (int i = 0; i < 3; i++) {
        self->ambientColor[i] = NiColor_WHITE[i];
    }
    for (int i = 0; i < 3; i++) {
        self->diffuseColor[i] = NiColor_BLACK[i];
    }
    for (int i = 0; i < 3; i++) {
        self->specularColor[i] = NiColor_BLACK[i];
    }
    self->constantAttenuation = 1.0f;
    self->linearAttenuation = 0.0f;
    self->quadraticAttenuation = 0.0f;
    self->outerSpotAngle = kDefaultOuterSpotAngle;
    self->innerSpotAngle = kDefaultInnerSpotAngle;
    self->spotExponent = 1.0f;
    PrimitiveArray_Construct(self->affectedEntities, size, growBy);
    self->light = nullptr;
    return self;
}

// 0x83CF80
void __fastcall Light_Destruct(NiLightComponent* self, re::Edx) {
    ReleaseRef(self->light);
    DestructPrimitiveArray(self->affectedEntities);
    ReleaseFixedString(&self->lightType);
    DestructBases(self);
}

// 0x83D010
NiLightComponent* __fastcall Light_DeletingDestructor(NiLightComponent* self, re::Edx, int flags) {
    LightComponent_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiLightComponent));
    }
    return self;
}

// ---- NiShadowGeneratorComponent

// 0x83D050 (the two arrays' sizes and growths)
NiShadowGeneratorComponent* __fastcall Shadow_Construct(NiShadowGeneratorComponent* self, re::Edx, uint32_t size1,
                                                        uint32_t growBy1, uint32_t size2, uint32_t growBy2) {
    ConstructRefObject(self);
    self->interfaceVtable = reinterpret_cast<void*>(kComponentInterfaceVtable);
    self->vtable = reinterpret_cast<void*>(0x0098A55C);
    self->interfaceVtable = reinterpret_cast<void*>(0x0098A430);
    self->unknownC = 0;
    self->shadowGenerator = nullptr;
    self->light = nullptr;
    self->masterComponent = nullptr;
    PrimitiveArray_Construct(self->unaffectedCasters, size1, growBy1);
    PrimitiveArray_Construct(self->unaffectedReceivers, size2, growBy2);
    self->technique = ni::sys::GlobalStringTable_AddString(kNiStandardShadowTechnique);
    self->depthBias = 0.0f;
    self->unknown46 = 1;
    self->unknown49 = 1;
    self->unknown4A = 1;
    self->unknown47 = 0;
    self->unknown48 = 0;
    self->sizeHint = 0x400;
    return self;
}

// 0x83D100: the shadow generator taken off its casters and receivers, given to the shadow
// manager to delete and let go
void __fastcall Shadow_DestroyShadowGenerator(NiShadowGeneratorComponent* self, re::Edx) {
    if (self->shadowGenerator == nullptr) {
        return;
    }
    PointerList_RemoveAll(re::PtrCast<uint8_t*>(self->shadowGenerator) + 0x10);
    PointerList_RemoveAll(re::PtrCast<uint8_t*>(self->shadowGenerator) + 0x20);
    NiShadowManager_DeleteShadowGenerator(self->shadowGenerator);
    if (self->shadowGenerator != nullptr) {
        ReleaseRef(self->shadowGenerator);
        self->shadowGenerator = nullptr;
    }
}

// 0x83D160
void __fastcall Shadow_Destruct(NiShadowGeneratorComponent* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(0x0098A55C);
    self->interfaceVtable = reinterpret_cast<void*>(0x0098A430);
    ShadowGeneratorComponent_DestroyShadowGenerator(self);
    ReleaseFixedString(&self->technique);
    DestructPrimitiveArray(self->unaffectedReceivers);
    DestructPrimitiveArray(self->unaffectedCasters);
    ReleaseRef(self->masterComponent);
    ReleaseRef(self->light);
    ReleaseRef(self->shadowGenerator);
    DestructBases(self);
}

// 0x83D630
NiShadowGeneratorComponent* __fastcall Shadow_DeletingDestructor(NiShadowGeneratorComponent* self, re::Edx, int flags) {
    ShadowGeneratorComponent_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiShadowGeneratorComponent));
    }
    return self;
}

// ---- NiGeneralComponent

// 0x83DAB0
NiGeneralComponent* __fastcall General_Construct(NiGeneralComponent* self, re::Edx) {
    ConstructRefObject(self);
    self->interfaceVtable = reinterpret_cast<void*>(kComponentInterfaceVtable);
    self->vtable = reinterpret_cast<void*>(0x0098A6C4);
    self->interfaceVtable = reinterpret_cast<void*>(0x0098A598);
    self->name = NullFixedString();
    self->properties.base = nullptr;
    self->properties.alloced = 0;
    self->properties.used = 0;
    self->masterComponent = nullptr;
    for (uint32_t& word : self->templateID) {
        word = 0;
    }
    return self;
}

// 0x83DB20
void __fastcall General_Destruct(NiGeneralComponent* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(0x0098A6C4);
    self->interfaceVtable = reinterpret_cast<void*>(0x0098A598);
    ReleaseRef(self->masterComponent);
    ObjectSet_RemoveAll(&self->properties);
    PointerArray_Delete(self->properties.base);
    ReleaseFixedString(&self->name);
    DestructBases(self);
}

// 0x83DBB0
NiGeneralComponent* __fastcall General_DeletingDestructor(NiGeneralComponent* self, re::Edx, int flags) {
    GeneralComponent_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiGeneralComponent));
    }
    return self;
}

} // namespace

#define NIENTITYCOMP_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIENTITYCOMP_REVERSED("NiEntityPropertyInterface::DeletingDestructor", 0x0083CA40, &PropertyInterface_DeletingDestructor);
NIENTITYCOMP_REVERSED("NiSceneGraphComponent::NiSceneGraphComponent()", 0x0083CA60, &SceneGraph_Construct);
NIENTITYCOMP_REVERSED("NiSceneGraphComponent::InterfaceDeletingDestructor", 0x0083CAB0,
                      (&InterfaceDeletingDestructor<&SceneGraphComponent_DeletingDestructor>));
NIENTITYCOMP_REVERSED("NiSceneGraphComponent::~NiSceneGraphComponent", 0x0083CAC0, &SceneGraph_Destruct);
NIENTITYCOMP_REVERSED("NiTransformationComponent::NiTransformationComponent()", 0x0083CB50, &Transformation_Construct);
NIENTITYCOMP_REVERSED("NiTransformationComponent::InterfaceDeletingDestructor", 0x0083CBC0,
                      (&InterfaceDeletingDestructor<&TransformationComponent_DeletingDestructor>));
NIENTITYCOMP_REVERSED("NiTransformationComponent::DeletingDestructor", 0x0083CBD0, &Transformation_DeletingDestructor);
NIENTITYCOMP_REVERSED("NiActorComponent::NiActorComponent()", 0x0083CD70, &Actor_Construct);
NIENTITYCOMP_REVERSED("NiActorComponent::InterfaceDeletingDestructor", 0x0083CDC0,
                      (&InterfaceDeletingDestructor<&ActorComponent_DeletingDestructor>));
NIENTITYCOMP_REVERSED("NiActorComponent::~NiActorComponent", 0x0083CDD0, &Actor_Destruct);
NIENTITYCOMP_REVERSED("NiActorComponent::DeletingDestructor", 0x0083CE60, &Actor_DeletingDestructor);
NIENTITYCOMP_REVERSED("NiLightComponent::NiLightComponent(arraySizes)", 0x0083CE80, &Light_Construct);
NIENTITYCOMP_REVERSED("NiLightComponent::InterfaceDeletingDestructor", 0x0083CF70,
                      (&InterfaceDeletingDestructor<&LightComponent_DeletingDestructor>));
NIENTITYCOMP_REVERSED("NiLightComponent::~NiLightComponent", 0x0083CF80, &Light_Destruct);
NIENTITYCOMP_REVERSED("NiLightComponent::DeletingDestructor", 0x0083D010, &Light_DeletingDestructor);
NIENTITYCOMP_REVERSED("NiSceneGraphComponent::DeletingDestructor", 0x0083D030, &SceneGraph_DeletingDestructor);
NIENTITYCOMP_REVERSED("NiShadowGeneratorComponent::NiShadowGeneratorComponent(arraySizes)", 0x0083D050, &Shadow_Construct);
NIENTITYCOMP_REVERSED("NiShadowGeneratorComponent::InterfaceDeletingDestructor", 0x0083D0F0,
                      (&InterfaceDeletingDestructor<&ShadowGeneratorComponent_DeletingDestructor>));
NIENTITYCOMP_REVERSED("NiShadowGeneratorComponent::DestroyShadowGenerator", 0x0083D100, &Shadow_DestroyShadowGenerator);
NIENTITYCOMP_REVERSED("NiShadowGeneratorComponent::~NiShadowGeneratorComponent", 0x0083D160, &Shadow_Destruct);
NIENTITYCOMP_REVERSED("NiShadowGeneratorComponent::DeletingDestructor", 0x0083D630, &Shadow_DeletingDestructor);
NIENTITYCOMP_REVERSED("NiGeneralComponent::NiGeneralComponent()", 0x0083DAB0, &General_Construct);
NIENTITYCOMP_REVERSED("NiGeneralComponent::InterfaceDeletingDestructor", 0x0083DB10,
                      (&InterfaceDeletingDestructor<&GeneralComponent_DeletingDestructor>));
NIENTITYCOMP_REVERSED("NiGeneralComponent::~NiGeneralComponent", 0x0083DB20, &General_Destruct);
NIENTITYCOMP_REVERSED("NiGeneralComponent::DeletingDestructor", 0x0083DBB0, &General_DeletingDestructor);
NIENTITYCOMP_REVERSED("NiEntityComponentInterface::_SDMInit", 0x00846040, &ComponentInterface_SDMInit);
NIENTITYCOMP_REVERSED("NiEntityComponentInterface::_SDMShutdown", 0x00846050, &ComponentInterface_SDMShutdown);
NIENTITYCOMP_REVERSED("NiEntityComponentInterface::GetInterfaceType", 0x00846060, &ComponentInterface_GetInterfaceType);
NIENTITYCOMP_REVERSED("NiEntityComponentInterface::CanAttachToEntityImpl", 0x00846090, &CanAttach);
NIENTITYCOMP_REVERSED("NiEntityComponentInterface::CanDetachFromEntityImpl", 0x008462D0, &CanDetach);
NIENTITYCOMP_REVERSED("NiEntityComponentInterface::CanAttachToEntity", 0x00846570, &CanAttachToEntity);
NIENTITYCOMP_REVERSED("NiEntityComponentInterface::CanDetachFromEntity", 0x00846590, &CanDetachFromEntity);
NIENTITYCOMP_REVERSED("NiEntityInterface::_SDMInit", 0x008465B0, &EntityInterface_SDMInit);
NIENTITYCOMP_REVERSED("NiEntityInterface::_SDMShutdown", 0x008465C0, &EntityInterface_SDMShutdown);
NIENTITYCOMP_REVERSED("NiEntityInterface::GetInterfaceType", 0x008465D0, &EntityInterface_GetInterfaceType);
// (folded `xor al, al; ret N` of every library: 5 bytes, hooked like the rest)
NIENTITYCOMP_REVERSED("NiEntityPropertyInterface::ReturnFalse5_846600", 0x00846600, &ReturnFalse5);
NIENTITYCOMP_REVERSED("NiEntityPropertyInterface::ReturnFalse3_846610", 0x00846610, &ReturnFalse3);
NIENTITYCOMP_REVERSED("NiEntityPropertyInterface::ReturnFalse4_846620", 0x00846620, &ReturnFalse4);

} // namespace ni::entity
