// A table of sounds by model: a header line of model names, then up to 67 rows of sound
// names (looked up in the sound bank at 0xB6F1E8), in four groups (the second and fourth
// for winter interiors). (The file is its own line reader; which one is 0xA14AA0's name.)

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

// the game's line file (MemoryFile::Load reads it whole)
struct LineFile {
    re::Ptr32<char> data;       // +0 (freed after)
    int32_t size;     // +4 (lines remain while +4 != +8)
    int32_t position; // +8
};
static_assert(sizeof(LineFile) == 0xC);

GAME_VAR(const char*, s_tableFileName, 0x00A14AA0);
GAME_VAR(const char*, s_noSound, 0x00A14AA4);       // a cell with this name stays -1
GAME_VAR(void*, s_soundBank, 0x00B6F1E8);
GAME_VAR(uint8_t, s_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, s_chapterSeason, 0x00A147EC);  // (2: winter)
GAME_VAR(int32_t, s_currArea, 0x00BD1008);
inline const char* const s_formatString = reinterpret_cast<const char*>(0x008FF970);  // "%s"

template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint16_t, __thiscall, SoundBank_GetId, 0x00667B60, void* bank, const char* name);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(char*, __cdecl, LineFile_NextLine, 0x0042B980, LineFile* file);
GAME_FN(uint8_t, __thiscall, LineFile_Load, 0x00426610, LineFile* file, const char* path);

struct ModelSoundTable {
    uint32_t models[50];  // +0 the header's names (hashes)
    int32_t numModels;    // +0xC8
    uint8_t parsed;       // +0xCC the header's been read (later files skip theirs)
    uint8_t unknownCD[3];
    re::Ptr32<uint16_t> sounds;     // +0xD0 [4 groups][67 rows][numModels], -1: none
    int32_t row;          // +0xD4
    int32_t unknownD8;    // +0xD8
    int32_t group;        // +0xDC

    void FreeSounds();
    void ParseHeader(const char* line);
    void ParseRow(const char* line);
    uint8_t HasModel(uint32_t model);
    uint16_t GetSound(const int* row, uint32_t model);
    uint16_t GetSound2(const int* row, uint32_t model);
    void ParseFile(LineFile* file);
    void Load();
};
static_assert(offsetof(ModelSoundTable, numModels) == 0xC8);
static_assert(offsetof(ModelSoundTable, sounds) == 0xD0);
static_assert(sizeof(ModelSoundTable) == 0xE0);

// (its own, called where they are)
GAME_FN(void, __thiscall, Table_ParseHeader, 0x00463FD0, ModelSoundTable* table, const char* line);
GAME_FN(void, __thiscall, Table_ParseRow, 0x00464190, ModelSoundTable* table, const char* line);
GAME_FN(void, __thiscall, Table_ParseFile, 0x004643B0, ModelSoundTable* table, LineFile* file);

// 0x463FA0 (thiscall)
void ModelSoundTable::FreeSounds() {
    if (sounds) {
        Free(sounds);
        sounds = nullptr;
    }
}

// 0x463FD0 (thiscall): the first line: a label, then the model names. (Each name is
// found one past the last: single separators only)
void ModelSoundTable::ParseHeader(const char* line) {
    char name[32];
    if (parsed) {
        const char* p = line;
        Sscanf(p, s_formatString, name);
        for (int i = 0; i < numModels; i++) {
            p += strlen(name) + 1;
            Sscanf(p, s_formatString, name);
            HashName(name);
        }
        return;
    }
    parsed = 1;
    Sscanf(line, s_formatString, name);
    int length = int(strlen(line));
    int n = 0;
    const char* p = line + strlen(name) + 1;
    if (Sscanf(p, s_formatString, name) > 0) {
        do {
            p += strlen(name) + 1;
            models[n] = HashName(name);
            n++;
            if (p - line >= length) {
                break;
            }
        } while (Sscanf(p, s_formatString, name) > 0);
    }
    numModels = n;
    uint32_t cells = uint32_t(n) * 0x10C;
    // (new[]'s overflow check: -1 when the byte count doesn't fit)
    sounds = static_cast<uint16_t*>(Malloc(cells > 0x7FFFFFFFu ? size_t(-1) : size_t(cells) * 2));
    for (int base = 0; base < 0x10C; base += 0x43) {
        for (int r = 0; r < 0x43; r++) {
            for (int m = 0; m < numModels; m++) {
                sounds[(base + r) * numModels + m] = 0xFFFF;
            }
        }
    }
}

