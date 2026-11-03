// The low heap (lowheap.h): every block below 2 GB, whatever else the process allocates.
//
// Address space is taken in regions below 2 GB (VirtualAlloc2's address requirements) and
// handed out in 16 KB units: runs of units for large blocks (over 64 KB) and for slabs, a slab
// holding the blocks of one size class (16 bytes to 64 KB, four classes to a doubling). Runs
// are committed when taken and decommitted when given back, so the memory goes back to the
// system while the address space stays ours. One table entry per 16 KB unit of the low 2 GB
// says what a pointer belongs to. One lock (the game allocates from its streaming thread too).
//
// --stress-high on the command line: at the first allocation all the free space below 4 GB is
// taken too (below 2 GB as more regions of ours, the rest held empty), so every allocation that
// isn't ours lands above 4 GB, and a pointer to one stored into a game field stops the game at
// that store (re::Ptr32: re::PointerTooWide). What the large-address-aware build has to fix
// shows up at once instead of when the low space happens to run out.

#include <windows.h>
#include <psapi.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <unordered_map>
#include <vector>

#include <MinHook.h>

#include "reversed.h"
#include "x64/lowheap.h"

namespace re {

// re::Ptr32 found a pointer its 4 bytes can't hold (reversed.h)
[[noreturn]] __declspec(noinline) void PointerTooWide(uintptr_t value) {
    ULONG_PTR info[1] = {value};
    RaiseException(kPointerTooWide, EXCEPTION_NONCONTINUABLE, 1, info);
    __fastfail(FAST_FAIL_FATAL_APP_EXIT);
}

namespace low {
namespace {

constexpr uintptr_t kLimit = 0x80000000;     // what a sign-extended 4-byte pointer reaches
constexpr uintptr_t k4GB = 0x100000000ull;
constexpr unsigned kShift = 14;              // 16 KB units
constexpr size_t kUnit = size_t(1) << kShift;
constexpr uint32_t kUnits = uint32_t(kLimit >> kShift);
constexpr uint32_t kNone = 0xFFFFFFFF;
constexpr size_t kFirstRegion = size_t(1536) << 20;
constexpr size_t kGrowRegion = size_t(256) << 20;
constexpr size_t kMaxSmall = 64 * 1024;
constexpr size_t kSlabHeader = 64;

enum : uint8_t { kOutside = 0, kFree, kSlab, kLarge };

struct Unit {
    uint8_t kind;
    uint8_t regionFirst;   // a region's first unit (runs never merge across regions)
    uint32_t start;        // the run's first unit: every unit of slabs and large runs, the last of free runs
    uint32_t length;       // (the run's first unit)
    uint32_t prev, next;   // free runs: the list (the first unit)
};
Unit g_units[kUnits];

struct Slab {
    Slab* prev;
    Slab* next;            // the class's slabs with a free block
    void* freeList;
    uint8_t* bump;         // blocks never handed out start here
    uint8_t* end;
    uint32_t used, capacity;
    uint32_t cls;
    uint32_t listed;
};
static_assert(sizeof(Slab) <= kSlabHeader);

constexpr int kMaxClasses = 64;
uint32_t g_classSize[kMaxClasses];
int g_classes = 0;
uint8_t g_smallClass[kMaxSmall / 16 + 1];   // (size + 15) / 16 -> class, up to 1024 bytes; above, searched
Slab* g_partial[kMaxClasses];

SRWLOCK g_lock = SRWLOCK_INIT;
bool g_ready = false;
uint32_t g_freeHead = kNone;
Stats g_stats{};
bool g_reportedForeign = false;

using VirtualAlloc2Fn = PVOID(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
VirtualAlloc2Fn g_virtualAlloc2 = nullptr;

uintptr_t Address(uint32_t unit) {
    return uintptr_t(unit) << kShift;
}

void LinkFree(uint32_t s, uint32_t n) {
    for (uint32_t i = s; i < s + n; i++) {
        g_units[i].kind = kFree;
    }
    g_units[s].length = n;
    g_units[s + n - 1].start = s;
    g_units[s].prev = kNone;
    g_units[s].next = g_freeHead;
    if (g_freeHead != kNone) {
        g_units[g_freeHead].prev = s;
    }
    g_freeHead = s;
}

void UnlinkFree(uint32_t s) {
    Unit& u = g_units[s];
    if (u.prev != kNone) {
        g_units[u.prev].next = u.next;
    } else {
        g_freeHead = u.next;
    }
    if (u.next != kNone) {
        g_units[u.next].prev = u.prev;
    }
}

// A region of address space (64 KB aligned, below kLimit) made ours
void AddRegion(uintptr_t base, size_t size) {
    uint32_t s = uint32_t(base >> kShift);
    uint32_t n = uint32_t(size >> kShift);
    if (n == 0) {
        return;
    }
    g_units[s].regionFirst = 1;
    LinkFree(s, n);
    g_stats.reserved += size_t(n) << kShift;
}

void* ReserveLow(size_t size) {
    if (g_virtualAlloc2) {
        MEM_ADDRESS_REQUIREMENTS req{};
        req.HighestEndingAddress = reinterpret_cast<PVOID>(kLimit - 1);
        MEM_EXTENDED_PARAMETER param{};
        param.Type = MemExtendedParameterAddressRequirements;
        param.Pointer = &req;
        return g_virtualAlloc2(GetCurrentProcess(), nullptr, size, MEM_RESERVE, PAGE_NOACCESS, &param, 1);
    }
    void* p = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    if (p && reinterpret_cast<uintptr_t>(p) + size > kLimit) {
        VirtualFree(p, 0, MEM_RELEASE);
        return nullptr;
    }
    return p;
}

// More address space (kGrowRegion, or less down to what's needed when the low space is short)
bool Grow(uint32_t units) {
    const size_t need = ((size_t(units) << kShift) + 0xFFFF) & ~size_t(0xFFFF);
    size_t size = need > kGrowRegion ? need : kGrowRegion;
    for (;;) {
        if (void* p = ReserveLow(size)) {
            AddRegion(reinterpret_cast<uintptr_t>(p), size);
            return true;
        }
        if (size == need) {
            return false;
        }
        size = size / 2 > need ? (size / 2 + 0xFFFF) & ~size_t(0xFFFF) : need;
    }
}

// The free space between 2 and 4 GB held empty (always: re::Narrow32 takes a value there for a
// number), and with `below` everything free below 2 GB taken as regions of ours
void HoldLowSpace(bool below) {
    uintptr_t a = below ? 0x10000 : kLimit;
    while (a < k4GB) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(reinterpret_cast<void*>(a), &mbi, sizeof mbi)) {
            break;
        }
        uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        uintptr_t end = base + mbi.RegionSize;
        if (mbi.State == MEM_FREE) {
            uintptr_t lo = (base + 0xFFFF) & ~uintptr_t(0xFFFF);
            uintptr_t hi = end < k4GB ? end : k4GB;
            if (lo < kLimit && hi > lo) {
                uintptr_t top = (hi < kLimit ? hi : kLimit) & ~uintptr_t(kUnit - 1);
                if (top > lo && VirtualAlloc(reinterpret_cast<void*>(lo), top - lo, MEM_RESERVE, PAGE_NOACCESS)) {
                    AddRegion(lo, top - lo);
                }
                lo = top > lo ? top : lo;
            }
            if (hi > lo && lo >= kLimit) {
                VirtualAlloc(reinterpret_cast<void*>(lo), hi - lo, MEM_RESERVE, PAGE_NOACCESS);   // (held, never used)
            }
        }
        a = end;
    }
}

void InitLocked() {
    if (g_ready) {
        return;
    }
    g_ready = true;
    if (HMODULE kb = GetModuleHandleW(L"kernelbase.dll")) {
        g_virtualAlloc2 = reinterpret_cast<VirtualAlloc2Fn>(GetProcAddress(kb, "VirtualAlloc2"));
    }
    // the size classes: 16 to 128 by 16, then four to a doubling
    for (uint32_t s = 16; s <= 128; s += 16) {
        g_classSize[g_classes++] = s;
    }
    for (uint32_t base = 128; base < kMaxSmall; base *= 2) {
        for (uint32_t q = 1; q <= 4; q++) {
            g_classSize[g_classes++] = base + base / 4 * q;
        }
    }
    int c = 0;
    for (uint32_t i = 0; i <= kMaxSmall / 16; i++) {
        while (c < g_classes - 1 && g_classSize[c] < i * 16) {
            c++;
        }
        g_smallClass[i] = uint8_t(c);
    }
    const char* command = GetCommandLineA();
    g_stats.stress = command && std::strstr(command, "--stress-high");
    HoldLowSpace(g_stats.stress);
    if (!g_stats.stress) {
        // (address space only, taken early, before the system's allocations spread over it)
        for (size_t size = kFirstRegion; size >= kGrowRegion; size -= kGrowRegion) {
            if (void* p = ReserveLow(size)) {
                AddRegion(reinterpret_cast<uintptr_t>(p), size);
                break;
            }
        }
    }
}

int ClassOf(size_t size) {
    return g_smallClass[(size + 15) / 16];
}

// A run of n units: the best fit among the free runs (more address space taken if none)
uint32_t TakeRun(uint32_t n, uint8_t kind) {
    for (int attempt = 0; attempt < 2; attempt++) {
        uint32_t best = kNone;
        for (uint32_t s = g_freeHead; s != kNone; s = g_units[s].next) {
            uint32_t len = g_units[s].length;
            if (len >= n && (best == kNone || len < g_units[best].length)) {
                best = s;
                if (len == n) {
                    break;
                }
            }
        }
        if (best != kNone) {
            uint32_t len = g_units[best].length;
            UnlinkFree(best);
            if (len > n) {
                LinkFree(best + n, len - n);   // (the rest stays in the same region: no regionFirst there)
            }
            if (!VirtualAlloc(reinterpret_cast<void*>(Address(best)), size_t(n) << kShift, MEM_COMMIT, PAGE_READWRITE)) {
                LinkFree(best, n);   // (the system is out of memory; the caller gets nullptr)
                return kNone;
            }
            for (uint32_t i = best; i < best + n; i++) {
                g_units[i].kind = kind;
                g_units[i].start = best;
            }
            g_units[best].length = n;
            g_stats.committed += size_t(n) << kShift;
            return best;
        }
        if (attempt == 0 && !Grow(n)) {
            break;
        }
    }
    return kNone;
}

void ReleaseRun(uint32_t s) {
    uint32_t n = g_units[s].length;
    VirtualFree(reinterpret_cast<void*>(Address(s)), size_t(n) << kShift, MEM_DECOMMIT);
    g_stats.committed -= size_t(n) << kShift;
    uint32_t end = s + n;
    if (!g_units[s].regionFirst && s > 0 && g_units[s - 1].kind == kFree) {
        uint32_t left = g_units[s - 1].start;
        UnlinkFree(left);
        n += s - left;
        s = left;
    }
    if (end < kUnits && !g_units[end].regionFirst && g_units[end].kind == kFree) {
        UnlinkFree(end);
        n += g_units[end].length;
    }
    LinkFree(s, n);
}

Slab* NewSlab(int cls) {
    size_t size = g_classSize[cls];
    size_t bytes = kSlabHeader + size * 8;
    if (bytes < 4 * kUnit) {
        bytes = 4 * kUnit;
    }
    uint32_t n = uint32_t((bytes + kUnit - 1) >> kShift);
    uint32_t s = TakeRun(n, kSlab);
    if (s == kNone) {
        return nullptr;
    }
    auto* slab = reinterpret_cast<Slab*>(Address(s));
    slab->prev = nullptr;
    slab->next = nullptr;
    slab->freeList = nullptr;
    slab->bump = reinterpret_cast<uint8_t*>(slab) + kSlabHeader;
    slab->capacity = uint32_t(((size_t(n) << kShift) - kSlabHeader) / size);
    slab->end = slab->bump + size_t(slab->capacity) * size;
    slab->used = 0;
    slab->cls = uint32_t(cls);
    slab->listed = 0;
    return slab;
}

void List(Slab* slab) {
    slab->prev = nullptr;
    slab->next = g_partial[slab->cls];
    if (slab->next) {
        slab->next->prev = slab;
    }
    g_partial[slab->cls] = slab;
    slab->listed = 1;
}

void Unlist(Slab* slab) {
    if (slab->prev) {
        slab->prev->next = slab->next;
    } else {
        g_partial[slab->cls] = slab->next;
    }
    if (slab->next) {
        slab->next->prev = slab->prev;
    }
    slab->listed = 0;
}

void* AllocLocked(size_t size) {
    InitLocked();
    if (size == 0) {
        size = 1;
    }
    if (size > kMaxSmall) {
        uint32_t n = uint32_t((size + kUnit - 1) >> kShift);
        uint32_t s = TakeRun(n, kLarge);
        if (s == kNone) {
            return nullptr;
        }
        g_stats.inUse += size_t(n) << kShift;
        g_stats.blocks++;
        return reinterpret_cast<void*>(Address(s));
    }
    int cls = ClassOf(size);
    Slab* slab = g_partial[cls];
    if (!slab) {
        slab = NewSlab(cls);
        if (!slab) {
            return nullptr;
        }
        List(slab);
    }
    void* p;
    if (slab->freeList) {
        p = slab->freeList;
        slab->freeList = *static_cast<void**>(p);
    } else {
        p = slab->bump;
        slab->bump += g_classSize[cls];
    }
    if (++slab->used == slab->capacity) {
        Unlist(slab);
    }
    g_stats.inUse += g_classSize[cls];
    g_stats.blocks++;
    return p;
}

void Foreign(const void* p, const char* what) {
    if (!g_reportedForeign) {
        g_reportedForeign = true;
        std::fprintf(stderr, "x64 low heap: %s of %p, not a block of ours (kept; reported once)\n", what, p);
    }
}

void FreeLocked(void* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    if (a >= kLimit) {
        Foreign(p, "free");
        return;
    }
    uint32_t u = uint32_t(a >> kShift);
    const Unit& unit = g_units[u];
    if (unit.kind == kLarge && Address(unit.start) == a) {
        g_stats.inUse -= size_t(g_units[unit.start].length) << kShift;
        g_stats.blocks--;
        ReleaseRun(unit.start);
        return;
    }
    if (unit.kind != kSlab) {
        Foreign(p, "free");
        return;
    }
    auto* slab = reinterpret_cast<Slab*>(Address(unit.start));
    *static_cast<void**>(p) = slab->freeList;
    slab->freeList = p;
    g_stats.inUse -= g_classSize[slab->cls];
    g_stats.blocks--;
    if (!slab->listed) {
        List(slab);
    }
    if (--slab->used == 0 && (slab->prev || slab->next)) {
        Unlist(slab);   // (empty, and not the class's only slab with room: its memory goes back)
        ReleaseRun(uint32_t(reinterpret_cast<uintptr_t>(slab) >> kShift));
    }
}

size_t CapacityLocked(const void* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    if (!p || a >= kLimit) {
        return 0;
    }
    const Unit& unit = g_units[a >> kShift];
    if (unit.kind == kLarge) {
        return (size_t(g_units[unit.start].length) << kShift) - (a - Address(unit.start));
    }
    if (unit.kind == kSlab) {
        return g_classSize[reinterpret_cast<const Slab*>(Address(unit.start))->cls];
    }
    return 0;
}

struct Lock {
    Lock() { AcquireSRWLockExclusive(&g_lock); }
    ~Lock() { ReleaseSRWLockExclusive(&g_lock); }
};

} // namespace

void* Alloc(size_t size) {
    Lock lock;
    return AllocLocked(size);
}

void Free(void* p) {
    if (!p) {
        return;
    }
    Lock lock;
    FreeLocked(p);
}

void* Realloc(void* p, size_t size) {
    if (!p) {
        return Alloc(size);
    }
    if (size == 0) {
        Free(p);
        return nullptr;
    }
    Lock lock;
    size_t capacity = CapacityLocked(p);
    if (capacity == 0) {
        Foreign(p, "realloc");
        return nullptr;
    }
    if (size <= capacity && (size > kMaxSmall || size > capacity / 2)) {
        return p;
    }
    void* q = AllocLocked(size);
    if (q) {
        std::memcpy(q, p, size < capacity ? size : capacity);
        FreeLocked(p);
    }
    return q;
}

size_t Capacity(const void* p) {
    Lock lock;
    return CapacityLocked(p);
}

bool Owns(const void* p) {
    return Capacity(p) != 0;
}

void* AllocPages(size_t size, unsigned long protect) {
    Lock lock;
    InitLocked();
    uint32_t s = TakeRun(uint32_t((size + kUnit - 1) >> kShift), kLarge);
    if (s == kNone) {
        return nullptr;
    }
    g_stats.inUse += size_t(g_units[s].length) << kShift;
    g_stats.blocks++;
    void* p = reinterpret_cast<void*>(Address(s));
    DWORD old;
    VirtualProtect(p, size, protect, &old);
    return p;
}

Stats GetStats() {
    Lock lock;
    InitLocked();
    return g_stats;
}

// ---- the system DLLs whose objects the game's fields hold: their heap imports turned to ours.
// Blocks under kSystemSmall (the objects: devices, textures, buffers, filters) come from the low
// heap; larger ones (their data: a texture's system copy, a buffer's contents) from the top of
// the address space (VirtualAlloc top-down, kept in a table), so that the data of a big mod
// never uses up the space below 2 GB the low heap grows into. A block that isn't ours (made
// before the patch) goes back the way it came: each C runtime import is kept as it was (the
// first DLL's: msvcrt's malloc has a heap of its own, the process heap is ucrt's) and called
// for those; HeapAlloc's go to the heap named, LocalAlloc's to the local heap.

namespace {

constexpr size_t kSystemSmall = 256 * 1024;

// the large blocks: their sizes (64 KB multiples) by address, and the ones given back kept for
// the next of the same size (up to kHighCacheBytes: d3d9 makes and drops buffers of the same
// sizes all the time, and a VirtualAlloc each time is kernel work and zeroed pages)
constexpr size_t kHighCacheBytes = size_t(256) << 20;
SRWLOCK g_highLock = SRWLOCK_INIT;
std::unordered_map<uintptr_t, size_t>* g_high = nullptr;
std::unordered_map<size_t, std::vector<void*>>* g_highCache = nullptr;
size_t g_highCached = 0;
HighStats g_highStats{};
volatile uintptr_t g_highLowest = ~uintptr_t(0);   // (the lowest high block made: top-down, so the rest are above)

void* HighAlloc(size_t size, bool zero) {
    size = (size + 0xFFFF) & ~size_t(0xFFFF);
    void* p = nullptr;
    AcquireSRWLockExclusive(&g_highLock);
    if (!g_high) {
        g_high = new std::unordered_map<uintptr_t, size_t>();
        g_highCache = new std::unordered_map<size_t, std::vector<void*>>();
    }
    auto cached = g_highCache->find(size);
    if (cached != g_highCache->end() && !cached->second.empty()) {
        p = cached->second.back();
        cached->second.pop_back();
        g_highCached -= size;
        g_highStats.reused++;
    }
    ReleaseSRWLockExclusive(&g_highLock);
    if (p) {
        if (zero) {
            std::memset(p, 0, size);
        }
    } else {
        p = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);   // (zeroed)
        if (!p) {
            return nullptr;
        }
        AcquireSRWLockExclusive(&g_highLock);
        g_highStats.allocated++;
        ReleaseSRWLockExclusive(&g_highLock);
    }
    AcquireSRWLockExclusive(&g_highLock);
    if (reinterpret_cast<uintptr_t>(p) < g_highLowest) {
        g_highLowest = reinterpret_cast<uintptr_t>(p);
    }
    (*g_high)[reinterpret_cast<uintptr_t>(p)] = size;
    g_highStats.blocks++;
    g_highStats.bytes += size;
    ReleaseSRWLockExclusive(&g_highLock);
    return p;
}
size_t HighSize(const void* p) {
    if (!p) {
        return 0;
    }
    AcquireSRWLockShared(&g_highLock);
    size_t size = 0;
    if (g_high) {
        auto it = g_high->find(reinterpret_cast<uintptr_t>(p));
        if (it != g_high->end()) {
            size = it->second;
        }
    }
    ReleaseSRWLockShared(&g_highLock);
    return size;
}
bool HighFree(void* p) {
    AcquireSRWLockExclusive(&g_highLock);
    size_t size = 0;
    if (g_high) {
        auto it = g_high->find(reinterpret_cast<uintptr_t>(p));
        if (it != g_high->end()) {
            size = it->second;
            g_high->erase(it);
            g_highStats.blocks--;
            g_highStats.bytes -= size;
        }
    }
    bool keep = size != 0 && g_highCached + size <= kHighCacheBytes;
    if (keep) {
        (*g_highCache)[size].push_back(p);
        g_highCached += size;
    } else if (size != 0) {
        g_highStats.released++;
    }
    ReleaseSRWLockExclusive(&g_highLock);
    if (size != 0 && !keep) {
        VirtualFree(p, 0, MEM_RELEASE);
    }
    return size != 0;
}

// a block for one of those DLLs: low under kSystemSmall, high from it
void* SysAlloc(size_t size, bool zero) {
    if (size >= kSystemSmall) {
        return HighAlloc(size, zero);
    }
    void* p = Alloc(size);
    if (p && zero) {
        std::memset(p, 0, size);
    }
    return p;
}
// the usable size of a block of ours (low or high), 0 for another's
size_t SysCapacity(const void* p) {
    if (size_t c = Capacity(p)) {
        return c;
    }
    return HighSize(p);
}
// a block of ours given back (false: not ours)
bool SysFree(void* p) {
    if (Owns(p)) {
        Free(p);
        return true;
    }
    return HighFree(p);
}
// a block of ours to `size` (in place when it fits and stays the same kind)
void* SysRealloc(void* p, size_t size, size_t capacity, bool zero) {
    bool low = Owns(p);
    if (size <= capacity && (low ? size < kSystemSmall : size >= kSystemSmall)) {
        return p;
    }
    void* q = SysAlloc(size, false);
    if (q) {
        std::memcpy(q, p, capacity < size ? capacity : size);
        if (zero && size > capacity) {
            std::memset(static_cast<uint8_t*>(q) + capacity, 0, size - capacity);
        }
        SysFree(p);
    }
    return q;
}

enum CrtFn { kCrtMalloc, kCrtOMalloc, kCrtCalloc, kCrtOCalloc, kCrtRealloc, kCrtORealloc, kCrtFree, kCrtOFree, kCrtMsize, kCrtOMsize, kCrtNew, kCrtNewArray,
             kCrtDelete, kCrtDeleteArray, kCrtCount };
void* g_original[kCrtCount];   // the import as the first DLL patched had it

template <int I, typename F>
F Original() {
    return reinterpret_cast<F>(g_original[I]);
}

template <int I>
void* __cdecl Hook_malloc(size_t size) {
    return SysAlloc(size ? size : 1, false);
}
template <int I>
void* __cdecl Hook_calloc(size_t count, size_t size) {
    if (size && count > SIZE_MAX / size) {
        return nullptr;
    }
    return SysAlloc(count * size ? count * size : 1, true);
}
template <int I>
void* __cdecl Hook_realloc(void* p, size_t size) {
    if (!p) {
        return SysAlloc(size ? size : 1, false);
    }
    size_t capacity = SysCapacity(p);
    if (capacity == 0) {
        return Original<I, void*(__cdecl*)(void*, size_t)>()(p, size);
    }
    if (size == 0) {
        SysFree(p);
        return nullptr;
    }
    return SysRealloc(p, size, capacity, false);
}
template <int I>
void __cdecl Hook_free(void* p) {
    if (p && !SysFree(p)) {
        Original<I, void(__cdecl*)(void*)>()(p);
    }
}
template <int I>
size_t __cdecl Hook_msize(void* p) {
    if (size_t c = SysCapacity(p)) {
        return c;
    }
    return Original<I, size_t(__cdecl*)(void*)>()(p);
}

LPVOID WINAPI Hook_HeapAlloc(HANDLE, DWORD flags, SIZE_T size) {
    return SysAlloc(size ? size : 1, (flags & HEAP_ZERO_MEMORY) != 0);
}
BOOL WINAPI Hook_HeapFree(HANDLE heap, DWORD flags, LPVOID p) {
    if (p && SysFree(p)) {
        return TRUE;
    }
    return HeapFree(heap, flags, p);
}
SIZE_T WINAPI Hook_HeapSize(HANDLE heap, DWORD flags, LPCVOID p) {
    if (size_t c = SysCapacity(p)) {
        return c;
    }
    return HeapSize(heap, flags, p);
}
LPVOID WINAPI Hook_HeapReAlloc(HANDLE heap, DWORD flags, LPVOID p, SIZE_T size) {
    size_t capacity = p ? SysCapacity(p) : 0;
    if (capacity == 0) {
        return HeapReAlloc(heap, flags, p, size);
    }
    if (size <= capacity) {
        return p;
    }
    if (flags & HEAP_REALLOC_IN_PLACE_ONLY) {
        return nullptr;
    }
    return SysRealloc(p, size, capacity, (flags & HEAP_ZERO_MEMORY) != 0);
}

HLOCAL WINAPI Hook_LocalAlloc(UINT flags, SIZE_T size) {
    if (flags & LMEM_MOVEABLE) {
        return LocalAlloc(flags, size);   // (a moveable block's is a handle, not memory)
    }
    return SysAlloc(size ? size : 1, (flags & LMEM_ZEROINIT) != 0);
}
HLOCAL WINAPI Hook_LocalFree(HLOCAL p) {
    if (p && SysFree(p)) {
        return nullptr;
    }
    return LocalFree(p);
}
HLOCAL WINAPI Hook_LocalReAlloc(HLOCAL p, SIZE_T size, UINT flags) {
    size_t capacity = p ? SysCapacity(p) : 0;
    if (capacity == 0) {
        return LocalReAlloc(p, size, flags);
    }
    if ((flags & LMEM_MODIFY) || size <= capacity) {
        return p;
    }
    if (!(flags & LMEM_MOVEABLE)) {
        return nullptr;   // (a fixed block that can't grow in place)
    }
    return SysRealloc(p, size, capacity, (flags & LMEM_ZEROINIT) != 0);
}

struct Hook {
    const char* name;
    void* fn;
    int original;   // the CrtFn its import is kept in, -1: none (the Heap and Local ones call kernel32's)
};
#define CRT_HOOK(name, hook, i) {name, reinterpret_cast<void*>(&hook<i>), i}
const Hook kHooks[] = {
    CRT_HOOK("malloc", Hook_malloc, kCrtMalloc),
    CRT_HOOK("_o_malloc", Hook_malloc, kCrtOMalloc),
    CRT_HOOK("calloc", Hook_calloc, kCrtCalloc),
    CRT_HOOK("_o_calloc", Hook_calloc, kCrtOCalloc),
    CRT_HOOK("realloc", Hook_realloc, kCrtRealloc),
    CRT_HOOK("_o_realloc", Hook_realloc, kCrtORealloc),
    CRT_HOOK("free", Hook_free, kCrtFree),
    CRT_HOOK("_o_free", Hook_free, kCrtOFree),
    CRT_HOOK("_msize", Hook_msize, kCrtMsize),
    CRT_HOOK("_o__msize", Hook_msize, kCrtOMsize),
    CRT_HOOK("??2@YAPEAX_K@Z", Hook_malloc, kCrtNew),           // operator new
    CRT_HOOK("??_U@YAPEAX_K@Z", Hook_malloc, kCrtNewArray),     // operator new[]
    CRT_HOOK("??3@YAXPEAX@Z", Hook_free, kCrtDelete),           // operator delete
    CRT_HOOK("??_V@YAXPEAX@Z", Hook_free, kCrtDeleteArray),     // operator delete[]
    {"HeapAlloc", reinterpret_cast<void*>(&Hook_HeapAlloc), -1},
    {"HeapFree", reinterpret_cast<void*>(&Hook_HeapFree), -1},
    {"HeapSize", reinterpret_cast<void*>(&Hook_HeapSize), -1},
    {"HeapReAlloc", reinterpret_cast<void*>(&Hook_HeapReAlloc), -1},
    {"LocalAlloc", reinterpret_cast<void*>(&Hook_LocalAlloc), -1},
    {"LocalFree", reinterpret_cast<void*>(&Hook_LocalFree), -1},
    {"LocalReAlloc", reinterpret_cast<void*>(&Hook_LocalReAlloc), -1},
};
#undef CRT_HOOK

// the DLLs whose objects end up in the game's fields (names lowercased, by prefix)
const char* const kSystemObjectDlls[] = {"d3d9.dll", "d3dx9_", "d3d10", "d3dx10_", "dinput", "xinput", "dsound", "d3dcompiler_"};

int PatchImports(HMODULE module) {
    auto* base = reinterpret_cast<uint8_t*>(module);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) {
        return 0;
    }
    int patched = 0;
    for (auto* d = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress); d->Name; d++) {
        if (!d->OriginalFirstThunk) {
            continue;   // (no names left to read)
        }
        auto* names = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + d->OriginalFirstThunk);
        auto* iat = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++) {
            if (IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal)) {
                continue;
            }
            const char* name = reinterpret_cast<const char*>(reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData)->Name);
            for (const Hook& h : kHooks) {
                if (std::strcmp(name, h.name) == 0) {
                    DWORD old;
                    void* was = reinterpret_cast<void*>(iat->u1.Function);
                    if (h.original >= 0) {
                        if (!g_original[h.original]) {
                            g_original[h.original] = was;
                        } else if (g_original[h.original] != was) {
                            // (another C runtime's import of the same name: its blocks made before the
                            // patch would go back to the first's; left alone)
                            std::fprintf(stderr, "x64 low heap: %s imported from two runtimes: left as it was here\n", name);
                            break;
                        }
                    }
                    if (VirtualProtect(&iat->u1.Function, sizeof iat->u1.Function, PAGE_READWRITE, &old)) {
                        iat->u1.Function = reinterpret_cast<ULONGLONG>(h.fn);
                        VirtualProtect(&iat->u1.Function, sizeof iat->u1.Function, old, &old);
                        patched++;
                    }
                    break;
                }
            }
        }
    }
    return patched;
}

