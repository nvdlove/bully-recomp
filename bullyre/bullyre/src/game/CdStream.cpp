#include "game/CdStream.h"

#include <windows.h>

#include <cstring>

#if defined(_WIN64)
#include "x64/lowheap.h"
#endif

namespace {

// File I/O through the game's imports (the mod loader hooks them), read at call time.
using ReadFile_t = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
using CreateFileA_t = HANDLE(WINAPI*)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
using CloseHandle_t = BOOL(WINAPI*)(HANDLE);
using GetOverlappedResult_t = BOOL(WINAPI*)(HANDLE, LPOVERLAPPED, LPDWORD, BOOL);
GAME_VAR(ReadFile_t, Import_ReadFile, 0x008FC06C);
GAME_VAR(CreateFileA_t, Import_CreateFileA, 0x008FC064);
GAME_VAR(CloseHandle_t, Import_CloseHandle, 0x008FC050);
GAME_VAR(GetOverlappedResult_t, Import_GetOverlappedResult, 0x008FC090);

GAME_FN(int, __cdecl, CdStreamSync, 0x0073A690, int channel);   // (patched by the launcher: stays the game's)
GAME_FN(unsigned long, __stdcall, CdStreamThread, 0x0073A720, void* param);
GAME_FN(int, __cdecl, CdStreamAddImage, 0x0073A890, const char* path);
GAME_FN(int, __cdecl, CdStreamReadFromFile, 0x0073AAA0, void* file, int channel, void* buffer, uint32_t sector, uint32_t sectors);
GAME_FN(void, __cdecl, CdStreamRemoveImages, 0x0073A930);
GAME_FN(void, __cdecl, CdStreamInitThread, 0x0073A9B0);
GAME_FN(const char*, __cdecl, FixPath, 0x00829900, const char* path);   // the path as the game opens it
GAME_FN(void*, __cdecl, MallocAligned, 0x005EE9C0, size_t size, size_t alignment);
GAME_FN(void, __cdecl, FreeAligned, 0x005EEA70, void* p);

// LocalAlloc(LPTR) as the game does; in the 64-bit build the low heap (the game's globals
// hold these in 4-byte fields)
void* ZeroedAlloc(size_t size) {
#if defined(_WIN64)
    void* p = re::low::Alloc(size);
    if (p) {
        std::memset(p, 0, size);
    }
    return p;
#else
    return LocalAlloc(LPTR, size);
#endif
}
void ZeroedFree(void* p) {
#if defined(_WIN64)
    re::low::Free(p);
#else
    LocalFree(p);
#endif
}

} // namespace

namespace CdStream {
SRWLOCK g_syncLock = SRWLOCK_INIT;
void LockSync() {
    AcquireSRWLockExclusive(&g_syncLock);
}
void UnlockSync() {
    ReleaseSRWLockExclusive(&g_syncLock);
}

// the channel's OVERLAPPED: its 0x14 bytes in the game's layout; on x64 (where one is 0x20) one
// of ours per channel (the game's code only hands it to the file API)
_OVERLAPPED* Overlapped(CdStreamInfo& ch) {
#if defined(_WIN64)
    static OVERLAPPED ours[64];
    ptrdiff_t index = &ch - static_cast<CdStreamInfo*>(ms_channels);
    if (index >= 0 && index < 64) {
        return &ours[index];
    }
#endif
    return reinterpret_cast<OVERLAPPED*>(ch.overlapped);
}
} // namespace CdStream
using CdStream::Overlapped;

