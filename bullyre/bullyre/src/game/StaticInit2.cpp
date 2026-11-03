// The exe's static initializers (the C runtime runs the table at 0x8FC350 before WinMain:
// globals constructed, their destructors given to atexit) and those destructors, transcribed
// from their code by a script (each a few calls with constant arguments, a dword copied or a
// float constant computed). Calls go through the game's addresses, so the registry's modes
// apply; the float constants are computed in double, as at start-up (the x87 at its default
// 53 bits, before Direct3D sets 24). StaticInit.h has the helpers.

#include "game/StaticInit.h"

namespace staticinit {
namespace {

// 0x8E01F0 (initializer)
void __cdecl Fn8E01F0() {
    Thiscall(0x005FA910, 0x01B64BE4u, 0x0092F034u /* "ConditionHavePOIOfType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F35C0));
}

// 0x8E0210 (initializer)
void __cdecl Fn8E0210() {
    Thiscall(0x005FA910, 0x01B64BE8u, 0x0092F04Cu /* "ConditionHaveDynamicPOI" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F35D0));
}

// 0x8E0230 (initializer)
void __cdecl Fn8E0230() {
    Thiscall(0x005FA910, 0x01B64BECu, 0x0092F064u /* "ConditionSensePlayer" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F35E0));
}

// 0x8E0250 (initializer)
void __cdecl Fn8E0250() {
    Thiscall(0x005FA910, 0x01B64BF0u, 0x0092F07Cu /* "ConditionSensePlayerDuration" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F35F0));
}

// 0x8E0270 (initializer)
void __cdecl Fn8E0270() {
    Thiscall(0x005FA910, 0x01B64BF4u, 0x0092F09Cu /* "ConditionIsPlayer" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3600));
}

// 0x8E0290 (initializer)
void __cdecl Fn8E0290() {
    Thiscall(0x005FA910, 0x01B64BF8u, 0x0092F0B0u /* "ConditionIsPlayerFatigued" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3610));
}

// 0x8E02B0 (initializer)
void __cdecl Fn8E02B0() {
    Thiscall(0x005FA910, 0x01B64BFCu, 0x0092F0CCu /* "ConditionIsPlayerTired" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3620));
}

// 0x8E02D0 (initializer)
void __cdecl Fn8E02D0() {
    Thiscall(0x005FA910, 0x01B64C00u, 0x0092F0E4u /* "ConditionIsPatrolPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3630));
}

// 0x8E02F0 (initializer)
void __cdecl Fn8E02F0() {
    Thiscall(0x005FA910, 0x01B64C04u, 0x0092F0FCu /* "ConditionNeedsToDive" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3640));
}

// 0x8E0310 (initializer)
void __cdecl Fn8E0310() {
    Thiscall(0x005FA910, 0x01B64C08u, 0x0092F114u /* "ConditionSenseAttacker" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3650));
}

// 0x8E0330 (initializer)
void __cdecl Fn8E0330() {
    Thiscall(0x005FA910, 0x01B64C0Cu, 0x0092F12Cu /* "ConditionSenseTargetNow" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3660));
}

// 0x8E0350 (initializer)
void __cdecl Fn8E0350() {
    Thiscall(0x005FA910, 0x01B64C10u, 0x0092F144u /* "ConditionAttacked" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3670));
}

// 0x8E0370 (initializer)
void __cdecl Fn8E0370() {
    Thiscall(0x005FA910, 0x01B64C14u, 0x0092F158u /* "ConditionFocusAttacked" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3680));
}

// 0x8E0390 (initializer)
void __cdecl Fn8E0390() {
    Thiscall(0x005FA910, 0x01B64C18u, 0x0092F170u /* "ConditionStat" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3690));
}

// 0x8E03B0 (initializer)
void __cdecl Fn8E03B0() {
    Thiscall(0x005FA910, 0x01B64C1Cu, 0x0092F180u /* "ConditionAttackTimerPlayTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F36A0));
}

// 0x8E03D0 (initializer)
void __cdecl Fn8E03D0() {
    Thiscall(0x005FA910, 0x01B64C20u, 0x0092F1A0u /* "ConditionHitTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F36B0));
}

// 0x8E03F0 (initializer)
void __cdecl Fn8E03F0() {
    Thiscall(0x005FA910, 0x01B64C24u, 0x0092F1B4u /* "ConditionHitStunTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F36C0));
}

// 0x8E0410 (initializer)
void __cdecl Fn8E0410() {
    Thiscall(0x005FA910, 0x01B64C28u, 0x0092F1CCu /* "ConditionHitStunFinished" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F36D0));
}

// 0x8E0430 (initializer)
void __cdecl Fn8E0430() {
    Thiscall(0x005FA910, 0x01B64C2Cu, 0x0092F1E8u /* "ConditionBlockTimerPlayTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F36E0));
}

// 0x8E0450 (initializer)
void __cdecl Fn8E0450() {
    Thiscall(0x005FA910, 0x01B64C30u, 0x0092F204u /* "ConditionEvadeTimerPlayTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F36F0));
}

// 0x8E0470 (initializer)
void __cdecl Fn8E0470() {
    Thiscall(0x005FA910, 0x01B64C34u, 0x0092F220u /* "ConditionTargetsPunishmentPoints" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3700));
}

// 0x8E0490 (initializer)
void __cdecl Fn8E0490() {
    Thiscall(0x005FA910, 0x01B64C38u, 0x0092F244u /* "ConditionObjective" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3710));
}

// 0x8E04B0 (initializer)
void __cdecl Fn8E04B0() {
    Thiscall(0x005FA910, 0x01B64C3Cu, 0x0092F258u /* "ConditionObjectiveOfType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3720));
}

// 0x8E04D0 (initializer)
void __cdecl Fn8E04D0() {
    Thiscall(0x005FA910, 0x01B64C40u, 0x0092F274u /* "ConditionTargetIsVisible" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3730));
}

// 0x8E04F0 (initializer)
void __cdecl Fn8E04F0() {
    Thiscall(0x005FA910, 0x01B64C44u, 0x0092F290u /* "ConditionStimulus" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3740));
}

// 0x8E0510 (initializer)
void __cdecl Fn8E0510() {
    Thiscall(0x005FA910, 0x01B64C48u, 0x0092F2A4u /* "ConditionHasNewAudibleStimulus" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3750));
}

// 0x8E0530 (initializer)
void __cdecl Fn8E0530() {
    Thiscall(0x005FA910, 0x01B64C4Cu, 0x0092F2C4u /* "ConditionStimulusRepeated" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3760));
}

// 0x8E0550 (initializer)
void __cdecl Fn8E0550() {
    Thiscall(0x005FA910, 0x01B64C50u, 0x0092F2E0u /* "ConditionTargetStimulusSubject" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3770));
}

// 0x8E0570 (initializer)
void __cdecl Fn8E0570() {
    Thiscall(0x005FA910, 0x01B64C54u, 0x0092F300u /* "ConditionTargetStimulusObject" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3780));
}

// 0x8E0590 (initializer)
void __cdecl Fn8E0590() {
    Thiscall(0x005FA910, 0x01B64C58u, 0x0092F320u /* "ConditionTargetStimulusEntitySubject" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3790));
}

// 0x8E05B0 (initializer)
void __cdecl Fn8E05B0() {
    Thiscall(0x005FA910, 0x01B64C5Cu, 0x0092F348u /* "ConditionTargetStimulusEntityObject" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F37A0));
}

// 0x8E05D0 (initializer)
void __cdecl Fn8E05D0() {
    Thiscall(0x005FA910, 0x01B64C60u, 0x0092F36Cu /* "ConditionIsStimulusObject" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F37B0));
}

// 0x8E05F0 (initializer)
void __cdecl Fn8E05F0() {
    Thiscall(0x005FA910, 0x01B64C64u, 0x0092F388u /* "ConditionIsInFactionWithStimulusObject" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F37C0));
}

// 0x8E0610 (initializer)
void __cdecl Fn8E0610() {
    Thiscall(0x005FA910, 0x01B64C68u, 0x0092F3B0u /* "ConditionIgnoreAttacks" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F37D0));
}

// 0x8E0630 (initializer)
void __cdecl Fn8E0630() {
    Thiscall(0x005FA910, 0x01B64C6Cu, 0x0092F3C8u /* "ConditionInCar" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F37E0));
}

// 0x8E0650 (initializer)
void __cdecl Fn8E0650() {
    Thiscall(0x005FA910, 0x01B64C70u, 0x0092F3D8u /* "ConditionOnBike" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F37F0));
}

// 0x8E0670 (initializer)
void __cdecl Fn8E0670() {
    Thiscall(0x005FA910, 0x01B64C74u, 0x0092F3E8u /* "ConditionOnVehicleOfClass" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3800));
}

// 0x8E0690 (initializer)
void __cdecl Fn8E0690() {
    Thiscall(0x005FA910, 0x01B64C78u, 0x0092F404u /* "ConditionInBikeRace" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3810));
}

// 0x8E06B0 (initializer)
void __cdecl Fn8E06B0() {
    Thiscall(0x005FA910, 0x01B64C7Cu, 0x0092F418u /* "ConditionMissionActive" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3820));
}

// 0x8E06D0 (initializer)
void __cdecl Fn8E06D0() {
    Thiscall(0x005FA910, 0x01B64C80u, 0x0092F430u /* "ConditionHavePath" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3830));
}

// 0x8E06F0 (initializer)
void __cdecl Fn8E06F0() {
    Thiscall(0x005FA910, 0x01B64C84u, 0x0092F444u /* "ConditionController" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3840));
}

// 0x8E0710 (initializer)
void __cdecl Fn8E0710() {
    Thiscall(0x005FA910, 0x01B64C88u, 0x0092F458u /* "ConditionSizeOfGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3850));
}

// 0x8E0730 (initializer)
void __cdecl Fn8E0730() {
    Thiscall(0x005FA910, 0x01B64C8Cu, 0x0092F470u /* "ConditionRangeToTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3860));
}

// 0x8E0750 (initializer)
void __cdecl Fn8E0750() {
    Thiscall(0x005FA910, 0x01B64C90u, 0x0092F488u /* "ConditionTimeSincePedalling" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3870));
}

// 0x8E0770 (initializer)
void __cdecl Fn8E0770() {
    Thiscall(0x005FA910, 0x01B64C94u, 0x0092F4A4u /* "ConditionBikeSprintAmount" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3880));
}

// 0x8E0790 (initializer)
void __cdecl Fn8E0790() {
    Thiscall(0x005FA910, 0x01B64C98u, 0x0092F4C0u /* "ConditionRangeToFocus" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3890));
}

// 0x8E07B0 (initializer)
void __cdecl Fn8E07B0() {
    Thiscall(0x005FA910, 0x01B64C9Cu, 0x0092F4D8u /* "ConditionRangeToObjectiveVehicle" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F38A0));
}

// 0x8E07D0 (initializer)
void __cdecl Fn8E07D0() {
    Thiscall(0x005FA910, 0x01B64CA0u, 0x0092F4FCu /* "ConditionRandom" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F38B0));
}

// 0x8E07F0 (initializer)
void __cdecl Fn8E07F0() {
    Thiscall(0x005FA910, 0x01B64CA4u, 0x0092F50Cu /* "ConditionWeightedRandom" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F38C0));
}

// 0x8E0810 (initializer)
void __cdecl Fn8E0810() {
    Thiscall(0x005FA910, 0x01B64CA8u, 0x0092F524u /* "ConditionFactionScan" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F38D0));
}

// 0x8E0830 (initializer)
void __cdecl Fn8E0830() {
    Thiscall(0x005FA910, 0x01B64CACu, 0x0092F53Cu /* "ConditionTargetMastersAttacker" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F38E0));
}

// 0x8E0850 (initializer)
void __cdecl Fn8E0850() {
    Thiscall(0x005FA910, 0x01B64CB0u, 0x0092F55Cu /* "ConditionTargetFriendsAttacker" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F38F0));
}

// 0x8E0870 (initializer)
void __cdecl Fn8E0870() {
    Thiscall(0x005FA910, 0x01B64CB4u, 0x0092F57Cu /* "ConditionNearbyDockLocation" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3900));
}

// 0x8E0890 (initializer)
void __cdecl Fn8E0890() {
    Thiscall(0x005FA910, 0x01B64CB8u, 0x0092F598u /* "ConditionNearbyFleeDockLocation" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3910));
}

// 0x8E08B0 (initializer)
void __cdecl Fn8E08B0() {
    Thiscall(0x005FA910, 0x01B64CBCu, 0x0092F5B8u /* "ConditionTargetFaction" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3920));
}

// 0x8E08D0 (initializer)
void __cdecl Fn8E08D0() {
    Thiscall(0x005FA910, 0x01B64CC0u, 0x0092F5D0u /* "ConditionClothing" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3930));
}

// 0x8E08F0 (initializer)
void __cdecl Fn8E08F0() {
    Thiscall(0x005FA910, 0x01B64CC4u, 0x0092F5E4u /* "ConditionTargetClothing" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3940));
}

// 0x8E0910 (initializer)
void __cdecl Fn8E0910() {
    Thiscall(0x005FA910, 0x01B64CC8u, 0x0092F5FCu /* "ConditionClothingStyle" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3950));
}

// 0x8E0930 (initializer)
void __cdecl Fn8E0930() {
    Thiscall(0x005FA910, 0x01B64CCCu, 0x0092F614u /* "ConditionClothingWarmthFactor" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3960));
}

// 0x8E0950 (initializer)
void __cdecl Fn8E0950() {
    Thiscall(0x005FA910, 0x01B64CD0u, 0x0092F634u /* "ConditionClothingSpecialKiss" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3970));
}

// 0x8E0970 (initializer)
void __cdecl Fn8E0970() {
    Thiscall(0x005FA910, 0x01B64CD4u, 0x0092F654u /* "ConditionAttackRole" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3980));
}

// 0x8E0990 (initializer)
void __cdecl Fn8E0990() {
    Thiscall(0x005FA910, 0x01B64CD8u, 0x0092F668u /* "ConditionFightClass" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3990));
}

// 0x8E09B0 (initializer)
void __cdecl Fn8E09B0() {
    Thiscall(0x005FA910, 0x01B64CDCu, 0x0092F67Cu /* "ConditionVisibleArea" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F39A0));
}

// 0x8E09D0 (initializer)
void __cdecl Fn8E09D0() {
    Thiscall(0x005FA910, 0x01B64CE0u, 0x0092F694u /* "ConditionCharacterSize" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F39B0));
}

// 0x8E09F0 (initializer)
void __cdecl Fn8E09F0() {
    Thiscall(0x005FA910, 0x01B64CE4u, 0x0092F6ACu /* "ConditionTargetRelativeCharacterSize" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F39C0));
}

// 0x8E0A10 (initializer)
void __cdecl Fn8E0A10() {
    Thiscall(0x005FA910, 0x01B64CE8u, 0x0092F6D4u /* "ConditionJoinCombatZone" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F39D0));
}

// 0x8E0A30 (initializer)
void __cdecl Fn8E0A30() {
    Thiscall(0x005FA910, 0x01B64CECu, 0x0092F6ECu /* "ConditionInCombatZone" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F39E0));
}

// 0x8E0A50 (initializer)
void __cdecl Fn8E0A50() {
    Thiscall(0x005FA910, 0x01B64CF0u, 0x0092F704u /* "ConditionAttackRoleTest" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F39F0));
}

// 0x8E0A70 (initializer)
void __cdecl Fn8E0A70() {
    Thiscall(0x005FA910, 0x01B64CF4u, 0x0092F71Cu /* "ConditionAcquireFocus" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A00));
}

// 0x8E0A90 (initializer)
void __cdecl Fn8E0A90() {
    Thiscall(0x005FA910, 0x01B64CF8u, 0x0092F734u /* "ConditionAcquireTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A10));
}

// 0x8E0AB0 (initializer)
void __cdecl Fn8E0AB0() {
    Thiscall(0x005FA910, 0x01B64CFCu, 0x0092F74Cu /* "ConditionAcquirePunishableTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A20));
}

// 0x8E0AD0 (initializer)
void __cdecl Fn8E0AD0() {
    Thiscall(0x005FA910, 0x01B64D00u, 0x0092F770u /* "ConditionAcquireRandomTargetInGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A30));
}

// 0x8E0AF0 (initializer)
void __cdecl Fn8E0AF0() {
    Thiscall(0x005FA910, 0x01B64D04u, 0x0092F794u /* "ConditionAcquireAuthorityFocus" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A40));
}

// 0x8E0B10 (initializer)
void __cdecl Fn8E0B10() {
    Thiscall(0x005FA910, 0x01B64D08u, 0x0092F7B4u /* "ConditionAcquireFactionFocus" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A50));
}

// 0x8E0B30 (initializer)
void __cdecl Fn8E0B30() {
    Thiscall(0x005FA910, 0x01B64D0Cu, 0x0092F7D4u /* "ConditionIsHealthAtHumiliationLevel" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A60));
}

// 0x8E0B50 (initializer)
void __cdecl Fn8E0B50() {
    Thiscall(0x005FA910, 0x01B64D10u, 0x0092F7F8u /* "ConditionTargetedByTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A70));
}

// 0x8E0B70 (initializer)
void __cdecl Fn8E0B70() {
    Thiscall(0x005FA910, 0x01B64D14u, 0x0092F814u /* "ConditionTargetedByPlayer" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A80));
}

// 0x8E0B90 (initializer)
void __cdecl Fn8E0B90() {
    Thiscall(0x005FA910, 0x01B64D18u, 0x0092F830u /* "ConditionTargetPlayersGF" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3A90));
}

// 0x8E0BB0 (initializer)
void __cdecl Fn8E0BB0() {
    Thiscall(0x005FA910, 0x01B64D1Cu, 0x0092F84Cu /* "ConditionTargetConditionGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3AA0));
}

// 0x8E0BD0 (initializer)
void __cdecl Fn8E0BD0() {
    Thiscall(0x005FA910, 0x01B64D20u, 0x0092F86Cu /* "ConditionFocusConditionGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3AB0));
}

// 0x8E0BF0 (initializer)
void __cdecl Fn8E0BF0() {
    Thiscall(0x005FA910, 0x01B64D24u, 0x0092F88Cu /* "ConditionPedConditionGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3AC0));
}

// 0x8E0C10 (initializer)
void __cdecl Fn8E0C10() {
    Thiscall(0x005FA910, 0x01B64D28u, 0x0092F8A8u /* "ConditionAIConditionGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3AD0));
}

// 0x8E0C30 (initializer)
void __cdecl Fn8E0C30() {
    Thiscall(0x005FA910, 0x01B64D2Cu, 0x0092F8C4u /* "ConditionAttackerConditionGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3AE0));
}

// 0x8E0C50 (initializer)
void __cdecl Fn8E0C50() {
    Thiscall(0x005FA910, 0x01B64D30u, 0x0092F8E4u /* "ConditionProjectile" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3AF0));
}

// 0x8E0C70 (initializer)
void __cdecl Fn8E0C70() {
    Thiscall(0x005FA910, 0x01B64D34u, 0x0092F8F8u /* "ConditionFightToken" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B00));
}

// 0x8E0C90 (initializer)
void __cdecl Fn8E0C90() {
    Thiscall(0x005FA910, 0x01B64D38u, 0x0092F90Cu /* "ConditionAttackPhase" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B10));
}

// 0x8E0CB0 (initializer)
void __cdecl Fn8E0CB0() {
    Thiscall(0x005FA910, 0x01B64D3Cu, 0x0092F924u /* "ConditionPedModelID" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B20));
}

// 0x8E0CD0 (initializer)
void __cdecl Fn8E0CD0() {
    Thiscall(0x005FA910, 0x01B64D40u, 0x0092F938u /* "ConditionEngageDistance" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B30));
}

// 0x8E0CF0 (initializer)
void __cdecl Fn8E0CF0() {
    Thiscall(0x005FA910, 0x01B64D44u, 0x0092F950u /* "ConditionInAttackCone" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B40));
}

// 0x8E0D10 (initializer)
void __cdecl Fn8E0D10() {
    Thiscall(0x005FA910, 0x01B64D48u, 0x0092F968u /* "ConditionCollisionStart" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B50));
}

// 0x8E0D30 (initializer)
void __cdecl Fn8E0D30() {
    Thiscall(0x005FA910, 0x01B64D4Cu, 0x0092F980u /* "ConditionHitReactionEnd" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B60));
}

// 0x8E0D50 (initializer)
void __cdecl Fn8E0D50() {
    Thiscall(0x005FA910, 0x01B64D50u, 0x0092F998u /* "ConditionFactionNear" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B70));
}

// 0x8E0D70 (initializer)
void __cdecl Fn8E0D70() {
    Thiscall(0x005FA910, 0x01B64D54u, 0x0092F9B0u /* "ConditionFindTandemHelper" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B80));
}

// 0x8E0D90 (initializer)
void __cdecl Fn8E0D90() {
    Thiscall(0x005FA910, 0x01B64D58u, 0x0092F9CCu /* "ConditionIsTandemHelper" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3B90));
}

// 0x8E0DB0 (initializer)
void __cdecl Fn8E0DB0() {
    Thiscall(0x005FA910, 0x01B64D5Cu, 0x0092F9E4u /* "ConditionPedIsTrigger" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3BA0));
}

// 0x8E0DD0 (initializer)
void __cdecl Fn8E0DD0() {
    Thiscall(0x005FA910, 0x01B64D60u, 0x0092F9FCu /* "ConditionGrapplingHookAnchord" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3BB0));
}

// 0x8E0E10 (initializer)
void __cdecl Fn8E0E10() {
    Thiscall(0x005FA910, 0x01B64D68u, 0x0092FA1Cu /* "ConditionDistToTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3BC0));
}

// 0x8E0E30 (initializer)
void __cdecl Fn8E0E30() {
    Thiscall(0x005FA910, 0x01B64D6Cu, 0x0092FA34u /* "ConditionHeightOfTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3BD0));
}

// 0x8E0E50 (initializer)
void __cdecl Fn8E0E50() {
    Thiscall(0x005FA910, 0x01B64D70u, 0x0092FA4Cu /* "ConditionLastSurfaceOn" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3BE0));
}

// 0x8E0E70 (initializer)
void __cdecl Fn8E0E70() {
    Thiscall(0x005FA910, 0x01B64D74u, 0x0092FA64u /* "ConditionSurfaceGrade" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3BF0));
}

// 0x8E0E90 (initializer)
void __cdecl Fn8E0E90() {
    Thiscall(0x005FA910, 0x01B64D78u, 0x0092FA7Cu /* "ConditionSeason" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C00));
}

// 0x8E0EB0 (initializer)
void __cdecl Fn8E0EB0() {
    Thiscall(0x005FA910, 0x01B64D7Cu, 0x0092FA8Cu /* "ConditionHaveIAttacked" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C10));
}

// 0x8E0ED0 (initializer)
void __cdecl Fn8E0ED0() {
    Thiscall(0x005FA910, 0x01B64D80u, 0x0092FAA4u /* "ConditionDriveForce" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C20));
}

// 0x8E0EF0 (initializer)
void __cdecl Fn8E0EF0() {
    Thiscall(0x005FA910, 0x01B64D84u, 0x0092FAB8u /* "ConditionOnMission" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C30));
}

// 0x8E0F10 (initializer)
void __cdecl Fn8E0F10() {
    Thiscall(0x005FA910, 0x01B64D88u, 0x0092FACCu /* "ConditionAnimationGroupLoaded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C40));
}

// 0x8E0F30 (initializer)
void __cdecl Fn8E0F30() {
    Thiscall(0x005FA910, 0x01B64D8Cu, 0x0092FAECu /* "ConditionCutsceneSoundLoaded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C50));
}

// 0x8E0F50 (initializer)
void __cdecl Fn8E0F50() {
    Thiscall(0x005FA910, 0x01B64D90u, 0x0092FB0Cu /* "ConditionIsMusicStreamLoaded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C60));
}

// 0x8E0F70 (initializer)
void __cdecl Fn8E0F70() {
    Thiscall(0x005FA910, 0x01B64D94u, 0x0092FB2Cu /* "ConditionIsSoundBankLoaded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C70));
}

// 0x8E0F90 (initializer)
void __cdecl Fn8E0F90() {
    Thiscall(0x005FA910, 0x01B64D98u, 0x0092FB48u /* "ConditionIsSpeechBusy" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C80));
}

// 0x8E0FB0 (initializer)
void __cdecl Fn8E0FB0() {
    Thiscall(0x005FA910, 0x01B64D9Cu, 0x0092FB60u /* "ConditionIsSpeechStreamFree" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3C90));
}

// 0x8E0FD0 (initializer)
void __cdecl Fn8E0FD0() {
    Thiscall(0x005FA910, 0x01B64DA0u, 0x0092FB7Cu /* "ConditionIsSpeechPreloaded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3CA0));
}

// 0x8E0FF0 (initializer)
void __cdecl Fn8E0FF0() {
    Thiscall(0x005FA910, 0x01B64DA4u, 0x0092FB98u /* "ConditionHasActionNode" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3CB0));
}

// 0x8E1010 (initializer)
void __cdecl Fn8E1010() {
    Thiscall(0x005FA910, 0x01B64DA8u, 0x0092FBB0u /* "ConditionHasSpeechEvent" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3CC0));
}

// 0x8E1030 (initializer)
void __cdecl Fn8E1030() {
    Thiscall(0x005FA910, 0x01B64DACu, 0x0092FBC8u /* "ConditionOpeningBranchChildHasChild" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3CD0));
}

// 0x8E1050 (initializer)
void __cdecl Fn8E1050() {
    Thiscall(0x005FA910, 0x01B64DB0u, 0x0092FBECu /* "ConditionDistToGround" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3CE0));
}

// 0x8E1070 (initializer)
void __cdecl Fn8E1070() {
    Thiscall(0x005FA910, 0x01B64DB4u, 0x0092FC04u /* "ConditionHasSweptSphereCollision" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3CF0));
}

// 0x8E1090 (initializer)
void __cdecl Fn8E1090() {
    Thiscall(0x005FA910, 0x01B64DB8u, 0x0092FC28u /* "ConditionMissionSuccessCount" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D00));
}

// 0x8E10B0 (initializer)
void __cdecl Fn8E10B0() {
    Thiscall(0x005FA910, 0x01B64DBCu, 0x0092FC48u /* "ConditionIsTargetOnBike" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D10));
}

// 0x8E10D0 (initializer)
void __cdecl Fn8E10D0() {
    Thiscall(0x005FA910, 0x01B64DC0u, 0x0092FC60u /* "ConditionIsFocusOnBike" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D20));
}

// 0x8E10F0 (initializer)
void __cdecl Fn8E10F0() {
    Thiscall(0x005FA910, 0x01B64DC4u, 0x0092FC78u /* "ConditionIsObjectiveVehicleOwner" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D30));
}

// 0x8E1110 (initializer)
void __cdecl Fn8E1110() {
    Thiscall(0x005FA910, 0x01B64DC8u, 0x0092FC9Cu /* "ConditionLineOfFireClear" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D40));
}

// 0x8E1130 (initializer)
void __cdecl Fn8E1130() {
    Thiscall(0x005FA910, 0x01B64DCCu, 0x0092FCB8u /* "ConditionIsGuarding" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D50));
}

// 0x8E1150 (initializer)
void __cdecl Fn8E1150() {
    Thiscall(0x005FA910, 0x01B64DD0u, 0x0092FCCCu /* "ConditionIsTargetCrouching" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D60));
}

// 0x8E1170 (initializer)
void __cdecl Fn8E1170() {
    Thiscall(0x005FA910, 0x01B64DD4u, 0x0092FCE8u /* "ConditionIsTargetOutOfReach" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D70));
}

// 0x8E1190 (initializer)
void __cdecl Fn8E1190() {
    Thiscall(0x005FA910, 0x01B64DD8u, 0x0092FD04u /* "ConditionIsStationary" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D80));
}

// 0x8E11B0 (initializer)
void __cdecl Fn8E11B0() {
    Thiscall(0x005FA910, 0x01B64DDCu, 0x0092FD1Cu /* "ConditionAttacksDisabled" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3D90));
}

// 0x8E11D0 (initializer)
void __cdecl Fn8E11D0() {
    Thiscall(0x005FA910, 0x01B64DE0u, 0x0092FD38u /* "ConditionIsStealthMissionChar" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3DA0));
}

// 0x8E11F0 (initializer)
void __cdecl Fn8E11F0() {
    Thiscall(0x005FA910, 0x01B64DE4u, 0x0092FD58u /* "ConditionWantToBeStealthy" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3DB0));
}

// 0x8E1210 (initializer)
void __cdecl Fn8E1210() {
    Thiscall(0x005FA910, 0x01B64DE8u, 0x0092FD74u /* "ConditionSpotted" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3DC0));
}

// 0x8E1230 (initializer)
void __cdecl Fn8E1230() {
    Thiscall(0x005FA910, 0x01B64DECu, 0x0092FD88u /* "ConditionHearPlayerFootsteps" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3DD0));
}

// 0x8E1250 (initializer)
void __cdecl Fn8E1250() {
    Thiscall(0x005FA910, 0x01B64DF0u, 0x0092FDA8u /* "ConditionIsTethered" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3DE0));
}

// 0x8E1270 (initializer)
void __cdecl Fn8E1270() {
    Thiscall(0x005FA910, 0x01B64DF4u, 0x0092FDBCu /* "ConditionIsInsideTether" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3DF0));
}

// 0x8E1290 (initializer)
void __cdecl Fn8E1290() {
    Thiscall(0x005FA910, 0x01B64DF8u, 0x0092FDD4u /* "ConditionIsTargetInRange" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E00));
}

// 0x8E12B0 (initializer)
void __cdecl Fn8E12B0() {
    Thiscall(0x005FA910, 0x01B64DFCu, 0x0092FDF0u /* "ConditionTargetRelativeHeight" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E10));
}

// 0x8E12D0 (initializer)
void __cdecl Fn8E12D0() {
    Thiscall(0x005FA910, 0x01B64E00u, 0x0092FE10u /* "ConditionIsTargetProp" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E20));
}

// 0x8E12F0 (initializer)
void __cdecl Fn8E12F0() {
    Thiscall(0x005FA910, 0x01B64E04u, 0x0092FE28u /* "ConditionTargetProxyProp" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E30));
}

// 0x8E1310 (initializer)
void __cdecl Fn8E1310() {
    Thiscall(0x005FA910, 0x01B64E08u, 0x0092FE44u /* "ConditionCanPropSetUsageState" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E40));
}

// 0x8E1330 (initializer)
void __cdecl Fn8E1330() {
    Thiscall(0x005FA910, 0x01B64E0Cu, 0x0092FE64u /* "ConditionInterval" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E50));
}

// 0x8E1350 (initializer)
void __cdecl Fn8E1350() {
    Thiscall(0x005FA910, 0x01B64E10u, 0x0092FE78u /* "ConditionCheckActionTimer" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E60));
}

// 0x8E1370 (initializer)
void __cdecl Fn8E1370() {
    Thiscall(0x005FA910, 0x01B64E14u, 0x0092FE94u /* "ConditionCanSecondaryMissionPlay" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E70));
}

// 0x8E1390 (initializer)
void __cdecl Fn8E1390() {
    Thiscall(0x005FA910, 0x01B64E18u, 0x0092FEB8u /* "ConditionCanLaunchMission" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E80));
}

// 0x8E13B0 (initializer)
void __cdecl Fn8E13B0() {
    Thiscall(0x005FA910, 0x01B64E1Cu, 0x0092FED4u /* "ConditionInClass" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3E90));
}

// 0x8E13D0 (initializer)
void __cdecl Fn8E13D0() {
    Thiscall(0x005FA910, 0x01B64E20u, 0x0092FEE8u /* "ConditionIsPlayerNearby" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3EA0));
}

// 0x8E13F0 (initializer)
void __cdecl Fn8E13F0() {
    Thiscall(0x005FA910, 0x01B64E24u, 0x0092FF00u /* "ConditionIsHungUp" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3EB0));
}

// 0x8E1410 (initializer)
void __cdecl Fn8E1410() {
    Thiscall(0x005FA910, 0x01B64E28u, 0x0092FF14u /* "ConditionStealthBehavior" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3EC0));
}

// 0x8E1430 (initializer)
void __cdecl Fn8E1430() {
    Thiscall(0x005FA910, 0x01B64E2Cu, 0x0092FF30u /* "ConditionIsOnSnow" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3ED0));
}

// 0x8E1450 (initializer)
void __cdecl Fn8E1450() {
    Thiscall(0x005FA910, 0x01B64E30u, 0x0092FF44u /* "ConditionFindSnow" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3EE0));
}

// 0x8E1470 (initializer)
void __cdecl Fn8E1470() {
    Thiscall(0x005FA910, 0x01B64E34u, 0x0092FF58u /* "ConditionFindSurface" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3EF0));
}

// 0x8E1490 (initializer)
void __cdecl Fn8E1490() {
    Thiscall(0x005FA910, 0x01B64E38u, 0x0092FF70u /* "ConditionRunningAttack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F00));
}

// 0x8E14B0 (initializer)
void __cdecl Fn8E14B0() {
    Thiscall(0x005FA910, 0x01B64E3Cu, 0x0092FF88u /* "ConditionIsAllyLeader" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F10));
}

// 0x8E14D0 (initializer)
void __cdecl Fn8E14D0() {
    Thiscall(0x005FA910, 0x01B64E40u, 0x0092FFA0u /* "ConditionIsAllyFollower" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F20));
}

// 0x8E14F0 (initializer)
void __cdecl Fn8E14F0() {
    Thiscall(0x005FA910, 0x01B64E44u, 0x0092FFB8u /* "ConditionIsTargetAlly" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F30));
}

// 0x8E1510 (initializer)
void __cdecl Fn8E1510() {
    Thiscall(0x005FA910, 0x01B64E48u, 0x0092FFD0u /* "ConditionCanRecruitTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F40));
}

// 0x8E1530 (initializer)
void __cdecl Fn8E1530() {
    Thiscall(0x005FA910, 0x01B64E4Cu, 0x0092FFECu /* "ConditionShouldSocialize" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F50));
}

// 0x8E1550 (initializer)
void __cdecl Fn8E1550() {
    Thiscall(0x005FA910, 0x01B64E50u, 0x00930008u /* "ConditionShouldAllowApology" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F60));
}

// 0x8E1570 (initializer)
void __cdecl Fn8E1570() {
    Thiscall(0x005FA910, 0x01B64E54u, 0x00930024u /* "ConditionIsSocialTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F70));
}

// 0x8E1590 (initializer)
void __cdecl Fn8E1590() {
    Thiscall(0x005FA910, 0x01B64E58u, 0x0093003Cu /* "ConditionUseSocialOverride" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F80));
}

// 0x8E15B0 (initializer)
void __cdecl Fn8E15B0() {
    Thiscall(0x005FA910, 0x01B64E5Cu, 0x00930058u /* "ConditionPlayerUseSocialOverride" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3F90));
}

// 0x8E15D0 (initializer)
void __cdecl Fn8E15D0() {
    Thiscall(0x005FA910, 0x01B64E60u, 0x0093007Cu /* "ConditionSocialAnimIs" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3FA0));
}

// 0x8E15F0 (initializer)
void __cdecl Fn8E15F0() {
    Thiscall(0x005FA910, 0x01B64E64u, 0x00930094u /* "ConditionSocialEmotionIs" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3FB0));
}

// 0x8E1610 (initializer)
void __cdecl Fn8E1610() {
    Thiscall(0x005FA910, 0x01B64E68u, 0x009300B0u /* "ConditionHumiliationGroupIs" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3FC0));
}

// 0x8E1630 (initializer)
void __cdecl Fn8E1630() {
    Thiscall(0x005FA910, 0x01B64E6Cu, 0x009300CCu /* "ConditionIsSocializing" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3FD0));
}

// 0x8E1650 (initializer)
void __cdecl Fn8E1650() {
    Thiscall(0x005FA910, 0x01B64E70u, 0x009300E4u /* "ConditionIsSocializingWithPlayer" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3FE0));
}

// 0x8E1670 (initializer)
void __cdecl Fn8E1670() {
    Thiscall(0x005FA910, 0x01B64E74u, 0x00930108u /* "ConditionIsWantingToSocialize" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3FF0));
}

// 0x8E1690 (initializer)
void __cdecl Fn8E1690() {
    Thiscall(0x005FA910, 0x01B64E78u, 0x00930128u /* "ConditionIsEmotionTowardsTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4000));
}

// 0x8E16B0 (initializer)
void __cdecl Fn8E16B0() {
    Thiscall(0x005FA910, 0x01B64E7Cu, 0x00930148u /* "ConditionIsEmotionTowardsPlayer" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4010));
}

// 0x8E16D0 (initializer)
void __cdecl Fn8E16D0() {
    Thiscall(0x005FA910, 0x01B64E80u, 0x00930168u /* "ConditionIsSocialStateWait" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4020));
}

// 0x8E16F0 (initializer)
void __cdecl Fn8E16F0() {
    Thiscall(0x005FA910, 0x01B64E84u, 0x00930184u /* "ConditionIsSocialStateAct" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4030));
}

// 0x8E1710 (initializer)
void __cdecl Fn8E1710() {
    Thiscall(0x005FA910, 0x01B64E88u, 0x009301A0u /* "ConditionIsSocialInitialReaction" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4040));
}

// 0x8E1730 (initializer)
void __cdecl Fn8E1730() {
    Thiscall(0x005FA910, 0x01B64E8Cu, 0x009301C4u /* "ConditionIsSocialInitialAction" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4050));
}

// 0x8E1750 (initializer)
void __cdecl Fn8E1750() {
    Thiscall(0x005FA910, 0x01B64E90u, 0x009301E4u /* "ConditionIsReadyToBeKissed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4060));
}

// 0x8E1770 (initializer)
void __cdecl Fn8E1770() {
    Thiscall(0x005FA910, 0x01B64E94u, 0x00930200u /* "ConditionSocialTypeFAT" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4070));
}

// 0x8E1790 (initializer)
void __cdecl Fn8E1790() {
    Thiscall(0x005FA910, 0x01B64E98u, 0x00930218u /* "ConditionSocialTypeGIRL" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4080));
}

// 0x8E17B0 (initializer)
void __cdecl Fn8E17B0() {
    Thiscall(0x005FA910, 0x01B64E9Cu, 0x00930230u /* "ConditionSocialTypeBULLY" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4090));
}

// 0x8E17D0 (initializer)
void __cdecl Fn8E17D0() {
    Thiscall(0x005FA910, 0x01B64EA0u, 0x0093024Cu /* "ConditionSocialTypeNERD" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F40A0));
}

// 0x8E17F0 (initializer)
void __cdecl Fn8E17F0() {
    Thiscall(0x005FA910, 0x01B64EA4u, 0x00930264u /* "ConditionSocialTypeAUTH" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F40B0));
}

// 0x8E1810 (initializer)
void __cdecl Fn8E1810() {
    Thiscall(0x005FA910, 0x01B64EA8u, 0x0093027Cu /* "ConditionSocialTypeGEN" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F40C0));
}

// 0x8E1830 (initializer)
void __cdecl Fn8E1830() {
    Thiscall(0x005FA910, 0x01B64EACu, 0x00930294u /* "ConditionSocialTypeOLD" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F40D0));
}

// 0x8E1850 (initializer)
void __cdecl Fn8E1850() {
    Thiscall(0x005FA910, 0x01B64EB0u, 0x009302ACu /* "ConditionSocialTypeNERDGIRL" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F40E0));
}

// 0x8E1870 (initializer)
void __cdecl Fn8E1870() {
    Thiscall(0x005FA910, 0x01B64EB4u, 0x009302C8u /* "ConditionSocialScriptCallback" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F40F0));
}

// 0x8E1890 (initializer)
void __cdecl Fn8E1890() {
    Thiscall(0x005FA910, 0x01B64EB8u, 0x009302E8u /* "ConditionSocialActionInterrupted" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4100));
}

// 0x8E18B0 (initializer)
void __cdecl Fn8E18B0() {
    Thiscall(0x005FA910, 0x01B64EBCu, 0x0093030Cu /* "ConditionIsKissCount" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4110));
}

// 0x8E18D0 (initializer)
void __cdecl Fn8E18D0() {
    Thiscall(0x005FA910, 0x01B64EC0u, 0x00930324u /* "ConditionTimeOfLastSocialAction" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4120));
}

// 0x8E18F0 (initializer)
void __cdecl Fn8E18F0() {
    Thiscall(0x005FA910, 0x01B64EC4u, 0x00930344u /* "ConditionIsTargetBusySocializing" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4130));
}

// 0x8E1910 (initializer)
void __cdecl Fn8E1910() {
    Thiscall(0x005FA910, 0x01B64EC8u, 0x00930368u /* "ConditionHaveIAssaultedTargetPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4140));
}

// 0x8E1930 (initializer)
void __cdecl Fn8E1930() {
    Thiscall(0x005FA910, 0x01B64ECCu, 0x0093038Cu /* "ConditionHaveIHarassedTargetPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4150));
}

// 0x8E1950 (initializer)
void __cdecl Fn8E1950() {
    Thiscall(0x005FA910, 0x01B64ED0u, 0x009303ACu /* "ConditionHaveIDismissedTargetPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4160));
}

// 0x8E1970 (initializer)
void __cdecl Fn8E1970() {
    Thiscall(0x005FA910, 0x01B64ED4u, 0x009303D0u /* "ConditionIsStimulusPriority" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4170));
}

// 0x8E1990 (initializer)
void __cdecl Fn8E1990() {
    Thiscall(0x005FA910, 0x01B64ED8u, 0x009303ECu /* "ConditionIsPowerSliding" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4180));
}

// 0x8E19B0 (initializer)
void __cdecl Fn8E19B0() {
    Thiscall(0x005FA910, 0x01B64EDCu, 0x00930404u /* "ConditionBikeIsLocked" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4190));
}

// 0x8E19D0 (initializer)
void __cdecl Fn8E19D0() {
    Thiscall(0x005FA910, 0x01B64EE0u, 0x0093041Cu /* "ConditionBikeIsUpright" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F41A0));
}

// 0x8E19F0 (initializer)
void __cdecl Fn8E19F0() {
    Thiscall(0x005FA910, 0x01B64EE4u, 0x00930434u /* "ConditionWeaponLastHitBy" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F41B0));
}

// 0x8E1A10 (initializer)
void __cdecl Fn8E1A10() {
    Thiscall(0x005FA910, 0x01B64EE8u, 0x00930450u /* "ConditionWillAcceptGift" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F41C0));
}

// 0x8E1A30 (initializer)
void __cdecl Fn8E1A30() {
    Thiscall(0x005FA910, 0x01B64EECu, 0x00930468u /* "ConditionPlayerHasRequiredGift" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F41D0));
}

// 0x8E1A50 (initializer)
void __cdecl Fn8E1A50() {
    Thiscall(0x005FA910, 0x01B64EF0u, 0x00930488u /* "ConditionIsStrafing" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F41E0));
}

// 0x8E1A70 (initializer)
void __cdecl Fn8E1A70() {
    Thiscall(0x005FA910, 0x01B64EF4u, 0x0093049Cu /* "ConditionTargetAttacker" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F41F0));
}

// 0x8E1A90 (initializer)
void __cdecl Fn8E1A90() {
    Thiscall(0x005FA910, 0x01B64EF8u, 0x009304B4u /* "ConditionTargetByType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4200));
}

// 0x8E1AB0 (initializer)
void __cdecl Fn8E1AB0() {
    Thiscall(0x005FA910, 0x01B64EFCu, 0x009304CCu /* "ConditionTestRequiredGiftType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4210));
}

// 0x8E1AD0 (initializer)
void __cdecl Fn8E1AD0() {
    Thiscall(0x005FA910, 0x01B64F00u, 0x009304ECu /* "ConditionTestFutureGiftType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4220));
}

// 0x8E1AF0 (initializer)
void __cdecl Fn8E1AF0() {
    Thiscall(0x005FA910, 0x01B64F04u, 0x00930508u /* "ConditionHasGiftItemOfType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4230));
}

// 0x8E1B10 (initializer)
void __cdecl Fn8E1B10() {
    Thiscall(0x005FA910, 0x01B64F08u, 0x00930524u /* "ConditionIsGiftRewardAPickup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4240));
}

// 0x8E1B30 (initializer)
void __cdecl Fn8E1B30() {
    Thiscall(0x005FA910, 0x01B64F0Cu, 0x00930544u /* "ConditionHasPlayerMaxItems" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4250));
}

// 0x8E1B50 (initializer)
void __cdecl Fn8E1B50() {
    Thiscall(0x005FA910, 0x01B64F10u, 0x00930560u /* "ConditionPlayerHasItemCount" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4260));
}

// 0x8E1B70 (initializer)
void __cdecl Fn8E1B70() {
    Thiscall(0x005FA910, 0x01B64F14u, 0x0093057Cu /* "ConditionStatInt" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4270));
}

// 0x8E1B90 (initializer)
void __cdecl Fn8E1B90() {
    Thiscall(0x005FA910, 0x01B64F18u, 0x00930590u /* "ConditionStatFloat" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4280));
}

// 0x8E1BB0 (initializer)
void __cdecl Fn8E1BB0() {
    Thiscall(0x005FA910, 0x01B64F1Cu, 0x009305A4u /* "ConditionCheckTargetSemaphore" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4290));
}

// 0x8E1BD0 (initializer)
void __cdecl Fn8E1BD0() {
    Thiscall(0x005FA910, 0x01B64F20u, 0x009305C4u /* "ConditionDetectGrenade" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F42A0));
}

// 0x8E1BF0 (initializer)
void __cdecl Fn8E1BF0() {
    Thiscall(0x005FA910, 0x01B64F24u, 0x009305DCu /* "ConditionDetectProjectile" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F42B0));
}

// 0x8E1C10 (initializer)
void __cdecl Fn8E1C10() {
    Thiscall(0x005FA910, 0x01B64F28u, 0x009305F8u /* "ConditionIsVehicleInRange" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F42C0));
}

// 0x8E1C30 (initializer)
void __cdecl Fn8E1C30() {
    Thiscall(0x005FA910, 0x01B64F2Cu, 0x00930614u /* "ConditionInGuardRadius" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F42D0));
}

// 0x8E1C50 (initializer)
void __cdecl Fn8E1C50() {
    Thiscall(0x005FA910, 0x01B64F30u, 0x0093062Cu /* "ConditionAllyLeaderConditionGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F42E0));
}

// 0x8E1C70 (initializer)
void __cdecl Fn8E1C70() {
    Thiscall(0x005FA910, 0x01B64F34u, 0x00930650u /* "ConditionAllyFollowerConditionGroup" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F42F0));
}

// 0x8E1C90 (initializer)
void __cdecl Fn8E1C90() {
    Thiscall(0x005FA910, 0x01B64F38u, 0x00930674u /* "ConditionCanAcquireAlly" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4300));
}

// 0x8E1CB0 (initializer)
void __cdecl Fn8E1CB0() {
    Thiscall(0x005FA910, 0x01B64F3Cu, 0x0093068Cu /* "ConditionSteering" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4310));
}

// 0x8E1CD0 (initializer)
void __cdecl Fn8E1CD0() {
    Thiscall(0x005FA910, 0x01B64F40u, 0x009306A0u /* "ConditionSteeringFraction" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4320));
}

// 0x8E1CF0 (initializer)
void __cdecl Fn8E1CF0() {
    Thiscall(0x005FA910, 0x01B64F44u, 0x009306BCu /* "ConditionCheckWeatherType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4330));
}

// 0x8E1D10 (initializer)
void __cdecl Fn8E1D10() {
    Thiscall(0x005FA910, 0x01B64F48u, 0x009306D8u /* "ConditionIsParametricStatic" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4340));
}

// 0x8E1D30 (initializer)
void __cdecl Fn8E1D30() {
    Thiscall(0x005FA910, 0x01B64F4Cu, 0x009306F4u /* "ConditionTargetAlly" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4350));
}

// 0x8E1D50 (initializer)
void __cdecl Fn8E1D50() {
    Thiscall(0x005FA910, 0x01B64F50u, 0x00930708u /* "ConditionInWeaponRange" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4360));
}

// 0x8E1D70 (initializer)
void __cdecl Fn8E1D70() {
    Thiscall(0x005FA910, 0x01B64F54u, 0x00930720u /* "ConditionTargetPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4370));
}

// 0x8E1D90 (initializer)
void __cdecl Fn8E1D90() {
    Thiscall(0x005FA910, 0x01B64F58u, 0x00930734u /* "ConditionBalanceMeterPosition" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4380));
}

// 0x8E1DB0 (initializer)
void __cdecl Fn8E1DB0() {
    Thiscall(0x005FA910, 0x01B64F5Cu, 0x00930754u /* "ConditionTargetColliding" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4390));
}

// 0x8E1DD0 (initializer)
void __cdecl Fn8E1DD0() {
    Thiscall(0x005FA910, 0x01B64F60u, 0x00930770u /* "ConditionPedInConversation" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F43A0));
}

// 0x8E1DF0 (initializer)
void __cdecl Fn8E1DF0() {
    Thiscall(0x005FA910, 0x01B64F64u, 0x0093078Cu /* "ConditionPedConversationStatus" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F43B0));
}

// 0x8E1E10 (initializer)
void __cdecl Fn8E1E10() {
    Thiscall(0x005FA910, 0x01B64F68u, 0x009307ACu /* "ConditionPedConversationMood" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F43C0));
}

// 0x8E1E30 (initializer)
void __cdecl Fn8E1E30() {
    Thiscall(0x005FA910, 0x01B64F6Cu, 0x009307CCu /* "ConditionPlayerIsInStealthProp" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F43D0));
}

// 0x8E1E50 (initializer)
void __cdecl Fn8E1E50() {
    Thiscall(0x005FA910, 0x01B64F70u, 0x009307ECu /* "ConditionHasLineOfSightToPlayer" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F43E0));
}

// 0x8E1E70 (initializer)
void __cdecl Fn8E1E70() {
    Thiscall(0x005FA910, 0x01B64F74u, 0x0093080Cu /* "ConditionParametricTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F43F0));
}

// 0x8E1E90 (initializer)
void __cdecl Fn8E1E90() {
    Thiscall(0x005FA910, 0x01B64F78u, 0x00930824u /* "PedGroundDistCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4400));
}

// 0x8E1EB0 (initializer)
void __cdecl Fn8E1EB0() {
    Thiscall(0x005FA910, 0x01B64F7Cu, 0x00930838u /* "PedVelocityCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4410));
}

// 0x8E1ED0 (initializer)
void __cdecl Fn8E1ED0() {
    Thiscall(0x005FA910, 0x01B64F80u, 0x00930848u /* "ConditionCanGetOffVehicleHere" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4420));
}

// 0x8E1EF0 (initializer)
void __cdecl Fn8E1EF0() {
    Thiscall(0x005FA910, 0x01B64F84u, 0x00930868u /* "ConditionCanGetOffVehicleHereLeft" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4430));
}

// 0x8E1F10 (initializer)
void __cdecl Fn8E1F10() {
    Thiscall(0x005FA910, 0x01B64F88u, 0x0093088Cu /* "ConditionCanGetOnVehicleHere" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4440));
}

// 0x8E1F30 (initializer)
void __cdecl Fn8E1F30() {
    Thiscall(0x005FA910, 0x01B64F8Cu, 0x009308ACu /* "ConditionCanGetOnVehicleLeftSide" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4450));
}

// 0x8E1F50 (initializer)
void __cdecl Fn8E1F50() {
    Thiscall(0x005FA910, 0x01B64F90u, 0x009308D0u /* "ConditionCanGetOnVehicleRightSide" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4460));
}

// 0x8E1F70 (initializer)
void __cdecl Fn8E1F70() {
    Thiscall(0x005FA910, 0x01B64F94u, 0x009308F4u /* "ConditionHasRangedWeapon" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4470));
}

// 0x8E1F90 (initializer)
void __cdecl Fn8E1F90() {
    Thiscall(0x005FA910, 0x01B64F98u, 0x00930910u /* "ConditionHasTXDLoaded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4480));
}

// 0x8E1FB0 (initializer)
void __cdecl Fn8E1FB0() {
    Thiscall(0x005FA910, 0x01B64F9Cu, 0x00930928u /* "ConditionIsLastManStanding" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4490));
}

// 0x8E1FD0 (initializer)
void __cdecl Fn8E1FD0() {
    Thiscall(0x005FA910, 0x01B64FA0u, 0x00930944u /* "ConditionPedSphereHit" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F44A0));
}

// 0x8E1FF0 (initializer)
void __cdecl Fn8E1FF0() {
    Thiscall(0x005FA910, 0x01B64FA4u, 0x0093095Cu /* "ConditionIsRumbleMode_Ambient" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F44B0));
}

// 0x8E2010 (initializer)
void __cdecl Fn8E2010() {
    Thiscall(0x005FA910, 0x01B64FA8u, 0x0093097Cu /* "ConditionIsRumbleMode_Mission" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F44C0));
}

// 0x8E2030 (initializer)
void __cdecl Fn8E2030() {
    Thiscall(0x005FA910, 0x01B64FACu, 0x0093099Cu /* "ConditionBeamLateralDeviation" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F44D0));
}

// 0x8E2050 (initializer)
void __cdecl Fn8E2050() {
    Thiscall(0x005FA910, 0x01B64FB0u, 0x009309BCu /* "ConditionBeamLateralPosition" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F44E0));
}

// 0x8E2070 (initializer)
void __cdecl Fn8E2070() {
    Thiscall(0x005FA910, 0x01B64FB4u, 0x009309DCu /* "ConditionVehicleHitOrientation" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F44F0));
}

// 0x8E2090 (initializer)
void __cdecl Fn8E2090() {
    Thiscall(0x005FA910, 0x01B64FB8u, 0x009309FCu /* "ConditionPlayerHasPunishmentPoints" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4500));
}

// 0x8E20B0 (initializer)
void __cdecl Fn8E20B0() {
    Thiscall(0x005FA910, 0x01B64FBCu, 0x00930A20u /* "ConditionTargetPropClearOfPeds" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4510));
}

// 0x8E20D0 (initializer)
void __cdecl Fn8E20D0() {
    Thiscall(0x005FA910, 0x01B64FC0u, 0x00930A40u /* "ConditionPOIClearOfPeds" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4520));
}

// 0x8E20F0 (initializer)
void __cdecl Fn8E20F0() {
    Thiscall(0x005FA910, 0x01B64FC4u, 0x00930A58u /* "ConditionAllyIsInCombat" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4530));
}

// 0x8E2110 (initializer)
void __cdecl Fn8E2110() {
    Thiscall(0x005FA910, 0x01B64FC8u, 0x00930A70u /* "ConditionAllyState" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4540));
}

// 0x8E2130 (initializer)
void __cdecl Fn8E2130() {
    Thiscall(0x005FA910, 0x01B64FCCu, 0x00930A84u /* "ConditionAllyMorale" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4550));
}

// 0x8E2150 (initializer)
void __cdecl Fn8E2150() {
    Thiscall(0x005FA910, 0x01B64FD0u, 0x00930A98u /* "ConditionMissionAvailable" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4560));
}

// 0x8E2170 (initializer)
void __cdecl Fn8E2170() {
    Thiscall(0x005FA910, 0x01B64FD4u, 0x00930AB4u /* "ConditionNumWeapons" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4570));
}

// 0x8E2190 (initializer)
void __cdecl Fn8E2190() {
    Thiscall(0x005FA910, 0x01B64FD8u, 0x00930AC8u /* "ConditionHasGrappleTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4580));
}

// 0x8E21B0 (initializer)
void __cdecl Fn8E21B0() {
    Thiscall(0x005FA910, 0x01B64FDCu, 0x00930AE4u /* "ConditionHasNodeCooldown" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4590));
}

// 0x8E21D0 (initializer)
void __cdecl Fn8E21D0() {
    Thiscall(0x005FA910, 0x01B64FE0u, 0x00930B00u /* "ConditionIsIgnoreTruants" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F45A0));
}

// 0x8E21F0 (initializer)
void __cdecl Fn8E21F0() {
    Thiscall(0x005FA910, 0x01B64FE4u, 0x00930B1Cu /* "ConditionThisNodeIsPlaying" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F45B0));
}

// 0x8E2210 (initializer)
void __cdecl Fn8E2210() {
    Thiscall(0x005FA910, 0x01B64FE8u, 0x00930B38u /* "ConditionThisNodeIsNotPlaying" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F45C0));
}

// 0x8E2230 (initializer)
void __cdecl Fn8E2230() {
    Thiscall(0x005FA910, 0x01B64FECu, 0x00930B58u /* "ConditionPlayerHasTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F45D0));
}

// 0x8E2250 (initializer)
void __cdecl Fn8E2250() {
    Thiscall(0x005FA910, 0x01B64FF0u, 0x00930B74u /* "ConditionNumAuthorityInPursuit" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F45E0));
}

// 0x8E2270 (initializer)
void __cdecl Fn8E2270() {
    Thiscall(0x005FA910, 0x01B64FF4u, 0x00930B94u /* "ConditionIsTagWaiting" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F45F0));
}

// 0x8E2290 (initializer)
void __cdecl Fn8E2290() {
    Thiscall(0x005FA910, 0x01B64FF8u, 0x00930BACu /* "ConditionIsTagDrawing" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4600));
}

// 0x8E22B0 (initializer)
void __cdecl Fn8E22B0() {
    Thiscall(0x005FA910, 0x01B64FFCu, 0x00930BC4u /* "ConditionIsNISorCutsceneRunning" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4610));
}

// 0x8E22D0 (initializer)
void __cdecl Fn8E22D0() {
    Thiscall(0x005FA910, 0x01B65000u, 0x00930BE4u /* "ConditionIsPlayerFacingMirror" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4620));
}

// 0x8E22F0 (initializer)
void __cdecl Fn8E22F0() {
    Thiscall(0x005FA910, 0x01B65004u, 0x00930C04u /* "ConditionIsElderly" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4630));
}

// 0x8E2310 (initializer)
void __cdecl Fn8E2310() {
    Thiscall(0x005FA910, 0x01B65008u, 0x00930C18u /* "ConditionIsRegisteredAsGirlfriend" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4640));
}

// 0x8E2330 (initializer)
void __cdecl Fn8E2330() {
    Thiscall(0x005FA910, 0x01B6500Cu, 0x00930C3Cu /* "ConditionIsInFight" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4650));
}

// 0x8E2350 (initializer)
void __cdecl Fn8E2350() {
    Thiscall(0x005FA910, 0x01B65010u, 0x00930C50u /* "ConditionScriptSaveData" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4660));
}

// 0x8E2370 (initializer)
void __cdecl Fn8E2370() {
    Thiscall(0x005FA910, 0x01B65014u, 0x00930C68u /* "ConditionPlayerHasPunishmentOfType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4670));
}

// 0x8E2390 (initializer)
void __cdecl Fn8E2390() {
    Thiscall(0x005FA910, 0x01B65018u, 0x00930C8Cu /* "ConditionIsMiniObjectiveComplete" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4680));
}

// 0x8E23B0 (initializer)
void __cdecl Fn8E23B0() {
    Thiscall(0x005FA910, 0x01B6501Cu, 0x00930CB0u /* "ConditionIsAvoiding" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4690));
}

// 0x8E23D0 (initializer)
void __cdecl Fn8E23D0() {
    Thiscall(0x005FA910, 0x01B65020u, 0x00930CC4u /* "ConditionFrontEndActive" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F46A0));
}

// 0x8E23F0 (initializer)
void __cdecl Fn8E23F0() {
    Thiscall(0x005FA910, 0x01B65024u, 0x00930CDCu /* "ConditionPedHasNumAttackers" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F46B0));
}

// 0x8E2410 (initializer)
void __cdecl Fn8E2410() {
    Thiscall(0x005FA910, 0x01B65028u, 0x00930CF8u /* "ConditionIsUsingDoor" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F46C0));
}

// 0x8E2430 (initializer)
void __cdecl Fn8E2430() {
    Float(0x01B65040) = float((1.0 / double(Float(0x00B0B198))));
}

// 0x8E2450 (initializer)
void __cdecl Fn8E2450() {
    Float(0x01B65044) = float((1.0 / (double(Float(0x00B0B198)) * double(Float(0x00B0B198)))));
}

// 0x8E2470 (initializer)
void __cdecl Fn8E2470() {
    Float(0x01B65048) = float((double(Float(0x01B65044)) * Double(0x00900140)));
}

// 0x8E2490 (initializer)
void __cdecl Fn8E2490() {
    Float(0x01B6504C) = float((Double(0x00900AD8) / double(Float(0x00B0B198))));
}

// 0x8E24B0 (initializer)
void __cdecl Fn8E24B0() {
    Thiscall(0x005FA910, 0x01B65050u, 0x00931720u /* "AnimationGroupReleaseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F46D0));
}

// 0x8E24D0 (initializer)
void __cdecl Fn8E24D0() {
    Thiscall(0x005FA910, 0x01B65054u, 0x0093173Cu /* "AnimationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F46E0));
}

// 0x8E24F0 (initializer)
void __cdecl Fn8E24F0() {
    Thiscall(0x005FA910, 0x01B65058u, 0x0093174Cu /* "DialogAnimationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F46F0));
}

// 0x8E2510 (initializer)
void __cdecl Fn8E2510() {
    Thiscall(0x005FA910, 0x01B6505Cu, 0x00931764u /* "HitAnimationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4700));
}

// 0x8E2530 (initializer)
void __cdecl Fn8E2530() {
    Thiscall(0x005FA910, 0x01B65060u, 0x00931778u /* "HitReactTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4710));
}

// 0x8E2550 (initializer)
void __cdecl Fn8E2550() {
    Thiscall(0x005FA910, 0x01B65064u, 0x00931788u /* "JointDriverTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4720));
}

// 0x8E2570 (initializer)
void __cdecl Fn8E2570() {
    Thiscall(0x005FA910, 0x01B65068u, 0x0093179Cu /* "DialogJointDriverTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4730));
}

// 0x8E2590 (initializer)
void __cdecl Fn8E2590() {
    Thiscall(0x005FA910, 0x01B6506Cu, 0x009317B4u /* "JointDriverParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4740));
}

// 0x8E25B0 (initializer)
void __cdecl Fn8E25B0() {
    Thiscall(0x005FA910, 0x01B65070u, 0x009317D0u /* "AnimationDriverSteerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4750));
}

// 0x8E25D0 (initializer)
void __cdecl Fn8E25D0() {
    Thiscall(0x005FA910, 0x01B65074u, 0x009317ECu /* "AnimationDriverStruggleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4760));
}

// 0x8E25F0 (initializer)
void __cdecl Fn8E25F0() {
    Thiscall(0x005FA910, 0x01B65078u, 0x0093180Cu /* "AnimationDriverTargetSyncTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4770));
}

// 0x8E2610 (initializer)
void __cdecl Fn8E2610() {
    Thiscall(0x005FA910, 0x01B6507Cu, 0x0093182Cu /* "ParametricAnimationControllerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4780));
}

// 0x8E2630 (initializer)
void __cdecl Fn8E2630() {
    Thiscall(0x005FA910, 0x01B65080u, 0x00931850u /* "ParametricTrackerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4790));
}

// 0x8E2650 (initializer)
void __cdecl Fn8E2650() {
    Thiscall(0x005FA910, 0x01B65084u, 0x00931868u /* "AIAimParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F47A0));
}

// 0x8E2670 (initializer)
void __cdecl Fn8E2670() {
    Thiscall(0x005FA910, 0x01B65088u, 0x00931880u /* "SpudCannonFireControllerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F47B0));
}

// 0x8E2690 (initializer)
void __cdecl Fn8E2690() {
    Thiscall(0x005FA910, 0x01B6508Cu, 0x009318A0u /* "AnimationDoorControlTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F47C0));
}

// 0x8E26B0 (initializer)
void __cdecl Fn8E26B0() {
    Thiscall(0x005FA910, 0x01B65090u, 0x009318BCu /* "AnimationParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F47D0));
}

// 0x8E26D0 (initializer)
void __cdecl Fn8E26D0() {
    Thiscall(0x005FA910, 0x01B65094u, 0x009318D8u /* "VehicleAnimationParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F47E0));
}

// 0x8E26F0 (initializer)
void __cdecl Fn8E26F0() {
    Thiscall(0x005FA910, 0x01B65098u, 0x009318F8u /* "AnimationReverseParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F47F0));
}

// 0x8E2710 (initializer)
void __cdecl Fn8E2710() {
    Thiscall(0x005FA910, 0x01B6509Cu, 0x00931918u /* "AnimationDriverRotationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4800));
}

// 0x8E2730 (initializer)
void __cdecl Fn8E2730() {
    Thiscall(0x005FA910, 0x01B650A0u, 0x00931938u /* "LocomotionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4810));
}

// 0x8E2750 (initializer)
void __cdecl Fn8E2750() {
    Thiscall(0x005FA910, 0x01B650A4u, 0x00931948u /* "LocomotionJumpTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4820));
}

// 0x8E2770 (initializer)
void __cdecl Fn8E2770() {
    Thiscall(0x005FA910, 0x01B650A8u, 0x0093195Cu /* "LocomotionSlideTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4830));
}

// 0x8E2790 (initializer)
void __cdecl Fn8E2790() {
    Thiscall(0x005FA910, 0x01B650ACu, 0x00931974u /* "LocomotionCycleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4840));
}

// 0x8E27B0 (initializer)
void __cdecl Fn8E27B0() {
    Thiscall(0x005FA910, 0x01B650B0u, 0x0093198Cu /* "LocomotionLiquidTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4850));
}

// 0x8E27D0 (initializer)
void __cdecl Fn8E27D0() {
    Thiscall(0x005FA910, 0x01B650B4u, 0x009319A4u /* "LocomotionRetardTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4860));
}

// 0x8E27F0 (initializer)
void __cdecl Fn8E27F0() {
    Thiscall(0x005FA910, 0x01B650B8u, 0x009319BCu /* "LocomotionAnimationNewTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4870));
}

// 0x8E2810 (initializer)
void __cdecl Fn8E2810() {
    Thiscall(0x005FA910, 0x01B650BCu, 0x009319D8u /* "AnimationLocoSyncTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4880));
}

// 0x8E2830 (initializer)
void __cdecl Fn8E2830() {
    Thiscall(0x005FA910, 0x01B650C0u, 0x009319F0u /* "LocomotionEventTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4890));
}

// 0x8E2850 (initializer)
void __cdecl Fn8E2850() {
    Thiscall(0x005FA910, 0x01B650C4u, 0x00931A08u /* "GravityTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F48A0));
}

// 0x8E2870 (initializer)
void __cdecl Fn8E2870() {
    Thiscall(0x005FA910, 0x01B650C8u, 0x00931A18u /* "AllowSwitchGoofyFootTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F48B0));
}

// 0x8E2890 (initializer)
void __cdecl Fn8E2890() {
    Thiscall(0x005FA910, 0x01B650CCu, 0x00931A34u /* "OrientOffSetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F48C0));
}

// 0x8E28B0 (initializer)
void __cdecl Fn8E28B0() {
    Thiscall(0x005FA910, 0x01B650D0u, 0x00931A48u /* "TargetFaceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F48D0));
}

// 0x8E28D0 (initializer)
void __cdecl Fn8E28D0() {
    Thiscall(0x005FA910, 0x01B650D4u, 0x00931A58u /* "TargetOverrideResetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F48E0));
}

// 0x8E28F0 (initializer)
void __cdecl Fn8E28F0() {
    Thiscall(0x005FA910, 0x01B650D8u, 0x00931A74u /* "TargetSyncTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F48F0));
}

// 0x8E2910 (initializer)
void __cdecl Fn8E2910() {
    Thiscall(0x005FA910, 0x01B650DCu, 0x00931A84u /* "SyncTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4900));
}

// 0x8E2930 (initializer)
void __cdecl Fn8E2930() {
    Thiscall(0x005FA910, 0x01B650E0u, 0x00931A90u /* "AttachIKTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4910));
}

// 0x8E2950 (initializer)
void __cdecl Fn8E2950() {
    Thiscall(0x005FA910, 0x01B650E4u, 0x00931AA0u /* "DetachIKTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4920));
}

// 0x8E2970 (initializer)
void __cdecl Fn8E2970() {
    Thiscall(0x005FA910, 0x01B650E8u, 0x00931AB0u /* "PedResetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4930));
}

// 0x8E2990 (initializer)
void __cdecl Fn8E2990() {
    Thiscall(0x005FA910, 0x01B650ECu, 0x00931AC0u /* "SetWorldAnchorTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4940));
}

// 0x8E29B0 (initializer)
void __cdecl Fn8E29B0() {
    Thiscall(0x005FA910, 0x01B650F0u, 0x00931AD4u /* "AttemptAnchorToVehicleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4950));
}

// 0x8E29D0 (initializer)
void __cdecl Fn8E29D0() {
    Thiscall(0x005FA910, 0x01B650F4u, 0x00931AF0u /* "TargetGrappleReleaseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4960));
}

// 0x8E29F0 (initializer)
void __cdecl Fn8E29F0() {
    Thiscall(0x005FA910, 0x01B650F8u, 0x00931B0Cu /* "EntityFlagsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4970));
}

// 0x8E2A10 (initializer)
void __cdecl Fn8E2A10() {
    Thiscall(0x005FA910, 0x01B650FCu, 0x00931B20u /* "SetVehicleEntityFlagsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4980));
}

// 0x8E2A30 (initializer)
void __cdecl Fn8E2A30() {
    Thiscall(0x005FA910, 0x01B65100u, 0x00931B3Cu /* "StruggleButtonOnTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4990));
}

// 0x8E2A50 (initializer)
void __cdecl Fn8E2A50() {
    Thiscall(0x005FA910, 0x01B65104u, 0x00931B54u /* "StruggleButtonOffTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F49A0));
}

// 0x8E2A70 (initializer)
void __cdecl Fn8E2A70() {
    Thiscall(0x005FA910, 0x01B65108u, 0x00931B6Cu /* "SetPunishmentPointsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F49B0));
}

// 0x8E2A90 (initializer)
void __cdecl Fn8E2A90() {
    Thiscall(0x005FA910, 0x01B6510Cu, 0x00931B88u /* "HudScreenFadeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F49C0));
}

// 0x8E2AB0 (initializer)
void __cdecl Fn8E2AB0() {
    Thiscall(0x005FA910, 0x01B65110u, 0x00931B9Cu /* "HUDComponentVisiblityTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F49D0));
}

// 0x8E2AD0 (initializer)
void __cdecl Fn8E2AD0() {
    Thiscall(0x005FA910, 0x01B65114u, 0x00931BB8u /* "HUDComponentFlashingTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F49E0));
}

// 0x8E2AF0 (initializer)
void __cdecl Fn8E2AF0() {
    Thiscall(0x005FA910, 0x01B65118u, 0x00931BD4u /* "HUDFlashBlipTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F49F0));
}

// 0x8E2B10 (initializer)
void __cdecl Fn8E2B10() {
    Thiscall(0x005FA910, 0x01B6511Cu, 0x00931BE8u /* "HudFlashMoneyTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A00));
}

// 0x8E2B30 (initializer)
void __cdecl Fn8E2B30() {
    Thiscall(0x005FA910, 0x01B65120u, 0x00931BFCu /* "HUDSetTagShapeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A10));
}

// 0x8E2B50 (initializer)
void __cdecl Fn8E2B50() {
    Thiscall(0x005FA910, 0x01B65124u, 0x00931C10u /* "WeaponWheelOffTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A20));
}

// 0x8E2B70 (initializer)
void __cdecl Fn8E2B70() {
    Thiscall(0x005FA910, 0x01B65128u, 0x00931C24u /* "VibrationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A30));
}

// 0x8E2B90 (initializer)
void __cdecl Fn8E2B90() {
    Thiscall(0x005FA910, 0x01B6512Cu, 0x00931C34u /* "AddRadarBlipTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A40));
}

// 0x8E2BB0 (initializer)
void __cdecl Fn8E2BB0() {
    Thiscall(0x005FA910, 0x01B65130u, 0x00931C48u /* "AttributesForCashTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A50));
}

// 0x8E2BD0 (initializer)
void __cdecl Fn8E2BD0() {
    Thiscall(0x005FA910, 0x01B65134u, 0x00931C60u /* "AddMoneyTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A60));
}

// 0x8E2BF0 (initializer)
void __cdecl Fn8E2BF0() {
    Thiscall(0x005FA910, 0x01B65138u, 0x00931C70u /* "HoldStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A70));
}

// 0x8E2C10 (initializer)
void __cdecl Fn8E2C10() {
    Thiscall(0x005FA910, 0x01B6513Cu, 0x00931C80u /* "PedFlagsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A80));
}

// 0x8E2C30 (initializer)
void __cdecl Fn8E2C30() {
    Thiscall(0x005FA910, 0x01B65140u, 0x00931C90u /* "AddForceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4A90));
}

// 0x8E2C50 (initializer)
void __cdecl Fn8E2C50() {
    Thiscall(0x005FA910, 0x01B65144u, 0x00931CA0u /* "AddTurnForceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4AA0));
}

// 0x8E2C70 (initializer)
void __cdecl Fn8E2C70() {
    Thiscall(0x005FA910, 0x01B65148u, 0x00931CB4u /* "DamageTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4AB0));
}

// 0x8E2C90 (initializer)
void __cdecl Fn8E2C90() {
    Thiscall(0x005FA910, 0x01B6514Cu, 0x00931CC0u /* "TargetDamageTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4AC0));
}

// 0x8E2CB0 (initializer)
void __cdecl Fn8E2CB0() {
    Thiscall(0x005FA910, 0x01B65150u, 0x00931CD4u /* "HandleDamageTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4AD0));
}

// 0x8E2CD0 (initializer)
void __cdecl Fn8E2CD0() {
    Thiscall(0x005FA910, 0x01B65154u, 0x00931CE8u /* "DeathTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4AE0));
}

// 0x8E2CF0 (initializer)
void __cdecl Fn8E2CF0() {
    Thiscall(0x005FA910, 0x01B65158u, 0x00931CF4u /* "SpecialIncrementChargeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4AF0));
}

// 0x8E2D10 (initializer)
void __cdecl Fn8E2D10() {
    Thiscall(0x005FA910, 0x01B6515Cu, 0x00931D10u /* "SpecialIncreaseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B00));
}

// 0x8E2D30 (initializer)
void __cdecl Fn8E2D30() {
    Thiscall(0x005FA910, 0x01B65160u, 0x00931D28u /* "SprintIncrementChargeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B10));
}

// 0x8E2D50 (initializer)
void __cdecl Fn8E2D50() {
    Thiscall(0x005FA910, 0x01B65164u, 0x00931D44u /* "SprintIncreaseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B20));
}

// 0x8E2D70 (initializer)
void __cdecl Fn8E2D70() {
    Thiscall(0x005FA910, 0x01B65168u, 0x00931D58u /* "SpecialPointsAddTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B30));
}

// 0x8E2D90 (initializer)
void __cdecl Fn8E2D90() {
    Thiscall(0x005FA910, 0x01B6516Cu, 0x00931D70u /* "ComboCounterAddTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B40));
}

// 0x8E2DB0 (initializer)
void __cdecl Fn8E2DB0() {
    Thiscall(0x005FA910, 0x01B65170u, 0x00931D88u /* "ComboCounterSetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B50));
}

// 0x8E2DD0 (initializer)
void __cdecl Fn8E2DD0() {
    Thiscall(0x005FA910, 0x01B65174u, 0x00931DA0u /* "TransferComboToSpecialTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B60));
}

// 0x8E2DF0 (initializer)
void __cdecl Fn8E2DF0() {
    Thiscall(0x005FA910, 0x01B65178u, 0x00931DBCu /* "DetachEffectTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B70));
}

// 0x8E2E10 (initializer)
void __cdecl Fn8E2E10() {
    Thiscall(0x005FA910, 0x01B6517Cu, 0x00931DD0u /* "AddAttachedEffectTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B80));
}

// 0x8E2E30 (initializer)
void __cdecl Fn8E2E30() {
    Thiscall(0x005FA910, 0x01B65180u, 0x00931DE8u /* "DialogAddAttachedEffectTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4B90));
}

// 0x8E2E50 (initializer)
void __cdecl Fn8E2E50() {
    Thiscall(0x005FA910, 0x01B65184u, 0x00931E08u /* "BreathTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4BA0));
}

// 0x8E2E70 (initializer)
void __cdecl Fn8E2E70() {
    Thiscall(0x005FA910, 0x01B65188u, 0x00931E14u /* "FXCeilingTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4BB0));
}

// 0x8E2E90 (initializer)
void __cdecl Fn8E2E90() {
    Thiscall(0x005FA910, 0x01B6518Cu, 0x00931E24u /* "FXDecalTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4BC0));
}

// 0x8E2EB0 (initializer)
void __cdecl Fn8E2EB0() {
    Thiscall(0x005FA910, 0x01B65190u, 0x00931E34u /* "DialogFXDecalTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4BD0));
}

// 0x8E2ED0 (initializer)
void __cdecl Fn8E2ED0() {
    Thiscall(0x005FA910, 0x01B65194u, 0x00931E48u /* "FXWaterRipplesTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4BE0));
}

// 0x8E2EF0 (initializer)
void __cdecl Fn8E2EF0() {
    Thiscall(0x005FA910, 0x01B65198u, 0x00931E5Cu /* "GroundImpactTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4BF0));
}

// 0x8E2F10 (initializer)
void __cdecl Fn8E2F10() {
    Thiscall(0x005FA910, 0x01B6519Cu, 0x00931E70u /* "WallImpactTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C00));
}

// 0x8E2F30 (initializer)
void __cdecl Fn8E2F30() {
    Thiscall(0x005FA910, 0x01B651A0u, 0x00931E80u /* "FXWeaponImpactTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C10));
}

// 0x8E2F50 (initializer)
void __cdecl Fn8E2F50() {
    Thiscall(0x005FA910, 0x01B651A4u, 0x00931E94u /* "ScreenFxDizzyTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C20));
}

// 0x8E2F70 (initializer)
void __cdecl Fn8E2F70() {
    Thiscall(0x005FA910, 0x01B651A8u, 0x00931EA8u /* "PurchaseItemTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C30));
}

// 0x8E2F90 (initializer)
void __cdecl Fn8E2F90() {
    Thiscall(0x005FA910, 0x01B651ACu, 0x00931EBCu /* "CameraShotTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C40));
}

// 0x8E2FB0 (initializer)
void __cdecl Fn8E2FB0() {
    Thiscall(0x005FA910, 0x01B651B0u, 0x00931ECCu /* "CameraShakeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C50));
}

// 0x8E2FD0 (initializer)
void __cdecl Fn8E2FD0() {
    Thiscall(0x005FA910, 0x01B651B4u, 0x00931EE0u /* "CameraFreezeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C60));
}

// 0x8E2FF0 (initializer)
void __cdecl Fn8E2FF0() {
    Thiscall(0x005FA910, 0x01B651B8u, 0x00931EF4u /* "RandomDiceRollTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C70));
}

// 0x8E3010 (initializer)
void __cdecl Fn8E3010() {
    Thiscall(0x005FA910, 0x01B651BCu, 0x00931F08u /* "ActivateDoorTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C80));
}

// 0x8E3030 (initializer)
void __cdecl Fn8E3030() {
    Thiscall(0x005FA910, 0x01B651C0u, 0x00931F1Cu /* "GameTimerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4C90));
}

// 0x8E3050 (initializer)
void __cdecl Fn8E3050() {
    Thiscall(0x005FA910, 0x01B651C4u, 0x00931F2Cu /* "RailDriverTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4CA0));
}

// 0x8E3070 (initializer)
void __cdecl Fn8E3070() {
    Thiscall(0x005FA910, 0x01B651C8u, 0x00931F3Cu /* "SetPosRotTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4CB0));
}

// 0x8E3090 (initializer)
void __cdecl Fn8E3090() {
    Thiscall(0x005FA910, 0x01B651CCu, 0x00931F4Cu /* "ShopSetStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4CC0));
}

// 0x8E30B0 (initializer)
void __cdecl Fn8E30B0() {
    Thiscall(0x005FA910, 0x01B651D0u, 0x00931F60u /* "SetInStealthPropTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4CD0));
}

// 0x8E30D0 (initializer)
void __cdecl Fn8E30D0() {
    Thiscall(0x005FA910, 0x01B651D4u, 0x00931F78u /* "NotifyOnSightPlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4CE0));
}

// 0x8E30F0 (initializer)
void __cdecl Fn8E30F0() {
    Thiscall(0x005FA910, 0x01B651D8u, 0x00931F94u /* "NotifyOnHearTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4CF0));
}

// 0x8E3110 (initializer)
void __cdecl Fn8E3110() {
    Thiscall(0x005FA910, 0x01B651DCu, 0x00931FA8u /* "NotifyOnMissionCriticalTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D00));
}

// 0x8E3130 (initializer)
void __cdecl Fn8E3130() {
    Thiscall(0x005FA910, 0x01B651E0u, 0x00931FC8u /* "EndMissionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D10));
}

// 0x8E3150 (initializer)
void __cdecl Fn8E3150() {
    Thiscall(0x005FA910, 0x01B651E4u, 0x00931FD8u /* "CreateGiftRewardPickupTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D20));
}

// 0x8E3170 (initializer)
void __cdecl Fn8E3170() {
    Thiscall(0x005FA910, 0x01B651E8u, 0x00931FF4u /* "AcceptGiftTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D30));
}

// 0x8E3190 (initializer)
void __cdecl Fn8E3190() {
    Thiscall(0x005FA910, 0x01B651ECu, 0x00932004u /* "RemoveItemTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D40));
}

// 0x8E31B0 (initializer)
void __cdecl Fn8E31B0() {
    Thiscall(0x005FA910, 0x01B651F0u, 0x00932014u /* "SetCanAcceptGiftTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D50));
}

// 0x8E31D0 (initializer)
void __cdecl Fn8E31D0() {
    Thiscall(0x005FA910, 0x01B651F4u, 0x0093202Cu /* "SetOverheadIconTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D60));
}

// 0x8E31F0 (initializer)
void __cdecl Fn8E31F0() {
    Thiscall(0x005FA910, 0x01B651F8u, 0x00932044u /* "StatSetAsIntTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D70));
}

// 0x8E3210 (initializer)
void __cdecl Fn8E3210() {
    Thiscall(0x005FA910, 0x01B651FCu, 0x00932058u /* "StatSetAsFloatTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D80));
}

// 0x8E3230 (initializer)
void __cdecl Fn8E3230() {
    Thiscall(0x005FA910, 0x01B65200u, 0x0093206Cu /* "StatAddToIntTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4D90));
}

// 0x8E3250 (initializer)
void __cdecl Fn8E3250() {
    Thiscall(0x005FA910, 0x01B65204u, 0x00932080u /* "StatAddToFloatTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4DA0));
}

// 0x8E3270 (initializer)
void __cdecl Fn8E3270() {
    Thiscall(0x005FA910, 0x01B65208u, 0x00932094u /* "BalanceMeterTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4DB0));
}

// 0x8E3290 (initializer)
void __cdecl Fn8E3290() {
    Thiscall(0x005FA910, 0x01B6520Cu, 0x009320A8u /* "CameraGetParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4DC0));
}

// 0x8E32B0 (initializer)
void __cdecl Fn8E32B0() {
    Thiscall(0x005FA910, 0x01B65210u, 0x009320C4u /* "HeadtrackGetParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4DD0));
}

// 0x8E32D0 (initializer)
void __cdecl Fn8E32D0() {
    Thiscall(0x005FA910, 0x01B65214u, 0x009320E0u /* "OffsetCameraMirror180Track" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4DE0));
}

// 0x8E32F0 (initializer)
void __cdecl Fn8E32F0() {
    Thiscall(0x005FA910, 0x01B65218u, 0x009320FCu /* "GiveItemToPlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4DF0));
}

// 0x8E3310 (initializer)
void __cdecl Fn8E3310() {
    Thiscall(0x005FA910, 0x01B6521Cu, 0x00932114u /* "BeamMoveToSafePosTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E00));
}

// 0x8E3330 (initializer)
void __cdecl Fn8E3330() {
    Thiscall(0x005FA910, 0x01B65220u, 0x0093212Cu /* "BeamOrientToSafeHeadingTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E10));
}

// 0x8E3350 (initializer)
void __cdecl Fn8E3350() {
    Thiscall(0x005FA910, 0x01B65224u, 0x0093214Cu /* "SetMotionIntentionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E20));
}

// 0x8E3370 (initializer)
void __cdecl Fn8E3370() {
    Thiscall(0x005FA910, 0x01B65228u, 0x00932164u /* "TargetClearTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E30));
}

// 0x8E3390 (initializer)
void __cdecl Fn8E3390() {
    Thiscall(0x005FA910, 0x01B6522Cu, 0x00932178u /* "SaveGameTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E40));
}

// 0x8E33B0 (initializer)
void __cdecl Fn8E33B0() {
    Thiscall(0x005FA910, 0x01B65230u, 0x00932188u /* "SpineControlTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E50));
}

// 0x8E33D0 (initializer)
void __cdecl Fn8E33D0() {
    Thiscall(0x005FA910, 0x01B65234u, 0x0093219Cu /* "SpineControlDisableTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E60));
}

// 0x8E33F0 (initializer)
void __cdecl Fn8E33F0() {
    Thiscall(0x005FA910, 0x01B65238u, 0x009321B8u /* "SpineControlSetTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E70));
}

// 0x8E3410 (initializer)
void __cdecl Fn8E3410() {
    Thiscall(0x005FA910, 0x01B6523Cu, 0x009321D4u /* "CompletionDisplayTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E80));
}

// 0x8E3430 (initializer)
void __cdecl Fn8E3430() {
    Thiscall(0x005FA910, 0x01B65240u, 0x009321ECu /* "usePelvisForHealthMeterTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4E90));
}

// 0x8E3450 (initializer)
void __cdecl Fn8E3450() {
    Thiscall(0x005FA910, 0x01B65244u, 0x0093220Cu /* "FallingDamageTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4EA0));
}

// 0x8E3470 (initializer)
void __cdecl Fn8E3470() {
    Thiscall(0x005FA910, 0x01B65248u, 0x00932220u /* "FallingDamageHandleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4EB0));
}

// 0x8E3490 (initializer)
void __cdecl Fn8E3490() {
    Thiscall(0x005FA910, 0x01B6524Cu, 0x0093223Cu /* "AttackTimerResetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4EC0));
}

// 0x8E34B0 (initializer)
void __cdecl Fn8E34B0() {
    Thiscall(0x0066BEA0, 0x01B65260u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4ED0));
}

// 0x8E34D0 (initializer)
void __cdecl Fn8E34D0() {
    Thiscall(0x0066BEA0, 0x01B65264u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4EE0));
}

// 0x8E34F0 (initializer)
void __cdecl Fn8E34F0() {
    Thiscall(0x0066BEA0, 0x01B65268u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4EF0));
}

// 0x8E3510 (initializer)
void __cdecl Fn8E3510() {
    Thiscall(0x0066BEA0, 0x01B6526Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F00));
}

// 0x8E3530 (initializer)
void __cdecl Fn8E3530() {
    Thiscall(0x0066BEA0, 0x01B65270u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F10));
}

// 0x8E3550 (initializer)
void __cdecl Fn8E3550() {
    Thiscall(0x0066BEA0, 0x01B65274u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F20));
}

// 0x8E3570 (initializer)
void __cdecl Fn8E3570() {
    Thiscall(0x0066BEA0, 0x01B65278u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F30));
}

// 0x8E3590 (initializer)
void __cdecl Fn8E3590() {
    Thiscall(0x0066BEA0, 0x01B6527Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F40));
}

// 0x8E35B0 (initializer)
void __cdecl Fn8E35B0() {
    Thiscall(0x0066BEA0, 0x01B65280u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F50));
}

// 0x8E35D0 (initializer)
void __cdecl Fn8E35D0() {
    Thiscall(0x0066BEA0, 0x01B65284u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F60));
}

// 0x8E35F0 (initializer)
void __cdecl Fn8E35F0() {
    Thiscall(0x0066BEA0, 0x01B65288u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F70));
}

// 0x8E3610 (initializer)
void __cdecl Fn8E3610() {
    Thiscall(0x0066BEA0, 0x01B6528Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F80));
}

// 0x8E3630 (initializer)
void __cdecl Fn8E3630() {
    Thiscall(0x0066BEA0, 0x01B65290u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4F90));
}

// 0x8E3650 (initializer)
void __cdecl Fn8E3650() {
    Thiscall(0x0066BEA0, 0x01B65294u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4FA0));
}

// 0x8E3670 (initializer)
void __cdecl Fn8E3670() {
    Thiscall(0x0066BEA0, 0x01B65298u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4FB0));
}

// 0x8E3690 (initializer)
void __cdecl Fn8E3690() {
    Thiscall(0x0066BEA0, 0x01B6529Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4FC0));
}

// 0x8E36B0 (initializer)
void __cdecl Fn8E36B0() {
    Thiscall(0x0066BEA0, 0x01B652A0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4FD0));
}

// 0x8E36D0 (initializer)
void __cdecl Fn8E36D0() {
    Thiscall(0x0066BEA0, 0x01B652A4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4FE0));
}

// 0x8E36F0 (initializer)
void __cdecl Fn8E36F0() {
    Thiscall(0x0066BEA0, 0x01B652A8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F4FF0));
}

// 0x8E3710 (initializer)
void __cdecl Fn8E3710() {
    Thiscall(0x0066BEA0, 0x01B652ACu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5000));
}

// 0x8E3730 (initializer)
void __cdecl Fn8E3730() {
    Thiscall(0x0066BEA0, 0x01B652B0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5010));
}

// 0x8E3750 (initializer)
void __cdecl Fn8E3750() {
    Thiscall(0x0066BEA0, 0x01B652B4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5020));
}

// 0x8E3770 (initializer)
void __cdecl Fn8E3770() {
    Thiscall(0x0066BEA0, 0x01B652B8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5030));
}

// 0x8E3790 (initializer)
void __cdecl Fn8E3790() {
    Thiscall(0x0066BEA0, 0x01B652BCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5040));
}

// 0x8E37B0 (initializer)
void __cdecl Fn8E37B0() {
    Thiscall(0x0066BEA0, 0x01B652C0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5050));
}

// 0x8E37D0 (initializer)
void __cdecl Fn8E37D0() {
    Thiscall(0x0066BEA0, 0x01B652C4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5060));
}

// 0x8E37F0 (initializer)
void __cdecl Fn8E37F0() {
    Thiscall(0x0066BEA0, 0x01B652C8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5070));
}

// 0x8E3810 (initializer)
void __cdecl Fn8E3810() {
    Thiscall(0x0066BEA0, 0x01B652CCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5080));
}

// 0x8E3830 (initializer)
void __cdecl Fn8E3830() {
    Thiscall(0x0066BEA0, 0x01B652D0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5090));
}

// 0x8E3850 (initializer)
void __cdecl Fn8E3850() {
    Thiscall(0x0066BEA0, 0x01B652D4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F50A0));
}

// 0x8E3870 (initializer)
void __cdecl Fn8E3870() {
    Thiscall(0x0066BEA0, 0x01B652D8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F50B0));
}

// 0x8E3890 (initializer)
void __cdecl Fn8E3890() {
    Thiscall(0x0066BEA0, 0x01B652DCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F50C0));
}

// 0x8E38B0 (initializer)
void __cdecl Fn8E38B0() {
    Thiscall(0x0066BEA0, 0x01B652E0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F50D0));
}

// 0x8E38D0 (initializer)
void __cdecl Fn8E38D0() {
    Thiscall(0x0066BEA0, 0x01B652E4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F50E0));
}

// 0x8E38F0 (initializer)
void __cdecl Fn8E38F0() {
    Thiscall(0x0066BEA0, 0x01B652E8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F50F0));
}

// 0x8E3910 (initializer)
void __cdecl Fn8E3910() {
    Thiscall(0x0066BEA0, 0x01B652ECu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5100));
}

// 0x8E3930 (initializer)
void __cdecl Fn8E3930() {
    Thiscall(0x0066BEA0, 0x01B652F0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5110));
}

// 0x8E3950 (initializer)
void __cdecl Fn8E3950() {
    Thiscall(0x0066BEA0, 0x01B652F4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5120));
}

// 0x8E3970 (initializer)
void __cdecl Fn8E3970() {
    Thiscall(0x0066BEA0, 0x01B652F8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5130));
}

// 0x8E3990 (initializer)
void __cdecl Fn8E3990() {
    Thiscall(0x0066BEA0, 0x01B652FCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5140));
}

// 0x8E39B0 (initializer)
void __cdecl Fn8E39B0() {
    Thiscall(0x0066BEA0, 0x01B65300u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5150));
}

// 0x8E39D0 (initializer)
void __cdecl Fn8E39D0() {
    Thiscall(0x0066BEA0, 0x01B65304u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5160));
}

// 0x8E39F0 (initializer)
void __cdecl Fn8E39F0() {
    Thiscall(0x0066BEA0, 0x01B65308u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5170));
}

// 0x8E3A10 (initializer)
void __cdecl Fn8E3A10() {
    Thiscall(0x0066BEA0, 0x01B6530Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5180));
}

// 0x8E3A30 (initializer)
void __cdecl Fn8E3A30() {
    Thiscall(0x0066BEA0, 0x01B65310u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5190));
}

// 0x8E3A50 (initializer)
void __cdecl Fn8E3A50() {
    Thiscall(0x0066BEA0, 0x01B65314u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F51A0));
}

// 0x8E3A70 (initializer)
void __cdecl Fn8E3A70() {
    Thiscall(0x0066BEA0, 0x01B65318u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F51B0));
}

// 0x8E3A90 (initializer)
void __cdecl Fn8E3A90() {
    Thiscall(0x0066BEA0, 0x01B6531Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F51C0));
}

// 0x8E3AB0 (initializer)
void __cdecl Fn8E3AB0() {
    Thiscall(0x0066BEA0, 0x01B65320u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F51D0));
}

// 0x8E3AD0 (initializer)
void __cdecl Fn8E3AD0() {
    Thiscall(0x0066BEA0, 0x01B65324u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F51E0));
}

// 0x8E3AF0 (initializer)
void __cdecl Fn8E3AF0() {
    Thiscall(0x0066BEA0, 0x01B65328u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F51F0));
}

// 0x8E3B10 (initializer)
void __cdecl Fn8E3B10() {
    Thiscall(0x0066BEA0, 0x01B6532Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5200));
}

// 0x8E3B30 (initializer)
void __cdecl Fn8E3B30() {
    Thiscall(0x0066BEA0, 0x01B65330u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5210));
}

// 0x8E3B50 (initializer)
void __cdecl Fn8E3B50() {
    Thiscall(0x0066BEA0, 0x01B65334u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5220));
}

// 0x8E3B70 (initializer)
void __cdecl Fn8E3B70() {
    Thiscall(0x0066BEA0, 0x01B65338u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5230));
}

// 0x8E3B90 (initializer)
void __cdecl Fn8E3B90() {
    Thiscall(0x0066BEA0, 0x01B6533Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5240));
}

// 0x8E3BB0 (initializer)
void __cdecl Fn8E3BB0() {
    Thiscall(0x0066BEA0, 0x01B65340u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5250));
}

// 0x8E3BD0 (initializer)
void __cdecl Fn8E3BD0() {
    Thiscall(0x0066BEA0, 0x01B65344u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5260));
}

// 0x8E3BF0 (initializer)
void __cdecl Fn8E3BF0() {
    Thiscall(0x0066BEA0, 0x01B65348u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5270));
}

// 0x8E3C10 (initializer)
void __cdecl Fn8E3C10() {
    Thiscall(0x0066BEA0, 0x01B6534Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5280));
}

// 0x8E3C30 (initializer)
void __cdecl Fn8E3C30() {
    Thiscall(0x0066BEA0, 0x01B65350u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5290));
}

// 0x8E3C50 (initializer)
void __cdecl Fn8E3C50() {
    Thiscall(0x0066BEA0, 0x01B65354u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F52A0));
}

// 0x8E3C70 (initializer)
void __cdecl Fn8E3C70() {
    Thiscall(0x0066BEA0, 0x01B65358u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F52B0));
}

// 0x8E3C90 (initializer)
void __cdecl Fn8E3C90() {
    Thiscall(0x0066BEA0, 0x01B6535Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F52C0));
}

// 0x8E3CB0 (initializer)
void __cdecl Fn8E3CB0() {
    Thiscall(0x0066BEA0, 0x01B65360u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F52D0));
}

// 0x8E3CD0 (initializer)
void __cdecl Fn8E3CD0() {
    Thiscall(0x0066BEA0, 0x01B65364u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F52E0));
}

// 0x8E3CF0 (initializer)
void __cdecl Fn8E3CF0() {
    Thiscall(0x0066BEA0, 0x01B65368u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F52F0));
}

// 0x8E3D10 (initializer)
void __cdecl Fn8E3D10() {
    Thiscall(0x0066BEA0, 0x01B6536Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5300));
}

// 0x8E3D30 (initializer)
void __cdecl Fn8E3D30() {
    Thiscall(0x0066BEA0, 0x01B65370u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5310));
}

// 0x8E3D50 (initializer)
void __cdecl Fn8E3D50() {
    Thiscall(0x0066BEA0, 0x01B65374u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5320));
}

// 0x8E3D70 (initializer)
void __cdecl Fn8E3D70() {
    Thiscall(0x0066BEA0, 0x01B65378u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5330));
}

// 0x8E3D90 (initializer)
void __cdecl Fn8E3D90() {
    Thiscall(0x0066BEA0, 0x01B6537Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5340));
}

// 0x8E3DB0 (initializer)
void __cdecl Fn8E3DB0() {
    Thiscall(0x0066BEA0, 0x01B65380u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5350));
}

// 0x8E3DD0 (initializer)
void __cdecl Fn8E3DD0() {
    Thiscall(0x0066BEA0, 0x01B65384u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5360));
}

// 0x8E3DF0 (initializer)
void __cdecl Fn8E3DF0() {
    Thiscall(0x0066BEA0, 0x01B65388u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5370));
}

// 0x8E3E10 (initializer)
void __cdecl Fn8E3E10() {
    Thiscall(0x0066BEA0, 0x01B6538Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5380));
}

// 0x8E3E30 (initializer)
void __cdecl Fn8E3E30() {
    Thiscall(0x0066BEA0, 0x01B65390u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5390));
}

// 0x8E3E50 (initializer)
void __cdecl Fn8E3E50() {
    Thiscall(0x0066BEA0, 0x01B65394u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F53A0));
}

// 0x8E3E70 (initializer)
void __cdecl Fn8E3E70() {
    Thiscall(0x0066BEA0, 0x01B65398u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F53B0));
}

// 0x8E3E90 (initializer)
void __cdecl Fn8E3E90() {
    Thiscall(0x0066BEA0, 0x01B6539Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F53C0));
}

// 0x8E3EB0 (initializer)
void __cdecl Fn8E3EB0() {
    Thiscall(0x0066BEA0, 0x01B653A0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F53D0));
}

// 0x8E3ED0 (initializer)
void __cdecl Fn8E3ED0() {
    Thiscall(0x0066BEA0, 0x01B653A4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F53E0));
}

// 0x8E3EF0 (initializer)
void __cdecl Fn8E3EF0() {
    Thiscall(0x0066BEA0, 0x01B653A8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F53F0));
}

// 0x8E3F10 (initializer)
void __cdecl Fn8E3F10() {
    Thiscall(0x0066BEA0, 0x01B653ACu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5400));
}

// 0x8E3F30 (initializer)
void __cdecl Fn8E3F30() {
    Thiscall(0x0066BEA0, 0x01B653B0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5410));
}

// 0x8E3F50 (initializer)
void __cdecl Fn8E3F50() {
    Thiscall(0x0066BEA0, 0x01B653B4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5420));
}

// 0x8E3F70 (initializer)
void __cdecl Fn8E3F70() {
    Thiscall(0x0066BEA0, 0x01B653B8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5430));
}

// 0x8E3F90 (initializer)
void __cdecl Fn8E3F90() {
    Thiscall(0x0066BEA0, 0x01B653BCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5440));
}

// 0x8E3FB0 (initializer)
void __cdecl Fn8E3FB0() {
    Thiscall(0x0066BEA0, 0x01B653C0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5450));
}

// 0x8E3FD0 (initializer)
void __cdecl Fn8E3FD0() {
    Thiscall(0x0066BEA0, 0x01B653C4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5460));
}

// 0x8E3FF0 (initializer)
void __cdecl Fn8E3FF0() {
    Thiscall(0x0066BEA0, 0x01B653C8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5470));
}

// 0x8E4010 (initializer)
void __cdecl Fn8E4010() {
    Thiscall(0x0066BEA0, 0x01B653CCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5480));
}

// 0x8E4030 (initializer)
void __cdecl Fn8E4030() {
    Thiscall(0x0066BEA0, 0x01B653D0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5490));
}

// 0x8E4050 (initializer)
void __cdecl Fn8E4050() {
    Thiscall(0x0066BEA0, 0x01B653D4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F54A0));
}

// 0x8E4070 (initializer)
void __cdecl Fn8E4070() {
    Thiscall(0x0066BEA0, 0x01B653D8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F54B0));
}

// 0x8E4090 (initializer)
void __cdecl Fn8E4090() {
    Thiscall(0x0066BEA0, 0x01B653DCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F54C0));
}

// 0x8E40B0 (initializer)
void __cdecl Fn8E40B0() {
    Thiscall(0x0066BEA0, 0x01B653E0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F54D0));
}

// 0x8E40D0 (initializer)
void __cdecl Fn8E40D0() {
    Thiscall(0x0066BEA0, 0x01B653E4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F54E0));
}

// 0x8E40F0 (initializer)
void __cdecl Fn8E40F0() {
    Thiscall(0x0066BEA0, 0x01B653E8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F54F0));
}

// 0x8E4110 (initializer)
void __cdecl Fn8E4110() {
    Thiscall(0x0066BEA0, 0x01B653ECu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5500));
}

// 0x8E4130 (initializer)
void __cdecl Fn8E4130() {
    Thiscall(0x0066BEA0, 0x01B653F0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5510));
}

// 0x8E4150 (initializer)
void __cdecl Fn8E4150() {
    Thiscall(0x0066BEA0, 0x01B653F4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5520));
}

// 0x8E4170 (initializer)
void __cdecl Fn8E4170() {
    Thiscall(0x0066BEA0, 0x01B653F8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5530));
}

// 0x8E4190 (initializer)
void __cdecl Fn8E4190() {
    Thiscall(0x0066BEA0, 0x01B653FCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5540));
}

// 0x8E41B0 (initializer)
void __cdecl Fn8E41B0() {
    Thiscall(0x0066BEA0, 0x01B65400u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5550));
}

// 0x8E41D0 (initializer)
void __cdecl Fn8E41D0() {
    Thiscall(0x0066BEA0, 0x01B65404u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5560));
}

// 0x8E41F0 (initializer)
void __cdecl Fn8E41F0() {
    Thiscall(0x0066BEA0, 0x01B65408u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5570));
}

// 0x8E4210 (initializer)
void __cdecl Fn8E4210() {
    Thiscall(0x0066BEA0, 0x01B6540Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5580));
}

// 0x8E4230 (initializer)
void __cdecl Fn8E4230() {
    Thiscall(0x0066BEA0, 0x01B65410u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5590));
}

// 0x8E4250 (initializer)
void __cdecl Fn8E4250() {
    Thiscall(0x0066BEA0, 0x01B65414u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F55A0));
}

// 0x8E4270 (initializer)
void __cdecl Fn8E4270() {
    Thiscall(0x0066BEA0, 0x01B65418u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F55B0));
}

// 0x8E4290 (initializer)
void __cdecl Fn8E4290() {
    Thiscall(0x0066BEA0, 0x01B6541Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F55C0));
}

// 0x8E42B0 (initializer)
void __cdecl Fn8E42B0() {
    Thiscall(0x0066BEA0, 0x01B65420u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F55D0));
}

// 0x8E42D0 (initializer)
void __cdecl Fn8E42D0() {
    Thiscall(0x0066BEA0, 0x01B65424u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F55E0));
}

// 0x8E42F0 (initializer)
void __cdecl Fn8E42F0() {
    Thiscall(0x0066BEA0, 0x01B65428u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F55F0));
}

// 0x8E4310 (initializer)
void __cdecl Fn8E4310() {
    Thiscall(0x0066BEA0, 0x01B6542Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5600));
}

// 0x8E4330 (initializer)
void __cdecl Fn8E4330() {
    Thiscall(0x0066BEA0, 0x01B65430u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5610));
}

// 0x8E4350 (initializer)
void __cdecl Fn8E4350() {
    Thiscall(0x0066BEA0, 0x01B65434u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5620));
}

// 0x8E4370 (initializer)
void __cdecl Fn8E4370() {
    Thiscall(0x0066BEA0, 0x01B65438u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5630));
}

// 0x8E4390 (initializer)
void __cdecl Fn8E4390() {
    Thiscall(0x0066BEA0, 0x01B6543Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5640));
}

// 0x8E43B0 (initializer)
void __cdecl Fn8E43B0() {
    Thiscall(0x0066BEA0, 0x01B65440u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5650));
}

// 0x8E43D0 (initializer)
void __cdecl Fn8E43D0() {
    Thiscall(0x0066BEA0, 0x01B65444u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5660));
}

// 0x8E43F0 (initializer)
void __cdecl Fn8E43F0() {
    Thiscall(0x0066BEA0, 0x01B65448u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5670));
}

// 0x8E4410 (initializer)
void __cdecl Fn8E4410() {
    Thiscall(0x0066BEA0, 0x01B6544Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5680));
}

// 0x8E4430 (initializer)
void __cdecl Fn8E4430() {
    Thiscall(0x0066BEA0, 0x01B65450u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5690));
}

// 0x8E4450 (initializer)
void __cdecl Fn8E4450() {
    Thiscall(0x0066BEA0, 0x01B65454u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F56A0));
}

// 0x8E4470 (initializer)
void __cdecl Fn8E4470() {
    Thiscall(0x0066BEA0, 0x01B65458u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F56B0));
}

// 0x8E4490 (initializer)
void __cdecl Fn8E4490() {
    Thiscall(0x0066BEA0, 0x01B6545Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F56C0));
}

// 0x8E44B0 (initializer)
void __cdecl Fn8E44B0() {
    Thiscall(0x0066BEA0, 0x01B65460u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F56D0));
}

// 0x8E44D0 (initializer)
void __cdecl Fn8E44D0() {
    Thiscall(0x0066BEA0, 0x01B65464u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F56E0));
}

// 0x8E44F0 (initializer)
void __cdecl Fn8E44F0() {
    Thiscall(0x0066BEA0, 0x01B65468u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F56F0));
}

// 0x8E4510 (initializer)
void __cdecl Fn8E4510() {
    Thiscall(0x0066BEA0, 0x01B6546Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5700));
}

// 0x8E4530 (initializer)
void __cdecl Fn8E4530() {
    Thiscall(0x0066BEA0, 0x01B65470u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5710));
}

// 0x8E4550 (initializer)
void __cdecl Fn8E4550() {
    Thiscall(0x0066BEA0, 0x01B65474u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5720));
}

// 0x8E4570 (initializer)
void __cdecl Fn8E4570() {
    Thiscall(0x0066BEA0, 0x01B65478u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5730));
}

// 0x8E4590 (initializer)
void __cdecl Fn8E4590() {
    Thiscall(0x0066BEA0, 0x01B6547Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5740));
}

// 0x8E45B0 (initializer)
void __cdecl Fn8E45B0() {
    Thiscall(0x0066BEA0, 0x01B65480u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5750));
}

// 0x8E45D0 (initializer)
void __cdecl Fn8E45D0() {
    Thiscall(0x0066BEA0, 0x01B65484u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5760));
}

// 0x8E45F0 (initializer)
void __cdecl Fn8E45F0() {
    Thiscall(0x0066BEA0, 0x01B65488u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5770));
}

// 0x8E4610 (initializer)
void __cdecl Fn8E4610() {
    Thiscall(0x0066BEA0, 0x01B6548Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5780));
}

// 0x8E4630 (initializer)
void __cdecl Fn8E4630() {
    Thiscall(0x0066BEA0, 0x01B65490u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5790));
}

// 0x8E4650 (initializer)
void __cdecl Fn8E4650() {
    Thiscall(0x0066BEA0, 0x01B65494u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F57A0));
}

// 0x8E4670 (initializer)
void __cdecl Fn8E4670() {
    Thiscall(0x0066BEA0, 0x01B65498u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F57B0));
}

// 0x8E4690 (initializer)
void __cdecl Fn8E4690() {
    Thiscall(0x0066BEA0, 0x01B6549Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F57C0));
}

// 0x8E46B0 (initializer)
void __cdecl Fn8E46B0() {
    Thiscall(0x0066BEA0, 0x01B654A0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F57D0));
}

// 0x8E46D0 (initializer)
void __cdecl Fn8E46D0() {
    Thiscall(0x0066BEA0, 0x01B654A4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F57E0));
}

// 0x8E46F0 (initializer)
void __cdecl Fn8E46F0() {
    Thiscall(0x0066BEA0, 0x01B654A8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F57F0));
}

// 0x8E4710 (initializer)
void __cdecl Fn8E4710() {
    Thiscall(0x0066BEA0, 0x01B654ACu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5800));
}

// 0x8E4730 (initializer)
void __cdecl Fn8E4730() {
    Thiscall(0x0066BEA0, 0x01B654B0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5810));
}

// 0x8E4750 (initializer)
void __cdecl Fn8E4750() {
    Thiscall(0x0066BEA0, 0x01B654B4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5820));
}

// 0x8E4770 (initializer)
void __cdecl Fn8E4770() {
    Thiscall(0x0066BEA0, 0x01B654B8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5830));
}

// 0x8E4790 (initializer)
void __cdecl Fn8E4790() {
    Thiscall(0x0066BEA0, 0x01B654BCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5840));
}

// 0x8E47B0 (initializer)
void __cdecl Fn8E47B0() {
    Thiscall(0x0066BEA0, 0x01B654C0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5850));
}

// 0x8E47D0 (initializer)
void __cdecl Fn8E47D0() {
    Thiscall(0x0066BEA0, 0x01B654C4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5860));
}

// 0x8E47F0 (initializer)
void __cdecl Fn8E47F0() {
    Thiscall(0x0066BEA0, 0x01B654C8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5870));
}

// 0x8E4810 (initializer)
void __cdecl Fn8E4810() {
    Thiscall(0x0066BEA0, 0x01B654CCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5880));
}

// 0x8E4830 (initializer)
void __cdecl Fn8E4830() {
    Thiscall(0x0066BEA0, 0x01B654D0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5890));
}

// 0x8E4850 (initializer)
void __cdecl Fn8E4850() {
    Thiscall(0x0066BEA0, 0x01B654D4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F58A0));
}

// 0x8E4870 (initializer)
void __cdecl Fn8E4870() {
    Thiscall(0x0066BEA0, 0x01B654D8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F58B0));
}

// 0x8E4890 (initializer)
void __cdecl Fn8E4890() {
    Thiscall(0x0066BEA0, 0x01B654DCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F58C0));
}

// 0x8E48B0 (initializer)
void __cdecl Fn8E48B0() {
    Thiscall(0x0066BEA0, 0x01B654E0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F58D0));
}

// 0x8E48D0 (initializer)
void __cdecl Fn8E48D0() {
    Thiscall(0x0066BEA0, 0x01B654E4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F58E0));
}

// 0x8E48F0 (initializer)
void __cdecl Fn8E48F0() {
    Thiscall(0x0066BEA0, 0x01B654E8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F58F0));
}

// 0x8E4910 (initializer)
void __cdecl Fn8E4910() {
    Thiscall(0x0066BEA0, 0x01B654ECu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5900));
}

// 0x8E4930 (initializer)
void __cdecl Fn8E4930() {
    Thiscall(0x0066BEA0, 0x01B654F0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5910));
}

// 0x8E4950 (initializer)
void __cdecl Fn8E4950() {
    Thiscall(0x0066BEA0, 0x01B654F4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5920));
}

// 0x8E4970 (initializer)
void __cdecl Fn8E4970() {
    Thiscall(0x0066BEA0, 0x01B654F8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5930));
}

// 0x8E4990 (initializer)
void __cdecl Fn8E4990() {
    Thiscall(0x0066BEA0, 0x01B654FCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5940));
}

// 0x8E49B0 (initializer)
void __cdecl Fn8E49B0() {
    Thiscall(0x0066BEA0, 0x01B65500u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5950));
}

// 0x8E49D0 (initializer)
void __cdecl Fn8E49D0() {
    Thiscall(0x0066BEA0, 0x01B65504u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5960));
}

// 0x8E49F0 (initializer)
void __cdecl Fn8E49F0() {
    Thiscall(0x0066BEA0, 0x01B65508u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5970));
}

// 0x8E4A10 (initializer)
void __cdecl Fn8E4A10() {
    Thiscall(0x0066BEA0, 0x01B6550Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5980));
}

// 0x8E4A30 (initializer)
void __cdecl Fn8E4A30() {
    Thiscall(0x0066BEA0, 0x01B65510u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5990));
}

// 0x8E4A50 (initializer)
void __cdecl Fn8E4A50() {
    Thiscall(0x0066BEA0, 0x01B65514u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F59A0));
}

// 0x8E4A70 (initializer)
void __cdecl Fn8E4A70() {
    Thiscall(0x0066BEA0, 0x01B65518u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F59B0));
}

// 0x8E4A90 (initializer)
void __cdecl Fn8E4A90() {
    Thiscall(0x0066BEA0, 0x01B6551Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F59C0));
}

// 0x8E4AB0 (initializer)
void __cdecl Fn8E4AB0() {
    Thiscall(0x0066BEA0, 0x01B65520u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F59D0));
}

// 0x8E4AD0 (initializer)
void __cdecl Fn8E4AD0() {
    Thiscall(0x0066BEA0, 0x01B65524u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F59E0));
}

// 0x8E4AF0 (initializer)
void __cdecl Fn8E4AF0() {
    Thiscall(0x0066BEA0, 0x01B65528u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F59F0));
}

// 0x8E4B10 (initializer)
void __cdecl Fn8E4B10() {
    Thiscall(0x0066BEA0, 0x01B6552Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A00));
}

// 0x8E4B30 (initializer)
void __cdecl Fn8E4B30() {
    Thiscall(0x0066BEA0, 0x01B65530u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A10));
}

// 0x8E4B50 (initializer)
void __cdecl Fn8E4B50() {
    Thiscall(0x0066BEA0, 0x01B65534u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A20));
}

// 0x8E4B70 (initializer)
void __cdecl Fn8E4B70() {
    Thiscall(0x0066BEA0, 0x01B65538u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A30));
}

// 0x8E4B90 (initializer)
void __cdecl Fn8E4B90() {
    Thiscall(0x0066BEA0, 0x01B6553Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A40));
}

// 0x8E4BB0 (initializer)
void __cdecl Fn8E4BB0() {
    Thiscall(0x0066BEA0, 0x01B65540u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A50));
}

// 0x8E4BD0 (initializer)
void __cdecl Fn8E4BD0() {
    Thiscall(0x0066BEA0, 0x01B65544u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A60));
}

// 0x8E4BF0 (initializer)
void __cdecl Fn8E4BF0() {
    Thiscall(0x0066BEA0, 0x01B65548u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A70));
}

// 0x8E4C10 (initializer)
void __cdecl Fn8E4C10() {
    Thiscall(0x0066BEA0, 0x01B6554Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A80));
}

// 0x8E4C30 (initializer)
void __cdecl Fn8E4C30() {
    Thiscall(0x0066BEA0, 0x01B65550u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5A90));
}

// 0x8E4C50 (initializer)
void __cdecl Fn8E4C50() {
    Thiscall(0x0066BEA0, 0x01B65554u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5AA0));
}

// 0x8E4C70 (initializer)
void __cdecl Fn8E4C70() {
    Thiscall(0x0066BEA0, 0x01B65558u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5AB0));
}

// 0x8E4C90 (initializer)
void __cdecl Fn8E4C90() {
    Thiscall(0x0066BEA0, 0x01B6555Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5AC0));
}

// 0x8E4CB0 (initializer)
void __cdecl Fn8E4CB0() {
    Thiscall(0x0066BEA0, 0x01B65560u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5AD0));
}

// 0x8E4CD0 (initializer)
void __cdecl Fn8E4CD0() {
    Thiscall(0x0066BEA0, 0x01B65564u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5AE0));
}

// 0x8E4CF0 (initializer)
void __cdecl Fn8E4CF0() {
    Float(0x01B65570) = float((1.0 / double(Float(0x00B0BCF0))));
}

// 0x8E4D10 (initializer)
void __cdecl Fn8E4D10() {
    Float(0x01B65574) = float((1.0 / (double(Float(0x00B0BCF0)) * double(Float(0x00B0BCF0)))));
}

// 0x8E4D30 (initializer)
void __cdecl Fn8E4D30() {
    Float(0x01B65578) = float((double(Float(0x01B65574)) * Double(0x00900140)));
}

// 0x8E4D50 (initializer)
void __cdecl Fn8E4D50() {
    Float(0x01B6557C) = float((Double(0x00900AD8) / double(Float(0x00B0BCF0))));
}

// 0x8E4D70 (initializer)
void __cdecl Fn8E4D70() {
    Thiscall(0x005FA910, 0x01B65580u, 0x00932FD0u /* "SoundClearSpeechFilterTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5AF0));
}

// 0x8E4D90 (initializer)
void __cdecl Fn8E4D90() {
    Thiscall(0x005FA910, 0x01B65584u, 0x00932FECu /* "SoundSetSpeechPriorityFilterTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B00));
}

// 0x8E4DB0 (initializer)
void __cdecl Fn8E4DB0() {
    Thiscall(0x005FA910, 0x01B65588u, 0x00933010u /* "SoundAddSpeechPriorityFilterTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B10));
}

// 0x8E4DD0 (initializer)
void __cdecl Fn8E4DD0() {
    Thiscall(0x005FA910, 0x01B6558Cu, 0x00933034u /* "SoundRemoveSpeechPriorityFilterTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B20));
}

// 0x8E4DF0 (initializer)
void __cdecl Fn8E4DF0() {
    Thiscall(0x005FA910, 0x01B65590u, 0x0093305Cu /* "SoundAmbienceOnTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B30));
}

// 0x8E4E10 (initializer)
void __cdecl Fn8E4E10() {
    Thiscall(0x005FA910, 0x01B65594u, 0x00933074u /* "SoundAmbienceOffTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B40));
}

// 0x8E4E30 (initializer)
void __cdecl Fn8E4E30() {
    Thiscall(0x005FA910, 0x01B65598u, 0x0093308Cu /* "PlayPreloadStreamTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B50));
}

// 0x8E4E50 (initializer)
void __cdecl Fn8E4E50() {
    Thiscall(0x005FA910, 0x01B6559Cu, 0x009330A4u /* "SoundPreloadStreamTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B60));
}

// 0x8E4E70 (initializer)
void __cdecl Fn8E4E70() {
    Thiscall(0x005FA910, 0x01B655A0u, 0x009330BCu /* "SoundStreamTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B70));
}

// 0x8E4E90 (initializer)
void __cdecl Fn8E4E90() {
    Thiscall(0x005FA910, 0x01B655A4u, 0x009330D0u /* "SoundStreamFadeoutTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B80));
}

// 0x8E4EB0 (initializer)
void __cdecl Fn8E4EB0() {
    Thiscall(0x005FA910, 0x01B655A8u, 0x009330E8u /* "SoundTimedBankLoadTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5B90));
}

// 0x8E4ED0 (initializer)
void __cdecl Fn8E4ED0() {
    Thiscall(0x005FA910, 0x01B655ACu, 0x00933100u /* "SoundTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5BA0));
}

// 0x8E4EF0 (initializer)
void __cdecl Fn8E4EF0() {
    Thiscall(0x005FA910, 0x01B655B0u, 0x0093310Cu /* "SoundRecycleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5BB0));
}

// 0x8E4F10 (initializer)
void __cdecl Fn8E4F10() {
    Thiscall(0x005FA910, 0x01B655B4u, 0x00933120u /* "DialogSoundFXTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5BC0));
}

// 0x8E4F30 (initializer)
void __cdecl Fn8E4F30() {
    Thiscall(0x005FA910, 0x01B655B8u, 0x00933134u /* "SoundLoopTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5BD0));
}

// 0x8E4F50 (initializer)
void __cdecl Fn8E4F50() {
    Thiscall(0x005FA910, 0x01B655BCu, 0x00933144u /* "CleanupDeadSoundLoopsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5BE0));
}

// 0x8E4F70 (initializer)
void __cdecl Fn8E4F70() {
    Thiscall(0x005FA910, 0x01B655C0u, 0x00933160u /* "SoundParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5BF0));
}

// 0x8E4F90 (initializer)
void __cdecl Fn8E4F90() {
    Thiscall(0x005FA910, 0x01B655C4u, 0x00933178u /* "SoundTrack2D" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C00));
}

// 0x8E4FB0 (initializer)
void __cdecl Fn8E4FB0() {
    Thiscall(0x005FA910, 0x01B655C8u, 0x00933188u /* "EmitterEnableTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C10));
}

// 0x8E5010 (initializer)
void __cdecl Fn8E5010() {
    Thiscall(0x005FA910, 0x01B655CCu, 0x0093319Cu /* "SpeechTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C20));
}

// 0x8E5030 (initializer)
void __cdecl Fn8E5030() {
    Thiscall(0x005FA910, 0x01B65620u, 0x009331A8u /* "SpeechStartTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C30));
}

// 0x8E5050 (initializer)
void __cdecl Fn8E5050() {
    Thiscall(0x005FA910, 0x01B65624u, 0x009331BCu /* "RemoveAllQueuedSpeechTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C40));
}

// 0x8E5070 (initializer)
void __cdecl Fn8E5070() {
    Thiscall(0x005FA910, 0x01B65628u, 0x009331D8u /* "BikeSoundStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C50));
}

// 0x8E5090 (initializer)
void __cdecl Fn8E5090() {
    Thiscall(0x005FA910, 0x01B6562Cu, 0x009331ECu /* "CrowdCheerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C60));
}

// 0x8E50B0 (initializer)
void __cdecl Fn8E50B0() {
    Thiscall(0x005FA910, 0x01B65630u, 0x009331FCu /* "CrowdBooTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C70));
}

// 0x8E50D0 (initializer)
void __cdecl Fn8E50D0() {
    Thiscall(0x005FA910, 0x01B65634u, 0x0093320Cu /* "CrowdExciteTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C80));
}

// 0x8E50F0 (initializer)
void __cdecl Fn8E50F0() {
    Thiscall(0x005FA910, 0x01B65638u, 0x00933220u /* "CrowdRelaxTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5C90));
}

// 0x8E5110 (initializer)
void __cdecl Fn8E5110() {
    Thiscall(0x005FA910, 0x01B6563Cu, 0x00933230u /* "AISocialSpeechEventTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5CA0));
}

// 0x8E5130 (initializer)
void __cdecl Fn8E5130() {
    Thiscall(0x005FA910, 0x01B65640u, 0x0093324Cu /* "PlayerSocialSpeechEventTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5CB0));
}

// 0x8E5150 (initializer)
void __cdecl Fn8E5150() {
    Thiscall(0x005FA910, 0x01B65644u, 0x0093326Cu /* "SoundFadeWithCameraTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5CC0));
}

// 0x8E5170 (initializer)
void __cdecl Fn8E5170() {
    Thiscall(0x005FA910, 0x01B65648u, 0x00933288u /* "MusicAllowPlayDuringCutscenesTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5CD0));
}

// 0x8E5190 (initializer)
void __cdecl Fn8E5190() {
    Float(0x01B6564C) = float((1.0 / double(Float(0x00B0C078))));
}

// 0x8E51B0 (initializer)
void __cdecl Fn8E51B0() {
    Float(0x01B65650) = float((1.0 / (double(Float(0x00B0C078)) * double(Float(0x00B0C078)))));
}

// 0x8E51D0 (initializer)
void __cdecl Fn8E51D0() {
    Float(0x01B65654) = float((double(Float(0x01B65650)) * Double(0x00900140)));
}

// 0x8E51F0 (initializer)
void __cdecl Fn8E51F0() {
    Float(0x01B65658) = float((Double(0x00900AD8) / double(Float(0x00B0C078))));
}

// 0x8E5210 (initializer)
void __cdecl Fn8E5210() {
    Thiscall(0x0061A7E0, 0x01B6565Cu);  // TrackRegistry::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D10));
}

// 0x8E5230 (initializer)
void __cdecl Fn8E5230() {
    Thiscall(0x005FA910, 0x01B6566Cu, 0x009332C0u /* "PrintTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D20));
}

// 0x8E5250 (initializer)
void __cdecl Fn8E5250() {
    Float(0x01B6572C) = float((1.0 / double(Float(0x00B0C0B4))));
}

// 0x8E5270 (initializer)
void __cdecl Fn8E5270() {
    Float(0x01B65730) = float((1.0 / (double(Float(0x00B0C0B4)) * double(Float(0x00B0C0B4)))));
}

// 0x8E5290 (initializer)
void __cdecl Fn8E5290() {
    Float(0x01B65734) = float((double(Float(0x01B65730)) * Double(0x00900140)));
}

// 0x8E52B0 (initializer)
void __cdecl Fn8E52B0() {
    Float(0x01B65738) = float((Double(0x00900AD8) / double(Float(0x00B0C0B4))));
}

// 0x8E52D0 (initializer)
void __cdecl Fn8E52D0() {
    Thiscall(0x005FA910, 0x01B6573Cu, 0x009336E8u /* "SeekEntityPositionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D30));
}

// 0x8E52F0 (initializer)
void __cdecl Fn8E52F0() {
    Thiscall(0x005FA910, 0x01B65740u, 0x00933700u /* "AIUpdateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D50));
}

// 0x8E5310 (initializer)
void __cdecl Fn8E5310() {
    Thiscall(0x005FA910, 0x01B65744u, 0x00933710u /* "ControllerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D60));
}

// 0x8E5330 (initializer)
void __cdecl Fn8E5330() {
    Thiscall(0x005FA910, 0x01B65748u, 0x00933720u /* "IdleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D70));
}

// 0x8E5350 (initializer)
void __cdecl Fn8E5350() {
    Thiscall(0x005FA910, 0x01B6574Cu, 0x0093372Cu /* "ScriptedAmbientTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D80));
}

// 0x8E5370 (initializer)
void __cdecl Fn8E5370() {
    Thiscall(0x005FA910, 0x01B65750u, 0x00933744u /* "WrestleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5D90));
}

// 0x8E5390 (initializer)
void __cdecl Fn8E5390() {
    Thiscall(0x005FA910, 0x01B65754u, 0x00933754u /* "CreateStimulusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5DA0));
}

// 0x8E53B0 (initializer)
void __cdecl Fn8E53B0() {
    Thiscall(0x005FA910, 0x01B65758u, 0x00933768u /* "ClearStimulusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5DB0));
}

// 0x8E53D0 (initializer)
void __cdecl Fn8E53D0() {
    Thiscall(0x005FA910, 0x01B6575Cu, 0x0093377Cu /* "ClearCurrentStimulusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5DC0));
}

// 0x8E53F0 (initializer)
void __cdecl Fn8E53F0() {
    Thiscall(0x005FA910, 0x01B65760u, 0x00933798u /* "DeteriorateAttitudeToAttackerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5DD0));
}

// 0x8E5410 (initializer)
void __cdecl Fn8E5410() {
    Thiscall(0x005FA910, 0x01B65764u, 0x009337BCu /* "WarnTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5DE0));
}

// 0x8E5430 (initializer)
void __cdecl Fn8E5430() {
    Thiscall(0x005FA910, 0x01B65768u, 0x009337CCu /* "WarnTarget2Track" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5DF0));
}

// 0x8E5450 (initializer)
void __cdecl Fn8E5450() {
    Thiscall(0x005FA910, 0x01B6576Cu, 0x009337E0u /* "PunishableTargetThreatenedTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E00));
}

// 0x8E5470 (initializer)
void __cdecl Fn8E5470() {
    Thiscall(0x005FA910, 0x01B65770u, 0x00933800u /* "GiveTargetToFocusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E10));
}

// 0x8E5490 (initializer)
void __cdecl Fn8E5490() {
    Thiscall(0x005FA910, 0x01B65774u, 0x00933818u /* "GivePunishmentPointsToTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E20));
}

// 0x8E54B0 (initializer)
void __cdecl Fn8E54B0() {
    Thiscall(0x005FA910, 0x01B65778u, 0x0093383Cu /* "GivePunishmentPointsToPlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E30));
}

// 0x8E54D0 (initializer)
void __cdecl Fn8E54D0() {
    Thiscall(0x005FA910, 0x01B6577Cu, 0x00933860u /* "CirculateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E40));
}

// 0x8E54F0 (initializer)
void __cdecl Fn8E54F0() {
    Thiscall(0x005FA910, 0x01B65780u, 0x00933870u /* "WanderTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E50));
}

// 0x8E5510 (initializer)
void __cdecl Fn8E5510() {
    Thiscall(0x005FA910, 0x01B65784u, 0x0093387Cu /* "RideAroundTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E60));
}

// 0x8E5530 (initializer)
void __cdecl Fn8E5530() {
    Thiscall(0x005FA910, 0x01B65788u, 0x0093388Cu /* "HaltVehicleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E70));
}

// 0x8E5550 (initializer)
void __cdecl Fn8E5550() {
    Thiscall(0x005FA910, 0x01B6578Cu, 0x009338A0u /* "FleeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E80));
}

// 0x8E5570 (initializer)
void __cdecl Fn8E5570() {
    Thiscall(0x005FA910, 0x01B65790u, 0x009338ACu /* "HangOutTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5E90));
}

// 0x8E5590 (initializer)
void __cdecl Fn8E5590() {
    Thiscall(0x005FA910, 0x01B65794u, 0x009338BCu /* "SpectatorTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5EA0));
}

// 0x8E55B0 (initializer)
void __cdecl Fn8E55B0() {
    Thiscall(0x005FA910, 0x01B65798u, 0x009338CCu /* "GenerateDynamicPOITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5EB0));
}

// 0x8E55D0 (initializer)
void __cdecl Fn8E55D0() {
    Thiscall(0x005FA910, 0x01B6579Cu, 0x009338E4u /* "JoinDynamicPOITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5EC0));
}

// 0x8E55F0 (initializer)
void __cdecl Fn8E55F0() {
    Thiscall(0x005FA910, 0x01B657A0u, 0x009338F8u /* "FaceDirectionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5ED0));
}

// 0x8E5610 (initializer)
void __cdecl Fn8E5610() {
    Thiscall(0x005FA910, 0x01B657A4u, 0x0093390Cu /* "FaceStimulusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5EE0));
}

// 0x8E5630 (initializer)
void __cdecl Fn8E5630() {
    Thiscall(0x005FA910, 0x01B657A8u, 0x00933920u /* "LookAroundTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5EF0));
}

// 0x8E5650 (initializer)
void __cdecl Fn8E5650() {
    Thiscall(0x005FA910, 0x01B657ACu, 0x00933930u /* "MoveToPOITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F00));
}

// 0x8E5670 (initializer)
void __cdecl Fn8E5670() {
    Thiscall(0x005FA910, 0x01B657B0u, 0x00933940u /* "ClearPOITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F10));
}

// 0x8E5690 (initializer)
void __cdecl Fn8E5690() {
    Thiscall(0x005FA910, 0x01B657B4u, 0x00933950u /* "IncrPlayerPhysicalStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F20));
}

// 0x8E56B0 (initializer)
void __cdecl Fn8E56B0() {
    Thiscall(0x005FA910, 0x01B657B8u, 0x00933970u /* "DecrPlayerPhysicalStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F30));
}

// 0x8E56D0 (initializer)
void __cdecl Fn8E56D0() {
    Thiscall(0x005FA910, 0x01B657BCu, 0x00933990u /* "ResetPlayerPhysicalStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F40));
}

// 0x8E56F0 (initializer)
void __cdecl Fn8E56F0() {
    Thiscall(0x005FA910, 0x01B657C0u, 0x009339B0u /* "RefreshPlayerPhysicalStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F50));
}

// 0x8E5710 (initializer)
void __cdecl Fn8E5710() {
    Thiscall(0x005FA910, 0x01B657C4u, 0x009339D0u /* "SitDownTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F60));
}

// 0x8E5730 (initializer)
void __cdecl Fn8E5730() {
    Thiscall(0x005FA910, 0x01B657C8u, 0x009339E0u /* "FaceTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F70));
}

// 0x8E5750 (initializer)
void __cdecl Fn8E5750() {
    Thiscall(0x005FA910, 0x01B657CCu, 0x009339F0u /* "FaceVehicleForEntryTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F80));
}

// 0x8E5770 (initializer)
void __cdecl Fn8E5770() {
    Thiscall(0x005FA910, 0x01B657D0u, 0x00933A0Cu /* "FaceDiveEntityTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5F90));
}

// 0x8E5790 (initializer)
void __cdecl Fn8E5790() {
    Thiscall(0x005FA910, 0x01B657D4u, 0x00933A20u /* "FaceHitDirectionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5FA0));
}

// 0x8E57B0 (initializer)
void __cdecl Fn8E57B0() {
    Thiscall(0x005FA910, 0x01B657D8u, 0x00933A38u /* "MoveToStimulusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5FB0));
}

// 0x8E57D0 (initializer)
void __cdecl Fn8E57D0() {
    Thiscall(0x005FA910, 0x01B657DCu, 0x00933A4Cu /* "InvestigateAreaTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5FC0));
}

// 0x8E57F0 (initializer)
void __cdecl Fn8E57F0() {
    Thiscall(0x005FA910, 0x01B657E0u, 0x00933A64u /* "AddWanderObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5FD0));
}

// 0x8E5810 (initializer)
void __cdecl Fn8E5810() {
    Thiscall(0x005FA910, 0x01B657E4u, 0x00933A7Cu /* "RemoveObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5FE0));
}

// 0x8E5830 (initializer)
void __cdecl Fn8E5830() {
    Thiscall(0x005FA910, 0x01B657E8u, 0x00933A94u /* "MoveAwayFromStimulusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F5FF0));
}

// 0x8E5850 (initializer)
void __cdecl Fn8E5850() {
    Thiscall(0x005FA910, 0x01B657ECu, 0x00933AB0u /* "MoveToCurrentPlayerPositionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6000));
}

// 0x8E5870 (initializer)
void __cdecl Fn8E5870() {
    Thiscall(0x005FA910, 0x01B657F0u, 0x00933AD4u /* "CheerFightTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6010));
}

// 0x8E5890 (initializer)
void __cdecl Fn8E5890() {
    Thiscall(0x005FA910, 0x01B657F4u, 0x00933AE4u /* "FollowPathTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6020));
}

// 0x8E58B0 (initializer)
void __cdecl Fn8E58B0() {
    Thiscall(0x005FA910, 0x01B657F8u, 0x00933AF4u /* "FleeOnPathOnBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6030));
}

// 0x8E58D0 (initializer)
void __cdecl Fn8E58D0() {
    Thiscall(0x005FA910, 0x01B657FCu, 0x00933B0Cu /* "FleeOnRoadOnBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6040));
}

// 0x8E58F0 (initializer)
void __cdecl Fn8E58F0() {
    Thiscall(0x005FA910, 0x01B65800u, 0x00933B24u /* "FollowFocusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6050));
}

// 0x8E5910 (initializer)
void __cdecl Fn8E5910() {
    Thiscall(0x005FA910, 0x01B65804u, 0x00933B38u /* "FollowFocusOnBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6060));
}

// 0x8E5930 (initializer)
void __cdecl Fn8E5930() {
    Thiscall(0x005FA910, 0x01B65808u, 0x00933B50u /* "FollowTargetOnBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6070));
}

// 0x8E5950 (initializer)
void __cdecl Fn8E5950() {
    Thiscall(0x005FA910, 0x01B6580Cu, 0x00933B68u /* "AttackOnBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6080));
}

// 0x8E5970 (initializer)
void __cdecl Fn8E5970() {
    Thiscall(0x005FA910, 0x01B65810u, 0x00933B7Cu /* "StealBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6090));
}

// 0x8E5990 (initializer)
void __cdecl Fn8E5990() {
    Thiscall(0x005FA910, 0x01B65814u, 0x00933B8Cu /* "RaceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F60A0));
}

// 0x8E59B0 (initializer)
void __cdecl Fn8E59B0() {
    Thiscall(0x005FA910, 0x01B65818u, 0x00933B98u /* "DockTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F60B0));
}

} // namespace
} // namespace staticinit

#define STATICINIT2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT2_REVERSED("StaticInit::8E01F0", 0x008E01F0, &staticinit::Fn8E01F0);
STATICINIT2_REVERSED("StaticInit::8E0210", 0x008E0210, &staticinit::Fn8E0210);
STATICINIT2_REVERSED("StaticInit::8E0230", 0x008E0230, &staticinit::Fn8E0230);
STATICINIT2_REVERSED("StaticInit::8E0250", 0x008E0250, &staticinit::Fn8E0250);
STATICINIT2_REVERSED("StaticInit::8E0270", 0x008E0270, &staticinit::Fn8E0270);
STATICINIT2_REVERSED("StaticInit::8E0290", 0x008E0290, &staticinit::Fn8E0290);
STATICINIT2_REVERSED("StaticInit::8E02B0", 0x008E02B0, &staticinit::Fn8E02B0);
STATICINIT2_REVERSED("StaticInit::8E02D0", 0x008E02D0, &staticinit::Fn8E02D0);
STATICINIT2_REVERSED("StaticInit::8E02F0", 0x008E02F0, &staticinit::Fn8E02F0);
STATICINIT2_REVERSED("StaticInit::8E0310", 0x008E0310, &staticinit::Fn8E0310);
STATICINIT2_REVERSED("StaticInit::8E0330", 0x008E0330, &staticinit::Fn8E0330);
STATICINIT2_REVERSED("StaticInit::8E0350", 0x008E0350, &staticinit::Fn8E0350);
STATICINIT2_REVERSED("StaticInit::8E0370", 0x008E0370, &staticinit::Fn8E0370);
STATICINIT2_REVERSED("StaticInit::8E0390", 0x008E0390, &staticinit::Fn8E0390);
STATICINIT2_REVERSED("StaticInit::8E03B0", 0x008E03B0, &staticinit::Fn8E03B0);
STATICINIT2_REVERSED("StaticInit::8E03D0", 0x008E03D0, &staticinit::Fn8E03D0);
STATICINIT2_REVERSED("StaticInit::8E03F0", 0x008E03F0, &staticinit::Fn8E03F0);
STATICINIT2_REVERSED("StaticInit::8E0410", 0x008E0410, &staticinit::Fn8E0410);
STATICINIT2_REVERSED("StaticInit::8E0430", 0x008E0430, &staticinit::Fn8E0430);
STATICINIT2_REVERSED("StaticInit::8E0450", 0x008E0450, &staticinit::Fn8E0450);
STATICINIT2_REVERSED("StaticInit::8E0470", 0x008E0470, &staticinit::Fn8E0470);
STATICINIT2_REVERSED("StaticInit::8E0490", 0x008E0490, &staticinit::Fn8E0490);
STATICINIT2_REVERSED("StaticInit::8E04B0", 0x008E04B0, &staticinit::Fn8E04B0);
STATICINIT2_REVERSED("StaticInit::8E04D0", 0x008E04D0, &staticinit::Fn8E04D0);
STATICINIT2_REVERSED("StaticInit::8E04F0", 0x008E04F0, &staticinit::Fn8E04F0);
STATICINIT2_REVERSED("StaticInit::8E0510", 0x008E0510, &staticinit::Fn8E0510);
STATICINIT2_REVERSED("StaticInit::8E0530", 0x008E0530, &staticinit::Fn8E0530);
STATICINIT2_REVERSED("StaticInit::8E0550", 0x008E0550, &staticinit::Fn8E0550);
STATICINIT2_REVERSED("StaticInit::8E0570", 0x008E0570, &staticinit::Fn8E0570);
STATICINIT2_REVERSED("StaticInit::8E0590", 0x008E0590, &staticinit::Fn8E0590);
STATICINIT2_REVERSED("StaticInit::8E05B0", 0x008E05B0, &staticinit::Fn8E05B0);
STATICINIT2_REVERSED("StaticInit::8E05D0", 0x008E05D0, &staticinit::Fn8E05D0);
STATICINIT2_REVERSED("StaticInit::8E05F0", 0x008E05F0, &staticinit::Fn8E05F0);
STATICINIT2_REVERSED("StaticInit::8E0610", 0x008E0610, &staticinit::Fn8E0610);
STATICINIT2_REVERSED("StaticInit::8E0630", 0x008E0630, &staticinit::Fn8E0630);
STATICINIT2_REVERSED("StaticInit::8E0650", 0x008E0650, &staticinit::Fn8E0650);
STATICINIT2_REVERSED("StaticInit::8E0670", 0x008E0670, &staticinit::Fn8E0670);
STATICINIT2_REVERSED("StaticInit::8E0690", 0x008E0690, &staticinit::Fn8E0690);
STATICINIT2_REVERSED("StaticInit::8E06B0", 0x008E06B0, &staticinit::Fn8E06B0);
STATICINIT2_REVERSED("StaticInit::8E06D0", 0x008E06D0, &staticinit::Fn8E06D0);
STATICINIT2_REVERSED("StaticInit::8E06F0", 0x008E06F0, &staticinit::Fn8E06F0);
STATICINIT2_REVERSED("StaticInit::8E0710", 0x008E0710, &staticinit::Fn8E0710);
STATICINIT2_REVERSED("StaticInit::8E0730", 0x008E0730, &staticinit::Fn8E0730);
STATICINIT2_REVERSED("StaticInit::8E0750", 0x008E0750, &staticinit::Fn8E0750);
STATICINIT2_REVERSED("StaticInit::8E0770", 0x008E0770, &staticinit::Fn8E0770);
STATICINIT2_REVERSED("StaticInit::8E0790", 0x008E0790, &staticinit::Fn8E0790);
STATICINIT2_REVERSED("StaticInit::8E07B0", 0x008E07B0, &staticinit::Fn8E07B0);
STATICINIT2_REVERSED("StaticInit::8E07D0", 0x008E07D0, &staticinit::Fn8E07D0);
STATICINIT2_REVERSED("StaticInit::8E07F0", 0x008E07F0, &staticinit::Fn8E07F0);
STATICINIT2_REVERSED("StaticInit::8E0810", 0x008E0810, &staticinit::Fn8E0810);
STATICINIT2_REVERSED("StaticInit::8E0830", 0x008E0830, &staticinit::Fn8E0830);
STATICINIT2_REVERSED("StaticInit::8E0850", 0x008E0850, &staticinit::Fn8E0850);
STATICINIT2_REVERSED("StaticInit::8E0870", 0x008E0870, &staticinit::Fn8E0870);
STATICINIT2_REVERSED("StaticInit::8E0890", 0x008E0890, &staticinit::Fn8E0890);
STATICINIT2_REVERSED("StaticInit::8E08B0", 0x008E08B0, &staticinit::Fn8E08B0);
STATICINIT2_REVERSED("StaticInit::8E08D0", 0x008E08D0, &staticinit::Fn8E08D0);
STATICINIT2_REVERSED("StaticInit::8E08F0", 0x008E08F0, &staticinit::Fn8E08F0);
STATICINIT2_REVERSED("StaticInit::8E0910", 0x008E0910, &staticinit::Fn8E0910);
STATICINIT2_REVERSED("StaticInit::8E0930", 0x008E0930, &staticinit::Fn8E0930);
STATICINIT2_REVERSED("StaticInit::8E0950", 0x008E0950, &staticinit::Fn8E0950);
STATICINIT2_REVERSED("StaticInit::8E0970", 0x008E0970, &staticinit::Fn8E0970);
STATICINIT2_REVERSED("StaticInit::8E0990", 0x008E0990, &staticinit::Fn8E0990);
STATICINIT2_REVERSED("StaticInit::8E09B0", 0x008E09B0, &staticinit::Fn8E09B0);
STATICINIT2_REVERSED("StaticInit::8E09D0", 0x008E09D0, &staticinit::Fn8E09D0);
STATICINIT2_REVERSED("StaticInit::8E09F0", 0x008E09F0, &staticinit::Fn8E09F0);
STATICINIT2_REVERSED("StaticInit::8E0A10", 0x008E0A10, &staticinit::Fn8E0A10);
STATICINIT2_REVERSED("StaticInit::8E0A30", 0x008E0A30, &staticinit::Fn8E0A30);
STATICINIT2_REVERSED("StaticInit::8E0A50", 0x008E0A50, &staticinit::Fn8E0A50);
STATICINIT2_REVERSED("StaticInit::8E0A70", 0x008E0A70, &staticinit::Fn8E0A70);
STATICINIT2_REVERSED("StaticInit::8E0A90", 0x008E0A90, &staticinit::Fn8E0A90);
STATICINIT2_REVERSED("StaticInit::8E0AB0", 0x008E0AB0, &staticinit::Fn8E0AB0);
STATICINIT2_REVERSED("StaticInit::8E0AD0", 0x008E0AD0, &staticinit::Fn8E0AD0);
STATICINIT2_REVERSED("StaticInit::8E0AF0", 0x008E0AF0, &staticinit::Fn8E0AF0);
STATICINIT2_REVERSED("StaticInit::8E0B10", 0x008E0B10, &staticinit::Fn8E0B10);
STATICINIT2_REVERSED("StaticInit::8E0B30", 0x008E0B30, &staticinit::Fn8E0B30);
STATICINIT2_REVERSED("StaticInit::8E0B50", 0x008E0B50, &staticinit::Fn8E0B50);
STATICINIT2_REVERSED("StaticInit::8E0B70", 0x008E0B70, &staticinit::Fn8E0B70);
STATICINIT2_REVERSED("StaticInit::8E0B90", 0x008E0B90, &staticinit::Fn8E0B90);
STATICINIT2_REVERSED("StaticInit::8E0BB0", 0x008E0BB0, &staticinit::Fn8E0BB0);
STATICINIT2_REVERSED("StaticInit::8E0BD0", 0x008E0BD0, &staticinit::Fn8E0BD0);
STATICINIT2_REVERSED("StaticInit::8E0BF0", 0x008E0BF0, &staticinit::Fn8E0BF0);
STATICINIT2_REVERSED("StaticInit::8E0C10", 0x008E0C10, &staticinit::Fn8E0C10);
STATICINIT2_REVERSED("StaticInit::8E0C30", 0x008E0C30, &staticinit::Fn8E0C30);
STATICINIT2_REVERSED("StaticInit::8E0C50", 0x008E0C50, &staticinit::Fn8E0C50);
STATICINIT2_REVERSED("StaticInit::8E0C70", 0x008E0C70, &staticinit::Fn8E0C70);
STATICINIT2_REVERSED("StaticInit::8E0C90", 0x008E0C90, &staticinit::Fn8E0C90);
STATICINIT2_REVERSED("StaticInit::8E0CB0", 0x008E0CB0, &staticinit::Fn8E0CB0);
STATICINIT2_REVERSED("StaticInit::8E0CD0", 0x008E0CD0, &staticinit::Fn8E0CD0);
STATICINIT2_REVERSED("StaticInit::8E0CF0", 0x008E0CF0, &staticinit::Fn8E0CF0);
STATICINIT2_REVERSED("StaticInit::8E0D10", 0x008E0D10, &staticinit::Fn8E0D10);
STATICINIT2_REVERSED("StaticInit::8E0D30", 0x008E0D30, &staticinit::Fn8E0D30);
STATICINIT2_REVERSED("StaticInit::8E0D50", 0x008E0D50, &staticinit::Fn8E0D50);
STATICINIT2_REVERSED("StaticInit::8E0D70", 0x008E0D70, &staticinit::Fn8E0D70);
STATICINIT2_REVERSED("StaticInit::8E0D90", 0x008E0D90, &staticinit::Fn8E0D90);
STATICINIT2_REVERSED("StaticInit::8E0DB0", 0x008E0DB0, &staticinit::Fn8E0DB0);
STATICINIT2_REVERSED("StaticInit::8E0DD0", 0x008E0DD0, &staticinit::Fn8E0DD0);
STATICINIT2_REVERSED("StaticInit::8E0E10", 0x008E0E10, &staticinit::Fn8E0E10);
STATICINIT2_REVERSED("StaticInit::8E0E30", 0x008E0E30, &staticinit::Fn8E0E30);
STATICINIT2_REVERSED("StaticInit::8E0E50", 0x008E0E50, &staticinit::Fn8E0E50);
STATICINIT2_REVERSED("StaticInit::8E0E70", 0x008E0E70, &staticinit::Fn8E0E70);
STATICINIT2_REVERSED("StaticInit::8E0E90", 0x008E0E90, &staticinit::Fn8E0E90);
STATICINIT2_REVERSED("StaticInit::8E0EB0", 0x008E0EB0, &staticinit::Fn8E0EB0);
STATICINIT2_REVERSED("StaticInit::8E0ED0", 0x008E0ED0, &staticinit::Fn8E0ED0);
STATICINIT2_REVERSED("StaticInit::8E0EF0", 0x008E0EF0, &staticinit::Fn8E0EF0);
STATICINIT2_REVERSED("StaticInit::8E0F10", 0x008E0F10, &staticinit::Fn8E0F10);
STATICINIT2_REVERSED("StaticInit::8E0F30", 0x008E0F30, &staticinit::Fn8E0F30);
STATICINIT2_REVERSED("StaticInit::8E0F50", 0x008E0F50, &staticinit::Fn8E0F50);
STATICINIT2_REVERSED("StaticInit::8E0F70", 0x008E0F70, &staticinit::Fn8E0F70);
STATICINIT2_REVERSED("StaticInit::8E0F90", 0x008E0F90, &staticinit::Fn8E0F90);
STATICINIT2_REVERSED("StaticInit::8E0FB0", 0x008E0FB0, &staticinit::Fn8E0FB0);
STATICINIT2_REVERSED("StaticInit::8E0FD0", 0x008E0FD0, &staticinit::Fn8E0FD0);
STATICINIT2_REVERSED("StaticInit::8E0FF0", 0x008E0FF0, &staticinit::Fn8E0FF0);
STATICINIT2_REVERSED("StaticInit::8E1010", 0x008E1010, &staticinit::Fn8E1010);
STATICINIT2_REVERSED("StaticInit::8E1030", 0x008E1030, &staticinit::Fn8E1030);
STATICINIT2_REVERSED("StaticInit::8E1050", 0x008E1050, &staticinit::Fn8E1050);
STATICINIT2_REVERSED("StaticInit::8E1070", 0x008E1070, &staticinit::Fn8E1070);
STATICINIT2_REVERSED("StaticInit::8E1090", 0x008E1090, &staticinit::Fn8E1090);
STATICINIT2_REVERSED("StaticInit::8E10B0", 0x008E10B0, &staticinit::Fn8E10B0);
STATICINIT2_REVERSED("StaticInit::8E10D0", 0x008E10D0, &staticinit::Fn8E10D0);
STATICINIT2_REVERSED("StaticInit::8E10F0", 0x008E10F0, &staticinit::Fn8E10F0);
STATICINIT2_REVERSED("StaticInit::8E1110", 0x008E1110, &staticinit::Fn8E1110);
STATICINIT2_REVERSED("StaticInit::8E1130", 0x008E1130, &staticinit::Fn8E1130);
STATICINIT2_REVERSED("StaticInit::8E1150", 0x008E1150, &staticinit::Fn8E1150);
STATICINIT2_REVERSED("StaticInit::8E1170", 0x008E1170, &staticinit::Fn8E1170);
STATICINIT2_REVERSED("StaticInit::8E1190", 0x008E1190, &staticinit::Fn8E1190);
STATICINIT2_REVERSED("StaticInit::8E11B0", 0x008E11B0, &staticinit::Fn8E11B0);
STATICINIT2_REVERSED("StaticInit::8E11D0", 0x008E11D0, &staticinit::Fn8E11D0);
STATICINIT2_REVERSED("StaticInit::8E11F0", 0x008E11F0, &staticinit::Fn8E11F0);
STATICINIT2_REVERSED("StaticInit::8E1210", 0x008E1210, &staticinit::Fn8E1210);
STATICINIT2_REVERSED("StaticInit::8E1230", 0x008E1230, &staticinit::Fn8E1230);
STATICINIT2_REVERSED("StaticInit::8E1250", 0x008E1250, &staticinit::Fn8E1250);
STATICINIT2_REVERSED("StaticInit::8E1270", 0x008E1270, &staticinit::Fn8E1270);
STATICINIT2_REVERSED("StaticInit::8E1290", 0x008E1290, &staticinit::Fn8E1290);
STATICINIT2_REVERSED("StaticInit::8E12B0", 0x008E12B0, &staticinit::Fn8E12B0);
STATICINIT2_REVERSED("StaticInit::8E12D0", 0x008E12D0, &staticinit::Fn8E12D0);
STATICINIT2_REVERSED("StaticInit::8E12F0", 0x008E12F0, &staticinit::Fn8E12F0);
STATICINIT2_REVERSED("StaticInit::8E1310", 0x008E1310, &staticinit::Fn8E1310);
STATICINIT2_REVERSED("StaticInit::8E1330", 0x008E1330, &staticinit::Fn8E1330);
STATICINIT2_REVERSED("StaticInit::8E1350", 0x008E1350, &staticinit::Fn8E1350);
STATICINIT2_REVERSED("StaticInit::8E1370", 0x008E1370, &staticinit::Fn8E1370);
STATICINIT2_REVERSED("StaticInit::8E1390", 0x008E1390, &staticinit::Fn8E1390);
STATICINIT2_REVERSED("StaticInit::8E13B0", 0x008E13B0, &staticinit::Fn8E13B0);
STATICINIT2_REVERSED("StaticInit::8E13D0", 0x008E13D0, &staticinit::Fn8E13D0);
STATICINIT2_REVERSED("StaticInit::8E13F0", 0x008E13F0, &staticinit::Fn8E13F0);
STATICINIT2_REVERSED("StaticInit::8E1410", 0x008E1410, &staticinit::Fn8E1410);
STATICINIT2_REVERSED("StaticInit::8E1430", 0x008E1430, &staticinit::Fn8E1430);
STATICINIT2_REVERSED("StaticInit::8E1450", 0x008E1450, &staticinit::Fn8E1450);
STATICINIT2_REVERSED("StaticInit::8E1470", 0x008E1470, &staticinit::Fn8E1470);
STATICINIT2_REVERSED("StaticInit::8E1490", 0x008E1490, &staticinit::Fn8E1490);
STATICINIT2_REVERSED("StaticInit::8E14B0", 0x008E14B0, &staticinit::Fn8E14B0);
STATICINIT2_REVERSED("StaticInit::8E14D0", 0x008E14D0, &staticinit::Fn8E14D0);
STATICINIT2_REVERSED("StaticInit::8E14F0", 0x008E14F0, &staticinit::Fn8E14F0);
STATICINIT2_REVERSED("StaticInit::8E1510", 0x008E1510, &staticinit::Fn8E1510);
STATICINIT2_REVERSED("StaticInit::8E1530", 0x008E1530, &staticinit::Fn8E1530);
STATICINIT2_REVERSED("StaticInit::8E1550", 0x008E1550, &staticinit::Fn8E1550);
STATICINIT2_REVERSED("StaticInit::8E1570", 0x008E1570, &staticinit::Fn8E1570);
STATICINIT2_REVERSED("StaticInit::8E1590", 0x008E1590, &staticinit::Fn8E1590);
STATICINIT2_REVERSED("StaticInit::8E15B0", 0x008E15B0, &staticinit::Fn8E15B0);
STATICINIT2_REVERSED("StaticInit::8E15D0", 0x008E15D0, &staticinit::Fn8E15D0);
STATICINIT2_REVERSED("StaticInit::8E15F0", 0x008E15F0, &staticinit::Fn8E15F0);
STATICINIT2_REVERSED("StaticInit::8E1610", 0x008E1610, &staticinit::Fn8E1610);
STATICINIT2_REVERSED("StaticInit::8E1630", 0x008E1630, &staticinit::Fn8E1630);
STATICINIT2_REVERSED("StaticInit::8E1650", 0x008E1650, &staticinit::Fn8E1650);
STATICINIT2_REVERSED("StaticInit::8E1670", 0x008E1670, &staticinit::Fn8E1670);
STATICINIT2_REVERSED("StaticInit::8E1690", 0x008E1690, &staticinit::Fn8E1690);
STATICINIT2_REVERSED("StaticInit::8E16B0", 0x008E16B0, &staticinit::Fn8E16B0);
STATICINIT2_REVERSED("StaticInit::8E16D0", 0x008E16D0, &staticinit::Fn8E16D0);
STATICINIT2_REVERSED("StaticInit::8E16F0", 0x008E16F0, &staticinit::Fn8E16F0);
STATICINIT2_REVERSED("StaticInit::8E1710", 0x008E1710, &staticinit::Fn8E1710);
STATICINIT2_REVERSED("StaticInit::8E1730", 0x008E1730, &staticinit::Fn8E1730);
STATICINIT2_REVERSED("StaticInit::8E1750", 0x008E1750, &staticinit::Fn8E1750);
STATICINIT2_REVERSED("StaticInit::8E1770", 0x008E1770, &staticinit::Fn8E1770);
STATICINIT2_REVERSED("StaticInit::8E1790", 0x008E1790, &staticinit::Fn8E1790);
STATICINIT2_REVERSED("StaticInit::8E17B0", 0x008E17B0, &staticinit::Fn8E17B0);
STATICINIT2_REVERSED("StaticInit::8E17D0", 0x008E17D0, &staticinit::Fn8E17D0);
STATICINIT2_REVERSED("StaticInit::8E17F0", 0x008E17F0, &staticinit::Fn8E17F0);
STATICINIT2_REVERSED("StaticInit::8E1810", 0x008E1810, &staticinit::Fn8E1810);
STATICINIT2_REVERSED("StaticInit::8E1830", 0x008E1830, &staticinit::Fn8E1830);
STATICINIT2_REVERSED("StaticInit::8E1850", 0x008E1850, &staticinit::Fn8E1850);
STATICINIT2_REVERSED("StaticInit::8E1870", 0x008E1870, &staticinit::Fn8E1870);
STATICINIT2_REVERSED("StaticInit::8E1890", 0x008E1890, &staticinit::Fn8E1890);
STATICINIT2_REVERSED("StaticInit::8E18B0", 0x008E18B0, &staticinit::Fn8E18B0);
STATICINIT2_REVERSED("StaticInit::8E18D0", 0x008E18D0, &staticinit::Fn8E18D0);
STATICINIT2_REVERSED("StaticInit::8E18F0", 0x008E18F0, &staticinit::Fn8E18F0);
STATICINIT2_REVERSED("StaticInit::8E1910", 0x008E1910, &staticinit::Fn8E1910);
STATICINIT2_REVERSED("StaticInit::8E1930", 0x008E1930, &staticinit::Fn8E1930);
STATICINIT2_REVERSED("StaticInit::8E1950", 0x008E1950, &staticinit::Fn8E1950);
STATICINIT2_REVERSED("StaticInit::8E1970", 0x008E1970, &staticinit::Fn8E1970);
STATICINIT2_REVERSED("StaticInit::8E1990", 0x008E1990, &staticinit::Fn8E1990);
STATICINIT2_REVERSED("StaticInit::8E19B0", 0x008E19B0, &staticinit::Fn8E19B0);
STATICINIT2_REVERSED("StaticInit::8E19D0", 0x008E19D0, &staticinit::Fn8E19D0);
STATICINIT2_REVERSED("StaticInit::8E19F0", 0x008E19F0, &staticinit::Fn8E19F0);
STATICINIT2_REVERSED("StaticInit::8E1A10", 0x008E1A10, &staticinit::Fn8E1A10);
STATICINIT2_REVERSED("StaticInit::8E1A30", 0x008E1A30, &staticinit::Fn8E1A30);
STATICINIT2_REVERSED("StaticInit::8E1A50", 0x008E1A50, &staticinit::Fn8E1A50);
STATICINIT2_REVERSED("StaticInit::8E1A70", 0x008E1A70, &staticinit::Fn8E1A70);
STATICINIT2_REVERSED("StaticInit::8E1A90", 0x008E1A90, &staticinit::Fn8E1A90);
STATICINIT2_REVERSED("StaticInit::8E1AB0", 0x008E1AB0, &staticinit::Fn8E1AB0);
STATICINIT2_REVERSED("StaticInit::8E1AD0", 0x008E1AD0, &staticinit::Fn8E1AD0);
STATICINIT2_REVERSED("StaticInit::8E1AF0", 0x008E1AF0, &staticinit::Fn8E1AF0);
STATICINIT2_REVERSED("StaticInit::8E1B10", 0x008E1B10, &staticinit::Fn8E1B10);
STATICINIT2_REVERSED("StaticInit::8E1B30", 0x008E1B30, &staticinit::Fn8E1B30);
STATICINIT2_REVERSED("StaticInit::8E1B50", 0x008E1B50, &staticinit::Fn8E1B50);
STATICINIT2_REVERSED("StaticInit::8E1B70", 0x008E1B70, &staticinit::Fn8E1B70);
STATICINIT2_REVERSED("StaticInit::8E1B90", 0x008E1B90, &staticinit::Fn8E1B90);
STATICINIT2_REVERSED("StaticInit::8E1BB0", 0x008E1BB0, &staticinit::Fn8E1BB0);
STATICINIT2_REVERSED("StaticInit::8E1BD0", 0x008E1BD0, &staticinit::Fn8E1BD0);
STATICINIT2_REVERSED("StaticInit::8E1BF0", 0x008E1BF0, &staticinit::Fn8E1BF0);
STATICINIT2_REVERSED("StaticInit::8E1C10", 0x008E1C10, &staticinit::Fn8E1C10);
STATICINIT2_REVERSED("StaticInit::8E1C30", 0x008E1C30, &staticinit::Fn8E1C30);
STATICINIT2_REVERSED("StaticInit::8E1C50", 0x008E1C50, &staticinit::Fn8E1C50);
STATICINIT2_REVERSED("StaticInit::8E1C70", 0x008E1C70, &staticinit::Fn8E1C70);
STATICINIT2_REVERSED("StaticInit::8E1C90", 0x008E1C90, &staticinit::Fn8E1C90);
STATICINIT2_REVERSED("StaticInit::8E1CB0", 0x008E1CB0, &staticinit::Fn8E1CB0);
STATICINIT2_REVERSED("StaticInit::8E1CD0", 0x008E1CD0, &staticinit::Fn8E1CD0);
STATICINIT2_REVERSED("StaticInit::8E1CF0", 0x008E1CF0, &staticinit::Fn8E1CF0);
STATICINIT2_REVERSED("StaticInit::8E1D10", 0x008E1D10, &staticinit::Fn8E1D10);
STATICINIT2_REVERSED("StaticInit::8E1D30", 0x008E1D30, &staticinit::Fn8E1D30);
STATICINIT2_REVERSED("StaticInit::8E1D50", 0x008E1D50, &staticinit::Fn8E1D50);
STATICINIT2_REVERSED("StaticInit::8E1D70", 0x008E1D70, &staticinit::Fn8E1D70);
STATICINIT2_REVERSED("StaticInit::8E1D90", 0x008E1D90, &staticinit::Fn8E1D90);
STATICINIT2_REVERSED("StaticInit::8E1DB0", 0x008E1DB0, &staticinit::Fn8E1DB0);
STATICINIT2_REVERSED("StaticInit::8E1DD0", 0x008E1DD0, &staticinit::Fn8E1DD0);
STATICINIT2_REVERSED("StaticInit::8E1DF0", 0x008E1DF0, &staticinit::Fn8E1DF0);
STATICINIT2_REVERSED("StaticInit::8E1E10", 0x008E1E10, &staticinit::Fn8E1E10);
STATICINIT2_REVERSED("StaticInit::8E1E30", 0x008E1E30, &staticinit::Fn8E1E30);
STATICINIT2_REVERSED("StaticInit::8E1E50", 0x008E1E50, &staticinit::Fn8E1E50);
STATICINIT2_REVERSED("StaticInit::8E1E70", 0x008E1E70, &staticinit::Fn8E1E70);
STATICINIT2_REVERSED("StaticInit::8E1E90", 0x008E1E90, &staticinit::Fn8E1E90);
STATICINIT2_REVERSED("StaticInit::8E1EB0", 0x008E1EB0, &staticinit::Fn8E1EB0);
STATICINIT2_REVERSED("StaticInit::8E1ED0", 0x008E1ED0, &staticinit::Fn8E1ED0);
STATICINIT2_REVERSED("StaticInit::8E1EF0", 0x008E1EF0, &staticinit::Fn8E1EF0);
STATICINIT2_REVERSED("StaticInit::8E1F10", 0x008E1F10, &staticinit::Fn8E1F10);
STATICINIT2_REVERSED("StaticInit::8E1F30", 0x008E1F30, &staticinit::Fn8E1F30);
STATICINIT2_REVERSED("StaticInit::8E1F50", 0x008E1F50, &staticinit::Fn8E1F50);
STATICINIT2_REVERSED("StaticInit::8E1F70", 0x008E1F70, &staticinit::Fn8E1F70);
STATICINIT2_REVERSED("StaticInit::8E1F90", 0x008E1F90, &staticinit::Fn8E1F90);
STATICINIT2_REVERSED("StaticInit::8E1FB0", 0x008E1FB0, &staticinit::Fn8E1FB0);
STATICINIT2_REVERSED("StaticInit::8E1FD0", 0x008E1FD0, &staticinit::Fn8E1FD0);
STATICINIT2_REVERSED("StaticInit::8E1FF0", 0x008E1FF0, &staticinit::Fn8E1FF0);
STATICINIT2_REVERSED("StaticInit::8E2010", 0x008E2010, &staticinit::Fn8E2010);
STATICINIT2_REVERSED("StaticInit::8E2030", 0x008E2030, &staticinit::Fn8E2030);
STATICINIT2_REVERSED("StaticInit::8E2050", 0x008E2050, &staticinit::Fn8E2050);
STATICINIT2_REVERSED("StaticInit::8E2070", 0x008E2070, &staticinit::Fn8E2070);
STATICINIT2_REVERSED("StaticInit::8E2090", 0x008E2090, &staticinit::Fn8E2090);
STATICINIT2_REVERSED("StaticInit::8E20B0", 0x008E20B0, &staticinit::Fn8E20B0);
STATICINIT2_REVERSED("StaticInit::8E20D0", 0x008E20D0, &staticinit::Fn8E20D0);
STATICINIT2_REVERSED("StaticInit::8E20F0", 0x008E20F0, &staticinit::Fn8E20F0);
STATICINIT2_REVERSED("StaticInit::8E2110", 0x008E2110, &staticinit::Fn8E2110);
STATICINIT2_REVERSED("StaticInit::8E2130", 0x008E2130, &staticinit::Fn8E2130);
STATICINIT2_REVERSED("StaticInit::8E2150", 0x008E2150, &staticinit::Fn8E2150);
STATICINIT2_REVERSED("StaticInit::8E2170", 0x008E2170, &staticinit::Fn8E2170);
STATICINIT2_REVERSED("StaticInit::8E2190", 0x008E2190, &staticinit::Fn8E2190);
STATICINIT2_REVERSED("StaticInit::8E21B0", 0x008E21B0, &staticinit::Fn8E21B0);
STATICINIT2_REVERSED("StaticInit::8E21D0", 0x008E21D0, &staticinit::Fn8E21D0);
STATICINIT2_REVERSED("StaticInit::8E21F0", 0x008E21F0, &staticinit::Fn8E21F0);
STATICINIT2_REVERSED("StaticInit::8E2210", 0x008E2210, &staticinit::Fn8E2210);
STATICINIT2_REVERSED("StaticInit::8E2230", 0x008E2230, &staticinit::Fn8E2230);
STATICINIT2_REVERSED("StaticInit::8E2250", 0x008E2250, &staticinit::Fn8E2250);
STATICINIT2_REVERSED("StaticInit::8E2270", 0x008E2270, &staticinit::Fn8E2270);
STATICINIT2_REVERSED("StaticInit::8E2290", 0x008E2290, &staticinit::Fn8E2290);
STATICINIT2_REVERSED("StaticInit::8E22B0", 0x008E22B0, &staticinit::Fn8E22B0);
STATICINIT2_REVERSED("StaticInit::8E22D0", 0x008E22D0, &staticinit::Fn8E22D0);
STATICINIT2_REVERSED("StaticInit::8E22F0", 0x008E22F0, &staticinit::Fn8E22F0);
STATICINIT2_REVERSED("StaticInit::8E2310", 0x008E2310, &staticinit::Fn8E2310);
STATICINIT2_REVERSED("StaticInit::8E2330", 0x008E2330, &staticinit::Fn8E2330);
STATICINIT2_REVERSED("StaticInit::8E2350", 0x008E2350, &staticinit::Fn8E2350);
STATICINIT2_REVERSED("StaticInit::8E2370", 0x008E2370, &staticinit::Fn8E2370);
STATICINIT2_REVERSED("StaticInit::8E2390", 0x008E2390, &staticinit::Fn8E2390);
STATICINIT2_REVERSED("StaticInit::8E23B0", 0x008E23B0, &staticinit::Fn8E23B0);
STATICINIT2_REVERSED("StaticInit::8E23D0", 0x008E23D0, &staticinit::Fn8E23D0);
STATICINIT2_REVERSED("StaticInit::8E23F0", 0x008E23F0, &staticinit::Fn8E23F0);
STATICINIT2_REVERSED("StaticInit::8E2410", 0x008E2410, &staticinit::Fn8E2410);
STATICINIT2_REVERSED("StaticInit::8E2430", 0x008E2430, &staticinit::Fn8E2430);
STATICINIT2_REVERSED("StaticInit::8E2450", 0x008E2450, &staticinit::Fn8E2450);
STATICINIT2_REVERSED("StaticInit::8E2470", 0x008E2470, &staticinit::Fn8E2470);
STATICINIT2_REVERSED("StaticInit::8E2490", 0x008E2490, &staticinit::Fn8E2490);
STATICINIT2_REVERSED("StaticInit::8E24B0", 0x008E24B0, &staticinit::Fn8E24B0);
STATICINIT2_REVERSED("StaticInit::8E24D0", 0x008E24D0, &staticinit::Fn8E24D0);
STATICINIT2_REVERSED("StaticInit::8E24F0", 0x008E24F0, &staticinit::Fn8E24F0);
STATICINIT2_REVERSED("StaticInit::8E2510", 0x008E2510, &staticinit::Fn8E2510);
STATICINIT2_REVERSED("StaticInit::8E2530", 0x008E2530, &staticinit::Fn8E2530);
STATICINIT2_REVERSED("StaticInit::8E2550", 0x008E2550, &staticinit::Fn8E2550);
STATICINIT2_REVERSED("StaticInit::8E2570", 0x008E2570, &staticinit::Fn8E2570);
STATICINIT2_REVERSED("StaticInit::8E2590", 0x008E2590, &staticinit::Fn8E2590);
STATICINIT2_REVERSED("StaticInit::8E25B0", 0x008E25B0, &staticinit::Fn8E25B0);
STATICINIT2_REVERSED("StaticInit::8E25D0", 0x008E25D0, &staticinit::Fn8E25D0);
STATICINIT2_REVERSED("StaticInit::8E25F0", 0x008E25F0, &staticinit::Fn8E25F0);
STATICINIT2_REVERSED("StaticInit::8E2610", 0x008E2610, &staticinit::Fn8E2610);
STATICINIT2_REVERSED("StaticInit::8E2630", 0x008E2630, &staticinit::Fn8E2630);
STATICINIT2_REVERSED("StaticInit::8E2650", 0x008E2650, &staticinit::Fn8E2650);
STATICINIT2_REVERSED("StaticInit::8E2670", 0x008E2670, &staticinit::Fn8E2670);
STATICINIT2_REVERSED("StaticInit::8E2690", 0x008E2690, &staticinit::Fn8E2690);
STATICINIT2_REVERSED("StaticInit::8E26B0", 0x008E26B0, &staticinit::Fn8E26B0);
STATICINIT2_REVERSED("StaticInit::8E26D0", 0x008E26D0, &staticinit::Fn8E26D0);
STATICINIT2_REVERSED("StaticInit::8E26F0", 0x008E26F0, &staticinit::Fn8E26F0);
STATICINIT2_REVERSED("StaticInit::8E2710", 0x008E2710, &staticinit::Fn8E2710);
STATICINIT2_REVERSED("StaticInit::8E2730", 0x008E2730, &staticinit::Fn8E2730);
STATICINIT2_REVERSED("StaticInit::8E2750", 0x008E2750, &staticinit::Fn8E2750);
STATICINIT2_REVERSED("StaticInit::8E2770", 0x008E2770, &staticinit::Fn8E2770);
STATICINIT2_REVERSED("StaticInit::8E2790", 0x008E2790, &staticinit::Fn8E2790);
STATICINIT2_REVERSED("StaticInit::8E27B0", 0x008E27B0, &staticinit::Fn8E27B0);
STATICINIT2_REVERSED("StaticInit::8E27D0", 0x008E27D0, &staticinit::Fn8E27D0);
STATICINIT2_REVERSED("StaticInit::8E27F0", 0x008E27F0, &staticinit::Fn8E27F0);
STATICINIT2_REVERSED("StaticInit::8E2810", 0x008E2810, &staticinit::Fn8E2810);
STATICINIT2_REVERSED("StaticInit::8E2830", 0x008E2830, &staticinit::Fn8E2830);
STATICINIT2_REVERSED("StaticInit::8E2850", 0x008E2850, &staticinit::Fn8E2850);
STATICINIT2_REVERSED("StaticInit::8E2870", 0x008E2870, &staticinit::Fn8E2870);
STATICINIT2_REVERSED("StaticInit::8E2890", 0x008E2890, &staticinit::Fn8E2890);
STATICINIT2_REVERSED("StaticInit::8E28B0", 0x008E28B0, &staticinit::Fn8E28B0);
STATICINIT2_REVERSED("StaticInit::8E28D0", 0x008E28D0, &staticinit::Fn8E28D0);
STATICINIT2_REVERSED("StaticInit::8E28F0", 0x008E28F0, &staticinit::Fn8E28F0);
STATICINIT2_REVERSED("StaticInit::8E2910", 0x008E2910, &staticinit::Fn8E2910);
STATICINIT2_REVERSED("StaticInit::8E2930", 0x008E2930, &staticinit::Fn8E2930);
STATICINIT2_REVERSED("StaticInit::8E2950", 0x008E2950, &staticinit::Fn8E2950);
STATICINIT2_REVERSED("StaticInit::8E2970", 0x008E2970, &staticinit::Fn8E2970);
STATICINIT2_REVERSED("StaticInit::8E2990", 0x008E2990, &staticinit::Fn8E2990);
STATICINIT2_REVERSED("StaticInit::8E29B0", 0x008E29B0, &staticinit::Fn8E29B0);
STATICINIT2_REVERSED("StaticInit::8E29D0", 0x008E29D0, &staticinit::Fn8E29D0);
STATICINIT2_REVERSED("StaticInit::8E29F0", 0x008E29F0, &staticinit::Fn8E29F0);
STATICINIT2_REVERSED("StaticInit::8E2A10", 0x008E2A10, &staticinit::Fn8E2A10);
STATICINIT2_REVERSED("StaticInit::8E2A30", 0x008E2A30, &staticinit::Fn8E2A30);
STATICINIT2_REVERSED("StaticInit::8E2A50", 0x008E2A50, &staticinit::Fn8E2A50);
STATICINIT2_REVERSED("StaticInit::8E2A70", 0x008E2A70, &staticinit::Fn8E2A70);
STATICINIT2_REVERSED("StaticInit::8E2A90", 0x008E2A90, &staticinit::Fn8E2A90);
STATICINIT2_REVERSED("StaticInit::8E2AB0", 0x008E2AB0, &staticinit::Fn8E2AB0);
STATICINIT2_REVERSED("StaticInit::8E2AD0", 0x008E2AD0, &staticinit::Fn8E2AD0);
STATICINIT2_REVERSED("StaticInit::8E2AF0", 0x008E2AF0, &staticinit::Fn8E2AF0);
STATICINIT2_REVERSED("StaticInit::8E2B10", 0x008E2B10, &staticinit::Fn8E2B10);
STATICINIT2_REVERSED("StaticInit::8E2B30", 0x008E2B30, &staticinit::Fn8E2B30);
STATICINIT2_REVERSED("StaticInit::8E2B50", 0x008E2B50, &staticinit::Fn8E2B50);
STATICINIT2_REVERSED("StaticInit::8E2B70", 0x008E2B70, &staticinit::Fn8E2B70);
STATICINIT2_REVERSED("StaticInit::8E2B90", 0x008E2B90, &staticinit::Fn8E2B90);
STATICINIT2_REVERSED("StaticInit::8E2BB0", 0x008E2BB0, &staticinit::Fn8E2BB0);
STATICINIT2_REVERSED("StaticInit::8E2BD0", 0x008E2BD0, &staticinit::Fn8E2BD0);
STATICINIT2_REVERSED("StaticInit::8E2BF0", 0x008E2BF0, &staticinit::Fn8E2BF0);
STATICINIT2_REVERSED("StaticInit::8E2C10", 0x008E2C10, &staticinit::Fn8E2C10);
STATICINIT2_REVERSED("StaticInit::8E2C30", 0x008E2C30, &staticinit::Fn8E2C30);
STATICINIT2_REVERSED("StaticInit::8E2C50", 0x008E2C50, &staticinit::Fn8E2C50);
STATICINIT2_REVERSED("StaticInit::8E2C70", 0x008E2C70, &staticinit::Fn8E2C70);
STATICINIT2_REVERSED("StaticInit::8E2C90", 0x008E2C90, &staticinit::Fn8E2C90);
STATICINIT2_REVERSED("StaticInit::8E2CB0", 0x008E2CB0, &staticinit::Fn8E2CB0);
STATICINIT2_REVERSED("StaticInit::8E2CD0", 0x008E2CD0, &staticinit::Fn8E2CD0);
STATICINIT2_REVERSED("StaticInit::8E2CF0", 0x008E2CF0, &staticinit::Fn8E2CF0);
STATICINIT2_REVERSED("StaticInit::8E2D10", 0x008E2D10, &staticinit::Fn8E2D10);
STATICINIT2_REVERSED("StaticInit::8E2D30", 0x008E2D30, &staticinit::Fn8E2D30);
STATICINIT2_REVERSED("StaticInit::8E2D50", 0x008E2D50, &staticinit::Fn8E2D50);
STATICINIT2_REVERSED("StaticInit::8E2D70", 0x008E2D70, &staticinit::Fn8E2D70);
STATICINIT2_REVERSED("StaticInit::8E2D90", 0x008E2D90, &staticinit::Fn8E2D90);
STATICINIT2_REVERSED("StaticInit::8E2DB0", 0x008E2DB0, &staticinit::Fn8E2DB0);
STATICINIT2_REVERSED("StaticInit::8E2DD0", 0x008E2DD0, &staticinit::Fn8E2DD0);
STATICINIT2_REVERSED("StaticInit::8E2DF0", 0x008E2DF0, &staticinit::Fn8E2DF0);
STATICINIT2_REVERSED("StaticInit::8E2E10", 0x008E2E10, &staticinit::Fn8E2E10);
STATICINIT2_REVERSED("StaticInit::8E2E30", 0x008E2E30, &staticinit::Fn8E2E30);
STATICINIT2_REVERSED("StaticInit::8E2E50", 0x008E2E50, &staticinit::Fn8E2E50);
STATICINIT2_REVERSED("StaticInit::8E2E70", 0x008E2E70, &staticinit::Fn8E2E70);
STATICINIT2_REVERSED("StaticInit::8E2E90", 0x008E2E90, &staticinit::Fn8E2E90);
STATICINIT2_REVERSED("StaticInit::8E2EB0", 0x008E2EB0, &staticinit::Fn8E2EB0);
STATICINIT2_REVERSED("StaticInit::8E2ED0", 0x008E2ED0, &staticinit::Fn8E2ED0);
STATICINIT2_REVERSED("StaticInit::8E2EF0", 0x008E2EF0, &staticinit::Fn8E2EF0);
STATICINIT2_REVERSED("StaticInit::8E2F10", 0x008E2F10, &staticinit::Fn8E2F10);
STATICINIT2_REVERSED("StaticInit::8E2F30", 0x008E2F30, &staticinit::Fn8E2F30);
STATICINIT2_REVERSED("StaticInit::8E2F50", 0x008E2F50, &staticinit::Fn8E2F50);
STATICINIT2_REVERSED("StaticInit::8E2F70", 0x008E2F70, &staticinit::Fn8E2F70);
STATICINIT2_REVERSED("StaticInit::8E2F90", 0x008E2F90, &staticinit::Fn8E2F90);
STATICINIT2_REVERSED("StaticInit::8E2FB0", 0x008E2FB0, &staticinit::Fn8E2FB0);
STATICINIT2_REVERSED("StaticInit::8E2FD0", 0x008E2FD0, &staticinit::Fn8E2FD0);
STATICINIT2_REVERSED("StaticInit::8E2FF0", 0x008E2FF0, &staticinit::Fn8E2FF0);
STATICINIT2_REVERSED("StaticInit::8E3010", 0x008E3010, &staticinit::Fn8E3010);
STATICINIT2_REVERSED("StaticInit::8E3030", 0x008E3030, &staticinit::Fn8E3030);
STATICINIT2_REVERSED("StaticInit::8E3050", 0x008E3050, &staticinit::Fn8E3050);
STATICINIT2_REVERSED("StaticInit::8E3070", 0x008E3070, &staticinit::Fn8E3070);
STATICINIT2_REVERSED("StaticInit::8E3090", 0x008E3090, &staticinit::Fn8E3090);
STATICINIT2_REVERSED("StaticInit::8E30B0", 0x008E30B0, &staticinit::Fn8E30B0);
STATICINIT2_REVERSED("StaticInit::8E30D0", 0x008E30D0, &staticinit::Fn8E30D0);
STATICINIT2_REVERSED("StaticInit::8E30F0", 0x008E30F0, &staticinit::Fn8E30F0);
STATICINIT2_REVERSED("StaticInit::8E3110", 0x008E3110, &staticinit::Fn8E3110);
STATICINIT2_REVERSED("StaticInit::8E3130", 0x008E3130, &staticinit::Fn8E3130);
STATICINIT2_REVERSED("StaticInit::8E3150", 0x008E3150, &staticinit::Fn8E3150);
STATICINIT2_REVERSED("StaticInit::8E3170", 0x008E3170, &staticinit::Fn8E3170);
STATICINIT2_REVERSED("StaticInit::8E3190", 0x008E3190, &staticinit::Fn8E3190);
STATICINIT2_REVERSED("StaticInit::8E31B0", 0x008E31B0, &staticinit::Fn8E31B0);
STATICINIT2_REVERSED("StaticInit::8E31D0", 0x008E31D0, &staticinit::Fn8E31D0);
STATICINIT2_REVERSED("StaticInit::8E31F0", 0x008E31F0, &staticinit::Fn8E31F0);
STATICINIT2_REVERSED("StaticInit::8E3210", 0x008E3210, &staticinit::Fn8E3210);
STATICINIT2_REVERSED("StaticInit::8E3230", 0x008E3230, &staticinit::Fn8E3230);
STATICINIT2_REVERSED("StaticInit::8E3250", 0x008E3250, &staticinit::Fn8E3250);
STATICINIT2_REVERSED("StaticInit::8E3270", 0x008E3270, &staticinit::Fn8E3270);
STATICINIT2_REVERSED("StaticInit::8E3290", 0x008E3290, &staticinit::Fn8E3290);
STATICINIT2_REVERSED("StaticInit::8E32B0", 0x008E32B0, &staticinit::Fn8E32B0);
STATICINIT2_REVERSED("StaticInit::8E32D0", 0x008E32D0, &staticinit::Fn8E32D0);
STATICINIT2_REVERSED("StaticInit::8E32F0", 0x008E32F0, &staticinit::Fn8E32F0);
STATICINIT2_REVERSED("StaticInit::8E3310", 0x008E3310, &staticinit::Fn8E3310);
STATICINIT2_REVERSED("StaticInit::8E3330", 0x008E3330, &staticinit::Fn8E3330);
STATICINIT2_REVERSED("StaticInit::8E3350", 0x008E3350, &staticinit::Fn8E3350);
STATICINIT2_REVERSED("StaticInit::8E3370", 0x008E3370, &staticinit::Fn8E3370);
STATICINIT2_REVERSED("StaticInit::8E3390", 0x008E3390, &staticinit::Fn8E3390);
STATICINIT2_REVERSED("StaticInit::8E33B0", 0x008E33B0, &staticinit::Fn8E33B0);
STATICINIT2_REVERSED("StaticInit::8E33D0", 0x008E33D0, &staticinit::Fn8E33D0);
STATICINIT2_REVERSED("StaticInit::8E33F0", 0x008E33F0, &staticinit::Fn8E33F0);
STATICINIT2_REVERSED("StaticInit::8E3410", 0x008E3410, &staticinit::Fn8E3410);
STATICINIT2_REVERSED("StaticInit::8E3430", 0x008E3430, &staticinit::Fn8E3430);
STATICINIT2_REVERSED("StaticInit::8E3450", 0x008E3450, &staticinit::Fn8E3450);
STATICINIT2_REVERSED("StaticInit::8E3470", 0x008E3470, &staticinit::Fn8E3470);
STATICINIT2_REVERSED("StaticInit::8E3490", 0x008E3490, &staticinit::Fn8E3490);
STATICINIT2_REVERSED("StaticInit::8E34B0", 0x008E34B0, &staticinit::Fn8E34B0);
STATICINIT2_REVERSED("StaticInit::8E34D0", 0x008E34D0, &staticinit::Fn8E34D0);
STATICINIT2_REVERSED("StaticInit::8E34F0", 0x008E34F0, &staticinit::Fn8E34F0);
STATICINIT2_REVERSED("StaticInit::8E3510", 0x008E3510, &staticinit::Fn8E3510);
STATICINIT2_REVERSED("StaticInit::8E3530", 0x008E3530, &staticinit::Fn8E3530);
STATICINIT2_REVERSED("StaticInit::8E3550", 0x008E3550, &staticinit::Fn8E3550);
STATICINIT2_REVERSED("StaticInit::8E3570", 0x008E3570, &staticinit::Fn8E3570);
STATICINIT2_REVERSED("StaticInit::8E3590", 0x008E3590, &staticinit::Fn8E3590);
STATICINIT2_REVERSED("StaticInit::8E35B0", 0x008E35B0, &staticinit::Fn8E35B0);
STATICINIT2_REVERSED("StaticInit::8E35D0", 0x008E35D0, &staticinit::Fn8E35D0);
STATICINIT2_REVERSED("StaticInit::8E35F0", 0x008E35F0, &staticinit::Fn8E35F0);
STATICINIT2_REVERSED("StaticInit::8E3610", 0x008E3610, &staticinit::Fn8E3610);
STATICINIT2_REVERSED("StaticInit::8E3630", 0x008E3630, &staticinit::Fn8E3630);
STATICINIT2_REVERSED("StaticInit::8E3650", 0x008E3650, &staticinit::Fn8E3650);
STATICINIT2_REVERSED("StaticInit::8E3670", 0x008E3670, &staticinit::Fn8E3670);
STATICINIT2_REVERSED("StaticInit::8E3690", 0x008E3690, &staticinit::Fn8E3690);
STATICINIT2_REVERSED("StaticInit::8E36B0", 0x008E36B0, &staticinit::Fn8E36B0);
STATICINIT2_REVERSED("StaticInit::8E36D0", 0x008E36D0, &staticinit::Fn8E36D0);
STATICINIT2_REVERSED("StaticInit::8E36F0", 0x008E36F0, &staticinit::Fn8E36F0);
STATICINIT2_REVERSED("StaticInit::8E3710", 0x008E3710, &staticinit::Fn8E3710);
STATICINIT2_REVERSED("StaticInit::8E3730", 0x008E3730, &staticinit::Fn8E3730);
STATICINIT2_REVERSED("StaticInit::8E3750", 0x008E3750, &staticinit::Fn8E3750);
STATICINIT2_REVERSED("StaticInit::8E3770", 0x008E3770, &staticinit::Fn8E3770);
STATICINIT2_REVERSED("StaticInit::8E3790", 0x008E3790, &staticinit::Fn8E3790);
STATICINIT2_REVERSED("StaticInit::8E37B0", 0x008E37B0, &staticinit::Fn8E37B0);
STATICINIT2_REVERSED("StaticInit::8E37D0", 0x008E37D0, &staticinit::Fn8E37D0);
STATICINIT2_REVERSED("StaticInit::8E37F0", 0x008E37F0, &staticinit::Fn8E37F0);
STATICINIT2_REVERSED("StaticInit::8E3810", 0x008E3810, &staticinit::Fn8E3810);
STATICINIT2_REVERSED("StaticInit::8E3830", 0x008E3830, &staticinit::Fn8E3830);
STATICINIT2_REVERSED("StaticInit::8E3850", 0x008E3850, &staticinit::Fn8E3850);
STATICINIT2_REVERSED("StaticInit::8E3870", 0x008E3870, &staticinit::Fn8E3870);
STATICINIT2_REVERSED("StaticInit::8E3890", 0x008E3890, &staticinit::Fn8E3890);
STATICINIT2_REVERSED("StaticInit::8E38B0", 0x008E38B0, &staticinit::Fn8E38B0);
STATICINIT2_REVERSED("StaticInit::8E38D0", 0x008E38D0, &staticinit::Fn8E38D0);
STATICINIT2_REVERSED("StaticInit::8E38F0", 0x008E38F0, &staticinit::Fn8E38F0);
STATICINIT2_REVERSED("StaticInit::8E3910", 0x008E3910, &staticinit::Fn8E3910);
STATICINIT2_REVERSED("StaticInit::8E3930", 0x008E3930, &staticinit::Fn8E3930);
STATICINIT2_REVERSED("StaticInit::8E3950", 0x008E3950, &staticinit::Fn8E3950);
STATICINIT2_REVERSED("StaticInit::8E3970", 0x008E3970, &staticinit::Fn8E3970);
STATICINIT2_REVERSED("StaticInit::8E3990", 0x008E3990, &staticinit::Fn8E3990);
STATICINIT2_REVERSED("StaticInit::8E39B0", 0x008E39B0, &staticinit::Fn8E39B0);
STATICINIT2_REVERSED("StaticInit::8E39D0", 0x008E39D0, &staticinit::Fn8E39D0);
STATICINIT2_REVERSED("StaticInit::8E39F0", 0x008E39F0, &staticinit::Fn8E39F0);
STATICINIT2_REVERSED("StaticInit::8E3A10", 0x008E3A10, &staticinit::Fn8E3A10);
STATICINIT2_REVERSED("StaticInit::8E3A30", 0x008E3A30, &staticinit::Fn8E3A30);
STATICINIT2_REVERSED("StaticInit::8E3A50", 0x008E3A50, &staticinit::Fn8E3A50);
STATICINIT2_REVERSED("StaticInit::8E3A70", 0x008E3A70, &staticinit::Fn8E3A70);
STATICINIT2_REVERSED("StaticInit::8E3A90", 0x008E3A90, &staticinit::Fn8E3A90);
STATICINIT2_REVERSED("StaticInit::8E3AB0", 0x008E3AB0, &staticinit::Fn8E3AB0);
STATICINIT2_REVERSED("StaticInit::8E3AD0", 0x008E3AD0, &staticinit::Fn8E3AD0);
STATICINIT2_REVERSED("StaticInit::8E3AF0", 0x008E3AF0, &staticinit::Fn8E3AF0);
STATICINIT2_REVERSED("StaticInit::8E3B10", 0x008E3B10, &staticinit::Fn8E3B10);
STATICINIT2_REVERSED("StaticInit::8E3B30", 0x008E3B30, &staticinit::Fn8E3B30);
STATICINIT2_REVERSED("StaticInit::8E3B50", 0x008E3B50, &staticinit::Fn8E3B50);
STATICINIT2_REVERSED("StaticInit::8E3B70", 0x008E3B70, &staticinit::Fn8E3B70);
STATICINIT2_REVERSED("StaticInit::8E3B90", 0x008E3B90, &staticinit::Fn8E3B90);
STATICINIT2_REVERSED("StaticInit::8E3BB0", 0x008E3BB0, &staticinit::Fn8E3BB0);
STATICINIT2_REVERSED("StaticInit::8E3BD0", 0x008E3BD0, &staticinit::Fn8E3BD0);
STATICINIT2_REVERSED("StaticInit::8E3BF0", 0x008E3BF0, &staticinit::Fn8E3BF0);
STATICINIT2_REVERSED("StaticInit::8E3C10", 0x008E3C10, &staticinit::Fn8E3C10);
STATICINIT2_REVERSED("StaticInit::8E3C30", 0x008E3C30, &staticinit::Fn8E3C30);
STATICINIT2_REVERSED("StaticInit::8E3C50", 0x008E3C50, &staticinit::Fn8E3C50);
STATICINIT2_REVERSED("StaticInit::8E3C70", 0x008E3C70, &staticinit::Fn8E3C70);
STATICINIT2_REVERSED("StaticInit::8E3C90", 0x008E3C90, &staticinit::Fn8E3C90);
STATICINIT2_REVERSED("StaticInit::8E3CB0", 0x008E3CB0, &staticinit::Fn8E3CB0);
STATICINIT2_REVERSED("StaticInit::8E3CD0", 0x008E3CD0, &staticinit::Fn8E3CD0);
STATICINIT2_REVERSED("StaticInit::8E3CF0", 0x008E3CF0, &staticinit::Fn8E3CF0);
STATICINIT2_REVERSED("StaticInit::8E3D10", 0x008E3D10, &staticinit::Fn8E3D10);
STATICINIT2_REVERSED("StaticInit::8E3D30", 0x008E3D30, &staticinit::Fn8E3D30);
STATICINIT2_REVERSED("StaticInit::8E3D50", 0x008E3D50, &staticinit::Fn8E3D50);
STATICINIT2_REVERSED("StaticInit::8E3D70", 0x008E3D70, &staticinit::Fn8E3D70);
STATICINIT2_REVERSED("StaticInit::8E3D90", 0x008E3D90, &staticinit::Fn8E3D90);
STATICINIT2_REVERSED("StaticInit::8E3DB0", 0x008E3DB0, &staticinit::Fn8E3DB0);
STATICINIT2_REVERSED("StaticInit::8E3DD0", 0x008E3DD0, &staticinit::Fn8E3DD0);
STATICINIT2_REVERSED("StaticInit::8E3DF0", 0x008E3DF0, &staticinit::Fn8E3DF0);
STATICINIT2_REVERSED("StaticInit::8E3E10", 0x008E3E10, &staticinit::Fn8E3E10);
STATICINIT2_REVERSED("StaticInit::8E3E30", 0x008E3E30, &staticinit::Fn8E3E30);
STATICINIT2_REVERSED("StaticInit::8E3E50", 0x008E3E50, &staticinit::Fn8E3E50);
STATICINIT2_REVERSED("StaticInit::8E3E70", 0x008E3E70, &staticinit::Fn8E3E70);
STATICINIT2_REVERSED("StaticInit::8E3E90", 0x008E3E90, &staticinit::Fn8E3E90);
STATICINIT2_REVERSED("StaticInit::8E3EB0", 0x008E3EB0, &staticinit::Fn8E3EB0);
STATICINIT2_REVERSED("StaticInit::8E3ED0", 0x008E3ED0, &staticinit::Fn8E3ED0);
STATICINIT2_REVERSED("StaticInit::8E3EF0", 0x008E3EF0, &staticinit::Fn8E3EF0);
STATICINIT2_REVERSED("StaticInit::8E3F10", 0x008E3F10, &staticinit::Fn8E3F10);
STATICINIT2_REVERSED("StaticInit::8E3F30", 0x008E3F30, &staticinit::Fn8E3F30);
STATICINIT2_REVERSED("StaticInit::8E3F50", 0x008E3F50, &staticinit::Fn8E3F50);
STATICINIT2_REVERSED("StaticInit::8E3F70", 0x008E3F70, &staticinit::Fn8E3F70);
STATICINIT2_REVERSED("StaticInit::8E3F90", 0x008E3F90, &staticinit::Fn8E3F90);
STATICINIT2_REVERSED("StaticInit::8E3FB0", 0x008E3FB0, &staticinit::Fn8E3FB0);
STATICINIT2_REVERSED("StaticInit::8E3FD0", 0x008E3FD0, &staticinit::Fn8E3FD0);
STATICINIT2_REVERSED("StaticInit::8E3FF0", 0x008E3FF0, &staticinit::Fn8E3FF0);
STATICINIT2_REVERSED("StaticInit::8E4010", 0x008E4010, &staticinit::Fn8E4010);
STATICINIT2_REVERSED("StaticInit::8E4030", 0x008E4030, &staticinit::Fn8E4030);
STATICINIT2_REVERSED("StaticInit::8E4050", 0x008E4050, &staticinit::Fn8E4050);
STATICINIT2_REVERSED("StaticInit::8E4070", 0x008E4070, &staticinit::Fn8E4070);
STATICINIT2_REVERSED("StaticInit::8E4090", 0x008E4090, &staticinit::Fn8E4090);
STATICINIT2_REVERSED("StaticInit::8E40B0", 0x008E40B0, &staticinit::Fn8E40B0);
STATICINIT2_REVERSED("StaticInit::8E40D0", 0x008E40D0, &staticinit::Fn8E40D0);
STATICINIT2_REVERSED("StaticInit::8E40F0", 0x008E40F0, &staticinit::Fn8E40F0);
STATICINIT2_REVERSED("StaticInit::8E4110", 0x008E4110, &staticinit::Fn8E4110);
STATICINIT2_REVERSED("StaticInit::8E4130", 0x008E4130, &staticinit::Fn8E4130);
STATICINIT2_REVERSED("StaticInit::8E4150", 0x008E4150, &staticinit::Fn8E4150);
STATICINIT2_REVERSED("StaticInit::8E4170", 0x008E4170, &staticinit::Fn8E4170);
STATICINIT2_REVERSED("StaticInit::8E4190", 0x008E4190, &staticinit::Fn8E4190);
STATICINIT2_REVERSED("StaticInit::8E41B0", 0x008E41B0, &staticinit::Fn8E41B0);
STATICINIT2_REVERSED("StaticInit::8E41D0", 0x008E41D0, &staticinit::Fn8E41D0);
STATICINIT2_REVERSED("StaticInit::8E41F0", 0x008E41F0, &staticinit::Fn8E41F0);
STATICINIT2_REVERSED("StaticInit::8E4210", 0x008E4210, &staticinit::Fn8E4210);
STATICINIT2_REVERSED("StaticInit::8E4230", 0x008E4230, &staticinit::Fn8E4230);
STATICINIT2_REVERSED("StaticInit::8E4250", 0x008E4250, &staticinit::Fn8E4250);
STATICINIT2_REVERSED("StaticInit::8E4270", 0x008E4270, &staticinit::Fn8E4270);
STATICINIT2_REVERSED("StaticInit::8E4290", 0x008E4290, &staticinit::Fn8E4290);
STATICINIT2_REVERSED("StaticInit::8E42B0", 0x008E42B0, &staticinit::Fn8E42B0);
STATICINIT2_REVERSED("StaticInit::8E42D0", 0x008E42D0, &staticinit::Fn8E42D0);
STATICINIT2_REVERSED("StaticInit::8E42F0", 0x008E42F0, &staticinit::Fn8E42F0);
STATICINIT2_REVERSED("StaticInit::8E4310", 0x008E4310, &staticinit::Fn8E4310);
STATICINIT2_REVERSED("StaticInit::8E4330", 0x008E4330, &staticinit::Fn8E4330);
STATICINIT2_REVERSED("StaticInit::8E4350", 0x008E4350, &staticinit::Fn8E4350);
STATICINIT2_REVERSED("StaticInit::8E4370", 0x008E4370, &staticinit::Fn8E4370);
STATICINIT2_REVERSED("StaticInit::8E4390", 0x008E4390, &staticinit::Fn8E4390);
STATICINIT2_REVERSED("StaticInit::8E43B0", 0x008E43B0, &staticinit::Fn8E43B0);
STATICINIT2_REVERSED("StaticInit::8E43D0", 0x008E43D0, &staticinit::Fn8E43D0);
STATICINIT2_REVERSED("StaticInit::8E43F0", 0x008E43F0, &staticinit::Fn8E43F0);
STATICINIT2_REVERSED("StaticInit::8E4410", 0x008E4410, &staticinit::Fn8E4410);
STATICINIT2_REVERSED("StaticInit::8E4430", 0x008E4430, &staticinit::Fn8E4430);
STATICINIT2_REVERSED("StaticInit::8E4450", 0x008E4450, &staticinit::Fn8E4450);
STATICINIT2_REVERSED("StaticInit::8E4470", 0x008E4470, &staticinit::Fn8E4470);
STATICINIT2_REVERSED("StaticInit::8E4490", 0x008E4490, &staticinit::Fn8E4490);
STATICINIT2_REVERSED("StaticInit::8E44B0", 0x008E44B0, &staticinit::Fn8E44B0);
STATICINIT2_REVERSED("StaticInit::8E44D0", 0x008E44D0, &staticinit::Fn8E44D0);
STATICINIT2_REVERSED("StaticInit::8E44F0", 0x008E44F0, &staticinit::Fn8E44F0);
STATICINIT2_REVERSED("StaticInit::8E4510", 0x008E4510, &staticinit::Fn8E4510);
STATICINIT2_REVERSED("StaticInit::8E4530", 0x008E4530, &staticinit::Fn8E4530);
STATICINIT2_REVERSED("StaticInit::8E4550", 0x008E4550, &staticinit::Fn8E4550);
STATICINIT2_REVERSED("StaticInit::8E4570", 0x008E4570, &staticinit::Fn8E4570);
STATICINIT2_REVERSED("StaticInit::8E4590", 0x008E4590, &staticinit::Fn8E4590);
STATICINIT2_REVERSED("StaticInit::8E45B0", 0x008E45B0, &staticinit::Fn8E45B0);
STATICINIT2_REVERSED("StaticInit::8E45D0", 0x008E45D0, &staticinit::Fn8E45D0);
STATICINIT2_REVERSED("StaticInit::8E45F0", 0x008E45F0, &staticinit::Fn8E45F0);
STATICINIT2_REVERSED("StaticInit::8E4610", 0x008E4610, &staticinit::Fn8E4610);
STATICINIT2_REVERSED("StaticInit::8E4630", 0x008E4630, &staticinit::Fn8E4630);
STATICINIT2_REVERSED("StaticInit::8E4650", 0x008E4650, &staticinit::Fn8E4650);
STATICINIT2_REVERSED("StaticInit::8E4670", 0x008E4670, &staticinit::Fn8E4670);
STATICINIT2_REVERSED("StaticInit::8E4690", 0x008E4690, &staticinit::Fn8E4690);
STATICINIT2_REVERSED("StaticInit::8E46B0", 0x008E46B0, &staticinit::Fn8E46B0);
STATICINIT2_REVERSED("StaticInit::8E46D0", 0x008E46D0, &staticinit::Fn8E46D0);
STATICINIT2_REVERSED("StaticInit::8E46F0", 0x008E46F0, &staticinit::Fn8E46F0);
STATICINIT2_REVERSED("StaticInit::8E4710", 0x008E4710, &staticinit::Fn8E4710);
STATICINIT2_REVERSED("StaticInit::8E4730", 0x008E4730, &staticinit::Fn8E4730);
STATICINIT2_REVERSED("StaticInit::8E4750", 0x008E4750, &staticinit::Fn8E4750);
STATICINIT2_REVERSED("StaticInit::8E4770", 0x008E4770, &staticinit::Fn8E4770);
STATICINIT2_REVERSED("StaticInit::8E4790", 0x008E4790, &staticinit::Fn8E4790);
STATICINIT2_REVERSED("StaticInit::8E47B0", 0x008E47B0, &staticinit::Fn8E47B0);
STATICINIT2_REVERSED("StaticInit::8E47D0", 0x008E47D0, &staticinit::Fn8E47D0);
STATICINIT2_REVERSED("StaticInit::8E47F0", 0x008E47F0, &staticinit::Fn8E47F0);
STATICINIT2_REVERSED("StaticInit::8E4810", 0x008E4810, &staticinit::Fn8E4810);
STATICINIT2_REVERSED("StaticInit::8E4830", 0x008E4830, &staticinit::Fn8E4830);
STATICINIT2_REVERSED("StaticInit::8E4850", 0x008E4850, &staticinit::Fn8E4850);
STATICINIT2_REVERSED("StaticInit::8E4870", 0x008E4870, &staticinit::Fn8E4870);
STATICINIT2_REVERSED("StaticInit::8E4890", 0x008E4890, &staticinit::Fn8E4890);
STATICINIT2_REVERSED("StaticInit::8E48B0", 0x008E48B0, &staticinit::Fn8E48B0);
STATICINIT2_REVERSED("StaticInit::8E48D0", 0x008E48D0, &staticinit::Fn8E48D0);
STATICINIT2_REVERSED("StaticInit::8E48F0", 0x008E48F0, &staticinit::Fn8E48F0);
STATICINIT2_REVERSED("StaticInit::8E4910", 0x008E4910, &staticinit::Fn8E4910);
STATICINIT2_REVERSED("StaticInit::8E4930", 0x008E4930, &staticinit::Fn8E4930);
STATICINIT2_REVERSED("StaticInit::8E4950", 0x008E4950, &staticinit::Fn8E4950);
STATICINIT2_REVERSED("StaticInit::8E4970", 0x008E4970, &staticinit::Fn8E4970);
STATICINIT2_REVERSED("StaticInit::8E4990", 0x008E4990, &staticinit::Fn8E4990);
STATICINIT2_REVERSED("StaticInit::8E49B0", 0x008E49B0, &staticinit::Fn8E49B0);
STATICINIT2_REVERSED("StaticInit::8E49D0", 0x008E49D0, &staticinit::Fn8E49D0);
STATICINIT2_REVERSED("StaticInit::8E49F0", 0x008E49F0, &staticinit::Fn8E49F0);
STATICINIT2_REVERSED("StaticInit::8E4A10", 0x008E4A10, &staticinit::Fn8E4A10);
STATICINIT2_REVERSED("StaticInit::8E4A30", 0x008E4A30, &staticinit::Fn8E4A30);
STATICINIT2_REVERSED("StaticInit::8E4A50", 0x008E4A50, &staticinit::Fn8E4A50);
STATICINIT2_REVERSED("StaticInit::8E4A70", 0x008E4A70, &staticinit::Fn8E4A70);
STATICINIT2_REVERSED("StaticInit::8E4A90", 0x008E4A90, &staticinit::Fn8E4A90);
STATICINIT2_REVERSED("StaticInit::8E4AB0", 0x008E4AB0, &staticinit::Fn8E4AB0);
STATICINIT2_REVERSED("StaticInit::8E4AD0", 0x008E4AD0, &staticinit::Fn8E4AD0);
STATICINIT2_REVERSED("StaticInit::8E4AF0", 0x008E4AF0, &staticinit::Fn8E4AF0);
STATICINIT2_REVERSED("StaticInit::8E4B10", 0x008E4B10, &staticinit::Fn8E4B10);
STATICINIT2_REVERSED("StaticInit::8E4B30", 0x008E4B30, &staticinit::Fn8E4B30);
STATICINIT2_REVERSED("StaticInit::8E4B50", 0x008E4B50, &staticinit::Fn8E4B50);
STATICINIT2_REVERSED("StaticInit::8E4B70", 0x008E4B70, &staticinit::Fn8E4B70);
STATICINIT2_REVERSED("StaticInit::8E4B90", 0x008E4B90, &staticinit::Fn8E4B90);
STATICINIT2_REVERSED("StaticInit::8E4BB0", 0x008E4BB0, &staticinit::Fn8E4BB0);
STATICINIT2_REVERSED("StaticInit::8E4BD0", 0x008E4BD0, &staticinit::Fn8E4BD0);
STATICINIT2_REVERSED("StaticInit::8E4BF0", 0x008E4BF0, &staticinit::Fn8E4BF0);
STATICINIT2_REVERSED("StaticInit::8E4C10", 0x008E4C10, &staticinit::Fn8E4C10);
STATICINIT2_REVERSED("StaticInit::8E4C30", 0x008E4C30, &staticinit::Fn8E4C30);
STATICINIT2_REVERSED("StaticInit::8E4C50", 0x008E4C50, &staticinit::Fn8E4C50);
STATICINIT2_REVERSED("StaticInit::8E4C70", 0x008E4C70, &staticinit::Fn8E4C70);
STATICINIT2_REVERSED("StaticInit::8E4C90", 0x008E4C90, &staticinit::Fn8E4C90);
STATICINIT2_REVERSED("StaticInit::8E4CB0", 0x008E4CB0, &staticinit::Fn8E4CB0);
STATICINIT2_REVERSED("StaticInit::8E4CD0", 0x008E4CD0, &staticinit::Fn8E4CD0);
STATICINIT2_REVERSED("StaticInit::8E4CF0", 0x008E4CF0, &staticinit::Fn8E4CF0);
STATICINIT2_REVERSED("StaticInit::8E4D10", 0x008E4D10, &staticinit::Fn8E4D10);
STATICINIT2_REVERSED("StaticInit::8E4D30", 0x008E4D30, &staticinit::Fn8E4D30);
STATICINIT2_REVERSED("StaticInit::8E4D50", 0x008E4D50, &staticinit::Fn8E4D50);
STATICINIT2_REVERSED("StaticInit::8E4D70", 0x008E4D70, &staticinit::Fn8E4D70);
STATICINIT2_REVERSED("StaticInit::8E4D90", 0x008E4D90, &staticinit::Fn8E4D90);
STATICINIT2_REVERSED("StaticInit::8E4DB0", 0x008E4DB0, &staticinit::Fn8E4DB0);
STATICINIT2_REVERSED("StaticInit::8E4DD0", 0x008E4DD0, &staticinit::Fn8E4DD0);
STATICINIT2_REVERSED("StaticInit::8E4DF0", 0x008E4DF0, &staticinit::Fn8E4DF0);
STATICINIT2_REVERSED("StaticInit::8E4E10", 0x008E4E10, &staticinit::Fn8E4E10);
STATICINIT2_REVERSED("StaticInit::8E4E30", 0x008E4E30, &staticinit::Fn8E4E30);
STATICINIT2_REVERSED("StaticInit::8E4E50", 0x008E4E50, &staticinit::Fn8E4E50);
STATICINIT2_REVERSED("StaticInit::8E4E70", 0x008E4E70, &staticinit::Fn8E4E70);
STATICINIT2_REVERSED("StaticInit::8E4E90", 0x008E4E90, &staticinit::Fn8E4E90);
STATICINIT2_REVERSED("StaticInit::8E4EB0", 0x008E4EB0, &staticinit::Fn8E4EB0);
STATICINIT2_REVERSED("StaticInit::8E4ED0", 0x008E4ED0, &staticinit::Fn8E4ED0);
STATICINIT2_REVERSED("StaticInit::8E4EF0", 0x008E4EF0, &staticinit::Fn8E4EF0);
STATICINIT2_REVERSED("StaticInit::8E4F10", 0x008E4F10, &staticinit::Fn8E4F10);
STATICINIT2_REVERSED("StaticInit::8E4F30", 0x008E4F30, &staticinit::Fn8E4F30);
STATICINIT2_REVERSED("StaticInit::8E4F50", 0x008E4F50, &staticinit::Fn8E4F50);
STATICINIT2_REVERSED("StaticInit::8E4F70", 0x008E4F70, &staticinit::Fn8E4F70);
STATICINIT2_REVERSED("StaticInit::8E4F90", 0x008E4F90, &staticinit::Fn8E4F90);
STATICINIT2_REVERSED("StaticInit::8E4FB0", 0x008E4FB0, &staticinit::Fn8E4FB0);
STATICINIT2_REVERSED("StaticInit::8E5010", 0x008E5010, &staticinit::Fn8E5010);
STATICINIT2_REVERSED("StaticInit::8E5030", 0x008E5030, &staticinit::Fn8E5030);
STATICINIT2_REVERSED("StaticInit::8E5050", 0x008E5050, &staticinit::Fn8E5050);
STATICINIT2_REVERSED("StaticInit::8E5070", 0x008E5070, &staticinit::Fn8E5070);
STATICINIT2_REVERSED("StaticInit::8E5090", 0x008E5090, &staticinit::Fn8E5090);
STATICINIT2_REVERSED("StaticInit::8E50B0", 0x008E50B0, &staticinit::Fn8E50B0);
STATICINIT2_REVERSED("StaticInit::8E50D0", 0x008E50D0, &staticinit::Fn8E50D0);
STATICINIT2_REVERSED("StaticInit::8E50F0", 0x008E50F0, &staticinit::Fn8E50F0);
STATICINIT2_REVERSED("StaticInit::8E5110", 0x008E5110, &staticinit::Fn8E5110);
STATICINIT2_REVERSED("StaticInit::8E5130", 0x008E5130, &staticinit::Fn8E5130);
STATICINIT2_REVERSED("StaticInit::8E5150", 0x008E5150, &staticinit::Fn8E5150);
STATICINIT2_REVERSED("StaticInit::8E5170", 0x008E5170, &staticinit::Fn8E5170);
STATICINIT2_REVERSED("StaticInit::8E5190", 0x008E5190, &staticinit::Fn8E5190);
STATICINIT2_REVERSED("StaticInit::8E51B0", 0x008E51B0, &staticinit::Fn8E51B0);
STATICINIT2_REVERSED("StaticInit::8E51D0", 0x008E51D0, &staticinit::Fn8E51D0);
STATICINIT2_REVERSED("StaticInit::8E51F0", 0x008E51F0, &staticinit::Fn8E51F0);
STATICINIT2_REVERSED("StaticInit::8E5210", 0x008E5210, &staticinit::Fn8E5210);
STATICINIT2_REVERSED("StaticInit::8E5230", 0x008E5230, &staticinit::Fn8E5230);
STATICINIT2_REVERSED("StaticInit::8E5250", 0x008E5250, &staticinit::Fn8E5250);
STATICINIT2_REVERSED("StaticInit::8E5270", 0x008E5270, &staticinit::Fn8E5270);
STATICINIT2_REVERSED("StaticInit::8E5290", 0x008E5290, &staticinit::Fn8E5290);
STATICINIT2_REVERSED("StaticInit::8E52B0", 0x008E52B0, &staticinit::Fn8E52B0);
STATICINIT2_REVERSED("StaticInit::8E52D0", 0x008E52D0, &staticinit::Fn8E52D0);
STATICINIT2_REVERSED("StaticInit::8E52F0", 0x008E52F0, &staticinit::Fn8E52F0);
STATICINIT2_REVERSED("StaticInit::8E5310", 0x008E5310, &staticinit::Fn8E5310);
STATICINIT2_REVERSED("StaticInit::8E5330", 0x008E5330, &staticinit::Fn8E5330);
STATICINIT2_REVERSED("StaticInit::8E5350", 0x008E5350, &staticinit::Fn8E5350);
STATICINIT2_REVERSED("StaticInit::8E5370", 0x008E5370, &staticinit::Fn8E5370);
STATICINIT2_REVERSED("StaticInit::8E5390", 0x008E5390, &staticinit::Fn8E5390);
STATICINIT2_REVERSED("StaticInit::8E53B0", 0x008E53B0, &staticinit::Fn8E53B0);
STATICINIT2_REVERSED("StaticInit::8E53D0", 0x008E53D0, &staticinit::Fn8E53D0);
STATICINIT2_REVERSED("StaticInit::8E53F0", 0x008E53F0, &staticinit::Fn8E53F0);
STATICINIT2_REVERSED("StaticInit::8E5410", 0x008E5410, &staticinit::Fn8E5410);
STATICINIT2_REVERSED("StaticInit::8E5430", 0x008E5430, &staticinit::Fn8E5430);
STATICINIT2_REVERSED("StaticInit::8E5450", 0x008E5450, &staticinit::Fn8E5450);
STATICINIT2_REVERSED("StaticInit::8E5470", 0x008E5470, &staticinit::Fn8E5470);
STATICINIT2_REVERSED("StaticInit::8E5490", 0x008E5490, &staticinit::Fn8E5490);
STATICINIT2_REVERSED("StaticInit::8E54B0", 0x008E54B0, &staticinit::Fn8E54B0);
STATICINIT2_REVERSED("StaticInit::8E54D0", 0x008E54D0, &staticinit::Fn8E54D0);
STATICINIT2_REVERSED("StaticInit::8E54F0", 0x008E54F0, &staticinit::Fn8E54F0);
STATICINIT2_REVERSED("StaticInit::8E5510", 0x008E5510, &staticinit::Fn8E5510);
STATICINIT2_REVERSED("StaticInit::8E5530", 0x008E5530, &staticinit::Fn8E5530);
STATICINIT2_REVERSED("StaticInit::8E5550", 0x008E5550, &staticinit::Fn8E5550);
STATICINIT2_REVERSED("StaticInit::8E5570", 0x008E5570, &staticinit::Fn8E5570);
STATICINIT2_REVERSED("StaticInit::8E5590", 0x008E5590, &staticinit::Fn8E5590);
STATICINIT2_REVERSED("StaticInit::8E55B0", 0x008E55B0, &staticinit::Fn8E55B0);
STATICINIT2_REVERSED("StaticInit::8E55D0", 0x008E55D0, &staticinit::Fn8E55D0);
STATICINIT2_REVERSED("StaticInit::8E55F0", 0x008E55F0, &staticinit::Fn8E55F0);
STATICINIT2_REVERSED("StaticInit::8E5610", 0x008E5610, &staticinit::Fn8E5610);
STATICINIT2_REVERSED("StaticInit::8E5630", 0x008E5630, &staticinit::Fn8E5630);
STATICINIT2_REVERSED("StaticInit::8E5650", 0x008E5650, &staticinit::Fn8E5650);
STATICINIT2_REVERSED("StaticInit::8E5670", 0x008E5670, &staticinit::Fn8E5670);
STATICINIT2_REVERSED("StaticInit::8E5690", 0x008E5690, &staticinit::Fn8E5690);
STATICINIT2_REVERSED("StaticInit::8E56B0", 0x008E56B0, &staticinit::Fn8E56B0);
STATICINIT2_REVERSED("StaticInit::8E56D0", 0x008E56D0, &staticinit::Fn8E56D0);
STATICINIT2_REVERSED("StaticInit::8E56F0", 0x008E56F0, &staticinit::Fn8E56F0);
STATICINIT2_REVERSED("StaticInit::8E5710", 0x008E5710, &staticinit::Fn8E5710);
STATICINIT2_REVERSED("StaticInit::8E5730", 0x008E5730, &staticinit::Fn8E5730);
STATICINIT2_REVERSED("StaticInit::8E5750", 0x008E5750, &staticinit::Fn8E5750);
STATICINIT2_REVERSED("StaticInit::8E5770", 0x008E5770, &staticinit::Fn8E5770);
STATICINIT2_REVERSED("StaticInit::8E5790", 0x008E5790, &staticinit::Fn8E5790);
STATICINIT2_REVERSED("StaticInit::8E57B0", 0x008E57B0, &staticinit::Fn8E57B0);
STATICINIT2_REVERSED("StaticInit::8E57D0", 0x008E57D0, &staticinit::Fn8E57D0);
STATICINIT2_REVERSED("StaticInit::8E57F0", 0x008E57F0, &staticinit::Fn8E57F0);
STATICINIT2_REVERSED("StaticInit::8E5810", 0x008E5810, &staticinit::Fn8E5810);
STATICINIT2_REVERSED("StaticInit::8E5830", 0x008E5830, &staticinit::Fn8E5830);
STATICINIT2_REVERSED("StaticInit::8E5850", 0x008E5850, &staticinit::Fn8E5850);
STATICINIT2_REVERSED("StaticInit::8E5870", 0x008E5870, &staticinit::Fn8E5870);
STATICINIT2_REVERSED("StaticInit::8E5890", 0x008E5890, &staticinit::Fn8E5890);
STATICINIT2_REVERSED("StaticInit::8E58B0", 0x008E58B0, &staticinit::Fn8E58B0);
STATICINIT2_REVERSED("StaticInit::8E58D0", 0x008E58D0, &staticinit::Fn8E58D0);
STATICINIT2_REVERSED("StaticInit::8E58F0", 0x008E58F0, &staticinit::Fn8E58F0);
STATICINIT2_REVERSED("StaticInit::8E5910", 0x008E5910, &staticinit::Fn8E5910);
STATICINIT2_REVERSED("StaticInit::8E5930", 0x008E5930, &staticinit::Fn8E5930);
STATICINIT2_REVERSED("StaticInit::8E5950", 0x008E5950, &staticinit::Fn8E5950);
STATICINIT2_REVERSED("StaticInit::8E5970", 0x008E5970, &staticinit::Fn8E5970);
STATICINIT2_REVERSED("StaticInit::8E5990", 0x008E5990, &staticinit::Fn8E5990);
STATICINIT2_REVERSED("StaticInit::8E59B0", 0x008E59B0, &staticinit::Fn8E59B0);
