// A heap validator for the debugging phase (bullyre.ini "heap_check = N": every N ms; 0 off):
// a thread walks the memory manager's four heaps under their locks and logs the first block
// whose header is inconsistent, with its neighbours' tags, so an overrun into a header is
// caught near its cause rather than at the next free (the soak of 2026-10-30: a free block
// with no list links removed in CombineFreeBlocks, on an OpenMP worker).
//
// Checked per block, start to end: the size a multiple of 16 and within the heap, the next
// consecutive block's back pointer, and a free block (memId 0) linked into the free list
// both ways. Then the free list, head to end: each entry free, each link's back link.

#include <windows.h>
#include <intrin.h>

#include <cstdint>
#include <cstring>

#include "game/HeapCheck.h"
#include "game/MemoryMgr.h"
#include "log.h"
#include "reversed.h"

namespace heapcheck {

// CMemoryHeap::Free checks each block it's given while this is set (MemoryMgr.cpp)
bool g_validateFrees = false;
bool g_history = false;
bool g_quarantine = true;

namespace {

struct Named {
    const char* name;
    CMemoryHeap* heap;
};

bool InHeap(const CMemoryHeap& h, const void* p) {
    return p >= static_cast<const void*>(h.start) && p <= static_cast<const void*>(h.end);
}

void Report(const char* heap, const char* what, const HeapBlockDesc* b, const HeapBlockDesc* before) {
    re::Log("heap_check %s: %s at block %p (size 0x%X memId %d next %p prev %p prevConsecutive %p); the block before "
            "it: %p (size 0x%X memId %d)",
            heap, what, static_cast<const void*>(b), unsigned(b->size), int(b->memId), static_cast<const void*>(b->next),
            static_cast<const void*>(b->prev), static_cast<const void*>(b->prevConsecutive),
            static_cast<const void*>(before), before ? unsigned(before->size) : 0u, before ? int(before->memId) : 0);
}

// one pass over a heap; true when something is wrong (reported once per pass)
bool Check(const Named& n) {
    CMemoryHeap& h = *n.heap;
    if (h.start == nullptr || h.end == nullptr) {
        return false;
    }
    auto* lock = reinterpret_cast<CRITICAL_SECTION*>(h.lock);
    EnterCriticalSection(lock);
    bool bad = false;
    const HeapBlockDesc* before = nullptr;
    uint32_t blocks = 0;
    for (HeapBlockDesc* b = h.start; b != h.end; b = b->GetNextConsecutive()) {
        if (++blocks > 4000000) {
            Report(n.name, "no end reached", b, before);
            bad = true;
            break;
        }
        if ((b->size & 0xF) != 0 || b->size > h.totalSize) {
            Report(n.name, "bad size", b, before);
            bad = true;
            break;
        }
        HeapBlockDesc* next = b->GetNextConsecutive();
        if (!InHeap(h, next)) {
            Report(n.name, "next consecutive outside the heap", b, before);
            bad = true;
            break;
        }
        if (next->prevConsecutive != b) {
            Report(n.name, "next block's back pointer wrong", b, before);
            bad = true;
            break;
        }
        if (b->memId == 0) {
            if (b->next == nullptr || b->prev == nullptr) {
                Report(n.name, "free block not in the free list", b, before);
                bad = true;
                break;
            }
            if (b->prev->next != b || b->next->prev != b) {
                Report(n.name, "free block's list links don't agree", b, before);
                bad = true;
                break;
            }
        }
        before = b;
    }
    if (!bad) {
        uint32_t steps = 0;
        for (HeapBlockDesc* b = h.freeList.next; b != &h.freeListEnd; b = b->next) {
            if (b == nullptr || ++steps > 4000000) {
                re::Log("heap_check %s: the free list doesn't reach its end (after %u entries)", n.name, steps);
                bad = true;
                break;
            }
            if (b->memId != 0 || !InHeap(h, b)) {
                Report(n.name, "free list entry not a free block of the heap", b, nullptr);
                bad = true;
                break;
            }
            if (b->next == nullptr || b->next->prev != b) {
                Report(n.name, "free list link without its back link", b, nullptr);
                bad = true;
                break;
            }
        }
    }
    LeaveCriticalSection(lock);
    return bad;
}

void CheckQuarantine();

DWORD WINAPI Thread(LPVOID param) {
    const DWORD interval = DWORD(reinterpret_cast<uintptr_t>(param));
    const Named heaps[] = {{"main", &CMemoryMgr::ms_mainHeap},
                           {"simple", &CMemoryMgr::ms_simpleHeap},
                           {"heap28", &CMemoryMgr::ms_heap28},
                           {"actiontree", &CMemoryMgr::ms_actionTreeHeap}};
    bool reported[4] = {};
    uint32_t passes = 0;
    for (;;) {
        Sleep(interval);
        if (!CMemoryMgr::ms_bInitialised) {
            continue;
        }
        for (int i = 0; i < 4; i++) {
            if (!reported[i] && Check(heaps[i])) {
                reported[i] = true;   // (once: the first finding is the one that matters)
                re::Log("heap_check: %s heap corrupt after %u passes; checks of it stop here", heaps[i].name, passes);
            }
        }
        CheckQuarantine();
        CheckCommonBlocks();
        passes++;
        if (passes == 1 || passes % 240 == 0) {
            re::Log("heap_check: %u passes, all heaps consistent so far: %s %s %s %s", passes, reported[0] ? "BAD" : "ok",
                    reported[1] ? "BAD" : "ok", reported[2] ? "BAD" : "ok", reported[3] ? "BAD" : "ok");
        }
    }
}

} // namespace

// ---- the object tracer ------------------------------------------------------------------

namespace {

struct Event {
    const void* object;
    Kind kind;
    int32_t count;          // the object's reference count (its +4) when recorded
    uint32_t thread;
    uintptr_t frames[12];   // code addresses found on the stack (module+offset when logged)
};
const char* const kKindNames[] = {"?", "alloc", "free", "addref", "release", "freed"};
constexpr uint32_t kRing = 32768;
Event g_ring[kRing];
volatile uint32_t g_ringNext = 0;
volatile long g_ringLock = 0;

void Spin(volatile long* lock) {
    while (_InterlockedExchange(lock, 1) != 0) {
        YieldProcessor();
    }
}

void Unlock(volatile long* lock) {
    _InterlockedExchange(lock, 0);   // (a barrier: the writes under the lock are done before it opens)
}

bool IsCode(uintptr_t v) {
    // the game's code, or ours
    static uintptr_t ourBase = 0, ourEnd = 0;
    if (ourBase == 0) {
        HMODULE m = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&IsCode), &m);
        ourBase = uintptr_t(m);
        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(m);
        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(ourBase + dos->e_lfanew);
        ourEnd = ourBase + nt->OptionalHeader.SizeOfImage;
    }
    return (v >= 0x401000 && v < 0x8FC000) || (v >= ourBase && v < ourEnd);
}