namespace CdStream {

void __cdecl Init(int numChannels) {
    ms_bOverlapped = 1;
    ms_bThreaded = 0;
    ms_openFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING;
    void* sector = MallocAligned(0x800, 0x200);
    SetLastError(0);
    ms_numImages = 0;
    ms_numChannels = numChannels;
    ms_channels = static_cast<CdStreamInfo*>(ZeroedAlloc(numChannels * sizeof(CdStreamInfo)));
    CdStreamAddImage("Stream\\world.img");
    // a test read without the thread: overlapped reads work, else they're turned off
    int read = CdStreamReadFromFile(ms_imageFiles[0], 0, sector, 0, 1);
    CdStreamRemoveImages();
    ms_bThreaded = 1;
    if (read != 1) {
        ms_openFlags &= ~uint32_t(FILE_FLAG_OVERLAPPED);
        ms_bOverlapped = 0;
    }
    CdStreamInitThread();
    FreeAligned(sector);
}

void __cdecl InitThread() {
    SetLastError(0);
    for (int i = 0; i < ms_numChannels; i++) {
        ms_channels[i].semaphore = CreateSemaphoreA(nullptr, 0, 2, nullptr);
        if (ms_channels[i].semaphore == nullptr) {
            return;
        }
    }
    ms_queueItems = static_cast<int32_t*>(ZeroedAlloc(ms_numChannels * 4 + 4));
    ms_queueHead = 0;
    ms_queueTail = 0;
    ms_queueSize = ms_numChannels + 1;
    // (the game names it "CdStream": a named one is the same for every copy of the game running,
    // whose releases then wake each other's threads and leave their own reads waiting for ever.
    // Nothing outside opens it: unnamed)
    ms_semaphore = CreateSemaphoreA(nullptr, 0, 5, nullptr);
    if (ms_semaphore == nullptr) {
        return;
    }
    ms_mutex = CreateMutexA(nullptr, FALSE, nullptr);
    if (ms_mutex == nullptr) {
        return;
    }
    // (the game's thread function, so its mode applies)
    ms_thread = CreateThread(nullptr, 0x10000, reinterpret_cast<LPTHREAD_START_ROUTINE>(CdStreamThread), nullptr, CREATE_SUSPENDED,
                             reinterpret_cast<DWORD*>(&ms_threadId));
    if (ms_thread == nullptr) {
        return;
    }
    SetThreadPriority(ms_thread, THREAD_PRIORITY_NORMAL);
    ResumeThread(ms_thread);
}

void __cdecl Shutdown() {
    if (ms_bThreaded) {
        ZeroedFree(ms_queueItems);
        Import_CloseHandle(ms_semaphore);
        Import_CloseHandle(ms_thread);
        for (int i = 0; i < ms_numChannels; i++) {
            Import_CloseHandle(ms_channels[i].semaphore);
        }
        // (the mutex stays open)
    }
    ZeroedFree(ms_channels);
}

int __cdecl GetStatus(int channel) {
    CdStreamInfo& ch = ms_channels[channel];
    if (!ms_bThreaded) {
        return CDSTREAM_STATUS_DONE;
    }
    if (ch.inUse) {
        return CDSTREAM_STATUS_BUSY;
    }
    if (ch.sectors) {
        return CDSTREAM_STATUS_RETRYING;
    }
    int status = ch.status;
    if (status != 0) {
        ch.status = 0;  // (reported once)
    }
    return status;
}

uint32_t __cdecl GetPosition() {
    return ms_lastPosn;
}

int __cdecl ReadFromFile(void* file, int channel, void* buffer, uint32_t sector, uint32_t sectors) {
    CdStreamInfo& ch = ms_channels[channel];
    ms_lastPosn = sector + sectors;
    ch.file = file;
    SetLastError(0);
    if (ms_bThreaded) {
        if (ch.sectors != 0 || ch.inUse != 0) {
            return 0;
        }
        ch.status = 0;
        ch.sector = sector;
        ch.sectors = sectors;
        ch.buffer = buffer;
        ch.syncWaiting = 0;
        ms_queueItems[ms_queueTail] = channel;
        ms_queueTail = (ms_queueTail + 1) % ms_queueSize;
        ReleaseSemaphore(ms_semaphore, 1, nullptr);
        return 1;
    }
    Overlapped(ch)->Offset = sector << 11;
    if (!Import_ReadFile(file, buffer, sectors << 11, nullptr, Overlapped(ch)) && GetLastError() != ERROR_IO_PENDING) {
        return 0;
    }
    return 1;
}

int __cdecl Read(int channel, void* buffer, uint32_t sector, uint32_t sectors) {
    return CdStreamReadFromFile(ms_imageFiles[0], channel, buffer, sector, sectors);
}

unsigned long __stdcall Thread(void*) {
    for (;;) {
        HANDLE handles[2] = {ms_semaphore, ms_mutex};
        WaitForMultipleObjects(2, handles, TRUE, INFINITE);
        if (ms_queueHead != ms_queueTail && ms_queueItems[ms_queueHead] >= 0) {
            CdStreamInfo& ch = ms_channels[ms_queueItems[ms_queueHead]];
            ch.inUse = 1;
            // (without overlapped reads, nothing is read: the game's as it is)
            if (ch.status == 0 && ms_bOverlapped) {
                Overlapped(ch)->Offset = ch.sector << 11;
                if (Import_ReadFile(ch.file, ch.buffer, ch.sectors << 11, nullptr, Overlapped(ch)) == TRUE) {
                    ch.status = 0;
                } else {
                    DWORD error = GetLastError();
                    if (error == ERROR_IO_PENDING) {
                        DWORD bytes;
                        if (Import_GetOverlappedResult(ch.file, Overlapped(ch), &bytes, TRUE)) {
                            ch.status = 0;
                        } else {
                            ch.status = GetLastError() != ERROR_HANDLE_EOF ? CDSTREAM_STATUS_ERROR : 0;
                        }
                    } else if (error == ERROR_NOT_ENOUGH_MEMORY || error == ERROR_DISK_CORRUPT || error == ERROR_INVALID_USER_BUFFER) {
                        ch.status = CDSTREAM_STATUS_ERROR;   // (8, 0x571, 0x6F8)
                    } else if (error == ERROR_HANDLE_EOF) {
                        ch.status = 0;
                    }
                    // (other errors leave the status as it was)
                }
            }
            if (ms_queueHead != ms_queueTail) {
                ms_queueHead = (ms_queueHead + 1) % ms_queueSize;
            }
            LockSync();
            ch.sectors = 0;
            if (ch.syncWaiting) {
                ReleaseSemaphore(ch.semaphore, 1, nullptr);
            }
            UnlockSync();
            ch.inUse = 0;
        }
        ReleaseMutex(ms_mutex);
    }
}

int __cdecl AddImage(const char* path) {
    const char* fullPath = FixPath(path);
    SetLastError(0);
    HANDLE file = Import_CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                     ms_openFlags | FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_READONLY, nullptr);
    int image = ms_numImages;
    ms_imageFiles[image] = file;
    // (a failed open is INVALID_HANDLE_VALUE, which passes: the game checks for 0)
    if (file == nullptr) {
        return 0;
    }
    strcpy(ms_imageNames[image], path);
    ms_numImages = image + 1;
    return 1;
}

