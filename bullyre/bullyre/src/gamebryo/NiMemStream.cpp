// NiMemStream (vtable 0x9A180C: 0 the deleting destructor, 1 operator bool (true), 2 Seek, 3
// GetPosition, 4 SetEndianSwap; 0x24 bytes): an NiBinaryStream over memory, what NiStream loads
// from and saves to a buffer. +0x10 the buffer, +0x14 the position, +0x18 the data's size, +0x1C
// the buffer's allocated size, +0x20 read-only (made over the caller's buffer), +0x21 whether the
// buffer was taken (Str: not freed then). Writing grows the buffer by the growth function (a
// static, 0xBCBB10: the default below), capped by 0xBCBB14.
//
// Also NiSearchPath (vtable 0x9A1820: 0 the deleting destructor, 1 Reset, 2 GetNextSearchPath;
// 0x210 bytes: +0x4 the next path's index, +0x8 the file path, +0x10C the reference path), the
// places NiStream looks for a file; NiThread::Suspend; and NiMain's pieces here: the B-spline
// control points' decompression, NiCullScene, NiDrawVisibleArray(Append).

#include "gamebryo/NiD3D10C44.h"
#include "gamebryo/NiFile.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr uintptr_t kMemStreamVtable = 0x009A180C;
constexpr uintptr_t kSearchPathVtable = 0x009A1820;
constexpr uint32_t kMemStreamSize = 0x24;
constexpr uint32_t kSearchPathSize = 0x210;
// the read / write functions a stream points at (the game's: the stream's users call through them)
constexpr uintptr_t kReadNoSwap = 0x008C5190;
constexpr uintptr_t kWriteNoSwap = 0x008C51D0;
constexpr uintptr_t kReadAndSwap = 0x008C51F0;
constexpr uintptr_t kWriteAndSwap = 0x008C5250;

using GrowthFn = uint32_t(__cdecl*)(uint32_t allocated, uint32_t needed, uint32_t available);
GAME_VAR(GrowthFn, ms_pfnGrowth, 0x00BCBB10);
GAME_VAR(uint32_t, ms_uiMaxGrowth, 0x00BCBB14);
GAME_ARRAY(char, ms_acDefaultPath, 0x020E93B0);   // NiSearchPath's default (a static)
GAME_VAR(void*, ms_pkRenderer, 0x020E2380);       // NiRenderer::ms_pkRenderer
constexpr uintptr_t kSuspendThread = 0x008FC054;

GAME_FN(void, __thiscall, BinaryStream_Construct, 0x007529C0, void* self);
GAME_FN(void, __thiscall, BinaryStream_Destruct, 0x007529E0, void* self);
GAME_FN(void, __cdecl, BinaryStream_DoByteSwap, 0x00752A80, void* data, uint32_t bytes, uint32_t* componentSizes,
        uint32_t components);
GAME_FN(uint8_t, __cdecl, Path_Standardize, 0x00752C20, char* path);
GAME_FN(void, __thiscall, Filename_Construct, 0x007538C0, NiFilename* self, const char* path);
GAME_FN(uint8_t, __thiscall, Filename_GetFullPath, 0x00753AD0, NiFilename* self, char* out, uint32_t size);
GAME_FN(void, __thiscall, Renderer_SetCameraData, 0x00755CB0, void* renderer, void* camera);
// this file's (called where modes apply)
GAME_FN(uint32_t, __thiscall, MemStream_Write, 0x008C5110, void* self, const void* buffer, uint32_t bytes);
GAME_FN(void, __thiscall, MemStream_Destruct, 0x008C5030, void* self);
GAME_FN(uint32_t, __cdecl, NiDrawVisibleArrayAppendFn, 0x008C57C0, void* visible);

struct MemStream {
    re::Ptr32<void> vtable;                  // +0x0
    uint32_t absoluteCurrentPos;   // +0x4
    re::Ptr32<void> read;                    // +0x8
    re::Ptr32<void> write;                   // +0xC
    re::Ptr32<uint8_t> buffer;               // +0x10
    uint32_t pos;                  // +0x14
    uint32_t end;                  // +0x18
    uint32_t allocated;            // +0x1C
    uint8_t readOnly;              // +0x20
    uint8_t taken;                 // +0x21
    uint8_t pad22[2];
};
static_assert(offsetof(MemStream, buffer) == 0x10);
static_assert(sizeof(MemStream) == kMemStreamSize);

} // namespace

