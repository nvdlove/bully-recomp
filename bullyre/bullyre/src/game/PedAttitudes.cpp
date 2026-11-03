// The attitudes of ped types to each other (Config\Dat\Attitude.dat): a table (0x7E0, at
// 0xC2AF9C) of 12 CVariableAttitudeSets (0x70: the file's attitudes and a change that
// decays by one a minute) and 12 plain CAttitudeSets (0x38) that override them unless 5.
// Attitudes are 0..4; 5 means "not set"; 2 is neutral. Type 13 shares slot 12.

#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CAttitudeSet {
    re::Ptr32<re::Ptr32<void>> vtable;           // +0 0x90C98C
    int32_t attitudes[13];   // +4

    CAttitudeSet* Construct();
    void Reset();
    void Parse(const char* line);
    int32_t* Get(int32_t* out, int type);
    void Set(int type, int attitude);
};
static_assert(sizeof(CAttitudeSet) == 0x38);

struct CVariableAttitudeSet : CAttitudeSet {
    int32_t changes[13];     // +0x38
    uint32_t lastDecay;      // +0x6C

    int32_t* Get(int32_t* out, int type);
    void Set(int type, int attitude);
    void Decay();
};
static_assert(offsetof(CVariableAttitudeSet, changes) == 0x38);
static_assert(sizeof(CVariableAttitudeSet) == 0x70);

struct AttitudeTable {
    CVariableAttitudeSet variable[12];  // +0
    CAttitudeSet fixed[12];             // +0x540

    AttitudeTable* Construct();
    void Load();
    void Reset();
    void DecayAll();
    int32_t* GetAttitude(int32_t* out, int from, int to);
    void SetFixed(int type, int to, int attitude);
    void SetVariable(int type, int to, int attitude);
    void Worsen(int type, int to);
    void ResetAll();
};
static_assert(offsetof(AttitudeTable, fixed) == 0x540);
static_assert(sizeof(AttitudeTable) == 0x7E0);

inline re::Ptr32<void>* const s_attitudeSetVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C98C);
inline re::Ptr32<void>* const s_variableAttitudeSetVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C998);
inline const char* const s_fileName = reinterpret_cast<const char*>(0x0090C9A0);  // "Config\Dat\Attitude.dat"
inline const char* const s_readMode = reinterpret_cast<const char*>(0x008FF548);  // "r"
inline const char* const s_formatString = reinterpret_cast<const char*>(0x008FF970);  // "%s"
inline void* const s_intConstructor = reinterpret_cast<void*>(0x0066BEA0);
GAME_VAR(AttitudeTable*, s_table, 0x00C2AF9C);
GAME_VAR(uint8_t, s_flagA0, 0x00C2AFA0);
GAME_VAR(uint8_t, s_flagA1, 0x00C2AFA1);
GAME_VAR(uint8_t, s_flagA2, 0x00C2AFA2);
GAME_VAR(uint32_t, s_timer, 0x00C1A9B4);
GAME_VAR(CPool<uint8_t>*, s_pedPool, 0x00C0F5F0);

template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(int, __cdecl, AttitudeFromName, 0x00488C50, const char* name);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* constructor);
inline void* __cdecl Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void*, __cdecl, OpenFile, 0x0042D260, const char* path, const char* mode, int arg);
GAME_FN(char*, __cdecl, ReadLine, 0x00429C10, void* file);
GAME_FN(int, __cdecl, CloseFile, 0x0042D450, void* file);
GAME_FN(void, __thiscall, Ped_ResetAttitudes, 0x004731F0, void* ped);
// (their own, called where they are)
GAME_FN(void, __thiscall, AttitudeSet_Parse, 0x0046F2E0, CAttitudeSet* set, const char* line);
GAME_FN(AttitudeTable*, __thiscall, AttitudeTable_Construct, 0x0046F340, AttitudeTable* table);
GAME_FN(void, __thiscall, AttitudeTable_Load, 0x0046F440, AttitudeTable* table);
GAME_FN(void, __thiscall, AttitudeTable_Reset, 0x0046F4A0, AttitudeTable* table);
GAME_FN(void, __thiscall, VariableAttitudeSet_Decay, 0x0046F650, CVariableAttitudeSet* set);

template <typename R, typename... A>
R VCall(void* object, int slot, A... args) {
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[slot]))(object, args...);
}

// 0x46F270 (thiscall)
CAttitudeSet* CAttitudeSet::Construct() {
    vtable = s_attitudeSetVtable;
    for (int32_t& a : attitudes) {
        a = 5;
    }
    return this;
}

// 0x46F2B0 (thiscall)
void CAttitudeSet::Reset() {
    for (int32_t& a : attitudes) {
        a = 5;
    }
}

