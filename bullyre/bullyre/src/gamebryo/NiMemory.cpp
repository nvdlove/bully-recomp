// Gamebryo's memory: NiMalloc/NiFree and NiMemObject's operators (all through the allocator
// NiMemManager holds), NiStaticDataManager (the library's start-up and shut-down), the
// default allocator Bully installs (NiStandardAllocator: the small sizes from a Loki-style
// NiSmallObjectAllocator, the rest from _aligned_malloc, a 4-byte size header in front of what
// it can't know the size of), NiSystemDesc and the timer.

#include <cstring>

#include <windows.h>

#include "game/HeapCheck.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

// NiMemEventType
enum : int {
    kOperNew = 1,
    kOperNewArray = 2,
    kOperDelete = 3,
    kOperDeleteArray = 4,
    kMalloc = 5,
    kRealloc = 6,
    kAlignedMalloc = 7,
    kAlignedRealloc = 8,
    kFree = 9,
    kAlignedFree = 10,
};

// NiMemManager::ms_pkMemManager: an object holding just the allocator (+0x0)
GAME_VAR(re::Ptr32<void>*, ms_pkMemManager, 0x020E2158);

// NiAllocator's slots: 0 the destructor, 1 Allocate(size&, alignment&, event, accurateSize,
// file, line, function), 2 Deallocate(p, event, size), 3 Reallocate(p, size&, alignment&,
// event, accurateSize, currentSize, file, line, function), 6 Initialize, 7 Shutdown
inline void* Allocator() {
    return *ms_pkMemManager;
}
inline void* Allocate(uint32_t* size, uint32_t* alignment, int event, int accurate) {
    return Virtual<void*>(Allocator(), 1, size, alignment, event, accurate, static_cast<const char*>(nullptr), -1,
                          static_cast<const char*>(nullptr));
}
inline void Deallocate(void* p, int event, uint32_t size) {
    Virtual<void>(Allocator(), 2, p, event, size);
}

// std::bad_alloc thrown as the library's operator new does (std::exception's "no copy"
// constructor, 0x85BE30: the message pointer kept, nothing to free; then bad_alloc's vtable)
[[noreturn]] void ThrowBadAlloc() {
    alignas(4) uint32_t error[3];
    error[0] = 0x008FFF1C;                            // std::bad_alloc's vtable
    error[1] = *reinterpret_cast<uint32_t*>(0x00BC74E8);   // "bad allocation"
    error[2] = 0;
    crt::CxxThrowException(error, reinterpret_cast<const void*>(0x009C1BA0));
    __assume(0);
}

// Bully's own heap (CMemoryMgr), which the library's long-lived objects come from
GAME_FN(void*, __cdecl, CMemoryMgr_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, CMemoryMgr_Free, 0x005EEAB0, void* p);

} // namespace

// ---- NiMalloc and friends

// 0x752300
void* __cdecl NiMallocImpl(uint32_t size) {
    uint32_t bytes = size != 0 ? size : 1;
    uint32_t alignment = 4;
    return Allocate(&bytes, &alignment, kMalloc, 0);
}

// 0x752340
void* __cdecl NiAlignedMalloc(uint32_t size, uint32_t alignment) {
    uint32_t bytes = size != 0 ? size : 1;
    uint32_t align = alignment;
    return Allocate(&bytes, &align, kAlignedMalloc, 0);
}

// 0x752380
void __cdecl NiFreeImpl(void* p) {
    if (p != nullptr) {
        Deallocate(p, kFree, 0xFFFFFFFF);
    }
}

// 0x7523A0
void __cdecl NiAlignedFree(void* p) {
    if (p != nullptr) {
        Deallocate(p, kAlignedFree, 0xFFFFFFFF);
    }
}

// 0x7523C0: no size frees (and returns null); no block allocates
void* __cdecl NiRealloc(void* p, uint32_t size) {
    if (size == 0) {
        if (p != nullptr) {
            Deallocate(p, kFree, 0xFFFFFFFF);
            return nullptr;
        }
    } else if (p != nullptr) {
        uint32_t bytes = size;
        uint32_t alignment = 4;
        return Virtual<void*>(Allocator(), 3, p, &bytes, &alignment, kRealloc, 0, 0xFFFFFFFFu,
                              static_cast<const char*>(nullptr), -1, static_cast<const char*>(nullptr));
    }
    uint32_t bytes = size != 0 ? size : 1;
    uint32_t alignment = 4;
    return Allocate(&bytes, &alignment, kMalloc, 0);
}