// 0x8C5030 (thiscall): NiMemStream's destructor (its buffer freed unless the caller's or taken)
void __fastcall MemStream_DestructImpl(MemStream* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kMemStreamVtable);
    if (self->readOnly == 0 && self->taken == 0) {
        NiFree(self->buffer);
    }
    BinaryStream_Destruct(self);
}

// 0x8C5060 (thiscall): Str (the buffer, taken: the stream won't free it)
uint8_t* __fastcall MemStream_Str(MemStream* self, re::Edx) {
    self->taken = 1;
    return self->buffer;
}

// 0x8C5070 (thiscall, slot 2): Seek (by an offset from the position, inside the allocation)
void __fastcall MemStream_Seek(MemStream* self, re::Edx, int32_t offset) {
    uint32_t pos = self->pos + offset;
    if (int32_t(pos) >= 0 && pos < self->allocated) {
        self->pos = pos;
        self->absoluteCurrentPos = pos;
    }
}

// 0x8C5090 (cdecl): the default growth (the buffer doubled when that's enough, else grown to fit;
// by at most the cap when the cap fits)
uint32_t __cdecl MemStream_DefaultGrowth(uint32_t allocated, uint32_t needed, uint32_t available) {
    uint32_t size = needed <= allocated + available ? allocated + allocated : allocated - available + needed;
    if (size - allocated > ms_uiMaxGrowth && needed - available < ms_uiMaxGrowth) {
        size = ms_uiMaxGrowth + allocated;
    }
    return size;
}

// 0x8C50D0 (thiscall, slot 0): the deleting destructor
void* __fastcall MemStream_DeletingDestructor(MemStream* self, re::Edx, uint32_t flags) {
    self->vtable = reinterpret_cast<void*>(kMemStreamVtable);
    if (self->readOnly == 0 && self->taken == 0) {
        NiFree(self->buffer);
    }
    BinaryStream_Destruct(self);
    if ((flags & 1) != 0) {
        NiDelete(self, kMemStreamSize);
    }
    return self;
}

// 0x8C5110 (thiscall): Write (0 to a read-only one; the buffer grown when it's short: the data
// copied over as far as its size)
uint32_t __fastcall MemStream_WriteImpl(MemStream* self, re::Edx, const void* data, uint32_t bytes) {
    if (self->readOnly != 0) {
        return 0;
    }
    uint32_t available = self->allocated - self->pos;
    if (bytes > available) {
        self->allocated = ms_pfnGrowth(self->allocated, bytes, available);
        uint8_t* block = static_cast<uint8_t*>(NiMalloc(self->allocated));
        crt::MemcpyS(block, self->end, self->buffer, self->end);
        NiFree(self->buffer);
        self->buffer = block;
    }
    crt::MemcpyS(self->buffer + self->pos, bytes, data, bytes);
    self->pos += bytes;
    if (self->pos > self->end) {
        self->end = self->pos;
    }
    return bytes;
}

// 0x8C5190 (cdecl): the read function (as much as there is)
uint32_t __cdecl MemStream_ReadNoSwap(MemStream* stream, void* buffer, uint32_t bytes, uint32_t*, uint32_t) {
    uint32_t left = stream->end - stream->pos;
    if (bytes > left) {
        bytes = left;
    }
    crt::MemcpyS(buffer, bytes, stream->buffer + stream->pos, bytes);
    stream->pos += bytes;
    return bytes;
}

// 0x8C51D0 (cdecl): the write function
uint32_t __cdecl MemStream_WriteNoSwap(MemStream* stream, const void* buffer, uint32_t bytes, uint32_t*, uint32_t) {
    return MemStream_Write(stream, buffer, bytes);
}

// 0x8C51F0 (cdecl): the read function swapping each component's bytes (all the bytes asked for)
uint32_t __cdecl MemStream_ReadAndSwap(MemStream* stream, void* buffer, uint32_t bytes, uint32_t* componentSizes,
                                       uint32_t components) {
    if (bytes == 0) {
        return 0;
    }
    uint32_t read = stream->end - stream->pos;
    if (bytes <= read) {
        read = bytes;
    }
    crt::MemcpyS(buffer, read, stream->buffer + stream->pos, read);
    stream->pos += read;
    BinaryStream_DoByteSwap(buffer, bytes, componentSizes, components);
    return read;
}

