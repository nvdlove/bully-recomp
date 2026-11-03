// NiDefaultShadowClickGenerator (vtable 0x9606A8, 0x60 bytes): the shadow click generator the
// shadow manager registers by default. Each frame it sorts the shadow generators by their
// light's distance from the scene camera (the nearest ones active), then for each active one
// gets a shadow map (or cube map) and fills its render click: the casters as the render view's
// scenes, the light's view as the camera, the near / far planes fitted to the casters' bounds.
// Its own slots (after NiRefObject's and GetRTTI): 3 GenerateRenderClicks,
// 4 ReorganizeActiveShadowGenerators, 5..7 Prepare{Spot,Point,Directional}LightShadowGenerator,
// 8..10 Handle{Spot,Point,Directional}Light, 11 ObtainAndAssignShadowMap,
// 12 ObtainAndAssignShadowCubeMap.
//
// Also NiShadowTechnique (vtable 0x9606DC, 0x4C bytes) and NiVSMShadowTechnique's constructor
// (0x6C bytes), its NiTPrimitiveArray<float> (SetAt, SetSize), its NiTPointerMap<void*, float>
// (vtables 0x960680 / 0x960694), Ni3DRenderView::AppendScene and
// NiViewRenderClick::SetProcessor.
//
// Light types (NiDynamicEffect +0xC5): 2 point, 3 directional, 4 spot; +3 when it casts
// shadows (the directional light's constructor, 0x7D5500, sets 3).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiShadowGenerator.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiDefaultShadowClickGenerator_RTTI, 0x020E3768);
GAME_VAR(RTTI, NiShadowTechnique_RTTI, 0x020E37D0);
GAME_VAR(long, ms_uiObjects, 0x020E2168);
GAME_VAR(uint32_t, kNi3DRenderViewClassIndex, 0x020E35D0);
GAME_VAR(uint32_t, kNiShadowCubeMapClassIndex, 0x020E2584);
GAME_VAR(uint8_t*, ms_pkShadowManager, 0x020E2410);
inline const char* const kVSMTechniqueName = reinterpret_cast<const char*>(0x00951654);   // "NiVSMShadowTechnique"

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kNiShadowClickGeneratorVtable = 0x0095F800;
constexpr uintptr_t kVtable = 0x009606A8;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;
constexpr uintptr_t kCasterListVtable = 0x00952E10;
constexpr uintptr_t kPointerListBaseVtable = 0x0099C748;
constexpr uintptr_t kMapBaseVtable = 0x00960680;   // NiTMapBase
constexpr uintptr_t kMapVtable = 0x00960694;       // NiTPointerMap
constexpr uintptr_t kNiShadowTechniqueVtable = 0x009606DC;
constexpr uintptr_t kNiVSMShadowTechniqueVtable = 0x009606EC;

// the shadow manager's fields used here (its layout: NiShadowManager.cpp)
constexpr int kMaxActiveGenerators = 0xB4;
constexpr int kSceneCamera = 0xC0;
constexpr int kPointSwapProcessor = 0xCC;
constexpr int kSpotSwapProcessor = 0xD0;
constexpr int kRenderClicks = 0xF8;   // NiTPointerList<NiRenderClick*> (its allocator: slot 1 a node)

// NiTPointerMap<void*, float>: NiTMapBase's layout; an item: next, key, value
struct MapItem {
    re::Ptr32<MapItem> next;
    re::Ptr32<void> key;
    float value;
};
struct Map {
    re::Ptr32<void> vtable;          // +0x0
    uint32_t hashSize;     // +0x4
    re::Ptr32<re::Ptr32<MapItem>> buckets;     // +0x8
    uint32_t count;        // +0xC
};
static_assert(sizeof(Map) == 0x10);

using GeneratorList = NiTPointerList<NiShadowGenerator*>;
using ObjectList = NiTPointerList<NiObject*>;

struct NiDefaultShadowClickGenerator : NiRefObject {
    uint32_t unknown8;                         // +0x8 (NiShadowClickGenerator's: 0)
    uint32_t unknownC;                         // +0xC (0)
    uint32_t unknown10;                        // +0x10 (0)
    uint32_t unknown14;                        // +0x14 (GenerateRenderClicks zeroes it)
    re::Ptr32<char> name;                                // +0x18 (NiFixedString: the class's name)
    NiTArray<float> distances;                 // +0x1C (NiTPrimitiveArray<float>)
    NiTArray<NiShadowGenerator*> generators;   // +0x2C (sorted by distance)
    ObjectList casters;                        // +0x3C (NiTPointerList<NiGeometry*>)
    Map map;                                   // +0x4C (37 buckets)
    re::Ptr32<char> techniqueName;                       // +0x5C ("NiVSMShadowTechnique")
};
static_assert(offsetof(NiDefaultShadowClickGenerator, name) == 0x18);
static_assert(offsetof(NiDefaultShadowClickGenerator, distances) == 0x1C);
static_assert(offsetof(NiDefaultShadowClickGenerator, generators) == 0x2C);
static_assert(offsetof(NiDefaultShadowClickGenerator, casters) == 0x3C);
static_assert(offsetof(NiDefaultShadowClickGenerator, map) == 0x4C);
static_assert(offsetof(NiDefaultShadowClickGenerator, techniqueName) == 0x5C);
static_assert(sizeof(NiDefaultShadowClickGenerator) == 0x60);

// NiShadowTechnique: its name, the fragment names it hands the materials (two sets of three,
// by light type), the shadow map texture's format prefs
struct NiShadowTechnique {
    re::Ptr32<void> vtable;                 // +0x0 (0 GetRTTI, 1 deleting destructor, 2 (map, click
                                  //  generator): VSM's adds its blur clicks, 3 PrepareShadowMap)
    re::Ptr32<char> name;                   // +0x4 (NiFixedString)
    re::Ptr32<char> fragmentsA[3];          // +0x8
    re::Ptr32<char> fragmentsB[3];          // +0x14
    float unknown20[6];           // +0x20 (0)
    uint32_t formatPrefs[3];      // +0x38 (NiTexture::FormatPrefs: pixel layout 9, alpha 3,
                                  //  mipmapped 0)
    uint16_t unknown44;           // +0x44 (0x10)
    uint16_t unknown46;           // +0x46 (0x10)
    uint8_t unknown48;            // +0x48 (the constructor's last argument)
    uint8_t unknown49;            // +0x49 (the one before)
    uint16_t pad4A;
};
static_assert(offsetof(NiShadowTechnique, fragmentsB) == 0x14);
static_assert(offsetof(NiShadowTechnique, formatPrefs) == 0x38);
static_assert(offsetof(NiShadowTechnique, unknown48) == 0x48);
static_assert(sizeof(NiShadowTechnique) == 0x4C);

