// The sound banks (cSCREAMBankManager, 0xCF5DC4; see SoundBanks.cpp): the bank list read
// from bankinfo.txt with its LOD groups, loading and unloading. 0x5994A0..0x599780, with
// the bank's and the weapon slot's constructors (0x5988F0, 0x597DC0).

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
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

// a bank (0x48 bytes)
struct Bank {
    char name[0x32];      // +0 (upper case, no path, no extension)
    int8_t slot;          // +0x32 the voice engine's (-1 not loaded)
    uint8_t unknown33;    // +0x33
    uint8_t flags;        // +0x34 1 loaded, 8 a LOD
    uint8_t pad35[3];
    uint32_t type;        // +0x38 the low five bits (Banks_ParseType)
    int32_t unknown3C;    // +0x3C the file's header's third dword
    int32_t size;         // +0x40 the file's size
    re::Ptr32<void> file;           // +0x44 its handle (CreateFileA)
};
static_assert(offsetof(Bank, flags) == 0x34);
static_assert(sizeof(Bank) == 0x48);

// a weapon's slot (8 bytes; +0x194 in the manager, one per weapon)
struct WeaponSlot {
    uint8_t unknown0;
    uint8_t flags;
    uint8_t pad2[2];
    int32_t unknown4;
};

inline uint8_t* const voiceEngine = reinterpret_cast<uint8_t*>(0x00CF7268);
GAME_ARRAY(Bank, bankLines, 0x00BD7800);        // (the scratch buffer)
GAME_ARRAY(int16_t, groupLines, 0x00BD7800);    // (the same, later)
GAME_VAR(int32_t, numBanks, 0x00CF68F4);
GAME_VAR(int16_t, numBankGroups, 0x00CF68EC);
GAME_VAR(int16_t*, bankGroups, 0x00CF68F0);
GAME_VAR(const char*, bankInfoFile, 0x00AE6BD4);   // "Audio\CONFIG\bankinfo.txt"
GAME_VAR(int32_t, firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, lastWeapon, 0x00A136B4);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
constexpr uintptr_t kCreateFileA = 0x008FC064;

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* constructor);
inline const char* __cdecl StrRChr(const char* s, int c) { return (const char*)crt::Strrchr(s, c); }
inline int __cdecl Strnicmp(const char* a, const char* b, size_t n) { return crt::Strnicmp(a, b, n); }
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
inline char* __cdecl Strncpy(char* dst, const char* src, size_t n) { return crt::Strncpy(dst, src, n); }
inline char* __cdecl StrUpr(char* s) { return crt::Strupr(s); }   // _strupr
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(void, __cdecl, LoadingScreen_Message, 0x0043C4F0, const char* title, const char* text, uintptr_t);
GAME_FN(uint8_t, __thiscall, LineFile_Load, 0x00426610, LineFile* file, const char* path);
GAME_FN(uint8_t, __cdecl, LineFile_ReadLine, 0x0042D600, LineFile* file, char* line, int size);
GAME_FN(const char*, __cdecl, FixPath, 0x00829900, const char* path);
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(int, __cdecl, File_Read, 0x0042D2A0, void* file, void* buffer, int size);
GAME_FN(uint8_t, __cdecl, File_Seek, 0x0042D300, void* file, int32_t offset, uint32_t origin);
GAME_FN(uint32_t, __cdecl, File_Tell, 0x0042D430, void* file);
GAME_FN(void, __cdecl, File_Close, 0x0042D450, void* file);
GAME_FN(void*, __thiscall, VoiceEngine_LoadBank, 0x005AA350, void* engine, int bank, int);
GAME_FN(int16_t, __thiscall, VoiceEngine_AddBank, 0x005A91F0, void* engine, void* handle);
// its own
GAME_FN(void, __thiscall, Banks_RemoveQueued, 0x00598830, void* self, int index);
GAME_FN(int16_t, __thiscall, Banks_GroupSlot, 0x005980C0, void* self, int bank);
GAME_FN(int, __thiscall, Banks_ParseType, 0x005986E0, void* self, const char* name);
GAME_FN(void, __thiscall, Banks_GetFileName, 0x005987B0, void* self, int bank, re::Ptr32<const char>* name, int16_t* length);
GAME_FN(int, __thiscall, Banks_Find, 0x00597FC0, void* self, const char* name);

