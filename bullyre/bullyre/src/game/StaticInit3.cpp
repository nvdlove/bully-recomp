// The exe's static initializers (the C runtime runs the table at 0x8FC350 before WinMain:
// globals constructed, their destructors given to atexit) and those destructors, transcribed
// from their code by a script (each a few calls with constant arguments, a dword copied or a
// float constant computed). Calls go through the game's addresses, so the registry's modes
// apply; the float constants are computed in double, as at start-up (the x87 at its default
// 53 bits, before Direct3D sets 24). StaticInit.h has the helpers.

#include "game/StaticInit.h"

namespace staticinit {
namespace {

// 0x8E59D0 (initializer)
void __cdecl Fn8E59D0() {
    Thiscall(0x005FA910, 0x01B6581Cu, 0x00933BA4u /* "GreetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F60C0));
}

// 0x8E59F0 (initializer)
void __cdecl Fn8E59F0() {
    Thiscall(0x005FA910, 0x01B65820u, 0x00933BB0u /* "TeacherTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F60D0));
}

// 0x8E5A10 (initializer)
void __cdecl Fn8E5A10() {
    Thiscall(0x005FA910, 0x01B65824u, 0x00933BC0u /* "MoveToVehicleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F60E0));
}

// 0x8E5A30 (initializer)
void __cdecl Fn8E5A30() {
    Thiscall(0x005FA910, 0x01B65828u, 0x00933BD4u /* "MoveObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F60F0));
}

// 0x8E5A50 (initializer)
void __cdecl Fn8E5A50() {
    Thiscall(0x005FA910, 0x01B6582Cu, 0x00933BE8u /* "FollowObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6100));
}

// 0x8E5A70 (initializer)
void __cdecl Fn8E5A70() {
    Thiscall(0x005FA910, 0x01B65830u, 0x00933C00u /* "FaceObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6110));
}

// 0x8E5A90 (initializer)
void __cdecl Fn8E5A90() {
    Thiscall(0x005FA910, 0x01B65834u, 0x00933C14u /* "CombatLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6120));
}

// 0x8E5AB0 (initializer)
void __cdecl Fn8E5AB0() {
    Thiscall(0x005FA910, 0x01B65838u, 0x00933C24u /* "PursuitLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6130));
}

// 0x8E5AD0 (initializer)
void __cdecl Fn8E5AD0() {
    Thiscall(0x005FA910, 0x01B6583Cu, 0x00933C38u /* "AttackTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6140));
}

// 0x8E5AF0 (initializer)
void __cdecl Fn8E5AF0() {
    Thiscall(0x005FA910, 0x01B65840u, 0x00933C44u /* "ActionRequestTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6150));
}

// 0x8E5B10 (initializer)
void __cdecl Fn8E5B10() {
    Thiscall(0x005FA910, 0x01B65844u, 0x00933C58u /* "ActionRegisterButtonHistoryTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6160));
}

// 0x8E5B30 (initializer)
void __cdecl Fn8E5B30() {
    Thiscall(0x005FA910, 0x01B65848u, 0x00933C7Cu /* "ActionNodeRequestTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6170));
}

// 0x8E5B50 (initializer)
void __cdecl Fn8E5B50() {
    Thiscall(0x005FA910, 0x01B6584Cu, 0x00933C94u /* "DialogActionNodeRequestTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6180));
}

// 0x8E5B70 (initializer)
void __cdecl Fn8E5B70() {
    Thiscall(0x005FA910, 0x01B65850u, 0x00933CB4u /* "BlockTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6190));
}

// 0x8E5B90 (initializer)
void __cdecl Fn8E5B90() {
    Thiscall(0x005FA910, 0x01B65854u, 0x00933CC0u /* "EvadeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F61A0));
}

// 0x8E5BB0 (initializer)
void __cdecl Fn8E5BB0() {
    Thiscall(0x005FA910, 0x01B65858u, 0x00933CCCu /* "ReleaseFightTokenTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F61B0));
}

// 0x8E5BD0 (initializer)
void __cdecl Fn8E5BD0() {
    Thiscall(0x005FA910, 0x01B6585Cu, 0x00933CE4u /* "EnableFightTokensTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F61C0));
}

// 0x8E5BF0 (initializer)
void __cdecl Fn8E5BF0() {
    Thiscall(0x005FA910, 0x01B65860u, 0x00933CFCu /* "DisableFightTokensTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F61D0));
}

// 0x8E5C10 (initializer)
void __cdecl Fn8E5C10() {
    Thiscall(0x005FA910, 0x01B65864u, 0x00933D14u /* "AttackObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F61E0));
}

// 0x8E5C30 (initializer)
void __cdecl Fn8E5C30() {
    Thiscall(0x005FA910, 0x01B65868u, 0x00933D2Cu /* "AttackPropObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F61F0));
}

// 0x8E5C50 (initializer)
void __cdecl Fn8E5C50() {
    Thiscall(0x005FA910, 0x01B6586Cu, 0x00933D48u /* "AttackVehicleObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6200));
}

// 0x8E5C70 (initializer)
void __cdecl Fn8E5C70() {
    Thiscall(0x005FA910, 0x01B65870u, 0x00933D64u /* "SetTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6210));
}

// 0x8E5C90 (initializer)
void __cdecl Fn8E5C90() {
    Thiscall(0x005FA910, 0x01B65874u, 0x00933D74u /* "ClearTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6220));
}

// 0x8E5CB0 (initializer)
void __cdecl Fn8E5CB0() {
    Thiscall(0x005FA910, 0x01B65878u, 0x00933D88u /* "SetStationaryTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6230));
}

// 0x8E5CD0 (initializer)
void __cdecl Fn8E5CD0() {
    Thiscall(0x005FA910, 0x01B6587Cu, 0x00933D9Cu /* "RangedWeaponFireTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6240));
}

// 0x8E5CF0 (initializer)
void __cdecl Fn8E5CF0() {
    Thiscall(0x005FA910, 0x01B65880u, 0x00933DB4u /* "PassBallTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6250));
}

// 0x8E5D10 (initializer)
void __cdecl Fn8E5D10() {
    Thiscall(0x005FA910, 0x01B65884u, 0x00933DC4u /* "AttackPropTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6260));
}

// 0x8E5D30 (initializer)
void __cdecl Fn8E5D30() {
    Thiscall(0x005FA910, 0x01B65888u, 0x00933DD4u /* "AttackVehicleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6270));
}

// 0x8E5D50 (initializer)
void __cdecl Fn8E5D50() {
    Thiscall(0x005FA910, 0x01B6588Cu, 0x00933DE8u /* "ProjectileCoverTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6280));
}

// 0x8E5D70 (initializer)
void __cdecl Fn8E5D70() {
    Thiscall(0x005FA910, 0x01B65890u, 0x00933E00u /* "ManageTargetsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F62A0));
}

// 0x8E5D90 (initializer)
void __cdecl Fn8E5D90() {
    Thiscall(0x005FA910, 0x01B65894u, 0x00933E14u /* "SetHealthTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F62B0));
}

// 0x8E5DB0 (initializer)
void __cdecl Fn8E5DB0() {
    Thiscall(0x005FA910, 0x01B65898u, 0x00933E24u /* "ChaseLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F62C0));
}

// 0x8E5DD0 (initializer)
void __cdecl Fn8E5DD0() {
    Thiscall(0x005FA910, 0x01B6589Cu, 0x00933E34u /* "GuardLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F62D0));
}

// 0x8E5DF0 (initializer)
void __cdecl Fn8E5DF0() {
    Thiscall(0x005FA910, 0x01B658A0u, 0x00933E44u /* "KeepDistanceLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F62E0));
}

// 0x8E5E10 (initializer)
void __cdecl Fn8E5E10() {
    Thiscall(0x005FA910, 0x01B658A4u, 0x00933E5Cu /* "TetherLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F62F0));
}

// 0x8E5E30 (initializer)
void __cdecl Fn8E5E30() {
    Thiscall(0x005FA910, 0x01B658A8u, 0x00933E6Cu /* "WaitForTargetLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6300));
}

// 0x8E5E50 (initializer)
void __cdecl Fn8E5E50() {
    Thiscall(0x005FA910, 0x01B658ACu, 0x00933E84u /* "ClearObjectivesTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6310));
}

// 0x8E5E70 (initializer)
void __cdecl Fn8E5E70() {
    Thiscall(0x005FA910, 0x01B658B0u, 0x00933E9Cu /* "CircleAroundTargetLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6320));
}

// 0x8E5E90 (initializer)
void __cdecl Fn8E5E90() {
    Thiscall(0x005FA910, 0x01B658B4u, 0x00933EB8u /* "OnBikeDashTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6330));
}

// 0x8E5EB0 (initializer)
void __cdecl Fn8E5EB0() {
    Thiscall(0x005FA910, 0x01B658B8u, 0x00933EC8u /* "FleeObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6340));
}

// 0x8E5ED0 (initializer)
void __cdecl Fn8E5ED0() {
    Thiscall(0x005FA910, 0x01B658BCu, 0x00933EDCu /* "SeekToPickupTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6350));
}

// 0x8E5EF0 (initializer)
void __cdecl Fn8E5EF0() {
    Thiscall(0x005FA910, 0x01B658C0u, 0x00933EF0u /* "PropSetUsageStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6360));
}

// 0x8E5F10 (initializer)
void __cdecl Fn8E5F10() {
    Thiscall(0x005FA910, 0x01B658C4u, 0x00933F08u /* "PropReleaseUsageStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6370));
}

// 0x8E5F30 (initializer)
void __cdecl Fn8E5F30() {
    Thiscall(0x005FA910, 0x01B658C8u, 0x00933F24u /* "SetActionTimerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6380));
}

// 0x8E5F50 (initializer)
void __cdecl Fn8E5F50() {
    Thiscall(0x005FA910, 0x01B658CCu, 0x00933F38u /* "JumpLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6390));
}

// 0x8E5F70 (initializer)
void __cdecl Fn8E5F70() {
    Thiscall(0x005FA910, 0x01B658D0u, 0x00933F48u /* "RatLocoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F63A0));
}

// 0x8E5F90 (initializer)
void __cdecl Fn8E5F90() {
    Thiscall(0x005FA910, 0x01B658D4u, 0x00933F58u /* "SeekToSnowTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F63B0));
}

// 0x8E5FB0 (initializer)
void __cdecl Fn8E5FB0() {
    Thiscall(0x005FA910, 0x01B658D8u, 0x00933F68u /* "SeekToSurfaceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F63C0));
}

// 0x8E5FD0 (initializer)
void __cdecl Fn8E5FD0() {
    Thiscall(0x005FA910, 0x01B658DCu, 0x00933F7Cu /* "PlayCatchObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F63D0));
}

// 0x8E5FF0 (initializer)
void __cdecl Fn8E5FF0() {
    Thiscall(0x005FA910, 0x01B658E0u, 0x00933F94u /* "PassBallObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F63E0));
}

// 0x8E6010 (initializer)
void __cdecl Fn8E6010() {
    Thiscall(0x005FA910, 0x01B658E4u, 0x00933FACu /* "ReceiveBallObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F63F0));
}

// 0x8E6030 (initializer)
void __cdecl Fn8E6030() {
    Thiscall(0x005FA910, 0x01B658E8u, 0x00933FC8u /* "RecruitAllyTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6400));
}

// 0x8E6050 (initializer)
void __cdecl Fn8E6050() {
    Thiscall(0x005FA910, 0x01B658ECu, 0x00933FDCu /* "DismissAllyTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6410));
}

// 0x8E6070 (initializer)
void __cdecl Fn8E6070() {
    Thiscall(0x005FA910, 0x01B658F0u, 0x00933FF0u /* "NeverOffscreenTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6420));
}

// 0x8E6090 (initializer)
void __cdecl Fn8E6090() {
    Thiscall(0x005FA910, 0x01B658F4u, 0x00934004u /* "SocializeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6430));
}

// 0x8E60B0 (initializer)
void __cdecl Fn8E60B0() {
    Thiscall(0x005FA910, 0x01B658F8u, 0x00934014u /* "SocializeActionCompleteTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6440));
}

// 0x8E60D0 (initializer)
void __cdecl Fn8E60D0() {
    Thiscall(0x005FA910, 0x01B658FCu, 0x00934034u /* "PlayerSocializingTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6450));
}

// 0x8E60F0 (initializer)
void __cdecl Fn8E60F0() {
    Thiscall(0x005FA910, 0x01B65900u, 0x0093404Cu /* "SetEmotionTowardsTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6460));
}

// 0x8E6110 (initializer)
void __cdecl Fn8E6110() {
    Thiscall(0x005FA910, 0x01B65904u, 0x0093406Cu /* "SetEmotionTowardsPlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6470));
}

// 0x8E6130 (initializer)
void __cdecl Fn8E6130() {
    Thiscall(0x005FA910, 0x01B65908u, 0x0093408Cu /* "SetWantsToSocializeWithTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6480));
}

// 0x8E6150 (initializer)
void __cdecl Fn8E6150() {
    Thiscall(0x005FA910, 0x01B6590Cu, 0x009340B0u /* "SetSocialWaitTimeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6490));
}

// 0x8E6170 (initializer)
void __cdecl Fn8E6170() {
    Thiscall(0x005FA910, 0x01B65910u, 0x009340C8u /* "StopSocializingTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F64A0));
}

// 0x8E6190 (initializer)
void __cdecl Fn8E6190() {
    Thiscall(0x005FA910, 0x01B65914u, 0x009340E0u /* "IncKissCountTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F64B0));
}

// 0x8E61B0 (initializer)
void __cdecl Fn8E61B0() {
    Thiscall(0x005FA910, 0x01B65918u, 0x009340F4u /* "GiveArmorBonusTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F64C0));
}

// 0x8E61D0 (initializer)
void __cdecl Fn8E61D0() {
    Thiscall(0x005FA910, 0x01B6591Cu, 0x00934108u /* "TargetNearestCarTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F64D0));
}

// 0x8E61F0 (initializer)
void __cdecl Fn8E61F0() {
    Thiscall(0x005FA910, 0x01B65920u, 0x00934120u /* "TargetPlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F64E0));
}

// 0x8E6210 (initializer)
void __cdecl Fn8E6210() {
    Thiscall(0x005FA910, 0x01B65924u, 0x00934134u /* "SetTargetSemaphoreTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F64F0));
}

// 0x8E6230 (initializer)
void __cdecl Fn8E6230() {
    Thiscall(0x005FA910, 0x01B65928u, 0x0093414Cu /* "EvadeGrenadeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6500));
}

// 0x8E6250 (initializer)
void __cdecl Fn8E6250() {
    Thiscall(0x005FA910, 0x01B6592Cu, 0x00934160u /* "EvadeProjectileTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6510));
}

// 0x8E6270 (initializer)
void __cdecl Fn8E6270() {
    Thiscall(0x005FA910, 0x01B65930u, 0x00934178u /* "RecruitAllySelfTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6520));
}

// 0x8E6290 (initializer)
void __cdecl Fn8E6290() {
    Thiscall(0x005FA910, 0x01B65934u, 0x00934190u /* "DismissAllySelfTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6530));
}

// 0x8E62B0 (initializer)
void __cdecl Fn8E62B0() {
    Thiscall(0x005FA910, 0x01B65938u, 0x009341A8u /* "SetMyTargetToMyLeadersTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6540));
}

// 0x8E62D0 (initializer)
void __cdecl Fn8E62D0() {
    Thiscall(0x005FA910, 0x01B6593Cu, 0x009341CCu /* "AttackMyLeadersTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6550));
}

// 0x8E62F0 (initializer)
void __cdecl Fn8E62F0() {
    Thiscall(0x005FA910, 0x01B65940u, 0x009341E8u /* "PedSetConversationStanceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6560));
}

// 0x8E6310 (initializer)
void __cdecl Fn8E6310() {
    Thiscall(0x005FA910, 0x01B65944u, 0x00934208u /* "DisableMoveOutOfWayTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6570));
}

// 0x8E6330 (initializer)
void __cdecl Fn8E6330() {
    Thiscall(0x005FA910, 0x01B65948u, 0x00934224u /* "EquipRangedWeaponTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6580));
}

// 0x8E6350 (initializer)
void __cdecl Fn8E6350() {
    Thiscall(0x005FA910, 0x01B6594Cu, 0x0093423Cu /* "ClearCurrentObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6590));
}

// 0x8E6370 (initializer)
void __cdecl Fn8E6370() {
    Thiscall(0x005FA910, 0x01B65950u, 0x00934258u /* "TargetAttackRightsAcquireTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F65A0));
}

// 0x8E6390 (initializer)
void __cdecl Fn8E6390() {
    Thiscall(0x005FA910, 0x01B65954u, 0x00934278u /* "TargetAttackRightsReleaseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F65B0));
}

// 0x8E63B0 (initializer)
void __cdecl Fn8E63B0() {
    Thiscall(0x005FA910, 0x01B65958u, 0x00934298u /* "TandemHelperClearTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F65C0));
}

// 0x8E63D0 (initializer)
void __cdecl Fn8E63D0() {
    Thiscall(0x005FA910, 0x01B6595Cu, 0x009342B0u /* "ClearLastVehicleUsedTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F65D0));
}

// 0x8E63F0 (initializer)
void __cdecl Fn8E63F0() {
    Thiscall(0x005FA910, 0x01B65960u, 0x009342CCu /* "TetherToTarget" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F65E0));
}

// 0x8E6410 (initializer)
void __cdecl Fn8E6410() {
    Thiscall(0x005FA910, 0x01B65964u, 0x009342DCu /* "TetherClear" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F65F0));
}

// 0x8E6430 (initializer)
void __cdecl Fn8E6430() {
    Thiscall(0x005FA910, 0x01B65968u, 0x009342E8u /* "SetAllyStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6600));
}

// 0x8E6450 (initializer)
void __cdecl Fn8E6450() {
    Thiscall(0x005FA910, 0x01B6596Cu, 0x009342FCu /* "SetAllyMoraleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6610));
}

// 0x8E6470 (initializer)
void __cdecl Fn8E6470() {
    Thiscall(0x005FA910, 0x01B65970u, 0x00934310u /* "RewardPlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6620));
}

// 0x8E6490 (initializer)
void __cdecl Fn8E6490() {
    Thiscall(0x005FA910, 0x01B65974u, 0x00934324u /* "GivePlayerRewardTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6630));
}

// 0x8E64B0 (initializer)
void __cdecl Fn8E64B0() {
    Thiscall(0x005FA910, 0x01B65978u, 0x0093433Cu /* "AddBlipTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6640));
}

// 0x8E64D0 (initializer)
void __cdecl Fn8E64D0() {
    Thiscall(0x005FA910, 0x01B6597Cu, 0x0093434Cu /* "ClearBlipTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6650));
}

// 0x8E64F0 (initializer)
void __cdecl Fn8E64F0() {
    Thiscall(0x005FA910, 0x01B65980u, 0x0093435Cu /* "SlingerObjectiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6660));
}

// 0x8E6510 (initializer)
void __cdecl Fn8E6510() {
    Thiscall(0x005FA910, 0x01B65984u, 0x00934374u /* "RegisterAsGirlfriendTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6670));
}

// 0x8E6530 (initializer)
void __cdecl Fn8E6530() {
    Thiscall(0x005FA910, 0x01B65988u, 0x00934390u /* "DeteriorateAttitudeToTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6680));
}

// 0x8E6550 (initializer)
void __cdecl Fn8E6550() {
    Thiscall(0x005FA910, 0x01B6598Cu, 0x009343B4u /* "SetTargetAsGrappleTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6690));
}

// 0x8E6570 (initializer)
void __cdecl Fn8E6570() {
    Thiscall(0x005FA910, 0x01B65990u, 0x009343D4u /* "ChangeDayTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F66A0));
}

// 0x8E6590 (initializer)
void __cdecl Fn8E6590() {
    Float(0x01B659DC) = float((1.0 / double(Float(0x00B13298))));
}

// 0x8E65B0 (initializer)
void __cdecl Fn8E65B0() {
    Float(0x01B659E0) = float((1.0 / (double(Float(0x00B13298)) * double(Float(0x00B13298)))));
}

// 0x8E65D0 (initializer)
void __cdecl Fn8E65D0() {
    Float(0x01B659E4) = float((double(Float(0x01B659E0)) * Double(0x00900140)));
}

// 0x8E65F0 (initializer)
void __cdecl Fn8E65F0() {
    Float(0x01B659E8) = float((Double(0x00900AD8) / double(Float(0x00B13298))));
}

// 0x8E6610 (initializer)
void __cdecl Fn8E6610() {
    Float(0x01B659EC) = float((1.0 / double(Float(0x00B132A0))));
}

// 0x8E6630 (initializer)
void __cdecl Fn8E6630() {
    Float(0x01B659F0) = float((1.0 / (double(Float(0x00B132A0)) * double(Float(0x00B132A0)))));
}

// 0x8E6650 (initializer)
void __cdecl Fn8E6650() {
    Float(0x01B659F4) = float((double(Float(0x01B659F0)) * Double(0x00900140)));
}

// 0x8E6670 (initializer)
void __cdecl Fn8E6670() {
    Float(0x01B659F8) = float((Double(0x00900AD8) / double(Float(0x00B132A0))));
}

// 0x8E6690 (initializer)
void __cdecl Fn8E6690() {
    Thiscall(0x005FA910, 0x01B659FCu, 0x0093BBE0u /* "ConditionTargetInteractiveProp" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F66B0));
}

// 0x8E66B0 (initializer)
void __cdecl Fn8E66B0() {
    Thiscall(0x005FA910, 0x01B65A00u, 0x0093BC00u /* "ConditionPedNearPropSocket" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F66C0));
}

// 0x8E66D0 (initializer)
void __cdecl Fn8E66D0() {
    Thiscall(0x005FA910, 0x01B65A04u, 0x0093BC1Cu /* "ConditionPropHealth" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F66D0));
}

// 0x8E66F0 (initializer)
void __cdecl Fn8E66F0() {
    Thiscall(0x005FA910, 0x01B65A08u, 0x0093BC30u /* "ConditionPropIsPlaying" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F66E0));
}

// 0x8E6710 (initializer)
void __cdecl Fn8E6710() {
    Thiscall(0x005FA910, 0x01B65A0Cu, 0x0093BC48u /* "ConditionPropFlags" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F66F0));
}

// 0x8E6730 (initializer)
void __cdecl Fn8E6730() {
    Thiscall(0x005FA910, 0x01B65A10u, 0x0093BC5Cu /* "ConditionPropModelID" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6700));
}

// 0x8E6750 (initializer)
void __cdecl Fn8E6750() {
    Thiscall(0x005FA910, 0x01B65A14u, 0x0093BC74u /* "ConditionPropDisplayUseText" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6710));
}

// 0x8E6770 (initializer)
void __cdecl Fn8E6770() {
    Thiscall(0x005FA910, 0x01B65A18u, 0x0093BC90u /* "ConditionPropPedIsUsingMe" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6720));
}

// 0x8E6790 (initializer)
void __cdecl Fn8E6790() {
    Thiscall(0x005FA910, 0x01B65A1Cu, 0x0093BCACu /* "ConditionPropTagPercentage" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6730));
}

// 0x8E67B0 (initializer)
void __cdecl Fn8E67B0() {
    Thiscall(0x005FA910, 0x01B65A20u, 0x0093BCC8u /* "ConditionPropModelLoaded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6740));
}

// 0x8E67D0 (initializer)
void __cdecl Fn8E67D0() {
    Thiscall(0x005FA910, 0x01B65A24u, 0x0093BCE4u /* "ConditionPropUniqueID" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6750));
}

// 0x8E67F0 (initializer)
void __cdecl Fn8E67F0() {
    Thiscall(0x005FA910, 0x01B65A28u, 0x0093BE04u /* "PropAnimationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6760));
}

// 0x8E6810 (initializer)
void __cdecl Fn8E6810() {
    Thiscall(0x005FA910, 0x01B65A2Cu, 0x0093BE18u /* "PropAnimationParametricTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6770));
}

// 0x8E6830 (initializer)
void __cdecl Fn8E6830() {
    Thiscall(0x005FA910, 0x01B65A30u, 0x0093BE38u /* "PropEffectTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6780));
}

// 0x8E6850 (initializer)
void __cdecl Fn8E6850() {
    Thiscall(0x005FA910, 0x01B65A34u, 0x0093BE48u /* "PropWeaponFireTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6790));
}

// 0x8E6870 (initializer)
void __cdecl Fn8E6870() {
    Thiscall(0x005FA910, 0x01B65A38u, 0x0093BE5Cu /* "PropInteractTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F67A0));
}

// 0x8E6890 (initializer)
void __cdecl Fn8E6890() {
    Thiscall(0x005FA910, 0x01B65A3Cu, 0x0093BE70u /* "PlayOnPropTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F67B0));
}

// 0x8E68B0 (initializer)
void __cdecl Fn8E68B0() {
    Thiscall(0x005FA910, 0x01B65A40u, 0x0093BE80u /* "PropSetPedActionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F67C0));
}

// 0x8E68D0 (initializer)
void __cdecl Fn8E68D0() {
    Thiscall(0x005FA910, 0x01B65A44u, 0x0093BE98u /* "PropGeometryTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F67D0));
}

// 0x8E68F0 (initializer)
void __cdecl Fn8E68F0() {
    Thiscall(0x005FA910, 0x01B65A48u, 0x0093AE88u /* "PropEntityFlagsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F67E0));
}

// 0x8E6910 (initializer)
void __cdecl Fn8E6910() {
    Thiscall(0x005FA910, 0x01B65A4Cu, 0x0093BEACu /* "PropCollisionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F67F0));
}

// 0x8E6930 (initializer)
void __cdecl Fn8E6930() {
    Thiscall(0x005FA910, 0x01B65A50u, 0x0093BEC0u /* "PropTaggedTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6800));
}

// 0x8E6950 (initializer)
void __cdecl Fn8E6950() {
    Thiscall(0x00510DE0, 0x01B65A54u, 0x00000032u, 0x00000017u, 0x00000017u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A58u, 0x00000013u, 0x00000025u, 0x00000012u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A5Cu, 0x00000015u, 0x0000001Du, 0x00000030u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A60u, 0x0000002Bu, 0x00000018u, 0x0000002Eu, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A64u, 0x00000060u, 0x0000005Cu, 0x0000005Eu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F68E0));
}

// 0x8E69D0 (initializer)
void __cdecl Fn8E69D0() {
    Thiscall(0x00510DE0, 0x01B65A68u, 0x0000001Eu, 0x0000000Eu, 0x0000000Eu, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A6Cu, 0x0000000Bu, 0x00000016u, 0x0000000Bu, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A70u, 0x0000000Du, 0x00000011u, 0x0000001Du, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A74u, 0x0000001Au, 0x0000000Eu, 0x0000001Cu, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A78u, 0x0000003Au, 0x00000037u, 0x00000038u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F68F0));
}

// 0x8E6A50 (initializer)
void __cdecl Fn8E6A50() {
    Thiscall(0x00510DE0, 0x01B65A7Cu, 0x00000040u, 0x00000010u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A80u, 0x00000010u, 0x00000040u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A84u, 0x00000010u, 0x00000010u, 0x00000040u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A88u, 0x00000040u, 0x00000010u, 0x00000040u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A8Cu, 0x00000040u, 0x00000040u, 0x00000040u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6900));
}

// 0x8E6AD0 (initializer)
void __cdecl Fn8E6AD0() {
    Thiscall(0x00510DE0, 0x01B65A90u, 0x00000026u, 0x00000009u, 0x00000009u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A94u, 0x00000009u, 0x00000026u, 0x00000009u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A98u, 0x00000009u, 0x00000009u, 0x00000026u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65A9Cu, 0x00000026u, 0x00000009u, 0x00000026u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x01B65AA0u, 0x00000026u, 0x00000026u, 0x00000026u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6910));
}

// 0x8E6B50 (initializer)
void __cdecl Fn8E6B50() {
    Thiscall(0x00510DE0, 0x01B65AA4u, 0x0000002Bu, 0x00000018u, 0x0000002Eu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6920));
}

// 0x8E6B80 (initializer)
void __cdecl Fn8E6B80() {
    Thiscall(0x00510DE0, 0x01B65AA8u, 0x0000001Au, 0x0000000Eu, 0x0000001Cu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6930));
}

// 0x8E6BB0 (initializer)
void __cdecl Fn8E6BB0() {
    Thiscall(0x005FA910, 0x01B65AACu, 0x0093BED0u /* "PropTagCompletedTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6810));
}

// 0x8E6BD0 (initializer)
void __cdecl Fn8E6BD0() {
    Thiscall(0x005FA910, 0x01B65AB0u, 0x0093BEE8u /* "PropTagEffectTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6820));
}

// 0x8E6BF0 (initializer)
void __cdecl Fn8E6BF0() {
    Thiscall(0x005FA910, 0x01B65AB4u, 0x0093BEFCu /* "PropFlagsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6830));
}

// 0x8E6C10 (initializer)
void __cdecl Fn8E6C10() {
    Thiscall(0x005FA910, 0x01B65AB8u, 0x0093BF0Cu /* "PropRotateProxyTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6840));
}

// 0x8E6C30 (initializer)
void __cdecl Fn8E6C30() {
    Thiscall(0x005FA910, 0x01B65ABCu, 0x0093BF24u /* "PropAlterObstacleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6850));
}

// 0x8E6C50 (initializer)
void __cdecl Fn8E6C50() {
    Thiscall(0x005FA910, 0x01B65AC0u, 0x0093BF3Cu /* "PropUseTextTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6860));
}

// 0x8E6C70 (initializer)
void __cdecl Fn8E6C70() {
    Thiscall(0x005FA910, 0x01B65AC4u, 0x0093BF50u /* "PropSetTargetSocketTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6870));
}

// 0x8E6C90 (initializer)
void __cdecl Fn8E6C90() {
    Thiscall(0x005FA910, 0x01B65AC8u, 0x0093BF6Cu /* "PropAttemptDoorTransitionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6880));
}

// 0x8E6CB0 (initializer)
void __cdecl Fn8E6CB0() {
    Thiscall(0x005FA910, 0x01B65ACCu, 0x0093BF8Cu /* "PropAttachPropAnimationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6890));
}

// 0x8E6CD0 (initializer)
void __cdecl Fn8E6CD0() {
    Thiscall(0x005FA910, 0x01B65AD0u, 0x0093BFACu /* "PropAnimAttachToPropAnimTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F68A0));
}

// 0x8E6CF0 (initializer)
void __cdecl Fn8E6CF0() {
    Thiscall(0x005FA910, 0x01B65AD4u, 0x0093BFCCu /* "PropAnimRemoveAttachmentTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F68B0));
}

// 0x8E6D10 (initializer)
void __cdecl Fn8E6D10() {
    Thiscall(0x005FA910, 0x01B65AD8u, 0x0093BFECu /* "PropTransferOwnershipTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F68C0));
}

// 0x8E6D30 (initializer)
void __cdecl Fn8E6D30() {
    Thiscall(0x005FA910, 0x01B65ADCu, 0x0093C008u /* "PropSaveLockerStateTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F68D0));
}

// 0x8E6D50 (initializer)
void __cdecl Fn8E6D50() {
    Float(0x01B65AE4) = float((1.0 / double(Float(0x00B22738))));
}

// 0x8E6D70 (initializer)
void __cdecl Fn8E6D70() {
    Float(0x01B65AE8) = float((1.0 / (double(Float(0x00B22738)) * double(Float(0x00B22738)))));
}

// 0x8E6D90 (initializer)
void __cdecl Fn8E6D90() {
    Float(0x01B65AEC) = float((double(Float(0x01B65AE8)) * Double(0x00900140)));
}

// 0x8E6DB0 (initializer)
void __cdecl Fn8E6DB0() {
    Float(0x01B65AF0) = float((Double(0x00900AD8) / double(Float(0x00B22738))));
}

// 0x8E6DD0 (initializer)
void __cdecl Fn8E6DD0() {
    Thiscall(0x005FA910, 0x01B65AF4u, 0x0093C0D4u /* "VehicleAnimationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6940));
}

// 0x8E6DF0 (initializer)
void __cdecl Fn8E6DF0() {
    Thiscall(0x005FA910, 0x01B65AF8u, 0x0093C0ECu /* "VehicleFlagForActionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6950));
}

// 0x8E6E10 (initializer)
void __cdecl Fn8E6E10() {
    Thiscall(0x005FA910, 0x01B65AFCu, 0x0093C108u /* "GetOnVehicleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6960));
}

// 0x8E6E30 (initializer)
void __cdecl Fn8E6E30() {
    Thiscall(0x005FA910, 0x01B65B00u, 0x0093C11Cu /* "GetOffVehicleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6970));
}

// 0x8E6E50 (initializer)
void __cdecl Fn8E6E50() {
    Thiscall(0x005FA910, 0x01B65B04u, 0x0093C130u /* "PedalTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6980));
}

// 0x8E6E70 (initializer)
void __cdecl Fn8E6E70() {
    Thiscall(0x005FA910, 0x01B65B08u, 0x0093C13Cu /* "SteerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6990));
}

// 0x8E6E90 (initializer)
void __cdecl Fn8E6E90() {
    Thiscall(0x005FA910, 0x01B65B0Cu, 0x0093C148u /* "CoastTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F69A0));
}

// 0x8E6EB0 (initializer)
void __cdecl Fn8E6EB0() {
    Thiscall(0x005FA910, 0x01B65B10u, 0x0093C154u /* "SpeedUpTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F69B0));
}

// 0x8E6ED0 (initializer)
void __cdecl Fn8E6ED0() {
    Thiscall(0x005FA910, 0x01B65B14u, 0x0093C164u /* "SprintTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F69C0));
}

// 0x8E6EF0 (initializer)
void __cdecl Fn8E6EF0() {
    Thiscall(0x005FA910, 0x01B65B18u, 0x0093C170u /* "BrakeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F69D0));
}

// 0x8E6F10 (initializer)
void __cdecl Fn8E6F10() {
    Thiscall(0x005FA910, 0x01B65B1Cu, 0x0093C17Cu /* "SetDesiredSpeedTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F69E0));
}

// 0x8E6F30 (initializer)
void __cdecl Fn8E6F30() {
    Thiscall(0x005FA910, 0x01B65B20u, 0x0093C194u /* "HopTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F69F0));
}

// 0x8E6F50 (initializer)
void __cdecl Fn8E6F50() {
    Thiscall(0x005FA910, 0x01B65B24u, 0x0093C1A0u /* "FPPassengerGetOnTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A00));
}

// 0x8E6F70 (initializer)
void __cdecl Fn8E6F70() {
    Thiscall(0x005FA910, 0x01B65B28u, 0x0093C1B8u /* "FPPassengerGetOffTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A10));
}

// 0x8E6F90 (initializer)
void __cdecl Fn8E6F90() {
    Thiscall(0x005FA910, 0x01B65B2Cu, 0x0093C1D0u /* "PickUpBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A20));
}

// 0x8E6FB0 (initializer)
void __cdecl Fn8E6FB0() {
    Thiscall(0x005FA910, 0x01B65B30u, 0x0093C1E0u /* "FreezeBikeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A30));
}

// 0x8E6FD0 (initializer)
void __cdecl Fn8E6FD0() {
    Thiscall(0x005FA910, 0x01B65B34u, 0x0093C1F0u /* "BlendBikeToPosTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A40));
}

// 0x8E6FF0 (initializer)
void __cdecl Fn8E6FF0() {
    Thiscall(0x005FA910, 0x01B65B38u, 0x0093C2F8u /* "ConditionGetProjectileOwner" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A50));
}

// 0x8E7010 (initializer)
void __cdecl Fn8E7010() {
    Thiscall(0x005FA910, 0x01B65B3Cu, 0x0093C314u /* "ConditionEntityIsModel" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A60));
}

// 0x8E7030 (initializer)
void __cdecl Fn8E7030() {
    Thiscall(0x005FA910, 0x01B65B40u, 0x0093C32Cu /* "ConditionEntitySpeed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A70));
}

// 0x8E7050 (initializer)
void __cdecl Fn8E7050() {
    Thiscall(0x005FA910, 0x01B65B44u, 0x0093C344u /* "ConditionGetPassingPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A80));
}

// 0x8E7070 (initializer)
void __cdecl Fn8E7070() {
    Thiscall(0x005FA910, 0x01B65B48u, 0x0093C35Cu /* "ConditionButtonState" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6A90));
}

// 0x8E7090 (initializer)
void __cdecl Fn8E7090() {
    Thiscall(0x005FA910, 0x01B65B4Cu, 0x0093C374u /* "ConditionEvaluatedEntityType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6AA0));
}

// 0x8E70B0 (initializer)
void __cdecl Fn8E70B0() {
    Thiscall(0x005FA910, 0x01B65B50u, 0x0093C394u /* "ConditionWeaponDamaged" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6AB0));
}

// 0x8E70D0 (initializer)
void __cdecl Fn8E70D0() {
    Thiscall(0x005FA910, 0x01B65B54u, 0x0093C3ACu /* "ConditionWeaponAmmo" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6AC0));
}

// 0x8E70F0 (initializer)
void __cdecl Fn8E70F0() {
    Thiscall(0x005FA910, 0x01B65B58u, 0x0093C3C0u /* "ConditionWeaponCount" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6AD0));
}

// 0x8E7110 (initializer)
void __cdecl Fn8E7110() {
    Thiscall(0x005FA910, 0x01B65B5Cu, 0x0093C3D8u /* "ConditionCollisionWeaponType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6AE0));
}

// 0x8E7130 (initializer)
void __cdecl Fn8E7130() {
    Thiscall(0x005FA910, 0x01B65B60u, 0x0093C3F8u /* "ConditionAltWeaponCollisionType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6AF0));
}

// 0x8E7150 (initializer)
void __cdecl Fn8E7150() {
    Thiscall(0x005FA910, 0x01B65B64u, 0x0093C418u /* "ConditionDamageFromExplosion" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B00));
}

// 0x8E7170 (initializer)
void __cdecl Fn8E7170() {
    Thiscall(0x005FA910, 0x01B65B68u, 0x0093C438u /* "ConditionDamageFromBurningFire" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B10));
}

// 0x8E7190 (initializer)
void __cdecl Fn8E7190() {
    Thiscall(0x005FA910, 0x01B65B6Cu, 0x0093C458u /* "ConditionFireInRange" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B20));
}

// 0x8E71B0 (initializer)
void __cdecl Fn8E71B0() {
    Thiscall(0x005FA910, 0x01B65B70u, 0x0093C470u /* "ConditionWeaponEquiped" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B30));
}

// 0x8E71D0 (initializer)
void __cdecl Fn8E71D0() {
    Thiscall(0x005FA910, 0x01B65B74u, 0x0093C488u /* "ConditionWeaponEquipedType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B40));
}

// 0x8E71F0 (initializer)
void __cdecl Fn8E71F0() {
    Thiscall(0x005FA910, 0x01B65B78u, 0x0093C4A4u /* "ConditionWeaponEquipedSocket" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B50));
}

// 0x8E7210 (initializer)
void __cdecl Fn8E7210() {
    Thiscall(0x005FA910, 0x01B65B7Cu, 0x0093C4C4u /* "ConditionPropAttached" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B60));
}

// 0x8E7230 (initializer)
void __cdecl Fn8E7230() {
    Thiscall(0x005FA910, 0x01B65B80u, 0x0093C4DCu /* "ConditionWeaponDesired" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B70));
}

// 0x8E7250 (initializer)
void __cdecl Fn8E7250() {
    Thiscall(0x005FA910, 0x01B65B84u, 0x0093C4F4u /* "ConditionWeaponDesiredSocket" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B80));
}

// 0x8E7270 (initializer)
void __cdecl Fn8E7270() {
    Thiscall(0x005FA910, 0x01B65B88u, 0x0093C514u /* "ConditionWeaponDesiredIsEquiped" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6B90));
}

// 0x8E7290 (initializer)
void __cdecl Fn8E7290() {
    Thiscall(0x005FA910, 0x01B65B8Cu, 0x0093C534u /* "ConditionWeaponDesiredIsFocusWeapon" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6BA0));
}

// 0x8E72B0 (initializer)
void __cdecl Fn8E72B0() {
    Thiscall(0x005FA910, 0x01B65B90u, 0x0093C558u /* "ConditionLastWeapon" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6BB0));
}

// 0x8E72D0 (initializer)
void __cdecl Fn8E72D0() {
    Thiscall(0x005FA910, 0x01B65B94u, 0x0093C56Cu /* "ConditionWeaponModelRequest" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6BC0));
}

// 0x8E72F0 (initializer)
void __cdecl Fn8E72F0() {
    Thiscall(0x005FA910, 0x01B65B98u, 0x0093C588u /* "ConditionPropSpecialState" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6BD0));
}

// 0x8E7310 (initializer)
void __cdecl Fn8E7310() {
    Thiscall(0x005FA910, 0x01B65B9Cu, 0x0093C5A4u /* "ConditionPropTargetHeight" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6BE0));
}

// 0x8E7330 (initializer)
void __cdecl Fn8E7330() {
    Thiscall(0x005FA910, 0x01B65BA0u, 0x0093C5C0u /* "ConditionPropTargetIsTwoHanded" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6BF0));
}

// 0x8E7350 (initializer)
void __cdecl Fn8E7350() {
    Thiscall(0x005FA910, 0x01B65BA4u, 0x0093C5E0u /* "ConditionTargetProjectile" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C00));
}

// 0x8E7370 (initializer)
void __cdecl Fn8E7370() {
    Thiscall(0x005FA910, 0x01B65BA8u, 0x0093C5FCu /* "ConditionTargetProp" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C10));
}

// 0x8E7390 (initializer)
void __cdecl Fn8E7390() {
    Thiscall(0x005FA910, 0x01B65BACu, 0x0093C610u /* "ConditionTargetMyTargetsProp" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C20));
}

// 0x8E73B0 (initializer)
void __cdecl Fn8E73B0() {
    Thiscall(0x005FA910, 0x01B65BB0u, 0x0093C630u /* "ConditionCameraDesired" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C30));
}

// 0x8E73D0 (initializer)
void __cdecl Fn8E73D0() {
    Thiscall(0x005FA910, 0x01B65BB4u, 0x0093C648u /* "ConditionCameraActive" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C40));
}

// 0x8E73F0 (initializer)
void __cdecl Fn8E73F0() {
    Thiscall(0x005FA910, 0x01B65BB8u, 0x0093C660u /* "ConditionWeaponNeedInit" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C50));
}

// 0x8E7410 (initializer)
void __cdecl Fn8E7410() {
    Thiscall(0x005FA910, 0x01B65BBCu, 0x0093C678u /* "ConditionWeaponIsInventory" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C60));
}

// 0x8E7430 (initializer)
void __cdecl Fn8E7430() {
    Thiscall(0x005FA910, 0x01B65BC0u, 0x0093C694u /* "ConditionWeaponIsThrowable" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C70));
}

// 0x8E7450 (initializer)
void __cdecl Fn8E7450() {
    Thiscall(0x005FA910, 0x01B65BC4u, 0x0093C6B0u /* "ConditionWeaponHudActive" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C80));
}

// 0x8E7470 (initializer)
void __cdecl Fn8E7470() {
    Thiscall(0x005FA910, 0x01B65BC8u, 0x0093C6CCu /* "ConditionWeaponHudLocked" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6C90));
}

// 0x8E7490 (initializer)
void __cdecl Fn8E7490() {
    Thiscall(0x005FA910, 0x01B65BCCu, 0x0093C6E8u /* "ConditionWeaponWheelOn" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6CA0));
}

// 0x8E74B0 (initializer)
void __cdecl Fn8E74B0() {
    Thiscall(0x005FA910, 0x01B65BD0u, 0x0093C700u /* "ConditionHudComponentVisible" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6CB0));
}

// 0x8E74D0 (initializer)
void __cdecl Fn8E74D0() {
    Thiscall(0x005FA910, 0x01B65BD4u, 0x0093C720u /* "ConditionCameraLocked" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6CC0));
}

// 0x8E74F0 (initializer)
void __cdecl Fn8E74F0() {
    Thiscall(0x005FA910, 0x01B65BD8u, 0x0093C738u /* "ConditionCameraAllowScriptedChange" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6CD0));
}

// 0x8E7510 (initializer)
void __cdecl Fn8E7510() {
    Thiscall(0x005FA910, 0x01B65BDCu, 0x0093C75Cu /* "ConditionCameraDesiredReset" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6CE0));
}

// 0x8E7530 (initializer)
void __cdecl Fn8E7530() {
    Thiscall(0x005FA910, 0x01B65BE0u, 0x0093C778u /* "ConditionCameraBlending" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6CF0));
}

// 0x8E7550 (initializer)
void __cdecl Fn8E7550() {
    Thiscall(0x005FA910, 0x01B65BE4u, 0x0093C790u /* "ConditionCameraFading" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D00));
}

// 0x8E7570 (initializer)
void __cdecl Fn8E7570() {
    Thiscall(0x005FA910, 0x01B65BE8u, 0x0093C7A8u /* "ConditionPropLifeTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D10));
}

// 0x8E7590 (initializer)
void __cdecl Fn8E7590() {
    Thiscall(0x005FA910, 0x01B65BECu, 0x0093C7C0u /* "ConditionHasCash" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D20));
}

// 0x8E75B0 (initializer)
void __cdecl Fn8E75B0() {
    Thiscall(0x005FA910, 0x01B65BF0u, 0x0093C7D4u /* "ConditionCameraInWidescreen" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D30));
}

// 0x8E75D0 (initializer)
void __cdecl Fn8E75D0() {
    Thiscall(0x005FA910, 0x01B65BF4u, 0x0093C7F0u /* "CatchProjectileCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D40));
}

// 0x8E75F0 (initializer)
void __cdecl Fn8E75F0() {
    Thiscall(0x005FA910, 0x01B65BF8u, 0x0093C804u /* "AttackPowerCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D50));
}

// 0x8E7610 (initializer)
void __cdecl Fn8E7610() {
    Thiscall(0x005FA910, 0x01B65BFCu, 0x0093C814u /* "ConditionWeaponFlags" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D60));
}

// 0x8E7630 (initializer)
void __cdecl Fn8E7630() {
    Thiscall(0x005FA910, 0x01B65C00u, 0x0093C82Cu /* "ConditionWeaponCanBeUsedOnBike" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D70));
}

// 0x8E7650 (initializer)
void __cdecl Fn8E7650() {
    Thiscall(0x005FA910, 0x01B65C04u, 0x0093C84Cu /* "ConditionIsWeaponFiredWhenDropped" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D80));
}

// 0x8E7670 (initializer)
void __cdecl Fn8E7670() {
    Thiscall(0x005FA910, 0x01B65C08u, 0x0093C870u /* "ConditionIsTakingPictureFreeze" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6D90));
}

// 0x8E7690 (initializer)
void __cdecl Fn8E7690() {
    Float(0x01B65C54) = float((1.0 / double(Float(0x00B31CB0))));
}

// 0x8E76B0 (initializer)
void __cdecl Fn8E76B0() {
    Float(0x01B65C58) = float((1.0 / (double(Float(0x00B31CB0)) * double(Float(0x00B31CB0)))));
}

// 0x8E76D0 (initializer)
void __cdecl Fn8E76D0() {
    Float(0x01B65C5C) = float((double(Float(0x01B65C58)) * Double(0x00900140)));
}

// 0x8E76F0 (initializer)
void __cdecl Fn8E76F0() {
    Float(0x01B65C60) = float((Double(0x00900AD8) / double(Float(0x00B31CB0))));
}

// 0x8E7710 (initializer)
void __cdecl Fn8E7710() {
    Thiscall(0x005FA910, 0x01B65C64u, 0x0093C9BCu /* "CameraFadeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6DA0));
}

// 0x8E7730 (initializer)
void __cdecl Fn8E7730() {
    Thiscall(0x005FA910, 0x01B65C68u, 0x0093C9CCu /* "WeaponFireFrisbeeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6DB0));
}

// 0x8E7750 (initializer)
void __cdecl Fn8E7750() {
    Thiscall(0x005FA910, 0x01B65C6Cu, 0x0093C9E4u /* "WeaponAnimationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6DC0));
}

// 0x8E7770 (initializer)
void __cdecl Fn8E7770() {
    Thiscall(0x005FA910, 0x01B65C70u, 0x0093C9FCu /* "WeaponSetChargeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6DD0));
}

// 0x8E7790 (initializer)
void __cdecl Fn8E7790() {
    Thiscall(0x005FA910, 0x01B65C74u, 0x0093CA14u /* "PropStartLifeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6DE0));
}

// 0x8E77B0 (initializer)
void __cdecl Fn8E77B0() {
    Thiscall(0x005FA910, 0x01B65C78u, 0x0093CA28u /* "WeaponIncrementChargeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6DF0));
}

// 0x8E77D0 (initializer)
void __cdecl Fn8E77D0() {
    Thiscall(0x005FA910, 0x01B65C7Cu, 0x0093CA44u /* "WeaponFireTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E00));
}

// 0x8E77F0 (initializer)
void __cdecl Fn8E77F0() {
    Thiscall(0x005FA910, 0x01B65C80u, 0x0093CA54u /* "WeaponFireSpecificOrientationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E10));
}

// 0x8E7810 (initializer)
void __cdecl Fn8E7810() {
    Thiscall(0x005FA910, 0x01B65C84u, 0x0093CA78u /* "WeaponFireRotationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E20));
}

// 0x8E7830 (initializer)
void __cdecl Fn8E7830() {
    Thiscall(0x005FA910, 0x01B65C88u, 0x0093CA90u /* "WeaponFireAmmoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E30));
}

// 0x8E7850 (initializer)
void __cdecl Fn8E7850() {
    Thiscall(0x005FA910, 0x01B65C8Cu, 0x0093CAA4u /* "WeaponFireOffsetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E40));
}

// 0x8E7870 (initializer)
void __cdecl Fn8E7870() {
    Thiscall(0x005FA910, 0x01B65C90u, 0x0093CABCu /* "WeaponFireStartOffsetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E50));
}

// 0x8E7890 (initializer)
void __cdecl Fn8E7890() {
    Thiscall(0x005FA910, 0x01B65C94u, 0x0093CAD8u /* "WeaponImpulseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E60));
}

// 0x8E78B0 (initializer)
void __cdecl Fn8E78B0() {
    Thiscall(0x005FA910, 0x01B65C98u, 0x0093CAECu /* "WeaponTargetImpulseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E70));
}

// 0x8E78D0 (initializer)
void __cdecl Fn8E78D0() {
    Thiscall(0x005FA910, 0x01B65C9Cu, 0x0093CB08u /* "WeaponMinigameImpulseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E80));
}

// 0x8E78F0 (initializer)
void __cdecl Fn8E78F0() {
    Thiscall(0x005FA910, 0x01B65CA0u, 0x0093CB24u /* "ExtinguisherFireTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6E90));
}

// 0x8E7910 (initializer)
void __cdecl Fn8E7910() {
    Thiscall(0x005FA910, 0x01B65CA4u, 0x0093CB3Cu /* "PoisonSprayFireTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6EA0));
}

// 0x8E7930 (initializer)
void __cdecl Fn8E7930() {
    Thiscall(0x005FA910, 0x01B65CA8u, 0x0093CB54u /* "WeaponFXTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6EB0));
}

// 0x8E7950 (initializer)
void __cdecl Fn8E7950() {
    Thiscall(0x005FA910, 0x01B65CACu, 0x0093CB64u /* "ProjectilePickupTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6EC0));
}

// 0x8E7970 (initializer)
void __cdecl Fn8E7970() {
    Thiscall(0x005FA910, 0x01B65CB0u, 0x0093CB7Cu /* "PropPickupTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6ED0));
}

// 0x8E7990 (initializer)
void __cdecl Fn8E7990() {
    Thiscall(0x005FA910, 0x01B65CB4u, 0x0093CB8Cu /* "PropAttachTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6EE0));
}

// 0x8E79B0 (initializer)
void __cdecl Fn8E79B0() {
    Thiscall(0x005FA910, 0x01B65CB8u, 0x0093CB9Cu /* "PropAttachExTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6EF0));
}

// 0x8E79D0 (initializer)
void __cdecl Fn8E79D0() {
    Thiscall(0x005FA910, 0x01B65CBCu, 0x0093CBB0u /* "PropAttachToPropTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F00));
}

// 0x8E79F0 (initializer)
void __cdecl Fn8E79F0() {
    Thiscall(0x005FA910, 0x01B65CC0u, 0x0093CBC8u /* "PropRemoveAttachmentTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F10));
}

// 0x8E7A10 (initializer)
void __cdecl Fn8E7A10() {
    Thiscall(0x005FA910, 0x01B65CC4u, 0x0093CBE4u /* "PropScatterAttachmentTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F20));
}

// 0x8E7A30 (initializer)
void __cdecl Fn8E7A30() {
    Thiscall(0x005FA910, 0x01B65CC8u, 0x0093CC00u /* "PropSetSocketTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F30));
}

// 0x8E7A50 (initializer)
void __cdecl Fn8E7A50() {
    Thiscall(0x005FA910, 0x01B65CCCu, 0x0093CC14u /* "PropDetachTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F40));
}

// 0x8E7A70 (initializer)
void __cdecl Fn8E7A70() {
    Thiscall(0x005FA910, 0x01B65CD0u, 0x0093CC24u /* "PropDetachExTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F50));
}

// 0x8E7A90 (initializer)
void __cdecl Fn8E7A90() {
    Thiscall(0x005FA910, 0x01B65CD4u, 0x0093CC38u /* "PropSetDesiredTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F60));
}

// 0x8E7AB0 (initializer)
void __cdecl Fn8E7AB0() {
    Thiscall(0x005FA910, 0x01B65CD8u, 0x0093CC4Cu /* "PropSetInTransitionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F70));
}

// 0x8E7AD0 (initializer)
void __cdecl Fn8E7AD0() {
    Thiscall(0x005FA910, 0x01B65CDCu, 0x0093CC68u /* "PropGetDesiredTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F80));
}

// 0x8E7AF0 (initializer)
void __cdecl Fn8E7AF0() {
    Thiscall(0x005FA910, 0x01B65CE0u, 0x0093CC7Cu /* "PropSpawnTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6F90));
}

// 0x8E7B10 (initializer)
void __cdecl Fn8E7B10() {
    Thiscall(0x005FA910, 0x01B65CE4u, 0x0093CC8Cu /* "SetCameraTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6FA0));
}

// 0x8E7B30 (initializer)
void __cdecl Fn8E7B30() {
    Thiscall(0x005FA910, 0x01B65CE8u, 0x0093CC9Cu /* "CameraLockTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6FB0));
}

// 0x8E7B50 (initializer)
void __cdecl Fn8E7B50() {
    Thiscall(0x005FA910, 0x01B65CECu, 0x0093CCACu /* "CameraSetStartPointPullbackTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6FC0));
}

// 0x8E7B70 (initializer)
void __cdecl Fn8E7B70() {
    Thiscall(0x005FA910, 0x01B65CF0u, 0x0093CCD0u /* "CameraSetSafeZoneTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6FD0));
}

// 0x8E7B90 (initializer)
void __cdecl Fn8E7B90() {
    Thiscall(0x005FA910, 0x01B65CF4u, 0x0093CCE8u /* "CameraSetLimitTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6FE0));
}

// 0x8E7BB0 (initializer)
void __cdecl Fn8E7BB0() {
    Thiscall(0x005FA910, 0x01B65CF8u, 0x0093CCFCu /* "CameraSetDesiredActiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F6FF0));
}

// 0x8E7BD0 (initializer)
void __cdecl Fn8E7BD0() {
    Thiscall(0x005FA910, 0x01B65CFCu, 0x0093CD18u /* "UpdateFirstPersonTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7000));
}

// 0x8E7BF0 (initializer)
void __cdecl Fn8E7BF0() {
    Thiscall(0x005FA910, 0x01B65D00u, 0x0093CD30u /* "WeaponCallbackTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7010));
}

// 0x8E7C10 (initializer)
void __cdecl Fn8E7C10() {
    Thiscall(0x005FA910, 0x01B65D04u, 0x0093CD44u /* "RenderFirstPersonTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7020));
}

// 0x8E7C30 (initializer)
void __cdecl Fn8E7C30() {
    Thiscall(0x005FA910, 0x01B65D08u, 0x0093CD5Cu /* "SetWeaponInfoTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7030));
}

// 0x8E7C50 (initializer)
void __cdecl Fn8E7C50() {
    Thiscall(0x005FA910, 0x01B65D0Cu, 0x0093CD70u /* "SelectBestDirTossTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7040));
}

// 0x8E7C70 (initializer)
void __cdecl Fn8E7C70() {
    Thiscall(0x005FA910, 0x01B65D10u, 0x0093CD90u /* "CameraZoomControlTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7050));
}

// 0x8E7C90 (initializer)
void __cdecl Fn8E7C90() {
    Thiscall(0x005FA910, 0x01B65D14u, 0x0093CDA8u /* "CameraIrisControlTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7060));
}

// 0x8E7CB0 (initializer)
void __cdecl Fn8E7CB0() {
    Thiscall(0x005FA910, 0x01B65D18u, 0x0093CDC0u /* "AmmoAttachmentTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7070));
}

// 0x8E7CD0 (initializer)
void __cdecl Fn8E7CD0() {
    Thiscall(0x005FA910, 0x01B65D1Cu, 0x0093CDD4u /* "AmmoChangeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7080));
}

// 0x8E7CF0 (initializer)
void __cdecl Fn8E7CF0() {
    Thiscall(0x005FA910, 0x01B65D20u, 0x0093CDE4u /* "WeaponRemoveCollisionProjectileTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7090));
}

// 0x8E7D10 (initializer)
void __cdecl Fn8E7D10() {
    Thiscall(0x005FA910, 0x01B65D24u, 0x0093CE0Cu /* "CPedTargetLockTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F70A0));
}

// 0x8E7D30 (initializer)
void __cdecl Fn8E7D30() {
    Thiscall(0x005FA910, 0x01B65D28u, 0x0093CE20u /* "ExecuteOnTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F70B0));
}

// 0x8E7D50 (initializer)
void __cdecl Fn8E7D50() {
    Thiscall(0x005FA910, 0x01B65D2Cu, 0x0093CE38u /* "CTraceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F70C0));
}

// 0x8E7D70 (initializer)
void __cdecl Fn8E7D70() {
    Thiscall(0x005FA910, 0x01B65D30u, 0x0093CE44u /* "WeaponCollisionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F70D0));
}

// 0x8E7D90 (initializer)
void __cdecl Fn8E7D90() {
    Thiscall(0x005FA910, 0x01B65D34u, 0x0093CE5Cu /* "SetWeaponFlagsTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F70E0));
}

// 0x8E7DB0 (initializer)
void __cdecl Fn8E7DB0() {
    Thiscall(0x005FA910, 0x01B65D38u, 0x0093CE70u /* "WeaponScaleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F70F0));
}

// 0x8E7DD0 (initializer)
void __cdecl Fn8E7DD0() {
    Thiscall(0x005FA910, 0x01B65D3Cu, 0x0093CE84u /* "WeaponSpawnDamageEffectTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7100));
}

// 0x8E7DF0 (initializer)
void __cdecl Fn8E7DF0() {
    Thiscall(0x005FA910, 0x01B65D40u, 0x0093CEA4u /* "WeaponSetNextDamageLevelTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7110));
}

// 0x8E7E10 (initializer)
void __cdecl Fn8E7E10() {
    Thiscall(0x005FA910, 0x01B65D44u, 0x0093CEC4u /* "WeaponPlayerFireAutoTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7120));
}

// 0x8E7E30 (initializer)
void __cdecl Fn8E7E30() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7130));
}

// 0x8E7E40 (initializer)
void __cdecl Fn8E7E40() {
    Thiscall(0x00669740, 0x01B65D50u);  // EffectPoolStorage::Construct_669740
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7140));
}

// 0x8E7E60 (initializer)
void __cdecl Fn8E7E60() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F71D0));
}

// 0x8E7E70 (initializer)
void __cdecl Fn8E7E70() {
    Thiscall(0x00667E20, 0x01B782C8u);  // EffectManager::Construct_667E20
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7150));
}

// 0x8E7E90 (initializer)
void __cdecl Fn8E7E90() {
    Thiscall(0x0066B7B0, 0x01B782ECu);  // InterpolatorElementStore::Construct_66B7B0
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7160));
}

// 0x8E7EB0 (initializer)
void __cdecl Fn8E7EB0() {
    Thiscall(0x00666470, 0x01B78318u);  // DefinitionStore::Construct_666470
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7170));
}

// 0x8E7ED0 (initializer)
void __cdecl Fn8E7ED0() {
    Thiscall(0x00669D30, 0x01B78408u);  // EffectPools::Construct_669D30
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7180));
}

// 0x8E7EF0 (initializer)
void __cdecl Fn8E7EF0() {
    Thiscall(0x0066F890, 0x01B786A8u);  // ParticlePool::Construct_66F890
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7190));
}

// 0x8E7F10 (initializer)
void __cdecl Fn8E7F10() {
    Thiscall(0x00667970, 0x01B82AC0u);  // EffectDefinitionManager::Construct_667970
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F71A0));
}

// 0x8E7F30 (initializer)
uint32_t __cdecl Fn8E7F30() {
    return Thiscall(0x005778D0, 0x01B83108u);  // ParticleRenderer::ParticleRenderer
}

// 0x8E7F40 (initializer)
void __cdecl Fn8E7F40() {
    Thiscall(0x00578230, 0x01B83278u);  // EffectTextures::EffectTextures
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F71B0));
}

// 0x8E7F60 (initializer)
void __cdecl Fn8E7F60() {
    Thiscall(0x00667BD0, 0x01B782FCu);  // EffectLightManager::Construct_667BD0
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F71C0));
}

// 0x8E7F80 (initializer)
void __cdecl Fn8E7F80() {
    Thiscall(0x0066C9F0, 0x01B850E0u);  // Trails::Construct_66C9F0
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F71E0));
}

// 0x8E7FA0 (initializer)
uint32_t __cdecl Fn8E7FA0() {
    return Thiscall(0x0083C8E0, 0x01B890B9u);
}

// 0x8E7FB0 (initializer)
void __cdecl Fn8E7FB0() {
    Float(0x01B890BC) = float((1.0 / double(Float(0x00B9D288))));
}

// 0x8E7FD0 (initializer)
void __cdecl Fn8E7FD0() {
    Float(0x01B890C0) = float((1.0 / (double(Float(0x00B9D288)) * double(Float(0x00B9D288)))));
}

// 0x8E7FF0 (initializer)
void __cdecl Fn8E7FF0() {
    Float(0x01B890C4) = float((double(Float(0x01B890C0)) * Double(0x00900140)));
}

// 0x8E8010 (initializer)
void __cdecl Fn8E8010() {
    Float(0x01B890C8) = float((Double(0x00900AD8) / double(Float(0x00B9D288))));
}

// 0x8E8030 (initializer)
void __cdecl Fn8E8030() {
    Thiscall(0x005FA910, 0x01B890CCu, 0x0093E114u /* "CDodgeballPedFlagCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F71F0));
}

// 0x8E8050 (initializer)
void __cdecl Fn8E8050() {
    Thiscall(0x005FA910, 0x01B890D0u, 0x0093E12Cu /* "CDodgeballSetPedFlagTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7200));
}

// 0x8E8070 (initializer)
void __cdecl Fn8E8070() {
    Thiscall(0x005FA910, 0x01B890D4u, 0x0093E148u /* "CDodgeballTargetSetPedFlagTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7210));
}

// 0x8E8090 (initializer)
void __cdecl Fn8E8090() {
    Thiscall(0x005FA910, 0x01B890D8u, 0x0093E168u /* "DodgeballPedSetDuckTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7220));
}

// 0x8E80B0 (initializer)
void __cdecl Fn8E80B0() {
    Thiscall(0x005FA910, 0x01B890DCu, 0x0093E184u /* "CDodgeballGameFlagCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7230));
}

// 0x8E80D0 (initializer)
void __cdecl Fn8E80D0() {
    Thiscall(0x005FA910, 0x01B890E0u, 0x0093E19Cu /* "CDodgeballSetGameFlagTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7240));
}

// 0x8E80F0 (initializer)
void __cdecl Fn8E80F0() {
    Thiscall(0x005FA910, 0x01B890E4u, 0x0093E1B8u /* "CDodgeballChangePlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7250));
}

// 0x8E8110 (initializer)
void __cdecl Fn8E8110() {
    Thiscall(0x005FA910, 0x01B890E8u, 0x0093E1D4u /* "CDodgeballPedMarkerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7260));
}

// 0x8E8130 (initializer)
void __cdecl Fn8E8130() {
    Thiscall(0x005FA910, 0x01B890ECu, 0x0093E1F0u /* "CDodgeballTargetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7270));
}

// 0x8E8150 (initializer)
void __cdecl Fn8E8150() {
    Thiscall(0x005FA910, 0x01B890F0u, 0x0093E208u /* "CDodgeballLocationCollisionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7280));
}

// 0x8E8170 (initializer)
void __cdecl Fn8E8170() {
    Thiscall(0x005FA910, 0x01B890F4u, 0x0093E22Cu /* "CDodgeballPedInAreaCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7290));
}

// 0x8E8190 (initializer)
void __cdecl Fn8E8190() {
    Thiscall(0x005FA910, 0x01B890F8u, 0x0093E244u /* "DodgeballBaseAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F72A0));
}

// 0x8E81B0 (initializer)
void __cdecl Fn8E81B0() {
    Thiscall(0x005FA910, 0x01B890FCu, 0x0093E25Cu /* "DodgeballGameIdleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F72B0));
}

// 0x8E81D0 (initializer)
void __cdecl Fn8E81D0() {
    Thiscall(0x005FA910, 0x01B89100u, 0x0093E274u /* "DodgeballGameTimerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F72C0));
}

// 0x8E81F0 (initializer)
void __cdecl Fn8E81F0() {
    Thiscall(0x005FA910, 0x01B89104u, 0x0093E28Cu /* "DodgeballSetTurnTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F72D0));
}

// 0x8E8210 (initializer)
void __cdecl Fn8E8210() {
    Thiscall(0x005FA910, 0x01B89108u, 0x0093E2A4u /* "DodgeballResetTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F72E0));
}

// 0x8E8230 (initializer)
void __cdecl Fn8E8230() {
    Thiscall(0x005FA910, 0x01B8910Cu, 0x0093E2B8u /* "DodgeballBallStateCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F72F0));
}

// 0x8E8250 (initializer)
void __cdecl Fn8E8250() {
    Thiscall(0x005FA910, 0x01B89110u, 0x0093E2D0u /* "DodgeballBallLocationCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7300));
}

// 0x8E8270 (initializer)
void __cdecl Fn8E8270() {
    Thiscall(0x005FA910, 0x01B89114u, 0x0093E2ECu /* "DodgeballPedLocationCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7310));
}

// 0x8E8290 (initializer)
void __cdecl Fn8E8290() {
    Thiscall(0x005FA910, 0x01B89118u, 0x0093E308u /* "DodgeballGameOverCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7320));
}

// 0x8E82B0 (initializer)
void __cdecl Fn8E82B0() {
    Thiscall(0x005FA910, 0x01B8911Cu, 0x0093E320u /* "DodgeballSetMessageTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7330));
}

// 0x8E82D0 (initializer)
void __cdecl Fn8E82D0() {
    Thiscall(0x005FA910, 0x01B89120u, 0x0093E33Cu /* "DodgeballBallMessageCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7340));
}

// 0x8E82F0 (initializer)
void __cdecl Fn8E82F0() {
    Thiscall(0x005FA910, 0x01B89124u, 0x0093E358u /* "DodgeballAttackerDistCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7350));
}

// 0x8E8310 (initializer)
void __cdecl Fn8E8310() {
    Thiscall(0x005FA910, 0x01B89128u, 0x0093E374u /* "DodgeballAttackTimeCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7360));
}

// 0x8E8330 (initializer)
void __cdecl Fn8E8330() {
    Thiscall(0x005FA910, 0x01B8912Cu, 0x0093E38Cu /* "DodgeballCameraViewTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7370));
}

// 0x8E8350 (initializer)
void __cdecl Fn8E8350() {
    Thiscall(0x005FA910, 0x01B89130u, 0x0093E3A8u /* "CDodgeballBallLocationCollisionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7380));
}

// 0x8E8370 (initializer)
void __cdecl Fn8E8370() {
    Thiscall(0x005FA910, 0x01B89134u, 0x0093E3D0u /* "DodgeballPlayersInCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7390));
}

// 0x8E8390 (initializer)
void __cdecl Fn8E8390() {
    Thiscall(0x005FA910, 0x01B89138u, 0x0093E3E8u /* "DodgeballCanPickupCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F73A0));
}

// 0x8E83B0 (initializer)
void __cdecl Fn8E83B0() {
    Thiscall(0x005FA910, 0x01B8913Cu, 0x0093E400u /* "CDodgeballPickupBallTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F73B0));
}

// 0x8E83D0 (initializer)
void __cdecl Fn8E83D0() {
    Thiscall(0x005FA910, 0x01B89140u, 0x0093E41Cu /* "DodgeballSetPathTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F73C0));
}

// 0x8E83F0 (initializer)
void __cdecl Fn8E83F0() {
    Thiscall(0x005FA910, 0x01B89144u, 0x0093E434u /* "DodgeballCanPathToCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F73D0));
}

// 0x8E8410 (initializer)
void __cdecl Fn8E8410() {
    Thiscall(0x005FA910, 0x01B89148u, 0x0093E44Cu /* "DodgeballFollowPathTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F73E0));
}

// 0x8E8430 (initializer)
void __cdecl Fn8E8430() {
    Thiscall(0x005FA910, 0x01B8914Cu, 0x0093E468u /* "DodgeballBallCountCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F73F0));
}

// 0x8E8450 (initializer)
void __cdecl Fn8E8450() {
    Float(0x01B89154) = float((1.0 / double(Float(0x00B9D9C0))));
}

// 0x8E8470 (initializer)
void __cdecl Fn8E8470() {
    Float(0x01B89158) = float((1.0 / (double(Float(0x00B9D9C0)) * double(Float(0x00B9D9C0)))));
}

// 0x8E8490 (initializer)
void __cdecl Fn8E8490() {
    Float(0x01B8915C) = float((double(Float(0x01B89158)) * Double(0x00900140)));
}

// 0x8E84B0 (initializer)
void __cdecl Fn8E84B0() {
    Float(0x01B89160) = float((Double(0x00900AD8) / double(Float(0x00B9D9C0))));
}

// 0x8E84D0 (initializer)
void __cdecl Fn8E84D0() {
    Thiscall(0x005FA910, 0x01B89164u, 0x0093E864u /* "DodgeballLeadAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7400));
}

// 0x8E84F0 (initializer)
void __cdecl Fn8E84F0() {
    Thiscall(0x005FA910, 0x01B89168u, 0x0093E87Cu /* "DodgeballGetBallAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7410));
}

// 0x8E8510 (initializer)
void __cdecl Fn8E8510() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7420));
}

// 0x8E8520 (initializer)
void __cdecl Fn8E8520() {
    Thiscall(0x005FA910, 0x01B8917Cu, 0x0093EA34u /* "DodgeballSidelineAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7430));
}

// 0x8E8540 (initializer)
void __cdecl Fn8E8540() {
    Thiscall(0x005FA910, 0x01B89180u, 0x0093EA50u /* "DodgeballSidelineLeadAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7440));
}

// 0x8E8560 (initializer)
void __cdecl Fn8E8560() {
    Thiscall(0x005FA910, 0x01B89184u, 0x0093EA78u /* "DodgeballSupportAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7450));
}

// 0x8E8580 (initializer)
void __cdecl Fn8E8580() {
    Thiscall(0x005FA910, 0x01B89188u, 0x0093EA90u /* "DodgeballFaceLeadTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7460));
}

// 0x8E85A0 (initializer)
void __cdecl Fn8E85A0() {
    Thiscall(0x005FA910, 0x01B8918Cu, 0x0093EAA8u /* "DodgeballCatchBallTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7470));
}

// 0x8E85C0 (initializer)
void __cdecl Fn8E85C0() {
    Thiscall(0x005FA910, 0x01B89190u, 0x0093EAC0u /* "DodgeballGotoAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7480));
}

// 0x8E85E0 (initializer)
void __cdecl Fn8E85E0() {
    Thiscall(0x005FA910, 0x01B89194u, 0x0093EAD8u /* "DodgeballIdleAITrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7490));
}

// 0x8E8600 (initializer)
void __cdecl Fn8E8600() {
    Float(0x01B891D0) = float((1.0 / double(Float(0x00B9DAB0))));
}

// 0x8E8620 (initializer)
void __cdecl Fn8E8620() {
    Float(0x01B891D4) = float((1.0 / (double(Float(0x00B9DAB0)) * double(Float(0x00B9DAB0)))));
}

// 0x8E8640 (initializer)
void __cdecl Fn8E8640() {
    Float(0x01B891D8) = float((double(Float(0x01B891D4)) * Double(0x00900140)));
}

// 0x8E8660 (initializer)
void __cdecl Fn8E8660() {
    Float(0x01B891DC) = float((Double(0x00900AD8) / double(Float(0x00B9DAB0))));
}

// 0x8E8680 (initializer)
void __cdecl Fn8E8680() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F75E0));
}

// 0x8E8690 (initializer)
void __cdecl Fn8E8690() {
    Thiscall(0x005FA910, 0x01B891E0u, 0x0093EBF4u /* "CinematicTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F74A0));
}

// 0x8E86B0 (initializer)
void __cdecl Fn8E86B0() {
    Thiscall(0x005FA910, 0x01B891E4u, 0x0093EC04u /* "DialogLineTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F74B0));
}

// 0x8E86D0 (initializer)
void __cdecl Fn8E86D0() {
    Thiscall(0x005FA910, 0x01B891E8u, 0x0093EC14u /* "CancelConversationTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F74C0));
}

// 0x8E86F0 (initializer)
void __cdecl Fn8E86F0() {
    Thiscall(0x005FA910, 0x01B891ECu, 0x0093EC2Cu /* "DialogShotTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F74D0));
}

// 0x8E8710 (initializer)
void __cdecl Fn8E8710() {
    Thiscall(0x005FA910, 0x01B891F0u, 0x0093EC3Cu /* "CameraCutscenePathTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F74E0));
}

// 0x8E8730 (initializer)
void __cdecl Fn8E8730() {
    Thiscall(0x005FA910, 0x01B891F4u, 0x0093EC54u /* "ScriptTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F74F0));
}

// 0x8E8750 (initializer)
void __cdecl Fn8E8750() {
    Thiscall(0x005FA910, 0x01B891F8u, 0x0093EC60u /* "MissionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7500));
}

// 0x8E8770 (initializer)
void __cdecl Fn8E8770() {
    Thiscall(0x005FA910, 0x01B891FCu, 0x0093EC70u /* "MenuTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7510));
}

// 0x8E8790 (initializer)
void __cdecl Fn8E8790() {
    Thiscall(0x005FA910, 0x01B89200u, 0x0093EC7Cu /* "FacialEyeTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7520));
}

// 0x8E87B0 (initializer)
void __cdecl Fn8E87B0() {
    Thiscall(0x005FA910, 0x01B89204u, 0x0093EC8Cu /* "FacialEyeLidTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7530));
}

// 0x8E87D0 (initializer)
void __cdecl Fn8E87D0() {
    Thiscall(0x005FA910, 0x01B89208u, 0x0093ECA0u /* "FacialEyeBrowTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7540));
}

// 0x8E87F0 (initializer)
void __cdecl Fn8E87F0() {
    Thiscall(0x005FA910, 0x01B8920Cu, 0x0093ECB4u /* "FacialMouthTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7550));
}

// 0x8E8810 (initializer)
void __cdecl Fn8E8810() {
    Thiscall(0x005FA910, 0x01B89210u, 0x0093ECC8u /* "CConversationActiveCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7560));
}

// 0x8E8830 (initializer)
void __cdecl Fn8E8830() {
    Thiscall(0x005FA910, 0x01B89214u, 0x0093ECE0u /* "CameraPathTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7570));
}

// 0x8E8850 (initializer)
void __cdecl Fn8E8850() {
    Thiscall(0x005FA910, 0x01B89218u, 0x0093ECF0u /* "TextLineTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7580));
}

// 0x8E8870 (initializer)
void __cdecl Fn8E8870() {
    Thiscall(0x005FA910, 0x01B8921Cu, 0x0093ED00u /* "TextTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7590));
}

// 0x8E8890 (initializer)
void __cdecl Fn8E8890() {
    Thiscall(0x005FA910, 0x01B89220u, 0x0093ED0Cu /* "DrawTextTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F75A0));
}

// 0x8E88B0 (initializer)
void __cdecl Fn8E88B0() {
    Thiscall(0x005FA910, 0x01B89224u, 0x0093ED1Cu /* "CameraXYZTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F75B0));
}

// 0x8E88D0 (initializer)
void __cdecl Fn8E88D0() {
    Thiscall(0x005FA910, 0x01B89228u, 0x0093ED2Cu /* "CameraFadeSetupTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F75C0));
}

// 0x8E88F0 (initializer)
void __cdecl Fn8E88F0() {
    Thiscall(0x005FA910, 0x01B8922Cu, 0x0093ED44u /* "CameraLookatPlayerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F75D0));
}

// 0x8E8910 (initializer)
void __cdecl Fn8E8910() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7610));
}

// 0x8E8920 (initializer)
void __cdecl Fn8E8920() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7620));
}

// 0x8E8930 (initializer)
void __cdecl Fn8E8930() {
    Thiscall(0x00510DE0, 0x0208B7FCu, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F75F0));
}

// 0x8E89D0 (initializer)
void __cdecl Fn8E89D0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7600));
}

// 0x8E89E0 (initializer)
void __cdecl Fn8E89E0() {
    Thiscall(0x0068DC40, 0x0208B9F0u);  // CLipStore::CLipStore_68DC40
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7650));
}

// 0x8E8A00 (initializer)
void __cdecl Fn8E8A00() {
    Thiscall(0x0049A650, 0x00BA5C1Cu);  // NameKey_Destruct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7660));
}

// 0x8E8A20 (initializer)
void __cdecl Fn8E8A20() {
    Thiscall(0x0068F250, 0x0208D388u);  // CSubtitles::CSubtitles
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7670));
}

// 0x8E8A40 (initializer)
void __cdecl Fn8E8A40() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7680));
}

// 0x8E8A50 (initializer)
void __cdecl Fn8E8A50() {
    Thiscall(0x00510DE0, 0x020C2FC0u, 0x00000000u, 0x00000000u, 0x00000000u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7690));
}

// 0x8E8A80 (initializer)
void __cdecl Fn8E8A80() {
    Thiscall(0x00510DE0, 0x020C2FC4u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F76A0));
}

// 0x8E8AB0 (initializer)
void __cdecl Fn8E8AB0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F628u /* "FE_DISPLAY_SETTINGS" */);  // HashName
    Dword(0x020C2FC8) = r1;
}

// 0x8E8AD0 (initializer)
void __cdecl Fn8E8AD0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F63Cu /* "FEDS_RESOLUTION" */);  // HashName
    Dword(0x020C2FCC) = r1;
}

