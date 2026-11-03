#include "game/Placement.h"

#include <cstring>

#include "game/FileLoader.h"
#include "game/Streaming.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

GAME_VAR(uint8_t, ms_bLoadBoxes, 0x00CF3074);           // CColStore's: the bounds come from the .bin files
GAME_VAR(uint8_t, ms_bSaveBoxes, 0x00CF3073);           // CColStore's
GAME_VAR(uint8_t, ms_bUseIpc, 0x00BF3811);              // CStreaming's
GAME_VAR(uint8_t, ms_bFirstLoad_BF3817, 0x00BF3817);    // (the loaders read it: the IDE section too)
GAME_VAR(uint8_t, ms_bStreamingDisabled_CA3984, 0x00CA3984);
GAME_VAR(float, ms_iplLoadMargin, 0x00BF3804);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t*, FindPlayerPed_ptr, 0x00C1AEA8);
GAME_VAR(int32_t, ms_playerVehicleModel_BF3E58, 0x00BF3E58);
GAME_VAR(uint8_t, ms_bExtraPoint, 0x00BD09EA);          // a second point to load around
GAME_VAR(int8_t, ms_extraPointArea, 0x00BD09EB);
GAME_VAR(col::CVector, ms_extraPoint, 0x00BD09F0);
GAME_VAR(uint8_t, ms_bKeepModelsForIpls, 0x00BF3815);
GAME_VAR(int32_t, ms_unknown_BD5A98, 0x00BD5A98);
GAME_VAR(int32_t, ms_unknown_BD5A9C, 0x00BD5A9C);
GAME_VAR(int32_t, ms_numPendingProps, 0x00BD5C54);
GAME_ARRAY(void*, ms_modelInfoPtrs, 0x00C67738);

// the pools the entities are in (as bytes: the entity classes aren't here)
GAME_VAR(CPool<uint8_t>*, ms_pBuildingPool, 0x00C0F5F8);
GAME_VAR(CPool<uint8_t>*, ms_pDummyPool, 0x00C0F600);
GAME_VAR(CPool<uint8_t>*, ms_pPropAnimPool, 0x00C0F608);
GAME_VAR(CPool<uint8_t>*, ms_pObjectPool, 0x00C0F614);
GAME_VAR(CPool<uint8_t>*, ms_pProjectilePool, 0x00C0F618);
GAME_VAR(CPool<uint8_t>*, ms_pTriggerPool, 0x020C7C48);
inline void* const TheTriggers = reinterpret_cast<void*>(0x020C7E98);   // the triggers' names/hashes
GAME_VAR(void*, ms_pObj_C0F5E4, 0x00C0F5E4);
GAME_VAR(void*, ms_pPatrolPaths, 0x00D13CB4);

// a prop whose trigger wasn't there yet (0xC): at 0xBD3E78
struct PendingProp {
    uint32_t triggerHash;
    int32_t ipl;
    uint8_t flag;
};
static_assert(sizeof(PendingProp) == 0xC);
GAME_ARRAY(PendingProp, ms_pendingProps, 0x00BD3E78);

