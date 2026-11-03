// NiEntitySDM (the entity library's static data, given to NiStaticDataManager by its static
// object's constructor: Bully's initializer 0x8E7FA0 makes it, so Init runs at NiInit) and
// NiFactories: the three NiTFactory objects (streaming formats, external asset handlers,
// entities and components; "EntityCompFactory" is the library's own name for the last), their
// creation functions and the registration of the library's own classes. The classes' layouts
// are in NiEntity.h and NiEntityComponents.cpp.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntity.h"
#include "reversed.h"

namespace ni::entity {
namespace {

GAME_VAR(uint8_t, NiEntitySDM_ms_bInitialized, 0x020E5BE0);
GAME_VAR(int32_t, NiEntitySDM_ms_iInitCount, 0x020E5BE4);
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);

constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kInit = 0x0083C7F0;
constexpr uintptr_t kShutdown = 0x0083C870;
constexpr uintptr_t kDeleteObject = 0x005B0900;   // a folded `delete p` (slot 0 with 1): the destroy functions

GAME_FN(void, __cdecl, NiStaticDataManager_AddLibrary, 0x00753680, void(__cdecl* init)(), void(__cdecl* shutdown)());

// the other classes' static data (chunk 36 / 37's but for the ones in this chunk, noted)
GAME_FN(void, __cdecl, SDMInit_854350, 0x00854350);
GAME_FN(void, __cdecl, SDMShutdown_854360, 0x00854360);
GAME_FN(void, __cdecl, SDMInit_852FE0, 0x00852FE0);
GAME_FN(void, __cdecl, SDMShutdown_853000, 0x00853000);
GAME_FN(void, __cdecl, SDMInit_852FC0, 0x00852FC0);
GAME_FN(void, __cdecl, SDMShutdown_852FD0, 0x00852FD0);
GAME_FN(void, __cdecl, SDMInit_851D50, 0x00851D50);
GAME_FN(void, __cdecl, SDMShutdown_851E60, 0x00851E60);
GAME_FN(void, __cdecl, SDMInit_850D50, 0x00850D50);
GAME_FN(void, __cdecl, SDMShutdown_850E10, 0x00850E10);
GAME_FN(void, __cdecl, SDMInit_84FBF0, 0x0084FBF0);
GAME_FN(void, __cdecl, SDMShutdown_84FD20, 0x0084FD20);
GAME_FN(void, __cdecl, SDMInit_84D990, 0x0084D990);
GAME_FN(void, __cdecl, SDMShutdown_84DC00, 0x0084DC00);
GAME_FN(void, __cdecl, SDMInit_84C4D0, 0x0084C4D0);
GAME_FN(void, __cdecl, SDMShutdown_84C5A0, 0x0084C5A0);
GAME_FN(void, __cdecl, SDMInit_84A530, 0x0084A530);
GAME_FN(void, __cdecl, SDMShutdown_84A6F0, 0x0084A6F0);
GAME_FN(void, __cdecl, SDMInit_849550, 0x00849550);
GAME_FN(void, __cdecl, SDMShutdown_8495D0, 0x008495D0);
GAME_FN(void, __cdecl, SDMInit_847920, 0x00847920);
GAME_FN(void, __cdecl, SDMShutdown_847930, 0x00847930);
GAME_FN(void, __cdecl, PropertyInterface_SDMInit, 0x00846630);      // the primitive type names
GAME_FN(void, __cdecl, PropertyInterface_SDMShutdown, 0x00846720);
GAME_FN(void, __cdecl, EntityInterface_SDMInit, 0x008465B0);        // (this chunk's)
GAME_FN(void, __cdecl, EntityInterface_SDMShutdown, 0x008465C0);
GAME_FN(void, __cdecl, ComponentInterface_SDMInit, 0x00846040);
GAME_FN(void, __cdecl, ComponentInterface_SDMShutdown, 0x00846050);
GAME_FN(void, __cdecl, NIFHandler_SDMInit, 0x00845550);
GAME_FN(void, __cdecl, NIFHandler_SDMShutdown, 0x00845580);
GAME_FN(void, __cdecl, KFMHandler_SDMInit, 0x008440B0);
GAME_FN(void, __cdecl, KFMHandler_SDMShutdown, 0x008440E0);
GAME_FN(void, __cdecl, KFHandler_SDMInit, 0x00843320);
GAME_FN(void, __cdecl, KFHandler_SDMShutdown, 0x00843340);
GAME_FN(void, __cdecl, StreamingAscii_SDMInit, 0x0083FA90);
GAME_FN(void, __cdecl, StreamingAscii_SDMShutdown, 0x0083FAB0);
GAME_FN(void, __cdecl, Factories_SDMInit, 0x0083DA30);
GAME_FN(void, __cdecl, Factories_SDMShutdown, 0x0083CA00);
GAME_FN(void, __cdecl, Factories_RegisterDefaults, 0x0083DC50);

// the factory's and the classes' constructors (where modes apply)
GAME_FN(NiTFactory*, __thiscall, Factory_Construct, 0x0083D770, NiTFactory* self);
GAME_FN(void, __thiscall, Factory_Destruct, 0x0083D900, NiTFactory* self);
GAME_FN(void*, __thiscall, StreamingAscii_Construct, 0x0083FB50, void* self);
GAME_FN(void*, __thiscall, KFHandler_Construct, 0x00843360, void* self, int alwaysReload);
GAME_FN(void*, __thiscall, NIFHandler_Construct, 0x00845610, void* self, int alwaysReload);
GAME_FN(void*, __thiscall, KFMHandler_Construct, 0x008443A0, void* self, int alwaysReload);
GAME_FN(uint8_t*, __thiscall, CameraComponent_Construct, 0x00850EB0, void* self);
GAME_FN(uint8_t*, __thiscall, InheritedTransformationComponent_Construct, 0x0084FE20, void* self);
GAME_FN(uint8_t*, __thiscall, GeneralEntity_Construct, 0x008548B0, void* self);
GAME_FN(uint8_t*, __thiscall, SceneGraphComponent_Construct, 0x0083CA60, void* self);
GAME_FN(uint8_t*, __thiscall, TransformationComponent_Construct, 0x0083CB50, void* self);
GAME_FN(uint8_t*, __thiscall, ActorComponent_Construct, 0x0083CD70, void* self);
GAME_FN(uint8_t*, __thiscall, LightComponent_Construct, 0x0083CE80, void* self, uint32_t size, uint32_t growBy);
GAME_FN(uint8_t*, __thiscall, ShadowGeneratorComponent_Construct, 0x0083D050, void* self, uint32_t size1,
        uint32_t growBy1, uint32_t size2, uint32_t growBy2);
GAME_FN(uint8_t*, __thiscall, GeneralComponent_Construct, 0x0083DAB0, void* self);

// the creation functions (this file's, registered by address)
constexpr uintptr_t kCreateStreamingAscii = 0x0083C910;
constexpr uintptr_t kCreateKFHandler = 0x0083C930;
constexpr uintptr_t kCreateNIFHandler = 0x0083C950;
constexpr uintptr_t kCreateKFMHandler = 0x0083C970;
constexpr uintptr_t kCreateCameraComponent = 0x0083C990;
constexpr uintptr_t kCreateInheritedTransformationComponent = 0x0083C9B0;
constexpr uintptr_t kCreateGeneralEntity = 0x0083C9D0;
constexpr uintptr_t kCreateSceneGraphComponent = 0x0083CCD0;
constexpr uintptr_t kCreateTransformationComponent = 0x0083CCF0;
constexpr uintptr_t kCreateActorComponent = 0x0083D4F0;
constexpr uintptr_t kCreateLightComponent = 0x0083D510;
constexpr uintptr_t kCreateShadowGeneratorComponent = 0x0083D5A0;
constexpr uintptr_t kCreateGeneralComponent = 0x0083DBD0;

// the registered names (the classes' statics for the persistent ones)
GAME_VAR(NiFixedString, NiEntityStreamingAscii_STREAMING_EXTENSION, 0x020E5BF4);   // "GSA"
// (the asset handlers' T_ASSETTYPE: 0x20E5BFC "KF", 0x20E5C0C "KFM", 0x20E5C10 "NIF")

// the component registrations' class names (the game's strings)
const char* const kNiCameraComponent = reinterpret_cast<const char*>(0x008FFE64);
const char* const kNiInheritedTransformationComponent = reinterpret_cast<const char*>(0x0098A19C);
const char* const kNiGeneralEntity = reinterpret_cast<const char*>(0x008FFEB4);
const char* const kNiSceneGraphComponent = reinterpret_cast<const char*>(0x008FFE80);
const char* const kNiTransformationComponent = reinterpret_cast<const char*>(0x008FFE98);
const char* const kNiActorComponent = reinterpret_cast<const char*>(0x0098A578);
const char* const kNiLightComponent = reinterpret_cast<const char*>(0x008FFE50);
const char* const kNiShadowGeneratorComponent = reinterpret_cast<const char*>(0x008FFE34);
const char* const kNiGeneralComponent = reinterpret_cast<const char*>(0x0098A6CC);

GAME_FN(void, __cdecl, RegisterStreamingAscii, 0x0083D230, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterKFHandler, 0x0083D2E0, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterNIFHandler, 0x0083D390, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterKFMHandler, 0x0083D440, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterActorComponent, 0x0083D650, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterCameraComponent, 0x0083CC10, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterGeneralComponent, 0x0083DBF0, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterInheritedTransformationComponent, 0x0083CC70, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterLightComponent, 0x0083D6B0, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterSceneGraphComponent, 0x0083D540, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterShadowGeneratorComponent, 0x0083D710, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterTransformationComponent, 0x0083D5D0, NiTFactory* factory);
GAME_FN(void, __cdecl, RegisterGeneralEntity, 0x0083CD10, NiTFactory* factory);

// ---- NiEntitySDM

// 0x83C7F0 Init (once): every class's static data, the factories, then the library's classes
// registered in them
void __cdecl Init() {
    if (NiEntitySDM_ms_bInitialized != 0) {
        return;
    }
    NiEntitySDM_ms_bInitialized = 1;
    SDMInit_854350();
    Factories_SDMInit();
    SDMInit_852FE0();
    SDMInit_852FC0();
    SDMInit_851D50();
    SDMInit_850D50();
    SDMInit_84FBF0();
    SDMInit_84D990();
    SDMInit_84C4D0();
    SDMInit_84A530();
    SDMInit_849550();
    SDMInit_847920();
    PropertyInterface_SDMInit();
    EntityInterface_SDMInit();
    ComponentInterface_SDMInit();
    NIFHandler_SDMInit();
    KFMHandler_SDMInit();
    KFHandler_SDMInit();
    StreamingAscii_SDMInit();
    Factories_RegisterDefaults();
}

// 0x83C870 Shutdown (once Init ran)
void __cdecl Shutdown() {
    if (NiEntitySDM_ms_bInitialized == 0) {
        return;
    }
    NiEntitySDM_ms_bInitialized = 0;
    SDMShutdown_854360();
    Factories_SDMShutdown();
    SDMShutdown_853000();
    SDMShutdown_852FD0();
    SDMShutdown_851E60();
    SDMShutdown_850E10();
    SDMShutdown_84FD20();
    SDMShutdown_84DC00();
    SDMShutdown_84C5A0();
    SDMShutdown_84A6F0();
    SDMShutdown_8495D0();
    SDMShutdown_847930();
    PropertyInterface_SDMShutdown();
    EntityInterface_SDMShutdown();
    ComponentInterface_SDMShutdown();
    NIFHandler_SDMShutdown();
    KFMHandler_SDMShutdown();
    KFHandler_SDMShutdown();
    StreamingAscii_SDMShutdown();
}

// 0x83C8E0 NiEntitySDM(): the first one gives Init and Shutdown to the static data manager
void* __fastcall Construct(void* self, re::Edx) {
    int32_t count = NiEntitySDM_ms_iInitCount;
    NiEntitySDM_ms_iInitCount = count + 1;
    if (count == 0) {
        NiStaticDataManager_AddLibrary(reinterpret_cast<void(__cdecl*)()>(kInit),
                                       reinterpret_cast<void(__cdecl*)()>(kShutdown));
    }
    return self;
}

// ---- the creation functions (the factories call them with one argument, unused). The
// persistent kinds return the object; entities and components their interface (+8)

void* __cdecl CreateStreamingAscii() {
    void* p = NiNew(0x348);
    return p != nullptr ? StreamingAscii_Construct(p) : nullptr;
}
void* __cdecl CreateKFHandler() {
    void* p = NiNew(0x34);
    return p != nullptr ? KFHandler_Construct(p, 0) : nullptr;
}
void* __cdecl CreateNIFHandler() {
    void* p = NiNew(0x440);
    return p != nullptr ? NIFHandler_Construct(p, 0) : nullptr;
}
void* __cdecl CreateKFMHandler() {
    void* p = NiNew(0x5C);
    return p != nullptr ? KFMHandler_Construct(p, 0) : nullptr;
}

template <uint32_t Size, uint8_t*(__thiscall* const* Ctor)(void*)>
void* __cdecl CreateComponent() {
    void* p = NiNew(Size);
    if (p != nullptr) {
        uint8_t* object = (*Ctor)(p);
        if (object != nullptr) {
            return object + 8;
        }
    }
    return nullptr;
}

void* __cdecl CreateLightComponent() {
    void* p = NiNew(0x68);
    if (p != nullptr) {
        uint8_t* object = LightComponent_Construct(p, 0, 8);
        if (object != nullptr) {
            return object + 8;
        }
    }
    return nullptr;
}

void* __cdecl CreateShadowGeneratorComponent() {
    void* p = NiNew(0x4C);
    if (p != nullptr) {
        uint8_t* object = ShadowGeneratorComponent_Construct(p, 0, 8, 0, 8);
        if (object != nullptr) {
            return object + 8;
        }
    }
    return nullptr;
}

// ---- NiFactories

// 0x83C9F0
NiTFactory* __cdecl GetEntityCompFactory() {
    return NiFactories_ms_pkEntityCompFactory;
}

// 0x83CA00 _SDMShutdown: the three deleted (slot 0)
void __cdecl FactoriesShutdown() {
    if (NiFactories_ms_pkStreamingFactory != nullptr) {
        Virtual<void>(NiFactories_ms_pkStreamingFactory, 0, 1);
    }
    if (NiFactories_ms_pkAssetFactory != nullptr) {
        Virtual<void>(NiFactories_ms_pkAssetFactory, 0, 1);
    }
    if (NiFactories_ms_pkEntityCompFactory != nullptr) {
        Virtual<void>(NiFactories_ms_pkEntityCompFactory, 0, 1);
    }
}

// 0x83DA30 _SDMInit
void __cdecl FactoriesInit() {
    void* p = NiNew(sizeof(NiTFactory));
    NiFactories_ms_pkStreamingFactory = p != nullptr ? Factory_Construct(static_cast<NiTFactory*>(p)) : nullptr;
    p = NiNew(sizeof(NiTFactory));
    NiFactories_ms_pkAssetFactory = p != nullptr ? Factory_Construct(static_cast<NiTFactory*>(p)) : nullptr;
    p = NiNew(sizeof(NiTFactory));
    NiFactories_ms_pkEntityCompFactory = p != nullptr ? Factory_Construct(static_cast<NiTFactory*>(p)) : nullptr;
}

// a class registered with its creation function only (the entities and components)
inline void RegisterCreator(NiTFactory* factory, const char* className, uintptr_t create) {
    NiFixedString name = ni::sys::GlobalStringTable_AddString(className);
    StringMap_SetAt(factory->creators, name, reinterpret_cast<void*>(create));
    ReleaseFixedString(&name);
}

template <const char* const* Name, uintptr_t Create>
void __cdecl RegisterComponent(NiTFactory* factory) {
    RegisterCreator(factory, *Name, Create);
}

// 0x83D230 (NiEntityStreamingAscii, "GSA")
void __cdecl RegisterStreamingAsciiImpl(NiTFactory* factory) {
    NiFixedString name = CopyFixedString(NiEntityStreamingAscii_STREAMING_EXTENSION);
    StringMap_SetAt(factory->creators, name, reinterpret_cast<void*>(kCreateStreamingAscii));
    StringMap_SetAt(factory->destroyers, name, reinterpret_cast<void*>(kDeleteObject));
    void* p = NiNew(0x348);
    void* instance = p != nullptr ? StreamingAscii_Construct(p) : nullptr;
    StringMap_SetAt(factory->instances, name, instance);
    ReleaseFixedString(&name);
}

// 0x83D2E0 / 0x83D390 / 0x83D440 (the asset handlers: "KF", "NIF", "KFM"; always-reload off)
template <uintptr_t AssetType, uintptr_t Create, uint32_t Size,
          void*(__thiscall* const* Ctor)(void*, int)>
void __cdecl RegisterHandler(NiTFactory* factory) {
    NiFixedString name = CopyFixedString(*reinterpret_cast<const re::Ptr32<const char>*>(AssetType));
    StringMap_SetAt(factory->creators, name, reinterpret_cast<void*>(Create));
    StringMap_SetAt(factory->destroyers, name, reinterpret_cast<void*>(kDeleteObject));
    void* p = NiNew(Size);
    void* instance = p != nullptr ? (*Ctor)(p, 0) : nullptr;
    StringMap_SetAt(factory->instances, name, instance);
    ReleaseFixedString(&name);
}

// 0x83DC50: the library's classes (the formats, the asset handlers, the components and the
// general entity), in the game's order
void __cdecl RegisterDefaults() {
    RegisterStreamingAscii(NiFactories_ms_pkStreamingFactory);
    RegisterKFHandler(NiFactories_ms_pkAssetFactory);
    RegisterNIFHandler(NiFactories_ms_pkAssetFactory);
    RegisterKFMHandler(NiFactories_ms_pkAssetFactory);
    RegisterActorComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterCameraComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterGeneralComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterInheritedTransformationComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterLightComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterSceneGraphComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterShadowGeneratorComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterTransformationComponent(NiFactories_ms_pkEntityCompFactory);
    RegisterGeneralEntity(NiFactories_ms_pkEntityCompFactory);
}

// ---- NiTFactory

// a NiTStringPointerMap made as the constructor inlines it (37 buckets, keys copied)
StringMap* NewStringMap() {
    auto* map = static_cast<StringMap*>(NiNew(sizeof(StringMap)));
    if (map == nullptr) {
        return nullptr;
    }
    map->vtable = reinterpret_cast<void*>(kNiTMapBaseStringVtable);
    map->hashSize = 0x25;
    map->count = 0;
    map->table = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(map->table, 0, map->hashSize * 4);
    map->copyKeys = 1;
    map->vtable = reinterpret_cast<void*>(kNiTStringPointerMapVtable);
    return map;
}

// 0x83D770
NiTFactory* __fastcall FactoryConstruct(NiTFactory* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    ni::sys::Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiTFactoryVtable);
    self->creators = NewStringMap();
    self->instances = NewStringMap();
    self->destroyers = NewStringMap();
    return self;
}

