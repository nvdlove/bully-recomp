// NiDX9GeometryGroupManager (vtable 0x99D50C on NiGeometryGroupManager's, 0x1C bytes: +0x4 its
// groups (an NiTPrimitiveSet: data, room, count), +0x10 the device (held), +0x14 the vertex buffer
// manager, +0x18 hardware skinning): the groups geometry's vertex data lives in (0 unshared, 1
// shared, 2 dynamic). Its slots: 0 the deleting destructor, 1 CreateGroup, 2 DestroyGroup, 3
// IsGroupEmpty, 4 AddObjectToGroup(group, data), 5 AddObjectToGroup(group, data, skin), 6
// RemoveObjectFromGroup(data), 7 RemoveObjectFromGroup(data, skin). A group: +0x4 its object
// count, +0x8 the device; slots 0 (no argument: its own destruction when empty), 1 AddObject, 2
// AddObject(data, skin, partition), 3 RemoveObject, 4 RemovePartition, 5 RemoveObject(data), 9 its
// deleting destructor.
//
// Also NiDX9Renderer's D3D error names (0x899150).

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

constexpr uintptr_t kGroupManagerVtable = 0x0099D50C;

GAME_FN(void*, __thiscall, GroupManagerBase_Construct, 0x008CC6E0, void* self);   // NiGeometryGroupManager's
GAME_FN(void, __thiscall, GroupManagerBase_Destruct, 0x008CC6F0, void* self);
GAME_FN(void, __thiscall, PrimitiveSet_Realloc, 0x0083AD20, void* set, uint32_t size);
GAME_FN(void, __cdecl, GeometryGroups_ReleaseStatics, 0x008A4C20);
GAME_FN(void*, __cdecl, UnsharedGeometryGroup_Create, 0x008A5470);
GAME_FN(void*, __cdecl, SharedGeometryGroup_Create, 0x008A55A0);
GAME_FN(void*, __cdecl, DynamicGeometryGroup_Create, 0x008A6020);
// this file's (called where modes apply)
GAME_FN(uint8_t, __thiscall, GroupManager_DestroyPartitionData, 0x008993B0, void* self, uint8_t* partition);
GAME_FN(uint8_t, __thiscall, GroupManager_AddObjectToGroup, 0x00899470, void* self, void* group, uint8_t* data,
        uint8_t* skin, int hardwareSkinned, uint32_t bonesPerPartition, uint32_t bonesPerVertex);
GAME_FN(void, __thiscall, GroupManager_Destruct, 0x00899410, void* self);

// a group destroyed when it holds nothing
inline void DestroyIfEmpty(void* group) {
    if (At<uint32_t>(group, 4) == 0) {
        Virtual<void>(group, 0);
    }
}

} // namespace

// 0x899150 (cdecl): NiDX9Renderer::GetErrorString (the D3D error's name; the game's strings)
const char* __cdecl NiDX9Renderer_GetErrorString(uint32_t hr) {
    switch (hr) {
    case 0x88760818:
        return reinterpret_cast<const char*>(0x0099D4B0);   // D3DERR_WRONGTEXTUREFORMAT
    case 0x80070057:
        return reinterpret_cast<const char*>(0x0099D4E4);   // E_INVALIDARG
    case 0x80004005:
        return reinterpret_cast<const char*>(0x0099D4F4);   // E_FAIL
    case 0x8007000E:
        return reinterpret_cast<const char*>(0x0099D4FC);   // E_OUTOFMEMORY
    case 0x8876017C:
        return reinterpret_cast<const char*>(0x0099D4CC);   // D3DERR_OUTOFVIDEOMEMORY
    case 0x88760819:
        return reinterpret_cast<const char*>(0x0099D308);
    case 0x8876081A:
        return reinterpret_cast<const char*>(0x0099D32C);
    case 0x8876081B:
        return reinterpret_cast<const char*>(0x0099D348);
    case 0x8876081C:
        return reinterpret_cast<const char*>(0x0099D36C);
    case 0x8876081D:
        return reinterpret_cast<const char*>(0x0099D388);
    case 0x8876081E:
        return reinterpret_cast<const char*>(0x0099D470);
    case 0x8876081F:
        return reinterpret_cast<const char*>(0x0099D2E8);
    case 0x88760821:
        return reinterpret_cast<const char*>(0x0099D490);
    case 0x88760822:
        return reinterpret_cast<const char*>(0x0099D2C8);
    case 0x88760826:
        return reinterpret_cast<const char*>(0x0099D44C);
    case 0x88760827:
        return reinterpret_cast<const char*>(0x0099D404);
    case 0x88760866:
        return reinterpret_cast<const char*>(0x0099D3A4);
    case 0x88760867:
        return reinterpret_cast<const char*>(0x0099D3C8);
    case 0x88760868:
        return reinterpret_cast<const char*>(0x0099D438);
    case 0x88760869:
        return reinterpret_cast<const char*>(0x0099D420);
    case 0x8876086A:
        return reinterpret_cast<const char*>(0x0099D3B4);
    case 0x8876086B:
        return reinterpret_cast<const char*>(0x0099D3D8);
    case 0x8876086C:
        return reinterpret_cast<const char*>(0x0099D3F0);
    default:
        return reinterpret_cast<const char*>(0x009613F0);   // "UNKNOWN!"
    }
}

