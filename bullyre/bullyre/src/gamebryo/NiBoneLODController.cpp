// NiBoneLODController (src/gamebryo/NiBoneLODController.h): setting a level (the bones' selective
// update flags, the shapes culled but the level's, each given its skin instance for the level),
// Update following the global level, construction and destruction, cloning (the bones and shapes
// mapped to their clones, the skin instances cloned), streaming (the sets as links), comparison,
// viewer strings.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiBoneLODController.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiInterpolators.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni::anim {
namespace {

using scene::AddViewerString;
using scene::GetViewerString;
using scene::GetViewerStringUint;
using scene::LoadBytes;
using scene::SaveBytes;
using scene::Stream_ReadLinkID;
using scene::Stream_ResolveLinkID;
using scene::Virtual;

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiBoneLODController_RTTI, 0x020E5444);
GAME_VAR(uint32_t, ms_NiNodeClassIndex, 0x020E2550);   // NiNode's bit in the RTTI class sets

constexpr uintptr_t NiBoneLODController_vtable = 0x009678B8;
constexpr uintptr_t PrimitiveArray_vtable = 0x00961988;   // NiTPrimitiveArray<T*>

using BoneSet = NiTSet<NiNode*>;
using SkinSet = NiTSet<BoneLODSkinInfo*>;

GAME_FN(char*, __cdecl, GetViewerStringInt, 0x007856A0, const char* prefix, int32_t value);
GAME_FN(void, __thiscall, Object_PostLinkObject, 0x008A20A0, void* self, void* stream);
// NiTPrimitiveArray<T*>::SetSize / SetAt (one instantiation for every pointer kind)
GAME_FN(void, __thiscall, PtrArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, PtrArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
// NiTPrimitiveSet<T*>::Realloc(allocated)
GAME_FN(void, __thiscall, PtrSet_Realloc, 0x0083AD20, void* set, uint32_t allocated);
// NiTimeController's
GAME_FN(NiTimeController*, __thiscall, TimeController_Construct, 0x0077DFA0, void* self);
GAME_FN(void, __thiscall, TimeController_Destruct, 0x0077E010, void* self);
GAME_FN(void, __thiscall, TimeController_CopyMembers, 0x0077E100, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, TimeController_ProcessClone, 0x0077DEE0, void* self, void* cloning);
GAME_FN(void, __thiscall, TimeController_LinkObject, 0x0077DF10, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_RegisterStreamables, 0x0077DF70, void* self, void* stream);
GAME_FN(uint8_t, __thiscall, TimeController_IsEqual, 0x0077DB90, void* self, void* other);
GAME_FN(void, __thiscall, TimeController_LoadBinary, 0x0077E1F0, void* self, void* stream);
GAME_FN(void, __thiscall, TimeController_SaveBinary, 0x0077E2F0, void* self, void* stream);
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, BoneLOD_SetBoneLOD, 0x0081DE20, NiBoneLODController* self, int32_t lod);
GAME_FN(NiBoneLODController*, __thiscall, BoneLOD_Construct, 0x0081E2B0, void* self);
GAME_FN(void, __thiscall, BoneLOD_Cleanup, 0x0081ED50, NiBoneLODController* self);

// the clone of an object in the cloning's map (NiTPointerMap<NiObject*, NiObject*>: the buckets
// at +0x8, their count at +0x4; an item: next, key, value), or nothing
inline const re::Ptr32<void>* FindClone(void* cloning, const void* object) {
    void* map = *static_cast<re::Ptr32<void>*>(cloning);
    re::Ptr32<void>* buckets = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reinterpret_cast<uintptr_t>(map) + 8);
    uint32_t count = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(map) + 4);
    for (re::Ptr32<void>* item = static_cast<re::Ptr32<void>*>(buckets[reinterpret_cast<uintptr_t>(object) % count]); item != nullptr;
         item = static_cast<re::Ptr32<void>*>(item[0])) {
        if (item[1] == object) {
            return &item[2];
        }
    }
    return nullptr;
}

