// NiActorComponent (vtables 0x98A2EC / 0x98A1C0 at +8, 0x24 bytes): an entity's animated actor,
// an NiActorManager loaded from its "KFM File Path" through the external asset manager
// (NiExternalAssetKFMParams), playing its "Active Sequence ID" ("Accumulate Transforms": the
// actor's accumulation root keeps the motion). "Sequence IDs" / "Sequence Names" list the
// controller manager's sequences, "Scene Root Pointer" is the actor's NIF root. The three
// settings come from a master component (+0x20) unless made unique (flags 0x2, 0x4, 0x8): the
// first one up the chain with its own. Its interface methods get the interface pointer (+8);
// the helpers the object. The default constructor and the destructor are the other pass's
// (0x83CD70, 0x83CDD0).
//
// (NiEntityComponents.cpp's NiActorComponent struct has +0x1C / +0x20 the other way round: the
// code here reads the master at +0x20 (GetMasterComponent, the constructor with a master) and
// the actor manager at +0x1C.)
//
// Also NiActorManager::SetTargetAnimation as the library inlines a copy of it here (0x851B70).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "gamebryo/NiActorManager.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiEntityComponentCommon.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::ec {
namespace {

using ni::actor::NiActorManager;

struct ActorComponent : NiRefObject {
    re::Ptr32<void> ifaceVtable;               // +0x8
    uint8_t flags;                   // +0xC (0x1 the KFM failed to load; 0x2, 0x4, 0x8 the KFM
                                     //  path, active sequence, accumulation are its own)
    uint8_t padD[3];
    re::Ptr32<const char> kfmFilePath;       // +0x10
    uint32_t activeSequenceID;       // +0x14 (NiActorManager::INVALID_SEQUENCE_ID)
    uint8_t accumulateTransforms;    // +0x18
    uint8_t pad19[3];
    re::Ptr32<NiActorManager> actorManager;    // +0x1C (a reference)
    re::Ptr32<ActorComponent> master;          // +0x20 (a reference: the master's object)
};
static_assert(offsetof(ActorComponent, kfmFilePath) == 0x10);
static_assert(offsetof(ActorComponent, accumulateTransforms) == 0x18);
static_assert(offsetof(ActorComponent, actorManager) == 0x1C);
static_assert(offsetof(ActorComponent, master) == 0x20);
static_assert(sizeof(ActorComponent) == 0x24);

enum : uint8_t {
    kLoadFailed = 0x1,
    kUniquePath = 0x2,
    kUniqueSequence = 0x4,
    kUniqueAccumulate = 0x8,
};

constexpr uintptr_t ActorComponent_vtable = 0x0098A2EC;
constexpr uintptr_t ActorComponentIface_vtable = 0x0098A1C0;

GAME_VAR(NiFixedString, ERR_TRANSLATION_NOT_FOUND, 0x020E5EDC);
GAME_VAR(NiFixedString, ERR_ROTATION_NOT_FOUND, 0x020E5EE0);
GAME_VAR(NiFixedString, ERR_SCALE_NOT_FOUND, 0x020E5EE4);
GAME_VAR(NiFixedString, ERR_ACTOR_MANAGER_NOT_CREATED, 0x020E5EE8);
GAME_VAR(NiFixedString, ms_kClassName, 0x020E5EEC);       // "NiActorComponent"
GAME_VAR(NiFixedString, ms_kComponentName, 0x020E5EF0);   // "Actor"
GAME_VAR(NiFixedString, PROP_KFM_FILE_PATH, 0x020E5EF4);
GAME_VAR(NiFixedString, PROP_ACTIVE_SEQUENCE_ID, 0x020E5EF8);
GAME_VAR(NiFixedString, PROP_ACCUMULATE_TRANSFORMS, 0x020E5EFC);
GAME_VAR(NiFixedString, PROP_SEQUENCE_IDS, 0x020E5F00);
GAME_VAR(NiFixedString, PROP_SEQUENCE_NAMES, 0x020E5F04);
GAME_VAR(NiFixedString, PROP_SCENE_ROOT_POINTER, 0x020E5F08);
GAME_VAR(NiFixedString, DESC_KFM_FILE_PATH, 0x020E5F0C);
GAME_VAR(NiFixedString, DESC_ACTIVE_SEQUENCE_ID, 0x020E5F10);
GAME_VAR(NiFixedString, DESC_ACCUMULATE_TRANSFORMS, 0x020E5F14);
GAME_VAR(NiFixedString, PROP_TRANSLATION, 0x020E5F18);
GAME_VAR(NiFixedString, PROP_ROTATION, 0x020E5F1C);
GAME_VAR(NiFixedString, PROP_SCALE, 0x020E5F20);
GAME_VAR(NiUniqueID, ms_kTemplateID, 0x020E5EC8);
GAME_VAR(uint32_t, ms_kTemplateIDGuard, 0x020E5ED8);
GAME_VAR(uint32_t, INVALID_SEQUENCE_ID, 0x00965A40);
const char* const kKFMFilenameSemantic = reinterpret_cast<const char*>(0x0098BEDC);   // "KFM Filename"
const char* const kSequenceIDSemantic = reinterpret_cast<const char*>(0x0098BED0);    // "Sequence ID"

// NiExternalAssetKFMParams (vtable 0x98C300, 0x10 bytes, the other pass's): the asset path,
// whether to accumulate, the actor manager made
struct ParamsKFM {
    re::Ptr32<void> vtable;                    // +0x0
    re::Ptr32<const char> assetPath;         // +0x4
    uint8_t accumulate;              // +0x8
    uint8_t pad9[3];
    re::Ptr32<NiActorManager> actorManager;    // +0xC
};
static_assert(sizeof(ParamsKFM) == 0x10);

GAME_FN(void, __thiscall, ParamsKFM_Construct, 0x00856AF0, ParamsKFM* self, int accumulate);
GAME_FN(void, __thiscall, ParamsKFM_Destruct, 0x00856AC0, ParamsKFM* self);
GAME_FN(void, __thiscall, ParamsKFM_SetAccumulate, 0x00856AD0, ParamsKFM* self, int accumulate);
GAME_FN(uint8_t, __thiscall, ParamsKFM_SetAssetPath, 0x00856A00, ParamsKFM* self, const re::Ptr32<const char>* path);
GAME_FN(NiActorManager*, __thiscall, ParamsKFM_GetActorManager, 0x00853140, ParamsKFM* self);   // (folded)

GAME_FN(uint8_t, __thiscall, PointerMap_GetAt, 0x005AC5F0, void* map, uint32_t key, re::Ptr32<void>* value);
GAME_FN(re::Ptr32<NiRefObject>*, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiRefObject>* self, NiRefObject* object);
GAME_FN(uint32_t, __thiscall, ActorManager_FindSequenceID, 0x008005A0, NiActorManager* self, const char* name);
GAME_FN(void, __thiscall, ActorManager_Update, 0x00803D90, NiActorManager* self, float time);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateSelected, 0x0075DB30, void* self, float time);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void, __thiscall, AVObject_UpdateEffects, 0x0075E240, void* self);
GAME_FN(uint8_t, __thiscall, Point3_NotEqual, 0x0076BEB0, const NiPoint3* self, const NiPoint3* other);
GAME_FN(uint8_t, __thiscall, Matrix3_Equal, 0x007686A0, const NiMatrix3* self, const NiMatrix3* other);