Bank* BanksOf(uint8_t* self) {
    return At<Bank*>(self, 8);
}

// the queue's first entry gone
void PopQueue(uint8_t* self) {
    int32_t& count = At<int32_t>(self, 0x110);
    count--;
    if (count > 0) {
        std::memmove(self + 0x114, self + 0x118, count * 4);
    }
    if (count != 0) {
        self[0x18C] = self[0x116] & 1;
        return;
    }
    self[0x18C] = 0;
}

// the name without its path (the last of the slashes)
const char* NameAfterPath(const char* full) {
    const char* back = StrRChr(full, '\\');
    const char* forward = StrRChr(full, '/');
    const char* p;
    if (back == nullptr && forward == nullptr) {
        p = full;
    } else {
        p = forward > back ? forward : back;
    }
    return p + 1;
}

// 0x5988F0 (thiscall): a bank's constructor
Bank* __fastcall Bank_Construct(Bank* self) {
    self->flags &= 0xF8;
    self->type &= ~0x1Fu;
    self->name[0] = 0;
    self->unknown33 = 0;
    self->slot = -1;
    self->unknown3C = -1;
    self->size = -1;
    self->file = reinterpret_cast<void*>(intptr_t(-1));
    return self;
}

// 0x597DC0 (thiscall): a weapon slot's constructor
WeaponSlot* __fastcall WeaponSlot_Construct(WeaponSlot* self) {
    self->flags &= 0xFE;
    self->unknown0 = 0;
    self->unknown4 = 0;
    return self;
}

// 0x5994A0 (thiscall): the queue's first bank loaded
void __fastcall Banks_Loaded(uint8_t* self, re::Edx, int bank) {
    BanksOf(self)[bank].flags |= 1;
    At<uint32_t>(self, 0x10C) = m_snTimeInMilliseconds;
    PopQueue(self);
}

// 0x599520 (thiscall): the queue's first bank unloaded (a later load of it dropped)
void __fastcall Banks_Unloaded(uint8_t* self, re::Edx, int bank) {
    Bank* b = &BanksOf(self)[bank];
    int32_t count = At<int32_t>(self, 0x110);
    if (count > 1) {
        for (int i = 1; i < At<int32_t>(self, 0x110); i++) {
            if (int32_t(At<int16_t>(self, 0x114 + i * 4)) == bank && (self[0x116 + i * 4] & 1) == 0) {
                Banks_RemoveQueued(self, i);
                break;
            }
        }
    }
    b->flags &= 0xFE;
    At<uint32_t>(self, 0x10C) = m_snTimeInMilliseconds;
    At<int32_t>(self, 0xC + b->slot * 8) = 0;
    At<int32_t>(self, 0x10 + b->slot * 8) = 0;
    b->slot = -1;
    b->unknown33 = 0;
    PopQueue(self);
}

// 0x5995F0 (thiscall): a bank (or its group's) loaded
uint32_t __fastcall Banks_IsLoaded(uint8_t* self, re::Edx, int bank) {
    Bank* banks = BanksOf(self);
    if (banks != nullptr && (banks[Banks_GroupSlot(self, bank)].flags & 1) != 0) {
        return 1;
    }
    return 0;
}

// 0x599620 (thiscall): a bank loaded into the voice engine now (0: failed)
uint8_t __fastcall Banks_LoadNow(uint8_t* self, re::Edx, int bank) {
    // (the game builds "audio\xact\<name>" for its debug messages here)
    if (BanksOf(self) != nullptr) {
        Banks_GroupSlot(self, bank);   // ("already in memory" is only printed)
    }
    Bank* banks = BanksOf(self);
    if (banks != nullptr && (banks[Banks_GroupSlot(self, bank)].flags & 1) != 0) {
        return 1;
    }
    if (banks[bank].slot == -1) {
        void* handle = VoiceEngine_LoadBank(voiceEngine, bank, 0);
        if (handle == nullptr) {
            return 0;   // ("Bank Load FAILED ! [ %s ]")
        }
        int16_t slot = VoiceEngine_AddBank(voiceEngine, handle);
        BanksOf(self)[bank].flags |= 1;
        BanksOf(self)[bank].slot = int8_t(slot);
        At<Bank*>(self, 0xC + slot * 8) = &BanksOf(self)[bank];
        At<void*>(self, 0x10 + slot * 8) = handle;
    }
    return 1;
}