// 0x752460: NiMemObject::operator new (the size given back on delete)
void* __cdecl NiMemObject_New(uint32_t size) {
    uint32_t bytes = size != 0 ? size : 1;
    uint32_t alignment = 4;
    void* p = Allocate(&bytes, &alignment, kOperNew, 1);
    if (p == nullptr) {
        ThrowBadAlloc();
    }
    return p;
}

// 0x7524D0: NiMemObject::operator new[]
void* __cdecl NiMemObject_NewArray(uint32_t size) {
    uint32_t bytes = size != 0 ? size : 1;
    uint32_t alignment = 4;
    void* p = Allocate(&bytes, &alignment, kOperNewArray, 0);
    if (p == nullptr) {
        ThrowBadAlloc();
    }
    return p;
}

// 0x752540: NiMemObject::operator delete
void __cdecl NiMemObject_Delete(void* p, uint32_t size) {
    if (p != nullptr) {
        Deallocate(p, kOperDelete, size);
    }
}

// 0x752570: NiMemObject::operator delete[]
void __cdecl NiMemObject_DeleteArray(void* p) {
    if (p != nullptr) {
        Deallocate(p, kOperDeleteArray, 0xFFFFFFFF);
    }
}

// ---- NiStaticDataManager: the library's init and shutdown functions, registered by its static
// initializers

namespace {
GAME_VAR(uint8_t, sdm_initialized, 0x020E2138);
GAME_VAR(uint8_t, sdm_ownsInitOptions, 0x020E2139);
GAME_VAR(void*, sdm_initOptions, 0x020E2144);
using VoidFn = void(__cdecl*)();
GAME_VAR(VoidFn, sdm_preInitCallback, 0x020E213C);
GAME_VAR(VoidFn, sdm_postShutdownCallback, 0x020E2140);
GAME_VAR(uint32_t, sdm_numEntries, 0x020E2130);
GAME_VAR(uint32_t, sdm_initIndex, 0x020E2134);
GAME_ARRAY(VoidFn, sdm_shutdownFunctions, 0x020E20B0);
GAME_FN(void, __thiscall, NiInitOptions_Construct, 0x00753870, void* self);
GAME_FN(void, __thiscall, NiInitOptions_Destruct, 0x00753850, void* self);
GAME_FN(void, __cdecl, NiSystemSDM_Init, 0x00753C30);
GAME_FN(void, __cdecl, NiSystemSDM_Shutdown, 0x00753C50);
// the initializers not run yet, from sdm_initIndex on (the tail of Init)
GAME_FN(void, __cdecl, NiStaticDataManager_ProcessAccumulatedLibraries, 0x007536B0);
GAME_ARRAY(VoidFn, sdm_initFunctions, 0x020E20F0);

// 0x7536B0: ProcessAccumulatedLibraries: once Init has run, the libraries' init functions added
// since the last time (sdm_initIndex up to the count, the count read again after each, as an
// init may add more), the index then the count. (Ghidra's list missed its start: written for
// the 64-bit dispatch, not hooked in the 32-bit game)
void __cdecl ProcessAccumulatedLibrariesImpl() {
    if (!sdm_initialized) {
        return;
    }
    uint32_t count = sdm_numEntries;
    for (uint32_t i = sdm_initIndex; i < count; i++) {
        sdm_initFunctions[i]();
        count = sdm_numEntries;
    }
    sdm_initIndex = count;
}
} // namespace
RE_UNHOOKED_FN("NiStaticDataManager::ProcessAccumulatedLibraries", 0x007536B0, &ProcessAccumulatedLibrariesImpl);

// 0x7536F0: no options: its own (with Bully's default allocator), freed at Shutdown
void __cdecl NiStaticDataManager_Init(void* options) {
    if (options == nullptr) {
        void* own = CMemoryMgr_Malloc(8);
        if (own != nullptr) {
            NiInitOptions_Construct(own);
        }
        sdm_ownsInitOptions = 1;
        options = own;
    } else {
        sdm_ownsInitOptions = 0;
    }
    sdm_initOptions = options;
    NiSystemSDM_Init();
    if (sdm_preInitCallback != nullptr) {
        sdm_preInitCallback();
    }
    sdm_initialized = 1;
    NiStaticDataManager_ProcessAccumulatedLibraries();
}

