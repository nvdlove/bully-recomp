#pragma once

// CMemoryMgr: the game's own heaps, carved out of a few big CRT allocations at start.
// Every game malloc/free goes through here (0x5EEAA0 / 0x5EEAB0). Which heap an
// allocation comes from depends on the memory tag it is made under (Push/PopMemoryTag).
// Each heap (CMemoryHeap, as in Rockstar's PS2-era code) keeps its blocks back to back,
// the free ones in a list sorted by size with a table of where each size class starts;
// small sizes come from its "common blocks" (fixed-size pools, 0x5EDA70...).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

// A block's header, right before the data (0x10 bytes). Free blocks keep their free list
// links in the first 8 bytes of the data.
struct HeapBlockDesc {
    uint32_t size;                    // +0 the data's (a multiple of 16)
    int16_t memId;                    // +4 the tag it was allocated under; 0: free (-1: none)
    int16_t ptrListIndex;             // +6 (-1)
    uint32_t unknown8;                // +8 (0)
    re::Ptr32<HeapBlockDesc> prevConsecutive;   // +0xC the block before it in memory (nullptr: the first)
    re::Ptr32<HeapBlockDesc> next;              // +0x10 free blocks: the next bigger one
    re::Ptr32<HeapBlockDesc> prev;              // +0x14

    void* GetDataPointer() { return reinterpret_cast<uint8_t*>(this) + 0x10; }
    HeapBlockDesc* GetNextConsecutive() {
        return reinterpret_cast<HeapBlockDesc*>(reinterpret_cast<uint8_t*>(this) + 0x10 + size);
    }
    static HeapBlockDesc* FromData(void* data) {
        return reinterpret_cast<HeapBlockDesc*>(reinterpret_cast<uint8_t*>(data) - 0x10);
    }

    // the free list's size class of a size: 0 under 32 bytes, then one per power of 2 up to 19
    static int SizeClass(uint32_t size);
    // 0x5EEBF0 (thiscall): `block` into the free list after this one, and first of its
    // class in `table` if it's the smallest there
    void InsertHeapFreeBlock(HeapBlockDesc* block, re::Ptr32<HeapBlockDesc>* table);
    // 0x5EEC50 (thiscall, this unused): the first free block of at least `size`
    // (the list's end if none)
    HeapBlockDesc* FindSmallestFreeBlock(uint32_t size, re::Ptr32<HeapBlockDesc>* table);
    // 0x5EECA0 (thiscall): out of the free list (and the class table)
    void RemoveHeapFreeBlock(re::Ptr32<HeapBlockDesc>* table);
};
static_assert(offsetof(HeapBlockDesc, memId) == 4);
static_assert(offsetof(HeapBlockDesc, prevConsecutive) == 0xC);
static_assert(offsetof(HeapBlockDesc, next) == 0x10);
static_assert(sizeof(HeapBlockDesc) == 0x18);

// The fixed-size pools a heap serves small sizes from (0x34 bytes, not reversed yet)
struct CommonBlocks;

// The sizes the common blocks are set up with (six numbers)
struct CommonBlocksParams {
    int32_t unknown0;   // +0 (0x1000, 0x400)
    int32_t unknown4;   // +4 the pools' memory (0xA00000 ...)
    int32_t unknown8;   // +8 (8, 4)
    int32_t maxSize;    // +0xC (as InitCommonBlocks')
    int32_t unknown10;  // +0x10 (8, 4)
    int32_t unknown14;  // +0x14 (0)
};

// A common blocks object on its own (the one at 0xCFC9B8)
struct CommonBlocksHandle {
    re::Ptr32<CommonBlocks> blocks;  // +0
    uint8_t shutDown;                // +4

