// CStreaming, the rest: the id ranges, the archive directories, the animation group
// files, making room (removing what's unused), the model flags, flushing. (The core,
// requests through loading and removal, is in Streaming.cpp.)

#include <cstdio>
#include <cstring>

#include "game/ModelInfo.h"
#include "game/Streaming.h"
#include "game/TxdStore.h"
#include "reversed.h"

// ours, called where they are (so bullyre.ini decides which runs)
namespace game {
namespace {
GAME_FN(int, __cdecl, BinarySearch, 0x0052DC10, const void* key, const void* base, uint32_t count, uint32_t size,
        int(__cdecl* compare)(const void*, const void*));
GAME_FN(int, __cdecl, CompareIdRange, 0x0052DC70, const void* a, const void* b);
GAME_FN(void, __cdecl, AddIdRange, 0x0052DDA0, int firstId, int count);
GAME_FN(uint8_t, __cdecl, HasTextureMemoryFor, 0x0052D8A0, int bytes);
GAME_FN(int, __cdecl, FindAgrId, 0x0052DAD0, const char* name);
GAME_FN(AgrGroup*, __cdecl, GetAgrGroupOf, 0x0052DB50, uint32_t id, uint32_t* index);
GAME_FN(void, __cdecl, LoadCdDirectoryIPB, 0x0052E020, const char* path, int image);
GAME_FN(uint8_t, __cdecl, HasMemoryFor, 0x0052E650, int bytes, int id);
GAME_FN(void, __cdecl, SetLoadedMissionRequired, 0x0052E720, int id);
GAME_FN(void, __cdecl, SetLoadedGameRequired, 0x0052E780, int id);
GAME_FN(void, __cdecl, SetModelIsDeletable, 0x0052FE40, int id);
GAME_FN(void, __cdecl, SetMissionDoesntRequireModel, 0x0052FF40, int id);
GAME_FN(uint8_t, __cdecl, RemoveLoadedPed, 0x00530060);
GAME_FN(void, __cdecl, RemoveAgrModel, 0x00530880, int id);
GAME_FN(void, __cdecl, DeleteAllRwObjects, 0x005308D0);
GAME_FN(uint8_t, __cdecl, RemoveLeastUsedModel, 0x00530C70, uint32_t keepFlags);
GAME_FN(void, __cdecl, DeleteRwObjectsBehindCamera, 0x00530DB0, int bytes, int id, int force);
GAME_FN(uint8_t, __cdecl, LoadModelData, 0x005317F0, MemoryBuffer* buffer, int id, CStreamingInfo* info, CBaseModelInfo* mi);
GAME_FN(void, __cdecl, FlushRequestList, 0x005331B0);
GAME_FN(void, __cdecl, RemoveAllUnusedModels, 0x00530A70, int showProgress);
GAME_FN(void, __cdecl, DeleteAllSectorRwObjects, 0x0052F080);
GAME_FN(void, __cdecl, DeleteSectorRwObjects, 0x0052F0E0);
GAME_FN(void, __cdecl, DeleteRwObjectsInList, 0x0052E4F0, re::Ptr32<uint32_t>* list);
GAME_FN(void, __cdecl, RequestBigBuildingsNear, 0x005322C0, int area, const float* pos);
GAME_FN(void, __cdecl, InstanceBigBuildingsNear, 0x0052F590, int area, const float* pos);
GAME_FN(void, __cdecl, RequestPlayerModels, 0x00533460);
GAME_FN(void, __cdecl, Update, 0x005346B0);
} // namespace
} // namespace game

namespace {

GAME_FN(int, __cdecl, CdStreamGetNumImages, 0x0073A9A0);
GAME_FN(const char*, __cdecl, CdStreamGetImageName, 0x0073A910, int image);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);

// the memory manager
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, FreeAligned, 0x005EEA70, void* p);
GAME_FN(void, __thiscall, Heap_Free, 0x005EEEF0, void* heap, void* p);
inline void* const heap = reinterpret_cast<void*>(0x00D14240);
GAME_FN(int, __cdecl, TextureHeapFree, 0x005F0A40);
// texture memory accounting (see HasMemoryFor)
GAME_VAR(int32_t, txdMemoryMax, 0x00A95800);
GAME_VAR(int32_t, txdMemoryBase, 0x00A95804);
GAME_VAR(float, txdMemoryScale, 0x00A95808);
GAME_VAR(int32_t, txdMemorySmall, 0x00A9580C);

// files (CFileMgr) and the CDirectory of extra entries (thiscall)
GAME_FN(void, __thiscall, LoadedFile_Load, 0x00426610, LoadedFile* file, const char* path);
GAME_FN(int, __cdecl, LoadedFile_Read, 0x0042D5B0, LoadedFile* file, void* buffer, uint32_t size);
GAME_FN(int, __cdecl, OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, ReadFile, 0x0042D2A0, int file, void* buffer, uint32_t size);
GAME_FN(int, __cdecl, SeekFile, 0x0042D300, int file, uint32_t offset, int origin);
GAME_FN(void, __cdecl, CloseFile, 0x0042D450, int file);
GAME_FN(void, __thiscall, Directory_AddItem, 0x00429040, void* dir, const CdDirEntry* entry);
GAME_FN(void, __thiscall, Directory_GetItem, 0x00429130, void* dir, int index, uint32_t* posn, uint32_t* size);
GAME_FN(void, __thiscall, Directory_SetItem, 0x00429160, void* dir, int index, uint32_t posn, uint32_t size);
inline uint8_t* const ipbBuffer = reinterpret_cast<uint8_t*>(0x00BD7800);   // (placement files are read here)

// the stores, by kind of entry: looked up by name, or added
GAME_FN(int, __cdecl, GetModelIndexHashed, 0x0051C390, const char* name);
GAME_FN(void*, __cdecl, GetModelInfoByName, 0x0051C190, const char* name, int* index);
GAME_FN(int, __cdecl, AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(int, __cdecl, FindColSlot, 0x0056EF10, const char* name);
GAME_FN(int, __cdecl, AddColSlot, 0x00570610, const char* name);
GAME_FN(int, __cdecl, FindIplSlot, 0x00437D00, const char* name);
GAME_FN(int, __cdecl, AddIplSlot, 0x00438810, const char* name);
GAME_FN(int, __cdecl, FindIplSlotByHash, 0x00437D10, uint32_t hash);
GAME_FN(int, __thiscall, LipStore_Find, 0x005A0A70, void* store, const char* name);
inline void* const lipStore = reinterpret_cast<void*>(0x00CF4378);
GAME_FN(int, __thiscall, CatPool_Find, 0x00450810, void* pool, const char* name);
GAME_FN(const char*, __thiscall, CatPool_GetName, 0x00450790, void* pool, int cat);
GAME_FN(int, __thiscall, LurStore_Find, 0x005DAFF0, void* store, const char* name);
GAME_FN(int, __thiscall, LurStore_Add, 0x005DAF70, void* store, const char* name);
GAME_FN(const char*, __thiscall, LurStore_GetName, 0x005DB080, void* store, int lur);
GAME_FN(void, __cdecl, ColStore_Update, 0x0056F570);   // (after a directory is read)
GAME_FN(uint32_t, __cdecl, TxdGetNameHash, 0x005F1060, int txd);
GAME_FN(const char*, __cdecl, ColGetName, 0x0056EEE0, int col);
GAME_FN(const char*, __cdecl, IplGetName, 0x00437CB0, int ipl);

// entities: the world sectors' lists, and the pools
GAME_FN(void*, __cdecl, GetPoolEntity, 0x0044A3E0, int pool, int slot);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, void* entity);
GAME_FN(uint8_t, __cdecl, Entity_IsInArea, 0x00451E70, void* entity, const float* pos);
GAME_FN(void, __cdecl, KeepLoadingScreenAlive, 0x0043C060, int, int);

GAME_VAR(int32_t, currArea, 0x00BD1008);   // CGame::currArea

// LoadModel's and the model requests'
GAME_FN(AgrEntry*, __thiscall, AgrGroup_GetEntry, 0x00450B80, AgrGroup* group, uint32_t index);
GAME_FN(void, __thiscall, AnimManager_AddRef, 0x006BD010, void* animManager, AgrGroup* group, uint32_t index);
GAME_FN(void, __thiscall, AnimManager_RemoveRef, 0x006BD030, void* animManager, AgrGroup* group, uint32_t index, int);
GAME_FN(uint8_t, __cdecl, LoadAtomicModel, 0x0052FCD0, game::MemoryBuffer* buffer, int id, CStreamingInfo* info, CBaseModelInfo* mi);
GAME_FN(uint8_t, __cdecl, LoadClumpModel, 0x0042B9D0, game::MemoryBuffer* buffer, int id);
GAME_FN(uint8_t, __thiscall, ModelInfo_MatchesName, 0x0050E940, CBaseModelInfo* mi, const char* name);
GAME_FN(void, __thiscall, ModelInfo_SetTexDictionary, 0x0050E900, CBaseModelInfo* mi, const char* name, int);
GAME_FN(void, __thiscall, ModelInfo_522220, 0x00522220, CBaseModelInfo* mi, const char* name, int);
GAME_FN(void, __thiscall, ModelInfo_522420, 0x00522420, CBaseModelInfo* mi);   // (the seasonal models, at Init)
GAME_FN(int, __cdecl, FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(uint8_t, __thiscall, Directory_FindItemByHash, 0x004290A0, void* dir, uint32_t hash, uint32_t* posn, uint32_t* size);
GAME_FN(uint8_t, __thiscall, Directory_FindItem3, 0x004290E0, void* dir, const char* name, uint32_t* posn, uint32_t* size);
GAME_FN(void, __thiscall, Directory_Construct, 0x004291B0, void* dir, int capacity);
GAME_FN(void, __thiscall, Ped_Remove, 0x00476600, void* ped);
GAME_FN(uint8_t, __thiscall, Object_CanBeDeleted, 0x004CF2B0, void* object);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, Object_Detach, 0x0045E690, void* entity);
inline re::Ptr32<CPool<uint8_t>>& pedPool = *reinterpret_cast<re::Ptr32<CPool<uint8_t>>*>(0x00C0F5F0);
inline re::Ptr32<CPool<uint8_t>>& propAnimPool = *reinterpret_cast<re::Ptr32<CPool<uint8_t>>*>(0x00C0F608);

// Init's
GAME_FN(void, __thiscall, ActTreeRefPool_Init, 0x00450690, void* pool, int size, const char* name);
GAME_FN(void, __cdecl, LoadCdDirectories, 0x0052ECA0);   // (patched by the launcher: stays the game's)
GAME_FN(void*, __cdecl, MallocAligned, 0x005EE9C0, size_t size, size_t alignment);
GAME_FN(void, __cdecl, SetSeasonalFlags_, 0x0052E8D0);

