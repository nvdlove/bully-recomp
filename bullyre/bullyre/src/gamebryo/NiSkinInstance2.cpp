// NiSkinInstance (vtable 0x953200, 0x30 bytes): the binding of a skinned geometry to its skin
// data, hardware partition, root parent and bones (one per bone of the skin data). Its slots:
// streaming (the bones as link IDs; before 10.1.0.101 the partition hung off the skin data and
// PostLinkObject moves it here), cloning (the root parent and bones mapped through the cloning
// process's clone map), comparison, viewer strings (the bones' names sorted), RTTI, destruction.
// The deformation and destructor are other chunks' (NiSkinInstance.cpp, NiImageConverter2.cpp).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiSkinInstance : NiObject {
    re::Ptr32<NiObject> skinData;       // +0x8 (NiSkinDataPtr: the bone count at its +0x44)
    re::Ptr32<NiObject> partition;      // +0xC (NiSkinPartitionPtr: the partition count at its +0x8)
    re::Ptr32<NiAVObject> rootParent;   // +0x10 (not a reference)
    re::Ptr32<re::Ptr32<NiAVObject>> bones;       // +0x14 (NiMalloc'd, not references)
    int32_t frameID;          // +0x18 (-1)
    uint32_t unknown1C[5];    // +0x1C (the bone matrices' renderer data: 0)
};
static_assert(offsetof(NiSkinInstance, frameID) == 0x18);
static_assert(sizeof(NiSkinInstance) == 0x30);

struct RTTI {
    re::Ptr32<const char> name;
    re::Ptr32<const RTTI> base;
};
GAME_VAR(RTTI, NiSkinInstance_RTTI, 0x020E2DF4);

constexpr uintptr_t NiSkinInstance_vtable = 0x00953200;

// the cloning process's clone map (NiTPointerMap<NiObject*, NiObject*>: an item is next, key,
// value)
struct MapItem {   // (the game's layouts: 0xC, the map's first 0xC)
    re::Ptr32<MapItem> next;
    re::Ptr32<void> key;
    re::Ptr32<void> value;
};
static_assert(sizeof(MapItem) == 0xC);
struct CloneMap {
    re::Ptr32<void> vtable;
    uint32_t hashSize;   // +0x4
    re::Ptr32<re::Ptr32<MapItem>> table;     // +0x8
};

GAME_FN(void, __thiscall, Object_LinkObjectEmpty, 0x008A20A0, void* self, void* stream);   // NiObject's (empty)
GAME_FN(void, __thiscall, SkinInstance_Destruct, 0x007D4850, NiSkinInstance* self);
GAME_FN(void, __thiscall, Pointer_Assign, 0x00800420, re::Ptr32<NiObject>* self, NiObject* value);
GAME_FN(int, __cdecl, NiSprintf_, 0x00402E00, char* buffer, size_t size, const char* format, ...);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, SkinInstance_CopyMembers, 0x00777190, NiSkinInstance* self, NiSkinInstance* dest,
        void* cloning);
constexpr uintptr_t CompareNames_address = 0x00776EB0;

using Compare = int(__cdecl*)(const void*, const void*);

// NiPointer::operator= as the code inlines it: the old one released first
inline void AssignRef(re::Ptr32<NiObject>& slot, NiObject* value) {
    NiObject* old = slot;
    if (old != value) {
        if (old != nullptr && _InterlockedDecrement(reinterpret_cast<volatile long*>(&old->refCount)) == 0) {
            Virtual<void>(old, 1);
        }
        slot = value;
        if (value != nullptr) {
            _InterlockedIncrement(reinterpret_cast<volatile long*>(&value->refCount));
        }
    }
}

// the clone of an object, or the object itself
inline void* CloneOf(void* cloning, void* object) {
    const CloneMap* map = static_cast<CloneMap*>(*static_cast<const re::Ptr32<CloneMap>*>(cloning));
    for (MapItem* item = map->table[reinterpret_cast<uintptr_t>(object) % map->hashSize]; item != nullptr;
         item = item->next) {
        if (item->key == object) {
            return item->value;
        }
    }
    return object;
}

inline uint32_t BoneCount(const NiSkinInstance* self) {
    return At<uint32_t>(self->skinData, 0x44);
}

} // namespace

// 0x776EB0: the bone names' qsort comparison (strcmp's order; a missing element sorts last)
int __cdecl NiSkinInstance_CompareNames(const void* a, const void* b) {
    if (a == nullptr) {
        return 1;
    }
    if (b == nullptr) {
        return -1;
    }
    const uint8_t* x = static_cast<const uint8_t*>(*static_cast<const re::Ptr32<const uint8_t>*>(a));
    const uint8_t* y = static_cast<const uint8_t*>(*static_cast<const re::Ptr32<const uint8_t>*>(b));
    for (;; x++, y++) {
        if (*x != *y) {
            return *x < *y ? -1 : 1;
        }
        if (*x == 0) {
            return 0;
        }
    }
}