// this file's, through their addresses
GAME_FN(uint8_t, __thiscall, SetTargetAnimation_, 0x00851B70, NiActorManager* self, uint32_t id);
GAME_FN(ActorComponent*, __thiscall, ConstructWithMaster_, 0x00851BB0, void* self, ActorComponent* master);
GAME_FN(const re::Ptr32<const char>*, __thiscall, GetKFMFilePath_, 0x00851C20, ActorComponent* self);
GAME_FN(uint32_t, __thiscall, GetActiveSequenceID_, 0x00851C40, ActorComponent* self);
GAME_FN(void, __thiscall, SetActiveSequenceID_, 0x00851C70, ActorComponent* self, uint32_t id);
GAME_FN(uint8_t, __thiscall, GetAccumulateTransforms_, 0x00851CC0, ActorComponent* self);
GAME_FN(uint8_t, __thiscall, ShouldReload_, 0x00851CE0, ActorComponent* self);
GAME_FN(void, __thiscall, SetKFMFilePath_, 0x00852B70, ActorComponent* self, const re::Ptr32<const char>* path);
GAME_FN(void, __thiscall, SetAccumulateTransforms_, 0x00852BF0, ActorComponent* self, int accumulate);

inline ActorComponent* Self(void* iface) {
    return FromIface<ActorComponent>(iface);
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

inline void ReleaseRef(NiRefObject* object) {
    if (object != nullptr && ni::sys::Decrement(&object->refCount) == 0) {
        re::PtrCast<void(__thiscall*)(NiRefObject*)>(VtableOf(object)[1])(object);
    }
}

// the three settings in effect, as the methods inline them: its own, or the master's (through
// the master's chain walk)
inline const re::Ptr32<const char>* EffectivePath(ActorComponent* self) {
    if ((self->flags & kUniquePath) == 0 && self->master != nullptr) {
        return GetKFMFilePath_(self->master);
    }
    return &self->kfmFilePath;
}
inline uint32_t EffectiveSequenceID(ActorComponent* self) {
    if ((self->flags & kUniqueSequence) == 0 && self->master != nullptr) {
        return GetActiveSequenceID_(self->master);
    }
    return self->actorManager != nullptr ? self->actorManager->targetID : self->activeSequenceID;
}
inline uint8_t EffectiveAccumulate(ActorComponent* self) {
    if ((self->flags & kUniqueAccumulate) == 0 && self->master != nullptr) {
        return GetAccumulateTransforms_(self->master);
    }
    return self->accumulateTransforms;
}

// the actor manager let go (the sequence it was playing kept as the active one first)
inline void DropActorManager(ActorComponent* self) {
    NiActorManager* manager = self->actorManager;
    if (manager != nullptr) {
        self->activeSequenceID = manager->targetID;
    }
    if (manager != nullptr) {
        ReleaseRef(manager);
        self->actorManager = nullptr;
    }
}

// its 6 properties; the three settings
inline bool IsSetting(NiFixedString n) {
    return n == PROP_KFM_FILE_PATH || n == PROP_ACTIVE_SEQUENCE_ID || n == PROP_ACCUMULATE_TRANSFORMS;
}
inline bool IsReadOnlyProperty(NiFixedString n) {
    return n == PROP_SEQUENCE_IDS || n == PROP_SEQUENCE_NAMES || n == PROP_SCENE_ROOT_POINTER;
}

// ---- the static names (set in this order; let go with the class and component names swapped)

constexpr StaticString kNames[] = {
    {0x020E5EDC, 0x0098B554},   // "Translation property not found."
    {0x020E5EE0, 0x0098B534},   // "Rotation property not found."
    {0x020E5EE4, 0x0098B518},   // "Scale property not found."
    {0x020E5EE8, 0x0098ACE8},   // "The actor manager could not be created from the specified KFM file."
    {0x020E5EF0, 0x0098BEC8},   // "Actor"
    {0x020E5EEC, 0x0098A578},   // "NiActorComponent"
    {0x020E5EF4, 0x0098BEB8},   // "KFM File Path"
    {0x020E5EF8, 0x0098BEA4},   // "Active Sequence ID"
    {0x020E5EFC, 0x0098BE8C},   // "Accumulate Transforms"
    {0x020E5F00, 0x0098BE7C},   // "Sequence IDs"
    {0x020E5F04, 0x0098BE6C},   // "Sequence Names"
    {0x020E5F08, 0x0098AF48},   // "Scene Root Pointer"
    {0x020E5F0C, 0x0098BE3C},   // "The path to a KFM file to use for the actor."
    {0x020E5F10, 0x0098BE0C},   // "The currently active sequence ID for the actor."
    {0x020E5F14, 0x0098BDB0},   // "Indicates whether or not the actor accumulates transformations..."
    {0x020E5F18, 0x008FFFBC},   // "Translation"
    {0x020E5F1C, 0x008FFF9C},   // "Rotation"
    {0x020E5F20, 0x008FFFA8},   // "Scale"
};

void __cdecl SDMInit() {
    SetStaticStrings(kNames);
}

void __cdecl SDMShutdown() {
    for (uintptr_t address = 0x020E5EDC; address <= 0x020E5F20; address += 4) {
        FixedString_SetString(reinterpret_cast<re::Ptr32<const char>*>(address), nullptr);
    }
}

// ---- NiActorManager::SetTargetAnimation (an inlined copy): a known sequence (or none)

uint8_t __fastcall SetTargetAnimation(NiActorManager* self, re::Edx, uint32_t id) {
    if (self->targetID == id) {
        return 1;
    }
    if (id != INVALID_SEQUENCE_ID) {
        re::Ptr32<void> sequence;   // (the game's: the argument's slot)
        uint8_t found = PointerMap_GetAt(&self->sequenceMap, id, &sequence);
        if (!found) {
            return found;
        }
    }
    self->targetID = id;
    self->rebuildTimeline = 1;
    return 1;
}

// ---- the master chain

void* __fastcall GetMasterComponent(void* iface, re::Edx) {
    ActorComponent* master = Self(iface)->master;
    return master != nullptr ? IfaceOf(master) : nullptr;
}

// (Clone's: inheriting from a master, the settings the defaults)
ActorComponent* __fastcall ConstructWithMaster(ActorComponent* self, re::Edx, ActorComponent* master) {
    self->vtable = reinterpret_cast<void*>(RefObject_vtable);
    self->refCount = 0;
    ni::sys::Increment(const_cast<long*>(&RefObject_ms_uiObjects));
    self->ifaceVtable = reinterpret_cast<void*>(ComponentInterface_vtable);
    self->vtable = reinterpret_cast<void*>(ActorComponent_vtable);
    self->ifaceVtable = reinterpret_cast<void*>(ActorComponentIface_vtable);
    self->flags = 0;
    self->kfmFilePath = NullFixedString;
    self->activeSequenceID = INVALID_SEQUENCE_ID;
    self->accumulateTransforms = 0;
    self->actorManager = nullptr;
    self->master = master;
    if (master != nullptr) {
        ni::sys::Increment(&master->refCount);
    }
    return self;
}

// the first up the chain with its own path
const re::Ptr32<const char>* __fastcall GetKFMFilePath(ActorComponent* self, re::Edx) {
    ActorComponent* c = self;
    while ((c->flags & kUniquePath) == 0 && c->master != nullptr) {
        c = c->master;
    }
    return &c->kfmFilePath;
}

// the first up the chain with its own sequence: its actor manager's target, or its value
uint32_t __fastcall GetActiveSequenceID(ActorComponent* self, re::Edx) {
    ActorComponent* c = self;
    while ((c->flags & kUniqueSequence) == 0 && c->master != nullptr) {
        c = c->master;
    }
    return c->actorManager != nullptr ? c->actorManager->targetID : c->activeSequenceID;
}

// a different sequence becomes its own (and the actor's target)
void __fastcall SetActiveSequenceID(ActorComponent* self, re::Edx, uint32_t id) {
    if (EffectiveSequenceID(self) == id) {
        return;
    }
    self->activeSequenceID = id;
    if (self->actorManager != nullptr) {
        SetTargetAnimation_(self->actorManager, id);
    }
    self->flags |= kUniqueSequence;
}

uint8_t __fastcall GetAccumulateTransforms(ActorComponent* self, re::Edx) {
    ActorComponent* c = self;
    while ((c->flags & kUniqueAccumulate) == 0 && c->master != nullptr) {
        c = c->master;
    }
    return c->accumulateTransforms;
}

// the actor to be (re)made: the path or the accumulation in effect changed, or there's none and
// loading hasn't failed
uint8_t __fastcall ShouldReload(ActorComponent* self, re::Edx) {
    uint8_t flags = self->flags;
    const re::Ptr32<const char>* path = ((flags & kUniquePath) == 0 && self->master != nullptr)
                                    ? GetKFMFilePath_(self->master)
                                    : &self->kfmFilePath;
    if (*path != self->kfmFilePath) {
        return 1;
    }
    uint8_t accumulate = ((flags & kUniqueAccumulate) == 0 && self->master != nullptr)
                             ? GetAccumulateTransforms_(self->master)
                             : self->accumulateTransforms;
    if (uint32_t(accumulate != 0 ? 1 : 0) != self->accumulateTransforms) {
        return 1;
    }
    if (self->actorManager != nullptr) {
        return 0;
    }
    return uint8_t(~flags & 1);
}

// a different path becomes its own: the actor dropped (made again by Update), no sequence
void __fastcall SetKFMFilePath(ActorComponent* self, re::Edx, const re::Ptr32<const char>* path) {
    if (*EffectivePath(self) == *path) {
        return;
    }
    FixedString_Assign(&self->kfmFilePath, path);
    DropActorManager(self);
    self->activeSequenceID = INVALID_SEQUENCE_ID;
    self->flags = uint8_t((self->flags & ~kLoadFailed) | kUniquePath);
}

// a different accumulation becomes its own: the actor dropped (made again by Update)
void __fastcall SetAccumulateTransforms(ActorComponent* self, re::Edx, int accumulate) {
    uint8_t value = uint8_t(accumulate);
    if (uint32_t(EffectiveAccumulate(self) != 0 ? 1 : 0) == value) {
        return;
    }
    self->accumulateTransforms = value;
    DropActorManager(self);
    self->flags = uint8_t((self->flags & ~kLoadFailed) | kUniqueAccumulate);
}

// ---- the interface

NiUniqueID* __fastcall GetTemplateID_(void*, re::Edx, NiUniqueID* result) {
    return ReturnTemplateID(result, ms_kTemplateIDGuard, ms_kTemplateID, 0x14617B26, 0x44378A50, 0x022718AB,
                            0xED4B2AA0);
}

// resettable: a setting made unique while there's a master
uint8_t __fastcall CanResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* canReset) {
    ActorComponent* self = Self(iface);
    NiFixedString n = *name;
    uint8_t bit;
    if (n == PROP_KFM_FILE_PATH) {
        bit = kUniquePath;
    } else if (n == PROP_ACTIVE_SEQUENCE_ID) {
        bit = kUniqueSequence;
    } else if (n == PROP_ACCUMULATE_TRANSFORMS) {
        bit = kUniqueAccumulate;
    } else if (IsReadOnlyProperty(n)) {
        *canReset = 0;
        return 1;
    } else {
        return 0;
    }
    *canReset = (self->master != nullptr && (self->flags & bit) != 0) ? 1 : 0;
    return 1;
}

