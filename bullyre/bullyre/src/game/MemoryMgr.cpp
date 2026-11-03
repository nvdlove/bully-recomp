#include "game/MemoryMgr.h"

#include <windows.h>

#include <cstring>

#include "game/Crt.h"
#include "game/HeapCheck.h"
#include "log.h"
#include "reversed.h"
// (the heap's lock is the game's CRITICAL_SECTION, 0x18 bytes on x86; x64's is 0x28, so the
// 64-bit build keeps its locks outside the game's layout: Lock() maps the heap to one)
#if !defined(_WIN64)
static_assert(sizeof(CRITICAL_SECTION) == sizeof(CMemoryHeap::lock));
#endif


namespace {
// the common blocks (0x5EDA70...), not reversed yet
GAME_FN(CommonBlocks*, __thiscall, CommonBlocks_Construct, 0x005EDA70, void* mem);
GAME_FN(void, __thiscall, CommonBlocks_Destruct, 0x005EDA90, CommonBlocks* blocks);
GAME_FN(void, __thiscall, CommonBlocks_Init, 0x005EDB50, CommonBlocks* blocks, const char* name, uintptr_t, CommonBlocksParams* params);
GAME_FN(void*, __thiscall, CommonBlocks_Malloc, 0x005EDD80, CommonBlocks* blocks, uint32_t size, int);
GAME_FN(void, __thiscall, CommonBlocks_Free, 0x005EDE80, CommonBlocks* blocks, void* ptr, int);
GAME_FN(uint8_t, __thiscall, CommonBlocks_Owns, 0x005EDAC0, CommonBlocks* blocks, void* ptr);
// the threads
GAME_FN(uint8_t, __cdecl, IsMainThread, 0x00411690);
GAME_FN(uint8_t, __cdecl, IsStreamingThread, 0x004116B0);
// the CRT's heap, which the game's heaps live in
inline void* __cdecl CrtMalloc(size_t size) { return crt::Malloc(size); }
inline void __cdecl CrtFree(void* p) { crt::Free(p); }
// the main heap full: streamed models removed to make room (CStreaming::MakeSpaceFor)
GAME_FN(void, __cdecl, MakeSpaceFor, 0x00531840, uint32_t bytes);
GAME_FN(void, __cdecl, Fn_4302A0, 0x004302A0, int, int);
GAME_FN(void, __cdecl, Fn_6C3DE0, 0x006C3DE0);
GAME_VAR(uint8_t, byte_20C5BE6, 0x020C5BE6);
// Init's, between the heaps
GAME_FN(void, __cdecl, Fn_5D7F50, 0x005D7F50);
// a common blocks object of its own (whose isn't known)
GAME_VAR(CommonBlocksHandle, commonBlocksCFC9B8, 0x00CFC9B8);

// (our own, called where modes apply)
GAME_FN(void, __cdecl, Mem_Init, 0x005EE630);
GAME_FN(void*, __cdecl, Mem_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void*, __cdecl, Mem_MallocDirect, 0x005EE830, size_t size);
GAME_FN(void, __cdecl, Mem_FreeDirect, 0x005EE940, void* ptr);
GAME_FN(void, __cdecl, Mem_Free, 0x005EEAB0, void* ptr);
GAME_FN(int32_t, __cdecl, Mem_GetMemoryTag, 0x005EEDE0);
GAME_FN(void, __cdecl, Mem_PushMemoryTag, 0x005EF0A0, int32_t tag);
GAME_FN(int32_t, __cdecl, Mem_PopMemoryTag, 0x005EF100);
GAME_FN(void, __thiscall, Block_Insert, 0x005EEBF0, HeapBlockDesc* after, HeapBlockDesc* block, re::Ptr32<HeapBlockDesc>* table);
GAME_FN(HeapBlockDesc*, __thiscall, Block_FindSmallest, 0x005EEC50, HeapBlockDesc* self, uint32_t size, re::Ptr32<HeapBlockDesc>* table);
GAME_FN(void, __thiscall, Block_Remove, 0x005EECA0, HeapBlockDesc* block, re::Ptr32<HeapBlockDesc>* table);
GAME_FN(void, __thiscall, Heap_Init, 0x005EF540, CMemoryHeap* heap, void* memory, uint32_t size, int);
GAME_FN(void, __thiscall, Heap_InitCommonBlocks, 0x005EEBB0, CMemoryHeap* heap, const char* name, int32_t maxSize, CommonBlocksParams* params);
GAME_FN(void, __thiscall, Heap_Shutdown, 0x005EEE50, CMemoryHeap* heap);
GAME_FN(void*, __thiscall, Heap_Release, 0x005EEEA0, CMemoryHeap* heap);
GAME_FN(void*, __thiscall, Heap_Malloc, 0x005EF2F0, CMemoryHeap* heap, uint32_t size, int memId);
GAME_FN(void*, __thiscall, Heap_MallocLocked, 0x005EE5F0, CMemoryHeap* heap, uint32_t size);
GAME_FN(void, __thiscall, Heap_Free, 0x005EEEF0, CMemoryHeap* heap, void* ptr);
GAME_FN(HeapBlockDesc*, __thiscall, Heap_WhereShouldMemoryMove, 0x005EF160, CMemoryHeap* heap, void* ptr);
GAME_FN(void*, __thiscall, Heap_MoveHeapBlock, 0x005EF460, CMemoryHeap* heap, HeapBlockDesc* dst, HeapBlockDesc* src);
GAME_FN(void, __thiscall, Heap_FillInBlockData, 0x005EF240, CMemoryHeap* heap, HeapBlockDesc* block, HeapBlockDesc* next, uint32_t size, int memId);
GAME_FN(HeapBlockDesc*, __thiscall, Heap_CombineFreeBlocks, 0x005EED20, CMemoryHeap* heap, HeapBlockDesc* block);
GAME_FN(void, __thiscall, Heap_RegisterMalloc, 0x005EF040, CMemoryHeap* heap, HeapBlockDesc* block, int memId);
GAME_FN(void, __thiscall, Heap_RegisterFree, 0x005EED80, CMemoryHeap* heap, HeapBlockDesc* block);
GAME_FN(void, __thiscall, CommonBlocksHandle_Shutdown, 0x005EEB20, CommonBlocksHandle* handle);

// (the layout's 0x18 bytes: on x64 a real section of ours behind them, re::GameSection)
CRITICAL_SECTION* Lock(CMemoryHeap& heap) {
    return re::GameSection(heap.lock);
}

bool InHeap(const CMemoryHeap& heap, const void* ptr) {
    return uintptr_t(ptr) >= uintptr_t(heap.start) && uintptr_t(ptr) < uintptr_t(heap.end);
}

// the free block into the list where its size goes (Free's and FillInBlockData's end)
void InsertFreeBlock(CMemoryHeap* heap, HeapBlockDesc* block) {
    HeapBlockDesc* at = Block_FindSmallest(&heap->freeList, block->size, heap->sizeClass);
    Block_Insert(at->prev, block, heap->sizeClass);
}
} // namespace

