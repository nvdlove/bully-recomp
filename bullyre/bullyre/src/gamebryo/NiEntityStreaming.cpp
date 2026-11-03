// The end of the entity library (0x856380..0x857030): NiEntityStreaming's own code (its
// constructor, InsertScene, the link IDs every entity, component and entity-pointer property
// of the scenes gets before a save), the external asset params (NiExternalAssetParams and its
// KF / KFM / NIF kinds: the asset path, and per kind the sequence, the cumulative flag or the
// scene root), NiGeneralComponentProperty's destructor and the base64 coder NiDOMTool writes
// binary data with.
//
// The params (no NiRTTI: slot 0 returns a static the handlers test a class bit of, slot 1 the
// deleting destructor): NiExternalAssetParams 8 bytes (vtable 0x98C2E0: +4 the path),
// NiExternalAssetKFParams 0x10 (0x98C2CC: +8 the sequence index, +0xC the sequence, held),
// NiExternalAssetKFMParams 0x10 (0x98C300: +8 cumulative, +0xC the actor manager),
// NiExternalAssetNIFParams 0xC
// (0x98C314: +8 the scene root, held).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntity.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::entity {
namespace {

using ni::sys::Decrement;
using ni::sys::FixedStringRelease;
using ni::sys::Increment;

constexpr uintptr_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t kStreamingVtable = 0x0098C2B4;
constexpr uintptr_t kSceneArrayVtable = 0x009893C0;      // NiTObjectArray<NiScenePtr> (folded)
constexpr uintptr_t kParamsVtable = 0x0098C2E0;
constexpr uintptr_t kKFParamsVtable = 0x0098C2CC;
constexpr uintptr_t kKFMParamsVtable = 0x0098C300;
constexpr uintptr_t kNIFParamsVtable = 0x0098C314;
constexpr uintptr_t kGeneralPropertyVtable = 0x0098AEB4;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;  // NiTPrimitiveArray (folded)
GAME_VAR(uint32_t, NiRefObject_ms_uiObjects, 0x020E2168);

// the params' class statics (what slot 0 returns)
inline void* const kKFParamsRTTI = reinterpret_cast<void*>(0x020E5F34);
inline void* const kParamsRTTI = reinterpret_cast<void*>(0x020E5F68);
inline void* const kKFMParamsRTTI = reinterpret_cast<void*>(0x020E5F9C);
inline void* const kNIFParamsRTTI = reinterpret_cast<void*>(0x020E5FD0);

// base64 (NiDOMTool's): the alphabet and the decoding table (0xC0 bits: not a digit)
GAME_ARRAY(const char, kBase64Alphabet, 0x0098C328);
GAME_ARRAY(const uint8_t, kBase64Decode, 0x0098C370);

GAME_FN(void, __cdecl, PointerArray_Delete, 0x00836F80, void* base);   // NiDelete[] of the elements
GAME_FN(void, __thiscall, ObjectArray_SetSize, 0x00856170, void* array, uint32_t size);
GAME_FN(void, __thiscall, ObjectArray_SetAt, 0x0040E800, void* array, uint32_t index, const re::Ptr32<NiRefObject>* value);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, PointerMap* self, void* key, uintptr_t value);
// NiTMap<unsigned int, NiEntityPropertyInterfaceIPtr>::SetAt (the pointer by value: released by it)
GAME_FN(void, __thiscall, IDMap_SetAt, 0x00855F60, PointerMap* self, uint32_t id, void* object);
GAME_FN(void, __thiscall, FixedStringArray_Delete, 0x0075B1B0, void* base, int flags);   // (vector deleting destructor)
GAME_FN(uint8_t, __cdecl, Path_IsUniqueAbsolute, 0x00752C90, const char* path);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, size_t size);

// this file's (called where modes apply)
GAME_FN(void, __thiscall, Streaming_AssignPropertyLinkIDs, 0x00856470, void* self, LinkIDMap* links, void* object);
GAME_FN(void, __thiscall, Streaming_AssignComponentLinkIDs, 0x00856630, void* self, LinkIDMap* links,
        void* component);
GAME_FN(void, __thiscall, Streaming_AssignEntityLinkIDs, 0x008566B0, void* self, LinkIDMap* links, void* entity);
GAME_FN(void, __thiscall, Params_Construct, 0x00856990, void* self);
GAME_FN(void, __thiscall, Params_Destruct, 0x008569C0, void* self);
GAME_FN(void, __thiscall, KFParams_Destruct, 0x008568C0, void* self);
GAME_FN(void, __thiscall, NIFParams_Destruct, 0x00856B90, void* self);
GAME_FN(uint8_t, __cdecl, Base64_DecodeQuantum, 0x00856EF0, uint32_t* value, const char* text, int count);

