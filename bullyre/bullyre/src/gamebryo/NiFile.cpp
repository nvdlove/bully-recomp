// NiBinaryStream and NiFile: the buffered C FILE the library's streams read and write, the
// byte swapping of the endian-swapped read/write functions, and NiFile's static hooks (the
// file factory, Access, CreateDirectory, DirectoryExists: function pointers Bully can replace).

#include "gamebryo/NiFile.h"

#include <cstring>

#include "gamebryo/NiSystem.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t kNiBinaryStreamVtable = 0x00950FE4;
constexpr uintptr_t kNiFileVtable = 0x00950FC4;

GAME_FN(void, __thiscall, NiBinaryStream_Construct, 0x007529C0, NiBinaryStream* self);
GAME_FN(void, __thiscall, NiBinaryStream_Destruct, 0x007529E0, NiBinaryStream* self);
GAME_FN(void, __cdecl, NiBinaryStream_DoByteSwap, 0x00752A80, void* data, uint32_t bytes, uint32_t* componentSizes,
        uint32_t components);
GAME_FN(void, __cdecl, NiEndian_Swap64, 0x00752A00, uint8_t* data, uint32_t count);
GAME_FN(uint8_t, __thiscall, NiFile_Flush, 0x00752630, NiFile* self);
GAME_FN(uint32_t, __thiscall, NiFile_Read, 0x007527E0, NiFile* self, void* buffer, uint32_t bytes);
GAME_FN(uint32_t, __thiscall, NiFile_Write, 0x007528A0, NiFile* self, const void* buffer, uint32_t bytes);
GAME_FN(void, __thiscall, NiFile_SetEndianSwap, 0x007534B0, NiFile* self, int swap);
GAME_FN(uint8_t, __cdecl, NiPath_Standardize, 0x00752C20, char* path);

// the read/write functions without swapping (lone forwarders to Read and Write)
constexpr uintptr_t kFileRead = 0x00753300;
constexpr uintptr_t kFileWrite = 0x00753320;
constexpr uintptr_t kFileReadAndSwap = 0x00753340;
constexpr uintptr_t kFileWriteAndSwap = 0x00753450;

// NiFile's static hooks (0x753390 NiNew's a NiFile, 0x7533D0, 0x7526E0 CreateDirectoryA,
// 0x752700 GetFileAttributesA)
using FileCreateFn = void*(__cdecl*)(const char*, int, uint32_t);
using FileAccessFn = uint8_t(__cdecl*)(const char*, int);
using DirFn = uint8_t(__cdecl*)(const char*);
GAME_VAR(FileCreateFn, ms_pfnFileCreateFunc, 0x00BC74EC);
GAME_VAR(FileAccessFn, ms_pfnFileAccessFunc, 0x00BC74F0);
GAME_VAR(DirFn, ms_pfnCreateDirFunc, 0x00BC74F4);
GAME_VAR(DirFn, ms_pfnDirExistsFunc, 0x00BC74F8);
GAME_VAR(int, ms_iSeekCur, 0x00950FBC);

} // namespace

// ---- NiBinaryStream

// 0x7529C0
NiBinaryStream* __fastcall NiBinaryStream_ConstructImpl(NiBinaryStream* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiBinaryStreamVtable);
    self->read = nullptr;
    self->write = nullptr;
    self->absoluteCurrentPos = 0;
    return self;
}

// 0x7529E0
void __fastcall NiBinaryStream_DestructImpl(NiBinaryStream* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiBinaryStreamVtable);
}

// 0x7529F0: the platform's endianness the streams compare their files with (a byte set at
// start-up)
uint8_t __cdecl NiSystemDesc_IsLittleEndian() {
    return *reinterpret_cast<uint8_t*>(0x020E20A8);
}

// 0x752A00: NiEndian::Swap64, `count` 8-byte values reversed in place
void __cdecl NiEndian_Swap64Impl(uint8_t* data, uint32_t count) {
    for (; count != 0; count--) {
        for (int i = 0; i < 4; i++) {
            uint8_t t = data[i];
            data[i] = data[7 - i];
            data[7 - i] = t;
        }
        data += 8;
    }
}

