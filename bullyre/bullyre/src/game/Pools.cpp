#include "game/Pools.h"

#include <cstdio>
#include <cstring>
#include "reversed.h"

// Pools of every type share their code: the game has a copy of each CPool method per
// element type, differing only in the element size, what New zeroes, and how the
// objects are constructed and destroyed. Ours are templates, one instantiation
// registered per game copy. (thiscalls as __fastcall with the unused edx.)

namespace {

using AnyPool = CPool<uint8_t>;

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
// the compiler's "eh vector constructor iterator" (what new T[n] calls)
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void(__thiscall* ctor)(void*));
// fprintf to a debug file
GAME_FN(void, __cdecl, DebugFilePrintf, 0x0042D460, void* file, const char* format, ...);
// (looks like CWorld::Remove)
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
// what SFXItem's delete tells first (thiscall on the list at 0xCF6910)
GAME_FN(void, __thiscall, SfxList_Remove, 0x0059D1D0, void* list, void* item);
inline void* const sfxList = reinterpret_cast<void*>(0x00CF6910);

// the pools' own code, called where modes apply
GAME_FN(int, __thiscall, Pool_GetJustIndex, 0x0044A730, void* pool, void* object);
GAME_FN(void, __thiscall, Pool_SetNext, 0x004498C0, void* pool, void* next);
GAME_FN(int, __thiscall, Pool_GetSizeChained, 0x00435BB0, void* pool);
GAME_FN(void*, __thiscall, Pool_GetAtChained, 0x00435BD0, void* pool, int slot, int onlyUsed);

// each pool's Init (thiscall, on a pool with its size set)
GAME_FN(void, __thiscall, PtrNodePool_Init, 0x0044A760, void* pool);
GAME_FN(void, __thiscall, EntryInfoNodePool_Init, 0x0044A850, void* pool);
GAME_FN(void, __thiscall, PedPool_Init, 0x0044A8D0, void* pool);
GAME_FN(void, __thiscall, ObjectPool_Init, 0x0044A990, void* pool);
GAME_FN(void, __thiscall, ProjectilePool_Init, 0x0044AA50, void* pool);
GAME_FN(void, __thiscall, CutScenePool_Init, 0x0044AB10, void* pool);
GAME_FN(void, __thiscall, DummyPool_Init, 0x0044ABD0, void* pool);
GAME_FN(void, __thiscall, BuildingPool_Init, 0x0044AC90, void* pool);
GAME_FN(void, __thiscall, TreadablePool_Init, 0x0044AD50, void* pool);
GAME_FN(void, __thiscall, AccessoryPool_Init, 0x0044AE10, void* pool);
GAME_FN(void, __thiscall, ColModelPool_Init, 0x0044AED0, void* pool);
GAME_FN(void, __thiscall, StimulusPool_Init, 0x0044B000, void* pool);
GAME_FN(void, __thiscall, EntityEffectKeeperPool_Init, 0x0044B130, void* pool);
GAME_FN(void, __thiscall, AccessoryContainerPool_Init, 0x0044B260, void* pool);
GAME_FN(void, __thiscall, AttitudeSetPool_Init, 0x0044B390, void* pool);
GAME_FN(void, __thiscall, PedActionTreePool_Init, 0x0044B440, void* pool);
GAME_FN(void, __thiscall, TargetingSystemPool_Init, 0x0044B590, void* pool);
GAME_FN(void, __thiscall, WeaponAccessoryContainerPool_Init, 0x0044B650, void* pool);
GAME_FN(void, __thiscall, MotionControllerPool_Init, 0x0044B780, void* pool);
GAME_FN(void, __thiscall, JointConstraintPool_Init, 0x0044B8B0, void* pool);
GAME_FN(void, __thiscall, LuaScriptPool_Init, 0x0044B960, void* pool);
GAME_FN(void, __thiscall, FloorMotionDriverPool_Init, 0x0044BAB0, void* pool);
GAME_FN(void, __thiscall, ReachDriverPool_Init, 0x0044BB60, void* pool);
GAME_FN(void, __thiscall, WeaponItemPool_Init, 0x0044BC10, void* pool);
GAME_FN(void, __thiscall, SFXItemPool_Init, 0x0044BCC0, void* pool);
GAME_FN(void, __thiscall, Pool624_Init, 0x0044BD70, void* pool);
GAME_FN(void, __thiscall, VehiclePool_Init, 0x0044CB90, void* pool);
GAME_FN(void, __thiscall, PropAnimPool_Init, 0x0044CFD0, void* pool);

// the Flushes (thiscall)
GAME_FN(void, __thiscall, Pool_Flush, 0x006D2370, void* pool);              // plain arrays
GAME_FN(void, __thiscall, PtrNodePool_Flush, 0x0044A800, void* pool);
GAME_FN(void, __thiscall, EntityPool_Flush, 0x004498F0, void* pool);        // virtual destructors (slot 0)
GAME_FN(void, __thiscall, ConstraintPool_Flush, 0x00449960, void* pool);    // virtual destructors (slot 1)
GAME_FN(void, __thiscall, ColModelPool_Flush, 0x0044AF80, void* pool);
GAME_FN(void, __thiscall, StimulusPool_Flush, 0x0044B0B0, void* pool);
GAME_FN(void, __thiscall, EntityEffectKeeperPool_Flush, 0x0044B1E0, void* pool);
GAME_FN(void, __thiscall, AccessoryContainerPool_Flush, 0x0044B310, void* pool);
GAME_FN(void, __thiscall, PedActionTreePool_Flush, 0x0044B500, void* pool);
GAME_FN(void, __thiscall, WeaponAccessoryContainerPool_Flush, 0x0044B700, void* pool);
GAME_FN(void, __thiscall, MotionControllerPool_Flush, 0x0044B830, void* pool);
GAME_FN(void, __thiscall, LuaScriptPool_Flush, 0x0044BA20, void* pool);
GAME_FN(void, __thiscall, Pool624_Flush, 0x0044BE20, void* pool);

uint32_t SaturatingMul(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return bytes >> 32 ? 0xFFFFFFFF : uint32_t(bytes);
}

template <typename T>
AnyPool* Any(CPool<T>* pool) {
    return reinterpret_cast<AnyPool*>(pool);
}

// the slot's element, free or not
template <typename T>
T* At(CPool<T>* pool, int slot) {
    return re::PtrCast<T*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
}

// (the game's (object - objects) / elemSize, signed)
template <typename T>
int SlotOf(CPool<T>* pool, const void* object) {
    return int32_t(re::PtrCast<uintptr_t>(object) - re::PtrCast<uintptr_t>(pool->objects)) / pool->elemSize;
}