// the seasons'
GAME_VAR(uint8_t, ms_bAllSeasonal, 0x00BF380D);      // every dictionary counts as seasonal
GAME_VAR(uint8_t, ms_bSettingSeasons, 0x00A147E8);
GAME_FN(void, __thiscall, SetChapter, 0x004545C0, void* chapter, int);   // (on CTxdStore::ms_currentChapter)
GAME_FN(int, __cdecl, FindTxdSlotByHash, 0x005F2450, uint32_t hash, int season);
GAME_FN(uint8_t, __cdecl, TxdGetSeason, 0x005F1090, int txd);
GAME_FN(void, __cdecl, Fn_444BB0, 0x00444BB0);
GAME_FN(void, __thiscall, LurStore_Init, 0x005DB050, void* store);
GAME_VAR(int32_t, seasonalModelsFirst, 0x00A136C8);
GAME_VAR(int32_t, seasonalModelsLast, 0x00A136CC);
inline re::Ptr32<void>& radar = *reinterpret_cast<re::Ptr32<void>*>(0x00C674CC);
GAME_FN(void, __thiscall, Radar_ReInit, 0x005492E0, void* radar);

// LoadScene's and Update's
struct Vec3 {
    float x, y, z;
};
GAME_FN(void, __cdecl, LoadAllRequestedModels_, 0x00533EE0, int priorityOnly);
GAME_FN(void, __cdecl, LoadRequestedModels_, 0x005330A0);
GAME_FN(void, __cdecl, RetryLoadFile_, 0x00533A50, int channel);
GAME_FN(void, __cdecl, AddModelsToRequestList, 0x00534190, const float* pos, uint32_t flags);
GAME_FN(void, __cdecl, ProcessSectorsAround, 0x0052EDC0, const void* camera);
GAME_FN(void, __cdecl, Fn_52F210, 0x0052F210, const float* pos);
GAME_FN(void, __cdecl, Fn_52F3D0, 0x0052F3D0, const float* pos);
GAME_FN(void, __thiscall, Fn_5E6480, 0x005E6480, void* object);
GAME_FN(void, __thiscall, Fn_5E6510, 0x005E6510, void* object, void* entity);
inline void* const object_D12AF8 = reinterpret_cast<void*>(0x00D12AF8);
GAME_FN(void, __cdecl, Fn_438830, 0x00438830);
GAME_FN(void, __cdecl, Fn_438850, 0x00438850);
GAME_FN(void, __thiscall, Fn_416DC0, 0x00416DC0, void* object);
GAME_FN(void, __thiscall, Fn_415AD0, 0x00415AD0, void* object);
GAME_FN(void, __thiscall, Fn_4A00B0, 0x004A00B0, void* object);
GAME_FN(void, __thiscall, Radar_Fn_54E120, 0x0054E120, void* radar, const float* pos);
GAME_FN(void, __cdecl, ColStore_LoadNear, 0x0056FE00, const float* xy);
GAME_FN(void, __cdecl, ColStore_Fn_570080, 0x00570080, const float* xy);
GAME_FN(void, __cdecl, IplStore_LoadNear, 0x00439CF0, Vec3 pos, int);
GAME_FN(void, __cdecl, IplStore_Fn_439770, 0x00439770, const float* pos, int);
GAME_FN(const float*, __thiscall, GetPlayerPos, 0x0041AAC0, void* object);
inline void* const player_C3CC68 = reinterpret_cast<void*>(0x00C3CC68);
GAME_FN(void*, __cdecl, Fn_6C82F0, 0x006C82F0, uintptr_t);
GAME_FN(void*, __cdecl, Fn_6C9A90, 0x006C9A90, void*);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(const float*, __cdecl, Fn_445D60, 0x00445D60);
GAME_FN(const float*, __cdecl, GetCameraTarget, 0x00446900, float* out);
GAME_FN(void, __cdecl, Fn_429990, 0x00429990);
GAME_FN(float, __cdecl, Fn_738840, 0x00738840, int, int);

// entities' virtuals
inline void Entity_Vf10(void* entity, int a, int b) {
    reinterpret_cast<void(__thiscall*)(void*, int, int)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[10]))(entity, a, b);
}
inline uint8_t Entity_Vf33(void* entity) {
    return reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[33]))(entity);
}
GAME_FN(uint8_t, __thiscall, Entity_IsVisible, 0x004686C0, void* entity);
GAME_VAR(int32_t, specialModelBase, 0x00A136D0);
GAME_VAR(uint8_t, ms_bLoadingFlagC1A9EA, 0x00C1A9EA);   // (RemoveAgr)

inline re::Ptr32<CPool<uint8_t>>& buildingPool = *reinterpret_cast<re::Ptr32<CPool<uint8_t>>*>(0x00C0F5F8);
inline re::Ptr32<CPool<uint8_t>>& treadablePool = *reinterpret_cast<re::Ptr32<CPool<uint8_t>>*>(0x00C0F5FC);
inline re::Ptr32<CPool<uint8_t>>& dummyPool = *reinterpret_cast<re::Ptr32<CPool<uint8_t>>*>(0x00C0F600);
inline re::Ptr32<CPool<uint8_t>>& objectPool = *reinterpret_cast<re::Ptr32<CPool<uint8_t>>*>(0x00C0F614);   // chained on (+0x10: the next pool)
GAME_FN(int, __thiscall, Pool_ChainSize, 0x00435BB0, void* next);   // the chained pools' slots
GAME_FN(void*, __thiscall, Pool_ChainAt, 0x00435BD0, void* next, int slot, int);

// An entity's fields streaming looks at (CEntity is elsewhere)
template <typename T>
re::Field<T>& Field(void* entity, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(entity) + offset);
}
int16_t EntityModel(void* entity) {
    return Field<int16_t>(entity, 0x10E);
}
void* EntityRwObject(void* entity) {
    return Field<void*>(entity, 0x18);
}

CStreamingInfo& Info(int id) {
    return CStreaming::ms_aInfoForModel[CStreaming::GetInfoIndex(id)];
}

void InitInfo(CStreamingInfo& info) {
    info.loadState = LOADSTATE_NOT_LOADED;
    info.next = 0xFFFF;
    info.prev = 0xFFFF;
    info.nextOnCd = -1;
    info.cdSize = 0;
    info.cdPosn = 0;
}

CStreamingInfo* AtIndex(uint16_t index) {
    return CStreaming::ms_pInfoBase + index;
}

int IdOf(CStreamingInfo* info) {
    return int(CStreaming::GetIdFromInfo(info) & 0xFFFF);
}

} // namespace

namespace CStreaming {

// ---- ids -----------------------------------------------------------------------------

int __cdecl BinarySearch(const void* key, const void* base, uint32_t count, uint32_t size, int(__cdecl* compare)(const void*, const void*)) {
    const uint8_t* low = static_cast<const uint8_t*>(base);
    while (count != 0) {
        const uint8_t* mid = low + (count >> 1) * size;
        int order = compare(key, mid);
        if (order == 0) {
            return int(uint32_t(mid - static_cast<const uint8_t*>(base)) / size);
        }
        if (order > 0) {
            low = mid + size;
            count--;
        }
        count >>= 1;
    }
    return -int(uint32_t(low - static_cast<const uint8_t*>(base)) / size) - 1;
}

int __cdecl CompareIdRange(const void* a, const void* b) {
    return int(*static_cast<const uint16_t*>(a)) - int(*static_cast<const uint16_t*>(b));
}

CStreamingInfo* __cdecl GetInfo(int id) {
    return &ms_aInfoForModel[GetInfoIndex(id)];
}

void __cdecl AddIdRange(int firstIdArg, int countArg) {
    uint16_t firstId = uint16_t(firstIdArg);
    uint16_t count = uint16_t(countArg);
    uint16_t key = firstId;
    int pos = game::BinarySearch(&key, ms_aIdRanges, ms_numIdRanges, sizeof(CStreamingIdRange), game::CompareIdRange);
    if (pos < 0) {
        pos = -1 - pos;
    }
    if (pos < int(ms_numIdRanges)) {
        // runs into the next range: what's past it is a range of its own, the rest up to it
        const CStreamingIdRange& next = ms_aIdRanges[pos];
        int end = firstId + count;
        if (end > next.firstId) {
            int nextEnd = next.firstId + next.count;
            if (end > nextEnd) {
                game::AddIdRange(nextEnd, end - nextEnd);
            }
            count = uint16_t(ms_aIdRanges[pos].firstId - firstId);
            if (count == 0) {
                return;
            }
        }
    }
    int prevRange = pos - 1;
    if (prevRange != -1) {
        CStreamingIdRange& prev = ms_aIdRanges[prevRange];
        int gap = int(firstId) - int(prev.firstId) - int(prev.count);
        if (gap < 0) {
            // starts inside the one before: only what's past it
            if (-gap >= int(count)) {
                return;
            }
            firstId = uint16_t(firstId - gap);
            count = uint16_t(count + gap);
        } else if (prevRange == int(ms_lastAddedRange) && gap < 10) {
            // just after the last one added: it grows instead (over the gap too)
            int start = ms_numInfos;
            int end = count + start + gap;
            for (int i = start; i < end; i++) {
                InitInfo(ms_aInfoForModel[i]);
            }
            ms_numInfos = uint16_t(end);
            prev.count = uint16_t(prev.count + gap + count);
            return;
        }
    }
    if (pos < int(ms_numIdRanges)) {
        memmove(&ms_aIdRanges[pos + 1], &ms_aIdRanges[pos], (ms_numIdRanges - pos) * sizeof(CStreamingIdRange));
    }
    int start = ms_numInfos;
    int end = count + start;
    for (int i = start; i < end; i++) {
        InitInfo(ms_aInfoForModel[i]);
    }
    ms_numIdRanges++;
    ms_aIdRanges[pos].firstId = firstId;
    ms_aIdRanges[pos].indexDelta = int16_t(start - firstId);
    ms_aIdRanges[pos].count = count;
    ms_numInfos = uint16_t(end);
    ms_lastAddedRange = uint16_t(pos);
}

// ---- the images ------------------------------------------------------------------------

uint32_t __cdecl GetImageBase(uint32_t posn) {
    uint32_t base = ms_imageOffsets[ms_currentImage];
    // (a position at an image's very start counts as outside it: it searches again)
    if (posn > base && posn <= base + ms_imageSize) {
        return base;
    }
    int nearest = -1;
    int nearestDistance = 0x7FFFFFFF;
    for (int i = 0; i < 4; i++) {
        uint32_t offset = ms_imageOffsets[i];
        if (offset == 0xFFFFFFFF) {
            continue;
        }
        int distance = posn <= offset ? int(offset - posn) : int(posn - offset - ms_imageSize);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearest = i;
        }
    }
    // (none: index -1, which reads ms_imageSize)
    ms_currentImage = nearest;
    return ms_imageOffsets[nearest];
}

