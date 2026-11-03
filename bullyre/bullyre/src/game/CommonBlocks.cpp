// The memory manager's common blocks: small allocations served from fixed-size blocks,
// each block carved into items of one size class (the classes from minSize to maxSize in
// steps). Per class there's a list of blocks with free items (sorted, fullest first) and
// one of full blocks; unused blocks sit on a free list. 0x5EDA70..0x5EDE80.

#include <windows.h>
#include <intrin.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
#include "game/HeapCheck.h"
#include "log.h"
namespace {

struct CbBlock {
    re::Ptr32<CbBlock> next;     // +0x0
    re::Ptr32<CbBlock> prev;     // +0x4
    re::Ptr32<void> freeItems;   // +0x8 (each free item's first dword the next)
    uint16_t used;     // +0xC
    uint16_t sizeClass; // +0xE (0xFFFF: unused)
};
static_assert(sizeof(CbBlock) == 0x10);

// the per-class limits (when set up with them)
struct CbLimit {
    int16_t max;
    int16_t count;
};

struct CbHeap {
    re::Ptr32<uint8_t> blocks;       // +0x0
    re::Ptr32<re::Ptr32<CbBlock>> partial;     // +0x4 (also the start of the memory)
    re::Ptr32<re::Ptr32<CbBlock>> full;        // +0x8
    re::Ptr32<CbBlock> freeBlocks;   // +0xC
    re::Ptr32<CbLimit> limits;       // +0x10
    re::Ptr32<const char> name;      // +0x14
    uint32_t freeCount;    // +0x18
    uint32_t blockSize;    // +0x1C
    uint32_t size;         // +0x20 (of the blocks)
    uint32_t minSize;      // +0x24
    uint32_t maxSize;      // +0x28
    uint32_t step;         // +0x2C
    uint32_t classes;      // +0x30
};
static_assert(offsetof(CbHeap, classes) == 0x30);

struct CbParams {
    uint32_t blockSize;    // +0x0
    uint32_t size;         // +0x4
    uint32_t minSize;      // +0x8
    uint32_t maxSize;      // +0xC
    uint32_t step;         // +0x10
    re::Ptr32<const uint32_t> limits; // +0x14 (the low half of each the class's maximum)
};

GAME_FN(void*, __cdecl, CB_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, CB_Free, 0x005EEAB0, void* p);
inline void* __cdecl CB_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(void, __thiscall, CB_Block_Init, 0x005EDAE0, CbBlock* self, int sizeClass, uint32_t itemSize, uint32_t blockSize);
GAME_FN(CbBlock*, __thiscall, CB_NewBlock, 0x005EDCF0, CbHeap* self, int sizeClass, uint32_t itemSize);

void Unlink(CbBlock* block) {
    if (block->prev) {
        block->prev->next = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
}
void PushFront(CbBlock* block, CbBlock* head) {
    block->prev = nullptr;
    block->next = head;
    if (head) {
        head->prev = block;
    }
}

// ---- heap_check (testing): the frees validated, the items quarantined (see HeapCheck.h)

namespace cbcheck {

struct Held {
    uint8_t* item;
    uint32_t size;
    const void* context;
    uint32_t thread;
    uintptr_t frames[8];
};
constexpr uint32_t kHeld = 16384;
struct Ring {
    CbHeap* heap;
    uint32_t next;   // (the slot the next one goes to; the oldest when full)
    bool full;
    Held held[kHeld];
};
Ring g_rings[4];
volatile long g_lock = 0;
int g_reports = 0;

// the per-item history (hashed on the address: a record names its item)
struct Rec {
    const void* item;
    uint32_t seq;
    uint32_t thread;
    const void* context;
    uintptr_t frames[7];
};
struct Slot {
    Rec alloc;       // the latest allocation
    Rec prevFree;    // the latest free (before the latest allocation, when the item is live)
    Rec prevAlloc;   // the allocation before the latest
};
constexpr uint32_t kSlots = 65536;
Slot g_slots[kSlots];
volatile long g_slotLock = 0;
uint32_t g_seq = 0;

inline Slot& SlotOf(const void* p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    return g_slots[((v >> 3) ^ (v >> 19)) & (kSlots - 1)];
}

void Record(Rec& r, const void* item) {
    r.item = item;
    r.thread = GetCurrentThreadId();
    r.context = heapcheck::Context();
    heapcheck::CaptureStackShort(r.frames, 7);
}

void NoteAlloc(const void* item) {
    Rec r;
    Record(r, item);
    while (_InterlockedExchange(&g_slotLock, 1) != 0) {
        _mm_pause();
    }
    r.seq = ++g_seq;
    Slot& s = SlotOf(item);
    s.prevAlloc = s.alloc;
    s.alloc = r;
    _InterlockedExchange(&g_slotLock, 0);
}

void NoteFree(const void* item) {
    Rec r;
    Record(r, item);
    while (_InterlockedExchange(&g_slotLock, 1) != 0) {
        _mm_pause();
    }
    r.seq = ++g_seq;
    SlotOf(item).prevFree = r;
    _InterlockedExchange(&g_slotLock, 0);
}

void LogRec(const char* what, const Rec& r, const void* item) {
    if (r.item == nullptr) {
        re::Log("    %s: (none recorded)", what);
        return;
    }
    re::Log("    %s: #%u by thread %u%s for %p%s:", what, r.seq, unsigned(r.thread),
            r.item == item ? "" : " (another item that hashed here)", r.context,
            r.item == item ? "" : "");
    heapcheck::PrintFrames(r.frames, 7);
}

void Spin() {
    while (_InterlockedExchange(&g_lock, 1) != 0) {
        _mm_pause();
    }
}
void Unlock() {
    _InterlockedExchange(&g_lock, 0);
}

inline uint32_t ItemSize(const CbHeap* self, const CbBlock* block) {
    return self->step * block->sizeClass + self->minSize;
}

inline CbBlock* BlockOf(CbHeap* self, const uint8_t* item) {
    uint8_t* base = self->blocks;
    return reinterpret_cast<CbBlock*>(base + uint32_t(item - base) / self->blockSize * self->blockSize);
}

// why p isn't an item of the block (null when it is)
const char* BadItem(const CbHeap* self, const CbBlock* block, const uint8_t* p) {
    if (block->sizeClass == 0xFFFF) {
        return "its block isn't in use";
    }
    if (block->sizeClass >= self->classes) {
        return "its block's class is out of range";
    }
    uint32_t itemSize = ItemSize(self, block);
    const uint8_t* first = reinterpret_cast<const uint8_t*>(block) + 0x10;
    if (p < first) {
        return "it's in the block's header";
    }
    if (uint32_t(p - first) % itemSize != 0) {
        return "it isn't at an item boundary";
    }
    if (p + itemSize > reinterpret_cast<const uint8_t*>(block) + self->blockSize) {
        return "it runs past the block";
    }
    return nullptr;
}

bool AllPoison(const uint8_t* p, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        if (p[i] != 0xDD) {
            return false;
        }
    }
    return true;
}

Ring* RingOf(CbHeap* heap) {
    for (Ring& r : g_rings) {
        if (r.heap == heap) {
            return &r;
        }
    }
    for (Ring& r : g_rings) {
        if (r.heap == nullptr) {
            r.heap = heap;
            return &r;
        }
    }
    return nullptr;
}

void Crash() {
    *static_cast<volatile int*>(nullptr) = 0;
}

void LogFree(const Held& h, const char* lead) {
    re::Log("    %s by thread %u for %p:", lead, unsigned(h.thread), h.context);
    heapcheck::PrintFrames(h.frames, 8);
    if (h.context != nullptr) {
        heapcheck::Report(h.context);
    }
}

// the chain's head (and the item after it) of a block about to hand out an item
void CheckHead(const CbHeap* self, const CbBlock* block) {
    const uint8_t* head = static_cast<const uint8_t*>(static_cast<void*>(block->freeItems));
    if (head == nullptr) {
        return;
    }
    const char* why = BadItem(self, block, head);
    const uint8_t* after = nullptr;
    if (why == nullptr) {
        after = static_cast<const uint8_t*>(*reinterpret_cast<const re::Ptr32<const uint8_t>*>(head));
        if (after != nullptr) {
            why = BadItem(self, block, after);
            if (why == nullptr && BlockOf(const_cast<CbHeap*>(self), after) != block) {
                why = "the item after the head is in another block";
            }
        }
    }
    if (why != nullptr) {
        re::Log("heap_check: %s: block %p (class %u, %u used) has a corrupt free chain: head %p, after it %p: %s",
                static_cast<const char*>(self->name), static_cast<const void*>(block), unsigned(block->sizeClass),
                unsigned(block->used), static_cast<const void*>(head), static_cast<const void*>(after), why);
        re::Log("    this thread's stack:");
        heapcheck::LogStack();
        heapcheck::ReportCommonBlock(head);
        if (after != nullptr) {
            heapcheck::ReportCommonBlock(after);
        }
        Crash();
    }
}

// a free: validated, then held; the item to free for real now (an older one), or null
uint8_t* OnFree(CbHeap* self, uint8_t* item) {
    CbBlock* block = BlockOf(self, item);
    const char* why = BadItem(self, block, item);
    if (why != nullptr) {
        re::Log("heap_check: %s: free of %p: %s (block %p class %u) on thread %u for %p", static_cast<const char*>(self->name),
                static_cast<void*>(item), why, static_cast<void*>(block), unsigned(block->sizeClass),
                unsigned(GetCurrentThreadId()), heapcheck::Context());
        re::Log("    this thread's stack:");
        heapcheck::LogStack();
        if (heapcheck::Context() != nullptr) {
            heapcheck::Report(heapcheck::Context());
        }
        Crash();
    }
    uint32_t size = ItemSize(self, block);
    // freed before: still filled from then, or on the block's chain already
    bool freedBefore = AllPoison(item, size);
    if (!freedBefore) {
        uint32_t steps = 0;
        for (const uint8_t* f = static_cast<const uint8_t*>(static_cast<void*>(block->freeItems)); f != nullptr && steps < 4096;
             f = static_cast<const uint8_t*>(*reinterpret_cast<const re::Ptr32<const uint8_t>*>(f)), steps++) {
            if (f == item) {
                freedBefore = true;
                break;
            }
            if (BadItem(self, block, f) != nullptr) {
                break;
            }
        }
    }
    Held h;
    h.item = item;
    h.size = size;
    h.context = heapcheck::Context();
    h.thread = GetCurrentThreadId();
    heapcheck::CaptureStack(h.frames, 8);
    if (freedBefore) {
        Held first = {};
        bool found = false;
        Spin();
        for (Ring& r : g_rings) {
            for (uint32_t i = 0; i < (r.full ? kHeld : r.next); i++) {
                if (r.held[i].item == item) {
                    first = r.held[i];
                    found = true;
                    r.held[i].item = nullptr;
                }
            }
        }
        Unlock();
        re::Log("heap_check: %s: %p (%u bytes, block %p class %u) freed twice%s", static_cast<const char*>(self->name),
                static_cast<void*>(item), size, static_cast<void*>(block), unsigned(block->sizeClass),
                found ? ", still held from its first free" : ", its first free not held any more");
        LogFree(h, "this free,");
        if (found) {
            LogFree(first, "its first free,");
        }
        Crash();
    }
    // the effect states and their list nodes (freed with a context set) churn by the thousand
    // a frame: they'd flush the quarantine in a few frames, so they're filled and freed at
    // once; everything else is held
    if (h.context != nullptr || !heapcheck::g_quarantine) {
        std::memset(item + 4, 0xDD, size - 4);
        return item;
    }
    std::memset(item, 0xDD, size);
    uint8_t* evicted = nullptr;
    Spin();
    Ring* r = RingOf(self);
    if (r != nullptr) {
        if (r->full) {
            evicted = r->held[r->next].item;   // (null when it was reported and dropped)
        }
        r->held[r->next] = h;
        if (++r->next == kHeld) {
            r->next = 0;
            r->full = true;
        }
    } else {
        evicted = item;   // (no ring: freed as usual)
    }
    Unlock();
    return evicted;
}

} // namespace cbcheck

// 0x5EDA70 (thiscall): the constructor (the first five dwords cleared)
CbHeap* __fastcall Construct(CbHeap* self) {
    uint32_t* p = reinterpret_cast<uint32_t*>(self);
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
    p[4] = 0;
    return self;
}

// 0x5EDA90 (thiscall): the memory freed (also when it was handed in)
void __fastcall Destruct(CbHeap* self) {
    if (self->partial) {
        CB_Free(self->partial);
        self->blocks = nullptr;
        self->partial = nullptr;
        self->full = nullptr;
    }
}

// 0x5EDAC0 (thiscall): whether a pointer is in the blocks (the end included)
uint8_t __fastcall Owns(const CbHeap* self, re::Edx, uintptr_t p) {
    uintptr_t base = re::PtrCast<uintptr_t>(self->blocks);
    return (p >= base && p <= self->size + base) ? 1 : 0;
}

// 0x5EDAE0 (thiscall, on a block): set up for a class: its items chained
void __fastcall Block_Init(CbBlock* self, re::Edx, int sizeClass, uint32_t itemSize, uint32_t blockSize) {
    self->used = 0;
    self->sizeClass = uint16_t(sizeClass);
    if (itemSize == 0) {
        return;
    }
    int32_t count = int32_t((blockSize - 0x10) / itemSize);
    uint8_t* first = reinterpret_cast<uint8_t*>(self) + 0x10;
    self->freeItems = first;
    if (count <= 0) {
        *reinterpret_cast<re::Ptr32<void>*>(first) = nullptr;
        return;
    }
    int32_t last = count - 1;
    if (last > 0) {
        uint8_t* item = first;
        for (int32_t n = last; n != 0; n--) {
            *reinterpret_cast<re::Ptr32<void>*>(item) = item + itemSize;
            item += itemSize;
        }
    }
    *reinterpret_cast<re::Ptr32<void>*>(first + uint32_t(last) * itemSize) = nullptr;
}

// 0x5EDB50 (thiscall): set up: the class lists (and limits) at the start of the memory
// (allocated when none is handed in), then the blocks, all chained as free
void __fastcall Init(CbHeap* self, re::Edx, const char* name, uint8_t* memory, const CbParams* params) {
    self->name = name;
    self->minSize = params->minSize;
    self->maxSize = params->maxSize;
    self->step = params->step;
    int32_t classes = int32_t(self->maxSize - self->minSize) / int32_t(params->step) + 1;
    uint32_t table = uint32_t(classes) * 4;
    self->classes = uint32_t(classes);
    if (table & 0xF) {
        table += 0x10 - (table & 0xF);
    }
    if (!memory) {
        self->blocks = static_cast<uint8_t*>(CB_Malloc(params->size + table * 2));
        self->size = params->size;
    } else {
        self->blocks = memory;
        self->size = params->size - table * 2;
    }
    uint8_t* base = self->blocks;
    self->partial = reinterpret_cast<re::Ptr32<CbBlock>*>(base);
    self->full = reinterpret_cast<re::Ptr32<CbBlock>*>(base + table);
    self->blocks = base + table + table;
    if (params->limits) {
        self->size += uint32_t(-int32_t(self->classes)) * 4;
        self->limits = re::PtrCast<CbLimit*>(self->blocks);
        self->blocks = self->blocks + self->classes * 4;
        for (uint32_t i = 0; i < self->classes; i++) {
            self->limits[i].max = *reinterpret_cast<const int16_t*>(params->limits + i);
            self->limits[i].count = 0;
        }
    }
    self->blockSize = params->blockSize;
    self->freeCount = self->size / params->blockSize;
    CB_Memset(self->blocks, 0, self->size);
    for (uint32_t i = 0; i < self->classes; i++) {
        self->partial[i] = nullptr;
        self->full[i] = nullptr;
    }
    for (uint32_t i = 0; i < self->freeCount; i++) {
        CbBlock* block = reinterpret_cast<CbBlock*>(self->blocks + self->blockSize * i);
        block->used = 0;
        block->sizeClass = 0xFFFF;
        CbBlock* prev = i == 0 ? nullptr : reinterpret_cast<CbBlock*>(self->blocks + (i - 1) * self->blockSize);
        CbBlock* next = i < self->freeCount - 1
                            ? reinterpret_cast<CbBlock*>(self->blocks + (i + 1) * self->blockSize)
                            : nullptr;
        Unlink(block);
        block->prev = prev;
        block->next = next;
        if (prev) {
            prev->next = block;
        }
        if (block->next) {
            block->next->prev = block;
        }
    }
    self->freeBlocks = re::PtrCast<CbBlock*>(self->blocks);
}

// 0x5EDCF0 (thiscall): a free block given to a class (its limit counted even when there's
// no free block)
CbBlock* __fastcall NewBlock(CbHeap* self, re::Edx, int sizeClass, uint32_t itemSize) {
    CbBlock* block = self->freeBlocks;
    if (self->limits) {
        CbLimit& limit = self->limits[sizeClass];
        if (!(limit.count < limit.max)) {
            return nullptr;
        }
        limit.count = int16_t(limit.count + 1);
    }
    if (!block) {
        return nullptr;
    }
    CB_Block_Init(block, sizeClass, itemSize, self->blockSize);
    self->freeBlocks = self->freeBlocks->next;
    CbBlock* head = self->partial[sizeClass];
    Unlink(block);
    PushFront(block, head);
    self->partial[sizeClass] = block;
    self->freeCount--;
    return block;
}

// 0x5EDD80 (thiscall): an item for a size (from the heap instead when it's too big or its
// class has no room, if fallback)
void* __fastcall Alloc(CbHeap* self, re::Edx, uint32_t size, uint8_t fallback) {
    if (size > self->maxSize) {
        return fallback ? CB_Malloc(size) : nullptr;
    }
    uint32_t step = self->step;
    uint32_t rest = (step - 1) & size;
    uint32_t rounded = size;
    if (rest) {
        rounded = step - rest + size;
    }
    if (rounded == 0) {
        rounded = self->minSize;
    }
    uint32_t minSize = self->minSize;
    uint32_t sizeClass = rounded > minSize ? (rounded - minSize) / step : 0;
    CbBlock* block = self->partial[sizeClass];
    if (!block) {
        block = CB_NewBlock(self, int(sizeClass), step * sizeClass + minSize);
        if (!block) {
            return fallback ? CB_Malloc(size) : nullptr;
        }
    }
    if (heapcheck::g_validateFrees) {
        cbcheck::CheckHead(self, block);   // (testing: see HeapCheck.h)
    }
    void* item = block->freeItems;
    if (item) {
        block->used++;
        block->freeItems = *static_cast<re::Ptr32<void>*>(item);
        if (heapcheck::g_history) {
            cbcheck::NoteAlloc(item);
        }
    } else {
        item = nullptr;
    }
    if (!block->freeItems) {
        // full: onto the full list
        self->partial[sizeClass] = block->next;
        CbBlock* head = self->full[sizeClass];
        Unlink(block);
        PushFront(block, head);
        self->full[sizeClass] = block;
    }
    return item;
}

// 0x5EDE80 (thiscall): an item given back (to the heap when it isn't the blocks', if
// fallback): an emptied block goes back to the free blocks, a full one back to its
// class's list, else it's moved back past the fuller ones
void ReleaseReal(CbHeap* self, uint8_t* item);

void __fastcall Release(CbHeap* self, re::Edx, void* p, uint8_t fallback) {
    uint8_t* base = self->blocks;
    uint8_t* item = static_cast<uint8_t*>(p);
    if (item < base || item > self->size + base) {
        if (fallback) {
            CB_Free(p);
        }
        return;
    }
    if (heapcheck::g_validateFrees) {
        if (heapcheck::g_history) {
            cbcheck::NoteFree(item);
        }
        item = cbcheck::OnFree(self, item);   // (testing: held; an older one freed instead)
        if (item == nullptr) {
            return;
        }
    }
    ReleaseReal(self, item);
}

void ReleaseReal(CbHeap* self, uint8_t* item) {
    uint8_t* base = self->blocks;
    CbBlock* block = reinterpret_cast<CbBlock*>(
        base + uint32_t(item - base) / self->blockSize * self->blockSize);
    void* oldFree = block->freeItems;
    *reinterpret_cast<re::Ptr32<void>*>(item) = oldFree;
    block->freeItems = item;
    bool wasFull = oldFree == nullptr;
    block->used = uint16_t(block->used - 1);
    if (block->used == 0) {
        uint32_t sizeClass = block->sizeClass;
        // (only the list with free items has its head checked)
        if (self->partial[sizeClass] == block) {
            self->partial[sizeClass] = block->next;
        }
        if (self->limits) {
            self->limits[sizeClass].count = int16_t(self->limits[sizeClass].count - 1);
        }
        CbBlock* head = self->freeBlocks;
        Unlink(block);
        PushFront(block, head);
        self->freeCount++;
        self->freeBlocks = block;
        block->used = 0;
        block->sizeClass = 0xFFFF;
        return;
    }
    uint32_t sizeClass = block->sizeClass;
    if (wasFull) {
        if (block == self->full[sizeClass]) {
            self->full[sizeClass] = block->next;
        }
        CbBlock* head = self->partial[sizeClass];
        Unlink(block);
        PushFront(block, head);
        self->partial[sizeClass] = block;
        return;
    }
    while (block->next && block->next->used > block->used) {
        CbBlock* next = block->next;
        if (block == self->partial[sizeClass]) {
            self->partial[sizeClass] = next;
        }
        next = block->next;
        CbBlock* prev = block->prev;
        block->next = next->next;
        block->prev = next;
        next->next = block;
        next->prev = prev;
        if (block->next) {
            block->next->prev = block;
        }
        if (prev) {
            prev->next = next;
        }
    }
}

} // namespace