// back to the master's value
uint8_t __fastcall ResetProperty(void* iface, re::Edx, const re::Ptr32<const char>* name) {
    ActorComponent* self = Self(iface);
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));   // (the game's: the name's slot)
    if (!V(iface, slot::CanResetProperty, name, &canReset) || canReset == 0) {
        return 0;
    }
    NiFixedString n = *name;
    if (n == PROP_KFM_FILE_PATH) {
        self->flags &= ~kUniquePath;
        return 1;
    }
    if (n == PROP_ACTIVE_SEQUENCE_ID) {
        self->flags &= ~kUniqueSequence;
        return 1;
    }
    if (n == PROP_ACCUMULATE_TRANSFORMS) {
        self->flags &= ~kUniqueAccumulate;
        return 1;
    }
    return 0;
}

// a setting taken off the master (its value stays what it holds)
uint8_t __fastcall MakePropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* madeUnique) {
    ActorComponent* self = Self(iface);
    uint8_t canReset = uint8_t(reinterpret_cast<uintptr_t>(name));   // (the name's slot)
    uint8_t found = V(iface, slot::CanResetProperty, name, &canReset);
    if (!found) {
        return found;
    }
    if (canReset != 0 || self->master == nullptr) {
        *madeUnique = 0;
        return 1;
    }
    NiFixedString n = *name;
    if (n == PROP_KFM_FILE_PATH) {
        self->flags |= kUniquePath;
        *madeUnique = 1;
    } else if (n == PROP_ACTIVE_SEQUENCE_ID) {
        self->flags |= kUniqueSequence;
        *madeUnique = 1;
    } else if (n == PROP_ACCUMULATE_TRANSFORMS) {
        self->flags |= kUniqueAccumulate;
        *madeUnique = 1;
    } else {
        *madeUnique = 0;
    }
    return 1;
}

