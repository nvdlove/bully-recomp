// The cheat codes (CheatCodeManager: a pad's last 30 buttons against the table at
// 0x9E5028) and what they do, the unlock lists read from config\cht_*.txt, and the
// debug line (frame rate, position, pool counts). 0x4251D0..0x425B00.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
#define CHEATS_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

// A pad's button history (the pads' at 0x20CECF0, 0x914 apart): +0x8C0 a ring of the
// last 30 buttons, +0x8FC the next slot
inline int32_t& HistoryNext(uint8_t* history) { return *reinterpret_cast<int32_t*>(history + 0x8FC); }
inline int16_t* HistoryButtons(uint8_t* history) { return reinterpret_cast<int16_t*>(history + 0x8C0); }

// A cheat (0x24): its buttons (-1 ends them), newest last, and what it does
struct CheatEntry {
    int16_t code[16];   // +0
    re::Ptr32<void __cdecl()> fn; // +0x20
};
static_assert(sizeof(CheatEntry) == 0x24);
GAME_ARRAY(CheatEntry, ch_cheats, 0x009E5028);  // [8]

struct CheatCodeManager {
    re::Ptr32<re::Ptr32<void>> vtable;     // +0
    uint8_t used;      // +4 a cheat was entered
    uint8_t unknown5[3];
};
static_assert(sizeof(CheatCodeManager) == 8);

// an input event (what 0x425240 is given)
struct PadEvent {
    int32_t unknown0;  // +0
    int32_t pad;       // +4
    float value;       // +8
    int32_t type;      // +0xC (0x10..0x13 aren't buttons)
};

GAME_VAR(float, ch_messageUntil, 0x00BCE8EC);  // (seconds)
inline uint8_t* const ch_message = reinterpret_cast<uint8_t*>(0x00BCE8F0);
GAME_VAR(uint8_t, ch_debugLine, 0x00BCEAF3);
GAME_VAR(uint32_t, ch_debugLineUntil, 0x00BCEB18);
GAME_VAR(float, ch_debugLastTime, 0x00BCEB38);
GAME_VAR(uint8_t, ch_flagBCEAF0, 0x00BCEAF0);
GAME_VAR(uint8_t, ch_flagBCEAF1, 0x00BCEAF1);
GAME_VAR(uint8_t, ch_flagBCEAF2, 0x00BCEAF2);
GAME_VAR(uint8_t, ch_flagBCEB1E, 0x00BCEB1E);
GAME_VAR(uint8_t, ch_padSwap, 0x00BCEB1C);
GAME_VAR(uint8_t, ch_padSwapIndex, 0x00BCEB1D);
GAME_VAR(int32_t, ch_valueBCEB00, 0x00BCEB00);
inline char* const ch_nameBCEB08 = reinterpret_cast<char*>(0x00BCEB08);   // [16]
GAME_VAR(uint8_t, ch_menuPaused, 0x020C5BE6);
GAME_VAR(uint8_t, ch_pausedC1A998, 0x00C1A998);
GAME_VAR(uint8_t, ch_pausedC1A999, 0x00C1A999);
GAME_VAR(uint8_t, ch_pausedC1A99A, 0x00C1A99A);
GAME_VAR(uint32_t, ch_timeMs, 0x00C1A9B4);
GAME_VAR(float, ch_timeMsFloat, 0x00C1A9B0);
GAME_VAR(int32_t, ch_frozenTimeMs, 0x00A14A50);
GAME_VAR(uint8_t*, ch_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, ch_firstWeapon, 0x00A136B0);
GAME_VAR(int32_t, ch_lastWeapon, 0x00A136B4);
GAME_VAR(uint8_t*, ch_weaponHud, 0x00C674F0);
GAME_VAR(void*, ch_camera, 0x00C3CCF4);
GAME_VAR(int32_t, ch_textFlag208B21C, 0x0208B21C);
GAME_VAR(int16_t, ch_text208B784, 0x0208B784);
GAME_VAR(int16_t, ch_text208B788, 0x0208B788);
GAME_ARRAY(uint8_t, ch_textColour, 0x0208B764);   // [4]
// the cheat car models' table (0xBCE8A0) and where they come from
GAME_ARRAY(int32_t, ch_carModels, 0x00BCE8A0);
GAME_VAR(int32_t, ch_seasonA, 0x00A136C8);
GAME_VAR(int32_t, ch_seasonB, 0x00A136CC);
GAME_ARRAY(int32_t, ch_seasonCopy, 0x00BCE8E0);   // [3]
inline void* const ch_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const ch_textTable = reinterpret_cast<void*>(0x020C2FA4);
inline uint8_t* const ch_padHistories = reinterpret_cast<uint8_t*>(0x020CECF0);