// 0x8992D0 (thiscall): NiDX9GeometryGroupManager::SetHWSkinning
void __fastcall NiDX9GeometryGroupManager_SetHWSkinning(uint8_t* self, re::Edx, int hardware) {
    At<uint8_t>(self, 0x18) = uint8_t(hardware);
}

// 0x8992E0 (thiscall): NiDX9GeometryGroupManager::DestroyGroup (an empty group taken out of the set
// (the last moved into its place; not found: the slot before the set's start, as the original
// writes it) and deleted)
uint8_t __fastcall NiDX9GeometryGroupManager_DestroyGroup(uint8_t* self, re::Edx, void* group) {
    if (group == nullptr || Virtual<uint8_t>(self, 3, group) == 0) {
        return 0;
    }
    re::Ptr32<void>* groups = At<re::Ptr32<void>*>(self, 4);
    int index = -1;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0xC); i++) {
        if (groups[i] == group) {
            index = int(i);
            break;
        }
    }
    At<uint32_t>(self, 0xC)--;
    groups[index] = groups[At<uint32_t>(self, 0xC)];
    Virtual<void*>(group, 9, 1);
    return 1;
}

// 0x899340 (thiscall): NiDX9GeometryGroupManager::IsGroupEmpty
uint8_t __fastcall NiDX9GeometryGroupManager_IsGroupEmpty(void*, re::Edx, void* group) {
    return At<uint32_t>(group, 4) == 0 ? 1 : 0;
}

// 0x899350 (thiscall): NiDX9GeometryGroupManager::AddObjectToGroup (unless it's in one)
uint8_t __fastcall NiDX9GeometryGroupManager_AddObjectToGroupData(void*, re::Edx, void* group, uint8_t* data) {
    if (At<void*>(data, 0x1C) != nullptr) {
        return 0;
    }
    Virtual<void>(group, 1, data);
    return 1;
}

// 0x899370 (thiscall): NiDX9GeometryGroupManager::RemoveObjectFromGroup (its group destroyed when
// it's left empty)
uint8_t __fastcall NiDX9GeometryGroupManager_RemoveObjectFromGroupData(void*, re::Edx, uint8_t* data) {
    uint8_t* owner = At<uint8_t*>(data, 0x1C);
    if (owner == nullptr) {
        return 0;
    }
    void* group = At<void*>(owner, 8);
    Virtual<void>(group, 3, data);
    DestroyIfEmpty(group);
    return 1;
}

// 0x8993B0 (thiscall): NiDX9GeometryGroupManager::DestroyPartitionData (every partition's data out
// of its group (the first partition's group the one checked for emptiness, read even when it has
// none, as the original does))
uint8_t __fastcall NiDX9GeometryGroupManager_DestroyPartitionData(void*, re::Edx, uint8_t* partition) {
    uint32_t count = At<uint32_t>(partition, 8);
    uint8_t* part = At<uint8_t*>(partition, 0xC);
    void* firstGroup = At<void*>(At<uint8_t*>(part, 0x28), 8);
    uint8_t removed = 0;
    for (uint32_t i = count; i != 0; i--, part += 0x2C) {
        uint8_t* data = At<uint8_t*>(part, 0x28);
        if (data != nullptr) {
            Virtual<void>(At<void*>(data, 8), 4, part);
            removed = 1;
        }
    }
    if (removed != 0 && At<uint32_t>(firstGroup, 4) == 0) {
        Virtual<void>(firstGroup, 0);
    }
    return removed;
}

