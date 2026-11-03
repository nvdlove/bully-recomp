// MdNiOverrideAllocator: the game's NiAllocator for Gamebryo (its vtable slots 1-5):
// everything on the game's own heap (CMemoryMgr), no tracking.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline void* __cdecl Memcpy(void* dst, const void* src, size_t size) { return crt::Memcpy(dst, src, size); }
} // namespace

namespace NiAllocator {

// vf1 (0x43B280): Allocate(size&, alignment, event, provideSize, file, line, function) (the size the
// game's 4-byte size_t)
static void* __fastcall Allocate(void*, re::Edx, uint32_t* size, size_t, int, int, const char*, int, const char*) {
    return Malloc(*size);
}

// vf2 (0x43B2A0): Deallocate(p, event, size)
static void __fastcall Deallocate(void*, re::Edx, void* p, int, size_t) {
    Free(p);
}

// vf3 (0x43B2B0): Reallocate(p, size&, alignment, event, provideSize, currentSize, file,
// line, function): a new block, the smaller of the two sizes copied, the old one freed
static void* __fastcall Reallocate(void*, re::Edx, void* p, uint32_t* size, size_t, int, int, size_t currentSize,
                                   const char*, int, const char*) {
    void* block = Malloc(*size);
    size_t copy = *size;
    if (currentSize < copy) {
        copy = currentSize;
    }
    Memcpy(block, p, copy);
    Free(p);
    return block;
}

// vf4 (0x43B2F0): TrackAllocate
static uint8_t __fastcall TrackAllocate(void*, re::Edx, const void*, size_t, int, const char*, int, const char*) {
    return 1;
}

// vf5 (0x43B300): TrackDeallocate
static uint8_t __fastcall TrackDeallocate(void*, re::Edx, const void*, int) {
    return 1;
}

} // namespace NiAllocator

// ---- registration ----------------------------------------------------------------------

#define NIALLOC_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace NiAllocator {
NIALLOC_REVERSED("MdNiOverrideAllocator::Allocate", 0x0043B280, &Allocate);
NIALLOC_REVERSED("MdNiOverrideAllocator::Deallocate", 0x0043B2A0, &Deallocate);
NIALLOC_REVERSED("MdNiOverrideAllocator::Reallocate", 0x0043B2B0, &Reallocate);
NIALLOC_REVERSED("MdNiOverrideAllocator::TrackAllocate", 0x0043B2F0, &TrackAllocate);
NIALLOC_REVERSED("MdNiOverrideAllocator::TrackDeallocate", 0x0043B300, &TrackDeallocate);
} // namespace NiAllocator
