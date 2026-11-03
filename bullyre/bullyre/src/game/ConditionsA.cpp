// The action tree's conditions (continued): weighted random, clothing, season,
// missions, sounds and speech, items and stats, the world's flags, tagging and the
// front end; their field initialisers. 0x5FC770..0x5FD680.
//
// Arithmetic is float in the game's order (its x87 runs in single precision, D3D9's
// default).

#include <cstdint>

#include "game/ActionTree.h"

using namespace acttree;

namespace {

GAME_VAR(uint32_t, weightedIdGuard, 0x01B64A00);
GAME_VAR(int16_t, weightedId, 0x01B649FC);
GAME_ARRAY(int32_t, outfitSlots_20C5F70, 0x020C5F70);   // 12 bytes each: +0 the item, +9 its style
GAME_VAR(int32_t, clothingWarmth_20C78F0, 0x020C78F0);
GAME_VAR(uint8_t, clothingFlags_20C5F7B, 0x020C5F7B);
GAME_VAR(int32_t, currentArea_BD1008, 0x00BD1008);
GAME_VAR(uint8_t, currentChapter_A147E0, 0x00A147E0);
GAME_ARRAY(int32_t, chapterSeason_A147EC, 0x00A147EC);
GAME_VAR(void*, cutsceneSound_CF4290, 0x00CF4290);
GAME_VAR(int32_t, speechStreamIndex_CF4644, 0x00CF4644);
GAME_VAR(void*, targetRanges_20C889C, 0x020C889C);
GAME_VAR(uint8_t, canLaunchMission_20C43EC, 0x020C43EC);
GAME_VAR(int32_t, missionSuccessCount_20C43E8, 0x020C43E8);
GAME_VAR(int16_t, weatherType_CE756C, 0x00CE756C);
GAME_VAR(uint8_t, parametricStatic_1B6502C, 0x01B6502C);
GAME_VAR(uint8_t*, playerInfo_C1AEA8, 0x00C1AEA8);
GAME_VAR(uint8_t, rumbleAmbient_C2AFA0, 0x00C2AFA0);
GAME_VAR(uint8_t, rumbleMission_C2AFA1, 0x00C2AFA1);
GAME_VAR(uint8_t, ignoreTruants_C2AFA2, 0x00C2AFA2);
GAME_VAR(int32_t, weaponModelFirst_A136B0, 0x00A136B0);
GAME_VAR(int32_t, weaponModelLast_A136B4, 0x00A136B4);
GAME_VAR(int32_t, skippedWeapon_BF4108, 0x00BF4108);
GAME_VAR(int32_t, skippedWeapon_BF4100, 0x00BF4100);
GAME_VAR(uint8_t*, tagging_C67538, 0x00C67538);
GAME_VAR(uint8_t, cutsceneRunning_20C5BE4, 0x020C5BE4);
GAME_ARRAY(int32_t, scriptSaveData_C1AFB8, 0x00C1AFB8);
GAME_VAR(uint8_t*, punishment_C0F78C, 0x00C0F78C);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(uint32_t, frontEndTime_20C3308, 0x020C3308);
GAME_VAR(uint8_t, frontEndActive_20C32FA, 0x020C32FA);
GAME_VAR(float, twoTo32_900AC8, 0x00900AC8);
GAME_VAR(double, uprightLimit_92E898, 0x0092E898);   // 0.88
inline void* const outfits_20C5F70 = reinterpret_cast<void*>(0x020C5F70);
inline void* const missionManager = reinterpret_cast<void*>(0x020C3CA0);
inline void* const musicStreams_CF42D4 = reinterpret_cast<void*>(0x00CF42D4);
inline void* const soundBanks_CF5DC4 = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const playerInfoSlot_C1AEA8 = reinterpret_cast<void*>(0x00C1AEA8);
inline uint8_t* const speechStreams_CF458C = reinterpret_cast<uint8_t*>(0x00CF458C);

GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint32_t*, __thiscall, NameKey_FromValue, 0x005FA870, uint32_t* key, uint32_t value);
GAME_FN(void, __thiscall, StringRef_Assign, 0x005772F0, uint32_t* field, uint32_t value);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint8_t, __cdecl, Condition_IsAStatic, 0x005FC700, int id);
GAME_FN(uint8_t, __cdecl, CompareFloats, 0x005FB8B0, float a, float b, int op);
GAME_FN(uint8_t, __cdecl, CompareInts, 0x005FBCB0, int32_t a, int32_t b, int op);
GAME_FN(uint8_t, __thiscall, Outfits_IsWorn_6CA9F0, 0x006CA9F0, void* outfits, int32_t outfit);
GAME_FN(uint8_t, __cdecl, Area_IsInterior_430320, 0x00430320, int32_t area);
GAME_FN(int32_t, __thiscall, Missions_FindByName, 0x006A9FD0, void* manager, uint32_t* name);
GAME_FN(uint8_t*, __thiscall, Missions_GetList_6AA680, 0x006AA680, void* manager);
GAME_FN(uint8_t*, __thiscall, Missions_GetList_6AA690, 0x006AA690, void* manager);
GAME_FN(uint8_t, __thiscall, MissionList_IsActive_6AA3F0, 0x006AA3F0, uint8_t* list);
GAME_FN(uint8_t, __thiscall, MissionList_Contains_6AA410, 0x006AA410, uint8_t* list, int32_t mission);
GAME_FN(uint8_t, __thiscall, Missions_AnyActive_6AA7E0, 0x006AA7E0, void* manager);
GAME_FN(uint8_t*, __thiscall, Missions_Get_6AA7C0, 0x006AA7C0, void* manager, int32_t mission);
GAME_FN(uint8_t, __thiscall, Missions_IsDone_6AA940, 0x006AA940, void* manager, int32_t mission);
GAME_FN(int32_t, __thiscall, Missions_SecondaryState_6AC0F0, 0x006AC0F0, void* manager, uint32_t kind);
GAME_FN(uint8_t, __thiscall, Missions_InClass_6AA840, 0x006AA840, void* manager);
GAME_FN(int32_t, __thiscall, CutsceneSound_IsLoaded_5A6DA0, 0x005A6DA0, void* sound);
GAME_FN(uint8_t, __thiscall, MusicStreams_IsLoaded_58EE40, 0x0058EE40, void* streams);
GAME_FN(int32_t, __thiscall, SoundBanks_Find_597FC0, 0x00597FC0, void* banks, uintptr_t name);
GAME_FN(uint8_t, __thiscall, SoundBanks_IsLoaded_5995F0, 0x005995F0, void* banks, int32_t bank);
GAME_FN(uint8_t*, __thiscall, TrackOrCondition_Construct, 0x0061A600, uint8_t* self);
GAME_FN(int32_t, __cdecl, Gifts_Find_541EC0, 0x00541EC0, int32_t item);
GAME_FN(int32_t, __cdecl, ModelIndexFromName_51AE20, 0x0051AE20, int32_t name, int, int);
GAME_FN(uint8_t*, __cdecl, ItemInfo_51B450, 0x0051B450, int32_t model);
GAME_FN(int32_t, __cdecl, Inventory_PlayerCount_541E00, 0x00541E00, int32_t model);
GAME_FN(int32_t, __cdecl, Stats_GetInt_459A00, 0x00459A00, int32_t stat);
GAME_FN(float, __cdecl, Stats_GetFloat_45A900, 0x0045A900, int32_t stat);
GAME_FN(int32_t, __thiscall, Inventory_Count_45CF40, 0x0045CF40, uint8_t* inventory, int32_t model);
GAME_FN(uint8_t, __cdecl, Cutscene_IsRunning_5BB380, 0x005BB380);
GAME_FN(uint8_t, __thiscall, PlayerInfo_IsMiniObjectiveComplete_446860, 0x00446860, void* player, int32_t objective);