namespace heapcheck {

void CheckCommonBlocks() {
    using namespace cbcheck;
    Held bad[4];
    uint32_t words[4][2];
    const char* names[4];
    int count = 0;
    Spin();
    for (Ring& r : g_rings) {
        for (uint32_t i = 0; i < (r.full ? kHeld : r.next) && count < 4; i++) {
            Held& h = r.held[i];
            if (h.item == nullptr) {
                continue;
            }
            if (!AllPoison(h.item, h.size)) {
                bad[count] = h;
                words[count][0] = *reinterpret_cast<const uint32_t*>(h.item);
                words[count][1] = *reinterpret_cast<const uint32_t*>(h.item + 4);
                names[count] = r.heap->name;
                count++;
                h.item = nullptr;   // (dropped: never handed back, never reported again)
            }
        }
    }
    Unlock();
    for (int i = 0; i < count && g_reports < 16; i++) {
        g_reports++;
        re::Log("heap_check: %s: freed item %p (%u bytes) written after its free: now %08X %08X ...", names[i],
                static_cast<void*>(bad[i].item), bad[i].size, words[i][0], words[i][1]);
        LogFree(bad[i], "it was freed");
    }
}

void AssertLive(const void* p, const char* what) {
    using namespace cbcheck;
    if (!g_validateFrees || p == nullptr) {
        return;
    }
    Held h = {};
    bool found = false;
    Spin();
    for (Ring& r : g_rings) {
        for (uint32_t i = 0; i < (r.full ? kHeld : r.next) && !found; i++) {
            if (r.held[i].item == p) {
                h = r.held[i];
                found = true;
            }
        }
    }
    Unlock();
    if (!found) {
        return;
    }
    re::Log("heap_check: %s uses %p, a freed common block item (%u bytes) on thread %u", what, p, h.size,
            unsigned(GetCurrentThreadId()));
    re::Log("    this thread's stack:");
    heapcheck::LogStack();
    LogFree(h, "it was freed");
    heapcheck::ReportHistory(p);
    Crash();
}

void CheckItem(const void* p, const void* previous, const char* what) {
    using namespace cbcheck;
    if (!g_validateFrees || p == nullptr) {
        return;
    }
    const uint8_t* item = static_cast<const uint8_t*>(p);
    CbHeap* heap = nullptr;
    for (Ring& r : g_rings) {
        if (r.heap != nullptr && item >= r.heap->blocks && item < r.heap->blocks + r.heap->size) {
            heap = r.heap;
            break;
        }
    }
    const char* why = nullptr;
    if ((reinterpret_cast<uintptr_t>(item) & 3) != 0 || reinterpret_cast<uintptr_t>(item) < 0x10000) {
        why = "not an aligned address";
    } else if (heap != nullptr) {
        const CbBlock* block = BlockOf(heap, item);
        why = BadItem(heap, block, item);
        if (why == nullptr && *reinterpret_cast<const uint32_t*>(item + 4) == 0xDDDDDDDDu) {
            why = "it's filled from a free";
        }
    }
    if (why == nullptr) {
        return;
    }
    re::Log("heap_check: %s: %p isn't a live item: %s (words %08X %08X) on thread %u", what, p, why,
            *reinterpret_cast<const uint32_t*>(item), *reinterpret_cast<const uint32_t*>(item + 4),
            unsigned(GetCurrentThreadId()));
    re::Log("    this thread's stack:");
    heapcheck::LogStack();
    ReportHistory(p);
    if (previous != nullptr) {
        re::Log("    the item before it in the list, %p (words %08X %08X):", previous,
                *static_cast<const uint32_t*>(previous), *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(previous) + 4));
        ReportHistory(previous);
    }
    if (heapcheck::Context() != nullptr) {
        re::Log("    the state being destructed:");
        heapcheck::Report(heapcheck::Context());
    }
    Crash();
}