// 0x8C5250 (cdecl): the write function swapping each component's bytes (through a copy)
uint32_t __cdecl MemStream_WriteAndSwap(MemStream* stream, const void* buffer, uint32_t bytes,
                                        uint32_t* componentSizes, uint32_t components) {
    if (bytes == 0) {
        return 0;
    }
    void* copy = NiMalloc(bytes);
    crt::MemcpyS(copy, bytes, buffer, bytes);
    BinaryStream_DoByteSwap(copy, bytes, componentSizes, components);
    uint32_t written = MemStream_Write(stream, copy, bytes);
    NiFree(copy);
    return written;
}

// 0x8C52B0 (thiscall, slot 4): SetEndianSwap (the swapping functions or the plain ones)
void __fastcall MemStream_SetEndianSwap(MemStream* self, re::Edx, int swap) {
    if (re::Flag(swap)) {
        self->read = reinterpret_cast<void*>(kReadAndSwap);
        self->write = reinterpret_cast<void*>(kWriteAndSwap);
        return;
    }
    self->read = reinterpret_cast<void*>(kReadNoSwap);
    self->write = reinterpret_cast<void*>(kWriteNoSwap);
}

// 0x8C52E0 (thiscall): the constructor over a buffer (read-only)
MemStream* __fastcall MemStream_ConstructRead(MemStream* self, re::Edx, uint8_t* buffer, uint32_t size) {
    BinaryStream_Construct(self);
    self->buffer = buffer;
    self->allocated = size;
    self->end = size;
    self->vtable = reinterpret_cast<void*>(kMemStreamVtable);
    self->pos = 0;
    self->readOnly = 1;
    self->taken = 0;
    self->read = reinterpret_cast<void*>(kReadNoSwap);
    self->write = reinterpret_cast<void*>(kWriteNoSwap);
    return self;
}

// 0x8C5320 (thiscall): the constructor for writing (1 KB to start)
MemStream* __fastcall MemStream_ConstructWrite(MemStream* self, re::Edx) {
    BinaryStream_Construct(self);
    self->vtable = reinterpret_cast<void*>(kMemStreamVtable);
    self->allocated = 0x400;
    self->buffer = static_cast<uint8_t*>(NiMalloc(0x400));
    self->pos = 0;
    self->end = 0;
    self->readOnly = 0;
    self->taken = 0;
    self->read = reinterpret_cast<void*>(kReadNoSwap);
    self->write = reinterpret_cast<void*>(kWriteNoSwap);
    return self;
}

// 0x8C5380 (thiscall): NiThread::Suspend (-1 without a thread; suspended, +0x18, when it worked)
uint32_t __fastcall Thread_Suspend(uint8_t* self, re::Edx) {
    void* handle = At<void*>(self, 0x20);
    if (handle == nullptr) {
        return 0xFFFFFFFF;
    }
    uint32_t result = Import<uint32_t>(kSuspendThread, handle);
    if (result != 0xFFFFFFFF) {
        At<uint32_t>(self, 0x18) = 1;
    }
    return result;
}

// 0x8C53B0 (thiscall): NiSearchPath's constructor
void* __fastcall SearchPath_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kSearchPathVtable;
    At<uint32_t>(self, 4) = 0;
    At<char>(self, 8) = 0;
    At<char>(self, 0x10C) = 0;
    return self;
}

// 0x8C53D0 (thiscall, slot 0): NiSearchPath's deleting destructor
void* __fastcall SearchPath_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kSearchPathVtable;
    if ((flags & 1) != 0) {
        NiDelete(self, kSearchPathSize);
    }
    return self;
}

// 0x8C5400 (thiscall): SetFilePath (standardized; empty without one)
void __fastcall SearchPath_SetFilePath(uint8_t* self, re::Edx, const char* path) {
    if (path == nullptr || *path == '\0') {
        At<char>(self, 8) = 0;
        return;
    }
    char* to = reinterpret_cast<char*>(self + 8);
    crt::StrncpyS(to, 0x104, path, 0x103);
    Path_Standardize(to);
}

// 0x8C5440 (thiscall): SetReferencePath (the path's drive and directory, standardized)
void __fastcall SearchPath_SetReferencePath(uint8_t* self, re::Edx, const char* path) {
    if (path == nullptr || *path == '\0') {
        At<char>(self, 0x10C) = 0;
        return;
    }
    NiFilename filename;
    Filename_Construct(&filename, path);
    crt::StrcpyS(filename.fname, 0x100, Text(0x008FEFBB));   // ""
    crt::StrcpyS(filename.ext, 0x100, Text(0x008FEFBB));
    char* to = reinterpret_cast<char*>(self + 0x10C);
    Filename_GetFullPath(&filename, to, 0x104);
    Path_Standardize(to);
}

