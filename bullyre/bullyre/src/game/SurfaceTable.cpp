// The surfaces (0x45B250..0x45B750): the six surface groups' adhesion table (loaded
// from a text file, symmetric), the weather's and winter's grip factors, the surface
// hardness names, and the name table reader the hardness file is read with.

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// A table of name hashes read from a text file: a row a line, the columns 0x80 apart
struct NameTable {
    int32_t rows;       // +0
    int32_t columns;    // +4 (the first line's)
    re::Ptr32<uint32_t> hashes;   // +8 [column * 0x80 + row]
};
static_assert(sizeof(NameTable) == 0xC);

GAME_ARRAY(float, st_adhesion, 0x00C1A8F8);         // [6][6]
GAME_VAR(NameTable, st_hardnessTable, 0x00C1A988);
GAME_ARRAY(char, st_fileBuffer, 0x00BD7800);         // 0x1C000 bytes
GAME_ARRAY(const char*, st_hardnessNames, 0x0090AD04);
GAME_VAR(int32_t, st_currArea, 0x00BD1008);
GAME_VAR(uint8_t, st_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, st_chapterSeasons, 0x00A147EC);  // 2: winter
GAME_VAR(float, st_wetness, 0x00CE7564);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(char*, __cdecl, File_ReadLine, 0x00429C10, void* file);
GAME_FN(void, __cdecl, File_Close, 0x0042D450, void* file);
GAME_FN(void, __cdecl, File_LoadText, 0x0042D180, const char* path, char* buffer, int size, const char* mode);
GAME_FN(uint8_t, __cdecl, Area_IsOutside, 0x00430320, int area);
GAME_FN(const char*, __cdecl, Hardness_GetName, 0x0045B250, uint32_t hardness);
GAME_FN(int32_t, __cdecl, Surface_GetGroup, 0x0045B310, int surface);
GAME_FN(void, __thiscall, NameTable_Load, 0x0045B750, NameTable* self, const char* path);
GAME_FN(void, __thiscall, NameTable_Free, 0x0045B670, NameTable* self);
GAME_FN(void, __thiscall, NameTable_ParseLine, 0x0045B690, NameTable* self, const char* line);

const char* const formatS = reinterpret_cast<const char*>(0x008FF970);   // "%s"
const char* const formatF = reinterpret_cast<const char*>(0x008FFF7C);   // "%f"
const char* const modeR = reinterpret_cast<const char*>(0x008FF548);     // "r"

// ---- the name table ------------------------------------------------------------------------

// 0x45B670 (thiscall)
void __fastcall NameTable_FreeImpl(NameTable* self) {
    Free(self->hashes);
    self->hashes = nullptr;
    self->rows = 0;
    self->columns = 0;
}

// 0x45B690 (thiscall): a line's words hashed into the next row
void __fastcall NameTable_ParseLineImpl(NameTable* self, re::Edx, const char* line) {
    int32_t length = int32_t(std::strlen(line));
    int32_t used = 0;
    int32_t count = 0;
    char word[32];
    if (Sscanf(line, formatS, word) > 0) {
        int column = 0;
        while (used < length) {
            int32_t n = int32_t(std::strlen(word)) + 1;
            line += n;
            used += n;
            uint32_t hash = HashName(word);
            count++;
            self->hashes[self->rows + column] = hash;
            column += 0x80;
            if (Sscanf(line, formatS, word) <= 0) {
                break;
            }
        }
    }
    self->rows++;
    if (self->columns == 0) {
        self->columns = count;
    }
}

// 0x45B750 (thiscall): the file's lines (not empty, not '#' comments)
void __fastcall NameTable_LoadImpl(NameTable* self, re::Edx, const char* path) {
    self->hashes = static_cast<uint32_t*>(Malloc(0x4000));
    void* file = File_Open(path, modeR, 1);
    for (char* line = File_ReadLine(file); line; line = File_ReadLine(file)) {
        if (line[0] != '\0' && line[0] != '#') {
            NameTable_ParseLine(self, line);
        }
    }
    File_Close(file);
}

// ---- hardness --------------------------------------------------------------------------------

// 0x45B250 (cdecl): a hardness's name ("INVALID" past 3)
const char* __cdecl Hardness_GetNameImpl(uint32_t hardness) {
    if (hardness <= 3) {
        return st_hardnessNames[hardness];
    }
    return reinterpret_cast<const char*>(0x00903168);   // "INVALID"
}

// 0x45B270 (thiscall): each of 0x43 surfaces' hardness (-1 unknown), from the second
// column of the file
void __fastcall Hardness_Load(int32_t* self, re::Edx, const char* path) {
    NameTable_Load(&st_hardnessTable, path);
    for (int offset = 0x204; offset < 0x310; offset += 4) {
        uint32_t hash = *reinterpret_cast<const uint32_t*>(re::PtrCast<const uint8_t*>(st_hardnessTable.hashes) + offset);
        int32_t hardness = -1;
        for (int k = 0; k < 4; k++) {
            if (hash == HashName(Hardness_GetName(uint32_t(k)))) {
                hardness = k;
            }
        }
        self[(offset - 0x204) / 4] = hardness;
    }
    NameTable_Free(&st_hardnessTable);
    reinterpret_cast<uint8_t*>(self)[0x10C] = 1;
}

// 0x45B2F0 (thiscall): a surface's hardness, into *out (eax: out, which callers read)
int32_t* __fastcall Hardness_Get(const int32_t* self, re::Edx, int32_t* out, const int32_t* surface) {
    *out = self[*surface];
    return out;
}