// the code addresses on this thread's stack from here up (what a crash report shows)
void ScanStack(uintptr_t* out, int count, uint32_t span = 0x200) {
    int n = 0;
    auto tib = reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    auto* sp = static_cast<const uintptr_t*>(_AddressOfReturnAddress());
    auto* top = static_cast<const uintptr_t*>(tib->StackBase);
    for (const uintptr_t* p = sp; p < top && n < count && p < sp + span; p++) {
        if (p >= out && p < out + count) {
            continue;   // (the output itself lies on this stack: not read back)
        }
        if (IsCode(*p)) {
            out[n++] = *p;
        }
    }
    for (; n < count; n++) {
        out[n] = 0;
    }
}

void LogFrames(const uintptr_t* frames, int count) {
    for (int i = 0; i < count && frames[i] != 0; i++) {
        HMODULE module = nullptr;
        char name[MAX_PATH] = "?";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(frames[i]), &module)) {
            GetModuleFileNameA(module, name, MAX_PATH);
        }
        const char* base = strrchr(name, '\\');
        re::Log("      %s+0x%X", base ? base + 1 : name, unsigned(frames[i] - uintptr_t(module)));
    }
}

} // namespace

void Trace(const void* object, Kind kind) {
    if (!g_validateFrees || object == nullptr) {
        return;
    }
    Event e;
    e.object = object;
    e.kind = kind;
    e.count = *reinterpret_cast<const int32_t*>(static_cast<const uint8_t*>(object) + 4);
    e.thread = GetCurrentThreadId();
    ScanStack(e.frames, 12);
    Spin(&g_ringLock);
    g_ring[g_ringNext % kRing] = e;
    g_ringNext = g_ringNext + 1;
    Unlock(&g_ringLock);
}

