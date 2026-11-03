// The trigger DAT manager (DATManager, the object at 0x20C7C48, 0x6D2190..0x6D4840): the
// pools of the placement objects the area DAT files hold (triggers, perimeters, paths,
// point lists, POIs, spawners, schedules...), loading an area's DAT out of
// Dat\Trigger.IMG, unloading areas, and those objects' small constructors and vector
// deleting destructors.
//
// DATManager (by offset): +0 .. +0x50 the pools (see Init), +0x54 the loaded DATs'
// hashes, +0x1E4 their areas (bytes), +0x248 how many (signed byte), +0x24C the area
// being loaded.

#include <cstdint>

#include "game/Pool.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using AnyPool = CPool<uint8_t>;

using re::At;

GAME_FN(void*, __cdecl, DM_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, DM_Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, DM_VectorConstruct, 0x00401240, void* array, uint32_t size, int count, void* ctor);
GAME_FN(void*, __thiscall, DM_Spawner_Construct, 0x004A1220, void* spawner, int, int, int, int);
GAME_FN(void, __thiscall, DM_ObjectModelInfo_Construct, 0x00464E00, void* info);
GAME_FN(void, __thiscall, DM_ObjectModelInfo_Destruct, 0x004647A0, void* info);

// the "link" arrays of the record pools: a uint16 next per slot, the last at +8
struct LinkArray {
    int32_t count;
    re::Ptr32<uint16_t> links;
    uint16_t last;
};
GAME_VAR(LinkArray, dm_poiLinks, 0x00BC5D04);
GAME_VAR(LinkArray, dm_pathNodeLinks, 0x00BC5D10);
GAME_VAR(LinkArray, dm_perimeterPointLinks, 0x00BC5D1C);
GAME_VAR(LinkArray, dm_pointLinks, 0x00BC5D28);
GAME_VAR(LinkArray, dm_railNodeLinks, 0x00BC5D34);
GAME_VAR(void*, dm_directory, 0x020C7C40);             // Trigger.IMG's CDirectory
GAME_VAR(void*, dm_areaTransitions, 0x009E4F4C);
inline void* const dm_triggers = reinterpret_cast<void*>(0x020C7E98);
inline void* const dm_pointLists = reinterpret_cast<void*>(0x00BC5E7C);
GAME_VAR(void*, dm_pathStore, 0x00D13CB4);
GAME_VAR(void*, dm_poiManager, 0x00C0F5E4);
GAME_VAR(AnyPool*, dm_propAnimPool, 0x00C0F608);
// the DAT's section names
GAME_VAR(const char*, dm_sectionPaths, 0x00BC5CF0);
GAME_VAR(const char*, dm_sectionPointLists, 0x00BC5CF4);
GAME_VAR(const char*, dm_sectionTriggersA, 0x00BC5CF8);
GAME_VAR(const char*, dm_sectionTriggersB, 0x00BC5CFC);
GAME_VAR(const char*, dm_sectionPois, 0x00BC5D00);
constexpr uintptr_t VT_CDirectory = 0x009186D0;

} // namespace

#define DATMGR_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// ---------------------------------------------------------------------------------------
// the objects' constructors (the base ones only set the vtable)

namespace {
template <uintptr_t vtable>
void* __fastcall SetVtable(uintptr_t* self, re::Edx) {
    *self = vtable;
    return self;
}
} // namespace

DATMGR_REVERSED("Path::Path_6D2190", 0x006D2190, &SetVtable<0x0092CF90>);
DATMGR_REVERSED("POIList::POIList_6D21A0", 0x006D21A0, &SetVtable<0x00905840>);
DATMGR_REVERSED("PointList::PointList_6D21B0", 0x006D21B0, &SetVtable<0x0094326C>);
DATMGR_REVERSED("RailPointList::RailPointList_6D21C0", 0x006D21C0, &SetVtable<0x00914158>);
DATMGR_REVERSED("Perimeter::Perimeter_6D21F0", 0x006D21F0, &SetVtable<0x0094328C>);
DATMGR_REVERSED("CSpawnLocation::CSpawnLocation_6D2200", 0x006D2200, &SetVtable<0x00912300>);

// 0x6D21D0 (thiscall): Trigger's (its 6 floats at +4 zeroed)
void* __fastcall Trigger_BaseConstruct(uint8_t* self, re::Edx) {
    for (int i = 0; i < 6; i++) {
        At<float>(self, 4 + i * 4) = 0.0f;
    }
    At<uint32_t>(self, 0) = 0x0094327C;
    return self;
}
DATMGR_REVERSED("Trigger::Trigger_6D21D0", 0x006D21D0, &Trigger_BaseConstruct);

