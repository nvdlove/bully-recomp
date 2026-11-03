// CSimpleModelInfo's small members (0x5273E0..0x527660: its atomic, its LOD distance),
// a bitset of 0x17D words (0xC9DD58) with its get / set, and the simple, time and 2d
// effect stores' New and Delete.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/ModelInfo.h"
#include "game/Pool.h"
#include "reversed.h"

namespace simplemi17 {

GAME_VAR(float, lodDistanceScale, 0x00C3CD00);   // TheCamera +0x98
GAME_ARRAY(uint32_t, modelBits, 0x00C9DD58);      // 0x17D words
GAME_VAR(CPool<uint8_t>, simpleStore, 0x00A76AD4);
GAME_VAR(CPool<uint8_t>, timeStore, 0x00A76AF0);
GAME_VAR(CPool<uint8_t>, effectStore, 0x00A76B28);
inline re::Ptr32<void>* const simpleVtable = reinterpret_cast<re::Ptr32<void>*>(0x00917B00);

GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_RemoveRef, 0x005F1160, int slot);
GAME_FN(void*, __cdecl, Atomic_Clone, 0x006C70D0, void* atomic);
GAME_FN(int, __cdecl, Atomic_Destroy, 0x006C72C0, void* atomic);
GAME_FN(void*, __thiscall, SimpleStore_New, 0x00527580, CPool<uint8_t>* pool);
GAME_FN(void*, __thiscall, Pool40_New, 0x007305B0, CPool<uint8_t>* pool);   // (0x40 bytes zeroed)
GAME_FN(void, __thiscall, BaseModelInfo_Destruct, 0x0050EE20, void* self);

using re::At;

// 0x5273E0: its atomic, when `distance` is within its LOD distance (nullptr beyond)
void* __fastcall GetAtomicFromDistance(CSimpleModelInfo* self, re::Edx, float distance) {
    if (distance < At<float>(self, 0x24) * lodDistanceScale) {
        return self->object;
    }
    return nullptr;
}

// 0x527410: its atomic and flags cleared
void __fastcall ClearAtomicAndFlags(CSimpleModelInfo* self, re::Edx) {
    self->object = nullptr;
    self->flags = 0;
}

// 0x527420: its LOD distance (scaled)
float __fastcall GetLodDistance(CSimpleModelInfo* self, re::Edx) {
    return At<float>(self, 0x24) * lodDistanceScale;
}

// 0x527440
void __fastcall SetLodDistance(CSimpleModelInfo* self, re::Edx, float distance) {
    At<float>(self, 0x24) = distance;
}

// 0x527450 vf4 CreateInstance: a copy of its atomic (flag 0x100: the atomic itself,
// shared). (eax keeps the atomic, so that's what's returned)
void* __fastcall CreateInstance(CSimpleModelInfo* self, re::Edx) {
    void* atomic = self->object;
    if (!atomic) {
        return nullptr;
    }
    if (self->flags & 0x100) {
        return atomic;
    }
    return Atomic_Clone(atomic);
}

// 0x527470: the bitset cleared. (The compiler's remainder store is off by one: word
// 0x17C is left as it was and 0x17D cleared)
void __cdecl ClearModelBits() {
    int i = 0;
    for (; i < 0x179; i += 4) {
        modelBits[i] = 0;
        modelBits[i + 1] = 0;
        modelBits[i + 2] = 0;
        modelBits[i + 3] = 0;
    }
    modelBits[i + 1] = 0;
}

// 0x5274E0 (all of eax: 0 / 1)
uint32_t __cdecl GetModelBit(int index) {
    return (modelBits[index >> 5] & (1u << (index & 31))) != 0 ? 1 : 0;
}

// 0x527510: (the whole word is overwritten: the other bits in it are lost)
void __cdecl SetModelBit(int index, int value) {
    modelBits[index >> 5] = uint32_t(value != 0) << (index & 31);
}

// 0x527530 vf12 SetAtomic: (its dictionary referenced)
void __fastcall SetAtomic(CSimpleModelInfo* self, re::Edx, void* atomic) {
    self->object = atomic;
    Txd_AddRef(self->txdIndex);
}

// 0x527550 vf3 DeleteObjects: its atomic destroyed, its dictionary let go
void __fastcall DeleteObjects(CSimpleModelInfo* self, re::Edx) {
    if (self->object) {
        Atomic_Destroy(self->object);
        self->object = nullptr;
        Txd_RemoveRef(self->txdIndex);
    }
}