// 0x8C5500 (thiscall, slot 2): GetNextSearchPath (the file path as given, then its name alone, then
// under the reference path with its own directory below it, under the reference path, under the
// default path when there is one; false when they're used up)
uint8_t __fastcall SearchPath_GetNextSearchPath(uint8_t* self, re::Edx, char* out, uint32_t size) {
    const char* empty = Text(0x008FEFBB);
    const char* filePath = reinterpret_cast<const char*>(self + 8);
    const char* referencePath = reinterpret_cast<const char*>(self + 0x10C);
    NiFilename filename;
    Filename_Construct(&filename, filePath);
    switch (At<uint32_t>(self, 4)) {
    case 0:
        break;
    case 1:
        crt::StrcpyS(filename.drive, 3, empty);
        crt::StrcpyS(filename.dir, 0x100, empty);
        break;
    case 2: {
        crt::StrcpyS(filename.drive, 3, empty);
        crt::StrcpyS(filename.dir, 0x100, referencePath);
        NiFilename own;
        Filename_Construct(&own, filePath);
        crt::StrcpyS(filename.platformSubDir, 0x100, own.dir);
        break;
    }
    case 3:
        crt::StrcpyS(filename.drive, 3, empty);
        crt::StrcpyS(filename.dir, 0x100, referencePath);
        crt::StrcpyS(filename.platformSubDir, 0x100, empty);
        break;
    case 4:
        if (ms_acDefaultPath[0] == '\0') {
            return 0;
        }
        crt::StrcpyS(filename.drive, 3, empty);
        crt::StrcpyS(filename.dir, 0x100, ms_acDefaultPath);
        break;
    default:
        return 0;
    }
    Filename_GetFullPath(&filename, out, size);
    Path_Standardize(out);
    At<uint32_t>(self, 4) += 1;
    return 1;
}

// 0x8C5690 (cdecl): NiBSplineData's compact control points decompressed (each short over 32767,
// times the half range, plus the offset; in float, the main thread's precision)
void __cdecl BSpline_DecompressControlPoints(const int16_t* source, uint32_t, float offset, float halfRange,
                                             float* out, int32_t count) {
    uint32_t i = 0;
    if (count >= 4) {
        uint32_t blocks = (uint32_t(count - 4) >> 2) + 1;
        i = blocks * 4;
        const int16_t* s = source;
        float* d = out;
        for (uint32_t b = 0; b < blocks; b++) {
            d[0] = float(s[0]) / 32767.0f * halfRange + offset;
            d[1] = float(s[1]) / 32767.0f * halfRange + offset;
            d[2] = float(s[2]) / 32767.0f * halfRange + offset;
            d[3] = float(s[3]) / 32767.0f * halfRange + offset;
            s += 4;
            d += 4;
        }
    }
    for (; i < uint32_t(count); i++) {
        out[i] = float(source[i]) / 32767.0f * halfRange + offset;
    }
}

// 0x8C5760 (cdecl): NiCullScene (the renderer's camera set; the culler's Process, slot 3, into the
// visible array, emptied first when asked; the objects added)
uint32_t __cdecl NiCullScene(void* camera, void* scene, void* culler, uint8_t* visible, int reset) {
    if (ms_pkRenderer == nullptr || camera == nullptr || scene == nullptr) {
        return 0;
    }
    Renderer_SetCameraData(ms_pkRenderer, camera);
    uint32_t before = 0;
    if (re::Flag(reset)) {
        At<uint32_t>(visible, 4) = 0;
    } else {
        before = At<uint32_t>(visible, 4);
    }
    Virtual<void>(culler, 3, camera, scene, visible);
    return At<uint32_t>(visible, 4) - before;
}