uint8_t __fastcall IsPropertyReadOnly(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* readOnly) {
    NiFixedString n = *name;
    if (IsSetting(n)) {
        *readOnly = 0;
        return 1;
    }
    if (IsReadOnlyProperty(n)) {
        *readOnly = 1;
        return 1;
    }
    return 0;
}

// unique: a setting while there's no master, else whether it was made unique
uint8_t __fastcall IsPropertyUnique(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* unique) {
    NiFixedString n = *name;
    if (IsSetting(n)) {
        if (Self(iface)->master != nullptr) {
            V(iface, slot::CanResetProperty, name, unique);
            return 1;
        }
        *unique = 1;
        return 1;
    }
    if (IsReadOnlyProperty(n)) {
        *unique = 1;
        return 1;
    }
    return 0;
}

// serializable: a unique, writable setting (the bytes start as the name's slot and the high
// byte of the pushed `this`)
uint8_t __fastcall IsPropertySerializable(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* serializable) {
    NiFixedString n = *name;
    if (IsSetting(n)) {
        uint8_t unique = uint8_t(reinterpret_cast<uintptr_t>(name));
        V(iface, slot::IsPropertyUnique, name, &unique);
        uint8_t readOnly = uint8_t(reinterpret_cast<uintptr_t>(iface) >> 24);
        V(iface, slot::IsPropertyReadOnly, name, &readOnly);
        *serializable = (unique != 0 && readOnly == 0) ? 1 : 0;
        return 1;
    }
    if (IsReadOnlyProperty(n)) {
        *serializable = 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall IsPropertyInheritable(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* inheritable) {
    NiFixedString n = *name;
    if (IsSetting(n)) {
        *inheritable = 1;
        return 1;
    }
    if (IsReadOnlyProperty(n)) {
        *inheritable = 0;
        return 1;
    }
    return 0;
}

// the KFM path is one
uint8_t __fastcall IsExternalAssetPath(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* isPath) {
    NiFixedString n = *name;
    if (n == PROP_KFM_FILE_PATH) {
        *isPath = 1;
        return 1;
    }
    if (n == PROP_ACTIVE_SEQUENCE_ID || n == PROP_ACCUMULATE_TRANSFORMS || IsReadOnlyProperty(n)) {
        *isPath = 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetElementCount(void*, re::Edx, const re::Ptr32<const char>* name, uint32_t, uint8_t* set) {
    NiFixedString n = *name;
    if (IsSetting(n) || n == PROP_SCENE_ROOT_POINTER || n == PROP_SEQUENCE_IDS || n == PROP_SEQUENCE_NAMES) {
        *set = 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall IsCollection(void*, re::Edx, const re::Ptr32<const char>* name, uint8_t* isCollection) {
    NiFixedString n = *name;
    if (IsSetting(n) || n == PROP_SCENE_ROOT_POINTER) {
        *isCollection = 0;
        return 1;
    }
    if (n == PROP_SEQUENCE_IDS || n == PROP_SEQUENCE_NAMES) {
        *isCollection = 1;
        return 1;
    }
    return 0;
}

// one, or the controller manager's sequences (none without an actor)
uint8_t __fastcall GetElementCount(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t* count) {
    NiFixedString n = *name;
    if (IsSetting(n) || n == PROP_SCENE_ROOT_POINTER) {
        *count = 1;
        return 1;
    }
    if (n == PROP_SEQUENCE_IDS || n == PROP_SEQUENCE_NAMES) {
        NiActorManager* manager = Self(iface)->actorManager;
        *count = manager != nullptr ? *reinterpret_cast<uint16_t*>(re::PtrCast<uint8_t*>(manager->manager) + 0x3E)
                                    : 0;
        return 1;
    }
    return 0;
}

// the controller manager's sequence at an index (NiTObjectArray at +0x38)
inline void* SequenceAt(NiActorManager* manager, uint32_t index) {
    return re::PtrCast<re::Ptr32<void>*>(*reinterpret_cast<re::Ptr32<void>*>(re::PtrCast<uint8_t*>(manager->manager) + 0x38))[index];
}
inline uint32_t SequenceCount(NiActorManager* manager) {
    return *reinterpret_cast<uint16_t*>(re::PtrCast<uint8_t*>(manager->manager) + 0x3E);
}

// the KFM path in effect; a sequence's name
uint8_t __fastcall GetString(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* value, uint32_t index) {
    ActorComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_KFM_FILE_PATH) {
        if (index != 0) {
            return 0;
        }
        FixedString_Assign(value, EffectivePath(self));
        return 1;
    }
    if (n == PROP_SEQUENCE_NAMES) {
        NiActorManager* manager = self->actorManager;
        if (manager == nullptr || index >= SequenceCount(manager)) {
            return 0;
        }
        FixedString_Assign(value, reinterpret_cast<re::Ptr32<const char>*>(static_cast<uint8_t*>(SequenceAt(manager, index)) + 8));
        return 1;
    }
    return 0;
}

// the active sequence in effect; a sequence's ID (found by its name)
uint8_t __fastcall GetUInt(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t* value, uint32_t index) {
    ActorComponent* self = Self(iface);
    NiFixedString n = *name;
    if (n == PROP_ACTIVE_SEQUENCE_ID) {
        if (index != 0) {
            return 0;
        }
        *value = EffectiveSequenceID(self);
        return 1;
    }
    if (n == PROP_SEQUENCE_IDS) {
        NiActorManager* manager = self->actorManager;
        if (manager == nullptr || index >= SequenceCount(manager)) {
            return 0;
        }
        const char* sequenceName = *reinterpret_cast<re::Ptr32<const char>*>(static_cast<uint8_t*>(SequenceAt(self->actorManager, index)) + 8);
        *value = ActorManager_FindSequenceID(self->actorManager, sequenceName);
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetUInt(void* iface, re::Edx, const re::Ptr32<const char>* name, uint32_t value, uint32_t index) {
    if (index == 0 && *name == PROP_ACTIVE_SEQUENCE_ID) {
        SetActiveSequenceID_(Self(iface), value);
        return 1;
    }
    return 0;
}

uint8_t __fastcall GetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, uint8_t* value, uint32_t index) {
    if (index == 0 && *name == PROP_ACCUMULATE_TRANSFORMS) {
        *value = EffectiveAccumulate(Self(iface)) != 0 ? 1 : 0;
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetBool(void* iface, re::Edx, const re::Ptr32<const char>* name, int value, uint32_t index) {
    if (index == 0 && *name == PROP_ACCUMULATE_TRANSFORMS) {
        SetAccumulateTransforms_(Self(iface), value);
        return 1;
    }
    return 0;
}

uint8_t __fastcall SetString(void* iface, re::Edx, const re::Ptr32<const char>* name, const re::Ptr32<const char>* value,
                             uint32_t index) {
    if (index == 0 && *name == PROP_KFM_FILE_PATH) {
        SetKFMFilePath_(Self(iface), value);
        return 1;
    }
    return 0;
}

// the actor's NIF root (no reference taken)
uint8_t __fastcall GetObject_(void* iface, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<NiAVObject>* value, uint32_t index) {
    if (index == 0 && *name == PROP_SCENE_ROOT_POINTER) {
        NiActorManager* manager = Self(iface)->actorManager;
        *value = manager != nullptr ? manager->nifRoot : nullptr;
        return 1;
    }
    return 0;
}

re::Ptr32<const char>* __fastcall GetClassName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnStatic(result, ms_kClassName);
}

re::Ptr32<const char>* __fastcall GetName_(void*, re::Edx, re::Ptr32<const char>* result) {
    return ReturnStatic(result, ms_kComponentName);
}

// inheriting: a new one with this as its master; else a copy of its settings with its master
void* __fastcall Clone(void* iface, re::Edx, int inherit) {
    ActorComponent* self = Self(iface);
    if (re::Flag(inherit)) {
        void* memory = NiNew(sizeof(ActorComponent));
        ActorComponent* clone = memory != nullptr ? ConstructWithMaster_(memory, self) : nullptr;
        return clone != nullptr ? IfaceOf(clone) : nullptr;
    }
    void* memory = NiNew(sizeof(ActorComponent));
    // (an allocation failure writes through null, as the game's does)
    ActorComponent* clone = memory != nullptr ? ConstructWithMaster_(memory, self->master) : nullptr;
    clone->flags = self->flags;
    FixedString_Assign(&clone->kfmFilePath, &self->kfmFilePath);
    clone->activeSequenceID = self->activeSequenceID;
    clone->accumulateTransforms = self->accumulateTransforms;
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(clone) + 8);
}

// the error handler told (slot 2: message, description, the entity's name, the property's)
void ReportMissing(void* errors, void* entity, const re::Ptr32<const char>* message, const re::Ptr32<const char>* property) {
    NiFixedString description = NullFixedString;
    NiFixedString entityName;
    re::Ptr32<const char>* namePtr = V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
    V<void>(errors, 2, message, &description, namePtr, property);
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&entityName));
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&description));
}

// the actor (re)made when its settings in effect changed: the settings taken as its own, the old
// actor let go, the KFM loaded through the asset manager (a failure reported and remembered);
// then the NIF root moved to the entity's transformation, the active sequence followed and
// the actor updated
void __fastcall Update(void* iface, re::Edx, void* entity, float time, void* errors, void* assets) {
    ActorComponent* self = Self(iface);
    if (ShouldReload_(self)) {
        FixedString_Assign(&self->kfmFilePath, EffectivePath(self));
        self->activeSequenceID = EffectiveSequenceID(self);
        self->accumulateTransforms = EffectiveAccumulate(self) != 0 ? 1 : 0;
        uint32_t sequenceID = EffectiveSequenceID(self);
        DropActorManager(self);
        if (*EffectivePath(self) != NullFixedString && assets != nullptr && V<void*>(assets, 17) != nullptr) {
            NiFixedString unused = NullFixedString;   // (in the asset manager's argument slot)
            ParamsKFM params;
            ParamsKFM_Construct(&params, 0);
            if (GetAccumulateTransforms_(self)) {
                ParamsKFM_SetAccumulate(&params, 1);
            }
            bool ok = ParamsKFM_SetAssetPath(&params, GetKFMFilePath_(self)) && V(assets, 5, &params);
            if (ok) {
                V<void*>(assets, 7, &params);
                Pointer_Assign(reinterpret_cast<re::Ptr32<NiRefObject>*>(&self->actorManager),
                               ParamsKFM_GetActorManager(&params));
                ok = self->actorManager != nullptr;
            }
            if (!ok) {
                self->flags |= kLoadFailed;
                NiFixedString entityName;
                re::Ptr32<const char>* namePtr = V<re::Ptr32<const char>*>(entity, slot::GetName, &entityName);
                const re::Ptr32<const char>* path = GetKFMFilePath_(self);
                V<void>(errors, 2, &ERR_ACTOR_MANAGER_NOT_CREATED, path, namePtr, &PROP_KFM_FILE_PATH);
                FixedString_Destruct(&entityName);
                ParamsKFM_Destruct(&params);
                FixedString_Destruct(&unused);
                return;
            }
            AVObject_Update(self->actorManager->nifRoot, 0.0f, 1);
            V<void>(self->actorManager->nifRoot, 19);   // UpdateNodeBound
            AVObject_UpdateProperties(self->actorManager->nifRoot);
            AVObject_UpdateEffects(self->actorManager->nifRoot);
            SetTargetAnimation_(self->actorManager, sequenceID);
            ParamsKFM_Destruct(&params);
            FixedString_Destruct(&unused);
        }
    }

    if (self->actorManager == nullptr) {
        return;
    }
    uint8_t allFound = 1;   // (the asset manager's argument slot)
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
    bool changed = false;
    if (allFound) {
        NiAVObject* root = self->actorManager->nifRoot;
        if (Point3_NotEqual(&root->local.translate, &translate)) {
            root->local.translate = translate;
            changed = true;
        }
        if (!Matrix3_Equal(&root->local.rotate, &rotate)) {
            root->local.rotate = rotate;
            changed = true;
        }
        if (!(scale == root->local.scale)) {
            uint32_t bits;   // (fabs: the sign bit cleared)
            std::memcpy(&bits, &scale, 4);
            bits &= 0x7FFFFFFF;
            std::memcpy(&root->local.scale, &bits, 4);
            changed = true;
        }
    }
    if (EffectiveSequenceID(self) != self->activeSequenceID) {
        SetTargetAnimation_(self->actorManager, EffectiveSequenceID(self));
        self->activeSequenceID = EffectiveSequenceID(self);
    }
    ActorManager_Update(self->actorManager, time);
    if (changed) {
        AVObject_Update(self->actorManager->nifRoot, time, 1);
    } else {
        AVObject_UpdateSelected(self->actorManager->nifRoot, time);
    }
}

// the actor drawn: the culler's Process (slot 3) on its NIF root into the culler's visible set
void __fastcall BuildVisibleSet(void* iface, re::Edx, void* context, void*) {
    NiActorManager* manager = Self(iface)->actorManager;
    if (manager == nullptr) {
        return;
    }
    void* culler = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(context) + 0x10);
    void* camera = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(context) + 0xC);
    void* visibleSet = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(culler) + 0xC);
    re::PtrCast<void(__thiscall*)(void*, void*, void*, void*)>(VtableOf(culler)[3])(culler, camera,
                                                                                         manager->nifRoot, visibleSet);
}