// ---- small things ---------------------------------------------------------------------

void __cdecl SetCatNode(int i, int value) {
    ms_aCatNodes[i] = value;
}

int __cdecl GetCatNode(int i) {
    return ms_aCatNodes[i];
}

void __cdecl Shutdown() {
    FreeAligned(ms_pStreamingBuffer[0]);   // (one allocation for both channels)
    ms_streamingBufferSize = 0;
    if (ms_pExtraObjectsDir != nullptr) {
        // its deleting destructor
        auto destroy = reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(ms_pExtraObjectsDir))[0]));
        destroy(ms_pExtraObjectsDir, 1);
    }
    if (ms_pAgrGroups != nullptr) {
        Free(ms_pAgrGroups);
    }
    if (ms_pAgrFirstIds != nullptr) {
        Free(ms_pAgrFirstIds);
    }
    for (int id = AGR_ID_BASE; id < AGR_ID_BASE + 0x244; id++) {
        if (ms_pPreloaded[id] != nullptr) {
            Heap_Free(heap, ms_pPreloaded[id]);
        }
    }
}

uint8_t __cdecl HasTextureMemoryFor(int bytes) {
    return TextureHeapFree() >= bytes;
}

void __cdecl ClearUnknownCA3688() {
    *reinterpret_cast<uint16_t*>(0x00CA368C) = 0;
    *reinterpret_cast<uint16_t*>(0x00CA3688) = 0;
}

int __cdecl FindFreePedSlot() {
    for (int i = 0; i < 20; i++) {
        if (ms_pedsLoaded[i] == -1) {
            return i;
        }
    }
    return -1;
}

// ---- animation groups -----------------------------------------------------------------

// (the CRT's new[] size: all ones when count * 4 overflows)
static size_t ArraySize(uint32_t count) {
    return count > 0x3FFFFFFF ? 0xFFFFFFFF : count * 4;
}

uint8_t __cdecl InitAgrGroups(uint32_t count) {
    if (ms_pAgrGroups != nullptr) {
        return 0;
    }
    ms_numAgrGroups = count;
    ms_pAgrGroups = static_cast<re::Ptr32<AgrGroup>*>(Malloc(ArraySize(count)));
    ms_pAgrFirstIds = static_cast<uint32_t*>(Malloc(ArraySize(ms_numAgrGroups)));
    for (uint32_t i = 0; i < ms_numAgrGroups; i++) {
        ms_pAgrGroups[i] = nullptr;
        ms_pAgrFirstIds[i] = 0;
    }
    return 1;
}

uint8_t __cdecl AddAgrGroup(AgrGroup* group, uint32_t index) {
    if (ms_pAgrGroups == nullptr || index >= ms_numAgrGroups || ms_pAgrGroups[index] != nullptr) {
        return 0;
    }
    ms_pAgrGroups[index] = group;
    // the first ids, in order (each after the one before: that one must be there)
    for (uint32_t i = 0; i <= index; i++) {
        if (ms_pAgrGroups[i] == nullptr) {
            continue;
        }
        ms_pAgrFirstIds[i] = i == 0 ? uint32_t(AGR_ID_BASE) : ms_pAgrGroups[i - 1]->numEntries + ms_pAgrFirstIds[i - 1];
    }
    return 1;
}

int __cdecl FindAgrId(const char* name) {
    for (uint32_t i = 0; i < ms_numAgrGroups; i++) {
        AgrGroup* group = ms_pAgrGroups[i];
        if (group == nullptr) {
            continue;
        }
        for (uint32_t j = 0; j < group->numEntries; j++) {
            if (_stricmp(name, group->entries[j].name) == 0) {
                return int(ms_pAgrFirstIds[i] + j);
            }
        }
    }
    return -1;
}

AgrGroup* __cdecl GetAgrGroup(uint32_t id, uint32_t* index) {
    if (id == 0xFFFFFFFF) {
        return nullptr;
    }
    for (uint32_t i = 0; i < ms_numAgrGroups; i++) {
        AgrGroup* group = ms_pAgrGroups[i];
        uint32_t first = ms_pAgrFirstIds[i];
        if (group != nullptr && id >= first && id < group->numEntries + first) {
            if (index != nullptr) {
                *index = id - first;
            }
            return group;
        }
    }
    return nullptr;
}

int __cdecl GetAgrId(AgrGroup* group, int index) {
    for (uint32_t i = 0; i < ms_numAgrGroups; i++) {
        if (ms_pAgrGroups[i] == group) {
            return int(ms_pAgrFirstIds[i]) + index;
        }
    }
    return 0;
}

// An animation's entry in its group (out of range: nullptr, which the game reads
// through where it doesn't check)
static AgrEntry* AgrEntryAt(AgrGroup* group, uint32_t index) {
    return index < group->numEntries ? &group->entries[index] : nullptr;
}

static uint16_t AgrRefs(const AgrEntry* entry) {
    return *reinterpret_cast<const volatile uint16_t*>(reinterpret_cast<uintptr_t>(entry) + offsetof(AgrEntry, numRefs));
}

static bool ModelNeedsAgr(int modelId, int agr) {
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[modelId];
    return mi->GetAnimFileIndex() == agr || mi->GetDependency8() == agr || mi->GetDependency9() == agr || mi->GetDependency10() == agr;
}

uint8_t __cdecl IsAgrWanted(int id) {
    uint32_t index;
    AgrGroup* group = game::GetAgrGroupOf(uint32_t(id), &index);
    if (group != nullptr && AgrRefs(AgrEntryAt(group, index)) != 0) {
        return 1;
    }
    for (CStreamingInfo* info = AtIndex(ms_pStartRequestedList->next); info != ms_pEndRequestedList; info = AtIndex(info->next)) {
        int model = IdOf(info);
        if (model < TXD_ID_BASE && ModelNeedsAgr(model, id)) {
            return 1;
        }
    }
    for (int i = 0; i < 4; i++) {
        for (int channel = 0; channel < 2; channel++) {
            int model = ms_channel[channel].modelIds[i];
            if (model != -1 && model < TXD_ID_BASE && ModelNeedsAgr(model, id)) {
                return 1;
            }
        }
    }
    return 0;
}

// ---- directories ------------------------------------------------------------------------

void __cdecl SetColLoaded(int col, int loaded) {
    Info(col + COL_ID_BASE).loadState = re::Flag(loaded) ? 1 : 0;
}

void __cdecl LoadCdDirectory(const char* path) {
    // (profiler markers around it, empty in the shipped exe)
    const char* ipbExtension = ms_bUseIpc ? "IPC" : "IPB";
    LoadedFile file{nullptr, 0, 0};
    LoadedFile_Load(&file, path);
    int prevId = -1;   // the entry before in the archive, for nextOnCd
    CdDirEntry entry;
    while (LoadedFile_Read(&file, &entry, sizeof(entry))) {
        char* dot = strchr(entry.name, '.');
        *dot = '\0';
        const char* extension = dot + 1;
        if (entry.size > uint32_t(ms_streamingBufferSize)) {
            ms_streamingBufferSize = int32_t(entry.size);
        }
        int id;
        if (_strnicmp(extension, "NIF", 3) == 0) {
            id = -1;
            if (ms_bModelNamesHashed) {
                id = GetModelIndexHashed(entry.name);
            } else {
                GetModelInfoByName(entry.name, &id);
            }
            if (id == -1) {
                // no model for it (yet): kept aside by name
                Directory_AddItem(ms_pExtraObjectsDir, &entry);
                prevId = -1;
                continue;
            }
        } else if (_strnicmp(extension, "NFT", 3) == 0) {
            id = FindTxdSlot(entry.name, 0);
            if (id == -1) {
                id = AddTxdSlot(entry.name);
            }
            id += TXD_ID_BASE;
        } else if (_strnicmp(extension, "COL", 3) == 0) {
            id = FindColSlot(entry.name);
            if (id == -1) {
                id = AddColSlot(entry.name);
            }
            id += COL_ID_BASE;
        } else if (_strnicmp(extension, "AGR", 3) == 0) {
            id = game::FindAgrId(entry.name);
            if (id == -1) {
                prevId = -1;
                continue;
            }
        } else if (_strnicmp(extension, ipbExtension, 3) == 0) {
            id = FindIplSlot(entry.name);
            if (id == -1) {
                id = AddIplSlot(entry.name);
            }
            id += IPB_ID_BASE;
        } else if (_strnicmp(extension, "LIP", 3) == 0) {
            id = LipStore_Find(lipStore, entry.name) + LIP_ID_BASE;
        } else if (_strnicmp(extension, "CAT", 3) == 0) {
            id = CatPool_Find(game::pool_C0F7B4, entry.name);
            if (id == -1) {
                continue;   // (unknown prop animations don't break the chain)
            }
            id += CAT_ID_BASE;
        } else if (_strnicmp(extension, "LUR", 3) == 0) {
            id = LurStore_Find(game::lurStore, entry.name);
            if (id == -1) {
                id = LurStore_Add(game::lurStore, entry.name);
            }
            id += LUR_ID_BASE;
        } else {
            *dot = '.';
            prevId = -1;
            continue;
        }
        // the first entry for an id wins; entries read in one go are chained
        CStreamingInfo& info = Info(id);
        if (info.cdSize != 0) {
            prevId = -1;
            continue;
        }
        info.cdPosn = entry.posn;
        info.cdSize = entry.size;
        if (prevId != -1) {
            Info(prevId).nextOnCd = int16_t(id);
        }
        prevId = id;
    }
    ColStore_Update();
    if (file.data != nullptr) {
        Free(file.data);
        file.data = nullptr;
    }
}

void __cdecl LoadCdDirectoryIPB(const char* path) {
    const char* extension = ms_bUseIpc ? "IPC" : "IPB";
    game::LoadingScreen("CStreaming::LoadCdDirectoryIPB", nullptr, 0);
    int dir = OpenFile(path, "r", 1);
    // the archive: the directory's name with .IMG
    char imagePath[40];
    size_t length = size_t(strrchr(path, '.') - path);
    strncpy(imagePath, path, length);
    imagePath[length] = '\0';
    strcat(imagePath, ".IMG");
    int image = 0;
    CdDirEntry entry;
    while (ReadFile(dir, &entry, sizeof(entry))) {
        char* dot = strchr(entry.name, '.');
        *dot = '\0';
        if (_strnicmp(dot + 1, extension, 3) != 0) {
            continue;
        }
        int ipl = FindIplSlotByHash(HashName(entry.name));
        if (image == 0) {
            image = OpenFile(imagePath, "r", 1);
            if (image == 0) {
                continue;
            }
        }
        SeekFile(image, entry.posn << 11, 0);
        int size = ReadFile(image, ipbBuffer, entry.size << 11);
        game::LoadIpb(ipl, ipbBuffer, size_t(size), 1);
    }
    if (image != 0) {
        CloseFile(image);
    }
    CloseFile(dir);
    ColStore_Update();
}