struct NiVSMShadowTechnique : NiShadowTechnique {
    uint8_t unknown4C;            // +0x4C (0)
    uint8_t unknown4D;            // +0x4D (0)
    uint16_t pad4E;
    uint32_t unknown50;           // +0x50 (4)
    float unknown54;              // +0x54 (512)
    float unknown58;              // +0x58 (512)
    uint32_t unknown5C[4];        // +0x5C (0)
};
static_assert(sizeof(NiVSMShadowTechnique) == 0x6C);

// NiFrustum: left, right, top, bottom, near, far, ortho
struct NiFrustum {
    float left, right, top, bottom, nearPlane, farPlane;
    uint8_t ortho;
    uint8_t pad[3];
};
static_assert(sizeof(NiFrustum) == 0x1C);

GAME_FN(void, __thiscall, Map_RemoveAll, 0x005AC520, Map* map);
GAME_FN(void, __thiscall, ShadowClickGenerator_Destruct, 0x007ABB60, NiDefaultShadowClickGenerator* self);
// the shadow manager's (static) and the shadow generator's
GAME_FN(void, __cdecl, ShadowManager_ReorganizeActiveGenerators, 0x007571B0);
GAME_FN(void*, __cdecl, ShadowManager_FindAvailableShadowMap, 0x00757C10, int width, int height, uint32_t* prefs,
        re::Ptr32<void>* found, int exactOnly);
GAME_FN(void*, __cdecl, ShadowManager_CreateShadowMap, 0x007575D0, int width, int height, uint32_t* prefs, int a4,
        int a5);
GAME_FN(void*, __cdecl, ShadowManager_FindAvailableShadowCubeMap, 0x00757CF0, int size, uint32_t* prefs,
        re::Ptr32<void>* found, int exactOnly);
GAME_FN(void*, __cdecl, ShadowManager_CreateShadowCubeMap, 0x00757980, int size, uint32_t* prefs, int a3, int a4);
GAME_FN(uint8_t, __thiscall, ShadowGenerator_GetShadowMapsDirty, 0x007729E0, NiShadowGenerator* self);
GAME_FN(uint8_t, __thiscall, ShadowGenerator_SetActive, 0x007736C0, NiShadowGenerator* self, int active, int force,
        int a3);
GAME_FN(uint8_t, __thiscall, ShadowGenerator_AssignShadowMap, 0x007734D0, NiShadowGenerator* self, void* map,
        uint32_t index, void* position, void* geometry);
GAME_FN(void, __thiscall, ShadowGenerator_GetCasterGeometryList, 0x00773360, NiShadowGenerator* self,
        ObjectList* list);
// lists and arrays
GAME_FN(void, __thiscall, PointerArray_SetSize, 0x0084A110, NiTArray<NiShadowGenerator*>* self, uint32_t size);
GAME_FN(void, __thiscall, PointerArray_SetAt, 0x00833C70, NiTArray<NiShadowGenerator*>* self, uint32_t index,
        const re::Ptr32<ni::NiShadowGenerator>* value);
GAME_FN(void, __thiscall, ObjectList_AddTail, 0x007843D0, ObjectList* self, const re::Ptr32<NiObject>* element);
GAME_FN(void, __thiscall, ObjectList_RemoveAll, 0x0075DB60, ObjectList* self);
// the camera
GAME_FN(float, __thiscall, Vector_Magnitude, 0x00415100, const NiPoint3* self);   // the game's CVector
GAME_FN(NiFrustum*, __thiscall, Frustum_Construct, 0x007718A0, NiFrustum* self, float left, float right, float top,
        float bottom, float nearPlane, float farPlane, int ortho);
GAME_FN(void, __thiscall, Camera_SetViewFrustum, 0x00762D60, NiAVObject* self, const NiFrustum* frustum);
GAME_FN(void, __thiscall, Camera_LookAtWorldPoint, 0x00763020, NiAVObject* self, const NiPoint3* point,
        const NiPoint3* up);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, NiAVObject* self, float time, int updateControllers);
// NiFixedString
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<char>* self, const char* text);
GAME_FN(re::Ptr32<char>*, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, FloatArray_SetAt, 0x007CF000, NiTArray<float>* self, uint32_t index, const float* value);
GAME_FN(void, __thiscall, FloatArray_SetSize, 0x007CF4F0, NiTArray<float>* self, uint32_t size);
GAME_FN(void, __thiscall, ViewRenderClick_SetProcessor, 0x007CF610, NiObject* self, NiRefObject* processor,
        void* data);
GAME_FN(void, __thiscall, ClickGenerator_AddRenderClick, 0x007CF680, NiDefaultShadowClickGenerator* self,
        NiObject* click);
GAME_FN(void, __thiscall, RenderView_AppendScene, 0x007CFB00, NiObject* self, NiObject* scene);
GAME_FN(void, __thiscall, ClickGenerator_Destruct, 0x007CF6D0, NiDefaultShadowClickGenerator* self);
GAME_FN(void, __thiscall, ShadowTechnique_Destruct, 0x007D0F40, NiShadowTechnique* self);
GAME_FN(NiShadowTechnique*, __thiscall, ShadowTechnique_Construct, 0x007D18F0, NiShadowTechnique* self,
        const re::Ptr32<char>* name, char* a0, char* a1, char* a2, char* b0, char* b1, char* b2, int arg8, int arg9);

// the light's world translation, rotation, scale (NiAVObject::world)
inline NiPoint3& WorldTranslate(NiAVObject* object) {
    return object->world.translate;
}

// NiIsKindOf by the class index bitset Bully's NiRTTI carries (+0xC)
inline bool HasClass(const void* rtti, uint32_t index) {
    return (At<uint32_t>(rtti, 0xC + (index >> 5) * 4) & (1u << (index & 0x1F))) != 0;
}

// the render click's first render view as an Ni3DRenderView (NiDynamicCast)
inline NiObject* FirstRenderView(NiObject* click) {
    NiObject* view = At<ObjectList>(click, 0x4C).head->element;
    if (view == nullptr) {
        return nullptr;
    }
    return HasClass(Virtual<void*>(view, 2), kNi3DRenderViewClassIndex) ? view : nullptr;
}

inline ObjectList* Scenes(NiObject* renderView) {
    return &At<ObjectList>(renderView, 0x28);
}