// ---- blocks ----------------------------------------------------------------------------

int HeapBlockDesc::SizeClass(uint32_t size) {
    int c = 0;
    if (size & ~0x1Fu) {
        while (c < 19) {
            c++;
            if ((int32_t(size) >> (c + 5)) == 0) {
                break;
            }
        }
    }
    return c;
}

// (Init inlines it)
static void InsertAfter(HeapBlockDesc* after, HeapBlockDesc* block, re::Ptr32<HeapBlockDesc>* table) {
    block->next = after->next;
    after->next->prev = block;
    block->prev = after;
    after->next = block;
    int c = HeapBlockDesc::SizeClass(block->size);
    HeapBlockDesc* first = table[c];
    // (the list's end as a class's first is replaced by anything)
    if (!first || !first->next || first->size > block->size) {
        table[c] = block;
    }
}

void HeapBlockDesc::InsertHeapFreeBlock(HeapBlockDesc* block, re::Ptr32<HeapBlockDesc>* table) {
    InsertAfter(this, block, table);
}

HeapBlockDesc* HeapBlockDesc::FindSmallestFreeBlock(uint32_t wanted, re::Ptr32<HeapBlockDesc>* table) {
    int c = SizeClass(wanted);
    HeapBlockDesc* block = table[c];
    while (!block && c < 20) {
        block = table[c];
        c++;
    }
    while (block->size < wanted) {
        block = block->next;
    }
    return block;
}

void HeapBlockDesc::RemoveHeapFreeBlock(re::Ptr32<HeapBlockDesc>* table) {
    next->prev = prev;
    prev->next = next;
    int c = SizeClass(size);
    if (table[c] == this) {
        HeapBlockDesc* following = next;
        table[c] = SizeClass(following->size) == c ? following : nullptr;
    }
}

// ---- common blocks on their own ---------------------------------------------------------

CommonBlocksHandle* CommonBlocksHandle::Construct() {
    blocks = nullptr;
    shutDown = 0;
    return this;
}

void CommonBlocksHandle::Destruct() {
    if (CommonBlocks* cb = blocks) {
        CommonBlocks_Destruct(cb);
        Mem_Free(cb);
    }
}

