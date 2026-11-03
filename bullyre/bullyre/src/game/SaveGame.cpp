// The save game block (0x56F4 bytes): built from every system's state and restored from
// it, a checksum over all but its first dword, the validation of its sections' headers
// and zero padding, the slot's summary. Several helpers take the block in a register
// (written here, not hooked). 0x733320..0x7351C0.
//
// The block: +0 checksum, +4 version 0x15, +8 size 0x56F4, +0xC the player, +0x178 the
// weapons, +0x2BC the script data, +0x378 the items, +0x504 the garages, +0x52C the
// missions, +0x11B0 the clothes, +0x2F3C the stats, +0x34F8 the collectibles, +0x3714
// the doors, +0x4424 the upgrades, +0x44B8 the settings, +0x4578 the minigame scores,
// +0x4964 the bulletin board, +0x49DC the props, +0x4D0C the errands, +0x4EA8 the mini
// objectives. Each section starts with a version and its size.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using vc::At;

struct SgVec {
    float x, y, z;
};
static_assert(sizeof(SgVec) == 0xC);

// the save slot's summary
struct SgSlotInfo {
    uint8_t used;        // +0x0
    uint8_t pad1[3];
    float playTime;      // +0x4
    int32_t chapter;     // +0x8
    int32_t area;        // +0xC
    uint32_t yearbook;   // +0x10
    int32_t grade[3];    // +0x14
    int32_t year;        // +0x20
    int32_t month;       // +0x24
    int32_t day;         // +0x28
    int32_t hour;        // +0x2C
    int32_t minute;      // +0x30
    int32_t second;      // +0x34
};
static_assert(sizeof(SgSlotInfo) == 0x38);

constexpr int kBlockSize = 0x56F4;
inline void* const sgPlayerInfo = reinterpret_cast<void*>(0x00C1AEA8);   // CWorld::Players[0]
inline void* const sgMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const sgChapter = reinterpret_cast<void*>(0x00A147E0);
inline void* const sgClothing = reinterpret_cast<void*>(0x020C5F70);
inline void* const sgCollectibles = reinterpret_cast<void*>(0x00BCEB50);
inline void* const sgDoors = reinterpret_cast<void*>(0x00C3B808);
inline void* const sgBulletinBoard = reinterpret_cast<void*>(0x00BCCE80);
inline void* const sgAudio = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const sgLurStore = reinterpret_cast<void*>(0x00D02850);
GAME_VAR(uint8_t*, sgPlayerPed, 0x00C1AEA8);
GAME_VAR(void*, sgSaveArea, 0x020CECE4);
GAME_VAR(void*, sgUpgrades, 0x020C3294);
GAME_VAR(void*, sgUnlocks, 0x00C1AA04);
GAME_VAR(void*, sgOptionsScreen, 0x020C329C);
GAME_VAR(void*, sgChapterHud, 0x00C674CC);
GAME_VAR(uint8_t*, sgWeaponHud, 0x00C674F0);
GAME_ARRAY(uint32_t, sgScriptData, 0x00C1AFB8);   // up to 0xC1B02C
GAME_ARRAY(int32_t, sgGrades, 0x00C1B02C);         // 10
GAME_VAR(uint32_t, sgCurrentDay, 0x020C3640);
GAME_VAR(uint8_t, sgSettingSeasons, 0x00A147E8);
GAME_VAR(uint8_t, sgPhysicalState, 0x00C1AF4C);
GAME_VAR(uint8_t, sgChapterNumber, 0x00A147E0);
GAME_VAR(uint8_t, sgB054, 0x00C1B054);
GAME_VAR(uint8_t, sgFilterOff, 0x00C1B171);
GAME_VAR(uint8_t, sgCanSave, 0x00C1B172);
GAME_VAR(uint8_t, sgCanSaveMore, 0x00C1B173);
GAME_VAR(uint8_t, sg9E515C, 0x009E515C);
GAME_VAR(uint8_t, sgClockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, sgClockMinutes, 0x00BCEB4A);
GAME_VAR(uint16_t, sgClockSeconds, 0x00BCEB48);
GAME_VAR(int32_t, sgClockDays, 0x00BCEB4C);
GAME_ARRAY(int32_t, sgTimeOfDay, 0x00C1A9F0);    // 3
GAME_VAR(int32_t, sgMovePlayer, 0x00BD0FD4);
GAME_ARRAY(float, sgMoveToPos, 0x00BD0FE0);
GAME_VAR(float, sgMoveToHeading, 0x00BD0FD8);
GAME_ARRAY(float, sgReturnToPos, 0x00BD0FEC);
GAME_VAR(int32_t, sgMoveToArea, 0x009F4760);
GAME_VAR(float, sgFarAway, 0x0094D3D8);          // 666
constexpr uintptr_t kSettings = 0x00C18700;      // the options (0x3F bytes)
constexpr uintptr_t kGetLocalTimeSlot = 0x008FC080;

