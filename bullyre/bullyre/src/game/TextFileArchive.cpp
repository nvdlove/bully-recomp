// The archives (0x5B06C0..0x5B104D): an archive's section helpers (by vtable slots
// 0x70..0x80, and a named-section table), opening and closing archives (text for paths
// ending in 't', else from a file at a sector), and TextFileArchive (vtable 0x922F64, 0x174
// bytes): tokens read from a 256-byte line buffer (+0x14, position +0x16C) split at the
// separators (+0x15C), '#' comments when +0x170, values written as "<indent><text>\n"
// (the indent at +0x115). The loose text file in memory (0xCFAD4C: {file, data, size,
// position}) is read a line at a time.

#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_VAR(uint8_t*, memoryTextFile, 0x00CFAD4C);   // {file, data, size, position}
GAME_VAR(void*, memoryTextData, 0x00CFAD50);

GAME_FN(void*, __cdecl, New, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline int __cdecl ToLower(int c) { return crt::Tolower(c); }
inline int __cdecl IsSpace(int c) { return crt::Isspace(c); }
inline char* __cdecl Strncpy(char* dst, const char* src, uint32_t n) { return crt::Strncpy(dst, src, (size_t)n); }
inline char* __cdecl Strchr(const char* s, int c) { return crt::Strchr(s, c); }
inline char* __cdecl StrNCat(char* dst, const char* src, uint32_t count) { return crt::Strncat(dst, src, (size_t)count); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(int, __cdecl, File_Write, 0x0042D2D0, void* file, const void* buffer, int size);
GAME_FN(int, __cdecl, File_Seek, 0x0042D300, void* file, int offset, int origin);
GAME_FN(int, __cdecl, File_Close, 0x0042D450, void* file);
GAME_FN(uint32_t, __cdecl, File_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, void* file);
GAME_FN(int, __cdecl, File_CloseStream, 0x0042D570, void* stream, void* out);
GAME_FN(int, __cdecl, LoadFile, 0x0042D180, const char* path, void* buffer, int size, const char* mode);
GAME_FN(void, __cdecl, Sound_Lock, 0x005EF0A0, int);
GAME_FN(void, __cdecl, Sound_Unlock, 0x005EF100);
// this file's
GAME_FN(void*, __cdecl, Archive_OpenText, 0x005B0B00, const char* path, const int32_t* mode, int);
GAME_FN(void*, __cdecl, Archive_OpenAtSector, 0x005B0B90, void* file, int32_t sector, int);
GAME_FN(void, __cdecl, Archive_CloseText, 0x005B0C10, void* archive);
GAME_FN(void, __cdecl, Archive_Delete, 0x005B0900, void* archive);
GAME_FN(void*, __thiscall, TextFileArchive_Construct, 0x005B0920, void* self, void* file, const int32_t* mode,
        const char* path, int);
GAME_FN(int, __cdecl, MemoryText_ReadLine, 0x005B0C70, void* text, char* buffer, int size);
GAME_FN(void, __thiscall, TextFileArchive_WriteLine, 0x005B0FA0, void* self, const char* text);

inline re::Ptr32<void>* VtableOf(void* object) {
    return *static_cast<re::Ptr32<re::Ptr32<void>>*>(object);
}

// ---- an archive's sections ----------------------------------------------------------------------

// 0x5B06C0 (thiscall): opens a section (vf 0x70); an object reads it (its vf1) then it
// closes (vf 0x78), else it's skipped (vf 0x74, 0x78)
uint8_t __fastcall Archive_ReadSection(uint8_t* self, re::Edx, void* reader, void* name) {
    using NameFn = uint8_t(__thiscall*)(void*, void*);
    using PlainFn = void(__thiscall*)(void*);
    uint8_t opened = re::PtrCast<NameFn>(VtableOf(self)[0x70 / 4])(self, name);
    if (opened == 0) {
        return opened;
    }
    if (reader != nullptr) {
        using ReadFn = void(__thiscall*)(void*, void*);
        re::PtrCast<ReadFn>(VtableOf(reader)[1])(reader, self);
        re::PtrCast<PlainFn>(VtableOf(self)[0x78 / 4])(self);
        return opened;
    }
    re::PtrCast<PlainFn>(VtableOf(self)[0x74 / 4])(self);
    re::PtrCast<PlainFn>(VtableOf(self)[0x78 / 4])(self);
    return opened;
}

// 0x5B0730 (thiscall): writes a section (vf 0x7C, the writer's vf2, vf 0x80)
void __fastcall Archive_WriteSection(uint8_t* self, re::Edx, void* writer, void* name) {
    using NameFn = void(__thiscall*)(void*, void*);
    using PlainFn = void(__thiscall*)(void*);
    using WriteFn = void(__thiscall*)(void*, void*);
    re::PtrCast<NameFn>(VtableOf(self)[0x7C / 4])(self, name);
    re::PtrCast<WriteFn>(VtableOf(writer)[2])(writer, self);
    re::PtrCast<PlainFn>(VtableOf(self)[0x80 / 4])(self);
}

// 0x5B0760 (thiscall): reads a named section of a table ({?, count, 0x18-byte entries: name,
// +0x16 the offset}): the reader gets the section in +4 while it reads
uint8_t __fastcall Archive_ReadNamedSection(uint8_t* self, re::Edx, void* reader, const char* name) {
    uint8_t* table = At<uint8_t*>(self, 0);
    for (uint32_t i = 0; i < At<uint32_t>(table, 4); ++i) {
        const char* entry = reinterpret_cast<const char*>(table + 8 + i * 0x18);
        const char* a = entry;
        const char* b = name;
        while (*a == *b && *a != 0) {
            ++a;
            ++b;
        }
        if (*a == *b) {
            At<uint8_t*>(self, 4) = table + At<uint16_t>(table, i * 0x18 + 0x1E);
            using ReadFn = void(__thiscall*)(void*, void*);
            re::PtrCast<ReadFn>(VtableOf(reader)[1])(reader, self);
            At<uint8_t*>(self, 4) = nullptr;
            return 1;
        }
    }
    return 0;
}

// ---- opening and closing ----------------------------------------------------------------------

// 0x5B07E0: a text archive for a path ending in 't' (either case), else none
void* __cdecl Archive_Open(const char* path, const int32_t* mode, int flags) {
    uint32_t length = 0;
    while (path[length] != 0) {
        ++length;
    }
    char last = path[length - 1];
    int wanted = ToLower(*reinterpret_cast<const char*>(0x00922EB4));   // 't'
    if (ToLower(last) != wanted) {
        return nullptr;
    }
    return Archive_OpenText(path, mode, flags);
}

// 0x5B0840: an archive over an open file at a sector (only for the first kind)
void* __cdecl Archive_OpenFromFile(void* file, int32_t kind, int32_t sector, int flags) {
    if (kind != 0) {
        return nullptr;
    }
    return Archive_OpenAtSector(file, sector, flags);
}

// 0x5B0860: closes a text archive (+8 zero)
void __cdecl Archive_Close(uint8_t* archive) {
    if (archive != nullptr && At<int32_t>(archive, 8) == 0) {
        Archive_CloseText(archive);
    }
}

// 0x5B0880: deletes an archive (+8 zero)
void __cdecl Archive_Release(uint8_t* archive) {
    if (archive != nullptr && At<int32_t>(archive, 8) == 0) {
        Archive_Delete(archive);
    }
}

// 0x5B08A0: a file loaded into the shared 0x1C000-byte buffer at 0xBD7800 ({buffer, 0})
void* __cdecl Archive_LoadToBuffer(const char* path) {
    uint8_t* handle = static_cast<uint8_t*>(New(8));
    if (handle != nullptr) {
        At<void*>(handle, 0) = nullptr;
        At<void*>(handle, 4) = nullptr;
    }
    LoadFile(path, reinterpret_cast<void*>(0x00BD7800), 0x1C000, reinterpret_cast<const char*>(0x009010BC));
    At<uint32_t>(handle, 0) = 0x00BD7800;   // (a failed allocation writes through null)
    return handle;
}

// 0x5B08E0: frees it
void __cdecl Archive_FreeBuffer(void* handle) {
    if (handle != nullptr) {
        Free(handle);
    }
}

// 0x5B0900: deletes an archive (vf0)
void __cdecl Archive_DeleteImpl(void* archive) {
    if (archive != nullptr) {
        using DeleteFn = void*(__thiscall*)(void*, int);
        re::PtrCast<DeleteFn>(VtableOf(archive)[0])(archive, 1);
    }
}

// 0x5B0920 (thiscall): the constructor (the file, the mode's first int)
uint8_t* __fastcall TextFileArchive_ConstructImpl(uint8_t* self, re::Edx, void* file, const int32_t* mode, const char*,
                                                  int) {
    At<void*>(self, 0xC) = file;
    At<uint8_t>(self, 5) = 1;
    At<int32_t>(self, 8) = 0;
    At<uint32_t>(self, 0) = 0x00922F64;
    At<int32_t>(self, 0x10) = *mode;
    At<uint8_t>(self, 0x14) = 0;
    At<uint8_t>(self, 0x114) = 0;
    At<uint8_t>(self, 0x115) = 0;
    At<int32_t>(self, 0x158) = 0;
    At<uint8_t>(self, 0x15C) = 0;
    At<int32_t>(self, 0x16C) = 0;
    At<uint8_t>(self, 0x170) = 0;
    return self;
}

// ---- TextFileArchive's forwarding slots ---------------------------------------------------------

using PairFn = uint32_t(__thiscall*)(void*, void*, int32_t);

// 0x5B0970, 0x5B0990, 0x5B09B0, 0x5B09D0 (thiscall, vf12, vf11, vf22, vf21): the value
// slots without their middle argument (vf 0x38, 0x34, 0x60, 0x5C)
uint32_t __fastcall TextFileArchive_Vf12(uint8_t* self, re::Edx, void* value, int32_t, int32_t count) {
    return re::PtrCast<PairFn>(VtableOf(self)[0x38 / 4])(self, value, count);
}
uint32_t __fastcall TextFileArchive_Vf11(uint8_t* self, re::Edx, void* value, int32_t, int32_t count) {
    return re::PtrCast<PairFn>(VtableOf(self)[0x34 / 4])(self, value, count);
}
uint32_t __fastcall TextFileArchive_Vf22(uint8_t* self, re::Edx, void* value, int32_t, int32_t count) {
    return re::PtrCast<PairFn>(VtableOf(self)[0x60 / 4])(self, value, count);
}
uint32_t __fastcall TextFileArchive_Vf21(uint8_t* self, re::Edx, void* value, int32_t, int32_t count) {
    return re::PtrCast<PairFn>(VtableOf(self)[0x5C / 4])(self, value, count);
}

// 0x5B09F0 (thiscall, vf2): a byte, read as an int (vf 0x38) into a temporary
void __fastcall TextFileArchive_ReadByte(uint8_t* self, re::Edx, uint8_t* out, int32_t value) {
    int32_t temporary = value;
    re::PtrCast<PairFn>(VtableOf(self)[0x38 / 4])(self, &temporary, value);
    *out = uint8_t(temporary);
}

// 0x5B0A10 (thiscall, vf1): a short, the same way
void __fastcall TextFileArchive_ReadShort(uint8_t* self, re::Edx, uint16_t* out, int32_t value) {
    int32_t temporary = value;
    re::PtrCast<PairFn>(VtableOf(self)[0x38 / 4])(self, &temporary, value);
    *out = uint16_t(temporary);
}

// 0x5B0A30 (thiscall, vf33): the separators (15 characters)
void __fastcall TextFileArchive_SetSeparators(uint8_t* self, re::Edx, const char* separators) {
    Strncpy(reinterpret_cast<char*>(self + 0x15C), separators, 0xF);
    At<uint8_t>(self, 0x16B) = 0;
}

// 0x5B0A60 (thiscall, vf34): '#' comments on or off
void __fastcall TextFileArchive_SetComments(uint8_t* self, re::Edx, int on) {
    At<uint8_t>(self, 0x170) = uint8_t(on);
}

// 0x5B0A70 (ecx unused): past the white space, the text after a prefix, or null
const char* __fastcall TextFileArchive_SkipPrefix(void*, re::Edx, const char* text, const char* prefix) {
    uint32_t i = 0;
    uint32_t j = 0;
    if (IsSpace(text[0]) != 0) {
        do {
            ++i;
        } while (IsSpace(text[i]) != 0);
    }
    while (text[i] == prefix[j] && text[i] != 0) {
        ++i;
        ++j;
    }
    return prefix[j] == 0 ? text + i : nullptr;
}

// 0x5B0AE0 (thiscall, vf0): the base archive's destructor
void* __fastcall Archive_Destruct(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = 0x00922EDC;
    if ((flags & 1) != 0) {
        Free(self);
    }
    return self;
}

// 0x5B0B00: opens a text archive for reading (mode 0) or writing (1)
void* __cdecl Archive_OpenTextImpl(const char* path, const int32_t* mode, int flags) {
    Sound_Lock(0x2C);
    void* file = nullptr;
    if (*mode == 0) {
        file = File_Open(path, reinterpret_cast<const char*>(0x008FF548), 1);   // "r"
    } else if (*mode == 1) {
        file = File_Open(path, reinterpret_cast<const char*>(0x00901B00), 1);   // "w"
    }
    void* archive = nullptr;
    if (file != nullptr) {
        void* memory = New(0x174);
        if (memory != nullptr) {
            archive = TextFileArchive_Construct(memory, file, mode, path, flags);
        }
    }
    Sound_Unlock();
    return archive;
}

// 0x5B0B90: a text archive over a file from a sector (2048 bytes)
void* __cdecl Archive_OpenAtSectorImpl(void* file, int32_t sector, int) {
    Sound_Lock(0x2C);
    File_Seek(file, sector << 11, 0);
    uint8_t* archive = nullptr;
    if (file != nullptr) {
        archive = static_cast<uint8_t*>(New(0x174));
        if (archive != nullptr) {
            At<int32_t>(archive, 8) = 0;
            At<int32_t>(archive, 0x10) = 0;
            At<uint8_t>(archive, 0x14) = 0;
            At<uint8_t>(archive, 0x114) = 0;
            At<uint8_t>(archive, 0x115) = 0;
            At<int32_t>(archive, 0x158) = 0;
            At<uint8_t>(archive, 0x15C) = 0;
            At<int32_t>(archive, 0x16C) = 0;
            At<uint8_t>(archive, 0x170) = 0;
            At<uint8_t>(archive, 5) = 1;
            At<uint32_t>(archive, 0) = 0x00922F64;
            At<void*>(archive, 0xC) = file;
        }
    }
    Sound_Unlock();
    return archive;
}

// 0x5B0C10: closes a text archive (the memory file's stream, or its file) and deletes it
void __cdecl Archive_CloseTextImpl(uint8_t* archive) {
    if (archive == nullptr) {
        return;
    }
    void* file = At<void*>(archive, 0xC);
    using DeleteFn = void*(__thiscall*)(void*, int);
    if (memoryTextFile != nullptr) {
        File_CloseStream(memoryTextFile, nullptr);
        Free(memoryTextData);
        memoryTextFile = nullptr;
        memoryTextData = nullptr;
        re::PtrCast<DeleteFn>(VtableOf(archive)[0])(archive, 1);
        return;
    }
    File_Close(file);
    re::PtrCast<DeleteFn>(VtableOf(archive)[0])(archive, 1);
}

// one character from the memory text ({file, data, size, position}): false at the end
bool NextCharacter(uint8_t* text, char* c) {
    if (At<void*>(text, 0) != nullptr) {
        return File_ReadItems(c, 1, 1, At<void*>(text, 0)) != 0;
    }
    if (At<char*>(text, 4) == nullptr) {
        return false;
    }
    *c = At<char*>(text, 4)[At<int32_t>(text, 0xC)];
    At<int32_t>(text, 0xC) += 1;
    return true;
}

// 0x5B0C70: a line (CR, LF or CRLF) from the memory text: its length, 0 when it fills the
// buffer or ends without one
int __cdecl MemoryText_ReadLineImpl(uint8_t* text, char* buffer, int size) {
    int length = 0;
    if (size <= 0) {
        return 0;
    }
    for (;;) {
        if (text == nullptr) {
            return 0;
        }
        char c;
        if (!NextCharacter(text, &c)) {
            return 0;
        }
        if (c == '\r') {
            char next;
            if (NextCharacter(text, &next) && next != '\n') {
                if (At<void*>(text, 0) != nullptr) {
                    File_Seek(At<void*>(text, 0), -1, 1);
                } else if (At<void*>(text, 4) != nullptr) {
                    int32_t position = At<int32_t>(text, 0xC) - 1;
                    if (position >= 0 && position <= At<int32_t>(text, 8)) {
                        At<int32_t>(text, 0xC) = position;
                    }
                }
            }
            buffer[length] = 0;
            return length;
        }
        if (c == '\n') {
            buffer[length] = 0;
            return length;
        }
        buffer[length++] = c;
        if (length >= size) {
            return 0;
        }
    }
}

// 0x5B0D60 (thiscall, vf37): the next token ({buffer, size}): separators skipped before it,
// '#' comments to the line's end (when on); false at the end of the input
uint8_t __fastcall TextFileArchive_ReadToken(uint8_t* self, re::Edx, uint8_t* token) {
    char* buffer = At<char*>(token, 0);
    uint32_t size = At<uint32_t>(token, 4);
    char single[2];
    single[1] = 0;
    Strncpy(buffer, reinterpret_cast<const char*>(0x008FEFBB), size);   // ""
    int state = 1;   // 0 in a token, 1 before one, 2 in a comment
    for (;;) {
        int32_t position = At<int32_t>(self, 0x16C);
        if (position >= 0x100 || At<char>(self, 0x14 + position) == 0) {
            char* line = reinterpret_cast<char*>(self + 0x14);
            int read = memoryTextFile != nullptr ? MemoryText_ReadLine(memoryTextFile, line, 0x100)
                                                 : File_Read(At<void*>(self, 0xC), line, 0x100);
            At<int32_t>(self, 0x16C) = 0;
            if (read == 0) {
                return 0;
            }
            if (read < 0x100) {
                line[read] = 0;
            }
        }
        char c = At<char>(self, 0x14 + At<int32_t>(self, 0x16C));
        At<int32_t>(self, 0x16C) += 1;
        single[0] = c;
        const char* separators = reinterpret_cast<const char*>(self + 0x15C);
        if (state == 0) {
            if (Strchr(separators, c) != nullptr) {
                return 1;
            }
            uint32_t used = 0;
            while (buffer[used] != 0) {
                ++used;
            }
            StrNCat(buffer, single, size - used);
        } else if (state == 1) {
            if (Strchr(separators, c) != nullptr) {
                continue;
            }
            if (c == '#' && At<uint8_t>(self, 0x170) != 0) {
                state = 2;
                continue;
            }
            state = 0;
            uint32_t used = 0;
            while (buffer[used] != 0) {
                ++used;
            }
            StrNCat(buffer, single, size - used);
        } else if (state == 2) {
            if (c == '\r' || c == '\n') {
                state = 1;
            }
        }
    }
}

// the next token into a 0x33-byte buffer (through vf 0x94)
bool NextToken(uint8_t* self, char* text) {
    struct {
        re::Ptr32<char> buffer;
        uint32_t size;
    } token = {text, 0x33};
    text[0] = 0;
    using TokenFn = uint8_t(__thiscall*)(void*, void*);
    return re::PtrCast<TokenFn>(VtableOf(self)[0x94 / 4])(self, &token) != 0;
}

// 0x5B0EE0 (thiscall, vf36): a float token
uint8_t __fastcall TextFileArchive_ReadFloat(uint8_t* self, re::Edx, float* out) {
    char text[0x34];
    if (!NextToken(self, text)) {
        return 0;
    }
    return Sscanf(text, reinterpret_cast<const char*>(0x008FFF7C), out) == 1 ? 1 : 0;   // "%f"
}

// 0x5B0F40 (thiscall, vf35): an int token
uint8_t __fastcall TextFileArchive_ReadInt(uint8_t* self, re::Edx, int32_t* out) {
    char text[0x34];
    if (!NextToken(self, text)) {
        return 0;
    }
    return Sscanf(text, reinterpret_cast<const char*>(0x008FFF78), out) == 1 ? 1 : 0;   // "%d"
}

// 0x5B0FA0 (thiscall): writes the indent, the text and a newline
void __fastcall TextFileArchive_WriteLineImpl(uint8_t* self, re::Edx, const char* text) {
    const char* indent = reinterpret_cast<const char*>(self + 0x115);
    int length = 0;
    while (indent[length] != 0) {
        ++length;
    }
    File_Write(At<void*>(self, 0xC), indent, length);
    length = 0;
    while (text[length] != 0) {
        ++length;
    }
    File_Write(At<void*>(self, 0xC), text, length);
    File_Write(At<void*>(self, 0xC), reinterpret_cast<const char*>(0x00917810), 1);   // "\n"
}

// 0x5B1000 (thiscall, vf25): writes "<name> TRUE|FALSE"
void __fastcall TextFileArchive_WriteBool(uint8_t* self, re::Edx, int value, const char* name) {
    char line[0x100];
    const char* text = reinterpret_cast<const char*>(uint8_t(value) != 0 ? 0x00923004 : 0x008FFF70);
    Sprintf(line, reinterpret_cast<const char*>(0x00922FFC), name, text);   // "%s %s"
    TextFileArchive_WriteLine(self, line);
}

// ---- registration ---------------------------------------------------------------------------

#define TFARCHIVE_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(tfarchive_original_, __LINE__);                                     \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(tfarchive_original_, __LINE__), \
             re::Mode::Original)

TFARCHIVE_REVERSED("Archive::ReadSection", 0x005B06C0, &Archive_ReadSection);
TFARCHIVE_REVERSED("Archive::WriteSection", 0x005B0730, &Archive_WriteSection);
TFARCHIVE_REVERSED("Archive::ReadNamedSection", 0x005B0760, &Archive_ReadNamedSection);
TFARCHIVE_REVERSED("Archive::Open", 0x005B07E0, &Archive_Open);
TFARCHIVE_REVERSED("Archive::OpenFromFile", 0x005B0840, &Archive_OpenFromFile);
TFARCHIVE_REVERSED("Archive::Close", 0x005B0860, &Archive_Close);
TFARCHIVE_REVERSED("Archive::Release", 0x005B0880, &Archive_Release);
TFARCHIVE_REVERSED("Archive::LoadToBuffer", 0x005B08A0, &Archive_LoadToBuffer);
TFARCHIVE_REVERSED("Archive::FreeBuffer", 0x005B08E0, &Archive_FreeBuffer);
TFARCHIVE_REVERSED("Archive::Delete", 0x005B0900, &Archive_DeleteImpl);
TFARCHIVE_REVERSED("TextFileArchive::TextFileArchive", 0x005B0920, &TextFileArchive_ConstructImpl);
TFARCHIVE_REVERSED("TextFileArchive::Vf12", 0x005B0970, &TextFileArchive_Vf12);
TFARCHIVE_REVERSED("TextFileArchive::Vf11", 0x005B0990, &TextFileArchive_Vf11);
TFARCHIVE_REVERSED("TextFileArchive::Vf22", 0x005B09B0, &TextFileArchive_Vf22);
TFARCHIVE_REVERSED("TextFileArchive::Vf21", 0x005B09D0, &TextFileArchive_Vf21);
TFARCHIVE_REVERSED("TextFileArchive::ReadByte", 0x005B09F0, &TextFileArchive_ReadByte);
TFARCHIVE_REVERSED("TextFileArchive::ReadShort", 0x005B0A10, &TextFileArchive_ReadShort);
TFARCHIVE_REVERSED("TextFileArchive::SetSeparators", 0x005B0A30, &TextFileArchive_SetSeparators);
TFARCHIVE_REVERSED("TextFileArchive::SetComments", 0x005B0A60, &TextFileArchive_SetComments);
TFARCHIVE_REVERSED("TextFileArchive::SkipPrefix", 0x005B0A70, &TextFileArchive_SkipPrefix);
TFARCHIVE_REVERSED("Archive::~Archive", 0x005B0AE0, &Archive_Destruct);
TFARCHIVE_REVERSED("Archive::OpenText", 0x005B0B00, &Archive_OpenTextImpl);
TFARCHIVE_REVERSED("Archive::OpenAtSector", 0x005B0B90, &Archive_OpenAtSectorImpl);
TFARCHIVE_REVERSED("Archive::CloseText", 0x005B0C10, &Archive_CloseTextImpl);
TFARCHIVE_REVERSED("MemoryText::ReadLine", 0x005B0C70, &MemoryText_ReadLineImpl);
TFARCHIVE_REVERSED("TextFileArchive::ReadToken", 0x005B0D60, &TextFileArchive_ReadToken);
TFARCHIVE_REVERSED("TextFileArchive::ReadFloat", 0x005B0EE0, &TextFileArchive_ReadFloat);
TFARCHIVE_REVERSED("TextFileArchive::ReadInt", 0x005B0F40, &TextFileArchive_ReadInt);
TFARCHIVE_REVERSED("TextFileArchive::WriteLine", 0x005B0FA0, &TextFileArchive_WriteLineImpl);
TFARCHIVE_REVERSED("TextFileArchive::WriteBool", 0x005B1000, &TextFileArchive_WriteBool);

} // namespace