// 0x6D2750 (thiscall): CSpawner's default: CSpawner(1, 0, 0, -1)
void* __fastcall Spawner_DefaultConstruct(void* self, re::Edx) {
    return DM_Spawner_Construct(self, 1, 0, 0, -1);
}
DATMGR_REVERSED("CSpawner::CSpawner_6D2750", 0x006D2750, &Spawner_DefaultConstruct);

// 0x6D2900 (thiscall): Point (at the origin)
void* __fastcall Point_BaseConstruct(uint8_t* self, re::Edx) {
    At<float>(self, 4) = 0.0f;
    At<uint32_t>(self, 0) = 0x00914150;
    At<float>(self, 8) = 0.0f;
    At<float>(self, 0xC) = 0.0f;
    return self;
}
DATMGR_REVERSED("Point::Point_6D2900", 0x006D2900, &Point_BaseConstruct);

// 0x6D2920 (thiscall): CMissionSpawnLocation (its two model lists)
void* __fastcall MissionSpawnLocation_BaseConstruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0x00912358;
    DM_ObjectModelInfo_Construct(self + 0x14);
    At<uint32_t>(self, 0x14) = 0x00900CF4;
    DM_ObjectModelInfo_Construct(self + 0x28);
    At<uint32_t>(self, 0x28) = 0x00911DBC;
    return self;
}
DATMGR_REVERSED("CMissionSpawnLocation::CMissionSpawnLocation_6D2920", 0x006D2920, &MissionSpawnLocation_BaseConstruct);

// 0x6D2950 (thiscall): CMissionSpawner
void* __fastcall MissionSpawner_BaseConstruct(uint8_t* self, re::Edx) {
    DM_Spawner_Construct(self, 1, 0, 0, -1);
    At<uint32_t>(self, 0) = 0x00912328;
    At<uint32_t>(self, 0xCC) = 0;
    At<uint32_t>(self, 0x170) = 0;
    return self;
}
DATMGR_REVERSED("CMissionSpawner::CMissionSpawner_6D2950", 0x006D2950, &MissionSpawner_BaseConstruct);

// 0x6D2980 (thiscall): CAmbientSpawner
void* __fastcall AmbientSpawner_BaseConstruct(uint8_t* self, re::Edx) {
    DM_Spawner_Construct(self, 1, 0, 0, -1);
    At<uint32_t>(self, 0) = 0x00912340;
    At<uint32_t>(self, 0x110) = 0;
    return self;
}
DATMGR_REVERSED("CAmbientSpawner::CAmbientSpawner_6D2980", 0x006D2980, &AmbientSpawner_BaseConstruct);

// 0x6D29B0 (thiscall): PedSchedule (its two lists empty)
void* __fastcall PedSchedule_BaseConstruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0xA0) = 0;
    At<uint32_t>(self, 0xF4) = 0;
    return self;
}
DATMGR_REVERSED("PedSchedule::PedSchedule_6D29B0", 0x006D29B0, &PedSchedule_BaseConstruct);

// ---------------------------------------------------------------------------------------
// vector deleting destructors: flag 2 an array (its count before it, destroyed last
// first), flag 1 freed (an array through CMemoryMgr, one through its operator delete).
// The destructor: `vtable` set first (0: not), then `dtor` (0: none).

namespace {
template <uintptr_t vtable, uintptr_t dtor>
void DestroyOne(uint8_t* object) {
    if constexpr (vtable != 0) {
        At<uint32_t>(object, 0) = vtable;
    }
    if constexpr (dtor != 0) {
        reinterpret_cast<void(__thiscall*)(void*)>(dtor)(object);
    }
}

template <uint32_t size, uintptr_t vtable, uintptr_t dtor, uintptr_t del>
void* __fastcall VectorDeletingDestructor(uint8_t* self, re::Edx, int flags) {
    if ((flags & 2) != 0) {
        uint8_t* block = self - 4;
        int32_t count = *reinterpret_cast<int32_t*>(block);
        uint8_t* object = self + count * size;
        for (int32_t i = count - 1; i >= 0; i--) {
            object -= size;
            DestroyOne<vtable, dtor>(object);
        }
        if ((flags & 1) != 0) {
            DM_Free(block);
        }
        return block;
    }
    DestroyOne<vtable, dtor>(self);
    if ((flags & 1) != 0) {
        reinterpret_cast<void(__cdecl*)(void*)>(del)(self);
    }
    return self;
}
} // namespace

DATMGR_REVERSED("Trigger::VectorDeletingDestructor", 0x006D23C0, (&VectorDeletingDestructor<0x40, 0, 0x006D6820, 0x006D6F70>));
DATMGR_REVERSED("CStaticPOI::VectorDeletingDestructor", 0x006D2430,
                (&VectorDeletingDestructor<0xA4, 0x00901858, 0x00447950, 0x004583F0>));