GAME_FN(uint8_t, __thiscall, CH_MatchesCode, 0x004251D0, uint8_t* history, const int16_t* code);
GAME_FN(int, __cdecl, CH_GetActivePad, 0x00738070);
GAME_FN(int, __cdecl, CH_GetOtherPad, 0x00738080);
GAME_FN(void, __thiscall, CH_Nop435A00, 0x00435A00, uint8_t* history, int pad, int arg);
GAME_FN(void, __thiscall, CH_Ped_SetHealth, 0x004782E0, uint8_t* ped, float health);
GAME_FN(void, __thiscall, CH_HashString_Construct, 0x005FA910, uint32_t* s, const char* text);
GAME_FN(void, __thiscall, CH_HashString_Destruct, 0x0049A650, uint32_t* s);
GAME_FN(int32_t, __thiscall, CH_MissionMgr_Find, 0x006A9FD0, void* mgr, uint32_t* name);
GAME_FN(uint8_t*, __thiscall, CH_MissionMgr_GetTaskFlags, 0x006AA7C0, void* mgr, int32_t index);
GAME_FN(void*, __cdecl, CH_FileOpen, 0x0042D260, const char* name, const char* mode, int flag);
GAME_FN(uint8_t, __cdecl, CH_FileReadLine, 0x0042D330, void* file, char* buffer, int size);
GAME_FN(void, __cdecl, CH_FileClose, 0x0042D450, void* file);
GAME_FN(void, __cdecl, CH_ResetText, 0x00688E60);
GAME_FN(void, __cdecl, CH_PrintMessage, 0x0068D610, int a, const uint8_t* text, int b);
GAME_FN(void, __cdecl, CH_Fn68CDF0, 0x0068CDF0);
GAME_FN(const uint8_t*, __thiscall, CH_GetText, 0x0068FE80, void* table, const char* key);
GAME_FN(void, __cdecl, CH_SetMessage, 0x0068FFD0, uint8_t* dest, const uint8_t* text, int alpha);
GAME_FN(void, __thiscall, CH_Inventory_Rewind, 0x007DB6B0, void* inventory);
GAME_FN(int16_t*, __thiscall, CH_Inventory_Next, 0x0045CDB0, void* inventory);
GAME_FN(uint8_t*, __cdecl, CH_GetWeaponInfo, 0x0051B450, int32_t weapon);
GAME_FN(void, __thiscall, CH_WeaponInfo_Fn7282A0, 0x007282A0, uint8_t* info);
GAME_FN(void, __thiscall, CH_Inventory_Give, 0x0045D030, void* inventory, int32_t weapon, int ammo, int a, int b);
GAME_FN(void, __thiscall, CH_Ped_AddMoney, 0x00475320, uint8_t* ped, int32_t amount);
GAME_FN(uint8_t*, __cdecl, CH_GetStreamingInfo, 0x0052DD30, int32_t model);
GAME_FN(void, __cdecl, CH_RequestModel, 0x00532020, int32_t model, int flags);
GAME_FN(void, __cdecl, CH_LoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, CH_CheatMessage, 0x004257F0);
GAME_FN(void, __cdecl, CH_UnlockMoves, 0x00425460, int count);
GAME_FN(void, __cdecl, CH_UnlockWrestling, 0x00425570, int count);
GAME_FN(int32_t, __cdecl, CH_GetTimeMs, 0x00425740);
GAME_FN(void, __cdecl, CH_Fn688050, 0x00688050, int a);
GAME_FN(void, __cdecl, CH_Fn688160, 0x00688160, int a);
GAME_FN(void*, __cdecl, CH_CameraObject, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, CH_WorldTransform, 0x006C9A90, void* object);
template <typename... A>
inline int __cdecl CH_sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(uint8_t*, __thiscall, CH_RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, CH_AsciiToText, 0x00690090, const char* ascii, uint16_t* text);
GAME_FN(void, __cdecl, CH_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int a);
inline void __cdecl CH_strncpy(char* dest, const char* src, size_t n) { crt::Strncpy(dest, src, n); }