// 0x752A50: NiBinaryStream's deleting destructor
NiBinaryStream* __fastcall NiBinaryStream_Destroy(NiBinaryStream* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kNiBinaryStreamVtable);
    if (flags & 1) {
        NiDelete(self, sizeof(NiBinaryStream));
    }
    return self;
}

// 0x752A80: the components of each element swapped (sizes 2, 4 and 8; others skipped), over
// `bytes`. (No components with bytes to do loops forever, as the game's does.)
void __cdecl NiBinaryStream_DoByteSwapImpl(uint8_t* data, uint32_t bytes, uint32_t* componentSizes,
                                           uint32_t components) {
    if (components == 1 && componentSizes[0] == 1) {
        return;
    }
    uint32_t done = 0;
    if (bytes == 0) {
        return;
    }
    do {
        for (uint32_t i = 0; i < components; i++) {
            uint32_t size = componentSizes[i];
            if (size == 2) {
                uint8_t t = data[0];
                data[0] = data[1];
                data[1] = t;
            } else if (size == 4) {
                uint8_t t = data[0];
                data[0] = data[3];
                data[3] = t;
                t = data[1];
                data[1] = data[2];
                data[2] = t;
            } else if (size == 8) {
                NiEndian_Swap64(data, 1);
            }
            data += size;
            done += size;
        }
    } while (done < bytes);
}

// 0x752B10: NiBinaryStream::GetLine, a line without its \r and \n (at most maxBytes - 1
// characters); the bytes read
uint32_t __fastcall NiBinaryStream_GetLine(NiBinaryStream* self, re::Edx, char* buffer, uint32_t maxBytes) {
    uint32_t total = 0;
    uint32_t written = 0;
    uint32_t next = 1;
    if (maxBytes <= 1) {
        buffer[0] = 0;
        return 0;
    }
    for (;;) {
        char c;
        uint32_t componentSize = 1;
        uint32_t got = self->read(self, &c, 1, &componentSize, 1);
        self->absoluteCurrentPos += got;
        total += got;
        if (got != 1 || c == '\n') {
            break;
        }
        if (c != '\r') {
            buffer[next - 1] = c;
            written++;
            next++;
        }
        if (next >= maxBytes) {
            break;
        }
    }
    buffer[written] = 0;
    return total;
}

// 0x752BD0: NiBinaryStream::PutS, the characters written one at a time; how many went
uint32_t __fastcall NiBinaryStream_PutS(NiBinaryStream* self, re::Edx, const char* s) {
    uint32_t count = 0;
    while (*s != 0) {
        uint32_t componentSize = 1;
        uint32_t put = self->write(self, s, 1, &componentSize, 1);
        self->absoluteCurrentPos += put;
        s++;
        if (put != 1) {
            break;
        }
        count += put;
    }
    return count;
}

// ---- NiFile

// 0x752590: NiFile(path, mode, bufferSize)
NiFile* __fastcall NiFile_Construct(NiFile* self, re::Edx, const char* path, int mode, uint32_t bufferSize) {
    NiBinaryStream_Construct(self);
    self->vtable = reinterpret_cast<void*>(kNiFileVtable);
    NiFile_SetEndianSwap(self, 0);
    self->mode = mode;
    const char* how = mode == 0 ? "rb" : mode == 1 ? "wb" : "ab";
    int error = FopenS(&self->file, path, how);
    uint8_t good = error == 0 && self->file != nullptr ? 1 : 0;
    self->good = good;
    self->bufferAllocSize = bufferSize;
    self->bufferReadSize = 0;
    self->pos = 0;
    if (good && bufferSize > 0) {
        self->buffer = static_cast<char*>(NiMalloc(bufferSize));
    } else {
        self->buffer = nullptr;
    }
    return self;
}