// NiTPointerList<NiAVObjectPtr>::AddTail with a reference made for it
inline void AddScene(ObjectList* scenes, NiObject* scene) {
    re::Ptr32<NiObject> held = scene;
    if (held != nullptr) {
        Increment(&held->refCount);
    }
    ObjectList_AddTail(scenes, &held);
    if (held != nullptr && Decrement(&held->refCount) == 0) {
        Virtual<void>(held, 1);
    }
}

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

inline void AddRefImport(NiRefObject* object) {
    if (object != nullptr) {
        Increment(&object->refCount);
    }
}

// a geometry's world bound center less the light's position
inline NiPoint3 Offset(NiObject* geometry, NiAVObject* light) {
    const NiBound& bound = At<NiBound>(geometry, 0x1C);
    NiPoint3 d;
    d.x = bound.center.x - light->world.translate.x;
    d.y = bound.center.y - light->world.translate.y;
    d.z = bound.center.z - light->world.translate.z;
    return d;
}

inline float Radius(NiObject* geometry) {
    return At<NiBound>(geometry, 0x1C).radius;
}

// ---- NiTPrimitiveArray<float>

// 0x7CF000 SetAt(index, value): the used count kept (an element of 0 is empty)
void __fastcall FloatArray_SetAtImpl(NiTArray<float>* self, re::Edx, uint32_t index, const float* value) {
    if (index >= self->size) {
        self->size = uint16_t(index + 1);
        if (*value != 0.0f) {
            self->esize++;
        }
    } else if (*value == 0.0f) {
        if (self->base[index] != 0.0f) {
            self->esize--;
        }
    } else if (self->base[index] == 0.0f) {
        self->esize++;
    }
    self->base[index] = *value;
}

// 0x7CF4F0 SetSize(maxSize): what's beyond the new size emptied, the elements moved to a new
// block, the rest zero
void __fastcall FloatArray_SetSizeImpl(NiTArray<float>* self, re::Edx, uint32_t maxSize) {
    if (maxSize == self->maxSize) {
        return;
    }
    if (maxSize < self->size) {
        for (uint16_t i = uint16_t(maxSize); i < self->size; i++) {
            if (self->base[i] != 0.0f) {
                self->base[i] = 0.0f;
                self->esize += 0xFFFF;
            }
        }
        self->size = uint16_t(maxSize);
    }
    float* old = self->base;
    self->maxSize = uint16_t(maxSize);
    if (maxSize == 0) {
        self->base = nullptr;
    } else {
        self->base = static_cast<float*>(NiMalloc(uint32_t(uint16_t(maxSize)) * 4));
        for (uint16_t i = 0; i < self->size; i++) {
            self->base[i] = old[i];
        }
        for (uint16_t i = self->size; i < self->maxSize; i++) {
            self->base[i] = 0.0f;
        }
    }
    NiFree(old);
}

// ---- NiTPointerMap<void*, float>

// 0x7CF080 SetValue(item, key, value)
void __fastcall Map_SetValueImpl(Map*, re::Edx, MapItem* item, void* key, uint32_t value) {
    reinterpret_cast<uint32_t&>(item->value) = value;
    item->key = key;
}

// 0x7CF0A0 NewItem (its value 0)
MapItem* __fastcall Map_NewItemImpl(Map*, re::Edx) {
    MapItem* item = static_cast<MapItem*>(NiNew(0xC));
    item->value = 0.0f;
    return item;
}

// 0x7CF0B0 DeleteItem
void __fastcall Map_DeleteItemImpl(Map*, re::Edx, MapItem* item) {
    item->value = 0.0f;
    NiDelete(item, 0xC);
}

// 0x7CF0D0 NiTMapBase's deleting destructor
Map* __fastcall MapBase_DeletingDestructorImpl(Map* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kMapBaseVtable);
    Map_RemoveAll(self);
    NiFree(self->buckets);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x7CF110 NiTPointerMap's deleting destructor
Map* __fastcall Map_DeletingDestructorImpl(Map* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kMapVtable);
    Map_RemoveAll(self);
    self->vtable = reinterpret_cast<void*>(kMapBaseVtable);
    Map_RemoveAll(self);
    NiFree(self->buckets);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// ---- NiDefaultShadowClickGenerator

// 0x7D0D30
NiDefaultShadowClickGenerator* __fastcall ConstructImpl(NiDefaultShadowClickGenerator* self, re::Edx) {
    const char* className = NiDefaultShadowClickGenerator_RTTI.name;
    re::Ptr32<char> name = className == nullptr ? static_cast<char*>(kNullFixedString) : GlobalStringTable_AddString(className);
    // NiShadowClickGenerator(name)
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kNiShadowClickGeneratorVtable);
    self->unknown8 = 0;
    self->unknownC = 0;
    self->unknown10 = 0;
    self->name = FixedStringAddRef(name);
    FixedStringRelease(&name);

    self->vtable = reinterpret_cast<void*>(kVtable);
    self->distances.maxSize = 0;
    self->distances.growBy = 1;
    self->distances.size = 0;
    self->distances.esize = 0;
    self->distances.base = nullptr;
    self->distances.vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    self->generators.maxSize = 0;
    self->generators.growBy = 1;
    self->generators.size = 0;
    self->generators.esize = 0;
    self->generators.base = nullptr;
    self->generators.vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    self->casters.count = 0;
    self->casters.head = nullptr;
    self->casters.tail = nullptr;
    self->casters.vtable = reinterpret_cast<void*>(kCasterListVtable);
    self->map.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    self->map.hashSize = 37;
    self->map.count = 0;
    self->map.buckets = static_cast<re::Ptr32<MapItem>*>(NiMalloc(0x94));
    crt::Memset(self->map.buckets, 0, self->map.hashSize * 4);
    self->map.vtable = reinterpret_cast<void*>(kMapVtable);
    self->techniqueName = kNullFixedString;
    FixedString_AssignText(&self->techniqueName, kVSMTechniqueName);
    return self;
}

// 0x7CF6D0
void __fastcall DestructImpl(NiDefaultShadowClickGenerator* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kVtable);
    FixedStringRelease(&self->techniqueName);
    self->map.vtable = reinterpret_cast<void*>(kMapVtable);
    Map_RemoveAll(&self->map);
    self->map.vtable = reinterpret_cast<void*>(kMapBaseVtable);
    Map_RemoveAll(&self->map);
    NiFree(self->map.buckets);
    self->casters.vtable = reinterpret_cast<void*>(kCasterListVtable);
    ObjectList_RemoveAll(&self->casters);
    self->casters.vtable = reinterpret_cast<void*>(kPointerListBaseVtable);
    self->generators.vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    NiFree(self->generators.base);
    self->distances.vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    NiFree(self->distances.base);
    ShadowClickGenerator_Destruct(self);
}

