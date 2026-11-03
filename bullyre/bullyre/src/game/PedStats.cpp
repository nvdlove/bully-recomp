// CPedStats (Config\Dat\PEDSTATS.DAT, one 0x11C-byte entry a line, ms_pPedStats at
// 0xC2C084) and the ped type data (Config\Dat\PED.DAT, 15 entries at 0xC2C048):
// 0x499CD0 .. 0x49A390.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace pedstats {

// A PEDSTATS.DAT line: a name, then up to 64 values (ints; some columns are names
// turned into ids: models, weapons, missions)
struct CPedStats {
    char name[0x18];      // +0x0
    uint32_t nameHash;    // +0x18 (-1 when made)
    int32_t values[0x40]; // +0x1C

    CPedStats* Construct();
    void Load(char* line);
};
static_assert(offsetof(CPedStats, nameHash) == 0x18);
static_assert(offsetof(CPedStats, values) == 0x1C);
static_assert(sizeof(CPedStats) == 0x11C);

struct CPedStatsList {
    re::Ptr32<CPedStats> stats;  // +0x0
    int32_t count;     // +0x4

    CPedStats* Find(const char* name);
    void Load();
    void Reload();
};
static_assert(sizeof(CPedStatsList) == 8);

// A PED.DAT entry (the columns are divided by 50 where they're distances)
struct CPedTypeData {
    float values[5];   // +0x0 (-1 when made; 0, 1, 3, 4 then set to 0)
    int32_t value14;   // +0x14
    int32_t value18;   // +0x18
};
static_assert(sizeof(CPedTypeData) == 0x1C);

} // namespace pedstats

using pedstats::CPedStats;
using pedstats::CPedStatsList;
using pedstats::CPedTypeData;