// 0x752630: NiFile::Flush (reading: the buffer dropped; writing: what's in it written)
uint8_t __fastcall NiFile_FlushImpl(NiFile* self, re::Edx) {
    if (self->mode == 0) {
        self->bufferReadSize = 0;
    } else if (self->pos > 0) {
        if (Fwrite(self->buffer, 1, self->pos, self->file) != self->pos) {
            self->good = 0;
            return 0;
        }
    }
    self->pos = 0;
    return 1;
}

// 0x752680: NiFile::GetFileSize (0 when ftell fails)
uint32_t __fastcall NiFile_GetFileSize(NiFile* self, re::Edx) {
    long here = Ftell(self->file);
    if (here < 0) {
        return 0;
    }
    Fseek(self->file, 0, 2);
    long size = Ftell(self->file);
    Fseek(self->file, here, 0);
    return size < 0 ? 0 : uint32_t(size);
}

// 0x752720: ~NiFile (flushed and closed if it opened; the buffer freed)
void __fastcall NiFile_Destruct(NiFile* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kNiFileVtable);
    if (self->good && self->file != nullptr) {
        NiFile_Flush(self);
        crt::Fclose(static_cast<FILE*>(self->file));
    }
    NiFree(self->buffer);
    NiBinaryStream_Destruct(self);
}

// 0x752760: NiFile::Seek(offset, origin): within the read buffer it just moves; otherwise
// flushed and fseek'd (a relative seek in read mode corrected for what was buffered)
void __fastcall NiFile_Seek(NiFile* self, re::Edx, int offset, int origin) {
    if (!self->good) {
        return;
    }
    if (origin == 1) {
        int newPos = int(self->pos) + offset;
        if (newPos >= 0 && newPos < int(self->bufferReadSize)) {
            self->absoluteCurrentPos += offset;
            self->pos = uint32_t(newPos);
            return;
        }
        if (self->mode == 0) {
            offset += int(self->pos) - int(self->bufferReadSize);
        }
    }
    NiFile_Flush(self);
    int error = Fseek(self->file, offset, origin);
    self->good = error == 0 ? 1 : 0;
    if (error == 0) {
        self->absoluteCurrentPos = uint32_t(Ftell(self->file));
    }
}

// 0x7527E0: NiFile::Read: from the buffer, refilled; a read bigger than the buffer goes
// straight to the file
uint32_t __fastcall NiFile_ReadImpl(NiFile* self, re::Edx, void* buffer, uint32_t bytes) {
    if (!self->good) {
        return 0;
    }
    uint8_t* to = static_cast<uint8_t*>(buffer);
    uint32_t available = self->bufferReadSize - self->pos;
    uint32_t done = 0;
    if (bytes > available) {
        if (available > 0) {
            crt::MemcpyS(to, available, self->buffer + self->pos, available);
            to += available;
            bytes -= available;
            done = available;
        }
        NiFile_Flush(self);
        if (bytes > self->bufferAllocSize) {
            return uint32_t(Fread(to, 1, bytes, self->file)) + done;
        }
        uint32_t got = uint32_t(Fread(self->buffer, 1, self->bufferAllocSize, self->file));
        self->bufferReadSize = got;
        if (got < bytes) {
            bytes = got;
        }
    }
    crt::MemcpyS(to, bytes, self->buffer + self->pos, bytes);
    self->pos += bytes;
    return done + bytes;
}

// 0x7528A0: NiFile::Write: into the buffer, flushed when full; a write at least the buffer's
// size goes straight to the file
uint32_t __fastcall NiFile_WriteImpl(NiFile* self, re::Edx, const void* buffer, uint32_t bytes) {
    if (!self->good) {
        return 0;
    }
    const uint8_t* from = static_cast<const uint8_t*>(buffer);
    uint32_t room = self->bufferAllocSize - self->pos;
    uint32_t done = 0;
    if (bytes > room) {
        if (room != 0) {
            crt::MemcpyS(self->buffer + self->pos, room, from, room);
            from += room;
            bytes -= room;
            self->pos = self->bufferAllocSize;
            done = room;
        }
        if (!NiFile_Flush(self)) {
            return 0;
        }
        if (bytes >= self->bufferAllocSize) {
            return uint32_t(Fwrite(from, 1, bytes, self->file)) + done;
        }
    }
    crt::MemcpyS(self->buffer + self->pos, bytes, from, bytes);
    self->pos += bytes;
    return done + bytes;
}