// a map emptied and deleted (slot 0), as the destructor inlines it
inline void DeleteMap(re::Ptr32<StringMap>& map) {
    StringMap_RemoveAll(map);
    if (map != nullptr) {
        Virtual<void>(map, 0, 1);
    }
    map = nullptr;
}

// 0x83D900: every persistent instance given to its destroy function (one missing: a null call,
// the game's), then the maps deleted
void __fastcall FactoryDestruct(NiTFactory* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiTFactoryVtable);
    if (self->creators != nullptr) {
        DeleteMap(self->creators);
    }
    if (self->instances != nullptr) {
        StringMap* instances = self->instances;
        re::Ptr32<MapItem> position = nullptr;
        for (uint32_t i = 0; i < instances->hashSize; i++) {
            if (instances->table[i] != nullptr) {
                position = instances->table[i];
                break;
            }
        }
        while (position != nullptr) {
            re::Ptr32<void> key = nullptr;
            re::Ptr32<void> value = nullptr;
            Map_GetNext(self->instances, &position, &key, &value);
            void(__cdecl * destroy)(void*) = nullptr;
            StringMap_GetAt(self->destroyers, static_cast<const char*>(key), &destroy);
            destroy(value);
        }
        DeleteMap(self->instances);
    }
    if (self->destroyers != nullptr) {
        DeleteMap(self->destroyers);
    }
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    ni::sys::Decrement(&NiRefObject_ms_uiObjects);
}

