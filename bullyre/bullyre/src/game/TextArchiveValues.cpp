// TextFileArchive's value readers and writers (0x5B1050..0x5B1DB0): the text form of
// the game's archives ("name value" lines, blocks between BEGIN and END, indented with
// tabs). A reader takes (out, name): the next line must start with the name (after
// whitespace), else the line is kept for the next read (pushedBack) and it returns 0;
// `out` may be null to skip the value. The writers take (value, name).
//
// All thiscall. The rest of the class (0x5B0970..0x5B1000) is another pass's.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// The part of TextFileArchive these use
struct TextArchive {
    re::Ptr32<void> vtable;          // +0
    uint8_t unknown4[8];   // +4
    int32_t file;          // +0xC
    uint8_t unknown10[4];  // +0x10
    char line[256];        // +0x14 the line last read
    uint8_t pushedBack;    // +0x114 the line is to be read again
    char indent[0x43];     // +0x115 tabs, written before each line
    int32_t depth;         // +0x158 how many tabs
};
static_assert(offsetof(TextArchive, file) == 0xC);
static_assert(offsetof(TextArchive, line) == 0x14);
static_assert(offsetof(TextArchive, pushedBack) == 0x114);
static_assert(offsetof(TextArchive, indent) == 0x115);
static_assert(offsetof(TextArchive, depth) == 0x158);

template <typename... A>
inline int __cdecl TA_sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
template <typename... A>
inline int __cdecl TA_sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline int __cdecl TA_isspace(int c) { return crt::Isspace(c); }
GAME_FN(uint8_t, __cdecl, TA_FileReadLine, 0x0042D330, int file, char* buffer, int size);
// reads a line from the memory reader (when there is one): its length, 0 at the end
GAME_FN(int, __cdecl, TA_MemReadLine, 0x005B0C70, void* reader, char* buffer, int size);
GAME_FN(void, __thiscall, TA_SetUnity, 0x00412770, void* matrix);
// 0x5B0A70: the value after `name` at the start of the line (after whitespace), else null
GAME_FN(char*, __thiscall, TA_FindValue, 0x005B0A70, TextArchive* self, char* line, const char* name);
// 0x5B0FA0: writes indent + text + a newline
GAME_FN(void, __thiscall, TA_WriteLine, 0x005B0FA0, TextArchive* self, const char* text);
GAME_FN(char*, __thiscall, TA_ReadLine, 0x005B14E0, TextArchive* self);

// Reading from memory instead of the file, when set
GAME_VAR(void*, g_archiveMemReader, 0x00CFAD4C);

void Indent(TextArchive* self) {
    self->indent[self->depth] = '\t';
    self->depth++;
    self->indent[self->depth] = '\0';
}

void Unindent(TextArchive* self) {
    self->depth--;
    self->indent[self->depth] = '\0';
}

// The line after `name`, or null (the line then pushed back)
char* ReadValue(TextArchive* self, const char* name) {
    char* line = TA_ReadLine(self);
    char* value = TA_FindValue(self, line, name);
    if (value == nullptr) {
        self->pushedBack = 1;
    }
    return value;
}

// ---- writers

void __fastcall WriteInt(TextArchive* self, re::Edx, int value, const char* name) {
    char buffer[256];
    TA_sprintf(buffer, "%s %d", name, value);
    TA_WriteLine(self, buffer);
}

void __fastcall WriteUInt(TextArchive* self, re::Edx, uint32_t value, const char* name) {
    char buffer[256];
    TA_sprintf(buffer, "%s %u", name, value);
    TA_WriteLine(self, buffer);
}

void __fastcall WriteFloat(TextArchive* self, re::Edx, float value, const char* name) {
    char buffer[256];
    TA_sprintf(buffer, "%s %f", name, double(value));
    TA_WriteLine(self, buffer);
}

// (value: a string object whose first field is the text)
void __fastcall WriteString(TextArchive* self, re::Edx, const re::Ptr32<const char>* value, const char* name) {
    char buffer[256];
    TA_sprintf(buffer, "%s \"%s\"", name, *value);
    TA_WriteLine(self, buffer);
}