template <typename T>
inline T* CloneOr(void* cloning, T* object) {
    const re::Ptr32<void>* found = FindClone(cloning, object);
    return found != nullptr ? static_cast<T*>(*found) : object;
}
#if defined(_WIN64)
template <typename T>
inline T* CloneOr(void* cloning, re::Ptr32<T> object) {
    return CloneOr(cloning, object.get());
}
#endif

// NiTArray::SetAtGrow through SetAt (as some of these have it)
template <typename T>
inline void SetAtGrowCall(NiTArray<T*>* array, uint32_t index, T* value) {
    if (array->maxSize <= index) {
        PtrArray_SetSize(array, array->growBy + index);
    }
    re::Ptr32<void> v = value;
    PtrArray_SetAt(array, index, &v);
}

// the same with SetAt inlined (as the others have it)
template <typename T>
inline void SetAtGrowInline(NiTArray<T*>* array, uint32_t index, T* value) {
    if (array->maxSize <= index) {
        PtrArray_SetSize(array, array->growBy + index);
    }
    if (index < array->size) {
        if (value == nullptr) {
            if (array->base[index] != nullptr) {
                array->esize--;
            }
        } else if (array->base[index] == nullptr) {
            array->esize++;
        }
    } else {
        array->size = uint16_t(index + 1);
        if (value != nullptr) {
            array->esize++;
        }
    }
    array->base[index] = value;
}

// NiTPrimitiveSet::Add through Realloc
template <typename T>
inline void SetAddCall(NiTSet<T>* set, T value) {
    if (set->count == set->allocated) {
        PtrSet_Realloc(set, set->allocated != 0 ? set->allocated * 2 : 1);
    }
    set->base[set->count] = value;
    set->count++;
}

// the same with Realloc inlined
template <typename T>
inline void SetAddInline(NiTSet<T>* set, T value) {
    if (set->count == set->allocated) {
        uint32_t allocated = set->allocated != 0 ? set->allocated * 2 : 1;
        if (allocated != set->allocated) {
            re::Field<T>* base = nullptr;
            if (allocated != 0) {
                base = static_cast<re::Field<T>*>(NiMalloc(allocated * 4));
                for (uint32_t i = 0; i < set->count; i++) {
                    base[i] = set->base[i];
                }
            }
            NiFree(set->base);
            set->base = base;
            set->allocated = allocated;
        }
    }
    set->base[set->count] = value;
    set->count++;
}

template <typename T>
inline NiTSet<T>* NewSet() {
    auto* set = static_cast<NiTSet<T>*>(NiNew(0xC));
    if (set == nullptr) {
        return nullptr;
    }
    set->base = nullptr;
    set->allocated = 0;
    set->count = 0;
    return set;
}

inline BoneLODSkinInfo* NewSkinInfo() {
    auto* info = static_cast<BoneLODSkinInfo*>(NiNew(8));
    if (info == nullptr) {
        return nullptr;
    }
    info->skinInstance = nullptr;   // (the shape left for the caller)
    return info;
}

inline re::Ptr32<NiObject>& SkinInstanceOf(NiAVObject* shape) {
    return *reinterpret_cast<re::Ptr32<NiObject>*>(reinterpret_cast<uintptr_t>(shape) + 0xD0);
}

// 0x81DAB0: RegisterStreamables (the skin instances; the bones and shapes are the scene's)
uint8_t __fastcall BoneLOD_RegisterStreamables(NiBoneLODController* self, re::Edx, void* stream) {
    if (!TimeController_RegisterStreamables(self, stream)) {
        return 0;
    }
    for (uint32_t i = 0; i < self->skins.size; i++) {
        SkinSet* set = self->skins.base[i];
        if (set == nullptr) {
            continue;
        }
        for (uint32_t j = 0; j < set->count; j++) {
            NiObject* instance = set->base[j]->skinInstance;
            if (instance != nullptr) {
                Virtual<uint8_t>(instance, 6, stream);
            }
        }
    }
    return 1;
}