// 0x7CFB50 vf0
NiDefaultShadowClickGenerator* __fastcall DeletingDestructorImpl(NiDefaultShadowClickGenerator* self, re::Edx,
                                                                uint32_t flags) {
    ClickGenerator_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x60);
    }
    return self;
}

// 0x7CF780 vf2
const RTTI* __fastcall GetRTTIImpl(NiDefaultShadowClickGenerator*, re::Edx) {
    return &NiDefaultShadowClickGenerator_RTTI;
}

// 0x7CF150 vf3 GenerateRenderClicks(generators): each active generator of a switched-on light
// handed to its light type's handler (static ones only when they're to regenerate or their
// maps are dirty); one that fails (or of no light type) is made inactive
uint8_t __fastcall GenerateRenderClicksImpl(NiDefaultShadowClickGenerator* self, re::Edx, GeneratorList* list) {
    self->unknown14 = 0;
    ShadowManager_ReorganizeActiveGenerators();
    for (NiTListNode<NiShadowGenerator*>* node = list->head; node != nullptr;) {
        NiShadowGenerator* generator = node->element;
        node = node->next;
        if (generator == nullptr) {
            continue;
        }
        uint8_t* effect = re::PtrCast<uint8_t*>(generator->dynamicEffect);
        uint8_t flags = uint8_t(generator->flags);
        uint8_t regenerate = 0;
        uint8_t update = 0;
        if (((flags >> 3) & 1) == 0 || effect[0xC4] == 0) {
            continue;
        }
        if (((flags >> 2) & 1) != 0) {
            if (((flags >> 1) & 1) != 0) {
                regenerate = 1;
            }
            if (ShadowGenerator_GetShadowMapsDirty(generator) != 0) {
                update = 1;
            }
            if (regenerate == 0 && update == 0) {
                continue;
            }
        } else {
            update = 1;
            if (((flags >> 1) & 1) != 0) {
                regenerate = 1;
            }
        }
        int slot;
        switch (effect[0xC5]) {
        case 2:
        case 5:
            slot = 9;   // HandlePointLight
            break;
        case 3:
        case 6:
            slot = 10;   // HandleDirectionalLight
            break;
        case 4:
        case 7:
            slot = 8;   // HandleSpotLight
            break;
        default:
            slot = -1;
            break;
        }
        if (slot >= 0 &&
            Virtual<uint8_t>(self, slot, generator, effect, int(regenerate), int(update)) != 0) {
            continue;
        }
        ShadowGenerator_SetActive(generator, 0, 1, 0);
    }
    return 1;
}

// 0x7CF880 vf4 ReorganizeActiveShadowGenerators(active, all): when there are more generators
// than the manager allows active, the nearest to the scene camera (directional lights count as
// distance 0) made active and the rest inactive; otherwise all of them active
void __fastcall ReorganizeActiveShadowGeneratorsImpl(NiDefaultShadowClickGenerator* self, re::Edx,
                                                     GeneratorList* active, GeneratorList* all) {
    if (active->count == all->count) {
        return;
    }
    if (At<uint32_t>(ms_pkShadowManager, kMaxActiveGenerators) >= all->count) {
        for (NiTListNode<NiShadowGenerator*>* node = all->head; node != nullptr;) {
            NiShadowGenerator* generator = node->element;
            node = node->next;
            ShadowGenerator_SetActive(generator, 1, 0, 0);
        }
        return;
    }
    NiAVObject* camera = At<NiAVObject*>(ms_pkShadowManager, kSceneCamera);
    if (camera == nullptr) {
        return;
    }
    FloatArray_SetSize(&self->distances, all->count);
    PointerArray_SetSize(&self->generators, all->count);
    NiPoint3 cameraPos = camera->world.translate;
    uint32_t index = 0;
    for (NiTListNode<NiShadowGenerator*>* node = all->head; node != nullptr; index++) {
        re::Ptr32<NiShadowGenerator> generator = node->element;
        node = node->next;
        PointerArray_SetAt(&self->generators, index, &generator);
        NiAVObject* light = re::PtrCast<NiAVObject*>(generator->dynamicEffect);
        uint8_t type = reinterpret_cast<uint8_t*>(light)[0xC5];
        float distance;
        if (type == 3 || type == 6) {
            distance = 0.0f;
        } else {
            NiPoint3 p = light->world.translate;
            float dx = p.x - cameraPos.x;
            float dy = p.y - cameraPos.y;
            float dz = p.z - cameraPos.z;
            distance = dy * dy + dx * dx + dz * dz;
        }
        FloatArray_SetAt(&self->distances, index, &distance);
    }
    // a bubble sort, nearest first
    uint32_t count = self->generators.size;
    for (uint32_t pass = 0; count - 1 != 0 && pass < count - 1; pass++) {
        uint32_t last = count - pass - 1;
        for (uint32_t i = 0; i < last; i++) {
            float* d = self->distances.base;
            if (d[i + 1] < d[i]) {
                float distance = d[i];
                FloatArray_SetAt(&self->distances, i, &d[i + 1]);
                FloatArray_SetAt(&self->distances, i + 1, &distance);
                re::Ptr32<NiShadowGenerator>* g = self->generators.base;
                re::Ptr32<NiShadowGenerator> generator = g[i];
                PointerArray_SetAt(&self->generators, i, &g[i + 1]);
                PointerArray_SetAt(&self->generators, i + 1, &generator);
            }
        }
    }
    for (uint32_t i = At<uint32_t>(ms_pkShadowManager, kMaxActiveGenerators); i < self->generators.size; i++) {
        ShadowGenerator_SetActive(self->generators.base[i], 0, 1, 0);
    }
    for (uint32_t i = 0; i < At<uint32_t>(ms_pkShadowManager, kMaxActiveGenerators); i++) {
        ShadowGenerator_SetActive(self->generators.base[i], 1, 1, 0);
    }
}

// the generator's callbacks the default ones (the shadow generator's own, 0x772410..0x772AC0)
inline void SetDefaultCallbacks(NiShadowGenerator* generator) {
    uint8_t* g = reinterpret_cast<uint8_t*>(generator);
    At<uint32_t>(g, 0x34) = 0x00772510;
    At<uint32_t>(g, 0x30) = 0x00772530;
    At<uint32_t>(g, 0x38) = 0x00772410;
    At<uint32_t>(g, 0x3C) = 0x00772430;
    At<uint32_t>(g, 0x40) = 0;
    At<uint32_t>(g, 0x44) = 0x00772AC0;
    At<uint32_t>(g, 0x48) = 0x00772550;
}