// 0x8E8AF0 (initializer)
void __cdecl Fn8E8AF0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F64Cu /* "FEDS_ANTIALIAS" */);  // HashName
    Dword(0x020C2FD0) = r1;
}

// 0x8E8B10 (initializer)
void __cdecl Fn8E8B10() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F65Cu /* "FEDS_VSYNC" */);  // HashName
    Dword(0x020C2FD4) = r1;
}

// 0x8E8B30 (initializer)
void __cdecl Fn8E8B30() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F668u /* "FEDS_SHADOWS" */);  // HashName
    Dword(0x020C2FD8) = r1;
}

// 0x8E8B50 (initializer)
void __cdecl Fn8E8B50() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F678u /* "FEM_ON" */);  // HashName
    Dword(0x020C2FDC) = r1;
}

// 0x8E8B70 (initializer)
void __cdecl Fn8E8B70() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F680u /* "FEM_OFF" */);  // HashName
    Dword(0x020C2FE0) = r1;
}

// 0x8E8B90 (initializer)
void __cdecl Fn8E8B90() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F688u /* "FEDS_LOW" */);  // HashName
    Dword(0x020C2FE4) = r1;
}

// 0x8E8BB0 (initializer)
void __cdecl Fn8E8BB0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F694u /* "FEDS_MED" */);  // HashName
    Dword(0x020C2FE8) = r1;
}