DATMGR_REVERSED("Perimeter::VectorDeletingDestructor", 0x006D24B0, (&VectorDeletingDestructor<0x30, 0, 0x006D57C0, 0x006D54F0>));
DATMGR_REVERSED("Path::VectorDeletingDestructor", 0x006D2520, (&VectorDeletingDestructor<0x14, 0, 0x005ECF80, 0x005ECAF0>));
DATMGR_REVERSED("PointList::VectorDeletingDestructor", 0x006D2580, (&VectorDeletingDestructor<0x1C, 0, 0x006D6240, 0x006D5A40>));
DATMGR_REVERSED("POIList::VectorDeletingDestructor", 0x006D25F0, (&VectorDeletingDestructor<0x1C, 0, 0x00447A60, 0x00447CF0>));
DATMGR_REVERSED("CDynamicPOI::VectorDeletingDestructor", 0x006D2660,
                (&VectorDeletingDestructor<0xA8, 0x00901858, 0x00447950, 0x00429470>));
DATMGR_REVERSED("CSpawner::VectorDeletingDestructor", 0x006D26E0, (&VectorDeletingDestructor<0xBC, 0, 0x004A1260, 0x004A2390>));
DATMGR_REVERSED("CSpawnLocation::VectorDeletingDestructor", 0x006D2760, (&VectorDeletingDestructor<0x14, 0x00912300, 0, 0x004A25A0>));
DATMGR_REVERSED("CMissionSpawner::VectorDeletingDestructor", 0x006D27C0,
                (&VectorDeletingDestructor<0x174, 0, 0x004A2060, 0x004A2440>));
DATMGR_REVERSED("Point::VectorDeletingDestructor", 0x006D2830, (&VectorDeletingDestructor<0x10, 0, 0x004F0290, 0x004F07E0>));
DATMGR_REVERSED("RailPointList::VectorDeletingDestructor", 0x006D28A0, (&VectorDeletingDestructor<0xC, 0, 0x004F0810, 0x004F0820>));
DATMGR_REVERSED("CPOIGroup::VectorDeletingDestructor", 0x006D3A20,
                (&VectorDeletingDestructor<0x110, 0x0090FAC4, 0x00489670, 0x0048A150>));
DATMGR_REVERSED("CAmbientSpawner::VectorDeletingDestructor", 0x006D3AA0,
                (&VectorDeletingDestructor<0x114, 0x00912340, 0x004A1260, 0x004A2570>));

namespace {
// CMissionSpawnLocation's destructor: its two model lists, then CSpawnLocation's vtable
void MissionSpawnLocation_Destroy(uint8_t* object) {
    At<uint32_t>(object, 0) = 0x00912358;
    DM_ObjectModelInfo_Destruct(object + 0x28);
    DM_ObjectModelInfo_Destruct(object + 0x14);
    At<uint32_t>(object, 0) = 0x00912300;
}
} // namespace

// 0x6D3B20 (thiscall)
void* __fastcall MissionSpawnLocation_VectorDeletingDestructor(uint8_t* self, re::Edx, int flags) {
    if ((flags & 2) != 0) {
        uint8_t* block = self - 4;
        int32_t count = *reinterpret_cast<int32_t*>(block);
        uint8_t* object = self + count * 0x3C;
        for (int32_t i = count - 1; i >= 0; i--) {
            object -= 0x3C;
            MissionSpawnLocation_Destroy(object);
        }
        if ((flags & 1) != 0) {
            DM_Free(block);
        }
        return block;
    }
    MissionSpawnLocation_Destroy(self);
    if ((flags & 1) != 0) {
        reinterpret_cast<void(__cdecl*)(void*)>(0x004A2540)(self);
    }
    return self;
}
DATMGR_REVERSED("CMissionSpawnLocation::VectorDeletingDestructor", 0x006D3B20, &MissionSpawnLocation_VectorDeletingDestructor);

// ---------------------------------------------------------------------------------------
// the pools' Init / Flush (CPool<T>, as Pools.cpp's)