// 0x7CF350 vf6 PreparePointLightShadowGenerator: the default callbacks, and false
uint8_t __fastcall PreparePointLightShadowGeneratorImpl(NiDefaultShadowClickGenerator*, re::Edx,
                                                        NiShadowGenerator* generator) {
    SetDefaultCallbacks(generator);
    return 0;
}

// 0x7CF390 vf5 / vf7 Prepare{Spot,Directional}LightShadowGenerator: the default callbacks
uint8_t __fastcall PrepareShadowGeneratorImpl(NiDefaultShadowClickGenerator*, re::Edx, NiShadowGenerator* generator) {
    SetDefaultCallbacks(generator);
    return 1;
}

// 0x7CF290 vf11 ObtainAndAssignShadowMap(generator, index, geometry): the generator's own (its
// callback) or one from the manager's pool (an exact fit, else a new one, else (unless the
// generator wants exact sizes) the nearest), assigned to the generator
void* __fastcall ObtainAndAssignShadowMapImpl(NiDefaultShadowClickGenerator*, re::Edx, NiShadowGenerator* generator,
                                              uint32_t index, void* geometry) {
    using Retrieve = void*(__cdecl*)(NiShadowGenerator*, void*, uint32_t);
    re::Ptr32<void> map = re::PtrCast<Retrieve>(generator->callback30)(generator, geometry, index);
    if (map != nullptr) {
        return map;
    }
    int size = generator->sizeHint;
    uint32_t* prefs = &At<uint32_t>(generator->technique, 0x38);
    uint8_t exactOnly = uint8_t(generator->flags >> 5) & 1;
    void* position = ShadowManager_FindAvailableShadowMap(size, size, prefs, &map, 1);
    if (position == nullptr) {
        map = ShadowManager_CreateShadowMap(size, size, prefs, 0, 3);
        if (map == nullptr && exactOnly != 0) {
            return nullptr;
        }
        position = ShadowManager_FindAvailableShadowMap(size, size, prefs, &map, exactOnly);
        if (position == nullptr) {
            return nullptr;
        }
    }
    ShadowGenerator_AssignShadowMap(generator, map, index, position, geometry);
    return map;
}

// 0x7CF790 vf12 ObtainAndAssignShadowCubeMap: the same with cube maps (the callback's answer
// taken only when it is one)
void* __fastcall ObtainAndAssignShadowCubeMapImpl(NiDefaultShadowClickGenerator*, re::Edx,
                                                  NiShadowGenerator* generator, uint32_t index, void* geometry) {
    using Retrieve = void*(__cdecl*)(NiShadowGenerator*, void*, uint32_t);
    void* found = re::PtrCast<Retrieve>(generator->callback30)(generator, geometry, index);
    re::Ptr32<void> map = nullptr;
    if (found != nullptr) {
        map = HasClass(Virtual<void*>(found, 0), kNiShadowCubeMapClassIndex) ? found : nullptr;
    }
    if (map != nullptr) {
        return map;
    }
    int size = generator->sizeHint;
    uint32_t* prefs = &At<uint32_t>(generator->technique, 0x38);
    uint8_t exactOnly = uint8_t(generator->flags >> 5) & 1;
    void* position = ShadowManager_FindAvailableShadowCubeMap(size, prefs, &map, 1);
    if (position == nullptr) {
        map = ShadowManager_CreateShadowCubeMap(size, prefs, 0, 3);
        if (map == nullptr && exactOnly != 0) {
            return nullptr;
        }
        position = ShadowManager_FindAvailableShadowCubeMap(size, prefs, &map, exactOnly);
        if (position == nullptr) {
            return nullptr;
        }
    }
    ShadowGenerator_AssignShadowMap(generator, map, index, position, geometry);
    return map;
}

// 0x7CF680 AddRenderClick(click): appended to the manager's render clicks
void __fastcall AddRenderClickImpl(NiDefaultShadowClickGenerator*, re::Edx, NiObject* click) {
    ObjectList* clicks = &At<ObjectList>(ms_pkShadowManager, kRenderClicks);
    NiTListNode<NiObject*>* node = Virtual<NiTListNode<NiObject*>*>(clicks, 1);
    node->element = click;
    node->next = nullptr;
    node->prev = clicks->tail;
    if (clicks->tail != nullptr) {
        clicks->tail->next = node;
        clicks->count++;
        clicks->tail = node;
        return;
    }
    clicks->count++;
    clicks->head = node;
    clicks->tail = node;
}