// 0x753750: the shutdown functions in reverse
void __cdecl NiStaticDataManager_Shutdown() {
    for (int i = int(sdm_numEntries) - 1; i >= 0; i--) {
        sdm_shutdownFunctions[i]();
    }
    sdm_initIndex = 0;
    if (sdm_postShutdownCallback != nullptr) {
        sdm_postShutdownCallback();
    }
    NiSystemSDM_Shutdown();
    if (sdm_ownsInitOptions && sdm_initOptions != nullptr) {
        void* options = sdm_initOptions;
        NiInitOptions_Destruct(options);
        CMemoryMgr_Free(options);
    }
    sdm_initialized = 0;
}

// 0x7537B0: NiGetCurrentTimeInSec, seconds since its first call (as a float; the x87 divides
// the two 64-bit counts and rounds once to float: the double division here rounds twice,
// which can differ in the last bit at an exact tie)
namespace {
GAME_VAR(uint8_t, timer_first, 0x00BC754E);
GAME_VAR(int64_t, timer_start, 0x020E2148);
GAME_VAR(int64_t, timer_frequency, 0x020E2150);
} // namespace
float __cdecl NiGetCurrentTimeInSec() {
    if (timer_first) {
        Import<int>(kQueryPerformanceFrequency, &timer_frequency);
        Import<int>(kQueryPerformanceCounter, &timer_start);
        timer_first = 0;
    }
    int64_t now;
    Import<int>(kQueryPerformanceCounter, &now);
    return float(double(now - timer_start) / double(timer_frequency));
}

// 0x753820 and 0x753830: lone jumps to _stricmp and _strnicmp (the library's NiStricmp /
// NiStrnicmp)
RE_UNHOOKED("NiStricmp", 0x00753820);
RE_UNHOOKED("NiStrnicmp", 0x00753830);

// ---- NiInitOptions: the allocator, and whether it's its own

struct NiInitOptions {
    re::Ptr32<void> allocator;          // +0x0
    uint8_t ownsAllocator;    // +0x4
};

// 0x753840
NiInitOptions* __fastcall NiInitOptions_ConstructWith(NiInitOptions* self, re::Edx, void* allocator) {
    self->allocator = allocator;
    self->ownsAllocator = 0;
    return self;
}

// 0x753850
void __fastcall NiInitOptions_DestructImpl(NiInitOptions* self, re::Edx) {
    if (self->ownsAllocator && self->allocator != nullptr) {
        Virtual<void>(self->allocator, 0, 1);
    }
}

// 0x753870: Bully's NiStandardAllocator (vtable 0x951000), from its own heap
NiInitOptions* __fastcall NiInitOptions_ConstructDefault(NiInitOptions* self, re::Edx) {
    re::Ptr32<void>* allocator = static_cast<re::Ptr32<void>*>(CMemoryMgr_Malloc(4));
    if (allocator != nullptr) {
        *allocator = reinterpret_cast<void*>(0x00951000);
        self->allocator = allocator;
    } else {
        self->allocator = nullptr;
    }
    self->ownsAllocator = 1;
    return self;
}

// 0x7538A0: NiAllocator's destructor (NiStandardAllocator's too), from CMemoryMgr
void* __fastcall NiAllocator_Destroy(re::Ptr32<void>* self, re::Edx, int flags) {
    *self = reinterpret_cast<void*>(0x0090387C);
    if (flags & 1) {
        CMemoryMgr_Free(self);
    }
    return self;
}

// ---- NiMemManager and the system library's static data

namespace {
GAME_VAR(uint8_t, systemSDM_initialized, 0x020E215C);
GAME_FN(void*, __thiscall, NiInitOptions_GetAllocator, 0x00773DB0, void* options);
GAME_FN(void, __cdecl, NiMemManager_SDMInit, 0x00753BF0);
GAME_FN(void, __cdecl, NiMemManager_SDMShutdown, 0x00753BC0);
GAME_FN(void, __cdecl, NiSystemDesc_SDMInit, 0x00753F80);
GAME_FN(void, __cdecl, NiSystemDesc_SDMShutdown, 0x00753F60);
} // namespace

// 0x753BC0: the allocator's Shutdown, the manager freed
void __cdecl NiMemManager_SDMShutdownImpl() {
    Virtual<void>(*ms_pkMemManager, 7);
    CMemoryMgr_Free(ms_pkMemManager);
    ms_pkMemManager = nullptr;
}

// 0x753BF0: the manager made (not checked) with the options' allocator, then its Initialize
void __cdecl NiMemManager_SDMInitImpl() {
    ms_pkMemManager = static_cast<re::Ptr32<void>*>(CMemoryMgr_Malloc(4));
    *ms_pkMemManager = NiInitOptions_GetAllocator(sdm_initOptions);
    Virtual<void>(*ms_pkMemManager, 6);
}