void __fastcall WriteVector(TextArchive* self, re::Edx, const float* v, const char* name) {
    char buffer[256];
    TA_sprintf(buffer, "%s %f, %f, %f", name, double(v[0]), double(v[1]), double(v[2]));
    TA_WriteLine(self, buffer);
}

// (a vector written as ints, truncated: _ftol)
void __fastcall WriteVectorInt(TextArchive* self, re::Edx, const float* v, const char* name) {
    char buffer[256];
    int z = int(v[2]);
    int y = int(v[1]);
    int x = int(v[0]);
    TA_sprintf(buffer, "%s %d, %d, %d", name, x, y, z);
    TA_WriteLine(self, buffer);
}

// A matrix: its name, then the four rows indented
void __fastcall WriteMatrix(TextArchive* self, re::Edx, const float* m, const char* name) {
    char buffer[316];
    TA_sprintf(buffer, "%s", name);
    TA_WriteLine(self, buffer);
    Indent(self);
    TA_sprintf(buffer, "%f, %f, %f,", double(m[0]), double(m[1]), double(m[2]));
    TA_WriteLine(self, buffer);
    TA_sprintf(buffer, "%f, %f, %f,", double(m[4]), double(m[5]), double(m[6]));
    TA_WriteLine(self, buffer);
    TA_sprintf(buffer, "%f, %f, %f,", double(m[8]), double(m[9]), double(m[10]));
    TA_WriteLine(self, buffer);
    TA_sprintf(buffer, "%f, %f, %f", double(m[12]), double(m[13]), double(m[14]));
    TA_WriteLine(self, buffer);
    Unindent(self);
}

// (four arguments, all unused: writes its uninitialised stack buffer as a line; zeroed
// here)
void __fastcall WriteNothing(TextArchive* self, re::Edx, int, int, int, int) {
    char buffer[256];
    buffer[0] = '\0';
    TA_WriteLine(self, buffer);
}

void __fastcall BeginBlock(TextArchive* self, re::Edx, const char* name) {
    char buffer[256];
    TA_sprintf(buffer, "%s", name);
    TA_WriteLine(self, buffer);
    Indent(self);
    TA_sprintf(buffer, "%s", "BEGIN");
    TA_WriteLine(self, buffer);
    Indent(self);
}

void __fastcall EndBlock(TextArchive* self, re::Edx) {
    char buffer[256];
    Unindent(self);
    TA_sprintf(buffer, "%s", "END");
    TA_WriteLine(self, buffer);
    Unindent(self);
}

// ---- readers

// The next line (the pushed-back one again if set); empty at the end
char* __fastcall ReadLine(TextArchive* self, re::Edx) {
    if (self->pushedBack == 0) {
        uint8_t ok;
        if (g_archiveMemReader == nullptr) {
            ok = TA_FileReadLine(self->file, self->line, 0x100);
        } else {
            ok = TA_MemReadLine(g_archiveMemReader, self->line, 0x100) != 0;
        }
        if (ok == 0) {
            self->line[0] = '\0';
        }
    }
    self->pushedBack = 0;
    return self->line;
}

// (0/F/f false, 1/T/t true, else left false)
uint8_t __fastcall ReadBool(TextArchive* self, re::Edx, uint8_t* out, const char* name) {
    if (out != nullptr) {
        *out = 0;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        char word[8];
        TA_sscanf(value, "%5s", word);
        switch (word[0]) {
        case '0':
        case 'F':
        case 'f':
            *out = 0;
            break;
        case '1':
        case 'T':
        case 't':
            *out = 1;
            break;
        }
    }
    return value != nullptr;
}

uint8_t __fastcall ReadInt(TextArchive* self, re::Edx, int32_t* out, const char* name) {
    if (out != nullptr) {
        *out = 0;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%d", out);
    }
    return value != nullptr;
}

uint8_t __fastcall ReadUInt(TextArchive* self, re::Edx, uint32_t* out, const char* name) {
    if (out != nullptr) {
        *out = 0;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%u", out);
    }
    return value != nullptr;
}

uint8_t __fastcall ReadFloat(TextArchive* self, re::Edx, float* out, const char* name) {
    if (out != nullptr) {
        *out = 0.0f;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%f", out);
    }
    return value != nullptr;
}