SRWLOCK g_patchLock = SRWLOCK_INIT;
HMODULE g_patched[64];
int g_patchedCount = 0;

// ---- every module's frees. A block one of those DLLs made (ours) can be handed to a DLL whose
// imports aren't turned (quartz's to a decoder's, the audio session's) and given back to the
// system's heap there: ntdll's RtlFreeHeap, RtlReAllocateHeap and RtlSizeHeap (where HeapFree,
// LocalFree, free, CoTaskMemFree... end) hooked, ours sent back to us.

using RtlFreeHeap_t = BOOLEAN(NTAPI*)(PVOID, ULONG, PVOID);
using RtlReAllocateHeap_t = PVOID(NTAPI*)(PVOID, ULONG, PVOID, SIZE_T);
using RtlSizeHeap_t = SIZE_T(NTAPI*)(PVOID, ULONG, LPCVOID);
RtlFreeHeap_t g_rtlFreeHeap = nullptr;
RtlReAllocateHeap_t g_rtlReAllocateHeap = nullptr;
RtlSizeHeap_t g_rtlSizeHeap = nullptr;

// (without the lock: a unit outside our regions never becomes ours while the system's heap
// holds memory there; above 2 GB only the high blocks' range is looked up)
bool MaybeOurs(const void* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    if (a < kLimit) {
        return g_units[a >> kShift].kind != kOutside;
    }
    return a >= g_highLowest;
}

