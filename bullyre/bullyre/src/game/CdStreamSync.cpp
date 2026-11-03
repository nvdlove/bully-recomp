// CdStreamSync (0x73A690): wait for a channel's read. The launcher patches the game's
// code at 0x73A6B6, so this is written here and not hooked (CdStream.cpp calls the game's).

#include <cstdint>

#include "game/CdStream.h"
#include "game/VCall.h"

namespace {

constexpr uintptr_t kWaitForSingleObject = 0x008FC048;
constexpr uintptr_t kGetOverlappedResult = 0x008FC090;

} // namespace

namespace cdstreamsync {

// the channel's status: with the thread, after waiting for its read; with overlapped
// reads, whether the read went through (0) or not (0xFE)
int __cdecl Sync(int channel) {
    CdStreamInfo& ch = CdStream::ms_channels[channel];
    if (CdStream::ms_bThreaded) {
        CdStream::LockSync();   // (with the thread's end of a read: CdStream.h)
        if (ch.sectors != 0) {
            ch.syncWaiting = 1;
            CdStream::UnlockSync();
            vc::Import<uint32_t>(kWaitForSingleObject, ch.semaphore, 0xFFFFFFFFu);
        } else {
            CdStream::UnlockSync();
        }
        ch.inUse = 0;
        return ch.status;
    }
    if (CdStream::ms_bOverlapped && ch.file != nullptr) {
        uint32_t bytes;
        int ok = vc::Import<int>(kGetOverlappedResult, ch.file, static_cast<void*>(CdStream::Overlapped(CdStream::ms_channels[channel])),
                                 &bytes, 1);
        return ok == 1 ? CDSTREAM_STATUS_DONE : CDSTREAM_STATUS_ERROR;
    }
    return CDSTREAM_STATUS_DONE;
}

} // namespace cdstreamsync

// (patched in place by the launcher at 0x73A6B6: not hooked in the 32-bit game; the 64-bit
// dispatch's body)
RE_UNHOOKED_FN("CdStreamSync", 0x0073A690, &cdstreamsync::Sync);