struct Params {
    re::Ptr32<void> vtable;           // +0x0
    re::Ptr32<const char> path;     // +0x4
};
static_assert(sizeof(Params) == 8);

struct KFParams : Params {
    uint32_t sequenceIndex;    // +0x8 (not set by the constructor)
    re::Ptr32<NiRefObject> sequence;     // +0xC (NiControllerSequencePtr)
};
static_assert(sizeof(KFParams) == 0x10);

struct KFMParams : Params {
    uint8_t cumulative;     // +0x8
    uint8_t pad9[3];
    re::Ptr32<NiRefObject> actorManager;   // +0xC (not set or released here: the handler's, 0x853140 reads it)
};
static_assert(sizeof(KFMParams) == 0x10);   // (the deleting destructor frees 0x10)

struct NIFParams : Params {
    re::Ptr32<NiRefObject> sceneRoot;    // +0x8 (NiAVObjectPtr)
};
static_assert(sizeof(NIFParams) == 0xC);

// NiGeneralComponentProperty (NiGeneralComponent.cpp's layout)
struct GeneralProperty : NiRefObject {
    re::Ptr32<const char> name;            // +0x8
    re::Ptr32<const char> displayName;     // +0xC
    re::Ptr32<const char> primitiveType;   // +0x10
    re::Ptr32<const char> semanticType;    // +0x14
    re::Ptr32<const char> description;     // +0x18
    uint8_t collection;            // +0x1C
    uint8_t pad1D[3];
    NiTArray<void*> data;          // +0x20
    NiTArray<uint32_t> sizes;      // +0x30
};
static_assert(offsetof(GeneralProperty, data) == 0x20);
static_assert(sizeof(GeneralProperty) == 0x40);

// an NiScene as the link ID pass reads it: its entities (an NiTObjectSet at +0xC: +0x14 the
// count) and its selection sets (at +0x28: +0x30 the count; each with its entities at +0xC / +0x14)
inline uint32_t SetCount(const void* owner, int offset) {
    return At<uint32_t>(owner, offset + 8);
}
inline void* SetAt(const void* owner, int offset, uint32_t index) {
    return At<re::Ptr32<void>*>(owner, offset)[index];
}

// the NiPointer release the code inlines
inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && Decrement(&object->refCount) == 0) {
        Virtual<void>(object, 1);
    }
}

// the object's own link ID, when it hasn't one yet (the pointer map's lookup inlined); then
// its master's
inline bool HasLinkID(LinkIDMap* links, void* object) {
    PointerMap& map = links->objectToID;
    for (MapItem* item = map.table[reinterpret_cast<uint32_t>(object) % map.hashSize]; item != nullptr;
         item = item->next) {
        if (item->key == object) {
            return true;
        }
    }
    return false;
}
inline void AddLinkID(LinkIDMap* links, void* object) {
    uint32_t id = links->idToObject.count;
    if (object != nullptr) {
        Virtual<void>(object, prop::AddReference);   // (the map's argument by value)
    }
    IDMap_SetAt(&links->idToObject, id, object);
    Map_SetAt(&links->objectToID, object, id);
}

} // namespace

// ---- NiEntityStreaming

// 0x856380 (thiscall): NiTObjectArray<NiPointer>'s deleting destructor (the elements released
// and the block freed)
void* __fastcall SceneArray_Destroy(NiTArray<NiRefObject*>* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kSceneArrayVtable);
    PointerArray_Delete(self->base);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x8563B0 (thiscall): NiEntityStreaming's constructor (no scenes, no error handler)
NiEntityStreaming* __fastcall Streaming_ConstructImpl(NiEntityStreaming* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    self->refCount = 0;
    Increment(&NiRefObject_ms_uiObjects);
    self->vtable = reinterpret_cast<void*>(kStreamingVtable);
    self->scenes.maxSize = 0;
    self->scenes.size = 0;
    self->scenes.esize = 0;
    self->scenes.base = nullptr;
    self->scenes.growBy = 1;
    self->scenes.vtable = reinterpret_cast<void*>(kSceneArrayVtable);
    self->errorHandler = nullptr;
    return self;
}