void __cdecl LoadCdDirectoryIPBFiles() {
    for (int image = CdStreamGetNumImages(); image != 0;) {
        image--;
        game::LoadingScreen("CStreaming::LoadCdDirectoryIPBFiles()", nullptr, 0);
        char path[128];
        strcpy(path, CdStreamGetImageName(image));
        strncpy(strrchr(path, '.') + 1, "DIR", 3);
        game::LoadCdDirectoryIPB(path, image);
    }
}

void __cdecl SwapWithExtraEntry(int id, int extra) {
    CStreamingInfo& info = Info(id);
    // (without a position, the game passes on whatever was on its stack)
    uint32_t oldPosn = 0, oldSize = 0;
    if (info.cdSize != 0) {
        oldSize = info.cdSize;
        oldPosn = info.cdPosn;
    }
    uint32_t posn, size;
    Directory_GetItem(ms_pExtraObjectsDir, extra, &posn, &size);
    info.cdSize = size;
    info.cdPosn = posn;
    Directory_SetItem(ms_pExtraObjectsDir, extra, oldPosn, oldSize);
    info.flags ^= 0x40;
}

void __cdecl AddExtraEntry(int id, const char* name) {
    CStreamingInfo& info = Info(id);
    CdDirEntry entry{};   // (the game's is uninitialised without a position)
    if (info.cdSize != 0) {
        entry.posn = info.cdPosn;
        entry.size = info.cdSize;
    }
    strcpy(entry.name, name);
    Directory_AddItem(ms_pExtraObjectsDir, &entry);
}

const char* __cdecl GetFileName(int id) {
    char* name = reinterpret_cast<char*>(0x00CE6CF8);
    if (id < TXD_ID_BASE) {
        sprintf(name, "%d.nif", CModelInfo::ms_modelInfoPtrs[id]->nameHash);
    } else if (id < COL_ID_BASE) {
        sprintf(name, "%d.nft", TxdGetNameHash(id - TXD_ID_BASE));
    } else if (id < AGR_ID_BASE) {
        sprintf(name, "%s.col", ColGetName(id - COL_ID_BASE));
    } else if (id < IPB_ID_BASE) {
        uint32_t index;
        AgrGroup* group = game::GetAgrGroupOf(uint32_t(id), &index);
        // (out of range, the game prints its null entry's name: address 4)
        const char* agr = index < group->numEntries ? group->entries[index].name : reinterpret_cast<const char*>(4);
        sprintf(name, "%s.AGR", agr);
    } else if (id < LIP_ID_BASE) {
        sprintf(name, ms_bUseIpc ? "%s.IPC" : "%s.IPB", IplGetName(id - IPB_ID_BASE));
    } else if (id < CAT_ID_BASE) {
        sprintf(name, "lipfile_%03d.lip.LIP", id - LIP_ID_BASE);
    } else if (id < LUR_ID_BASE) {
        sprintf(name, "%s.%s", CatPool_GetName(game::pool_C0F7B4, id - CAT_ID_BASE), "CAT");
    } else {
        sprintf(name, "%s.LUC", LurStore_GetName(game::lurStore, id - LUR_ID_BASE));
    }
    return name;
}

// ---- memory ---------------------------------------------------------------------------

uint8_t __cdecl HasMemoryFor(int bytes, int id) {
    // a dictionary's textures take more (or at least 32K) of the texture heap
    int textureBytes = bytes;
    if (unsigned(id - TXD_ID_BASE) < 10000) {
        if (bytes <= txdMemorySmall) {
            textureBytes = 0x8000;
        } else {
            textureBytes = int(double(bytes) * double(txdMemoryScale) + double(txdMemoryBase));
        }
        if (textureBytes > txdMemoryMax) {
            textureBytes = txdMemoryMax;
        }
    }
    if (id == TXD_ID_BASE) {
        return TextureHeapFree() >= bytes;   // (the first dictionary: the texture heap only)
    }
    if (id != -1 && unsigned(id - TXD_ID_BASE) < 10000) {
        return *ms_pFreeMemory >= bytes && game::HasTextureMemoryFor(textureBytes);
    }
    return *ms_pFreeMemory >= bytes;
}

// ---- flags ----------------------------------------------------------------------------

static void SetLoadedRequired(int id, uint8_t flag) {
    CStreamingInfo& info = Info(id);
    if (info.loadState != LOADSTATE_LOADED) {
        return;
    }
    // kept in memory: out of the loaded list (what gets removed)
    if (info.InList()) {
        info.RemoveFromList();
    }
    info.flags |= flag;
}

void __cdecl SetLoadedMissionRequired(int id) {
    SetLoadedRequired(id, STREAMING_MISSION_REQUIRED);
}

void __cdecl SetLoadedGameRequired(int id) {
    SetLoadedRequired(id, STREAMING_GAME_REQUIRED);
}

// SetModelIsDeletable / SetMissionDoesntRequireModel: one flag off; the entry goes back
// to the loaded list (or is dropped, if it isn't loaded) unless the other keeps it, then
// the same for what a model needs
static void ClearRequiredFlag(int id, uint8_t flag, void(__cdecl* self)(int)) {
    for (;;) {
        CStreamingInfo& info = Info(id);
        info.flags &= uint8_t(~flag);
        uint8_t other = STREAMING_KEEP_IN_MEMORY & ~flag;
        // (peds (type 7) aren't; for ids other than models, dictionaries and animation
        // groups the game reads a model info past the array)
        bool txdOrAgr = unsigned(id - TXD_ID_BASE) < unsigned(COL_ID_BASE - TXD_ID_BASE) ||
                        unsigned(id - AGR_ID_BASE) < unsigned(IPB_ID_BASE - AGR_ID_BASE);
        if ((txdOrAgr || CModelInfo::ms_modelInfoPtrs[id]->type != 7) && !(info.flags & other)) {
            if (info.loadState == LOADSTATE_LOADED) {
                if (!info.InList()) {
                    info.AddToList(ms_pStartLoadedList);
                }
            } else {
                game::RemoveModel(id);
            }
        }
        if (id >= TXD_ID_BASE) {
            return;
        }
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
        self(mi->txdIndex + TXD_ID_BASE);
        int dependency = mi->GetAnimFileIndex();
        if (dependency != -1) {
            self(dependency);
        }
        dependency = mi->GetDependency8();
        if (dependency != -1) {
            self(dependency);
        }
        dependency = mi->GetDependency9();
        if (dependency != -1) {
            self(dependency);
        }
        id = mi->GetDependency10();   // (the last one in the loop)
        if (id == -1) {
            return;
        }
    }
}

void __cdecl SetModelIsDeletable(int id) {
    ClearRequiredFlag(id, STREAMING_GAME_REQUIRED, game::SetModelIsDeletable);
}

void __cdecl SetMissionDoesntRequireModel(int id) {
    ClearRequiredFlag(id, STREAMING_MISSION_REQUIRED, game::SetMissionDoesntRequireModel);
}

void __cdecl SetMissionDoesntRequireSpecialModel(int index) {
    game::SetMissionDoesntRequireModel(specialModelBase + index);
}

uint8_t __cdecl RequestModelIfNotLoaded(int id, uint32_t flags) {
    if (Info(id).loadState == LOADSTATE_REQUESTED) {
        return 0;
    }
    if (Info(id).loadState != LOADSTATE_LOADED) {
        game::RequestModel(id, flags | 0x14);
        return 0;
    }
    if (flags & STREAMING_MISSION_REQUIRED) {
        game::SetLoadedMissionRequired(id);
    }
    if (flags & STREAMING_GAME_REQUIRED) {
        game::SetLoadedGameRequired(id);
    }
    return 1;
}

// ---- removing -------------------------------------------------------------------------

uint8_t __cdecl RemoveLoadedPed() {
    for (int tries = 1;; tries++) {
        ms_lastPedSlotRemoved++;
        if (ms_lastPedSlotRemoved == 20) {
            ms_lastPedSlotRemoved = 0;
        }
        int id = ms_pedsLoaded[ms_lastPedSlotRemoved];
        if (tries > 20) {
            return 0;
        }
        if (id == -1 || (Info(id).flags & STREAMING_KEEP_IN_MEMORY) || CModelInfo::ms_modelInfoPtrs[id]->numRefs > 0 ||
            Info(id).loadState != LOADSTATE_LOADED) {
            continue;
        }
        game::RemoveModel(id);
        ms_numPedsLoaded--;
        ms_pedsLoaded[ms_lastPedSlotRemoved] = -1;
        return 1;
    }
}

void __cdecl RemoveTxdModel(int txd) {
    game::RemoveModel(txd + TXD_ID_BASE);
}

void __cdecl RemoveAgr(int id) {
    game::RemoveModel(id);
    int image = id % 4;
    if (image != 0 && (ms_aUnknownCA38A0[image] != ms_aUnknownCA38A0[0] || ms_imageOffsets[image] == ms_imageOffsets[0])) {
        ms_bLoadingFlagC1A9EA = 1;
    }
}

// An entity out of its pool slot (or nullptr)
static void* PoolAt(CPool<uint8_t>* pool, int slot) {
    return pool->IsFreeSlot(slot) ? nullptr : pool->objects + pool->elemSize * slot;
}

static void DeleteRwObjectAndModel(void* entity) {
    Entity_DeleteRwObject(entity);
    int model = EntityModel(entity);
    if (CModelInfo::ms_modelInfoPtrs[model]->numRefs == 0) {
        game::RemoveModel(model);
    }
}