// 0x776F70: LoadBinary (slot 4): the link IDs (the partition from 10.1.0.101)
void __fastcall NiSkinInstance_LoadBinary(NiSkinInstance* self, re::Edx, void* stream) {
    Object_LoadBinary(self, stream);
    Stream_ReadLinkID(stream);
    if (StreamVersion(stream) >= 0x0A010065) {
        Stream_ReadLinkID(stream);
    }
    Stream_ReadLinkID(stream);
    Stream_ReadMultipleLinkIDs(stream);
}

// 0x776FB0: LinkObject (slot 5): the skin data, partition, root parent and bones resolved
void __fastcall NiSkinInstance_LinkObject(NiSkinInstance* self, re::Edx, void* stream) {
    Object_LinkObjectEmpty(self, stream);
    AssignRef(self->skinData, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
    if (StreamVersion(stream) >= 0x0A010065) {
        AssignRef(self->partition, static_cast<NiObject*>(Stream_ResolveLinkID(stream)));
    }
    self->rootParent = static_cast<NiAVObject*>(Stream_ResolveLinkID(stream));
    uint32_t count = Stream_GetNumberOfLinkIDs(stream);
    self->bones = static_cast<re::Ptr32<NiAVObject>*>(NiMalloc(count * 4));
    for (uint32_t i = 0; i < count; i++) {
        self->bones[i] = static_cast<NiAVObject*>(Stream_ResolveLinkID(stream));
    }
}

// 0x777090: RegisterStreamables (slot 6)
uint8_t __fastcall NiSkinInstance_RegisterStreamables(NiSkinInstance* self, re::Edx, void* stream) {
    if (!Object_RegisterStreamables(self, stream)) {
        return 0;
    }
    Virtual<uint8_t>(self->skinData, 6, stream);
    if (self->partition != nullptr) {
        Virtual<uint8_t>(self->partition, 6, stream);
    }
    Virtual<uint8_t>(self->rootParent, 6, stream);
    uint32_t count = BoneCount(self);
    for (uint32_t i = 0; i < count; i++) {
        Virtual<uint8_t>(self->bones[i], 6, stream);
    }
    return 1;
}

// 0x777100: IsEqual (slot 8): the skin data and the partition (one with none equals one without)
uint8_t __fastcall NiSkinInstance_IsEqual(NiSkinInstance* self, re::Edx, NiSkinInstance* other) {
    if (!Object_IsEqual(self, other)) {
        return 0;
    }
    if (!Virtual<uint8_t>(self->skinData, 8, other->skinData)) {
        return 0;
    }
    if (self->partition != nullptr && !Virtual<uint8_t>(self->partition, 8, other->partition)) {
        return 0;
    }
    if (self->partition == nullptr && other->partition != nullptr) {
        return 0;
    }
    return 1;
}

// 0x777160: GetRTTI (slot 2)
const RTTI* __fastcall NiSkinInstance_GetRTTI(NiSkinInstance*, re::Edx) {
    return &NiSkinInstance_RTTI;
}

// 0x777170: the deleting destructor (slot 0)
NiSkinInstance* __fastcall NiSkinInstance_DeletingDestructor(NiSkinInstance* self, re::Edx, uint32_t flags) {
    SkinInstance_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, sizeof(NiSkinInstance));
    }
    return self;
}

// 0x777190: CopyMembers (the skin data and partition shared)
void __fastcall NiSkinInstance_CopyMembers(NiSkinInstance* self, re::Edx, NiSkinInstance* dest, void* cloning) {
    Object_CopyMembers(self, dest, cloning);
    AssignRef(dest->skinData, self->skinData);
    AssignRef(dest->partition, self->partition);
}

// 0x777270: PostLinkObject (slot 12): before 10.1.0.101 the partition taken off the skin data
void __fastcall NiSkinInstance_PostLinkObject(NiSkinInstance* self, re::Edx, void* stream) {
    if (StreamVersion(stream) >= 0x0A010065 || self->skinData == nullptr) {
        return;
    }
    Pointer_Assign(&self->partition, At<NiObject*>(self->skinData, 8));
    re::Ptr32<NiObject>& old = At<NiObject*>(self->skinData, 8);
    NiObject* partition = old;
    if (partition != nullptr) {
        if (_InterlockedDecrement(reinterpret_cast<volatile long*>(&partition->refCount)) == 0) {
            Virtual<void>(partition, 1);
        }
        old = nullptr;
    }
}

// 0x7772D0: SaveBinary (slot 7)
void __fastcall NiSkinInstance_SaveBinary(NiSkinInstance* self, re::Edx, void* stream) {
    Object_LinkObjectEmpty(self, stream);   // (NiObject's SaveBinary: the same empty function)
    Virtual<void>(stream, 12, self->skinData);
    Virtual<void>(stream, 12, self->partition);
    Virtual<void>(stream, 12, self->rootParent);
    uint32_t count = BoneCount(self);
    SaveBytes(stream, &count, 4);
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(stream, 12, self->bones[i]);
    }
}