// 0x464190 (thiscall): a row: its label, then a sound name per model
void ModelSoundTable::ParseRow(const char* line) {
    char name[32];
    unknownD8 = 0;
    const char* p = line;
    Sscanf(p, s_formatString, name);
    for (int m = 0; m < numModels; m++) {
        p += strlen(name) + 1;
        Sscanf(p, s_formatString, name);
        if (strcmp(name, s_noSound) != 0) {
            sounds[(group * 0x43 + row) * numModels + m] = SoundBank_GetId(s_soundBank, name);
        }
    }
    row++;
}

// 0x464280 (thiscall)
uint8_t ModelSoundTable::HasModel(uint32_t model) {
    for (int i = 0; i < numModels; i++) {
        if (models[i] == model) {
            return 1;
        }
    }
    return 0;
}

// 0x4642B0 (thiscall): a row's sound for a model (group 0, 1 in a winter interior). (A
// model not in the table reads the cell before the row's)
uint16_t ModelSoundTable::GetSound(const int* rowIndex, uint32_t model) {
    int column = -1;
    for (int i = 0; i < numModels; i++) {
        if (models[i] == model) {
            column = i;
            break;
        }
    }
    int g = 0;
    if (s_chapterSeason[s_chapter] == 2 && IsAreaInterior(s_currArea)) {
        g = 1;
    }
    return sounds[(g * 0x43 + *rowIndex) * numModels + column];
}

// 0x464330 (thiscall): the same from groups 2 and 3
uint16_t ModelSoundTable::GetSound2(const int* rowIndex, uint32_t model) {
    int column = -1;
    for (int i = 0; i < numModels; i++) {
        if (models[i] == model) {
            column = i;
            break;
        }
    }
    int g = 2;
    if (s_chapterSeason[s_chapter] == 2 && IsAreaInterior(s_currArea)) {
        g = 3;
    }
    return sounds[(g * 0x43 + *rowIndex) * numModels + column];
}

// 0x4643B0 (thiscall): the file's lines into the current group (blank and '#' lines
// skipped; 67 rows at most)
void ModelSoundTable::ParseFile(LineFile* file) {
    row = 0;
    bool header = false;
    while (file->size != file->position) {
        char* line = LineFile_NextLine(file);
        if (!line || row >= 0x43) {
            return;
        }
        if (line[0] != '\0' && line[0] != '#') {
            if (header) {
                Table_ParseRow(this, line);
            } else {
                Table_ParseHeader(this, line);
                header = true;
            }
        }
    }
}

// 0x464420 (thiscall): the file into the four groups. (It's read once: the later
// passes find it at its end)
void ModelSoundTable::Load() {
    row = 0;
    unknownD8 = 0;
    LineFile file{nullptr, 0, 0};
    LineFile_Load(&file, s_tableFileName);
    for (int g = 0; g < 4; g++) {
        group = g;
        Table_ParseFile(this, &file);
    }
    if (file.data) {
        Free(file.data);
    }
}

} // namespace

#define MST_C05_REVERSED(name, address, fn) REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)

MST_C05_REVERSED("ModelSoundTable::FreeSounds", 0x00463FA0, &ModelSoundTable::FreeSounds);
MST_C05_REVERSED("ModelSoundTable::ParseHeader", 0x00463FD0, &ModelSoundTable::ParseHeader);
MST_C05_REVERSED("ModelSoundTable::ParseRow", 0x00464190, &ModelSoundTable::ParseRow);
MST_C05_REVERSED("ModelSoundTable::HasModel", 0x00464280, &ModelSoundTable::HasModel);
MST_C05_REVERSED("ModelSoundTable::GetSound", 0x004642B0, &ModelSoundTable::GetSound);
MST_C05_REVERSED("ModelSoundTable::GetSound2", 0x00464330, &ModelSoundTable::GetSound2);
MST_C05_REVERSED("ModelSoundTable::ParseFile", 0x004643B0, &ModelSoundTable::ParseFile);
MST_C05_REVERSED("ModelSoundTable::Load", 0x00464420, &ModelSoundTable::Load);
