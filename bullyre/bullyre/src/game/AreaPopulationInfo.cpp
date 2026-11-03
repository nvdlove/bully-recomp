// The population mixes (CPopulationInfo, 0x24 bytes) and the four of an area or a
// population trigger (CAreaPopulationInfo: its vtable 0x911DD8 and the four mixes, 0x94
// bytes - the game's pool and vector code step by 0x94; PedPopulation.h's struct of
// that name goes on into the area block's trigger list), 0x4A0C70 .. 0x4A1100: their
// constructors and destructors, their archive reading and writing (by field name:
// "TOTAL", the ped types' names, "CAR", "BIKE"; the four mixes "POPULATION_DAY",
// "_NIGHT", "_CLASS", "_CURFEW"), and the triggers' pool of them (*0xC2C128, 70).
// A mix's archivable part is at +0x18 (vtable 0x911E8C: vf0 the deleting destructor's
// thunk, vf1 read, vf2 write; its base's vtable 0x900CDC).

#include <cstddef>
#include <cstdint>

#include "game/PedPopulation.h"
#include "game/Pool.h"
#include "reversed.h"

namespace {

using pop::At;

template <typename R, typename... A>
R VCall(void* obj, int offset, A... args) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(obj))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(fn)(obj, args...);
}

constexpr int kAreaInfoSize = 0x94;   // (the game's CAreaPopulationInfo proper)
static_assert(offsetof(CAreaPopulationInfo, infos) + sizeof(CPopulationInfo) * 4 == kAreaInfoSize);

constexpr uintptr_t vtblPopulationInfo = 0x00911E9C;
constexpr uintptr_t vtblPopulationInfoArchivable = 0x00911E8C;
constexpr uintptr_t vtblArchivable = 0x00900CDC;   // the archivable base
constexpr uintptr_t kCar = 0x00911EA4;             // "CAR"
constexpr uintptr_t kBike = 0x0090A540;            // "BIKE"

GAME_VAR(CPool<uint8_t>*, areaInfoPool, 0x00C2C128);   // the triggers' CAreaPopulationInfos
GAME_VAR(const char*, kTotal, 0x00A350FC);             // "TOTAL"
GAME_VAR(const char*, kPopulationDay, 0x00A350EC);     // the four mixes' names
GAME_VAR(const char*, kPopulationClass, 0x00A350F0);
GAME_VAR(const char*, kPopulationNight, 0x00A350F4);
GAME_VAR(const char*, kPopulationCurfew, 0x00A350F8);

// the fixed array base of a mix (vtable 0x90BCC4; its constructor ignores its argument)
GAME_FN(void, __thiscall, FixedArray_Construct, 0x004645C0, void* self, int capacity);
GAME_FN(void, __thiscall, FixedArray_Destruct, 0x004645E0, void* self);
GAME_FN(void, __thiscall, AreaInfo_Destruct, 0x0049C1C0, void* self);
GAME_FN(void*, __thiscall, PopInfo_DeletingDestructor, 0x004A0E20, void* self, int flags);
GAME_FN(void, __thiscall, AreaInfoPool_Init, 0x004A0FB0, void* pool);
GAME_FN(void*, __thiscall, Pool94_New, 0x00520A50, void* pool);   // (shared with CPool<CPAnimModelInfo>)
// an archive (the object reading or writing a save/config) by an archivable and its name
GAME_FN(void, __thiscall, Archive_ReadObject, 0x005B0720, void* archive, void* archivable, const char* name);
GAME_FN(void, __thiscall, Archive_WriteObject, 0x005B0730, void* archive, void* archivable, const char* name);
GAME_FN(const char*, __cdecl, PedTypeName, 0x00488AF0, int pedType);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// the compiler's `eh vector constructor iterator`
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* constructor);

void SetVtable(void* object, int offset, uintptr_t vtable) {
    At<uint32_t>(object, offset) = vtable;
}

// a mix's archivable part (null for a null mix, as the game's cast)
void* Archivable(CPopulationInfo* info) {
    return info ? reinterpret_cast<uint8_t*>(info) + 0x18 : nullptr;
}

// the archivable part's own fields
uint32_t& Cars(CPopulationInfo* info) {
    return At<uint32_t>(info, 0x1C);
}
uint32_t& Bikes(CPopulationInfo* info) {
    return At<uint32_t>(info, 0x20);
}

// (each archive call: a field by its name; vf 0x38 reads into a dword, vf 0x60 writes one)
void ArchiveRead(void* archive, void* dst, const void* name) {
    VCall<void>(archive, 0x38, dst, name);
}
void ArchiveWrite(void* archive, uint32_t value, const void* name) {
    VCall<void>(archive, 0x60, value, name);
}

} // namespace

