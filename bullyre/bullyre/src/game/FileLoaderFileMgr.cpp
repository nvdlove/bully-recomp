// CFileMgr: the game's file access (see FileLoader.h)

#include <windows.h>

#include <cstdarg>
#include <cstring>

#include "game/FileLoader.h"

#include "game/Crt.h"
namespace {
// The game's imports: read at each call (the mod loader hooks them)
GAME_VAR(decltype(&::CloseHandle), Import_CloseHandle, 0x008FC050);
GAME_VAR(decltype(&::GetLastError), Import_GetLastError, 0x008FC060);
GAME_VAR(decltype(&::CreateFileA), Import_CreateFileA, 0x008FC064);
GAME_VAR(decltype(&::SetFilePointer), Import_SetFilePointer, 0x008FC068);
GAME_VAR(decltype(&::ReadFile), Import_ReadFile, 0x008FC06C);
GAME_VAR(decltype(&::WriteFile), Import_WriteFile, 0x008FC070);

// the path under the game's root, in a static buffer
GAME_FN(char*, __cdecl, GetFullPath, 0x00829900, const char* path);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// the CRT's _vsnprintf
inline int __cdecl VsnPrintf(char* buffer, size_t size, const char* format, va_list args) { return crt::Vsnprintf(buffer, size, format, args); }
// (CFileMgr's own, called where modes apply)
GAME_FN(FileHandle, __cdecl, File_OpenFileRaw, 0x0042D040, const char* path, const char* mode);

const char* const modeRead = reinterpret_cast<const char*>(0x008FF548);   // "r"
const char* const modeWrite = reinterpret_cast<const char*>(0x00901B00);  // "w"
const char* const modeWriteBinary = reinterpret_cast<const char*>(0x00901AE8);  // "wb"
} // namespace