void NoteForeign(const char* what, const void* p) {
    AcquireSRWLockExclusive(&g_highLock);
    size_t n = ++g_highStats.foreign;
    ReleaseSRWLockExclusive(&g_highLock);
    if (n <= 8) {
        // (the first few said with their callers' modules)
        void* frames[6] = {};
        USHORT count = CaptureStackBackTrace(2, 6, frames, nullptr);
        char line[512];
        int at = std::snprintf(line, sizeof line, "x64 low heap: %s of our block %p from outside:", what, p);
        for (USHORT i = 0; i < count && at < int(sizeof line) - 64; i++) {
            HMODULE m = nullptr;
            char name[MAX_PATH] = "?";
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   static_cast<LPCSTR>(frames[i]), &m)) {
                K32GetModuleBaseNameA(GetCurrentProcess(), m, name, sizeof name);
            }
            at += std::snprintf(line + at, sizeof line - at, " %s+0x%llX", name,
                                static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(frames[i]) - reinterpret_cast<uintptr_t>(m)));
        }
        std::fprintf(stderr, "%s\n", line);
    }
}

BOOLEAN NTAPI Hook_RtlFreeHeap(PVOID heap, ULONG flags, PVOID p) {
    if (p && MaybeOurs(p) && SysFree(p)) {
        NoteForeign("a free", p);
        return TRUE;
    }
    return g_rtlFreeHeap(heap, flags, p);
}
PVOID NTAPI Hook_RtlReAllocateHeap(PVOID heap, ULONG flags, PVOID p, SIZE_T size) {
    size_t capacity = p && MaybeOurs(p) ? SysCapacity(p) : 0;
    if (capacity == 0) {
        return g_rtlReAllocateHeap(heap, flags, p, size);
    }
    NoteForeign("a realloc", p);
    if (size <= capacity) {
        return p;
    }
    if (flags & HEAP_REALLOC_IN_PLACE_ONLY) {
        return nullptr;
    }
    return SysRealloc(p, size, capacity, (flags & HEAP_ZERO_MEMORY) != 0);
}
SIZE_T NTAPI Hook_RtlSizeHeap(PVOID heap, ULONG flags, LPCVOID p) {
    if (p && MaybeOurs(p)) {
        if (size_t c = SysCapacity(p)) {
            return c;
        }
    }
    return g_rtlSizeHeap(heap, flags, p);
}