GAME_FN(int, __thiscall, Pool_GetSizeChained, 0x00435BB0, void* pool);
GAME_FN(uint8_t*, __thiscall, Pool_GetAtChained, 0x00435BD0, void* pool, int slot, int onlyUsed);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
inline int __cdecl StrICmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(void*, __cdecl, File_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(uint32_t, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, uint32_t size);
GAME_FN(uint32_t, __cdecl, File_Write, 0x0042D2D0, void* file, const void* buffer, uint32_t size);
GAME_FN(uint8_t, __cdecl, File_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(uint32_t, __cdecl, File_Tell, 0x0042D430, void* file);
GAME_FN(int, __cdecl, File_CloseFile, 0x0042D450, void* file);
GAME_FN(CStreamingInfo*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, uint32_t flags);
GAME_FN(void, __cdecl, LoadAllRequestedModels, 0x00533EE0, int);
GAME_FN(void, __cdecl, Fn_45B7B0, 0x0045B7B0);
GAME_FN(void, __cdecl, Fn_45B7D0, 0x0045B7D0);
GAME_FN(void, __cdecl, LoadingScreen, 0x0043C4F0, const char* title, const char* message, uintptr_t);
GAME_FN(uint8_t, __thiscall, Rect_IsPointInside, 0x00435AA0, const col::CRect* rect, const float* xy);
GAME_FN(uint8_t, __thiscall, Rect_IsPointInsideMargin, 0x00435AF0, const col::CRect* rect, const float* xy, float margin);

// the entities
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, uint8_t* entity);
GAME_FN(void, __cdecl, Projectile_Fn_4D3F10, 0x004D3F10, void*, int);
GAME_FN(uint8_t*, __cdecl, CreatePropForTrigger, 0x004EFFB0, void* trigger, int, int, int);
GAME_FN(int, __thiscall, Triggers_FindByHash, 0x006D79C0, void* triggers, uint32_t hash);
GAME_FN(void, __thiscall, ModelInfo_SetColModel, 0x0050EB30, void* mi, void* col, int owns);
GAME_FN(void, __cdecl, ModelInfo_Fn_51C600, 0x0051C600, int id);
GAME_FN(void, __cdecl, ModelInfo_Fn_51CB30, 0x0051CB30, int id, void* col);
GAME_FN(uint8_t, __thiscall, ModelInfo_Fn_520440, 0x00520440, void* mi);
// the per-file data the other stores keep
GAME_FN(void, __thiscall, Triggers_RemoveIpl, 0x006D78B0, void* triggers, int ipl);
GAME_FN(void, __thiscall, Points_RemoveIpl, 0x006D6370, void* points, int ipl);
GAME_FN(void, __thiscall, Fn_448A40, 0x00448A40, void*, int ipl);
GAME_FN(void, __thiscall, PatrolPaths_RemoveIpl, 0x005ED260, void* paths, int ipl);

// the quad tree
GAME_FN(void, __thiscall, QuadTree_ForAllMatching, 0x00576FE0, void* tree, const float* xy, void* callback);
GAME_FN(void, __thiscall, QuadTree_AddItem, 0x00577120, void* tree, void* item, const col::CRect* rect);
GAME_FN(void*, __thiscall, QuadTreeNodePool_New, 0x0056EDF0, void* pool);
GAME_FN(void*, __thiscall, QuadTreeNode_Construct, 0x00576FA0, void* node, const col::CRect* rect, int depth);

// the file reader
GAME_FN(void*, __thiscall, SectionLoaderImpl_Construct, 0x00438240, void* loader);
GAME_FN(uint8_t, __thiscall, IplLoader_Load, 0x004358D0, void* loader, IplStream* stream);
GAME_FN(void, __thiscall, PathLoader_Destruct, 0x005EC7B0, void* loader);
GAME_FN(uint8_t, __cdecl, LoadModelInfoFromReader, 0x004373F0, IplStream* stream, int ipl);
GAME_FN(void, __cdecl, Fn_4D0D10, 0x004D0D10);
inline void* const IplLoaderImpl_vftable = reinterpret_cast<void*>(0x0090373C);

// ours, through the game
GAME_FN(IplDef*, __thiscall, Game_PoolNew, 0x00436B10, CPool<IplDef>* pool);
GAME_FN(void, __thiscall, Game_PoolInit, 0x004396B0, CPool<IplDef>* pool);
GAME_FN(void, __thiscall, Game_QuadTreePoolInit, 0x00437BE0, CPool<uint8_t>* pool);
GAME_FN(void, __thiscall, Game_ArrayResize, 0x00439610, IplDefArray* array, uint32_t capacity, int setCount);
GAME_FN(int, __cdecl, Game_GetSize, 0x00436AF0);
GAME_FN(void, __cdecl, Game_IplDefInit, 0x00435A10, IplDef* def);
GAME_FN(int, __cdecl, Game_AddIplSlot, 0x00438440, const char* name);
GAME_FN(void, __cdecl, Game_IncludeEntity, 0x004384B0, int slot, uint8_t* entity);
GAME_FN(void, __cdecl, Game_RemoveIpb, 0x004377F0, int slot);
GAME_FN(uint8_t, __cdecl, Game_LoadIpb, 0x004390A0, int slot, uint8_t* data, int size, int flag);
GAME_FN(void, __cdecl, Game_CreateQuadTreeNodePool, 0x004387C0);
GAME_FN(void, __cdecl, Game_LoadStoreBin, 0x00439790);
GAME_FN(void, __cdecl, Game_SaveStoreBin, 0x004399F0);
GAME_FN(void, __cdecl, Game_MarkAreaRequired, 0x00436AB0, const float* pos, int area);
GAME_FN(void, __cdecl, Game_LoadNear, 0x00439B90, float x, float y, float z, int);
inline void* const MarkRequiredCallback = reinterpret_cast<void*>(0x00436A70);
inline void* const IplDefConstructor = reinterpret_cast<void*>(0x004387A0);
inline void* const QuadTreeNodeConstructor = reinterpret_cast<void*>(0x00436BE0);

// (the arrays' sizes: n * size, all ones when it overflows)
uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes);
}

// (inlined everywhere: a slot's def, from the array or the pool)
IplDef* GetDef(int slot) {
    if (ms_bLoadBoxes) {
        return CIplStore::ms_array.data + slot;
    }
    CPool<IplDef>* pool = CIplStore::ms_pPool;
    if (pool->flags[slot] & 0x80) {
        return nullptr;
    }
    return reinterpret_cast<IplDef*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
}

int SlotOf(const CPool<uint8_t>* pool, const uint8_t* entity) {
    return int(entity - pool->objects) / pool->elemSize;
}

// the entity at a slot, on into the chained pools (without the used check the chain's lookup makes)
uint8_t* ChainedAt(CPool<uint8_t>* pool, int slot) {
    if (slot < pool->size) {
        return pool->objects + pool->elemSize * slot;
    }
    if (pool->Next()) {
        return Pool_GetAtChained(pool->Next(), slot - pool->size, 1);
    }
    return nullptr;
}

// out of the world, and deleted (the virtual deleting destructor)
void DeleteEntity(uint8_t* entity) {
    World_Remove(entity);
    (*reinterpret_cast<re::Ptr32<re::Ptr32<void __thiscall(void*, int)>>*>(entity))[0](entity, 1);
}

