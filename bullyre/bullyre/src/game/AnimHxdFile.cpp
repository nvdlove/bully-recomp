// A hierarchy file (.HXD, version up to 1.12): the skeleton's weight sets, its sockets,
// its anims (0x40 bytes each, with their events) and anim groups (0x3C bytes), read
// from a stream or the named file, and its reset / free / lookups. 0x6B17A0..0x6B3D99.
// Also the 0xB0-byte object reset at 0x6B1DF0..0x6B1EDC.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/FileLoader.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

// An anim (0x40)
//  +0, +4 its data (loaded later; both 0 while not), +0xC the duration, +0x10, +0x14 the
//  name's hash, +0x18 flags, +0x1C, +0x20 its group, +0x24 events (0x10 each), +0x28 their
//  count, +0x2A..+0x30 shorts, +0x32 its index, +0x34 a vector
// An anim group (0x3C)
//  +4 its name (0x20), +0x24 flags (1 loaded, 2 kept), +0x28, +0x2C a use count,
//  +0x30, +0x34 a byte, +0x38 the group's data (when loaded in one piece)
struct HxdFile {
    re::Ptr32<uint8_t> anims;          // +0
    uint32_t animCount;      // +4
    re::Ptr32<uint8_t> weightSets;     // +8 (0x2C each: a name, the weights, their count, a float)
    uint32_t weightSetCount; // +0xC
    uint32_t unknown10;
    uint32_t unknown14;
    uint32_t unknown18;      // +0x18 (read from the file)
    char name[0x40];         // +0x1C
    uint32_t socketCount;    // +0x5C
    re::Ptr32<uint8_t> sockets;        // +0x60 AM_Socket (0xC each: vtable, the name's hash, index)
    uint32_t unknown64;      // +0x64 (the default group's +0x30)
    uint32_t groupCount;     // +0x68
    re::Ptr32<uint8_t> groups;         // +0x6C
    re::Ptr32<uint8_t> socketData;     // +0x70 socketCount * 0xC bytes (Gamebryo's heap)
    re::Ptr32<uint8_t> unknown74;      // +0x74 0x30 each
    uint32_t unknown78;      // +0x78 their count
};
static_assert(offsetof(HxdFile, socketCount) == 0x5C);
static_assert(offsetof(HxdFile, unknown78) == 0x78);

GAME_ARRAY(uint32_t, hxd_zeroVector, 0x00BCC564);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, NiMalloc, 0x007524D0, uint32_t size);
GAME_FN(void, __cdecl, NiFreeArray, 0x00752570, void* p, uint32_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __cdecl, FileMgr_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(uint8_t, __cdecl, File_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(uint32_t, __cdecl, File_Tell, 0x0042D430, void* file);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(void, __cdecl, File_Close, 0x0042D450, void* file);
GAME_FN(uint32_t, __cdecl, File_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);
GAME_FN(FileStream*, __cdecl, OpenStream_, 0x0042D4C0, int kind, int mode, void* arg);
GAME_FN(int, __cdecl, CloseStream_, 0x0042D570, FileStream* stream, FileStreamBuffer* out);
GAME_FN(int, __cdecl, DataStream_Read, 0x0042E730, FileStream* stream, void* to, int size);
inline void* __cdecl Memcpy(void* dst, const void* src, size_t n) { return crt::Memcpy(dst, src, n); }
inline void* const kWeightSetCtor = reinterpret_cast<void*>(0x006B17A0);
inline void* const kSocketCtor = reinterpret_cast<void*>(0x006B17C0);

// count * size, all ones when it doesn't fit (new[]'s check)
inline uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
}

// the stream reads the loader inlines: a file's through the file manager, memory's
// copied (nothing when the stream or its memory is missing)
void Read(FileStream* s, void* to, uint32_t size) {
    if (s == nullptr) {
        return;
    }
    if (s->file) {
        File_ReadItems(to, size, 1, s->file);
    } else if (s->data) {
        if (size == 2) {
            *static_cast<uint16_t*>(to) = *reinterpret_cast<uint16_t*>(s->data + s->position);
        } else if (size == 4) {
            *static_cast<uint32_t*>(to) = *reinterpret_cast<uint32_t*>(s->data + s->position);
        } else if (size == 8 || size == 12) {
            std::memcpy(to, s->data + s->position, size);  // (copied a word at a time)
        } else {
            Memcpy(to, s->data + s->position, size);
        }
        s->position += size;
    }
}
void Skip(FileStream* s, uint32_t size) {
    if (s == nullptr) {
        return;
    }
    if (s->file) {
        File_Seek(s->file, int32_t(size), 1);
    } else if (s->data) {
        s->position += size;
    }
}

void ConstructGroups(uint8_t* groups, uint32_t count) {
    for (int i = int(count) - 1, j = 0; i >= 0; i--, j++) {
        uint8_t* g = groups + j * 0x3C;
        At<float>(g, 0x28) = 0.0f;
        At<uint32_t>(g, 0) = 0;
        At<uint16_t>(g, 0x24) = 0;
        At<uint16_t>(g, 0x2C) = 0;
        At<uint32_t>(g, 0x30) = 0;
        At<uint8_t>(g, 0x34) = 0;
        At<uint32_t>(g, 0x38) = 0;
        At<uint8_t>(g, 4) = 0;
    }
}

// 0x6B17A0: a weight set's constructor (0x2C)
void* __fastcall WeightSet_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0x28) = 0.3f;
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    At<uint8_t>(self, 0) = 0;
    return self;
}