// 0x5FC770 (thiscall): ConditionWeightedRandom::IsA
uint8_t __fastcall WeightedRandom_IsA(uint8_t*, re::Edx, int id) {
    if (int16_t(id) == TypeId(weightedIdGuard, weightedId)) {
        return 1;
    }
    return Condition_IsAStatic(id);
}

// 0x5FC7E0 (thiscall): ConditionWeightedRandom: its share of `total` added to the
// running `acc` (kept at +8)
float __fastcall WeightedRandom_Accumulate(uint8_t* self, re::Edx, float acc, float total) {
    if (total > 0.0f) {
        vc::At<float>(self, 8) = vc::At<float>(self, 4) / total + acc;
    }
    return vc::At<float>(self, 8);
}

void __fastcall Fields_5FC810(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 2;
    vc::At<int32_t>(self, 8) = 0;
    self[0xC] = 1;
    self[0xD] = 1;
    self[0xE] = 1;
    self[0xF] = 0;
}

// 0x5FC830 (thiscall): ConditionClothing: an outfit worn, or a slot's item
uint32_t __fastcall Clothing_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t outfit = vc::At<int32_t>(self, 0xC);
    if (outfit) {
        return Outfits_IsWorn_6CA9F0(outfits_20C5F70, outfit);
    }
    return outfitSlots_20C5F70[vc::At<int32_t>(self, 4) * 3] == vc::At<int32_t>(self, 8) ? 1 : 0;
}

void __fastcall Fields_5FC860(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    self[8] = 0;
}
void __fastcall Fields_5FC870(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 1;
    vc::At<int32_t>(self, 4) = 0;
    self[0xC] = 1;
}

// 0x5FC890 (thiscall): ConditionClothingStyle: any slot (+0xC) or one slot in the style
uint32_t __fastcall ClothingStyle_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t style = vc::At<int32_t>(self, 4);
    uint8_t* slots = reinterpret_cast<uint8_t*>(0x020C5F79);
    if (self[0xC]) {
        for (uint8_t* p = slots; p < reinterpret_cast<uint8_t*>(0x020C5FC1); p += 0xC) {
            if (style == int8_t(*p)) {
                return 1;
            }
        }
        return 0;
    }
    return style == int8_t(slots[vc::At<int32_t>(self, 8) * 0xC]) ? 1 : 0;
}

void __fastcall Fields_5FC8E0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 5;
    vc::At<int32_t>(self, 8) = 0;
}