// 0x899410 (thiscall): NiDX9GeometryGroupManager's destructor (every group deleted, the groups'
// statics released, the device let go)
void __fastcall NiDX9GeometryGroupManager_Destruct(uint8_t* self, re::Edx) {
    uint32_t count = At<uint32_t>(self, 0xC);
    At<uint32_t>(self, 0) = kGroupManagerVtable;
    for (uint32_t i = 0; i < count; i++) {
        void* group = At<re::Ptr32<void>*>(self, 4)[i];
        if (group != nullptr) {
            Virtual<void*>(group, 9, 1);
        }
    }
    GeometryGroups_ReleaseStatics();
    if (At<void*>(self, 0x10) != nullptr) {
        Com<long>(At<void*>(self, 0x10), 2);   // Release
    }
    NiFree(At<void*>(self, 4));
    GroupManagerBase_Destruct(self);
}

// 0x899470 (thiscall, the manager unused): NiDX9GeometryGroupManager::AddObjectToGroup (a hardware
// skinned geometry's partitions each added, unless the first already is in one; else the geometry's
// data, unless it's in one already; the bone counts aren't used)
uint8_t __fastcall NiDX9GeometryGroupManager_AddObjectToGroup(void*, re::Edx, void* group, uint8_t* data,
                                                               uint8_t* skin, int hardwareSkinned, uint32_t,
                                                               uint32_t) {
    if (skin != nullptr && uint8_t(hardwareSkinned) != 0) {
        uint8_t* partition = At<uint8_t*>(skin, 0xC);
        if (partition != nullptr) {
            uint8_t* part = At<uint8_t*>(partition, 0xC);
            if (At<void*>(part, 0x28) != nullptr) {
                return 0;
            }
            for (uint32_t i = At<uint32_t>(partition, 8); i != 0; i--, part += 0x2C) {
                Virtual<void>(group, 2, data, skin, part);
            }
            return 1;
        }
    }
    if (At<void*>(data, 0x3C) != nullptr) {
        return 0;
    }
    Virtual<void>(group, 2, data, skin, static_cast<void*>(nullptr));
    return 1;
}

// 0x8994F0 (thiscall): NiDX9GeometryGroupManager::RemoveObjectFromGroup (a skinned geometry's
// partitions; else its data, its group destroyed when left empty)
uint8_t __fastcall NiDX9GeometryGroupManager_RemoveObjectFromGroup(uint8_t* self, re::Edx, uint8_t* data, uint8_t* skin) {
    if (skin != nullptr && At<uint8_t*>(skin, 0xC) != nullptr) {
        return GroupManager_DestroyPartitionData(self, At<uint8_t*>(skin, 0xC));
    }
    uint8_t* owner = At<uint8_t*>(data, 0x3C);
    if (owner == nullptr) {
        return 0;
    }
    void* group = At<void*>(owner, 8);
    Virtual<void>(group, 5, data);
    DestroyIfEmpty(group);
    return 1;
}

// 0x899540 (thiscall): NiDX9GeometryGroupManager's deleting destructor
void* __fastcall NiDX9GeometryGroupManager_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    GroupManager_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x1C);
    }
    return self;
}

// 0x899560 (cdecl): NiDX9GeometryGroupManager::Create (the device held)
void* __cdecl NiDX9GeometryGroupManager_Create(void* device, void* vbManager) {
    uint8_t* manager = static_cast<uint8_t*>(NiNew(0x1C));
    if (manager != nullptr) {
        GroupManagerBase_Construct(manager);
        At<uint32_t>(manager, 0) = kGroupManagerVtable;
        At<void*>(manager, 4) = nullptr;
        At<uint32_t>(manager, 8) = 0;
        At<uint32_t>(manager, 0xC) = 0;
        At<void*>(manager, 0x10) = nullptr;
        At<void*>(manager, 0x14) = nullptr;
        At<uint8_t>(manager, 0x18) = 0;
    }
    At<void*>(manager, 0x10) = device;
    Com<long>(device, 1);   // AddRef
    At<void*>(manager, 0x14) = vbManager;
    return manager;
}