// 0x6B1B20 / 0x6B2EB0 (the same code twice): empty
void __fastcall Hxd_Reset(HxdFile* self, re::Edx) {
    self->anims = nullptr;
    self->animCount = 0;
    self->weightSets = nullptr;
    self->weightSetCount = 0;
    self->unknown10 = 0;
    self->unknown14 = 0;
    self->unknown18 = 0;
    self->socketCount = 0;
    self->sockets = nullptr;
    self->unknown64 = 0;
    self->groupCount = 0;
    self->groups = nullptr;
    self->socketData = nullptr;
    self->unknown74 = nullptr;
    self->unknown78 = 0;
    self->name[0] = 0;
}

// 0x6B1B60: frees everything (the weight sets' own weights aren't)
void __fastcall Hxd_Free(HxdFile* self, re::Edx) {
    if (self->anims) {
        Free(self->anims);
    }
    self->anims = nullptr;
    self->animCount = 0;
    if (self->weightSets) {
        Free(self->weightSets);
    }
    self->weightSets = nullptr;
    self->weightSetCount = 0;
    self->unknown10 = 0;
    self->unknown14 = 0;
    self->unknown18 = 0;
    if (self->sockets) {
        Free(self->sockets);
    }
    self->sockets = nullptr;
    self->socketCount = 0;
    self->unknown64 = 0;
    if (self->groups) {
        Free(self->groups);
    }
    self->groups = nullptr;
    self->groupCount = 0;
    if (self->socketData) {
        NiFreeArray(self->socketData, 0xC);
    }
    self->socketData = nullptr;
    if (self->unknown74) {
        Free(self->unknown74);
        self->unknown74 = nullptr;
    }
    self->unknown74 = nullptr;
    self->unknown78 = 0;
    self->name[0] = 0;
}

// 0x6B1C00: a jump to Hxd_Free
RE_UNHOOKED_FN("HxdFile::Free_thunk", 0x006B1C00, &Hxd_Free);

// 0x6B1C60: one more user of a group (2: no such group)
int __fastcall Hxd_AddGroupRef(HxdFile* self, re::Edx, uint32_t group) {
    if (group >= self->groupCount) {
        return 2;
    }
    At<uint16_t>(self->groups + group * 0x3C, 0x2C) += 1;
    return 0;
}