// A quoted string into (*out)[0..size-1] (out: a string object whose first field is
// the buffer). (Skips the whitespace and one character, the opening quote; copies up to
// the closing quote, which must be there)
uint8_t __fastcall ReadString(TextArchive* self, re::Edx, re::Ptr32<char>* out, int size, const char* name) {
    char* buffer = out != nullptr ? *out : nullptr;
    if (buffer != nullptr) {
        *buffer = '\0';
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && buffer != nullptr) {
        int i = 0;
        while (TA_isspace(value[i]) != 0) {
            i++;
        }
        const char* p = value + i + 1;
        int n = 0;
        while (*p != '"') {
            if (n < size - 1) {
                buffer[n] = *p;
                n++;
            }
            p++;
        }
        buffer[n] = '\0';
    }
    return value != nullptr;
}

uint8_t __fastcall ReadVector(TextArchive* self, re::Edx, float* out, const char* name) {
    if (out != nullptr) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%f, %f, %f", &out[0], &out[1], &out[2]);
    }
    return value != nullptr;
}

// A colour as 0..255 floats, scaled to 0..1 (alpha, +0xC, stays 0). (The scaling reads
// through `out` even when it is null)
uint8_t __fastcall ReadColourFloat(TextArchive* self, re::Edx, float* out, const char* name) {
    if (out != nullptr) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        out[3] = 0.0f;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%f, %f, %f", &out[0], &out[1], &out[2]);
    }
    out[0] = float(double(out[0]) / 255.0);
    out[1] = float(double(out[1]) / 255.0);
    out[2] = float(double(out[2]) / 255.0);
    return value != nullptr;
}

// A matrix: set to identity, then the four rows from the next four lines. (Each row
// after the first is copied as 16 bytes from a stack temporary whose fourth float is
// uninitialised: +0x1C, +0x2C and +0x3C get stack garbage. Left alone here)
uint8_t __fastcall ReadMatrix(TextArchive* self, re::Edx, float* m, const char* name) {
    if (m != nullptr) {
        TA_SetUnity(m);
    }
    char* line = TA_ReadLine(self);
    uint8_t found = TA_FindValue(self, line, name) != nullptr;
    if (!found) {
        self->pushedBack = 1;
        return found;
    }
    if (m == nullptr) {
        TA_ReadLine(self);
        TA_ReadLine(self);
        TA_ReadLine(self);
        TA_ReadLine(self);
        return found;
    }
    float x = 0.0f, y = 0.0f, z = 0.0f;
    TA_sscanf(TA_ReadLine(self), "%f, %f, %f", &x, &y, &z);
    m[0] = x;
    m[1] = y;
    m[2] = z;
    for (int row = 1; row < 4; row++) {
        TA_sscanf(TA_ReadLine(self), "%f, %f, %f", &x, &y, &z);
        m[row * 4 + 0] = x;
        m[row * 4 + 1] = y;
        m[row * 4 + 2] = z;
    }
    return found;
}

// Four ints to bytes (a colour)
uint8_t __fastcall ReadColour(TextArchive* self, re::Edx, uint8_t* out, const char* name) {
    if (out != nullptr) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = 0;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        int r, g, b, a;
        TA_sscanf(value, "%d, %d, %d, %d", &r, &g, &b, &a);
        out[0] = uint8_t(r);
        out[3] = uint8_t(a);
        out[1] = uint8_t(g);
        out[2] = uint8_t(b);
    }
    return value != nullptr;
}

uint8_t __fastcall ReadShort4(TextArchive* self, re::Edx, int16_t* out, const char* name) {
    if (out != nullptr) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = 0;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%hd, %hd, %hd, %hd", &out[0], &out[1], &out[2], &out[3]);
    }
    return value != nullptr;
}

uint8_t __fastcall ReadShort2(TextArchive* self, re::Edx, int16_t* out, const char* name) {
    if (out != nullptr) {
        out[0] = 0;
        out[1] = 0;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%hd, %hd", &out[0], &out[1]);
    }
    return value != nullptr;
}

// An int, -1 by default (the two middle arguments unused)
uint8_t __fastcall ReadIndex(TextArchive* self, re::Edx, int32_t* out, int, int, const char* name) {
    if (out != nullptr) {
        *out = -1;
    }
    char* value = ReadValue(self, name);
    if (value != nullptr && out != nullptr) {
        TA_sscanf(value, "%d", out);
    }
    return value != nullptr;
}