// 0x856400 (thiscall): NiEntityStreaming::InsertScene (m_kScenes.Add through a temporary
// NiScenePtr)
void __fastcall Streaming_InsertSceneImpl(NiEntityStreaming* self, re::Edx, NiRefObject* scene) {
    re::Ptr32<NiRefObject> held = scene;
    AddRef(held);
    uint32_t index = self->scenes.size;
    if (index >= self->scenes.maxSize) {
        ObjectArray_SetSize(&self->scenes, self->scenes.growBy + index);
    }
    ObjectArray_SetAt(&self->scenes, index, &held);
    ReleaseRef(scene);
}

// 0x856470 (thiscall): the entities an object's serializable entity-pointer properties point
// at get their link IDs (GetPropertyNames, then per name GetPrimitiveType ==
// PT_ENTITYPOINTER, IsPropertySerializable, GetElementCount, GetPropertyData(entity, index))
void __fastcall Streaming_AssignPropertyLinkIDsImpl(void* self, re::Edx, LinkIDMap* links, void* object) {
    NiTSet<NiFixedString> names = {nullptr, 0, 0};
    Virtual<void>(object, prop::GetPropertyNames, &names);
    uint32_t count = names.used;
    for (uint32_t i = 0; i < count; i++) {
        const re::Ptr32<const char>* name = &names.base[i];
        NiFixedString type = ni::sys::kNullFixedString;
        if (Virtual<uint8_t>(object, prop::GetPrimitiveType, name, &type) != 0 && type == PT_ENTITYPOINTER) {
            // (the game keeps the flag in its `object` argument's stack slot)
            uint8_t serializable = 0;
            if (Virtual<uint8_t>(object, prop::IsPropertySerializable, name, &serializable) != 0 &&
                serializable != 0) {
                uint32_t elements;
                Virtual<uint8_t>(object, prop::GetElementCount, name, &elements);
                for (uint32_t index = 0; index < elements; index++) {
                    void* entity = nullptr;
                    Virtual<uint8_t>(object, prop::GetEntityPointer, name, &entity, index);
                    if (entity != nullptr) {
                        Streaming_AssignEntityLinkIDs(self, links, entity);
                    }
                }
            }
        }
        FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&type));
    }
    if (names.base != nullptr) {
        FixedStringArray_Delete(names.base, 3);   // (the set's elements freed)
    }
}

// 0x856630 (thiscall): a component's link ID (once), its master's, then its entity pointers'
void __fastcall Streaming_AssignComponentLinkIDsImpl(void* self, re::Edx, LinkIDMap* links, void* component) {
    if (HasLinkID(links, component)) {
        return;
    }
    AddLinkID(links, component);
    void* master = Virtual<void*>(component, comp::GetMasterComponent);
    if (master != nullptr) {
        Streaming_AssignComponentLinkIDs(self, links, master);
    }
    Streaming_AssignPropertyLinkIDs(self, links, component);
}

// 0x8566B0 (thiscall): an entity's link ID (once), its master's, its components', then its
// entity pointers'
void __fastcall Streaming_AssignEntityLinkIDsImpl(void* self, re::Edx, LinkIDMap* links, void* entity) {
    if (HasLinkID(links, entity)) {
        return;
    }
    AddLinkID(links, entity);
    void* master = Virtual<void*>(entity, ent::GetMasterEntity);
    if (master != nullptr) {
        Streaming_AssignEntityLinkIDs(self, links, master);
    }
    uint32_t count = Virtual<uint32_t>(entity, ent::GetComponentCount);
    for (uint32_t i = 0; i < count; i++) {
        void* component = Virtual<void*>(entity, ent::GetComponentAt, i);
        if (component != nullptr) {
            Streaming_AssignComponentLinkIDs(self, links, component);
        }
    }
    Streaming_AssignPropertyLinkIDs(self, links, entity);
}