float TimeSeconds() {
    // (the time as unsigned, through int with 2^32 added when negative)
    float t = float(double(int32_t(ch_timeMs)) + (int32_t(ch_timeMs) < 0 ? 4294967296.0 : 0.0));
    return float(t * double(0.001f));
}

// the unlock lists: each line (but "Level") a mission whose flag 8 is set for the
// first `count` of them
void UnlockFromFile(void* file, int count, bool stripSecondChar) {
    // (a byte before the buffer: 0x425460 reads it for an empty line)
    char storage[4 + 0x100];
    char* line = storage + 4;
    int index = 0;
    while (CH_FileReadLine(file, line, 0x100)) {
        if (stripSecondChar) {
            // (0x425570 tests the second character, not the last)
            if (line[1] == '\n') {
                line[0] = 0;
            }
        } else {
            size_t len = std::strlen(line);
            if (line[int(len) - 1] == '\n') {
                line[int(len) - 1] = 0;
            }
        }
        if (std::memcmp(line, "Level", 6) == 0) {
            continue;
        }
        uint32_t name;
        CH_HashString_Construct(&name, line);
        int32_t mission = CH_MissionMgr_Find(ch_missionMgr, &name);
        if (mission >= 0) {
            uint8_t* flags = CH_MissionMgr_GetTaskFlags(ch_missionMgr, mission);
            flags[4] ^= (uint8_t((index < count) << 3) ^ flags[4]) & 8;
        }
        index++;
        CH_HashString_Destruct(&name);
    }
}

} // namespace