void __cdecl DeleteAllRwObjects() {
    for (int i = buildingPool->size; i != 0;) {
        i--;
        void* entity = PoolAt(buildingPool, i);
        if (entity != nullptr && Field<int32_t>(entity, 0x80) == 0) {
            DeleteRwObjectAndModel(entity);
        }
    }
    for (int i = treadablePool->size; i != 0;) {
        i--;
        void* entity = PoolAt(treadablePool, i);
        if (entity != nullptr && Field<int32_t>(entity, 0x80) == 0) {
            DeleteRwObjectAndModel(entity);
        }
    }
    // (the objects: the pools chained on after it too)
    CPool<uint8_t>* pool = objectPool;
    void* overflow = reinterpret_cast<void*>(pool->unknown10);
    int count = pool->size;
    if (overflow != nullptr) {
        count += Pool_ChainSize(overflow);
    }
    for (int i = count; i != 0;) {
        i--;
        void* entity = nullptr;
        if (i < pool->size) {
            entity = pool->objects + pool->elemSize * i;
        } else if (pool->unknown10 != 0) {
            entity = Pool_ChainAt(reinterpret_cast<void*>(pool->unknown10), i - pool->size, 1);
        }
        // (the flags are read at i even past this pool's slots, as in the game)
        if (!(pool->flags[i] & 0x80) && entity != nullptr && Field<int32_t>(entity, 0x80) == 0 && Field<uint8_t>(entity, 0x208) == 1) {
            DeleteRwObjectAndModel(entity);
        }
    }
    for (int i = dummyPool->size; i != 0;) {
        i--;
        void* entity = PoolAt(dummyPool, i);
        if (entity != nullptr && Field<int32_t>(entity, 0x80) == 0) {
            DeleteRwObjectAndModel(entity);
        }
    }
}

void __cdecl DeleteAllRwObjectsIf(int doIt) {
    if (doIt != 0) {
        game::DeleteAllRwObjects();
    }
}

// (every 76 steps, the loading screen gets a turn)
struct ProgressTicker {
    bool show;
    int countdown = 0x4B;
    void Step() {
        if (show && countdown-- < 1) {
            KeepLoadingScreenAlive(1, 0);
            countdown = 0x4B;
        }
    }
};

void __cdecl RemoveAllUnusedModels(int showProgress) {
    bool show = re::Flag(showProgress);
    if (show) {
        KeepLoadingScreenAlive(1, 0);
    }
    ProgressTicker ticker{show};
    for (int i = 0; i < 20; i++) {
        game::RemoveLoadedPed();
        ticker.Step();
    }
    if (show) {
        KeepLoadingScreenAlive(1, 0);
    }
    ProgressTicker ticker2{show};
    for (int id = 0x280; id < TXD_ID_BASE; id++) {
        int index = GetInfoIndex(id);
        if (index != -1) {
            CStreamingInfo& info = ms_aInfoForModel[index];
            if (info.loadState == LOADSTATE_LOADED && !(info.flags & STREAMING_GAME_REQUIRED) &&
                CModelInfo::ms_modelInfoPtrs[id]->numRefs == 0) {
                game::RemoveModel(id);
                info.flags &= STREAMING_KEPT_BITS;
            }
        }
        ticker2.Step();
    }
}

void __cdecl RemoveUnusedModelsInLoadedList() {
    const char* title = "CStreaming::RemoveUnusedModelsInLoadedList()";
    game::LoadingScreen(title, "Start", 0);
    game::LoadingScreen(title, "Remove Vehicles", 0);
    for (int i = 0; i < 20; i++) {
        game::RemoveLoadedPed();
    }
    // from the oldest
    CStreamingInfo* info = AtIndex(ms_pEndLoadedList->prev);
    while (info != ms_pStartLoadedList) {
        game::LoadingScreen(title, "Loop", 0);
        int id = IdOf(info);
        info = AtIndex(info->prev);
        if (id < TXD_ID_BASE) {
            if (CModelInfo::ms_modelInfoPtrs[id]->numRefs == 0) {
                game::RemoveModel(id);
                info = AtIndex(ms_pEndLoadedList->prev);   // (from the end again)
            }
        } else if (id < COL_ID_BASE) {
            int txd = id - TXD_ID_BASE;
            if (game::TxdGetNumRefs(txd) == 0 && !game::IsTxdWanted(txd)) {
                game::RemoveModel(id);
            }
        }
    }
    game::LoadingScreen(title, "End", 0);
}

uint8_t __cdecl RemoveLeastUsedModel(uint32_t keepFlags) {
    for (CStreamingInfo* info = AtIndex(ms_pEndLoadedList->prev); info != ms_pStartLoadedList; info = AtIndex(info->prev)) {
        int id = IdOf(info);
        if (info->flags & uint8_t(keepFlags)) {
            continue;
        }
        if (id < TXD_ID_BASE) {
            if (CModelInfo::ms_modelInfoPtrs[id]->numRefs == 0) {
                game::RemoveModel(id);
                return 1;
            }
        } else if (id < COL_ID_BASE) {
            int txd = id - TXD_ID_BASE;
            if (game::TxdGetNumRefs(txd) == 0 && !game::IsTxdWanted(txd)) {
                game::RemoveModel(id);
                return 1;
            }
        } else if (unsigned(id - AGR_ID_BASE) < 0x244) {
            uint32_t index;
            AgrGroup* group = game::GetAgrGroupOf(uint32_t(id), &index);
            if (group->entries[index].numRefs == 0 && !game::IsAgrWanted(id)) {
                game::RemoveAgrModel(id);
                return 1;
            }
        }
    }
    // then the peds, if there are many (or any, indoors)
    if ((ms_numPedsLoaded > 7 || (currArea != 0 && ms_numPedsLoaded > 0)) && game::RemoveLoadedPed()) {
        return 1;
    }
    return 0;
}

void __cdecl MakeSpaceFor(int bytes) {
    if (*ms_pFreeMemory >= bytes) {
        return;
    }
    while (game::RemoveLeastUsedModel(STREAMING_0x10)) {
        if (bytes <= *ms_pFreeMemory) {
            return;
        }
    }
    game::DeleteRwObjectsBehindCamera(bytes, -1, 0);
}

void __cdecl MakeSpaceForMemoryObject(int bytes, int id) {
    // only models, dictionaries and prop animations count
    if ((id >= COL_ID_BASE && id < LIP_ID_BASE) || (id >= LIP_ID_BASE && id < CAT_ID_BASE) || id >= LUR_ID_BASE) {
        return;
    }
    // (asked twice, as in the game)
    if (game::HasMemoryFor(bytes, id) || game::HasMemoryFor(bytes, id)) {
        return;
    }
    while (game::RemoveLeastUsedModel(STREAMING_0x10)) {
        if (game::HasMemoryFor(bytes, id)) {
            return;
        }
    }
    game::DeleteRwObjectsBehindCamera(bytes, id, 0);
}

// ---- requests ---------------------------------------------------------------------------

// buildings with a LOD (+0x58) that isn't one of a pair (+0x5C)
void __cdecl RequestBigBuildings() {
    for (int i = buildingPool->size; i != 0;) {
        i--;
        void* entity = PoolAt(buildingPool, i);
        if (entity != nullptr && Field<void*>(entity, 0x58) != nullptr && Field<void*>(entity, 0x5C) == nullptr) {
            game::RequestModel(EntityModel(entity), STREAMING_GAME_REQUIRED);
        }
    }
}

void __cdecl RequestBigBuildingsNear(int, const float* pos) {
    for (int i = buildingPool->size; i != 0;) {
        i--;
        void* entity = PoolAt(buildingPool, i);
        if (entity == nullptr || Field<void*>(entity, 0x58) == nullptr) {
            continue;
        }
        if (Field<void*>(entity, 0x5C) == nullptr) {
            game::RequestModel(EntityModel(entity), STREAMING_GAME_REQUIRED);
        } else if (Entity_IsInArea(entity, pos)) {
            game::RequestModel(EntityModel(entity), 0);
        }
    }
}

void __cdecl RequestPlayerModels() {
    // (from the last)
    for (int i = 5; i >= 0; i--) {
        game::RequestModel(ms_playerModels[i], 0x17);
    }
}

// ---- flushing -----------------------------------------------------------------------------

void __cdecl FlushChannels() {
    if (ms_channel[1].state == 2) {
        game::ProcessLoadingChannel(1, true);
    }
    if (ms_channel[0].state == 1) {
        game::CdStreamSync(0);
        game::ProcessLoadingChannel(0, true);
    }
    if (ms_channel[0].state == 2) {
        game::ProcessLoadingChannel(0, true);
    }
    if (ms_channel[1].state == 1) {
        game::CdStreamSync(1);
        game::ProcessLoadingChannel(1, true);
    }
    if (ms_channel[1].state == 2) {
        game::ProcessLoadingChannel(1, true);
    }
}

void __cdecl FlushRequestList() {
    CStreamingInfo* info = AtIndex(ms_pStartRequestedList->next);
    while (info != ms_pEndRequestedList) {
        CStreamingInfo* next = AtIndex(info->next);
        game::RemoveModel(IdOf(info));
        info = next;
    }
    game::FlushChannels();
}

// ---- models ------------------------------------------------------------------------------

// A dictionary slot's textures (the game reads through a free slot's null)
static TexDictionary* TxdDict(int slot) {
    CPool<TxdDef>* pool = CTxdStore::ms_pTxdPool;
    TxdDef* def = pool->IsFreeSlot(slot) ? nullptr : &pool->objects[slot];
    return *reinterpret_cast<TexDictionary* const volatile*>(reinterpret_cast<uintptr_t>(def) + offsetof(TxdDef, dict));
}

static uint8_t AgrLoaded(const AgrEntry* entry) {
    return *reinterpret_cast<const volatile uint8_t*>(reinterpret_cast<uintptr_t>(entry) + offsetof(AgrEntry, loaded));
}

uint8_t __cdecl LoadModel(game::MemoryBuffer* buffer, int stream, int id, CStreamingInfo* info, CBaseModelInfo* mi) {
    int dependencies[4] = {mi->GetAnimFileIndex(), mi->GetDependency8(), mi->GetDependency9(), mi->GetDependency10()};
    AgrGroup* groups[4];
    uint32_t indices[4] = {};
    for (int i = 0; i < 4; i++) {
        groups[i] = game::GetAgrGroupOf(uint32_t(dependencies[i]), &indices[i]);
    }
    // its dictionary and animations must be in
    bool ready = TxdDict(mi->txdIndex) != nullptr;
    for (int i = 0; i < 4 && ready; i++) {
        if (dependencies[i] == -1) {
            continue;
        }
        if (groups[i] == nullptr) {
            ready = false;
            break;
        }
        // (the first two inlined, the others through the group's getter: the same)
        const AgrEntry* entry = i < 2 ? AgrEntryAt(groups[i], indices[i]) : AgrGroup_GetEntry(groups[i], indices[i]);
        ready = AgrLoaded(entry) != 0;
    }
    if (!ready) {
        game::RemoveModel(id);
        if (TxdDict(mi->txdIndex) == nullptr) {
            game::RemoveModel(mi->txdIndex + TXD_ID_BASE);
        }
        game::RequestModel(id, info->flags);
        game::CloseStream(stream, buffer);
        return 0;
    }
    // held while it's made
    game::Txd_5F1130(mi->txdIndex);
    for (int i = 0; i < 4; i++) {
        if (dependencies[i] != -1 && groups[i] != nullptr) {
            AnimManager_AddRef(game::animManager, groups[i], indices[i]);
        }
    }
    game::PushMemoryTag(0xD);
    game::Txd_5F1100(mi->txdIndex);
    uint8_t made = game::LoadModelData(buffer, id, info, mi);
    game::PopMemoryTag();
    // (a big one still being made keeps them until it's finished)
    if (info->loadState != LOADSTATE_FINISHING) {
        game::Txd_5F11B0(mi->txdIndex);
        for (int i = 0; i < 4; i++) {
            if (dependencies[i] != -1 && groups[i] != nullptr) {
                AnimManager_RemoveRef(game::animManager, groups[i], indices[i], 1);
            }
        }
    }
    if (!made) {
        game::RemoveModel(id);
        game::RequestModel(id, info->flags);
        game::CloseStream(stream, buffer);
        return 0;
    }
    return 1;
}