bool WasFreed(const void* object) {
    if (!g_validateFrees || object == nullptr) {
        return false;
    }
    bool freed = false;
    Spin(&g_ringLock);
    uint32_t next = g_ringNext;
    uint32_t n = next < kRing ? next : kRing;
    for (uint32_t i = 0; i < n; i++) {   // newest first
        const Event& e = g_ring[(next - 1 - i) % kRing];
        if (e.object == object) {
            freed = e.kind == kFree || e.kind == kFreed;
            break;
        }
    }
    Unlock(&g_ringLock);
    return freed;
}

void Report(const void* object) {
    Spin(&g_ringLock);
    uint32_t next = g_ringNext;
    uint32_t n = next < kRing ? next : kRing;
    int shown = 0;
    re::Log("heap_check: the traced events of %p, newest first:", object);
    for (uint32_t i = 0; i < n && shown < 24; i++) {
        const Event& e = g_ring[(next - 1 - i) % kRing];
        if (e.object != object) {
            continue;
        }
        re::Log("    %s (count %d) on thread %u:", kKindNames[e.kind <= kFreed ? e.kind : 0], int(e.count), unsigned(e.thread));
        LogFrames(e.frames, e.kind == kAddRef || e.kind == kRelease ? 5 : 12);
        shown++;
    }
    if (shown == 0) {
        re::Log("    (none recorded)");
    }
    Unlock(&g_ringLock);
}

void LogStack() {
    uintptr_t frames[12];
    ScanStack(frames, 12);
    LogFrames(frames, 12);
}

// ---- the concurrent-entry detector ------------------------------------------------------

namespace {

struct Live {
    const void* key;
    uint32_t thread;
    const char* what;
    uintptr_t frames[8];
};
Live g_live[1024];
uint32_t g_liveCount = 0;
volatile long g_liveLock = 0;
int g_liveReports = 0;

} // namespace

void Enter(const void* key, const char* what) {
    if (!g_validateFrees || key == nullptr) {
        return;
    }
    Live e;
    e.key = key;
    e.thread = GetCurrentThreadId();
    e.what = what;
    ScanStack(e.frames, 8, 0x60);
    Live other = {};
    bool found = false;
    Spin(&g_liveLock);
    for (uint32_t i = 0; i < g_liveCount; i++) {
        if (g_live[i].key == key) {
            other = g_live[i];
            found = true;
            break;
        }
    }
    if (g_liveCount < 1024) {
        g_live[g_liveCount++] = e;
    }
    bool report = found && g_liveReports < 16;
    if (report) {
        g_liveReports++;
    }
    Unlock(&g_liveLock);
    if (report) {
        re::Log("heap_check: %s on %p entered by thread %u while thread %u is in %s on it%s", what, key, unsigned(e.thread),
                unsigned(other.thread), other.what, other.thread == e.thread ? " (the same thread: a cycle)" : "");
        re::Log("    the other thread's stack:");
        LogFrames(other.frames, 8);
        re::Log("    this thread's stack:");
        LogFrames(e.frames, 8);
    }
}

// ---- the quarantine ---------------------------------------------------------------------

namespace {

struct Held {
    void* p;
    const void* context;
    uint32_t thread;
    uintptr_t frames[8];
};
__declspec(thread) const void* t_context = nullptr;
constexpr uint32_t kHeld = 65536;
Held g_held[kHeld];
uint32_t g_heldNext = 0;   // (the slot the next one goes to; the oldest when full)
bool g_heldFull = false;
volatile long g_heldLock = 0;
int g_heldReports = 0;

} // namespace

void SetContext(const void* object) {
    t_context = object;
}

const void* Context() {
    return t_context;
}

void CaptureStack(uintptr_t* out, int count) {
    ScanStack(out, count);
}

void CaptureStackShort(uintptr_t* out, int count) {
    ScanStack(out, count, 0x60);
}

void PrintFrames(const uintptr_t* frames, int count) {
    LogFrames(frames, count);
}