// 0x6B1C90: unloads a group's anims (not when it's kept)
int __fastcall Hxd_UnloadGroup(HxdFile* self, re::Edx, uint32_t group) {
    if (group >= self->groupCount) {
        return 2;
    }
    uint8_t* g = self->groups + group * 0x3C;
    uint16_t flags = At<uint16_t>(g, 0x24);
    if (flags & 2) {
        return 0;
    }
    At<uint16_t>(g, 0x24) = flags & 0xFFFE;
    At<uint8_t>(g, 0x34) = 0;
    if (At<void*>(g, 0x38)) {
        // the group's anims point into its one block
        Free(At<void*>(g, 0x38));
        At<void*>(g, 0x38) = nullptr;
        for (uint32_t i = 0; i < self->animCount; i++) {
            uint8_t* anim = self->anims + i * 0x40;
            if (At<uint32_t>(anim, 0x20) == group) {
                At<void*>(anim, 4) = nullptr;
                At<void*>(self->anims + i * 0x40, 0) = nullptr;
            }
        }
        return 0;
    }
    for (uint32_t i = 0; i < self->animCount; i++) {
        uint8_t* anim = self->anims + i * 0x40;
        if (At<uint32_t>(anim, 0x20) == group) {
            Free(At<void*>(anim, 4));
            At<void*>(self->anims + i * 0x40, 4) = nullptr;
            At<void*>(self->anims + i * 0x40, 0) = nullptr;
        }
    }
    return 0;
}

// 0x6B1D60: a loaded anim by its name's hash (or the name when the hash is 0)
uint8_t* __fastcall Hxd_FindAnim(HxdFile* self, re::Edx, uint32_t hash, const char* name, uint32_t* index) {
    if (name == nullptr) {
        if (hash == 0) {
            return nullptr;
        }
    } else if (hash == 0) {
        hash = HashName(name);
    }
    uint32_t i = 0;
    for (; i < self->animCount; i++) {
        uint8_t* anim = self->anims + i * 0x40;
        if (At<uint32_t>(anim, 0x14) == hash && At<void*>(anim, 0) != nullptr) {
            break;
        }
    }
    if (i >= self->animCount) {
        return nullptr;
    }
    if (index) {
        *index = i;
    }
    return self->anims + (i << 6);
}