// 0x5FC8F0 (thiscall): ConditionClothingWarmthFactor
uint8_t __fastcall ClothingWarmth_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return CompareInts(clothingWarmth_20C78F0, vc::At<int32_t>(self, 8), vc::At<int32_t>(self, 4));
}

// 0x5FC910 (thiscall): ConditionClothingSpecialKiss
uint32_t __fastcall ClothingSpecialKiss_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return (uint32_t(clothingFlags_20C5F7B) >> 3) & 1;
}

// 0x5FC920 (thiscall): ConditionVisibleArea
uint32_t __fastcall VisibleArea_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return currentArea_BD1008 == vc::At<int32_t>(self, 4) ? 1 : 0;
}

void __fastcall Fields_5FC940(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 4;
    vc::At<int32_t>(self, 8) = 0;
}
void __fastcall Fields_5FC950(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 4;
    vc::At<float>(self, 8) = 0.15f;
}

// 0x5FC990 (thiscall): ConditionConditionGroup's destructor
void __fastcall ConditionGroup_Destruct(uint8_t* self, re::Edx) {
    vc::At<uint32_t>(self, 0) = 0x0092E6BC;
}

void __fastcall Fields_5FC9A0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<float>(self, 8) = 0.0f;
}
void __fastcall Fields_5FC9E0(uint8_t* self, re::Edx) {
    self[0xC] = 0;
    self[0xD] = 0;
    self[0xE] = 0;
    self[0xF] = 0;
}
void __fastcall Fields_5FC9F0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 1.0f;
}
void __fastcall Fields_5FCA00(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 5;
    vc::At<float>(self, 4) = 0.785f;
}
void __fastcall Fields_5FCA20(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 3;
    self[4] = 1;
}

// 0x5FCA30 (thiscall): ConditionSeason: the chapter's season (outdoors only with +4)
uint8_t __fastcall Season_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    if (self[4]) {
        uint8_t interior = Area_IsInterior_430320(currentArea_BD1008);
        if (!interior) {
            return interior;
        }
    }
    return chapterSeason_A147EC[currentChapter_A147E0] == vc::At<int32_t>(self, 8);
}

// 0x5FCAA0 (thiscall): ConditionOnMission: the named mission in either active list (no
// name: any mission)
uint8_t __fastcall OnMission_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    uint32_t* name = &vc::At<uint32_t>(self, 4);
    if (*name == 0) {
        return Missions_AnyActive_6AA7E0(missionManager);
    }
    int32_t mission = Missions_FindByName(missionManager, name);
    uint8_t* list = Missions_GetList_6AA680(missionManager);
    if (MissionList_IsActive_6AA3F0(list) && MissionList_Contains_6AA410(Missions_GetList_6AA680(missionManager), mission)) {
        return 1;
    }
    list = Missions_GetList_6AA690(missionManager);
    if (MissionList_IsActive_6AA3F0(list) && MissionList_Contains_6AA410(Missions_GetList_6AA690(missionManager), mission)) {
        return 1;
    }
    return 0;
}

void ClearKey(uint8_t* self, int at) {
    uint32_t key;
    NameKey_FromValue(&key, 0);
    StringRef_Assign(&vc::At<uint32_t>(self, at), key);
    NameKey_Destruct(&key);
}
void __fastcall Fields_5FCB30(uint8_t* self, re::Edx) {
    self[8] = 1;
    ClearKey(self, 4);
    vc::At<float>(self, 0xC) = 20.0f;
}

// 0x5FCBA0 (thiscall): ConditionCutsceneSoundLoaded (given up after 500 tries)
uint8_t __fastcall CutsceneSoundLoaded_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    vc::At<int32_t>(self, 4) += 1;
    if (vc::At<int32_t>(self, 4) > 500) {
        return 1;
    }
    return CutsceneSound_IsLoaded_5A6DA0(cutsceneSound_CF4290) != 0;
}

// 0x5FCBD0 (thiscall): ConditionIsMusicStreamLoaded (given up after 500 tries)
uint8_t __fastcall MusicStreamLoaded_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    vc::At<int32_t>(self, 4) += 1;
    if (vc::At<int32_t>(self, 4) > 500) {
        return 1;
    }
    return MusicStreams_IsLoaded_58EE40(musicStreams_CF42D4);
}

// 0x5FCBF0 (thiscall): ConditionIsSoundBankLoaded (true every 101st try)
uint8_t __fastcall SoundBankLoaded_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    vc::At<int32_t>(self, 8) += 1;
    if (vc::At<int32_t>(self, 8) > 100) {
        vc::At<int32_t>(self, 8) = 0;
        return 1;
    }
    int32_t bank = SoundBanks_Find_597FC0(soundBanks_CF5DC4, vc::At<int32_t>(self, 4));
    return SoundBanks_IsLoaded_5995F0(soundBanks_CF5DC4, bank);
}

// 0x5FCC30 (thiscall): ConditionIsSpeechStreamFree: either of the two streams idle
uint32_t __fastcall SpeechStreamFree_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    int32_t i = speechStreamIndex_CF4644;
    if (vc::At<int32_t>(speechStreams_CF458C, i * 0x5C) == 0) {
        return 1;
    }
    if (vc::At<int32_t>(speechStreams_CF458C, ((i - 1) & 1) * 0x5C) == 0) {
        return 1;
    }
    return 0;
}

