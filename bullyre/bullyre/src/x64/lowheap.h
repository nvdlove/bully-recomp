#pragma once

// The low heap (bullyre_x64 only): memory below 2 GB, the only memory a game structure may
// point at (its pointer fields are 4 bytes: re::Ptr32, sign-extended). The process itself is
// large-address-aware, so the system's own allocations (the graphics driver, its buffers and
// textures, sound) may use the rest of the address space; whatever the game's data holds comes
// from here: the game's malloc family (crt64.cpp), every new/delete of ours, the dispatch's
// islands. lowheap.cpp says how it's laid out.

#include <cstddef>
#include <cstdint>

namespace re::low {

void* Alloc(size_t size);                 // 16-byte aligned; nullptr when the low space is gone
void Free(void* p);                       // (a pointer of ours or nullptr; others are reported once and kept)
void* Realloc(void* p, size_t size);
size_t Capacity(const void* p);           // the usable size of a block of ours, else 0
bool Owns(const void* p);
void* AllocPages(size_t size, unsigned long protect);   // whole pages (code made at run time)

// The system DLLs whose objects the game's fields hold (Direct3D 9, D3DX, DirectInput, ...):
// their heap imports (malloc, HeapAlloc, LocalAlloc and the rest) turned to the low heap for
// blocks under 256 KB, so the device, the textures and the buffers they make come back below
// 2 GB; larger blocks (the data, not the objects) stay theirs. Every loaded DLL of the list
// is done once; called after the imports are bound and after each LoadLibrary of the game's.
void KeepSystemObjectsLow();
// the same for one DLL (a COM class's, loaded before the class makes its object)
void KeepModuleObjectsLow(void* module);

struct Stats {
    size_t reserved;      // address space held below 2 GB
    size_t committed;     // of it, backed by memory
    size_t inUse;         // handed out in blocks (their size classes' sizes)
    size_t blocks;
    bool stress;          // the rest of the space below 4 GB held empty (--stress-high)
};
Stats GetStats();

// the system DLLs' large blocks (their data, above 4 GB: lowheap.cpp)
struct HighStats {
    size_t blocks, bytes;   // live
    size_t allocated;       // VirtualAlloc'd in all
    size_t reused;          // handed out again from the ones kept
    size_t released;        // VirtualFree'd
    size_t foreign;         // blocks of ours a DLL without turned imports gave to the system's heap
                            // (ntdll's RtlFreeHeap / RtlReAllocateHeap / RtlSizeHeap: back to us)
};
HighStats GetHighStats();

} // namespace re::low