// 0x6B2EF0: loads the file from `stream`, or from "<name>.HXD" when that's null (read
// into memory whole). 0: done, 6: out of memory, 10: a newer version, 0x26: no file
int __fastcall Hxd_Load(HxdFile* self, re::Edx, const char* name, FileStream* stream) {
    bool opened = false;
    void* buffer = nullptr;
    if (stream == nullptr) {
        opened = true;
        char path[0x7D + 0x20];
        path[0] = 0;
        std::strcat(path, name);
        std::strcat(path, ".HXD");
        void* file = FileMgr_OpenFile(path, "r", 1);
        if (file == nullptr) {
            return 0x26;
        }
        File_Seek(file, 0, 2);
        uint32_t size = File_Tell(file);
        File_Seek(file, 0, 0);
        buffer = Malloc(size);
        File_Read(file, buffer, int(size));
        File_Close(file);
        FileStreamBuffer memory{static_cast<uint8_t*>(buffer), size};
        stream = OpenStream_(3, 1, &memory);
    }
    float version = 0.0f;
    if (stream) {
        if (stream->file || stream->data) {
            Read(stream, &version, 4);
            if (version > 1.12f) {
                CloseStream_(stream, nullptr);  // (the memory's left)
                return 10;
            }
        }
    }
    std::strcpy(self->name, name);
    Read(stream, &self->unknown18, 4);
    Read(stream, &self->weightSetCount, 4);

    uint32_t count = self->weightSetCount;
    uint8_t* sets = static_cast<uint8_t*>(Malloc(ArrayBytes(count, 0x2C)));
    if (sets) {
        VectorConstruct(sets, 0x2C, count, kWeightSetCtor);
    }
    self->weightSets = sets;
    if (sets == nullptr) {
        return 6;
    }
    for (uint32_t i = 0; i < self->weightSetCount; i++) {
        Read(stream, self->weightSets + i * 0x2C, 0x20);
        Read(stream, self->weightSets + i * 0x2C + 0x24, 4);
        Read(stream, self->weightSets + i * 0x2C + 0x28, 4);
        uint8_t* set = self->weightSets + i * 0x2C;
        if (At<uint32_t>(set, 0x24) == 0) {
            At<void*>(set, 0x20) = nullptr;
        } else {
            At<void*>(self->weightSets + i * 0x2C, 0x20) = Malloc(ArrayBytes(At<uint32_t>(set, 0x24), 4));
            set = self->weightSets + i * 0x2C;
            DataStream_Read(stream, At<void*>(set, 0x20), int(At<uint32_t>(set, 0x24) * 4));
        }
    }

    Read(stream, &self->socketCount, 4);
    count = self->socketCount;
    if (count) {
        uint8_t* sockets = static_cast<uint8_t*>(Malloc(ArrayBytes(count, 0xC)));
        if (sockets) {
            VectorConstruct(sockets, 0xC, count, kSocketCtor);
        }
        self->sockets = sockets;
        if (sockets == nullptr) {
            return 6;
        }
        for (uint32_t i = 0; i < self->socketCount; i++) {
            // (the game's buffer runs on into other locals: the name is 0x20 bytes at most)
            char socketName[0x21] = {};
            Read(stream, socketName, 0x20);
            At<uint32_t>(self->sockets + i * 0xC, 4) = HashName(socketName);
            At<uint32_t>(self->sockets + i * 0xC, 8) = i;
        }
    }

    Read(stream, &self->animCount, 4);
    count = self->animCount;
    uint8_t* anims = static_cast<uint8_t*>(Malloc(ArrayBytes(count, 0x40)));
    if (anims) {
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* a = anims + i * 0x40;
            At<float>(a, 0xC) = 0.0f;
            At<uint32_t>(a, 0) = 0;
            At<uint32_t>(a, 4) = 0;
            At<float>(a, 0x10) = 0.0f;
            At<uint32_t>(a, 8) = 0;
            for (int off = 0x14; off <= 0x24; off += 4) {
                At<uint32_t>(a, off) = 0;
            }
            for (int off = 0x28; off <= 0x30; off += 2) {
                At<uint16_t>(a, off) = 0;
            }
            At<uint16_t>(a, 0x32) = 0xFFFF;
            At<uint32_t>(a, 0x34) = hxd_zeroVector[0];
            At<uint32_t>(a, 0x38) = hxd_zeroVector[1];
            At<uint32_t>(a, 0x3C) = hxd_zeroVector[2];
        }
    }
    self->anims = anims;
    if (anims == nullptr) {
        return 6;
    }
    for (uint32_t i = 0; i < self->animCount; i++) {
        uint32_t a = i * 0x40;
        Read(stream, self->anims + a + 0xC, 4);
        Read(stream, self->anims + a + 0x10, 4);
        Skip(stream, 0x40);
        Read(stream, self->anims + a + 0x14, 4);
        Read(stream, self->anims + a + 0x18, 4);
        Read(stream, self->anims + a + 0x1C, 4);
        Read(stream, self->anims + a + 0x20, 4);
        Read(stream, self->anims + a + 0x28, 2);
        At<uint16_t>(self->anims + a, 0x32) = uint16_t(i);
        uint32_t events = At<uint16_t>(self->anims + a, 0x28);
        if (events == 0) {
            At<void*>(self->anims + a, 0x24) = nullptr;
        } else {
            At<void*>(self->anims + a, 0x24) = Malloc(ArrayBytes(events, 0x10));
            for (uint32_t e = 0; e < events; e++) {
                uint32_t ev = e * 0x10;
                Read(stream, At<uint8_t*>(self->anims + a, 0x24) + ev, 4);
                Read(stream, At<uint8_t*>(self->anims + a, 0x24) + ev + 4, 4);
                Read(stream, At<uint8_t*>(self->anims + a, 0x24) + ev + 8, 4);
                uint8_t* event = At<uint8_t*>(self->anims + a, 0x24) + ev;
                if (At<int32_t>(event, 8) == 1) {
                    At<void*>(At<uint8_t*>(self->anims + a, 0x24) + ev, 0xC) = Malloc(8);
                    void* data = At<void*>(At<uint8_t*>(self->anims + a, 0x24) + ev, 0xC);
                    if (stream) {
                        Read(stream, data, 8);
                        Skip(stream, 0x20);
                    }
                } else {
                    At<void*>(event, 0xC) = nullptr;
                }
            }
        }
        Read(stream, self->anims + a + 0x2A, 2);
        Read(stream, self->anims + a + 0x2E, 2);
        Read(stream, self->anims + a + 0x2C, 2);
        Read(stream, self->anims + a + 0x30, 2);
        Skip(stream, 4);
        if (version > 1.09f) {
            Skip(stream, 4);
            if (version > 1.11f) {
                Skip(stream, 4);
            }
        }
        Read(stream, self->anims + a + 0x34, 0xC);
        At<uint32_t>(self->anims + a, 0) = 0;
        At<uint32_t>(self->anims + a, 4) = 0;
    }

    Read(stream, &self->groupCount, 4);
    count = self->groupCount;
    if (count) {
        uint8_t* groups = static_cast<uint8_t*>(Malloc(ArrayBytes(count, 0x3C)));
        if (groups) {
            ConstructGroups(groups, count);
        }
        self->groups = groups;
        for (uint32_t i = 0; i < self->groupCount; i++) {
            Read(stream, self->groups + i * 0x3C + 4, 0x20);
            Read(stream, self->groups + i * 0x3C + 0x30, 4);
            if (version >= 1.11f) {
                Skip(stream, 0x20);
                Read(stream, self->groups + i * 0x3C + 0x28, 4);
            }
        }
    }

    count = self->socketCount;
    uint8_t* socketData = static_cast<uint8_t*>(NiMalloc(ArrayBytes(count, 0xC)));
    if (socketData) {
        for (uint32_t i = 0; i < count; i++) {
            At<float>(socketData + i * 0xC, 0) = 0.0f;
            At<float>(socketData + i * 0xC, 4) = 0.0f;
            At<float>(socketData + i * 0xC, 8) = 0.0f;
        }
    }
    self->socketData = socketData;
    Read(stream, socketData, self->socketCount * 0xC);

    Read(stream, &self->unknown78, 4);
    count = self->unknown78;
    if (count) {
        uint8_t* items = static_cast<uint8_t*>(Malloc(ArrayBytes(count, 0x30)));
        if (items) {
            for (uint32_t i = 0; i < count; i++) {
                uint8_t* item = items + i * 0x30;
                At<float>(item, 0x28) = 0.0f;
                At<float>(item, 0x20) = 0.0f;
                At<float>(item, 0x24) = 0.0f;
                At<float>(item, 0x2C) = 1.0f;
                At<uint8_t>(item, 0) = 0;
            }
        }
        self->unknown74 = items;
        Read(stream, items, self->unknown78 * 0x30);
    }
    Read(stream, &self->unknown64, 4);

    if (opened) {
        CloseStream_(stream, nullptr);
    }
    if (buffer) {
        Free(buffer);
    }
    if (self->groupCount == 0) {
        // one group, "default"
        self->groupCount = 1;
        Free(self->groups);
        uint8_t* groups = static_cast<uint8_t*>(Malloc(ArrayBytes(self->groupCount, 0x3C)));
        if (groups) {
            ConstructGroups(groups, self->groupCount);
        }
        self->groups = groups;
        std::memcpy(groups + 4, "default", 8);
        At<uint32_t>(self->groups, 0x30) = self->unknown64;
    }
    uint8_t* set = self->weightSets;
    if (At<void*>(set, 0x20) == nullptr) {
        // the first set weighs every socket fully
        At<uint32_t>(set, 0x24) = self->socketCount;
        At<float*>(self->weightSets, 0x20) = static_cast<float*>(Malloc(ArrayBytes(self->socketCount, 4)));
        for (uint32_t i = 0; i < self->socketCount; i++) {
            At<float*>(self->weightSets, 0x20)[i] = 1.0f;
        }
    }
    return 0;
}