// 0x83DA10
NiTFactory* __fastcall FactoryDeletingDestructor(NiTFactory* self, re::Edx, int flags) {
    Factory_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiTFactory));
    }
    return self;
}

} // namespace

#define NIENTITYSDM_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIENTITYSDM_REVERSED("NiEntitySDM::Init", 0x0083C7F0, &Init);
NIENTITYSDM_REVERSED("NiEntitySDM::Shutdown", 0x0083C870, &Shutdown);
NIENTITYSDM_REVERSED("NiEntitySDM::NiEntitySDM", 0x0083C8E0, &Construct);
NIENTITYSDM_REVERSED("NiEntityStreamingAscii::Create", 0x0083C910, &CreateStreamingAscii);
NIENTITYSDM_REVERSED("NiExternalAssetKFHandler::Create", 0x0083C930, &CreateKFHandler);
NIENTITYSDM_REVERSED("NiExternalAssetNIFHandler::Create", 0x0083C950, &CreateNIFHandler);
NIENTITYSDM_REVERSED("NiExternalAssetKFMHandler::Create", 0x0083C970, &CreateKFMHandler);
NIENTITYSDM_REVERSED("NiCameraComponent::Create", 0x0083C990, (&CreateComponent<0x20, &CameraComponent_Construct>));
NIENTITYSDM_REVERSED("NiInheritedTransformationComponent::Create", 0x0083C9B0,
                     (&CreateComponent<0x54, &InheritedTransformationComponent_Construct>));