namespace {
uint32_t SaturatingMul(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return (bytes >> 32) != 0 ? 0xFFFFFFFF : uint32_t(bytes);
}

// CPool<T>::Init: the arrays for `size` slots, all free. With unknown19 the objects are
// a new[] array (a count before it when `counted`), each made by `ctor` (0: none).
template <int32_t elemSize, bool counted, uintptr_t ctor>
void __fastcall DmPoolInit(AnyPool* pool, re::Edx) {
    pool->ownsArrays = 1;
    pool->elemSize = elemSize;
    if (!pool->IsConstructedArray()) {
        pool->objects = static_cast<uint8_t*>(DM_Malloc(uint32_t(pool->size * elemSize)));
    } else {
        uint32_t count = uint32_t(pool->size);
        uint32_t bytes = SaturatingMul(count, elemSize);
        if constexpr (counted) {
            bytes = bytes > 0xFFFFFFFB ? 0xFFFFFFFF : bytes + 4;
        }
        auto* array = static_cast<uint8_t*>(DM_Malloc(bytes));
        if (array != nullptr) {
            if constexpr (counted) {
                *reinterpret_cast<uint32_t*>(array) = count;
                array += 4;
            }
            if constexpr (ctor != 0) {
                DM_VectorConstruct(array, elemSize, int(count), reinterpret_cast<void*>(ctor));
            }
        }
        pool->objects = array;
    }
    pool->flags = static_cast<uint8_t*>(DM_Malloc(uint32_t(pool->size)));
    pool->lastUsed = 0;
    for (int slot = 0; slot < pool->size; slot++) {
        pool->flags[slot] |= 0x80;
        pool->flags[slot] &= 0x80;
    }
}

// CPool<T>::Flush with `dtor` on each element of a new[] array (last first)
template <int32_t elemSize, uintptr_t dtor>
void __fastcall DmPoolFlushEach(AnyPool* pool, re::Edx) {
    if (pool->size <= 0) {
        return;
    }
    if (pool->ownsArrays) {
        uint8_t* objects = pool->objects;
        if (!pool->IsConstructedArray()) {
            DM_Free(objects);
        } else if (objects != nullptr) {
            int32_t* count = reinterpret_cast<int32_t*>(objects) - 1;
            for (int i = *count - 1; i >= 0; i--) {
                reinterpret_cast<void(__thiscall*)(void*)>(dtor)(objects + elemSize * i);
            }
            DM_Free(count);
        }
        DM_Free(pool->flags);
    }
    pool->objects = nullptr;
    pool->flags = nullptr;
    pool->size = 0;
    pool->lastUsed = 0;
}
} // namespace

DATMGR_REVERSED("CPool<Trigger>::Init", 0x006D29D0, (&DmPoolInit<0x40, true, 0x006D21D0>));
DATMGR_REVERSED("CPool<CStaticPOI>::Init", 0x006D2A80, (&DmPoolInit<0xA4, true, 0x004292D0>));
DATMGR_REVERSED("CPool<PointRecord>::Init", 0x006D2B40, (&DmPoolInit<0x2C, false, 0x006D57E0>));
DATMGR_REVERSED("CPool<PathNode>::Init", 0x006D2BF0, (&DmPoolInit<0x24, true, 0x005EC570>));
DATMGR_REVERSED("CPool<PathNode>::Flush", 0x006D2CA0, (&DmPoolFlushEach<0x24, 0x005EC7B0>));
DATMGR_REVERSED("CPool<PerimeterPoint>::Init", 0x006D2D20, (&DmPoolInit<8, false, 0>));
DATMGR_REVERSED("CPool<Perimeter>::Init", 0x006D2DA0, (&DmPoolInit<0x30, true, 0x006D21F0>));
DATMGR_REVERSED("CPool<Path>::Init", 0x006D2E50, (&DmPoolInit<0x14, true, 0x006D2190>));
DATMGR_REVERSED("CPool<PointList>::Init", 0x006D2F00, (&DmPoolInit<0x1C, true, 0x006D21B0>));
DATMGR_REVERSED("CPool<POIList>::Init", 0x006D2FC0, (&DmPoolInit<0x1C, true, 0x006D21A0>));
DATMGR_REVERSED("CPool<PatrolEntry>::Init", 0x006D3080, (&DmPoolInit<0x28, true, 0x00471E30>));
DATMGR_REVERSED("CPool<PatrolEntry>::Flush", 0x006D3140, (&DmPoolFlushEach<0x28, 0x00472420>));
DATMGR_REVERSED("CPool<CPOIGroup>::Init", 0x006D31C0, (&DmPoolInit<0x110, true, 0x0048A000>));
DATMGR_REVERSED("CPool<CDynamicPOI>::Init", 0x006D3280, (&DmPoolInit<0xA8, true, 0x004292F0>));
DATMGR_REVERSED("CPool<PedSchedule>::Init", 0x006D3340, (&DmPoolInit<0x10C, true, 0x006D29B0>));
DATMGR_REVERSED("CPool<PedSchedule>::Flush", 0x006D3400, (&DmPoolFlushEach<0x10C, 0x0046EB70>));
DATMGR_REVERSED("CPool<CSpawner>::Init", 0x006D3490, (&DmPoolInit<0xBC, true, 0x006D2750>));
DATMGR_REVERSED("CPool<Pool0xC>::Init_6D3550", 0x006D3550, (&DmPoolInit<0xC, false, 0>));
DATMGR_REVERSED("CPool<CSpawnLocation>::Init", 0x006D35D0, (&DmPoolInit<0x14, true, 0x006D2200>));
DATMGR_REVERSED("CPool<CAmbientSpawner>::Init", 0x006D3680, (&DmPoolInit<0x114, true, 0x006D2980>));
DATMGR_REVERSED("CPool<CMissionSpawner>::Init", 0x006D3740, (&DmPoolInit<0x174, true, 0x006D2950>));
DATMGR_REVERSED("CPool<CMissionSpawnLocation>::Init", 0x006D3800, (&DmPoolInit<0x3C, true, 0x006D2920>));
DATMGR_REVERSED("CPool<Point>::Init", 0x006D38C0, (&DmPoolInit<0x10, true, 0x006D2900>));
DATMGR_REVERSED("CPool<RailPointList>::Init", 0x006D3970, (&DmPoolInit<0xC, true, 0x006D21C0>));