namespace {
GAME_VAR(CPedStatsList*, ms_pPedStats, 0x00C2C084);
GAME_ARRAY(CPedTypeData*, ms_apPedTypeData, 0x00C2C048);   // 15
constexpr int NUM_PED_TYPE_DATA = 15;

GAME_FN(void*, __cdecl, PS_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, PS_Free, 0x005EEAB0, void* p);
inline void* __cdecl PS_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
template <typename... A>
inline int __cdecl PS_Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline int __cdecl PS_Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint32_t, __cdecl, PS_HashName, 0x00576ED0, const char* name);
GAME_FN(int, __cdecl, PS_GetModelIndex, 0x0051C330, const char* name);
GAME_FN(int, __cdecl, PS_GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(int, __cdecl, PS_Fn488B50, 0x00488B50, const char* name);
// a mission name's key (4 bytes; its destructor is the empty 0x49A650)
GAME_FN(void, __thiscall, PS_NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __thiscall, PS_NameKey_Destruct, 0x0049A650, uint32_t* key);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_FN(int, __thiscall, PS_MissionMgr_Find, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(void, __cdecl, PS_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(int, __cdecl, PS_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, PS_OpenFile, 0x0042D260, const char* name, const char* mode, int);
GAME_FN(char*, __cdecl, PS_LoadLine, 0x00429C10, void* file);
GAME_FN(uint8_t, __cdecl, PS_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(int, __cdecl, PS_CloseFile, 0x0042D450, void* file);
GAME_FN(void, __stdcall, PS_ConstructArray, 0x00401240, void* p, size_t size, int count, void* ctor);
// (ours below)
GAME_FN(void, __thiscall, PS_Load, 0x00499ED0, CPedStats* stats, char* line);
GAME_FN(void, __thiscall, PS_ListLoad, 0x0049A1F0, CPedStatsList* list);
// PED.DAT's parse (0x499CF0, the rest of what Ghidra shows as 0x49A140)
GAME_FN(void, __cdecl, PS_LoadPedTypeData, 0x00499CF0);
GAME_FN(int32_t, __cdecl, PS_LoadFile, 0x0042D180, const char* path, void* buffer, int size, const char* mode);
GAME_FN(int, __cdecl, PS_PedTypeFromName, 0x00488B10, const char* name);

inline const char* const readMode = reinterpret_cast<const char*>(0x008FF548);
} // namespace

namespace pedstats {

// 0x499EA0 (thiscall; the array constructor's)
CPedStats* CPedStats::Construct() {
    PS_Memset(values, 0, sizeof(values));
    nameHash = 0xFFFFFFFF;
    return this;
}

// 0x499ED0 (thiscall): a line (its tokens separated by spaces)
void CPedStats::Load(char* line) {
    char text[32];
    PS_Sscanf(line, "%s", name);
    nameHash = PS_HashName(name);
    char* p = line;
    for (int column = 0; column < 0x40; column++) {
        // (past the token before, then the spaces)
        while (*p && *p != ' ') {
            p++;
        }
        while (*p == ' ') {
            p++;
        }
        switch (column) {
        case 0: {
            PS_Sscanf(p, "%s", text);
            int model = -1;
            if (memcmp(text, "none", 5) != 0) {
                model = PS_GetModelIndex(text);
            }
            values[0] = model;
            break;
        }
        case 0x11:
            PS_Sscanf(p, "%s", text);
            PS_Fn488B50(text);   // (called twice; the first result unused)
            values[0x11] = PS_Fn488B50(text);
            break;
        case 0x28:
        case 0x2D:
        case 0x31:
        case 0x35:
        case 0x39:
            PS_Sscanf(p, "%s", text);
            values[column] = PS_GetWeaponIndex(text);
            break;
        case 0x29:
        case 0x2A:
        case 0x2B: {
            PS_Sscanf(p, "%s", text);
            int model = PS_GetModelIndex(text);
            values[column] = model != -1 ? model : 0;
            break;
        }
        case 0x30:
        case 0x34:
        case 0x38:
        case 0x3C:
            PS_Sscanf(p, "%s", text);
            if (PS_Stricmp(text, "init") != 0) {
                uint32_t key;
                PS_NameKey_Construct(&key, text);
                int mission = PS_MissionMgr_Find(missionMgr, &key);
                PS_NameKey_Destruct(&key);
                values[column] = mission;
            } else {
                values[column] = -1;
            }
            break;
        default:
            PS_Sscanf(p, "%d", &values[column]);
            break;
        }
    }
}

// 0x49A0F0 (thiscall): by name (nullptr: none)
CPedStats* CPedStatsList::Find(const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(name, stats[i].name) == 0) {
            return &stats[i];
        }
    }
    return nullptr;
}

// 0x49A1F0 (thiscall): counts the lines (onto `count`), makes the entries, reads them
void CPedStatsList::Load() {
    PS_PushMemoryTag(0x1F);
    void* file = PS_OpenFile("Config\\Dat\\PEDSTATS.DAT", readMode, 1);
    for (char* line = PS_LoadLine(file); line; line = PS_LoadLine(file)) {
        if (*line != '\0' && *line != '#') {
            count += 1;
        }
    }
    int n = count;
    uint64_t bytes = uint64_t(uint32_t(n)) * sizeof(CPedStats);
    void* memory = PS_Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : size_t(bytes));
    if (memory) {
        PS_ConstructArray(memory, sizeof(CPedStats), n, reinterpret_cast<void*>(0x00499EA0));
    } else {
        memory = nullptr;
    }
    stats = static_cast<CPedStats*>(memory);
    PS_Seek(file, 0, 0);
    int offset = 0;
    for (char* line = PS_LoadLine(file); line; line = PS_LoadLine(file)) {
        if (*line != '\0' && *line != '#') {
            PS_Load(reinterpret_cast<CPedStats*>(re::PtrCast<uint8_t*>(stats) + offset), line);
            offset += sizeof(CPedStats);
        }
    }
    PS_CloseFile(file);
    PS_PopMemoryTag();
}

// 0x49A2D0 (thiscall): the file again, into the entries there are
void CPedStatsList::Reload() {
    PS_PushMemoryTag(0x1F);
    void* file = PS_OpenFile("Config\\Dat\\PEDSTATS.DAT", readMode, 1);
    int offset = 0;
    for (char* line = PS_LoadLine(file); line; line = PS_LoadLine(file)) {
        if (*line != '\0' && *line != '#') {
            PS_Load(reinterpret_cast<CPedStats*>(re::PtrCast<uint8_t*>(stats) + offset), line);
            offset += sizeof(CPedStats);
        }
    }
    PS_CloseFile(file);
    PS_PopMemoryTag();
}

} // namespace pedstats