namespace Cheats {

// 0x4251D0 (thiscall, on a pad's history): the code's buttons are the last ones
// pressed, newest first
uint8_t __fastcall MatchesCode(uint8_t* history, re::Edx, const int16_t* code) {
    int32_t slot = HistoryNext(history) != 0 ? HistoryNext(history) - 1 : 0x1D;
    int i = 0;
    if (code[0] == HistoryButtons(history)[slot]) {
        do {
            i++;
            slot = slot != 0 ? slot - 1 : 0x1D;
        } while (code[i] == HistoryButtons(history)[slot]);
    }
    return code[i] == -1;
}

// 0x425230: the cheat message's time over
void __cdecl ClearMessage() {
    ch_messageUntil = 0.0f;
}

// 0x425240 (thiscall): a button pressed on another pad than the active one: every
// cheat whose code it completes runs (none while paused)
uint8_t __fastcall OnButton(CheatCodeManager* self, re::Edx, const PadEvent* e) {
    int32_t type = e->type;
    if (type >= 0x10 && type <= 0x13) {
        return 0;
    }
    int32_t pad = e->pad;
    if (pad == CH_GetActivePad()) {
        return 0;
    }
    if (!(0.0f < e->value)) {
        return 0;
    }
    uint8_t* history = ch_padHistories + pad * 0x914;
    for (CheatEntry* c = &ch_cheats[0]; reinterpret_cast<uintptr_t>(&c->fn) < 0x009E5168; c++) {
        if (!CH_MatchesCode(history, c->code)) {
            continue;
        }
        if (ch_menuPaused || ch_pausedC1A99A || ch_pausedC1A999 || ch_pausedC1A998) {
            return 0;
        }
        self->used = 1;
        c->fn();
    }
    return 0;
}

// 0x4252D0: the player's 0xBCEAF0 state: full health, and its +0x138C and +0x1344
void __cdecl ApplyFlagBCEAF0() {
    uint8_t* player = ch_playerPed;
    if (!player) {
        return;
    }
    if (ch_flagBCEAF0) {
        CH_Ped_SetHealth(player, *reinterpret_cast<float*>(player + 0x1CA4));
    }
    *reinterpret_cast<uint32_t*>(player + 0x138C) = ch_flagBCEAF0;
    *reinterpret_cast<uint32_t*>(player + 0x1344) = ch_flagBCEAF0;
}

// 0x425320 / 0x425330 / 0x425340 / 0x425700: cheat states
uint8_t __cdecl GetFlagBCEAF0() {
    return ch_flagBCEAF0;
}
uint8_t __cdecl GetFlagBCEAF1() {
    return ch_flagBCEAF1;
}
uint8_t __cdecl GetFlagBCEAF2() {
    return ch_flagBCEAF2;
}
uint8_t __cdecl GetFlagBCEB1E() {
    return ch_flagBCEB1E;
}

// 0x425350: the seasonal pair copied (Ghidra shows these as one value; A, B, A)
void __cdecl CopySeasons() {
    int32_t a = ch_seasonA;
    int32_t b = ch_seasonB;
    ch_seasonCopy[2] = a;
    ch_seasonCopy[1] = b;
    ch_seasonCopy[0] = a;
}

// 0x425370: the cheat cars' models from the model ids (12 of them, then 0s)
void __cdecl InitCarModels() {
    int32_t* m = ch_carModels;
    auto id = [](uintptr_t address) { return *reinterpret_cast<int32_t*>(address); };
    m[0] = id(0x00BF4324);
    m[1] = id(0x00BF4320);
    m[2] = id(0x00BF411C);
    m[3] = id(0x00BF4130);
    m[4] = id(0x00BF448C);
    m[5] = id(0x00BF4454);
    m[6] = id(0x00BF412C);
    m[7] = id(0x00BF4128);
    m[8] = id(0x00BF4124);
    m[9] = id(0x00BF4120);
    m[10] = id(0x00BF4110);
    m[15] = 0;   // 0xBCE8DC
    m[14] = 0xC; // 0xBCE8D8 (the count)
    m[11] = id(0x00BF4314);
    m[12] = id(0x00BF4310);
    m[13] = 0;   // 0xBCE8D4
}

// 0x425430: a name (16 characters) for 0xBCEB08
void __cdecl SetNameBCEB08(const char* name) {
    CH_strncpy(ch_nameBCEB08, name, 0x10);
}

// 0x425450
void __cdecl SetValueBCEB00(int32_t value) {
    ch_valueBCEB00 = value;
}

// 0x425460: the moves listed in config\cht_move.txt unlocked (the first `count`)
void __cdecl UnlockMoves(int count) {
    void* file = CH_FileOpen("config\\cht_move.txt", "r", 1);
    if (!file) {
        return;
    }
    UnlockFromFile(file, count, false);
    CH_FileClose(file);
}

// 0x425570: GrapTestUnlock (with 99) and the wrestling moves of config\cht_wrst.txt
void __cdecl UnlockWrestling(int count) {
    uint32_t grapTest;
    CH_HashString_Construct(&grapTest, "GrapTestUnlock");
    int32_t mission = CH_MissionMgr_Find(ch_missionMgr, &grapTest);
    if (mission >= 0) {
        uint8_t* flags = CH_MissionMgr_GetTaskFlags(ch_missionMgr, mission);
        flags[4] ^= (uint8_t((count == 99) << 3) ^ flags[4]) & 8;
    }
    void* file = CH_FileOpen("config\\cht_wrst.txt", "r", 1);
    if (file) {
        UnlockFromFile(file, count, true);
        CH_FileClose(file);
    }
    CH_HashString_Destruct(&grapTest);
}

// 0x4256B0: with the pads swapped, both pads' histories told (0x435A00 does nothing)
void __cdecl SwapPads() {
    if (!ch_padSwap) {
        return;
    }
    int index = ch_padSwapIndex;
    CH_Nop435A00(ch_padHistories + CH_GetActivePad() * 0x914, index, 0x10);
    index = ch_padSwapIndex;
    CH_Nop435A00(ch_padHistories + CH_GetOtherPad() * 0x914, index, 0x10);
}

// 0x425710: the game's time in seconds
float __cdecl GetTimeSeconds() {
    return TimeSeconds();
}

// 0x425740: the game's time (the float at 0xC1A9B0) in whole milliseconds
int32_t __cdecl GetTimeMs() {
    return int32_t(int64_t(ch_timeMsFloat));
}

// 0x425770 vf0: the destructor (flags 1: and freed)
CheatCodeManager* __fastcall Destroy(CheatCodeManager* self, re::Edx, uint8_t flags) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00900F5C);
    if (flags & 1) {
        reinterpret_cast<void(__cdecl*)(void*)>(0x005EEAB0)(self);
    }
    return self;
}