namespace CFileMgr {

FileHandle __cdecl OpenFileRaw(const char* path, const char* mode) {
    DWORD access = 0;
    DWORD share = 0;
    DWORD creation = 0;
    DWORD attributes = 0;
    if (std::strchr(mode, 'r')) {
        creation = OPEN_EXISTING;
        attributes = FILE_ATTRIBUTE_READONLY;
        access = GENERIC_READ;
        share = FILE_SHARE_READ;
    }
    if (std::strchr(mode, 'w')) {
        creation = CREATE_ALWAYS;
        access |= GENERIC_WRITE;
        attributes = FILE_ATTRIBUTE_NORMAL;
    }
    // (appending opens it at the start)
    if (std::strchr(mode, 'a')) {
        access |= GENERIC_WRITE;
        creation = OPEN_ALWAYS;
        attributes = FILE_ATTRIBUTE_NORMAL;
    }
    HANDLE file = Import_CreateFileA(path, access, share, nullptr, creation, attributes, nullptr);
    Import_GetLastError();
    return file != INVALID_HANDLE_VALUE ? file : nullptr;
}

uint32_t __cdecl ReadItems(void* buffer, uint32_t size, uint32_t count, FileHandle file) {
    DWORD read = 0;
    Import_ReadFile(file, buffer, size * count, &read, nullptr);
    return read;
}

uint32_t __cdecl WriteItems(const void* buffer, uint32_t size, uint32_t count, FileHandle file) {
    DWORD written = size;
    Import_WriteFile(file, buffer, size * count, &written, nullptr);
    return written / size;
}

int __cdecl CloseFileRaw(FileHandle file) {
    Import_CloseHandle(file);
    return 0;
}

void __cdecl Initialise() {
    // (the root is hardcoded)
    std::memcpy(ms_rootDirName, "D:\\", 4);
    std::strcat(ms_rootDirName, "\\");
}

int __cdecl LoadFile(const char* path, uint8_t* buffer, int, const char* mode) {
    FileHandle file = File_OpenFileRaw(GetFullPath(path), mode);
    if (!file) {
        return -1;
    }
    // in 16 KB reads until one comes back short
    int total = 0;
    DWORD read;
    do {
        read = 0;
        Import_ReadFile(file, buffer + total, 0x4000, &read, nullptr);
        if (int(read) < 0) {
            Import_CloseHandle(file);
            return -1;
        }
        total += int(read);
    } while (read == 0x4000);
    buffer[total] = 0;
    Import_CloseHandle(file);
    return total;
}

int __cdecl GetFileLength(const char* path) {
    FileHandle file = File_OpenFileRaw(GetFullPath(path), modeRead);
    if (!file) {
        return -1;
    }
    Import_SetFilePointer(file, 0, nullptr, FILE_END);
    DWORD length = Import_SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    Import_CloseHandle(file);
    return int(length);
}

FileHandle __cdecl OpenFile(const char* path, const char* mode) {
    return File_OpenFileRaw(GetFullPath(path), mode);
}

FileHandle __cdecl OpenFileForWriting(const char* path) {
    return File_OpenFileRaw(GetFullPath(path), modeWriteBinary);
}

uint32_t __cdecl Read(FileHandle file, void* buffer, uint32_t size) {
    DWORD read = 0;
    Import_ReadFile(file, buffer, size, &read, nullptr);
    return read;
}

uint32_t __cdecl Write(FileHandle file, const void* buffer, uint32_t size) {
    // (the count goes where the size was: unchanged if WriteFile doesn't set it)
    DWORD written = size;
    Import_WriteFile(file, buffer, size, &written, nullptr);
    return written;
}

uint8_t __cdecl Seek(FileHandle file, int32_t offset, uint32_t origin) {
    return Import_SetFilePointer(file, offset, nullptr, origin) != INVALID_SET_FILE_POINTER;
}

uint8_t __cdecl ReadLine(FileHandle file, char* buffer, int size) {
    char* out = buffer;
    bool atEnd = false;
    // a byte at a time (the end of the file doesn't stop the loop)
    for (int i = 0; i < size - 1; i++) {
        char c;
        DWORD read = 0;
        BOOL ok = Import_ReadFile(file, &c, 1, &read, nullptr);
        if (ok && read == 0) {
            atEnd = true;
            continue;
        }
        atEnd = false;
        if (read == 0) {
            continue;
        }
        if (c != '\r' && c != '\n') {
            *out++ = c;
            continue;
        }
        // the end of the line: a second CR/LF goes with it, anything else is put back
        i = size;
        read = 0;
        ok = Import_ReadFile(file, &c, 1, &read, nullptr);
        if (ok && read == 0) {
            atEnd = true;
            continue;
        }
        atEnd = false;
        if (read != 0 && c != '\r' && c != '\n') {
            Import_SetFilePointer(file, -1, nullptr, FILE_CURRENT);
        }
    }
    *out = 0;
    if (atEnd && buffer[0] == 0) {
        return 0;
    }
    return 1;
}

uint32_t __cdecl Tell(FileHandle file) {
    return Import_SetFilePointer(file, 0, nullptr, FILE_CURRENT);
}

int __cdecl CloseFile(FileHandle file) {
    return Import_CloseHandle(file);
}

int __cdecl Printf(FileHandle file, const char* format, ...) {
    char text[0x400];
    va_list args;
    va_start(args, format);
    VsnPrintf(text, 0x3FF, format, args);
    va_end(args);
    DWORD written;
    return Import_WriteFile(file, text, DWORD(std::strlen(text)), &written, nullptr);
}

// ---- streams --------------------------------------------------------------------------

static FileStream* NewStream() {
    // (the game writes through nullptr when out of memory)
    FileStream* stream = static_cast<FileStream*>(Malloc(sizeof(FileStream)));
    if (stream) {
        stream->file = nullptr;
        stream->position = 0;
        stream->mode = 0;
        stream->data = nullptr;
        stream->size = 0;
    }
    return stream;
}

FileStream* __cdecl OpenStream(int kind, int mode, void* arg) {
    const char* fileMode = mode == 1 ? modeRead : modeWrite;
    if (kind == 2) {
        FileHandle file = File_OpenFileRaw(GetFullPath(static_cast<const char*>(arg)), fileMode);
        if (!file) {
            return nullptr;
        }
        FileStream* stream = NewStream();
        stream->file = file;
        stream->mode = mode;
        return stream;
    }
    if (kind == 3) {
        FileStreamBuffer* buffer = static_cast<FileStreamBuffer*>(arg);
        if (!buffer || mode == 3) {
            return nullptr;
        }
        FileStream* stream = NewStream();
        stream->mode = mode;
        stream->data = buffer->data;
        stream->size = buffer->size;
        return stream;
    }
    return nullptr;
}

int __cdecl CloseStream(FileStream* stream, FileStreamBuffer* out) {
    if (out) {
        out->data = stream->data;
        out->size = stream->size;
    }
    if (stream) {
        if (stream->file) {
            Import_CloseHandle(stream->file);
        }
        Free(stream);
    }
    return 1;
}

// ---- memory files -----------------------------------------------------------------------

uint32_t __cdecl ReadFromMemory(MemoryFile* stream, void* buffer, uint32_t size) {
    uint32_t left = stream->size - stream->position;
    if (size > left) {
        size = left;
    }
    if (size != 0) {
        std::memcpy(buffer, stream->data + stream->position, size);
        if (stream->position + size <= stream->size) {
            stream->position += size;
        }
    }
    return size;
}

// (the game's bounds check on every move: position never passes the size)
static bool MemoryGet(MemoryFile* stream, char* c) {
    uint32_t position = stream->position;
    if (stream->size == position) {
        return false;
    }
    *c = char(stream->data[position]);
    if (position + 1 <= stream->size) {
        stream->position = position + 1;
    }
    return true;
}

uint8_t __cdecl ReadLineFromMemory(MemoryFile* stream, char* buffer, int size) {
    char* out = buffer;
    bool atEnd = false;
    for (int i = 0; i < size - 1; i++) {
        char c;
        if (!MemoryGet(stream, &c)) {
            atEnd = true;
            continue;
        }
        atEnd = false;
        if (c != '\r' && c != '\n') {
            *out++ = c;
            continue;
        }
        i = size;
        if (!MemoryGet(stream, &c)) {
            atEnd = true;
            continue;
        }
        atEnd = false;
        if (c != '\r' && c != '\n' && stream->position - 1 <= stream->size) {
            stream->position--;
        }
    }
    *out = 0;
    if (atEnd && buffer[0] == 0) {
        return 0;
    }
    return 1;
}

} // namespace CFileMgr