// 0x856770 (thiscall): every scene's entities get link IDs: its selection sets' first, then
// its own
void __fastcall Streaming_AssignUniqueLinkIDsImpl(NiEntityStreaming* self, re::Edx, LinkIDMap* links) {
    uint32_t scenes = self->scenes.size;
    for (uint32_t s = 0; s < scenes; s++) {
        void* scene = self->scenes.base[s];
        if (scene == nullptr) {
            continue;
        }
        uint32_t sets = SetCount(scene, 0x28);
        for (uint32_t i = 0; i < sets; i++) {
            void* set = SetAt(scene, 0x28, i);
            uint32_t entities = SetCount(set, 0xC);
            for (uint32_t e = 0; e < entities; e++) {
                Streaming_AssignEntityLinkIDs(self, links, SetAt(set, 0xC, e));
            }
        }
        uint32_t entities = SetCount(scene, 0xC);
        for (uint32_t e = 0; e < entities; e++) {
            void* entity = SetAt(scene, 0xC, e);
            if (entity != nullptr) {
                Streaming_AssignEntityLinkIDs(self, links, entity);
            }
        }
    }
}

// 0x856840 (thiscall): the +8 field set (folded: NiAccumulator::StartAccumulating's camera,
// NiExternalAssetKFParams::SetSequenceIndex, and many more)
void __fastcall Folded_SetField8(void* self, re::Edx, uint32_t value) {
    At<uint32_t>(self, 8) = value;
}

// 0x856850 (thiscall): the +8 field read (folded likewise: 4 bytes, not hooked)
uint32_t __fastcall Folded_GetField8(void* self, re::Edx) {
    return At<uint32_t>(self, 8);
}
RE_UNHOOKED_FN("Folded::GetField8", 0x00856850, &Folded_GetField8);

// ---- the external asset params

// 0x856990 (thiscall): NiExternalAssetParams's constructor (the path assigned null)
Params* __fastcall Params_ConstructImpl(Params* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kParamsVtable);
    self->path = ni::sys::kNullFixedString;
    FixedString_AssignText(&self->path, nullptr);
    return self;
}

// 0x8569B0, 0x8568B0, 0x856AE0, 0x856B80: slot 0 of each kind (its class static)
void* __fastcall Params_GetRTTI(Params*, re::Edx) {
    return kParamsRTTI;
}
void* __fastcall KFParams_GetRTTI(Params*, re::Edx) {
    return kKFParamsRTTI;
}
void* __fastcall KFMParams_GetRTTI(Params*, re::Edx) {
    return kKFMParamsRTTI;
}
void* __fastcall NIFParams_GetRTTI(Params*, re::Edx) {
    return kNIFParamsRTTI;
}

// 0x8569C0 (thiscall): ~NiExternalAssetParams
void __fastcall Params_DestructImpl(Params* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kParamsVtable);
    FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->path));
}

// 0x856A00 (thiscall): NiExternalAssetParams::SetAssetPath (only a unique absolute path)
uint8_t __fastcall Params_SetAssetPath(Params* self, re::Edx, const re::Ptr32<const char>* path) {
    if (Path_IsUniqueAbsolute(*path) == 0) {
        return 0;
    }
    FixedString_Assign(&self->path, path);
    return 1;
}

// 0x856A30 (thiscall): NiExternalAssetParams::GetAssetPath (a counted copy)
re::Ptr32<const char>* __fastcall Params_GetAssetPath(Params* self, re::Edx, re::Ptr32<const char>* result) {
    if (self->path != ni::sys::kNullFixedString) {
        Increment(re::PtrCast<char*>(self->path) - 8);
    }
    *result = self->path;
    return result;
}

// 0x856A60 (thiscall): NiExternalAssetParams's deleting destructor (its destructor inlined)
Params* __fastcall Params_Destroy(Params* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kParamsVtable);
    FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->path));
    if (flags & 1) {
        NiDelete(self, 8);
    }
    return self;
}

// 0x856870 (thiscall): NiExternalAssetKFParams's constructor (the sequence nulled twice: the
// NiPointer's constructor and the body's assignment)
KFParams* __fastcall KFParams_Construct(KFParams* self, re::Edx) {
    Params_Construct(self);
    self->vtable = reinterpret_cast<void*>(kKFParamsVtable);
    self->sequence = nullptr;
    NiRefObject* old = self->sequence;
    if (old != nullptr) {
        ReleaseRef(old);
        self->sequence = nullptr;
    }
    return self;
}

// 0x8568C0 (thiscall): ~NiExternalAssetKFParams
void __fastcall KFParams_DestructImpl(KFParams* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kKFParamsVtable);
    NiRefObject* old = self->sequence;
    if (old != nullptr) {
        ReleaseRef(old);
        self->sequence = nullptr;
    }
    ReleaseRef(self->sequence);   // (the NiPointer's destructor)
    Params_Destruct(self);
}