void __fastcall Fields_5FCC70(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 8) = -1;
    self[0xC] = 0;
}

// 0x5FCCB0 (thiscall): ConditionOpeningBranchChildHasChild's deleting destructor (two keys)
uint8_t* __fastcall OpeningBranchChild_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    vc::At<uint32_t>(self, 0) = 0x0092E7B8;
    NameKey_Destruct(self + 8);
    NameKey_Destruct(self + 4);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

void __fastcall Fields_5FCCE0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 0.35f;
    self[0x20] = 0;
    self[0x21] = 0;
    vc::At<float>(self, 8) = 0.0f;
    self[0x22] = 0;
    vc::At<float>(self, 0xC) = 0.0f;
    self[0x23] = 0;
    vc::At<float>(self, 0x10) = 0.0f;
    self[0x24] = 1;
    vc::At<float>(self, 0x14) = 0.0f;
    self[0x25] = 0;
    vc::At<float>(self, 0x18) = 1.0f;
    vc::At<float>(self, 0x1C) = 0.0f;
    vc::At<float>(self, 0x28) = 1.0f;
}

// 0x5FCD40 (thiscall): ConditionMissionSuccessCount's constructor
uint8_t* __fastcall MissionSuccessCount_Construct(uint8_t* self, re::Edx) {
    TrackOrCondition_Construct(self);
    vc::At<uint32_t>(self, 0) = 0x0092E7E0;
    NameKey_Default(&vc::At<uint32_t>(self, 0xC));
    vc::At<int32_t>(self, 0x10) = -2;
    return self;
}

void __fastcall Fields_5FCD70(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 8) = 4;
}

// 0x5FCD80 (thiscall): ConditionMissionSuccessCount: the named mission's (looked up
// once) or all missions' successes compared
uint8_t __fastcall MissionSuccessCount_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t count = missionSuccessCount_20C43E8;
    uint32_t* name = &vc::At<uint32_t>(self, 0xC);
    if (*name != 0) {
        if (vc::At<int32_t>(self, 0x10) == -2) {
            vc::At<int32_t>(self, 0x10) = Missions_FindByName(missionManager, name);
        }
        int32_t mission = vc::At<int32_t>(self, 0x10);
        if (mission == -1) {
            count = 0;
        } else {
            count = vc::At<uint16_t>(Missions_Get_6AA7C0(missionManager, mission), 2);
        }
    }
    return CompareInts(count, vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8));
}

void __fastcall Fields_5FCDE0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 8) = 2;
}

// 0x5FCDF0 (thiscall, this unused): ConditionIsTargetInRange's range `kind` (0..2)
float __fastcall IsTargetInRange_Range(uint8_t*, re::Edx, int kind) {
    switch (kind) {
    case 0: return vc::At<float>(targetRanges_20C889C, 0x2C);
    case 1: return vc::At<float>(targetRanges_20C889C, 0x30);
    case 2: return vc::At<float>(targetRanges_20C889C, 0x34);
    default: return 0.0f;
    }
}

void __fastcall Fields_5FCE30(uint8_t* self, re::Edx) {
    self[4] = 0;
    vc::At<float>(self, 8) = 0.5f;
    vc::At<float>(self, 0xC) = 0.8f;
}
void __fastcall Fields_5FCE50(uint8_t* self, re::Edx) {
    self[9] = 0;
    vc::At<float>(self, 4) = 0.0f;
    vc::At<float>(self, 0xC) = 2.0f;
}
void __fastcall Fields_5FCEB0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0x20;
}

// 0x5FCEC0 (thiscall): ConditionCanSecondaryMissionPlay (the game passes the kind byte
// in a dword whose upper bytes are the condition's address)
uint8_t __fastcall CanSecondaryMissionPlay_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    uint32_t arg = (uint32_t(reinterpret_cast<uintptr_t>(self)) & 0xFFFFFF00u) | self[8];
    return Missions_SecondaryState_6AC0F0(missionManager, arg) == vc::At<int32_t>(self, 4);
}

// 0x5FCEF0 / 0x5FD120 / 0x5FD250 / 0x5FD260 / 0x5FD3A0 (thiscall): a world flag
uint8_t __fastcall CanLaunchMission_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return canLaunchMission_20C43EC;
}
uint8_t __fastcall IsParametricStatic_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return parametricStatic_1B6502C;
}
uint8_t __fastcall IsRumbleAmbient_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return rumbleAmbient_C2AFA0;
}
uint8_t __fastcall IsRumbleMission_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return rumbleMission_C2AFA1;
}
uint8_t __fastcall IsIgnoreTruants_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return ignoreTruants_C2AFA2;
}

// 0x5FCF00 (thiscall): ConditionInClass
uint8_t __fastcall InClass_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return Missions_InClass_6AA840(missionManager);
}