void HookSystemFrees() {
    static bool done = false;
    if (done) {
        return;
    }
    done = true;
    MH_STATUS s = MH_Initialize();
    if (s != MH_OK && s != MH_ERROR_ALREADY_INITIALIZED) {
        std::fprintf(stderr, "x64 low heap: MinHook: %s\n", MH_StatusToString(s));
        return;
    }
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    struct {
        const char* name;
        void* hook;
        void** original;
    } hooks[] = {
        {"RtlFreeHeap", reinterpret_cast<void*>(&Hook_RtlFreeHeap), reinterpret_cast<void**>(&g_rtlFreeHeap)},
        {"RtlReAllocateHeap", reinterpret_cast<void*>(&Hook_RtlReAllocateHeap), reinterpret_cast<void**>(&g_rtlReAllocateHeap)},
        {"RtlSizeHeap", reinterpret_cast<void*>(&Hook_RtlSizeHeap), reinterpret_cast<void**>(&g_rtlSizeHeap)},
    };
    for (const auto& h : hooks) {
        void* target = reinterpret_cast<void*>(GetProcAddress(ntdll, h.name));
        if (!target || (s = MH_CreateHook(target, h.hook, h.original)) != MH_OK || (s = MH_EnableHook(target)) != MH_OK) {
            std::fprintf(stderr, "x64 low heap: %s not hooked: %s\n", h.name, target ? MH_StatusToString(s) : "not found");
        }
    }
}

} // namespace