void CommonBlocksHandle::Init(const char* name, CommonBlocksParams* params) {
    void* mem = Mem_Malloc(0x34);
    CommonBlocks* cb = mem ? CommonBlocks_Construct(mem) : nullptr;
    blocks = cb;
    CommonBlocks_Init(cb, name, 0, params);
}

void CommonBlocksHandle::Shutdown() {
    CommonBlocks* cb = blocks;
    shutDown = 1;
    if (cb) {
        CommonBlocks_Destruct(cb);
        Mem_Free(cb);
    }
    blocks = nullptr;
}

uint8_t CommonBlocksHandle::IsInitialised() {
    return blocks != nullptr;
}

void* CommonBlocksHandle::Malloc(uint32_t size) {
    return blocks ? CommonBlocks_Malloc(blocks, size, 0) : nullptr;
}

void CommonBlocksHandle::Free(void* ptr) {
    if (blocks) {
        CommonBlocks_Free(blocks, ptr, 0);
    }
}

uint8_t CommonBlocksHandle::Owns(void* ptr) {
    return blocks ? CommonBlocks_Owns(blocks, ptr) : 0;
}

// ---- the heap ---------------------------------------------------------------------------

CMemoryHeap* CMemoryHeap::Construct() {
    start = nullptr;
    commonBlocks = nullptr;
    commonBlocksShutDown = 0;
    *reinterpret_cast<uint32_t*>(lock) = 0;   // (x64: a new section behind it)
    InitializeCriticalSection(Lock(*this));
    commonMaxSize = -1;
    return this;
}

void CMemoryHeap::Init(void* memory, uint32_t size, int) {
    start = static_cast<HeapBlockDesc*>(memory);
    unknownC8 = -1;
    uint32_t usable = size - 0x10;
    end = reinterpret_cast<HeapBlockDesc*>(static_cast<uint8_t*>(memory) + usable);
    moveOnlyIfTight = 1;
    totalMemUsed = 0;
    peakMemUsed = 0;
    memUsed = nullptr;
    blocksUsed = nullptr;
    totalBlocksUsed = 0;
    // the end: an empty block that's never free (nothing merges past it)
    end->memId = 1;
    end->size = 0;
    end->prevConsecutive = start;
    // the rest: one free block
    start->size = usable - 0x10;
    start->memId = 0;
    start->prevConsecutive = nullptr;
    freeListEnd.size = 0x7FFFFFFF;
    freeList.prev = nullptr;
    freeListEnd.next = nullptr;
    freeListEnd.prev = &freeList;
    freeList.next = &freeListEnd;
    for (int c = 0; c < 19; c++) {
        sizeClass[c] = nullptr;
    }
    sizeClass[19] = &freeListEnd;
    InsertAfter(&freeList, start, sizeClass);
    // the tag counters, from the heap itself (uncounted)
    memUsed = static_cast<uint32_t*>(Heap_Malloc(this, 0x100, -1));
    blocksUsed = static_cast<int32_t*>(Heap_Malloc(this, 0x100, -1));
    for (int i = 0; i < 0x40; i++) {
        memUsed[i] = 0;
        blocksUsed[i] = 0;
    }
    totalSize = usable;
}

void CMemoryHeap::InitCommonBlocks(const char* name, int32_t maxSize, CommonBlocksParams* params) {
    commonMaxSize = maxSize;
    void* mem = Mem_Malloc(0x34);
    CommonBlocks* cb = mem ? CommonBlocks_Construct(mem) : nullptr;
    commonBlocks = cb;
    CommonBlocks_Init(cb, name, 0, params);
}

void CMemoryHeap::Shutdown() {
    commonBlocksShutDown = 1;
    if (CommonBlocks* cb = commonBlocks) {
        CommonBlocks_Destruct(cb);
        Mem_Free(cb);
    }
    commonBlocks = nullptr;
    CrtFree(start);
    start = nullptr;  // (end stays: Free's range check then takes anything below it)
}

void* CMemoryHeap::Release() {
    end = nullptr;
    commonBlocksShutDown = 1;
    if (CommonBlocks* cb = commonBlocks) {
        CommonBlocks_Destruct(cb);
        Mem_Free(cb);
    }
    commonBlocks = nullptr;
    void* memory = start;
    start = nullptr;
    return memory;
}

void CMemoryHeap::RegisterMalloc(HeapBlockDesc* block, int memId) {
    block->memId = int16_t(memId);
    totalBlocksUsed++;
    totalMemUsed += block->size + 0x10;
    if (int32_t(totalMemUsed) > peakMemUsed) {
        peakMemUsed = int32_t(totalMemUsed);
    }
    if (memId != -1) {
        memUsed[memId] += block->size + 0x10;
        blocksUsed[memId]++;
    }
}

