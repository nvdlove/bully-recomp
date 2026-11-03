#pragma once

// NiBinaryStream (what NiStream reads and writes through) and NiFile (one on a C FILE, with
// its own buffer), as the exe has them. Layouts from the constructors 0x7529C0 and 0x752590.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {

struct NiBinaryStream;

// the stream's read and write functions: `bytes` in `components` parts of the given sizes
// (swapped one by one when the stream's endianness differs); the bytes moved
using NiBinaryStreamReadFn = uint32_t(__cdecl*)(NiBinaryStream* stream, void* buffer, uint32_t bytes,
                                               uint32_t* componentSizes, uint32_t components);
using NiBinaryStreamWriteFn = uint32_t(__cdecl*)(NiBinaryStream* stream, const void* buffer, uint32_t bytes,
                                                uint32_t* componentSizes, uint32_t components);

// NiBinaryStream (vtable 0x950FE4): 0 the destructor, 1 operator bool, 2 Seek(int), 3 GetPosition,
// 4 SetEndianSwap(bool); NiFile adds 5 Seek(int offset, int origin), 6 GetFileSize
struct NiBinaryStream {
    re::Ptr32<void> vtable;                   // +0x0
    uint32_t absoluteCurrentPos;    // +0x4
    re::Ptr32<uint32_t __cdecl(NiBinaryStream* stream, void* buffer, uint32_t bytes, uint32_t* componentSizes, uint32_t components)> read;      // +0x8
    re::Ptr32<uint32_t __cdecl(NiBinaryStream* stream, const void* buffer, uint32_t bytes, uint32_t* componentSizes, uint32_t components)> write;    // +0xC
};
static_assert(sizeof(NiBinaryStream) == 0x10);

// NiFile (vtable 0x950FC4, 0x2C bytes from NiNew)
struct NiFile : NiBinaryStream {
    uint32_t bufferAllocSize;   // +0x10
    uint32_t bufferReadSize;    // +0x14 (read mode: how much of the buffer holds file data)
    uint32_t pos;               // +0x18 (in the buffer)
    re::Ptr32<char> buffer;               // +0x1C (NiMalloc)
    re::Ptr32<void> file;                 // +0x20 (the game's FILE)
    int mode;                   // +0x24 (0 read, 1 write, 2 append)
    uint8_t good;               // +0x28
};
static_assert(offsetof(NiFile, bufferAllocSize) == 0x10);
static_assert(offsetof(NiFile, good) == 0x28);
static_assert(sizeof(NiFile) == 0x2C);

// NiFilename: a path split by _splitpath_s, plus a platform subdirectory put between the
// directory and the name
struct NiFilename {
    char dir[0x100];              // +0x0
    char drive[3];                // +0x100
    char ext[0x100];              // +0x103
    char fname[0x100];            // +0x203
    char platformSubDir[0x100];   // +0x303
};
static_assert(offsetof(NiFilename, ext) == 0x103);
static_assert(offsetof(NiFilename, platformSubDir) == 0x303);
static_assert(sizeof(NiFilename) == 0x403);

} // namespace ni