void __fastcall Fields_5FCF10(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 30.0f;
}
void __fastcall Fields_5FCF20(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 0xC) = 0;
    vc::At<float>(self, 4) = 10.0f;
    self[8] = 1;
}
void __fastcall Fields_5FCF40(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0x19;
}
void __fastcall Fields_5FCF50(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0x17;
}
void __fastcall Fields_5FCF60(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 3;
}
void __fastcall Fields_5FCF70(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 8) = 5;
}

// 0x5FCF80 (thiscall): ConditionHasGiftItemOfType
uint8_t __fastcall HasGiftItemOfType_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return Gifts_Find_541EC0(vc::At<int32_t>(self, 4)) >= 0;
}

// 0x5FCFA0 (thiscall): ConditionHasPlayerMaxItems: the player has as many of the item as
// the other item's limit (true when it has none)
uint8_t __fastcall HasPlayerMaxItems_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t item = ModelIndexFromName_51AE20(vc::At<int32_t>(self, 4), 0, 0x2FA7);
    uint8_t* info = ItemInfo_51B450(ModelIndexFromName_51AE20(vc::At<int32_t>(self, 8), 0, 0x2FA7));
    if (!info) {
        return 1;
    }
    int32_t max = vc::At<int32_t>(info, 0x34);
    if (Inventory_PlayerCount_541E00(item) < max) {
        return 0;
    }
    return 1;
}

void __fastcall Fields_5FD000(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 0xC) = 0;
    NameKey_FromString(&vc::At<uint32_t>(self, 4), "");
    vc::At<int32_t>(self, 8) = 5;
}

// 0x5FD020 (thiscall): ConditionPlayerHasItemCount
uint8_t __fastcall PlayerHasItemCount_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t item = ModelIndexFromName_51AE20(vc::At<int32_t>(self, 4), 0, 0x2FA7);
    float wanted = float(vc::At<int32_t>(self, 0xC));
    int32_t op = vc::At<int32_t>(self, 8);
    return CompareFloats(float(Inventory_PlayerCount_541E00(item)), wanted, op);
}

void __fastcall Fields_5FD060(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 0xC) = 5;
}

// 0x5FD070 (thiscall): ConditionStatInt
uint8_t __fastcall StatInt_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t value = Stats_GetInt_459A00(vc::At<int32_t>(self, 8));
    return CompareFloats(float(value), float(vc::At<int32_t>(self, 4)), vc::At<int32_t>(self, 0xC));
}

void __fastcall Fields_5FD0B0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 0xC) = 5;
    vc::At<float>(self, 4) = 0.0f;
}

// 0x5FD0C0 (thiscall): ConditionStatFloat
uint8_t __fastcall StatFloat_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    float value = Stats_GetFloat_45A900(vc::At<int32_t>(self, 8));
    return CompareFloats(value, vc::At<float>(self, 4), vc::At<int32_t>(self, 0xC));
}

// 0x5FD100 (thiscall): ConditionCheckWeatherType
uint8_t __fastcall CheckWeatherType_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return int32_t(weatherType_CE756C) == vc::At<int32_t>(self, 4);
}

void __fastcall Fields_5FD130(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 3;
    vc::At<float>(self, 4) = 1.0f;
}
void __fastcall Fields_5FD140(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
}
void __fastcall Fields_5FD150(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 2;
}

// 0x5FD160 (thiscall): ConditionPlayerIsInStealthProp
uint8_t __fastcall PlayerIsInStealthProp_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return playerInfo_C1AEA8[0x1E91];
}

void __fastcall Fields_5FD170(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 0;
    vc::At<float>(self, 4) = 0.0f;
}
void __fastcall Fields_5FD180(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 8) = 5;
    vc::At<float>(self, 4) = 0.5f;
}
void __fastcall Fields_5FD1C0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 0.5f;
    self[0xD] = 0;
    self[0xC] = 1;
    vc::At<int32_t>(self, 8) = 5;
    self[0xE] = 0;
}
void __fastcall Fields_5FD270(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = -1.5707964f;
    self[0xC] = 1;
    self[0xD] = 1;
    vc::At<float>(self, 8) = 1.5707964f;
}
void __fastcall Fields_5FD290(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 1;
    vc::At<int32_t>(self, 8) = 5;
}

// 0x5FD2A0 (thiscall): ConditionPlayerHasPunishmentPoints
uint8_t __fastcall PlayerHasPunishmentPoints_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return CompareInts(vc::At<int32_t>(playerInfo_C1AEA8, 0x1D40), vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8));
}

void __fastcall Fields_5FD2C0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 8) = 0.5f;
}
void __fastcall Fields_5FD2D0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 2.0f;
}

// 0x5FD2E0 (thiscall): ConditionMissionAvailable: the named mission open and not done
uint32_t __fastcall MissionAvailable_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    uint32_t* name = &vc::At<uint32_t>(self, 4);
    if (*name == 0) {
        return 0;
    }
    int32_t mission = Missions_FindByName(missionManager, name);
    if (mission == -1) {
        return 0;
    }
    if ((Missions_Get_6AA7C0(missionManager, mission)[4] & 1) && !Missions_IsDone_6AA940(missionManager, mission)) {
        return 1;
    }
    return 0;
}