// 0x527580: CPool<CSimpleModelInfo>::New (0x34 bytes zeroed; nullptr when full)
void* __fastcall SimplePool_New(CPool<uint8_t>* pool, re::Edx) {
    bool wrapped = false;
    if ((pool->flags[pool->lastUsed] & 0x80) == 0) {
        do {
            pool->lastUsed++;
            if (pool->lastUsed >= pool->size) {
                if (wrapped) {
                    return nullptr;
                }
                pool->lastUsed = 0;
                wrapped = true;
            }
        } while ((pool->flags[pool->lastUsed] & 0x80) == 0);
    }
    pool->flags[pool->lastUsed] &= 0x7F;
    uint8_t& flag = pool->flags[pool->lastUsed];
    flag ^= uint8_t((uint8_t(flag + 1) ^ flag) & 0x7F);
    uint8_t* object = pool->objects + pool->elemSize * pool->lastUsed;
    std::memset(object, 0, 0x34);
    return object;
}

// 0x527600: a simple model info from its store
void* __cdecl SimpleStore_NewObject() {
    return SimpleStore_New(&simpleStore);
}

// 0x527610 / 0x527620: from the time and 2d effect stores (0x7305B0: 0x40 bytes zeroed)
void* __cdecl TimeStore_NewObject() {
    return Pool40_New(&timeStore);
}
void* __cdecl EffectStore_NewObject() {
    return Pool40_New(&effectStore);
}

// 0x527630: the destructor's body (its atomic only forgotten, its dictionary let go)
void __fastcall Destruct(CSimpleModelInfo* self, re::Edx) {
    self->vtable = simpleVtable;
    if (self->object) {
        self->object = nullptr;
        Txd_RemoveRef(self->txdIndex);
    }
    BaseModelInfo_Destruct(self);
}

// 0x527660 / 0x527690 / 0x5276C0: the stores' Delete
static void PoolDelete(CPool<uint8_t>& pool, void* object) {
    int slot = (static_cast<uint8_t*>(object) - pool.objects) / pool.elemSize;
    pool.flags[slot] |= 0x80;
    if (slot < pool.lastUsed) {
        pool.lastUsed = slot;
    }
}
void __cdecl SimpleStore_Delete(void* object) {
    PoolDelete(simpleStore, object);
}
void __cdecl TimeStore_Delete(void* object) {
    PoolDelete(timeStore, object);
}
void __cdecl EffectStore_Delete(void* object) {
    PoolDelete(effectStore, object);
}

} // namespace simplemi17

// ---- registration ----------------------------------------------------------------------

#define SIMPLEMI17_REVERSED(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace simplemi17 {
SIMPLEMI17_REVERSED("CSimpleModelInfo::GetAtomicFromDistance", 0x005273E0, &GetAtomicFromDistance);
SIMPLEMI17_REVERSED("CSimpleModelInfo::ClearAtomicAndFlags", 0x00527410, &ClearAtomicAndFlags);
SIMPLEMI17_REVERSED("CSimpleModelInfo::GetLodDistance", 0x00527420, &GetLodDistance);
SIMPLEMI17_REVERSED("CSimpleModelInfo::SetLodDistance", 0x00527440, &SetLodDistance);
SIMPLEMI17_REVERSED("CSimpleModelInfo::CreateInstance", 0x00527450, &CreateInstance);
SIMPLEMI17_REVERSED("ModelBits::Clear", 0x00527470, &ClearModelBits);
SIMPLEMI17_REVERSED("ModelBits::Get", 0x005274E0, &GetModelBit);
SIMPLEMI17_REVERSED("ModelBits::Set", 0x00527510, &SetModelBit);
SIMPLEMI17_REVERSED("CSimpleModelInfo::SetAtomic", 0x00527530, &SetAtomic);
SIMPLEMI17_REVERSED("CSimpleModelInfo::DeleteObjects", 0x00527550, &DeleteObjects);
SIMPLEMI17_REVERSED("CPool<CSimpleModelInfo>::New", 0x00527580, &SimplePool_New);
SIMPLEMI17_REVERSED("CModelInfo::ms_simpleModelStore.New", 0x00527600, &SimpleStore_NewObject);
SIMPLEMI17_REVERSED("CModelInfo::ms_timeModelStore.New", 0x00527610, &TimeStore_NewObject);
SIMPLEMI17_REVERSED("CModelInfo::ms_2dEffectStore.New", 0x00527620, &EffectStore_NewObject);
SIMPLEMI17_REVERSED("CSimpleModelInfo::Destruct", 0x00527630, &Destruct);
SIMPLEMI17_REVERSED("CModelInfo::ms_simpleModelStore.Delete", 0x00527660, &SimpleStore_Delete);
SIMPLEMI17_REVERSED("CModelInfo::ms_timeModelStore.Delete", 0x00527690, &TimeStore_Delete);
SIMPLEMI17_REVERSED("CModelInfo::ms_2dEffectStore.Delete", 0x005276C0, &EffectStore_Delete);
} // namespace simplemi17