// 0x81DB20: LinkObject (each placeholder replaced by its resolved link; a set's ReplaceAt checks
// the index)
void __fastcall BoneLOD_LinkObject(NiBoneLODController* self, re::Edx, void* stream) {
    TimeController_LinkObject(self, stream);
    for (uint32_t i = 0; i < self->bones.size; i++) {
        BoneSet* set = self->bones.base[i];
        if (set == nullptr) {
            continue;
        }
        for (uint32_t j = 0; j < set->count; j++) {
            auto* bone = static_cast<NiNode*>(Stream_ResolveLinkID(stream));
            if (j < set->count) {
                set->base[j] = bone;
            }
        }
    }
    for (uint32_t i = 0; i < self->skins.size; i++) {
        SkinSet* set = self->skins.base[i];
        if (set == nullptr) {
            continue;
        }
        for (uint32_t j = 0; j < set->count; j++) {
            BoneLODSkinInfo* info = set->base[j];
            info->shape = static_cast<NiAVObject*>(Stream_ResolveLinkID(stream));
            AssignReleasingFirst(info->skinInstance, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
        }
    }
    for (uint32_t j = 0; j < self->shapes.count; j++) {
        auto* shape = static_cast<NiAVObject*>(Stream_ResolveLinkID(stream));
        if (j < self->shapes.count) {
            self->shapes.base[j] = shape;
        }
    }
}

// 0x81DC60: IsEqual (the sets' sizes, then each bone, shape and skin instance by its IsEqual)
uint8_t __fastcall BoneLOD_IsEqual(NiBoneLODController* self, re::Edx, NiBoneLODController* other) {
    if (!TimeController_IsEqual(self, other) || self->lod != other->lod || self->numLODs != other->numLODs ||
        self->bones.size != other->bones.size || self->skins.size != other->skins.size ||
        self->shapes.count != other->shapes.count) {
        return 0;
    }
    for (uint32_t i = 0; i < self->bones.size; i++) {
        BoneSet* mine = self->bones.base[i];
        BoneSet* theirs = other->bones.base[i];
        if (mine == nullptr) {
            if (theirs != nullptr) {
                return 0;
            }
            continue;
        }
        if (theirs == nullptr || mine->count != theirs->count) {
            return 0;
        }
        for (uint32_t j = 0; j < mine->count; j++) {
            if (!Virtual<uint8_t>(mine->base[j], 8, theirs->base[j])) {
                return 0;
            }
        }
    }
    for (uint32_t i = 0; i < self->skins.size; i++) {
        SkinSet* mine = self->skins.base[i];
        SkinSet* theirs = other->skins.base[i];
        if (mine == nullptr) {
            if (theirs != nullptr) {
                return 0;
            }
            continue;
        }
        if (theirs == nullptr || mine->count != theirs->count) {
            return 0;
        }
        for (uint32_t j = 0; j < mine->count; j++) {
            BoneLODSkinInfo* a = mine->base[j];
            BoneLODSkinInfo* b = theirs->base[j];
            if (!Virtual<uint8_t>(a->shape, 8, b->shape)) {
                return 0;
            }
            if (!Virtual<uint8_t>(a->skinInstance, 8, b->skinInstance)) {
                return 0;
            }
        }
    }
    for (uint32_t j = 0; j < self->shapes.count; j++) {
        if (!Virtual<uint8_t>(self->shapes.base[j], 8, other->shapes.base[j])) {
            return 0;
        }
    }
    return 1;
}

// 0x81DE20: SetBoneLOD(level) (false when there are no levels, it's the current one, or it's
// out of range; -1 clears): the bone sets from the level's on selectively updated, those before
// not (a missing set at the level itself leaves the flag off until the next set, as the game
// has it); every shape culled, then the level's shown with its skin instance
uint8_t __fastcall BoneLOD_SetBoneLODImpl(NiBoneLODController* self, re::Edx, int32_t lod) {
    if (self->numLODs == 0 || self->lod == lod) {
        return 0;
    }
    if (lod != -1 && (lod < -1 || uint32_t(lod) >= self->numLODs)) {
        return 0;
    }
    uint8_t on = 0;
    for (uint32_t i = 0; i < self->bones.size; i++) {
        BoneSet* set = self->bones.base[i];
        if (set == nullptr) {
            continue;
        }
        if (i == uint32_t(lod)) {
            on = 1;
        }
        for (uint32_t j = 0; j < set->count; j++) {
            NiNode* bone = set->base[j];
            if (on) {
                bone->flags |= 2;
            } else {
                bone->flags &= 0xFFFD;
            }
        }
    }
    for (uint32_t j = 0; j < self->shapes.count; j++) {
        NiAVObject* shape = self->shapes.base[j];
        shape->flags = uint16_t((shape->flags & 0xFFFD) | 1);
    }
    if (lod != -1) {
        SkinSet* set = self->skins.base[lod];
        if (set != nullptr) {
            for (uint32_t j = 0; j < set->count; j++) {
                NiAVObject* shape = set->base[j]->shape;
                shape->flags = uint16_t((shape->flags & 0xFFFE) | 2);
                AssignReleasingFirst(SkinInstanceOf(shape), set->base[j]->skinInstance);
            }
        }
    }
    self->lod = lod;
    return 1;
}

// 0x81DFA0: SaveBinary (each set's count, 0 for a missing one, then its links)
void __fastcall BoneLOD_SaveBinary(NiBoneLODController* self, re::Edx, void* stream) {
    TimeController_SaveBinary(self, stream);
    SaveBytes(stream, &self->lod, 4);
    SaveBytes(stream, &self->numLODs, 4);
    uint32_t count = self->bones.size;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < self->bones.size; i++) {
        BoneSet* set = self->bones.base[i];
        if (set == nullptr) {
            uint32_t none = 0;
            SaveBytes(stream, &none, 4);
            continue;
        }
        uint32_t n = set->count;
        SaveBytes(stream, &n, 4);
        for (uint32_t j = 0; j < set->count; j++) {
            Virtual<void>(stream, 12, set->base[j]);   // SaveLinkID
        }
    }
    count = self->skins.size;
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < self->skins.size; i++) {
        SkinSet* set = self->skins.base[i];
        if (set == nullptr) {
            uint32_t none = 0;
            SaveBytes(stream, &none, 4);
            continue;
        }
        uint32_t n = set->count;
        SaveBytes(stream, &n, 4);
        for (uint32_t j = 0; j < set->count; j++) {
            BoneLODSkinInfo* info = set->base[j];
            Virtual<void>(stream, 12, info->shape);
            Virtual<void>(stream, 12, info->skinInstance);
        }
    }
    count = self->shapes.count;
    SaveBytes(stream, &count, 4);
    for (uint32_t j = 0; j < self->shapes.count; j++) {
        Virtual<void>(stream, 12, self->shapes.base[j]);
    }
}