NIENTITYSDM_REVERSED("NiGeneralEntity::Create", 0x0083C9D0, (&CreateComponent<0x34, &GeneralEntity_Construct>));
NIENTITYSDM_REVERSED("NiFactories::GetEntityCompFactory", 0x0083C9F0, &GetEntityCompFactory);
NIENTITYSDM_REVERSED("NiFactories::_SDMShutdown", 0x0083CA00, &FactoriesShutdown);
NIENTITYSDM_REVERSED("NiCameraComponent::RegisterFactory", 0x0083CC10, (&RegisterComponent<&kNiCameraComponent, kCreateCameraComponent>));
NIENTITYSDM_REVERSED("NiInheritedTransformationComponent::RegisterFactory", 0x0083CC70,
                     (&RegisterComponent<&kNiInheritedTransformationComponent, kCreateInheritedTransformationComponent>));
NIENTITYSDM_REVERSED("NiSceneGraphComponent::Create", 0x0083CCD0, (&CreateComponent<0x1C, &SceneGraphComponent_Construct>));
NIENTITYSDM_REVERSED("NiTransformationComponent::Create", 0x0083CCF0,
                     (&CreateComponent<0x40, &TransformationComponent_Construct>));
NIENTITYSDM_REVERSED("NiGeneralEntity::RegisterFactory", 0x0083CD10, (&RegisterComponent<&kNiGeneralEntity, kCreateGeneralEntity>));
NIENTITYSDM_REVERSED("NiEntityStreamingAscii::RegisterFactory", 0x0083D230, &RegisterStreamingAsciiImpl);
NIENTITYSDM_REVERSED("NiExternalAssetKFHandler::RegisterFactory", 0x0083D2E0,
                     (&RegisterHandler<0x020E5BFC, kCreateKFHandler, 0x34, &KFHandler_Construct>));