    // 0x5EEAD0
    CommonBlocksHandle* Construct();
    // 0x5EEE00: the destructor (doesn't clear the pointer)
    void Destruct();
    // 0x5EEAE0
    void Init(const char* name, CommonBlocksParams* params);
    // 0x5EEB20
    void Shutdown();
    // 0x5EEB50
    uint8_t IsInitialised();
    // 0x5EEB60
    void* Malloc(uint32_t size);
    // 0x5EEB80
    void Free(void* ptr);
    // 0x5EEBA0
    uint8_t Owns(void* ptr);
};
static_assert(sizeof(CommonBlocksHandle) == 8);

struct CMemoryHeap {
    int32_t commonMaxSize;           // +0 sizes up to this come from the common blocks (-1: none)
    re::Ptr32<HeapBlockDesc> start;            // +4
    re::Ptr32<HeapBlockDesc> end;              // +8 a last, empty block that is never free (memId 1)
    HeapBlockDesc freeList;          // +0xC the free list's head
    HeapBlockDesc freeListEnd;       // +0x24 and end (size 0x7FFFFFFF)
    re::Ptr32<HeapBlockDesc> sizeClass[20];    // +0x3C each size class's first free block (the last: the end)
    re::Ptr32<CommonBlocks> commonBlocks;      // +0x8C
    uint8_t commonBlocksShutDown;    // +0x90 frees no longer go to them
    uint32_t totalMemUsed;           // +0x94 (headers included)
    uint32_t totalSize;              // +0x98
    uint8_t moveOnlyIfTight;         // +0x9C MoveAllocation: not into a much bigger block
    int32_t peakMemUsed;             // +0xA0
    uint8_t lock[0x18];              // +0xA4 CRITICAL_SECTION
    re::Ptr32<uint32_t> memUsed;               // +0xBC by tag (64 of them)
    int32_t totalBlocksUsed;         // +0xC0
    re::Ptr32<int32_t> blocksUsed;             // +0xC4 by tag
    int32_t unknownC8;               // +0xC8 (-1)
    int32_t unknownCC;               // +0xCC

    // 0x5EEE20: the constructor
    CMemoryHeap* Construct();
    // 0x5EF540: the heap over `size` bytes at `memory` (the flag is unused)
    void Init(void* memory, uint32_t size, int unused);
    // 0x5EEBB0
    void InitCommonBlocks(const char* name, int32_t maxSize, CommonBlocksParams* params);
    // 0x5EEE50: frees the heap's memory (to the CRT)
    void Shutdown();
    // 0x5EEEA0: gives the heap's memory back (to be freed elsewhere)
    void* Release();
    // 0x5EF2F0: `size` bytes counted under `memId` (the caller holds the lock). The main
    // heap makes room by removing streamed models when it's full.
    void* Malloc(uint32_t size, int memId);
    // 0x5EE5F0: Malloc under the lock, with the current tag
    void* MallocLocked(uint32_t size);
    // 0x5EEEF0
    void Free(void* ptr);
    // 0x5EF4D0: the allocation moved to a better place (defragmenting), where it is now
    void* MoveAllocation(void* ptr);  // (GTA's MoveMemory, a Win32 macro)
    // 0x5EF160: a free block the allocation would better be in, nullptr: none
    HeapBlockDesc* WhereShouldMemoryMove(void* ptr);
    // 0x5EF460: `src`'s allocation into the free block `dst`, its data there
    void* MoveHeapBlock(HeapBlockDesc* dst, HeapBlockDesc* src);
    // 0x5EF240: `block` (free, up to `next`) allocated with `size`; the rest split off
    // into a free block if it's big enough
    void FillInBlockData(HeapBlockDesc* block, HeapBlockDesc* next, uint32_t size, int memId);
    // 0x5EED20: a free block merged with its free neighbours, the result
    HeapBlockDesc* CombineFreeBlocks(HeapBlockDesc* block);
    // 0x5EF040
    void RegisterMalloc(HeapBlockDesc* block, int memId);
    // 0x5EED80
    void RegisterFree(HeapBlockDesc* block);
    // 0x5EEDD0
    uint32_t GetMemoryUsed(int memId);
    // 0x5EEF90: walks the heap for statistics it then drops (a debug leftover)
    void ParseHeap();
};
static_assert(offsetof(CMemoryHeap, freeList) == 0xC);
static_assert(offsetof(CMemoryHeap, freeListEnd) == 0x24);
static_assert(offsetof(CMemoryHeap, sizeClass) == 0x3C);
static_assert(offsetof(CMemoryHeap, commonBlocks) == 0x8C);
static_assert(offsetof(CMemoryHeap, totalMemUsed) == 0x94);
static_assert(offsetof(CMemoryHeap, lock) == 0xA4);
static_assert(offsetof(CMemoryHeap, memUsed) == 0xBC);
static_assert(offsetof(CMemoryHeap, blocksUsed) == 0xC4);
static_assert(sizeof(CMemoryHeap) == 0xD0);