namespace area_population_info {

// 0x4A0C70
CPopulationInfo* __fastcall PopulationInfo_Construct(CPopulationInfo* self, re::Edx) {
    FixedArray_Construct(self, 12);
    SetVtable(self, 0x18, vtblArchivable);
    Cars(self) = 0;
    Bikes(self) = 0;
    SetVtable(self, 0, vtblPopulationInfo);
    SetVtable(self, 0x18, vtblPopulationInfoArchivable);
    return self;
}

// 0x4A0CA0
void __fastcall PopulationInfo_Destruct(CPopulationInfo* self, re::Edx) {
    SetVtable(self, 0, vtblPopulationInfo);
    SetVtable(self, 0x18, vtblPopulationInfoArchivable);
    SetVtable(self, 0x18, vtblArchivable);
    FixedArray_Destruct(self);
}

// 0x4A0CC0 (vf1): the capacity of counts[]
int __fastcall PopulationInfo_GetCapacity(CPopulationInfo*, re::Edx) {
    return 12;
}

// 0x4A0E20 (vf0)
CPopulationInfo* __fastcall PopulationInfo_DeletingDestructor(CPopulationInfo* self, re::Edx, int flags) {
    SetVtable(self, 0, vtblPopulationInfo);
    SetVtable(self, 0x18, vtblPopulationInfoArchivable);
    SetVtable(self, 0x18, vtblArchivable);
    FixedArray_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x4A0E10 (the archivable part's vf0: the thunk to the mix's)
void* __fastcall PopulationInfoArchivable_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    return PopInfo_DeletingDestructor(self - 0x18, flags);
}

// 0x4A0E60 (the archivable part's vf1): read. The counts go on after those already
// there (no bound).
void __fastcall PopulationInfoArchivable_Read(uint8_t* self, re::Edx, void* archive) {
    CPopulationInfo* info = reinterpret_cast<CPopulationInfo*>(self - 0x18);
    ArchiveRead(archive, &info->total, kTotal);
    int capacity = VCall<int>(info, 4);
    for (int i = 0; i < capacity; i++) {
        // (the game reads into its argument's stack slot and keeps the low byte)
        uint32_t value = uint32_t(reinterpret_cast<uintptr_t>(archive));
        ArchiveRead(archive, &value, PedTypeName(i));
        info->counts[info->numCounts] = uint8_t(value);
        info->numCounts++;
    }
    ArchiveRead(archive, &Cars(info), reinterpret_cast<const void*>(kCar));
    ArchiveRead(archive, &Bikes(info), reinterpret_cast<const void*>(kBike));
}

// 0x4A0F00 (the archivable part's vf2): write (0 for the counts not set)
void __fastcall PopulationInfoArchivable_Write(uint8_t* self, re::Edx, void* archive) {
    CPopulationInfo* info = reinterpret_cast<CPopulationInfo*>(self - 0x18);
    ArchiveWrite(archive, uint32_t(info->total), kTotal);
    int capacity = VCall<int>(info, 4);
    for (int i = 0; i < capacity; i++) {
        uint32_t value = uint32_t(i) < info->numCounts ? info->counts[i] : 0;
        ArchiveWrite(archive, value, PedTypeName(i));
    }
    ArchiveWrite(archive, Cars(info), reinterpret_cast<const void*>(kCar));
    ArchiveWrite(archive, Bikes(info), reinterpret_cast<const void*>(kBike));
}

// 0x4A0CD0 (vf1) / 0x4A0D70 (vf2): the four mixes read / written
void __fastcall AreaPopulationInfo_Read(CAreaPopulationInfo* self, re::Edx, void* archive) {
    Archive_ReadObject(archive, Archivable(&self->infos[0]), kPopulationDay);
    Archive_ReadObject(archive, Archivable(&self->infos[1]), kPopulationNight);
    Archive_ReadObject(archive, Archivable(&self->infos[2]), kPopulationClass);
    Archive_ReadObject(archive, Archivable(&self->infos[3]), kPopulationCurfew);
}

void __fastcall AreaPopulationInfo_Write(CAreaPopulationInfo* self, re::Edx, void* archive) {
    Archive_WriteObject(archive, Archivable(&self->infos[0]), kPopulationDay);
    Archive_WriteObject(archive, Archivable(&self->infos[1]), kPopulationNight);
    Archive_WriteObject(archive, Archivable(&self->infos[2]), kPopulationClass);
    Archive_WriteObject(archive, Archivable(&self->infos[3]), kPopulationCurfew);
}

// 0x4A1070 (vf0): the deleting destructor, arrays (flags & 2: the count before them)
// included; single ones go back to the triggers' pool
void* __fastcall AreaPopulationInfo_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    if (flags & 2) {
        int32_t* block = reinterpret_cast<int32_t*>(self) - 1;
        for (int i = *block - 1; i >= 0; i--) {
            AreaInfo_Destruct(self + i * kAreaInfoSize);
        }
        if (flags & 1) {
            Free(block);
        }
        return block;
    }
    AreaInfo_Destruct(self);
    if (flags & 1) {
        CPool<uint8_t>* pool = areaInfoPool;
        int slot = int(self - pool->objects) / pool->elemSize;
        pool->flags[slot] |= 0x80;
        if (slot < pool->lastUsed) {
            pool->lastUsed = slot;
        }
    }
    return self;
}