NIENTITYSDM_REVERSED("NiExternalAssetNIFHandler::RegisterFactory", 0x0083D390,
                     (&RegisterHandler<0x020E5C10, kCreateNIFHandler, 0x440, &NIFHandler_Construct>));
NIENTITYSDM_REVERSED("NiExternalAssetKFMHandler::RegisterFactory", 0x0083D440,
                     (&RegisterHandler<0x020E5C0C, kCreateKFMHandler, 0x5C, &KFMHandler_Construct>));
NIENTITYSDM_REVERSED("NiActorComponent::Create", 0x0083D4F0, (&CreateComponent<0x24, &ActorComponent_Construct>));
NIENTITYSDM_REVERSED("NiLightComponent::Create", 0x0083D510, &CreateLightComponent);
NIENTITYSDM_REVERSED("NiSceneGraphComponent::RegisterFactory", 0x0083D540,
                     (&RegisterComponent<&kNiSceneGraphComponent, kCreateSceneGraphComponent>));
NIENTITYSDM_REVERSED("NiShadowGeneratorComponent::Create", 0x0083D5A0, &CreateShadowGeneratorComponent);
NIENTITYSDM_REVERSED("NiTransformationComponent::RegisterFactory", 0x0083D5D0,
                     (&RegisterComponent<&kNiTransformationComponent, kCreateTransformationComponent>));