// 0x6B1DF0: the 0xB0-byte object's constructor
void* __fastcall HierarchyFile_Construct(uint8_t* self, re::Edx) {
    At<float>(self, 0xAC) = 0.0f;
    for (int off = 0; off <= 0x18; off += 4) {
        At<uint32_t>(self, off) = 0;
    }
    At<uint32_t>(self, 0x9C) = 0;
    At<uint32_t>(self, 0xA0) = 0;
    At<uint32_t>(self, 0xA4) = 0;
    At<uint32_t>(self, 0xA8) = 0;
    At<uint8_t>(self, 0x1C) = 0;
    At<uint8_t>(self, 0x5C) = 0;
    return self;
}

// 0x6B1E30: empties it (frees +0xA0 and the Gamebryo array at +0xA8)
void __fastcall HierarchyFile_Reset(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0x10) = 0;
    At<uint32_t>(self, 0x14) = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x9C) = 0;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0xA4) = 0;
    if (At<void*>(self, 0xA0)) {
        Free(At<void*>(self, 0xA0));
    }
    if (At<void*>(self, 0xA8)) {
        NiFreeArray(At<void*>(self, 0xA8), 0xC);
    }
    At<void*>(self, 0xA0) = nullptr;
    At<void*>(self, 0xA8) = nullptr;
    At<float>(self, 0xAC) = 0.0f;
    At<uint8_t>(self, 0x1C) = 0;
    At<uint8_t>(self, 0x5C) = 0;
}