// 0x8E8BD0 (initializer)
void __cdecl Fn8E8BD0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F6A0u /* "FEDS_HIGH" */);  // HashName
    Dword(0x020C2FEC) = r1;
}

// 0x8E8BF0 (initializer)
void __cdecl Fn8E8BF0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093F6ACu /* "FEDS_RESTARTMSG" */);  // HashName
    Dword(0x020C2FF0) = r1;
}

// 0x8E8C10 (initializer)
void __cdecl Fn8E8C10() {
    Thiscall(0x00510DE0, 0x020C2FF4u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000040u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F76B0));
}

// 0x8E8C30 (initializer)
void __cdecl Fn8E8C30() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0093FBA4u /* "FE_MAPT" */);  // HashName
    Dword(0x020C2FF8) = r1;
}

// 0x8E8D70 (initializer)
void __cdecl Fn8E8D70() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x009406C8u /* "FE_STATUST" */);  // HashName
    Dword(0x020C3090) = r1;
}

// 0x8E8D90 (initializer)
void __cdecl Fn8E8D90() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00940890u /* "MISTATUS" */);  // HashName
    Dword(0x020C3094) = r1;
}

// 0x8E8DB0 (initializer)
void __cdecl Fn8E8DB0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0094089Cu /* "TASKS" */);  // HashName
    Dword(0x020C3098) = r1;
}

// 0x8E8DD0 (initializer)
void __cdecl Fn8E8DD0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x009408A4u /* "CLASSES" */);  // HashName
    Dword(0x020C309C) = r1;
}

// 0x8E8DF0 (initializer)
void __cdecl Fn8E8DF0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x009408ACu /* "FE_TASKTITLE" */);  // HashName
    Dword(0x020C30A0) = r1;
}

// 0x8E8E10 (initializer)
void __cdecl Fn8E8E10() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x009408BCu /* "FE_NOTASKS" */);  // HashName
    Dword(0x020C30A4) = r1;
}

// 0x8E8E30 (initializer)
void __cdecl Fn8E8E30() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x009408C8u /* "FE_NOCLASSES" */);  // HashName
    Dword(0x020C30A8) = r1;
}

// 0x8E8E50 (initializer)
void __cdecl Fn8E8E50() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x009408D8u /* "FE_NOOBJECTIVES" */);  // HashName
    Dword(0x020C30AC) = r1;
}

// 0x8E9210 (initializer)
void __cdecl Fn8E9210() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F76C0));
}

// 0x8E9220 (initializer)
void __cdecl Fn8E9220() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F77D0));
}

// 0x8E9230 (initializer)
void __cdecl Fn8E9230() {
    Thiscall(0x006A4AC0, 0x020C3370u);  // FENameList::Init
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F77E0));
}

// 0x8E9250 (initializer)
void __cdecl Fn8E9250() {
    Thiscall(0x00510DE0, 0x020C33CCu, 0x000000F9u, 0x000000AEu, 0x0000001Du, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F76D0));
}

// 0x8E9280 (initializer)
void __cdecl Fn8E9280() {
    Thiscall(0x00510DE0, 0x020C33D0u, 0x00000030u, 0x00000030u, 0x00000030u, 0x000000D8u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F76E0));
}

// 0x8E92B0 (initializer)
void __cdecl Fn8E92B0() {
    Thiscall(0x00510DE0, 0x020C33D4u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F76F0));
}

