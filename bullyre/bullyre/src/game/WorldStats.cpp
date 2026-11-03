// The player's statistics (0x458640..0x45A9D0): the 0x104 stored ones (0xC1A338, ints or
// floats by kind), the derived ones past them (ids 0x104..0x17D, worked out from the
// missions, the classes and other counters), the stats menu's layout, saving and
// loading them, and the "best of" lookups.

#include <cstdint>
#include <cstring>
#include <initializer_list>

#include "reversed.h"

namespace {

// ---- the game's --------------------------------------------------------------------

GAME_ARRAY(int32_t, ws_stats, 0x00C1A338);          // 0x104 stored stats (int or float bits)
GAME_ARRAY(uint8_t, ws_statKinds, 0x00C1A1B8);      // 0x17E: 0/1/5/6 ints, 2/4 floats, 3, 7
GAME_ARRAY(int32_t, ws_baseline, 0x00C19300);       // 0x26: each tracked stat's value when marked
GAME_ARRAY(uint16_t, ws_baselineIds, 0x00C19398);   // 0x26 stat ids
GAME_ARRAY(uint16_t, ws_classIds, 0x00C192D4);      // 12 class stats
GAME_ARRAY(uint16_t, ws_groupIds, 0x00C192EC);      // 10
GAME_ARRAY(uint16_t, ws_minigameIds, 0x00C192C8);   // 6
GAME_VAR(uint16_t, ws_numLayoutItems, 0x00C1A748);
GAME_ARRAY(uint8_t, ws_layoutItems, 0x00C193E8);    // 8 bytes each: +0 id, +2 kind, +4 name hash
GAME_VAR(uint8_t, ws_tracking, 0x00C193E4);
GAME_VAR(uint32_t, ws_lastTime, 0x00C1A74C);
GAME_VAR(uint32_t, ws_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t, ws_firstPedModel, 0x00A136E0);
// the names the mission counts go by (4-byte string objects, 0xC1A760..0xC1A7C8)
inline uintptr_t const ws_names = 0x00C1A760;

inline void* const ws_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const ws_classMgr = reinterpret_cast<void*>(0x00BCEB50);
inline void* const ws_player = reinterpret_cast<void*>(0x00C1AEA8);

GAME_FN(int32_t, __thiscall, Missions_Count, 0x006AA140, void* self);
GAME_FN(uint8_t, __thiscall, Missions_Fn6AB020, 0x006AB020, void* self, int index);
GAME_FN(uint8_t*, __thiscall, Missions_Get, 0x006AA660, void* self, int index);
GAME_FN(int32_t, __thiscall, Missions_CountPassed, 0x006AAA70, void* self, void* name, uintptr_t);
GAME_FN(int32_t, __thiscall, Missions_CountAll, 0x006AAB70, void* self, void* name, uintptr_t);
GAME_FN(int16_t, __thiscall, Classes_Fn426440, 0x00426440, void* self, int);
GAME_FN(int32_t, __cdecl, Fn69FC30, 0x0069FC30);
GAME_FN(int32_t, __thiscall, Player_Fn4467B0, 0x004467B0, void* self, int);
GAME_FN(int32_t, __thiscall, Player_Fn4467E0, 0x004467E0, void* self, int);
GAME_FN(int32_t, __thiscall, Player_Fn446780, 0x00446780, void* self, int);
GAME_FN(int32_t, __cdecl, Fn488B30, 0x00488B30, int);
GAME_FN(int32_t, __cdecl, Fn488B10, 0x00488B10, uintptr_t);
GAME_FN(int32_t, __cdecl, GetIntStat, 0x00458640, int id);   // (ours below)
GAME_FN(float, __cdecl, GetFloatStat, 0x00459A20, int id);   // (ours below)
GAME_FN(float, __cdecl, Percentage, 0x004585D0, float part, float whole);
GAME_FN(float, __cdecl, FESave_Fn6A98C0, 0x006A98C0);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __thiscall, Name_Set, 0x005FA890, void* self, const char* name);
GAME_FN(void, __thiscall, Word_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(uint8_t, __cdecl, Save_CheckValue, 0x00733320, int, int, int32_t value, int32_t expected);
GAME_FN(uint8_t, __cdecl, Save_CheckBlock, 0x00733620, const char* name, void* data, int);
GAME_FN(void, __cdecl, Stats_AddTime, 0x00458480, int id, int32_t ms);
GAME_FN(uint8_t, __thiscall, Fn6A8D20, 0x006A8D20, void* self);
GAME_FN(uint8_t, __cdecl, Stats_BestClass, 0x0045A6F0);
GAME_FN(uint8_t, __cdecl, Stats_BestGroup, 0x0045A740);
GAME_FN(uint8_t, __cdecl, Stats_WorstGroup, 0x0045A7C0);
GAME_FN(uint8_t, __cdecl, Stats_BestMinigame, 0x0045A860);
GAME_FN(uint8_t, __cdecl, Stats_WorstClassGrade, 0x00459240);

void* Name(uintptr_t address) {
    return reinterpret_cast<void*>(address);
}
int32_t Passed(uintptr_t name) {
    return Missions_CountPassed(ws_missionMgr, Name(name), 0);
}
int32_t All(uintptr_t name) {
    return Missions_CountAll(ws_missionMgr, Name(name), 0);
}
int32_t Global(uintptr_t address) {
    return *reinterpret_cast<const int32_t*>(address);
}
// a stored stat, or a derived one
int32_t IntStat(uint32_t id) {
    return id < 0x104 ? ws_stats[id] : GetIntStat(int(id));
}

// ---- the derived stats -----------------------------------------------------------------

// 0x458640 (cdecl): a derived int stat (0 for the float ones and unknown ids)
int32_t __cdecl GetIntStatImpl(int id) {
    switch (id) {
    case 0x106: {   // missions done with their +0x4A set
        int32_t count = 0;
        if (Missions_Count(ws_missionMgr) <= 0) {
            return 0;
        }
        uint16_t i = 0;
        int index = 0;
        do {
            if (Missions_Fn6AB020(ws_missionMgr, index) && Missions_Get(ws_missionMgr, index)[0x4A]) {
                count++;
            }
            i++;
            index = i;
        } while (index < Missions_Count(ws_missionMgr));
        return count;
    }
    case 0x108: return Classes_Fn426440(ws_classMgr, 5);
    case 0x109: return Classes_Fn426440(ws_classMgr, 4);
    case 0x10A: return Classes_Fn426440(ws_classMgr, 0);
    case 0x10B: return Classes_Fn426440(ws_classMgr, 1);
    case 0x10C: return Classes_Fn426440(ws_classMgr, 2);
    case 0x10D: return Classes_Fn426440(ws_classMgr, 3);
    case 0x10E: return Global(0x020C6B70) - Global(0x020C7BAC) - Global(0x020C7BA4);
    case 0x110: return Global(0x00C1A410) - Global(0x00C1A414);
    case 0x111: return Global(0x00C1A418) - Global(0x00C1A41C);
    case 0x117: {   // how many of five mission groups have one passed
        int32_t n = Passed(0x00C1A7C4) > 0;
        n += Passed(0x00C1A7C0) > 0;
        n += Passed(0x00C1A7BC) > 0;
        n += Passed(0x00C1A7B8) > 0;
        n += Passed(0x00C1A7B4) > 0;
        return n;
    }
    case 0x118: return Fn69FC30();
    case 0x12E: return Global(0x00C1A5C4) + Global(0x00C1A5BC) + Global(0x00C1A5B4) + Global(0x00C1A5CC);
    case 0x132: {
        int32_t a = GetIntStat(0x133), b = GetIntStat(0x136), c = GetIntStat(0x139), d = GetIntStat(0x13C);
        int32_t e = GetIntStat(0x13F), f = GetIntStat(0x142), g = GetIntStat(0x145), h = GetIntStat(0x148);
        int32_t i = GetIntStat(0x14B), j = GetIntStat(0x14E);
        return j + i + h + g + f + e + d + c + b + a;
    }
    // the ten mission groups: all, and all minus passed
    case 0x133: return All(0x00C1A778);
    case 0x134: return All(0x00C1A778) - Passed(0x00C1A778);
    case 0x136: return All(0x00C1A77C);
    case 0x137: return All(0x00C1A77C) - Passed(0x00C1A77C);
    case 0x139: return All(0x00C1A770);
    case 0x13A: return All(0x00C1A770) - Passed(0x00C1A770);
    case 0x13C: return All(0x00C1A76C);
    case 0x13D: return All(0x00C1A76C) - Passed(0x00C1A76C);
    case 0x13F: return All(0x00C1A774);
    case 0x140: return All(0x00C1A774) - Passed(0x00C1A774);
    case 0x142: return All(0x00C1A764);
    case 0x143: return All(0x00C1A764) - Passed(0x00C1A764);
    case 0x145: return All(0x00C1A784);
    case 0x146: return All(0x00C1A784) - Passed(0x00C1A784);
    case 0x148: return All(0x00C1A78C);
    case 0x149: return All(0x00C1A78C) - Passed(0x00C1A78C);
    case 0x14B: return All(0x00C1A780);
    case 0x14C: return All(0x00C1A780) - Passed(0x00C1A780);
    case 0x14E: return All(0x00C1A788);
    case 0x14F: return All(0x00C1A788) - Passed(0x00C1A788);
    case 0x151: return All(0x00C1A790);
    case 0x152: return Passed(0x00C1A790);
    case 0x155: return Global(0x00C1A5E8) - Global(0x00C1A5EC);
    case 0x157: return Global(0x00C1A5F8) - Global(0x00C1A5FC);
    case 0x159: return All(0x00C1A7B0);
    case 0x15A: return Passed(0x00C1A7B0);
    case 0x15B: {
        int32_t a = GetIntStat(0x159);
        return a - GetIntStat(0x15A);
    }
    case 0x15D: return Global(0x00C1A608) - Global(0x00C1A60C);
    case 0x15F: return Global(0x00C1A670) - Global(0x00C1A674);
    case 0x160: {
        int32_t a = All(0x00C1A7AC), b = All(0x00C1A7A8), c = All(0x00C1A7A4), d = All(0x00C1A760);
        return d + c + a + b;
    }
    case 0x161: {
        int32_t a = Passed(0x00C1A7AC), b = Passed(0x00C1A7A8), c = Passed(0x00C1A7A4), d = Passed(0x00C1A760);
        return d + c + a + b;
    }
    case 0x162: {
        int32_t a = GetIntStat(0x160);
        return a - GetIntStat(0x161);
    }
    case 0x165: return Global(0x00C1A688) - Global(0x00C1A68C);
    case 0x166: return Global(0x00C1A690) - Global(0x00C1A694);
    case 0x167: {
        int32_t sum = 0;
        for (int i = 0; i < 0x32; i++) {
            sum += Player_Fn4467B0(ws_player, i);
        }
        return sum;
    }
    case 0x168: {
        int32_t sum = 0;
        for (int i = 0; i < 0x32; i++) {
            sum += Player_Fn4467E0(ws_player, i);
        }
        return sum;
    }
    case 0x16A: {
        int32_t a = GetIntStat(0x16D), b = GetIntStat(0x171), c = GetIntStat(0x175), d = GetIntStat(0x17A);
        return d + c + b + a;
    }
    case 0x16B: {
        int32_t a = GetIntStat(0x16E), b = GetIntStat(0x172), c = GetIntStat(0x176), d = GetIntStat(0x17B);
        return d + c + b + a;
    }
    case 0x16C: {
        int32_t a = GetIntStat(0x16A);
        return a - GetIntStat(0x16B);
    }
    case 0x16D: return All(0x00C1A794);
    case 0x16E: return Passed(0x00C1A794);
    case 0x16F: {
        int32_t a = GetIntStat(0x16D);
        return a - GetIntStat(0x16E);
    }
    case 0x171: return All(0x00C1A798);
    case 0x172: return Passed(0x00C1A798);
    case 0x173: {
        int32_t a = GetIntStat(0x171);
        return a - GetIntStat(0x172);
    }
    case 0x175: return All(0x00C1A79C);
    case 0x176: return Passed(0x00C1A79C);
    case 0x177: {
        int32_t a = GetIntStat(0x175);
        return a - GetIntStat(0x176);
    }
    case 0x17A: return All(0x00C1A7A0);
    case 0x17B: return Passed(0x00C1A7A0);
    case 0x17C: {
        int32_t a = GetIntStat(0x17A);
        return a - GetIntStat(0x17B);
    }
    default:
        return 0;
    }
}

float PercentOf(uintptr_t part, uintptr_t whole) {
    return Percentage(float(Global(part)), float(Global(whole)));
}
// passed of all: (all - (all - passed)) of all
float PercentGroup(int allId, int leftId) {
    int32_t whole = GetIntStat(allId);
    int32_t all = GetIntStat(allId);
    float wholeF = float(whole);
    int32_t done = all - GetIntStat(leftId);
    return Percentage(float(done), wholeF);
}
float PercentStats(int wholeId, int partId) {
    float whole = float(GetIntStat(wholeId));
    return Percentage(float(GetIntStat(partId)), whole);
}

// 0x459A20 (cdecl): a derived float stat (percentages; 0 for the rest)
float __cdecl GetFloatStatImpl(int id) {
    float result = 0.0f;
    switch (id) {
    case 0x105: return FESave_Fn6A98C0();
    case 0x107: return PercentOf(0x00C1A378, 0x00C1A374);
    case 0x112: return PercentOf(0x00C1A454, 0x00C1A450);
    case 0x113: return PercentOf(0x00C1A45C, 0x00C1A458);
    case 0x114: return PercentOf(0x00C1A464, 0x00C1A460);
    case 0x115: return PercentOf(0x00C1A470, 0x00C1A46C);
    case 0x116: return PercentOf(0x00C1A484, 0x00C1A480);
    // hits of hits plus misses
    case 0x119: return Percentage(float(Global(0x00C1A488)), float(Global(0x00C1A48C) + Global(0x00C1A488)));
    case 0x11A: return Percentage(float(Global(0x00C1A490)), float(Global(0x00C1A494) + Global(0x00C1A490)));
    case 0x11B: return Percentage(float(Global(0x00C1A498)), float(Global(0x00C1A49C) + Global(0x00C1A498)));
    case 0x11C: return Percentage(float(Global(0x00C1A4A0)), float(Global(0x00C1A4A4) + Global(0x00C1A4A0)));
    case 0x11D: return Percentage(float(Global(0x00C1A4A8)), float(Global(0x00C1A4AC) + Global(0x00C1A4A8)));
    case 0x11E: return Percentage(float(Global(0x00C1A4B0)), float(Global(0x00C1A4B4) + Global(0x00C1A4B0)));
    case 0x11F: return PercentOf(0x00C1A4C8, 0x00C1A4C4);
    case 0x120: return PercentOf(0x00C1A4D0, 0x00C1A4CC);
    case 0x121: return PercentOf(0x00C1A4D8, 0x00C1A4D4);
    case 0x122: return PercentOf(0x00C1A4E0, 0x00C1A4DC);
    case 0x123: return PercentOf(0x00C1A4E8, 0x00C1A4E4);
    case 0x124: return PercentOf(0x00C1A4F4, 0x00C1A4F0);
    case 0x125: return PercentOf(0x00C1A500, 0x00C1A4FC);
    case 0x126: return PercentOf(0x00C1A50C, 0x00C1A508);
    case 0x127: return PercentOf(0x00C1A518, 0x00C1A514);
    case 0x128: return PercentOf(0x00C1A520, 0x00C1A51C);
    case 0x129: return PercentOf(0x00C1A528, 0x00C1A524);
    case 0x12A: return PercentOf(0x00C1A530, 0x00C1A52C);
    case 0x12B: return PercentOf(0x00C1A538, 0x00C1A534);
    case 0x12F: {
        float whole = float(Global(0x00C1A5D0) + GetIntStat(0x132));
        return Percentage(float(GetIntStat(0x132)), whole);
    }
    case 0x135: return PercentGroup(0x133, 0x134);
    case 0x138: return PercentGroup(0x136, 0x137);
    case 0x13B: return PercentGroup(0x139, 0x13A);
    case 0x13E: return PercentGroup(0x13C, 0x13D);
    case 0x141: return PercentGroup(0x13F, 0x140);
    case 0x144: return PercentGroup(0x142, 0x143);
    case 0x147: return PercentGroup(0x145, 0x146);
    case 0x14A: return PercentGroup(0x148, 0x149);
    case 0x14D: return PercentGroup(0x14B, 0x14C);
    case 0x150: return PercentGroup(0x14E, 0x14F);
    case 0x153: return PercentStats(0x151, 0x152);
    case 0x154: return PercentOf(0x00C1A5D8, 0x00C1A5D4);
    case 0x156: return PercentOf(0x00C1A5EC, 0x00C1A5E8);
    case 0x158: return PercentOf(0x00C1A5FC, 0x00C1A5F8);
    case 0x15C: return PercentStats(0x159, 0x15A);
    case 0x15E: return PercentOf(0x00C1A60C, 0x00C1A608);
    case 0x163: return PercentStats(0x160, 0x161);
    case 0x164: return PercentOf(0x00C1A684, 0x00C1A680);
    case 0x169: return PercentStats(0x167, 0x168);
    case 0x170: return PercentStats(0x16D, 0x16E);
    case 0x174: return PercentStats(0x171, 0x172);
    case 0x178: return PercentOf(0x00C1A6F0, 0x00C1A6EC);
    case 0x179: return PercentStats(0x175, 0x176);
    case 0x17D: result = PercentStats(0x17A, 0x17B); break;
    }
    return result;
}

// 0x459A00 (cdecl): an int stat, stored or derived
int32_t __cdecl GetStatInt(int id) {
    if (id >= 0x104) {
        return GetIntStat(id);
    }
    return ws_stats[id];
}

// 0x45A900 (cdecl): a float stat, stored or derived
float __cdecl GetStatFloat(int id) {
    if (id >= 0x104) {
        return GetFloatStat(id);
    }
    return reinterpret_cast<const float*>(ws_stats)[id];
}

// ---- the stats menu's layout --------------------------------------------------------------

void AddLayoutItem(uint16_t statId, uint8_t kind, const char* name) {
    uint8_t* item = ws_layoutItems + ws_numLayoutItems * 8;
    *reinterpret_cast<uint16_t*>(item) = statId;
    item[2] = kind;
    uint32_t hash = HashName(name);
    uint16_t n = ws_numLayoutItems;
    *reinterpret_cast<uint32_t*>(ws_layoutItems + n * 8 + 4) = hash;
    ws_numLayoutItems = uint16_t(n + 1);
}

// 0x459150 (cdecl): a heading
void __cdecl Stats_AddHeading(const char* name) {
    AddLayoutItem(0xFFFF, 0, name);
}
// 0x4591A0 (cdecl): a sub-heading
void __cdecl Stats_AddSubHeading(const char* name) {
    AddLayoutItem(0xFFFF, 1, name);
}
// 0x4591F0 (cdecl): a stat
void __cdecl Stats_AddStat(const char* name, int statId) {
    AddLayoutItem(uint16_t(statId), 2, name);
}

// ---- save and load --------------------------------------------------------------------------

// 0x4592C0 (cdecl): into the save block (a header, the stored stats, the baselines)
void __cdecl Stats_Save(int32_t* block) {
    block[0] = 1;
    block[1] = 0x5BC;
    block[2] = 0x104;
    block[3] = 0x26;
    int32_t* to = block + 4;
    for (int32_t* from = ws_stats; reinterpret_cast<uintptr_t>(from) < 0x00C1A748; from++, to++) {
        Word_Copy(to, from);
    }
    to = block + 0x108;
    for (int32_t* from = ws_baseline; reinterpret_cast<uintptr_t>(from) < 0x00C19398; from++, to++) {
        Word_Copy(to, from);
    }
}

// 0x459330 (cdecl): from the save block, once its header checks out
void __cdecl Stats_Load(const int32_t* block) {
    if (!Save_CheckValue(0, 0, block[0], 1) || !Save_CheckValue(0, 0, block[1], 0x5BC) ||
        !Save_CheckValue(0, 0, block[2], 0x104) || !Save_CheckValue(0, 0, block[3], 0x26) ||
        !Save_CheckBlock(reinterpret_cast<const char*>(0x0090A320), const_cast<int32_t*>(block) + 0x12E, 0x41)) {   // "StatsSaveData"
        return;
    }
    std::memcpy(ws_stats, block + 4, 0x104 * 4);
    std::memcpy(ws_baseline, block + 0x108, 0x26 * 4);
}

// ---- init -------------------------------------------------------------------------------------

// 0x4593F0 (cdecl): everything cleared, the stats' kinds, the tracked and grouped ids,
// the mission group names
void __cdecl Stats_Init() {
    std::memset(ws_stats, 0, 0x410);
    std::memset(ws_statKinds, 0, 0x17E);
    std::memset(ws_layoutItems, 0, 0xDD0);
    std::memset(ws_baseline, 0, 0x98);
    std::memset(ws_baselineIds, 0, 0x4C);
    auto kinds = [](uint8_t kind, std::initializer_list<uint32_t> ids) {
        for (uint32_t id : ids) {
            ws_statKinds[id] = kind;
        }
    };
    kinds(1, {0x01, 0x10B, 0x10C, 0x10D, 0x118, 0x117});
    kinds(4, {0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B});
    ws_numLayoutItems = 0;
    kinds(3, {0x105, 0x107, 0x112, 0x113, 0x114, 0x115, 0x116, 0x119, 0x11A, 0x11B, 0x11C, 0x11D, 0x11E,
              0x11F, 0x120, 0x121, 0x122, 0x123, 0x124, 0x125, 0x126, 0x127, 0x128, 0x129, 0x12A, 0x12B,
              0x12F, 0x135, 0x138, 0x13B, 0x13E, 0x141, 0x144, 0x147, 0x14A, 0x14D, 0x150, 0x153, 0x154,
              0x156, 0x158, 0x15C, 0x15E, 0x163, 0x164, 0x169, 0x170, 0x174, 0x178, 0x179, 0x17D});
    kinds(5, {0x00, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0xAB, 0xAF, 0xB3, 0xB6, 0xD8, 0xDB, 0xDE, 0xE1,
              0xE3, 0xE5});
    kinds(6, {0x02, 0x9C, 0xAE, 0xB2, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC,
              0xFD});
    kinds(7, {0x10F, 0x130, 0x131, 0x12C, 0x12D});
    static const uint16_t baselineIds[0x26] = {
        0x2E, 0x1F, 0xA6, 0x134, 0x143, 0x137, 0x13A, 0x140, 0x13D, 0xA9, 0xAA, 0xCD, 0, 0, 0xC6, 0x22,
        0, 0x23, 0x9A, 0x9E, 0x92, 0xA0, 0x94, 0xA2, 0x96, 0xA4, 0x98, 0x99, 0, 0x19, 0x1A, 0x16, 0x14,
        0x15, 0, 0, 0x12E, 0x18};
    for (int i = 0; i < 0x26; i++) {
        if (baselineIds[i]) {
            ws_baselineIds[i] = baselineIds[i];
        }
    }
    // (3, 4, 5 and 9 left as they were)
    ws_groupIds[6] = 0x142;
    ws_groupIds[2] = 0x139;
    ws_groupIds[0] = 0x133;
    ws_groupIds[8] = 0x13F;
    ws_groupIds[1] = 0x136;
    ws_groupIds[7] = 0x13C;
    for (int i = 0; i < 12; i++) {
        ws_classIds[i] = uint16_t(0xF2 + i);
    }
    static const uint16_t minigameIds[6] = {0x87, 0x8F, 0x91, 0x8D, 0x8B, 0x89};
    for (int i = 0; i < 6; i++) {
        ws_minigameIds[i] = minigameIds[i];
    }
    // "Class_Dodgeball" .. "Class_Geography", "2_R03_GROUP" .. "MG_SoccerPenalty"
    static const uintptr_t names[27] = {
        0x0090A4A0, 0x0090A490, 0x0090A480, 0x0090A46C, 0x0090A45C, 0x0090A450, 0x0090A444,
        0x0090A438, 0x0090A428, 0x0090A41C, 0x0090A410, 0x0090A400, 0x0090A3F4, 0x0090A3E4,
        0x0090A3D4, 0x0090A3C8, 0x0090A3BC, 0x0090A3AC, 0x0090A39C, 0x0090A38C, 0x0090A37C,
        0x0090A374, 0x0090A368, 0x0090A35C, 0x0090A350, 0x0090A344, 0x0090A330};
    for (int i = 0; i < 27; i++) {
        Name_Set(Name(ws_names + i * 4), reinterpret_cast<const char*>(names[i]));
    }
    ws_lastTime = ws_timeInMs;
    ws_tracking = 0;
}

// ---- tracking ---------------------------------------------------------------------------------

// 0x45A630 (cdecl): a tracked stat's change since its baseline
int32_t __cdecl Stats_GetChange(int index) {
    return IntStat(ws_baselineIds[index]) - ws_baseline[index];
}

// 0x45A670 (cdecl): time in a class, by the teacher ped's model
void __cdecl Stats_AddClassTime(uint8_t* ped) {
    if (!ws_tracking) {
        return;
    }
    int32_t elapsed = int32_t(ws_timeInMs - ws_lastTime);
    int8_t index = int8_t(ped[0x10E] - ws_firstPedModel);
    Stats_AddTime(ws_classIds[index], elapsed);
}

// 0x45A6B0 (cdecl): the play time (and time in 0x20C35E8's state)
void __cdecl Stats_UpdateTime() {
    uint32_t now = ws_timeInMs;
    int32_t elapsed = int32_t(now - ws_lastTime);
    ws_lastTime = now;
    if (!ws_tracking) {
        return;
    }
    ws_stats[2] += elapsed;
    if (Fn6A8D20(reinterpret_cast<void*>(0x020C35E8))) {
        ws_stats[0x9C] += elapsed;
    }
}

// ---- best of ----------------------------------------------------------------------------------

// 0x45A6F0 (cdecl): the class with the highest stat (1-based; 0 none above 0)
uint8_t __cdecl Stats_BestClassImpl() {
    int32_t best = 0;
    uint8_t which = 0;
    for (uint8_t i = 0; i < 12; i++) {
        int32_t value = IntStat(ws_classIds[i]);
        if (value > best) {
            best = value;
            which = uint8_t(i + 1);
        }
    }
    return which;
}

// 0x45A740 (cdecl): the group with the highest stat (0 on a tie for it)
uint8_t __cdecl Stats_BestGroupImpl() {
    int32_t ties = 0;
    int32_t best = 0;
    uint8_t which = 0;
    for (uint8_t i = 0; i < 10; i++) {
        int32_t value = IntStat(ws_groupIds[i]);
        if (value > best) {
            which = uint8_t(i + 1);
            best = value;
            ties = 0;
        } else if (value == best) {
            ties++;
        }
    }
    return ties > 0 ? 0 : which;
}

// 0x45A7C0 (cdecl): the group with the lowest stat (0 on a tie for it)
uint8_t __cdecl Stats_WorstGroupImpl() {
    int32_t ties = 0;
    int32_t worst = IntStat(ws_groupIds[0]);
    uint8_t which = 1;
    for (uint8_t i = 1; i < 10; i++) {
        int32_t value = IntStat(ws_groupIds[i]);
        if (value < worst) {
            which = uint8_t(i + 1);
            ties = 0;
            worst = value;
        } else if (value == worst) {
            ties++;
        }
    }
    return ties > 0 ? 0 : which;
}

// 0x45A860 (cdecl): the minigame with the highest stat (0 on a tie for it)
uint8_t __cdecl Stats_BestMinigameImpl() {
    int32_t ties = 0;
    int32_t best = 0;
    uint8_t which = 0;
    for (uint8_t i = 0; i < 6; i++) {
        Fn488B10(Fn488B30(i));   // (its result unused)
        int32_t value = IntStat(ws_minigameIds[i]);
        if (value > best) {
            best = value;
            which = uint8_t(i + 1);
            ties = 0;
        } else if (value == best) {
            ties++;
        }
    }
    return ties > 0 ? 0 : which;
}

// 0x459240 (cdecl): the class with the lowest grade (0 on a tie for it)
uint8_t __cdecl Stats_WorstClassGradeImpl() {
    int32_t ties = 0;
    int32_t worst = Player_Fn446780(ws_player, 1);
    uint8_t which = 1;
    int index = 1;
    for (uint8_t i = 1; i < 6; i++, index++) {
        int32_t value = Player_Fn446780(ws_player, Fn488B10(Fn488B30(index)));
        if (value < worst) {
            which = uint8_t(i + 1);
            worst = value;
            ties = 0;
        } else if (value == worst) {
            ties++;
        }
    }
    return ties > 0 ? 0 : which;
}

// 0x45A920 (cdecl): a "best of" stat's text (nullptr for other ids)
const char* __cdecl Stats_GetBestName(int id) {
    switch (id) {
    case 0x10F: return reinterpret_cast<const re::Ptr32<const char>*>(0x00A149EC)[Stats_BestClass()];
    case 0x12C: return reinterpret_cast<const re::Ptr32<const char>*>(0x00A14A20)[Stats_BestMinigame()];
    case 0x12D: return reinterpret_cast<const re::Ptr32<const char>*>(0x00A14A20)[Stats_WorstClassGrade()];
    case 0x130: return reinterpret_cast<const re::Ptr32<const char>*>(0x00A149C0)[Stats_BestGroup()];
    case 0x131: return reinterpret_cast<const re::Ptr32<const char>*>(0x00A149C0)[Stats_WorstGroup()];
    default: return nullptr;
    }
}

// 0x45A9D0 (cdecl): every tracked stat's baseline set to its value now (the bits as
// they are: ints for kinds 0, 1, 5, 6; floats for 2 and 4)
void __cdecl Stats_MarkBaselines() {
    for (int i = 0; i < 0x26; i++) {
        uint32_t id = ws_baselineIds[i];
        uint8_t kind = ws_statKinds[id];
        if (kind == 0 || kind == 1 || kind == 5 || kind == 6) {
            ws_baseline[i] = IntStat(id);
        } else if (kind == 2 || kind == 4) {
            float value = id < 0x104 ? reinterpret_cast<const float*>(ws_stats)[id] : GetFloatStat(int(id));
            reinterpret_cast<float*>(ws_baseline)[i] = value;
        }
    }
}


// ---- distances travelled ----------------------------------------------------------------------

GAME_FN(int32_t, __thiscall, Fn4F12F0, 0x004F12F0, void* self);
GAME_FN(uint8_t, __thiscall, Fn415380, 0x00415380, void* self);
GAME_FN(uint8_t, __thiscall, Ped_IsInVehicle, 0x004CE690, void* ped);
GAME_FN(int32_t*, __thiscall, Vehicle_GetKind, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(uint8_t, __thiscall, Player_HasAward, 0x00446860, void* self, int award);
GAME_FN(void, __thiscall, Player_SetAward, 0x004467F0, void* self, int award, int);
GAME_FN(void, __cdecl, Stats_AddClassTimeGame, 0x0045A670, void* entity);
GAME_VAR(uint8_t, ws_c2b109, 0x00C2B109);
GAME_VAR(void*, ws_9e4f4c, 0x009E4F4C);

float& FloatStat(int index) {
    return reinterpret_cast<float*>(ws_stats)[index];
}

// 0x45AA60 (cdecl): this frame's distance (the player's or the vehicle's +0x19C) onto the
// walked / ridden / driven stats, and the 100 km awards
void __cdecl Stats_UpdateDistances() {
    if (!ws_tracking || Fn4F12F0(reinterpret_cast<void*>(0x00C3CC68)) == 1 || Fn415380(ws_9e4f4c)) {
        return;
    }
    uint8_t* player = *reinterpret_cast<re::Ptr32<uint8_t>*>(ws_player);
    if (Ped_IsInVehicle(player)) {
        uint8_t* vehicle = *reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x1554);
        if (!vehicle) {
            return;
        }
        int32_t type = *reinterpret_cast<int32_t*>(vehicle + 0x46C);
        float moved = *reinterpret_cast<float*>(vehicle + 0x19C);
        int32_t kind;
        if (type == 1) {   // a bike
            if (*Vehicle_GetKind(vehicle, &kind) == 8) {
                FloatStat(9) = moved + FloatStat(9);
                return;
            }
            Stats_AddClassTimeGame(vehicle);
            FloatStat(7) = *reinterpret_cast<float*>(vehicle + 0x19C) + FloatStat(7);
            if (!Player_HasAward(ws_player, 9) && FloatStat(7) > 100000.0f) {
                Player_SetAward(ws_player, 9, 1);
            }
        } else if (type == 0) {
            int32_t k = *Vehicle_GetKind(vehicle, &kind);
            if (k == 6) {
                FloatStat(10) = moved + FloatStat(10);
            } else if (k == 7) {
                FloatStat(8) = moved + FloatStat(8);
            }
        }
        return;
    }
    float moved = *reinterpret_cast<float*>(player + 0x19C);
    if (ws_c2b109) {
        FloatStat(11) = moved + FloatStat(11);
        return;
    }
    if (*reinterpret_cast<int32_t*>(player + 0x1470)) {
        FloatStat(6) = moved + FloatStat(6);
        return;
    }
    FloatStat(5) = moved + FloatStat(5);
    if (!Player_HasAward(ws_player, 10) && FloatStat(5) > 100000.0f) {
        Player_SetAward(ws_player, 10, 1);
    }
}

// ---- showing a stat ---------------------------------------------------------------------------

inline void* const ws_text = reinterpret_cast<void*>(0x020C2FA4);
GAME_FN(void, __cdecl, Text_PushNumber, 0x006880D0, float value);
GAME_FN(void, __cdecl, Text_PushString, 0x006880F0, const void* text);
GAME_FN(const void*, __thiscall, Text_Get, 0x0068FE80, void* self, const char* key);
GAME_FN(void, __cdecl, Text_Print, 0x00688ED0, uintptr_t, const void* text, int);
GAME_FN(int16_t, __thiscall, Classes_Fn426430, 0x00426430, void* self, int);
GAME_FN(int32_t, __cdecl, GetStatIntGame, 0x00459A00, int id);
GAME_FN(float, __cdecl, GetStatFloatGame, 0x0045A900, int id);
GAME_FN(const char*, __cdecl, Stats_GetBestNameGame, 0x0045A920, int id);
GAME_FN(void, __thiscall, Stats_ShowGame, 0x0045AC10, const uint8_t* kind, int id, int, int);

void Print(const char* key, int a, int b) {
    Text_Print(a, Text_Get(ws_text, key), b);
}

// 0x45AC10 (thiscall on the stat's kind byte): the stat's value(s) pushed, then printed
// with the kind's text
void __fastcall Stats_Show(const uint8_t* kind, re::Edx, int id, int a, int b) {
    switch (*kind) {
    case 0:
        Text_PushNumber(float(GetStatIntGame(id)));
        Print(reinterpret_cast<const char*>(0x0090A514), a, b);   // "STAT_INT"
        return;
    case 1: {   // out of a fixed or known total
        Text_PushNumber(float(GetStatIntGame(id)));
        int32_t total = -1;
        if (id > 0x10D) {
            if (id == 0x117) {
                total = 5;
            } else if (id == 0x118) {
                total = 11;
            }
        } else if (id == 0x10D) {
            total = Classes_Fn426430(ws_classMgr, 3);
        } else if (id == 1) {
            total = 40;
        } else if (id == 0x10B) {
            total = Classes_Fn426430(ws_classMgr, 1);
        } else if (id == 0x10C) {
            total = Classes_Fn426430(ws_classMgr, 2);
        }
        Text_PushNumber(float(total));
        Print(reinterpret_cast<const char*>(0x0090A508), a, b);   // "STAT_RATIO"
        return;
    }
    case 2:
        Text_PushNumber(GetStatFloatGame(id));
        Print(reinterpret_cast<const char*>(0x0090A4FC), a, b);   // "STAT_FLOAT"
        return;
    case 3:
        Text_PushNumber(GetStatFloatGame(id));
        Print(reinterpret_cast<const char*>(0x0090A4D4), a, b);   // "STAT_PCT"
        return;
    case 4:
        Text_PushNumber(float(GetStatFloatGame(id) / 1000.0));
        Print(reinterpret_cast<const char*>(0x0090A4EC), a, b);   // "STAT_DISTANCE"
        return;
    case 5:
        Text_PushNumber(float(GetStatIntGame(id)));
        Print(reinterpret_cast<const char*>(0x0090A4E0), a, b);   // "STAT_MONEY"
        return;
    case 6: {   // a time (ms): days (their low byte), hours, minutes (low bytes), seconds
        uint32_t seconds = uint32_t(GetStatIntGame(id)) / 1000;
        uint32_t days = seconds / 86400;
        uint32_t rest = seconds - days * 86400;
        uint32_t hours = rest / 3600;
        rest -= hours * 3600;
        uint32_t minutes = rest / 60;
        uint32_t secs = rest - minutes * 60;
        uint8_t dayByte = uint8_t(days);
        if (dayByte) {
            Text_PushNumber(float(int32_t(dayByte)));
        }
        Text_PushNumber(float(int32_t(uint8_t(hours))));
        Text_PushNumber(float(int32_t(uint8_t(minutes))));
        Text_PushNumber(float(double(secs)));
        Print(reinterpret_cast<const char*>(dayByte ? 0x0090A4BC : 0x0090A4C8), a, b);   // "STAT_TIME_D" / "STAT_TIME"
        return;
    }
    case 7:
        Text_PushString(Text_Get(ws_text, Stats_GetBestNameGame(id)));
        Print(reinterpret_cast<const char*>(0x0090A4B0), a, b);   // "STAT_STRING"
        return;
    }
}

// 0x45AFD0 (thiscall on a layout item: +0 the stat id, +2 its kind): a stat's line
// (headings, kinds 0 and 1, print nothing)
void __fastcall Stats_ShowItem(const uint8_t* item, re::Edx, int a, int b) {
    uint8_t kind = item[2];
    if (kind == 0 || kind == 1) {
        return;
    }
    uint16_t id = *reinterpret_cast<const uint16_t*>(item);
    Stats_ShowGame(ws_statKinds + id, id, a, b);
}

#define C04E_STATS(name, address, fn)                                                          \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04E_STATS("CStats::GetDerivedInt", 0x00458640, &GetIntStatImpl);
C04E_STATS("CStats::AddHeading", 0x00459150, &Stats_AddHeading);
C04E_STATS("CStats::AddSubHeading", 0x004591A0, &Stats_AddSubHeading);
C04E_STATS("CStats::AddStat", 0x004591F0, &Stats_AddStat);
C04E_STATS("CStats::WorstClassGrade", 0x00459240, &Stats_WorstClassGradeImpl);
C04E_STATS("CStats::Save", 0x004592C0, &Stats_Save);
C04E_STATS("CStats::Load", 0x00459330, &Stats_Load);
C04E_STATS("CStats::Init", 0x004593F0, &Stats_Init);
C04E_STATS("CStats::GetStatInt", 0x00459A00, &GetStatInt);
C04E_STATS("CStats::GetDerivedFloat", 0x00459A20, &GetFloatStatImpl);
C04E_STATS("CStats::GetChange", 0x0045A630, &Stats_GetChange);
C04E_STATS("CStats::AddClassTime", 0x0045A670, &Stats_AddClassTime);
C04E_STATS("CStats::UpdateTime", 0x0045A6B0, &Stats_UpdateTime);
C04E_STATS("CStats::BestClass", 0x0045A6F0, &Stats_BestClassImpl);
C04E_STATS("CStats::BestGroup", 0x0045A740, &Stats_BestGroupImpl);
C04E_STATS("CStats::WorstGroup", 0x0045A7C0, &Stats_WorstGroupImpl);
C04E_STATS("CStats::BestMinigame", 0x0045A860, &Stats_BestMinigameImpl);
C04E_STATS("CStats::GetStatFloat", 0x0045A900, &GetStatFloat);
C04E_STATS("CStats::GetBestName", 0x0045A920, &Stats_GetBestName);
C04E_STATS("CStats::MarkBaselines", 0x0045A9D0, &Stats_MarkBaselines);
C04E_STATS("CStats::UpdateDistances", 0x0045AA60, &Stats_UpdateDistances);
C04E_STATS("CStats::Show", 0x0045AC10, &Stats_Show);
C04E_STATS("CStats::ShowItem", 0x0045AFD0, &Stats_ShowItem);

} // namespace