// 0x81E230: PostLinkObject: the level set again once the links are in
void __fastcall BoneLOD_PostLinkObject(NiBoneLODController* self, re::Edx, void* stream) {
    Object_PostLinkObject(self, stream);
    int32_t lod = self->lod;
    self->lod = -1;
    BoneLOD_SetBoneLOD(self, lod);
}

// 0x81E260: NiTPrimitiveSet<T>(allocated) (a set of pointers NiMalloc'd at that size)
NiTSet<void*>* __fastcall PtrSet_Construct(NiTSet<void*>* self, re::Edx, uint32_t allocated) {
    if (allocated != 0) {
        re::Ptr32<void>* base = static_cast<re::Ptr32<void>*>(NiMalloc(allocated * 4));
        self->allocated = allocated;
        self->base = base;
        self->count = 0;
        return self;
    }
    self->allocated = allocated;
    self->base = nullptr;
    self->count = 0;
    return self;
}

inline void InitArray(void* array) {
    auto* a = static_cast<NiTArray<void*>*>(array);
    a->maxSize = 0;
    a->size = 0;
    a->esize = 0;
    a->base = nullptr;
    a->vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    a->growBy = 1;
}

// 0x81E2B0: NiBoneLODController(): no level, the arrays and the shape set empty
NiBoneLODController* __fastcall BoneLOD_ConstructImpl(NiBoneLODController* self, re::Edx) {
    TimeController_Construct(self);
    self->vtable = reinterpret_cast<void*>(NiBoneLODController_vtable);
    self->lod = -1;
    self->numLODs = 0;
    InitArray(&self->bones);
    InitArray(&self->skins);
    self->shapes.base = nullptr;
    self->shapes.allocated = 0;
    self->shapes.count = 0;
    return self;
}