// 0x856860 (thiscall): NiExternalAssetKFParams::GetSequence (not counted)
uint8_t __fastcall KFParams_GetSequence(KFParams* self, re::Edx, re::Ptr32<NiRefObject>* sequence) {
    *sequence = self->sequence;
    return 1;
}

// 0x856920 (thiscall): NiExternalAssetKFParams::SetSequence
uint8_t __fastcall KFParams_SetSequence(KFParams* self, re::Edx, NiRefObject* sequence) {
    NiRefObject* old = self->sequence;
    if (old != sequence) {
        ReleaseRef(old);
        self->sequence = sequence;
        if (sequence != nullptr) {
            Increment(&sequence->refCount);
        }
    }
    return 1;
}

// 0x856970 (thiscall): NiExternalAssetKFParams's deleting destructor
KFParams* __fastcall KFParams_Destroy(KFParams* self, re::Edx, int flags) {
    KFParams_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x856AF0 (thiscall): NiExternalAssetKFMParams's constructor
KFMParams* __fastcall KFMParams_Construct(KFMParams* self, re::Edx, int cumulative) {
    Params_Construct(self);
    self->cumulative = uint8_t(cumulative);
    self->vtable = reinterpret_cast<void*>(kKFMParamsVtable);
    return self;
}

// 0x856AC0 (thiscall): ~NiExternalAssetKFMParams
void __fastcall KFMParams_Destruct(KFMParams* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kKFMParamsVtable);
    Params_Destruct(self);
}

// 0x856AD0 (thiscall): NiExternalAssetKFMParams::SetCumulative
void __fastcall KFMParams_SetCumulative(KFMParams* self, re::Edx, int cumulative) {
    self->cumulative = uint8_t(cumulative);
}

// 0x856B10 (thiscall): NiExternalAssetKFMParams's deleting destructor
KFMParams* __fastcall KFMParams_Destroy(KFMParams* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kKFMParamsVtable);
    Params_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x10);
    }
    return self;
}

// 0x856B40 (thiscall): NiExternalAssetNIFParams's constructor (the root nulled twice)
NIFParams* __fastcall NIFParams_Construct(NIFParams* self, re::Edx) {
    Params_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNIFParamsVtable);
    self->sceneRoot = nullptr;
    NiRefObject* old = self->sceneRoot;
    if (old != nullptr) {
        ReleaseRef(old);
        self->sceneRoot = nullptr;
    }
    return self;
}

// 0x856B90 (thiscall): ~NiExternalAssetNIFParams
void __fastcall NIFParams_DestructImpl(NIFParams* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNIFParamsVtable);
    NiRefObject* old = self->sceneRoot;
    if (old != nullptr) {
        ReleaseRef(old);
        self->sceneRoot = nullptr;
    }
    ReleaseRef(self->sceneRoot);
    Params_Destruct(self);
}

// 0x856BF0 (thiscall): NiExternalAssetNIFParams::GetSceneRoot (false without one; not counted)
uint8_t __fastcall NIFParams_GetSceneRoot(NIFParams* self, re::Edx, re::Ptr32<NiRefObject>* root) {
    NiRefObject* sceneRoot = self->sceneRoot;
    if (sceneRoot == nullptr) {
        return 0;
    }
    *root = sceneRoot;
    return 1;
}

// 0x856C10 (thiscall): NiExternalAssetNIFParams's deleting destructor
NIFParams* __fastcall NIFParams_Destroy(NIFParams* self, re::Edx, int flags) {
    NIFParams_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0xC);
    }
    return self;
}

// ---- NiGeneralComponentProperty

// 0x856C30 (thiscall): ~NiGeneralComponentProperty (each data block freed, then the two
// arrays', the five strings let go)
void __fastcall GeneralProperty_Destruct(GeneralProperty* self, re::Edx) {
    uint32_t count = self->data.size;
    self->vtable = reinterpret_cast<void*>(kGeneralPropertyVtable);
    for (uint32_t i = 0; i < count; i++) {
        NiFree(self->data.base[i]);
    }
    self->sizes.vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    NiFree(self->sizes.base);
    self->data.vtable = reinterpret_cast<void*>(kPrimitiveArrayVtable);
    NiFree(self->data.base);
    FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->description));
    FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->semanticType));
    FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->primitiveType));
    FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->displayName));
    FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->name));
    self->vtable = reinterpret_cast<void*>(kNiRefObjectVtable);
    Decrement(&NiRefObject_ms_uiObjects);
}

