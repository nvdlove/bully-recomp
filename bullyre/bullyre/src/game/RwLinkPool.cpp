// A pooled link list (the world/sector code's, 0x6B9610.. and 0x6C32D0 use it): an array
// of 0xC-byte links whose +8 chains free ones onto the owner's free list at +0x10.
// 0x5EF6C0..0x5EF750.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct LinkPool {
    uint32_t unknown0;     // +0x0
    re::Ptr32<uint8_t> head;         // +0x4
    re::Ptr32<uint8_t> tail;         // +0x8
    re::Ptr32<uint8_t> links;        // +0xC (0xC each)
    re::Ptr32<uint8_t> freeLinks;    // +0x10
    int32_t count;         // +0x14
    int32_t used;          // +0x18
    int32_t owned;         // +0x1C
    uint32_t unknown20;    // +0x20
    re::Ptr32<LinkPool> owner;       // +0x24 (whose free list and +0x28 count)
    int32_t users;         // +0x28
};
static_assert(offsetof(LinkPool, users) == 0x28);

GAME_FN(void*, __cdecl, LP_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __stdcall, LP_ConstructArray, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(void, __thiscall, LP_Reset, 0x005EF6C0, LinkPool* self);
void* const kLinkConstruct = reinterpret_cast<void*>(0x0066F280);

// 0x5EF6C0 (thiscall): every link onto the owner's free list
void __fastcall Reset(LinkPool* self) {
    self->freeLinks = nullptr;
    if (self->count <= 0) {
        self->used = 0;
        return;
    }
    for (int32_t i = 0; i < self->count; i++) {
        uint8_t* link = self->links + i * 0xC;
        *reinterpret_cast<re::Ptr32<uint8_t>*>(link + 8) = self->owner->freeLinks;
        self->owner->freeLinks = link;
        self->used--;
    }
    self->used = 0;
}

// 0x5EF710 (thiscall): the chain from head to tail given back to the owner's free list,
// and all cleared (the links array isn't freed)
void __fastcall Release(LinkPool* self) {
    if (self->tail) {
        *reinterpret_cast<re::Ptr32<uint8_t>*>(self->tail + 8) = self->owner->freeLinks;
        self->owner->freeLinks = self->head;
    }
    LinkPool* owner = self->owner;
    self->used = 0;
    self->head = nullptr;
    self->tail = nullptr;
    owner->users--;
    self->count = 0;
    self->links = nullptr;
    self->freeLinks = nullptr;
    self->owner = nullptr;
    self->used = 0;
}

// 0x5EF750 (thiscall): its own links made (count of them), itself the owner
void __fastcall Init(LinkPool* self, re::Edx, uint32_t count) {
    self->count = int32_t(count);
    uint64_t bytes = uint64_t(count) * 0xC;
    void* links = LP_Malloc(bytes >> 32 ? 0xFFFFFFFFu : uint32_t(bytes));
    if (links) {
        LP_ConstructArray(links, 0xC, int(count), kLinkConstruct);
    } else {
        links = nullptr;
    }
    self->links = static_cast<uint8_t*>(links);
    self->owner = self;
    self->owned = 1;
    LP_Reset(self);
}

#define LINKPOOL_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(LinkPoolOriginal_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(LinkPoolOriginal_, __LINE__), \
             re::Mode::Original)

LINKPOOL_REVERSED("RwLinkPool::Reset", 0x005EF6C0, &Reset);
LINKPOOL_REVERSED("RwLinkPool::Release", 0x005EF710, &Release);
LINKPOOL_REVERSED("RwLinkPool::Init", 0x005EF750, &Init);

} // namespace