// Untested: the game's code runs until each is checked.
#define FM_REVERSED(fn, address)            \
    static void* Original_##fn;             \
    REVERSED("CFileMgr::" #fn, address, RE_BODY(&CFileMgr::fn), nullptr, &Original_##fn, re::Mode::Original)

FM_REVERSED(OpenFileRaw, 0x0042D040);
FM_REVERSED(ReadItems, 0x0042D0E0);
FM_REVERSED(WriteItems, 0x0042D110);
FM_REVERSED(CloseFileRaw, 0x0042D140);
FM_REVERSED(Initialise, 0x0042D150);
FM_REVERSED(LoadFile, 0x0042D180);
FM_REVERSED(GetFileLength, 0x0042D210);
FM_REVERSED(OpenFile, 0x0042D260);
FM_REVERSED(OpenFileForWriting, 0x0042D280);
FM_REVERSED(Read, 0x0042D2A0);
FM_REVERSED(Write, 0x0042D2D0);
FM_REVERSED(Seek, 0x0042D300);
FM_REVERSED(ReadLine, 0x0042D330);
FM_REVERSED(Tell, 0x0042D430);
FM_REVERSED(CloseFile, 0x0042D450);
FM_REVERSED(Printf, 0x0042D460);
FM_REVERSED(OpenStream, 0x0042D4C0);
FM_REVERSED(CloseStream, 0x0042D570);
FM_REVERSED(ReadFromMemory, 0x0042D5B0);
FM_REVERSED(ReadLineFromMemory, 0x0042D600);

// ---- CGame's file setup ---------------------------------------------------------------

namespace {
GAME_FN(void, __cdecl, File_Initialise, 0x0042D150);
GAME_FN(void, __cdecl, CdStreamInit, 0x0073ABD0, int channels);
GAME_FN(FileHandle, __cdecl, File_OpenFile, 0x0042D260, const char* path, const char* mode);
GAME_FN(uint8_t, __cdecl, File_ReadLine, 0x0042D330, FileHandle file, char* buffer, int size);
GAME_FN(int, __cdecl, File_CloseFile, 0x0042D450, FileHandle file);
inline int __cdecl AtoI(const char* s) { return (int)crt::Atol(s); }   // the CRT's atoi
GAME_FN(uint8_t, __cdecl, InitPropHXDs, 0x0052D9D0, int count);   // (CStreaming's: an array of `count`)
} // namespace

namespace CGame {

uint8_t __cdecl InitialiseOnceBeforeRW() {
    File_Initialise();
    CdStreamInit(5);
    return 1;
}

void __cdecl LoadPropHXDsCount() {
    char line[0x80];
    FileHandle file = File_OpenFile("Config\\Dat\\PropHXDs.dat", "r");
    File_ReadLine(file, line, sizeof(line));
    InitPropHXDs(AtoI(line) + 18);
    File_CloseFile(file);
}

} // namespace CGame

static void* Original_InitialiseOnceBeforeRW;
REVERSED("CGame::InitialiseOnceBeforeRW", 0x0042E450, RE_BODY(&CGame::InitialiseOnceBeforeRW), nullptr,
         &Original_InitialiseOnceBeforeRW, re::Mode::Original);
static void* Original_LoadPropHXDsCount;
REVERSED("CGame::LoadPropHXDsCount", 0x0042E470, RE_BODY(&CGame::LoadPropHXDsCount), nullptr,
         &Original_LoadPropHXDsCount, re::Mode::Original);