NIENTITYSDM_REVERSED("NiActorComponent::RegisterFactory", 0x0083D650, (&RegisterComponent<&kNiActorComponent, kCreateActorComponent>));
NIENTITYSDM_REVERSED("NiLightComponent::RegisterFactory", 0x0083D6B0, (&RegisterComponent<&kNiLightComponent, kCreateLightComponent>));
NIENTITYSDM_REVERSED("NiShadowGeneratorComponent::RegisterFactory", 0x0083D710,
                     (&RegisterComponent<&kNiShadowGeneratorComponent, kCreateShadowGeneratorComponent>));
NIENTITYSDM_REVERSED("NiTFactory::NiTFactory", 0x0083D770, &FactoryConstruct);
NIENTITYSDM_REVERSED("NiTFactory::~NiTFactory", 0x0083D900, &FactoryDestruct);
NIENTITYSDM_REVERSED("NiTFactory::DeletingDestructor", 0x0083DA10, &FactoryDeletingDestructor);
NIENTITYSDM_REVERSED("NiFactories::_SDMInit", 0x0083DA30, &FactoriesInit);
NIENTITYSDM_REVERSED("NiGeneralComponent::Create", 0x0083DBD0, (&CreateComponent<0x30, &GeneralComponent_Construct>));
NIENTITYSDM_REVERSED("NiGeneralComponent::RegisterFactory", 0x0083DBF0,
                     (&RegisterComponent<&kNiGeneralComponent, kCreateGeneralComponent>));
NIENTITYSDM_REVERSED("NiFactories::RegisterDefaults", 0x0083DC50, &RegisterDefaults);

} // namespace ni::entity