void* Quarantine(void* p) {
    Held h;
    h.p = p;
    h.context = t_context;
    h.thread = GetCurrentThreadId();
    ScanStack(h.frames, 8);
    // a block already filled is one freed before and not handed out since: freed twice
    const uint32_t* w = static_cast<const uint32_t*>(p);
    if (w[0] == 0xDDDDDDDDu && w[1] == 0xDDDDDDDDu) {
        Held first = {};
        bool found = false;
        Spin(&g_heldLock);
        uint32_t n = g_heldFull ? kHeld : g_heldNext;
        for (uint32_t i = 0; i < n; i++) {
            if (g_held[i].p == p) {
                first = g_held[i];
                found = true;
                g_held[i].p = nullptr;   // (dropped: this free's record takes over)
            }
        }
        Unlock(&g_heldLock);
        re::Log("heap_check: 8-byte block %p freed twice (%s): now by thread %u for %p:", p,
                found ? "held in the quarantine since its first free" : "filled, first free not held any more",
                unsigned(h.thread), h.context);
        LogFrames(h.frames, 8);
        if (h.context != nullptr) {
            heapcheck::Report(h.context);
        }
        if (found) {
            re::Log("    its first free, by thread %u for %p:", unsigned(first.thread), first.context);
            LogFrames(first.frames, 8);
            if (first.context != nullptr && first.context != h.context) {
                heapcheck::Report(first.context);
            }
        }
        *static_cast<volatile int*>(nullptr) = 0;
    }
    std::memset(p, 0xDD, 8);
    void* out = nullptr;
    Spin(&g_heldLock);
    if (g_heldFull) {
        out = g_held[g_heldNext].p;   // (the oldest; null when it was reported and dropped)
    }
    g_held[g_heldNext] = h;
    g_heldNext++;
    if (g_heldNext == kHeld) {
        g_heldNext = 0;
        g_heldFull = true;
    }
    Unlock(&g_heldLock);
    return out;
}

void ReportQuarantine(const void* p) {
    Spin(&g_heldLock);
    uint32_t n = g_heldFull ? kHeld : g_heldNext;
    bool found = false;
    const void* context = nullptr;
    for (uint32_t i = 0; i < n; i++) {
        const Held& h = g_held[i];
        if (h.p == p) {
            re::Log("    %p was freed (and is held in the quarantine) by thread %u, for %p:", p, unsigned(h.thread),
                    h.context);
            LogFrames(h.frames, 8);
            found = true;
            context = h.context;
        }
    }
    Unlock(&g_heldLock);
    if (!found) {
        re::Log("    %p is not in the quarantine (not a freed 8-byte block, or freed too long ago)", p);
    } else if (context != nullptr) {
        Report(context);
    }
}

namespace {

void CheckQuarantine() {
    Held bad[4];
    uint32_t words[4][2];
    int count = 0;
    Spin(&g_heldLock);
    uint32_t n = g_heldFull ? kHeld : g_heldNext;
    for (uint32_t i = 0; i < n && count < 4; i++) {
        Held& h = g_held[i];
        if (h.p == nullptr) {
            continue;
        }
        const uint32_t* w = static_cast<const uint32_t*>(h.p);
        if (w[0] != 0xDDDDDDDDu || w[1] != 0xDDDDDDDDu) {
            bad[count] = h;
            words[count][0] = w[0];
            words[count][1] = w[1];
            count++;
            h.p = nullptr;   // (dropped: not handed back to the allocator, never reported again)
        }
    }
    Unlock(&g_heldLock);
    for (int i = 0; i < count && g_heldReports < 16; i++) {
        g_heldReports++;
        re::Log("heap_check: freed 8-byte block %p written after its free: now %08X %08X; it was freed by thread %u "
                "for %p:",
                bad[i].p, words[i][0], words[i][1], unsigned(bad[i].thread), bad[i].context);
        LogFrames(bad[i].frames, 8);
        if (bad[i].context != nullptr) {
            heapcheck::Report(bad[i].context);
        }
    }
}

} // namespace

void Leave(const void* key) {
    if (!g_validateFrees || key == nullptr) {
        return;
    }
    uint32_t thread = GetCurrentThreadId();
    Spin(&g_liveLock);
    for (uint32_t i = g_liveCount; i > 0; i--) {   // the latest entry of this thread for the key
        if (g_live[i - 1].key == key && g_live[i - 1].thread == thread) {
            g_live[i - 1] = g_live[--g_liveCount];
            break;
        }
    }
    Unlock(&g_liveLock);
}

void Start(int intervalMs) {
    if (intervalMs <= 0) {
        return;
    }
    g_validateFrees = true;
    g_history = re::IniInt("heap_check_history", 0) != 0;
    g_quarantine = re::IniInt("heap_check_quarantine", 0) != 0;   // (the hold distorts the renderer: opt-in)
    HANDLE t = CreateThread(nullptr, 0, &Thread, reinterpret_cast<LPVOID>(uintptr_t(intervalMs)), 0, nullptr);
    if (t) {
        CloseHandle(t);
        re::Log("heap_check: on, every %d ms%s", intervalMs, g_history ? ", with the allocation history" : "");
    }
}

} // namespace heapcheck