// the IplDef's own init, inlined in LoadStoreBin
void InitDef(IplDef* def) {
    def->firstBuilding = 0x7FFF;
    def->firstDummy = 0x7FFF;
    def->firstPropAnim = 0x7FFF;
    def->firstObject = 0x7FFF;
    def->firstProjectile = 0x7FFF;
    def->loaded = 0;
    def->required = 0;
    def->ignore = 1;
    def->ignoreWhenDeleted = 0;
    def->unknown56 = 0;
    def->unknown5B = 0;
    def->unknown54 = -1;
    def->data = nullptr;
    def->dataSize = 0;
    def->lastBuilding = -0x8000;
    def->lastDummy = -0x8000;
    def->lastPropAnim = -0x8000;
    def->lastObject = -0x8000;
    def->lastProjectile = -0x8000;
    def->filePos = 0;
    for (int32_t& v : def->unknown64) {
        v = 0;
    }
}

void ClearRanges(IplDef* def) {
    def->lastBuilding = -0x8000;
    def->lastDummy = -0x8000;
    def->lastPropAnim = -0x8000;
    def->lastObject = -0x8000;
    def->lastProjectile = -0x8000;
    def->firstBuilding = 0x7FFF;
    def->firstDummy = 0x7FFF;
    def->firstPropAnim = 0x7FFF;
    def->firstObject = 0x7FFF;
    def->firstProjectile = 0x7FFF;
    def->filePos = 0;
    for (int32_t& v : def->unknown64) {
        v = 0;
    }
}

void Widen(int16_t& first, int16_t& last, int slot) {
    if (slot < first) {
        first = int16_t(slot);
    }
    if (slot > last) {
        last = int16_t(slot);
    }
}

// the streaming keeps it (flags 1|2: a mission's or the game's)
bool Kept(int slot) {
    return (GetStreamingInfo(slot + IPB_ID_BASE)->flags & 3) != 0;
}

// the other stores' data from the file (triggers, points, POIs, patrol paths)
void RemoveExtras(int slot) {
    Triggers_RemoveIpl(TheTriggers, slot);
    Points_RemoveIpl(reinterpret_cast<void*>(0x00BC5E7C), slot);
    Fn_448A40(ms_pObj_C0F5E4, slot);
    PatrolPaths_RemoveIpl(ms_pPatrolPaths, slot);
}

// IplLoaderImpl (on the stack of LoadIpb): the reader's object for one file
struct IplLoader {
    alignas(4) uint8_t bytes[0x1A0];
    void Construct(IplDef* def, int slot, uint8_t flag, uint8_t firstRead) {
        *reinterpret_cast<re::Ptr32<void>*>(bytes) = IplLoaderImpl_vftable;
        *reinterpret_cast<int32_t*>(bytes + 0xC) = slot;
        bytes[0x10] = firstRead;
        SectionLoaderImpl_Construct(bytes + 0x14);
        *reinterpret_cast<re::Ptr32<IplDef>*>(bytes + 4) = def;
        bytes[8] = flag;
    }
    void Destruct() { PathLoader_Destruct(bytes + 0x130); }
};

} // namespace

// ---- the slots ---------------------------------------------------------------------------