// 0x46F2E0 (thiscall): 13 attitude names. (Each is looked for two past the last's end)
void CAttitudeSet::Parse(const char* line) {
    char name[24];
    for (int i = 0;; i++) {
        Sscanf(line, s_formatString, name);
        attitudes[i] = AttitudeFromName(name);
        if (i + 1 > 12) {
            break;
        }
        size_t length = 0;
        while (name[length]) {
            length++;
        }
        line += length + 2;
    }
}

// 0x46F510 vf0 (thiscall): (types 12 and 14+: neutral)
int32_t* CAttitudeSet::Get(int32_t* out, int type) {
    if (type == 13) {
        *out = attitudes[12];
    } else if (type < 12) {
        *out = attitudes[type];
    } else {
        *out = 2;
    }
    return out;
}

// 0x46F550 vf1 (thiscall)
void CAttitudeSet::Set(int type, int attitude) {
    if (type == 13) {
        attitudes[12] = attitude;
    } else if (type < 12) {
        attitudes[type] = attitude;
    }
}

// 0x46F580 vf0 (thiscall): the attitude plus a third of its change, within 0..4
int32_t* CVariableAttitudeSet::Get(int32_t* out, int type) {
    int index;
    int base;
    if (type == 13) {
        index = 12;
    } else if (type < 12) {
        index = type;
    } else {
        *out = 2;
        return out;
    }
    base = attitudes[index];
    int value = changes[index] / 3 + base;
    if (value < 0) {
        value = 0;
    } else if (value > 4) {
        value = 4;
    }
    *out = value;
    return out;
}

// 0x46F600 vf1 (thiscall): set, its change cleared
void CVariableAttitudeSet::Set(int type, int attitude) {
    int index;
    if (type == 13) {
        index = 12;
    } else if (type < 12) {
        index = type;
    } else {
        return;
    }
    attitudes[index] = attitude;
    changes[index] = 0;
}

// 0x46F650 (thiscall): once a minute the changes one step back towards 0
void CVariableAttitudeSet::Decay() {
    if (lastDecay - s_timer < 60000) {
        return;
    }
    lastDecay = s_timer;
    for (int32_t& c : changes) {
        if (c < 0) {
            c++;
        } else if (float(c) > 0.0f) {
            c--;
        }
    }
}

// 0x46F340 (thiscall)
AttitudeTable* AttitudeTable::Construct() {
    for (CVariableAttitudeSet& set : variable) {
        set.vtable = s_attitudeSetVtable;
        ConstructArray(set.attitudes, 4, 13, s_intConstructor);
        for (int32_t& a : set.attitudes) {
            a = 5;
        }
        set.vtable = s_variableAttitudeSetVtable;
        Memset(set.changes, 0, 0x34);
    }
    for (CAttitudeSet& set : fixed) {
        set.vtable = s_attitudeSetVtable;
        ConstructArray(set.attitudes, 4, 13, s_intConstructor);
        for (int32_t& a : set.attitudes) {
            a = 5;
        }
    }
    return this;
}

// 0x46F420: the table freed
void __cdecl DestroyTable() {
    if (s_table) {
        Free(s_table);
        s_table = nullptr;
    }
}

// 0x46F440 (thiscall): a line per variable set (blank and '#' lines skipped)
void AttitudeTable::Load() {
    void* file = OpenFile(s_fileName, s_readMode, 1);
    CVariableAttitudeSet* set = variable;
    for (char* line = ReadLine(file); line; line = ReadLine(file)) {
        if (line[0] != '\0' && line[0] != '#') {
            AttitudeSet_Parse(set, line);
            set++;
        }
    }
    CloseFile(file);
}

// 0x46F4A0 (thiscall): the changes cleared, the fixed sets unset
void AttitudeTable::Reset() {
    for (int i = 0; i < 12; i++) {
        Memset(variable[i].changes, 0, 0x34);
        for (int32_t& a : fixed[i].attitudes) {
            a = 5;
        }
    }
    s_flagA0 = 0;
    s_flagA1 = 0;
    s_flagA2 = 0;
}

// 0x46F7B0: the table, made and loaded if there's none
void __cdecl CreateTable() {
    if (s_table) {
        return;
    }
    AttitudeTable* table = static_cast<AttitudeTable*>(Malloc(0x7E0));
    s_table = table ? AttitudeTable_Construct(table) : nullptr;
    AttitudeTable_Load(s_table);
}

// 0x46F800 (thiscall)
void AttitudeTable::DecayAll() {
    for (CVariableAttitudeSet& set : variable) {
        VariableAttitudeSet_Decay(&set);
    }
}