// 0x8E92E0 (initializer)
void __cdecl Fn8E92E0() {
    Thiscall(0x00510DE0, 0x020C33D8u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x00000060u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7700));
}

// 0x8E9310 (initializer)
void __cdecl Fn8E9310() {
    Thiscall(0x00510DE0, 0x020C33DCu, 0x0000003Cu, 0x0000003Cu, 0x0000003Cu, 0x0000008Cu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7710));
}

// 0x8E9340 (initializer)
void __cdecl Fn8E9340() {
    Thiscall(0x00510DE0, 0x020C33E0u, 0x00000032u, 0x00000032u, 0x00000032u, 0x00000040u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7720));
}

// 0x8E9360 (initializer)
void __cdecl Fn8E9360() {
    Thiscall(0x00510DE0, 0x020C33E4u, 0x0000008Cu, 0x000000B8u, 0x000000BDu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7730));
}

// 0x8E9390 (initializer)
void __cdecl Fn8E9390() {
    Thiscall(0x00510DE0, 0x020C33E8u, 0x000000F9u, 0x000000AEu, 0x0000001Du, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7740));
}

// 0x8E93C0 (initializer)
void __cdecl Fn8E93C0() {
    Thiscall(0x00510DE0, 0x020C33ECu, 0x00000020u, 0x000000A9u, 0x000000F5u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7750));
}

// 0x8E93F0 (initializer)
void __cdecl Fn8E93F0() {
    Thiscall(0x00510DE0, 0x020C33F0u, 0x00000055u, 0x00000055u, 0x00000055u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7760));
}

// 0x8E9420 (initializer)
void __cdecl Fn8E9420() {
    Thiscall(0x0069B1C0, 0x020C33F8u, 0x0000000Au);  // ScrollList_Construct
    Dword(0x020C33F8) = 0x00941004u;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7770));
}

// 0x8E9450 (initializer)
void __cdecl Fn8E9450() {
    Thiscall(0x0069C3A0, 0x020C3488u, 0x020C34F8u, 0x00000003u);  // SelList_Construct
    Dword(0x020C3488) = 0x0093FCFCu /* "Pti" */;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7790));
}

// 0x8E9480 (initializer)
void __cdecl Fn8E9480() {
    Thiscall(0x0069C3A0, 0x020C3508u, 0x020C3578u, 0x00000003u);  // SelList_Construct
    Dword(0x020C3508) = 0x0093FCFCu /* "Pti" */;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F77B0));
}

// 0x8E94B0 (initializer)
void __cdecl Fn8E94B0() {
    Thiscall(0x005FA910, 0x020C359Cu, 0x0094184Cu /* "CMissionData" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F77F0));
}

// 0x8E94D0 (initializer)
void __cdecl Fn8E94D0() {
    Thiscall(0x005FA910, 0x020C35A0u, 0x0094185Cu /* "CMissionCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7800));
}

// 0x8E94F0 (initializer)
void __cdecl Fn8E94F0() {
    Thiscall(0x005FA910, 0x020C35A4u, 0x0094186Cu /* "CErrandCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7830));
}

// 0x8E9510 (initializer)
void __cdecl Fn8E9510() {
    Thiscall(0x005FA910, 0x020C35A8u, 0x00941878u /* "ConditionLastMission" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7840));
}

// 0x8E9530 (initializer)
void __cdecl Fn8E9530() {
    Thiscall(0x005FA910, 0x020C35ACu, 0x00941890u /* "CMissionUnlockedCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7850));
}

// 0x8E9550 (initializer)
void __cdecl Fn8E9550() {
    Thiscall(0x005FA910, 0x020C35B0u, 0x009418A8u /* "CClassSkippedCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7860));
}

// 0x8E9570 (initializer)
void __cdecl Fn8E9570() {
    Thiscall(0x005FA910, 0x020C35B4u, 0x009418BCu /* "CTimeCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7870));
}

// 0x8E9590 (initializer)
void __cdecl Fn8E9590() {
    Thiscall(0x005FA910, 0x020C35B8u, 0x009418C8u /* "CPeriodCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7880));
}

// 0x8E95B0 (initializer)
void __cdecl Fn8E95B0() {
    Thiscall(0x005FA910, 0x020C35BCu, 0x009418D4u /* "CDayCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7890));
}

// 0x8E95D0 (initializer)
void __cdecl Fn8E95D0() {
    Thiscall(0x005FA910, 0x020C35C0u, 0x009418E0u /* "CLuaCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F78A0));
}

// 0x8E95F0 (initializer)
void __cdecl Fn8E95F0() {
    Thiscall(0x005FA910, 0x020C35C4u, 0x009418ECu /* "CMissionCharTriggerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F78B0));
}

// 0x8E9610 (initializer)
void __cdecl Fn8E9610() {
    Thiscall(0x005FA910, 0x020C35C8u, 0x00941908u /* "CMissionVehicleTriggerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F78C0));
}

// 0x8E9630 (initializer)
void __cdecl Fn8E9630() {
    Thiscall(0x005FA910, 0x020C35CCu, 0x00941924u /* "CMissionButtonTriggerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F78D0));
}

// 0x8E9650 (initializer)
void __cdecl Fn8E9650() {
    Thiscall(0x005FA910, 0x020C35D0u, 0x00941940u /* "CMissionDoorTriggerTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F78E0));
}

// 0x8E9670 (initializer)
void __cdecl Fn8E9670() {
    Thiscall(0x005FA910, 0x020C35D4u, 0x0094179Cu /* "CMissionDoorLockTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F78F0));
}

// 0x8E9690 (initializer)
void __cdecl Fn8E9690() {
    Thiscall(0x005FA910, 0x020C35D8u, 0x0094195Cu /* "CTutorialShowMessageTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7900));
}

// 0x8E96B0 (initializer)
void __cdecl Fn8E96B0() {
    Thiscall(0x005FA910, 0x020C35DCu, 0x00941978u /* "CTutorialCompleteActiveTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7910));
}

// 0x8E96D0 (initializer)
void __cdecl Fn8E96D0() {
    Thiscall(0x005FA910, 0x020C35E0u, 0x00941998u /* "CTutorialPauseTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7920));
}

// 0x8E96F0 (initializer)
void __cdecl Fn8E96F0() {
    Thiscall(0x005FA910, 0x020C35E4u, 0x009419ACu /* "CTutorialStartTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7930));
}

// 0x8E9710 (initializer)
void __cdecl Fn8E9710() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7940));
}

// 0x8E9720 (initializer)
void __cdecl Fn8E9720() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0090A37Cu /* "3_R08_SignUp" */);  // HashName
    Dword(0x020C3670) = r1;
}

// 0x8E9740 (initializer)
void __cdecl Fn8E9740() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00941BB4u /* "GoKartMissions" */);  // HashName
    Dword(0x020C3674) = r1;
}

// 0x8E9760 (initializer)
void __cdecl Fn8E9760() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00941BC4u /* "Job_LawnMowing" */);  // HashName
    Dword(0x020C3678) = r1;
}

// 0x8E9780 (initializer)
void __cdecl Fn8E9780() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00941BD4u /* "2_R03_X" */);  // HashName
    Dword(0x020C367C) = r1;
}

// 0x8E97A0 (initializer)
void __cdecl Fn8E97A0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00941BDCu /* "CarnivalRides" */);  // HashName
    Dword(0x020C3680) = r1;
}

// 0x8E97C0 (initializer)
void __cdecl Fn8E97C0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00941BECu /* "CarnivalGames" */);  // HashName
    Dword(0x020C3684) = r1;
}

// 0x8E97E0 (initializer)
void __cdecl Fn8E97E0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x0090A330u /* "MG_SoccerPenalty" */);  // HashName
    Dword(0x020C3688) = r1;
}

// 0x8E9800 (initializer)
void __cdecl Fn8E9800() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00941BFCu /* "MGHackySack" */);  // HashName
    Dword(0x020C368C) = r1;
}

// 0x8E9820 (initializer)
void __cdecl Fn8E9820() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00941C08u /* "2_R11" */);  // HashName
    Dword(0x020C3690) = r1;
}

// 0x8E9840 (initializer)
void __cdecl Fn8E9840() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7950));
}

// 0x8E9850 (initializer)
void __cdecl Fn8E9850() {
    Thiscall(0x006AC520, 0x020C3CA0u);  // CMissionMgr::CMissionMgr
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7960));
}

// 0x8E9870 (initializer)
void __cdecl Fn8E9870() {
    Thiscall(0x006AE660, 0x020C4420u);  // CMissionObjectiveMgr::CMissionObjectiveMgr
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7970));
}

// 0x8E9890 (initializer)
void __cdecl Fn8E9890() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7980));
}

// 0x8E98A0 (initializer)
void __cdecl Fn8E98A0() {
    Float(0x020C4874) = float((1.0 / double(Float(0x00BB60F8))));
}

// 0x8E98C0 (initializer)
void __cdecl Fn8E98C0() {
    Float(0x020C4878) = float((1.0 / (double(Float(0x00BB60F8)) * double(Float(0x00BB60F8)))));
}

// 0x8E98E0 (initializer)
void __cdecl Fn8E98E0() {
    Float(0x020C487C) = float((double(Float(0x020C4878)) * Double(0x00900140)));
}

// 0x8E9900 (initializer)
void __cdecl Fn8E9900() {
    Float(0x020C4880) = float((Double(0x00900AD8) / double(Float(0x00BB60F8))));
}

// 0x8E9950 (initializer)
void __cdecl Fn8E9950() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7990));
}

// 0x8E9960 (initializer)
void __cdecl Fn8E9960() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F79A0));
}

// 0x8E9970 (initializer)
void __cdecl Fn8E9970() {
    Thiscall(0x006C2580, 0x020C4910u);  // AM_Manager::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F79B0));
}

// 0x8E9990 (initializer)
void __cdecl Fn8E9990() {
    Float(0x020C5C10) = float((1.0 / double(Float(0x00BB64F4))));
}

// 0x8E99B0 (initializer)
void __cdecl Fn8E99B0() {
    Float(0x020C5C14) = float((1.0 / (double(Float(0x00BB64F4)) * double(Float(0x00BB64F4)))));
}

// 0x8E99D0 (initializer)
void __cdecl Fn8E99D0() {
    Float(0x020C5C18) = float((double(Float(0x020C5C14)) * Double(0x00900140)));
}

// 0x8E99F0 (initializer)
void __cdecl Fn8E99F0() {
    Float(0x020C5C1C) = float((Double(0x00900AD8) / double(Float(0x00BB64F4))));
}

// 0x8E9A10 (initializer)
void __cdecl Fn8E9A10() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F79E0));
}

// 0x8E9A20 (initializer)
void __cdecl Fn8E9A20() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F79F0));
}

// 0x8E9A30 (initializer)
void __cdecl Fn8E9A30() {
    Thiscall(0x006CBB10, 0x020C5F70u);  // CClothing::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7A20));
}

// 0x8E9A50 (initializer)
void __cdecl Fn8E9A50() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00942AACu /* "hair" */);  // HashName
    Dword(0x020C5F6C) = r1;
}

// 0x8E9A70 (initializer)
void __cdecl Fn8E9A70() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00911AA0u /* "none" */);  // HashName
    Dword(0x020C7BB0) = r1;
}

// 0x8E9A90 (initializer)
void __cdecl Fn8E9A90() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00942AB4u /* "underwear" */);  // HashName
    Dword(0x020C7BB4) = r1;
}

// 0x8E9AB0 (initializer)
void __cdecl Fn8E9AB0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00942AC0u /* "Ninja_BLK" */);  // HashName
    Dword(0x020C7BB8) = r1;
}

// 0x8E9AD0 (initializer)
void __cdecl Fn8E9AD0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00942ACCu /* "Ninja_WHT" */);  // HashName
    Dword(0x020C7BBC) = r1;
}

// 0x8E9AF0 (initializer)
void __cdecl Fn8E9AF0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00933600u /* "Ninja_RED" */);  // HashName
    Dword(0x020C7BC0) = r1;
}

// 0x8E9B10 (initializer)
void __cdecl Fn8E9B10() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00942AD8u /* "Orderly" */);  // HashName
    Dword(0x020C7BC4) = r1;
}

// 0x8E9B30 (initializer)
void __cdecl Fn8E9B30() {
    Thiscall(0x00510DE0, 0x020C7BD4u, 0x0000002Du, 0x0000002Du, 0x0000002Du, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7A30));
}

// 0x8E9B60 (initializer)
void __cdecl Fn8E9B60() {
    Thiscall(0x006D2160, 0x020C7BECu);  // COctTreeBase::COctTreeBase
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7A40));
}

// 0x8E9B80 (initializer)
void __cdecl Fn8E9B80() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7A50));
}

// 0x8E9BC0 (initializer)
void __cdecl Fn8E9BC0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7A70));
}

// 0x8E9BD0 (initializer)
void __cdecl Fn8E9BD0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7A90));
}

// 0x8E9BE0 (initializer)
void __cdecl Fn8E9BE0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7AB0));
}

// 0x8E9BF0 (initializer)
void __cdecl Fn8E9BF0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7AD0));
}

// 0x8E9C00 (initializer)
void __cdecl Fn8E9C00() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7AF0));
}

// 0x8E9C10 (initializer)
void __cdecl Fn8E9C10() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B10));
}

// 0x8E9C20 (initializer)
void __cdecl Fn8E9C20() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B20));
}

// 0x8E9C30 (initializer)
void __cdecl Fn8E9C30() {
    Thiscall(0x006D7680, 0x020C7E98u);  // TriggerManager::TriggerManager
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B30));
}

// 0x8E9C50 (initializer)
void __cdecl Fn8E9C50() {
    Thiscall(0x00510DE0, 0x020C8214u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B40));
}

// 0x8E9CB0 (initializer)
void __cdecl Fn8E9CB0() {
    Thiscall(0x00510DE0, 0x020C8218u, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000C0u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B50));
}

// 0x8E9CE0 (initializer)
void __cdecl Fn8E9CE0() {
    Thiscall(0x00510DE0, 0x020C821Cu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B60));
}

// 0x8E9D10 (initializer)
void __cdecl Fn8E9D10() {
    Thiscall(0x00510DE0, 0x020C822Cu, 0x00000010u, 0x00000010u, 0x00000010u, 0x00000080u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B70));
}

// 0x8E9D40 (initializer)
void __cdecl Fn8E9D40() {
    Thiscall(0x00510DE0, 0x020C8230u, 0x000000EBu, 0x000000EBu, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B80));
}

// 0x8E9D70 (initializer)
void __cdecl Fn8E9D70() {
    Thiscall(0x00510DE0, 0x020C8234u, 0x00000022u, 0x0000005Du, 0x00000042u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7B90));
}

// 0x8E9DA0 (initializer)
void __cdecl Fn8E9DA0() {
    Thiscall(0x00510DE0, 0x020C8238u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7BA0));
}

// 0x8E9DD0 (initializer)
void __cdecl Fn8E9DD0() {
    Thiscall(0x00510DE0, 0x020C8244u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7BB0));
}

// 0x8E9E00 (initializer)
void __cdecl Fn8E9E00() {
    Thiscall(0x00510DE0, 0x020C8248u, 0x00000000u, 0x00000000u, 0x00000000u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7BC0));
}

// 0x8E9E30 (initializer)
void __cdecl Fn8E9E30() {
    Thiscall(0x00510DE0, 0x020C824Cu, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7BD0));
}

// 0x8E9E60 (initializer)
void __cdecl Fn8E9E60() {
    Thiscall(0x00510DE0, 0x020C8250u, 0x00000050u, 0x000000AFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7BE0));
}

// 0x8E9E90 (initializer)
void __cdecl Fn8E9E90() {
    Thiscall(0x00510DE0, 0x020C8254u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7BF0));
}

// 0x8E9EC0 (initializer)
void __cdecl Fn8E9EC0() {
    Thiscall(0x00510DE0, 0x020C8258u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C00));
}

// 0x8E9EF0 (initializer)
void __cdecl Fn8E9EF0() {
    Thiscall(0x00510DE0, 0x020C825Cu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C10));
}

// 0x8E9F20 (initializer)
void __cdecl Fn8E9F20() {
    Thiscall(0x00510DE0, 0x020C8260u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C20));
}

// 0x8E9F50 (initializer)
void __cdecl Fn8E9F50() {
    Thiscall(0x00510DE0, 0x020C8264u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C30));
}

// 0x8E9F80 (initializer)
void __cdecl Fn8E9F80() {
    Thiscall(0x00510DE0, 0x020C8268u, 0x000000F5u, 0x000000DCu, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C40));
}

// 0x8E9FB0 (initializer)
void __cdecl Fn8E9FB0() {
    Thiscall(0x00510DE0, 0x020C826Cu, 0x00000010u, 0x00000080u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C50));
}

// 0x8E9FE0 (initializer)
void __cdecl Fn8E9FE0() {
    Thiscall(0x00510DE0, 0x020C8270u, 0x0000005Fu, 0x00000049u, 0x000000D0u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C60));
}

// 0x8EA010 (initializer)
void __cdecl Fn8EA010() {
    Thiscall(0x00510DE0, 0x020C8274u, 0x00000010u, 0x00000080u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C70));
}

// 0x8EA040 (initializer)
void __cdecl Fn8EA040() {
    Thiscall(0x00510DE0, 0x020C8278u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C80));
}

// 0x8EA070 (initializer)
void __cdecl Fn8EA070() {
    Thiscall(0x00510DE0, 0x020C827Cu, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7C90));
}

// 0x8EA0A0 (initializer)
void __cdecl Fn8EA0A0() {
    Thiscall(0x00510DE0, 0x020C8280u, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x000000C8u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7CA0));
}

// 0x8EA0D0 (initializer)
void __cdecl Fn8EA0D0() {
    Thiscall(0x00510DE0, 0x020C8290u, 0x0000000Au, 0x0000000Au, 0x0000000Au, 0x000000A0u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7CB0));
}

// 0x8EA100 (initializer)
void __cdecl Fn8EA100() {
    Thiscall(0x00510DE0, 0x020C8294u, 0x0000000Au, 0x0000000Au, 0x0000000Au, 0x00000000u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7CC0));
}

// 0x8EA120 (initializer)
void __cdecl Fn8EA120() {
    Thiscall(0x00510DE0, 0x020C8298u, 0x00000040u, 0x00000040u, 0x00000080u, 0x00000064u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7CD0));
}

// 0x8EA150 (initializer)
void __cdecl Fn8EA150() {
    Thiscall(0x00510DE0, 0x020C829Cu, 0x00000080u, 0x00000080u, 0x00000040u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7CE0));
}

// 0x8EA180 (initializer)
void __cdecl Fn8EA180() {
    Thiscall(0x00510DE0, 0x020C82A0u, 0x00000064u, 0x00000064u, 0x00000000u, 0x00000080u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7CF0));
}

// 0x8EA1B0 (initializer)
void __cdecl Fn8EA1B0() {
    Thiscall(0x00510DE0, 0x020C82A4u, 0x000000B4u, 0x000000B4u, 0x000000B4u, 0x000000C0u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D00));
}

// 0x8EA1E0 (initializer)
void __cdecl Fn8EA1E0() {
    Thiscall(0x00510DE0, 0x020C82A8u, 0x000000C0u, 0x000000C0u, 0x000000C0u, 0x000000C0u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D10));
}

// 0x8EA210 (initializer)
void __cdecl Fn8EA210() {
    Thiscall(0x00510DE0, 0x020C82ACu, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D20));
}

// 0x8EA240 (initializer)
void __cdecl Fn8EA240() {
    Thiscall(0x00510DE0, 0x020C82B0u, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D30));
}

// 0x8EA270 (initializer)
void __cdecl Fn8EA270() {
    Thiscall(0x00510DE0, 0x020C82B4u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D40));
}

// 0x8EA2A0 (initializer)
void __cdecl Fn8EA2A0() {
    Thiscall(0x00510DE0, 0x020C82B8u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D50));
}

// 0x8EA2D0 (initializer)
void __cdecl Fn8EA2D0() {
    Thiscall(0x00510DE0, 0x020C82BCu, 0x00000010u, 0x00000010u, 0x00000010u, 0x000000C8u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D60));
}

// 0x8EA300 (initializer)
void __cdecl Fn8EA300() {
    Thiscall(0x00510DE0, 0x020C82C0u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D70));
}

// 0x8EA330 (initializer)
void __cdecl Fn8EA330() {
    Thiscall(0x00510DE0, 0x020C82C4u, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D80));
}

// 0x8EA360 (initializer)
void __cdecl Fn8EA360() {
    Thiscall(0x00510DE0, 0x020C82C8u, 0x00000010u, 0x00000010u, 0x00000010u, 0x00000080u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7D90));
}

// 0x8EA390 (initializer)
void __cdecl Fn8EA390() {
    Thiscall(0x00510DE0, 0x020C82CCu, 0x00000010u, 0x00000010u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7DA0));
}

// 0x8EA3C0 (initializer)
void __cdecl Fn8EA3C0() {
    Thiscall(0x00510DE0, 0x020C82D0u, 0x00000010u, 0x00000010u, 0x00000010u, 0x00000080u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7DB0));
}

// 0x8EA3F0 (initializer)
void __cdecl Fn8EA3F0() {
    Thiscall(0x00510DE0, 0x020C82D4u, 0x00000024u, 0x00000036u, 0x00000088u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7DC0));
}

// 0x8EA420 (initializer)
void __cdecl Fn8EA420() {
    Thiscall(0x00510DE0, 0x020C82D8u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7DD0));
}

// 0x8EA450 (initializer)
void __cdecl Fn8EA450() {
    Thiscall(0x00510DE0, 0x020C82DCu, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x00000096u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7DE0));
}

// 0x8EA480 (initializer)
void __cdecl Fn8EA480() {
    Thiscall(0x00510DE0, 0x020C82E0u, 0x00000010u, 0x00000010u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7DF0));
}

// 0x8EA4B0 (initializer)
void __cdecl Fn8EA4B0() {
    Thiscall(0x00510DE0, 0x020C82E4u, 0x00000024u, 0x00000036u, 0x00000088u, 0x00000096u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E00));
}

// 0x8EA4E0 (initializer)
void __cdecl Fn8EA4E0() {
    Thiscall(0x00510DE0, 0x020C82E8u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E10));
}

// 0x8EA510 (initializer)
void __cdecl Fn8EA510() {
    Thiscall(0x00510DE0, 0x020C82ECu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E20));
}

// 0x8EA540 (initializer)
void __cdecl Fn8EA540() {
    Thiscall(0x00510DE0, 0x020C82F0u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E30));
}

// 0x8EA570 (initializer)
void __cdecl Fn8EA570() {
    Thiscall(0x00510DE0, 0x020C82F4u, 0x00000022u, 0x0000005Du, 0x00000042u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E40));
}

// 0x8EA5A0 (initializer)
void __cdecl Fn8EA5A0() {
    Thiscall(0x00510DE0, 0x020C82F8u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E50));
}

// 0x8EA5D0 (initializer)
void __cdecl Fn8EA5D0() {
    Thiscall(0x00510DE0, 0x020C82FCu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E60));
}

// 0x8EA600 (initializer)
void __cdecl Fn8EA600() {
    Thiscall(0x00510DE0, 0x020C8300u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E70));
}

// 0x8EA630 (initializer)
void __cdecl Fn8EA630() {
    Thiscall(0x00510DE0, 0x020C8304u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E80));
}

// 0x8EA660 (initializer)
void __cdecl Fn8EA660() {
    Thiscall(0x00510DE0, 0x020C8308u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7E90));
}

// 0x8EA690 (initializer)
void __cdecl Fn8EA690() {
    Thiscall(0x00510DE0, 0x020C830Cu, 0x00000022u, 0x0000005Du, 0x00000042u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7EA0));
}

// 0x8EA6C0 (initializer)
void __cdecl Fn8EA6C0() {
    Thiscall(0x00510DE0, 0x020C8310u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7EB0));
}

// 0x8EA6F0 (initializer)
void __cdecl Fn8EA6F0() {
    Thiscall(0x00510DE0, 0x020C8314u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7EC0));
}

// 0x8EA720 (initializer)
void __cdecl Fn8EA720() {
    Thiscall(0x00510DE0, 0x020C8318u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7ED0));
}

// 0x8EA750 (initializer)
void __cdecl Fn8EA750() {
    Thiscall(0x00510DE0, 0x020C831Cu, 0x000000F5u, 0x000000DCu, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7EE0));
}

// 0x8EA780 (initializer)
void __cdecl Fn8EA780() {
    Thiscall(0x00510DE0, 0x020C8320u, 0x00000005u, 0x00000064u, 0x0000000Au, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7EF0));
}

// 0x8EA7B0 (initializer)
void __cdecl Fn8EA7B0() {
    Thiscall(0x00510DE0, 0x020C8324u, 0x00000005u, 0x00000064u, 0x0000000Au, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F00));
}

// 0x8EA7E0 (initializer)
void __cdecl Fn8EA7E0() {
    Thiscall(0x00510DE0, 0x020C8328u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F10));
}

// 0x8EA810 (initializer)
void __cdecl Fn8EA810() {
    Thiscall(0x00510DE0, 0x020C832Cu, 0x00000069u, 0x00000055u, 0x000000F5u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F20));
}

// 0x8EA840 (initializer)
void __cdecl Fn8EA840() {
    Thiscall(0x00510DE0, 0x020C8330u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000B9u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F30));
}

// 0x8EA870 (initializer)
void __cdecl Fn8EA870() {
    Thiscall(0x00510DE0, 0x020C8334u, 0x0000009Bu, 0x0000009Bu, 0x000000FFu, 0x00000091u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F40));
}

// 0x8EA8A0 (initializer)
void __cdecl Fn8EA8A0() {
    Thiscall(0x00510DE0, 0x020C8338u, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F50));
}

// 0x8EA8D0 (initializer)
void __cdecl Fn8EA8D0() {
    Thiscall(0x00510DE0, 0x020C833Cu, 0x00000037u, 0x00000037u, 0x00000037u, 0x000000BCu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F60));
}

// 0x8EA900 (initializer)
void __cdecl Fn8EA900() {
    Thiscall(0x00510DE0, 0x020C8340u, 0x000000FFu, 0x00000037u, 0x00000037u, 0x000000BCu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F70));
}

// 0x8EA930 (initializer)
void __cdecl Fn8EA930() {
    Thiscall(0x00510DE0, 0x020C8344u, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x000000C8u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F80));
}

// 0x8EA960 (initializer)
void __cdecl Fn8EA960() {
    Thiscall(0x00510DE0, 0x020C8348u, 0x000000FFu, 0x000000C0u, 0x00000004u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7F90));
}

// 0x8EA990 (initializer)
void __cdecl Fn8EA990() {
    Thiscall(0x00510DE0, 0x020C834Cu, 0x00000033u, 0x00000098u, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7FA0));
}

// 0x8EA9C0 (initializer)
void __cdecl Fn8EA9C0() {
    Thiscall(0x00510DE0, 0x020C8350u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7FB0));
}

// 0x8EA9F0 (initializer)
void __cdecl Fn8EA9F0() {
    Thiscall(0x00510DE0, 0x020C8354u, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7FC0));
}

// 0x8EAA20 (initializer)
void __cdecl Fn8EAA20() {
    Thiscall(0x00510DE0, 0x020C8358u, 0x00000050u, 0x000000AFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7FD0));
}

// 0x8EAA50 (initializer)
void __cdecl Fn8EAA50() {
    Thiscall(0x00510DE0, 0x020C835Cu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7FE0));
}

// 0x8EAA80 (initializer)
void __cdecl Fn8EAA80() {
    Thiscall(0x00510DE0, 0x020C8360u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F7FF0));
}

// 0x8EAAB0 (initializer)
void __cdecl Fn8EAAB0() {
    Thiscall(0x00510DE0, 0x020C8364u, 0x00000022u, 0x0000005Du, 0x00000042u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8000));
}

// 0x8EAAE0 (initializer)
void __cdecl Fn8EAAE0() {
    Thiscall(0x00510DE0, 0x020C8368u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8010));
}