inline void* __cdecl Sg_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(uint32_t, __cdecl, Sg_Checksum, 0x00733E60, const void* data, int size);
GAME_FN(uint32_t, __cdecl, Sg_MissionHash, 0x00733EB0);
GAME_FN(int32_t, __thiscall, Sg_GetErrand, 0x004467B0, void* info, int errand);
GAME_FN(int32_t, __thiscall, Sg_GetErrandCompletion, 0x004467E0, void* info, int errand);
GAME_FN(void, __thiscall, Sg_SetErrand, 0x00446790, void* info, int errand, int count);
GAME_FN(void, __thiscall, Sg_SetErrandCompletion, 0x004467C0, void* info, int errand, int count);
GAME_FN(uint8_t, __thiscall, Sg_GetMiniObjective, 0x00446860, void* info, int which);
GAME_FN(void, __thiscall, Sg_SetMiniObjective, 0x004467F0, void* info, int which, int done);
GAME_FN(int32_t, __thiscall, Sg_GetRespect, 0x00446780, void* info, int faction);
GAME_FN(void, __thiscall, Sg_SetRespect, 0x00446760, void* info, int faction, int respect);
GAME_FN(uint32_t, __thiscall, Sg_GetYearbookBits, 0x004466F0, void* info, int which);
GAME_FN(void, __thiscall, Sg_SetPhysicalState, 0x00445E30, void* info, int state);
GAME_FN(int32_t, __thiscall, Sg_MissionCount, 0x006AA140, void* mgr);
GAME_FN(uint8_t, __thiscall, Sg_MissionIsDebug, 0x006AB2E0, void* mgr, int index);
GAME_FN(uint8_t*, __thiscall, Sg_MissionTrack, 0x006AAF10, void* mgr, int index);
GAME_FN(uint8_t, __cdecl, Sg_AtSavePoint, 0x00737DD0, void* area);
GAME_FN(SgVec*, __cdecl, Sg_SavePointPos, 0x00737E20, SgVec* out, void* area);
GAME_FN(float, __cdecl, Sg_SavePointHeading, 0x00737EE0, void* area);
GAME_FN(SgVec*, __cdecl, Sg_SavePointReturnPos, 0x00737E80, SgVec* out, void* area);
GAME_FN(void, __thiscall, Sg_SetChapter, 0x004545C0, void* chapter, int number);
GAME_FN(void, __thiscall, Sg_SetHealth, 0x004782E0, uint8_t* ped, float health);
GAME_FN(void, __thiscall, Sg_SetMoney, 0x004752A0, uint8_t* ped, int money);
GAME_FN(void, __thiscall, Sg_SetPunishment, 0x00477520, uint8_t* ped, int points);
GAME_FN(void, __thiscall, Sg_Set18C0And18C8, 0x00474350, uint8_t* ped, int value);
GAME_FN(void, __thiscall, Sg_Set18C8Clamped, 0x00472DD0, uint8_t* ped, int value);
GAME_FN(void, __thiscall, Sg_Set18C4, 0x00474340, uint8_t* ped, float value);
GAME_FN(uint8_t, __cdecl, Sg_IsInWorld, 0x0045D450, float x, float y);
GAME_FN(uint8_t, __cdecl, Sg_VectorNotEqual, 0x004305F0, const void* a, const void* b);
GAME_FN(int, __cdecl, Sg_GetArea, 0x0054AA20, SgVec pos);
GAME_FN(void, __cdecl, Sg_Fn42E710, 0x0042E710);
GAME_FN(void, __thiscall, Sg_AudioSetOutput, 0x00595840, void* audio, int output);
GAME_FN(void, __thiscall, Sg_AudioSetVolume, 0x00594ED0, void* audio, int category, int volume);
GAME_FN(void, __thiscall, Sg_WeaponsSave, 0x0045CFC0, void* weapons, void* block);
GAME_FN(void, __thiscall, Sg_WeaponsLoad, 0x0045D2C0, void* weapons, void* block);
GAME_FN(void, __cdecl, Sg_ItemsSave, 0x00541B40, void* block);
GAME_FN(void, __cdecl, Sg_ItemsLoad, 0x00541B80, void* block);
GAME_FN(void, __cdecl, Sg_GaragesSave, 0x004C8AB0, void* block);
GAME_FN(void, __cdecl, Sg_GaragesLoad, 0x004C8B00, void* block);
GAME_FN(void, __thiscall, Sg_MissionsSave, 0x006ABC40, void* mgr, void* block);
GAME_FN(void, __thiscall, Sg_MissionsLoad, 0x006ABD50, void* mgr, void* block);
GAME_FN(void, __thiscall, Sg_ClothingSave, 0x006CAFF0, void* clothing, void* block);
GAME_FN(void, __thiscall, Sg_ClothingLoad, 0x006CB100, void* clothing, void* block);
GAME_FN(void, __cdecl, Sg_StatsSave, 0x004592C0, void* block);
GAME_FN(void, __cdecl, Sg_StatsLoad, 0x00459330, void* block);
GAME_FN(void, __thiscall, Sg_CollectiblesSave, 0x004264E0, void* mgr, void* block);
GAME_FN(void, __thiscall, Sg_CollectiblesLoad, 0x00426520, void* mgr, void* block);
GAME_FN(void, __thiscall, Sg_DoorsSave, 0x004E9760, void* mgr, void* block);
GAME_FN(void, __thiscall, Sg_DoorsLoad, 0x004E97C0, void* mgr, void* block);
GAME_FN(void, __cdecl, Sg_PropsSave, 0x004EA5B0, void* block);
GAME_FN(void, __cdecl, Sg_PropsLoad, 0x004EAF70, void* block);
GAME_FN(void, __thiscall, Sg_UpgradesSave, 0x0069FE90, void* upgrades, void* block);
GAME_FN(void, __thiscall, Sg_UpgradesLoad, 0x0069FED0, void* upgrades, void* block);
GAME_FN(void, __cdecl, Sg_MinigamesSave, 0x00705610, void* block);
GAME_FN(void, __cdecl, Sg_MinigamesLoad, 0x00705690, void* block);
GAME_FN(void, __thiscall, Sg_BulletinBoardSave, 0x00418870, void* board, void* block);
GAME_FN(void, __thiscall, Sg_BulletinBoardLoad, 0x004188C0, void* board, void* block);
GAME_FN(void, __cdecl, Sg_CatalogBegin, 0x006CB550);
GAME_FN(void*, __cdecl, Sg_CatalogGet, 0x006CA7C0);
GAME_FN(void, __thiscall, Sg_CatalogDressPed, 0x006CC7C0, void* catalog, uint8_t* ped);
GAME_FN(void, __cdecl, Sg_CatalogEnd, 0x006CB620);
GAME_FN(int16_t, __thiscall, Sg_CountCollected, 0x00426440, void* mgr, int type);
GAME_FN(int16_t, __thiscall, Sg_CountOfType, 0x00426430, void* mgr, int type);
GAME_FN(void, __thiscall, Sg_UnlocksReadAll, 0x0045BFA0, void* unlocks);
GAME_FN(void, __thiscall, Sg_OptionsApply, 0x00696620, void* screen);
GAME_FN(void, __thiscall, Sg_ChapterHudUpdate, 0x0054AD80, void* hud);
GAME_FN(uint8_t*, __thiscall, Sg_LurGetScript, 0x005DB8D0, void* store, int index);
GAME_FN(int32_t, __thiscall, Sg_ScriptCall, 0x005D8200, uint8_t* script, const char* name, int args, int results);
GAME_FN(void, __thiscall, Sg_LurUpdate, 0x005DBB40, void* store, int a);
GAME_FN(void, __thiscall, Sg_LurCollectGarbage, 0x005DAF40, void* store);

