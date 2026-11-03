// The sound banks (cSCREAMBankManager, 0xCF5DC4 = TheSounds+0x2024): the bank list
// (0x48 bytes each: the name first, +0x34 flags, +0x38 the type), the trigger banks
// (audio\CONFIG\trigbank.txt, per area), the area bank matrix (areabank.txt), the
// missions' banks (missbank.txt), the weather's (weather.txt), the weapons' banks and
// the load queue. 0x597DE0..0x599210.
//
// The manager: +4, +8 the banks, +0xC (0x100 bytes), +0x110 the queue's count, +0x114 the
// queue (a bank index and flags each), +0x18C the queue's first is a load, +0x198/+0x19C
// the missions, +0x1A0 twenty slots, +0x244 a sound, +0x250 the weather's banks (4
// seasons x 6 weathers: a sound id, a bank), +0x310 a sound, +0x314 the area loaded,
// +0x31C/+0x320/+0x324 the area matrix (a bit per area and bank).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct LineFile {
    re::Ptr32<char> data;     // +0 (freed after)
    int32_t left;   // +4
    int32_t end;    // +8
};
static_assert(sizeof(LineFile) == 0xC);

// a trigger's bank (8 bytes)
struct TriggerBank {
    int16_t bank;        // +0
    uint8_t flags;       // +2 (bit 0)
    uint8_t pad3;
    uint32_t hash;       // +4 the trigger
};
static_assert(sizeof(TriggerBank) == 8);

// the areas' lists (0xCF5AA4, 64 of 0xC bytes; +1 and +8 are the audio environments')
struct AreaLists {
    uint8_t bankCount;       // +0
    uint8_t reverbCount;     // +1
    uint8_t pad2[2];
    re::Ptr32<TriggerBank> banks;      // +4
    re::Ptr32<void> reverbs;           // +8
};
static_assert(sizeof(AreaLists) == 0xC);

// a mission's banks (0x30 bytes)
struct MissionBanks {
    int32_t mission;         // +0
    uint16_t loadCount;      // +4
    uint16_t unloadCount;    // +6
    int16_t load[10];        // +8
    int16_t unload[10];      // +0x1C
};
static_assert(sizeof(MissionBanks) == 0x30);

// a trigger bank line before the lists are built (0xC bytes; the scratch buffer)
struct TriggerBankLine {
    int16_t bank;            // +0
    uint8_t unknown2[2];     // (the scratch buffer's: copied along)
    uint32_t hash;           // +4
    int32_t area;            // +8
};
static_assert(sizeof(TriggerBankLine) == 0xC);

// a bit matrix (the area banks': +0x31C in the manager)
struct BitMatrix {
    int32_t width;    // +0
    int32_t height;   // +4
    re::Ptr32<uint8_t> bits;    // +8
};