uint8_t __cdecl LoadModelData(game::MemoryBuffer* buffer, int id, CStreamingInfo* info, CBaseModelInfo* mi) {
    if (mi->type == MODEL_INFO_TYPE_1 || mi->type == MODEL_INFO_TYPE_3) {
        return LoadAtomicModel(buffer, id, info, mi);
    }
    // (peds (7) and the rest: the same call)
    return LoadClumpModel(buffer, id);
}

void __cdecl RequestSpecialModel(int id, const char* name, uint32_t flags) {
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
    char shortName[64];
    strncpy(shortName, name, sizeof(shortName));
    if (ModelInfo_MatchesName(mi, shortName)) {
        game::RequestModel(id, flags);
        return;
    }
    if (mi->numRefs > 0) {
        // what uses it goes first: peds (unless in state 0xD), objects
        for (int i = pedPool->size; i != 0;) {
            i--;
            if (mi->numRefs <= 0) {
                break;
            }
            void* ped = PoolAt(pedPool, i);
            if (ped != nullptr && EntityModel(ped) == id && Field<int32_t>(ped, 0x1310) != 0xD) {
                Ped_Remove(ped);
            }
        }
        int count = objectPool->size;
        if (objectPool->unknown10 != 0) {
            count += Pool_ChainSize(reinterpret_cast<void*>(objectPool->unknown10));
        }
        while (count != 0) {
            count--;
            if (mi->numRefs <= 0) {
                break;
            }
            CPool<uint8_t>* objects = objectPool;
            void* object = nullptr;
            if (count < objects->size) {
                object = objects->objects + objects->elemSize * count;
            } else if (objects->unknown10 != 0) {
                object = Pool_ChainAt(reinterpret_cast<void*>(objects->unknown10), count - objects->size, 1);
            }
            if (!(objects->flags[count] & 0x80) && object != nullptr && EntityModel(object) == id && Object_CanBeDeleted(object)) {
                World_Remove(object);
                Object_Detach(object);
                reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*static_cast<re::Ptr32<re::Ptr32<void>>*>(object))[0]))(object, 1);   // deleted
            }
        }
    }
    ModelInfo_522220(mi, name, 1);
    game::RemoveModel(id);
    uint32_t posn, size;
    Directory_FindItem3(ms_pExtraObjectsDir, name, &posn, &size);
    mi->txdIndex = -1;
    ModelInfo_SetTexDictionary(mi, FindTxdSlot(name, 0) == -1 ? "generic" : name, 0);
    CStreamingInfo& info = Info(id);
    info.cdPosn = posn;
    info.cdSize = size;
    game::RequestModel(id, flags);
}

void __cdecl RequestModelFromExtraEntry(int id, uint32_t hash, uint32_t flags) {
    game::RemoveModel(id);
    uint32_t posn, size;
    if (Directory_FindItemByHash(ms_pExtraObjectsDir, hash, &posn, &size)) {
        ModelInfo_SetTexDictionary(CModelInfo::ms_modelInfoPtrs[id], "generic", 0);
        CStreamingInfo& info = Info(id);
        info.cdPosn = posn;
        info.cdSize = size;
        game::RequestModel(id, flags);
    }
}

// ---- init ----------------------------------------------------------------------------------

static uint16_t ListIndex(const CStreamingInfo* info) {
    return uint16_t(uint32_t(reinterpret_cast<uintptr_t>(info) - re::PtrCast<uintptr_t>(ms_pInfoBase)) >> 4);
}

void __cdecl Init() {
    ActTreeRefPool_Init(game::pool_C0F7B4, 0x78, "ActTreeRefPool");
    ms_pInfoBase = ms_aInfoForModel;
    ms_pStartLoadedList = &ms_aInfoForModel[0x4330];
    ms_pEndLoadedList = &ms_aInfoForModel[0x4331];
    ms_pStartRequestedList = &ms_aInfoForModel[0x4332];
    ms_pEndRequestedList = &ms_aInfoForModel[0x4333];
    game::AddIdRange(TXD_ID_BASE, 0x2E5C);   // dictionaries through the lip sync files
    ms_pStartLoadedList->next = ListIndex(ms_pEndLoadedList);
    ms_pStartLoadedList->prev = 0xFFFF;
    ms_pEndLoadedList->next = 0xFFFF;
    ms_pEndLoadedList->prev = ListIndex(ms_pStartLoadedList);
    ms_pStartRequestedList->next = ListIndex(ms_pEndRequestedList);
    ms_pStartRequestedList->prev = 0xFFFF;
    ms_pEndRequestedList->next = 0xFFFF;
    ms_pEndRequestedList->prev = ListIndex(ms_pStartRequestedList);
    *reinterpret_cast<int32_t*>(0x00CA367C) = 0;
    *reinterpret_cast<int32_t*>(0x00CA3678) = 0;
    ms_streamingBufferSize = 0;
    ms_bDisableStreaming = 0;
    ms_unknownCA398C = 0;
    ms_channelError = -1;
    ms_bSingleChannel = 0;
    ms_channel[0].state = 0;
    ms_channel[1].state = 0;
    for (int i = 0; i < 4; i++) {
        ms_channel[0].modelIds[i] = -1;
        ms_channel[1].modelIds[i] = -1;
    }
    const char* title = "CStreaming::Init()";
    game::LoadingScreen(title, "Loading stuff", 0);
    game::LoadingScreen(title, "Init TXD", 0);
    // the dictionaries loaded already are loaded as far as streaming goes
    for (int slot = 0; slot < 10000; slot++) {
        CPool<TxdDef>* pool = CTxdStore::ms_pTxdPool;
        TxdDef* def = pool->IsFreeSlot(slot) ? nullptr : &pool->objects[slot];
        if (def != nullptr && def->dict != nullptr) {
            Info(slot + TXD_ID_BASE).loadState = LOADSTATE_LOADED;
        }
    }
    for (int i = 0; i < 20; i++) {
        ms_pedsLoaded[i] = -1;
    }
    ms_numPedsLoaded = 0;
    *reinterpret_cast<int32_t*>(0x00CA3690) = 8;
    void* dir = Malloc(0x10);
    if (dir != nullptr) {
        Directory_Construct(dir, 0x640);
        *static_cast<re::Ptr32<void>*>(dir) = reinterpret_cast<void*>(0x009186D0);   // CDirectory's vtable
        ms_pExtraObjectsDir = dir;
    } else {
        ms_pExtraObjectsDir = nullptr;
    }
    memset(ms_aCatNodes, 0, 0x78 * sizeof(int32_t));
    game::LoadingScreen(title, "LoadCdDirectory", 0);
    *reinterpret_cast<uint16_t*>(0x00CA368C) = 0;
    *reinterpret_cast<int32_t*>(0x00CA3684) = -1;
    ms_numPriorityRequests = 0;
    LoadCdDirectories();
    // one buffer for both channels, each as big as the biggest entry
    ms_streamingBufferSize *= 2;
    game::LoadingScreen(title, "Allocate streaming buffer", 0);
    game::PushMemoryTag(0x1B);
    ms_pStreamingBuffer[0] = MallocAligned(size_t(ms_streamingBufferSize) << 11, 0x40);
    game::PopMemoryTag();
    ms_streamingBufferSize /= 2;
    ms_desiredNumPeds = 12;
    ms_pStreamingBuffer[1] = static_cast<uint8_t*>(ms_pStreamingBuffer[0]) + ms_streamingBufferSize * 0x800;
    SetSeasonalFlags_();
    game::LoadingScreen(title, "Island Graphics", 0);
    ms_bStreamingInitialised = 1;
    for (int id = seasonalModelsFirst; id <= seasonalModelsLast; id++) {
        if (CModelInfo::ms_modelInfoPtrs[id] != nullptr) {
            ModelInfo_522420(CModelInfo::ms_modelInfoPtrs[id]);
        }
    }
    // the lip sync files and the animation groups are read in from the start
    for (int i = 0; i < 0x1ED; i++) {
        game::RequestModel(LIP_ID_BASE + i, 0);
    }
    for (int id = AGR_ID_BASE; id < AGR_ID_BASE + 0x244; id++) {
        ms_pPreloaded[id] = nullptr;
        if (Info(id).cdSize != 0) {
            game::RequestModel(id, 0);
        }
    }
    LurStore_Init(game::lurStore);
}

// (inlined in the callers; the same as RequestPlayerModels)
static void RequestPlayerModelsInline() {
    for (int i = 5; i >= 0; i--) {
        game::RequestModel(ms_playerModels[i], 0x17);
    }
}

void __cdecl ReInit() {
    KeepLoadingScreenAlive(1, 0);
    game::FlushRequestList();
    for (int id = 0; id < TXD_ID_BASE; id++) {
        KeepLoadingScreenAlive(1, 0);
        if (CModelInfo::ms_modelInfoPtrs[id] != nullptr) {
            Info(id).flags &= uint8_t(~STREAMING_KEEP_IN_MEMORY);
            game::SetMissionDoesntRequireModel(id);
            game::SetModelIsDeletable(id);
            game::RemoveModel(id);
        }
    }
    for (int i = 0; i < 20; i++) {
        KeepLoadingScreenAlive(1, 0);
        game::RemoveLoadedPed();
    }
    KeepLoadingScreenAlive(1, 0);
    if (radar != nullptr) {
        Radar_ReInit(radar);
    }
    game::RemoveAllUnusedModels(1);
    RequestPlayerModelsInline();
    KeepLoadingScreenAlive(1, 0);
}