// ---------------------------------------------------------------------------------------
// DATManager

namespace {
GAME_FN(void, __thiscall, DM_PathStore_RemoveArea, 0x005ED170, void* store, int area, int all);
GAME_FN(void, __thiscall, DM_PointLists_RemoveArea, 0x006D6300, void* lists, int area, int all);
GAME_FN(void, __thiscall, DM_Triggers_RemoveArea, 0x006D7740, void* triggers, int area, int all);
GAME_FN(void, __thiscall, DM_Pois_RemoveArea, 0x00448990, void* pois, int area, int all);
GAME_FN(void, __thiscall, DM_AreaTransitions_CollectRouteTriggers, 0x00416150, void* manager);
GAME_FN(void, __thiscall, DM_AreaTransitions_CreateRouteObjects, 0x00415630, void* manager);
GAME_FN(void, __thiscall, DM_Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(void, __cdecl, DM_World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, DM_Fn45E690, 0x0045E690, void* entity);
GAME_FN(void, __thiscall, DM_Nothing, 0x0049A650, void* self);   // (an empty function)
inline int __cdecl DM_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(int32_t, __cdecl, DM_HashName, 0x00576ED0, const char* name);
GAME_FN(void, __cdecl, DM_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, DM_PopMemoryTag, 0x005EF100);
inline char* __cdecl DM_Strncpy(char* dst, const char* src, uint32_t n) { return crt::Strncpy(dst, src, (size_t)n); }
inline char* __cdecl DM_StrNCat(char* dst, const char* src, uint32_t count) { return crt::Strncat(dst, src, (size_t)count); }
GAME_FN(void, __cdecl, DM_LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(void*, __cdecl, DM_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(void, __cdecl, DM_CloseFile, 0x0042D450, void* file);
GAME_FN(uint8_t, __thiscall, DM_Directory_FindItemByHash, 0x004290A0, void* directory, int32_t hash, uint32_t* offset,
        uint32_t* size);
GAME_FN(void*, __thiscall, DM_Directory_Construct, 0x004291B0, void* directory, int capacity);
GAME_FN(void, __thiscall, DM_Directory_ReadDirFile, 0x004291F0, void* directory, const char* path);
GAME_FN(void*, __cdecl, DM_Archive_Open, 0x005B0840, void* file, int, uint32_t offset, uint32_t size);
GAME_FN(void, __cdecl, DM_Archive_Close, 0x005B0880, void* archive);
GAME_FN(void, __thiscall, DM_Triggers_Load, 0x006D7B40, void* triggers, void* archive, int32_t a, int32_t b);
GAME_FN(void, __thiscall, DM_PathStore_Load, 0x005ED060, void* store, void* archive, int32_t count);
GAME_FN(void, __thiscall, DM_PointLists_Load, 0x006D6260, void* lists, void* archive, int32_t count, int);
GAME_FN(void, __thiscall, DM_Pois_Load, 0x00448250, void* pois, void* archive, int32_t count, int);
GAME_FN(void*, __cdecl, DM_Path_New, 0x005EC820, uint32_t size);
GAME_FN(void*, __thiscall, DM_Path_Construct, 0x005EC5E0, void* path, int id);
GAME_FN(void*, __cdecl, DM_Trigger_New, 0x006D6AE0, uint32_t size);
GAME_FN(void*, __thiscall, DM_Trigger_Construct, 0x006D6A70, void* trigger, int id);
GAME_FN(void*, __cdecl, DM_Perimeter_New, 0x006D4CF0, uint32_t size);
GAME_FN(void*, __thiscall, DM_Perimeter_Construct, 0x006D4B30, void* perimeter, int id);
GAME_FN(void*, __cdecl, DM_PointList_New, 0x006D59A0, uint32_t size);
GAME_FN(void*, __thiscall, DM_PointList_Construct, 0x006D5920, void* list, int id);
// the Flushes
GAME_FN(void, __thiscall, DM_PoolFlushVirtual, 0x004498F0, void* pool);
GAME_FN(void, __thiscall, DM_PoolFlushPlain, 0x006D2370, void* pool);
GAME_FN(void, __thiscall, DM_PathNodePool_Flush, 0x006D2CA0, void* pool);
GAME_FN(void, __thiscall, DM_PatrolEntryPool_Flush, 0x006D3140, void* pool);
GAME_FN(void, __thiscall, DM_PedSchedulePool_Flush, 0x006D3400, void* pool);

int8_t& DatCount(uint8_t* self) {
    return At<int8_t>(self, 0x248);
}

// reads a section of the archive: its value, or 0 when it isn't there
int32_t ReadSection(void* archive, const char* name) {
    int32_t value;
    uint8_t ok = vc::Call<uint8_t>(archive, 0x38, &value, name);
    return ok != 0 ? value : 0;
}

// the link array of a record pool, all 0xFFFF
void MakeLinks(LinkArray& links, int32_t count) {
    links.count = count;
    links.links = static_cast<uint16_t*>(DM_Malloc(SaturatingMul(uint32_t(count), 2)));
    for (int32_t i = 0; i < count * 2; i += 2) {
        *re::PtrCast<uint16_t*>(re::PtrCast<uint8_t*>(links.links) + i) = 0xFFFF;
    }
    links.last = 0xFFFF;
}

// a new pool (its size, not a new[] array) made by `init`
AnyPool* NewPool(int32_t size, uintptr_t init) {
    auto* pool = static_cast<AnyPool*>(DM_Malloc(0x1C));
    if (pool == nullptr) {
        return nullptr;
    }
    pool->lastUsed = 0;
    pool->objects = nullptr;
    pool->flags = nullptr;
    At<uint32_t>(pool, 0x14) = 0;
    pool->ownsArrays = 1;
    pool->size = size;
    pool->unknown19 = 0;
    reinterpret_cast<void(__thiscall*)(void*)>(init)(pool);
    return pool;
}

// a pool flushed and freed
void DeletePool(re::Ptr32<AnyPool>& pool, void(__thiscall* flush)(void*)) {
    AnyPool* p = pool;
    if (p != nullptr) {
        flush(p);
        DM_Free(p);
    }
    pool = nullptr;
}
} // namespace

// 0x6D2210 (thiscall): unloads an area's placement objects and forgets its DATs
void __fastcall DATManager_RemoveArea(uint8_t* self, re::Edx, int area) {
    DM_PathStore_RemoveArea(dm_pathStore, area, 0);
    DM_PointLists_RemoveArea(dm_pointLists, area, 0);
    DM_Triggers_RemoveArea(dm_triggers, area, 0);
    DM_Pois_RemoveArea(dm_poiManager, area, 0);
    int32_t i = DatCount(self) - 1;
    while (i >= 0) {
        if (At<int8_t>(self, 0x1E4 + i) != area) {
            i--;
            continue;
        }
        int8_t count = DatCount(self);
        int32_t next;
        if (i == count - 1) {
            next = i - 1;
            count = int8_t(count - 1);
            DatCount(self) = count;
        } else {
            next = i;
            for (int32_t j = i + 1; j < DatCount(self); j++) {
                At<int32_t>(self, 0x50 + j * 4) = At<int32_t>(self, 0x54 + j * 4);
                At<int8_t>(self, 0x1E3 + j) = At<int8_t>(self, 0x1E4 + j);
            }
            DatCount(self) = int8_t(DatCount(self) - 1);
            count = DatCount(self);
        }
        // (the next index: the lower of the two, through floats)
        float a = float(next);
        float b = float(int32_t(count) - 1);
        i = int32_t(b <= a ? b : a);
    }
    DM_AreaTransitions_CollectRouteTriggers(dm_areaTransitions);
}
DATMGR_REVERSED("DATManager::RemoveArea", 0x006D2210, &DATManager_RemoveArea);

// 0x6D2350 (cdecl): the area transitions' routes rebuilt
void __cdecl DATManager_RebuildRoutes() {
    DM_AreaTransitions_CollectRouteTriggers(dm_areaTransitions);
    DM_AreaTransitions_CreateRouteObjects(dm_areaTransitions);
}
DATMGR_REVERSED("DATManager::RebuildRoutes", 0x006D2350, &DATManager_RebuildRoutes);

// 0x6D3BB0 (thiscall): unloads everything: the prop anims, then every area's objects
void __fastcall DATManager_Clear(uint8_t* self, re::Edx) {
    AnyPool* pool = dm_propAnimPool;
    int32_t slot = pool->size;
    while (slot != 0) {
        slot--;
        if ((pool->flags[slot] & 0x80) != 0) {
            continue;
        }
        uint8_t* prop = pool->objects + pool->elemSize * slot;
        if (prop == nullptr) {
            continue;
        }
        if (At<void*>(prop, 0x18) != nullptr) {
            DM_Entity_DeleteRwObject(prop);
        }
        vc::Call<void>(prop, 0xC);
        DM_World_Remove(prop);
        DM_Fn45E690(prop);
        vc::Call<void>(prop, 0, 1);
    }
    DatCount(self) = 0;
    DM_Nothing(dm_triggers);
    for (int area = 0; area < 7; area++) {
        DM_PathStore_RemoveArea(dm_pathStore, area, 1);
        DM_PointLists_RemoveArea(dm_pointLists, area, 1);
        DM_Triggers_RemoveArea(dm_triggers, area, 1);
        DM_Pois_RemoveArea(dm_poiManager, area, 1);
    }
}
DATMGR_REVERSED("DATManager::Clear", 0x006D3BB0, &DATManager_Clear);

// 0x6D3C70 (thiscall): loads an area's DAT (once) out of Dat\Trigger.IMG: its paths,
// point lists, triggers and POIs. (The "PAn_Main.DAT" comparison's result is unused.)
void __fastcall DATManager_LoadDat(uint8_t* self, re::Edx, const char* name, int area) {
    DM_Stricmp(name, "PAn_Main.DAT");
    At<int32_t>(self, 0x24C) = area;
    int32_t hash = DM_HashName(name);
    for (int i = 0; i < DatCount(self); i++) {
        if (hash == At<int32_t>(self, 0x54 + i * 4)) {
            return;
        }
    }
    At<int32_t>(self, 0x54 + DatCount(self) * 4) = hash;
    At<int8_t>(self, 0x1E4 + DatCount(self)) = int8_t(area);
    DatCount(self) = int8_t(DatCount(self) + 1);
    DM_PushMemoryTag(0x21);
    char path[0x44];
    char* buffer = path;
    uint32_t bufferSize = 0x41;
    path[0] = 0;
    DM_Strncpy(buffer, "DAT\\", 0x41);
    uint32_t length = 0;
    while (buffer[length] != 0) {
        length++;
    }
    DM_StrNCat(buffer, name, bufferSize - length);
    DM_LoadingScreen_Message("DATManager::LoadDat", name, 0);
    void* file = DM_OpenFile("Dat\\Trigger.IMG", reinterpret_cast<const char*>(0x008FF548), 1);
    uint32_t offset;
    uint32_t size = 0;
    if (DM_Directory_FindItemByHash(dm_directory, hash, &offset, &size) != 0) {
        void* archive = DM_Archive_Open(file, 0, offset, size);
        int32_t paths = ReadSection(archive, dm_sectionPaths);
        int32_t pointLists = ReadSection(archive, dm_sectionPointLists);
        int32_t triggersB = ReadSection(archive, dm_sectionTriggersB);
        int32_t triggersA = ReadSection(archive, dm_sectionTriggersA);
        int32_t pois = ReadSection(archive, dm_sectionPois);
        DM_Triggers_Load(dm_triggers, archive, triggersB, triggersA);
        DM_PathStore_Load(dm_pathStore, archive, paths);
        DM_PointLists_Load(dm_pointLists, archive, pointLists, 0);
        DM_Pois_Load(dm_poiManager, archive, pois, 0);
        DM_Archive_Close(archive);
    }
    DM_CloseFile(file);
    DM_PopMemoryTag();
}
DATMGR_REVERSED("DATManager::LoadDat", 0x006D3C70, &DATManager_LoadDat);

// 0x6D3EA0 (thiscall): the directory of Trigger.IMG, the link arrays, the pools, and
// the default path, trigger (with its perimeter) and point list
void __fastcall DATManager_Init(uint8_t* self, re::Edx) {
    void* directory = DM_Malloc(0x10);
    if (directory != nullptr) {
        DM_Directory_Construct(directory, 0x19F);
        At<uint32_t>(directory, 0) = VT_CDirectory;
    }
    dm_directory = directory;
    At<uint32_t>(directory, 0xC) = 0;
    void* file = DM_OpenFile("Dat\\Trigger.DIR", reinterpret_cast<const char*>(0x009010BC), 1);
    DM_Directory_ReadDirFile(dm_directory, "Dat\\Trigger.DIR");
    DM_CloseFile(file);
    MakeLinks(dm_poiLinks, 0x1EA);
    MakeLinks(dm_pathNodeLinks, 800);
    MakeLinks(dm_pointLinks, 0x60E);
    MakeLinks(dm_perimeterPointLinks, 0x92E);
    MakeLinks(dm_railNodeLinks, 200);
    re::Ptr32<AnyPool>* pools = reinterpret_cast<re::Ptr32<AnyPool>*>(self);
    pools[0xF] = NewPool(0x1EA, 0x006D2A80);   // CStaticPOI
    pools[0x10] = NewPool(0x60E, 0x006D2B40);  // the point records
    pools[0x11] = NewPool(800, 0x006D2BF0);    // PathNode
    pools[0x12] = NewPool(0x92E, 0x006D2D20);  // the perimeter points
    pools[0] = NewPool(800, 0x006D29D0);       // Trigger
    pools[1] = NewPool(0x15E, 0x006D2DA0);     // Perimeter
    pools[2] = NewPool(200, 0x006D2E50);       // Path
    pools[3] = NewPool(800, 0x006D2F00);       // PointList
    pools[4] = NewPool(0xFA, 0x006D2FC0);      // POIList
    pools[5] = NewPool(100, 0x006D3080);       // PatrolEntry
    pools[6] = NewPool(0x50, 0x006D31C0);      // CPOIGroup
    pools[7] = NewPool(0x14, 0x006D3280);      // CDynamicPOI
    pools[8] = NewPool(0xF, 0x006D3340);       // PedSchedule
    pools[9] = NewPool(0xF, 0x006D3490);       // CSpawner
    pools[0xA] = NewPool(0x96, 0x006D3550);    // (0xC bytes)
    pools[0xB] = NewPool(0x96, 0x006D35D0);    // CSpawnLocation
    pools[0xC] = NewPool(0x14, 0x006D3680);    // CAmbientSpawner
    pools[0xD] = NewPool(0x14, 0x006D3740);    // CMissionSpawner
    pools[0xE] = NewPool(0x14, 0x006D3800);    // CMissionSpawnLocation
    pools[0x13] = NewPool(200, 0x006D38C0);    // Point (the rail nodes)
    pools[0x14] = NewPool(0x19, 0x006D3970);   // RailPointList
    void* path = DM_Path_New(0x14);
    if (path != nullptr) {
        DM_Path_Construct(path, -1);
    }
    void* memory = DM_Trigger_New(0x40);
    uint8_t* trigger = memory != nullptr ? static_cast<uint8_t*>(DM_Trigger_Construct(memory, -1)) : nullptr;
    memory = DM_Perimeter_New(0x30);
    void* perimeter = memory != nullptr ? DM_Perimeter_Construct(memory, -1) : nullptr;
    memory = DM_PointList_New(0x1C);
    if (memory != nullptr) {
        DM_PointList_Construct(memory, -1);
    }
    At<void*>(trigger, 0x20) = perimeter;
    At<float>(trigger, 4) = 0.0f;
    At<float>(trigger, 8) = 0.0f;
    At<float>(trigger, 0xC) = 0.0f;
}
DATMGR_REVERSED("DATManager::Init", 0x006D3EA0, &DATManager_Init);

// 0x6D45A0 (thiscall): frees the directory, the link arrays and the pools (not the
// rail list pool at +0x50)
void __fastcall DATManager_Shutdown(uint8_t* self, re::Edx) {
    if (dm_directory != nullptr) {
        vc::Call<void>(dm_directory, 0, 1);
    }
    dm_directory = nullptr;
    LinkArray* links[5] = {&dm_poiLinks, &dm_pathNodeLinks, &dm_pointLinks, &dm_perimeterPointLinks, &dm_railNodeLinks};
    for (LinkArray* array : links) {
        if (array->links != nullptr) {
            DM_Free(array->links);
        }
        array->links = nullptr;
    }
    re::Ptr32<AnyPool>* pools = reinterpret_cast<re::Ptr32<AnyPool>*>(self);
    DeletePool(pools[0xF], DM_PoolFlushVirtual);
    DeletePool(pools[0x10], DM_PoolFlushPlain);
    DeletePool(pools[0x11], DM_PathNodePool_Flush);
    DeletePool(pools[0x12], DM_PoolFlushPlain);
    DeletePool(pools[0], DM_PoolFlushVirtual);
    DeletePool(pools[1], DM_PoolFlushVirtual);
    DeletePool(pools[2], DM_PoolFlushVirtual);
    DeletePool(pools[3], DM_PoolFlushVirtual);
    DeletePool(pools[4], DM_PoolFlushVirtual);
    DeletePool(pools[5], DM_PatrolEntryPool_Flush);
    DeletePool(pools[6], DM_PoolFlushVirtual);
    DeletePool(pools[7], DM_PoolFlushVirtual);
    DeletePool(pools[8], DM_PedSchedulePool_Flush);
    DeletePool(pools[9], DM_PoolFlushVirtual);
    DeletePool(pools[0xA], DM_PoolFlushPlain);
    DeletePool(pools[0xB], DM_PoolFlushVirtual);
    DeletePool(pools[0xC], DM_PoolFlushVirtual);
    DeletePool(pools[0xD], DM_PoolFlushVirtual);
    DeletePool(pools[0xE], DM_PoolFlushVirtual);
    DeletePool(pools[0x13], DM_PoolFlushVirtual);
}
DATMGR_REVERSED("DATManager::Shutdown", 0x006D45A0, &DATManager_Shutdown);