// ---- base64 (NiDOMTool's binary data). Its own variant: the last one or two bytes as two or
// three digits, no '=' padding

// 0x856D70: the first `size` bytes (at most `maxSize` when that isn't 0) as text (operator new)
char* __cdecl Base64_Encode(const uint8_t* data, int size, int maxSize) {
    int n = size;
    if (maxSize != 0 && size >= maxSize) {
        n = maxSize;
    }
    char* text = static_cast<char*>(OperatorNew(size_t(n * 4 / 3 + 3)));
    char* p = text + 3;
    int groups = n / 3;
    for (int i = 0; i < groups; i++) {
        uint32_t v = uint32_t(data[0]) << 16 | uint32_t(data[1]) << 8 | data[2];
        p[0] = kBase64Alphabet[v & 0x3F];
        p[-1] = kBase64Alphabet[(v >> 6) & 0x3F];
        p[-2] = kBase64Alphabet[(v >> 12) & 0x3F];
        p[-3] = kBase64Alphabet[(v >> 18) & 0x3F];
        data += 3;
        p += 4;
    }
    switch (n - groups * 3) {
    case 0:
        p -= 3;
        break;
    case 1: {
        uint32_t v = data[0];
        p[-2] = kBase64Alphabet[v & 0x3F];
        p[-3] = kBase64Alphabet[(v >> 6) & 0x3F];
        p[-1] = '\0';
        return text;
    }
    case 2: {
        uint32_t v = uint32_t(data[0]) << 8 | data[1];
        p[-1] = kBase64Alphabet[v & 0x3F];
        p[-2] = kBase64Alphabet[(v >> 6) & 0x3F];
        p[-3] = kBase64Alphabet[(v >> 12) & 0x3F];
        p[0] = '\0';
        return text;
    }
    default:   // (a negative size)
        break;
    }
    *p = '\0';
    return text;
}

// 0x856EF0: 2 to 4 digits shifted into *value, 6 bits each (false on anything else)
uint8_t __cdecl Base64_DecodeQuantumImpl(uint32_t* value, const char* text, int count) {
    if (uint32_t(count - 2) > 2) {
        return 0;
    }
    int i = 0;
    do {
        char c = *text++;
        if (c < 0) {
            return 0;
        }
        uint8_t digit = kBase64Decode[uint8_t(c)];
        if (digit & 0xC0) {
            return 0;
        }
        *value = *value << 6 | digit;
    } while (i++ < count - 1);
    return 1;
}

// 0x856F50: text back to bytes (*size counts them); false (and the first byte zeroed) on a bad
// digit. (The quantum keeps its earlier bits: only the low ones are written.)
uint8_t __cdecl Base64_Decode(uint8_t* data, const char* text, int* size) {
    *size = 0;
    uint8_t* p = data + 2;
    uint32_t value = 0;
    uint32_t length = 0;   // (strlen inlined)
    while (text[length] != '\0') {
        length++;
    }
    int groups = int(length >> 2);
    for (int i = 0; i < groups; i++) {
        if (Base64_DecodeQuantum(&value, text, 4) == 0) {
            *data = 0;
            return 0;
        }
        text += 4;
        p[0] = uint8_t(value);
        p[-1] = uint8_t(value >> 8);
        p[-2] = uint8_t(value >> 16);
        *size += 3;
        p += 3;
    }
    int rest = int(length & 3);
    if (rest != 0) {
        if (Base64_DecodeQuantum(&value, text, rest) == 0) {
            *data = 0;
            return 0;
        }
        if (rest == 2) {
            p[rest - 4] = uint8_t(value);
            *size += 1;
            return 1;
        }
        p[rest - 4] = uint8_t(value);
        p[rest - 5] = uint8_t(value >> 8);
        *size += 2;
    }
    return 1;
}

#define ENTSTREAM38_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