void CMemoryHeap::RegisterFree(HeapBlockDesc* block) {
    totalBlocksUsed--;
    totalMemUsed -= block->size + 0x10;
    if (block->memId != -1) {
        memUsed[block->memId] -= block->size + 0x10;
        blocksUsed[block->memId]--;
    }
}

uint32_t CMemoryHeap::GetMemoryUsed(int memId) {
    return memUsed[memId];
}

HeapBlockDesc* CMemoryHeap::CombineFreeBlocks(HeapBlockDesc* block) {
    HeapBlockDesc* next = block->GetNextConsecutive();
    // (the first block has no previous one: it's never freed, the tag counters are in it)
    HeapBlockDesc* prev = block->prevConsecutive;
    if (next->memId == 0) {
        Block_Remove(next, sizeClass);
        next = next->GetNextConsecutive();
    }
    if (prev->memId == 0) {
        Block_Remove(prev, sizeClass);
        block = prev;
    }
    block->size = uint32_t(reinterpret_cast<uint8_t*>(next) - reinterpret_cast<uint8_t*>(block) - 0x10);
    next->prevConsecutive = block;
    return block;
}

void CMemoryHeap::FillInBlockData(HeapBlockDesc* block, HeapBlockDesc* next, uint32_t size, int memId) {
    auto* split = reinterpret_cast<HeapBlockDesc*>(reinterpret_cast<uint8_t*>(block) + size + 0x10);
    block->size = size;
    block->ptrListIndex = -1;
    block->unknown8 = 0;
    if (uintptr_t(split) < uintptr_t(next) - CMemoryMgr::ms_minSplitSize) {
        Heap_RegisterMalloc(this, block, memId);
        split->size = uint32_t(reinterpret_cast<uint8_t*>(next) - reinterpret_cast<uint8_t*>(split) - 0x10);
        split->memId = 0;
        split->prevConsecutive = block;
        split->unknown8 = 0;
        next->prevConsecutive = split;
        InsertFreeBlock(this, Heap_CombineFreeBlocks(this, split));
        return;
    }
    // (too little left to split off: the block keeps it)
    if (uintptr_t(split) < uintptr_t(next)) {
        block->size = uint32_t(reinterpret_cast<uint8_t*>(next) - reinterpret_cast<uint8_t*>(block) - 0x10);
    }
    next->prevConsecutive = block;
    Heap_RegisterMalloc(this, block, memId);
}

void* CMemoryHeap::Malloc(uint32_t size, int memId) {
    CMemoryMgr::ms_mallocDepth++;
    uint32_t aligned = size;
    if (size & 0xF) {
        aligned = size + (0x10 - (size & 0xF));
    }
    if (aligned == 0) {
        CMemoryMgr::ms_mallocDepth--;
        return nullptr;
    }
    // (commonBlocks is tested twice)
    if (int32_t(size) <= commonMaxSize && commonBlocks && commonBlocks) {
        if (void* p = CommonBlocks_Malloc(commonBlocks, size, 0)) {
            CMemoryMgr::ms_mallocDepth--;
            return p;
        }
    }
    for (HeapBlockDesc* block = Block_FindSmallest(&freeList, aligned, sizeClass); block != &freeListEnd;
         block = block->next) {
        if (block->size >= aligned) {
            Block_Remove(block, sizeClass);
            Heap_FillInBlockData(this, block, block->GetNextConsecutive(), aligned, memId);
            CMemoryMgr::ms_mallocDepth--;
            return block->GetDataPointer();
        }
    }
    if (this != &CMemoryMgr::ms_mainHeap) {
        CMemoryMgr::ms_mallocDepth--;
        return nullptr;
    }
    // the main heap is full: models go until there's the free space plus 250 KB
    CMemoryHeap& main = CMemoryMgr::ms_mainHeap;
    uint32_t usedBefore = main.totalMemUsed;
    MakeSpaceFor(main.totalSize - main.totalMemUsed + 0x3E800);
    if (CMemoryMgr::ms_mallocDepth > 10) {
        Fn_4302A0(1, 0);
    } else if (CMemoryMgr::ms_mallocDepth > 6) {
        Fn_4302A0(0, 1);
    }
    if (CMemoryMgr::ms_mallocDepth > 12 && usedBefore == main.totalMemUsed && freeListEnd.prev->size < aligned) {
        Fn_4302A0(1, 1);
        MakeSpaceFor(aligned);
        if (!byte_20C5BE6) {
            CMemoryMgr::ms_mallocDepth--;
            return nullptr;
        }
        Fn_6C3DE0();
    }
    // (and again, with the tag of the main thread: until it fits or it's deep enough to give up)
    void* p = Heap_MallocLocked(this, aligned);
    CMemoryMgr::ms_mallocDepth--;
    return p;
}