// 0x7CFB70 vf8 HandleSpotLight(generator, light, regenerateViews, updateShadowMaps): the map's
// render view gets the casters and a camera at the light looking down its cone (the frustum's
// half-width the tangent of the outer angle); with the generator's auto near / far flags
// (0x40, 0x80) the planes fit the casters' bounds (the nearest less 15%, at least 1; the
// farthest plus 10%)
uint8_t __fastcall HandleSpotLightImpl(NiDefaultShadowClickGenerator* self, re::Edx, NiShadowGenerator* generator,
                                       NiAVObject* light, int regenerateViews, int updateShadowMaps) {
    void* map = Virtual<void*>(self, 11, generator, 0u, static_cast<void*>(nullptr));
    if (map == nullptr) {
        return 0;
    }
    NiObject* click = At<NiObject*>(map, 0xC);
    NiObject* view = FirstRenderView(click);
    NiAVObject* camera = At<NiAVObject*>(view, 0x20);
    if (re::Flag(regenerateViews)) {
        ObjectList* scenes = Scenes(view);
        ObjectList_RemoveAll(scenes);
        ShadowGenerator_GetCasterGeometryList(generator, &self->casters);
        float nearPlane = generator->unknown58;
        float farPlane = generator->unknown5C;
        NiTListNode<NiObject*>* node = self->casters.head;
        if ((generator->flags & 0xC0) == 0) {
            for (; node != nullptr; node = node->next) {
                AddScene(scenes, node->element);
            }
        } else {
            float minNear = 0.0f;
            float maxFar = 0.0f;
            if (node != nullptr) {
                NiObject* geometry = node->element;
                node = node->next;
                RenderView_AppendScene(view, geometry);
                NiPoint3 d = Offset(geometry, light);
                NiPoint3 v = d;
                minNear = Vector_Magnitude(&v) - Radius(geometry);
                v = d;
                maxFar = Vector_Magnitude(&v) + Radius(geometry);
                for (; node != nullptr; node = node->next) {
                    geometry = node->element;
                    d = Offset(geometry, light);
                    float lengthSquared = d.y * d.y + d.x * d.x + d.z * d.z;
                    float length = crt::Sqrtf(lengthSquared);
                    float geometryNear = length - Radius(geometry);
                    lengthSquared = d.y * d.y + d.x * d.x + d.z * d.z;
                    length = crt::Sqrtf(lengthSquared);
                    float geometryFar = length + Radius(geometry);
                    if (!(geometryNear > minNear)) {
                        minNear = geometryNear;
                    }
                    if (!(geometryFar < maxFar)) {
                        maxFar = geometryFar;
                    }
                    AddScene(scenes, geometry);
                }
                if (minNear < 1.0f) {
                    minNear = 1.0f;
                }
            }
            uint16_t flags = generator->flags;
            if (((uint8_t(flags) >> 6) & 1) != 0) {
                nearPlane = minNear * 0.85f;
                generator->unknown58 = nearPlane;
            }
            if (((uint8_t(flags) >> 7) & 1) != 0) {
                farPlane = maxFar * 1.1f;
                generator->unknown5C = farPlane;
            }
        }
        float angle = At<float>(light, 0x13C) * 3.14159274f / 180.0f;
        float t = crt::Tanf(angle);
        NiFrustum frustum;
        Frustum_Construct(&frustum, -t, t, t, -t, nearPlane, farPlane, 0);
        Camera_SetViewFrustum(camera, &frustum);
    }
    camera->local.translate = light->world.translate;
    camera->local.rotate = light->world.rotate;
    camera->local.scale = std::fabs(light->world.scale);
    AVObject_Update(camera, 0.0f, 1);
    NiRefObject* processor = At<NiRefObject*>(ms_pkShadowManager, kSpotSwapProcessor);
    AddRefImport(processor);
    ViewRenderClick_SetProcessor(click, processor, nullptr);
    ReleaseRef(processor);
    if (re::Flag(updateShadowMaps)) {
        ClickGenerator_AddRenderClick(self, click);
        void* technique = generator->technique;
        Virtual<void>(technique, 3, map);
        Virtual<void>(technique, 2, map, static_cast<void*>(self));
    }
    generator->flags &= 0xFFFD;
    return 1;
}

// 0x7D0060 vf9 HandlePointLight: the same for the cube map's six faces (90 degree frustums,
// the camera at the light looking along each axis); the auto near plane is 1, the far plane
// the farthest caster plus 10% (over all six faces)
uint8_t __fastcall HandlePointLightImpl(NiDefaultShadowClickGenerator* self, re::Edx, NiShadowGenerator* generator,
                                        NiAVObject* light, int regenerateViews, int updateShadowMaps) {
    void* cube = Virtual<void*>(self, 12, generator, 0u, static_cast<void*>(nullptr));
    if (cube == nullptr) {
        return 0;
    }
    float nearPlane = generator->unknown58;
    float farPlane = generator->unknown5C;
    for (uint32_t face = 0; face < 6; face++) {
        NiObject* click = At<NiObject*>(cube, 0x10 + uint8_t(face) * 4);
        NiObject* view = FirstRenderView(click);
        NiAVObject* camera = At<NiAVObject*>(view, 0x20);
        if (re::Flag(regenerateViews)) {
            float zero = 0.0f;
            At<float>(click, 0x38) = zero;   // the background color
            At<float>(click, 0x3C) = zero;
            At<float>(click, 0x40) = zero;
            At<float>(click, 0x44) = zero;
            ObjectList* scenes = Scenes(view);
            ObjectList_RemoveAll(scenes);
            ShadowGenerator_GetCasterGeometryList(generator, &self->casters);
            NiTListNode<NiObject*>* node = self->casters.head;
            if ((generator->flags & 0xC0) == 0) {
                for (; node != nullptr; node = node->next) {
                    AddScene(scenes, node->element);
                }
            } else {
                float maxFar = 0.0f;
                if (node != nullptr) {
                    NiObject* geometry = node->element;
                    node = node->next;
                    RenderView_AppendScene(view, geometry);
                    NiPoint3 d = Offset(geometry, light);
                    float lengthSquared = d.x * d.x + d.y * d.y + d.z * d.z;
                    maxFar = crt::Sqrtf(lengthSquared) + Radius(geometry);
                    for (; node != nullptr; node = node->next) {
                        geometry = node->element;
                        d = Offset(geometry, light);
                        lengthSquared = d.x * d.x + d.y * d.y + d.z * d.z;
                        float geometryFar = crt::Sqrtf(lengthSquared) + Radius(geometry);
                        if (maxFar < geometryFar) {
                            maxFar = geometryFar;
                        }
                        AddScene(scenes, geometry);
                    }
                }
                if (face == 0) {
                    uint16_t flags = generator->flags;
                    if (((uint8_t(flags) >> 6) & 1) != 0) {
                        nearPlane = 1.0f;
                    }
                    if (((uint8_t(flags) >> 7) & 1) != 0) {
                        farPlane = maxFar * 1.1f;
                    }
                } else if (((uint8_t(generator->flags) >> 7) & 1) != 0) {
                    float faceFar = maxFar * 1.1f;
                    if (!(faceFar < farPlane)) {
                        farPlane = faceFar;
                    }
                }
            }
        }
        camera->local.translate = light->world.translate;
        AVObject_Update(camera, 0.0f, 1);
        NiPoint3 position = light->world.translate;
        NiPoint3 up;
        switch (face) {
        case 0:
            position.x = position.x + 1.0f;
            position.y = position.y + 0.0f;
            position.z = 0.0f + position.z;
            up = {0.0f, 1.0f, 0.0f};
            break;
        case 1:
            position.x = position.x - 1.0f;
            position.y = position.y + 0.0f;
            position.z = 0.0f + position.z;
            up = {0.0f, 1.0f, 0.0f};
            break;
        case 2:
            position.x = position.x + 0.0f;
            position.y = position.y + 1.0f;
            position.z = 0.0f + position.z;
            up = {0.0f, 0.0f, 1.0f};
            break;
        case 3:
            position.x = position.x + 0.0f;
            position.y = position.y - 1.0f;
            position.z = 0.0f + position.z;
            up = {0.0f, 0.0f, -1.0f};
            break;
        case 4:
            position.x = position.x + 0.0f;
            position.y = 0.0f + position.y;
            position.z = position.z - 1.0f;
            up = {0.0f, 1.0f, 0.0f};
            break;
        default:   // 5
            position.x = position.x + 0.0f;
            position.y = 0.0f + position.y;
            position.z = position.z + 1.0f;
            up = {0.0f, 1.0f, 0.0f};
            break;
        }
        Camera_LookAtWorldPoint(camera, &position, &up);
        camera->local.scale = std::fabs(light->world.scale);
        AVObject_Update(camera, 0.0f, 1);
        // the click's processor the manager's point light material swap (NiPointer assigned)
        NiRefObject* processor = At<NiRefObject*>(ms_pkShadowManager, kPointSwapProcessor);
        AddRefImport(processor);
        re::Ptr32<NiRefObject>& slot = At<NiRefObject*>(click, 0x5C);
        if (slot != processor) {
            ReleaseRef(slot);
            slot = processor;
            AddRefImport(processor);
        }
        At<void*>(click, 0x60) = nullptr;
        ReleaseRef(processor);
        if (re::Flag(updateShadowMaps)) {
            ObjectList* clicks = &At<ObjectList>(ms_pkShadowManager, kRenderClicks);
            NiTListNode<NiObject*>* added = Virtual<NiTListNode<NiObject*>*>(clicks, 1);
            added->element = click;
            added->next = nullptr;
            added->prev = clicks->tail;
            if (clicks->tail != nullptr) {
                clicks->tail->next = added;
            } else {
                clicks->head = added;
            }
            clicks->count++;
            clicks->tail = added;
        }
    }
    if (re::Flag(regenerateViews)) {
        uint16_t flags = generator->flags;
        if (((uint8_t(flags) >> 6) & 1) != 0) {
            generator->unknown58 = nearPlane;
        }
        if (((uint8_t(flags) >> 7) & 1) != 0) {
            generator->unknown5C = farPlane;
        }
        for (uint32_t face = 0; face < 6; face++) {
            NiObject* click = At<NiObject*>(cube, 0x10 + uint8_t(face) * 4);
            NiAVObject* camera = At<NiAVObject*>(FirstRenderView(click), 0x20);
            NiFrustum frustum;
            Frustum_Construct(&frustum, -1.0f, 1.0f, 1.0f, -1.0f, nearPlane, farPlane, 0);
            Camera_SetViewFrustum(camera, &frustum);
        }
    }
    if (re::Flag(updateShadowMaps)) {
        void* technique = generator->technique;
        Virtual<void>(technique, 3, cube);
        Virtual<void>(technique, 2, cube, static_cast<void*>(self));
    }
    generator->flags &= 0xFFFD;
    return 1;
}