ENTSTREAM38_REVERSED("NiTObjectArray<NiPointer>::Destroy", 0x00856380, SceneArray_Destroy);
ENTSTREAM38_REVERSED("NiEntityStreaming::NiEntityStreaming", 0x008563B0, Streaming_ConstructImpl);
ENTSTREAM38_REVERSED("NiEntityStreaming::InsertScene", 0x00856400, Streaming_InsertSceneImpl);
ENTSTREAM38_REVERSED("NiEntityStreaming::AssignPropertyLinkIDs", 0x00856470, Streaming_AssignPropertyLinkIDsImpl);
ENTSTREAM38_REVERSED("NiEntityStreaming::AssignComponentLinkIDs", 0x00856630, Streaming_AssignComponentLinkIDsImpl);
ENTSTREAM38_REVERSED("NiEntityStreaming::AssignEntityLinkIDs", 0x008566B0, Streaming_AssignEntityLinkIDsImpl);
ENTSTREAM38_REVERSED("NiEntityStreaming::AssignUniqueLinkIDs", 0x00856770, Streaming_AssignUniqueLinkIDsImpl);
ENTSTREAM38_REVERSED("Folded::SetField8", 0x00856840, Folded_SetField8);
ENTSTREAM38_REVERSED("NiExternalAssetKFParams::GetSequence", 0x00856860, KFParams_GetSequence);
ENTSTREAM38_REVERSED("NiExternalAssetKFParams::NiExternalAssetKFParams", 0x00856870, KFParams_Construct);
ENTSTREAM38_REVERSED("NiExternalAssetKFParams::GetRTTI", 0x008568B0, KFParams_GetRTTI);
ENTSTREAM38_REVERSED("NiExternalAssetKFParams::~NiExternalAssetKFParams", 0x008568C0, KFParams_DestructImpl);
ENTSTREAM38_REVERSED("NiExternalAssetKFParams::SetSequence", 0x00856920, KFParams_SetSequence);
ENTSTREAM38_REVERSED("NiExternalAssetKFParams::Destroy", 0x00856970, KFParams_Destroy);
ENTSTREAM38_REVERSED("NiExternalAssetParams::NiExternalAssetParams", 0x00856990, Params_ConstructImpl);
ENTSTREAM38_REVERSED("NiExternalAssetParams::GetRTTI", 0x008569B0, Params_GetRTTI);
ENTSTREAM38_REVERSED("NiExternalAssetParams::~NiExternalAssetParams", 0x008569C0, Params_DestructImpl);
ENTSTREAM38_REVERSED("NiExternalAssetParams::SetAssetPath", 0x00856A00, Params_SetAssetPath);
ENTSTREAM38_REVERSED("NiExternalAssetParams::GetAssetPath", 0x00856A30, Params_GetAssetPath);
ENTSTREAM38_REVERSED("NiExternalAssetParams::Destroy", 0x00856A60, Params_Destroy);
ENTSTREAM38_REVERSED("NiExternalAssetKFMParams::~NiExternalAssetKFMParams", 0x00856AC0, KFMParams_Destruct);
ENTSTREAM38_REVERSED("NiExternalAssetKFMParams::SetCumulative", 0x00856AD0, KFMParams_SetCumulative);
ENTSTREAM38_REVERSED("NiExternalAssetKFMParams::GetRTTI", 0x00856AE0, KFMParams_GetRTTI);
ENTSTREAM38_REVERSED("NiExternalAssetKFMParams::NiExternalAssetKFMParams", 0x00856AF0, KFMParams_Construct);
ENTSTREAM38_REVERSED("NiExternalAssetKFMParams::Destroy", 0x00856B10, KFMParams_Destroy);
ENTSTREAM38_REVERSED("NiExternalAssetNIFParams::NiExternalAssetNIFParams", 0x00856B40, NIFParams_Construct);
ENTSTREAM38_REVERSED("NiExternalAssetNIFParams::GetRTTI", 0x00856B80, NIFParams_GetRTTI);
ENTSTREAM38_REVERSED("NiExternalAssetNIFParams::~NiExternalAssetNIFParams", 0x00856B90, NIFParams_DestructImpl);
ENTSTREAM38_REVERSED("NiExternalAssetNIFParams::GetSceneRoot", 0x00856BF0, NIFParams_GetSceneRoot);
ENTSTREAM38_REVERSED("NiExternalAssetNIFParams::Destroy", 0x00856C10, NIFParams_Destroy);
ENTSTREAM38_REVERSED("NiGeneralComponentProperty::~NiGeneralComponentProperty", 0x00856C30, GeneralProperty_Destruct);
ENTSTREAM38_REVERSED("NiBase64::Encode", 0x00856D70, Base64_Encode);
ENTSTREAM38_REVERSED("NiBase64::DecodeQuantum", 0x00856EF0, Base64_DecodeQuantumImpl);
ENTSTREAM38_REVERSED("NiBase64::Decode", 0x00856F50, Base64_Decode);

} // namespace ni::entity