void CheckIndex(const void* object, const void* array, int index, int capacity, const char* what) {
    using namespace cbcheck;
    if (!g_validateFrees) {
        return;
    }
    AssertLive(object, what);
    AssertLive(array, what);
    if (array == nullptr || (index >= 0 && index < capacity)) {
        return;
    }
    re::Log("heap_check: %s: index %d outside the array %p of %d (object %p) on thread %u", what, index, array, capacity,
            object, unsigned(GetCurrentThreadId()));
    re::Log("    this thread's stack:");
    heapcheck::LogStack();
    ReportHistory(array);
    Crash();
}

void ReportHistory(const void* p) {
    using namespace cbcheck;
    Slot s;
    while (_InterlockedExchange(&g_slotLock, 1) != 0) {
        _mm_pause();
    }
    s = SlotOf(p);
    _InterlockedExchange(&g_slotLock, 0);
    re::Log("    the history of %p (sequence numbers order the events):", p);
    LogRec("latest allocation", s.alloc, p);
    LogRec("latest free", s.prevFree, p);
    LogRec("the allocation before", s.prevAlloc, p);
}

void ReportCommonBlock(const void* p) {
    using namespace cbcheck;
    Held h = {};
    bool found = false;
    Spin();
    for (Ring& r : g_rings) {
        for (uint32_t i = 0; i < (r.full ? kHeld : r.next); i++) {
            if (r.held[i].item == p) {
                h = r.held[i];
                found = true;
            }
        }
    }
    Unlock();
    if (found) {
        re::Log("    %p is a common block item held in the quarantine (%u bytes)", p, h.size);
        LogFree(h, "freed");
    } else {
        re::Log("    %p is not held in the common blocks' quarantine", p);
    }
}

} // namespace heapcheck

namespace {

#define COMMONBLK_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(CommonBlkOriginal_, __LINE__);                                         \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(CommonBlkOriginal_, __LINE__), \
             re::Mode::Original)

COMMONBLK_REVERSED("CommonBlocks::Construct", 0x005EDA70, &Construct);
COMMONBLK_REVERSED("CommonBlocks::Destruct", 0x005EDA90, &Destruct);
COMMONBLK_REVERSED("CommonBlocks::Owns", 0x005EDAC0, &Owns);
COMMONBLK_REVERSED("CommonBlocks::BlockInit", 0x005EDAE0, &Block_Init);
COMMONBLK_REVERSED("CommonBlocks::Init", 0x005EDB50, &Init);
COMMONBLK_REVERSED("CommonBlocks::NewBlock", 0x005EDCF0, &NewBlock);
COMMONBLK_REVERSED("CommonBlocks::Malloc", 0x005EDD80, &Alloc);
COMMONBLK_REVERSED("CommonBlocks::Free", 0x005EDE80, &Release);
// 0x5EEAA0: CMemoryMgr::Malloc, a lone jmp to 0x5EE830
RE_UNHOOKED("CMemoryMgr::MallocJmp", 0x005EEAA0);

} // namespace