// the display name: the setting's own (none for the read-only ones)
uint8_t __fastcall GetDisplayName(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* displayName) {
    NiFixedString n = *name;
    if (n == PROP_KFM_FILE_PATH) {
        FixedString_Assign(displayName, &PROP_KFM_FILE_PATH);
    } else if (n == PROP_ACTIVE_SEQUENCE_ID) {
        FixedString_Assign(displayName, &PROP_ACTIVE_SEQUENCE_ID);
    } else if (n == PROP_ACCUMULATE_TRANSFORMS) {
        FixedString_Assign(displayName, &PROP_ACCUMULATE_TRANSFORMS);
    } else if (IsReadOnlyProperty(n)) {
        FixedString_SetString(displayName, nullptr);
    } else {
        return 0;
    }
    return 1;
}

uint8_t __fastcall GetPrimitiveType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* type) {
    NiFixedString n = *name;
    const re::Ptr32<const char>* t;
    if (n == PROP_KFM_FILE_PATH) {
        t = &PT_STRING;
    } else if (n == PROP_ACTIVE_SEQUENCE_ID) {
        t = &PT_UINT;
    } else if (n == PROP_ACCUMULATE_TRANSFORMS) {
        t = &PT_BOOL;
    } else if (n == PROP_SEQUENCE_IDS) {
        t = &PT_UINT;
    } else if (n == PROP_SEQUENCE_NAMES) {
        t = &PT_STRING;
    } else if (n == PROP_SCENE_ROOT_POINTER) {
        t = &PT_NIOBJECTPOINTER;
    } else {
        return 0;
    }
    FixedString_Assign(type, t);
    return 1;
}