void* CMemoryHeap::MallocLocked(uint32_t size) {
    EnterCriticalSection(Lock(*this));
    void* p = Heap_Malloc(this, size, CMemoryMgr::ms_memoryTag);
    LeaveCriticalSection(Lock(*this));
    return p;
}

void CMemoryHeap::Free(void* ptr) {
    EnterCriticalSection(Lock(*this));
    if (ptr) {
        if (commonBlocks && CommonBlocks_Owns(commonBlocks, ptr) && !commonBlocksShutDown) {
            if (commonBlocks) {
                CommonBlocks_Free(commonBlocks, ptr, 0);
            }
            LeaveCriticalSection(Lock(*this));
            return;
        }
        HeapBlockDesc* block = HeapBlockDesc::FromData(ptr);
        // (testing, bullyre.ini "heap_check": a block freed twice, or with a damaged header,
        // stops here, where the culprit is on the stack, instead of at a later free)
        if (heapcheck::g_validateFrees) {
            HeapBlockDesc* next = block->GetNextConsecutive();
            bool bad = block->memId == 0 || (block->size & 0xF) != 0 || block->size > totalSize ||
                       uintptr_t(next) > uintptr_t(end) || next->prevConsecutive != block;
            if (bad) {
                re::Log("heap_check: bad free of %p: block size 0x%X memId %d next %p (its back pointer %p), heap %p",
                        ptr, unsigned(block->size), int(block->memId), static_cast<void*>(next),
                        uintptr_t(next) <= uintptr_t(end) ? static_cast<void*>(next->prevConsecutive) : nullptr,
                        static_cast<void*>(this));
                // the block the pointer falls in (its tag says what kind of allocation it is)
                // and the bytes there (a name? an object?), to tell what was freed
                for (HeapBlockDesc* b = start; b != nullptr && b != end; b = b->GetNextConsecutive()) {
                    if (uintptr_t(ptr) >= uintptr_t(b) && uintptr_t(ptr) < uintptr_t(b->GetNextConsecutive())) {
                        re::Log("  inside block %p: size 0x%X memId %d (ptr is +0x%X into its data)", static_cast<void*>(b),
                                unsigned(b->size), int(b->memId), unsigned(uintptr_t(ptr) - uintptr_t(b) - 0x10));
                        break;
                    }
                    if ((b->size & 0xF) != 0 || b->size > totalSize) {
                        break;
                    }
                }
                heapcheck::Report(ptr);
                const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
                re::Log("  bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X  text \"%.16s\"",
                        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9],
                        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15], reinterpret_cast<const char*>(ptr));
                // the callers (what the stack walk gets without frame pointers), then a crash
                // report through the null write: bullygfx's handler writes bullygfx_crash.txt
                void* frames[24];
                USHORT n = CaptureStackBackTrace(0, 24, frames, nullptr);
                for (USHORT i = 0; i < n; i++) {
                    HMODULE module = nullptr;
                    char name[MAX_PATH] = "?";
                    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                           static_cast<LPCSTR>(frames[i]), &module)) {
                        GetModuleFileNameA(module, name, MAX_PATH);
                    }
                    const char* base = strrchr(name, '\\');
                    re::Log("  frame %u: %s+0x%X", unsigned(i), base ? base + 1 : name,
                            unsigned(uintptr_t(frames[i]) - uintptr_t(module)));
                }
                LeaveCriticalSection(Lock(*this));
                *static_cast<volatile int*>(nullptr) = 0;
                EnterCriticalSection(Lock(*this));
            }
        }
        Heap_RegisterFree(this, block);
        block->memId = 0;
        InsertFreeBlock(this, Heap_CombineFreeBlocks(this, block));
    }
    LeaveCriticalSection(Lock(*this));
}

HeapBlockDesc* CMemoryHeap::WhereShouldMemoryMove(void* ptr) {
    EnterCriticalSection(Lock(*this));
    HeapBlockDesc* block = HeapBlockDesc::FromData(ptr);
    uint32_t size = block->size;
    auto* next = reinterpret_cast<HeapBlockDesc*>(static_cast<uint8_t*>(ptr) + size);
    if (uintptr_t(next) < uintptr_t(end)) {
        // the space it would free up, with its free neighbours
        uint32_t space = size;
        if (next->memId == 0) {
            space += next->size + 0x10;
        }
        HeapBlockDesc* prev = block->prevConsecutive;
        if (prev->memId == 0) {
            space += prev->size + 0x10;
        }
        if (space != size) {
            HeapBlockDesc* best = Block_FindSmallest(&freeList, size, sizeClass);
            if (best->size < space - 0x10) {
                // (the game compares on the x87)
                if (!moveOnlyIfTight || double(size) * 1.25 + 16.0 > double(best->size)) {
                    LeaveCriticalSection(Lock(*this));
                    return best;
                }
            }
        }
    }
    LeaveCriticalSection(Lock(*this));
    return nullptr;
}