// a section's zero padding
bool IsZero(const uint8_t* block, int at, int count) {
    for (int i = 0; i < count; i++) {
        if (At<int32_t>(const_cast<uint8_t*>(block), at + 4 * i) != 0) {
            return false;
        }
    }
    return true;
}

} // namespace

namespace savegame {

// 0x733320 (cdecl): a saved value as expected (the first two arguments unused)
uint8_t __cdecl CheckValue(int, int, int value, int expected) {
    return value == expected;
}

// 0x733330 (thiscall): a 6-byte record copied (its flag bits 1..6 from the other, bit 0
// kept, the rest cleared)
uint16_t* __fastcall CopyRecord(uint16_t* self, re::Edx, const uint16_t* from) {
    self[0] = from[0];
    self[1] = from[1];
    uint8_t bits = *reinterpret_cast<const uint8_t*>(from + 2);
    uint16_t w = self[2];
    w = uint16_t((w & 0xFFFD) | (((bits >> 1) & 1) << 1));
    w = uint16_t((w & 0xFFFB) | (((bits >> 2) & 1) << 2));
    w = uint16_t((w & 0xFFF7) | (((bits >> 3) & 1) << 3));
    w = uint16_t((w & 0xFFEF) | (((bits >> 4) & 1) << 4));
    w = uint16_t((w & 0xFFDF) | (((bits >> 5) & 1) << 5));
    uint8_t low = uint8_t((((bits >> 6) & 1) << 6) | (uint8_t(w) & 0x3F));
    self[2] = uint16_t(low & 0x7F);
    return self;
}

// 0x733410, 0x733430, 0x733450, 0x733470 (thiscall): small records copied
uint8_t* __fastcall CopyDwordByte(uint8_t* self, re::Edx, const uint8_t* from) {
    At<uint32_t>(self, 0) = At<uint32_t>(const_cast<uint8_t*>(from), 0);
    self[4] = from[4];
    return self;
}
uint32_t* __fastcall CopyThreeDwords(uint32_t* self, re::Edx, const uint32_t* from) {
    self[0] = from[0];
    self[1] = from[1];
    self[2] = from[2];
    return self;
}
uint8_t* __fastcall CopyTwoBytes(uint8_t* self, re::Edx, const uint8_t* from) {
    self[0] = from[0];
    self[1] = from[1];
    return self;
}
uint32_t* __fastcall CopyTwoDwords(uint32_t* self, re::Edx, const uint32_t* from) {
    self[0] = from[0];
    self[1] = from[1];
    return self;
}

// 0x733490 (the section in eax): the settings saved
void SaveSettings(uint8_t* s) {
    const uint8_t* g = reinterpret_cast<const uint8_t*>(kSettings);
    At<int32_t>(s, 0) = 2;
    At<int32_t>(s, 4) = 0xC0;
    s[8] = g[0];
    s[9] = g[1];
    At<uint32_t>(s, 0xC) = *reinterpret_cast<const uint32_t*>(g + 4);
    At<uint32_t>(s, 0x10) = *reinterpret_cast<const uint32_t*>(g + 8);
    for (int i = 0; i < 9; i++) {
        s[0x14 + i] = g[0xC + i];   // (0x14..0x1C)
    }
    At<uint32_t>(s, 0x20) = *reinterpret_cast<const uint32_t*>(g + 0x18);
    At<uint32_t>(s, 0x24) = *reinterpret_cast<const uint32_t*>(g + 0x1C);
    s[0x28] = g[0x20];
    s[0x29] = g[0x21];
    At<uint32_t>(s, 0x2C) = *reinterpret_cast<const uint32_t*>(g + 0x24);
    At<uint32_t>(s, 0x30) = *reinterpret_cast<const uint32_t*>(g + 0x28);
    At<float>(s, 0x34) = *reinterpret_cast<const float*>(g + 0x2C);
    At<float>(s, 0x38) = *reinterpret_cast<const float*>(g + 0x30);
    s[0x3C] = g[0x34];
    s[0x3D] = g[0x35];
    s[0x3E] = g[0x36];
}

// 0x733590 (the section in eax): the errands saved
void SaveErrands(uint8_t* s) {
    At<int32_t>(s, 0) = 1;
    At<int32_t>(s, 4) = 0x19C;
    At<int32_t>(s, 8) = 0x32;
    for (int i = 0; i < 0x32; i++) {
        At<uint16_t>(s, 0xC + 4 * i) = uint16_t(Sg_GetErrand(sgPlayerInfo, i));
        At<uint16_t>(s, 0xE + 4 * i) = uint16_t(Sg_GetErrandCompletion(sgPlayerInfo, i));
    }
}

// 0x7335E0 (the section in edi): the mini objectives saved
void SaveMiniObjectives(uint8_t* s) {
    At<int32_t>(s, 0) = 2;
    At<int32_t>(s, 4) = 0x4C;
    At<int32_t>(s, 8) = 0x15;
    for (int i = 0; i < 0x15; i++) {
        s[0xC + i] = Sg_GetMiniObjective(sgPlayerInfo, i);
    }
}

// 0x733620 (cdecl): a run of dwords all zero (the name unused)
uint8_t __cdecl CheckBlock(const char*, const int32_t* data, int count) {
    for (int i = 0; i < count; i++) {
        if (data[i] != 0) {
            return 0;
        }
    }
    return 1;
}

// the sections' zero padding (offset, dwords)
struct SgPad {
    int at;
    int count;
};
const SgPad kPadding[] = {
    {0x100, 0x1E}, {0x27C, 0x10}, {0x33C, 0xF},   {0x4C4, 0x10},  {0x51C, 4},     {0x1130, 0x20},
    {0x2B3C, 0x100}, {0x33F4, 0x41}, {0x3694, 0x20}, {0x43A4, 0x20}, {0x4478, 0x10}, {0x44F8, 0x20},
    {0x47E4, 0x60}, {0x499C, 0x10}, {0x4C44, 0x32}, {0x4DE0, 0x32}, {0x4EF4, 0x200},
};

// 0x733650 (the block in edx): the padding cleared
void ClearPadding(uint8_t* block) {
    for (const SgPad& pad : kPadding) {
        for (int i = 0; i < pad.count; i++) {
            At<int32_t>(block, pad.at + 4 * i) = 0;
        }
    }
}

// 0x733C50 (the block in edx): the padding all zero
int32_t PaddingIsZero(const uint8_t* block) {
    for (const SgPad& pad : kPadding) {
        if (!IsZero(block, pad.at, pad.count)) {
            return 0;
        }
    }
    return 1;
}

// 0x733E60 (cdecl): the checksum (an ELF-style hash, from the last byte back)
uint32_t __cdecl Checksum(const uint8_t* data, int size) {
    uint32_t h = 0;
    while (size != 0) {
        uint8_t b = data[size - 1];
        size -= 1;
        h = (h << 4) + b;
        uint32_t g = h & 0xF000000;
        if (g != 0) {
            h ^= (g >> 24) ^ g;   // (the game's `sar 24`: g never has the sign bit set)
        }
    }
    return h;
}

// 0x733EB0 (cdecl): the hash of the missions' names (debug missions as 0)
uint32_t __cdecl MissionHash() {
    uint8_t names[0x7F8];
    int count = Sg_MissionCount(sgMissionMgr);
    Sg_Memset(names, 0, sizeof(names));
    for (int i = 0; i < count; i++) {
        if (!Sg_MissionIsDebug(sgMissionMgr, i)) {
            uint8_t* track = Sg_MissionTrack(sgMissionMgr, i);
            // (no bound: over 510 missions the game writes past its buffer)
            At<uint32_t>(names, i * 4) = *vc::Call<uint32_t*>(track, 0x8);
        }
    }
    return Checksum(names, sizeof(names));
}

// 0x733F60 (the block in esi): every section's header as expected and its padding zero
// (the missions' with the hash of the missions there are)
uint8_t Validate(const uint8_t* b) {
    uint8_t* block = const_cast<uint8_t*>(b);
    auto is = [block](int at, int32_t value) { return At<int32_t>(block, at) == value; };
    if (!is(4, 0x15) || !is(8, 0x56F4) || !IsZero(b, 0x4EF4, 0x200)) return 0;
    if (!is(0xC, 3) || !is(0x10, 0x16C) || !is(0x14, 0xA) || !is(0x18, 0xF) || !is(0x1C, 4)) return 0;
    if (!IsZero(b, 0x100, 0x1E)) return 0;
    if (!is(0x178, 1) || !is(0x17C, 0x144) || !is(0x180, 0x1E)) return 0;
    if (!IsZero(b, 0x27C, 0x10)) return 0;
    if (!is(0x2BC, 1) || !is(0x2C0, 0xBC) || !is(0x2C4, 0x1D)) return 0;
    if (!IsZero(b, 0x33C, 0xF)) return 0;
    if (!is(0x378, 1) || !is(0x37C, 0x18C) || !is(0x380, 0x50)) return 0;
    if (!IsZero(b, 0x4C4, 0x10)) return 0;
    if (!is(0x504, 1) || !is(0x508, 0x28) || !is(0x50C, 2)) return 0;
    if (!IsZero(b, 0x51C, 4)) return 0;
    if (!is(0x52C, 1) || !is(0x530, 0xC84) || !is(0x534, 0x1FE)) return 0;
    if (!IsZero(b, 0x1130, 0x20)) return 0;
    if (Sg_MissionHash() != At<uint32_t>(block, 0x538)) return 0;
    if (!is(0x11B0, 2) || !is(0x11B4, 0x1D8C) || !is(0x11B8, 6) || !is(0x11BC, 0x177) || !is(0x11C0, 0x23) ||
        !is(0x11C4, 0x28)) {
        return 0;
    }
    if (!IsZero(b, 0x2B3C, 0x100)) return 0;
    if (!is(0x2F3C, 1) || !is(0x2F40, 0x5BC) || !is(0x2F44, 0x104) || !is(0x2F48, 0x26)) return 0;
    if (!IsZero(b, 0x33F4, 0x41)) return 0;
    if (!is(0x34F8, 1) || !is(0x34FC, 0x21C) || !is(0x3500, 0xC8)) return 0;
    if (!IsZero(b, 0x3694, 0x20)) return 0;
    if (!is(0x3714, 2) || !is(0x3718, 0xD10) || !is(0x371C, 0x190)) return 0;
    if (!IsZero(b, 0x43A4, 0x20)) return 0;
    if (!is(0x49DC, 1) || !is(0x49E0, 0x330) || !is(0x49E4, 0x4B)) return 0;
    if (!IsZero(b, 0x4C44, 0x32)) return 0;
    if (!is(0x4424, 1) || !is(0x4428, 0x94) || !is(0x442C, 0x48)) return 0;
    if (!IsZero(b, 0x4478, 0x10)) return 0;
    if (!is(0x44B8, 2) || !is(0x44BC, 0xC0)) return 0;
    if (!IsZero(b, 0x44F8, 0x20)) return 0;
    if (!is(0x4578, 1) || !is(0x457C, 0x3EC) || !is(0x4580, 0xA) || !is(0x4584, 5) || !is(0x4588, 3)) return 0;
    if (!IsZero(b, 0x47E4, 0x60)) return 0;
    if (!is(0x4964, 1) || !is(0x4968, 0x78) || !is(0x496C, 0x28)) return 0;
    if (!IsZero(b, 0x499C, 0x10)) return 0;
    if (!is(0x4D0C, 1) || !is(0x4D10, 0x19C) || !is(0x4D14, 0x32)) return 0;
    if (!IsZero(b, 0x4DE0, 0x32)) return 0;
    return PaddingIsZero(b) != 0;
}

// 0x734470 (the block in esi): an older block brought up to date (checksum renewed)
void Upgrade(uint8_t* block) {
    if (At<int32_t>(block, 0x4EA8) == 1) {
        block[0x4EC7] = 0;
        block[0x4EC8] = 0;
        At<int32_t>(block, 0x4EA8) = 2;
        At<int32_t>(block, 0x4EB0) = 0x14;
        At<uint32_t>(block, 0) = Sg_Checksum(block + 4, 0x56F0);
    }
    if (At<int32_t>(block, 0xC) == 2) {
        At<int32_t>(block, 0xC) = 3;
        At<uint32_t>(block, 0) = Sg_Checksum(block + 4, 0x56F0);
    }
}

// 0x7344D0 (cdecl): a block checked: corrupt (checksum) and valid (headers)
void __cdecl Check(uint8_t* block, uint8_t* valid, uint8_t* corrupt) {
    Upgrade(block);
    *corrupt = Sg_Checksum(block + 4, 0x56F0) != At<uint32_t>(block, 0);
    *valid = Validate(block);
}

// 0x734510 (the section in esi): the player saved (at the save point when there is one)
void SavePlayer(uint8_t* s) {
    SgVec dir{0.0f, 0.0f, 0.0f};
    At<int32_t>(s, 0) = 3;
    At<int32_t>(s, 4) = 0x16C;
    At<int32_t>(s, 8) = 0xA;
    At<int32_t>(s, 0xC) = 0xF;
    At<int32_t>(s, 0x10) = 4;
    void* area = sgSaveArea;
    uint8_t* ped = sgPlayerPed;
    SgVec pos;
    float heading;
    SgVec returnPos;
    if (Sg_AtSavePoint(area)) {
        pos = *Sg_SavePointPos(&dir, area);
        heading = Sg_SavePointHeading(area);
        returnPos = *Sg_SavePointReturnPos(&dir, area);
    } else {
        uint8_t* m = At<uint8_t*>(ped, 0x14);
        const SgVec* p = m != nullptr ? reinterpret_cast<SgVec*>(m + 0x30) : reinterpret_cast<SgVec*>(ped + 4);
        pos = *p;
        heading = At<float>(ped, 0x1700);
        returnPos = dir;
    }
    At<float>(s, 0x14) = At<float>(ped, 0x1CB8);
    At<float>(s, 0x18) = At<float>(ped, 0x1CA4);
    At<int32_t>(s, 0x1C) = At<int32_t>(ped, 0x1D40);
    At<float>(s, 0x20) = At<float>(ped, 0x18C0);
    At<float>(s, 0x24) = float(At<int32_t>(ped, 0x18C8));
    At<float>(s, 0x28) = At<float>(ped, 0x18C4);
    At<int32_t>(s, 0x2C) = At<int32_t>(ped, 0x1CA0);
    At<float>(s, 0x3C) = heading;
    At<SgVec>(s, 0x30) = pos;
    At<SgVec>(s, 0x40) = returnPos;
    s[0x68] = sgPhysicalState;
    At<int32_t>(s, 0x4C) = sgClockHours;
    At<int32_t>(s, 0x50) = sgClockMinutes;
    At<int32_t>(s, 0x54) = *reinterpret_cast<uint8_t*>(&sgClockSeconds);
    At<int32_t>(s, 0x58) = sgClockDays;
    At<int32_t>(s, 0x5C) = sgTimeOfDay[0];
    At<int32_t>(s, 0x60) = sgTimeOfDay[1];
    At<int32_t>(s, 0x64) = sgTimeOfDay[2];
    s[0x69] = sgChapterNumber;
    s[0x6A] = sgB054;
    uint8_t flags = s[0x6B];
    flags ^= uint8_t((uint8_t(sgFilterOff * 4) ^ flags) & 4);
    flags = uint8_t(((flags ^ sgCanSave) & 1) ^ flags);
    flags = uint8_t(((uint8_t(sgCanSaveMore * 2) ^ flags) & 2) ^ flags);
    s[0x6B] = uint8_t(((sg9E515C & 1) << 3) | (flags & 7));
    for (int i = 0; i < 10; i++) {
        At<int32_t>(s, 0x6C + 4 * i) = sgGrades[i];
    }
    for (int i = 0; i < 0xF; i++) {
        At<int32_t>(s, 0x94 + 4 * i) = Sg_GetRespect(sgPlayerInfo, i);
    }
    for (int i = 0; i < 4; i++) {
        At<float>(s, 0xD0 + 4 * i) = At<float>(ped, 0x1CCC + 4 * i);
        At<float>(s, 0xE0 + 4 * i) = At<float>(ped, 0x1CF4 + 4 * i);
    }
    At<uint32_t>(s, 0xF0) = sgCurrentDay;
}

// 0x7347E0 (the section in esi): the player restored (moved to the saved point)
void LoadPlayer(uint8_t* s) {
    if (At<int32_t>(s, 0) != 3 || At<int32_t>(s, 4) != 0x16C || At<int32_t>(s, 8) != 0xA || At<int32_t>(s, 0xC) != 0xF ||
        At<int32_t>(s, 0x10) != 4) {
        return;
    }
    if (!IsZero(s, 0xF4, 0x1E)) {
        return;
    }
    uint8_t* ped = sgPlayerPed;
    sgSettingSeasons = 0;
    Sg_SetChapter(sgChapter, s[0x69]);
    At<float>(ped, 0x1CA4) = At<float>(s, 0x18);
    Sg_SetHealth(ped, At<float>(s, 0x14));
    Sg_SetMoney(ped, At<int32_t>(s, 0x2C));
    sgMovePlayer = 1;
    sgMoveToPos[0] = At<float>(s, 0x30);
    sgMoveToPos[1] = At<float>(s, 0x34);
    sgMoveToPos[2] = At<float>(s, 0x38);
    sgMoveToHeading = At<float>(s, 0x3C);
    sgReturnToPos[0] = 0.0f;
    sgReturnToPos[1] = 0.0f;
    sgReturnToPos[2] = 0.0f;
    float far = sgFarAway;
    SgVec farAway{far, far, far};
    const SgVec* back = &At<SgVec>(s, 0x40);
    if (Sg_IsInWorld(back->x, back->y) && Sg_VectorNotEqual(back, &farAway)) {
        sgReturnToPos[0] = back->x;
        sgReturnToPos[1] = back->y;
        sgReturnToPos[2] = back->z;
    }
    sgMoveToArea = Sg_GetArea(At<SgVec>(s, 0x30));
    Sg_Fn42E710();
    sgClockDays = At<int32_t>(s, 0x58);
    Sg_SetPunishment(ped, At<int32_t>(s, 0x1C));
    sgTimeOfDay[0] = At<int32_t>(s, 0x5C);
    sgTimeOfDay[1] = At<int32_t>(s, 0x60);
    sgTimeOfDay[2] = At<int32_t>(s, 0x64);
    Sg_Set18C0And18C8(ped, int(At<float>(s, 0x20)));   // (_ftol: truncation)
    Sg_Set18C8Clamped(ped, int(At<float>(s, 0x24)));
    Sg_Set18C4(ped, At<float>(s, 0x28));
    Sg_SetPhysicalState(sgPlayerInfo, s[0x68]);
    sgFilterOff = (s[0x6B] >> 2) & 1;
    sgCanSave = s[0x6B] & 1;
    sgCanSaveMore = (s[0x6B] >> 1) & 1;
    if (s[0x6B] & 8) {
        sg9E515C = 1;
    }
    sgClockHours = s[0x4C];
    sgClockMinutes = s[0x50];
    sgClockSeconds = At<uint16_t>(s, 0x54);
    sgB054 = s[0x6A];
    for (int i = 0; i < 10; i++) {
        sgGrades[i] = At<int32_t>(s, 0x6C + 4 * i);
    }
    for (int i = 0; i < 0xF; i++) {
        Sg_SetRespect(sgPlayerInfo, i, At<int32_t>(s, 0x94 + 4 * i));
    }
    for (int i = 0; i < 4; i++) {
        At<float>(ped, 0x1CCC + 4 * i) = At<float>(s, 0xD0 + 4 * i);
        At<float>(ped, 0x1CF4 + 4 * i) = At<float>(s, 0xE0 + 4 * i);
    }
    sgCurrentDay = At<uint32_t>(s, 0xF0);
}

// 0x734B00 (the section in edx): the script data restored
void LoadScriptData(const uint8_t* s) {
    uint8_t* sec = const_cast<uint8_t*>(s);
    if (At<int32_t>(sec, 0) != 1 || At<int32_t>(sec, 4) != 0xBC || At<int32_t>(sec, 8) != 0x1D || !IsZero(s, 0x80, 0xF)) {
        return;
    }
    for (int i = 0; i < 0x1D; i++) {
        sgScriptData[i] = At<uint32_t>(sec, 0xC + 4 * i);
    }
}

// 0x734B50 (the section in eax): the settings restored (and the audio set from them)
void LoadSettings(const uint8_t* s) {
    uint8_t* sec = const_cast<uint8_t*>(s);
    if (At<int32_t>(sec, 0) != 2 || At<int32_t>(sec, 4) != 0xC0 || !IsZero(s, 0x40, 0x20)) {
        return;
    }
    uint8_t* g = reinterpret_cast<uint8_t*>(kSettings);
    g[0] = s[8];
    g[1] = s[9];
    At<uint32_t>(g, 4) = At<uint32_t>(sec, 0xC);
    At<uint32_t>(g, 8) = At<uint32_t>(sec, 0x10);
    g[0xC] = s[0x14];
    g[0xD] = s[0x15];
    g[0xF] = s[0x17];
    g[0x10] = s[0x18];
    g[0x11] = s[0x19];
    g[0x12] = s[0x1A];
    g[0x13] = s[0x1B];
    g[0x14] = s[0x1C];
    At<uint32_t>(g, 0x18) = At<uint32_t>(sec, 0x20);
    At<uint32_t>(g, 0x1C) = At<uint32_t>(sec, 0x24);
    g[0x20] = s[0x28];
    g[0x21] = s[0x29];
    At<float>(g, 0x2C) = At<float>(sec, 0x34);
    At<float>(g, 0x30) = At<float>(sec, 0x38);
    g[0x34] = s[0x3C];
    g[0x35] = s[0x3D];
    g[0x36] = s[0x3E];
    Sg_AudioSetOutput(sgAudio, int8_t(s[0x1B]));
    Sg_AudioSetVolume(sgAudio, 4, At<int32_t>(g, 0x18));
    Sg_AudioSetVolume(sgAudio, 1, At<int32_t>(g, 0x1C));
    Sg_AudioSetVolume(sgAudio, 2, int8_t(g[0x20]));
}

// 0x734CA0 (the section in edx): the errands restored
void LoadErrands(const uint8_t* s) {
    uint8_t* sec = const_cast<uint8_t*>(s);
    if (At<int32_t>(sec, 0) != 1 || At<int32_t>(sec, 4) != 0x19C || At<int32_t>(sec, 8) != 0x32 || !IsZero(s, 0xD4, 0x32)) {
        return;
    }
    for (int i = 0; i < 0x32; i++) {
        Sg_SetErrand(sgPlayerInfo, i, At<uint16_t>(sec, 0xC + 4 * i));
        Sg_SetErrandCompletion(sgPlayerInfo, i, At<uint16_t>(sec, 0xE + 4 * i));
    }
}

// 0x734D10 (the section in edi): the mini objectives restored
void LoadMiniObjectives(const uint8_t* s) {
    uint8_t* sec = const_cast<uint8_t*>(s);
    if (At<int32_t>(sec, 0) != 2 || At<int32_t>(sec, 4) != 0x4C || At<int32_t>(sec, 8) != 0x15 || !IsZero(s, 0x24, 0xA)) {
        return;
    }
    for (int i = 0; i < 0x15; i++) {
        Sg_SetMiniObjective(sgPlayerInfo, i, s[0xC + i]);
    }
}

// 0x734D60 (cdecl): the block built
void __cdecl Save(uint8_t* block) {
    Sg_Memset(block, 0, kBlockSize);
    ClearPadding(block);
    uint8_t* ped = sgPlayerPed;
    At<int32_t>(block, 4) = 0x15;
    At<int32_t>(block, 8) = 0x56F4;
    SavePlayer(block + 0xC);
    Sg_WeaponsSave(At<void*>(ped, 0x1C4), block + 0x178);
    At<int32_t>(block, 0x2BC) = 1;
    At<int32_t>(block, 0x2C0) = 0xBC;
    At<int32_t>(block, 0x2C4) = 0x1D;
    for (int i = 0; i < 0x1D; i++) {
        At<uint32_t>(block, 0x2C8 + 4 * i) = sgScriptData[i];
    }
    Sg_ItemsSave(block + 0x378);
    Sg_GaragesSave(block + 0x504);
    Sg_MissionsSave(sgMissionMgr, block + 0x52C);
    Sg_ClothingSave(sgClothing, block + 0x11B0);
    Sg_StatsSave(block + 0x2F3C);
    Sg_CollectiblesSave(sgCollectibles, block + 0x34F8);
    Sg_DoorsSave(sgDoors, block + 0x3714);
    Sg_PropsSave(block + 0x49DC);
    Sg_UpgradesSave(sgUpgrades, block + 0x4424);
    SaveSettings(block + 0x44B8);
    Sg_MinigamesSave(block + 0x4578);
    Sg_BulletinBoardSave(sgBulletinBoard, block + 0x4964);
    SaveErrands(block + 0x4D0C);
    SaveMiniObjectives(block + 0x4EA8);
    At<uint32_t>(block, 0) = Sg_Checksum(block + 4, 0x56F0);
}

// 0x734EE0 (cdecl): a block loaded (when its checksum and headers hold), the player
// dressed, the script told
void __cdecl Load(uint8_t* block) {
    Upgrade(block);
    bool corrupt = Sg_Checksum(block + 4, 0x56F0) != At<uint32_t>(block, 0);
    if (!Validate(block) || corrupt) {
        return;
    }
    uint8_t* ped = sgPlayerPed;
    if (At<int32_t>(block, 4) != 0x15 || At<int32_t>(block, 8) != 0x56F4) {
        return;
    }
    LoadPlayer(block + 0xC);
    Sg_WeaponsLoad(At<void*>(ped, 0x1C4), block + 0x178);
    LoadScriptData(block + 0x2BC);
    Sg_ItemsLoad(block + 0x378);
    Sg_GaragesLoad(block + 0x504);
    Sg_MissionsLoad(sgMissionMgr, block + 0x52C);
    Sg_ClothingLoad(sgClothing, block + 0x11B0);
    Sg_StatsLoad(block + 0x2F3C);
    Sg_CollectiblesLoad(sgCollectibles, block + 0x34F8);
    Sg_DoorsLoad(sgDoors, block + 0x3714);
    Sg_PropsLoad(block + 0x49DC);
    Sg_UpgradesLoad(sgUpgrades, block + 0x4424);
    LoadSettings(block + 0x44B8);
    Sg_MinigamesLoad(block + 0x4578);
    Sg_BulletinBoardLoad(sgBulletinBoard, block + 0x4964);
    LoadErrands(block + 0x4D0C);
    LoadMiniObjectives(block + 0x4EA8);
    Sg_CatalogBegin();
    uint8_t* player = sgPlayerPed;
    Sg_CatalogDressPed(Sg_CatalogGet(), player);
    Sg_CatalogEnd();
    int16_t collected = Sg_CountCollected(sgCollectibles, 2);
    int16_t total = Sg_CountOfType(sgCollectibles, 2);
    if (collected >= total) {
        sgWeaponHud[0x15D] = 1;
    }
    Sg_UnlocksReadAll(sgUnlocks);
    Sg_OptionsApply(sgOptionsScreen);
    Sg_ChapterHudUpdate(sgChapterHud);
    uint8_t* script = Sg_LurGetScript(sgLurStore, 0);
    if (script != nullptr) {
        Sg_ScriptCall(script, "F_HandleLoadGame", 0, 1);
        Sg_LurUpdate(sgLurStore, 0);
        Sg_LurCollectGarbage(sgLurStore);
    }
}

// 0x7350E0 (thiscall): the slot's summary from the player section
void __fastcall SlotInfo_Set(SgSlotInfo* self, re::Edx, uint8_t* player, float playTime) {
    self->playTime = playTime;
    self->used = 1;
    self->chapter = player[0x75];
    self->area = Sg_GetArea(At<SgVec>(player, 0x3C));
    self->yearbook = Sg_GetYearbookBits(sgPlayerInfo, 0x1C);
    self->grade[0] = At<int32_t>(player, 0x68);
    self->grade[1] = At<int32_t>(player, 0x6C);
    self->grade[2] = At<int32_t>(player, 0x70);
    uint16_t now[8];
    vc::Import<void>(kGetLocalTimeSlot, now);   // GetLocalTime (SYSTEMTIME)
    self->year = now[0];
    if (self->year > 2000) {
        self->year = now[0] - 2000;
    }
    self->month = now[1];
    self->day = now[3];
    self->hour = now[4];
    self->minute = now[5];
    self->second = now[6];
}

// 0x735190 (thiscall): the slot's summary cleared
void __fastcall SlotInfo_Clear(SgSlotInfo* self, re::Edx) {
    self->playTime = 0.0f;
    self->used = 0;
    self->chapter = 0;
    self->area = 0;
    self->yearbook = 0;
    self->grade[0] = 0;
    self->grade[1] = 0;
    self->grade[2] = 0;
    self->year = 0;
    self->month = 0;
    self->day = 0;
    self->hour = 0;
    self->minute = 0;
    self->second = 0;
}

} // namespace savegame