// a handle's element: its slot's flag byte must be the handle's low byte (a free
// slot's never is, having 0x80)
template <typename T>
T* FromRef(CPool<T>* pool, int handle) {
    int slot = handle >> 8;
    if (pool->flags[slot] == uint8_t(handle)) {
        return At(pool, slot);
    }
    return nullptr;
}

template <typename T>
int ToRef(CPool<T>* pool, const void* object) {
    int slot = SlotOf(pool, object);
    return pool->flags[slot] + (slot << 8);
}

// (the pointer checks' range test allows slot == size)
template <typename T>
bool InRange(CPool<T>* pool, const void* object) {
    int slot = SlotOf(pool, object);
    return slot >= 0 && slot <= pool->size;
}
#if defined(_WIN64)
// (the pools' globals are the game's 4-byte pointers: the templates deduce through them)
template <typename T>
AnyPool* Any(re::Ptr32<CPool<T>> pool) {
    return Any(pool.get());
}
template <typename T>
T* At(re::Ptr32<CPool<T>> pool, int slot) {
    return At(pool.get(), slot);
}
template <typename T>
int SlotOf(re::Ptr32<CPool<T>> pool, const void* object) {
    return SlotOf(pool.get(), object);
}
template <typename T>
T* FromRef(re::Ptr32<CPool<T>> pool, int handle) {
    return FromRef(pool.get(), handle);
}
template <typename T>
int ToRef(re::Ptr32<CPool<T>> pool, const void* object) {
    return ToRef(pool.get(), object);
}
template <typename T>
bool InRange(re::Ptr32<CPool<T>> pool, const void* object) {
    return InRange(pool.get(), object);
}
#endif

} // namespace

// ---- the generic pool code -----------------------------------------------------------