void* CMemoryHeap::MoveHeapBlock(HeapBlockDesc* dst, HeapBlockDesc* src) {
    Block_Remove(dst, sizeClass);
    Heap_FillInBlockData(this, dst, dst->GetNextConsecutive(), src->size, src->memId);
    std::memcpy(dst->GetDataPointer(), src->GetDataPointer(), src->size);
    CMemoryMgr::ms_bytesMoved += src->size;
    dst->ptrListIndex = src->ptrListIndex;
    src->ptrListIndex = -1;
    Heap_Free(this, src->GetDataPointer());
    return dst->GetDataPointer();
}

void* CMemoryHeap::MoveAllocation(void* ptr) {
    EnterCriticalSection(Lock(*this));
    if (InHeap(*this, ptr) && !(commonBlocks && CommonBlocks_Owns(commonBlocks, ptr))) {
        if (HeapBlockDesc* to = Heap_WhereShouldMemoryMove(this, ptr)) {
            void* moved = Heap_MoveHeapBlock(this, to, HeapBlockDesc::FromData(ptr));
            LeaveCriticalSection(Lock(*this));
            return moved;
        }
    }
    LeaveCriticalSection(Lock(*this));
    return ptr;
}

void CMemoryHeap::ParseHeap() {
    EnterCriticalSection(Lock(*this));
    // (the free list walked twice, for nothing)
    for (HeapBlockDesc* b = freeList.next; b != &freeListEnd; b = b->next) {
    }
    for (HeapBlockDesc* b = freeList.next; b != &freeListEnd; b = b->next) {
    }
    uint32_t byTag[0x40] = {};
    for (HeapBlockDesc* b = start; b != end; b = b->GetNextConsecutive()) {
        if (b->memId != 0 && b->memId != -1) {
            byTag[b->memId] += b->size + 0x10;
        }
    }
    for (HeapBlockDesc* b = end->prevConsecutive; b; b = b->prevConsecutive) {
    }
    (void)byTag;
    LeaveCriticalSection(Lock(*this));
}

// ---- CMemoryMgr ---------------------------------------------------------------------------