// 0x425790 (thiscall)
void __fastcall Reset(CheatCodeManager* self, re::Edx) {
    self->used = 0;
    ch_messageUntil = 0.0f;
}

// 0x4257A0: the "cheat" message while its time lasts
void __cdecl DrawMessage() {
    float until = ch_messageUntil;
    if (TimeSeconds() < until) {
        CH_ResetText();
        CH_PrintMessage(0, ch_message, 0);
        CH_Fn68CDF0();
    }
}

// 0x4257F0: the "CHEAT1" message, for half a second
void __cdecl ShowCheatMessage() {
    CH_SetMessage(ch_message, CH_GetText(ch_textTable, "CHEAT1"), 0xFF);
    ch_messageUntil = float(TimeSeconds() + 0.5);
}

// 0x425850: 999 of every weapon's ammo the player has
void __cdecl CheatAmmo() {
    CH_CheatMessage();
    uint8_t* player = ch_playerPed;
    if (!player) {
        return;
    }
    CH_Inventory_Rewind(*reinterpret_cast<re::Ptr32<void>*>(player + 0x1C4));
    for (int16_t* item = CH_Inventory_Next(*reinterpret_cast<re::Ptr32<void>*>(player + 0x1C4)); item;
         item = CH_Inventory_Next(*reinterpret_cast<re::Ptr32<void>*>(player + 0x1C4))) {
        int32_t weapon = item[1];
        uint8_t* info = CH_GetWeaponInfo(weapon);
        CH_WeaponInfo_Fn7282A0(info);
        int32_t ammo = *reinterpret_cast<int32_t*>(info + 0x84);
        if (ammo < 0) {
            ammo = weapon;
        }
        CH_Inventory_Give(*reinterpret_cast<re::Ptr32<void>*>(player + 0x1C4), ammo, 999, 1, 0);
    }
}

// 0x4258D0: 0xBCEAF2 toggled
void __cdecl CheatToggleBCEAF2() {
    ch_flagBCEAF2 = ch_flagBCEAF2 == 0;
    CH_CheatMessage();
}

// 0x4258F0: 10000 money
void __cdecl CheatMoney() {
    CH_CheatMessage();
    if (ch_playerPed) {
        CH_Ped_AddMoney(ch_playerPed, 10000);
    }
}

// 0x425910: every weapon of kind 1 but five
void __cdecl CheatWeapons() {
    CH_CheatMessage();
    uint8_t* player = ch_playerPed;
    if (!player) {
        return;
    }
    for (int32_t w = ch_firstWeapon; w <= ch_lastWeapon; w++) {
        uint8_t* info = CH_GetWeaponInfo(w);
        if (*reinterpret_cast<int32_t*>(info + 0x64) != 1) {
            continue;
        }
        if (w == *reinterpret_cast<int32_t*>(0x00BF4360) || w == *reinterpret_cast<int32_t*>(0x00BF443C) ||
            w == *reinterpret_cast<int32_t*>(0x00BF437C) || w == *reinterpret_cast<int32_t*>(0x00BF43A4) ||
            w == *reinterpret_cast<int32_t*>(0x00BF4430)) {
            continue;
        }
        CH_Inventory_Give(*reinterpret_cast<re::Ptr32<void>*>(player + 0x1C4), w, 1, 1, 0);
    }
    ch_weaponHud[0x15D] = 1;
}