namespace {

// 0x499CD0: the ped type data freed (the pointers left)
void __cdecl PedTypeData_Shutdown() {
    for (int i = 0; i < NUM_PED_TYPE_DATA; i++) {
        PS_Free(ms_apPedTypeData[i]);
    }
}

// 0x499CF0: Config\Dat\PED.DAT (read whole, at most 0x4000 bytes): a line at a time (\r, ','
// and tabs made spaces; blank lines and '#' comments skipped, the blanks before the first
// character any byte up to ' ', signed), "name a b c d e f g": the type's data, its first three
// divided by 50
void __cdecl PedTypeData_Load() {
    auto* text = static_cast<char*>(PS_Malloc(0x4000));
    int32_t size = PS_LoadFile(reinterpret_cast<const char*>(0x00911A8C), text, 0x4000, readMode);
    if (size > 0) {
        int32_t at = 0;
        do {
            char line[0x100];
            int32_t n = 0;
            char c = text[at];
            if (c != '\n') {
                do {
                    if (at >= size) {
                        break;
                    }
                    line[n] = (c == '\r' || c == ',' || c == '\t') ? ' ' : c;
                    c = text[at + 1];
                    at++;
                    n++;
                } while (c != '\n');
            }
            at++;
            line[n] = 0;
            int32_t first = 0;
            while (int8_t(line[first]) <= ' ') {
                first++;
            }
            if (first == n || line[first] == '#') {
                continue;
            }
            char name[0x20];
            float a = 0.0f, b = 0.0f, c3 = 0.0f, d = 0.0f, e = 0.0f;
            int32_t f = 0, g = 0;
            PS_Sscanf(line, "%s", name);
            PS_Sscanf(line, "%s %f %f %f %f %f %d %d", name, &a, &b, &c3, &d, &e, &f, &g);
            int type = PS_PedTypeFromName(name);
            CPedTypeData* data = ms_apPedTypeData[type];
            data->values[0] = float(double(a) / 50.0);
            data->values[1] = float(double(b) / 50.0);
            data->values[2] = float(double(c3) / 50.0);
            data->values[3] = d;
            data->values[4] = e;
            data->value18 = f;
            data->value14 = g;
        } while (at < size);
    }
    PS_Free(text);
}

// 0x49A140: the ped type data made, then PED.DAT read (0x499CF0)
void __cdecl PedTypeData_Initialise() {
    for (int i = 0; i < NUM_PED_TYPE_DATA; i++) {
        CPedTypeData* data = static_cast<CPedTypeData*>(PS_Malloc(sizeof(CPedTypeData)));
        if (data) {
            data->value14 = 0;
            data->values[0] = -1.0f;
            data->value18 = 0;
            data->values[1] = -1.0f;
            data->values[2] = -1.0f;
            data->values[3] = -1.0f;
            data->values[4] = -1.0f;
        } else {
            data = nullptr;
        }
        ms_apPedTypeData[i] = data;
        // (through nullptr when the allocation failed)
        ms_apPedTypeData[i]->values[0] = 0.0f;
        ms_apPedTypeData[i]->values[1] = 0.0f;
        ms_apPedTypeData[i]->values[3] = 0.0f;
        ms_apPedTypeData[i]->values[4] = 0.0f;
    }
    PS_LoadPedTypeData();
}

// 0x49A1B0
void __cdecl PedStats_Shutdown() {
    CPedStatsList* list = ms_pPedStats;
    if (list) {
        if (list->stats) {
            PS_Free(list->stats);
            list->stats = nullptr;
        }
        PS_Free(list);
        ms_pPedStats = nullptr;
    }
}

// 0x49A340: the list made (once) and loaded
void __cdecl PedStats_Initialise() {
    PS_PushMemoryTag(0x1F);
    if (!ms_pPedStats) {
        CPedStatsList* list = static_cast<CPedStatsList*>(PS_Malloc(sizeof(CPedStatsList)));
        if (list) {
            list->stats = nullptr;
            list->count = 0;
            ms_pPedStats = list;
            PS_ListLoad(list);
            PS_PopMemoryTag();
            return;
        }
        ms_pPedStats = nullptr;
    }
    PS_ListLoad(ms_pPedStats);
    PS_PopMemoryTag();
}

} // namespace

#define C08P4_REVERSED(name, address, fn) \
    REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)
#define C08P4_REVERSED_STATIC(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P4_REVERSED_STATIC("CPedType::ShutdownData", 0x00499CD0, &PedTypeData_Shutdown);
C08P4_REVERSED("CPedStats::Construct", 0x00499EA0, &CPedStats::Construct);
C08P4_REVERSED("CPedStats::Load", 0x00499ED0, &CPedStats::Load);
C08P4_REVERSED("CPedStatsList::Find", 0x0049A0F0, &CPedStatsList::Find);
C08P4_REVERSED_STATIC("CPedType::LoadPedData", 0x00499CF0, &PedTypeData_Load);
C08P4_REVERSED_STATIC("CPedType::Initialise", 0x0049A140, &PedTypeData_Initialise);
C08P4_REVERSED_STATIC("CPedStats::Shutdown", 0x0049A1B0, &PedStats_Shutdown);
C08P4_REVERSED("CPedStatsList::Load", 0x0049A1F0, &CPedStatsList::Load);
C08P4_REVERSED("CPedStatsList::Reload", 0x0049A2D0, &CPedStatsList::Reload);
C08P4_REVERSED_STATIC("CPedStats::Initialise", 0x0049A340, &PedStats_Initialise);