HighStats GetHighStats() {
    AcquireSRWLockShared(&g_highLock);
    HighStats s = g_highStats;
    ReleaseSRWLockShared(&g_highLock);
    return s;
}

void KeepSystemObjectsLow() {
    AcquireSRWLockExclusive(&g_patchLock);
    HookSystemFrees();
    HMODULE modules[512];
    DWORD bytes = 0;
    if (K32EnumProcessModules(GetCurrentProcess(), modules, sizeof modules, &bytes)) {
        for (DWORD i = 0; i < bytes / sizeof(HMODULE) && i < 512; i++) {
            HMODULE m = modules[i];
            bool done = false;
            for (int k = 0; k < g_patchedCount; k++) {
                done = done || g_patched[k] == m;
            }
            char name[MAX_PATH];
            if (done || !K32GetModuleBaseNameA(GetCurrentProcess(), m, name, sizeof name)) {
                continue;
            }
            for (char* c = name; *c; c++) {
                *c = char(*c >= 'A' && *c <= 'Z' ? *c + 32 : *c);
            }
            for (const char* prefix : kSystemObjectDlls) {
                if (std::strncmp(name, prefix, std::strlen(prefix)) == 0 && g_patchedCount < 64) {
                    g_patched[g_patchedCount++] = m;
                    int n = PatchImports(m);
                    std::fprintf(stderr, "x64 low heap: %s's heap imports turned to the low heap (%d)\n", name, n);
                    break;
                }
            }
        }
    }
    ReleaseSRWLockExclusive(&g_patchLock);
}

