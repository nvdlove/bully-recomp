// Loading an anim group's AGR file (the HXD's directory, the group's name, ".AGR"): read
// whole into memory and each of the group's anims' data made from it (a version word 0x100,
// the codec's kind, three words, then the codec's own loader). Groups over 50000 bytes get
// a block per anim, the others one block with the size as a sentinel word after it.
// 0x6BE5E0, 0x6BEFA0..0x6BF55F.

#include <cstdint>
#include <cstring>

#include "game/FileLoader.h"
#include "reversed.h"

namespace {

using re::At;

// An anim (0x40): +0 its data, +4 its own block (big groups), +8 the HXD, +0x1C the data's
// size, +0x20 its group. A group (0x3C): +4 its name, +0x24 flags (1 loaded), +0x30 its
// data's size, +0x34 a byte (loading asked), +0x38 its block. The HXD: +0 anims, +4 their
// count, +0x10 flags (1 something loaded), +0x1C its path, +0x68 groups' count, +0x6C groups.
// A codec (0x30, the table at 0x20C5C58): +0 kind, +0x20 its loader (stream, data).

GAME_VAR(uint8_t*, agr_loadingAnim, 0x020C4904);

GAME_FN(void*, __cdecl, Agr_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Agr_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, Agr_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Agr_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Agr_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(uint8_t, __cdecl, Agr_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(uint32_t, __cdecl, Agr_Tell, 0x0042D430, void* file);
GAME_FN(int, __cdecl, Agr_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(void, __cdecl, Agr_Close, 0x0042D450, void* file);
GAME_FN(uint32_t, __cdecl, Agr_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);
GAME_FN(FileStream*, __cdecl, Agr_OpenStream, 0x0042D4C0, int kind, int mode, void* arg);
GAME_FN(int, __cdecl, Agr_CloseStream, 0x0042D570, FileStream* stream, FileStreamBuffer* out);
GAME_FN(void*, __cdecl, Agr_StreamRead, 0x006B2BF0, FileStream* stream, void* to, int size);
GAME_FN(uint8_t*, __cdecl, Agr_FindCodec, 0x006C80B0, uint32_t kind);
GAME_FN(uint8_t*, __stdcall, Agr_LoadAnimData, 0x006BE5E0, const char* name, uint8_t* out, uint32_t size,
        FileStream* stream);
GAME_FN(int, __thiscall, Agr_LoadInPieces, 0x006BEFA0, void* manager, uint8_t* hxd, uint32_t group,
        FileStream* stream);
GAME_FN(int, __thiscall, Agr_LoadInOne, 0x006BF240, void* manager, uint8_t* hxd, uint32_t group,
        FileStream* stream);

// a word read the way the loader inlines it (4: read)
uint32_t ReadWord(FileStream* s, uint32_t* to) {
    if (s->file) {
        return Agr_ReadItems(to, 4, 1, s->file);
    }
    if (s->data) {
        *to = *reinterpret_cast<uint32_t*>(s->data + s->position);
        s->position += 4;
        return 4;
    }
    return 0;
}

// 0x6BE5E0: an anim's data read into `out` from the stream (or the named file): the
// version (0x100), the codec's kind, three words, the codec's part. Null when anything's
// wrong (`out` null included: the named file is left open then). `size` isn't used.
uint8_t* __stdcall LoadAnimData(const char* name, uint8_t* out, uint32_t, FileStream* stream) {
    bool opened = false;
    if (stream == nullptr) {
        if (name == nullptr) {
            return nullptr;
        }
        opened = true;
        stream = Agr_OpenStream(2, 1, const_cast<char*>(name));
        if (stream == nullptr) {
            return nullptr;
        }
    }
    uint32_t version;
    if (ReadWord(stream, &version) != 4 || version != 0x100) {
        return nullptr;
    }
    uint32_t kind;
    if (ReadWord(stream, &kind) != 4) {
        return nullptr;
    }
    uint8_t* codec = Agr_FindCodec(kind);
    if (codec == nullptr || out == nullptr) {
        return nullptr;
    }
    At<uint8_t*>(out, 0) = codec;
    if (!Agr_StreamRead(stream, out + 4, 4) || !Agr_StreamRead(stream, out + 8, 4) ||
        !Agr_StreamRead(stream, out + 0xC, 4)) {
        return nullptr;
    }
    At<uint8_t*>(out, 0x10) = out + 0x18;
    using Load = void(__cdecl*)(FileStream*, uint8_t*);
    At<Load>(codec, 0x20)(stream, out);
    if (opened) {
        Agr_CloseStream(stream, nullptr);
    }
    return out;
}

// the group's AGR file's path (the HXD's up to its last '\', the name, ".AGR")
void AgrPath(char* path, uint8_t* hxd, uint8_t* group) {
    std::strcpy(path, reinterpret_cast<char*>(hxd + 0x1C));
    uint32_t n = uint32_t(std::strlen(path));
    uint32_t cut = 0;
    if (path[n] == '\\') {
        if (n != 0) {
            cut = n + 1;
        }
    } else {
        while (n != 0) {
            n--;
            if (path[n] == '\\') {
                if (n != 0) {
                    cut = n + 1;
                }
                break;
            }
        }
    }
    path[cut] = 0;
    std::strcat(path, reinterpret_cast<char*>(group + 4));
    std::strcat(path, ".AGR");
}

// the file read whole into memory (tag 0x13) and a memory stream opened on it
FileStream* OpenAgr(const char* path, re::Ptr32<void>* block) {
    void* file = Agr_OpenFile(path, "r", 1);
    Agr_Seek(file, 0, 2);
    uint32_t size = Agr_Tell(file);
    Agr_Seek(file, 0, 0);
    Agr_PushMemoryTag(0x13);
    void* data = Agr_Malloc(size);
    *block = data;
    Agr_PopMemoryTag();
    Agr_Read(file, data, int(size));
    Agr_Close(file);
    FileStreamBuffer memory{static_cast<uint8_t*>(data), size};
    return Agr_OpenStream(3, 1, &memory);
}

// 0x6BEFA0 (thiscall, the manager unused): the group's anims each in a block of their
// own. 0 loaded (or already), 0x1D no file, 0x1F the sizes don't add up (the file's memory
// kept then); else 1 when an anim failed (its size then 0)
int __fastcall LoadInPieces(void* self, re::Edx, uint8_t* hxd, uint32_t groupIndex, FileStream* stream) {
    uint8_t* group = At<uint8_t*>(hxd, 0x6C) + groupIndex * 0x3C;
    if (At<uint8_t>(group, 0x24) & 1) {
        return 0;
    }
    uint32_t total = 0;
    At<uint8_t>(group, 0x34) = 1;
    char path[0x80];
    bool opened = false;
    bool read = false;
    re::Ptr32<void> block = nullptr;
    if (stream == nullptr) {
        AgrPath(path, hxd, group);
        stream = OpenAgr(path, &block);
        read = true;
        if (stream == nullptr) {
            return 0x1D;
        }
        opened = true;
    }
    int failed = 0;
    for (uint32_t i = 0; i < At<uint32_t>(hxd, 4); i++) {
        uint8_t* anim = At<uint8_t*>(hxd, 0) + i * 0x40;
        if (At<uint32_t>(anim, 0x20) != groupIndex) {
            continue;
        }
        agr_loadingAnim = anim;
        uint8_t* data = static_cast<uint8_t*>(Agr_Malloc(At<uint32_t>(anim, 0x1C)));
        At<uint8_t*>(anim, 4) = data;
        uint8_t* loaded = Agr_LoadAnimData(nullptr, data, At<uint32_t>(anim, 0x1C), stream);
        At<uint8_t*>(anim, 0) = loaded;
        At<uint8_t*>(anim, 8) = hxd;
        if (loaded == nullptr) {
            failed++;
            At<uint32_t>(anim, 0x1C) = 0;
        }
        total += At<uint32_t>(anim, 0x1C);
        if (total > At<uint32_t>(group, 0x30)) {
            return 0x1F;
        }
    }
    (void)self;
    if (opened) {
        Agr_CloseStream(stream, nullptr);
    }
    if (read) {
        Agr_Free(block);
    }
    if (total != At<uint32_t>(group, 0x30)) {
        return 0x1F;
    }
    At<uint16_t>(group, 0x24) |= 1;
    At<uint32_t>(hxd, 0x10) |= 1;
    return failed != 0 ? 1 : 0;
}

// 0x6BF240 (thiscall, the manager unused): the group's anims in one block (+0x38, the size
// written after it and checked at the end). 0 loaded, 6 no memory, 0x1D no file, 0x1F the
// sizes don't add up
int __fastcall LoadInOne(void* self, re::Edx, uint8_t* hxd, uint32_t groupIndex, FileStream* stream) {
    uint8_t* group = At<uint8_t*>(hxd, 0x6C) + groupIndex * 0x3C;
    if (At<uint8_t>(group, 0x24) & 1) {
        return 0;
    }
    uint32_t total = 0;
    uint8_t* data = static_cast<uint8_t*>(Agr_Malloc(At<uint32_t>(group, 0x30) + 4));
    At<uint8_t*>(group, 0x38) = data;
    if (data == nullptr) {
        return 6;
    }
    At<uint32_t>(data, At<uint32_t>(group, 0x30)) = At<uint32_t>(group, 0x30);
    char path[0x80];
    At<uint8_t>(group, 0x34) = 1;
    bool opened = false;
    bool read = false;
    re::Ptr32<void> block = nullptr;
    if (stream == nullptr) {
        AgrPath(path, hxd, group);
        stream = OpenAgr(path, &block);
        read = true;
        if (stream == nullptr) {
            return 0x1D;
        }
        opened = true;
    }
    for (uint32_t i = 0; i < At<uint32_t>(hxd, 4); i++) {
        uint8_t* anim = At<uint8_t*>(hxd, 0) + i * 0x40;
        if (At<uint32_t>(anim, 0x20) != groupIndex) {
            continue;
        }
        agr_loadingAnim = anim;
        uint8_t* loaded =
            Agr_LoadAnimData(nullptr, At<uint8_t*>(group, 0x38) + total, At<uint32_t>(anim, 0x1C), stream);
        At<uint8_t*>(anim, 0) = loaded;
        if (loaded == nullptr) {
            At<uint32_t>(anim, 0x1C) = 0;
        }
        At<uint8_t*>(anim, 8) = hxd;
        total += At<uint32_t>(anim, 0x1C);
        if (total > At<uint32_t>(group, 0x30)) {
            return 0x1F;
        }
    }
    (void)self;
    if (opened) {
        Agr_CloseStream(stream, nullptr);
    }
    if (read) {
        Agr_Free(block);
    }
    uint32_t size = At<uint32_t>(group, 0x30);
    if (At<uint32_t>(At<uint8_t*>(group, 0x38), size) != size) {
        return 0x1F;
    }
    At<uint16_t>(group, 0x24) |= 1;
    At<uint32_t>(hxd, 0x10) |= 1;
    return 0;
}

// 0x6BF500 (thiscall, the manager passed on): an anim group's AGR loaded. 0x16 no HXD, 2 no
// such group, 0 loaded already, else the loaders' result
int __fastcall LoadAgr(void* self, re::Edx, uint8_t* hxd, uint32_t groupIndex, FileStream* stream) {
    if (hxd == nullptr) {
        return 0x16;
    }
    if (groupIndex >= At<uint32_t>(hxd, 0x68)) {
        return 2;
    }
    uint8_t* group = At<uint8_t*>(hxd, 0x6C) + groupIndex * 0x3C;
    if (At<uint8_t>(group, 0x24) & 1) {
        return 0;
    }
    if (At<uint32_t>(group, 0x30) > 50000) {
        return Agr_LoadInPieces(self, hxd, groupIndex, stream);
    }
    return Agr_LoadInOne(self, hxd, groupIndex, stream);
}

} // namespace

#define AMAGR_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMAGR_REVERSED("AM_Anim::LoadData", 0x006BE5E0, &LoadAnimData);
AMAGR_REVERSED("AM_Manager::LoadAgrInPieces", 0x006BEFA0, &LoadInPieces);
AMAGR_REVERSED("AM_Manager::LoadAgrInOne", 0x006BF240, &LoadInOne);
AMAGR_REVERSED("AM_Manager::LoadAgr", 0x006BF500, &LoadAgr);