char* __cdecl GetImageName(int image) {
    return ms_imageFiles[image] != nullptr ? ms_imageNames[image] : nullptr;
}

void __cdecl RemoveImages() {
    for (int i = 0; i < ms_numChannels; i++) {
        CdStreamSync(i);
    }
    for (int i = 0; i < ms_numImages; i++) {
        SetLastError(0);
        Import_CloseHandle(ms_imageFiles[i]);
        ms_imageFiles[i] = nullptr;
    }
    ms_numImages = 0;
}

int __cdecl GetNumImages() {
    return ms_numImages;
}

// ---- registration -----------------------------------------------------------------

#define CDSTREAM_REVERSED(fn, name, address)                                     \
    static decltype(&fn) Original_##fn;                                                         \
    REVERSED("CdStream::" name, address, RE_BODY(&fn), nullptr,        \
             reinterpret_cast<void**>(&Original_##fn), re::Mode::Original)

CDSTREAM_REVERSED(Init, "Init", 0x0073ABD0);
CDSTREAM_REVERSED(InitThread, "InitThread", 0x0073A9B0);
CDSTREAM_REVERSED(Shutdown, "Shutdown", 0x0073A5C0);
CDSTREAM_REVERSED(GetStatus, "GetStatus", 0x0073A630);
CDSTREAM_REVERSED(GetPosition, "GetPosition", 0x0073A680);
CDSTREAM_REVERSED(ReadFromFile, "ReadFromFile", 0x0073AAA0);
CDSTREAM_REVERSED(Read, "Read", 0x0073ABA0);
CDSTREAM_REVERSED(Thread, "Thread", 0x0073A720);
CDSTREAM_REVERSED(AddImage, "AddImage", 0x0073A890);
CDSTREAM_REVERSED(GetImageName, "GetImageName", 0x0073A910);
CDSTREAM_REVERSED(RemoveImages, "RemoveImages", 0x0073A930);
CDSTREAM_REVERSED(GetNumImages, "GetNumImages", 0x0073A9A0);

} // namespace CdStream
