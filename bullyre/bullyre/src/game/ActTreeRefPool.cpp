// A pool of names kept as hashes, each with a reference count: the act trees' refs
// (0xC0F7B4, "ActTreeRefPool"; Streaming2.cpp's CatPool, ModelInfo2.cpp's CCatStore).

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

namespace {

struct HashRefEntry {
    uint32_t hash;   // +0
    int32_t numRefs; // +4
};
static_assert(sizeof(HashRefEntry) == 8);

struct HashRefPool {
    re::Ptr32<CPool<HashRefEntry>> pool;  // +0 (made by Init)
    int32_t size;               // +4
};
static_assert(sizeof(HashRefPool) == 8);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __thiscall, Pool_InitArrays, 0x006D2D20, void* pool);   // the arrays, for `size` slots
GAME_FN(HashRefEntry*, __thiscall, Pool_New, 0x00450620, void* pool);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);

inline const char* const nameHash = reinterpret_cast<const char*>(0x009064E8);   // "Hash"
inline const char* const nameNull = reinterpret_cast<const char*>(0x009064F0);   // "Null"

// (the game's GetAt for a free slot is nullptr, and these then go through it at +4:
// an access violation, kept)
int32_t& NullRefs() {
    return *reinterpret_cast<int32_t*>(uintptr_t(4));
}

// 0x450690 (thiscall): the pool, once (the name unused)
void __fastcall Init(HashRefPool* self, re::Edx, int size, const char*) {
    if (self->pool) {
        return;
    }
    self->size = size;
    auto* pool = static_cast<CPool<HashRefEntry>*>(Malloc(sizeof(CPool<HashRefEntry>)));
    if (!pool) {
        self->pool = nullptr;
        return;
    }
    pool->unknown10 = 0;
    pool->objects = nullptr;
    pool->flags = nullptr;
    pool->lastUsed = 0;
    pool->ownsArrays = 1;
    pool->size = size;
    pool->unknown19 = 0;
    Pool_InitArrays(pool);
    self->pool = pool;
}

// 0x4506F0 (thiscall)
void __fastcall AddRef(HashRefPool* self, re::Edx, int slot) {
    CPool<HashRefEntry>* pool = self->pool;
    if (!pool) {
        return;
    }
    if (pool->IsFreeSlot(slot)) {
        NullRefs() += 1;
        return;
    }
    reinterpret_cast<HashRefEntry*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot)->numRefs++;
}

// 0x450720 (thiscall): none left
uint8_t __fastcall RemoveRef(HashRefPool* self, re::Edx, int slot) {
    CPool<HashRefEntry>* pool = self->pool;
    if (!pool) {
        return 0;
    }
    if (pool->IsFreeSlot(slot)) {
        return --NullRefs() == 0;
    }
    return --reinterpret_cast<HashRefEntry*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot)->numRefs == 0;
}

// 0x450760 (thiscall): -1 without a pool
int32_t __fastcall GetNumRefs(HashRefPool* self, re::Edx, int slot) {
    CPool<HashRefEntry>* pool = self->pool;
    if (!pool) {
        return -1;
    }
    if (pool->IsFreeSlot(slot)) {
        return NullRefs();
    }
    return reinterpret_cast<HashRefEntry*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot)->numRefs;
}

// 0x450790 (thiscall): the names aren't kept
const char* __fastcall GetName(HashRefPool* self, re::Edx, int) {
    return self->pool ? nameHash : nameNull;
}

// 0x4507B0 (thiscall): the slot with the name's hash, -1: none
int __fastcall FindIndex(HashRefPool* self, re::Edx, const char* name) {
    if (!self->pool) {
        return -1;
    }
    uint32_t hash = HashName(name);
    CPool<HashRefEntry>* pool = self->pool;
    for (int slot = 0; slot < pool->size; slot++) {
        if (pool->IsFreeSlot(slot)) {
            continue;
        }
        auto* entry = reinterpret_cast<HashRefEntry*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
        if (entry && hash == entry->hash) {
            return slot;
        }
    }
    return -1;
}

// 0x450810 (thiscall): a new slot for the name (no references), -1 without a pool.
// (A full pool's New fails and this writes through nullptr, as the game does)
int __fastcall Add(HashRefPool* self, re::Edx, const char* name) {
    if (!self->pool) {
        return -1;
    }
    HashRefEntry* entry = Pool_New(self->pool);
    entry->numRefs = 0;
    entry->hash = HashName(name);
    return int((reinterpret_cast<uint8_t*>(entry) - re::PtrCast<uint8_t*>(self->pool->objects)) / self->pool->elemSize);
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define C04B_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04B_REVERSED("HashRefPool::Init", 0x00450690, &Init);
C04B_REVERSED("HashRefPool::AddRef", 0x004506F0, &AddRef);
C04B_REVERSED("HashRefPool::RemoveRef", 0x00450720, &RemoveRef);
C04B_REVERSED("HashRefPool::GetNumRefs", 0x00450760, &GetNumRefs);
C04B_REVERSED("HashRefPool::GetName", 0x00450790, &GetName);
C04B_REVERSED("HashRefPool::FindIndex", 0x004507B0, &FindIndex);
C04B_REVERSED("HashRefPool::Add", 0x00450810, &Add);