// (the block or a section in a register)
RE_UNHOOKED("SaveGame::SaveSettings", 0x00733490);
RE_UNHOOKED("SaveGame::SaveErrands", 0x00733590);
RE_UNHOOKED("SaveGame::SaveMiniObjectives", 0x007335E0);
RE_UNHOOKED("SaveGame::ClearPadding", 0x00733650);
RE_UNHOOKED("SaveGame::PaddingIsZero", 0x00733C50);
RE_UNHOOKED("SaveGame::Validate", 0x00733F60);
RE_UNHOOKED("SaveGame::Upgrade", 0x00734470);
RE_UNHOOKED("SaveGame::SavePlayer", 0x00734510);
RE_UNHOOKED("SaveGame::LoadPlayer", 0x007347E0);
RE_UNHOOKED("SaveGame::LoadScriptData", 0x00734B00);
RE_UNHOOKED("SaveGame::LoadSettings", 0x00734B50);
RE_UNHOOKED("SaveGame::LoadErrands", 0x00734CA0);
RE_UNHOOKED("SaveGame::LoadMiniObjectives", 0x00734D10);

#define SAVEGAME_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

SAVEGAME_REVERSED("SaveGame::CheckValue", 0x00733320, &savegame::CheckValue);
SAVEGAME_REVERSED("SaveGame::CopyRecord", 0x00733330, &savegame::CopyRecord);
SAVEGAME_REVERSED("SaveGame::CopyDwordByte", 0x00733410, &savegame::CopyDwordByte);
SAVEGAME_REVERSED("SaveGame::CopyThreeDwords", 0x00733430, &savegame::CopyThreeDwords);
SAVEGAME_REVERSED("SaveGame::CopyTwoBytes", 0x00733450, &savegame::CopyTwoBytes);
SAVEGAME_REVERSED("SaveGame::CopyTwoDwords", 0x00733470, &savegame::CopyTwoDwords);
SAVEGAME_REVERSED("SaveGame::CheckBlock", 0x00733620, &savegame::CheckBlock);
SAVEGAME_REVERSED("SaveGame::Checksum", 0x00733E60, &savegame::Checksum);
SAVEGAME_REVERSED("SaveGame::MissionHash", 0x00733EB0, &savegame::MissionHash);
SAVEGAME_REVERSED("SaveGame::Check", 0x007344D0, &savegame::Check);
SAVEGAME_REVERSED("SaveGame::Save", 0x00734D60, &savegame::Save);
SAVEGAME_REVERSED("SaveGame::Load", 0x00734EE0, &savegame::Load);
SAVEGAME_REVERSED("SaveSlotInfo::Set", 0x007350E0, &savegame::SlotInfo_Set);
SAVEGAME_REVERSED("SaveSlotInfo::Clear", 0x00735190, &savegame::SlotInfo_Clear);