// 0x81E310
const RTTI* __fastcall BoneLOD_GetRTTI(NiBoneLODController*, re::Edx) {
    return &NiBoneLODController_RTTI;
}

// 0x81E320: TargetIsRequiredType (slot 27): a NiNode
uint8_t __fastcall BoneLOD_TargetIsRequiredType(NiBoneLODController* self, re::Edx) {
    if (self->target == nullptr) {
        return 0;
    }
    return IsKindOf(self->target, ms_NiNodeClassIndex);
}

// 0x81E350: Update(time) (slot 20): the global level followed (one past the last level is
// clamped to the last only when beyond the count: the count itself is refused by SetBoneLOD, as
// the game has it)
void __fastcall BoneLOD_Update(NiBoneLODController* self, re::Edx, float) {
    int32_t lod = ms_iGlobalLOD;
    if (lod == -1 || lod == self->lod) {
        return;
    }
    int32_t numLODs = int32_t(self->numLODs);
    if (lod > numLODs) {
        lod = numLODs - 1;
    }
    if (lod == self->lod) {
        return;
    }
    BoneLOD_SetBoneLOD(self, lod);
}

// 0x81E380: CreateClone (the level and count; the sets are made by ProcessClone. A failed
// allocation still copied onto null, as the game has it)
NiBoneLODController* __fastcall BoneLOD_CreateClone(NiBoneLODController* self, re::Edx, void* cloning) {
    void* memory = NiNew(0x68);
    NiBoneLODController* clone = memory != nullptr ? BoneLOD_Construct(memory) : nullptr;
    TimeController_CopyMembers(self, clone, cloning);
    clone->lod = self->lod;
    clone->numLODs = self->numLODs;
    return clone;
}

// 0x81E3C0: ProcessClone: the clone's sets made: bones and shapes mapped to their clones (or
// kept), each skin info with its shape mapped and its skin instance cloned (CreateClone, then the
// original's ProcessClone); then the clone's level set again. (This controller's clone missing
// from the map: written through null, as the game has it)
void __fastcall BoneLOD_ProcessClone(NiBoneLODController* self, re::Edx, void* cloning) {
    TimeController_ProcessClone(self, cloning);
    const re::Ptr32<void>* found = FindClone(cloning, self);
    auto* clone = found != nullptr ? static_cast<NiBoneLODController*>(*found) : nullptr;
    for (uint32_t i = 0; i < self->bones.size; i++) {
        BoneSet* set = self->bones.base[i];
        if (set == nullptr) {
            SetAtGrowCall<BoneSet>(&clone->bones, i, nullptr);
            continue;
        }
        BoneSet* newSet = NewSet<NiNode*>();
        SetAtGrowCall(&clone->bones, i, newSet);
        for (uint32_t j = 0; j < set->count; j++) {
            SetAddCall(newSet, CloneOr(cloning, set->base[j]));
        }
    }
    for (uint32_t i = 0; i < self->skins.size; i++) {
        SkinSet* set = self->skins.base[i];
        if (set == nullptr) {
            SetAtGrowInline<SkinSet>(&clone->skins, i, nullptr);
            continue;
        }
        SkinSet* newSet = NewSet<BoneLODSkinInfo*>();
        SetAtGrowInline(&clone->skins, i, newSet);
        for (uint32_t j = 0; j < set->count; j++) {
            BoneLODSkinInfo* old = set->base[j];
            BoneLODSkinInfo* info = NewSkinInfo();
            SetAddInline(newSet, info);
            info->shape = CloneOr(cloning, old->shape);
            AssignReleasingFirst(info->skinInstance, Virtual<NiObject*>(old->skinInstance, 3, cloning));
            Virtual<void>(old->skinInstance, 11, cloning);
        }
    }
    for (uint32_t j = 0; j < self->shapes.count; j++) {
        SetAddInline(&clone->shapes, CloneOr(cloning, self->shapes.base[j]));
    }
    int32_t lod = clone->lod;
    clone->lod = -1;
    BoneLOD_SetBoneLOD(clone, lod);
}