namespace {

// New's search: the first free slot from lastUsed on, wrapping round once. (A full
// pool leaves lastUsed at `size`, and the next New reads the flag past the end, as
// GTA's does.) The slot's reuse count (the flag's low 7 bits) goes up.
uint8_t* NewSlot(AnyPool* pool) {
    uint8_t* flags = pool->flags;
    bool wrapped = false;
    if (!(flags[pool->lastUsed] & 0x80)) {
        do {
            pool->lastUsed++;
            if (pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while (!(flags[pool->lastUsed] & 0x80));
    }
    flags[pool->lastUsed] &= 0x7F;
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag = uint8_t((flag & 0x80) | ((flag + 1) & 0x7F));
    return pool->objects + pool->elemSize * pool->lastUsed;
}

// CPool<T>::New: a free slot, its sizeof(T) bytes zeroed (nullptr: the pool's full)
template <size_t bytes>
void* __fastcall PoolNew(AnyPool* pool, re::Edx) {
    uint8_t* object = NewSlot(pool);
    if (object) {
        std::memset(object, 0, bytes);
    }
    return object;
}

// CPool<T>::Init: the arrays for `size` slots, all free. With unknown19 (never set by
// CPools) the objects are a new[] array: its size saturates (as the compiler's does),
// with a count before it (`counted`), each constructed by `ctor` (0: none).
template <int32_t elemSize, bool counted, uintptr_t ctor>
void __fastcall PoolInit(AnyPool* pool, re::Edx) {
    pool->ownsArrays = 1;
    pool->elemSize = elemSize;
    if (!pool->IsConstructedArray()) {
        pool->objects = static_cast<uint8_t*>(Malloc(size_t(pool->size * elemSize)));
    } else {
        uint32_t count = uint32_t(pool->size);
        uint32_t bytes = SaturatingMul(count, elemSize);
        if constexpr (counted) {
            bytes = bytes > 0xFFFFFFFB ? 0xFFFFFFFF : bytes + 4;
        }
        auto* array = static_cast<uint8_t*>(Malloc(bytes));
        if (array) {
            if constexpr (counted) {
                *reinterpret_cast<uint32_t*>(array) = count;
                array += 4;
            }
            if constexpr (ctor != 0) {
                VectorConstruct(array, elemSize, int(count), reinterpret_cast<void(__thiscall*)(void*)>(ctor));
            }
        }
        pool->objects = array;
    }
    pool->flags = static_cast<uint8_t*>(Malloc(size_t(pool->size)));
    pool->lastUsed = 0;
    for (int slot = 0; slot < pool->size; slot++) {
        pool->flags[slot] |= 0x80;
        pool->flags[slot] &= 0x80;
    }
}

// How a Flush destroys a constructed (new[]) array
enum class Destroy {
    Plain,     // never: freed as it is
    Counted,   // no destructor: the count and all freed
    Virtual0,  // the first element's virtual (vector deleting) destructor, slot 0
    Virtual1,  // ... slot 1
    Each,      // `dtor` on each, last first
};

// CPool<T>::Flush: frees the arrays (if it owns them) and empties the pool
template <Destroy destroy, int32_t elemSize = 0, uintptr_t dtor = 0>
void __fastcall PoolFlush(AnyPool* pool, re::Edx) {
    if (pool->size <= 0) {
        return;
    }
    if (pool->ownsArrays) {
        uint8_t* objects = pool->objects;
        if (destroy == Destroy::Plain || !pool->IsConstructedArray()) {
            Free(objects);
        } else if (objects) {
            int32_t* count = reinterpret_cast<int32_t*>(objects) - 1;
            if constexpr (destroy == Destroy::Counted) {
                Free(count);
            } else if constexpr (destroy == Destroy::Virtual0 || destroy == Destroy::Virtual1) {
                if (*count == 0) {
                    Free(count);
                } else {
                    // (frees the array too)
                    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(objects);
                    reinterpret_cast<void*(__thiscall*)(void*, int)>(static_cast<void*>(vtable[destroy == Destroy::Virtual0 ? 0 : 1]))(objects, 3);
                }
            } else {
                for (int i = *count - 1; i >= 0; i--) {
                    reinterpret_cast<void(__thiscall*)(void*)>(dtor)(objects + elemSize * i);
                }
                Free(count);
            }
        }
        Free(pool->flags);
    }
    pool->objects = nullptr;
    pool->flags = nullptr;
    pool->size = 0;
    pool->lastUsed = 0;
}

// 0x415DE0: the slot's element, nullptr if it's free
void* __fastcall Pool_GetAt(AnyPool* pool, re::Edx, int slot) {
    return pool->GetAt(slot);
}

// 0x44A730: an element's slot, -1 if the pointer isn't at the start of one (not
// range checked)
int __fastcall Pool_GetJustIndexImpl(AnyPool* pool, re::Edx, uint8_t* object) {
    int slot = SlotOf(pool, object);
    if (object != pool->objects + pool->elemSize * slot) {
        slot = -1;
    }
    return slot;
}

// 0x4498C0: `next` at the end of the pool's chain
void __fastcall Pool_SetNextImpl(AnyPool* pool, re::Edx, AnyPool* next) {
    while (pool->Next()) {
        pool = pool->Next();
    }
    pool->Next() = next;
}

// 0x435BB0: the slots of the pool and those chained to it
int __fastcall Pool_GetSizeChainedImpl(AnyPool* pool, re::Edx) {
    int size = pool->size;
    if (pool->Next()) {
        size += Pool_GetSizeChained(pool->Next());
    }
    return size;
}

// 0x435BD0: a slot of the chain (nullptr past its end); `onlyUsed`: nullptr for a
// free one. (The free test reads this pool's flags even for a slot further down the
// chain, past its array: the chained lookup has made it nullptr already when free.)
void* __fastcall Pool_GetAtChainedImpl(AnyPool* pool, re::Edx, int slot, int onlyUsed) {
    void* object = nullptr;
    if (slot < pool->size) {
        object = At(pool, slot);
    } else if (pool->Next()) {
        object = Pool_GetAtChained(pool->Next(), slot - pool->size, 1);
    }
    if (re::Flag(onlyUsed) && (pool->flags[slot] & 0x80)) {
        object = nullptr;
    }
    return object;
}

// 0x436B90: the same, always only used slots
void* __fastcall Pool_GetAtChainedUsed(AnyPool* pool, re::Edx, int slot) {
    void* object = nullptr;
    if (slot < pool->size) {
        object = At(pool, slot);
    } else if (pool->Next()) {
        object = Pool_GetAtChained(pool->Next(), slot - pool->size, 1);
    }
    return (pool->flags[slot] & 0x80) ? nullptr : object;
}

// 0x5C1470: a handle's slot in the chain, its flag byte not checked (a slot further
// down the chain must be used)
void* __fastcall Pool_GetAtRefChained(AnyPool* pool, re::Edx, int handle) {
    int slot = handle >> 8;
    if (slot < pool->size) {
        return At(pool, slot);
    }
    if (pool->Next()) {
        return Pool_GetAtChained(pool->Next(), slot - pool->size, 1);
    }
    return nullptr;
}

} // namespace

#define POOL_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

POOL_REVERSED("CPool::GetAt", 0x00415DE0, &Pool_GetAt);
POOL_REVERSED("CPool::GetJustIndex", 0x0044A730, &Pool_GetJustIndexImpl);
POOL_REVERSED("CPool::SetNext", 0x004498C0, &Pool_SetNextImpl);
POOL_REVERSED("CPool::GetSizeChained", 0x00435BB0, &Pool_GetSizeChainedImpl);
POOL_REVERSED("CPool::GetAtChained", 0x00435BD0, &Pool_GetAtChainedImpl);
POOL_REVERSED("CPool::GetAtChainedUsed", 0x00436B90, &Pool_GetAtChainedUsed);
POOL_REVERSED("CPool::GetAtRefChained", 0x005C1470, &Pool_GetAtRefChained);

// New, per element type (sizeof(T) zeroed). Shared copies: 0x7305B0 (0x40 bytes: col
// models, floor motion drivers), 0x468AE0 (0x74: entity effect keepers, SFX items)
POOL_REVERSED("CPool<CPtrNode>::New", 0x0044DFC0, &PoolNew<4>);
POOL_REVERSED("CPool<CEntryInfoNode>::New", 0x005EC730, &PoolNew<0x14>);
POOL_REVERSED("CPool<CPed>::New", 0x004787A0, &PoolNew<0x1F44>);
POOL_REVERSED("CPool<CVehicle>::New", 0x004CC410, &PoolNew<0x8C0>);
POOL_REVERSED("CPool<CBuilding>::New", 0x004CECF0, &PoolNew<0x120>);
POOL_REVERSED("CPool<CDummy>::New", 0x00465320, &PoolNew<0x128>);
POOL_REVERSED("CPool<CColModel>::New", 0x007305B0, &PoolNew<0x40>);
POOL_REVERSED("CPool<CPropAnim>::New", 0x004EB100, &PoolNew<0x3B8>);
POOL_REVERSED("CPool<CAccessory>::New", 0x004A6F20, &PoolNew<0x2E8>);
POOL_REVERSED("CPool<CStimulus>::New", 0x004A6350, &PoolNew<0x2C>);
POOL_REVERSED("CPool<CObject>::New", 0x004CFB20, &PoolNew<0x22C>);
POOL_REVERSED("CPool<CProjectile>::New", 0x004D2230, &PoolNew<0x24C>);
POOL_REVERSED("CPool<CCutsceneObject>::New", 0x004CEFC0, &PoolNew<0x33C>);
POOL_REVERSED("CPool<CEntityEffectKeeper>::New", 0x00468AE0, &PoolNew<0x74>);
POOL_REVERSED("CPool<Pool624>::New", 0x005E5620, &PoolNew<0x64>);
POOL_REVERSED("CPool<CAccessoryContainer>::New", 0x00450620, &PoolNew<8>);
POOL_REVERSED("CPool<CAttitudeSet>::New", 0x0046F970, &PoolNew<0x38>);
POOL_REVERSED("CPool<CPedActionTree>::New", 0x00484B80, &PoolNew<0x5C0>);
POOL_REVERSED("CPool<CTargetingSystem>::New", 0x004D6300, &PoolNew<0xE0>);
POOL_REVERSED("CPool<CWeaponAccessoryContainer>::New", 0x004DACD0, &PoolNew<0x20>);
POOL_REVERSED("CPool<CMotionController>::New", 0x007306B0, &PoolNew<0x200>);
POOL_REVERSED("CPool<JointConstraint>::New", 0x00730530, &PoolNew<0x80>);
POOL_REVERSED("CPool<ReachDriver>::New", 0x00730630, &PoolNew<0x50>);
POOL_REVERSED("CPool<CLuaScript>::New", 0x005D9B40, &PoolNew<0x2B0C>);
POOL_REVERSED("CPool<WeaponItem>::New", 0x0045CAD0, &PoolNew<0x28>);
POOL_REVERSED("CPool<TxdDef>::New", 0x005F1640, &PoolNew<0x10>);

// Init, per element type: (element size, a count before a new[] array, constructor)
POOL_REVERSED("CPool<CPtrNode>::Init", 0x0044A760, &PoolInit<4, true, 0>);
POOL_REVERSED("CPool<CEntryInfoNode>::Init", 0x0044A850, &PoolInit<0x14, false, 0>);
POOL_REVERSED("CPool<CPed>::Init", 0x0044A8D0, &PoolInit<0x1F44, true, 0x0049B300>);
POOL_REVERSED("CPool<CObject>::Init", 0x0044A990, &PoolInit<0x22C, true, 0x004CF410>);
POOL_REVERSED("CPool<CProjectile>::Init", 0x0044AA50, &PoolInit<0x24C, true, 0x00449550>);
POOL_REVERSED("CPool<CCutsceneObject>::Init", 0x0044AB10, &PoolInit<0x33C, true, 0x004CEE60>);
POOL_REVERSED("CPool<CDummy>::Init", 0x0044ABD0, &PoolInit<0x128, true, 0x00465150>);
POOL_REVERSED("CPool<CBuilding>::Init", 0x0044AC90, &PoolInit<0x120, true, 0x004CECB0>);
POOL_REVERSED("CPool<CTreadable>::Init", 0x0044AD50, &PoolInit<0x120, true, 0x004CEE00>);
POOL_REVERSED("CPool<CAccessory>::Init", 0x0044AE10, &PoolInit<0x2E8, true, 0x004A6E40>);
POOL_REVERSED("CPool<CColModel>::Init", 0x0044AED0, &PoolInit<0x40, true, 0x0056E7A0>);
POOL_REVERSED("CPool<CStimulus>::Init", 0x0044B000, &PoolInit<0x2C, true, 0x0044A3C0>);
POOL_REVERSED("CPool<CEntityEffectKeeper>::Init", 0x0044B130, &PoolInit<0x74, true, 0x00468950>);
POOL_REVERSED("CPool<CAccessoryContainer>::Init", 0x0044B260, &PoolInit<8, true, 0x006CC2D0>);
POOL_REVERSED("CPool<CAttitudeSet>::Init", 0x0044B390, &PoolInit<0x38, false, 0x0046F270>);
POOL_REVERSED("CPool<CPedActionTree>::Init", 0x0044B440, &PoolInit<0x5C0, true, 0x00484C00>);
POOL_REVERSED("CPool<CTargetingSystem>::Init", 0x0044B590, &PoolInit<0xE0, true, 0x004D82F0>);
POOL_REVERSED("CPool<CWeaponAccessoryContainer>::Init", 0x0044B650, &PoolInit<0x20, true, 0x004DA9A0>);
POOL_REVERSED("CPool<CMotionController>::Init", 0x0044B780, &PoolInit<0x200, true, 0x00731400>);
POOL_REVERSED("CPool<JointConstraint>::Init", 0x0044B8B0, &PoolInit<0x80, true, 0x007332C0>);
POOL_REVERSED("CPool<CLuaScript>::Init", 0x0044B960, &PoolInit<0x2B0C, true, 0x005D8A30>);
POOL_REVERSED("CPool<FloorMotionDriver>::Init", 0x0044BAB0, &PoolInit<0x40, true, 0x0044A100>);
POOL_REVERSED("CPool<ReachDriver>::Init", 0x0044BB60, &PoolInit<0x50, true, 0x0044A2F0>);
POOL_REVERSED("CPool<WeaponItem>::Init", 0x0044BC10, &PoolInit<0x28, false, 0x0044A390>);
POOL_REVERSED("CPool<SFXItem>::Init", 0x0044BCC0, &PoolInit<0x74, true, 0x00449640>);
POOL_REVERSED("CPool<Pool624>::Init", 0x0044BD70, &PoolInit<0x64, true, 0x005E57A0>);
POOL_REVERSED("CPool<CVehicle>::Init", 0x0044CB90, &PoolInit<0x8C0, true, 0x0044C0C0>);
POOL_REVERSED("CPool<CPropAnim>::Init", 0x0044CFD0, &PoolInit<0x3B8, true, 0x0044CC50>);
POOL_REVERSED("CPool<TxdDef>::Init", 0x005F0F10, &PoolInit<0x10, false, 0>);

// Flush, per way of destroying (shared where the game shares them)
POOL_REVERSED("CPool::Flush_6D2370", 0x006D2370, &PoolFlush<Destroy::Plain>);
POOL_REVERSED("CPool<CPtrNode>::Flush", 0x0044A800, &PoolFlush<Destroy::Counted>);
POOL_REVERSED("CPool<CEntity>::Flush", 0x004498F0, &PoolFlush<Destroy::Virtual0>);
POOL_REVERSED("CPool<JointConstraint>::Flush", 0x00449960, &PoolFlush<Destroy::Virtual1>);
POOL_REVERSED("CPool<CColModel>::Flush", 0x0044AF80, &PoolFlush<Destroy::Each, 0x40, 0x0056E7D0>);
POOL_REVERSED("CPool<CStimulus>::Flush", 0x0044B0B0, &PoolFlush<Destroy::Each, 0x2C, 0x004A64F0>);
POOL_REVERSED("CPool<CEntityEffectKeeper>::Flush", 0x0044B1E0, &PoolFlush<Destroy::Each, 0x74, 0x00468980>);
POOL_REVERSED("CPool<CAccessoryContainer>::Flush", 0x0044B310, &PoolFlush<Destroy::Each, 8, 0x0049A650>);
POOL_REVERSED("CPool<CPedActionTree>::Flush", 0x0044B500, &PoolFlush<Destroy::Each, 0x5C0, 0x004844F0>);
POOL_REVERSED("CPool<CWeaponAccessoryContainer>::Flush", 0x0044B700, &PoolFlush<Destroy::Each, 0x20, 0x0049A650>);
POOL_REVERSED("CPool<CMotionController>::Flush", 0x0044B830, &PoolFlush<Destroy::Each, 0x200, 0x0072FE60>);
POOL_REVERSED("CPool<CLuaScript>::Flush", 0x0044BA20, &PoolFlush<Destroy::Each, 0x2B0C, 0x005DAAA0>);
POOL_REVERSED("CPool<Pool624>::Flush", 0x0044BE20, &PoolFlush<Destroy::Each, 0x64, 0x005E5450>);

// ---- operator new / delete: the classes that live in a pool ---------------------------
// new takes the size and ignores it (the pool's New zeroes sizeof(T)); delete frees the
// slot (the destructor has run).

namespace {

template <uintptr_t poolVar>
AnyPool* PoolAt() {
    return *reinterpret_cast<re::Ptr32<AnyPool>*>(poolVar);
}

template <uintptr_t poolVar, uintptr_t poolNew>
void* __cdecl OperatorNew(size_t) {
    return reinterpret_cast<void*(__thiscall*)(AnyPool*)>(poolNew)(PoolAt<poolVar>());
}

// (these zero the size asked for after New: nullptr from a full pool isn't checked)
template <uintptr_t poolVar, uintptr_t poolNew>
void* __cdecl OperatorNewZeroed(size_t size) {
    void* object = reinterpret_cast<void*(__thiscall*)(AnyPool*)>(poolNew)(PoolAt<poolVar>());
    std::memset(object, 0, size);
    return object;
}

// (not null checked, unless `checked`)
template <uintptr_t poolVar, bool checked = false>
void __cdecl OperatorDelete(void* object) {
    if constexpr (checked) {
        if (!object) {
            return;
        }
    }
#if defined(_M_X64)
    // (bring-up check: a physical entity (ped, vehicle, object) freed while still on the world's
    // moving list: whoever deleted it skipped CWorld::Remove)
    if constexpr (poolVar == 0x00C0F5F0 || poolVar == 0x00C0F5F4 || poolVar == 0x00C0F614) {
        if (object && *reinterpret_cast<const uint32_t*>(static_cast<uint8_t*>(object) + 0x17C) != 0) {
            std::fprintf(stderr, "x64 check: entity %p (pool 0x%08X) deleted while on the moving list\n", object, unsigned(poolVar));
            __debugbreak();
        }
    }
#endif
    PoolAt<poolVar>()->Delete(static_cast<uint8_t*>(object));
}

// 0x59D910: tells the SFX list first
void __cdecl SFXItem_Delete(void* item) {
    SfxList_Remove(sfxList, item);
    PoolAt<0x00C0F620>()->Delete(static_cast<uint8_t*>(item));
}

} // namespace

POOL_REVERSED("CPtrNode::operator new", 0x0044E030, &OperatorNew<0x00C0F5E8, 0x0044DFC0>);
POOL_REVERSED("CEntryInfoNode::operator new", 0x00429800, &OperatorNew<0x00C0F5EC, 0x005EC730>);
POOL_REVERSED("CPed::operator new", 0x004789E0, &OperatorNew<0x00C0F5F0, 0x004787A0>);
POOL_REVERSED("CVehicle::operator new", 0x004CC490, &OperatorNew<0x00C0F5F4, 0x004CC410>);
POOL_REVERSED("CBuilding::operator new", 0x004CED70, &OperatorNew<0x00C0F5F8, 0x004CECF0>);
POOL_REVERSED("CDummy::operator new", 0x004653A0, &OperatorNew<0x00C0F600, 0x00465320>);
POOL_REVERSED("CPropAnim::operator new", 0x004EB180, &OperatorNew<0x00C0F608, 0x004EB100>);
POOL_REVERSED("CAccessory::operator new", 0x004A7070, &OperatorNew<0x00C0F60C, 0x004A6F20>);
POOL_REVERSED("CObject::operator new", 0x004CFCD0, &OperatorNew<0x00C0F614, 0x004CFB20>);
POOL_REVERSED("CProjectile::operator new", 0x004D2640, &OperatorNew<0x00C0F618, 0x004D2230>);
POOL_REVERSED("CCutsceneObject::operator new", 0x004CF0B0, &OperatorNew<0x00C0F61C, 0x004CEFC0>);
POOL_REVERSED("SFXItem::operator new", 0x0059D430, &OperatorNew<0x00C0F620, 0x00468AE0>);
POOL_REVERSED("CEntityEffectKeeper::operator new", 0x00468B60, &OperatorNewZeroed<0x00C0F628, 0x00468AE0>);
POOL_REVERSED("CAccessoryContainer::operator new", 0x004A7550, &OperatorNewZeroed<0x00C0F62C, 0x00450620>);
POOL_REVERSED("CAttitudeSet::operator new", 0x0046F9F0, &OperatorNewZeroed<0x00C0F630, 0x0046F970>);
POOL_REVERSED("CPedActionTree::operator new", 0x00485190, &OperatorNew<0x00C0F634, 0x00484B80>);
POOL_REVERSED("CTargetingSystem::operator new", 0x004D64E0, &OperatorNew<0x00C0F638, 0x004D6300>);
POOL_REVERSED("CWeaponAccessoryContainer::operator new", 0x004DAD50, &OperatorNew<0x00C0F63C, 0x004DACD0>);
POOL_REVERSED("CMotionController::operator new", 0x007315B0, &OperatorNew<0x00C0F640, 0x007306B0>);
POOL_REVERSED("JointConstraint::operator new", 0x007307C0, &OperatorNew<0x00C0F644, 0x00730530>);
POOL_REVERSED("FloorMotionDriver::operator new", 0x00730830, &OperatorNew<0x00C0F648, 0x007305B0>);
POOL_REVERSED("ReachDriver::operator new", 0x00730F70, &OperatorNew<0x00C0F64C, 0x00730630>);
POOL_REVERSED("CLuaScript::operator new", 0x005D9DA0, &OperatorNew<0x00C0F650, 0x005D9B40>);

POOL_REVERSED("CPtrNode::operator delete", 0x0044E040, &OperatorDelete<0x00C0F5E8>);
POOL_REVERSED("CEntryInfoNode::operator delete", 0x00429810, &OperatorDelete<0x00C0F5EC, true>);
POOL_REVERSED("CPed::operator delete", 0x004789F0, &OperatorDelete<0x00C0F5F0>);
POOL_REVERSED("CVehicle::operator delete", 0x004CD270, &OperatorDelete<0x00C0F5F4>);
POOL_REVERSED("CBuilding::operator delete", 0x004CED80, &OperatorDelete<0x00C0F5F8>);
POOL_REVERSED("CTreadable::operator delete", 0x004CEE30, &OperatorDelete<0x00C0F5FC>);
POOL_REVERSED("CDummy::operator delete", 0x00465420, &OperatorDelete<0x00C0F600>);
POOL_REVERSED("CObject::operator delete", 0x004D0690, &OperatorDelete<0x00C0F614>);
POOL_REVERSED("CProjectile::operator delete", 0x004D2650, &OperatorDelete<0x00C0F618>);
POOL_REVERSED("CCutsceneObject::operator delete", 0x004CF1D0, &OperatorDelete<0x00C0F61C>);
POOL_REVERSED("SFXItem::operator delete", 0x0059D910, &SFXItem_Delete);
POOL_REVERSED("CEntityEffectKeeper::operator delete", 0x00468B90, &OperatorDelete<0x00C0F628>);
POOL_REVERSED("CAccessoryContainer::operator delete", 0x004A7580, &OperatorDelete<0x00C0F62C>);
POOL_REVERSED("CAttitudeSet::operator delete", 0x0046FA60, &OperatorDelete<0x00C0F630>);
POOL_REVERSED("CPedActionTree::operator delete", 0x004852B0, &OperatorDelete<0x00C0F634>);
POOL_REVERSED("CTargetingSystem::operator delete", 0x004D79B0, &OperatorDelete<0x00C0F638>);
POOL_REVERSED("CWeaponAccessoryContainer::operator delete", 0x004DAD60, &OperatorDelete<0x00C0F63C>);
POOL_REVERSED("CMotionController::operator delete", 0x00731C00, &OperatorDelete<0x00C0F640>);
POOL_REVERSED("JointConstraint::operator delete", 0x00731750, &OperatorDelete<0x00C0F644, true>);
POOL_REVERSED("FloorMotionDriver::operator delete", 0x00731780, &OperatorDelete<0x00C0F648, true>);
POOL_REVERSED("ReachDriver::operator delete", 0x007317B0, &OperatorDelete<0x00C0F64C, true>);
POOL_REVERSED("CLuaScript::operator delete", 0x005DA9F0, &OperatorDelete<0x00C0F650>);
POOL_REVERSED("WeaponItem::operator delete", 0x0045CF90, &OperatorDelete<0x00C0F654>);

// ---- CPools ---------------------------------------------------------------------------

namespace CPools {

namespace {

// Initialise's inlined pool constructor, then the type's Init
template <typename T>
void Make(re::Ptr32<CPool<T>>& global, int32_t size, void(__thiscall* init)(void*)) {
    auto* pool = static_cast<CPool<T>*>(Malloc(sizeof(CPool<T>)));
    if (pool) {
        pool->unknown10 = 0;
        pool->objects = nullptr;
        pool->flags = nullptr;
        pool->lastUsed = 0;
        pool->ownsArrays = 1;
        pool->size = size;
        pool->unknown19 = 0;
        init(pool);
    }
    global = pool;
}

template <typename T>
void Dispose(CPool<T>* pool, void(__thiscall* flush)(void*)) {
    if (pool) {
        flush(pool);
        Free(pool);
    }
}
#if defined(_WIN64)
template <typename T>
void Dispose(re::Ptr32<CPool<T>> pool, void(__thiscall* flush)(void*)) {
    Dispose(pool.get(), flush);
}
#endif

} // namespace

// The pools the world's placement fills. (bullyre_x64, the remaster's: room for more of the world
// at once, an interior kept loaded beside the town at its door (SeamlessDoors.cpp); the game's
// sizes ran out there, a placement file's prop anim made from a full pool. A sector list's node
// packs an entity's slot and the next node's index in 14 bits each (0x3FFF: none), so no pool
// here goes past 0x3FFF slots)
#if defined(_WIN64)
constexpr int32_t kPtrNodes = 0x3FFF, kDummies = 1200, kPropAnims = 1000, kBuildings = 6000;
#else
constexpr int32_t kPtrNodes = 15000, kDummies = 300, kPropAnims = 220, kBuildings = 2250;
#endif

void __cdecl Initialise() {
    PushMemoryTag(6);
    Make(ms_pPtrNodePool, kPtrNodes, PtrNodePool_Init);
    Make(ms_pEntryInfoNodePool, 2000, EntryInfoNodePool_Init);
    Make(ms_pVehiclePool, 15, VehiclePool_Init);
    Make(ms_pPedPool, 24, PedPool_Init);
    Make(ms_pEntityEffectKeeperPool, 24, EntityEffectKeeperPool_Init);
    Make(ms_pAccessoryContainerPool, 24, AccessoryContainerPool_Init);
    Make(ms_pAttitudeSetPool, 24, AttitudeSetPool_Init);
    Make(ms_pPedActionTreePool, 24, PedActionTreePool_Init);
    Make(ms_pTargetingSystemPool, 24, TargetingSystemPool_Init);
    Make(ms_pWeaponAccessoryContainerPool, 24, WeaponAccessoryContainerPool_Init);
    Make(ms_pMotionControllerPool, 24, MotionControllerPool_Init);
    Make(ms_pJointConstraintPool, 48, JointConstraintPool_Init);
    Make(ms_pFloorMotionDriverPool, 24, FloorMotionDriverPool_Init);
    Make(ms_pReachDriverPool, 48, ReachDriverPool_Init);
    Make(ms_pWeaponItemPool, 57, WeaponItemPool_Init);
    Make(ms_pLuaScriptPool, 8, LuaScriptPool_Init);
    Make(ms_pDummyPool, kDummies, DummyPool_Init);
    Make(ms_pPropAnimPool, kPropAnims, PropAnimPool_Init);
    Make(ms_pBuildingPool, kBuildings, BuildingPool_Init);
    Make(ms_pTreadablePool, 1, TreadablePool_Init);
    Make(ms_pAccessoryPool, 48, AccessoryPool_Init);
    Make(ms_pColModelPool, 4150, ColModelPool_Init);
    Make(ms_pStimulusPool, 87, StimulusPool_Init);
    Make(ms_pObjectPool, 275, ObjectPool_Init);
    Make(ms_pProjectilePool, 35, ProjectilePool_Init);
    Make(ms_pCutScenePool, 30, CutScenePool_Init);
    Make(ms_pSFXItemPool, 48, SFXItemPool_Init);
    Make(ms_pPool624, 200, Pool624_Init);
    // projectiles and cutscene objects are objects: their pools go on from the
    // object pool's slots (its Next() inlined when it has none)
    if (!Any(ms_pObjectPool)->Next()) {
        Any(ms_pObjectPool)->Next() = Any(ms_pProjectilePool);
    } else {
        Pool_SetNext(ms_pObjectPool, ms_pProjectilePool);
    }
    if (!Any(ms_pObjectPool)->Next()) {
        Any(ms_pObjectPool)->Next() = Any(ms_pCutScenePool);
    } else {
        Pool_SetNext(ms_pObjectPool, ms_pCutScenePool);
    }
    ms_pPtrNodes = ms_pPtrNodePool->objects;
    PopMemoryTag();
}

void __cdecl ShutDown() {
    // (the pointer nodes' isn't null checked)
    CPool<CPtrNode>* ptrNodes = ms_pPtrNodePool;
    PtrNodePool_Flush(ptrNodes);
    Free(ptrNodes);
    Dispose(ms_pEntryInfoNodePool, Pool_Flush);
    Dispose(ms_pPedPool, EntityPool_Flush);
    Dispose(ms_pVehiclePool, EntityPool_Flush);
    Dispose(ms_pBuildingPool, EntityPool_Flush);
    Dispose(ms_pTreadablePool, EntityPool_Flush);
    Dispose(ms_pObjectPool, EntityPool_Flush);
    Dispose(ms_pDummyPool, EntityPool_Flush);
    Dispose(ms_pColModelPool, ColModelPool_Flush);
    Dispose(ms_pPropAnimPool, EntityPool_Flush);
    Dispose(ms_pStimulusPool, StimulusPool_Flush);
    Dispose(ms_pAccessoryPool, EntityPool_Flush);
    Dispose(ms_pPool624, Pool624_Flush);
    Dispose(ms_pEntityEffectKeeperPool, EntityEffectKeeperPool_Flush);
    Dispose(ms_pAccessoryContainerPool, AccessoryContainerPool_Flush);
    Dispose(ms_pAttitudeSetPool, Pool_Flush);
    Dispose(ms_pPedActionTreePool, PedActionTreePool_Flush);
    Dispose(ms_pTargetingSystemPool, EntityPool_Flush);
    Dispose(ms_pWeaponAccessoryContainerPool, WeaponAccessoryContainerPool_Flush);
    Dispose(ms_pMotionControllerPool, MotionControllerPool_Flush);
    Dispose(ms_pJointConstraintPool, ConstraintPool_Flush);
    Dispose(ms_pFloorMotionDriverPool, ConstraintPool_Flush);
    Dispose(ms_pReachDriverPool, ConstraintPool_Flush);
    Dispose(ms_pLuaScriptPool, LuaScriptPool_Flush);
    Dispose(ms_pWeaponItemPool, Pool_Flush);
}

namespace {

template <typename T>
void PrintPool(void* file, CPool<T>* pool, int elemSize, const char* format) {
    DebugFilePrintf(file, format, pool->size, pool->GetNoOfUsedSpaces(), elemSize, pool->size * elemSize);
}
#if defined(_WIN64)
template <typename T>
void PrintPool(void* file, re::Ptr32<CPool<T>> pool, int elemSize, const char* format) {
    PrintPool(file, pool.get(), elemSize, format);
}
#endif

} // namespace

void __cdecl PrintUsage(void* file) {
    // (the sizes are the classes': JointConstraint's is its base's, 0x1C, not its slots')
    PrintPool(file, ms_pPtrNodePool, 4, "CPools::GetPtrNodePool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pEntryInfoNodePool, 0x14, "CPools::GetEntryInfoNodePool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pVehiclePool, 0x8C0, "CPools::GetVehiclePool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pPedPool, 0x1F44, "CPools::GetPedPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pObjectPool, 0x22C, "CPools::GetObjectPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pProjectilePool, 0x24C, "CPools::GetProjectilePool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pCutScenePool, 0x33C, "CPools::GetCutScenePool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pDummyPool, 0x128, "CPools::GetDummyPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pPropAnimPool, 0x3B8, "CPools::GetPropAnimPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pBuildingPool, 0x120, "CPools::GetBuildingPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pTreadablePool, 0x120, "CPools::GetTreadablePool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pAccessoryPool, 0x2E8, "CPools::GetAccessoryPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pColModelPool, 0x40, "CPools::GetColModelPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pStimulusPool, 0x2C, "CPools::GetStimulusPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pEntityEffectKeeperPool, 0x74, "CPools::GetEntityEffectKeeperPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pAccessoryContainerPool, 8, "CPools::GetAccessoryContainerPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pAttitudeSetPool, 0x38, "CPools::GetAttitudeSetPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pPedActionTreePool, 0x5C0, "CPools::GetPedActionTreePool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pTargetingSystemPool, 0xE0, "CPools::GetTargetingSystemPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pWeaponAccessoryContainerPool, 0x20, "CPools::GetWeaponAccessoryContainerPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pMotionControllerPool, 0x200, "CPools::GetMotionControllerPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pJointConstraintPool, 0x1C, "CPools::GetJointConstraintPool() Pool\t%d\t%d\t%d\t%d\r\n");
    PrintPool(file, ms_pLuaScriptPool, 0x2B0C, "CPools::GetLuaScriptPool() Pool\t%d\t%d\t%d\t%d\r\n");
}

void* __cdecl GetEntityInPool(uint32_t pool, int slot) {
    switch (pool) {
    case POOL_PED: return At(ms_pPedPool, slot);
    case POOL_VEHICLE: return At(ms_pVehiclePool, slot);
    case POOL_OBJECT: return At(ms_pObjectPool, slot);
    case POOL_PROJECTILE: return At(ms_pProjectilePool, slot);
    case POOL_CUTSCENE: return At(ms_pCutScenePool, slot);
    case POOL_DUMMY: return At(ms_pDummyPool, slot);
    case POOL_PROPANIM: return At(ms_pPropAnimPool, slot);
    case POOL_BUILDING: return At(ms_pBuildingPool, slot);
    case POOL_TREADABLE: return At(ms_pTreadablePool, slot);
    case POOL_ACCESSORY: return At(ms_pAccessoryPool, slot);
    default: return nullptr;
    }
}

namespace {

// the entity's type (+0x108, low 3 bits): 1 building, 2 vehicle, 3 ped, 4 object,
// 5 dummy, 6 prop anim, 7 accessory
uint8_t EntityType(CEntity* entity) {
    return reinterpret_cast<uint8_t*>(entity)[0x108] & 7;
}

// the entity's virtual at +0x88 (a building that's a treadable)
uint8_t IsTreadable(CEntity* entity) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity);
    return re::PtrCast<uint8_t(__thiscall*)(CEntity*)>(vtable[0x88 / 4])(entity);
}

template <typename T>
void SetPoolAndSlot(int32_t* pool, int32_t* slot, ePoolType type, CPool<T>* entities, CEntity* entity) {
    *pool = type;
    *slot = Pool_GetJustIndex(entities, entity);
}
#if defined(_WIN64)
template <typename T>
void SetPoolAndSlot(int32_t* pool, int32_t* slot, ePoolType type, re::Ptr32<CPool<T>> entities, CEntity* entity) {
    SetPoolAndSlot(pool, slot, type, entities.get(), entity);
}
#endif

} // namespace

void __cdecl GetEntityPoolAndSlot(CEntity* entity, int32_t* pool, int32_t* slot) {
    auto* fields = reinterpret_cast<uint8_t*>(entity);
    switch (EntityType(entity)) {
    case 1:
        if (IsTreadable(entity)) {
            SetPoolAndSlot(pool, slot, POOL_TREADABLE, ms_pTreadablePool, entity);
        } else {
            SetPoolAndSlot(pool, slot, POOL_BUILDING, ms_pBuildingPool, entity);
        }
        break;
    case 2: SetPoolAndSlot(pool, slot, POOL_VEHICLE, ms_pVehiclePool, entity); break;
    case 3: SetPoolAndSlot(pool, slot, POOL_PED, ms_pPedPool, entity); break;
    case 4:
        // (a projectile or cutscene object by the fields they set)
        if (*reinterpret_cast<int32_t*>(fields + 0xC4)) {
            SetPoolAndSlot(pool, slot, POOL_PROJECTILE, ms_pProjectilePool, entity);
        } else if (*reinterpret_cast<int32_t*>(fields + 0xEC)) {
            SetPoolAndSlot(pool, slot, POOL_CUTSCENE, ms_pCutScenePool, entity);
        } else {
            SetPoolAndSlot(pool, slot, POOL_OBJECT, ms_pObjectPool, entity);
        }
        break;
    case 5: SetPoolAndSlot(pool, slot, POOL_DUMMY, ms_pDummyPool, entity); break;
    case 6: SetPoolAndSlot(pool, slot, POOL_PROPANIM, ms_pPropAnimPool, entity); break;
    case 7: SetPoolAndSlot(pool, slot, POOL_ACCESSORY, ms_pAccessoryPool, entity); break;
    default: *pool = POOL_NONE; break;  // (slot left as it was)
    }
}

void __cdecl DeleteAll(CPool<uint8_t>* pool, int keep) {
    if (keep != 0) {
        return;
    }
    int total = pool->size;
    if (pool->Next()) {
        total += Pool_GetSizeChained(pool->Next());
    }
    for (int slot = 0; slot < total; slot++) {
        void* entity = nullptr;
        if (slot < pool->size) {
            entity = At(pool, slot);
        } else if (pool->Next()) {
            entity = Pool_GetAtChained(pool->Next(), slot - pool->size, 1);
        }
        // (the free test reads the first pool's flags past its end for chained
        // slots; those are nullptr when free anyway)
        if (!(pool->flags[slot] & 0x80) && entity) {
            World_Remove(entity);
            re::Ptr32<void>* vtable = *static_cast<re::Ptr32<re::Ptr32<void>>*>(entity);
            re::PtrCast<void*(__thiscall*)(void*, int)>(vtable[0])(entity, 1);  // delete
        }
    }
}

CPed* __cdecl GetPed(int handle) {
    return FromRef(ms_pPedPool, handle);
}

CVehicle* __cdecl GetVehicle(int handle) {
    return FromRef(ms_pVehiclePool, handle);
}

CObject* __cdecl GetObjectFromRef(int handle) {
    return FromRef(ms_pObjectPool, handle);
}

CPropAnim* __cdecl GetPropAnim(int handle) {
    return FromRef(ms_pPropAnimPool, handle);
}

CEntity* __cdecl GetEntity(int handle, int kind) {
    if (kind == 0) {
        return reinterpret_cast<CEntity*>(FromRef(ms_pObjectPool, handle));
    }
    if (kind == 4) {
        return reinterpret_cast<CEntity*>(FromRef(ms_pBuildingPool, handle));
    }
    if (kind == 5) {
        return reinterpret_cast<CEntity*>(FromRef(ms_pPropAnimPool, handle));
    }
    return nullptr;
}

int __cdecl GetVehicleRef(CVehicle* vehicle) {
    return ToRef(ms_pVehiclePool, vehicle);
}

int __cdecl GetObjectRef(CObject* object) {
    return ToRef(ms_pObjectPool, object);
}

int __cdecl GetPropAnimRef(CPropAnim* prop) {
    return ToRef(ms_pPropAnimPool, prop);
}

CPropAnim* __cdecl FindPropAnim(int id) {
    // (each slot number is looked up as a handle: slot i >> 8, i.e. slot 0, whose flag
    // byte must be i. So only slot 0's prop anim is ever found)
    CPool<CPropAnim>* pool = ms_pPropAnimPool;
    for (int i = 0; i < pool->size; i++) {
        CPropAnim* prop = FromRef(pool, i);
        if (prop && *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(prop) + 0x3AC) == id) {
            return prop;
        }
    }
    return nullptr;
}

int __cdecl GetSFXItemIndex(SFXItem* item) {
    CPool<SFXItem>* pool = ms_pSFXItemPool;
    int slot = SlotOf(pool, item);
    if (slot < 0 || slot >= pool->size || item != At(pool, slot)) {
        slot = -1;
    }
    return slot;
}

uint8_t __cdecl IsBuildingPointerValid(CEntity* building) {
    if (!building) {
        return 0;
    }
    if (IsTreadable(building)) {
        return InRange(ms_pTreadablePool, building);
    }
    return InRange(ms_pBuildingPool, building);
}

uint8_t __cdecl IsVehiclePointerValid(CVehicle* vehicle) {
    if (!vehicle) {
        return 0;
    }
    // (the range is fixed at 0..15, the pool's size + 1)
    if (uint32_t(SlotOf(ms_pVehiclePool, vehicle)) > 15) {
        return 0;
    }
    return *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(vehicle) + 0x178) != 0;
}