// 0x7CF610 NiViewRenderClick::SetProcessor(processor, data)
void __fastcall ViewRenderClick_SetProcessorImpl(NiObject* self, re::Edx, NiRefObject* processor, void* data) {
    re::Ptr32<NiRefObject>& slot = At<NiRefObject*>(self, 0x5C);
    if (slot == processor) {
        At<void*>(self, 0x60) = data;
        return;
    }
    ReleaseRef(slot);
    slot = processor;
    AddRefImport(processor);
    At<void*>(self, 0x60) = data;
}

// 0x7CFB00 Ni3DRenderView::AppendScene(scene)
void __fastcall RenderView_AppendSceneImpl(NiObject* self, re::Edx, NiObject* scene) {
    AddScene(Scenes(self), scene);
}

// ---- NiShadowTechnique

// 0x7D18F0 NiShadowTechnique(name, a0, a1, a2, b0, b1, b2, arg8, arg9): the six fragment names
// by value (let go here)
NiShadowTechnique* __fastcall ShadowTechnique_ConstructImpl(NiShadowTechnique* self, re::Edx, const re::Ptr32<char>* name,
                                                            re::Ptr32<char> a0, re::Ptr32<char> a1, re::Ptr32<char> a2, re::Ptr32<char> b0, re::Ptr32<char> b1,
                                                            re::Ptr32<char> b2, int arg8, int arg9) {
    self->vtable = reinterpret_cast<void*>(kNiShadowTechniqueVtable);
    self->name = kNullFixedString;
    self->fragmentsA[0] = kNullFixedString;
    self->fragmentsA[1] = kNullFixedString;
    self->fragmentsA[2] = kNullFixedString;
    self->fragmentsB[0] = kNullFixedString;
    self->fragmentsB[1] = kNullFixedString;
    self->fragmentsB[2] = kNullFixedString;
    self->formatPrefs[0] = 6;
    self->formatPrefs[1] = 3;
    self->formatPrefs[2] = 2;
    self->unknown48 = uint8_t(arg9);
    self->unknown44 = 0x10;
    self->unknown46 = 0x10;
    self->unknown49 = uint8_t(arg8);
    FixedString_Assign(&self->name, name);
    self->formatPrefs[0] = 9;
    self->formatPrefs[2] = 0;
    for (int i = 0; i < 6; i++) {
        self->unknown20[i] = 0.0f;
    }
    char* values[6] = {a0, a1, a2, b0, b1, b2};
    re::Ptr32<char>* slots[6] = {&self->fragmentsA[0], &self->fragmentsA[1], &self->fragmentsA[2],
                       &self->fragmentsB[0], &self->fragmentsB[1], &self->fragmentsB[2]};
    for (int i = 0; i < 6; i++) {
        if (*slots[i] != values[i]) {
            char* held = FixedStringAddRef(values[i]);
            FixedStringRelease(slots[i]);
            *slots[i] = held;
        }
    }
    FixedStringRelease(&a0);
    FixedStringRelease(&a1);
    FixedStringRelease(&a2);
    FixedStringRelease(&b0);
    FixedStringRelease(&b1);
    FixedStringRelease(&b2);
    return self;
}

// 0x7D0F40
void __fastcall ShadowTechnique_DestructImpl(NiShadowTechnique* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiShadowTechniqueVtable);
    for (int i = 2; i >= 0; i--) {
        FixedStringRelease(&self->fragmentsB[i]);
    }
    for (int i = 2; i >= 0; i--) {
        FixedStringRelease(&self->fragmentsA[i]);
    }
    FixedStringRelease(&self->name);
}

// 0x7D1020 vf0
const RTTI* __fastcall ShadowTechnique_GetRTTIImpl(NiShadowTechnique*, re::Edx) {
    return &NiShadowTechnique_RTTI;
}

// 0x7D1030 vf3 PrepareShadowMap(map): the map's filter mode nearest (its flags' bits 8..11)
void __fastcall ShadowTechnique_PrepareShadowMapImpl(NiShadowTechnique*, re::Edx, uint8_t* map) {
    At<uint16_t>(map, 4) &= 0xF0FF;
}