// 0x8995C0 (thiscall): NiDX9GeometryGroupManager::CreateGroup (a new group of the type, added to the
// set unless it's there, given the device)
void* __fastcall NiDX9GeometryGroupManager_CreateGroup(uint8_t* self, re::Edx, uint32_t type) {
    void* group;
    switch (type) {
    case 0:
        group = UnsharedGeometryGroup_Create();
        break;
    case 1:
        group = SharedGeometryGroup_Create();
        break;
    case 2:
        group = DynamicGeometryGroup_Create();
        break;
    default:
        return nullptr;
    }
    if (group == nullptr) {
        return nullptr;
    }
    uint32_t count = At<uint32_t>(self, 0xC);
    for (uint32_t i = 0; i < count; i++) {
        if (At<re::Ptr32<void>*>(self, 4)[i] == group) {
            if (int(i) >= 0) {
                return group;
            }
            break;
        }
    }
    uint32_t alloced = At<uint32_t>(self, 8);
    if (count == alloced) {
        PrimitiveSet_Realloc(self + 4, alloced > 0 ? alloced * 2 : 1);
    }
    At<re::Ptr32<void>*>(self, 4)[At<uint32_t>(self, 0xC)] = group;
    At<uint32_t>(self, 0xC)++;
    void* device = At<void*>(self, 0x10);
    At<void*>(group, 8) = device;
    Com<long>(device, 1);   // AddRef
    return group;
}

// 0x899660 (thiscall): NiDX9GeometryGroupManager::AddObjectToGroup(group, data, skin) (with its
// hardware skinning)
uint8_t __fastcall NiDX9GeometryGroupManager_AddObjectToGroupSkin(uint8_t* self, re::Edx, void* group, uint8_t* data,
                                                                  uint8_t* skin) {
    return GroupManager_AddObjectToGroup(self, group, data, skin, At<uint8_t>(self, 0x18), 0, 0);
}

#define C40L_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40L_REVERSED("NiDX9Renderer::GetErrorString", 0x00899150, NiDX9Renderer_GetErrorString);
C40L_REVERSED("NiDX9GeometryGroupManager::SetHWSkinning", 0x008992D0, NiDX9GeometryGroupManager_SetHWSkinning);
C40L_REVERSED("NiDX9GeometryGroupManager::DestroyGroup", 0x008992E0, NiDX9GeometryGroupManager_DestroyGroup);
C40L_REVERSED("NiDX9GeometryGroupManager::IsGroupEmpty", 0x00899340, NiDX9GeometryGroupManager_IsGroupEmpty);
C40L_REVERSED("NiDX9GeometryGroupManager::AddObjectToGroup_899350", 0x00899350,
              NiDX9GeometryGroupManager_AddObjectToGroupData);
C40L_REVERSED("NiDX9GeometryGroupManager::RemoveObjectFromGroup_899370", 0x00899370,
              NiDX9GeometryGroupManager_RemoveObjectFromGroupData);
C40L_REVERSED("NiDX9GeometryGroupManager::DestroyPartitionData", 0x008993B0,
              NiDX9GeometryGroupManager_DestroyPartitionData);
C40L_REVERSED("NiDX9GeometryGroupManager::~NiDX9GeometryGroupManager", 0x00899410, NiDX9GeometryGroupManager_Destruct);
C40L_REVERSED("NiDX9GeometryGroupManager::AddObjectToGroup", 0x00899470, NiDX9GeometryGroupManager_AddObjectToGroup);
C40L_REVERSED("NiDX9GeometryGroupManager::RemoveObjectFromGroup", 0x008994F0,
              NiDX9GeometryGroupManager_RemoveObjectFromGroup);
C40L_REVERSED("NiDX9GeometryGroupManager::DeletingDestructor", 0x00899540, NiDX9GeometryGroupManager_DeletingDestructor);
C40L_REVERSED("NiDX9GeometryGroupManager::Create", 0x00899560, NiDX9GeometryGroupManager_Create);
C40L_REVERSED("NiDX9GeometryGroupManager::CreateGroup", 0x008995C0, NiDX9GeometryGroupManager_CreateGroup);
C40L_REVERSED("NiDX9GeometryGroupManager::AddObjectToGroup_899660", 0x00899660,
              NiDX9GeometryGroupManager_AddObjectToGroupSkin);

} // namespace ni::c40
