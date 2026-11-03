// Loading the 0xB0-byte hierarchy file (reset / freed in AnimHxdFile.cpp: AM_HierarchyFile):
// version 1.06 only, a 0x50-byte header, its sockets (AM_SocketOffSet, 0x28 each) from
// 0x60-byte records, and (1.04 on) a vector array and (1.05 on) a float.
// 0x6B5780..0x6B5A7D.

#include <cstdint>
#include <cstring>

#include "game/FileLoader.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __cdecl, NiMalloc, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint32_t, __cdecl, File_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);
GAME_FN(int, __cdecl, CloseStream_, 0x0042D570, FileStream* stream, FileStreamBuffer* out);
GAME_FN(int, __cdecl, DataStream_Read, 0x0042E730, FileStream* stream, void* to, int size);
GAME_FN(void*, __cdecl, Stream_ReadExactly, 0x006B2BF0, FileStream* stream, void* to, int size);
inline void* __cdecl Memcpy(void* dst, const void* src, size_t n) { return crt::Memcpy(dst, src, n); }
GAME_FN(void, __cdecl, ReadSockets, 0x006B5780, uint32_t count, uint8_t* sockets, FileStream* stream, float version);
inline void* const kSocketOffSetCtor = reinterpret_cast<void*>(0x006B2D00);
inline void* const kVectorCtor = reinterpret_cast<void*>(0x00508410);

inline uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
}

inline void Read4(FileStream* s, void* to) {
    if (s->file) {
        File_ReadItems(to, 4, 1, s->file);
    } else if (s->data) {
        *static_cast<uint32_t*>(to) = *reinterpret_cast<uint32_t*>(s->data + s->position);
        s->position += 4;
    }
}

// 0x6B5780: the sockets from `count` 0x60-byte records (+0x0 the 7-dword transform,
// +0x3C the bone index, +0x40 the name) read into a buffer of 32 (more overrun it, as in
// the game). (The game's loop pointer starts at record + 0x3C.)
void __cdecl ReadSocketsImpl(uint32_t count, uint8_t* sockets, FileStream* stream) {
    uint8_t buffer[0xC00];
    for (int i = 0; i < 32; i++) {
        At<float>(buffer + i * 0x60, 0) = 0.0f;
        At<float>(buffer + i * 0x60, 4) = 0.0f;
        At<float>(buffer + i * 0x60, 8) = 0.0f;
    }
    uint32_t size = count * 0x60;
    if (stream) {
        if (stream->file) {
            File_ReadItems(buffer, size, 1, stream->file);
        } else if (stream->data) {
            Memcpy(buffer, stream->data + stream->position, size);
            stream->position += size;
        }
    }
    uint8_t* record = buffer;
    uint8_t* socket = sockets;
    for (int n = int(count); n > 0; n--, record += 0x60, socket += 0x28) {
        for (int off = 0xC; off <= 0x24; off += 4) {
            At<uint32_t>(socket, off) = At<uint32_t>(record, off - 0xC);
        }
        At<uint32_t>(socket, 8) = At<uint32_t>(record, 0x3C);
        At<uint32_t>(socket, 4) = HashName(reinterpret_cast<const char*>(record + 0x40));
    }
}

// 0x6B5870: loads it from `stream` (0x1D: none, 10: not version 1.06); the header's name
// is copied to `nameOut` when given
int __fastcall Load(uint8_t* self, re::Edx, const char* name, FileStream* stream, char* nameOut) {
    if (stream == nullptr) {
        return 0x1D;
    }
    float version = 0.0f;
    if (stream->file == nullptr && stream->data == nullptr) {
        CloseStream_(stream, nullptr);
        return 10;
    }
    Read4(stream, &version);
    if (!(version == 1.06f)) {
        CloseStream_(stream, nullptr);
        return 10;
    }
    uint32_t header[20];
    DataStream_Read(stream, header, 0x50);
    At<uint32_t>(self, 0x18) = header[18];
    At<uint32_t>(self, 0x14) = header[0];
    At<uint32_t>(self, 0xC) = header[1];
    At<uint32_t>(self, 0x9C) = header[19];
    if (nameOut) {
        std::strcpy(nameOut, reinterpret_cast<const char*>(&header[2]));
    }
    std::strcpy(reinterpret_cast<char*>(self + 0x1C), name);
    uint32_t count = At<uint32_t>(self, 0x9C);
    if (count) {
        uint8_t* sockets = static_cast<uint8_t*>(Malloc(ArrayBytes(count, 0x28)));
        if (sockets) {
            VectorConstruct(sockets, 0x28, count, kSocketOffSetCtor);
        }
        At<uint8_t*>(self, 0xA0) = sockets;
        ReadSockets(At<uint32_t>(self, 0x9C), sockets, stream, version);
    }
    if (version >= 1.04f) {
        Read4(stream, self + 0xA4);
        count = At<uint32_t>(self, 0xA4);
        if (count) {
            uint8_t* vectors = static_cast<uint8_t*>(NiMalloc(ArrayBytes(count, 0xC)));
            if (vectors) {
                VectorConstruct(vectors, 0xC, count, kVectorCtor);
            }
            At<uint8_t*>(self, 0xA8) = vectors;
            DataStream_Read(stream, vectors, int(At<uint32_t>(self, 0xA4) * 0xC));
            if (version >= 1.05f) {
                Stream_ReadExactly(stream, self + 0xAC, 4);
            }
        }
    }
    return 0;
}

} // namespace

// 0x508410 (thiscall): CVector's constructor as the vector-constructor iterators call it (fldz,
// three stores: zeroed), the object back. Not in Ghidra's function list; the 64-bit dispatch's
// body (the 32-bit game runs its own)
static float* __fastcall VectorConstructZero(float* self, re::Edx) {
    self[0] = 0.0f;
    self[1] = 0.0f;
    self[2] = 0.0f;
    return self;
}
RE_UNHOOKED_FN("CVector::CVector (zeroed)", 0x00508410, &VectorConstructZero);

#define AMHIERLOAD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMHIERLOAD_REVERSED("AM_HierarchyFile::ReadSockets", 0x006B5780, &ReadSocketsImpl);
AMHIERLOAD_REVERSED("AM_HierarchyFile::Load", 0x006B5870, &Load);
