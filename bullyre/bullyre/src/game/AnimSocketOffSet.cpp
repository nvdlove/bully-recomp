// AM_SocketOffSet (vtable 0x941F5C): a socket with its own offset (+0xC a translation,
// +0x18 a rotation stored w first), set from Euler angles in degrees, read back as
// degrees in 0..360, and loaded from an HXD stream. 0x6B4150..0x6B43C4, 0x6B5700.

#include <cstdint>

#include "game/FileLoader.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct CVector {
    float x, y, z;
};
struct CQuaternion {
    float x, y, z, w;
};

constexpr float kPi = 3.14159274f;
constexpr float kTwoPi = 6.28318548f;
constexpr float kOne180th = 0.0055555556900799274f;

GAME_FN(void, __thiscall, Quat_SetAngles, 0x00413ED0, CQuaternion* q, const CVector* angles);
GAME_FN(void, __thiscall, Quat_GetAngles, 0x004140C0, const CQuaternion* q, CVector* angles);
GAME_FN(uint32_t, __cdecl, File_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);
GAME_FN(uint8_t, __cdecl, File_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(int, __cdecl, DataStream_Read, 0x0042E730, FileStream* stream, void* to, int size);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
inline void* __cdecl Memcpy(void* dst, const void* src, size_t n) { return crt::Memcpy(dst, src, n); }

// the inlined stream read (nothing without a stream or its memory)
void Read(FileStream* s, void* to, uint32_t size) {
    if (s == nullptr) {
        return;
    }
    if (s->file) {
        File_ReadItems(to, size, 1, s->file);
    } else if (s->data) {
        if (size == 0x20) {
            Memcpy(to, s->data + s->position, size);
        } else {
            for (uint32_t i = 0; i < size; i += 4) {
                *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(to) + i) =
                    *reinterpret_cast<uint32_t*>(s->data + s->position + i);
            }
        }
        s->position += size;
    }
}

// 0x6B4150: the rotation as Euler angles in degrees (each in 0..360)
void __fastcall ToEulerDegrees(uint8_t* self, re::Edx, float* x, float* y, float* z) {
    CQuaternion q{At<float>(self, 0x1C), At<float>(self, 0x20), At<float>(self, 0x24), At<float>(self, 0x18)};
    CVector angles;
    Quat_GetAngles(&q, &angles);
    *x = angles.x;
    *y = angles.y;
    *z = angles.z;
    if (*x < 0.0f) {
        *x = *x + kTwoPi;
    }
    if (*y < 0.0f) {
        *y = *y + kTwoPi;
    }
    if (*z < 0.0f) {
        *z = kTwoPi + *z;
    }
    *x = *x * 180.0f / kPi;
    *y = *y * 180.0f / kPi;
    *z = 180.0f * *z / kPi;
}

// 0x6B5700: sets the rotation from Euler angles in degrees
void __fastcall SetEulerDegrees(uint8_t* self, re::Edx, float x, float y, float z) {
    CVector radians{x * kPi * kOne180th, y * kPi * kOne180th, kOne180th * (z * kPi)};
    CQuaternion q;
    Quat_SetAngles(&q, &radians);
    At<float>(self, 0x1C) = q.x;
    At<float>(self, 0x20) = q.y;
    At<float>(self, 0x24) = q.z;
    At<float>(self, 0x18) = q.w;
}

// 0x6B4200: loads it (the translation; the rotation raw from version 1.02, as degrees
// before; from 1.0 on 0x20 bytes skipped; +8; then the name, hashed into +4)
void __fastcall Load(uint8_t* self, re::Edx, FileStream* stream, float version) {
    Read(stream, self + 0xC, 0xC);
    if (version >= 1.02f) {
        DataStream_Read(stream, self + 0x18, 0x10);
    } else {
        // (without a stream the game passes uninitialised locals, the last one the
        // stream pointer's bits)
        float angles[3] = {0.0f, 0.0f, 0.0f};
        Read(stream, &angles[0], 4);
        Read(stream, &angles[1], 4);
        Read(stream, &angles[2], 4);
        using SetAngles = void(__thiscall*)(uint8_t*, float, float, float);
        re::PtrCast<SetAngles>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self))[2])(self, angles[0], angles[1], angles[2]);
    }
    if (1.0f < version && stream) {
        if (stream->file) {
            File_Seek(stream->file, 0x20, 1);
        } else if (stream->data) {
            stream->position += 0x20;
        }
    }
    DataStream_Read(stream, self + 8, 4);
    char name[0x21] = {};  // (the game's 0x20-byte buffer runs on into other locals)
    Read(stream, name, 0x20);
    At<uint32_t>(self, 4) = HashName(name);
}

} // namespace

#define AMSOCKOFS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMSOCKOFS_REVERSED("AM_SocketOffSet::ToEulerDegrees", 0x006B4150, &ToEulerDegrees);
AMSOCKOFS_REVERSED("AM_SocketOffSet::Load", 0x006B4200, &Load);
AMSOCKOFS_REVERSED("AM_SocketOffSet::SetEulerDegrees", 0x006B5700, &SetEulerDegrees);