// 0x753C30
void __cdecl NiSystemSDM_InitImpl() {
    if (!systemSDM_initialized) {
        systemSDM_initialized = 1;
        NiMemManager_SDMInit();
        NiSystemDesc_SDMInit();
    }
}

// 0x753C50
void __cdecl NiSystemSDM_ShutdownImpl() {
    if (systemSDM_initialized) {
        systemSDM_initialized = 0;
        NiSystemDesc_SDMShutdown();
        NiMemManager_SDMShutdown();
    }
}

// NiSystemDesc: the processor count and the performance counter's frequency
namespace {
struct NiSystemDesc {
    uint32_t numProcessors;   // +0x0
    float counterFrequency;   // +0x4
};
GAME_VAR(NiSystemDesc*, ms_pkSystemDesc, 0x020E2164);
} // namespace

// 0x753F60
void __cdecl NiSystemDesc_SDMShutdownImpl() {
    NiDelete(ms_pkSystemDesc, 8);
    ms_pkSystemDesc = nullptr;
}

// 0x753F80
void __cdecl NiSystemDesc_SDMInitImpl() {
    NiSystemDesc* desc = static_cast<NiSystemDesc*>(NiNew(8));
    if (desc == nullptr) {
        ms_pkSystemDesc = nullptr;
        return;
    }
    SYSTEM_INFO info;   // (the system's own layout: 0x24 bytes in the 32-bit game, 0x30 on x64)
    Import<void>(kGetSystemInfo, &info);
    desc->numProcessors = info.dwNumberOfProcessors;
    int64_t frequency;
    Import<int>(kQueryPerformanceFrequency, &frequency);
    desc->counterFrequency = float(frequency);
    ms_pkSystemDesc = desc;
}

// ---- NiSmallObjectAllocator: 256 NiFixedAllocators (sizes 1..256), each a vector of chunks of
// up to 255 blocks, each block's first byte the index of the next free one

namespace {
struct NiChunk {
    re::Ptr32<uint8_t> data;                // +0x0
    uint8_t firstAvailableBlock;  // +0x4
    uint8_t blocksAvailable;      // +0x5
    uint8_t pad[2];
};
static_assert(sizeof(NiChunk) == 8);

struct NiFixedAllocator {
    uint32_t blockSize;          // +0x0
    uint8_t numBlocks;           // +0x4
    uint8_t pad5[3];
    re::Ptr32<NiChunk> chunks;             // +0x8 (realloc'd)
    uint32_t chunkCount;         // +0xC
    uint32_t chunkCapacity;      // +0x10
    re::Ptr32<NiChunk> allocChunk;         // +0x14
    re::Ptr32<NiChunk> deallocChunk;       // +0x18
    re::Ptr32<NiChunk> emptyChunk;         // +0x1C
    NiCriticalSection lock;      // +0x20
};
static_assert(sizeof(NiFixedAllocator) == 0x40);

struct NiSmallObjectAllocator {
    NiFixedAllocator pool[256];   // +0x0 (pool[n - 1] serves n bytes)
    uint32_t chunkSize;           // +0x4000
};
static_assert(sizeof(NiSmallObjectAllocator) == 0x4004);

GAME_VAR(NiSmallObjectAllocator*, ms_pkSmallAlloc, 0x020E2160);
GAME_FN(NiSmallObjectAllocator*, __thiscall, NiSmallObjectAllocator_Construct, 0x00754290, NiSmallObjectAllocator* self,
        uint32_t chunkSize);
GAME_FN(void*, __thiscall, NiSmallObjectAllocator_Allocate, 0x00754340, NiSmallObjectAllocator* self, uint32_t size);
GAME_FN(void, __thiscall, NiSmallObjectAllocator_Deallocate, 0x00754360, NiSmallObjectAllocator* self, void* p,
        uint32_t size);
GAME_FN(void, __thiscall, NiFixedAllocator_Destruct, 0x00754060, NiFixedAllocator* self);
GAME_FN(void*, __thiscall, NiFixedAllocator_Allocate, 0x007540A0, NiFixedAllocator* self);
GAME_FN(NiChunk*, __thiscall, NiFixedAllocator_VicinityFind, 0x00753FE0, NiFixedAllocator* self, void* p);
GAME_FN(void, __thiscall, NiFixedAllocator_DoDeallocate, 0x00754210, NiFixedAllocator* self, void* p);
GAME_FN(uint32_t, __cdecl, NiStandardAllocator_GetSize, 0x00753CA0, const void* header);

// the size header (written a byte at a time: the block may not be aligned)
inline void WriteSize(uint8_t* p, uint32_t size) {
    p[0] = uint8_t(size);
    p[1] = uint8_t(size >> 8);
    p[2] = uint8_t(size >> 16);
    p[3] = uint8_t(size >> 24);
}
} // namespace