namespace CMemoryMgr {

// The heaps: the main one (tags 8..0x16), the general one (everything else), one for tag
// 0x28 and the action trees' (0x26), those two inside the general one
GAME_VAR(CMemoryHeap, ms_mainHeap, 0x00D14170);
GAME_VAR(CMemoryHeap, ms_simpleHeap, 0x00D14240);
GAME_VAR(CMemoryHeap, ms_heap28, 0x00D14310);
GAME_VAR(CMemoryHeap, ms_actionTreeHeap, 0x00D143E0);
GAME_VAR(uint8_t, ms_bInitialised, 0x00D1416C);
GAME_VAR(uint32_t, ms_mainHeapSize, 0x00CA3980);
GAME_VAR(uint32_t, ms_simpleHeapSize, 0x00D14158);
GAME_VAR(uint32_t, ms_highestAddress, 0x00D14164);  // the end of the highest allocation
GAME_VAR(int32_t, ms_mallocDepth, 0x00D144B4);        // CMemoryHeap::Malloc's recursion
GAME_VAR(uint32_t, ms_bytesMoved, 0x00D144B8);        // by MoveAllocation
GAME_VAR(uint32_t, ms_minSplitSize, 0x00B0A23C);      // smaller leftovers stay with the block (32)
// the tags: the main thread's and the streaming thread's, each with a stack
GAME_VAR(int32_t, ms_memoryTag, 0x00B0A238);
GAME_VAR(int32_t, ms_streamingMemoryTag, 0x00D144B0);
GAME_ARRAY(int32_t, ms_tagStack, 0x00D144C0);
GAME_VAR(int32_t, ms_tagStackSize, 0x00D14500);
GAME_ARRAY(int32_t, ms_streamingTagStack, 0x00D14508);
GAME_VAR(int32_t, ms_streamingTagStackSize, 0x00D14548);

// 0x5EE630: the heaps (on the first Malloc)
void __cdecl Init();
// 0x5EE5A0
void __cdecl Shutdown();
// 0x5EE580: (reads the system's memory status and drops it)
void __cdecl UpdateMemoryStatus();
// 0x5EE830 (and 0x5EEAA0, 0x5EEA50): zeroed memory from the tag's heap; nullptr: none
void* __cdecl Malloc(size_t size);
// 0x5EEA50
void* __cdecl MallocWrapper(size_t size);
// 0x5EE940: to the heap the pointer is in (nothing if none)
void __cdecl Free(void* ptr);
// 0x5EEAB0
void __cdecl FreeIfAny(void* ptr);
// 0x5EE9C0: `align` a power of 2; the real pointer kept before the returned one
void* __cdecl MallocAlign(size_t size, size_t align);
// 0x5EEA70
void __cdecl FreeAlign(void* ptr);
// 0x5EEDE0: the current thread's tag
int32_t __cdecl GetMemoryTag();
// 0x5EF0A0
void __cdecl PushMemoryTag(int32_t tag);
// 0x5EF100: the tag that was current (0 off the main and streaming threads)
int32_t __cdecl PopMemoryTag();

} // namespace CMemoryMgr