// the weapons' banks by model (compared in this order)
struct WeaponBank {
    uintptr_t model;   // the model index's variable
    uintptr_t name;    // the bank's name
};
const WeaponBank kWeaponBanks[] = {
    {0x00BF444C, 0x0091E1F8},   // weapons\slingsht.bnk
    {0x00BF44D8, 0x0091E1F8},   // weapons\slingsht.bnk
    {0x00BF4448, 0x0091E210},   // weapons\chrybmb.bnk
    {0x00BF43DC, 0x0091E210},   // weapons\chrybmb.bnk
    {0x00BF4338, 0x0091E210},   // weapons\chrybmb.bnk
    {0x00BF4444, 0x0091E224},   // weapons\spudgun.bnk
    {0x00BF443C, 0x0091E224},   // weapons\spudgun.bnk
    {0x00BF4440, 0x0091E224},   // weapons\spudgun.bnk
    {0x00BF4348, 0x0091E550},   // weapons\wtrbln.bnk
    {0x00BF4344, 0x0091E53C},   // weapons\banana.bnk
    {0x00BF4438, 0x0091E528},   // weapons\btlrokt.bnk
    {0x00BF4434, 0x0091E518},   // weapons\egg.bnk
    {0x00BF442C, 0x0091E500},   // weapons\stinkbmb.bnk
    {0x00BF4430, 0x0091E238},   // mission\camera.bnk
    {0x00BF433C, 0x0091E238},   // mission\camera.bnk
    {0x00BF44E0, 0x0091E4E8},   // weapons\spraycan.bnk
    {0x00BF4428, 0x0091E4D4},   // weapons\apple.bnk
    {0x00BF4424, 0x0091E24C},   // weapons\bat.bnk
    {0x00BF4420, 0x0091E24C},   // weapons\bat.bnk
    {0x00BF43F8, 0x0091E24C},   // weapons\bat.bnk
    {0x00BF437C, 0x0091E24C},   // weapons\bat.bnk
    {0x00BF43AC, 0x0091E24C},   // weapons\bat.bnk
    {0x00BF4378, 0x0091E24C},   // weapons\bat.bnk
    {0x00BF436C, 0x0091E4BC},   // objects_2\umbrela.bnk
    {0x00BF4368, 0x0091E4A8},   // weapons\lure02.bnk
    {0x00BF4364, 0x0091E494},   // weapons\lure01.bnk
    {0x00BF441C, 0x0091E480},   // weapons\brick.bnk
    {0x00BF4418, 0x0091E25C},   // weapons\lid.bnk
    {0x00BF43D8, 0x0091E25C},   // weapons\lid.bnk
    {0x00BF4404, 0x0091E25C},   // weapons\lid.bnk
    {0x00BF4414, 0x0091E46C},   // weapons\newsp.bnk
    {0x00BF440C, 0x0091E458},   // weapons\snoball.bnk
    {0x00BF4408, 0x0091E444},   // weapons\yardstk.bnk
    {0x00BF44D4, 0x0091E430},   // OBJECTS\FireExt.bnk
    {0x00BF4410, 0x0091E418},   // weapons\SledgHmr.bnk
    {0x00BF43F4, 0x0091E404},   // weapons\KickMe.bnk
    {0x00BF43F0, 0x0091E3EC},   // minigame\beeball.bnk
    {0x00BF43EC, 0x0091E3D8},   // minigame\soccer.bnk
    {0x00BF43E8, 0x0091E3C4},   // weapons\marbles.bnk
    {0x00BF4158, 0x0091E26C},   // OBJECTS_2\Books.bnk
    {0x00BF4154, 0x0091E26C},   // OBJECTS_2\Books.bnk
    {0x00BF4150, 0x0091E26C},   // OBJECTS_2\Books.bnk
    {0x00BF414C, 0x0091E26C},   // OBJECTS_2\Books.bnk
    {0x00BF4148, 0x0091E26C},   // OBJECTS_2\Books.bnk
    {0x00BF43C4, 0x0091E280},   // objects\plate.bnk
    {0x00BF4400, 0x0091E280},   // objects\plate.bnk
    {0x00BF43C0, 0x0091E294},   // objects\pVase.bnk
    {0x00BF43B8, 0x0091E294},   // objects\pVase.bnk
    {0x00BF43B4, 0x0091E3AC},   // minigame\snowShvl.bnk
    {0x00BF43B0, 0x0091E394},   // weapons\socrball.bnk
    {0x00BF43A8, 0x0091E380},   // objects\pedrats.bnk
    {0x00BF43A0, 0x0091E36C},   // mission\brush.bnk
    {0x00BF439C, 0x0091E358},   // objects\frisbee.bnk
    {0x00BF4398, 0x0091E340},   // objects\football.bnk
    {0x00BF4394, 0x0091E328},   // OBJECTS_3\FtBallXp.bnk
    {0x00BF4390, 0x0091E314},   // objects\tennis.bnk
    {0x00BF4388, 0x0091E300},   // weapons\pipe.bnk
    {0x00BF4384, 0x0091E2EC},   // weapons\itch.bnk
    {0x00BF4380, 0x0091E2D4},   // weapons\spraygun.bnk
    {0x00BF4374, 0x0091E2C0},   // weapons\candy.bnk
    {0x00BF432C, 0x0091E2A8},   // weapons\RbBndBal.bnk
};