// 0x81E900: CreateObject
NiObject* __cdecl BoneLOD_CreateObject() {
    void* memory = NiNew(0x68);
    if (memory == nullptr) {
        return nullptr;
    }
    return BoneLOD_Construct(memory);
}

// 0x81E920: LoadBinary (each set made with this controller as every link's placeholder,
// LinkObject resolving them; a skin info's shape left unset until then)
void __fastcall BoneLOD_LoadBinary(NiBoneLODController* self, re::Edx, void* stream) {
    TimeController_LoadBinary(self, stream);
    LoadBytes(stream, &self->lod, 4);
    LoadBytes(stream, &self->numLODs, 4);
    uint32_t boneSets = 0;
    LoadBytes(stream, &boneSets, 4);
    for (uint32_t i = 0; i < boneSets; i++) {
        BoneSet* set = NewSet<NiNode*>();
        SetAtGrowInline(&self->bones, i, set);
        uint32_t n = 0;
        LoadBytes(stream, &n, 4);
        for (uint32_t j = 0; j < n; j++) {
            SetAddCall(set, reinterpret_cast<NiNode*>(self));
            Stream_ReadLinkID(stream);
        }
    }
    uint32_t skinSets = 0;
    LoadBytes(stream, &skinSets, 4);
    for (uint32_t i = 0; i < skinSets; i++) {
        SkinSet* set = NewSet<BoneLODSkinInfo*>();
        SetAtGrowInline(&self->skins, i, set);
        uint32_t n = 0;
        LoadBytes(stream, &n, 4);
        for (uint32_t j = 0; j < n; j++) {
            SetAddInline(set, NewSkinInfo());
            Stream_ReadLinkID(stream);
            Stream_ReadLinkID(stream);
        }
    }
    uint32_t shapes = 0;
    LoadBytes(stream, &shapes, 4);
    for (uint32_t j = 0; j < shapes; j++) {
        SetAddInline(&self->shapes, reinterpret_cast<NiAVObject*>(self));
        Stream_ReadLinkID(stream);
    }
}

// 0x81ED50: the sets let go (each skin info's instance released), the arrays emptied, no shapes
void __fastcall BoneLOD_CleanupImpl(NiBoneLODController* self, re::Edx) {
    for (uint32_t i = 0; i < self->bones.size; i++) {
        BoneSet* set = self->bones.base[i];
        if (set != nullptr) {
            NiFree(set->base);
            NiDelete(set, 0xC);
        }
    }
    for (uint16_t i = 0; i < self->bones.size; i++) {
        self->bones.base[i] = nullptr;
    }
    self->bones.size = 0;
    self->bones.esize = 0;
    for (uint32_t i = 0; i < self->skins.size; i++) {
        SkinSet* set = self->skins.base[i];
        if (set == nullptr) {
            continue;
        }
        for (uint32_t j = 0; j < set->count; j++) {
            BoneLODSkinInfo* info = set->base[j];
            if (info != nullptr) {
                ReleaseRef(info->skinInstance);
                NiDelete(info, 8);
            }
        }
        NiFree(set->base);
        NiDelete(set, 0xC);
    }
    for (uint16_t i = 0; i < self->skins.size; i++) {
        self->skins.base[i] = nullptr;
    }
    self->skins.size = 0;
    self->skins.esize = 0;
    self->shapes.count = 0;
}