namespace CIplStore {

IplDef* __fastcall IplDef_Construct(IplDef* def, re::Edx) {
    def->bounds.left = 1000000.0f;
    def->bounds.bottom = 1000000.0f;
    def->bounds.right = -1000000.0f;
    def->bounds.top = -1000000.0f;
    return def;
}

void __cdecl IplDef_Init(IplDef* def) {
    InitDef(def);
}

int __cdecl GetSize() {
    return ms_bLoadBoxes ? int(ms_array.count) : 100;
}

IplDef* __fastcall Pool_New(CPool<IplDef>* pool, re::Edx) {
    bool wrapped = false;
    if (!(pool->flags[pool->lastUsed] & 0x80)) {
        do {
            if (++pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while (!(pool->flags[pool->lastUsed] & 0x80));
    }
    pool->flags[pool->lastUsed] &= 0x7F;
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag ^= ((flag + 1) ^ flag) & 0x7F;   // (the reuse count up by one)
    IplDef* def = reinterpret_cast<IplDef*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * pool->lastUsed);
    std::memset(def, 0, 0x78);
    return def;
}

static void ClearFlags(CPool<uint8_t>* pool) {
    pool->flags = static_cast<uint8_t*>(Malloc(size_t(pool->size)));
    pool->lastUsed = 0;
    for (int i = 0; i < pool->size; i++) {
        pool->flags[i] |= 0x80;
        pool->flags[i] &= 0x80;
    }
}

void __fastcall Pool_Init(CPool<IplDef>* pool, re::Edx) {
    pool->ownsArrays = 1;
    pool->elemSize = 0x78;
    if (pool->unknown19) {
        // (made with their constructor; no count before them, unlike the node pool's)
        int count = pool->size;
        void* objects = Malloc(ArrayBytes(uint32_t(count), 0x78));
        if (objects) {
            ConstructArray(objects, 0x78, count, IplDefConstructor);
        }
        pool->objects = static_cast<IplDef*>(objects);
    } else {
        pool->objects = static_cast<IplDef*>(Malloc(size_t(pool->size) * 0x78));
    }
    ClearFlags(reinterpret_cast<CPool<uint8_t>*>(pool));
}

void __fastcall QuadTreePool_Init(CPool<uint8_t>* pool, re::Edx) {
    pool->ownsArrays = 1;
    pool->elemSize = 0x30;
    if (pool->unknown19) {
        // (new[]: the count before them)
        int count = pool->size;
        uint32_t bytes = ArrayBytes(uint32_t(count), 0x30);
        bytes = bytes + 4 < bytes ? 0xFFFFFFFFu : bytes + 4;
        uint32_t* memory = static_cast<uint32_t*>(Malloc(bytes));
        if (memory) {
            *memory = uint32_t(count);
            ConstructArray(memory + 1, 0x30, count, QuadTreeNodeConstructor);
            pool->objects = reinterpret_cast<uint8_t*>(memory + 1);
        } else {
            pool->objects = nullptr;
        }
    } else {
        pool->objects = static_cast<uint8_t*>(Malloc(size_t(pool->size) * 0x30));
    }
    ClearFlags(pool);
}

int __cdecl FindIplSlot(const char* name) {
    for (int slot = 0; slot < Game_GetSize(); slot++) {
        IplDef* def = GetDef(slot);
        if (def && StrICmp(name, def->name) == 0) {
            return slot;
        }
    }
    return -1;
}

int __cdecl FindIplSlotByHash(uint32_t hash) {
    int size = Game_GetSize();   // (once)
    for (int slot = 0; slot < size; slot++) {
        IplDef* def = GetDef(slot);
        if (def && hash == def->nameHash) {
            return slot;
        }
    }
    return -1;
}

const char* __cdecl GetIplName(int slot) {
    if (ms_bLoadBoxes) {
        return ms_array.data[slot].name;
    }
    CPool<IplDef>* pool = ms_pPool;
    if (pool->flags[slot] & 0x80) {
        return reinterpret_cast<const char*>(uintptr_t(0x10));   // (a free slot's "name": nullptr + 0x10)
    }
    return reinterpret_cast<IplDef*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot)->name;
}

int __cdecl AddIplSlot(const char* name) {
    IplDef* def = Game_PoolNew(ms_pPool);
    def->haveBounds = 0;
    std::strcpy(def->name, name);   // (no limit: the game's)
    def->nameHash = HashName(name);
    def->bounds.left = 1000000.0f;
    def->bounds.bottom = 1000000.0f;
    def->area = -1;
    def->bounds.right = -1000000.0f;
    def->bounds.top = -1000000.0f;
    Game_IplDefInit(def);
    return int(reinterpret_cast<uint8_t*>(def) - re::PtrCast<uint8_t*>(ms_pPool->objects)) / ms_pPool->elemSize;
}

void __cdecl IncludeEntity(int slot, uint8_t* entity) {
    IplDef* def = GetDef(slot);
    uint8_t type = entity[0x108] & 7;
    if (type == 1) {
        Widen(def->firstBuilding, def->lastBuilding, SlotOf(ms_pBuildingPool, entity));
    } else if (type == 5) {
        Widen(def->firstDummy, def->lastDummy, SlotOf(ms_pDummyPool, entity));
    } else if (type == 6) {
        Widen(def->firstPropAnim, def->lastPropAnim, SlotOf(ms_pPropAnimPool, entity));
    } else if (*reinterpret_cast<int32_t*>(entity + 0xC4) == 0) {
        if (type == 4) {
            Widen(def->firstObject, def->lastObject, SlotOf(ms_pObjectPool, entity));
        }
    } else {
        // (anything else with +0xC4 set counts as a projectile)
        Widen(def->firstProjectile, def->lastProjectile, SlotOf(ms_pProjectilePool, entity));
    }
}

void __cdecl RemoveIpb(int slot) {
    IplDef* def = GetDef(slot);
    def->loaded = 0;
    for (int i = def->firstBuilding; i <= def->lastBuilding; i++) {
        CPool<uint8_t>* pool = ms_pBuildingPool;
        if (!(pool->flags[i] & 0x80)) {
            uint8_t* entity = pool->objects + pool->elemSize * i;
            if (entity && entity[0x110] == slot) {
                DeleteEntity(entity);
            }
        }
    }
    // (the objects of every file are looked at, not only this one's range)
    for (int i = 0; i < Pool_GetSizeChained(ms_pObjectPool); i++) {
        CPool<uint8_t>* pool = ms_pObjectPool;
        uint8_t* entity = ChainedAt(pool, i);
        if (!(pool->flags[i] & 0x80) && entity && entity[0x110] == slot) {
            DeleteEntity(entity);
        }
    }
    for (int i = def->firstDummy; i <= def->lastDummy; i++) {
        CPool<uint8_t>* pool = ms_pDummyPool;
        if (!(pool->flags[i] & 0x80)) {
            uint8_t* entity = pool->objects + pool->elemSize * i;
            if (entity && entity[0x110] == slot) {
                DeleteEntity(entity);
            }
        }
    }
    for (int i = def->firstPropAnim; i <= def->lastPropAnim; i++) {
        CPool<uint8_t>* pool = ms_pPropAnimPool;
        if (!(pool->flags[i] & 0x80)) {
            uint8_t* entity = pool->objects + pool->elemSize * i;
            if (entity && entity[0x110] == slot) {
                // the player's in it (a bike, ...): the file stays, flagged loaded
                if (*reinterpret_cast<int32_t*>(FindPlayerPed_ptr + 0x13DC) &&
                    *reinterpret_cast<int16_t*>(entity + 0x10E) == ms_playerVehicleModel_BF3E58) {
                    def->loaded = 1;
                    return;
                }
                World_Remove(entity);
                if (uint8_t* owner = *reinterpret_cast<re::Ptr32<uint8_t>*>(entity + 0x23C)) {
                    *reinterpret_cast<int32_t*>(owner + 0x1C) = 0;
                }
                (*reinterpret_cast<re::Ptr32<re::Ptr32<void __thiscall(void*, int)>>*>(entity))[0](entity, 1);
            }
        }
    }
    for (int i = def->firstObject; i <= def->lastObject; i++) {
        CPool<uint8_t>* pool = ms_pObjectPool;
        uint8_t* entity = ChainedAt(pool, i);
        if (!(pool->flags[i] & 0x80) && entity && entity[0x110] == slot) {
            DeleteEntity(entity);
        }
    }
    for (int i = def->firstProjectile; i <= def->lastProjectile; i++) {
        CPool<uint8_t>* pool = ms_pProjectilePool;
        uint8_t* entity = ChainedAt(pool, i);
        if (!(pool->flags[i] & 0x80) && entity && entity[0x110] == slot) {
            Projectile_Fn_4D3F10(*reinterpret_cast<re::Ptr32<void>*>(entity + 0x238), 0);
        }
    }
    RemoveExtras(slot);
    if (ms_bKeepModelsForIpls && slot != 0) {
        // the models only this file uses: their collision given back
        for (int id = 1; id < 12200; id++) {
            uint8_t* mi = static_cast<uint8_t*>(ms_modelInfoPtrs[id]);
            if (!mi || mi[0xA] != slot) {
                continue;
            }
            void* col = *reinterpret_cast<re::Ptr32<void>*>(mi + 0xC);
            void* kept = nullptr;
            if (col) {
                ModelInfo_SetColModel(mi, nullptr, 0);
                kept = col;
            }
            ModelInfo_Fn_51C600(id);
            if (kept) {
                ModelInfo_Fn_51CB30(id, kept);
            }
        }
    }
    ClearRanges(def);
}

void __cdecl RemoveIpbsNotKept() {
    for (int slot = 1; slot < Game_GetSize(); slot++) {
        IplDef* def = GetDef(slot);
        if (def && GetDef(slot)->loaded && !Kept(slot)) {
            Game_RemoveIpb(slot);
        }
    }
}

void __cdecl RemoveIpbExtrasNotKept() {
    for (int slot = 1; slot < Game_GetSize(); slot++) {
        IplDef* def = GetDef(slot);
        if (def && GetDef(slot)->loaded && !Kept(slot) && GetDef(slot)) {
            RemoveExtras(slot);
        }
    }
}

void __cdecl CreateQuadTreeNodePool() {
    if (ms_pQuadTreeNodePool) {
        return;
    }
    CPool<uint8_t>* pool = static_cast<CPool<uint8_t>*>(Malloc(0x1C));
    if (!pool) {
        ms_pQuadTreeNodePool = nullptr;
        return;
    }
    pool->unknown10 = 0;
    pool->objects = nullptr;
    pool->flags = nullptr;
    pool->lastUsed = 0;
    pool->ownsArrays = 1;
    pool->size = 0x100;
    pool->unknown19 = 0;
    Game_QuadTreePoolInit(pool);
    ms_pQuadTreeNodePool = pool;
}

void __cdecl CreatePendingProps() {
    for (int i = 0; i < ms_numPendingProps; i++) {
        PendingProp& prop = ms_pendingProps[i];
        if (prop.triggerHash == 0xFFFFFFFF) {
            continue;
        }
        int index = Triggers_FindByHash(TheTriggers, prop.triggerHash);
        if (index == -1) {
            continue;
        }
        CPool<uint8_t>* pool = ms_pTriggerPool;
        if (pool->flags[index] & 0x80) {
            continue;
        }
        uint8_t* trigger = pool->objects + pool->elemSize * index;
        if (!trigger) {
            continue;
        }
        uint8_t* entity = CreatePropForTrigger(trigger, prop.flag, -1, 1);
        if (entity != nullptr) {   // (none when the PropAnim pool is full: PropAnimSpawn.cpp)
            entity[0x110] = uint8_t(prop.ipl);
            Game_IncludeEntity(prop.ipl, entity);
        }
        prop.triggerHash = 0xFFFFFFFF;
        prop.ipl = -1;
        prop.flag = 0;
    }
}

uint8_t __cdecl LoadIpb(int slot, uint8_t* data, int size, int flag) {
    PushMemoryTag(0x1E);
    IplDef* def = GetDef(slot);
    Sprintf(ms_debugName, "%s [%d]", def->name, slot);
    IplStream stream;
    IplLoader loader;
    if (def->bounds.right < def->bounds.left || def->bounds.top < def->bounds.bottom) {
        // the first read: all of it, for the bounds; the file kept
        // (stream.file is never set: the game's reads it uninitialised when there's no data)
        stream.file = nullptr;
        stream.data = data;
        stream.position = 0;
        stream.size = uint32_t(size);
        loader.Construct(def, slot, 1, 1);
        IplLoader_Load(loader.bytes, &stream);
        QuadTree_AddItem(ms_pQuadTree, def, &def->bounds);
        def->haveBounds = 1;
        def->ignore = 0;
        PushMemoryTag(0x18);
        def->data = static_cast<uint8_t*>(Malloc(size_t(size)));
        def->dataSize = size;
        PopMemoryTag();
        std::memcpy(def->data, data, size_t(size));
        PopMemoryTag();
        loader.Destruct();
        if (!stream.data) {
            File_CloseFile(stream.file);
        }
        return 1;
    }
    bool partial = def->filePos != 0;
    if (data && size > 0) {
        def->dataSize = size;
        std::memcpy(def->data, data, size_t(size));
    }
    stream.file = nullptr;   // (uninitialised in the game; only used without data)
    stream.data = def->data;
    stream.size = uint32_t(def->dataSize);
    stream.position = 0;
    if (!stream.data) {
        File_Seek(stream.file, def->filePos, 0);
    } else {
        stream.position = uint32_t(def->filePos);
    }
    if (ms_bUseIpc) {
        ms_bFirstLoad_BF3817 = def->firstLoad;
        if (!partial) {
            LoadModelInfoFromReader(&stream, slot);
            Fn_4D0D10();
        }
    }
    uint8_t loaderFlag;
    if (def->firstLoad) {
        loaderFlag = 1;
        def->firstLoad = 0;
    } else {
        loaderFlag = uint8_t(flag);
    }
    loader.Construct(def, slot, loaderFlag, 0);
    if (!IplLoader_Load(loader.bytes, &stream)) {
        def->filePos = stream.data ? int32_t(stream.position) : int32_t(File_Tell(stream.file));
        PopMemoryTag();
        loader.Destruct();
        if (!stream.data) {
            File_CloseFile(stream.file);
        }
        return 0;
    }
    def->loaded = 1;
    PopMemoryTag();
    loader.Destruct();
    if (!stream.data) {
        File_CloseFile(stream.file);
    }
    return 1;
}

void __cdecl MarkRequired(const float* xy, IplDef* def) {
    if (ms_scanArea == 0) {
        if (Rect_IsPointInside(&def->bounds, xy) && def->area == 0) {
            def->required = 1;
        }
    } else if (def->area == ms_scanArea) {
        def->required = 1;
    }
}

void __cdecl MarkAreaRequired(const float* pos, int area) {
    ms_scanArea = area == -1 ? currArea : area;
    float xy[2] = {pos[0], pos[1]};
    QuadTree_ForAllMatching(ms_pQuadTree, xy, MarkRequiredCallback);
}

void __cdecl LoadIpls(const float* pos, int removeFar) {
    if (ms_bStreamingDisabled_CA3984) {
        return;
    }
    ms_scanArea = currArea;
    float xy[2] = {pos[0], pos[1]};
    QuadTree_ForAllMatching(ms_pQuadTree, xy, MarkRequiredCallback);
    if (ms_bExtraPoint) {
        col::CVector extra = ms_extraPoint;
        Game_MarkAreaRequired(&extra.x, ms_extraPointArea);
    }
    if (re::Flag(removeFar)) {
        for (int slot = 1; slot < Game_GetSize(); slot++) {
            IplDef* def = GetDef(slot);
            if (def && def->loaded && !def->required && !Kept(slot)) {
                Game_RemoveIpb(slot);
            }
        }
    }
    for (int slot = 1; slot < Game_GetSize(); slot++) {
        IplDef* def = GetDef(slot);
        if (!def || def->ignore || !def->required) {
            continue;
        }
        float at[2] = {pos[0], pos[1]};
        if (Rect_IsPointInsideMargin(&def->bounds, at, ms_iplLoadMargin) && (!def->data || !def->dataSize)) {
            // close and not in memory: now
            RequestModel(slot + IPB_ID_BASE, 0xC);
            Fn_45B7B0();
            LoadingScreen("CIplStoreI::EnsureIplsAreInMemory()", "CStreaming::LoadAllRequestedModels()", 0);
            LoadAllRequestedModels(1);
            Fn_45B7D0();
        } else if (!def->loaded) {
            Game_LoadIpb(slot, nullptr, 0, 0);
        }
        def->required = 0;
    }
}

void __cdecl ReloadAllIpbs() {
    for (int slot = 1; slot < Game_GetSize(); slot++) {
        IplDef* def = GetDef(slot);
        if (def && def->loaded) {
            Game_RemoveIpb(slot);
            Game_LoadIpb(slot, nullptr, 0, 0);
        }
    }
}

void __cdecl LoadStoreBin() {
    void* file = File_OpenFile("Config\\Extra\\IplStore.bin", "r", 1);
    uint32_t count;
    uint32_t entrySize;
    File_Read(file, &count, 4);
    File_Read(file, &entrySize, 4);
    if (count > ms_array.capacity) {
        Game_ArrayResize(&ms_array, count, 1);
    }
    for (int slot = 0; slot < int(count); slot++) {
        IplDef* def = GetDef(slot);
        InitDef(def);
        File_Read(file, def, entrySize);
        QuadTree_AddItem(ms_pQuadTree, def, &def->bounds);
        def->firstLoad = 1;
        def->ignore = 0;
        def->haveBounds = 1;
    }
    File_CloseFile(file);
}

void __cdecl Initialise() {
    PushMemoryTag(0x1E);
    col::CRect world{-900.0f, 900.0f, 900.0f, -900.0f};
    Game_CreateQuadTreeNodePool();
    void* node = QuadTreeNodePool_New(ms_pQuadTreeNodePool);
    ms_pQuadTree = node ? QuadTreeNode_Construct(node, &world, 3) : nullptr;
    if (!ms_bLoadBoxes) {
        if (!ms_pPool) {
            CPool<IplDef>* pool = static_cast<CPool<IplDef>*>(Malloc(0x1C));
            if (pool) {
                pool->unknown10 = 0;
                pool->objects = nullptr;
                pool->flags = nullptr;
                pool->lastUsed = 0;
                pool->ownsArrays = 1;
                pool->size = 100;
                pool->unknown19 = 0;
                Game_PoolInit(pool);
            }
            ms_pPool = pool;
        }
        Game_AddIplSlot("generic");
    } else {
        Game_LoadStoreBin();
    }
    ms_unknown_BD5A9C = 0;
    ms_unknown_BD5A98 = 0;
    PopMemoryTag();
}

void __cdecl SaveStoreBin() {
    if (!ms_bSaveBoxes) {
        return;
    }
    void* file = File_OpenFile("Config\\Extra\\IplStore.bin", "w", 1);
    int count = ms_bLoadBoxes ? int(ms_array.count) : 100;
    uint32_t entrySize = 0x40;   // (the bounds, the name, the hash and the area)
    int written = 0;
    IplDefArray copy{nullptr, 0, 0};
    if (count != 0) {
        Game_ArrayResize(&copy, uint32_t(count), 1);
    }
    for (int slot = 0; slot < count; slot++) {
        if (GetDef(slot)) {
            std::memcpy(&copy.data[written], GetDef(slot), 0x78);
            written++;
        }
    }
    File_Write(file, &written, 4);
    File_Write(file, &entrySize, 4);
    for (int i = 0; i < written; i++) {
        File_Write(file, &copy.data[i], entrySize);
    }
    File_CloseFile(file);
    if (copy.data) {
        Free(copy.data);
    }
}

void __cdecl LoadNear(float x, float y, float, int) {
    if (ms_bSaveStoreBin) {
        Game_SaveStoreBin();
    }
    float xy[2] = {x, y};
    ms_scanArea = currArea;
    QuadTree_ForAllMatching(ms_pQuadTree, xy, MarkRequiredCallback);
    if (ms_bExtraPoint) {
        int area = ms_extraPointArea;
        ms_scanArea = area == -1 ? currArea : area;
        float extra[2] = {ms_extraPoint.x, ms_extraPoint.y};
        QuadTree_ForAllMatching(ms_pQuadTree, extra, MarkRequiredCallback);
    }
    for (int slot = 1; slot < Game_GetSize(); slot++) {
        if (!GetDef(slot)) {
            continue;
        }
        IplDef* def = GetDef(slot);
        if (def->ignore) {
            continue;
        }
        if (def->required) {
            if (!def->loaded) {
                Game_LoadIpb(slot, nullptr, 0, 0);
            }
            def->required = 0;
        } else if (def->loaded) {
            Game_RemoveIpb(slot);
            if (def->ignoreWhenDeleted) {
                def->ignore = 1;
            }
        }
    }
}

void __cdecl LoadNearVector(col::CVector pos, int flag) {
    Game_LoadNear(pos.x, pos.y, pos.z, flag);
}

} // namespace CIplStore