void KeepModuleObjectsLow(void* module) {
    HMODULE m = static_cast<HMODULE>(module);
    if (!m) {
        return;
    }
    AcquireSRWLockExclusive(&g_patchLock);
    bool done = false;
    for (int k = 0; k < g_patchedCount; k++) {
        done = done || g_patched[k] == m;
    }
    if (!done && g_patchedCount < 64) {
        g_patched[g_patchedCount++] = m;
        char name[MAX_PATH] = "?";
        K32GetModuleBaseNameA(GetCurrentProcess(), m, name, sizeof name);
        int n = PatchImports(m);
        std::fprintf(stderr, "x64 low heap: %s's heap imports turned to the low heap (%d)\n", name, n);
    }
    ReleaseSRWLockExclusive(&g_patchLock);
}

} // namespace low
} // namespace re

// ---- every new and delete of the 64-bit build's code: the low heap's (what our code makes is
// handed to the game's structures)

namespace {
void* NewOrThrow(size_t size) {
    if (void* p = re::low::Alloc(size)) {
        return p;
    }
    throw std::bad_alloc();
}
// over-aligned: the block's start stored just before the aligned pointer
void* NewAligned(size_t size, size_t alignment) {
    uint8_t* raw = static_cast<uint8_t*>(re::low::Alloc(size + alignment + sizeof(void*)));
    if (!raw) {
        return nullptr;
    }
    uintptr_t aligned = (reinterpret_cast<uintptr_t>(raw) + sizeof(void*) + alignment - 1) & ~uintptr_t(alignment - 1);
    reinterpret_cast<void**>(aligned)[-1] = raw;
    return reinterpret_cast<void*>(aligned);
}
void DeleteAligned(void* p) {
    if (p) {
        re::low::Free(static_cast<void**>(p)[-1]);
    }
}
} // namespace