// 0x4A0FA0: CAreaPopulationInfo's operator new (from the triggers' pool)
void* __cdecl AreaPopulationInfo_New(size_t) {
    return Pool94_New(areaInfoPool);
}

// 0x4A0FB0: CPool<CAreaPopulationInfo>'s arrays for `size` slots, all free (constructed
// ones when the pool says so)
void __fastcall AreaInfoPool_InitArrays(CPool<uint8_t>* self, re::Edx) {
    self->ownsArrays = 1;
    self->elemSize = kAreaInfoSize;
    if (self->unknown19) {
        int count = self->size;
        // (new[]'s size: -1 on overflow)
        uint64_t bytes = uint64_t(uint32_t(count)) * kAreaInfoSize;
        uint32_t size = bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
        size = size > 0xFFFFFFFBu ? 0xFFFFFFFFu : size + 4;
        int32_t* block = static_cast<int32_t*>(Malloc(size));
        if (block != nullptr) {
            *block = count;
            VectorConstruct(block + 1, kAreaInfoSize, count, reinterpret_cast<void*>(0x0049C190));
            self->objects = reinterpret_cast<uint8_t*>(block + 1);
        } else {
            self->objects = nullptr;
        }
    } else {
        self->objects = static_cast<uint8_t*>(Malloc(self->size * kAreaInfoSize));
    }
    self->flags = static_cast<uint8_t*>(Malloc(self->size));
    self->lastUsed = 0;
    for (int slot = 0; slot < self->size; slot++) {
        self->flags[slot] |= 0x80;
        self->flags[slot] &= 0x80;
    }
}

// 0x4A1100: the triggers' pool of them made (70 slots)
void __cdecl AreaInfoPool_Create() {
    CPool<uint8_t>* pool = static_cast<CPool<uint8_t>*>(Malloc(sizeof(CPool<uint8_t>)));
    if (pool == nullptr) {
        areaInfoPool = nullptr;
        return;
    }
    pool->unknown10 = 0;
    pool->objects = nullptr;
    pool->flags = nullptr;
    pool->lastUsed = 0;
    pool->ownsArrays = 1;
    pool->size = 70;
    pool->unknown19 = 0;
    AreaInfoPool_Init(pool);
    areaInfoPool = pool;
}

} // namespace area_population_info

#define C08P6_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P6_REVERSED("CPopulationInfo::CPopulationInfo", 0x004A0C70, &area_population_info::PopulationInfo_Construct);
C08P6_REVERSED("CPopulationInfo::~CPopulationInfo", 0x004A0CA0, &area_population_info::PopulationInfo_Destruct);
C08P6_REVERSED("CPopulationInfo::GetCapacity", 0x004A0CC0, &area_population_info::PopulationInfo_GetCapacity);
C08P6_REVERSED("CAreaPopulationInfo::Read", 0x004A0CD0, &area_population_info::AreaPopulationInfo_Read);
C08P6_REVERSED("CAreaPopulationInfo::Write", 0x004A0D70, &area_population_info::AreaPopulationInfo_Write);
C08P6_REVERSED("CPopulationInfo::Archivable::DeletingDestructor", 0x004A0E10,
               &area_population_info::PopulationInfoArchivable_DeletingDestructor);
C08P6_REVERSED("CPopulationInfo::DeletingDestructor", 0x004A0E20, &area_population_info::PopulationInfo_DeletingDestructor);
C08P6_REVERSED("CPopulationInfo::Archivable::Read", 0x004A0E60, &area_population_info::PopulationInfoArchivable_Read);
C08P6_REVERSED("CPopulationInfo::Archivable::Write", 0x004A0F00, &area_population_info::PopulationInfoArchivable_Write);
C08P6_REVERSED("CAreaPopulationInfo::operator new", 0x004A0FA0, &area_population_info::AreaPopulationInfo_New);
C08P6_REVERSED("CPool<CAreaPopulationInfo>::InitArrays", 0x004A0FB0, &area_population_info::AreaInfoPool_InitArrays);
C08P6_REVERSED("CAreaPopulationInfo::DeletingDestructor", 0x004A1070,
               &area_population_info::AreaPopulationInfo_DeletingDestructor);
C08P6_REVERSED("CAreaPopulationInfo::CreatePool", 0x004A1100, &area_population_info::AreaInfoPool_Create);