// 0x5FD340 (thiscall): ConditionNumWeapons: the weapons the player carries compared
uint8_t __fastcall NumWeapons_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t count = 0;
    for (int32_t model = weaponModelFirst_A136B0; model <= weaponModelLast_A136B4; model++) {
        if (model == skippedWeapon_BF4108 || model == skippedWeapon_BF4100) {
            continue;
        }
        if (Inventory_Count_45CF40(vc::At<uint8_t*>(playerInfo_C1AEA8, 0x1C4), model) > 0) {
            count++;
        }
    }
    return CompareInts(count, vc::At<int32_t>(self, 4), vc::At<int32_t>(self, 8));
}

void __fastcall Fields_5FD3B0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 3.0f;
}
void __fastcall Fields_5FD3C0(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 3;
    vc::At<int32_t>(self, 8) = 5;
}

// 0x5FD3D0 (thiscall): ConditionIsTagWaiting
uint8_t __fastcall IsTagWaiting_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    return tagging_C67538[0x2888];
}

// 0x5FD3E0 (thiscall): ConditionIsTagDrawing: tagging with no progress yet
uint32_t __fastcall IsTagDrawing_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    uint8_t* tag = tagging_C67538;
    if (tag[0x2848] && 0.0f == vc::At<float>(tag, 0x288C)) {
        return 1;
    }
    return 0;
}

void __fastcall Fields_5FD410(uint8_t* self, re::Edx) {
    self[4] = 1;
    self[5] = 1;
}

// 0x5FD420 (thiscall): ConditionIsNISorCutsceneRunning
uint8_t __fastcall IsNisOrCutsceneRunning_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    if (self[4] && Cutscene_IsRunning_5BB380()) {
        return 1;
    }
    if (self[5] && cutsceneRunning_20C5BE4) {
        return 1;
    }
    return 0;
}

void __fastcall Fields_5FD450(uint8_t* self, re::Edx) {
    vc::At<float>(self, 4) = 10.0f;
}
void __fastcall Fields_5FD460(uint8_t* self, re::Edx) {
    vc::At<int32_t>(self, 4) = 0;
    vc::At<int32_t>(self, 8) = 0;
    vc::At<int32_t>(self, 0xC) = 0;
}

// 0x5FD470 (thiscall): ConditionScriptSaveData
uint8_t __fastcall ScriptSaveData_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return CompareInts(vc::At<int32_t>(self, 0xC), scriptSaveData_C1AFB8[vc::At<int32_t>(self, 4)], vc::At<int32_t>(self, 8));
}

// 0x5FD490 (thiscall): ConditionPlayerHasPunishmentOfType
uint32_t __fastcall PlayerHasPunishmentOfType_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return vc::At<int32_t>(punishment_C0F78C, 0x2C) == vc::At<int32_t>(self, 4) ? 1 : 0;
}

// 0x5FD4B0 (thiscall): ConditionIsMiniObjectiveComplete
uint8_t __fastcall IsMiniObjectiveComplete_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    return PlayerInfo_IsMiniObjectiveComplete_446860(playerInfoSlot_C1AEA8, vc::At<int32_t>(self, 4));
}

void __fastcall Fields_5FD4D0(uint8_t* self, re::Edx) {
    self[4] = 1;
    self[5] = 0;
    self[6] = 0;
    self[7] = 0;
    self[8] = 1;
}
void __fastcall Fields_5FD4F0(uint8_t* self, re::Edx) {
    vc::At<float>(self, 8) = 1.0f;
    self[4] = 1;
    self[5] = 1;
}

// 0x5FD500 (thiscall): ConditionFrontEndActive: the front end on (+4) or off (+5) for
// less than +8 seconds
uint32_t __fastcall FrontEndActive_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int32_t elapsedMs = int32_t(timeInMs - frontEndTime_20C3308);
    double ms = double(elapsedMs);
    if (elapsedMs < 0) {
        ms = float(ms + double(twoTo32_900AC8));
    }
    float elapsed = float(ms / 1000.0);
    uint8_t within = vc::At<float>(self, 8) > elapsed ? 1 : 0;
    uint8_t active = frontEndActive_20C32FA;
    uint32_t on = self[4] && active ? 1 : 0;
    uint32_t off = self[5] && !active ? 1 : 0;
    if ((on | off) && within) {
        return 1;
    }
    return 0;
}

// 0x5FD680 (thiscall on an entity): upright (its matrix's up z above 0.88; without a
// matrix: yes)
uint32_t __fastcall Entity_IsUpright(uint8_t* self, re::Edx) {
    uint8_t* matrix = vc::At<uint8_t*>(self, 0x14);
    float upZ = matrix ? vc::At<float>(matrix, 0x28) : 1.0f;
    return double(upZ) > uprightLimit_92E898 ? 1 : 0;
}

} // namespace

#define CONDA_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

