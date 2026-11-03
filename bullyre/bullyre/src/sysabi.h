#pragma once

// System structures the game keeps in its 32-bit layout (4-byte pointers) but hands to the
// system: on x64 a native copy is made at the call (the 32-bit build passes the game's own).

#include <cstdint>
#include <vector>

namespace re {

// A DirectInput data format (DIDATAFORMAT, 0x18 bytes, and its DIOBJECTDATAFORMATs, 0x10
// each, in the game's layout: c_dfDIKeyboard, c_dfDIMouse and the joystick formats the game
// builds) as IDirectInputDevice8::SetDataFormat takes it. DirectInput copies it in the call:
// the temporary may go after. Use: Com<long>(device, 11, re::DIDataFormat(format).get()).
class DIDataFormat {
public:
    explicit DIDataFormat(const void* game) {
#if defined(_M_X64)
        const uint32_t* f = static_cast<const uint32_t*>(game);
        const uint32_t* o = reinterpret_cast<const uint32_t*>(uintptr_t(f[5]));
        objects_.resize(f[4]);
        for (uint32_t i = 0; i < f[4]; i++) {
            objects_[i] = {reinterpret_cast<const void*>(uintptr_t(o[i * 4])), o[i * 4 + 1], o[i * 4 + 2], o[i * 4 + 3]};
        }
        format_ = {uint32_t(sizeof(Format)), uint32_t(sizeof(Object)), f[2], f[3], f[4], objects_.data()};
#else
        game_ = game;
#endif
    }
    const void* get() const {
#if defined(_M_X64)
        return &format_;
#else
        return game_;
#endif
    }

private:
#if defined(_M_X64)
    struct Object {
        const void* guid;
        uint32_t offset, type, flags;
    };
    struct Format {
        uint32_t size, objectSize, flags, dataSize, count;
        const Object* objects;
    };
    static_assert(sizeof(Object) == 0x18 && sizeof(Format) == 0x20);
    std::vector<Object> objects_;
    Format format_{};
#else
    const void* game_ = nullptr;
#endif
};

// A game OVERLAPPED (0x14 bytes in the game's layouts: Internal, InternalHigh, Offset,
// OffsetHigh, hEvent) for the file API (ReadFile, GetOverlappedResult). On x64, where one is 0x20
// bytes, a native one per game address, kept for the process (the system writes it while a read
// is pending): SystemOverlapped gives it with the game's Offset, OffsetHigh and hEvent copied in;
// SyncOverlapped copies its Internal (the status) and InternalHigh back, for the game's code that
// reads them. On Win32 the game's own (and nothing to copy).
void* SystemOverlapped(void* game);
void SyncOverlapped(void* game);

// A game object that implements a system COM interface (the effect state manager D3DX calls
// back through), as the system takes it. On x64 the game's vtable has 4-byte slots where the
// system reads 8: a native object stands in, its vtable's slot i putting the game object in
// rcx and jumping to the game's method i (its game address, which reaches our body); the
// other arguments are left as they came. QueryInterface hands out the native object (AddRef'd
// through the game's slot 1). One native object per game object, kept for the process. The
// 32-bit build passes the game's own.
void* NativeInterface(void* game, int slots);

} // namespace re