// 0x8C57C0 (cdecl): NiDrawVisibleArrayAppend (to the renderer's sorter, slot 20; without one each
// object drawn, slot 34; the count)
uint32_t __cdecl NiDrawVisibleArrayAppend(uint8_t* visible) {
    void* renderer = ms_pkRenderer;
    if (renderer == nullptr) {
        return 0;
    }
    void* sorter = At<void*>(renderer, 8);
    if (sorter != nullptr) {
        Increment(&At<uint32_t>(sorter, 4));
        Virtual<void>(sorter, 20, visible);   // RegisterObjectArray
    } else {
        uint32_t count = At<uint32_t>(visible, 4);
        for (uint32_t i = 0; i < count; i++) {
            Virtual<void>(At<re::Ptr32<void>*>(visible, 0)[i], 34, renderer);   // RenderImmediate
        }
    }
    uint32_t count = At<uint32_t>(visible, 4);
    ReleaseRef(sorter);
    return count;
}

// 0x8C5850 (cdecl): NiDrawVisibleArray (the sorter started for the camera, slot 18; the array
// appended; the sorter finished, slot 19; the count)
uint32_t __cdecl NiDrawVisibleArray(void* camera, uint8_t* visible) {
    if (ms_pkRenderer == nullptr || camera == nullptr) {
        return 0;
    }
    void* sorter = At<void*>(ms_pkRenderer, 8);
    if (sorter != nullptr) {
        Increment(&At<uint32_t>(sorter, 4));
        Virtual<void>(sorter, 18, camera);   // StartAccumulating
    }
    NiDrawVisibleArrayAppendFn(visible);
    if (sorter != nullptr) {
        Virtual<void>(sorter, 19);   // FinishAccumulating
    }
    uint32_t count = At<uint32_t>(visible, 4);
    ReleaseRef(sorter);
    return count;
}

// 0x8C5370: a lone jmp to NiThread::Suspend
RE_UNHOOKED("NiThread::Suspend_Thunk", 0x008C5370);

#define GB44S2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44S2_REVERSED("NiMemStream::~NiMemStream", 0x008C5030, MemStream_DestructImpl);
GB44S2_REVERSED("NiMemStream::Str", 0x008C5060, MemStream_Str);
GB44S2_REVERSED("NiMemStream::Seek", 0x008C5070, MemStream_Seek);
GB44S2_REVERSED("NiMemStream::DefaultGrowth", 0x008C5090, MemStream_DefaultGrowth);
GB44S2_REVERSED("NiMemStream::DeletingDestructor", 0x008C50D0, MemStream_DeletingDestructor);
GB44S2_REVERSED("NiMemStream::Write", 0x008C5110, MemStream_WriteImpl);
GB44S2_REVERSED("NiMemStream::ReadNoSwap", 0x008C5190, MemStream_ReadNoSwap);
GB44S2_REVERSED("NiMemStream::WriteNoSwap", 0x008C51D0, MemStream_WriteNoSwap);
GB44S2_REVERSED("NiMemStream::ReadAndSwap", 0x008C51F0, MemStream_ReadAndSwap);
GB44S2_REVERSED("NiMemStream::WriteAndSwap", 0x008C5250, MemStream_WriteAndSwap);
GB44S2_REVERSED("NiMemStream::SetEndianSwap", 0x008C52B0, MemStream_SetEndianSwap);
GB44S2_REVERSED("NiMemStream::NiMemStream_Read", 0x008C52E0, MemStream_ConstructRead);
GB44S2_REVERSED("NiMemStream::NiMemStream_Write", 0x008C5320, MemStream_ConstructWrite);
GB44S2_REVERSED("NiThread::Suspend", 0x008C5380, Thread_Suspend);
GB44S2_REVERSED("NiSearchPath::NiSearchPath", 0x008C53B0, SearchPath_Construct);
GB44S2_REVERSED("NiSearchPath::DeletingDestructor", 0x008C53D0, SearchPath_DeletingDestructor);
GB44S2_REVERSED("NiSearchPath::SetFilePath", 0x008C5400, SearchPath_SetFilePath);
GB44S2_REVERSED("NiSearchPath::SetReferencePath", 0x008C5440, SearchPath_SetReferencePath);
GB44S2_REVERSED("NiSearchPath::GetNextSearchPath", 0x008C5500, SearchPath_GetNextSearchPath);
GB44S2_REVERSED("NiBSplineData::DecompressControlPoints", 0x008C5690, BSpline_DecompressControlPoints);
GB44S2_REVERSED("NiCullScene", 0x008C5760, NiCullScene);
GB44S2_REVERSED("NiDrawVisibleArrayAppend", 0x008C57C0, NiDrawVisibleArrayAppend);
GB44S2_REVERSED("NiDrawVisibleArray", 0x008C5850, NiDrawVisibleArray);

} // namespace ni::c44