CONDA_REVERSED("ConditionWeightedRandom::IsA", 0x005FC770, &WeightedRandom_IsA);
CONDA_REVERSED("ConditionWeightedRandom::Accumulate", 0x005FC7E0, &WeightedRandom_Accumulate);
CONDA_REVERSED("Condition::Fields_5FC810", 0x005FC810, &Fields_5FC810);
CONDA_REVERSED("ConditionClothing::Evaluate", 0x005FC830, &Clothing_Evaluate);
CONDA_REVERSED("Condition::Fields_5FC860", 0x005FC860, &Fields_5FC860);
CONDA_REVERSED("Condition::Fields_5FC870", 0x005FC870, &Fields_5FC870);
CONDA_REVERSED("ConditionClothingStyle::Evaluate", 0x005FC890, &ClothingStyle_Evaluate);
CONDA_REVERSED("Condition::Fields_5FC8E0", 0x005FC8E0, &Fields_5FC8E0);
CONDA_REVERSED("ConditionClothingWarmthFactor::Evaluate", 0x005FC8F0, &ClothingWarmth_Evaluate);
CONDA_REVERSED("ConditionClothingSpecialKiss::Evaluate", 0x005FC910, &ClothingSpecialKiss_Evaluate);
CONDA_REVERSED("ConditionVisibleArea::Evaluate", 0x005FC920, &VisibleArea_Evaluate);
CONDA_REVERSED("Condition::Fields_5FC940", 0x005FC940, &Fields_5FC940);
CONDA_REVERSED("Condition::Fields_5FC950", 0x005FC950, &Fields_5FC950);
CONDA_REVERSED("ConditionConditionGroup::Destruct", 0x005FC990, &ConditionGroup_Destruct);
CONDA_REVERSED("Condition::Fields_5FC9A0", 0x005FC9A0, &Fields_5FC9A0);
CONDA_REVERSED("Condition::Fields_5FC9E0", 0x005FC9E0, &Fields_5FC9E0);
CONDA_REVERSED("Condition::Fields_5FC9F0", 0x005FC9F0, &Fields_5FC9F0);
CONDA_REVERSED("Condition::Fields_5FCA00", 0x005FCA00, &Fields_5FCA00);
CONDA_REVERSED("Condition::Fields_5FCA20", 0x005FCA20, &Fields_5FCA20);
CONDA_REVERSED("ConditionSeason::Evaluate", 0x005FCA30, &Season_Evaluate);
CONDA_REVERSED("ConditionOnMission::Evaluate", 0x005FCAA0, &OnMission_Evaluate);
CONDA_REVERSED("Condition::Fields_5FCB30", 0x005FCB30, &Fields_5FCB30);
CONDA_REVERSED("ConditionCutsceneSoundLoaded::Evaluate", 0x005FCBA0, &CutsceneSoundLoaded_Evaluate);
CONDA_REVERSED("ConditionIsMusicStreamLoaded::Evaluate", 0x005FCBD0, &MusicStreamLoaded_Evaluate);
CONDA_REVERSED("ConditionIsSoundBankLoaded::Evaluate", 0x005FCBF0, &SoundBankLoaded_Evaluate);
CONDA_REVERSED("ConditionIsSpeechStreamFree::Evaluate", 0x005FCC30, &SpeechStreamFree_Evaluate);
CONDA_REVERSED("Condition::Fields_5FCC70", 0x005FCC70, &Fields_5FCC70);
CONDA_REVERSED("ConditionOpeningBranchChildHasChild::DeletingDestruct", 0x005FCCB0, &OpeningBranchChild_DeletingDestruct);
CONDA_REVERSED("Condition::Fields_5FCCE0", 0x005FCCE0, &Fields_5FCCE0);
CONDA_REVERSED("ConditionMissionSuccessCount::Construct", 0x005FCD40, &MissionSuccessCount_Construct);
CONDA_REVERSED("Condition::Fields_5FCD70", 0x005FCD70, &Fields_5FCD70);
CONDA_REVERSED("ConditionMissionSuccessCount::Evaluate", 0x005FCD80, &MissionSuccessCount_Evaluate);
CONDA_REVERSED("Condition::Fields_5FCDE0", 0x005FCDE0, &Fields_5FCDE0);
CONDA_REVERSED("ConditionIsTargetInRange::Range", 0x005FCDF0, &IsTargetInRange_Range);
CONDA_REVERSED("Condition::Fields_5FCE30", 0x005FCE30, &Fields_5FCE30);
CONDA_REVERSED("Condition::Fields_5FCE50", 0x005FCE50, &Fields_5FCE50);
CONDA_REVERSED("Condition::Fields_5FCEB0", 0x005FCEB0, &Fields_5FCEB0);
CONDA_REVERSED("ConditionCanSecondaryMissionPlay::Evaluate", 0x005FCEC0, &CanSecondaryMissionPlay_Evaluate);
CONDA_REVERSED("ConditionCanLaunchMission::Evaluate", 0x005FCEF0, &CanLaunchMission_Evaluate);
CONDA_REVERSED("ConditionInClass::Evaluate", 0x005FCF00, &InClass_Evaluate);
CONDA_REVERSED("Condition::Fields_5FCF10", 0x005FCF10, &Fields_5FCF10);
CONDA_REVERSED("Condition::Fields_5FCF20", 0x005FCF20, &Fields_5FCF20);
CONDA_REVERSED("Condition::Fields_5FCF40", 0x005FCF40, &Fields_5FCF40);
CONDA_REVERSED("Condition::Fields_5FCF50", 0x005FCF50, &Fields_5FCF50);
CONDA_REVERSED("Condition::Fields_5FCF60", 0x005FCF60, &Fields_5FCF60);
CONDA_REVERSED("Condition::Fields_5FCF70", 0x005FCF70, &Fields_5FCF70);
CONDA_REVERSED("ConditionHasGiftItemOfType::Evaluate", 0x005FCF80, &HasGiftItemOfType_Evaluate);
CONDA_REVERSED("ConditionHasPlayerMaxItems::Evaluate", 0x005FCFA0, &HasPlayerMaxItems_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD000", 0x005FD000, &Fields_5FD000);
CONDA_REVERSED("ConditionPlayerHasItemCount::Evaluate", 0x005FD020, &PlayerHasItemCount_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD060", 0x005FD060, &Fields_5FD060);
CONDA_REVERSED("ConditionStatInt::Evaluate", 0x005FD070, &StatInt_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD0B0", 0x005FD0B0, &Fields_5FD0B0);
CONDA_REVERSED("ConditionStatFloat::Evaluate", 0x005FD0C0, &StatFloat_Evaluate);
CONDA_REVERSED("ConditionCheckWeatherType::Evaluate", 0x005FD100, &CheckWeatherType_Evaluate);
CONDA_REVERSED("ConditionIsParametricStatic::Evaluate", 0x005FD120, &IsParametricStatic_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD130", 0x005FD130, &Fields_5FD130);
CONDA_REVERSED("Condition::Fields_5FD140", 0x005FD140, &Fields_5FD140);
CONDA_REVERSED("Condition::Fields_5FD150", 0x005FD150, &Fields_5FD150);
CONDA_REVERSED("ConditionPlayerIsInStealthProp::Evaluate", 0x005FD160, &PlayerIsInStealthProp_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD170", 0x005FD170, &Fields_5FD170);
CONDA_REVERSED("Condition::Fields_5FD180", 0x005FD180, &Fields_5FD180);
CONDA_REVERSED("Condition::Fields_5FD1C0", 0x005FD1C0, &Fields_5FD1C0);
CONDA_REVERSED("ConditionIsRumbleMode_Ambient::Evaluate", 0x005FD250, &IsRumbleAmbient_Evaluate);
CONDA_REVERSED("ConditionIsRumbleMode_Mission::Evaluate", 0x005FD260, &IsRumbleMission_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD270", 0x005FD270, &Fields_5FD270);
CONDA_REVERSED("Condition::Fields_5FD290", 0x005FD290, &Fields_5FD290);
CONDA_REVERSED("ConditionPlayerHasPunishmentPoints::Evaluate", 0x005FD2A0, &PlayerHasPunishmentPoints_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD2C0", 0x005FD2C0, &Fields_5FD2C0);
CONDA_REVERSED("Condition::Fields_5FD2D0", 0x005FD2D0, &Fields_5FD2D0);
CONDA_REVERSED("ConditionMissionAvailable::Evaluate", 0x005FD2E0, &MissionAvailable_Evaluate);
CONDA_REVERSED("ConditionNumWeapons::Evaluate", 0x005FD340, &NumWeapons_Evaluate);
CONDA_REVERSED("ConditionIsIgnoreTruants::Evaluate", 0x005FD3A0, &IsIgnoreTruants_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD3B0", 0x005FD3B0, &Fields_5FD3B0);
CONDA_REVERSED("Condition::Fields_5FD3C0", 0x005FD3C0, &Fields_5FD3C0);
CONDA_REVERSED("ConditionIsTagWaiting::Evaluate", 0x005FD3D0, &IsTagWaiting_Evaluate);
CONDA_REVERSED("ConditionIsTagDrawing::Evaluate", 0x005FD3E0, &IsTagDrawing_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD410", 0x005FD410, &Fields_5FD410);
CONDA_REVERSED("ConditionIsNISorCutsceneRunning::Evaluate", 0x005FD420, &IsNisOrCutsceneRunning_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD450", 0x005FD450, &Fields_5FD450);
CONDA_REVERSED("Condition::Fields_5FD460", 0x005FD460, &Fields_5FD460);
CONDA_REVERSED("ConditionScriptSaveData::Evaluate", 0x005FD470, &ScriptSaveData_Evaluate);
CONDA_REVERSED("ConditionPlayerHasPunishmentOfType::Evaluate", 0x005FD490, &PlayerHasPunishmentOfType_Evaluate);
CONDA_REVERSED("ConditionIsMiniObjectiveComplete::Evaluate", 0x005FD4B0, &IsMiniObjectiveComplete_Evaluate);
CONDA_REVERSED("Condition::Fields_5FD4D0", 0x005FD4D0, &Fields_5FD4D0);
CONDA_REVERSED("Condition::Fields_5FD4F0", 0x005FD4F0, &Fields_5FD4F0);
CONDA_REVERSED("ConditionFrontEndActive::Evaluate", 0x005FD500, &FrontEndActive_Evaluate);
CONDA_REVERSED("Entity::IsUpright_5FD680", 0x005FD680, &Entity_IsUpright);