// 0x46F830 (thiscall): `from`'s attitude to `to`: the fixed one, else the variable one
int32_t* AttitudeTable::GetAttitude(int32_t* out, int from, int to) {
    if (from == 13 || from >= 12) {
        *out = 2;
        return out;
    }
    int32_t value;
    VCall<int32_t*>(&fixed[from], 0, &value, to);
    if (value == 5) {
        int32_t variableValue;
        value = *VCall<int32_t*>(&variable[from], 0, &variableValue, to);
    }
    *out = value;
    return out;
}

// 0x46F8B0 (thiscall): (types 12 and 14+ index one past the fixed sets)
void AttitudeTable::SetFixed(int type, int to, int attitude) {
    int index = type == 13 ? 12 : type >= 12 ? 13 : type;
    CAttitudeSet* set = reinterpret_cast<CAttitudeSet*>(reinterpret_cast<uint8_t*>(this) + (index + 0x18) * 0x38);
    VCall<void>(set, 1, to, attitude);
}

// 0x46F8F0 (thiscall)
void AttitudeTable::SetVariable(int type, int to, int attitude) {
    int index = type == 13 ? 12 : type >= 12 ? 13 : type;
    CAttitudeSet* set = reinterpret_cast<CAttitudeSet*>(reinterpret_cast<uint8_t*>(this) + index * 0x70);
    VCall<void>(set, 1, to, attitude);
}

// 0x46F930 (thiscall): `type`'s change towards `to` one down
void AttitudeTable::Worsen(int type, int to) {
    if (type == 13 || type >= 12) {
        return;
    }
    CVariableAttitudeSet& set = variable[type];
    if (to == 13) {
        set.changes[12]--;
    } else if (to < 12) {
        set.changes[to]--;
    }
}

// 0x46FA20 (thiscall): every ped's attitudes reset, then the table's
void AttitudeTable::ResetAll() {
    CPool<uint8_t>* pool = s_pedPool;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->IsFreeSlot(i)) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * i;
        if (ped) {
            Ped_ResetAttitudes(ped);
        }
    }
    AttitudeTable_Reset(this);
}

} // namespace

#define ATT_C05_REVERSED(name, address, fn) REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)

ATT_C05_REVERSED("CAttitudeSet::CAttitudeSet", 0x0046F270, &CAttitudeSet::Construct);
ATT_C05_REVERSED("CAttitudeSet::Reset", 0x0046F2B0, &CAttitudeSet::Reset);
ATT_C05_REVERSED("CAttitudeSet::Parse", 0x0046F2E0, &CAttitudeSet::Parse);
ATT_C05_REVERSED("AttitudeTable::AttitudeTable", 0x0046F340, &AttitudeTable::Construct);
REVERSED("AttitudeTable::Destroy", 0x0046F420, RE_BODY(&DestroyTable), nullptr, nullptr, re::Mode::Original);
ATT_C05_REVERSED("AttitudeTable::Load", 0x0046F440, &AttitudeTable::Load);
ATT_C05_REVERSED("AttitudeTable::Reset", 0x0046F4A0, &AttitudeTable::Reset);
ATT_C05_REVERSED("CAttitudeSet::Get", 0x0046F510, &CAttitudeSet::Get);
ATT_C05_REVERSED("CAttitudeSet::Set", 0x0046F550, &CAttitudeSet::Set);
ATT_C05_REVERSED("CVariableAttitudeSet::Get", 0x0046F580, &CVariableAttitudeSet::Get);
ATT_C05_REVERSED("CVariableAttitudeSet::Set", 0x0046F600, &CVariableAttitudeSet::Set);
ATT_C05_REVERSED("CVariableAttitudeSet::Decay", 0x0046F650, &CVariableAttitudeSet::Decay);
REVERSED("AttitudeTable::Create", 0x0046F7B0, RE_BODY(&CreateTable), nullptr, nullptr, re::Mode::Original);
ATT_C05_REVERSED("AttitudeTable::DecayAll", 0x0046F800, &AttitudeTable::DecayAll);
ATT_C05_REVERSED("AttitudeTable::GetAttitude", 0x0046F830, &AttitudeTable::GetAttitude);
ATT_C05_REVERSED("AttitudeTable::SetFixed", 0x0046F8B0, &AttitudeTable::SetFixed);
ATT_C05_REVERSED("AttitudeTable::SetVariable", 0x0046F8F0, &AttitudeTable::SetVariable);
ATT_C05_REVERSED("AttitudeTable::Worsen", 0x0046F930, &AttitudeTable::Worsen);
ATT_C05_REVERSED("AttitudeTable::ResetAll", 0x0046FA20, &AttitudeTable::ResetAll);