// 0x8EAB10 (initializer)
void __cdecl Fn8EAB10() {
    Thiscall(0x00510DE0, 0x020C836Cu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8020));
}

// 0x8EAB40 (initializer)
void __cdecl Fn8EAB40() {
    Thiscall(0x00510DE0, 0x020C8370u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8030));
}

// 0x8EAB70 (initializer)
void __cdecl Fn8EAB70() {
    Thiscall(0x00510DE0, 0x020C8374u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8040));
}

// 0x8EABA0 (initializer)
void __cdecl Fn8EABA0() {
    Thiscall(0x00510DE0, 0x020C8378u, 0x000000D7u, 0x00000000u, 0x00000000u, 0x00000078u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8050));
}

// 0x8EABD0 (initializer)
void __cdecl Fn8EABD0() {
    Thiscall(0x00510DE0, 0x020C837Cu, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x00000096u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8060));
}

// 0x8EAC00 (initializer)
void __cdecl Fn8EAC00() {
    Thiscall(0x00510DE0, 0x020C8380u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8070));
}

// 0x8EAC30 (initializer)
void __cdecl Fn8EAC30() {
    Thiscall(0x00510DE0, 0x020C8384u, 0x00000028u, 0x000000EBu, 0x00000032u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8080));
}

// 0x8EAC60 (initializer)
void __cdecl Fn8EAC60() {
    Thiscall(0x00510DE0, 0x020C8388u, 0x000000FFu, 0x000000FFu, 0x00000000u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8090));
}

// 0x8EAC90 (initializer)
void __cdecl Fn8EAC90() {
    Thiscall(0x00510DE0, 0x020C838Cu, 0x000000EBu, 0x0000001Eu, 0x0000000Fu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F80A0));
}

// 0x8EACC0 (initializer)
void __cdecl Fn8EACC0() {
    Thiscall(0x00510DE0, 0x020C8390u, 0x00000064u, 0x00000064u, 0x00000000u, 0x00000080u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F80B0));
}

// 0x8EAD50 (initializer)
void __cdecl Fn8EAD50() {
    Thiscall(0x00510DE0, 0x020C83A4u, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F80C0));
}

// 0x8EAD80 (initializer)
void __cdecl Fn8EAD80() {
    Thiscall(0x00510DE0, 0x020C83A8u, 0x00000010u, 0x00000010u, 0x00000010u, 0x00000040u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F80D0));
}

// 0x8EADA0 (initializer)
void __cdecl Fn8EADA0() {
    Thiscall(0x00510DE0, 0x020C83ACu, 0x000000EBu, 0x000000EBu, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F80E0));
}

// 0x8EADD0 (initializer)
void __cdecl Fn8EADD0() {
    Thiscall(0x00510DE0, 0x020C83C8u, 0x000000C0u, 0x000000C0u, 0x000000C0u, 0x000000C0u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F80F0));
}

// 0x8EAE00 (initializer)
void __cdecl Fn8EAE00() {
    Thiscall(0x00510DE0, 0x020C83CCu, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8100));
}

// 0x8EAE30 (initializer)
void __cdecl Fn8EAE30() {
    Thiscall(0x00510DE0, 0x020C83D0u, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8110));
}

// 0x8EAE80 (initializer)
void __cdecl Fn8EAE80() {
    Thiscall(0x00510DE0, 0x020C83DCu, 0x000000C8u, 0x000000C8u, 0x000000C8u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8120));
}

// 0x8EAEB0 (initializer)
void __cdecl Fn8EAEB0() {
    Thiscall(0x00510DE0, 0x020C83E0u, 0x00000080u, 0x00000040u, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8130));
}

// 0x8EAEE0 (initializer)
void __cdecl Fn8EAEE0() {
    Thiscall(0x00510DE0, 0x020C83E4u, 0x00000028u, 0x000000DCu, 0x00000028u, 0x000000C8u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8140));
}

// 0x8EAF40 (initializer)
void __cdecl Fn8EAF40() {
    Thiscall(0x00510DE0, 0x020C87F0u, 0x00000080u, 0x00000080u, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8180));
}

// 0x8EAF70 (initializer)
void __cdecl Fn8EAF70() {
    Thiscall(0x00510DE0, 0x020C87F4u, 0x00000022u, 0x0000005Du, 0x00000042u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x020C87F8u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x020C87FCu, 0x00000024u, 0x00000036u, 0x00000088u, 0x000000FFu);  // RGBA_Construct
    Thiscall(0x00510DE0, 0x020C8800u, 0x000000A6u, 0x00000093u, 0x0000003Fu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8190));
}

// 0x8EAFE0 (initializer)
void __cdecl Fn8EAFE0() {
    Float(0x020C8808) = float((1.0 / double(Float(0x00BC6FD4))));
}

// 0x8EB000 (initializer)
void __cdecl Fn8EB000() {
    Float(0x020C880C) = float((1.0 / (double(Float(0x00BC6FD4)) * double(Float(0x00BC6FD4)))));
}

// 0x8EB020 (initializer)
void __cdecl Fn8EB020() {
    Float(0x020C8810) = float((double(Float(0x020C880C)) * Double(0x00900140)));
}

// 0x8EB040 (initializer)
void __cdecl Fn8EB040() {
    Float(0x020C8814) = float((Double(0x00900AD8) / double(Float(0x00BC6FD4))));
}

// 0x8EB060 (initializer)
void __cdecl Fn8EB060() {
    Thiscall(0x00510DE0, 0x020C8818u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F81A0));
}

// 0x8EB090 (initializer)
void __cdecl Fn8EB090() {
    Thiscall(0x00510DE0, 0x020C881Cu, 0x00000022u, 0x0000005Du, 0x00000042u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F81B0));
}

// 0x8EB0C0 (initializer)
void __cdecl Fn8EB0C0() {
    Thiscall(0x00510DE0, 0x020C8820u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F81C0));
}

// 0x8EB0F0 (initializer)
void __cdecl Fn8EB0F0() {
    Float(0x020C8828) = float((1.0 / double(Float(0x00BC7038))));
}

// 0x8EB110 (initializer)
void __cdecl Fn8EB110() {
    Float(0x020C882C) = float((1.0 / (double(Float(0x00BC7038)) * double(Float(0x00BC7038)))));
}

// 0x8EB130 (initializer)
void __cdecl Fn8EB130() {
    Float(0x020C8830) = float((double(Float(0x020C882C)) * Double(0x00900140)));
}

// 0x8EB150 (initializer)
void __cdecl Fn8EB150() {
    Float(0x020C8834) = float((Double(0x00900AD8) / double(Float(0x00BC7038))));
}

// 0x8EB170 (initializer)
void __cdecl Fn8EB170() {
    Thiscall(0x00510DE0, 0x020C8838u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F81D0));
}

// 0x8EB1A0 (initializer)
void __cdecl Fn8EB1A0() {
    Thiscall(0x00510DE0, 0x020C883Cu, 0x00000010u, 0x00000010u, 0x00000010u, 0x00000040u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F81E0));
}

// 0x8EB1C0 (initializer)
void __cdecl Fn8EB1C0() {
    Thiscall(0x00510DE0, 0x020C8840u, 0x000000EBu, 0x000000EBu, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F81F0));
}

// 0x8EB1F0 (initializer)
void __cdecl Fn8EB1F0() {
    Thiscall(0x00510DE0, 0x020C8844u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8200));
}

// 0x8EB220 (initializer)
void __cdecl Fn8EB220() {
    Thiscall(0x00510DE0, 0x020C8848u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8210));
}

// 0x8EB250 (initializer)
void __cdecl Fn8EB250() {
    Thiscall(0x00510DE0, 0x020C884Cu, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8220));
}

// 0x8EB280 (initializer)
void __cdecl Fn8EB280() {
    Thiscall(0x00510DE0, 0x020C8850u, 0x000000EBu, 0x000000C8u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8230));
}

// 0x8EB2B0 (initializer)
void __cdecl Fn8EB2B0() {
    Thiscall(0x00510DE0, 0x020C8854u, 0x00000050u, 0x000000AFu, 0x000000FFu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8240));
}

// 0x8EB2E0 (initializer)
void __cdecl Fn8EB2E0() {
    Thiscall(0x00510DE0, 0x020C8858u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8250));
}

// 0x8EB310 (initializer)
void __cdecl Fn8EB310() {
    Thiscall(0x00510DE0, 0x020C885Cu, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8260));
}

// 0x8EB340 (initializer)
void __cdecl Fn8EB340() {
    Thiscall(0x00510DE0, 0x020C8860u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8270));
}

// 0x8EB370 (initializer)
void __cdecl Fn8EB370() {
    Thiscall(0x00510DE0, 0x020C8864u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8280));
}

// 0x8EB3A0 (initializer)
void __cdecl Fn8EB3A0() {
    Thiscall(0x00510DE0, 0x020C8868u, 0x000000D7u, 0x000000D7u, 0x000000D7u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8290));
}

// 0x8EB3D0 (initializer)
void __cdecl Fn8EB3D0() {
    Thiscall(0x005FA910, 0x020C893Cu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F82A0));
}

// 0x8EB3F0 (initializer)
void __cdecl Fn8EB3F0() {
    Thiscall(0x005FA910, 0x020C8940u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F82B0));
}

// 0x8EB410 (initializer)
void __cdecl Fn8EB410() {
    Thiscall(0x005FA910, 0x020C8944u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F82C0));
}

// 0x8EB430 (initializer)
void __cdecl Fn8EB430() {
    Thiscall(0x005FA910, 0x020C8948u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F82D0));
}

// 0x8EB450 (initializer)
void __cdecl Fn8EB450() {
    Thiscall(0x005FA910, 0x020C894Cu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F82E0));
}

// 0x8EB470 (initializer)
void __cdecl Fn8EB470() {
    Thiscall(0x005FA910, 0x020C8950u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F82F0));
}

// 0x8EB490 (initializer)
void __cdecl Fn8EB490() {
    Thiscall(0x005FA910, 0x020C8954u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8300));
}

// 0x8EB4B0 (initializer)
void __cdecl Fn8EB4B0() {
    Thiscall(0x005FA910, 0x020C8958u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8310));
}

// 0x8EB4D0 (initializer)
void __cdecl Fn8EB4D0() {
    Thiscall(0x005FA910, 0x020C895Cu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8320));
}

// 0x8EB4F0 (initializer)
void __cdecl Fn8EB4F0() {
    Thiscall(0x005FA910, 0x020C8960u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8330));
}

// 0x8EB510 (initializer)
void __cdecl Fn8EB510() {
    Thiscall(0x005FA910, 0x020C8964u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8340));
}

// 0x8EB530 (initializer)
void __cdecl Fn8EB530() {
    Thiscall(0x005FA910, 0x020C8968u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8350));
}

// 0x8EB550 (initializer)
void __cdecl Fn8EB550() {
    Thiscall(0x005FA910, 0x020C896Cu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8360));
}

// 0x8EB570 (initializer)
void __cdecl Fn8EB570() {
    Thiscall(0x005FA910, 0x020C8970u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8370));
}

// 0x8EB590 (initializer)
void __cdecl Fn8EB590() {
    Thiscall(0x005FA910, 0x020C8974u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8380));
}

// 0x8EB5B0 (initializer)
void __cdecl Fn8EB5B0() {
    Thiscall(0x005FA910, 0x020C8978u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8390));
}

// 0x8EB5D0 (initializer)
void __cdecl Fn8EB5D0() {
    Thiscall(0x005FA910, 0x020C897Cu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F83A0));
}

// 0x8EB5F0 (initializer)
void __cdecl Fn8EB5F0() {
    Thiscall(0x005FA910, 0x020C8980u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F83B0));
}

// 0x8EB610 (initializer)
void __cdecl Fn8EB610() {
    Thiscall(0x005FA910, 0x020C8984u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F83C0));
}

// 0x8EB630 (initializer)
void __cdecl Fn8EB630() {
    Thiscall(0x005FA910, 0x020C8988u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F83D0));
}

// 0x8EB650 (initializer)
void __cdecl Fn8EB650() {
    Thiscall(0x005FA910, 0x020C898Cu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F83E0));
}

// 0x8EB670 (initializer)
void __cdecl Fn8EB670() {
    Thiscall(0x005FA910, 0x020C8990u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F83F0));
}

// 0x8EB690 (initializer)
void __cdecl Fn8EB690() {
    Thiscall(0x005FA910, 0x020C8994u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8400));
}

// 0x8EB6B0 (initializer)
void __cdecl Fn8EB6B0() {
    Thiscall(0x005FA910, 0x020C8998u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8410));
}

// 0x8EB6D0 (initializer)
void __cdecl Fn8EB6D0() {
    Thiscall(0x005FA910, 0x020C899Cu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8420));
}

// 0x8EB6F0 (initializer)
void __cdecl Fn8EB6F0() {
    Thiscall(0x005FA910, 0x020C89A0u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8430));
}

// 0x8EB710 (initializer)
void __cdecl Fn8EB710() {
    Thiscall(0x005FA910, 0x020C89A4u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8440));
}

// 0x8EB730 (initializer)
void __cdecl Fn8EB730() {
    Thiscall(0x005FA910, 0x020C89A8u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8450));
}

// 0x8EB750 (initializer)
void __cdecl Fn8EB750() {
    Thiscall(0x005FA910, 0x020C89ACu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8460));
}

// 0x8EB770 (initializer)
void __cdecl Fn8EB770() {
    Thiscall(0x005FA910, 0x020C89B0u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8470));
}

// 0x8EB790 (initializer)
void __cdecl Fn8EB790() {
    Thiscall(0x005FA910, 0x020C89B4u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8480));
}

// 0x8EB7B0 (initializer)
void __cdecl Fn8EB7B0() {
    Thiscall(0x005FA910, 0x020C89B8u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8490));
}

// 0x8EB7D0 (initializer)
void __cdecl Fn8EB7D0() {
    Thiscall(0x005FA910, 0x020C89BCu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F84A0));
}

// 0x8EB7F0 (initializer)
void __cdecl Fn8EB7F0() {
    Thiscall(0x005FA910, 0x020C89C0u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F84B0));
}

// 0x8EB810 (initializer)
void __cdecl Fn8EB810() {
    Thiscall(0x005FA910, 0x020C89C4u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F84C0));
}

// 0x8EB830 (initializer)
void __cdecl Fn8EB830() {
    Thiscall(0x005FA910, 0x020C89C8u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F84D0));
}

// 0x8EB850 (initializer)
void __cdecl Fn8EB850() {
    Thiscall(0x005FA910, 0x020C89CCu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F84E0));
}

// 0x8EB870 (initializer)
void __cdecl Fn8EB870() {
    Thiscall(0x005FA910, 0x020C89D0u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F84F0));
}

// 0x8EB890 (initializer)
void __cdecl Fn8EB890() {
    Thiscall(0x005FA910, 0x020C89D4u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8500));
}

// 0x8EB8B0 (initializer)
void __cdecl Fn8EB8B0() {
    Thiscall(0x005FA910, 0x020C89D8u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8510));
}

// 0x8EB8D0 (initializer)
void __cdecl Fn8EB8D0() {
    Thiscall(0x005FA910, 0x020C89DCu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8520));
}

// 0x8EB8F0 (initializer)
void __cdecl Fn8EB8F0() {
    Thiscall(0x005FA910, 0x020C89E0u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8530));
}

// 0x8EB910 (initializer)
void __cdecl Fn8EB910() {
    Thiscall(0x005FA910, 0x020C89E4u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8540));
}

// 0x8EB930 (initializer)
void __cdecl Fn8EB930() {
    Thiscall(0x005FA910, 0x020C89E8u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8550));
}

// 0x8EB950 (initializer)
void __cdecl Fn8EB950() {
    Thiscall(0x005FA910, 0x020C89ECu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8560));
}

// 0x8EB970 (initializer)
void __cdecl Fn8EB970() {
    Thiscall(0x005FA910, 0x020C89F0u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8570));
}

// 0x8EB990 (initializer)
void __cdecl Fn8EB990() {
    Thiscall(0x005FA910, 0x020C89F4u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8580));
}

// 0x8EB9B0 (initializer)
void __cdecl Fn8EB9B0() {
    Thiscall(0x005FA910, 0x020C89F8u, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8590));
}

// 0x8EB9D0 (initializer)
void __cdecl Fn8EB9D0() {
    Thiscall(0x005FA910, 0x020C89FCu, 0x0094CF4Cu /* "BUTE_CLASS" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F85A0));
}

// 0x8EB9F0 (initializer)
void __cdecl Fn8EB9F0() {
    Thiscall(0x0072BEF0, 0x020C8A00u);  // Tagging::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F85B0));
}

// 0x8EBA10 (initializer)
void __cdecl Fn8EBA10() {
    Thiscall(0x0072E350, 0x020C8BA0u);  // SurfaceStepEffects::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F85C0));
}

// 0x8EBA30 (initializer)
void __cdecl Fn8EBA30() {
    Float(0x020C902C) = float((1.0 / double(Float(0x00BC7260))));
}

// 0x8EBA50 (initializer)
void __cdecl Fn8EBA50() {
    Float(0x020C9030) = float((1.0 / (double(Float(0x00BC7260)) * double(Float(0x00BC7260)))));
}

// 0x8EBA70 (initializer)
void __cdecl Fn8EBA70() {
    Float(0x020C9034) = float((double(Float(0x020C9030)) * Double(0x00900140)));
}

// 0x8EBA90 (initializer)
void __cdecl Fn8EBA90() {
    Float(0x020C9038) = float((Double(0x00900AD8) / double(Float(0x00BC7260))));
}

// 0x8EBAB0 (initializer)
void __cdecl Fn8EBAB0() {
    Thiscall(0x00737C90, 0x020CEB78u);  // SavePoints::Construct
    Dword(0x020CECE4) = 0x00000000u;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F85D0));
}

// 0x8EBB10 (initializer)
uint32_t __cdecl Fn8EBB10() {
    return Thiscall(0x0074D980, 0x020DCE0Cu);
}

// 0x8EBB20 (initializer)
void __cdecl Fn8EBB20() {
    Thiscall(0x00765410, 0x020DCE10u, 0x00950704u /* "NiD3DXEffectShader" */, 0x020E6E58u);  // NiRTTI::NiRTTI
}

// 0x8EBB40 (initializer)
void __cdecl Fn8EBB40() {
    Thiscall(0x00765410, 0x020E2184u, 0x0095102Cu /* "NiTexture" */, 0x020E2774u);  // NiRTTI::NiRTTI
}

// 0x8EBB60 (initializer)
void __cdecl Fn8EBB60() {
    Stdcall(Dword(0x008FC084), 0x020E21B8u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F85F0));
}

// 0x8EBBC0 (initializer)
uint32_t __cdecl Fn8EBBC0() {
    return Thiscall(0x0076B280, 0x020E2200u);
}

// 0x8EBBD0 (initializer)
uint32_t __cdecl Fn8EBBD0() {
    return Thiscall(0x007861F0, 0x020E2244u);
}

// 0x8EBBE0 (initializer)
void __cdecl Fn8EBBE0() {
    Thiscall(0x00765410, 0x020E2248u, 0x00951098u /* "NiObject" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EBC00 (initializer)
void __cdecl Fn8EBC00() {
    Thiscall(0x00765410, 0x020E2384u, 0x00951238u /* "NiRenderer" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EBC20 (initializer)
void __cdecl Fn8EBC20() {
    Thiscall(0x00765410, 0x020E23BCu, 0x0095129Cu /* "NiShadowMap" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EBC40 (initializer)
void __cdecl Fn8EBC40() {
    Dword(0x020E23F0) = Dword(0x00951028);
    Dword(0x020E23F4) = Dword(0x00951028);
    Dword(0x020E23F8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8640));
}

// 0x8EBC60 (initializer)
void __cdecl Fn8EBC60() {
    Stdcall(Dword(0x008FC084), 0x020E2434u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8690));
}

// 0x8EBC80 (initializer)
void __cdecl Fn8EBC80() {
    Stdcall(Dword(0x008FC084), 0x020E2414u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F86A0));
}

// 0x8EBCA0 (initializer)
void __cdecl Fn8EBCA0() {
    Stdcall(Dword(0x008FC084), 0x020E245Cu);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F86B0));
}

// 0x8EBCC0 (initializer)
void __cdecl Fn8EBCC0() {
    Stdcall(Dword(0x008FC084), 0x020E248Cu);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F86C0));
}

// 0x8EBCE0 (initializer)
void __cdecl Fn8EBCE0() {
    Thiscall(0x00765410, 0x020E24ACu, 0x009519A4u /* "NiRenderTargetGroup" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EBD00 (initializer)
void __cdecl Fn8EBD00() {
    Thiscall(0x00765410, 0x020E24E0u, 0x00951A5Cu /* "NiRenderedTexture" */, 0x020E2184u);  // NiRTTI::NiRTTI
}

// 0x8EBD20 (initializer)
void __cdecl Fn8EBD20() {
    Thiscall(0x00765410, 0x020E2514u, 0x00951BE0u /* "NiAVObject" */, 0x020E2774u);  // NiRTTI::NiRTTI
}

// 0x8EBD40 (initializer)
void __cdecl Fn8EBD40() {
    Thiscall(0x00765410, 0x020E2548u, 0x00951CACu /* "NiNode" */, 0x020E2514u);  // NiRTTI::NiRTTI
}

// 0x8EBD60 (initializer)
void __cdecl Fn8EBD60() {
    Thiscall(0x00765410, 0x020E257Cu, 0x00951CB4u /* "NiShadowCubeMap" */, 0x020E23BCu);  // NiRTTI::NiRTTI
}

// 0x8EBD80 (initializer)
void __cdecl Fn8EBD80() {
    Dword(0x020E25B0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F86D0));
}

// 0x8EBDA0 (initializer)
void __cdecl Fn8EBDA0() {
    Thiscall(0x00765410, 0x020E25B4u, 0x00951D60u /* "NiGeometry" */, 0x020E2514u);  // NiRTTI::NiRTTI
}

// 0x8EBDC0 (initializer)
void __cdecl Fn8EBDC0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8710));
}

// 0x8EBDD0 (initializer)
void __cdecl Fn8EBDD0() {
    Thiscall(0x00765410, 0x020E2708u, 0x00951DE4u /* "NiMaterial" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EBDF0 (initializer)
void __cdecl Fn8EBDF0() {
    Thiscall(0x00765410, 0x020E273Cu, 0x00951F04u /* "NiCamera" */, 0x020E2514u);  // NiRTTI::NiRTTI
}

// 0x8EBE10 (initializer)
void __cdecl Fn8EBE10() {
    Thiscall(0x00765410, 0x020E2774u, 0x00951F74u /* "NiObjectNET" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EBE30 (initializer)
void __cdecl Fn8EBE30() {
    Thiscall(0x00765410, 0x020E27A8u, 0x00951FD0u /* "NiClickRenderStep" */, 0x020E3908u);  // NiRTTI::NiRTTI
}

// 0x8EBE50 (initializer)
void __cdecl Fn8EBE50() {
    Thiscall(0x00765410, 0x020E27DCu, 0x00951FE4u /* "NiDefaultClickRenderStep" */, 0x020E27A8u);  // NiRTTI::NiRTTI
}

// 0x8EBE70 (initializer)
void __cdecl Fn8EBE70() {
    Dword(0x020E2810) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8720));
}

// 0x8EBE90 (initializer)
void __cdecl Fn8EBE90() {
    Thiscall(0x00765410, 0x020E2820u, 0x00952000u /* "NiProperty" */, 0x020E2774u);  // NiRTTI::NiRTTI
}

// 0x8EBEB0 (initializer)
void __cdecl Fn8EBEB0() {
    Thiscall(0x00765410, 0x020E2854u, 0x0095206Cu /* "NiExtraData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EBED0 (initializer)
void __cdecl Fn8EBED0() {
    Thiscall(0x00765410, 0x020E2888u, 0x00952084u /* "NiBooleanExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EBEF0 (initializer)
void __cdecl Fn8EBEF0() {
    Thiscall(0x00765410, 0x020E28BCu, 0x009523F8u /* "NiTexturingProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EBF10 (initializer)
void __cdecl Fn8EBF10() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8760));
}

// 0x8EBF20 (initializer)
void __cdecl Fn8EBF20() {
    Dword(0x020E28F4) = Dword(0x00951028);
    Dword(0x020E28F8) = Dword(0x00951028);
    Dword(0x020E28FC) = Dword(0x00951028);
    Dword(0x020E2900) = Dword(0x00951028);
    Dword(0x020E2904) = Dword(0x00951028);
    Dword(0x020E2908) = Dword(0x00951028);
    Dword(0x020E290C) = Dword(0x00951028);
    Dword(0x020E2910) = Dword(0x00951028);
    Dword(0x020E2914) = Dword(0x00951028);
    Dword(0x020E2918) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8790));
}

// 0x8EBF70 (initializer)
void __cdecl Fn8EBF70() {
    Thiscall(0x00765410, 0x020E291Cu, 0x00952418u /* "NiFloatExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EBF90 (initializer)
void __cdecl Fn8EBF90() {
    Thiscall(0x00765410, 0x020E2950u, 0x00952438u /* "NiIntegerExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EBFB0 (initializer)
void __cdecl Fn8EBFB0() {
    Thiscall(0x00765410, 0x020E2984u, 0x009524ACu /* "NiStringExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EBFD0 (initializer)
void __cdecl Fn8EBFD0() {
    Thiscall(0x00765410, 0x020E29DCu, 0x0095254Cu /* "NiColorExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EBFF0 (initializer)
void __cdecl Fn8EBFF0() {
    Thiscall(0x00765410, 0x020E2A10u, 0x009525BCu /* "NiSourceTexture" */, 0x020E2184u);  // NiRTTI::NiRTTI
}

// 0x8EC010 (initializer)
void __cdecl Fn8EC010() {
    Thiscall(0x00765410, 0x020E2A44u, 0x0095265Cu /* "NiPixelData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC030 (initializer)
void __cdecl Fn8EC030() {
    Thiscall(0x00765410, 0x020E2A78u, 0x00952850u /* "NiTextureEffect" */, 0x020E2C00u);  // NiRTTI::NiRTTI
}

} // namespace
} // namespace staticinit