inline uint8_t* const soundBanks = reinterpret_cast<uint8_t*>(0x00CF5DC4);
inline uint8_t* const theSounds = reinterpret_cast<uint8_t*>(0x00CF3DA0);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
GAME_ARRAY(AreaLists, areaLists, 0x00CF5AA4);
GAME_ARRAY(TriggerBankLine, triggerBankLines, 0x00BD7800);   // (the scratch buffer)
GAME_ARRAY(MissionBanks, missionBankLines, 0x00BD7800);      // (the same)
GAME_VAR(int32_t, numBanks, 0x00CF68F4);
GAME_VAR(uint8_t*, bankList, 0x00CF5DCC);                    // (the manager's +8)
GAME_VAR(int16_t, numBankGroups, 0x00CF68EC);
GAME_VAR(int16_t*, bankGroups, 0x00CF68F0);                  // three banks each (-1 none)
GAME_VAR(const char*, areaBankFile, 0x00AE6BD8);             // "Audio\CONFIG\areabank.txt"
GAME_VAR(const char*, missionBankFile, 0x00AE6BDC);          // "Audio\CONFIG\missbank.txt"
GAME_VAR(const char*, weatherBankFile, 0x00AE6BE0);          // "Audio\CONFIG\weather.txt"
GAME_VAR(int32_t, currArea, 0x00BD1008);

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, void* soundRef, void* sound);
inline const char* __cdecl StrRChr(const char* s, int c) { return (const char*)crt::Strrchr(s, c); }
inline int __cdecl Strnicmp(const char* a, const char* b, size_t n) { return crt::Strnicmp(a, b, n); }
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline size_t __cdecl Strcspn(const char* s, const char* set) { return crt::Strcspn(s, set); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(void, __cdecl, LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(uint8_t, __thiscall, LineFile_Load, 0x00426610, LineFile* file, const char* path);
GAME_FN(uint8_t, __cdecl, LineFile_ReadLine, 0x0042D600, LineFile* file, char* line, int size);
GAME_FN(const char*, __cdecl, GetAreaName, 0x004305B0, int area);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, Sounds_AddTrigger, 0x005960A0, void* sounds, uint32_t hash);
GAME_FN(int32_t, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(int32_t, __thiscall, MissionMgr_FindMission, 0x006A9FD0, void* mgr, uint32_t* key);
// its own
GAME_FN(void, __thiscall, Banks_ClearTriggerLists, 0x00597DE0, void* self);
GAME_FN(int, __thiscall, Banks_Find, 0x00597FC0, void* self, const char* name);
GAME_FN(void, __thiscall, Banks_RemoveQueued, 0x00598830, void* self, int index);

uint8_t BankFlags(uint8_t* self, int bank) {
    return At<uint8_t*>(self, 8)[0x34 + bank * 0x48];
}

// ---- the lists --------------------------------------------------------------------------------

// 0x597DE0 (thiscall, ecx unused): the areas' trigger bank lists freed
void __fastcall Banks_ClearTriggerListsImpl(void*) {
    for (int i = 0; i < 64; i++) {
        if (areaLists[i].banks != nullptr) {
            Free(areaLists[i].banks);
        }
        areaLists[i].bankCount = 0;
        areaLists[i].banks = nullptr;
    }
}

// 0x597E10 (thiscall): a mission's banks (null: none)
MissionBanks* __fastcall Banks_FindMission(uint8_t* self, re::Edx, int32_t mission) {
    int32_t count = At<int32_t>(self, 0x198);
    MissionBanks* missions = At<MissionBanks*>(self, 0x19C);
    for (int i = 0; i < count; i++) {
        if (missions[i].mission == mission) {
            return &missions[i];
        }
    }
    return nullptr;
}

// 0x597E60 (thiscall): the matrix and the sounds freed
void __fastcall Banks_Release(uint8_t* self) {
    if (At<void*>(self, 0x324) != nullptr) {
        Free(At<void*>(self, 0x324));
    }
    At<void*>(self, 0x324) = nullptr;
    Sound_Set(self + 0x310, nullptr);
    Sound_Set(self + 0x244, nullptr);
}

// 0x597EA0 (thiscall): two banks of the same group (both flag 8): *order -1 or 1 as b
// comes before or after a in it
uint8_t __fastcall Banks_SameGroup(uint8_t* self, re::Edx, int a16, int b16, int16_t* order) {
    int16_t a = int16_t(a16);
    int16_t b = int16_t(b16);
    *order = 0;
    if ((BankFlags(self, a) & 8) == 0 || (BankFlags(self, b) & 8) == 0) {
        return a == b ? 1 : 0;
    }
    if (a == b) {
        return 1;
    }
    int16_t foundA = -1;
    int16_t foundB = -1;
    int32_t indexA = -1;
    int32_t indexB = -1;
    int16_t* group = bankGroups;
    for (int g = 0; g < numBankGroups; g++) {
        for (int i = 0; i < 3; i++, group++) {
            uint16_t v = uint16_t(*group);
            if (v == 0xFFFF) {
                continue;
            }
            if (v == uint16_t(a)) {
                indexA = uint16_t(i);
                foundA = int16_t(v);
            }
            if (v == uint16_t(b)) {
                foundB = int16_t(v);
                indexB = uint16_t(i);
            }
        }
        if (foundA != -1) {
            if (foundB == -1) {
                return 0;
            }
            *order = int16_t(int16_t(indexA) > int16_t(indexB) ? 1 : -1);
            return 1;
        }
        if (foundB != foundA) {
            return 0;
        }
    }
    return 0;
}

// 0x597FC0 (thiscall, ecx unused): a bank's index by its file's name (-1: none)
int __fastcall Banks_FindImpl(void*, re::Edx, const char* name) {
    if (name[0] == '\0') {
        return -1;
    }
    char buffer[512];
    std::strcpy(buffer, name);
    const char* slash = StrRChr(buffer, '\\');
    if (slash == nullptr) {
        slash = StrRChr(buffer, '/');
    }
    if (slash != nullptr) {
        std::memmove(buffer, slash + 1, std::strlen(slash + 1) + 1);
    }
    size_t length = std::strlen(buffer);
    size_t n = length == 0 ? 1 : length - 4;   // (without ".bnk")
    for (int i = 0; i < numBanks; i++) {
        if (Strnicmp(buffer, reinterpret_cast<const char*>(bankList + i * 0x48), n) == 0) {
            return i;
        }
    }
    return -1;
}

// 0x5980C0 (thiscall): a bank's position (0..2) in its group when a bank before it has
// flag 1 (else the bank itself)
int16_t __fastcall Banks_GroupSlot(uint8_t* self, re::Edx, int bank16) {
    int16_t bank = int16_t(bank16);
    uint8_t* banks = At<uint8_t*>(self, 8);
    if (banks == nullptr || (banks[0x34 + bank * 0x48] & 8) == 0) {
        return bank;
    }
    int16_t* group = bankGroups;
    for (int g = 0; g < numBankGroups; g++, group += 3) {
        int16_t first = -1;
        for (int i = 0; i < 3; i++) {
            int16_t v = group[i];
            if (v == -1) {
                continue;
            }
            if (first == -1 && (banks[0x34 + v * 0x48] & 1) != 0) {
                first = int16_t(i);
            }
            if (v == bank) {
                return first != -1 ? first : bank;
            }
        }
    }
    return bank;
}

// 0x598180 (thiscall, ecx unused): a weapon model's bank (-1: none)
int __fastcall Banks_ForWeapon(void*, re::Edx, int32_t model) {
    if (model < 0) {
        return -1;
    }
    for (const WeaponBank& w : kWeaponBanks) {
        if (model == *reinterpret_cast<const int32_t*>(w.model)) {
            return Banks_Find(soundBanks, reinterpret_cast<const char*>(w.name));
        }
    }
    return -1;
}

// 0x5986E0 (thiscall, ecx unused): a bank type's name
int __fastcall Banks_ParseType(void*, re::Edx, const char* name) {
    if (Stricmp(name, "STATIC") == 0) {
        return 1;
    }
    if (Stricmp(name, "AREA") == 0) {
        return 3;
    }
    if (Stricmp(name, "TRIGGER") == 0) {
        return 4;
    }
    if (Stricmp(name, "MISSION") == 0) {
        return 2;
    }
    if (Stricmp(name, "MISSIONFORCELOAD") == 0) {
        return 7;
    }
    if (Stricmp(name, "MANUAL") == 0) {
        return 5;
    }
    return Stricmp(name, "AREAFORCELOAD") == 0 ? 6 : 0;
}

// 0x5987B0 (thiscall): a bank's file name without its path (and the length of its part
// before the digits). (With no path the first character is skipped too)
void __fastcall Banks_GetFileName(uint8_t* self, re::Edx, int bank16, re::Ptr32<const char>* name, int16_t* length) {
    int16_t bank = int16_t(bank16);
    const char* full = reinterpret_cast<const char*>(At<uint8_t*>(self, 8) + bank * 0x48);
    const char* back = StrRChr(full, '\\');
    const char* forward = StrRChr(reinterpret_cast<const char*>(At<uint8_t*>(self, 8) + bank * 0x48), '/');
    const char* p;
    if (back == nullptr && forward == nullptr) {
        p = reinterpret_cast<const char*>(At<uint8_t*>(self, 8) + bank * 0x48);
    } else {
        p = forward > back ? forward : back;
    }
    p++;
    if (length != nullptr) {
        *length = int16_t(Strcspn(p, "0123456789."));
        *name = p;
        return;
    }
    *name = p;
}

// 0x598830 (thiscall): a queue entry removed
void __fastcall Banks_RemoveQueuedImpl(uint8_t* self, re::Edx, int index) {
    int32_t& count = At<int32_t>(self, 0x110);
    count--;
    if (index < count) {
        std::memmove(self + 0x114 + index * 4, self + 0x118 + index * 4, (count - index) * 4);
    }
    if (index == 0) {
        if (count != 0) {
            self[0x18C] = self[0x116] & 1;
            return;
        }
        self[0x18C] = 0;
    }
}

// 0x5988A0 (thiscall): a bank's queue entries removed but the first. (A bank that is
// the first entry loops forever: the game's)
void __fastcall Banks_Unqueue(uint8_t* self, re::Edx, int bank) {
    int i = 0;
    while (i < At<int32_t>(self, 0x110)) {
        if (int32_t(At<int16_t>(self, 0x114 + i * 4)) == bank) {
            if (i != 0) {
                Banks_RemoveQueued(self, i);
            }
        } else {
            i++;
        }
    }
}

// 0x598920 (thiscall): a bit matrix (width x height) made
void __fastcall BitMatrix_Init(BitMatrix* self, re::Edx, int32_t width, int32_t height) {
    PushMemoryTag(0x2E);
    size_t size = size_t((width * height) / 8 + 1);
    self->bits = static_cast<uint8_t*>(Malloc(size));
    PopMemoryTag();
    std::memset(self->bits, 0, size);
    self->width = width;
    self->height = height;
}

// 0x598970 (thiscall): the constructor
uint8_t* __fastcall Banks_Construct(uint8_t* self) {
    for (int i = 0; i < 20; i++) {
        uint8_t* slot = self + 0x1A0 + i * 8;
        slot[2] &= 0xFE;
        At<int32_t>(slot, 4) = 0;
    }
    At<int32_t>(self, 0x244) = 0;
    for (int i = 0; i < 24; i++) {
        At<int32_t>(self, 0x250 + i * 8) = 0;
        At<int16_t>(self, 0x254 + i * 8) = -1;
    }
    At<int32_t>(self, 0x310) = 0;
    At<int32_t>(self, 0x31C) = 0;
    At<int32_t>(self, 0x324) = 0;
    At<int32_t>(self, 0x110) = 0;
    At<int32_t>(self, 0x194) = 0;
    At<int32_t>(self, 0x190) = 0;
    At<int32_t>(self, 0x19C) = 0;
    At<int32_t>(self, 0x198) = 0;
    self[0x18C] = 0;
    At<int32_t>(self, 8) = 0;
    At<int32_t>(self, 0x24C) = -1;
    At<int32_t>(self, 0x248) = -1;
    Sound_Set(self + 0x244, nullptr);
    Sound_Set(self + 0x310, nullptr);
    self[4] = 0;
    At<int32_t>(self, 0x314) = 0x40;
    std::memset(self + 0xC, 0, 0x100);
    return self;
}

// 0x598A50 (thiscall): the area matrix's bit for the current area
uint32_t __fastcall Banks_IsAreaBank(uint8_t* self, re::Edx, int bank) {
    int32_t bit = At<int32_t>(self, 0x31C) * currArea + bank;
    uint8_t byte = At<uint8_t*>(self, 0x324)[bit / 8];
    int32_t shift = bit % 8;
    uint8_t mask = uint8_t(1u << (shift & 31));
    return (byte & mask) != 0 ? 1 : 0;
}

// ---- the files ---------------------------------------------------------------------------------

// 0x598AA0 (thiscall): trigbank.txt ("trigger area bank"): each area's trigger banks.
// (A line naming no known area goes to the last one found, -1 before any; an unknown
// bank ends the reading and nothing is built)
void __fastcall Banks_LoadTriggerBankFile(uint8_t* self) {
    int numLines = 0;
    int area = -1;
    LoadingScreen_Message("cSCREAMBankManager::LoadTriggerBankFile()", "ClearBankTriggerList()", 0);
    Banks_ClearTriggerLists(self);
    LineFile file{nullptr, 0, 0};
    char line[0x100];
    char areaName[0x80];
    char trigger[0x100];
    char bankName[0x100];
    LineFile_Load(&file, "audio\\CONFIG\\trigbank.txt");
    if (LineFile_ReadLine(&file, line, 0x100) != 0) {
        TriggerBankLine* out = triggerBankLines;
        do {
            if (line[0] == '#' || std::strlen(line) < 8) {
                continue;
            }
            Sscanf(line, "%s\t%s\t%s", trigger, areaName, bankName);
            uint32_t hash = HashName(trigger);
            for (int a = 0; a < 0x40; a++) {
                if (std::strcmp(areaName, GetAreaName(a)) == 0) {
                    area = a;
                    break;
                }
            }
            int bank = Banks_Find(self, bankName);
            if (bank == -1) {
                if (file.data != nullptr) {
                    Free(file.data);
                }
                return;
            }
            out->area = area;
            out->hash = hash;
            Sounds_AddTrigger(theSounds, hash);
            numLines++;
            areaLists[area].bankCount++;
            out->bank = int16_t(bank);
            out++;
        } while (LineFile_ReadLine(&file, line, 0x100) != 0);
    }
    for (int a = 0; a < 64; a++) {
        AreaLists& lists = areaLists[a];
        uint8_t count = lists.bankCount;
        if (count == 0) {
            continue;
        }
        TriggerBank* banks = static_cast<TriggerBank*>(Malloc(count * sizeof(TriggerBank)));
        if (banks != nullptr) {
            for (int j = count - 1; j >= 0; j--) {
                banks[j].flags &= 0xFE;
                banks[j].hash = 0;
                banks[j].bank = -1;
            }
        }
        lists.banks = banks;
        uint32_t filled = 0;
        for (int j = 0; j < numLines; j++) {
            if (triggerBankLines[j].area != a) {
                continue;
            }
            std::memcpy(&lists.banks[filled], &triggerBankLines[j], 8);
            filled++;
            if (filled == lists.bankCount) {
                break;
            }
        }
    }
    if (file.data != nullptr) {
        Free(file.data);
    }
}

// 0x598CF0 (thiscall): weather.txt ("season weather bank sound")
void __fastcall Banks_LoadWeatherBankFile(uint8_t* self) {
    // (only the first four weathers of each season are cleared)
    for (int season = 0; season < 4; season++) {
        for (int weather = 0; weather < 4; weather++) {
            At<int16_t>(self, 0x254 + (season * 6 + weather) * 8) = -1;
        }
    }
    LoadingScreen_Message("cSCREAMBankManager::LoadWeatherBankFile()", "Open file", 0);
    LineFile file{nullptr, 0, 0};
    char line[0x200];
    char season[0x200];
    char weather[0x200];
    char bankName[0x200];
    char sound[0x200];
    LineFile_Load(&file, weatherBankFile);
    while (LineFile_ReadLine(&file, line, 0x100) != 0) {
        Sscanf(line, "%s\t%s\t%s\t%s\n", season, weather, bankName, sound);
        if (line[0] == '#' || std::strlen(line) < 7) {
            continue;
        }
        int s;
        if (Stricmp(season, "summer") == 0) {
            s = 0;
        } else if (Stricmp(season, "fall") == 0) {
            s = 1;
        } else if (Stricmp(season, "winter") == 0) {
            s = 2;
        } else if (Stricmp(season, "spring") == 0) {
            s = 3;
        } else {
            continue;
        }
        int w;
        if (Stricmp(weather, "sunny") == 0) {
            w = 0;
        } else if (Stricmp(weather, "cloudy") == 0) {
            w = 1;
        } else if (Stricmp(weather, "rainy") == 0) {
            w = 2;
        } else if (Stricmp(weather, "foggy") == 0) {
            w = 3;
        } else if (Stricmp(weather, "ExtraSunny") == 0) {
            w = 4;
        } else if (Stricmp(weather, "hurricane") == 0) {
            w = 5;
        } else {
            continue;
        }
        int bank = Banks_Find(self, bankName);
        if (bank == -1) {
            continue;   // ("WARNING: Sound bank %s not found while parsing %s")
        }
        int index = w + s * 6;
        At<int16_t>(self, 0x254 + index * 8) = int16_t(bank);
        At<int32_t>(self, 0x250 + index * 8) = Sounds_GetId(theSounds, sound);
    }
    if (file.data != nullptr) {
        Free(file.data);
    }
}

// 0x598FB0 (thiscall): the trigger's banks of the loaded area all loaded
uint8_t __fastcall Banks_TriggerBanksLoaded(uint8_t* self, re::Edx, uint32_t hash) {
    if (At<int32_t>(self, 0x314) != currArea) {
        return 0;
    }
    int count = areaLists[currArea].bankCount;
    if (count <= 0) {
        return 1;
    }
    TriggerBank* banks = areaLists[currArea].banks;
    for (int i = 0; i < count; i++) {
        if (banks[i].hash == hash && (BankFlags(self, banks[i].bank) & 1) == 0) {
            return 0;
        }
    }
    return 1;
}

// 0x599030 (thiscall): areabank.txt ("area bank"): the area matrix. (An unknown bank ends
// the reading)
void __fastcall Banks_LoadAreaBankFile(uint8_t* self) {
    LoadingScreen_Message("cSCREAMBankManager::LoadAreaBankFile()", "m_AreaBankMatrix.Init()", 0);
    int32_t n = numBanks;
    PushMemoryTag(0x2E);
    size_t size = size_t((n * 0x41) / 8 + 1);
    At<void*>(self, 0x324) = Malloc(size);
    PopMemoryTag();
    std::memset(At<void*>(self, 0x324), 0, size);
    At<int32_t>(self, 0x31C) = n;
    At<int32_t>(self, 0x320) = 0x41;
    LineFile file{nullptr, 0, 0};
    char line[0x200];
    char areaName[0x200];
    char bankName[0x200];
    LineFile_Load(&file, areaBankFile);
    if (LineFile_ReadLine(&file, line, 0x100) != 0) {
        do {
            Sscanf(line, "%s\t%s\n", areaName, bankName);
            if (line[0] == '#' || std::strlen(line) < 7) {
                continue;
            }
            int area = 0;
            for (; area < 0x40; area++) {
                if (Stricmp(areaName, GetAreaName(area)) == 0) {
                    break;
                }
            }
            if (area == 0x40) {
                continue;   // ("Bad Area Name in areabank.txt. Area %s not found!")
            }
            int bank = Banks_Find(self, bankName);
            if (bank == -1) {
                break;
            }
            int32_t bit = At<int32_t>(self, 0x31C) * area + bank;
            uint8_t* byte = At<uint8_t*>(self, 0x324) + bit / 8;
            int32_t shift = bit % 8;
            *byte |= uint8_t(1u << (shift & 31));
        } while (LineFile_ReadLine(&file, line, 0x100) != 0);
    }
    if (file.data != nullptr) {
        Free(file.data);
    }
}

// 0x599210 (thiscall): missbank.txt ("mission bank", "-bank": a static bank unloaded
// for it)
void __fastcall Banks_LoadMissionBankFile(uint8_t* self) {
    LoadingScreen_Message("cSCREAMBankManager::LoadMissionBankFile()", "WorkBufferLock()", 0);
    At<int32_t>(self, 0x198) = 0;
    LineFile file{nullptr, 0, 0};
    char line[0x200];
    char mission[0x200];
    char bankName[0x200];
    LineFile_Load(&file, missionBankFile);
    if (LineFile_ReadLine(&file, line, 0x100) != 0) {
        do {
            Sscanf(line, "%s\t%s\n", mission, bankName);
            if (line[0] == '#' || std::strlen(line) < 7) {
                continue;
            }
            uint32_t key;
            NameKey_Construct(&key, mission);
            int32_t m = MissionMgr_FindMission(missionMgr, &key);
            if (m == -1) {
                continue;
            }
            int bank;
            bool unload = bankName[0] == '-';
            if (unload) {
                bank = Banks_Find(self, bankName + 1);
                if ((At<int32_t>(At<uint8_t*>(self, 8), 0x38 + bank * 0x48) & 0x1F) != 1) {
                    continue;   // ("WARNING: %s is not a static bank, it cannot be unloaded for mission %s")
                }
            } else {
                bank = Banks_Find(self, bankName);
            }
            int32_t count = At<int32_t>(self, 0x198);
            int i = 0;
            for (; i < count; i++) {
                if (missionBankLines[i].mission == m) {
                    break;
                }
            }
            if (i == count) {
                At<int32_t>(self, 0x198) = count + 1;
                missionBankLines[i].mission = m;
                missionBankLines[i].loadCount = 0;
                missionBankLines[i].unloadCount = 0;
            }
            if (bank == -1) {
                continue;
            }
            MissionBanks& record = missionBankLines[i];
            if (unload) {
                record.unload[int16_t(record.unloadCount)] = int16_t(bank);
                record.unloadCount++;
            } else {
                record.load[int16_t(record.loadCount)] = int16_t(bank);
                record.loadCount++;
            }
        } while (LineFile_ReadLine(&file, line, 0x100) != 0);
    }
    int32_t count = At<int32_t>(self, 0x198);
    MissionBanks* missions = static_cast<MissionBanks*>(Malloc(count * sizeof(MissionBanks)));
    if (missions != nullptr) {
        for (int j = count - 1; j >= 0; j--) {
            missions[j].mission = -1;
            missions[j].loadCount = 0;
            missions[j].unloadCount = 0;
        }
    }
    At<MissionBanks*>(self, 0x19C) = missions;
    std::memcpy(missions, missionBankLines, At<int32_t>(self, 0x198) * sizeof(MissionBanks));
    if (file.data != nullptr) {
        Free(file.data);
    }
}

// ---- registration -----------------------------------------------------------------------------

#define SOUNDBANKS_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(soundbanks_original_, __LINE__);                                              \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(soundbanks_original_, __LINE__), \
             re::Mode::Original)

SOUNDBANKS_REVERSED("SoundBanks::ClearTriggerLists", 0x00597DE0, &Banks_ClearTriggerListsImpl);
SOUNDBANKS_REVERSED("SoundBanks::FindMission", 0x00597E10, &Banks_FindMission);
SOUNDBANKS_REVERSED("SoundBanks::Release", 0x00597E60, &Banks_Release);
SOUNDBANKS_REVERSED("SoundBanks::SameGroup", 0x00597EA0, &Banks_SameGroup);
SOUNDBANKS_REVERSED("SoundBanks::Find", 0x00597FC0, &Banks_FindImpl);
SOUNDBANKS_REVERSED("SoundBanks::GroupSlot", 0x005980C0, &Banks_GroupSlot);
SOUNDBANKS_REVERSED("SoundBanks::ForWeapon", 0x00598180, &Banks_ForWeapon);
SOUNDBANKS_REVERSED("SoundBanks::ParseType", 0x005986E0, &Banks_ParseType);
SOUNDBANKS_REVERSED("SoundBanks::GetFileName", 0x005987B0, &Banks_GetFileName);
SOUNDBANKS_REVERSED("SoundBanks::RemoveQueued", 0x00598830, &Banks_RemoveQueuedImpl);
SOUNDBANKS_REVERSED("SoundBanks::Unqueue", 0x005988A0, &Banks_Unqueue);
SOUNDBANKS_REVERSED("BitMatrix::Init", 0x00598920, &BitMatrix_Init);
SOUNDBANKS_REVERSED("SoundBanks::SoundBanks", 0x00598970, &Banks_Construct);
SOUNDBANKS_REVERSED("SoundBanks::IsAreaBank", 0x00598A50, &Banks_IsAreaBank);
SOUNDBANKS_REVERSED("SoundBanks::LoadTriggerBankFile", 0x00598AA0, &Banks_LoadTriggerBankFile);
SOUNDBANKS_REVERSED("SoundBanks::LoadWeatherBankFile", 0x00598CF0, &Banks_LoadWeatherBankFile);
SOUNDBANKS_REVERSED("SoundBanks::TriggerBanksLoaded", 0x00598FB0, &Banks_TriggerBanksLoaded);
SOUNDBANKS_REVERSED("SoundBanks::LoadAreaBankFile", 0x00599030, &Banks_LoadAreaBankFile);
SOUNDBANKS_REVERSED("SoundBanks::LoadMissionBankFile", 0x00599210, &Banks_LoadMissionBankFile);

} // namespace