// the path and the sequence have their own semantic types (made from the texts each time)
uint8_t __fastcall GetSemanticType(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* type) {
    NiFixedString n = *name;
    if (n == PROP_KFM_FILE_PATH) {
        FixedString_SetString(type, kKFMFilenameSemantic);
        return 1;
    }
    if (n == PROP_ACTIVE_SEQUENCE_ID) {
        FixedString_SetString(type, kSequenceIDSemantic);
        return 1;
    }
    const re::Ptr32<const char>* t;
    if (n == PROP_ACCUMULATE_TRANSFORMS) {
        t = &PT_BOOL;
    } else if (n == PROP_SEQUENCE_IDS) {
        t = &PT_UINT;
    } else if (n == PROP_SEQUENCE_NAMES) {
        t = &PT_STRING;
    } else if (n == PROP_SCENE_ROOT_POINTER) {
        t = &PT_NIOBJECTPOINTER;
    } else {
        return 0;
    }
    FixedString_Assign(type, t);
    return 1;
}

uint8_t __fastcall GetDescription(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* description) {
    NiFixedString n = *name;
    if (n == PROP_KFM_FILE_PATH) {
        FixedString_Assign(description, &DESC_KFM_FILE_PATH);
    } else if (n == PROP_ACTIVE_SEQUENCE_ID) {
        FixedString_Assign(description, &DESC_ACTIVE_SEQUENCE_ID);
    } else if (n == PROP_ACCUMULATE_TRANSFORMS) {
        FixedString_Assign(description, &DESC_ACCUMULATE_TRANSFORMS);
    } else if (IsReadOnlyProperty(n)) {
        FixedString_SetString(description, nullptr);
    } else {
        return 0;
    }
    return 1;
}