namespace CMemoryMgr {

void __cdecl UpdateMemoryStatus() {
    MEMORYSTATUS status;
    GlobalMemoryStatus(&status);
}

void __cdecl Init() {
    MEMORYSTATUS before, after;
    GlobalMemoryStatus(&before);
    ms_bInitialised = 1;
    ms_simpleHeapSize = 0x18000000;
    ms_mainHeapSize = 0x4000000;
    GlobalMemoryStatus(&after);
    Heap_Init(&ms_mainHeap, CrtMalloc(ms_mainHeapSize), ms_mainHeapSize, 0);
    CommonBlocksParams params = {0x1000, 0xA00000, 8, 0x120, 8, 0};
    Mem_PushMemoryTag(8);
    Heap_InitCommonBlocks(&ms_mainHeap, "MainHeapCommonBlock", 0x120, &params);
    Mem_PopMemoryTag();
    Heap_Init(&ms_simpleHeap, CrtMalloc(ms_simpleHeapSize), ms_simpleHeapSize, 0);
    params = {0x1000, 0x1000000, 8, 0x100, 8, 0};
    Heap_InitCommonBlocks(&ms_simpleHeap, "SimpleHeapCommonBlock", 0x100, &params);
    Fn_5D7F50();
    // the other two inside the general one
    Mem_PushMemoryTag(0x28);
    EnterCriticalSection(Lock(ms_simpleHeap));
    void* memory = Heap_Malloc(&ms_simpleHeap, 0x5BCC0, ms_memoryTag);
    LeaveCriticalSection(Lock(ms_simpleHeap));
    Heap_Init(&ms_heap28, memory, 0x5BCC0, 1);
    Mem_PopMemoryTag();
    params = {0x400, 0x14000, 4, 0x50, 4, 0};
    Mem_PushMemoryTag(0x26);
    EnterCriticalSection(Lock(ms_simpleHeap));
    memory = Heap_Malloc(&ms_simpleHeap, 250000, ms_memoryTag);
    LeaveCriticalSection(Lock(ms_simpleHeap));
    Heap_Init(&ms_actionTreeHeap, memory, 250000, 0);
    Heap_InitCommonBlocks(&ms_actionTreeHeap, "ActionTreeHeapCommonBlock", 0x50, &params);
    Mem_PopMemoryTag();
    GlobalMemoryStatus(&before);
}

void __cdecl Shutdown() {
    Heap_Free(&ms_simpleHeap, Heap_Release(&ms_actionTreeHeap));
    Heap_Free(&ms_simpleHeap, Heap_Release(&ms_heap28));
    CommonBlocksHandle_Shutdown(&commonBlocksCFC9B8);
    Heap_Shutdown(&ms_simpleHeap);
    Heap_Shutdown(&ms_mainHeap);
}

// under a heap's lock (the heap's own Malloc doesn't lock)
static void* LockedMalloc(CMemoryHeap& heap, size_t size) {
    EnterCriticalSection(Lock(heap));
    void* p = Heap_Malloc(&heap, uint32_t(size), ms_memoryTag);
    LeaveCriticalSection(Lock(heap));
    return p;
}

void* __cdecl Malloc(size_t size) {
    if (!ms_bInitialised) {
        Mem_Init();
    }
    int32_t tag = Mem_GetMemoryTag();
    void* p = nullptr;
    // (a full tag 0x28 or action tree heap falls back to the general one)
    if (tag == 0x28) {
        p = LockedMalloc(ms_heap28, size);
        if (p) {
            std::memset(p, 0, size);  // (not counted in the highest address)
            return p;
        }
    } else if (tag == 0x26) {
        p = LockedMalloc(ms_actionTreeHeap, size);
        if (p) {
            std::memset(p, 0, size);
            return p;
        }
    } else if (tag >= 8 && tag <= 0x16) {
        p = Heap_MallocLocked(&ms_mainHeap, uint32_t(size));
        if (!p) {
            return nullptr;
        }
    }
    if (!p) {
        p = LockedMalloc(ms_simpleHeap, size);
        if (!p) {
            return nullptr;
        }
    }
    if (uintptr_t(p) + size > ms_highestAddress) {
        ms_highestAddress = uintptr_t(p) + size;
    }
    std::memset(p, 0, size);
    return p;
}

void* __cdecl MallocWrapper(size_t size) {
    return Mem_MallocDirect(size);
}

void __cdecl Free(void* ptr) {
    if (!ptr) {
        return;
    }
    if (InHeap(ms_heap28, ptr)) {
        Heap_Free(&ms_heap28, ptr);
    } else if (InHeap(ms_actionTreeHeap, ptr)) {
        Heap_Free(&ms_actionTreeHeap, ptr);
    } else if (InHeap(ms_mainHeap, ptr)) {
        Heap_Free(&ms_mainHeap, ptr);
    } else if (InHeap(ms_simpleHeap, ptr)) {
        Heap_Free(&ms_simpleHeap, ptr);
    }
}

void __cdecl FreeIfAny(void* ptr) {
    if (ptr) {
        Mem_FreeDirect(ptr);
    }
}

void* __cdecl MallocAlign(size_t size, size_t align) {
    int32_t tag = Mem_GetMemoryTag();
    CMemoryHeap& heap = uint32_t(tag - 8) <= 0xE ? ms_mainHeap : ms_simpleHeap;
    // (no check for running out)
    void* p = LockedMalloc(heap, size + align);
    uintptr_t aligned = (uintptr_t(p) + align) & ~(align - 1);
    reinterpret_cast<re::Ptr32<void>*>(aligned)[-1] = p;
    return reinterpret_cast<void*>(aligned);
}

void __cdecl FreeAlign(void* ptr) {
    void* p = static_cast<re::Ptr32<void>*>(ptr)[-1];
    // (only the two heaps MallocAlign uses)
    Heap_Free(InHeap(ms_mainHeap, p) ? &ms_mainHeap : &ms_simpleHeap, p);
}

int32_t __cdecl GetMemoryTag() {
    return IsStreamingThread() ? ms_streamingMemoryTag : ms_memoryTag;
}

void __cdecl PushMemoryTag(int32_t tag) {
    if (IsMainThread()) {
        ms_tagStack[ms_tagStackSize] = ms_memoryTag;
        ms_tagStackSize++;
        ms_memoryTag = tag;
        return;
    }
    if (IsStreamingThread()) {
        // (it saves the main thread's tag, which its Pop then restores as its own)
        ms_streamingTagStack[ms_streamingTagStackSize] = ms_memoryTag;
        ms_streamingTagStackSize++;
        ms_streamingMemoryTag = tag;
    }
}

int32_t __cdecl PopMemoryTag() {
    if (IsMainThread()) {
        int32_t tag = ms_memoryTag;
        ms_tagStackSize--;
        ms_memoryTag = ms_tagStack[ms_tagStackSize];
        return tag;
    }
    if (IsStreamingThread()) {
        int32_t tag = ms_streamingMemoryTag;
        ms_streamingTagStackSize--;
        ms_streamingMemoryTag = ms_streamingTagStack[ms_streamingTagStackSize];
        return tag;
    }
    return 0;
}

} // namespace CMemoryMgr