// 0x4259A0: every weapon of kind 2, their models loaded
void __cdecl CheatWeapons2() {
    CH_CheatMessage();
    uint8_t* player = ch_playerPed;
    if (!player) {
        return;
    }
    for (int32_t w = ch_firstWeapon; w <= ch_lastWeapon; w++) {
        uint8_t* info = CH_GetWeaponInfo(w);
        if (*reinterpret_cast<int32_t*>(info + 0x64) != 2) {
            continue;
        }
        CH_Inventory_Give(*reinterpret_cast<re::Ptr32<void>*>(player + 0x1C4), w, 1, 1, 0);
        if (CH_GetStreamingInfo(w)[4] != 1) {
            CH_RequestModel(w, 0x16);
        }
    }
    CH_LoadAllRequestedModels(0);
}

// 0x425A20: a quarter of the maximum health back
void __cdecl CheatHealth() {
    CH_CheatMessage();
    uint8_t* player = ch_playerPed;
    if (!player) {
        return;
    }
    float max = *reinterpret_cast<float*>(player + 0x1CA4);
    float health = float(*reinterpret_cast<float*>(player + 0x1CA4) * 0.25 + *reinterpret_cast<float*>(player + 0x1CB8));
    CH_Ped_SetHealth(player, max <= health ? max : health);
}

// 0x425A90 / 0x425AB0: every move / every wrestling move
void __cdecl CheatUnlockMoves() {
    CH_CheatMessage();
    CH_UnlockMoves(999);
}
void __cdecl CheatUnlockWrestling() {
    CH_CheatMessage();
    CH_UnlockWrestling(999);
}

// 0x425AD0 (thiscall, on a pool): the object of a handle (slot << 8 | its flag byte)
void* __fastcall PoolGetAtRef(re::Ptr32<uint8_t>* pool, re::Edx, int32_t ref) {
    int32_t slot = ref >> 8;
    if (pool[1][slot] == uint8_t(ref)) {
        return pool[0] + *reinterpret_cast<int32_t*>(pool + 3) * slot;
    }
    return nullptr;
}

// 0x425B00: the debug line (0xBCEAF3, until 0xBCEB18's time): the frame rate, the
// player's position, the time, the peds and vehicles
void __cdecl DrawDebugLine() {
    if (!ch_debugLine) {
        return;
    }
    uint8_t* player = ch_playerPed;
    int32_t ms = ch_frozenTimeMs;
    if (!ch_pausedC1A99A && !ch_pausedC1A999 && !ch_pausedC1A998) {
        ms = CH_GetTimeMs();
    }
    float now = float(double(ms) + (ms < 0 ? 4294967296.0 : 0.0));
    float frame = now - ch_debugLastTime;
    ch_debugLastTime = now;
    if (ch_debugLineUntil != 0) {
        float until = float(double(int32_t(ch_debugLineUntil)) + (int32_t(ch_debugLineUntil) < 0 ? 4294967296.0 : 0.0));
        if (until <= now) {
            ch_debugLine = 0;
            ch_debugLineUntil = 0;
            return;
        }
    }
    CH_ResetText();
    if (ch_textFlag208B21C != 0) {
        ch_text208B788 = 1;
        ch_text208B784 = 0;
    }
    CH_Fn688050(5);
    ch_text208B784 = 0;
    CH_Fn688160(0);
    float pos[3] = {0.0f, 0.0f, 0.0f};
    if (player) {
        uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(player + 0x14);
        float* p = matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(player + 4);
        pos[0] = p[0];
        pos[1] = p[1];
        pos[2] = p[2];
    }
    CH_WorldTransform(CH_CameraObject(ch_camera));
    double fps = frame > 0.0f ? 1000.0 / frame : 0.0;
    auto countUsed = [](uintptr_t poolVar) {
        uint8_t* pool = *reinterpret_cast<re::Ptr32<uint8_t>*>(poolVar);
        int size = *reinterpret_cast<int32_t*>(pool + 8);
        uint8_t* flags = *reinterpret_cast<re::Ptr32<uint8_t>*>(pool + 4);
        int used = 0;
        for (int i = 0; i < size; i++) {
            if (!(flags[i] & 0x80)) {
                used++;
            }
        }
        return used;
    };
    int vehicles = countUsed(0x00C0F5F4);
    int peds = countUsed(0x00C0F5F0);
    char line[128];
    CH_sprintf(line, "%5.2f [%7.2f, %7.2f, %7.2f] %7.0f %2d %2d", fps, double(pos[0]), double(pos[1]), double(pos[2]), double(now), peds, vehicles);
    uint8_t rgba[4];
    uint8_t* c = CH_RGBA_Construct(rgba, 100, 0x96, 0x96, 0xFF);
    ch_textColour[0] = c[0];
    ch_textColour[1] = c[1];
    ch_textColour[2] = c[2];
    ch_textColour[3] = c[3];
    uint16_t text[128];
    CH_AsciiToText(line, text);
    CH_PrintString(50.0f, 450.0f, text, 1);
}