void IplDefArray::Resize(uint32_t newCapacity, int setCount) {
    IplDef* defs = nullptr;
    capacity = newCapacity;
    if (newCapacity != 0) {
        defs = static_cast<IplDef*>(Malloc(ArrayBytes(newCapacity, 0x78)));
        if (defs) {
            ConstructArray(defs, 0x78, int(newCapacity), IplDefConstructor);
        } else {
            defs = nullptr;
        }
    }
    if (data) {
        for (uint32_t i = 0; i < count; i++) {
            defs[i] = data[i];
        }
        Free(data);
    }
    data = defs;
    if (re::Flag(setCount)) {
        count = capacity;
    }
}

// ---- the reader ------------------------------------------------------------------------

// 0x4358D0 (thiscall, IplLoaderImpl's Load): the section count (after a 0x13-byte header),
// then each section through its loader (vf2), 0x435290 reading it; stops where a section
// isn't complete (the def keeps where: +0x70 the section, +0x74 the count), false then.
// The def's +0x64 set: go on from there instead of the header.
static uint8_t __fastcall IplLoader_LoadImpl(uint8_t* loader, re::Edx, IplStream* stream);

namespace {
GAME_FN(uint8_t, __thiscall, Section_Load, 0x00435290, void* section, IplStream* stream);
using LoaderVf0 = void(__thiscall*)(void*);
using LoaderVf1 = void(__thiscall*)(void*, uint32_t);
using LoaderVf2 = void*(__thiscall*)(void*, uint32_t);
using LoaderVf3 = void(__thiscall*)(void*, uint32_t, void*);
re::Ptr32<void>* Vtable(void* object) {
    return *static_cast<re::Ptr32<re::Ptr32<void>>*>(object);
}
} // namespace