// 0x752960: NiFile's deleting destructor
NiFile* __fastcall NiFile_Destroy(NiFile* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<void*>(kNiFileVtable);
    if (self->good && self->file != nullptr) {
        NiFile_Flush(self);
        crt::Fclose(static_cast<FILE*>(self->file));
    }
    NiFree(self->buffer);
    NiBinaryStream_Destruct(self);
    if (flags & 1) {
        NiDelete(self, sizeof(NiFile));
    }
    return self;
}

// 0x753340: the swapping read function (the bytes asked for swapped, whatever was read)
uint32_t __cdecl NiFile_FileReadAndSwap(NiBinaryStream* stream, void* buffer, uint32_t bytes,
                                        uint32_t* componentSizes, uint32_t components) {
    if (bytes == 0) {
        return 0;
    }
    uint32_t got = NiFile_Read(static_cast<NiFile*>(stream), buffer, bytes);
    NiBinaryStream_DoByteSwap(buffer, bytes, componentSizes, components);
    return got;
}

// 0x753450: the swapping write function (a swapped copy written)
uint32_t __cdecl NiFile_FileWriteAndSwap(NiBinaryStream* stream, const void* buffer, uint32_t bytes,
                                         uint32_t* componentSizes, uint32_t components) {
    if (bytes == 0) {
        return 0;
    }
    void* copy = NiMalloc(bytes);
    crt::MemcpyS(copy, bytes, buffer, bytes);
    NiBinaryStream_DoByteSwap(copy, bytes, componentSizes, components);
    uint32_t put = NiFile_Write(static_cast<NiFile*>(stream), copy, bytes);
    NiFree(copy);
    return put;
}

// 0x753380, 0x7533C0, 0x753430, 0x753440: NiFile::GetFile, Access, CreateDirectory,
// DirectoryExists, lone jumps through the hooks
void* __cdecl NiFile_GetFile(const char* path, int mode, uint32_t bufferSize) {
    return ms_pfnFileCreateFunc(path, mode, bufferSize);
}
uint8_t __cdecl NiFile_Access(const char* path, int mode) {
    return ms_pfnFileAccessFunc(path, mode);
}
uint8_t __cdecl NiFile_CreateDirectory(const char* path) {
    return ms_pfnCreateDirFunc(path);
}
uint8_t __cdecl NiFile_DirectoryExists(const char* path) {
    return ms_pfnDirExistsFunc(path);
}
RE_UNHOOKED_FN("NiFile::GetFile", 0x00753380, &NiFile_GetFile);
RE_UNHOOKED_FN("NiFile::Access", 0x007533C0, &NiFile_Access);
RE_UNHOOKED_FN("NiFile::CreateDirectory", 0x00753430, &NiFile_CreateDirectory);
RE_UNHOOKED_FN("NiFile::DirectoryExists", 0x00753440, &NiFile_DirectoryExists);

// 0x753400: NiFile::operator bool (4 bytes)
uint8_t __fastcall NiFile_IsGood(NiFile* self, re::Edx) {
    return self->good;
}
RE_UNHOOKED_FN("NiFile::operator bool", 0x00753400, &NiFile_IsGood);

// 0x753410: NiFile::Seek(offset): relative, through slot 5
void __fastcall NiFile_SeekRelative(NiFile* self, re::Edx, int offset) {
    Virtual<void>(self, 5, offset, ms_iSeekCur);
}

// 0x7534B0: NiFile::SetEndianSwap (the read/write functions chosen)
void __fastcall NiFile_SetEndianSwapImpl(NiFile* self, re::Edx, int swap) {
    if (re::Flag(swap)) {
        self->read = reinterpret_cast<NiBinaryStreamReadFn>(kFileReadAndSwap);
        self->write = reinterpret_cast<NiBinaryStreamWriteFn>(kFileWriteAndSwap);
        return;
    }
    self->read = reinterpret_cast<NiBinaryStreamReadFn>(kFileRead);
    self->write = reinterpret_cast<NiBinaryStreamWriteFn>(kFileWrite);
}