void* operator new(size_t size) {
    return NewOrThrow(size);
}
void* operator new[](size_t size) {
    return NewOrThrow(size);
}
void* operator new(size_t size, const std::nothrow_t&) noexcept {
    return re::low::Alloc(size);
}
void* operator new[](size_t size, const std::nothrow_t&) noexcept {
    return re::low::Alloc(size);
}
void operator delete(void* p) noexcept {
    re::low::Free(p);
}
void operator delete[](void* p) noexcept {
    re::low::Free(p);
}
void operator delete(void* p, size_t) noexcept {
    re::low::Free(p);
}
void operator delete[](void* p, size_t) noexcept {
    re::low::Free(p);
}
void operator delete(void* p, const std::nothrow_t&) noexcept {
    re::low::Free(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept {
    re::low::Free(p);
}
void* operator new(size_t size, std::align_val_t alignment) {
    if (void* p = NewAligned(size, size_t(alignment))) {
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](size_t size, std::align_val_t alignment) {
    return operator new(size, alignment);
}
void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    return NewAligned(size, size_t(alignment));
}
void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    return NewAligned(size, size_t(alignment));
}
void operator delete(void* p, std::align_val_t) noexcept {
    DeleteAligned(p);
}
void operator delete[](void* p, std::align_val_t) noexcept {
    DeleteAligned(p);
}
void operator delete(void* p, size_t, std::align_val_t) noexcept {
    DeleteAligned(p);
}
void operator delete[](void* p, size_t, std::align_val_t) noexcept {
    DeleteAligned(p);
}
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept {
    DeleteAligned(p);
}
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept {
    DeleteAligned(p);
}

// ---- for a 64-bit renderer DLL (bullygfx64.dll): what it hands the game (the device it wraps,
// the thunks it puts in the game's 4-byte vtable and import slots) must be below 2 GB too.
// Found with GetProcAddress(GetModuleHandle(nullptr), ...).

extern "C" __declspec(dllexport) void* bullyre_low_alloc(size_t size) {
    return re::low::Alloc(size);
}
extern "C" __declspec(dllexport) void bullyre_low_free(void* p) {
    re::low::Free(p);
}
extern "C" __declspec(dllexport) void* bullyre_low_pages(size_t size, unsigned long protect) {
    return re::low::AllocPages(size, protect);
}

// A read of the process's own memory that can't fault (the renderer looks through the game's
// objects by address): a copy under SEH, which costs nothing unless it faults, for the system
// call ReadProcessMemory makes of each one. A guard page it touches (a thread's stack below its
// top) gets its guard back. 1: read, 0: not readable.
namespace {
int ReadFilter(EXCEPTION_POINTERS* e, const void** fault) {
    const EXCEPTION_RECORD* r = e->ExceptionRecord;
    if (r->ExceptionCode != EXCEPTION_ACCESS_VIOLATION && r->ExceptionCode != STATUS_GUARD_PAGE_VIOLATION &&
        r->ExceptionCode != EXCEPTION_IN_PAGE_ERROR) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    *fault = r->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION && r->NumberParameters >= 2
                 ? reinterpret_cast<const void*>(r->ExceptionInformation[1])
                 : nullptr;
    return EXCEPTION_EXECUTE_HANDLER;
}
}
extern "C" __declspec(dllexport) int bullyre_read(const void* address, void* out, size_t size) {
    const void* guard = nullptr;
    __try {
        std::memcpy(out, address, size);
        return 1;
    } __except (ReadFilter(GetExceptionInformation(), &guard)) {
        if (guard) {
            MEMORY_BASIC_INFORMATION m;
            DWORD old;
            if (VirtualQuery(guard, &m, sizeof m)) {
                VirtualProtect(const_cast<void*>(guard), 1, m.Protect | PAGE_GUARD, &old);
            }
        }
        return 0;
    }
}