// 0x599780 (thiscall): bankinfo.txt ("file type [lod]"): the bank list, the LOD groups
// (banks whose names share their stem, sorted, three a group), the weapon slots, each
// bank's file opened and its size
uint8_t __fastcall Banks_InitBankArray(uint8_t* self) {
    const char* title = "cSCREAMBankManager::InitBankArray()";
    PushMemoryTag(0x2E);
    int16_t* lods = static_cast<int16_t*>(Malloc(0x190));
    int32_t numLods = 0;
    numBanks = 0;
    LineFile file{nullptr, 0, 0};
    char line[0x100];
    char name[0x200];
    char type[0x100];
    char lod[0xC];
    LineFile_Load(&file, bankInfoFile);
    while (LineFile_ReadLine(&file, line, 0x100) != 0) {
        char first = line[0];
        if (first == '#' || std::strlen(line) < 7 || first == '\t') {
            continue;
        }
        lod[0] = 0;
        Sscanf(line, "%s\t%s\t%s\n", name, type, lod);
        Bank* bank = &bankLines[numBanks];
        bank->flags &= 0xF8;
        bank->type &= ~0x1Fu;
        bank->unknown3C = -1;
        bank->size = -1;
        bank->file = reinterpret_cast<void*>(intptr_t(-1));
        bank->name[0] = 0;
        bank->unknown33 = 0;
        bank->slot = -1;
        Strncpy(bankLines[numBanks].name, name, 0x32);
        char* stored = bankLines[numBanks].name;
        stored[std::strlen(stored) - 4] = 0;   // (the extension)
        StrUpr(bankLines[numBanks].name);
        const char* slash = StrRChr(bankLines[numBanks].name, '\\');
        if (slash == nullptr) {
            slash = StrRChr(bankLines[numBanks].name, '/');
        }
        if (slash != nullptr) {
            const char* src = slash + 1;
            char* dst = bankLines[numBanks].name;
            char c;
            do {
                c = *src++;
                *dst++ = c;
            } while (c != 0);
        }
        if (lod[0] != 0 && Stricmp(lod, "lod") == 0) {
            bankLines[numBanks].flags |= 8;
            lods[int16_t(numLods)] = int16_t(numBanks);
            numLods++;
        } else {
            bankLines[numBanks].flags &= 0xF7;
        }
        int t = Banks_ParseType(self, type);
        uint32_t& bits = bankLines[numBanks].type;
        bits ^= (bits ^ uint32_t(t)) & 0x1F;
        bankLines[numBanks].flags &= 0xFE;
        numBanks++;
    }
    LoadingScreen_Message(title, "BankInfo", 0);
    if (file.data != nullptr) {
        Free(file.data);
        file.data = nullptr;
    }
    // the banks
    int32_t n = numBanks;
    void* p = Malloc(n * sizeof(Bank));
    Bank* banks = nullptr;
    if (p != nullptr) {
        ConstructArray(p, sizeof(Bank), n, reinterpret_cast<void*>(0x005988F0));
        banks = static_cast<Bank*>(p);
    }
    At<Bank*>(self, 8) = banks;
    std::memcpy(banks, bankLines, numBanks * sizeof(Bank));
    // a slot per weapon
    int32_t weapons = lastWeapon - firstWeapon + 1;
    At<int32_t>(self, 0x190) = weapons;
    p = Malloc(weapons * sizeof(WeaponSlot));
    WeaponSlot* slots = nullptr;
    if (p != nullptr) {
        ConstructArray(p, sizeof(WeaponSlot), weapons, reinterpret_cast<void*>(0x00597DC0));
        slots = static_cast<WeaponSlot*>(p);
    }
    At<WeaponSlot*>(self, 0x194) = slots;
    for (int i = 0; i < At<int32_t>(self, 0x190); i++) {
        At<WeaponSlot*>(self, 0x194)[i].unknown0 = 0;
        At<WeaponSlot*>(self, 0x194)[i].flags &= 0xFE;
        At<WeaponSlot*>(self, 0x194)[i].unknown4 = 0;
    }
    LoadingScreen_Message(title, "Order LODs", 0);
    PopMemoryTag();
    // The LOD groups. (The game's first id of a group goes to ids[k], k the LOD's index,
    // not ids[0]: a later group keeps an earlier one's first. Its ids and names arrays
    // are stack locals that further ids/names overrun into its other locals; not so here)
    int32_t count = int16_t(numLods);
    int16_t* out = groupLines;
    int16_t ids[0x100] = {};
    re::Ptr32<const char> names[0x102] = {};
    for (int k = 0; k < count; k++) {
        if (uint16_t(lods[k]) == 0xFFFF) {
            continue;
        }
        int16_t length;
        Banks_GetFileName(self, lods[k], &names[1], &length);
        int members = 1;
        ids[k] = lods[k];
        lods[k] = -1;
        for (int j = 0; j < count; j++) {
            if (j == k) {
                continue;
            }
            const char* other = NameAfterPath(BanksOf(self)[int16_t(lods[j])].name);
            if (Strnicmp(names[members], other, size_t(int32_t(length))) == 0) {
                int16_t id = lods[j];
                names[members + 1] = other;
                ids[members] = id;
                members++;
                lods[j] = -1;
            }
        }
        int32_t m = int16_t(members);
        bool swapped;
        do {
            swapped = false;
            if (m <= 1) {
                break;
            }
            for (int i = 1; i < m; i++) {
                const char* a = names[i];
                const char* b = names[i + 1];
                if (Stricmp(a, b) > 0) {
                    int16_t id = ids[i - 1];
                    names[i] = b;
                    ids[i - 1] = ids[i];
                    names[i + 1] = a;
                    ids[i] = id;
                    swapped = true;
                }
            }
        } while (swapped);
        for (int c = 0; c < 3; c++) {
            out[c] = c < m ? ids[c] : int16_t(-1);
        }
        out += 3;
    }
    uint32_t bytes = uint32_t(reinterpret_cast<uint8_t*>(out) - reinterpret_cast<uint8_t*>(groupLines));
    int16_t* groups = static_cast<int16_t*>(Malloc((bytes >> 1) * 2));
    bankGroups = groups;
    numBankGroups = int16_t(bytes / 6);
    std::memcpy(groups, groupLines, bytes);
    LoadingScreen_Message(title, "FindSizeForAll", 0);
    // each bank's file
    for (int i = 0; i < numBanks; i++) {
        char path[0x100];
        std::strcpy(path, "audio\\xact\\");
        std::strcat(path, BanksOf(self)[i].name);
        std::strcat(path, ".sea");
        const char* fixed = FixPath(path);
        BanksOf(self)[i].file = vc::Import<void*>(kCreateFileA, fixed, 0x80000000u, 1u, static_cast<void*>(nullptr),
                                                  3u, 0x70000001u, static_cast<void*>(nullptr));
        void* f = File_Open(path, "r", 0);
        if (f != nullptr) {
            int32_t header[4];
            if (File_Read(f, header, 0x10) == 0x10) {
                BanksOf(self)[i].unknown3C = header[2];
            } else {
                BanksOf(self)[i].unknown3C = -1;
            }
            File_Seek(f, 0, 2);
            BanksOf(self)[i].size = int32_t(File_Tell(f));
            File_Close(f);
        }
        LoadingScreen_Message(title, "Crowd Bank", 0);
    }
    Free(lods);
    At<int32_t>(self, 0x240) = Banks_Find(self, "MISSION\\Crowd.bnk");
    return 1;
}

// ---- registration -----------------------------------------------------------------------------

#define SOUNDBANKS2_REVERSED(name, address, fn)                                                          \
    static void* RE_CONCAT(soundbanks2_original_, __LINE__);                                              \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(soundbanks2_original_, __LINE__), \
             re::Mode::Original)

SOUNDBANKS2_REVERSED("SoundBank::SoundBank", 0x005988F0, &Bank_Construct);
SOUNDBANKS2_REVERSED("SoundBanks::WeaponSlot", 0x00597DC0, &WeaponSlot_Construct);
SOUNDBANKS2_REVERSED("SoundBanks::Loaded", 0x005994A0, &Banks_Loaded);
SOUNDBANKS2_REVERSED("SoundBanks::Unloaded", 0x00599520, &Banks_Unloaded);
SOUNDBANKS2_REVERSED("SoundBanks::IsLoaded", 0x005995F0, &Banks_IsLoaded);
SOUNDBANKS2_REVERSED("SoundBanks::LoadNow", 0x00599620, &Banks_LoadNow);
SOUNDBANKS2_REVERSED("SoundBanks::InitBankArray", 0x00599780, &Banks_InitBankArray);

} // namespace