uint8_t __fastcall GetCategory(void*, re::Edx, const re::Ptr32<const char>* name, re::Ptr32<const char>* category) {
    NiFixedString n = *name;
    if (!IsSetting(n) && !IsReadOnlyProperty(n)) {
        return 0;
    }
    FixedString_Assign(category, &ms_kComponentName);
    return 1;
}

// a master taken (a reference held); none: every property made unique first, then the master
// let go
void __fastcall SetMasterComponent(void* iface, re::Edx, void* masterIface) {
    ActorComponent* self = Self(iface);
    if (masterIface != nullptr) {
        ActorComponent* master = FromIface<ActorComponent>(masterIface);
        ActorComponent* old = self->master;
        if (old != master) {
            ReleaseRef(old);
            self->master = master;
            if (master != nullptr) {
                ni::sys::Increment(&master->refCount);
            }
        }
        return;
    }
    StringSet names{FixedStringArray_New(10), 10, 0};
    V<void>(iface, slot::GetPropertyNames, &names);
    uint32_t count = names.used;
    uint8_t madeUnique = 0;   // (its slot: the argument's, the null master)
    for (uint32_t i = 0; i < count; i++) {
        V(iface, slot::MakePropertyUnique, &names.base[i], &madeUnique);
    }
    ActorComponent* master = self->master;
    if (master != nullptr) {
        ReleaseRef(master);
        self->master = nullptr;
    }
    if (names.base != nullptr) {
        FixedStringArray_Delete(names.base, 3);
    }
}

// the transformation it follows
void __fastcall GetDependentPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_TRANSLATION);
    StringSetAdd(names, &PROP_ROTATION);
    StringSetAdd(names, &PROP_SCALE);
}

void __fastcall GetPropertyNames(void*, re::Edx, StringSet* names) {
    StringSetAdd(names, &PROP_KFM_FILE_PATH);
    StringSetAdd(names, &PROP_ACTIVE_SEQUENCE_ID);
    StringSetAdd(names, &PROP_ACCUMULATE_TRANSFORMS);
    StringSetAdd(names, &PROP_SEQUENCE_IDS);
    StringSetAdd(names, &PROP_SEQUENCE_NAMES);
    StringSetAdd(names, &PROP_SCENE_ROOT_POINTER);
}

} // namespace
} // namespace ni::ec

// ---- registration ---------------------------------------------------------------------

#define NIACTORCOMP_REVERSED(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

namespace ac = ni::ec;