// 0x81EE80: GetViewerStrings (the base's not listed)
void __fastcall BoneLOD_GetViewerStrings(NiBoneLODController* self, re::Edx, NiTArray<char*>* strings) {
    AddViewerString(strings, GetViewerString(NiBoneLODController_RTTI.name));
    AddViewerString(strings, GetViewerStringInt("m_iLOD", self->lod));
    AddViewerString(strings, GetViewerStringUint("m_uiNumLODs", self->numLODs));
}

// 0x81EF50: the deleting destructor
NiBoneLODController* __fastcall BoneLOD_DeletingDestructor(NiBoneLODController* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(NiBoneLODController_vtable);
    BoneLOD_Cleanup(self);
    NiFree(self->shapes.base);
    self->skins.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    NiFree(self->skins.base);
    self->bones.vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
    NiFree(self->bones.base);
    TimeController_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, 0x68);
    }
    return self;
}

}   // namespace
}   // namespace ni::anim

// ---- registration ---------------------------------------------------------------------

#define NIC32_BONELOD_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                   \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIC32_BONELOD_REVERSED("NiBoneLODController::RegisterStreamables", 0x0081DAB0,
                       &ni::anim::BoneLOD_RegisterStreamables);
NIC32_BONELOD_REVERSED("NiBoneLODController::LinkObject", 0x0081DB20, &ni::anim::BoneLOD_LinkObject);
NIC32_BONELOD_REVERSED("NiBoneLODController::IsEqual", 0x0081DC60, &ni::anim::BoneLOD_IsEqual);
NIC32_BONELOD_REVERSED("NiBoneLODController::SetBoneLOD", 0x0081DE20, &ni::anim::BoneLOD_SetBoneLODImpl);
NIC32_BONELOD_REVERSED("NiBoneLODController::SaveBinary", 0x0081DFA0, &ni::anim::BoneLOD_SaveBinary);
NIC32_BONELOD_REVERSED("NiBoneLODController::PostLinkObject", 0x0081E230, &ni::anim::BoneLOD_PostLinkObject);
NIC32_BONELOD_REVERSED("NiTPrimitiveSet<T*>::NiTPrimitiveSet_81E260", 0x0081E260, &ni::anim::PtrSet_Construct);
NIC32_BONELOD_REVERSED("NiBoneLODController::NiBoneLODController", 0x0081E2B0, &ni::anim::BoneLOD_ConstructImpl);
NIC32_BONELOD_REVERSED("NiBoneLODController::GetRTTI", 0x0081E310, &ni::anim::BoneLOD_GetRTTI);
NIC32_BONELOD_REVERSED("NiBoneLODController::TargetIsRequiredType", 0x0081E320,
                       &ni::anim::BoneLOD_TargetIsRequiredType);
NIC32_BONELOD_REVERSED("NiBoneLODController::Update", 0x0081E350, &ni::anim::BoneLOD_Update);
NIC32_BONELOD_REVERSED("NiBoneLODController::CreateClone", 0x0081E380, &ni::anim::BoneLOD_CreateClone);
NIC32_BONELOD_REVERSED("NiBoneLODController::ProcessClone", 0x0081E3C0, &ni::anim::BoneLOD_ProcessClone);
NIC32_BONELOD_REVERSED("NiBoneLODController::CreateObject", 0x0081E900, &ni::anim::BoneLOD_CreateObject);
NIC32_BONELOD_REVERSED("NiBoneLODController::LoadBinary", 0x0081E920, &ni::anim::BoneLOD_LoadBinary);
NIC32_BONELOD_REVERSED("NiBoneLODController::Cleanup", 0x0081ED50, &ni::anim::BoneLOD_CleanupImpl);
NIC32_BONELOD_REVERSED("NiBoneLODController::GetViewerStrings", 0x0081EE80, &ni::anim::BoneLOD_GetViewerStrings);
NIC32_BONELOD_REVERSED("NiBoneLODController::DeletingDestructor", 0x0081EF50,
                       &ni::anim::BoneLOD_DeletingDestructor);