// 0x753C70: NiStandardAllocator::Initialize (the small object allocator, from CMemoryMgr)
void __fastcall NiStandardAllocator_Initialize(void* self, re::Edx) {
    (void)self;
    void* memory = CMemoryMgr_Malloc(sizeof(NiSmallObjectAllocator));
    if (memory != nullptr) {
        ms_pkSmallAlloc = NiSmallObjectAllocator_Construct(static_cast<NiSmallObjectAllocator*>(memory), 0x6400);
    } else {
        ms_pkSmallAlloc = nullptr;
    }
}

// 0x753CA0: the size header in front of a block
uint32_t __cdecl NiStandardAllocator_GetSizeImpl(const void* header) {
    const uint8_t* p = static_cast<const uint8_t*>(header);
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

// 0x753CD0: NiStandardAllocator::Allocate
void* __fastcall NiStandardAllocator_Allocate(void* self, re::Edx, uint32_t* size, uint32_t* alignment, int event,
                                              int accurate, const char*, int, const char*) {
    (void)self;
    bool prependSize = false;
    if (*alignment == 4 && !re::Flag(accurate) && event != kAlignedMalloc) {
        if (event != kAlignedRealloc) {
            *size += 4;
            prependSize = true;
        }
    }
    uint8_t* p;
    if (event != kAlignedMalloc && event != kAlignedRealloc && *size <= 0x100 && *alignment == 4) {
        p = static_cast<uint8_t*>(NiSmallObjectAllocator_Allocate(ms_pkSmallAlloc, *size));
    } else {
        p = static_cast<uint8_t*>(AlignedMalloc(*size, *alignment));
    }
    if (p != nullptr && prependSize) {
        WriteSize(p, *size);
        p += 4;
    }
    return p;
}

// 0x753D60: NiStandardAllocator::Deallocate (an unknown size read from the header)
void __fastcall NiStandardAllocator_Deallocate(void* self, re::Edx, void* p, int event, uint32_t size) {
    (void)self;
    if (p == nullptr) {
        return;
    }
    uint8_t* block = static_cast<uint8_t*>(p);
    if (event != kAlignedFree && event != kAlignedRealloc && size == 0xFFFFFFFF) {
        block -= 4;
        size = NiStandardAllocator_GetSize(block);
    }
    if (size <= 0x100) {
        NiSmallObjectAllocator_Deallocate(ms_pkSmallAlloc, block, size);
        return;
    }
    AlignedFree(block);
}

// 0x753DC0: NiStandardAllocator::Reallocate. (The small path takes 4 off the size whether or
// not it added them.)
void* __fastcall NiStandardAllocator_Reallocate(void* self, re::Edx, void* p, uint32_t* size, uint32_t* alignment,
                                                int event, int accurate, uint32_t currentSize, const char* file,
                                                int line, const char* function) {
    uint8_t* block = static_cast<uint8_t*>(p);
    if (event != kAlignedFree && event != kAlignedRealloc && currentSize == 0xFFFFFFFF && !re::Flag(accurate)) {
        block -= 4;
        currentSize = NiStandardAllocator_GetSize(block);
    }
    bool prependSize = false;
    if (*alignment == 4 && !re::Flag(accurate) && event != kAlignedMalloc && event != kAlignedRealloc) {
        *size += 4;
        prependSize = true;
    }
    if (currentSize != 0xFFFFFFFF && *size <= currentSize && prependSize) {
        *size = currentSize;
        return block + 4;
    }
    uint8_t* result;
    if (event == kAlignedMalloc || event == kAlignedRealloc || (currentSize > 0x100 && *size > 0x100)) {
        result = static_cast<uint8_t*>(AlignedRealloc(block, *size, *alignment));
    } else {
        *size -= 4;
        result = static_cast<uint8_t*>(
            Virtual<void*>(self, 1, size, alignment, event, accurate, file, line, function));
        prependSize = false;
        if (currentSize != 0xFFFFFFFF) {
            crt::MemcpyS(result, *size, block + 4, currentSize - 4);
            Virtual<void>(self, 2, block, event, currentSize);
        }
    }
    if (result != nullptr && prependSize) {
        WriteSize(result, *size);
        result += 4;
    }
    return result;
}

// 0x753F10: NiStandardAllocator::Shutdown (the pool destroyed from the last allocator down)
void __fastcall NiStandardAllocator_Shutdown(void* self, re::Edx) {
    (void)self;
    NiSmallObjectAllocator* small = ms_pkSmallAlloc;
    if (small != nullptr) {
        for (int i = 255; i >= 0; i--) {
            NiFixedAllocator_Destruct(&small->pool[i]);
        }
        CMemoryMgr_Free(small);
    }
    ms_pkSmallAlloc = nullptr;
}

// 0x753FE0: NiFixedAllocator::VicinityFind, the chunk holding p, searched outwards from the
// last one freed into
NiChunk* __fastcall NiFixedAllocator_VicinityFindImpl(NiFixedAllocator* self, re::Edx, void* p) {
    uint32_t chunkLength = uint32_t(self->numBlocks) * self->blockSize;
    NiChunk* lo = self->deallocChunk;
    NiChunk* hi = lo + 1;
    NiChunk* loBound = self->chunks;
    NiChunk* hiBound = self->chunks + self->chunkCount;
    uint8_t* bytes = static_cast<uint8_t*>(p);
    if (hi == hiBound) {
        hi = nullptr;
    }
    for (;;) {
        if (lo != nullptr) {
            if (lo->data <= bytes && bytes < lo->data + chunkLength) {
                return lo;
            }
            if (lo == loBound) {
                lo = nullptr;
                if (hi == nullptr) {
                    return nullptr;
                }
            } else {
                lo--;
            }
        }
        if (hi != nullptr) {
            if (hi->data <= bytes && bytes < hi->data + chunkLength) {
                return hi;
            }
            if (++hi == hiBound) {
                hi = nullptr;
                if (lo == nullptr) {
                    return nullptr;
                }
            }
        }
    }
}

// 0x754060: NiFixedAllocator's destructor
void __fastcall NiFixedAllocator_DestructImpl(NiFixedAllocator* self, re::Edx) {
    for (uint32_t i = 0; i < self->chunkCount; i++) {
        AlignedFree(self->chunks[i].data);
    }
    crt::Free(self->chunks);
    Import<void>(kDeleteCriticalSection, &self->lock.section);
}

// 0x7540A0: NiFixedAllocator::Allocate
void* __fastcall NiFixedAllocator_AllocateImpl(NiFixedAllocator* self, re::Edx) {
    Lock(self->lock);
    if (self->allocChunk != nullptr && self->allocChunk->blocksAvailable == 0) {
        self->allocChunk = nullptr;
    }
    if (self->emptyChunk != nullptr) {
        self->allocChunk = self->emptyChunk;
        self->emptyChunk = nullptr;
    }
    if (self->allocChunk == nullptr) {
        for (uint32_t i = 0; i < self->chunkCount; i++) {
            if (self->chunks[i].blocksAvailable != 0) {
                self->allocChunk = &self->chunks[i];
                break;
            }
        }
        if (self->allocChunk == nullptr) {
            // (reserve(size + 1), then a new chunk pushed back)
            uint32_t needed = self->chunkCount + 1;
            if (needed > self->chunkCapacity) {
                self->chunks = static_cast<NiChunk*>(Realloc(self->chunks, needed * 8));
                self->chunkCapacity = needed;
            }
            uint8_t blocks = self->numBlocks;
            uint32_t blockSize = self->blockSize;
            uint8_t* data = static_cast<uint8_t*>(AlignedMalloc(uint32_t(blocks) * blockSize, 4));
            uint8_t* block = data;
            for (uint8_t i = 0; i != blocks; block += blockSize) {
                *block = ++i;
            }
            uint32_t index = self->chunkCount;
            needed = index + 1;
            if (needed > self->chunkCapacity) {
                self->chunks = static_cast<NiChunk*>(Realloc(self->chunks, needed * 8));
                self->chunkCapacity = needed;
            }
            NiChunk& chunk = self->chunks[index];
            chunk.data = data;
            chunk.firstAvailableBlock = 0;
            chunk.blocksAvailable = blocks;
            chunk.pad[0] = chunk.pad[1] = 0;   // (the game copies 2 bytes of its stack here)
            self->chunkCount++;
            self->allocChunk = &self->chunks[self->chunkCount - 1];
            self->deallocChunk = self->chunks;
        }
    }
    NiChunk* chunk = self->allocChunk;
    uint8_t* result = nullptr;
    if (chunk->blocksAvailable != 0) {
        result = chunk->data + uint32_t(chunk->firstAvailableBlock) * self->blockSize;
        chunk->firstAvailableBlock = *result;
        chunk->blocksAvailable--;
    }
    Unlock(self->lock);
    return result;
}

// 0x754210: NiFixedAllocator::DoDeallocate (into deallocChunk); a second empty chunk is freed
void __fastcall NiFixedAllocator_DoDeallocateImpl(NiFixedAllocator* self, re::Edx, void* p) {
    NiChunk* chunk = self->deallocChunk;
    uint8_t* block = static_cast<uint8_t*>(p);
    *block = chunk->firstAvailableBlock;
    chunk->firstAvailableBlock = uint8_t(uint32_t(block - chunk->data) / self->blockSize);
    chunk->blocksAvailable++;
    NiChunk* dealloc = self->deallocChunk;
    if (dealloc->blocksAvailable != self->numBlocks) {
        return;
    }
    NiChunk* empty = self->emptyChunk;
    if (empty != nullptr) {
        NiChunk* last = &self->chunks[self->chunkCount - 1];
        if (last == dealloc) {
            self->deallocChunk = empty;
        } else if (last != empty) {
            NiChunk swap = *empty;
            *empty = *last;
            *last = swap;
        }
        AlignedFree(last->data);
        dealloc = self->deallocChunk;
        self->chunkCount--;
        self->allocChunk = dealloc;
    }
    self->emptyChunk = dealloc;
}

// 0x754290: NiSmallObjectAllocator's constructor. (The pools' page size is 0x6400 whatever
// the argument: the game's code has the constant.)
NiSmallObjectAllocator* __fastcall NiSmallObjectAllocator_ConstructImpl(NiSmallObjectAllocator* self, re::Edx,
                                                                        uint32_t chunkSize) {
    for (int i = 0; i < 256; i++) {
        NiFixedAllocator& f = self->pool[i];
        f.blockSize = 0;
        f.chunks = nullptr;
        f.chunkCount = 0;
        f.chunkCapacity = 0;
        f.allocChunk = nullptr;
        f.deallocChunk = nullptr;
        f.emptyChunk = nullptr;
        f.lock.owner = 0;
        f.lock.lockCount = 0;
        Import<void>(kInitializeCriticalSection, &f.lock.section);
    }
    self->chunkSize = chunkSize;
    for (uint32_t size = 1; size < 0x101; size++) {
        NiFixedAllocator& f = self->pool[size - 1];
        uint32_t blocks = 0x6400 / size;
        f.chunkCount = 0;
        f.chunkCapacity = 0;
        f.chunks = nullptr;
        f.allocChunk = nullptr;
        f.blockSize = size;
        if (blocks > 0xFF) {
            blocks = 0xFF;
        } else if (blocks == 0) {
            blocks = size * 8;
        }
        f.numBlocks = uint8_t(blocks);
    }
    return self;
}

// 0x754340
void* __fastcall NiSmallObjectAllocator_AllocateImpl(NiSmallObjectAllocator* self, re::Edx, uint32_t size) {
    return NiFixedAllocator_Allocate(&self->pool[size - 1]);
}

// 0x754360
void __fastcall NiSmallObjectAllocator_DeallocateImpl(NiSmallObjectAllocator* self, re::Edx, void* p, uint32_t size) {
    NiFixedAllocator* f = &self->pool[size - 1];
    Lock(f->lock);
    f->deallocChunk = NiFixedAllocator_VicinityFind(f, p);
    // (testing, bullyre.ini "heap_check": a freed block filled with 0xDD past its first byte,
    // the only one the allocator uses, so a stale pointer to it reads as such (a state's
    // vtable, a list node's next): the 2026-10-30 soaks' effect states)
    if (heapcheck::g_validateFrees && size > 1) {
        std::memset(static_cast<uint8_t*>(p) + 1, 0xDD, size - 1);
    }
    // (the 8-byte blocks are held back a while: see HeapCheck.h's Quarantine)
    if (heapcheck::g_validateFrees && size == 8) {
        p = heapcheck::Quarantine(p);
        if (p == nullptr) {
            Unlock(f->lock);
            return;
        }
        f->deallocChunk = NiFixedAllocator_VicinityFind(f, p);
    }
    NiFixedAllocator_DoDeallocate(f, p);
    Unlock(f->lock);
}

// 0x7543C0: NiRefObject::DeleteThis (the deleting destructor)
void __fastcall NiRefObject_DeleteThis(void* self, re::Edx) {
    if (self != nullptr) {
        Virtual<void>(self, 0, 1);
    }
}

#define NIMEM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIMEM_REVERSED("NiMalloc", 0x00752300, NiMallocImpl);
NIMEM_REVERSED("NiAlignedMalloc", 0x00752340, NiAlignedMalloc);
NIMEM_REVERSED("NiFree", 0x00752380, NiFreeImpl);
NIMEM_REVERSED("NiAlignedFree", 0x007523A0, NiAlignedFree);
NIMEM_REVERSED("NiRealloc", 0x007523C0, NiRealloc);
NIMEM_REVERSED("NiMemObject::operator new", 0x00752460, NiMemObject_New);
NIMEM_REVERSED("NiMemObject::operator new[]", 0x007524D0, NiMemObject_NewArray);
NIMEM_REVERSED("NiMemObject::operator delete", 0x00752540, NiMemObject_Delete);
NIMEM_REVERSED("NiMemObject::operator delete[]", 0x00752570, NiMemObject_DeleteArray);
NIMEM_REVERSED("NiStaticDataManager::Init", 0x007536F0, NiStaticDataManager_Init);
NIMEM_REVERSED("NiStaticDataManager::Shutdown", 0x00753750, NiStaticDataManager_Shutdown);
NIMEM_REVERSED("NiGetCurrentTimeInSec", 0x007537B0, NiGetCurrentTimeInSec);
NIMEM_REVERSED("NiInitOptions::NiInitOptions(NiAllocator*)", 0x00753840, NiInitOptions_ConstructWith);
NIMEM_REVERSED("NiInitOptions::~NiInitOptions", 0x00753850, NiInitOptions_DestructImpl);
NIMEM_REVERSED("NiInitOptions::NiInitOptions", 0x00753870, NiInitOptions_ConstructDefault);
NIMEM_REVERSED("NiAllocator::~NiAllocator", 0x007538A0, NiAllocator_Destroy);
NIMEM_REVERSED("NiMemManager::_SDMShutdown", 0x00753BC0, NiMemManager_SDMShutdownImpl);
NIMEM_REVERSED("NiMemManager::_SDMInit", 0x00753BF0, NiMemManager_SDMInitImpl);
NIMEM_REVERSED("NiSystemSDM::Init", 0x00753C30, NiSystemSDM_InitImpl);
NIMEM_REVERSED("NiSystemSDM::Shutdown", 0x00753C50, NiSystemSDM_ShutdownImpl);
NIMEM_REVERSED("NiStandardAllocator::Initialize", 0x00753C70, NiStandardAllocator_Initialize);
NIMEM_REVERSED("NiStandardAllocator::GetSizeFromAddress", 0x00753CA0, NiStandardAllocator_GetSizeImpl);
NIMEM_REVERSED("NiStandardAllocator::Allocate", 0x00753CD0, NiStandardAllocator_Allocate);
NIMEM_REVERSED("NiStandardAllocator::Deallocate", 0x00753D60, NiStandardAllocator_Deallocate);
NIMEM_REVERSED("NiStandardAllocator::Reallocate", 0x00753DC0, NiStandardAllocator_Reallocate);
NIMEM_REVERSED("NiStandardAllocator::Shutdown", 0x00753F10, NiStandardAllocator_Shutdown);
NIMEM_REVERSED("NiSystemDesc::_SDMShutdown", 0x00753F60, NiSystemDesc_SDMShutdownImpl);
NIMEM_REVERSED("NiSystemDesc::_SDMInit", 0x00753F80, NiSystemDesc_SDMInitImpl);
NIMEM_REVERSED("NiFixedAllocator::VicinityFind", 0x00753FE0, NiFixedAllocator_VicinityFindImpl);
NIMEM_REVERSED("NiFixedAllocator::~NiFixedAllocator", 0x00754060, NiFixedAllocator_DestructImpl);
NIMEM_REVERSED("NiFixedAllocator::Allocate", 0x007540A0, NiFixedAllocator_AllocateImpl);
NIMEM_REVERSED("NiFixedAllocator::DoDeallocate", 0x00754210, NiFixedAllocator_DoDeallocateImpl);
NIMEM_REVERSED("NiSmallObjectAllocator::NiSmallObjectAllocator", 0x00754290, NiSmallObjectAllocator_ConstructImpl);
NIMEM_REVERSED("NiSmallObjectAllocator::Allocate", 0x00754340, NiSmallObjectAllocator_AllocateImpl);
NIMEM_REVERSED("NiSmallObjectAllocator::Deallocate", 0x00754360, NiSmallObjectAllocator_DeallocateImpl);
NIMEM_REVERSED("NiRefObject::DeleteThis", 0x007543C0, NiRefObject_DeleteThis);

} // namespace ni
