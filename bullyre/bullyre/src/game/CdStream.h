#pragma once

// CdStream: the archive reader. Reads of whole 2048-byte sectors from the archive
// images (Stream\World.img), one channel per reader (CStreaming uses two), done by a
// thread with overlapped I/O. (GTA's CdStream, near enough.)

#include <cstddef>
#include <cstdint>

#include "reversed.h"

// A channel: one read at a time.
struct CdStreamInfo {
    uint32_t sector;        // +0x00
    uint32_t sectors;       // +0x04 non-zero: a read queued or being done
    re::Ptr32<void> buffer;           // +0x08
    uint32_t unknown0C;     // +0x0C
    int32_t status;         // +0x10 0 fine, 0xFE an error (read once by GetStatus)
    re::Ptr32<void> semaphore;        // +0x14 released when a read Sync waits for is done
    int32_t inUse;          // +0x18 the thread is on it
    int32_t syncWaiting;    // +0x1C Sync waits on the semaphore
    uint8_t overlapped[0x14];  // +0x20 OVERLAPPED (+0x28: its offset)
    re::Ptr32<void> file;             // +0x34 the image being read
};
static_assert(sizeof(CdStreamInfo) == 0x38);
static_assert(offsetof(CdStreamInfo, overlapped) == 0x20);
static_assert(offsetof(CdStreamInfo, file) == 0x34);

struct _OVERLAPPED;   // (windows.h's OVERLAPPED)
namespace CdStream {
// a channel's OVERLAPPED (CdStream.cpp: the game's 0x14 bytes, or ours on x64)
_OVERLAPPED* Overlapped(CdStreamInfo& ch);
// Sync's test of a channel's sectors and setting of syncWaiting, and the thread's clearing of
// the sectors and test of syncWaiting, made one step each (ours): between the original's two
// steps the thread could finish without releasing the semaphore Sync then waits on for ever
// (the PC launcher patches the game's Sync there, 0x73A6B6)
void LockSync();
void UnlockSync();
}

enum : int {
    CDSTREAM_STATUS_DONE = 0,
    CDSTREAM_STATUS_RETRYING = 0xFA,   // (still queued)
    CDSTREAM_STATUS_ERROR = 0xFE,
    CDSTREAM_STATUS_BUSY = 0xFF,
};

namespace CdStream {

GAME_VAR(CdStreamInfo*, ms_channels, 0x020DCD94);   // LocalAlloc'd, ms_numChannels
GAME_VAR(int32_t, ms_numChannels, 0x020DCDB4);
GAME_VAR(uint32_t, ms_lastPosn, 0x020DCD70);         // sector after the last read asked for
GAME_VAR(uint32_t, ms_openFlags, 0x020DCD98);        // CreateFileA's flags (overlapped, no buffering)
GAME_VAR(uint8_t, ms_bThreaded, 0x020DCD9C);         // reads go through the thread
GAME_VAR(uint8_t, ms_bOverlapped, 0x020DCD9D);       // overlapped reads work
// the thread's queue: channel numbers, a ring of ms_queueSize
GAME_VAR(int32_t*, ms_queueItems, 0x020DCD84);
GAME_VAR(int32_t, ms_queueHead, 0x020DCD88);
GAME_VAR(int32_t, ms_queueTail, 0x020DCD8C);
GAME_VAR(int32_t, ms_queueSize, 0x020DCD90);
GAME_VAR(void*, ms_semaphore, 0x020DCDB0);           // a read queued
GAME_VAR(void*, ms_mutex, 0x020DCDAC);
GAME_VAR(void*, ms_thread, 0x020DCDA8);
GAME_VAR(uint32_t, ms_threadId, 0x020DCDA4);
// the images (the array is followed by ms_threadId: the game has room for one)
GAME_ARRAY(void*, ms_imageFiles, 0x020DCDA0);
GAME_VAR(int32_t, ms_numImages, 0x020DCDB8);
inline char (*const ms_imageNames)[0x40] = reinterpret_cast<char (*)[0x40]>(0x020DCDC0);

// 0x73ABD0
void __cdecl Init(int numChannels);
// 0x73A9B0: the semaphores, the queue, the thread
void __cdecl InitThread();
// 0x73A5C0
void __cdecl Shutdown();
// 0x73A630: 0 done, 0xFF busy, 0xFA queued, else the error (once)
int __cdecl GetStatus(int channel);
// 0x73A680
uint32_t __cdecl GetPosition();
// 0x73AAA0: queues a read (threaded) or starts it; 0 if the channel's busy or it failed
int __cdecl ReadFromFile(void* file, int channel, void* buffer, uint32_t sector, uint32_t sectors);
// 0x73ABA0: from the first image
int __cdecl Read(int channel, void* buffer, uint32_t sector, uint32_t sectors);
// 0x73A720: the reader thread
unsigned long __stdcall Thread(void* param);
// 0x73A890
int __cdecl AddImage(const char* path);
// 0x73A910: an image's path, nullptr if it isn't open
char* __cdecl GetImageName(int image);
// 0x73A930: waits for the channels and closes the images
void __cdecl RemoveImages();
// 0x73A9A0
int __cdecl GetNumImages();

} // namespace CdStream