// 0x777370: CreateClone (slot 3): the root parent and bones are their clones (when cloned)
NiSkinInstance* __fastcall NiSkinInstance_CreateClone(NiSkinInstance* self, re::Edx, void* cloning) {
    NiSkinInstance* clone = static_cast<NiSkinInstance*>(NiNew(sizeof(NiSkinInstance)));
    if (clone != nullptr) {
        RefObject_Construct(clone);
        clone->vtable = reinterpret_cast<void*>(NiSkinInstance_vtable);
        clone->skinData = nullptr;
        clone->partition = nullptr;
        clone->rootParent = nullptr;
        clone->bones = nullptr;
        clone->frameID = -1;
        for (uint32_t& value : clone->unknown1C) {
            value = 0;
        }
    }
    SkinInstance_CopyMembers(self, clone, cloning);
    clone->rootParent = static_cast<NiAVObject*>(CloneOf(cloning, self->rootParent));
    uint32_t count = BoneCount(self);
    clone->bones = static_cast<re::Ptr32<NiAVObject>*>(NiMalloc(count * 4));
    for (uint32_t i = 0; i < count; i++) {
        clone->bones[i] = static_cast<NiAVObject*>(CloneOf(cloning, self->bones[i]));
    }
    return clone;
}

// 0x777460: GetViewerStrings (slot 9): the root parent, the partitions, the bones' names sorted
void __fastcall NiSkinInstance_GetViewerStrings(NiSkinInstance* self, re::Edx, NiTArray<char*>* strings) {
    Object_GetViewerStrings(self, strings);
    AddViewerString(strings, GetViewerString(NiSkinInstance_RTTI.name));
    if (self->rootParent != nullptr) {
        AddViewerString(strings, GetViewerStringString("Root Parent", self->rootParent->name));
    }
    NiObject* skinData = self->skinData;
    if (skinData == nullptr) {
        return;
    }
    uint32_t partitions = self->partition != nullptr ? At<uint32_t>(self->partition, 8) : 0;
    AddViewerString(strings, GetViewerStringUint("Hardware partitions", partitions));
    uint32_t count = At<uint32_t>(skinData, 0x44);
    AddViewerString(strings, GetViewerStringUint("Bone Count", count));
    re::Ptr32<const char>* names = static_cast<re::Ptr32<const char>*>(NiMalloc(count * 4));
    for (uint32_t i = 0; i < count; i++) {
        names[i] = self->bones[i]->name;
    }
    crt::Qsort(names, count, 4, reinterpret_cast<Compare>(CompareNames_address));
    for (uint32_t i = 0; i < count; i++) {
        char label[12];
        NiSprintf_(label, 10, "  %3d", i);
        const char* name = names[i] != nullptr ? static_cast<const char*>(names[i]) : "<noname>";
        AddViewerStringInline(strings, GetViewerStringString(label, name));
    }
    NiFree(names);
}

#define NISKININST2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISKININST2_REVERSED("NiSkinInstance::CompareNames", 0x00776EB0, NiSkinInstance_CompareNames);
NISKININST2_REVERSED("NiSkinInstance::LoadBinary", 0x00776F70, NiSkinInstance_LoadBinary);
NISKININST2_REVERSED("NiSkinInstance::LinkObject", 0x00776FB0, NiSkinInstance_LinkObject);
NISKININST2_REVERSED("NiSkinInstance::RegisterStreamables", 0x00777090, NiSkinInstance_RegisterStreamables);
NISKININST2_REVERSED("NiSkinInstance::IsEqual", 0x00777100, NiSkinInstance_IsEqual);
NISKININST2_REVERSED("NiSkinInstance::GetRTTI", 0x00777160, NiSkinInstance_GetRTTI);
NISKININST2_REVERSED("NiSkinInstance::DeletingDestructor", 0x00777170, NiSkinInstance_DeletingDestructor);
NISKININST2_REVERSED("NiSkinInstance::CopyMembers", 0x00777190, NiSkinInstance_CopyMembers);
NISKININST2_REVERSED("NiSkinInstance::PostLinkObject", 0x00777270, NiSkinInstance_PostLinkObject);
NISKININST2_REVERSED("NiSkinInstance::SaveBinary", 0x007772D0, NiSkinInstance_SaveBinary);
NISKININST2_REVERSED("NiSkinInstance::CreateClone", 0x00777370, NiSkinInstance_CreateClone);
NISKININST2_REVERSED("NiSkinInstance::GetViewerStrings", 0x00777460, NiSkinInstance_GetViewerStrings);

} // namespace ni