void __cdecl RemoveMissionModels() {
    KeepLoadingScreenAlive(1, 0);
    for (int id = 0; id < TXD_ID_BASE; id++) {
        if (CModelInfo::ms_modelInfoPtrs[id] != nullptr && (Info(id).flags & STREAMING_MISSION_REQUIRED)) {
            game::SetMissionDoesntRequireModel(id);
        }
    }
    game::FlushRequestList();
    KeepLoadingScreenAlive(1, 0);
    game::DeleteAllSectorRwObjects();
    KeepLoadingScreenAlive(1, 0);
    game::RemoveAllUnusedModels(1);
    ms_bDisableStreaming = 0;
    KeepLoadingScreenAlive(1, 0);
    RequestPlayerModelsInline();
}

// ---- the scene -------------------------------------------------------------------------------

// Requests nothing keeps (none of the flags 1..8) dropped, from the newest
static void RemoveUnflaggedRequests() {
    CStreamingInfo* info = AtIndex(ms_pEndRequestedList->prev);
    while (info != ms_pStartRequestedList) {
        CStreamingInfo* prev = AtIndex(info->prev);
        if (!(info->flags & 0xF)) {
            game::RemoveModel(IdOf(info));
        }
        info = prev;
    }
}

void __cdecl LoadScene(const float* pos) {
    const char* title = "CStreaming::LoadScene()";
    game::LoadingScreen(title, "Start", 0);
    RemoveUnflaggedRequests();
    *reinterpret_cast<uint8_t*>(0x00C13F04) = 0;
    Fn_5E6480(object_D12AF8);
    game::DeleteSectorRwObjects();
    int area = *reinterpret_cast<int32_t*>(0x00BD0FC4);
    Fn_438830();
    Fn_416DC0(*reinterpret_cast<re::Ptr32<void>*>(0x009E4F4C));
    float xy[2] = {pos[0], pos[1]};
    ColStore_LoadNear(xy);
    xy[0] = pos[0];
    xy[1] = pos[1];
    ColStore_Fn_570080(xy);
    game::RequestBigBuildingsNear(area, pos);
    game::RequestBigBuildingsNear(0, pos);
    IplStore_LoadNear(Vec3{pos[0], pos[1], pos[2]}, 1);
    IplStore_Fn_439770(pos, 1);
    Fn_415AD0(*reinterpret_cast<re::Ptr32<void>*>(0x009E4F4C));
    game::InstanceBigBuildingsNear(area, pos);
    game::InstanceBigBuildingsNear(0, pos);
    AddModelsToRequestList(pos, 0x20);
    Fn_4A00B0(*reinterpret_cast<re::Ptr32<void>*>(0x00C2C108));
    if (radar != nullptr) {
        Radar_Fn_54E120(radar, pos);
    }
    game::LoadingScreen(title, "CStreaming::LoadAllRequestedModels()", 0);
    LoadAllRequestedModels_(0);
    Fn_52F210(pos);
    Fn_52F3D0(pos);
    Fn_438850();
    for (int i = 0; i < int(ms_numInfos); i++) {
        ms_aInfoForModel[i].flags &= uint8_t(~0x20);   // (LOADSCENE)
    }
    game::LoadingScreen(title, "End", 0);
}

void __cdecl Update() {
    if (ms_channelError != -1) {
        RetryLoadFile_(ms_channelError);
    } else {
        if (!*reinterpret_cast<uint8_t*>(0x00C1A99A) && !ms_bLoadingStalled && !*reinterpret_cast<uint8_t*>(0x00C1A998)) {
            if (!ms_bDisableStreaming && double(GetPlayerPos(player_C3CC68)[2]) < 55.0) {
                AddModelsToRequestList(GetPlayerPos(player_C3CC68), 0);
            }
            void* camera = Fn_6C9A90(Fn_6C82F0(*reinterpret_cast<int32_t*>(0x00C3CCF4)));
            ProcessSectorsAround(static_cast<uint8_t*>(camera) + 0x24);
            if (IsAreaInterior(currArea)) {
                // moving: what's behind the camera goes
                const float* velocity = Fn_445D60();
                float speedSq = float(double(velocity[0]) * velocity[0] + double(velocity[1]) * velocity[1]);
                if (speedSq > 0.09f) {
                    game::DeleteRwObjectsBehindCamera(0, -1, 1);
                }
            }
            float scratch[3];
            const float* at = GetCameraTarget(scratch);
            float xy[2] = {at[0], at[1]};
            ColStore_LoadNear(xy);
            float scratch2[3];
            at = GetCameraTarget(scratch2);
            IplStore_LoadNear(Vec3{at[0], at[1], at[2]}, 0);
            Fn_429990();
        }
        LoadRequestedModels_();
        // (a count of idle frames, and a busy flag)
        int32_t& idleFrames = *reinterpret_cast<int32_t*>(0x00CA399C);
        uint8_t& busy = *reinterpret_cast<uint8_t*>(0x00A957D4);
        if (ms_numModelsRequested <= 0 && ms_channel[0].state == 0 && ms_channel[1].state == 0) {
            if (++idleFrames > 5) {
                busy = 0;
            }
        } else {
            idleFrames--;
            if (!busy) {
                idleFrames = 0;
            }
            busy = 1;
        }
        RemoveUnflaggedRequests();
    }
    if (Fn_738840(0xC, 0) != 0.0f && Fn_738840(0xD, 0) != 0.0f) {
        Fn_738840(0, 0);
    }
}

// ---- the sectors' entity lists ------------------------------------------------------------

// The entity at a node, and the node after it
static void* NodeEntity(const uint32_t* node) {
    return GetPoolEntity(int(*node & 0xF), int((*node >> 4) & 0x3FFF));
}

static uint32_t* NextNode(const uint32_t* node) {
    uint32_t next = *node >> 18;
    return next == 0x3FFF ? nullptr : &ms_pEntityListNodes[next];
}

// sector (x, y)'s list (0..4)
static re::Ptr32<uint32_t>* SectorList(int x, int y, int list) {
    return reinterpret_cast<re::Ptr32<uint32_t>*>(0x00C1B178 + (y * 36 + x) * 0x14 + list * 4);
}

static bool InCurrentArea(void* entity) {
    uint8_t area = Field<uint8_t>(entity, 0x111);
    return int(area) == currArea || area == 1 ||
           (*reinterpret_cast<uint8_t*>(0x00BD09EC) && int(area) == int(*reinterpret_cast<int8_t*>(0x00BD09EB)));
}

void __cdecl DeleteRwObjectsInList(re::Ptr32<uint32_t>* list) {
    for (uint32_t* node = *list; node != nullptr;) {
        void* entity = NodeEntity(node);
        node = NextNode(node);
        if (Field<int32_t>(entity, 0x74) == 0 && Field<int32_t>(entity, 0x80) == 0) {
            Entity_DeleteRwObject(entity);
        }
    }
}

void __cdecl CreateRwObjectsInList(re::Ptr32<uint32_t>* list) {
    for (uint32_t* node = *list; node != nullptr;) {
        void* entity = NodeEntity(node);
        node = NextNode(node);
        if (InCurrentArea(entity) && EntityRwObject(entity) == nullptr) {
            Entity_Vf10(entity, 0, 1);
        }
    }
}

void __cdecl ProcessEntitiesInList(re::Ptr32<uint32_t>* list) {
    for (uint32_t* node = *list; node != nullptr;) {
        void* entity = NodeEntity(node);
        node = NextNode(node);
        if (Entity_Vf33(entity)) {
            Fn_5E6510(object_D12AF8, entity);
        }
    }
}

void __cdecl DeleteAllSectorRwObjects() {
    for (int x = 0; x < 36; x++) {
        for (int y = 0; y < 36; y++) {
            static const int lists[] = {0, 1, 4, 2, 3};
            for (int list : lists) {
                game::DeleteRwObjectsInList(SectorList(x, y, list));
            }
        }
    }
}

void __cdecl DeleteSectorRwObjects() {
    Fn_6C9A90(Fn_6C82F0(*reinterpret_cast<int32_t*>(0x00C3CCF4)));   // (unused)
    for (int x = 0; x < 36; x++) {
        for (int y = 0; y < 36; y++) {
            static const int lists[] = {0, 1, 4};
            for (int list : lists) {
                game::DeleteRwObjectsInList(SectorList(x, y, list));
            }
        }
    }
}

void __cdecl InstanceBigBuildingsNear(int, const float* pos) {
    for (int i = buildingPool->size; i != 0;) {
        i--;
        void* entity = PoolAt(buildingPool, i);
        if (entity != nullptr && Field<void*>(entity, 0x58) != nullptr && Field<void*>(entity, 0x5C) != nullptr &&
            EntityRwObject(entity) == nullptr && Entity_IsInArea(entity, pos)) {
            Entity_Vf10(entity, 1, 1);
        }
    }
}

void __cdecl DeletePropAnimRwObjects() {
    for (int i = propAnimPool->size; i != 0;) {
        i--;
        void* entity = PoolAt(propAnimPool, i);
        if (entity != nullptr && EntityRwObject(entity) != nullptr) {
            Entity_DeleteRwObject(entity);
        }
    }
}

// The model of an entity (if in the loaded list): its object deleted, the model removed
// if nothing else uses it; true once there's room (force: at once)
static bool RemoveEntityModel(void* entity, int bytes, int id, bool force) {
    int model = EntityModel(entity);
    if (!Info(model).InList()) {
        return false;
    }
    Entity_DeleteRwObject(entity);
    if (CModelInfo::ms_modelInfoPtrs[model]->numRefs != 0) {
        return false;
    }
    game::RemoveModel(model);
    return force || game::HasMemoryFor(bytes, id);
}

uint8_t __cdecl DeleteRwObjectsBehindCameraInList(re::Ptr32<uint32_t>* list, int bytes, int id, int force) {
    for (uint32_t* node = *list; node != nullptr;) {
        void* entity = NodeEntity(node);
        node = NextNode(node);
        if (!re::Flag(force) && (Field<int32_t>(entity, 0x48) != 0 || Field<int32_t>(entity, 0x74) != 0 ||
                                 Field<int32_t>(entity, 0x80) != 0 || EntityRwObject(entity) == nullptr)) {
            continue;
        }
        if (re::Flag(force) && (Field<int32_t>(entity, 0x74) != 0 || Field<int32_t>(entity, 0x80) != 0)) {
            continue;
        }
        if (RemoveEntityModel(entity, bytes, id, re::Flag(force))) {
            return 1;
        }
    }
    return 0;
}

uint8_t __cdecl DeleteRwObjectsNotInFrustumInList(re::Ptr32<uint32_t>* list, int bytes, int id, int force) {
    for (uint32_t* node = *list; node != nullptr;) {
        void* entity = NodeEntity(node);
        node = NextNode(node);
        if (Field<int32_t>(entity, 0x74) != 0 || Field<int32_t>(entity, 0x80) != 0 || EntityRwObject(entity) == nullptr) {
            continue;
        }
        if (Entity_IsVisible(entity) && Field<int32_t>(entity, 0xA4) == 0) {
            continue;
        }
        Entity_IsVisible(entity);   // (asked again, the answer unused)
        if (RemoveEntityModel(entity, bytes, id, re::Flag(force))) {
            return 1;
        }
    }
    return 0;
}