static uint8_t __fastcall IplLoader_LoadImpl(uint8_t* loader, re::Edx, IplStream* stream) {
    uint8_t* def = *reinterpret_cast<re::Ptr32<uint8_t>*>(loader + 4);
    uint32_t first;
    uint32_t count;
    if (*reinterpret_cast<int32_t*>(def + 0x64) == 0) {
        char header[0x13];
        if (!stream->data) {
            File_Read(stream->file, header, 0x13);
        } else {
            stream->position += 0x13;
        }
        if (!stream->data) {
            File_Read(stream->file, &count, 4);
        } else {
            std::memcpy(&count, stream->data + stream->position, 4);
            stream->position += 4;
        }
        re::PtrCast<LoaderVf0>(Vtable(loader)[0])(loader);
        re::PtrCast<LoaderVf1>(Vtable(loader)[1])(loader, count);
        first = 0;
    } else {
        first = *reinterpret_cast<uint32_t*>(def + 0x70);
        count = *reinterpret_cast<uint32_t*>(def + 0x74);
    }
    for (uint32_t i = first; i < count; i++) {
        void* section = re::PtrCast<LoaderVf2>(Vtable(loader)[2])(loader, i);
        if (!Section_Load(section, stream)) {
            uint8_t* current = *reinterpret_cast<re::Ptr32<uint8_t>*>(loader + 4);
            *reinterpret_cast<uint32_t*>(current + 0x70) = i;
            *reinterpret_cast<uint32_t*>(current + 0x74) = count;
            return 0;
        }
        re::PtrCast<LoaderVf3>(Vtable(loader)[3])(loader, i, section);
    }
    re::PtrCast<LoaderVf0>(Vtable(loader)[4])(loader);
    re::PtrCast<LoaderVf0>(Vtable(loader)[5])(loader);
    return 1;
}