// 0x7534E0: NiFile::CreateDirectoryRecursive, each level made in turn (paths over 0x104 fail).
// (The game scans all 0x104 bytes of its copy, past the terminator into its uninitialised
// stack: a separator there only repeats the whole path's check. Ours is zeroed.)
uint8_t __cdecl NiFile_CreateDirectoryRecursive(const char* path) {
    if (ms_pfnDirExistsFunc(path)) {
        return 1;
    }
    if (std::strlen(path) > 0x104) {
        return 0;
    }
    char buffer[0x104] = {};
    crt::StrcpyS(buffer, 0x104, path);
    NiPath_Standardize(buffer);
    uint32_t i = 0;
    if (buffer[1] == ':') {
        i = 2;
    }
    while ((buffer[i] == '\\' || buffer[i] == '/') && ++i < 0x104) {
    }
    for (; i < 0x104; i++) {
        char c = buffer[i];
        if (c == '/' || c == '\\') {
            buffer[i] = 0;
            if (!ms_pfnDirExistsFunc(buffer) && !ms_pfnCreateDirFunc(buffer)) {
                return 0;
            }
            buffer[i] = c;
        }
    }
    if (ms_pfnDirExistsFunc(buffer) || ms_pfnCreateDirFunc(buffer)) {
        return 1;
    }
    return 0;
}

#define NIFILE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NIFILE_REVERSED("NiFile::NiFile", 0x00752590, NiFile_Construct);
NIFILE_REVERSED("NiFile::Flush", 0x00752630, NiFile_FlushImpl);
NIFILE_REVERSED("NiFile::GetFileSize", 0x00752680, NiFile_GetFileSize);
NIFILE_REVERSED("NiFile::~NiFile", 0x00752720, NiFile_Destruct);
NIFILE_REVERSED("NiFile::Seek", 0x00752760, NiFile_Seek);
NIFILE_REVERSED("NiFile::Read", 0x007527E0, NiFile_ReadImpl);
NIFILE_REVERSED("NiFile::Write", 0x007528A0, NiFile_WriteImpl);
NIFILE_REVERSED("NiFile::Destroy", 0x00752960, NiFile_Destroy);
NIFILE_REVERSED("NiBinaryStream::NiBinaryStream", 0x007529C0, NiBinaryStream_ConstructImpl);
NIFILE_REVERSED("NiBinaryStream::~NiBinaryStream", 0x007529E0, NiBinaryStream_DestructImpl);
NIFILE_REVERSED("NiSystemDesc::IsLittleEndian", 0x007529F0, NiSystemDesc_IsLittleEndian);
NIFILE_REVERSED("NiEndian::Swap64", 0x00752A00, NiEndian_Swap64Impl);
NIFILE_REVERSED("NiBinaryStream::Destroy", 0x00752A50, NiBinaryStream_Destroy);
NIFILE_REVERSED("NiBinaryStream::DoByteSwap", 0x00752A80, NiBinaryStream_DoByteSwapImpl);
NIFILE_REVERSED("NiBinaryStream::GetLine", 0x00752B10, NiBinaryStream_GetLine);
NIFILE_REVERSED("NiBinaryStream::PutS", 0x00752BD0, NiBinaryStream_PutS);
NIFILE_REVERSED("NiFile::FileReadAndSwap", 0x00753340, NiFile_FileReadAndSwap);
NIFILE_REVERSED("NiFile::SeekRelative", 0x00753410, NiFile_SeekRelative);
NIFILE_REVERSED("NiFile::FileWriteAndSwap", 0x00753450, NiFile_FileWriteAndSwap);
NIFILE_REVERSED("NiFile::SetEndianSwap", 0x007534B0, NiFile_SetEndianSwapImpl);
NIFILE_REVERSED("NiFile::CreateDirectoryRecursive", 0x007534E0, NiFile_CreateDirectoryRecursive);

} // namespace ni