// 0x7D1040 vf1
NiShadowTechnique* __fastcall ShadowTechnique_DeletingDestructorImpl(NiShadowTechnique* self, re::Edx,
                                                                     uint32_t flags) {
    ShadowTechnique_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x4C);
    }
    return self;
}

// 0x7D1070 NiVSMShadowTechnique (same arguments; copies of the names handed on)
NiVSMShadowTechnique* __fastcall VSMShadowTechnique_ConstructImpl(NiVSMShadowTechnique* self, re::Edx,
                                                                  const re::Ptr32<char>* name, re::Ptr32<char> a0, re::Ptr32<char> a1, re::Ptr32<char> a2,
                                                                  re::Ptr32<char> b0, re::Ptr32<char> b1, re::Ptr32<char> b2, int arg8,
                                                                  int arg9) {
    char* cb2 = FixedStringAddRef(b2);
    char* cb1 = FixedStringAddRef(b1);
    char* cb0 = FixedStringAddRef(b0);
    char* ca2 = FixedStringAddRef(a2);
    char* ca1 = FixedStringAddRef(a1);
    char* ca0 = FixedStringAddRef(a0);
    ShadowTechnique_Construct(self, name, ca0, ca1, ca2, cb0, cb1, cb2, arg8, arg9);
    self->unknown54 = 512.0f;
    self->unknown58 = 512.0f;
    self->unknown4C = 0;
    self->unknown4D = 0;
    self->vtable = reinterpret_cast<void*>(kNiVSMShadowTechniqueVtable);
    self->unknown50 = 4;
    self->unknown5C[0] = 0;
    self->unknown5C[1] = 0;
    self->unknown5C[2] = 0;
    self->unknown5C[3] = 0;
    FixedStringRelease(&a0);
    FixedStringRelease(&a1);
    FixedStringRelease(&a2);
    FixedStringRelease(&b0);
    FixedStringRelease(&b1);
    FixedStringRelease(&b2);
    return self;
}

// 0x7D0E60 (4 bytes, folded with the shaders' pass getter: +0x50, likely NiD3DPass::GetPixelShader;
// its callers are the D3D shaders', next to 0x51A7B0 for +0x4C)
uint32_t __fastcall Get50(uint8_t* self, re::Edx) {
    return At<uint32_t>(self, 0x50);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIDEFSHADOWCLICK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIDEFSHADOWCLICK_REVERSED("NiTPrimitiveArray<float>::SetAt", 0x007CF000, ni::FloatArray_SetAtImpl);
NIDEFSHADOWCLICK_REVERSED("NiTPointerMap<void*,float>::SetValue", 0x007CF080, ni::Map_SetValueImpl);
NIDEFSHADOWCLICK_REVERSED("NiTPointerMap<void*,float>::NewItem", 0x007CF0A0, ni::Map_NewItemImpl);
NIDEFSHADOWCLICK_REVERSED("NiTPointerMap<void*,float>::DeleteItem", 0x007CF0B0, ni::Map_DeleteItemImpl);
NIDEFSHADOWCLICK_REVERSED("NiTMapBase<void*,float>::DeletingDestructor", 0x007CF0D0,
                          ni::MapBase_DeletingDestructorImpl);
NIDEFSHADOWCLICK_REVERSED("NiTPointerMap<void*,float>::DeletingDestructor", 0x007CF110,
                          ni::Map_DeletingDestructorImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::GenerateRenderClicks", 0x007CF150,
                          ni::GenerateRenderClicksImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::ObtainAndAssignShadowMap", 0x007CF290,
                          ni::ObtainAndAssignShadowMapImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::PreparePointLightShadowGenerator", 0x007CF350,
                          ni::PreparePointLightShadowGeneratorImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::PrepareSpotLightShadowGenerator", 0x007CF390,
                          ni::PrepareShadowGeneratorImpl);
NIDEFSHADOWCLICK_REVERSED("NiTPrimitiveArray<float>::SetSize", 0x007CF4F0, ni::FloatArray_SetSizeImpl);
NIDEFSHADOWCLICK_REVERSED("NiViewRenderClick::SetProcessor", 0x007CF610, ni::ViewRenderClick_SetProcessorImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::AddRenderClick", 0x007CF680, ni::AddRenderClickImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::~NiDefaultShadowClickGenerator", 0x007CF6D0,
                          ni::DestructImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::GetRTTI", 0x007CF780, ni::GetRTTIImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::ObtainAndAssignShadowCubeMap", 0x007CF790,
                          ni::ObtainAndAssignShadowCubeMapImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::ReorganizeActiveShadowGenerators", 0x007CF880,
                          ni::ReorganizeActiveShadowGeneratorsImpl);
NIDEFSHADOWCLICK_REVERSED("Ni3DRenderView::AppendScene", 0x007CFB00, ni::RenderView_AppendSceneImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::DeletingDestructor", 0x007CFB50,
                          ni::DeletingDestructorImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::HandleSpotLight", 0x007CFB70, ni::HandleSpotLightImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::HandlePointLight", 0x007D0060, ni::HandlePointLightImpl);
NIDEFSHADOWCLICK_REVERSED("NiDefaultShadowClickGenerator::NiDefaultShadowClickGenerator", 0x007D0D30,
                          ni::ConstructImpl);
// 0x007D0E60: [8b 41 50 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_007D0E60(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x50);
}
RE_UNHOOKED_FN("NiD3DPass::GetPixelShader (+0x50)", 0x007D0E60, &Tiny_007D0E60);   // 4 bytes
NIDEFSHADOWCLICK_REVERSED("NiShadowTechnique::~NiShadowTechnique", 0x007D0F40, ni::ShadowTechnique_DestructImpl);
NIDEFSHADOWCLICK_REVERSED("NiShadowTechnique::GetRTTI", 0x007D1020, ni::ShadowTechnique_GetRTTIImpl);
NIDEFSHADOWCLICK_REVERSED("NiShadowTechnique::PrepareShadowMap", 0x007D1030,
                          ni::ShadowTechnique_PrepareShadowMapImpl);
NIDEFSHADOWCLICK_REVERSED("NiShadowTechnique::DeletingDestructor", 0x007D1040,
                          ni::ShadowTechnique_DeletingDestructorImpl);
NIDEFSHADOWCLICK_REVERSED("NiVSMShadowTechnique::NiVSMShadowTechnique", 0x007D1070,
                          ni::VSMShadowTechnique_ConstructImpl);
NIDEFSHADOWCLICK_REVERSED("NiShadowTechnique::NiShadowTechnique", 0x007D18F0, ni::ShadowTechnique_ConstructImpl);