// 0x6B1EA0: frees its arrays
void __fastcall HierarchyFile_FreeArrays(uint8_t* self, re::Edx) {
    if (At<void*>(self, 0xA0)) {
        Free(At<void*>(self, 0xA0));
    }
    if (At<void*>(self, 0xA8)) {
        NiFreeArray(At<void*>(self, 0xA8), 0xC);
    }
    At<void*>(self, 0xA8) = nullptr;
    At<void*>(self, 0xA0) = nullptr;
}

} // namespace

#define AMHXD_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMHXD_REVERSED("HxdWeightSet::HxdWeightSet", 0x006B17A0, &WeightSet_Construct);
AMHXD_REVERSED("HxdFile::Reset", 0x006B1B20, &Hxd_Reset);
AMHXD_REVERSED("HxdFile::Free", 0x006B1B60, &Hxd_Free);
AMHXD_REVERSED("HxdFile::AddGroupRef", 0x006B1C60, &Hxd_AddGroupRef);
AMHXD_REVERSED("HxdFile::UnloadGroup", 0x006B1C90, &Hxd_UnloadGroup);
AMHXD_REVERSED("HxdFile::FindAnim", 0x006B1D60, &Hxd_FindAnim);
AMHXD_REVERSED("AM_HierarchyFile::AM_HierarchyFile", 0x006B1DF0, &HierarchyFile_Construct);
AMHXD_REVERSED("AM_HierarchyFile::Reset", 0x006B1E30, &HierarchyFile_Reset);
AMHXD_REVERSED("AM_HierarchyFile::FreeArrays", 0x006B1EA0, &HierarchyFile_FreeArrays);
AMHXD_REVERSED("HxdFile::Reset_6B2EB0", 0x006B2EB0, &Hxd_Reset);
AMHXD_REVERSED("HxdFile::Load", 0x006B2EF0, &Hxd_Load);
