#pragma once

// The debugging phase's heap validator (src/game/HeapCheck.cpp), on with bullyre.ini
// "heap_check = N" (N ms between heap walks).

#include <cstdint>

namespace heapcheck {

// true while heap_check is on: CMemoryHeap::Free checks each block, the object tracer records
extern bool g_validateFrees;
// true with bullyre.ini "heap_check_history = 1": every common block allocation and free
// records a short stack (slows the game enough to hide the 2026-10-30 race: off by default)
extern bool g_history;
// true with "heap_check_quarantine = 1": freed common block items are held back (filled either way;
// the hold starves the common blocks enough to distort the renderer, so it is opt-in)
extern bool g_quarantine;

void Start(int intervalMs);

// The object tracer: a ring of the last allocations and frees of the objects a file chooses
// to trace (the NiDynamicEffectStates, freed twice in the 2026-10-30 soaks), each with the
// code addresses found on the stack at the time. Report(p) logs p's recorded events.
// kAddRef / kRelease are recorded by the traced code around its own AddRef / Release (the
// count shown is the object's at the time: before a release, after an add-ref); kFreed when
// a destructor has finished
enum Kind : uint8_t { kAlloc = 1, kFree = 2, kAddRef = 3, kRelease = 4, kFreed = 5 };
void Trace(const void* object, Kind kind);
void Report(const void* object);
// true when the last recorded event for the object is a free (freed again without a new
// allocation in between): the caller reports and stops
bool WasFreed(const void* object);
// this thread's code addresses (what a crash report shows), under a report
void LogStack();

// The concurrent-entry detector (the 2026-10-30 soaks: a node or geometry whose effect state
// two threads update at once would release a state twice): Enter(key, what) notes this thread
// is in `what` for key, and reports another thread already in it, with both stacks (16 reports
// at most); Leave(key) when done
void Enter(const void* key, const char* what);
void Leave(const void* key);

// The quarantine for the small allocator's 8-byte blocks (the effect states' list nodes, one
// of which was read after its free in the 2026-10-30 soaks): Quarantine(p) keeps a freed
// block aside, filled with 0xDD, and hands back the oldest one to free for real once 65536 are
// held (null otherwise); the heap-check thread finds a held block written to and logs it with
// the free's stack. ReportQuarantine(p) logs the free that put p in it, if any
void* Quarantine(void* p);
void ReportQuarantine(const void* p);
// the object a thread is freeing list nodes for (a state in its destructor, a list in a
// removal), kept with the quarantine's record of each free and reported with it
void SetContext(const void* object);
const void* Context();

// for other files' detectors: this thread's code addresses, and their logging
void CaptureStack(uintptr_t* out, int count);
void CaptureStackShort(uintptr_t* out, int count);   // (the nearest frames only: cheap)
void PrintFrames(const uintptr_t* frames, int count);

// The memory manager's common blocks (CommonBlocks.cpp), where Bully's NiAllocator puts every
// small Gamebryo allocation, the effect states and their list nodes among them: under
// heap_check each free is validated (an item start of a block in use, not free already), the
// item filled with 0xDD and held back in a per-heap quarantine of 16384; the heap-check thread
// (CheckCommonBlocks) reports a held item written to, with the free's stack and context.
// ReportCommonBlock(p) logs the free that holds p, if any
void CheckCommonBlocks();
void ReportCommonBlock(const void* p);
// a pointer about to be used that the quarantine holds (freed) is reported, with the free's
// stack and this one's, and the game stopped
void AssertLive(const void* p, const char* what);
// each common block item's last allocation, the free before it and the allocation before
// that (stacks and threads), kept in a hashed table: ReportHistory(p) logs them, so a block
// found holding another owner's data names that owner
void ReportHistory(const void* p);
// an indexed write into a small array owned by an object (a weapon's child slots): the object
// and the array must be live and the index within the capacity, else reported and stopped
void CheckIndex(const void* object, const void* array, int index, int capacity, const char* what);
// a pointer read as a live small object (an effect state's list node): when it lies in a
// common blocks region it must start an item of a block in use that isn't filled from a
// free; otherwise reported (with the item before it in the list, if given) and the game stopped
void CheckItem(const void* p, const void* previous, const char* what);

} // namespace heapcheck