// ---- registration -----------------------------------------------------------------------

#define MM_REVERSED(name, address, fn)                                                            \
    static void* RE_CONCAT(mm_original_, __LINE__);                                              \
    REVERSED(name, address, fn, nullptr, &RE_CONCAT(mm_original_, __LINE__), re::Mode::Original)
#define MM_FN(ns, fn, address) MM_REVERSED(#ns "::" #fn, address, RE_BODY(&ns::fn))
#define MM_MEMBER(cls, fn, address) MM_REVERSED(#cls "::" #fn, address, re::MemberAddress(&cls::fn))

MM_FN(CMemoryMgr, UpdateMemoryStatus, 0x005EE580);
MM_FN(CMemoryMgr, Shutdown, 0x005EE5A0);
MM_FN(CMemoryMgr, Init, 0x005EE630);
MM_FN(CMemoryMgr, Malloc, 0x005EE830);
MM_FN(CMemoryMgr, Free, 0x005EE940);
MM_FN(CMemoryMgr, MallocAlign, 0x005EE9C0);
MM_FN(CMemoryMgr, MallocWrapper, 0x005EEA50);
MM_FN(CMemoryMgr, FreeAlign, 0x005EEA70);
MM_FN(CMemoryMgr, FreeIfAny, 0x005EEAB0);
MM_FN(CMemoryMgr, GetMemoryTag, 0x005EEDE0);
MM_FN(CMemoryMgr, PushMemoryTag, 0x005EF0A0);
MM_FN(CMemoryMgr, PopMemoryTag, 0x005EF100);

MM_MEMBER(CommonBlocksHandle, Construct, 0x005EEAD0);
MM_MEMBER(CommonBlocksHandle, Init, 0x005EEAE0);
MM_MEMBER(CommonBlocksHandle, Shutdown, 0x005EEB20);
MM_MEMBER(CommonBlocksHandle, IsInitialised, 0x005EEB50);
MM_MEMBER(CommonBlocksHandle, Malloc, 0x005EEB60);
MM_MEMBER(CommonBlocksHandle, Free, 0x005EEB80);
MM_MEMBER(CommonBlocksHandle, Owns, 0x005EEBA0);
MM_MEMBER(CommonBlocksHandle, Destruct, 0x005EEE00);

MM_MEMBER(HeapBlockDesc, InsertHeapFreeBlock, 0x005EEBF0);
MM_MEMBER(HeapBlockDesc, FindSmallestFreeBlock, 0x005EEC50);
MM_MEMBER(HeapBlockDesc, RemoveHeapFreeBlock, 0x005EECA0);

MM_MEMBER(CMemoryHeap, MallocLocked, 0x005EE5F0);
MM_MEMBER(CMemoryHeap, InitCommonBlocks, 0x005EEBB0);
MM_MEMBER(CMemoryHeap, CombineFreeBlocks, 0x005EED20);
MM_MEMBER(CMemoryHeap, RegisterFree, 0x005EED80);
MM_MEMBER(CMemoryHeap, GetMemoryUsed, 0x005EEDD0);
MM_MEMBER(CMemoryHeap, Construct, 0x005EEE20);
MM_MEMBER(CMemoryHeap, Shutdown, 0x005EEE50);
MM_MEMBER(CMemoryHeap, Release, 0x005EEEA0);
MM_MEMBER(CMemoryHeap, Free, 0x005EEEF0);
MM_MEMBER(CMemoryHeap, ParseHeap, 0x005EEF90);
MM_MEMBER(CMemoryHeap, RegisterMalloc, 0x005EF040);
MM_MEMBER(CMemoryHeap, WhereShouldMemoryMove, 0x005EF160);
MM_MEMBER(CMemoryHeap, FillInBlockData, 0x005EF240);
MM_MEMBER(CMemoryHeap, Malloc, 0x005EF2F0);
MM_MEMBER(CMemoryHeap, MoveHeapBlock, 0x005EF460);
MM_MEMBER(CMemoryHeap, MoveAllocation, 0x005EF4D0);
MM_MEMBER(CMemoryHeap, Init, 0x005EF540);