// ---- adhesion ------------------------------------------------------------------------------------

// 0x45B310 (cdecl): a surface's adhesion group
int32_t __cdecl Surface_GetGroupImpl(int surface) {
    switch (uint8_t(surface)) {
    case 0x1A: case 0x26: case 0x30:
        return 0;
    case 0x05: case 0x0E: case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
    case 0x17: case 0x18: case 0x19: case 0x27: case 0x2E: case 0x2F: case 0x34: case 0x35:
        return 1;
    case 0x03: case 0x04: case 0x2B: case 0x2D: case 0x31:
        return 3;
    case 0x1B: case 0x36:
        return 4;
    case 0x1E:
        return 5;
    default:
        return 2;
    }
}

// 0x45B3A0 (cdecl): the adhesion table, a lower triangle (a name, then the row's values;
// '-' for 0; ';' comments), mirrored. (No end-of-text checks: the file must be complete.)
void __cdecl Adhesion_Load(const char* path) {
    File_LoadText(path, st_fileBuffer, 0x1C000, modeR);
    const char* p = st_fileBuffer;
    int row = 0;
    int rowBase = 0;
    float* column = st_adhesion;
    char name[256];
    for (;;) {
        char c = *p;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            p++;
            continue;
        }
        if (c == ';') {
            while (c != '\n' && c != '\r') {
                c = *++p;
            }
            continue;
        }
        Sscanf(p, formatS, name);
        c = *p;
        while (c != ' ' && c != '\t' && c != ',') {
            c = *++p;
        }
        float* cell = column;
        for (int i = 0; i <= row; i++) {
            while (*p == ' ' || *p == '\t' || *p == ',') {
                p++;
            }
            float value = 0.0f;
            if (*p != '-') {
                Sscanf(p, formatF, &value);
            }
            c = *p;
            while (c != ' ' && c != '\t' && c != ',' && c != '\n') {
                c = *++p;
            }
            st_adhesion[rowBase + i] = value;
            *cell = value;
            cell += 6;
        }
        rowBase += 6;
        column++;
        row++;
        if (column >= st_adhesion + 6) {
            return;
        }
    }
}

// 0x45B500 (cdecl): a colpoint's adhesion, by its two surfaces (+0x22, +0x20)
float __cdecl Adhesion_Get(const uint8_t* colPoint) {
    int32_t a = Surface_GetGroup(colPoint[0x22]);
    int32_t b = Surface_GetGroup(colPoint[0x20]);
    return st_adhesion[a * 6 + b];
}

// 0x45B530 (cdecl): the grip factor outdoors: winter's (0.78 slippery surfaces, 0.86
// the rest), else the wetness's
float __cdecl Surface_GetWeatherGrip(int surface) {
    if (!Area_IsOutside(st_currArea)) {
        return 1.0f;
    }
    uint32_t s = uint8_t(surface);
    if (st_chapterSeasons[st_chapter] == 2) {
        switch (s) {
        case 0x00: case 0x01: case 0x06: case 0x07: case 0x0E: case 0x0F: case 0x10: case 0x11: case 0x12:
        case 0x13: case 0x14: case 0x15: case 0x17: case 0x18: case 0x19: case 0x1A: case 0x1C: case 0x20:
        case 0x21: case 0x22: case 0x26: case 0x27: case 0x2D: case 0x2E: case 0x2F: case 0x30: case 0x31:
        case 0x34: case 0x35: case 0x37:
            return *reinterpret_cast<const float*>(0x0090AD28);   // 0.78
        default:
            return *reinterpret_cast<const float*>(0x0090AD2C);   // 0.86
        }
    }
    switch (s) {
    case 0x03: case 0x0E: case 0x0F: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x17:
    case 0x18: case 0x19: case 0x1A: case 0x26: case 0x27: case 0x2D: case 0x2E: case 0x2F: case 0x30:
    case 0x35:
        return float(1.0 - st_wetness * 0.1);
    case 0x00: case 0x01: case 0x05: case 0x06: case 0x07: case 0x10: case 0x20: case 0x21: case 0x22:
    case 0x2B: case 0x31: case 0x34: case 0x37:
        return float(1.0 - st_wetness * 0.05);
    case 0x1B: case 0x36:
        return float(st_wetness * 0.5 + 1.0);
    default:
        return 1.0f;
    }
}

#define C04E_SURF(name, address, fn)                                                           \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04E_SURF("Hardness::GetName", 0x0045B250, &Hardness_GetNameImpl);
C04E_SURF("Hardness::Load", 0x0045B270, &Hardness_Load);
C04E_SURF("Hardness::Get", 0x0045B2F0, &Hardness_Get);
C04E_SURF("Surface::GetGroup", 0x0045B310, &Surface_GetGroupImpl);
C04E_SURF("Adhesion::Load", 0x0045B3A0, &Adhesion_Load);
C04E_SURF("Adhesion::Get", 0x0045B500, &Adhesion_Get);
C04E_SURF("Surface::GetWeatherGrip", 0x0045B530, &Surface_GetWeatherGrip);
C04E_SURF("NameTable::Free", 0x0045B670, &NameTable_FreeImpl);
C04E_SURF("NameTable::ParseLine", 0x0045B690, &NameTable_ParseLineImpl);
C04E_SURF("NameTable::Load", 0x0045B750, &NameTable_LoadImpl);

} // namespace