// The line with the name, then the next (skipped)
uint8_t __fastcall SkipValue(TextArchive* self, re::Edx, const char* name) {
    char* line = TA_ReadLine(self);
    if (TA_FindValue(self, line, name) != nullptr) {
        TA_ReadLine(self);
        return 1;
    }
    self->pushedBack = 1;
    return 0;
}

// Skips lines up to the END matching the first BEGIN met (the END's line then pushed
// back)
void __fastcall SkipBlock(TextArchive* self, re::Edx) {
    int depth = 0;
    char* line = self->line;
    for (;;) {
        if (self->pushedBack == 0) {
            uint8_t ok;
            if (g_archiveMemReader == nullptr) {
                ok = TA_FileReadLine(self->file, line, 0x100);
            } else {
                ok = TA_MemReadLine(g_archiveMemReader, line, 0x100) != 0;
            }
            if (ok == 0) {
                *line = '\0';
            }
        }
        self->pushedBack = 0;
        if (TA_FindValue(self, line, "BEGIN") != nullptr) {
            depth++;
        }
        if (TA_FindValue(self, line, "END") != nullptr) {
            depth--;
            if (depth < 0) {
                break;
            }
        }
    }
    self->pushedBack = 1;
}

} // namespace

#define TEXTARCHIVE_REVERSED(name, address, fn)                                     \
    static void* RE_CONCAT(Original_TA_, address);                                  \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &RE_CONCAT(Original_TA_, address), \
             re::Mode::Original)

TEXTARCHIVE_REVERSED("TextFileArchive::WriteInt_5B1050", 0x005B1050, WriteInt);
TEXTARCHIVE_REVERSED("TextFileArchive::WriteUInt_5B1090", 0x005B1090, WriteUInt);
TEXTARCHIVE_REVERSED("TextFileArchive::WriteFloat_5B10D0", 0x005B10D0, WriteFloat);
TEXTARCHIVE_REVERSED("TextFileArchive::WriteString_5B1110", 0x005B1110, WriteString);
TEXTARCHIVE_REVERSED("TextFileArchive::WriteVector_5B1150", 0x005B1150, WriteVector);
TEXTARCHIVE_REVERSED("TextFileArchive::WriteVectorInt_5B11B0", 0x005B11B0, WriteVectorInt);
TEXTARCHIVE_REVERSED("TextFileArchive::WriteMatrix_5B1210", 0x005B1210, WriteMatrix);
TEXTARCHIVE_REVERSED("TextFileArchive::WriteNothing_5B13C0", 0x005B13C0, WriteNothing);
TEXTARCHIVE_REVERSED("TextFileArchive::BeginBlock_5B13E0", 0x005B13E0, BeginBlock);
TEXTARCHIVE_REVERSED("TextFileArchive::EndBlock_5B1480", 0x005B1480, EndBlock);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadLine_5B14E0", 0x005B14E0, ReadLine);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadBool_5B1530", 0x005B1530, ReadBool);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadInt_5B1620", 0x005B1620, ReadInt);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadUInt_5B1680", 0x005B1680, ReadUInt);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadFloat_5B16E0", 0x005B16E0, ReadFloat);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadString_5B1740", 0x005B1740, ReadString);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadVector_5B1800", 0x005B1800, ReadVector);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadColourFloat_5B1890", 0x005B1890, ReadColourFloat);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadMatrix_5B1910", 0x005B1910, ReadMatrix);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadColour_5B1AF0", 0x005B1AF0, ReadColour);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadShort4_5B1B90", 0x005B1B90, ReadShort4);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadShort2_5B1C10", 0x005B1C10, ReadShort2);
TEXTARCHIVE_REVERSED("TextFileArchive::ReadIndex_5B1C80", 0x005B1C80, ReadIndex);
TEXTARCHIVE_REVERSED("TextFileArchive::SkipValue_5B1CE0", 0x005B1CE0, SkipValue);
TEXTARCHIVE_REVERSED("TextFileArchive::SkipBlock_5B1D20", 0x005B1D20, SkipBlock);
// vf30: a lone jmp to ReadLine
RE_UNHOOKED("TextFileArchive::ReadLineThunk_5B1DB0", 0x005B1DB0);