CHEATS_REVERSED("CheatCodeManager::MatchesCode", 0x004251D0, &MatchesCode);
CHEATS_REVERSED("Cheats::ClearMessage", 0x00425230, &ClearMessage);
CHEATS_REVERSED("CheatCodeManager::OnButton", 0x00425240, &OnButton);
CHEATS_REVERSED("Cheats::ApplyFlagBCEAF0", 0x004252D0, &ApplyFlagBCEAF0);
CHEATS_REVERSED("Cheats::GetFlagBCEAF0", 0x00425320, &GetFlagBCEAF0);
CHEATS_REVERSED("Cheats::GetFlagBCEAF1", 0x00425330, &GetFlagBCEAF1);
CHEATS_REVERSED("Cheats::GetFlagBCEAF2", 0x00425340, &GetFlagBCEAF2);
CHEATS_REVERSED("Cheats::CopySeasons", 0x00425350, &CopySeasons);
CHEATS_REVERSED("Cheats::InitCarModels", 0x00425370, &InitCarModels);
CHEATS_REVERSED("Cheats::SetNameBCEB08", 0x00425430, &SetNameBCEB08);
CHEATS_REVERSED("Cheats::SetValueBCEB00", 0x00425450, &SetValueBCEB00);
CHEATS_REVERSED("Cheats::UnlockMoves", 0x00425460, &UnlockMoves);
CHEATS_REVERSED("Cheats::UnlockWrestling", 0x00425570, &UnlockWrestling);
CHEATS_REVERSED("Cheats::SwapPads", 0x004256B0, &SwapPads);
CHEATS_REVERSED("Cheats::GetFlagBCEB1E", 0x00425700, &GetFlagBCEB1E);
CHEATS_REVERSED("Cheats::GetTimeSeconds", 0x00425710, &GetTimeSeconds);
CHEATS_REVERSED("Cheats::GetTimeMs", 0x00425740, &GetTimeMs);
CHEATS_REVERSED("CheatCodeManager::Destroy", 0x00425770, &Destroy);
CHEATS_REVERSED("CheatCodeManager::Reset", 0x00425790, &Reset);
CHEATS_REVERSED("Cheats::DrawMessage", 0x004257A0, &DrawMessage);
CHEATS_REVERSED("Cheats::ShowCheatMessage", 0x004257F0, &ShowCheatMessage);
CHEATS_REVERSED("Cheats::CheatAmmo", 0x00425850, &CheatAmmo);
CHEATS_REVERSED("Cheats::CheatToggleBCEAF2", 0x004258D0, &CheatToggleBCEAF2);
CHEATS_REVERSED("Cheats::CheatMoney", 0x004258F0, &CheatMoney);
CHEATS_REVERSED("Cheats::CheatWeapons", 0x00425910, &CheatWeapons);
CHEATS_REVERSED("Cheats::CheatWeapons2", 0x004259A0, &CheatWeapons2);
CHEATS_REVERSED("Cheats::CheatHealth", 0x00425A20, &CheatHealth);
CHEATS_REVERSED("Cheats::CheatUnlockMoves", 0x00425A90, &CheatUnlockMoves);
CHEATS_REVERSED("Cheats::CheatUnlockWrestling", 0x00425AB0, &CheatUnlockWrestling);
CHEATS_REVERSED("CPool::GetAtRef", 0x00425AD0, &PoolGetAtRef);
CHEATS_REVERSED("Cheats::DrawDebugLine", 0x00425B00, &DrawDebugLine);

} // namespace Cheats