NIACTORCOMP_REVERSED("NiActorComponent::GetMasterComponent", 0x00851610, &ac::GetMasterComponent);
NIACTORCOMP_REVERSED("NiActorComponent::GetTemplateID", 0x00851620, &ac::GetTemplateID_);
NIACTORCOMP_REVERSED("NiActorComponent::CanResetProperty", 0x008516D0, &ac::CanResetProperty);
NIACTORCOMP_REVERSED("NiActorComponent::ResetProperty", 0x00851780, &ac::ResetProperty);
NIACTORCOMP_REVERSED("NiActorComponent::MakePropertyUnique", 0x008517F0, &ac::MakePropertyUnique);
NIACTORCOMP_REVERSED("NiActorComponent::IsPropertyReadOnly", 0x00851890, &ac::IsPropertyReadOnly);
NIACTORCOMP_REVERSED("NiActorComponent::IsPropertyUnique", 0x008518F0, &ac::IsPropertyUnique);
NIACTORCOMP_REVERSED("NiActorComponent::IsPropertySerializable", 0x00851960, &ac::IsPropertySerializable);
NIACTORCOMP_REVERSED("NiActorComponent::IsPropertyInheritable", 0x00851A00, &ac::IsPropertyInheritable);
NIACTORCOMP_REVERSED("NiActorComponent::IsExternalAssetPath", 0x00851A60, &ac::IsExternalAssetPath);
NIACTORCOMP_REVERSED("NiActorComponent::SetElementCount", 0x00851AC0, &ac::SetElementCount);
NIACTORCOMP_REVERSED("NiActorComponent::IsCollection", 0x00851B10, &ac::IsCollection);
NIACTORCOMP_REVERSED("NiActorManager::SetTargetAnimation_851B70", 0x00851B70, &ac::SetTargetAnimation);
NIACTORCOMP_REVERSED("NiActorComponent::NiActorComponent(master)", 0x00851BB0, &ac::ConstructWithMaster);
NIACTORCOMP_REVERSED("NiActorComponent::GetKFMFilePath", 0x00851C20, &ac::GetKFMFilePath);
NIACTORCOMP_REVERSED("NiActorComponent::GetActiveSequenceID", 0x00851C40, &ac::GetActiveSequenceID);
NIACTORCOMP_REVERSED("NiActorComponent::SetActiveSequenceID", 0x00851C70, &ac::SetActiveSequenceID);
NIACTORCOMP_REVERSED("NiActorComponent::GetAccumulateTransforms", 0x00851CC0, &ac::GetAccumulateTransforms);
NIACTORCOMP_REVERSED("NiActorComponent::ShouldReload", 0x00851CE0, &ac::ShouldReload);
NIACTORCOMP_REVERSED("NiActorComponent::_SDMInit", 0x00851D50, &ac::SDMInit);
NIACTORCOMP_REVERSED("NiActorComponent::_SDMShutdown", 0x00851E60, &ac::SDMShutdown);
NIACTORCOMP_REVERSED("NiActorComponent::Clone", 0x00851F40, &ac::Clone);
NIACTORCOMP_REVERSED("NiActorComponent::GetClassName", 0x00851FC0, &ac::GetClassName_);
NIACTORCOMP_REVERSED("NiActorComponent::GetName", 0x00851FF0, &ac::GetName_);
NIACTORCOMP_REVERSED("NiActorComponent::Update", 0x00852020, &ac::Update);
NIACTORCOMP_REVERSED("NiActorComponent::BuildVisibleSet", 0x00852610, &ac::BuildVisibleSet);
NIACTORCOMP_REVERSED("NiActorComponent::GetDisplayName", 0x00852640, &ac::GetDisplayName);
NIACTORCOMP_REVERSED("NiActorComponent::GetPrimitiveType", 0x008526D0, &ac::GetPrimitiveType);
NIACTORCOMP_REVERSED("NiActorComponent::GetSemanticType", 0x00852760, &ac::GetSemanticType);
NIACTORCOMP_REVERSED("NiActorComponent::GetDescription", 0x00852810, &ac::GetDescription);
NIACTORCOMP_REVERSED("NiActorComponent::GetCategory", 0x008528A0, &ac::GetCategory);
NIACTORCOMP_REVERSED("NiActorComponent::GetElementCount", 0x008528F0, &ac::GetElementCount);
NIACTORCOMP_REVERSED("NiActorComponent::GetPropertyData(NiFixedString)", 0x00852970, &ac::GetString);
NIACTORCOMP_REVERSED("NiActorComponent::GetPropertyData(unsigned int)", 0x00852A00, &ac::GetUInt);
NIACTORCOMP_REVERSED("NiActorComponent::SetPropertyData(unsigned int)", 0x00852AA0, &ac::SetUInt);
NIACTORCOMP_REVERSED("NiActorComponent::GetPropertyData(bool)", 0x00852AD0, &ac::GetBool);
NIACTORCOMP_REVERSED("NiActorComponent::GetPropertyData(NiObject*)", 0x00852B30, &ac::GetObject_);
NIACTORCOMP_REVERSED("NiActorComponent::SetKFMFilePath", 0x00852B70, &ac::SetKFMFilePath);
NIACTORCOMP_REVERSED("NiActorComponent::SetAccumulateTransforms", 0x00852BF0, &ac::SetAccumulateTransforms);
NIACTORCOMP_REVERSED("NiActorComponent::SetPropertyData(NiFixedString)", 0x00852C70, &ac::SetString);
NIACTORCOMP_REVERSED("NiActorComponent::SetPropertyData(bool)", 0x00852CA0, &ac::SetBool);
NIACTORCOMP_REVERSED("NiActorComponent::SetMasterComponent", 0x00852CD0, &ac::SetMasterComponent);
NIACTORCOMP_REVERSED("NiActorComponent::GetDependentPropertyNames", 0x00852DC0, &ac::GetDependentPropertyNames);
NIACTORCOMP_REVERSED("NiActorComponent::GetPropertyNames", 0x00852E70, &ac::GetPropertyNames);