uint8_t __cdecl IsPropAnimPointerValid(CPropAnim* prop) {
    if (!prop) {
        return 0;
    }
    return InRange(ms_pPropAnimPool, prop);
}

#define POOLS_REVERSED(fn, address) \
    REVERSED("CPools::" #fn, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

POOLS_REVERSED(Initialise, 0x0044D320);
POOLS_REVERSED(ShutDown, 0x0044D090);
POOLS_REVERSED(PrintUsage, 0x0044C330);
POOLS_REVERSED(GetEntityInPool, 0x0044A3E0);
POOLS_REVERSED(GetEntityPoolAndSlot, 0x0044C930);
POOLS_REVERSED(DeleteAll, 0x0044A4D0);
POOLS_REVERSED(GetPed, 0x0044A550);
POOLS_REVERSED(GetVehicle, 0x0044A580);
POOLS_REVERSED(GetObjectFromRef, 0x0044A5B0);
POOLS_REVERSED(GetPropAnim, 0x0044A5E0);
POOLS_REVERSED(GetEntity, 0x0044A660);
POOLS_REVERSED(GetVehicleRef, 0x0044CAC0);
POOLS_REVERSED(GetObjectRef, 0x0044CAF0);
POOLS_REVERSED(GetPropAnimRef, 0x0044CB20);
POOLS_REVERSED(FindPropAnim, 0x0044A610);
POOLS_REVERSED(GetSFXItemIndex, 0x0044CB50);
POOLS_REVERSED(IsBuildingPointerValid, 0x004CEDB0);
POOLS_REVERSED(IsVehiclePointerValid, 0x004CE390);
POOLS_REVERSED(IsPropAnimPointerValid, 0x004ED800);

} // namespace CPools