// ---- registration ----------------------------------------------------------------------

#define IPL_REVERSED(name, fn, address)                                                                 static void* Original_##fn;                                                                         REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace CIplStore {
IPL_REVERSED("IplDef::IplDef", IplDef_Construct, 0x004387A0);
IPL_REVERSED("CIplStore::IplDefInit", IplDef_Init, 0x00435A10);
IPL_REVERSED("CIplStore::GetSize", GetSize, 0x00436AF0);
IPL_REVERSED("CIplStore::PoolNew", Pool_New, 0x00436B10);
IPL_REVERSED("CIplStore::PoolInit", Pool_Init, 0x004396B0);
IPL_REVERSED("CIplStore::QuadTreePoolInit", QuadTreePool_Init, 0x00437BE0);
IPL_REVERSED("CIplStore::FindIplSlot", FindIplSlot, 0x00437130);
IPL_REVERSED("CIplStore::FindIplSlotByHash", FindIplSlotByHash, 0x004371B0);
IPL_REVERSED("CIplStore::GetIplName", GetIplName, 0x00437CB0);
IPL_REVERSED("CIplStore::AddIplSlot", AddIplSlot, 0x00438440);
IPL_REVERSED("CIplStore::IncludeEntity", IncludeEntity, 0x004384B0);
IPL_REVERSED("CIplStore::RemoveIpb", RemoveIpb, 0x004377F0);
IPL_REVERSED("CIplStore::RemoveIpbsNotKept", RemoveIpbsNotKept, 0x004385E0);
IPL_REVERSED("CIplStore::RemoveIpbExtrasNotKept", RemoveIpbExtrasNotKept, 0x00438690);
IPL_REVERSED("CIplStore::CreateQuadTreeNodePool", CreateQuadTreeNodePool, 0x004387C0);
IPL_REVERSED("CIplStore::CreatePendingProps", CreatePendingProps, 0x00438850);
IPL_REVERSED("CIplStore::LoadIpb", LoadIpb, 0x004390A0);
IPL_REVERSED("CIplStore::LoadIpls", LoadIpls, 0x00439370);
IPL_REVERSED("CIplStore::ReloadAllIpbs", ReloadAllIpbs, 0x00439590);
IPL_REVERSED("CIplStore::LoadStoreBin", LoadStoreBin, 0x00439790);
IPL_REVERSED("CIplStore::Initialise", Initialise, 0x004398F0);
IPL_REVERSED("CIplStore::SaveStoreBin", SaveStoreBin, 0x004399F0);
IPL_REVERSED("CIplStore::LoadNear", LoadNear, 0x00439B90);
IPL_REVERSED("CIplStore::LoadNearVector", LoadNearVector, 0x00439CF0);
IPL_REVERSED("CIplStore::MarkRequired", MarkRequired, 0x00436A70);
IPL_REVERSED("CIplStore::MarkAreaRequired", MarkAreaRequired, 0x00436AB0);
} // namespace CIplStore
IPL_REVERSED("IplLoaderImpl::Load", IplLoader_LoadImpl, 0x004358D0);
static void* Original_Resize;
REVERSED("IplDefArray::Resize", 0x00439610, re::MemberAddress(&IplDefArray::Resize), nullptr, &Original_Resize,
         re::Mode::Original);