// ---- seasons ---------------------------------------------------------------------------------

// Dictionaries with seasonal versions (a base, and a winter / spring one found by
// the same name in chapters 2 / 3) get the seasonal flags: 0xC0 on the base, 0x40 on
// the winter one, 0x80 on the spring one; and the models using a flagged dictionary 0xC0.
void __cdecl SetSeasonalFlags() {
    if (ms_bAllSeasonal) {
        for (int id = TXD_ID_BASE; id < COL_ID_BASE; id++) {
            int index = GetInfoIndex(id);
            if (index != -1) {
                ms_aInfoForModel[index].flags |= 0xC0;
            }
        }
        return;
    }
    uint8_t chapter = CTxdStore::ms_currentChapter;
    ms_bSettingSeasons = 1;
    for (int slot = 0; slot + TXD_ID_BASE < COL_ID_BASE; slot++) {
        CPool<TxdDef>* pool = CTxdStore::ms_pTxdPool;
        if (pool->IsFreeSlot(slot) || re::PtrCast<uintptr_t>(pool->objects) + uintptr_t(pool->elemSize * slot) == 0) {
            continue;
        }
        uint32_t hash = TxdGetNameHash(slot);
        SetChapter(&CTxdStore::ms_currentChapter, 0);
        int base = FindTxdSlotByHash(hash, 2);
        if (base != slot) {
            continue;   // (only from the base's own slot)
        }
        TxdGetNameHash(base);   // (unused)
        SetChapter(&CTxdStore::ms_currentChapter, 2);
        int winter = FindTxdSlotByHash(hash, 3);
        if (winter != -1 && winter != base) {
            TxdGetNameHash(winter);
            Info(base + TXD_ID_BASE).flags |= 0xC0;
            Info(winter + TXD_ID_BASE).flags |= 0x40;
        }
        SetChapter(&CTxdStore::ms_currentChapter, 3);
        int spring = FindTxdSlotByHash(hash, 4);
        if (spring != -1 && spring != base) {
            TxdGetNameHash(spring);
            Info(base + TXD_ID_BASE).flags |= 0xC0;
            Info(spring + TXD_ID_BASE).flags |= 0x80;
        }
    }
    SetChapter(&CTxdStore::ms_currentChapter, 0);
    for (int id = 300; id < TXD_ID_BASE; id++) {
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
        if (mi != nullptr && (Info(mi->txdIndex + TXD_ID_BASE).flags & 0xC0)) {
            Info(id).flags |= 0xC0;
        }
    }
    ms_bSettingSeasons = 0;
    SetChapter(&CTxdStore::ms_currentChapter, chapter);
}

void __cdecl SwapSeasonalTextures() {
    if (!ms_bStreamingInitialised) {
        return;
    }
    Fn_444BB0();
    game::FlushRequestList();
    game::DeleteSectorRwObjects();
    for (int i = 0; i < 20; i++) {
        game::RemoveLoadedPed();
    }
    // the models nothing uses go (their dictionaries are swapped below)
    for (int id = 0x280; id < TXD_ID_BASE; id++) {
        int index = GetInfoIndex(id);
        if (index == -1) {
            continue;
        }
        CStreamingInfo& info = ms_aInfoForModel[index];
        if (info.loadState == LOADSTATE_LOADED && !(info.flags & STREAMING_GAME_REQUIRED) && CModelInfo::ms_modelInfoPtrs[id]->numRefs == 0) {
            game::RemoveModel(id);
            info.flags &= STREAMING_KEPT_BITS;
        }
    }
    game::RequestPlayerModels();
    // each seasonal model's dictionary: the current season's version (looked up once per dictionary)
    int32_t* swapped = static_cast<int32_t*>(Malloc(10000 * sizeof(int32_t)));
    memset(swapped, 0, 10000 * sizeof(int32_t));
    for (int id = 300; id < TXD_ID_BASE; id++) {
        CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[id];
        if (mi == nullptr) {
            continue;
        }
        int txd = mi->txdIndex;
        if (!(Info(ms_bAllSeasonal ? txd + TXD_ID_BASE : id).flags & 0xC0)) {
            continue;
        }
        int known = swapped[txd];
        if (known != 0) {
            if (known != txd) {
                mi->txdIndex = int16_t(known);
            }
        } else {
            uint32_t hash = TxdGetNameHash(mi->txdIndex);
            uint8_t season = TxdGetSeason(mi->txdIndex);
            int16_t slot = int16_t(FindTxdSlotByHash(hash, season));
            mi->txdIndex = slot;
            swapped[txd] = slot;
        }
    }
    Free(swapped);
    game::Update();
}

// ---- registration -----------------------------------------------------------------------

#define STREAMING2_REVERSED(fn, address)                                               \
    static decltype(&fn) Original_##fn;                                                \
    REVERSED("CStreaming::" #fn, address, RE_BODY(&fn), nullptr,       \
             reinterpret_cast<void**>(&Original_##fn), re::Mode::Original)

STREAMING2_REVERSED(BinarySearch, 0x0052DC10);
STREAMING2_REVERSED(CompareIdRange, 0x0052DC70);
STREAMING2_REVERSED(GetInfo, 0x0052DD30);
STREAMING2_REVERSED(AddIdRange, 0x0052DDA0);
STREAMING2_REVERSED(GetImageBase, 0x0052D8C0);
STREAMING2_REVERSED(SetCatNode, 0x0052D800);
STREAMING2_REVERSED(GetCatNode, 0x0052D810);
STREAMING2_REVERSED(Shutdown, 0x0052D820);
STREAMING2_REVERSED(HasTextureMemoryFor, 0x0052D8A0);
STREAMING2_REVERSED(ClearUnknownCA3688, 0x0052D9A0);
STREAMING2_REVERSED(FindFreePedSlot, 0x0052D9B0);
STREAMING2_REVERSED(InitAgrGroups, 0x0052D9D0);
STREAMING2_REVERSED(AddAgrGroup, 0x0052DA60);
STREAMING2_REVERSED(FindAgrId, 0x0052DAD0);
STREAMING2_REVERSED(GetAgrGroup, 0x0052DB50);
STREAMING2_REVERSED(GetAgrId, 0x0052DBD0);
STREAMING2_REVERSED(IsAgrWanted, 0x0052FB40);
STREAMING2_REVERSED(SetColLoaded, 0x0052DFF0);
STREAMING2_REVERSED(LoadCdDirectory, 0x0052E1B0);
STREAMING2_REVERSED(LoadCdDirectoryIPB, 0x0052E020);
STREAMING2_REVERSED(LoadCdDirectoryIPBFiles, 0x0052ED40);
STREAMING2_REVERSED(SwapWithExtraEntry, 0x0052E7F0);
STREAMING2_REVERSED(AddExtraEntry, 0x0052E870);
STREAMING2_REVERSED(GetFileName, 0x0052EAE0);
STREAMING2_REVERSED(HasMemoryFor, 0x0052E650);
STREAMING2_REVERSED(SetLoadedMissionRequired, 0x0052E720);
STREAMING2_REVERSED(SetLoadedGameRequired, 0x0052E780);
STREAMING2_REVERSED(SetModelIsDeletable, 0x0052FE40);
STREAMING2_REVERSED(SetMissionDoesntRequireModel, 0x0052FF40);
STREAMING2_REVERSED(SetMissionDoesntRequireSpecialModel, 0x00530040);
STREAMING2_REVERSED(RequestModelIfNotLoaded, 0x00533030);
STREAMING2_REVERSED(RemoveLoadedPed, 0x00530060);
STREAMING2_REVERSED(RemoveTxdModel, 0x00530860);
STREAMING2_REVERSED(RemoveAgr, 0x00530880);
STREAMING2_REVERSED(DeleteAllRwObjects, 0x005308D0);
STREAMING2_REVERSED(DeleteAllRwObjectsIf, 0x00531930);
STREAMING2_REVERSED(RemoveAllUnusedModels, 0x00530A70);
STREAMING2_REVERSED(RemoveUnusedModelsInLoadedList, 0x00530B50);
STREAMING2_REVERSED(RemoveLeastUsedModel, 0x00530C70);
STREAMING2_REVERSED(MakeSpaceFor, 0x00531840);
STREAMING2_REVERSED(MakeSpaceForMemoryObject, 0x00531880);
STREAMING2_REVERSED(RequestBigBuildings, 0x00532270);
STREAMING2_REVERSED(RequestBigBuildingsNear, 0x005322C0);
STREAMING2_REVERSED(RequestPlayerModels, 0x00533460);
STREAMING2_REVERSED(FlushChannels, 0x00533130);
STREAMING2_REVERSED(FlushRequestList, 0x005331B0);
STREAMING2_REVERSED(LoadModel, 0x00531950);
STREAMING2_REVERSED(LoadModelData, 0x005317F0);
STREAMING2_REVERSED(RequestSpecialModel, 0x00533200);
STREAMING2_REVERSED(RequestModelFromExtraEntry, 0x005333E0);
STREAMING2_REVERSED(Init, 0x005334C0);
STREAMING2_REVERSED(ReInit, 0x00533880);
STREAMING2_REVERSED(RemoveMissionModels, 0x00533980);
STREAMING2_REVERSED(LoadScene, 0x00534510);
STREAMING2_REVERSED(Update, 0x005346B0);
STREAMING2_REVERSED(DeleteRwObjectsInList, 0x0052E4F0);
STREAMING2_REVERSED(CreateRwObjectsInList, 0x0052E550);
STREAMING2_REVERSED(ProcessEntitiesInList, 0x0052E5E0);
STREAMING2_REVERSED(DeleteAllSectorRwObjects, 0x0052F080);
STREAMING2_REVERSED(DeleteSectorRwObjects, 0x0052F0E0);
STREAMING2_REVERSED(InstanceBigBuildingsNear, 0x0052F590);
STREAMING2_REVERSED(DeletePropAnimRwObjects, 0x0052F600);
STREAMING2_REVERSED(DeleteRwObjectsBehindCameraInList, 0x0052F8B0);
STREAMING2_REVERSED(DeleteRwObjectsNotInFrustumInList, 0x0052F9A0);
STREAMING2_REVERSED(SetSeasonalFlags, 0x0052E8D0);
STREAMING2_REVERSED(SwapSeasonalTextures, 0x005348D0);

} // namespace CStreaming