#define STATICINIT3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT3_REVERSED("StaticInit::8E59D0", 0x008E59D0, &staticinit::Fn8E59D0);
STATICINIT3_REVERSED("StaticInit::8E59F0", 0x008E59F0, &staticinit::Fn8E59F0);
STATICINIT3_REVERSED("StaticInit::8E5A10", 0x008E5A10, &staticinit::Fn8E5A10);
STATICINIT3_REVERSED("StaticInit::8E5A30", 0x008E5A30, &staticinit::Fn8E5A30);
STATICINIT3_REVERSED("StaticInit::8E5A50", 0x008E5A50, &staticinit::Fn8E5A50);
STATICINIT3_REVERSED("StaticInit::8E5A70", 0x008E5A70, &staticinit::Fn8E5A70);
STATICINIT3_REVERSED("StaticInit::8E5A90", 0x008E5A90, &staticinit::Fn8E5A90);
STATICINIT3_REVERSED("StaticInit::8E5AB0", 0x008E5AB0, &staticinit::Fn8E5AB0);
STATICINIT3_REVERSED("StaticInit::8E5AD0", 0x008E5AD0, &staticinit::Fn8E5AD0);
STATICINIT3_REVERSED("StaticInit::8E5AF0", 0x008E5AF0, &staticinit::Fn8E5AF0);
STATICINIT3_REVERSED("StaticInit::8E5B10", 0x008E5B10, &staticinit::Fn8E5B10);
STATICINIT3_REVERSED("StaticInit::8E5B30", 0x008E5B30, &staticinit::Fn8E5B30);
STATICINIT3_REVERSED("StaticInit::8E5B50", 0x008E5B50, &staticinit::Fn8E5B50);
STATICINIT3_REVERSED("StaticInit::8E5B70", 0x008E5B70, &staticinit::Fn8E5B70);
STATICINIT3_REVERSED("StaticInit::8E5B90", 0x008E5B90, &staticinit::Fn8E5B90);
STATICINIT3_REVERSED("StaticInit::8E5BB0", 0x008E5BB0, &staticinit::Fn8E5BB0);
STATICINIT3_REVERSED("StaticInit::8E5BD0", 0x008E5BD0, &staticinit::Fn8E5BD0);
STATICINIT3_REVERSED("StaticInit::8E5BF0", 0x008E5BF0, &staticinit::Fn8E5BF0);
STATICINIT3_REVERSED("StaticInit::8E5C10", 0x008E5C10, &staticinit::Fn8E5C10);
STATICINIT3_REVERSED("StaticInit::8E5C30", 0x008E5C30, &staticinit::Fn8E5C30);
STATICINIT3_REVERSED("StaticInit::8E5C50", 0x008E5C50, &staticinit::Fn8E5C50);
STATICINIT3_REVERSED("StaticInit::8E5C70", 0x008E5C70, &staticinit::Fn8E5C70);
STATICINIT3_REVERSED("StaticInit::8E5C90", 0x008E5C90, &staticinit::Fn8E5C90);
STATICINIT3_REVERSED("StaticInit::8E5CB0", 0x008E5CB0, &staticinit::Fn8E5CB0);
STATICINIT3_REVERSED("StaticInit::8E5CD0", 0x008E5CD0, &staticinit::Fn8E5CD0);
STATICINIT3_REVERSED("StaticInit::8E5CF0", 0x008E5CF0, &staticinit::Fn8E5CF0);
STATICINIT3_REVERSED("StaticInit::8E5D10", 0x008E5D10, &staticinit::Fn8E5D10);
STATICINIT3_REVERSED("StaticInit::8E5D30", 0x008E5D30, &staticinit::Fn8E5D30);
STATICINIT3_REVERSED("StaticInit::8E5D50", 0x008E5D50, &staticinit::Fn8E5D50);
STATICINIT3_REVERSED("StaticInit::8E5D70", 0x008E5D70, &staticinit::Fn8E5D70);
STATICINIT3_REVERSED("StaticInit::8E5D90", 0x008E5D90, &staticinit::Fn8E5D90);
STATICINIT3_REVERSED("StaticInit::8E5DB0", 0x008E5DB0, &staticinit::Fn8E5DB0);
STATICINIT3_REVERSED("StaticInit::8E5DD0", 0x008E5DD0, &staticinit::Fn8E5DD0);
STATICINIT3_REVERSED("StaticInit::8E5DF0", 0x008E5DF0, &staticinit::Fn8E5DF0);
STATICINIT3_REVERSED("StaticInit::8E5E10", 0x008E5E10, &staticinit::Fn8E5E10);
STATICINIT3_REVERSED("StaticInit::8E5E30", 0x008E5E30, &staticinit::Fn8E5E30);
STATICINIT3_REVERSED("StaticInit::8E5E50", 0x008E5E50, &staticinit::Fn8E5E50);
STATICINIT3_REVERSED("StaticInit::8E5E70", 0x008E5E70, &staticinit::Fn8E5E70);
STATICINIT3_REVERSED("StaticInit::8E5E90", 0x008E5E90, &staticinit::Fn8E5E90);
STATICINIT3_REVERSED("StaticInit::8E5EB0", 0x008E5EB0, &staticinit::Fn8E5EB0);
STATICINIT3_REVERSED("StaticInit::8E5ED0", 0x008E5ED0, &staticinit::Fn8E5ED0);
STATICINIT3_REVERSED("StaticInit::8E5EF0", 0x008E5EF0, &staticinit::Fn8E5EF0);
STATICINIT3_REVERSED("StaticInit::8E5F10", 0x008E5F10, &staticinit::Fn8E5F10);
STATICINIT3_REVERSED("StaticInit::8E5F30", 0x008E5F30, &staticinit::Fn8E5F30);
STATICINIT3_REVERSED("StaticInit::8E5F50", 0x008E5F50, &staticinit::Fn8E5F50);
STATICINIT3_REVERSED("StaticInit::8E5F70", 0x008E5F70, &staticinit::Fn8E5F70);
STATICINIT3_REVERSED("StaticInit::8E5F90", 0x008E5F90, &staticinit::Fn8E5F90);
STATICINIT3_REVERSED("StaticInit::8E5FB0", 0x008E5FB0, &staticinit::Fn8E5FB0);
STATICINIT3_REVERSED("StaticInit::8E5FD0", 0x008E5FD0, &staticinit::Fn8E5FD0);
STATICINIT3_REVERSED("StaticInit::8E5FF0", 0x008E5FF0, &staticinit::Fn8E5FF0);
STATICINIT3_REVERSED("StaticInit::8E6010", 0x008E6010, &staticinit::Fn8E6010);
STATICINIT3_REVERSED("StaticInit::8E6030", 0x008E6030, &staticinit::Fn8E6030);
STATICINIT3_REVERSED("StaticInit::8E6050", 0x008E6050, &staticinit::Fn8E6050);
STATICINIT3_REVERSED("StaticInit::8E6070", 0x008E6070, &staticinit::Fn8E6070);
STATICINIT3_REVERSED("StaticInit::8E6090", 0x008E6090, &staticinit::Fn8E6090);
STATICINIT3_REVERSED("StaticInit::8E60B0", 0x008E60B0, &staticinit::Fn8E60B0);
STATICINIT3_REVERSED("StaticInit::8E60D0", 0x008E60D0, &staticinit::Fn8E60D0);
STATICINIT3_REVERSED("StaticInit::8E60F0", 0x008E60F0, &staticinit::Fn8E60F0);
STATICINIT3_REVERSED("StaticInit::8E6110", 0x008E6110, &staticinit::Fn8E6110);
STATICINIT3_REVERSED("StaticInit::8E6130", 0x008E6130, &staticinit::Fn8E6130);
STATICINIT3_REVERSED("StaticInit::8E6150", 0x008E6150, &staticinit::Fn8E6150);
STATICINIT3_REVERSED("StaticInit::8E6170", 0x008E6170, &staticinit::Fn8E6170);
STATICINIT3_REVERSED("StaticInit::8E6190", 0x008E6190, &staticinit::Fn8E6190);
STATICINIT3_REVERSED("StaticInit::8E61B0", 0x008E61B0, &staticinit::Fn8E61B0);
STATICINIT3_REVERSED("StaticInit::8E61D0", 0x008E61D0, &staticinit::Fn8E61D0);
STATICINIT3_REVERSED("StaticInit::8E61F0", 0x008E61F0, &staticinit::Fn8E61F0);
STATICINIT3_REVERSED("StaticInit::8E6210", 0x008E6210, &staticinit::Fn8E6210);
STATICINIT3_REVERSED("StaticInit::8E6230", 0x008E6230, &staticinit::Fn8E6230);
STATICINIT3_REVERSED("StaticInit::8E6250", 0x008E6250, &staticinit::Fn8E6250);
STATICINIT3_REVERSED("StaticInit::8E6270", 0x008E6270, &staticinit::Fn8E6270);
STATICINIT3_REVERSED("StaticInit::8E6290", 0x008E6290, &staticinit::Fn8E6290);
STATICINIT3_REVERSED("StaticInit::8E62B0", 0x008E62B0, &staticinit::Fn8E62B0);
STATICINIT3_REVERSED("StaticInit::8E62D0", 0x008E62D0, &staticinit::Fn8E62D0);
STATICINIT3_REVERSED("StaticInit::8E62F0", 0x008E62F0, &staticinit::Fn8E62F0);
STATICINIT3_REVERSED("StaticInit::8E6310", 0x008E6310, &staticinit::Fn8E6310);
STATICINIT3_REVERSED("StaticInit::8E6330", 0x008E6330, &staticinit::Fn8E6330);
STATICINIT3_REVERSED("StaticInit::8E6350", 0x008E6350, &staticinit::Fn8E6350);
STATICINIT3_REVERSED("StaticInit::8E6370", 0x008E6370, &staticinit::Fn8E6370);
STATICINIT3_REVERSED("StaticInit::8E6390", 0x008E6390, &staticinit::Fn8E6390);
STATICINIT3_REVERSED("StaticInit::8E63B0", 0x008E63B0, &staticinit::Fn8E63B0);
STATICINIT3_REVERSED("StaticInit::8E63D0", 0x008E63D0, &staticinit::Fn8E63D0);
STATICINIT3_REVERSED("StaticInit::8E63F0", 0x008E63F0, &staticinit::Fn8E63F0);
STATICINIT3_REVERSED("StaticInit::8E6410", 0x008E6410, &staticinit::Fn8E6410);
STATICINIT3_REVERSED("StaticInit::8E6430", 0x008E6430, &staticinit::Fn8E6430);
STATICINIT3_REVERSED("StaticInit::8E6450", 0x008E6450, &staticinit::Fn8E6450);
STATICINIT3_REVERSED("StaticInit::8E6470", 0x008E6470, &staticinit::Fn8E6470);
STATICINIT3_REVERSED("StaticInit::8E6490", 0x008E6490, &staticinit::Fn8E6490);
STATICINIT3_REVERSED("StaticInit::8E64B0", 0x008E64B0, &staticinit::Fn8E64B0);
STATICINIT3_REVERSED("StaticInit::8E64D0", 0x008E64D0, &staticinit::Fn8E64D0);
STATICINIT3_REVERSED("StaticInit::8E64F0", 0x008E64F0, &staticinit::Fn8E64F0);
STATICINIT3_REVERSED("StaticInit::8E6510", 0x008E6510, &staticinit::Fn8E6510);
STATICINIT3_REVERSED("StaticInit::8E6530", 0x008E6530, &staticinit::Fn8E6530);
STATICINIT3_REVERSED("StaticInit::8E6550", 0x008E6550, &staticinit::Fn8E6550);
STATICINIT3_REVERSED("StaticInit::8E6570", 0x008E6570, &staticinit::Fn8E6570);
STATICINIT3_REVERSED("StaticInit::8E6590", 0x008E6590, &staticinit::Fn8E6590);
STATICINIT3_REVERSED("StaticInit::8E65B0", 0x008E65B0, &staticinit::Fn8E65B0);
STATICINIT3_REVERSED("StaticInit::8E65D0", 0x008E65D0, &staticinit::Fn8E65D0);
STATICINIT3_REVERSED("StaticInit::8E65F0", 0x008E65F0, &staticinit::Fn8E65F0);
STATICINIT3_REVERSED("StaticInit::8E6610", 0x008E6610, &staticinit::Fn8E6610);
STATICINIT3_REVERSED("StaticInit::8E6630", 0x008E6630, &staticinit::Fn8E6630);
STATICINIT3_REVERSED("StaticInit::8E6650", 0x008E6650, &staticinit::Fn8E6650);
STATICINIT3_REVERSED("StaticInit::8E6670", 0x008E6670, &staticinit::Fn8E6670);
STATICINIT3_REVERSED("StaticInit::8E6690", 0x008E6690, &staticinit::Fn8E6690);
STATICINIT3_REVERSED("StaticInit::8E66B0", 0x008E66B0, &staticinit::Fn8E66B0);
STATICINIT3_REVERSED("StaticInit::8E66D0", 0x008E66D0, &staticinit::Fn8E66D0);
STATICINIT3_REVERSED("StaticInit::8E66F0", 0x008E66F0, &staticinit::Fn8E66F0);
STATICINIT3_REVERSED("StaticInit::8E6710", 0x008E6710, &staticinit::Fn8E6710);
STATICINIT3_REVERSED("StaticInit::8E6730", 0x008E6730, &staticinit::Fn8E6730);
STATICINIT3_REVERSED("StaticInit::8E6750", 0x008E6750, &staticinit::Fn8E6750);
STATICINIT3_REVERSED("StaticInit::8E6770", 0x008E6770, &staticinit::Fn8E6770);
STATICINIT3_REVERSED("StaticInit::8E6790", 0x008E6790, &staticinit::Fn8E6790);
STATICINIT3_REVERSED("StaticInit::8E67B0", 0x008E67B0, &staticinit::Fn8E67B0);
STATICINIT3_REVERSED("StaticInit::8E67D0", 0x008E67D0, &staticinit::Fn8E67D0);
STATICINIT3_REVERSED("StaticInit::8E67F0", 0x008E67F0, &staticinit::Fn8E67F0);
STATICINIT3_REVERSED("StaticInit::8E6810", 0x008E6810, &staticinit::Fn8E6810);
STATICINIT3_REVERSED("StaticInit::8E6830", 0x008E6830, &staticinit::Fn8E6830);
STATICINIT3_REVERSED("StaticInit::8E6850", 0x008E6850, &staticinit::Fn8E6850);
STATICINIT3_REVERSED("StaticInit::8E6870", 0x008E6870, &staticinit::Fn8E6870);
STATICINIT3_REVERSED("StaticInit::8E6890", 0x008E6890, &staticinit::Fn8E6890);
STATICINIT3_REVERSED("StaticInit::8E68B0", 0x008E68B0, &staticinit::Fn8E68B0);
STATICINIT3_REVERSED("StaticInit::8E68D0", 0x008E68D0, &staticinit::Fn8E68D0);
STATICINIT3_REVERSED("StaticInit::8E68F0", 0x008E68F0, &staticinit::Fn8E68F0);
STATICINIT3_REVERSED("StaticInit::8E6910", 0x008E6910, &staticinit::Fn8E6910);
STATICINIT3_REVERSED("StaticInit::8E6930", 0x008E6930, &staticinit::Fn8E6930);
STATICINIT3_REVERSED("StaticInit::8E6950", 0x008E6950, &staticinit::Fn8E6950);
STATICINIT3_REVERSED("StaticInit::8E69D0", 0x008E69D0, &staticinit::Fn8E69D0);
STATICINIT3_REVERSED("StaticInit::8E6A50", 0x008E6A50, &staticinit::Fn8E6A50);
STATICINIT3_REVERSED("StaticInit::8E6AD0", 0x008E6AD0, &staticinit::Fn8E6AD0);
STATICINIT3_REVERSED("StaticInit::8E6B50", 0x008E6B50, &staticinit::Fn8E6B50);
STATICINIT3_REVERSED("StaticInit::8E6B80", 0x008E6B80, &staticinit::Fn8E6B80);
STATICINIT3_REVERSED("StaticInit::8E6BB0", 0x008E6BB0, &staticinit::Fn8E6BB0);
STATICINIT3_REVERSED("StaticInit::8E6BD0", 0x008E6BD0, &staticinit::Fn8E6BD0);
STATICINIT3_REVERSED("StaticInit::8E6BF0", 0x008E6BF0, &staticinit::Fn8E6BF0);
STATICINIT3_REVERSED("StaticInit::8E6C10", 0x008E6C10, &staticinit::Fn8E6C10);
STATICINIT3_REVERSED("StaticInit::8E6C30", 0x008E6C30, &staticinit::Fn8E6C30);
STATICINIT3_REVERSED("StaticInit::8E6C50", 0x008E6C50, &staticinit::Fn8E6C50);
STATICINIT3_REVERSED("StaticInit::8E6C70", 0x008E6C70, &staticinit::Fn8E6C70);
STATICINIT3_REVERSED("StaticInit::8E6C90", 0x008E6C90, &staticinit::Fn8E6C90);
STATICINIT3_REVERSED("StaticInit::8E6CB0", 0x008E6CB0, &staticinit::Fn8E6CB0);
STATICINIT3_REVERSED("StaticInit::8E6CD0", 0x008E6CD0, &staticinit::Fn8E6CD0);
STATICINIT3_REVERSED("StaticInit::8E6CF0", 0x008E6CF0, &staticinit::Fn8E6CF0);
STATICINIT3_REVERSED("StaticInit::8E6D10", 0x008E6D10, &staticinit::Fn8E6D10);
STATICINIT3_REVERSED("StaticInit::8E6D30", 0x008E6D30, &staticinit::Fn8E6D30);
STATICINIT3_REVERSED("StaticInit::8E6D50", 0x008E6D50, &staticinit::Fn8E6D50);
STATICINIT3_REVERSED("StaticInit::8E6D70", 0x008E6D70, &staticinit::Fn8E6D70);
STATICINIT3_REVERSED("StaticInit::8E6D90", 0x008E6D90, &staticinit::Fn8E6D90);
STATICINIT3_REVERSED("StaticInit::8E6DB0", 0x008E6DB0, &staticinit::Fn8E6DB0);
STATICINIT3_REVERSED("StaticInit::8E6DD0", 0x008E6DD0, &staticinit::Fn8E6DD0);
STATICINIT3_REVERSED("StaticInit::8E6DF0", 0x008E6DF0, &staticinit::Fn8E6DF0);
STATICINIT3_REVERSED("StaticInit::8E6E10", 0x008E6E10, &staticinit::Fn8E6E10);
STATICINIT3_REVERSED("StaticInit::8E6E30", 0x008E6E30, &staticinit::Fn8E6E30);
STATICINIT3_REVERSED("StaticInit::8E6E50", 0x008E6E50, &staticinit::Fn8E6E50);
STATICINIT3_REVERSED("StaticInit::8E6E70", 0x008E6E70, &staticinit::Fn8E6E70);
STATICINIT3_REVERSED("StaticInit::8E6E90", 0x008E6E90, &staticinit::Fn8E6E90);
STATICINIT3_REVERSED("StaticInit::8E6EB0", 0x008E6EB0, &staticinit::Fn8E6EB0);
STATICINIT3_REVERSED("StaticInit::8E6ED0", 0x008E6ED0, &staticinit::Fn8E6ED0);
STATICINIT3_REVERSED("StaticInit::8E6EF0", 0x008E6EF0, &staticinit::Fn8E6EF0);
STATICINIT3_REVERSED("StaticInit::8E6F10", 0x008E6F10, &staticinit::Fn8E6F10);
STATICINIT3_REVERSED("StaticInit::8E6F30", 0x008E6F30, &staticinit::Fn8E6F30);
STATICINIT3_REVERSED("StaticInit::8E6F50", 0x008E6F50, &staticinit::Fn8E6F50);
STATICINIT3_REVERSED("StaticInit::8E6F70", 0x008E6F70, &staticinit::Fn8E6F70);
STATICINIT3_REVERSED("StaticInit::8E6F90", 0x008E6F90, &staticinit::Fn8E6F90);
STATICINIT3_REVERSED("StaticInit::8E6FB0", 0x008E6FB0, &staticinit::Fn8E6FB0);
STATICINIT3_REVERSED("StaticInit::8E6FD0", 0x008E6FD0, &staticinit::Fn8E6FD0);
STATICINIT3_REVERSED("StaticInit::8E6FF0", 0x008E6FF0, &staticinit::Fn8E6FF0);
STATICINIT3_REVERSED("StaticInit::8E7010", 0x008E7010, &staticinit::Fn8E7010);
STATICINIT3_REVERSED("StaticInit::8E7030", 0x008E7030, &staticinit::Fn8E7030);
STATICINIT3_REVERSED("StaticInit::8E7050", 0x008E7050, &staticinit::Fn8E7050);
STATICINIT3_REVERSED("StaticInit::8E7070", 0x008E7070, &staticinit::Fn8E7070);
STATICINIT3_REVERSED("StaticInit::8E7090", 0x008E7090, &staticinit::Fn8E7090);
STATICINIT3_REVERSED("StaticInit::8E70B0", 0x008E70B0, &staticinit::Fn8E70B0);
STATICINIT3_REVERSED("StaticInit::8E70D0", 0x008E70D0, &staticinit::Fn8E70D0);
STATICINIT3_REVERSED("StaticInit::8E70F0", 0x008E70F0, &staticinit::Fn8E70F0);
STATICINIT3_REVERSED("StaticInit::8E7110", 0x008E7110, &staticinit::Fn8E7110);
STATICINIT3_REVERSED("StaticInit::8E7130", 0x008E7130, &staticinit::Fn8E7130);
STATICINIT3_REVERSED("StaticInit::8E7150", 0x008E7150, &staticinit::Fn8E7150);
STATICINIT3_REVERSED("StaticInit::8E7170", 0x008E7170, &staticinit::Fn8E7170);
STATICINIT3_REVERSED("StaticInit::8E7190", 0x008E7190, &staticinit::Fn8E7190);
STATICINIT3_REVERSED("StaticInit::8E71B0", 0x008E71B0, &staticinit::Fn8E71B0);
STATICINIT3_REVERSED("StaticInit::8E71D0", 0x008E71D0, &staticinit::Fn8E71D0);
STATICINIT3_REVERSED("StaticInit::8E71F0", 0x008E71F0, &staticinit::Fn8E71F0);
STATICINIT3_REVERSED("StaticInit::8E7210", 0x008E7210, &staticinit::Fn8E7210);
STATICINIT3_REVERSED("StaticInit::8E7230", 0x008E7230, &staticinit::Fn8E7230);
STATICINIT3_REVERSED("StaticInit::8E7250", 0x008E7250, &staticinit::Fn8E7250);
STATICINIT3_REVERSED("StaticInit::8E7270", 0x008E7270, &staticinit::Fn8E7270);
STATICINIT3_REVERSED("StaticInit::8E7290", 0x008E7290, &staticinit::Fn8E7290);
STATICINIT3_REVERSED("StaticInit::8E72B0", 0x008E72B0, &staticinit::Fn8E72B0);
STATICINIT3_REVERSED("StaticInit::8E72D0", 0x008E72D0, &staticinit::Fn8E72D0);
STATICINIT3_REVERSED("StaticInit::8E72F0", 0x008E72F0, &staticinit::Fn8E72F0);
STATICINIT3_REVERSED("StaticInit::8E7310", 0x008E7310, &staticinit::Fn8E7310);
STATICINIT3_REVERSED("StaticInit::8E7330", 0x008E7330, &staticinit::Fn8E7330);
STATICINIT3_REVERSED("StaticInit::8E7350", 0x008E7350, &staticinit::Fn8E7350);
STATICINIT3_REVERSED("StaticInit::8E7370", 0x008E7370, &staticinit::Fn8E7370);
STATICINIT3_REVERSED("StaticInit::8E7390", 0x008E7390, &staticinit::Fn8E7390);
STATICINIT3_REVERSED("StaticInit::8E73B0", 0x008E73B0, &staticinit::Fn8E73B0);
STATICINIT3_REVERSED("StaticInit::8E73D0", 0x008E73D0, &staticinit::Fn8E73D0);
STATICINIT3_REVERSED("StaticInit::8E73F0", 0x008E73F0, &staticinit::Fn8E73F0);
STATICINIT3_REVERSED("StaticInit::8E7410", 0x008E7410, &staticinit::Fn8E7410);
STATICINIT3_REVERSED("StaticInit::8E7430", 0x008E7430, &staticinit::Fn8E7430);
STATICINIT3_REVERSED("StaticInit::8E7450", 0x008E7450, &staticinit::Fn8E7450);
STATICINIT3_REVERSED("StaticInit::8E7470", 0x008E7470, &staticinit::Fn8E7470);
STATICINIT3_REVERSED("StaticInit::8E7490", 0x008E7490, &staticinit::Fn8E7490);
STATICINIT3_REVERSED("StaticInit::8E74B0", 0x008E74B0, &staticinit::Fn8E74B0);
STATICINIT3_REVERSED("StaticInit::8E74D0", 0x008E74D0, &staticinit::Fn8E74D0);
STATICINIT3_REVERSED("StaticInit::8E74F0", 0x008E74F0, &staticinit::Fn8E74F0);
STATICINIT3_REVERSED("StaticInit::8E7510", 0x008E7510, &staticinit::Fn8E7510);
STATICINIT3_REVERSED("StaticInit::8E7530", 0x008E7530, &staticinit::Fn8E7530);
STATICINIT3_REVERSED("StaticInit::8E7550", 0x008E7550, &staticinit::Fn8E7550);
STATICINIT3_REVERSED("StaticInit::8E7570", 0x008E7570, &staticinit::Fn8E7570);
STATICINIT3_REVERSED("StaticInit::8E7590", 0x008E7590, &staticinit::Fn8E7590);
STATICINIT3_REVERSED("StaticInit::8E75B0", 0x008E75B0, &staticinit::Fn8E75B0);
STATICINIT3_REVERSED("StaticInit::8E75D0", 0x008E75D0, &staticinit::Fn8E75D0);
STATICINIT3_REVERSED("StaticInit::8E75F0", 0x008E75F0, &staticinit::Fn8E75F0);
STATICINIT3_REVERSED("StaticInit::8E7610", 0x008E7610, &staticinit::Fn8E7610);
STATICINIT3_REVERSED("StaticInit::8E7630", 0x008E7630, &staticinit::Fn8E7630);
STATICINIT3_REVERSED("StaticInit::8E7650", 0x008E7650, &staticinit::Fn8E7650);
STATICINIT3_REVERSED("StaticInit::8E7670", 0x008E7670, &staticinit::Fn8E7670);
STATICINIT3_REVERSED("StaticInit::8E7690", 0x008E7690, &staticinit::Fn8E7690);
STATICINIT3_REVERSED("StaticInit::8E76B0", 0x008E76B0, &staticinit::Fn8E76B0);
STATICINIT3_REVERSED("StaticInit::8E76D0", 0x008E76D0, &staticinit::Fn8E76D0);
STATICINIT3_REVERSED("StaticInit::8E76F0", 0x008E76F0, &staticinit::Fn8E76F0);
STATICINIT3_REVERSED("StaticInit::8E7710", 0x008E7710, &staticinit::Fn8E7710);
STATICINIT3_REVERSED("StaticInit::8E7730", 0x008E7730, &staticinit::Fn8E7730);
STATICINIT3_REVERSED("StaticInit::8E7750", 0x008E7750, &staticinit::Fn8E7750);
STATICINIT3_REVERSED("StaticInit::8E7770", 0x008E7770, &staticinit::Fn8E7770);
STATICINIT3_REVERSED("StaticInit::8E7790", 0x008E7790, &staticinit::Fn8E7790);
STATICINIT3_REVERSED("StaticInit::8E77B0", 0x008E77B0, &staticinit::Fn8E77B0);
STATICINIT3_REVERSED("StaticInit::8E77D0", 0x008E77D0, &staticinit::Fn8E77D0);
STATICINIT3_REVERSED("StaticInit::8E77F0", 0x008E77F0, &staticinit::Fn8E77F0);
STATICINIT3_REVERSED("StaticInit::8E7810", 0x008E7810, &staticinit::Fn8E7810);
STATICINIT3_REVERSED("StaticInit::8E7830", 0x008E7830, &staticinit::Fn8E7830);
STATICINIT3_REVERSED("StaticInit::8E7850", 0x008E7850, &staticinit::Fn8E7850);
STATICINIT3_REVERSED("StaticInit::8E7870", 0x008E7870, &staticinit::Fn8E7870);
STATICINIT3_REVERSED("StaticInit::8E7890", 0x008E7890, &staticinit::Fn8E7890);
STATICINIT3_REVERSED("StaticInit::8E78B0", 0x008E78B0, &staticinit::Fn8E78B0);
STATICINIT3_REVERSED("StaticInit::8E78D0", 0x008E78D0, &staticinit::Fn8E78D0);
STATICINIT3_REVERSED("StaticInit::8E78F0", 0x008E78F0, &staticinit::Fn8E78F0);
STATICINIT3_REVERSED("StaticInit::8E7910", 0x008E7910, &staticinit::Fn8E7910);
STATICINIT3_REVERSED("StaticInit::8E7930", 0x008E7930, &staticinit::Fn8E7930);
STATICINIT3_REVERSED("StaticInit::8E7950", 0x008E7950, &staticinit::Fn8E7950);
STATICINIT3_REVERSED("StaticInit::8E7970", 0x008E7970, &staticinit::Fn8E7970);
STATICINIT3_REVERSED("StaticInit::8E7990", 0x008E7990, &staticinit::Fn8E7990);
STATICINIT3_REVERSED("StaticInit::8E79B0", 0x008E79B0, &staticinit::Fn8E79B0);
STATICINIT3_REVERSED("StaticInit::8E79D0", 0x008E79D0, &staticinit::Fn8E79D0);
STATICINIT3_REVERSED("StaticInit::8E79F0", 0x008E79F0, &staticinit::Fn8E79F0);
STATICINIT3_REVERSED("StaticInit::8E7A10", 0x008E7A10, &staticinit::Fn8E7A10);
STATICINIT3_REVERSED("StaticInit::8E7A30", 0x008E7A30, &staticinit::Fn8E7A30);
STATICINIT3_REVERSED("StaticInit::8E7A50", 0x008E7A50, &staticinit::Fn8E7A50);
STATICINIT3_REVERSED("StaticInit::8E7A70", 0x008E7A70, &staticinit::Fn8E7A70);
STATICINIT3_REVERSED("StaticInit::8E7A90", 0x008E7A90, &staticinit::Fn8E7A90);
STATICINIT3_REVERSED("StaticInit::8E7AB0", 0x008E7AB0, &staticinit::Fn8E7AB0);
STATICINIT3_REVERSED("StaticInit::8E7AD0", 0x008E7AD0, &staticinit::Fn8E7AD0);
STATICINIT3_REVERSED("StaticInit::8E7AF0", 0x008E7AF0, &staticinit::Fn8E7AF0);
STATICINIT3_REVERSED("StaticInit::8E7B10", 0x008E7B10, &staticinit::Fn8E7B10);
STATICINIT3_REVERSED("StaticInit::8E7B30", 0x008E7B30, &staticinit::Fn8E7B30);
STATICINIT3_REVERSED("StaticInit::8E7B50", 0x008E7B50, &staticinit::Fn8E7B50);
STATICINIT3_REVERSED("StaticInit::8E7B70", 0x008E7B70, &staticinit::Fn8E7B70);
STATICINIT3_REVERSED("StaticInit::8E7B90", 0x008E7B90, &staticinit::Fn8E7B90);
STATICINIT3_REVERSED("StaticInit::8E7BB0", 0x008E7BB0, &staticinit::Fn8E7BB0);
STATICINIT3_REVERSED("StaticInit::8E7BD0", 0x008E7BD0, &staticinit::Fn8E7BD0);
STATICINIT3_REVERSED("StaticInit::8E7BF0", 0x008E7BF0, &staticinit::Fn8E7BF0);
STATICINIT3_REVERSED("StaticInit::8E7C10", 0x008E7C10, &staticinit::Fn8E7C10);
STATICINIT3_REVERSED("StaticInit::8E7C30", 0x008E7C30, &staticinit::Fn8E7C30);
STATICINIT3_REVERSED("StaticInit::8E7C50", 0x008E7C50, &staticinit::Fn8E7C50);
STATICINIT3_REVERSED("StaticInit::8E7C70", 0x008E7C70, &staticinit::Fn8E7C70);
STATICINIT3_REVERSED("StaticInit::8E7C90", 0x008E7C90, &staticinit::Fn8E7C90);
STATICINIT3_REVERSED("StaticInit::8E7CB0", 0x008E7CB0, &staticinit::Fn8E7CB0);
STATICINIT3_REVERSED("StaticInit::8E7CD0", 0x008E7CD0, &staticinit::Fn8E7CD0);
STATICINIT3_REVERSED("StaticInit::8E7CF0", 0x008E7CF0, &staticinit::Fn8E7CF0);
STATICINIT3_REVERSED("StaticInit::8E7D10", 0x008E7D10, &staticinit::Fn8E7D10);
STATICINIT3_REVERSED("StaticInit::8E7D30", 0x008E7D30, &staticinit::Fn8E7D30);
STATICINIT3_REVERSED("StaticInit::8E7D50", 0x008E7D50, &staticinit::Fn8E7D50);
STATICINIT3_REVERSED("StaticInit::8E7D70", 0x008E7D70, &staticinit::Fn8E7D70);
STATICINIT3_REVERSED("StaticInit::8E7D90", 0x008E7D90, &staticinit::Fn8E7D90);
STATICINIT3_REVERSED("StaticInit::8E7DB0", 0x008E7DB0, &staticinit::Fn8E7DB0);
STATICINIT3_REVERSED("StaticInit::8E7DD0", 0x008E7DD0, &staticinit::Fn8E7DD0);
STATICINIT3_REVERSED("StaticInit::8E7DF0", 0x008E7DF0, &staticinit::Fn8E7DF0);
STATICINIT3_REVERSED("StaticInit::8E7E10", 0x008E7E10, &staticinit::Fn8E7E10);
STATICINIT3_REVERSED("StaticInit::8E7E30", 0x008E7E30, &staticinit::Fn8E7E30);
STATICINIT3_REVERSED("StaticInit::8E7E40", 0x008E7E40, &staticinit::Fn8E7E40);
STATICINIT3_REVERSED("StaticInit::8E7E60", 0x008E7E60, &staticinit::Fn8E7E60);
STATICINIT3_REVERSED("StaticInit::8E7E70", 0x008E7E70, &staticinit::Fn8E7E70);
STATICINIT3_REVERSED("StaticInit::8E7E90", 0x008E7E90, &staticinit::Fn8E7E90);
STATICINIT3_REVERSED("StaticInit::8E7EB0", 0x008E7EB0, &staticinit::Fn8E7EB0);
STATICINIT3_REVERSED("StaticInit::8E7ED0", 0x008E7ED0, &staticinit::Fn8E7ED0);
STATICINIT3_REVERSED("StaticInit::8E7EF0", 0x008E7EF0, &staticinit::Fn8E7EF0);
STATICINIT3_REVERSED("StaticInit::8E7F10", 0x008E7F10, &staticinit::Fn8E7F10);
STATICINIT3_REVERSED("StaticInit::8E7F30", 0x008E7F30, &staticinit::Fn8E7F30);
STATICINIT3_REVERSED("StaticInit::8E7F40", 0x008E7F40, &staticinit::Fn8E7F40);
STATICINIT3_REVERSED("StaticInit::8E7F60", 0x008E7F60, &staticinit::Fn8E7F60);
STATICINIT3_REVERSED("StaticInit::8E7F80", 0x008E7F80, &staticinit::Fn8E7F80);
STATICINIT3_REVERSED("StaticInit::8E7FA0", 0x008E7FA0, &staticinit::Fn8E7FA0);
STATICINIT3_REVERSED("StaticInit::8E7FB0", 0x008E7FB0, &staticinit::Fn8E7FB0);
STATICINIT3_REVERSED("StaticInit::8E7FD0", 0x008E7FD0, &staticinit::Fn8E7FD0);
STATICINIT3_REVERSED("StaticInit::8E7FF0", 0x008E7FF0, &staticinit::Fn8E7FF0);
STATICINIT3_REVERSED("StaticInit::8E8010", 0x008E8010, &staticinit::Fn8E8010);
STATICINIT3_REVERSED("StaticInit::8E8030", 0x008E8030, &staticinit::Fn8E8030);
STATICINIT3_REVERSED("StaticInit::8E8050", 0x008E8050, &staticinit::Fn8E8050);
STATICINIT3_REVERSED("StaticInit::8E8070", 0x008E8070, &staticinit::Fn8E8070);
STATICINIT3_REVERSED("StaticInit::8E8090", 0x008E8090, &staticinit::Fn8E8090);
STATICINIT3_REVERSED("StaticInit::8E80B0", 0x008E80B0, &staticinit::Fn8E80B0);
STATICINIT3_REVERSED("StaticInit::8E80D0", 0x008E80D0, &staticinit::Fn8E80D0);
STATICINIT3_REVERSED("StaticInit::8E80F0", 0x008E80F0, &staticinit::Fn8E80F0);
STATICINIT3_REVERSED("StaticInit::8E8110", 0x008E8110, &staticinit::Fn8E8110);
STATICINIT3_REVERSED("StaticInit::8E8130", 0x008E8130, &staticinit::Fn8E8130);
STATICINIT3_REVERSED("StaticInit::8E8150", 0x008E8150, &staticinit::Fn8E8150);
STATICINIT3_REVERSED("StaticInit::8E8170", 0x008E8170, &staticinit::Fn8E8170);
STATICINIT3_REVERSED("StaticInit::8E8190", 0x008E8190, &staticinit::Fn8E8190);
STATICINIT3_REVERSED("StaticInit::8E81B0", 0x008E81B0, &staticinit::Fn8E81B0);
STATICINIT3_REVERSED("StaticInit::8E81D0", 0x008E81D0, &staticinit::Fn8E81D0);
STATICINIT3_REVERSED("StaticInit::8E81F0", 0x008E81F0, &staticinit::Fn8E81F0);
STATICINIT3_REVERSED("StaticInit::8E8210", 0x008E8210, &staticinit::Fn8E8210);
STATICINIT3_REVERSED("StaticInit::8E8230", 0x008E8230, &staticinit::Fn8E8230);
STATICINIT3_REVERSED("StaticInit::8E8250", 0x008E8250, &staticinit::Fn8E8250);
STATICINIT3_REVERSED("StaticInit::8E8270", 0x008E8270, &staticinit::Fn8E8270);
STATICINIT3_REVERSED("StaticInit::8E8290", 0x008E8290, &staticinit::Fn8E8290);
STATICINIT3_REVERSED("StaticInit::8E82B0", 0x008E82B0, &staticinit::Fn8E82B0);
STATICINIT3_REVERSED("StaticInit::8E82D0", 0x008E82D0, &staticinit::Fn8E82D0);
STATICINIT3_REVERSED("StaticInit::8E82F0", 0x008E82F0, &staticinit::Fn8E82F0);
STATICINIT3_REVERSED("StaticInit::8E8310", 0x008E8310, &staticinit::Fn8E8310);
STATICINIT3_REVERSED("StaticInit::8E8330", 0x008E8330, &staticinit::Fn8E8330);
STATICINIT3_REVERSED("StaticInit::8E8350", 0x008E8350, &staticinit::Fn8E8350);
STATICINIT3_REVERSED("StaticInit::8E8370", 0x008E8370, &staticinit::Fn8E8370);
STATICINIT3_REVERSED("StaticInit::8E8390", 0x008E8390, &staticinit::Fn8E8390);
STATICINIT3_REVERSED("StaticInit::8E83B0", 0x008E83B0, &staticinit::Fn8E83B0);
STATICINIT3_REVERSED("StaticInit::8E83D0", 0x008E83D0, &staticinit::Fn8E83D0);
STATICINIT3_REVERSED("StaticInit::8E83F0", 0x008E83F0, &staticinit::Fn8E83F0);
STATICINIT3_REVERSED("StaticInit::8E8410", 0x008E8410, &staticinit::Fn8E8410);
STATICINIT3_REVERSED("StaticInit::8E8430", 0x008E8430, &staticinit::Fn8E8430);
STATICINIT3_REVERSED("StaticInit::8E8450", 0x008E8450, &staticinit::Fn8E8450);
STATICINIT3_REVERSED("StaticInit::8E8470", 0x008E8470, &staticinit::Fn8E8470);
STATICINIT3_REVERSED("StaticInit::8E8490", 0x008E8490, &staticinit::Fn8E8490);
STATICINIT3_REVERSED("StaticInit::8E84B0", 0x008E84B0, &staticinit::Fn8E84B0);
STATICINIT3_REVERSED("StaticInit::8E84D0", 0x008E84D0, &staticinit::Fn8E84D0);
STATICINIT3_REVERSED("StaticInit::8E84F0", 0x008E84F0, &staticinit::Fn8E84F0);
STATICINIT3_REVERSED("StaticInit::8E8510", 0x008E8510, &staticinit::Fn8E8510);
STATICINIT3_REVERSED("StaticInit::8E8520", 0x008E8520, &staticinit::Fn8E8520);
STATICINIT3_REVERSED("StaticInit::8E8540", 0x008E8540, &staticinit::Fn8E8540);
STATICINIT3_REVERSED("StaticInit::8E8560", 0x008E8560, &staticinit::Fn8E8560);
STATICINIT3_REVERSED("StaticInit::8E8580", 0x008E8580, &staticinit::Fn8E8580);
STATICINIT3_REVERSED("StaticInit::8E85A0", 0x008E85A0, &staticinit::Fn8E85A0);
STATICINIT3_REVERSED("StaticInit::8E85C0", 0x008E85C0, &staticinit::Fn8E85C0);
STATICINIT3_REVERSED("StaticInit::8E85E0", 0x008E85E0, &staticinit::Fn8E85E0);
STATICINIT3_REVERSED("StaticInit::8E8600", 0x008E8600, &staticinit::Fn8E8600);
STATICINIT3_REVERSED("StaticInit::8E8620", 0x008E8620, &staticinit::Fn8E8620);
STATICINIT3_REVERSED("StaticInit::8E8640", 0x008E8640, &staticinit::Fn8E8640);
STATICINIT3_REVERSED("StaticInit::8E8660", 0x008E8660, &staticinit::Fn8E8660);
STATICINIT3_REVERSED("StaticInit::8E8680", 0x008E8680, &staticinit::Fn8E8680);
STATICINIT3_REVERSED("StaticInit::8E8690", 0x008E8690, &staticinit::Fn8E8690);
STATICINIT3_REVERSED("StaticInit::8E86B0", 0x008E86B0, &staticinit::Fn8E86B0);
STATICINIT3_REVERSED("StaticInit::8E86D0", 0x008E86D0, &staticinit::Fn8E86D0);
STATICINIT3_REVERSED("StaticInit::8E86F0", 0x008E86F0, &staticinit::Fn8E86F0);
STATICINIT3_REVERSED("StaticInit::8E8710", 0x008E8710, &staticinit::Fn8E8710);
STATICINIT3_REVERSED("StaticInit::8E8730", 0x008E8730, &staticinit::Fn8E8730);
STATICINIT3_REVERSED("StaticInit::8E8750", 0x008E8750, &staticinit::Fn8E8750);
STATICINIT3_REVERSED("StaticInit::8E8770", 0x008E8770, &staticinit::Fn8E8770);
STATICINIT3_REVERSED("StaticInit::8E8790", 0x008E8790, &staticinit::Fn8E8790);
STATICINIT3_REVERSED("StaticInit::8E87B0", 0x008E87B0, &staticinit::Fn8E87B0);
STATICINIT3_REVERSED("StaticInit::8E87D0", 0x008E87D0, &staticinit::Fn8E87D0);
STATICINIT3_REVERSED("StaticInit::8E87F0", 0x008E87F0, &staticinit::Fn8E87F0);
STATICINIT3_REVERSED("StaticInit::8E8810", 0x008E8810, &staticinit::Fn8E8810);
STATICINIT3_REVERSED("StaticInit::8E8830", 0x008E8830, &staticinit::Fn8E8830);
STATICINIT3_REVERSED("StaticInit::8E8850", 0x008E8850, &staticinit::Fn8E8850);
STATICINIT3_REVERSED("StaticInit::8E8870", 0x008E8870, &staticinit::Fn8E8870);
STATICINIT3_REVERSED("StaticInit::8E8890", 0x008E8890, &staticinit::Fn8E8890);
STATICINIT3_REVERSED("StaticInit::8E88B0", 0x008E88B0, &staticinit::Fn8E88B0);
STATICINIT3_REVERSED("StaticInit::8E88D0", 0x008E88D0, &staticinit::Fn8E88D0);
STATICINIT3_REVERSED("StaticInit::8E88F0", 0x008E88F0, &staticinit::Fn8E88F0);
STATICINIT3_REVERSED("StaticInit::8E8910", 0x008E8910, &staticinit::Fn8E8910);
STATICINIT3_REVERSED("StaticInit::8E8920", 0x008E8920, &staticinit::Fn8E8920);
STATICINIT3_REVERSED("StaticInit::8E8930", 0x008E8930, &staticinit::Fn8E8930);
STATICINIT3_REVERSED("StaticInit::8E89D0", 0x008E89D0, &staticinit::Fn8E89D0);
STATICINIT3_REVERSED("StaticInit::8E89E0", 0x008E89E0, &staticinit::Fn8E89E0);
STATICINIT3_REVERSED("StaticInit::8E8A00", 0x008E8A00, &staticinit::Fn8E8A00);
STATICINIT3_REVERSED("StaticInit::8E8A20", 0x008E8A20, &staticinit::Fn8E8A20);
STATICINIT3_REVERSED("StaticInit::8E8A40", 0x008E8A40, &staticinit::Fn8E8A40);
STATICINIT3_REVERSED("StaticInit::8E8A50", 0x008E8A50, &staticinit::Fn8E8A50);
STATICINIT3_REVERSED("StaticInit::8E8A80", 0x008E8A80, &staticinit::Fn8E8A80);
STATICINIT3_REVERSED("StaticInit::8E8AB0", 0x008E8AB0, &staticinit::Fn8E8AB0);
STATICINIT3_REVERSED("StaticInit::8E8AD0", 0x008E8AD0, &staticinit::Fn8E8AD0);
STATICINIT3_REVERSED("StaticInit::8E8AF0", 0x008E8AF0, &staticinit::Fn8E8AF0);
STATICINIT3_REVERSED("StaticInit::8E8B10", 0x008E8B10, &staticinit::Fn8E8B10);
STATICINIT3_REVERSED("StaticInit::8E8B30", 0x008E8B30, &staticinit::Fn8E8B30);
STATICINIT3_REVERSED("StaticInit::8E8B50", 0x008E8B50, &staticinit::Fn8E8B50);
STATICINIT3_REVERSED("StaticInit::8E8B70", 0x008E8B70, &staticinit::Fn8E8B70);
STATICINIT3_REVERSED("StaticInit::8E8B90", 0x008E8B90, &staticinit::Fn8E8B90);
STATICINIT3_REVERSED("StaticInit::8E8BB0", 0x008E8BB0, &staticinit::Fn8E8BB0);
STATICINIT3_REVERSED("StaticInit::8E8BD0", 0x008E8BD0, &staticinit::Fn8E8BD0);
STATICINIT3_REVERSED("StaticInit::8E8BF0", 0x008E8BF0, &staticinit::Fn8E8BF0);
STATICINIT3_REVERSED("StaticInit::8E8C10", 0x008E8C10, &staticinit::Fn8E8C10);
STATICINIT3_REVERSED("StaticInit::8E8C30", 0x008E8C30, &staticinit::Fn8E8C30);
STATICINIT3_REVERSED("StaticInit::8E8D70", 0x008E8D70, &staticinit::Fn8E8D70);
STATICINIT3_REVERSED("StaticInit::8E8D90", 0x008E8D90, &staticinit::Fn8E8D90);
STATICINIT3_REVERSED("StaticInit::8E8DB0", 0x008E8DB0, &staticinit::Fn8E8DB0);
STATICINIT3_REVERSED("StaticInit::8E8DD0", 0x008E8DD0, &staticinit::Fn8E8DD0);
STATICINIT3_REVERSED("StaticInit::8E8DF0", 0x008E8DF0, &staticinit::Fn8E8DF0);
STATICINIT3_REVERSED("StaticInit::8E8E10", 0x008E8E10, &staticinit::Fn8E8E10);
STATICINIT3_REVERSED("StaticInit::8E8E30", 0x008E8E30, &staticinit::Fn8E8E30);
STATICINIT3_REVERSED("StaticInit::8E8E50", 0x008E8E50, &staticinit::Fn8E8E50);
STATICINIT3_REVERSED("StaticInit::8E9210", 0x008E9210, &staticinit::Fn8E9210);
STATICINIT3_REVERSED("StaticInit::8E9220", 0x008E9220, &staticinit::Fn8E9220);
STATICINIT3_REVERSED("StaticInit::8E9230", 0x008E9230, &staticinit::Fn8E9230);
STATICINIT3_REVERSED("StaticInit::8E9250", 0x008E9250, &staticinit::Fn8E9250);
STATICINIT3_REVERSED("StaticInit::8E9280", 0x008E9280, &staticinit::Fn8E9280);
STATICINIT3_REVERSED("StaticInit::8E92B0", 0x008E92B0, &staticinit::Fn8E92B0);
STATICINIT3_REVERSED("StaticInit::8E92E0", 0x008E92E0, &staticinit::Fn8E92E0);
STATICINIT3_REVERSED("StaticInit::8E9310", 0x008E9310, &staticinit::Fn8E9310);
STATICINIT3_REVERSED("StaticInit::8E9340", 0x008E9340, &staticinit::Fn8E9340);
STATICINIT3_REVERSED("StaticInit::8E9360", 0x008E9360, &staticinit::Fn8E9360);
STATICINIT3_REVERSED("StaticInit::8E9390", 0x008E9390, &staticinit::Fn8E9390);
STATICINIT3_REVERSED("StaticInit::8E93C0", 0x008E93C0, &staticinit::Fn8E93C0);
STATICINIT3_REVERSED("StaticInit::8E93F0", 0x008E93F0, &staticinit::Fn8E93F0);
STATICINIT3_REVERSED("StaticInit::8E9420", 0x008E9420, &staticinit::Fn8E9420);
STATICINIT3_REVERSED("StaticInit::8E9450", 0x008E9450, &staticinit::Fn8E9450);
STATICINIT3_REVERSED("StaticInit::8E9480", 0x008E9480, &staticinit::Fn8E9480);
STATICINIT3_REVERSED("StaticInit::8E94B0", 0x008E94B0, &staticinit::Fn8E94B0);
STATICINIT3_REVERSED("StaticInit::8E94D0", 0x008E94D0, &staticinit::Fn8E94D0);
STATICINIT3_REVERSED("StaticInit::8E94F0", 0x008E94F0, &staticinit::Fn8E94F0);
STATICINIT3_REVERSED("StaticInit::8E9510", 0x008E9510, &staticinit::Fn8E9510);
STATICINIT3_REVERSED("StaticInit::8E9530", 0x008E9530, &staticinit::Fn8E9530);
STATICINIT3_REVERSED("StaticInit::8E9550", 0x008E9550, &staticinit::Fn8E9550);
STATICINIT3_REVERSED("StaticInit::8E9570", 0x008E9570, &staticinit::Fn8E9570);
STATICINIT3_REVERSED("StaticInit::8E9590", 0x008E9590, &staticinit::Fn8E9590);
STATICINIT3_REVERSED("StaticInit::8E95B0", 0x008E95B0, &staticinit::Fn8E95B0);
STATICINIT3_REVERSED("StaticInit::8E95D0", 0x008E95D0, &staticinit::Fn8E95D0);
STATICINIT3_REVERSED("StaticInit::8E95F0", 0x008E95F0, &staticinit::Fn8E95F0);
STATICINIT3_REVERSED("StaticInit::8E9610", 0x008E9610, &staticinit::Fn8E9610);
STATICINIT3_REVERSED("StaticInit::8E9630", 0x008E9630, &staticinit::Fn8E9630);
STATICINIT3_REVERSED("StaticInit::8E9650", 0x008E9650, &staticinit::Fn8E9650);
STATICINIT3_REVERSED("StaticInit::8E9670", 0x008E9670, &staticinit::Fn8E9670);
STATICINIT3_REVERSED("StaticInit::8E9690", 0x008E9690, &staticinit::Fn8E9690);
STATICINIT3_REVERSED("StaticInit::8E96B0", 0x008E96B0, &staticinit::Fn8E96B0);
STATICINIT3_REVERSED("StaticInit::8E96D0", 0x008E96D0, &staticinit::Fn8E96D0);
STATICINIT3_REVERSED("StaticInit::8E96F0", 0x008E96F0, &staticinit::Fn8E96F0);
STATICINIT3_REVERSED("StaticInit::8E9710", 0x008E9710, &staticinit::Fn8E9710);
STATICINIT3_REVERSED("StaticInit::8E9720", 0x008E9720, &staticinit::Fn8E9720);
STATICINIT3_REVERSED("StaticInit::8E9740", 0x008E9740, &staticinit::Fn8E9740);
STATICINIT3_REVERSED("StaticInit::8E9760", 0x008E9760, &staticinit::Fn8E9760);
STATICINIT3_REVERSED("StaticInit::8E9780", 0x008E9780, &staticinit::Fn8E9780);
STATICINIT3_REVERSED("StaticInit::8E97A0", 0x008E97A0, &staticinit::Fn8E97A0);
STATICINIT3_REVERSED("StaticInit::8E97C0", 0x008E97C0, &staticinit::Fn8E97C0);
STATICINIT3_REVERSED("StaticInit::8E97E0", 0x008E97E0, &staticinit::Fn8E97E0);
STATICINIT3_REVERSED("StaticInit::8E9800", 0x008E9800, &staticinit::Fn8E9800);
STATICINIT3_REVERSED("StaticInit::8E9820", 0x008E9820, &staticinit::Fn8E9820);
STATICINIT3_REVERSED("StaticInit::8E9840", 0x008E9840, &staticinit::Fn8E9840);
STATICINIT3_REVERSED("StaticInit::8E9850", 0x008E9850, &staticinit::Fn8E9850);
STATICINIT3_REVERSED("StaticInit::8E9870", 0x008E9870, &staticinit::Fn8E9870);
STATICINIT3_REVERSED("StaticInit::8E9890", 0x008E9890, &staticinit::Fn8E9890);
STATICINIT3_REVERSED("StaticInit::8E98A0", 0x008E98A0, &staticinit::Fn8E98A0);
STATICINIT3_REVERSED("StaticInit::8E98C0", 0x008E98C0, &staticinit::Fn8E98C0);
STATICINIT3_REVERSED("StaticInit::8E98E0", 0x008E98E0, &staticinit::Fn8E98E0);
STATICINIT3_REVERSED("StaticInit::8E9900", 0x008E9900, &staticinit::Fn8E9900);
STATICINIT3_REVERSED("StaticInit::8E9950", 0x008E9950, &staticinit::Fn8E9950);
STATICINIT3_REVERSED("StaticInit::8E9960", 0x008E9960, &staticinit::Fn8E9960);
STATICINIT3_REVERSED("StaticInit::8E9970", 0x008E9970, &staticinit::Fn8E9970);
STATICINIT3_REVERSED("StaticInit::8E9990", 0x008E9990, &staticinit::Fn8E9990);
STATICINIT3_REVERSED("StaticInit::8E99B0", 0x008E99B0, &staticinit::Fn8E99B0);
STATICINIT3_REVERSED("StaticInit::8E99D0", 0x008E99D0, &staticinit::Fn8E99D0);
STATICINIT3_REVERSED("StaticInit::8E99F0", 0x008E99F0, &staticinit::Fn8E99F0);
STATICINIT3_REVERSED("StaticInit::8E9A10", 0x008E9A10, &staticinit::Fn8E9A10);
STATICINIT3_REVERSED("StaticInit::8E9A20", 0x008E9A20, &staticinit::Fn8E9A20);
STATICINIT3_REVERSED("StaticInit::8E9A30", 0x008E9A30, &staticinit::Fn8E9A30);
STATICINIT3_REVERSED("StaticInit::8E9A50", 0x008E9A50, &staticinit::Fn8E9A50);
STATICINIT3_REVERSED("StaticInit::8E9A70", 0x008E9A70, &staticinit::Fn8E9A70);
STATICINIT3_REVERSED("StaticInit::8E9A90", 0x008E9A90, &staticinit::Fn8E9A90);
STATICINIT3_REVERSED("StaticInit::8E9AB0", 0x008E9AB0, &staticinit::Fn8E9AB0);
STATICINIT3_REVERSED("StaticInit::8E9AD0", 0x008E9AD0, &staticinit::Fn8E9AD0);
STATICINIT3_REVERSED("StaticInit::8E9AF0", 0x008E9AF0, &staticinit::Fn8E9AF0);
STATICINIT3_REVERSED("StaticInit::8E9B10", 0x008E9B10, &staticinit::Fn8E9B10);
STATICINIT3_REVERSED("StaticInit::8E9B30", 0x008E9B30, &staticinit::Fn8E9B30);
STATICINIT3_REVERSED("StaticInit::8E9B60", 0x008E9B60, &staticinit::Fn8E9B60);
STATICINIT3_REVERSED("StaticInit::8E9B80", 0x008E9B80, &staticinit::Fn8E9B80);
STATICINIT3_REVERSED("StaticInit::8E9BC0", 0x008E9BC0, &staticinit::Fn8E9BC0);
STATICINIT3_REVERSED("StaticInit::8E9BD0", 0x008E9BD0, &staticinit::Fn8E9BD0);
STATICINIT3_REVERSED("StaticInit::8E9BE0", 0x008E9BE0, &staticinit::Fn8E9BE0);
STATICINIT3_REVERSED("StaticInit::8E9BF0", 0x008E9BF0, &staticinit::Fn8E9BF0);
STATICINIT3_REVERSED("StaticInit::8E9C00", 0x008E9C00, &staticinit::Fn8E9C00);
STATICINIT3_REVERSED("StaticInit::8E9C10", 0x008E9C10, &staticinit::Fn8E9C10);
STATICINIT3_REVERSED("StaticInit::8E9C20", 0x008E9C20, &staticinit::Fn8E9C20);
STATICINIT3_REVERSED("StaticInit::8E9C30", 0x008E9C30, &staticinit::Fn8E9C30);
STATICINIT3_REVERSED("StaticInit::8E9C50", 0x008E9C50, &staticinit::Fn8E9C50);
STATICINIT3_REVERSED("StaticInit::8E9CB0", 0x008E9CB0, &staticinit::Fn8E9CB0);
STATICINIT3_REVERSED("StaticInit::8E9CE0", 0x008E9CE0, &staticinit::Fn8E9CE0);
STATICINIT3_REVERSED("StaticInit::8E9D10", 0x008E9D10, &staticinit::Fn8E9D10);
STATICINIT3_REVERSED("StaticInit::8E9D40", 0x008E9D40, &staticinit::Fn8E9D40);
STATICINIT3_REVERSED("StaticInit::8E9D70", 0x008E9D70, &staticinit::Fn8E9D70);
STATICINIT3_REVERSED("StaticInit::8E9DA0", 0x008E9DA0, &staticinit::Fn8E9DA0);
STATICINIT3_REVERSED("StaticInit::8E9DD0", 0x008E9DD0, &staticinit::Fn8E9DD0);
STATICINIT3_REVERSED("StaticInit::8E9E00", 0x008E9E00, &staticinit::Fn8E9E00);
STATICINIT3_REVERSED("StaticInit::8E9E30", 0x008E9E30, &staticinit::Fn8E9E30);
STATICINIT3_REVERSED("StaticInit::8E9E60", 0x008E9E60, &staticinit::Fn8E9E60);
STATICINIT3_REVERSED("StaticInit::8E9E90", 0x008E9E90, &staticinit::Fn8E9E90);
STATICINIT3_REVERSED("StaticInit::8E9EC0", 0x008E9EC0, &staticinit::Fn8E9EC0);
STATICINIT3_REVERSED("StaticInit::8E9EF0", 0x008E9EF0, &staticinit::Fn8E9EF0);
STATICINIT3_REVERSED("StaticInit::8E9F20", 0x008E9F20, &staticinit::Fn8E9F20);
STATICINIT3_REVERSED("StaticInit::8E9F50", 0x008E9F50, &staticinit::Fn8E9F50);
STATICINIT3_REVERSED("StaticInit::8E9F80", 0x008E9F80, &staticinit::Fn8E9F80);
STATICINIT3_REVERSED("StaticInit::8E9FB0", 0x008E9FB0, &staticinit::Fn8E9FB0);
STATICINIT3_REVERSED("StaticInit::8E9FE0", 0x008E9FE0, &staticinit::Fn8E9FE0);
STATICINIT3_REVERSED("StaticInit::8EA010", 0x008EA010, &staticinit::Fn8EA010);
STATICINIT3_REVERSED("StaticInit::8EA040", 0x008EA040, &staticinit::Fn8EA040);
STATICINIT3_REVERSED("StaticInit::8EA070", 0x008EA070, &staticinit::Fn8EA070);
STATICINIT3_REVERSED("StaticInit::8EA0A0", 0x008EA0A0, &staticinit::Fn8EA0A0);
STATICINIT3_REVERSED("StaticInit::8EA0D0", 0x008EA0D0, &staticinit::Fn8EA0D0);
STATICINIT3_REVERSED("StaticInit::8EA100", 0x008EA100, &staticinit::Fn8EA100);
STATICINIT3_REVERSED("StaticInit::8EA120", 0x008EA120, &staticinit::Fn8EA120);
STATICINIT3_REVERSED("StaticInit::8EA150", 0x008EA150, &staticinit::Fn8EA150);
STATICINIT3_REVERSED("StaticInit::8EA180", 0x008EA180, &staticinit::Fn8EA180);
STATICINIT3_REVERSED("StaticInit::8EA1B0", 0x008EA1B0, &staticinit::Fn8EA1B0);
STATICINIT3_REVERSED("StaticInit::8EA1E0", 0x008EA1E0, &staticinit::Fn8EA1E0);
STATICINIT3_REVERSED("StaticInit::8EA210", 0x008EA210, &staticinit::Fn8EA210);
STATICINIT3_REVERSED("StaticInit::8EA240", 0x008EA240, &staticinit::Fn8EA240);
STATICINIT3_REVERSED("StaticInit::8EA270", 0x008EA270, &staticinit::Fn8EA270);
STATICINIT3_REVERSED("StaticInit::8EA2A0", 0x008EA2A0, &staticinit::Fn8EA2A0);
STATICINIT3_REVERSED("StaticInit::8EA2D0", 0x008EA2D0, &staticinit::Fn8EA2D0);
STATICINIT3_REVERSED("StaticInit::8EA300", 0x008EA300, &staticinit::Fn8EA300);
STATICINIT3_REVERSED("StaticInit::8EA330", 0x008EA330, &staticinit::Fn8EA330);
STATICINIT3_REVERSED("StaticInit::8EA360", 0x008EA360, &staticinit::Fn8EA360);
STATICINIT3_REVERSED("StaticInit::8EA390", 0x008EA390, &staticinit::Fn8EA390);
STATICINIT3_REVERSED("StaticInit::8EA3C0", 0x008EA3C0, &staticinit::Fn8EA3C0);
STATICINIT3_REVERSED("StaticInit::8EA3F0", 0x008EA3F0, &staticinit::Fn8EA3F0);
STATICINIT3_REVERSED("StaticInit::8EA420", 0x008EA420, &staticinit::Fn8EA420);
STATICINIT3_REVERSED("StaticInit::8EA450", 0x008EA450, &staticinit::Fn8EA450);
STATICINIT3_REVERSED("StaticInit::8EA480", 0x008EA480, &staticinit::Fn8EA480);
STATICINIT3_REVERSED("StaticInit::8EA4B0", 0x008EA4B0, &staticinit::Fn8EA4B0);
STATICINIT3_REVERSED("StaticInit::8EA4E0", 0x008EA4E0, &staticinit::Fn8EA4E0);
STATICINIT3_REVERSED("StaticInit::8EA510", 0x008EA510, &staticinit::Fn8EA510);
STATICINIT3_REVERSED("StaticInit::8EA540", 0x008EA540, &staticinit::Fn8EA540);
STATICINIT3_REVERSED("StaticInit::8EA570", 0x008EA570, &staticinit::Fn8EA570);
STATICINIT3_REVERSED("StaticInit::8EA5A0", 0x008EA5A0, &staticinit::Fn8EA5A0);
STATICINIT3_REVERSED("StaticInit::8EA5D0", 0x008EA5D0, &staticinit::Fn8EA5D0);
STATICINIT3_REVERSED("StaticInit::8EA600", 0x008EA600, &staticinit::Fn8EA600);
STATICINIT3_REVERSED("StaticInit::8EA630", 0x008EA630, &staticinit::Fn8EA630);
STATICINIT3_REVERSED("StaticInit::8EA660", 0x008EA660, &staticinit::Fn8EA660);
STATICINIT3_REVERSED("StaticInit::8EA690", 0x008EA690, &staticinit::Fn8EA690);
STATICINIT3_REVERSED("StaticInit::8EA6C0", 0x008EA6C0, &staticinit::Fn8EA6C0);
STATICINIT3_REVERSED("StaticInit::8EA6F0", 0x008EA6F0, &staticinit::Fn8EA6F0);
STATICINIT3_REVERSED("StaticInit::8EA720", 0x008EA720, &staticinit::Fn8EA720);
STATICINIT3_REVERSED("StaticInit::8EA750", 0x008EA750, &staticinit::Fn8EA750);
STATICINIT3_REVERSED("StaticInit::8EA780", 0x008EA780, &staticinit::Fn8EA780);
STATICINIT3_REVERSED("StaticInit::8EA7B0", 0x008EA7B0, &staticinit::Fn8EA7B0);
STATICINIT3_REVERSED("StaticInit::8EA7E0", 0x008EA7E0, &staticinit::Fn8EA7E0);
STATICINIT3_REVERSED("StaticInit::8EA810", 0x008EA810, &staticinit::Fn8EA810);
STATICINIT3_REVERSED("StaticInit::8EA840", 0x008EA840, &staticinit::Fn8EA840);
STATICINIT3_REVERSED("StaticInit::8EA870", 0x008EA870, &staticinit::Fn8EA870);
STATICINIT3_REVERSED("StaticInit::8EA8A0", 0x008EA8A0, &staticinit::Fn8EA8A0);
STATICINIT3_REVERSED("StaticInit::8EA8D0", 0x008EA8D0, &staticinit::Fn8EA8D0);
STATICINIT3_REVERSED("StaticInit::8EA900", 0x008EA900, &staticinit::Fn8EA900);
STATICINIT3_REVERSED("StaticInit::8EA930", 0x008EA930, &staticinit::Fn8EA930);
STATICINIT3_REVERSED("StaticInit::8EA960", 0x008EA960, &staticinit::Fn8EA960);
STATICINIT3_REVERSED("StaticInit::8EA990", 0x008EA990, &staticinit::Fn8EA990);
STATICINIT3_REVERSED("StaticInit::8EA9C0", 0x008EA9C0, &staticinit::Fn8EA9C0);
STATICINIT3_REVERSED("StaticInit::8EA9F0", 0x008EA9F0, &staticinit::Fn8EA9F0);
STATICINIT3_REVERSED("StaticInit::8EAA20", 0x008EAA20, &staticinit::Fn8EAA20);
STATICINIT3_REVERSED("StaticInit::8EAA50", 0x008EAA50, &staticinit::Fn8EAA50);
STATICINIT3_REVERSED("StaticInit::8EAA80", 0x008EAA80, &staticinit::Fn8EAA80);
STATICINIT3_REVERSED("StaticInit::8EAAB0", 0x008EAAB0, &staticinit::Fn8EAAB0);
STATICINIT3_REVERSED("StaticInit::8EAAE0", 0x008EAAE0, &staticinit::Fn8EAAE0);
STATICINIT3_REVERSED("StaticInit::8EAB10", 0x008EAB10, &staticinit::Fn8EAB10);
STATICINIT3_REVERSED("StaticInit::8EAB40", 0x008EAB40, &staticinit::Fn8EAB40);
STATICINIT3_REVERSED("StaticInit::8EAB70", 0x008EAB70, &staticinit::Fn8EAB70);
STATICINIT3_REVERSED("StaticInit::8EABA0", 0x008EABA0, &staticinit::Fn8EABA0);
STATICINIT3_REVERSED("StaticInit::8EABD0", 0x008EABD0, &staticinit::Fn8EABD0);
STATICINIT3_REVERSED("StaticInit::8EAC00", 0x008EAC00, &staticinit::Fn8EAC00);
STATICINIT3_REVERSED("StaticInit::8EAC30", 0x008EAC30, &staticinit::Fn8EAC30);
STATICINIT3_REVERSED("StaticInit::8EAC60", 0x008EAC60, &staticinit::Fn8EAC60);
STATICINIT3_REVERSED("StaticInit::8EAC90", 0x008EAC90, &staticinit::Fn8EAC90);
STATICINIT3_REVERSED("StaticInit::8EACC0", 0x008EACC0, &staticinit::Fn8EACC0);
STATICINIT3_REVERSED("StaticInit::8EAD50", 0x008EAD50, &staticinit::Fn8EAD50);
STATICINIT3_REVERSED("StaticInit::8EAD80", 0x008EAD80, &staticinit::Fn8EAD80);
STATICINIT3_REVERSED("StaticInit::8EADA0", 0x008EADA0, &staticinit::Fn8EADA0);
STATICINIT3_REVERSED("StaticInit::8EADD0", 0x008EADD0, &staticinit::Fn8EADD0);
STATICINIT3_REVERSED("StaticInit::8EAE00", 0x008EAE00, &staticinit::Fn8EAE00);
STATICINIT3_REVERSED("StaticInit::8EAE30", 0x008EAE30, &staticinit::Fn8EAE30);
STATICINIT3_REVERSED("StaticInit::8EAE80", 0x008EAE80, &staticinit::Fn8EAE80);
STATICINIT3_REVERSED("StaticInit::8EAEB0", 0x008EAEB0, &staticinit::Fn8EAEB0);
STATICINIT3_REVERSED("StaticInit::8EAEE0", 0x008EAEE0, &staticinit::Fn8EAEE0);
STATICINIT3_REVERSED("StaticInit::8EAF40", 0x008EAF40, &staticinit::Fn8EAF40);
STATICINIT3_REVERSED("StaticInit::8EAF70", 0x008EAF70, &staticinit::Fn8EAF70);
STATICINIT3_REVERSED("StaticInit::8EAFE0", 0x008EAFE0, &staticinit::Fn8EAFE0);
STATICINIT3_REVERSED("StaticInit::8EB000", 0x008EB000, &staticinit::Fn8EB000);
STATICINIT3_REVERSED("StaticInit::8EB020", 0x008EB020, &staticinit::Fn8EB020);
STATICINIT3_REVERSED("StaticInit::8EB040", 0x008EB040, &staticinit::Fn8EB040);
STATICINIT3_REVERSED("StaticInit::8EB060", 0x008EB060, &staticinit::Fn8EB060);
STATICINIT3_REVERSED("StaticInit::8EB090", 0x008EB090, &staticinit::Fn8EB090);
STATICINIT3_REVERSED("StaticInit::8EB0C0", 0x008EB0C0, &staticinit::Fn8EB0C0);
STATICINIT3_REVERSED("StaticInit::8EB0F0", 0x008EB0F0, &staticinit::Fn8EB0F0);
STATICINIT3_REVERSED("StaticInit::8EB110", 0x008EB110, &staticinit::Fn8EB110);
STATICINIT3_REVERSED("StaticInit::8EB130", 0x008EB130, &staticinit::Fn8EB130);
STATICINIT3_REVERSED("StaticInit::8EB150", 0x008EB150, &staticinit::Fn8EB150);
STATICINIT3_REVERSED("StaticInit::8EB170", 0x008EB170, &staticinit::Fn8EB170);
STATICINIT3_REVERSED("StaticInit::8EB1A0", 0x008EB1A0, &staticinit::Fn8EB1A0);
STATICINIT3_REVERSED("StaticInit::8EB1C0", 0x008EB1C0, &staticinit::Fn8EB1C0);
STATICINIT3_REVERSED("StaticInit::8EB1F0", 0x008EB1F0, &staticinit::Fn8EB1F0);
STATICINIT3_REVERSED("StaticInit::8EB220", 0x008EB220, &staticinit::Fn8EB220);
STATICINIT3_REVERSED("StaticInit::8EB250", 0x008EB250, &staticinit::Fn8EB250);
STATICINIT3_REVERSED("StaticInit::8EB280", 0x008EB280, &staticinit::Fn8EB280);
STATICINIT3_REVERSED("StaticInit::8EB2B0", 0x008EB2B0, &staticinit::Fn8EB2B0);
STATICINIT3_REVERSED("StaticInit::8EB2E0", 0x008EB2E0, &staticinit::Fn8EB2E0);
STATICINIT3_REVERSED("StaticInit::8EB310", 0x008EB310, &staticinit::Fn8EB310);
STATICINIT3_REVERSED("StaticInit::8EB340", 0x008EB340, &staticinit::Fn8EB340);
STATICINIT3_REVERSED("StaticInit::8EB370", 0x008EB370, &staticinit::Fn8EB370);
STATICINIT3_REVERSED("StaticInit::8EB3A0", 0x008EB3A0, &staticinit::Fn8EB3A0);
STATICINIT3_REVERSED("StaticInit::8EB3D0", 0x008EB3D0, &staticinit::Fn8EB3D0);
STATICINIT3_REVERSED("StaticInit::8EB3F0", 0x008EB3F0, &staticinit::Fn8EB3F0);
STATICINIT3_REVERSED("StaticInit::8EB410", 0x008EB410, &staticinit::Fn8EB410);
STATICINIT3_REVERSED("StaticInit::8EB430", 0x008EB430, &staticinit::Fn8EB430);
STATICINIT3_REVERSED("StaticInit::8EB450", 0x008EB450, &staticinit::Fn8EB450);
STATICINIT3_REVERSED("StaticInit::8EB470", 0x008EB470, &staticinit::Fn8EB470);
STATICINIT3_REVERSED("StaticInit::8EB490", 0x008EB490, &staticinit::Fn8EB490);
STATICINIT3_REVERSED("StaticInit::8EB4B0", 0x008EB4B0, &staticinit::Fn8EB4B0);
STATICINIT3_REVERSED("StaticInit::8EB4D0", 0x008EB4D0, &staticinit::Fn8EB4D0);
STATICINIT3_REVERSED("StaticInit::8EB4F0", 0x008EB4F0, &staticinit::Fn8EB4F0);
STATICINIT3_REVERSED("StaticInit::8EB510", 0x008EB510, &staticinit::Fn8EB510);
STATICINIT3_REVERSED("StaticInit::8EB530", 0x008EB530, &staticinit::Fn8EB530);
STATICINIT3_REVERSED("StaticInit::8EB550", 0x008EB550, &staticinit::Fn8EB550);
STATICINIT3_REVERSED("StaticInit::8EB570", 0x008EB570, &staticinit::Fn8EB570);
STATICINIT3_REVERSED("StaticInit::8EB590", 0x008EB590, &staticinit::Fn8EB590);
STATICINIT3_REVERSED("StaticInit::8EB5B0", 0x008EB5B0, &staticinit::Fn8EB5B0);
STATICINIT3_REVERSED("StaticInit::8EB5D0", 0x008EB5D0, &staticinit::Fn8EB5D0);
STATICINIT3_REVERSED("StaticInit::8EB5F0", 0x008EB5F0, &staticinit::Fn8EB5F0);
STATICINIT3_REVERSED("StaticInit::8EB610", 0x008EB610, &staticinit::Fn8EB610);
STATICINIT3_REVERSED("StaticInit::8EB630", 0x008EB630, &staticinit::Fn8EB630);
STATICINIT3_REVERSED("StaticInit::8EB650", 0x008EB650, &staticinit::Fn8EB650);
STATICINIT3_REVERSED("StaticInit::8EB670", 0x008EB670, &staticinit::Fn8EB670);
STATICINIT3_REVERSED("StaticInit::8EB690", 0x008EB690, &staticinit::Fn8EB690);
STATICINIT3_REVERSED("StaticInit::8EB6B0", 0x008EB6B0, &staticinit::Fn8EB6B0);
STATICINIT3_REVERSED("StaticInit::8EB6D0", 0x008EB6D0, &staticinit::Fn8EB6D0);
STATICINIT3_REVERSED("StaticInit::8EB6F0", 0x008EB6F0, &staticinit::Fn8EB6F0);
STATICINIT3_REVERSED("StaticInit::8EB710", 0x008EB710, &staticinit::Fn8EB710);
STATICINIT3_REVERSED("StaticInit::8EB730", 0x008EB730, &staticinit::Fn8EB730);
STATICINIT3_REVERSED("StaticInit::8EB750", 0x008EB750, &staticinit::Fn8EB750);
STATICINIT3_REVERSED("StaticInit::8EB770", 0x008EB770, &staticinit::Fn8EB770);
STATICINIT3_REVERSED("StaticInit::8EB790", 0x008EB790, &staticinit::Fn8EB790);
STATICINIT3_REVERSED("StaticInit::8EB7B0", 0x008EB7B0, &staticinit::Fn8EB7B0);
STATICINIT3_REVERSED("StaticInit::8EB7D0", 0x008EB7D0, &staticinit::Fn8EB7D0);
STATICINIT3_REVERSED("StaticInit::8EB7F0", 0x008EB7F0, &staticinit::Fn8EB7F0);
STATICINIT3_REVERSED("StaticInit::8EB810", 0x008EB810, &staticinit::Fn8EB810);
STATICINIT3_REVERSED("StaticInit::8EB830", 0x008EB830, &staticinit::Fn8EB830);
STATICINIT3_REVERSED("StaticInit::8EB850", 0x008EB850, &staticinit::Fn8EB850);
STATICINIT3_REVERSED("StaticInit::8EB870", 0x008EB870, &staticinit::Fn8EB870);
STATICINIT3_REVERSED("StaticInit::8EB890", 0x008EB890, &staticinit::Fn8EB890);
STATICINIT3_REVERSED("StaticInit::8EB8B0", 0x008EB8B0, &staticinit::Fn8EB8B0);
STATICINIT3_REVERSED("StaticInit::8EB8D0", 0x008EB8D0, &staticinit::Fn8EB8D0);
STATICINIT3_REVERSED("StaticInit::8EB8F0", 0x008EB8F0, &staticinit::Fn8EB8F0);
STATICINIT3_REVERSED("StaticInit::8EB910", 0x008EB910, &staticinit::Fn8EB910);
STATICINIT3_REVERSED("StaticInit::8EB930", 0x008EB930, &staticinit::Fn8EB930);
STATICINIT3_REVERSED("StaticInit::8EB950", 0x008EB950, &staticinit::Fn8EB950);
STATICINIT3_REVERSED("StaticInit::8EB970", 0x008EB970, &staticinit::Fn8EB970);
STATICINIT3_REVERSED("StaticInit::8EB990", 0x008EB990, &staticinit::Fn8EB990);
STATICINIT3_REVERSED("StaticInit::8EB9B0", 0x008EB9B0, &staticinit::Fn8EB9B0);
STATICINIT3_REVERSED("StaticInit::8EB9D0", 0x008EB9D0, &staticinit::Fn8EB9D0);
STATICINIT3_REVERSED("StaticInit::8EB9F0", 0x008EB9F0, &staticinit::Fn8EB9F0);
STATICINIT3_REVERSED("StaticInit::8EBA10", 0x008EBA10, &staticinit::Fn8EBA10);
STATICINIT3_REVERSED("StaticInit::8EBA30", 0x008EBA30, &staticinit::Fn8EBA30);
STATICINIT3_REVERSED("StaticInit::8EBA50", 0x008EBA50, &staticinit::Fn8EBA50);
STATICINIT3_REVERSED("StaticInit::8EBA70", 0x008EBA70, &staticinit::Fn8EBA70);
STATICINIT3_REVERSED("StaticInit::8EBA90", 0x008EBA90, &staticinit::Fn8EBA90);
STATICINIT3_REVERSED("StaticInit::8EBAB0", 0x008EBAB0, &staticinit::Fn8EBAB0);
STATICINIT3_REVERSED("StaticInit::8EBB10", 0x008EBB10, &staticinit::Fn8EBB10);
STATICINIT3_REVERSED("StaticInit::8EBB20", 0x008EBB20, &staticinit::Fn8EBB20);
STATICINIT3_REVERSED("StaticInit::8EBB40", 0x008EBB40, &staticinit::Fn8EBB40);
STATICINIT3_REVERSED("StaticInit::8EBB60", 0x008EBB60, &staticinit::Fn8EBB60);
STATICINIT3_REVERSED("StaticInit::8EBBC0", 0x008EBBC0, &staticinit::Fn8EBBC0);
STATICINIT3_REVERSED("StaticInit::8EBBD0", 0x008EBBD0, &staticinit::Fn8EBBD0);
STATICINIT3_REVERSED("StaticInit::8EBBE0", 0x008EBBE0, &staticinit::Fn8EBBE0);
STATICINIT3_REVERSED("StaticInit::8EBC00", 0x008EBC00, &staticinit::Fn8EBC00);
STATICINIT3_REVERSED("StaticInit::8EBC20", 0x008EBC20, &staticinit::Fn8EBC20);
STATICINIT3_REVERSED("StaticInit::8EBC40", 0x008EBC40, &staticinit::Fn8EBC40);
STATICINIT3_REVERSED("StaticInit::8EBC60", 0x008EBC60, &staticinit::Fn8EBC60);
STATICINIT3_REVERSED("StaticInit::8EBC80", 0x008EBC80, &staticinit::Fn8EBC80);
STATICINIT3_REVERSED("StaticInit::8EBCA0", 0x008EBCA0, &staticinit::Fn8EBCA0);
STATICINIT3_REVERSED("StaticInit::8EBCC0", 0x008EBCC0, &staticinit::Fn8EBCC0);
STATICINIT3_REVERSED("StaticInit::8EBCE0", 0x008EBCE0, &staticinit::Fn8EBCE0);
STATICINIT3_REVERSED("StaticInit::8EBD00", 0x008EBD00, &staticinit::Fn8EBD00);
STATICINIT3_REVERSED("StaticInit::8EBD20", 0x008EBD20, &staticinit::Fn8EBD20);
STATICINIT3_REVERSED("StaticInit::8EBD40", 0x008EBD40, &staticinit::Fn8EBD40);
STATICINIT3_REVERSED("StaticInit::8EBD60", 0x008EBD60, &staticinit::Fn8EBD60);
STATICINIT3_REVERSED("StaticInit::8EBD80", 0x008EBD80, &staticinit::Fn8EBD80);
STATICINIT3_REVERSED("StaticInit::8EBDA0", 0x008EBDA0, &staticinit::Fn8EBDA0);
STATICINIT3_REVERSED("StaticInit::8EBDC0", 0x008EBDC0, &staticinit::Fn8EBDC0);
STATICINIT3_REVERSED("StaticInit::8EBDD0", 0x008EBDD0, &staticinit::Fn8EBDD0);
STATICINIT3_REVERSED("StaticInit::8EBDF0", 0x008EBDF0, &staticinit::Fn8EBDF0);
STATICINIT3_REVERSED("StaticInit::8EBE10", 0x008EBE10, &staticinit::Fn8EBE10);
STATICINIT3_REVERSED("StaticInit::8EBE30", 0x008EBE30, &staticinit::Fn8EBE30);
STATICINIT3_REVERSED("StaticInit::8EBE50", 0x008EBE50, &staticinit::Fn8EBE50);
STATICINIT3_REVERSED("StaticInit::8EBE70", 0x008EBE70, &staticinit::Fn8EBE70);
STATICINIT3_REVERSED("StaticInit::8EBE90", 0x008EBE90, &staticinit::Fn8EBE90);
STATICINIT3_REVERSED("StaticInit::8EBEB0", 0x008EBEB0, &staticinit::Fn8EBEB0);
STATICINIT3_REVERSED("StaticInit::8EBED0", 0x008EBED0, &staticinit::Fn8EBED0);
STATICINIT3_REVERSED("StaticInit::8EBEF0", 0x008EBEF0, &staticinit::Fn8EBEF0);
STATICINIT3_REVERSED("StaticInit::8EBF10", 0x008EBF10, &staticinit::Fn8EBF10);
STATICINIT3_REVERSED("StaticInit::8EBF20", 0x008EBF20, &staticinit::Fn8EBF20);
STATICINIT3_REVERSED("StaticInit::8EBF70", 0x008EBF70, &staticinit::Fn8EBF70);
STATICINIT3_REVERSED("StaticInit::8EBF90", 0x008EBF90, &staticinit::Fn8EBF90);
STATICINIT3_REVERSED("StaticInit::8EBFB0", 0x008EBFB0, &staticinit::Fn8EBFB0);
STATICINIT3_REVERSED("StaticInit::8EBFD0", 0x008EBFD0, &staticinit::Fn8EBFD0);
STATICINIT3_REVERSED("StaticInit::8EBFF0", 0x008EBFF0, &staticinit::Fn8EBFF0);
STATICINIT3_REVERSED("StaticInit::8EC010", 0x008EC010, &staticinit::Fn8EC010);
STATICINIT3_REVERSED("StaticInit::8EC030", 0x008EC030, &staticinit::Fn8EC030);
