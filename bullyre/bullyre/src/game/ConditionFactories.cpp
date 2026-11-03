// The action tree's conditions' factories (0x6373F0..0x64A650) and their registration
// (0x64B1F0): each condition name with its two factories in the condition registry
// (0x1B64A94). +4 New: the condition allocated and built (for code); +8 CreateAt: built in
// place in the loaded tree's memory. The shapes are in ActionFactories.h.
//
// The New functions and 13 of the CreateAt ones are missing from Ghidra's function list
// (only this table refers to them); they're registered here too.

#include <cstdint>

#include "game/ActionFactories.h"

using namespace actfac;

namespace {

GAME_FN(void, __thiscall, Registry_Reserve, 0x0061A690, void* registry);
GAME_FN(void, __thiscall, Registry_Add, 0x0061A700, void* registry, uint32_t* entry);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
inline void* const conditionRegistry = reinterpret_cast<void*>(0x01B64A94);

// a condition: its name (a string of the game's), New, CreateAt, and the global key the
// name is also kept in
struct ConditionRow {
    uint32_t name;
    uint32_t make;
    uint32_t createAt;
    uint32_t key;
};

// (the game builds this table on the stack, row by row, before the loop)
const ConditionRow kConditionRows[] = {
    {0x00924318, 0x00644CE0, 0x006373F0, 0x01B65BD8},  // CameraAllowScriptedChange
    {0x00939F0C, 0x00637410, 0x00637440, 0x01B65B80},  // WeaponDesired
    {0x00939EF8, 0x00644D10, 0x00637470, 0x01B65B84},  // WeaponDesiredSocket
    {0x00939EE0, 0x00644D60, 0x006374C0, 0x01B65B88},  // WeaponDesiredIsEquiped
    {0x00939EC4, 0x00644DA0, 0x00637500, 0x01B65B8C},  // WeaponDesiredIsFocusWeapon
    {0x00939EB4, 0x00644DE0, 0x00637540, 0x01B65B70},  // WeaponEquiped
    {0x00939EA0, 0x00644E20, 0x00637590, 0x01B65B74},  // WeaponEquipedType
    {0x00939E8C, 0x00644E60, 0x006375D0, 0x01B65B78},  // WeaponEquipedSocket
    {0x00939E80, 0x00637600, 0x00637630, 0x01B65B90},  // WeaponLast
    {0x00939E70, 0x00644EA0, 0x00637660, 0x01B65B50},  // WeaponDamaged
    {0x00939E64, 0x00644EE0, 0x006376A0, 0x01B65B54},  // WeaponAmmo
    {0x00939E58, 0x006376C0, 0x006376F0, 0x01B65B58},  // WeaponCount
    {0x00939E48, 0x00644F20, 0x00637720, 0x01B65B7C},  // PropAttached
    {0x00939E34, 0x00644F60, 0x00637770, 0x01B65B94},  // WeaponModelRequest
    {0x00939E20, 0x006377A0, 0x006377D0, 0x01B65B5C},  // CollisionWeaponType
    {0x00939E08, 0x006377E0, 0x00637810, 0x01B65B60},  // AltWeaponCollisionType
    {0x00939DF4, 0x00644FA0, 0x00637840, 0x01B65B64},  // DamageFromExplosion
    {0x00939DDC, 0x00644FD0, 0x00637880, 0x01B65B68},  // DamageFromBurningFire
    {0x00939DD0, 0x00645000, 0x006378C0, 0x01B65B6C},  // FireInRange
    {0x00939DBC, 0x00645040, 0x006378E0, 0x01B65BC0},  // WeaponIsThrowable
    {0x00939DAC, 0x00645070, 0x00637920, 0x01B65BE0},  // CameraBlending
    {0x0090DD2C, 0x006450A0, 0x00637960, 0x01B65BA8},  // TargetProp
    {0x00939D9C, 0x006450F0, 0x006379A0, 0x01B65BB8},  // WeaponNeedInit
    {0x00939D88, 0x00645120, 0x006379C0, 0x01B65BBC},  // WeaponIsInventory
    {0x00939D78, 0x00645150, 0x006379E0, 0x01B65BC4},  // WeaponHudActive
    {0x00939D68, 0x00645180, 0x00637A00, 0x01B65BC8},  // WeaponHudLocked
    {0x00939D58, 0x006451B0, 0x00637A20, 0x01B65BCC},  // WeaponWheelOn
    {0x00939D44, 0x006451E0, 0x00637A40, 0x01B65BD0},  // HudComponentVisible
    {0x00939D34, 0x00645220, 0x00637A80, 0x01B65BE8},  // PropLifeTime
    {0x00939D20, 0x00645260, 0x00637AC0, 0x01B65B98},  // PropSpecialState
    {0x00939D0C, 0x006452A0, 0x00637B00, 0x01B65B9C},  // PropTargetHeight
    {0x0090DCFC, 0x006452E0, 0x00637B40, 0x01B65BA4},  // TargetProjectile
    {0x00939CFC, 0x00645320, 0x00637B90, 0x01B65BB0},  // CameraDesired
    {0x00939CEC, 0x00645360, 0x00637BB0, 0x01B65BB4},  // CameraActive
    {0x00939CDC, 0x006453A0, 0x00637BF0, 0x01B65BD4},  // CameraLocked
    {0x00939CC8, 0x006453D0, 0x00637C10, 0x01B65BF0},  // CameraInWidescreen
    {0x00939CBC, 0x00645400, 0x00637C30, 0x01B65B48},  // ButtonState
    {0x00939CA8, 0x00645440, 0x00637C70, 0x01B65BDC},  // CameraDesiredReset
    {0x00939C98, 0x00645470, 0x00637CB0, 0x01B65BEC},  // HasEnoughCash
    {0x00939C84, 0x00637CD0, 0x00637D00, 0x01B65B4C},  // EvaluatedEntityType
    {0x00939C70, 0x006454B0, 0x00637D30, 0x01B65B38},  // GetProjectileOwner
    {0x00939C60, 0x006454E0, 0x00637D70, 0x01B65B44},  // GetPassingPed
    {0x00939C50, 0x00637D90, 0x00637DF0, 0x01B65B3C},  // EntityIsModel
    {0x00939C44, 0x00645510, 0x00637E40, 0x01B65B40},  // EntitySpeed
    {0x0090E2C0, 0x00645550, 0x00637E80, 0x01B65BF4},  // CatchProjectile
    {0x00939C38, 0x00645590, 0x00637EC0, 0x01B65BF8},  // AttackPower
    {0x00939C20, 0x006455D0, 0x00637F00, 0x01B659FC},  // PropTargetInteractive
    {0x00939C0C, 0x00645610, 0x00637F50, 0x01B65A00},  // PedNearPropSocket
    {0x00939C00, 0x00645670, 0x00637FB0, 0x01B65A04},  // PropHealth
    {0x00939BF0, 0x0064B0B0, 0x006456D0, 0x01B65A08},  // PropIsPlaying
    {0x00914090, 0x00645700, 0x00637FF0, 0x01B65A0C},  // PropFlags
    {0x00939BDC, 0x00645740, 0x00638030, 0x01B65A1C},  // PropTagPercentage
    {0x00939BCC, 0x00645780, 0x00638070, 0x01B65A20},  // PropModelLoaded
    {0x00939BB8, 0x006457C0, 0x006380A0, 0x01B64AB0},  // ActionNodePlayTime
    {0x00939BA0, 0x00645800, 0x006380E0, 0x01B64B2C},  // ActionNodePlayTimePast
    {0x00939B90, 0x00645830, 0x00638120, 0x01B64ACC},  // ActionRequest
    {0x00939B7C, 0x00645870, 0x00638160, 0x01B64AD0},  // ActionNodeRequest
    {0x00939B68, 0x006458B0, 0x006381A0, 0x01B64AD4},  // ActionRequestButton
    {0x00939B50, 0x006458F0, 0x006381C0, 0x01B64D04},  // AcquireAuthorityFocus
    {0x00939B3C, 0x00645920, 0x006381E0, 0x01B64D08},  // AcquireFactionFocus
    {0x00939B30, 0x00645950, 0x00638200, 0x01B64BDC},  // AcquirePOI
    {0x00939B18, 0x00645990, 0x00638220, 0x01B64CFC},  // AcquirePunishableTarget
    {0x00939AFC, 0x006459C0, 0x00638240, 0x01B64D00},  // AcquireRandomTargetInGroup
    {0x00939AEC, 0x006459F0, 0x00638260, 0x01B64CF4},  // AcquireFocus
    {0x00939ADC, 0x00645A20, 0x00638280, 0x01B64CF8},  // AcquireTarget
    {0x00939AC8, 0x0064B100, 0x00645A50, 0x01B64D28},  // AIConditionGroup
    {0x00939ABC, 0x00645A70, 0x006382A0, 0x01B64B90},  // AIIsPlaying
    {0x00939AA4, 0x00645AC0, 0x006382D0, 0x01B64D88},  // AnimationGroupLoaded
    {0x00939A90, 0x00645B00, 0x00638320, 0x01B64AF0},  // AnimationParametric
    {0x00939A7C, 0x00638340, 0x00638370, 0x01B64D8C},  // CutsceneSoundLoaded
    {0x00939A68, 0x006383A0, 0x006383D0, 0x01B64D90},  // IsMusicStreamLoaded
    {0x00913FEC, 0x00638400, 0x00638440, 0x01B64D94},  // IsSoundBankLoaded
    {0x00939A58, 0x00638470, 0x006384A0, 0x01B64D98},  // IsSpeechBusy
    {0x00939A44, 0x006384C0, 0x00638500, 0x01B64DA0},  // IsSpeechPreloaded
    {0x00939A30, 0x00638520, 0x00638550, 0x01B64D9C},  // IsSpeechStreamFree
    {0x00939A20, 0x00638570, 0x006385B0, 0x01B64DA8},  // HasSpeechEvent
    {0x00939A14, 0x00645B40, 0x006385F0, 0x01B64C10},  // Attacked
    {0x0090EDC0, 0x00645B70, 0x00638630, 0x01B64CD4},  // AttackRole
    {0x0090EDDC, 0x00645BB0, 0x00638670, 0x01B64D38},  // AttackPhase
    {0x00939A08, 0x00645BF0, 0x006386B0, 0x01B64C1C},  // AttackTimer
    {0x009399F8, 0x00645C30, 0x006386D0, 0x01B64DDC},  // AttacksDisabled
    {0x009399EC, 0x00645C60, 0x00638710, 0x01B64C2C},  // BlockTimer
    {0x009399E0, 0x00645CA0, 0x00638750, 0x01B64C30},  // EvadeTimer
    {0x009399D0, 0x00645CE0, 0x00638790, 0x01B64D48},  // CollisionStart
    {0x0090EDCC, 0x00645D10, 0x006387D0, 0x01B64CE0},  // CharacterSize
    {0x009399B4, 0x00645D50, 0x00638810, 0x01B64CE4},  // TargetRelativeCharacterSize
    {0x009399A4, 0x00645D90, 0x00638850, 0x01B64D4C},  // HitReactionEnd
    {0x00939994, 0x00645DC0, 0x006388A0, 0x01B64B14},  // ConditionFlag
    {0x00939984, 0x00645DF0, 0x006388D0, 0x01B64AB8},  // ConditionGroup
    {0x00932A30, 0x00645E30, 0x00638910, 0x01B64C84},  // Controller
    {0x00939974, 0x00645E70, 0x00638950, 0x01B64D68},  // DistToTarget
    {0x00939964, 0x00645EB0, 0x00638990, 0x01B64D40},  // EngageDistance
    {0x00939958, 0x00645EF0, 0x006389D0, 0x01B64CA8},  // FactionScan
    {0x0093994C, 0x00645F30, 0x00638A10, 0x01B64D50},  // FactionNear
    {0x00939938, 0x00645F70, 0x00638A50, 0x01B64D54},  // FindTandemHelper
    {0x00939928, 0x00645FA0, 0x00638A90, 0x01B64D58},  // IsTandemHelper
    {0x00939920, 0x00645FD0, 0x00638AB0, 0x01B64AA4},  // False
    {0x00939914, 0x00646000, 0x00638AF0, 0x01B64D34},  // FightToken
    {0x00939904, 0x00646040, 0x00646070, 0x01B64D7C},  // HaveIAttacked
    {0x009398FC, 0x00646090, 0x00638B10, 0x01B64BE0},  // HavePOI
    {0x009398EC, 0x006460C0, 0x00638B30, 0x01B64BE4},  // HavePOIOfType
    {0x0090A83C, 0x00646100, 0x00638B70, 0x01B64CD8},  // FightClass
    {0x009398DC, 0x00646140, 0x00638BB0, 0x01B64C14},  // FocusAttacked
    {0x009398C4, 0x00646170, 0x00638BF0, 0x01B64D60},  // GrapplingHookAnchord
    {0x009398B4, 0x006461B0, 0x00638C30, 0x020C35B0},  // ClassSkipped
    {0x009398A8, 0x006461F0, 0x00638C70, 0x01B64B30},  // FirstPerson
    {0x0093989C, 0x00646220, 0x00638C90, 0x01B64C80},  // PathValid
    {0x00939884, 0x00646250, 0x00638CB0, 0x01B64DB4},  // HasSweptSphereCollision
    {0x0093987C, 0x00646290, 0x00638CF0, 0x01B64AF4},  // Health
    {0x00939868, 0x006462D0, 0x00638D30, 0x01B64AF8},  // PercentageHealth
    {0x00939854, 0x00646310, 0x00638D70, 0x01B64AFC},  // PercentageSpecial
    {0x0093984C, 0x00646350, 0x00638DB0, 0x01B64B00},  // Damage
    {0x0093983C, 0x00646390, 0x00638DF0, 0x01B64B04},  // DamagePending
    {0x0093982C, 0x006463D0, 0x00638E10, 0x01B64DA4},  // HasActionNode
    {0x00939810, 0x00646410, 0x00638E40, 0x01B64DAC},  // OpeningBranchChildHasChild
    {0x009397FC, 0x00646460, 0x00638E90, 0x01B64B1C},  // WallHitDirection
    {0x009397EC, 0x006464A0, 0x00638EE0, 0x01B64B20},  // PedIsInOutfit
    {0x009397E0, 0x006464E0, 0x00638F30, 0x01B64B18},  // HitByPed
    {0x009397D8, 0x00646520, 0x00638F70, 0x01B64C20},  // HitTime
    {0x009397CC, 0x00646560, 0x00638FB0, 0x01B64B24},  // HitLocation
    {0x009397C0, 0x006465A0, 0x00638FF0, 0x01B64C24},  // HitStunTime
    {0x009397B0, 0x006465E0, 0x00639030, 0x01B64C28},  // HitStunFinished
    {0x009397A0, 0x00646620, 0x00639070, 0x01B64B28},  // HitFlagClear
    {0x0090F66C, 0x00646650, 0x006390B0, 0x01B64C68},  // IgnoreAttacks
    {0x00939790, 0x00646680, 0x006390F0, 0x01B64D44},  // InAttackCone
    {0x00939784, 0x006466B0, 0x00639110, 0x01B64C78},  // InBikeRace
    {0x0090315C, 0x006466E0, 0x00639130, 0x01B64CDC},  // VisibleArea
    {0x00939778, 0x00646720, 0x00639150, 0x01B64B94},  // LiquidLevel
    {0x0093976C, 0x00646760, 0x00639170, 0x01B64B98},  // LiquidType
    {0x0090AD18, 0x006467A0, 0x00639190, 0x01B64B9C},  // SurfaceHardness
    {0x00939760, 0x006467E0, 0x006391D0, 0x01B64BB8},  // IsAuthority
    {0x00939754, 0x00646810, 0x00639210, 0x01B64BBC},  // IsFemale
    {0x00939744, 0x00646840, 0x00639230, 0x01B64BC4},  // IsHangingOut
    {0x00939728, 0x00646870, 0x00639270, 0x01B64D0C},  // IsHealthAtHumiliationLevel
    {0x00939708, 0x006468B0, 0x00639290, 0x01B64C64},  // IsInFactionWithStimulusObject
    {0x009396F4, 0x006468E0, 0x006392D0, 0x01B64BC0},  // IsMissionSpecific
    {0x009396E0, 0x00646910, 0x006392F0, 0x01B64BD4},  // IsScriptedAmbient
    {0x009396C4, 0x00646940, 0x00639310, 0x01B64BD8},  // IsTargetAttackRightsTaken
    {0x009396B4, 0x00646970, 0x00639330, 0x01B64DD8},  // IsStationary
    {0x009396A0, 0x006469A0, 0x00639350, 0x01B64C60},  // IsStimulusObject
    {0x00939690, 0x006469D0, 0x00639390, 0x01B64CEC},  // InCombatZone
    {0x00939680, 0x00646A10, 0x006393D0, 0x01B64CE8},  // JoinCombatZone
    {0x00939670, 0x00646A50, 0x006393F0, 0x01B64EE4},  // WeaponLastHitBy
    {0x00939664, 0x00646A80, 0x00639440, 0x01B64B84},  // MePlaying
    {0x00939644, 0x00646AD0, 0x00639470, 0x01B64B88},  // MePlayingDefaultNeutralState
    {0x00939638, 0x00646B10, 0x006394B0, 0x01B64B8C},  // IsDayTime
    {0x0093962C, 0x00646B40, 0x006394F0, 0x01B64AEC},  // MotionCycle
    {0x0093961C, 0x00646B80, 0x00639530, 0x01B64AE4},  // MotionIntention
    {0x00939604, 0x00646BC0, 0x00639570, 0x01B64AE8},  // MotionIntentionSpeed
    {0x009395F0, 0x00646C00, 0x00639590, 0x01B64CB4},  // NearbyDockLocation
    {0x009395D8, 0x00646C30, 0x006395B0, 0x01B64CB8},  // NearbyFleeDockLocation
    {0x009395D4, 0x00646C70, 0x006395D0, 0x01B64AA8},  // NOT
    {0x009395D0, 0x00646CA0, 0x006395F0, 0x01B64AAC},  // OR
    {0x0090EDB4, 0x00646CD0, 0x00639610, 0x01B64C38},  // Objective
    {0x009395C8, 0x00646D10, 0x00639630, 0x01B64C6C},  // InCar
    {0x009395C0, 0x00646D40, 0x00639650, 0x01B64C70},  // OnBike
    {0x009395AC, 0x00646D70, 0x00639690, 0x01B64C74},  // OnVehicleOfClass
    {0x00939598, 0x00646DB0, 0x006396D0, 0x01B64AE0},  // PedFightingSystem
    {0x0090F9D4, 0x00646DF0, 0x00639710, 0x01B64AD8},  // PedFlags
    {0x0093958C, 0x00646E30, 0x00639750, 0x01B64ADC},  // SetPedFlags
    {0x00939580, 0x00646E70, 0x00639770, 0x01B64D3C},  // PedModelID
    {0x0090A54C, 0x00646EB0, 0x006397A0, 0x01B64BB0},  // PedType
    {0x00939574, 0x00646EF0, 0x006397E0, 0x01B64BB4},  // PedIsDriver
    {0x0093956C, 0x00646F20, 0x00639800, 0x01B64AB4},  // Print
    {0x00939558, 0x00646F60, 0x00639850, 0x01B64D30},  // ProjectileThreat
    {0x00939550, 0x00646FA0, 0x00639890, 0x01B64CA0},  // Random
    {0x00939540, 0x00646FE0, 0x006398B0, 0x01B64CA4},  // WeightedRandom
    {0x00939530, 0x00647020, 0x006398F0, 0x01B64C8C},  // RangeToTarget
    {0x0093951C, 0x00647060, 0x00639930, 0x01B64C90},  // TimeSincePedalling
    {0x00939508, 0x006470A0, 0x00639970, 0x01B64C94},  // BikeSprintAmount
    {0x009394F8, 0x006470E0, 0x006399B0, 0x01B64C98},  // RangeToFocus
    {0x009394E0, 0x00647120, 0x006399F0, 0x01B64C9C},  // RangeToObjectiveVehicle
    {0x009394D4, 0x00647160, 0x00639A10, 0x01B64BEC},  // SensePlayer
    {0x009394C0, 0x00647190, 0x00639A30, 0x01B64BF0},  // SensePlayerDuration
    {0x009394AC, 0x006471C0, 0x00639A50, 0x01B64DEC},  // HearPlayerFootsteps
    {0x0093949C, 0x006471F0, 0x00639A70, 0x01B64C08},  // SenseAttacker
    {0x00939490, 0x00647220, 0x00639A90, 0x01B64C88},  // SizeOfGroup
    {0x0090A800, 0x00647260, 0x00639AD0, 0x01B64B08},  // Special
    {0x0090A848, 0x006472A0, 0x00639B10, 0x01B64B0C},  // Sprint
    {0x00939480, 0x006472E0, 0x00639B50, 0x01B64B10},  // SpecialPoints
    {0x0090A084, 0x00647320, 0x00639B90, 0x01B64C18},  // Stat
    {0x0093946C, 0x00647360, 0x00639BB0, 0x01B64D20},  // FocusConditionGroup
    {0x00939458, 0x006473A0, 0x00639BD0, 0x01B64D24},  // PedConditionGroup
    {0x00939440, 0x006473E0, 0x00639BF0, 0x01B64D2C},  // AttackerConditionGroup
    {0x0090B430, 0x00647420, 0x00639C10, 0x01B64C44},  // Stimulus
    {0x0093942C, 0x00647460, 0x00639C30, 0x01B64C4C},  // StimulusRepeated
    {0x00939418, 0x006474A0, 0x00639C70, 0x01B64D10},  // TargetedByTarget
    {0x00939404, 0x006474E0, 0x00639CB0, 0x01B64D14},  // TargetedByPlayer
    {0x009393F4, 0x00647520, 0x00639CF0, 0x01B64D18},  // TargetPlayersGF
    {0x009393DC, 0x00647560, 0x00639D30, 0x01B64D1C},  // TargetConditionGroup
    {0x009393CC, 0x006475A0, 0x00639D70, 0x01B64CBC},  // TargetFaction
    {0x009393B4, 0x006475E0, 0x00639D90, 0x01B64CB0},  // TargetFriendsAttacker
    {0x0093939C, 0x00647610, 0x00639DB0, 0x01B64CAC},  // TargetMastersAttacker
    {0x0093938C, 0x00647640, 0x00639DD0, 0x01B64B78},  // TargetHealth
    {0x0093937C, 0x00647680, 0x00639DF0, 0x01B64C40},  // TargetIsVisible
    {0x0093936C, 0x006476B0, 0x00639E10, 0x01B64B80},  // TargetPlaying
    {0x0093935C, 0x00647700, 0x00639E60, 0x01B64B7C},  // TargetPlayTime
    {0x00939340, 0x00647740, 0x00639EA0, 0x01B64B70},  // TargetRelativeOrientation
    {0x00939328, 0x00647780, 0x00639EE0, 0x01B64B68},  // TargetRelativePosition
    {0x00939304, 0x006477C0, 0x00639F20, 0x01B64B6C},  // TargetRelativePedCollisionSpheres
    {0x009392E4, 0x00647800, 0x00639F60, 0x01B64B74},  // TargetRelativePositionInsideBox
    {0x009392D0, 0x00647840, 0x00639F80, 0x01B64C34},  // TargetsPunishPoints
    {0x009392B8, 0x00647880, 0x00639FA0, 0x01B64C50},  // TargetStimulusSubject
    {0x009392A0, 0x006478B0, 0x00639FC0, 0x01B64C54},  // TargetStimulusObject
    {0x00939284, 0x006478E0, 0x00639FE0, 0x01B64C58},  // TargetStimulusEntitySubject
    {0x00939268, 0x00647910, 0x0063A000, 0x01B64C5C},  // TargetStimulusEntityObject
    {0x0090DD38, 0x00647940, 0x0063A040, 0x01B64B38},  // TargetVehicle
    {0x00939254, 0x00647980, 0x0063A080, 0x01B64B3C},  // TargetVehicleCar
    {0x00939240, 0x006479C0, 0x0063A0C0, 0x01B64B40},  // TargetVehicleBike
    {0x00939228, 0x00647A00, 0x0063A100, 0x01B64B44},  // AttemptAnchorToVehicle
    {0x00939214, 0x00647A30, 0x0063A140, 0x01B64B60},  // RelativePosition
    {0x009391FC, 0x00647A70, 0x0063A180, 0x01B64B64},  // VehicleWheelsOnGround
    {0x009391E4, 0x00647AB0, 0x0063A1D0, 0x01B64B58},  // TargetVehicleIsPlaying
    {0x009391D4, 0x00647AF0, 0x0063A200, 0x01B64BA0},  // InTriggerVolume
    {0x009391BC, 0x00647B30, 0x0063A230, 0x01B64BA4},  // PlayerInTriggerVolume
    {0x009391B0, 0x0063A260, 0x0063A290, 0x01B64BAC},  // NearPoint
    {0x009391A4, 0x0063A2A0, 0x0063A2D0, 0x020C35A0},  // MissionCond
    {0x00939198, 0x0063A2E0, 0x0063A310, 0x020C35A4},  // ErrandCond
    {0x00939188, 0x0063A320, 0x0063A350, 0x020C35AC},  // MissionUnlocked
    {0x0093917C, 0x00647B70, 0x0063A3D0, 0x020C35B4},  // TimeCond
    {0x00939170, 0x0063A3F0, 0x0063A420, 0x020C35B8},  // PeriodCond
    {0x00939168, 0x00647BB0, 0x0063A450, 0x020C35BC},  // DayCond
    {0x00939160, 0x00647BF0, 0x0063A490, 0x020C35C0},  // LuaCond
    {0x0093914C, 0x00647C40, 0x0063A4E0, 0x01B89210},  // ConversationActive
    {0x00939140, 0x00647C80, 0x0063A520, 0x01B64BF4},  // IsPlayer
    {0x00939130, 0x00647CB0, 0x0063A560, 0x01B64D5C},  // PedIsTrigger
    {0x00939124, 0x00647CE0, 0x0063A5A0, 0x01B64D80},  // DriveForce
    {0x00939114, 0x00647D20, 0x0063A5E0, 0x01B64D70},  // LastSurfaceOn
    {0x00939104, 0x00647D60, 0x0063A620, 0x01B64D74},  // SurfaceGrade
    {0x00918EA8, 0x00647DA0, 0x0063A660, 0x01B64D78},  // Season
    {0x009390F8, 0x00647DE0, 0x0063A680, 0x01B64D84},  // OnMission
    {0x009390E8, 0x00647E20, 0x0063A6D0, 0x01B64DB0},  // DistToGround
    {0x009390D4, 0x0063A6F0, 0x0063A720, 0x01B64DB8},  // MissionSuccessCount
    {0x009390C4, 0x00647E60, 0x0063A730, 0x01B64DBC},  // IsTargetOnBike
    {0x009390B4, 0x00647E90, 0x0063A750, 0x01B64DC0},  // IsFocusOnBike
    {0x0093909C, 0x00647EC0, 0x0063A770, 0x01B64DC4},  // IsObjectiveVehicleOwner
    {0x0093908C, 0x00647EF0, 0x0063A790, 0x01B64DC8},  // LineOfFireClear
    {0x00939080, 0x00647F30, 0x0063A7B0, 0x01B64DCC},  // IsGuarding
    {0x0093906C, 0x00647F60, 0x0063A7D0, 0x01B64DD0},  // IsTargetCrouching
    {0x00939058, 0x0063A7F0, 0x0063A840, 0x01B64DD4},  // IsTargetOutOfReach
    {0x00939040, 0x00647F90, 0x0063A860, 0x01B64DE0},  // IsStealthMissionChar
    {0x00939038, 0x00647FC0, 0x0063A880, 0x01B64DE8},  // Spotted
    {0x00939024, 0x00647FF0, 0x0063A8A0, 0x01B64DE4},  // WantToBeStealthy
    {0x00939018, 0x00648030, 0x0063A8C0, 0x01B64DF0},  // IsTethered
    {0x00939008, 0x00648060, 0x0063A8E0, 0x01B64DF4},  // IsInsideTether
    {0x00938FF8, 0x00648090, 0x0063A900, 0x01B64DF8},  // IsTargetInRange
    {0x00938FE8, 0x006480D0, 0x0063A920, 0x01B64BE8},  // HaveDynamicPOI
    {0x00901A34, 0x00648110, 0x0063A970, 0x01B64CC0},  // Clothing
    {0x00938FD8, 0x00648160, 0x0063A9C0, 0x01B64CC4},  // TargetClothing
    {0x00938FC8, 0x006481A0, 0x0063AA00, 0x01B64CC8},  // ClothingStyle
    {0x00938FB0, 0x006481E0, 0x0063AA40, 0x01B64CCC},  // ClothingWarmthFactor
    {0x00938F9C, 0x00648220, 0x0063AA80, 0x01B64CD0},  // ClothingSpecialKiss
    {0x00938F84, 0x00648250, 0x0063AAA0, 0x01B64DFC},  // TargetRelativeHeight
    {0x00938F74, 0x00648290, 0x0063AAC0, 0x01B64E00},  // IsTargetProp
    {0x00938F68, 0x006482C0, 0x0063AAE0, 0x01B64E0C},  // Interval
    {0x00938F54, 0x00648300, 0x0063AB00, 0x01B64E10},  // CheckActionTimer
    {0x00938F40, 0x00648330, 0x0063AB20, 0x01B64E18},  // MissionCanLaunch
    {0x00938F28, 0x00648360, 0x0063AB40, 0x01B64E14},  // CanSecondaryMissionPlay
    {0x00926068, 0x006483A0, 0x0063AB60, 0x01B64E1C},  // MissionInClass
    {0x00938F18, 0x006483D0, 0x0063AB80, 0x01B64E20},  // IsPlayerNearby
    {0x00938F0C, 0x00648410, 0x0063ABA0, 0x01B64E24},  // IsHungUp
    {0x00938F00, 0x00648440, 0x0063ABE0, 0x01B64C04},  // NeedsToDive
    {0x00938EE8, 0x00648470, 0x0063AC00, 0x01B64C48},  // HasNewAudibleStimulus
    {0x00938ED8, 0x006484B0, 0x0063AC20, 0x01B64E04},  // TargetProxyProp
    {0x00938EC0, 0x006484F0, 0x0063AC40, 0x01B64E08},  // CanPropSetUsageState
    {0x0090EDE8, 0x00648530, 0x0063AC80, 0x01B64E28},  // StealthBehavior
    {0x00938EB0, 0x00648570, 0x0063ACC0, 0x01B64B48},  // TargetVehicleIs
    {0x00938E90, 0x006485B0, 0x0063AD00, 0x01B64B4C},  // TargetVehicleIsLastVehicleUsed
    {0x00938E6C, 0x006485E0, 0x0063AD40, 0x01B64B50},  // TargetVehicleOwnerDifferentFaction
    {0x00938E4C, 0x00648610, 0x0063AD80, 0x01B64B54},  // TargetVehicleIsAlreadyOccupied
    {0x00938E34, 0x00648640, 0x0063ADC0, 0x01B64B5C},  // TargetMyTargetsVehicle
    {0x00938E28, 0x00648670, 0x0063ADE0, 0x01B64E2C},  // IsOnSnow
    {0x00938E1C, 0x006486A0, 0x0063AE00, 0x01B64E30},  // FindSnow
    {0x00938E0C, 0x006486E0, 0x0063AE20, 0x01B64E38},  // RunningAttack
    {0x00938DFC, 0x00648730, 0x0063AE40, 0x01B64E3C},  // IsAllyLeader
    {0x00938DEC, 0x00648760, 0x0063AE60, 0x01B64E40},  // IsAllyFollower
    {0x00938DDC, 0x00648790, 0x0063AE80, 0x01B64E44},  // IsTargetAlly
    {0x00938DC8, 0x006487C0, 0x0063AEA0, 0x01B64E48},  // CanRecruitTarget
    {0x00938DB8, 0x006487F0, 0x0063AEC0, 0x01B64E4C},  // ShouldSocialize
    {0x00938DA8, 0x00648820, 0x0063AEE0, 0x01B64E54},  // IsSocialTarget
    {0x00938D98, 0x00648850, 0x0063AF00, 0x01B64E60},  // SocialAnimIs
    {0x00938D88, 0x00648890, 0x0063AF20, 0x01B64E64},  // SocialEmotionIs
    {0x00938D74, 0x006488D0, 0x0063AF40, 0x01B64E68},  // HumiliationGroupIs
    {0x00938D64, 0x00648910, 0x0063AF60, 0x01B64E6C},  // IsSocializing
    {0x00938D4C, 0x00648950, 0x0063AF80, 0x01B64E70},  // IsSocializingWithPlayer
    {0x00938D34, 0x00648980, 0x0063AFA0, 0x01B64E74},  // IsWantingToSocialize
    {0x00938D1C, 0x006489B0, 0x0063AFC0, 0x01B64E78},  // IsEmotionTowardsTarget
    {0x00938D04, 0x006489F0, 0x0063AFE0, 0x01B64E7C},  // IsEmotionTowardsPlayer
    {0x00938CF0, 0x00648A30, 0x0063B000, 0x01B64E80},  // IsSocialStateWait
    {0x00938CDC, 0x00648A60, 0x0063B020, 0x01B64E84},  // IsSocialStateAct
    {0x00938CC4, 0x00648A90, 0x0063B040, 0x01B64E88},  // IsSocialInitialReaction
    {0x00938CAC, 0x00648AC0, 0x0063B060, 0x01B64E8C},  // IsSocialInitialAction
    {0x00938C98, 0x00648AF0, 0x0063B080, 0x01B64E90},  // IsReadyToBeKissed
    {0x00938C88, 0x00648B20, 0x0063B0A0, 0x01B64E94},  // SocialTypeFAT
    {0x00938C78, 0x00648B50, 0x0063B0C0, 0x01B64E98},  // SocialTypeGIRL
    {0x00938C68, 0x00648B80, 0x0063B0E0, 0x01B64E9C},  // SocialTypeBULLY
    {0x00938C58, 0x00648BB0, 0x0063B100, 0x01B64EA0},  // SocialTypeNERD
    {0x00938C48, 0x00648BE0, 0x0063B120, 0x01B64EA4},  // SocialTypeAUTH
    {0x00938C38, 0x00648C10, 0x0063B140, 0x01B64EA8},  // SocialTypeGEN
    {0x00938C28, 0x00648C40, 0x0063B160, 0x01B64EAC},  // SocialTypeOLD
    {0x00938C14, 0x00648C70, 0x0063B180, 0x01B64EB0},  // SocialTypeNERDGIRL
    {0x00938C08, 0x00648CA0, 0x0063B1A0, 0x01B64EBC},  // IsKissCount
    {0x00938BF0, 0x00648CE0, 0x0063B1C0, 0x01B64EC0},  // TimeOfLastSocialAction
    {0x00938BD8, 0x00648D20, 0x0063B1E0, 0x01B64EC4},  // IsTargetBusySocializing
    {0x00938BC0, 0x00648D50, 0x0063B200, 0x01B64EC8},  // HaveIAssaultedTargetPed
    {0x00938BA8, 0x00648D80, 0x0063B220, 0x01B64ECC},  // HaveIHarassedTargetPed
    {0x00938B90, 0x00648DB0, 0x0063B240, 0x01B64ED0},  // HaveIDismissedTargetPed
    {0x00938B7C, 0x00648DE0, 0x0063B260, 0x01B64ED4},  // IsStimulusPriority
    {0x00938B70, 0x00648E20, 0x0063B280, 0x01B64EF0},  // IsStrafing
    {0x00938B60, 0x00648E50, 0x0063B2A0, 0x01B64EF4},  // TargetAttacker
    {0x00938B50, 0x00648E80, 0x0063B2C0, 0x01B64EF8},  // TargetByType
    {0x00938B40, 0x00648EC0, 0x0063B2E0, 0x01B64EDC},  // BikeIsLocked
    {0x00938B30, 0x00648EF0, 0x0063B300, 0x01B64EE0},  // BikeIsUpright
    {0x00938B20, 0x00648F20, 0x0063B320, 0x01B64EE8},  // WillAcceptGift
    {0x00938B08, 0x00648F50, 0x0063B340, 0x01B64EEC},  // PlayerHasRequiredGift
    {0x00938B00, 0x00648F90, 0x0063B360, 0x01B64F14},  // StatInt
    {0x00938AF4, 0x00648FD0, 0x0063B380, 0x01B64F18},  // StatFloat
    {0x00938ADC, 0x00649010, 0x0063B3A0, 0x01B64F1C},  // CheckTargetSemaphore
    {0x00938ACC, 0x00649040, 0x0063B3C0, 0x01B64F20},  // DetectGrenade
    {0x00938AB8, 0x00649070, 0x0063B3E0, 0x01B64F24},  // DetectProjectile
    {0x00938AA0, 0x006490A0, 0x0063B400, 0x01B64BA8},  // InTriggerVolumeOfType
    {0x00938A8C, 0x006490E0, 0x0063B420, 0x01B64F28},  // IsVehicleInRange
    {0x00938A7C, 0x00649120, 0x0063B440, 0x01B64F2C},  // InGuardRadius
    {0x00938A60, 0x00649160, 0x0063B460, 0x01B64F30},  // AllyLeaderConditionGroup
    {0x00938A44, 0x006491A0, 0x0063B480, 0x01B64F34},  // AllyFollowerConditionGroup
    {0x00938A34, 0x006491E0, 0x0063B4A0, 0x01B64C7C},  // IsMissionActive
    {0x00938A24, 0x00649210, 0x0063B4C0, 0x01B64F38},  // CanAcquireAlly
    {0x00938A18, 0x00649250, 0x0063B500, 0x01B64F3C},  // Steering
    {0x00938A04, 0x00649290, 0x0063B540, 0x01B64F40},  // SteeringFraction
    {0x009389F4, 0x006492D0, 0x0063B560, 0x01B64F44},  // CheckWeather
    {0x009389E8, 0x00649300, 0x0063B580, 0x01B64BC8},  // IsSpectator
    {0x009389D4, 0x00649330, 0x0063B5A0, 0x01B64F48},  // IsParametricStatic
    {0x009389C8, 0x00649360, 0x0063B5C0, 0x01B64F4C},  // TargetAlly
    {0x009389BC, 0x00649390, 0x0063B5E0, 0x01B64E34},  // FindSurface
    {0x009389AC, 0x006493D0, 0x0063B600, 0x01B64F50},  // InWeaponRange
    {0x009389A0, 0x00649400, 0x0063B620, 0x01B64F54},  // TargetPed
    {0x00938988, 0x00649460, 0x0063B670, 0x01B64F58},  // BalanceMeterPosition
    {0x00938974, 0x006494A0, 0x0063B6B0, 0x01B64BF8},  // IsPlayerFatigued
    {0x00938964, 0x006494D0, 0x0063B6F0, 0x01B64BFC},  // IsPlayerTired
    {0x00938954, 0x00649500, 0x0063B710, 0x01B64F5C},  // TargetColliding
    {0x00927FE0, 0x00649530, 0x0063B750, 0x01B64F60},  // PedInConversation
    {0x0093893C, 0x00649570, 0x0063B790, 0x01B64F64},  // PedConversationStatus
    {0x00938928, 0x006495B0, 0x0063B7D0, 0x01B64F68},  // PedConversationMood
    {0x00938918, 0x006495F0, 0x0063B7F0, 0x01B64B34},  // CanPathToEntity
    {0x0093890C, 0x00649630, 0x0063B830, 0x01B64C00},  // IsPatrolPed
    {0x00928DE8, 0x00649660, 0x0063B870, 0x01B64F6C},  // PlayerIsInStealthProp
    {0x009388F4, 0x006496A0, 0x0063B8B0, 0x01B64F70},  // HasLineOfSightToPlayer
    {0x009388E4, 0x006496E0, 0x0063B8F0, 0x01B64F74},  // ParametricTime
    {0x009388D4, 0x00649720, 0x0063B910, 0x01B64F78},  // PedGroundDist
    {0x009388C8, 0x00649760, 0x0063B930, 0x01B64F7C},  // PedVelocity
    {0x009388B8, 0x006497A0, 0x0063B950, 0x01B64F94},  // HasRangedWeapon
    {0x009388A8, 0x006497E0, 0x0063B970, 0x01B64F98},  // HasTXDLoaded
    {0x00938898, 0x00649820, 0x0063B9C0, 0x01B64FA0},  // PedSphereHit
    {0x00938884, 0x00649860, 0x0063BA00, 0x01B64FA4},  // IsRumbleModeAmbient
    {0x00938870, 0x00649890, 0x0063BA40, 0x01B64FA8},  // IsRumbleModeMission
    {0x0090F040, 0x006498C0, 0x0063BA60, 0x01B64BCC},  // IsScenarioPed
    {0x00938858, 0x006498F0, 0x0063BA80, 0x01B64BD0},  // HasCompletedScenarioObj
    {0x00938840, 0x00649920, 0x0063BAA0, 0x01B64EB4},  // SocialScriptCallback
    {0x0093882C, 0x00649950, 0x0063BAC0, 0x01B64E58},  // UseSocialOverride
    {0x00938814, 0x00649990, 0x0063BAE0, 0x01B64E5C},  // PlayerUseSocialOverride
    {0x009387FC, 0x006499D0, 0x0063BB00, 0x01B64EB8},  // SocialActionInterrupted
    {0x009387E0, 0x00649A00, 0x0063BB40, 0x01B64F84},  // CanGetOffVehicleHereLeft
    {0x009387C8, 0x00649A30, 0x0063BB80, 0x01B64F80},  // CanGetOffVehicleHere
    {0x009387B4, 0x00649A60, 0x0063BBC0, 0x01B64F88},  // CanGetOnVehicleHere
    {0x0093879C, 0x00649A90, 0x0063BC00, 0x01B64F8C},  // CanGetOnVehicleLeftSide
    {0x00938780, 0x00649AC0, 0x0063BC40, 0x01B64F90},  // CanGetOnVehicleRightSide
    {0x00938768, 0x00649AF0, 0x0063BC60, 0x01B64EFC},  // TestRequiredGiftType
    {0x00938754, 0x00649B20, 0x0063BC80, 0x01B64F00},  // TestFutureGiftType
    {0x00938740, 0x00649B50, 0x0063BCA0, 0x01B64F04},  // HasGiftItemOfType
    {0x0093872C, 0x00649B80, 0x0063BCC0, 0x01B64F0C},  // HasPlayerMaxItems
    {0x00938718, 0x00649BC0, 0x0063BCF0, 0x01B64F10},  // PlayerHasItemCount
    {0x00938708, 0x00649C00, 0x0063BD20, 0x01B64ED8},  // IsPowerSliding
    {0x009386F4, 0x00649C30, 0x0063BD40, 0x01B64E50},  // ShouldAllowApology
    {0x009386DC, 0x00649C60, 0x0063BD80, 0x01B64FAC},  // BeamLateralDeviation
    {0x009386C8, 0x00649CA0, 0x0063BDC0, 0x01B64FB0},  // BeamLateralPosition
    {0x00913CF4, 0x00649CE0, 0x0063BE00, 0x01B65BFC},  // WeaponFlags
    {0x009386B0, 0x00649D20, 0x0063BE40, 0x01B65C00},  // WeaponCanBeUsedOnBike
    {0x009386A4, 0x00649D50, 0x0063BE80, 0x01B65A10},  // PropModelID
    {0x00938694, 0x00649D90, 0x0063BED0, 0x01B65A24},  // PropUniqueID
    {0x0093867C, 0x00649DD0, 0x0063BF20, 0x01B64FB4},  // VehicleHitOrientation
    {0x00938660, 0x00649E10, 0x0063BF60, 0x01B64FB8},  // PlayerHasPunishmentPoints
    {0x00938648, 0x00649E50, 0x0063BFB0, 0x01B64FBC},  // TargetPropClearOfPeds
    {0x00938638, 0x00649E90, 0x0063C000, 0x01B64FC0},  // POIClearOfPeds
    {0x00938624, 0x00649ED0, 0x0063C040, 0x01B65A14},  // PropDisplayUseText
    {0x00938614, 0x00649F10, 0x0063C080, 0x01B64FC4},  // AllyIsInCombat
    {0x00938604, 0x00649F40, 0x0063C0C0, 0x01B64FC8},  // HasAllyState
    {0x009385F4, 0x00649F80, 0x0063C100, 0x01B64FCC},  // HasAllyMorale
    {0x009385E0, 0x00649FC0, 0x0063C150, 0x01B64FD0},  // MissionAvailable
    {0x009385CC, 0x00649FF0, 0x0063C1A0, 0x01B65A18},  // PropPedIsUsingMe
    {0x009385C0, 0x0063C1C0, 0x0063C1F0, 0x020C35A8},  // LastMission
    {0x009385B4, 0x0064A030, 0x0063C220, 0x01B64FD4},  // NumWeapons
    {0x009385A4, 0x0064A070, 0x0063C260, 0x01B65BE4},  // CameraFading
    {0x00938590, 0x0064A0A0, 0x0063C2A0, 0x01B64FD8},  // HasGrappleTarget
    {0x00938580, 0x0064A0D0, 0x0063C2E0, 0x01B64FDC},  // HasNodeCooldown
    {0x00938570, 0x0064A110, 0x0063C320, 0x01B64FE0},  // IsIgnoreTruants
    {0x0093855C, 0x0064A140, 0x0063C360, 0x01B64FE4},  // ThisNodeIsPlaying
    {0x00938544, 0x0064A180, 0x0063C3A0, 0x01B64FE8},  // ThisNodeIsNotPlaying
    {0x00938534, 0x0064A1C0, 0x0063C3E0, 0x01B64FEC},  // PlayerHasTarget
    {0x00938520, 0x0064A1F0, 0x0063C420, 0x01B65BAC},  // TargetMyTargetsProp
    {0x00938508, 0x0064A220, 0x0063C460, 0x01B64FF0},  // NumAuthorityInPursuit
    {0x009384F8, 0x0064A260, 0x0063C480, 0x01B64FF4},  // IsTagWaiting
    {0x009384E8, 0x0064A290, 0x0063C4A0, 0x01B64FF8},  // IsTagDrawing
    {0x009384D0, 0x0064A2C0, 0x0063C4E0, 0x01B64FFC},  // IsNISorCutsceneRunning
    {0x009384B8, 0x0064A300, 0x0063C500, 0x01B65000},  // IsPlayerFacingMirror
    {0x009384A0, 0x0064A340, 0x0063C540, 0x01B65BA0},  // PropTargetIsTwoHanded
    {0x00938490, 0x0064A380, 0x0063C560, 0x01B64C3C},  // ObjectiveOfType
    {0x00938480, 0x0064A3C0, 0x0063C580, 0x01B64C0C},  // SenseTargetNow
    {0x00938464, 0x0064A3F0, 0x0063C5C0, 0x01B65C04},  // IsWeaponFiredWhenDropped
    {0x00938458, 0x0064A420, 0x0063C600, 0x01B65004},  // IsElderly
    {0x0093843C, 0x0064A450, 0x0063C640, 0x01B65008},  // IsRegisteredAsGirlfriend
    {0x00938430, 0x0064A480, 0x0063C680, 0x01B6500C},  // IsInFight
    {0x0090EE00, 0x0064A4B0, 0x0063C6C0, 0x01B65010},  // ScriptSaveData
    {0x00938414, 0x0064A4F0, 0x0063C700, 0x01B65014},  // PlayerHasPunishmentOfType
    {0x009383FC, 0x0064A520, 0x0063C740, 0x01B65018},  // IsMiniObjectiveComplete
    {0x009383F0, 0x0064A560, 0x0063C780, 0x01B6501C},  // IsAvoiding
    {0x009383E0, 0x0064A5A0, 0x0063C7C0, 0x01B65020},  // FrontEndActive
    {0x009383CC, 0x0064A5E0, 0x0063C800, 0x01B65024},  // PedHasNumAttackers
    {0x009383B4, 0x0064A620, 0x0063C840, 0x01B65C08},  // IsTakingPictureFreeze
    {0x009383A8, 0x0064A650, 0x0063C860, 0x01B65028},  // IsUsingDoor
};
static_assert(sizeof(kConditionRows) / sizeof(kConditionRows[0]) == 0x1A3);

// 0x64B1F0 (cdecl): every condition registered: an entry (+0 the name's key, +4 New, +8
// CreateAt) added to the registry, and the name's key set in its global too
void __cdecl RegisterAll() {
    Registry_Reserve(conditionRegistry);
    for (const ConditionRow& row : kConditionRows) {
        uint32_t* entry = static_cast<uint32_t*>(Malloc(0xC));
        if (entry) {
            NameKey_Default(entry);
        }
        // (a failed allocation is written at null, as the game does)
        const char* name = reinterpret_cast<const char*>(uintptr_t(row.name));
        NameKey_FromString(entry, name);
        At<uint32_t>(reinterpret_cast<uint8_t*>(entry), 8) = row.createAt;
        At<uint32_t>(reinterpret_cast<uint8_t*>(entry), 4) = row.make;
        NameKey_FromString(reinterpret_cast<uint32_t*>(uintptr_t(row.key)), name);
        Registry_Add(conditionRegistry, entry);
    }
}

} // namespace

// (variadic: the template arguments' commas)
#define CONDFAC_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

CONDFAC_REVERSED("ActionConditions::RegisterAll", 0x0064B1F0, &RegisterAll);

// CreateAt (cdecl, the memory)
CONDFAC_REVERSED("ConditionCameraAllowScriptedChange::CreateAt", 0x006373F0, &CreateAt<CondBase, Vt(0x00936554)>);
CONDFAC_REVERSED("ConditionWeaponDesired::CreateAt", 0x00637440, &CreateVia<0x0065AD90>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionWeaponDesiredSocket::CreateAt", 0x00637470, &CreateAt<CondBase, Vt(0x00936568), Key(0x4), Key(0x8)>);
CONDFAC_REVERSED("ConditionWeaponDesiredIsEquiped::CreateAt", 0x006374C0, &CreateAt<CondBase, Vt(0x0093657C)>);
CONDFAC_REVERSED("ConditionWeaponDesiredIsFocusWeapon::CreateAt", 0x00637500, &CreateAt<CondBase, Vt(0x00936590)>);
CONDFAC_REVERSED("ConditionWeaponEquiped::CreateAt", 0x00637540, &CreateAt<CondBase, Vt(0x009365A4), Key(0x4)>);
CONDFAC_REVERSED("ConditionWeaponEquipedType::CreateAt", 0x00637590, &CreateAt<CondBase, Vt(0x009365B8)>);
CONDFAC_REVERSED("ConditionWeaponEquipedSocket::CreateAt", 0x006375D0, &CreateAt<CondBase, Vt(0x009365CC), Key(0x8)>);
CONDFAC_REVERSED("ConditionLastWeapon::CreateAt", 0x00637630, &CreateVia<0x0065AE40>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionWeaponDamaged::CreateAt", 0x00637660, &CreateAt<CondBase, Vt(0x009365E0)>);
CONDFAC_REVERSED("ConditionWeaponAmmo::CreateAt", 0x006376A0, &CreateAt<CondBase, Vt(0x009365F4)>);
CONDFAC_REVERSED("ConditionWeaponCount::CreateAt", 0x006376F0, &CreateVia<0x0065AAC0>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionPropAttached::CreateAt", 0x00637720, &CreateAt<CondBase, Vt(0x00936608), Key(0x4)>);
CONDFAC_REVERSED("ConditionWeaponModelRequest::CreateAt", 0x00637770, &CreateAt<CondBase, Vt(0x0093661C), Key(0x4)>);
CONDFAC_REVERSED("ConditionCollisionWeaponType::CreateAt", 0x006377D0, &CreateVia<0x0065AB20>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionAltWeaponCollisionType::CreateAt", 0x00637810, &CreateVia<0x0065AC70>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionDamageFromExplosion::CreateAt", 0x00637840, &CreateAt<CondBase, Vt(0x00936630)>);
CONDFAC_REVERSED("ConditionDamageFromBurningFire::CreateAt", 0x00637880, &CreateAt<CondBase, Vt(0x00936644)>);
CONDFAC_REVERSED("ConditionFireInRange::CreateAt", 0x006378C0, &CreateAt<CondBase, Vt(0x00936658)>);
CONDFAC_REVERSED("ConditionWeaponIsThrowable::CreateAt", 0x006378E0, &CreateAt<CondBase, Vt(0x0093666C)>);
CONDFAC_REVERSED("ConditionCameraBlending::CreateAt", 0x00637920, &CreateAt<CondBase, Vt(0x0093667C)>);
CONDFAC_REVERSED("ConditionTargetProp::CreateAt", 0x00637960, &CreateAt<CondBase, Vt(0x00936690), Key(0x10), Key(0x14), Key(0x1C)>);
CONDFAC_REVERSED("ConditionWeaponNeedInit::CreateAt", 0x006379A0, &CreateAt<CondBase, Vt(0x009366A4)>);
CONDFAC_REVERSED("ConditionWeaponIsInventory::CreateAt", 0x006379C0, &CreateAt<CondBase, Vt(0x009366B4)>);
CONDFAC_REVERSED("ConditionWeaponHudActive::CreateAt", 0x006379E0, &CreateAt<CondBase, Vt(0x009366C4)>);
CONDFAC_REVERSED("ConditionWeaponHudLocked::CreateAt", 0x00637A00, &CreateAt<CondBase, Vt(0x009366D4)>);
CONDFAC_REVERSED("ConditionWeaponWheelOn::CreateAt", 0x00637A20, &CreateAt<CondBase, Vt(0x009366E4)>);
CONDFAC_REVERSED("ConditionHudComponentVisible::CreateAt", 0x00637A40, &CreateAt<CondBase, Vt(0x009366F4)>);
CONDFAC_REVERSED("ConditionPropLifeTime::CreateAt", 0x00637A80, &CreateAt<CondBase, Vt(0x00936704)>);
CONDFAC_REVERSED("ConditionPropSpecialState::CreateAt", 0x00637AC0, &CreateAt<CondBase, Vt(0x00936718)>);
CONDFAC_REVERSED("ConditionPropTargetHeight::CreateAt", 0x00637B00, &CreateAt<CondBase, Vt(0x0093672C)>);
CONDFAC_REVERSED("ConditionTargetProjectile::CreateAt", 0x00637B40, &CreateAt<CondBase, Vt(0x00936740), Key(0x8)>);
CONDFAC_REVERSED("ConditionCameraDesired::CreateAt", 0x00637B90, &CreateAt<CondBase, Vt(0x00936754)>);
CONDFAC_REVERSED("ConditionCameraActive::CreateAt", 0x00637BB0, &CreateAt<CondBase, Vt(0x00936768)>);
CONDFAC_REVERSED("ConditionCameraLocked::CreateAt", 0x00637BF0, &CreateAt<CondBase, Vt(0x00936778)>);
CONDFAC_REVERSED("ConditionCameraInWidescreen::CreateAt", 0x00637C10, &CreateAt<CondBase, Vt(0x0093678C)>);
CONDFAC_REVERSED("ConditionButtonState::CreateAt", 0x00637C30, &CreateAt<CondBase, Vt(0x0093679C)>);
CONDFAC_REVERSED("ConditionCameraDesiredReset::CreateAt", 0x00637C70, &CreateAt<CondBase, Vt(0x009367AC)>);
CONDFAC_REVERSED("ConditionHasCash::CreateAt", 0x00637CB0, &CreateAt<CondBase, Vt(0x009367C0)>);
CONDFAC_REVERSED("ConditionEvaluatedEntityType::CreateAt", 0x00637D00, &CreateVia<0x0065AA70>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionGetProjectileOwner::CreateAt", 0x00637D30, &CreateAt<CondBase, Vt(0x009367D4)>);
CONDFAC_REVERSED("ConditionGetPassingPed::CreateAt", 0x00637D70, &CreateAt<CondBase, Vt(0x009367E8)>);
CONDFAC_REVERSED("ConditionEntityIsModel::CreateAt", 0x00637DF0, &CreateAt<CondBase, Vt(0x0093446C), Key(0x4), I32(0x8, 0xFFFFFFFE)>);
CONDFAC_REVERSED("ConditionEntitySpeed::CreateAt", 0x00637E40, &CreateAt<CondBase, Vt(0x009367FC)>);
CONDFAC_REVERSED("CatchProjectileCond::CreateAt", 0x00637E80, &CreateAt<CondBase, Vt(0x00936810)>);
CONDFAC_REVERSED("AttackPowerCond::CreateAt", 0x00637EC0, &CreateAt<CondBase, Vt(0x00936824)>);
CONDFAC_REVERSED("ConditionTargetInteractiveProp::CreateAt", 0x00637F00, &CreateAt<CondBase, Vt(0x00936838), Key(0x8)>);
CONDFAC_REVERSED("ConditionPedNearPropSocket::CreateAt", 0x00637F50, &CreateAt<CondBase, Vt(0x0093684C), Key(0x10), Key(0x18), Key(0x1C), Key(0x20)>);
CONDFAC_REVERSED("ConditionPropHealth::CreateAt", 0x00637FB0, &CreateAt<CondBase, Vt(0x00936860)>);
CONDFAC_REVERSED("ConditionPropFlags::CreateAt", 0x00637FF0, &CreateAt<CondBase, Vt(0x00936874)>);
CONDFAC_REVERSED("ConditionPropTagPercentage::CreateAt", 0x00638030, &CreateAt<CondBase, Vt(0x00936888)>);
CONDFAC_REVERSED("ConditionPropModelLoaded::CreateAt", 0x00638070, &CreateAt<CondBase, Vt(0x0093689C), Key(0x4)>);
CONDFAC_REVERSED("ConditionActionNodePlayTime::CreateAt", 0x006380A0, &CreateAt<CondBase, Vt(0x009368B0)>);
CONDFAC_REVERSED("ConditionActionNodePlayTimePast::CreateAt", 0x006380E0, &CreateAt<CondBase, Vt(0x009368C0)>);
CONDFAC_REVERSED("ConditionActionIntentionActionRequest::CreateAt", 0x00638120, &CreateAt<CondBase, Vt(0x009368D4)>);
CONDFAC_REVERSED("ConditionIntentionActionNodeRequest::CreateAt", 0x00638160, &CreateAt<CondBase, Vt(0x009368E8)>);
CONDFAC_REVERSED("ConditionActionRequestButton::CreateAt", 0x006381A0, &CreateAt<CondBase, Vt(0x009368FC)>);
CONDFAC_REVERSED("ConditionAcquireAuthorityFocus::CreateAt", 0x006381C0, &CreateAt<CondBase, Vt(0x00936910)>);
CONDFAC_REVERSED("ConditionAcquireFactionFocus::CreateAt", 0x006381E0, &CreateAt<CondBase, Vt(0x00936920)>);
CONDFAC_REVERSED("ConditionAcquirePOI::CreateAt", 0x00638200, &CreateAt<CondBase, Vt(0x00936930)>);
CONDFAC_REVERSED("ConditionAcquirePunishableTarget::CreateAt", 0x00638220, &CreateAt<CondBase, Vt(0x00936940)>);
CONDFAC_REVERSED("ConditionAcquireRandomTargetInGroup::CreateAt", 0x00638240, &CreateAt<CondBase, Vt(0x00936950)>);
CONDFAC_REVERSED("ConditionAcquireFocus::CreateAt", 0x00638260, &CreateAt<CondBase, Vt(0x00936960)>);
CONDFAC_REVERSED("ConditionAcquireTarget::CreateAt", 0x00638280, &CreateAt<CondBase, Vt(0x00936970)>);
CONDFAC_REVERSED("ConditionAIIsPlaying::CreateAt", 0x006382A0, &CreateAt<CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00936980)>);
CONDFAC_REVERSED("ConditionAnimationGroupLoaded::CreateAt", 0x006382D0, &CreateAt<CondBase, Vt(0x0092E704), Key(0x4)>);
CONDFAC_REVERSED("ConditionAnimationParametric::CreateAt", 0x00638320, &CreateAt<CondBase, Vt(0x00936994)>);
CONDFAC_REVERSED("ConditionCutsceneSoundLoaded::CreateAt", 0x00638370, &CreateAt<CondBase, Vt(0x0092E718), I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionIsMusicStreamLoaded::CreateAt", 0x006383D0, &CreateAt<CondBase, Vt(0x0092E72C), I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionIsSoundBankLoaded::CreateAt", 0x00638440, &CreateAt<CondBase, Vt(0x0092E740), Key(0x4), I32(0x8, 0x0)>);
CONDFAC_REVERSED("ConditionIsSpeechBusy::CreateAt", 0x006384A0, &CreateAt<CondBase, Vt(0x0092E754)>);
CONDFAC_REVERSED("ConditionIsSpeechPreloaded::CreateAt", 0x00638500, &CreateAt<CondBase, Vt(0x0092E77C)>);
CONDFAC_REVERSED("ConditionIsSpeechStreamFree::CreateAt", 0x00638550, &CreateAt<CondBase, Vt(0x0092E768)>);
CONDFAC_REVERSED("ConditionHasSpeechEvent::CreateAt", 0x006385B0, &CreateAt<CondBase, Vt(0x0092E7A4)>);
CONDFAC_REVERSED("ConditionAttacked::CreateAt", 0x006385F0, &CreateAt<CondBase, Vt(0x009369A8)>);
CONDFAC_REVERSED("ConditionAttackRole::CreateAt", 0x00638630, &CreateAt<CondBase, Vt(0x009369BC)>);
CONDFAC_REVERSED("ConditionAttackPhase::CreateAt", 0x00638670, &CreateAt<CondBase, Vt(0x009369D0)>);
CONDFAC_REVERSED("ConditionAttackTimerPlayTime::CreateAt", 0x006386B0, &CreateAt<CondBase, Vt(0x009369E4)>);
CONDFAC_REVERSED("ConditionAttacksDisabled::CreateAt", 0x006386D0, &CreateAt<CondBase, Vt(0x009369F8)>);
CONDFAC_REVERSED("ConditionBlockTimerPlayTime::CreateAt", 0x00638710, &CreateAt<CondBase, Vt(0x00936A08)>);
CONDFAC_REVERSED("ConditionEvadeTimerPlayTime::CreateAt", 0x00638750, &CreateAt<CondBase, Vt(0x00936A1C)>);
CONDFAC_REVERSED("ConditionCollisionStart::CreateAt", 0x00638790, &CreateAt<CondBase, Vt(0x00936A30)>);
CONDFAC_REVERSED("ConditionCharacterSize::CreateAt", 0x006387D0, &CreateAt<CondBase, Vt(0x00936A44)>);
CONDFAC_REVERSED("ConditionTargetRelativeCharacterSize::CreateAt", 0x00638810, &CreateAt<CondBase, Vt(0x00936A58)>);
CONDFAC_REVERSED("ConditionHitReactionEnd::CreateAt", 0x00638850, &CreateAt<CondBase, Vt(0x00936A6C)>);
CONDFAC_REVERSED("ConditionCollisionFlag::CreateAt", 0x006388A0, &CreateAt<CondBase, Vt(0x00936A80), Key(0x4)>);
CONDFAC_REVERSED("ConditionGroupCondition::CreateAt", 0x006388D0, &CreateAt<CondBase, Vt(0x0092E140)>);
CONDFAC_REVERSED("ConditionController::CreateAt", 0x00638910, &CreateAt<CondBase, Vt(0x00936A94)>);
CONDFAC_REVERSED("ConditionDistToTarget::CreateAt", 0x00638950, &CreateAt<CondBase, Vt(0x00936AA8)>);
CONDFAC_REVERSED("ConditionEngageDistance::CreateAt", 0x00638990, &CreateAt<CondBase, Vt(0x00936ABC)>);
CONDFAC_REVERSED("ConditionFactionScan::CreateAt", 0x006389D0, &CreateAt<CondBase, Vt(0x00936AD0)>);
CONDFAC_REVERSED("ConditionFactionNear::CreateAt", 0x00638A10, &CreateAt<CondBase, Vt(0x00936AE4)>);
CONDFAC_REVERSED("ConditionFindTandemHelper::CreateAt", 0x00638A50, &CreateAt<CondBase, Vt(0x00936AF8)>);
CONDFAC_REVERSED("ConditionIsTandemHelper::CreateAt", 0x00638A90, &CreateAt<CondBase, Vt(0x00936B0C)>);
CONDFAC_REVERSED("ConditionFalse::CreateAt", 0x00638AB0, &CreateAt<CondBase, Vt(0x00936B20)>);
CONDFAC_REVERSED("ConditionFightToken::CreateAt", 0x00638AF0, &CreateAt<CondBase, Vt(0x00936B30)>);
CONDFAC_REVERSED("ConditionHavePOI::CreateAt", 0x00638B10, &CreateAt<CondBase, Vt(0x00936B44)>);
CONDFAC_REVERSED("ConditionHavePOIOfType::CreateAt", 0x00638B30, &CreateAt<CondBase, Vt(0x00936B54)>);
CONDFAC_REVERSED("ConditionFightClass::CreateAt", 0x00638B70, &CreateAt<CondBase, Vt(0x00936B64)>);
CONDFAC_REVERSED("ConditionFocusAttacked::CreateAt", 0x00638BB0, &CreateAt<CondBase, Vt(0x00936B78)>);
CONDFAC_REVERSED("ConditionGrapplingHookAnchord::CreateAt", 0x00638BF0, &CreateAt<CondBase, Vt(0x00936B8C)>);
CONDFAC_REVERSED("CClassSkippedCond::CreateAt", 0x00638C30, &CreateAt<CondBase, Vt(0x00936BA0)>);
CONDFAC_REVERSED("ConditionFirstPerson::CreateAt", 0x00638C70, &CreateAt<CondBase, Vt(0x00936BB4)>);
CONDFAC_REVERSED("ConditionHavePath::CreateAt", 0x00638C90, &CreateAt<CondBase, Vt(0x00936BC8)>);
CONDFAC_REVERSED("ConditionHasSweptSphereCollision::CreateAt", 0x00638CB0, &CreateAt<CondBase, Vt(0x0092E7CC)>);
CONDFAC_REVERSED("ConditionHealth::CreateAt", 0x00638CF0, &CreateAt<CondBase, Vt(0x00936BD8)>);
CONDFAC_REVERSED("ConditionPercentageHealth::CreateAt", 0x00638D30, &CreateAt<CondBase, Vt(0x00936BEC)>);
CONDFAC_REVERSED("ConditionPercentageSpecial::CreateAt", 0x00638D70, &CreateAt<CondBase, Vt(0x00936C00)>);
CONDFAC_REVERSED("ConditionDamage::CreateAt", 0x00638DB0, &CreateAt<CondBase, Vt(0x00936C14)>);
CONDFAC_REVERSED("ConditionDamagePending::CreateAt", 0x00638DF0, &CreateAt<CondBase, Vt(0x00936C28)>);
CONDFAC_REVERSED("ConditionHasActionNode::CreateAt", 0x00638E10, &CreateAt<CondBase, Vt(0x0092E790), Key(0x4)>);
CONDFAC_REVERSED("ConditionOpeningBranchChildHasChild::CreateAt", 0x00638E40, &CreateAt<CondBase, Vt(0x0092E7B8), Key(0x4), Key(0x8)>);
CONDFAC_REVERSED("ConditionWallHitDirection::CreateAt", 0x00638E90, &CreateAt<CondBase, Vt(0x00936C3C)>);
CONDFAC_REVERSED("ConditionPedIsInOutfit::CreateAt", 0x00638EE0, &CreateAt<CondBase, Vt(0x00936C50), Key(0x4)>);
CONDFAC_REVERSED("ConditionHitByPed::CreateAt", 0x00638F30, &CreateAt<CondBase, Vt(0x00936C64)>);
CONDFAC_REVERSED("ConditionHitTime::CreateAt", 0x00638F70, &CreateAt<CondBase, Vt(0x00936C78)>);
CONDFAC_REVERSED("ConditionHitFlag::CreateAt", 0x00638FB0, &CreateAt<CondBase, Vt(0x00936C8C)>);
CONDFAC_REVERSED("ConditionHitStunTime::CreateAt", 0x00638FF0, &CreateAt<CondBase, Vt(0x00936CA0)>);
CONDFAC_REVERSED("ConditionHitStunFinished::CreateAt", 0x00639030, &CreateAt<CondBase, Vt(0x00936CB4)>);
CONDFAC_REVERSED("ConditionHitFlagClear::CreateAt", 0x00639070, &CreateAt<CondBase, Vt(0x00936CC8)>);
CONDFAC_REVERSED("ConditionIgnoreAttacks::CreateAt", 0x006390B0, &CreateAt<CondBase, Vt(0x00936CDC)>);
CONDFAC_REVERSED("ConditionInAttackCone::CreateAt", 0x006390F0, &CreateAt<CondBase, Vt(0x00936CF0)>);
CONDFAC_REVERSED("ConditionInBikeRace::CreateAt", 0x00639110, &CreateAt<CondBase, Vt(0x00936D04)>);
CONDFAC_REVERSED("ConditionVisibleArea::CreateAt", 0x00639130, &CreateAt<CondBase, Vt(0x00936D14)>);
CONDFAC_REVERSED("ConditionLiquidLevel::CreateAt", 0x00639150, &CreateAt<CondBase, Vt(0x0092E5E0)>);
CONDFAC_REVERSED("ConditionLiquidType::CreateAt", 0x00639170, &CreateAt<CondBase, Vt(0x0092E5F4)>);
CONDFAC_REVERSED("ConditionSurfaceHardness::CreateAt", 0x00639190, &CreateAt<CondBase, Vt(0x0092E608)>);
CONDFAC_REVERSED("ConditionIsAuthority::CreateAt", 0x006391D0, &CreateAt<CondBase, Vt(0x00936D24)>);
CONDFAC_REVERSED("ConditionIsFemale::CreateAt", 0x00639210, &CreateAt<CondBase, Vt(0x00936D38)>);
CONDFAC_REVERSED("ConditionIsHangingOut::CreateAt", 0x00639230, &CreateAt<CondBase, Vt(0x00936D4C)>);
CONDFAC_REVERSED("ConditionIsHealthAtHumiliationLevel::CreateAt", 0x00639270, &CreateAt<CondBase, Vt(0x00936D5C)>);
CONDFAC_REVERSED("ConditionIsInFactionWithStimulusObject::CreateAt", 0x00639290, &CreateAt<CondBase, Vt(0x00936D70)>);
CONDFAC_REVERSED("ConditionIsMissionSpecific::CreateAt", 0x006392D0, &CreateAt<CondBase, Vt(0x00936D80)>);
CONDFAC_REVERSED("ConditionIsScriptedAmbient::CreateAt", 0x006392F0, &CreateAt<CondBase, Vt(0x00936D94)>);
CONDFAC_REVERSED("ConditionIsTargetAttackRightsTaken::CreateAt", 0x00639310, &CreateAt<CondBase, Vt(0x00936DA4)>);
CONDFAC_REVERSED("ConditionIsStationary::CreateAt", 0x00639330, &CreateAt<CondBase, Vt(0x00936DB4)>);
CONDFAC_REVERSED("ConditionIsStimulusObject::CreateAt", 0x00639350, &CreateAt<CondBase, Vt(0x00936DC4)>);
CONDFAC_REVERSED("ConditionInCombatZone::CreateAt", 0x00639390, &CreateAt<CondBase, Vt(0x00936DD4)>);
CONDFAC_REVERSED("ConditionJoinCombatZone::CreateAt", 0x006393D0, &CreateAt<CondBase, Vt(0x00936DE8)>);
CONDFAC_REVERSED("ConditionWeaponLastHitBy::CreateAt", 0x006393F0, &CreateAt<CondBase, Vt(0x00936DFC), Key(0x4)>);
CONDFAC_REVERSED("ConditionMePlaying::CreateAt", 0x00639440, &CreateAt<CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00936E0C)>);
CONDFAC_REVERSED("ConditionMePlayingDefaultNeutralState::CreateAt", 0x00639470, &CreateAt<CondBase, Vt(0x0092E5CC)>);
CONDFAC_REVERSED("ConditionIsDayTime::CreateAt", 0x006394B0, &CreateAt<CondBase, Vt(0x00936E20)>);
CONDFAC_REVERSED("ConditionMotionCycle::CreateAt", 0x006394F0, &CreateAt<CondBase, Vt(0x00936E34)>);
CONDFAC_REVERSED("ConditionMotionIntentionDirection::CreateAt", 0x00639530, &CreateAt<CondBase, Vt(0x00936E48)>);
CONDFAC_REVERSED("ConditionMotionIntentionSpeed::CreateAt", 0x00639570, &CreateAt<CondBase, Vt(0x00936E5C)>);
CONDFAC_REVERSED("ConditionNearbyDockLocation::CreateAt", 0x00639590, &CreateAt<CondBase, Vt(0x00936E70)>);
CONDFAC_REVERSED("ConditionNearbyFleeDockLocation::CreateAt", 0x006395B0, &CreateAt<CondBase, Vt(0x00936E80)>);
CONDFAC_REVERSED("ConditionNOT::CreateAt", 0x006395D0, &CreateAt<CondBase, Vt(0x00936E90)>);
CONDFAC_REVERSED("ConditionOR::CreateAt", 0x006395F0, &CreateAt<CondBase, Vt(0x00936EA0)>);
CONDFAC_REVERSED("ConditionObjective::CreateAt", 0x00639610, &CreateAt<CondBase, Vt(0x0092E680)>);
CONDFAC_REVERSED("ConditionInCar::CreateAt", 0x00639630, &CreateAt<CondBase, Vt(0x00936EB0)>);
CONDFAC_REVERSED("ConditionOnBike::CreateAt", 0x00639650, &CreateAt<CondBase, Vt(0x00936EC0)>);
CONDFAC_REVERSED("ConditionOnVehicleOfClass::CreateAt", 0x00639690, &CreateAt<CondBase, Vt(0x00936ED0)>);
CONDFAC_REVERSED("ConditionPedFightingSystem::CreateAt", 0x006396D0, &CreateAt<CondBase, Vt(0x00936EE4)>);
CONDFAC_REVERSED("ConditionPedFlags::CreateAt", 0x00639710, &CreateAt<CondBase, Vt(0x00936EF8)>);
CONDFAC_REVERSED("ConditionSetPedFlags::CreateAt", 0x00639750, &CreateAt<CondBase, Vt(0x00936F0C)>);
CONDFAC_REVERSED("ConditionPedModelID::CreateAt", 0x00639770, &CreateAt<CondBase, Vt(0x0092E6DC), Key(0x4)>);
CONDFAC_REVERSED("ConditionPedType::CreateAt", 0x006397A0, &CreateAt<CondBase, Vt(0x0092E66C)>);
CONDFAC_REVERSED("ConditionPedIsDriver::CreateAt", 0x006397E0, &CreateAt<CondBase, Vt(0x00936F20)>);
CONDFAC_REVERSED("ConditionPrint::CreateAt", 0x00639800, &CreateAt<CondBase, Vt(0x0092E12C), Key(0x4)>);
CONDFAC_REVERSED("ConditionProjectile::CreateAt", 0x00639850, &CreateAt<CondBase, Vt(0x00936F34)>);
CONDFAC_REVERSED("ConditionRandom::CreateAt", 0x00639890, &CreateAt<CondBase, Vt(0x00936F48)>);
CONDFAC_REVERSED("ConditionWeightedRandom::CreateAt", 0x006398B0, &CreateAt<CondBase, Vt(0x0092E6A8)>);
CONDFAC_REVERSED("ConditionRangeToTarget::CreateAt", 0x006398F0, &CreateAt<CondBase, Vt(0x00936F5C)>);
CONDFAC_REVERSED("ConditionTimeSincePedalling::CreateAt", 0x00639930, &CreateAt<CondBase, Vt(0x00936F70)>);
CONDFAC_REVERSED("ConditionBikeSprintAmount::CreateAt", 0x00639970, &CreateAt<CondBase, Vt(0x00936F84)>);
CONDFAC_REVERSED("ConditionRangeToFocus::CreateAt", 0x006399B0, &CreateAt<CondBase, Vt(0x00936F98)>);
CONDFAC_REVERSED("ConditionRangeToObjectiveVehicle::CreateAt", 0x006399F0, &CreateAt<CondBase, Vt(0x00936FAC)>);
CONDFAC_REVERSED("ConditionSensePlayer::CreateAt", 0x00639A10, &CreateAt<CondBase, Vt(0x00936FC0)>);
CONDFAC_REVERSED("ConditionSensePlayerDuration::CreateAt", 0x00639A30, &CreateAt<CondBase, Vt(0x00936FD0)>);
CONDFAC_REVERSED("ConditionHearPlayerFootsteps::CreateAt", 0x00639A50, &CreateAt<CondBase, Vt(0x00936FE0)>);
CONDFAC_REVERSED("ConditionSenseAttacker::CreateAt", 0x00639A70, &CreateAt<CondBase, Vt(0x00936FF0)>);
CONDFAC_REVERSED("ConditionSizeOfGroup::CreateAt", 0x00639A90, &CreateAt<CondBase, Vt(0x00937000)>);
CONDFAC_REVERSED("ConditionSpecial::CreateAt", 0x00639AD0, &CreateAt<CondBase, Vt(0x00937010)>);
CONDFAC_REVERSED("ConditionSprint::CreateAt", 0x00639B10, &CreateAt<CondBase, Vt(0x00937024)>);
CONDFAC_REVERSED("ConditionSpecialPoints::CreateAt", 0x00639B50, &CreateAt<CondBase, Vt(0x00937038)>);
CONDFAC_REVERSED("ConditionStat::CreateAt", 0x00639B90, &CreateAt<CondBase, Vt(0x0093704C)>);
CONDFAC_REVERSED("ConditionFocusConditionGroup::CreateAt", 0x00639BB0, &CreateAt<CondBase, Vt(0x00937060)>);
CONDFAC_REVERSED("ConditionPedConditionGroup::CreateAt", 0x00639BD0, &CreateAt<CondBase, Vt(0x00937080)>);
CONDFAC_REVERSED("ConditionAttackerConditionGroup::CreateAt", 0x00639BF0, &CreateAt<CondBase, Vt(0x009370A0)>);
CONDFAC_REVERSED("ConditionStimulus::CreateAt", 0x00639C10, &CreateAt<CondBase, Vt(0x009370C0)>);
CONDFAC_REVERSED("ConditionStimulusRepeated::CreateAt", 0x00639C30, &CreateAt<CondBase, Vt(0x009370D0)>);
CONDFAC_REVERSED("ConditionTargetedByTarget::CreateAt", 0x00639C70, &CreateAt<CondBase, Vt(0x009370E0)>);
CONDFAC_REVERSED("ConditionTargetedByPlayer::CreateAt", 0x00639CB0, &CreateAt<CondBase, Vt(0x009370F4)>);
CONDFAC_REVERSED("ConditionTargetPlayersGF::CreateAt", 0x00639CF0, &CreateAt<CondBase, Vt(0x00937108)>);
CONDFAC_REVERSED("ConditionTargetConditionGroup::CreateAt", 0x00639D30, &CreateAt<CondBase, Vt(0x0093711C)>);
CONDFAC_REVERSED("ConditionTargetFaction::CreateAt", 0x00639D70, &CreateAt<CondBase, Vt(0x0093713C)>);
CONDFAC_REVERSED("ConditionTargetFriendsAttacker::CreateAt", 0x00639D90, &CreateAt<CondBase, Vt(0x00937150)>);
CONDFAC_REVERSED("ConditionTargetMastersAttacker::CreateAt", 0x00639DB0, &CreateAt<CondBase, Vt(0x00937160)>);
CONDFAC_REVERSED("ConditionTargetHealth::CreateAt", 0x00639DD0, &CreateAt<CondBase, Vt(0x0092E5A4)>);
CONDFAC_REVERSED("ConditionTargetIsVisible::CreateAt", 0x00639DF0, &CreateAt<CondBase, Vt(0x00937170)>);
CONDFAC_REVERSED("ConditionTargetPlaying::CreateAt", 0x00639E10, &CreateAt<CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00937180)>);
CONDFAC_REVERSED("ConditionTargetPlayTime::CreateAt", 0x00639E60, &CreateAt<CondBase, Vt(0x00937194)>);
CONDFAC_REVERSED("ConditionTargetRelativeOrientation::CreateAt", 0x00639EA0, &CreateAt<CondBase, Vt(0x009371A8)>);
CONDFAC_REVERSED("ConditionTargetRelativePosition::CreateAt", 0x00639EE0, &CreateAt<CondBase, Vt(0x009371BC)>);
CONDFAC_REVERSED("ConditionTargetRelativePedCollisionSpheres::CreateAt", 0x00639F20, &CreateAt<CondBase, Vt(0x009371D0)>);
CONDFAC_REVERSED("ConditionTargetRelativePositionInsideBox::CreateAt", 0x00639F60, &CreateAt<CondBase, Vt(0x009371E4)>);
CONDFAC_REVERSED("ConditionTargetsPunishmentPoints::CreateAt", 0x00639F80, &CreateAt<CondBase, Vt(0x009371F8)>);
CONDFAC_REVERSED("ConditionTargetStimulusSubject::CreateAt", 0x00639FA0, &CreateAt<CondBase, Vt(0x00937208)>);
CONDFAC_REVERSED("ConditionTargetStimulusObject::CreateAt", 0x00639FC0, &CreateAt<CondBase, Vt(0x00937218)>);
CONDFAC_REVERSED("ConditionTargetStimulusEntitySubject::CreateAt", 0x00639FE0, &CreateAt<CondBase, Vt(0x00937228)>);
CONDFAC_REVERSED("ConditionTargetStimulusEntityObject::CreateAt", 0x0063A000, &CreateAt<CondBase, Vt(0x00937238)>);
CONDFAC_REVERSED("ConditionTargetVehicle::CreateAt", 0x0063A040, &CreateAt<CondBase, Vt(0x00937248)>);
CONDFAC_REVERSED("ConditionTargetVehicleCar::CreateAt", 0x0063A080, &CreateAt<CondBase, Vt(0x0093725C)>);
CONDFAC_REVERSED("ConditionTargetVehicleBike::CreateAt", 0x0063A0C0, &CreateAt<CondBase, Vt(0x00937270)>);
CONDFAC_REVERSED("ConditionAttemptAnchorToVehicle::CreateAt", 0x0063A100, &CreateAt<CondBase, Vt(0x00937284)>);
CONDFAC_REVERSED("ConditionRelativePosition::CreateAt", 0x0063A140, &CreateAt<CondBase, Vt(0x00937298)>);
CONDFAC_REVERSED("ConditionVehicleWheelsOnGround::CreateAt", 0x0063A180, &CreateAt<CondBase, Vt(0x009372AC)>);
CONDFAC_REVERSED("ConditionTargetVehicleIsPlaying::CreateAt", 0x0063A1D0, &CreateAt<CondBase, Vt(0x009372C0), Key(0x4)>);
CONDFAC_REVERSED("ConditionInTriggerVolume::CreateAt", 0x0063A200, &CreateAt<CondBase, Vt(0x0092E61C), Key(0x4)>);
CONDFAC_REVERSED("ConditionPlayerInTriggerVolume::CreateAt", 0x0063A230, &CreateAt<CondBase, Vt(0x0092E630), Key(0x4)>);
CONDFAC_REVERSED("ConditionNearPoint::CreateAt", 0x0063A290, &CreateVia<0x005FC450>);  // (not in Ghidra's list)
CONDFAC_REVERSED("CMissionCond::CreateAt", 0x0063A2D0, &CreateVia<0x006A7EB0>);  // (not in Ghidra's list)
CONDFAC_REVERSED("CErrandCond::CreateAt", 0x0063A310, &CreateVia<0x006A72F0>);  // (not in Ghidra's list)
CONDFAC_REVERSED("CMissionUnlockedCond::CreateAt", 0x0063A350, &CreateVia<0x006A73D0>);  // (not in Ghidra's list)
CONDFAC_REVERSED("CTimeCond::CreateAt", 0x0063A3D0, &CreateAt<CondBase, Vt(0x009372D4)>);
CONDFAC_REVERSED("CPeriodCond::CreateAt", 0x0063A420, &CreateVia<0x006A7500>);  // (not in Ghidra's list)
CONDFAC_REVERSED("CDayCond::CreateAt", 0x0063A450, &CreateAt<CondBase, Vt(0x009372EC)>);
CONDFAC_REVERSED("CLuaCond::CreateAt", 0x0063A490, &CreateAt<CondBase, Vt(0x00937300), Key(0xC), Key(0x10)>);
CONDFAC_REVERSED("CConversationActiveCond::CreateAt", 0x0063A4E0, &CreateAt<CondBase, Vt(0x00937318)>);
CONDFAC_REVERSED("ConditionIsPlayer::CreateAt", 0x0063A520, &CreateAt<CondBase, Vt(0x00937330)>);
CONDFAC_REVERSED("ConditionPedIsTrigger::CreateAt", 0x0063A560, &CreateAt<CondBase, Vt(0x00937344)>);
CONDFAC_REVERSED("ConditionDriveForce::CreateAt", 0x0063A5A0, &CreateAt<CondBase, Vt(0x00937358)>);
CONDFAC_REVERSED("ConditionLastSurfaceOn::CreateAt", 0x0063A5E0, &CreateAt<CondBase, Vt(0x0093736C)>);
CONDFAC_REVERSED("ConditionSurfaceGrade::CreateAt", 0x0063A620, &CreateAt<CondBase, Vt(0x00937380)>);
CONDFAC_REVERSED("ConditionSeason::CreateAt", 0x0063A660, &CreateAt<CondBase, Vt(0x00937394)>);
CONDFAC_REVERSED("ConditionOnMission::CreateAt", 0x0063A680, &CreateAt<CondBase, Vt(0x0092E6F0), Key(0x4)>);
CONDFAC_REVERSED("ConditionDistToGround::CreateAt", 0x0063A6D0, &CreateAt<CondBase, Vt(0x009373A8)>);
CONDFAC_REVERSED("ConditionMissionSuccessCount::CreateAt", 0x0063A720, &CreateVia<0x005FCD40>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionIsTargetOnBike::CreateAt", 0x0063A730, &CreateAt<CondBase, Vt(0x009373BC)>);
CONDFAC_REVERSED("ConditionIsFocusOnBike::CreateAt", 0x0063A750, &CreateAt<CondBase, Vt(0x009373CC)>);
CONDFAC_REVERSED("ConditionIsObjectiveVehicleOwner::CreateAt", 0x0063A770, &CreateAt<CondBase, Vt(0x009373DC)>);
CONDFAC_REVERSED("ConditionLineOfFireClear::CreateAt", 0x0063A790, &CreateAt<CondBase, Vt(0x009373EC)>);
CONDFAC_REVERSED("ConditionIsGuarding::CreateAt", 0x0063A7B0, &CreateAt<CondBase, Vt(0x009373FC)>);
CONDFAC_REVERSED("ConditionIsTargetCrouching::CreateAt", 0x0063A7D0, &CreateAt<CondBase, Vt(0x0093740C)>);
CONDFAC_REVERSED("ConditionIsTargetOutOfReach::CreateAt", 0x0063A840, &CreateAt<CondBase, Vt(0x0093445C)>);
CONDFAC_REVERSED("ConditionIsStealthMissionChar::CreateAt", 0x0063A860, &CreateAt<CondBase, Vt(0x0093741C)>);
CONDFAC_REVERSED("ConditionSpotted::CreateAt", 0x0063A880, &CreateAt<CondBase, Vt(0x0093742C)>);
CONDFAC_REVERSED("ConditionWantToBeStealthy::CreateAt", 0x0063A8A0, &CreateAt<CondBase, Vt(0x0093743C)>);
CONDFAC_REVERSED("ConditionIsTethered::CreateAt", 0x0063A8C0, &CreateAt<CondBase, Vt(0x0093744C)>);
CONDFAC_REVERSED("ConditionIsInsideTether::CreateAt", 0x0063A8E0, &CreateAt<CondBase, Vt(0x0093745C)>);
CONDFAC_REVERSED("ConditionIsTargetInRange::CreateAt", 0x0063A900, &CreateAt<CondBase, Vt(0x0093746C)>);
CONDFAC_REVERSED("ConditionHaveDynamicPOI::CreateAt", 0x0063A920, &CreateAt<CondBase, Vt(0x0093747C)>);
CONDFAC_REVERSED("ConditionClothing::CreateAt", 0x0063A970, &CreateAt<CondBase, Vt(0x0093748C), Key(0x8), Key(0xC)>);
CONDFAC_REVERSED("ConditionTargetClothing::CreateAt", 0x0063A9C0, &CreateAt<CondBase, Vt(0x009374A0)>);
CONDFAC_REVERSED("ConditionClothingStyle::CreateAt", 0x0063AA00, &CreateAt<CondBase, Vt(0x009374B4)>);
CONDFAC_REVERSED("ConditionClothingWarmthFactor::CreateAt", 0x0063AA40, &CreateAt<CondBase, Vt(0x009374C8)>);
CONDFAC_REVERSED("ConditionClothingSpecialKiss::CreateAt", 0x0063AA80, &CreateAt<CondBase, Vt(0x009374DC)>);
CONDFAC_REVERSED("ConditionTargetRelativeHeight::CreateAt", 0x0063AAA0, &CreateAt<CondBase, Vt(0x0092E7F4)>);
CONDFAC_REVERSED("ConditionIsTargetProp::CreateAt", 0x0063AAC0, &CreateAt<CondBase, Vt(0x009374F0)>);
CONDFAC_REVERSED("ConditionInterval::CreateAt", 0x0063AAE0, &CreateAt<CondBase, Vt(0x0092E82C)>);
CONDFAC_REVERSED("ConditionCheckActionTimer::CreateAt", 0x0063AB00, &CreateAt<CondBase, Vt(0x00937500)>);
CONDFAC_REVERSED("ConditionCanLaunchMission::CreateAt", 0x0063AB20, &CreateAt<CondBase, Vt(0x00937510)>);
CONDFAC_REVERSED("ConditionCanSecondaryMissionPlay::CreateAt", 0x0063AB40, &CreateAt<CondBase, Vt(0x00937520)>);
CONDFAC_REVERSED("ConditionInClass::CreateAt", 0x0063AB60, &CreateAt<CondBase, Vt(0x00937530)>);
CONDFAC_REVERSED("ConditionIsPlayerNearby::CreateAt", 0x0063AB80, &CreateAt<CondBase, Vt(0x0092E83C)>);
CONDFAC_REVERSED("ConditionIsHungUp::CreateAt", 0x0063ABA0, &CreateAt<CondBase, Vt(0x00937540)>);
CONDFAC_REVERSED("ConditionNeedsToDive::CreateAt", 0x0063ABE0, &CreateAt<CondBase, Vt(0x00937550)>);
CONDFAC_REVERSED("ConditionHasNewAudibleStimulus::CreateAt", 0x0063AC00, &CreateAt<CondBase, Vt(0x00937564)>);
CONDFAC_REVERSED("ConditionTargetProxyProp::CreateAt", 0x0063AC20, &CreateAt<CondBase, Vt(0x0092E804)>);
CONDFAC_REVERSED("ConditionCanPropSetUsageState::CreateAt", 0x0063AC40, &CreateAt<CondBase, Vt(0x0092E818)>);
CONDFAC_REVERSED("ConditionStealthBehavior::CreateAt", 0x0063AC80, &CreateAt<CondBase, Vt(0x00937574)>);
CONDFAC_REVERSED("ConditionTargetVehicleIs::CreateAt", 0x0063ACC0, &CreateAt<CondBase, Vt(0x00937588)>);
CONDFAC_REVERSED("ConditionTargetVehicleIsLastVehicleUsed::CreateAt", 0x0063AD00, &CreateAt<CondBase, Vt(0x0093759C)>);
CONDFAC_REVERSED("ConditionTargetVehicleOwnerDifferentFaction::CreateAt", 0x0063AD40, &CreateAt<CondBase, Vt(0x009375B0)>);
CONDFAC_REVERSED("ConditionTargetVehicleIsAlreadyOccupied::CreateAt", 0x0063AD80, &CreateAt<CondBase, Vt(0x009375C4)>);
CONDFAC_REVERSED("ConditionTargetMyTargetsVehicle::CreateAt", 0x0063ADC0, &CreateAt<CondBase, Vt(0x009375D8)>);
CONDFAC_REVERSED("ConditionIsOnSnow::CreateAt", 0x0063ADE0, &CreateAt<CondBase, Vt(0x009375EC)>);
CONDFAC_REVERSED("ConditionFindSnow::CreateAt", 0x0063AE00, &CreateAt<CondBase, Vt(0x009375FC)>);
CONDFAC_REVERSED("ConditionRunningAttack::CreateAt", 0x0063AE20, &CreateAt<CondBase, Vt(0x0093760C)>);
CONDFAC_REVERSED("ConditionIsAllyLeader::CreateAt", 0x0063AE40, &CreateAt<CondBase, Vt(0x0093761C)>);
CONDFAC_REVERSED("ConditionIsAllyFollower::CreateAt", 0x0063AE60, &CreateAt<CondBase, Vt(0x0093762C)>);
CONDFAC_REVERSED("ConditionIsTargetAlly::CreateAt", 0x0063AE80, &CreateAt<CondBase, Vt(0x0093763C)>);
CONDFAC_REVERSED("ConditionCanRecruitTarget::CreateAt", 0x0063AEA0, &CreateAt<CondBase, Vt(0x0093764C)>);
CONDFAC_REVERSED("ConditionShouldSocialize::CreateAt", 0x0063AEC0, &CreateAt<CondBase, Vt(0x0093765C)>);
CONDFAC_REVERSED("ConditionIsSocialTarget::CreateAt", 0x0063AEE0, &CreateAt<CondBase, Vt(0x0093766C)>);
CONDFAC_REVERSED("ConditionSocialAnimIs::CreateAt", 0x0063AF00, &CreateAt<CondBase, Vt(0x0093767C)>);
CONDFAC_REVERSED("ConditionSocialEmotionIs::CreateAt", 0x0063AF20, &CreateAt<CondBase, Vt(0x0093768C)>);
CONDFAC_REVERSED("ConditionHumiliationGroupIs::CreateAt", 0x0063AF40, &CreateAt<CondBase, Vt(0x0093769C)>);
CONDFAC_REVERSED("ConditionIsSocializing::CreateAt", 0x0063AF60, &CreateAt<CondBase, Vt(0x009376AC)>);
CONDFAC_REVERSED("ConditionIsSocializingWithPlayer::CreateAt", 0x0063AF80, &CreateAt<CondBase, Vt(0x009376BC)>);
CONDFAC_REVERSED("ConditionIsWantingToSocialize::CreateAt", 0x0063AFA0, &CreateAt<CondBase, Vt(0x009376CC)>);
CONDFAC_REVERSED("ConditionIsEmotionTowardsTarget::CreateAt", 0x0063AFC0, &CreateAt<CondBase, Vt(0x009376DC)>);
CONDFAC_REVERSED("ConditionIsEmotionTowardsPlayer::CreateAt", 0x0063AFE0, &CreateAt<CondBase, Vt(0x009376EC)>);
CONDFAC_REVERSED("ConditionIsSocialStateWait::CreateAt", 0x0063B000, &CreateAt<CondBase, Vt(0x009376FC)>);
CONDFAC_REVERSED("ConditionIsSocialStateAct::CreateAt", 0x0063B020, &CreateAt<CondBase, Vt(0x0093770C)>);
CONDFAC_REVERSED("ConditionIsSocialInitialReaction::CreateAt", 0x0063B040, &CreateAt<CondBase, Vt(0x0093771C)>);
CONDFAC_REVERSED("ConditionIsSocialInitialAction::CreateAt", 0x0063B060, &CreateAt<CondBase, Vt(0x0093772C)>);
CONDFAC_REVERSED("ConditionIsReadyToBeKissed::CreateAt", 0x0063B080, &CreateAt<CondBase, Vt(0x0093773C)>);
CONDFAC_REVERSED("ConditionSocialTypeFAT::CreateAt", 0x0063B0A0, &CreateAt<CondBase, Vt(0x0093774C)>);
CONDFAC_REVERSED("ConditionSocialTypeGIRL::CreateAt", 0x0063B0C0, &CreateAt<CondBase, Vt(0x0093775C)>);
CONDFAC_REVERSED("ConditionSocialTypeBULLY::CreateAt", 0x0063B0E0, &CreateAt<CondBase, Vt(0x0093776C)>);
CONDFAC_REVERSED("ConditionSocialTypeNERD::CreateAt", 0x0063B100, &CreateAt<CondBase, Vt(0x0093777C)>);
CONDFAC_REVERSED("ConditionSocialTypeAUTH::CreateAt", 0x0063B120, &CreateAt<CondBase, Vt(0x0093778C)>);
CONDFAC_REVERSED("ConditionSocialTypeGEN::CreateAt", 0x0063B140, &CreateAt<CondBase, Vt(0x0093779C)>);
CONDFAC_REVERSED("ConditionSocialTypeOLD::CreateAt", 0x0063B160, &CreateAt<CondBase, Vt(0x009377AC)>);
CONDFAC_REVERSED("ConditionSocialTypeNERDGIRL::CreateAt", 0x0063B180, &CreateAt<CondBase, Vt(0x009377BC)>);
CONDFAC_REVERSED("ConditionIsKissCount::CreateAt", 0x0063B1A0, &CreateAt<CondBase, Vt(0x009377CC)>);
CONDFAC_REVERSED("ConditionTimeOfLastSocialAction::CreateAt", 0x0063B1C0, &CreateAt<CondBase, Vt(0x009377DC)>);
CONDFAC_REVERSED("ConditionIsTargetBusySocializing::CreateAt", 0x0063B1E0, &CreateAt<CondBase, Vt(0x009377EC)>);
CONDFAC_REVERSED("ConditionHaveIAssaultedTargetPed::CreateAt", 0x0063B200, &CreateAt<CondBase, Vt(0x009377FC)>);
CONDFAC_REVERSED("ConditionHaveIHarassedTargetPed::CreateAt", 0x0063B220, &CreateAt<CondBase, Vt(0x0093780C)>);
CONDFAC_REVERSED("ConditionHaveIDismissedTargetPed::CreateAt", 0x0063B240, &CreateAt<CondBase, Vt(0x0093781C)>);
CONDFAC_REVERSED("ConditionIsStimulusPriority::CreateAt", 0x0063B260, &CreateAt<CondBase, Vt(0x0093782C)>);
CONDFAC_REVERSED("ConditionIsStrafing::CreateAt", 0x0063B280, &CreateAt<CondBase, Vt(0x0093783C)>);
CONDFAC_REVERSED("ConditionTargetAttacker::CreateAt", 0x0063B2A0, &CreateAt<CondBase, Vt(0x0093784C)>);
CONDFAC_REVERSED("ConditionTargetByType::CreateAt", 0x0063B2C0, &CreateAt<CondBase, Vt(0x0093785C)>);
CONDFAC_REVERSED("ConditionBikeIsLocked::CreateAt", 0x0063B2E0, &CreateAt<CondBase, Vt(0x0093786C)>);
CONDFAC_REVERSED("ConditionBikeIsUpright::CreateAt", 0x0063B300, &CreateAt<CondBase, Vt(0x0093787C)>);
CONDFAC_REVERSED("ConditionWillAcceptGift::CreateAt", 0x0063B320, &CreateAt<CondBase, Vt(0x0093788C)>);
CONDFAC_REVERSED("ConditionPlayerHasRequiredGift::CreateAt", 0x0063B340, &CreateAt<CondBase, Vt(0x0093789C)>);
CONDFAC_REVERSED("ConditionStatInt::CreateAt", 0x0063B360, &CreateAt<CondBase, Vt(0x009378AC)>);
CONDFAC_REVERSED("ConditionStatFloat::CreateAt", 0x0063B380, &CreateAt<CondBase, Vt(0x009378BC)>);
CONDFAC_REVERSED("ConditionCheckTargetSemaphore::CreateAt", 0x0063B3A0, &CreateAt<CondBase, Vt(0x009378CC)>);
CONDFAC_REVERSED("ConditionDetectGrenade::CreateAt", 0x0063B3C0, &CreateAt<CondBase, Vt(0x009378DC)>);
CONDFAC_REVERSED("ConditionDetectProjectile::CreateAt", 0x0063B3E0, &CreateAt<CondBase, Vt(0x009378EC)>);
CONDFAC_REVERSED("ConditionInTriggerVolumeOfType::CreateAt", 0x0063B400, &CreateAt<CondBase, Vt(0x0092E644)>);
CONDFAC_REVERSED("ConditionIsVehicleInRange::CreateAt", 0x0063B420, &CreateAt<CondBase, Vt(0x009378FC)>);
CONDFAC_REVERSED("ConditionInGuardRadius::CreateAt", 0x0063B440, &CreateAt<CondBase, Vt(0x0093790C)>);
CONDFAC_REVERSED("ConditionAllyLeaderConditionGroup::CreateAt", 0x0063B460, &CreateAt<CondBase, Vt(0x0093791C)>);
CONDFAC_REVERSED("ConditionAllyFollowerConditionGroup::CreateAt", 0x0063B480, &CreateAt<CondBase, Vt(0x0093793C)>);
CONDFAC_REVERSED("ConditionMissionActive::CreateAt", 0x0063B4A0, &CreateAt<CondBase, Vt(0x0093795C)>);
CONDFAC_REVERSED("ConditionCanAcquireAlly::CreateAt", 0x0063B4C0, &CreateAt<CondBase, Vt(0x0093796C)>);
CONDFAC_REVERSED("ConditionSteering::CreateAt", 0x0063B500, &CreateAt<CondBase, Vt(0x0093797C)>);
CONDFAC_REVERSED("ConditionSteeringFraction::CreateAt", 0x0063B540, &CreateAt<CondBase, Vt(0x00937990)>);
CONDFAC_REVERSED("ConditionCheckWeatherType::CreateAt", 0x0063B560, &CreateAt<CondBase, Vt(0x009379A4)>);
CONDFAC_REVERSED("ConditionIsSpectator::CreateAt", 0x0063B580, &CreateAt<CondBase, Vt(0x009379B4)>);
CONDFAC_REVERSED("ConditionIsParametricStatic::CreateAt", 0x0063B5A0, &CreateAt<CondBase, Vt(0x009379C4)>);
CONDFAC_REVERSED("ConditionTargetAlly::CreateAt", 0x0063B5C0, &CreateAt<CondBase, Vt(0x009379D4)>);
CONDFAC_REVERSED("ConditionFindSurface::CreateAt", 0x0063B5E0, &CreateAt<CondBase, Vt(0x009379E4)>);
CONDFAC_REVERSED("ConditionInWeaponRange::CreateAt", 0x0063B600, &CreateAt<CondBase, Vt(0x009379F4)>);
CONDFAC_REVERSED("ConditionTargetPed::CreateAt", 0x0063B620, &CreateAt<CondBase, Vt(0x00937A04), Key(0x4)>);
CONDFAC_REVERSED("ConditionBalanceMeterPosition::CreateAt", 0x0063B670, &CreateAt<CondBase, Vt(0x00937A14)>);
CONDFAC_REVERSED("ConditionIsPlayerFatigued::CreateAt", 0x0063B6B0, &CreateAt<CondBase, Vt(0x00937A28)>);
CONDFAC_REVERSED("ConditionIsPlayerTired::CreateAt", 0x0063B6F0, &CreateAt<CondBase, Vt(0x00937A3C)>);
CONDFAC_REVERSED("ConditionTargetColliding::CreateAt", 0x0063B710, &CreateAt<CondBase, Vt(0x00937A50)>);
CONDFAC_REVERSED("ConditionPedInConversation::CreateAt", 0x0063B750, &CreateAt<CondBase, Vt(0x00937A60)>);
CONDFAC_REVERSED("ConditionPedConversationStatus::CreateAt", 0x0063B790, &CreateAt<CondBase, Vt(0x00937A74)>);
CONDFAC_REVERSED("ConditionPedConversationMood::CreateAt", 0x0063B7D0, &CreateAt<CondBase, Vt(0x00937A88)>);
CONDFAC_REVERSED("CanPathToEntityCond::CreateAt", 0x0063B7F0, &CreateAt<CondBase, Vt(0x0092E588)>);
CONDFAC_REVERSED("ConditionIsPatrolPed::CreateAt", 0x0063B830, &CreateAt<CondBase, Vt(0x00937A9C)>);
CONDFAC_REVERSED("ConditionPlayerIsInStealthProp::CreateAt", 0x0063B870, &CreateAt<CondBase, Vt(0x00937AB0)>);
CONDFAC_REVERSED("ConditionHasLineOfSightToPlayer::CreateAt", 0x0063B8B0, &CreateAt<CondBase, Vt(0x00937AC4)>);
CONDFAC_REVERSED("ConditionParametricTime::CreateAt", 0x0063B8F0, &CreateAt<CondBase, Vt(0x00937AD8)>);
CONDFAC_REVERSED("PedGroundDistCond::CreateAt", 0x0063B910, &CreateAt<CondBase, Vt(0x0092E84C)>);
CONDFAC_REVERSED("PedVelocityCond::CreateAt", 0x0063B930, &CreateAt<CondBase, Vt(0x0092E860)>);
CONDFAC_REVERSED("ConditionHasRangedWeapon::CreateAt", 0x0063B950, &CreateAt<CondBase, Vt(0x0092E874)>);
CONDFAC_REVERSED("ConditionHasTXDLoaded::CreateAt", 0x0063B970, &CreateAt<CondBase, Vt(0x0092E888), Key(0x4)>);
CONDFAC_REVERSED("ConditionPedSphereHit::CreateAt", 0x0063B9C0, &CreateAt<CondBase, Vt(0x00937AEC)>);
CONDFAC_REVERSED("ConditionIsRumbleMode_Ambient::CreateAt", 0x0063BA00, &CreateAt<CondBase, Vt(0x00937B00)>);
CONDFAC_REVERSED("ConditionIsRumbleMode_Mission::CreateAt", 0x0063BA40, &CreateAt<CondBase, Vt(0x00937B14)>);
CONDFAC_REVERSED("ConditionIsScenarioPed::CreateAt", 0x0063BA60, &CreateAt<CondBase, Vt(0x00937B28)>);
CONDFAC_REVERSED("ConditionHasCompletedScenarioObj::CreateAt", 0x0063BA80, &CreateAt<CondBase, Vt(0x00937B38)>);
CONDFAC_REVERSED("ConditionSocialScriptCallback::CreateAt", 0x0063BAA0, &CreateAt<CondBase, Vt(0x00937B48)>);
CONDFAC_REVERSED("ConditionUseSocialOverride::CreateAt", 0x0063BAC0, &CreateAt<CondBase, Vt(0x00937B58)>);
CONDFAC_REVERSED("ConditionPlayerUseSocialOverride::CreateAt", 0x0063BAE0, &CreateAt<CondBase, Vt(0x00937B68)>);
CONDFAC_REVERSED("ConditionSocialActionInterrupted::CreateAt", 0x0063BB00, &CreateAt<CondBase, Vt(0x00937B78)>);
CONDFAC_REVERSED("ConditionCanGetOffVehicleHereLeft::CreateAt", 0x0063BB40, &CreateAt<CondBase, Vt(0x00937B88)>);
CONDFAC_REVERSED("ConditionCanGetOffVehicleHere::CreateAt", 0x0063BB80, &CreateAt<CondBase, Vt(0x00937B9C)>);
CONDFAC_REVERSED("ConditionCanGetOnVehicleHere::CreateAt", 0x0063BBC0, &CreateAt<CondBase, Vt(0x00937BB0)>);
CONDFAC_REVERSED("ConditionCanGetOnVehicleLeftSide::CreateAt", 0x0063BC00, &CreateAt<CondBase, Vt(0x00937BC4)>);
CONDFAC_REVERSED("ConditionCanGetOnVehicleRightSide::CreateAt", 0x0063BC40, &CreateAt<CondBase, Vt(0x00937BD8)>);
CONDFAC_REVERSED("ConditionTestRequiredGiftType::CreateAt", 0x0063BC60, &CreateAt<CondBase, Vt(0x00937BEC)>);
CONDFAC_REVERSED("ConditionTestFutureGiftType::CreateAt", 0x0063BC80, &CreateAt<CondBase, Vt(0x00937BFC)>);
CONDFAC_REVERSED("ConditionHasGiftItemOfType::CreateAt", 0x0063BCA0, &CreateAt<CondBase, Vt(0x00937C0C)>);
CONDFAC_REVERSED("ConditionHasPlayerMaxItems::CreateAt", 0x0063BCC0, &CreateAt<CondBase, Vt(0x00937C1C), Key(0x4), Key(0x8)>);
CONDFAC_REVERSED("ConditionPlayerHasItemCount::CreateAt", 0x0063BCF0, &CreateAt<CondBase, Vt(0x00937C2C), Key(0x4)>);
CONDFAC_REVERSED("ConditionIsPowerSliding::CreateAt", 0x0063BD20, &CreateAt<CondBase, Vt(0x00937C3C)>);
CONDFAC_REVERSED("ConditionShouldAllowApology::CreateAt", 0x0063BD40, &CreateAt<CondBase, Vt(0x00937C4C)>);
CONDFAC_REVERSED("ConditionBeamLateralDeviation::CreateAt", 0x0063BD80, &CreateAt<CondBase, Vt(0x00937C5C)>);
CONDFAC_REVERSED("ConditionBeamLateralPosition::CreateAt", 0x0063BDC0, &CreateAt<CondBase, Vt(0x00937C70)>);
CONDFAC_REVERSED("ConditionWeaponFlags::CreateAt", 0x0063BE00, &CreateAt<CondBase, Vt(0x00937C84)>);
CONDFAC_REVERSED("ConditionWeaponCanBeUsedOnBike::CreateAt", 0x0063BE40, &CreateAt<CondBase, Vt(0x00937C98)>);
CONDFAC_REVERSED("ConditionPropModelID::CreateAt", 0x0063BE80, &CreateAt<CondBase, Vt(0x00937CAC), Key(0x4)>);
CONDFAC_REVERSED("ConditionPropUniqueID::CreateAt", 0x0063BED0, &CreateAt<CondBase, Vt(0x00937CC0), Key(0x4)>);
CONDFAC_REVERSED("ConditionVehicleHitOrientation::CreateAt", 0x0063BF20, &CreateAt<CondBase, Vt(0x00937CD4)>);
CONDFAC_REVERSED("ConditionPlayerHasPunishmentPoints::CreateAt", 0x0063BF60, &CreateAt<CondBase, Vt(0x00937CE8)>);
CONDFAC_REVERSED("ConditionTargetPropClearOfPeds::CreateAt", 0x0063BFB0, &CreateAt<CondBase, Vt(0x00937CFC), Key(0x4)>);
CONDFAC_REVERSED("ConditionPOIClearOfPeds::CreateAt", 0x0063C000, &CreateAt<CondBase, Vt(0x00937D10)>);
CONDFAC_REVERSED("ConditionPropDisplayUseText::CreateAt", 0x0063C040, &CreateAt<CondBase, Vt(0x00937D24)>);
CONDFAC_REVERSED("ConditionAllyIsInCombat::CreateAt", 0x0063C080, &CreateAt<CondBase, Vt(0x00937D38)>);
CONDFAC_REVERSED("ConditionAllyState::CreateAt", 0x0063C0C0, &CreateAt<CondBase, Vt(0x00937D4C)>);
CONDFAC_REVERSED("ConditionAllyMorale::CreateAt", 0x0063C100, &CreateAt<CondBase, Vt(0x00937D60)>);
CONDFAC_REVERSED("ConditionMissionAvailable::CreateAt", 0x0063C150, &CreateAt<CondBase, Vt(0x00937D74), Key(0x4)>);
CONDFAC_REVERSED("ConditionPropPedIsUsingMe::CreateAt", 0x0063C1A0, &CreateAt<CondBase, Vt(0x00937D88)>);
CONDFAC_REVERSED("ConditionLastMission::CreateAt", 0x0063C1F0, &CreateVia<0x006A7330>);  // (not in Ghidra's list)
CONDFAC_REVERSED("ConditionNumWeapons::CreateAt", 0x0063C220, &CreateAt<CondBase, Vt(0x00937D9C)>);
CONDFAC_REVERSED("ConditionCameraFading::CreateAt", 0x0063C260, &CreateAt<CondBase, Vt(0x00937DB0)>);
CONDFAC_REVERSED("ConditionHasGrappleTarget::CreateAt", 0x0063C2A0, &CreateAt<CondBase, Vt(0x00937DC4)>);
CONDFAC_REVERSED("ConditionHasNodeCooldown::CreateAt", 0x0063C2E0, &CreateAt<CondBase, Vt(0x00937DD8)>);
CONDFAC_REVERSED("ConditionIsIgnoreTruants::CreateAt", 0x0063C320, &CreateAt<CondBase, Vt(0x00937DEC)>);
CONDFAC_REVERSED("ConditionThisNodeIsPlaying::CreateAt", 0x0063C360, &CreateAt<CondBase, Vt(0x00937E00)>);
CONDFAC_REVERSED("ConditionThisNodeIsNotPlaying::CreateAt", 0x0063C3A0, &CreateAt<CondBase, Vt(0x00937E14)>);
CONDFAC_REVERSED("ConditionPlayerHasTarget::CreateAt", 0x0063C3E0, &CreateAt<CondBase, Vt(0x00937E28)>);
CONDFAC_REVERSED("ConditionTargetMyTargetsProp::CreateAt", 0x0063C420, &CreateAt<CondBase, Vt(0x00937E3C)>);
CONDFAC_REVERSED("ConditionNumAuthorityInPursuit::CreateAt", 0x0063C460, &CreateAt<CondBase, Vt(0x00937E50)>);
CONDFAC_REVERSED("ConditionIsTagWaiting::CreateAt", 0x0063C480, &CreateAt<CondBase, Vt(0x00937E64)>);
CONDFAC_REVERSED("ConditionIsTagDrawing::CreateAt", 0x0063C4A0, &CreateAt<CondBase, Vt(0x00937E74)>);
CONDFAC_REVERSED("ConditionIsNISorCutsceneRunning::CreateAt", 0x0063C4E0, &CreateAt<CondBase, Vt(0x00937E84)>);
CONDFAC_REVERSED("ConditionIsPlayerFacingMirror::CreateAt", 0x0063C500, &CreateAt<CondBase, Vt(0x00937E98)>);
CONDFAC_REVERSED("ConditionPropTargetIsTwoHanded::CreateAt", 0x0063C540, &CreateAt<CondBase, Vt(0x00937EA8)>);
CONDFAC_REVERSED("ConditionObjectiveOfType::CreateAt", 0x0063C560, &CreateAt<CondBase, Vt(0x0092E694)>);
CONDFAC_REVERSED("ConditionSenseTargetNow::CreateAt", 0x0063C580, &CreateAt<CondBase, Vt(0x00937EBC)>);
CONDFAC_REVERSED("ConditionIsWeaponFiredWhenDropped::CreateAt", 0x0063C5C0, &CreateAt<CondBase, Vt(0x00937ECC)>);
CONDFAC_REVERSED("ConditionIsElderly::CreateAt", 0x0063C600, &CreateAt<CondBase, Vt(0x00937EE0)>);
CONDFAC_REVERSED("ConditionIsRegisteredAsGirlfriend::CreateAt", 0x0063C640, &CreateAt<CondBase, Vt(0x00937EF4)>);
CONDFAC_REVERSED("ConditionIsInFight::CreateAt", 0x0063C680, &CreateAt<CondBase, Vt(0x00937F08)>);
CONDFAC_REVERSED("ConditionScriptSaveData::CreateAt", 0x0063C6C0, &CreateAt<CondBase, Vt(0x00937F1C)>);
CONDFAC_REVERSED("ConditionPlayerHasPunishmentOfType::CreateAt", 0x0063C700, &CreateAt<CondBase, Vt(0x00937F30)>);
CONDFAC_REVERSED("ConditionIsMiniObjectiveComplete::CreateAt", 0x0063C740, &CreateAt<CondBase, Vt(0x00937F44)>);
CONDFAC_REVERSED("ConditionIsAvoiding::CreateAt", 0x0063C780, &CreateAt<CondBase, Vt(0x00937F58)>);
CONDFAC_REVERSED("ConditionFrontEndActive::CreateAt", 0x0063C7C0, &CreateAt<CondBase, Vt(0x00937F6C)>);
CONDFAC_REVERSED("ConditionPedHasNumAttackers::CreateAt", 0x0063C800, &CreateAt<CondBase, Vt(0x00937F80)>);
CONDFAC_REVERSED("ConditionIsTakingPictureFreeze::CreateAt", 0x0063C840, &CreateAt<CondBase, Vt(0x00937F94)>);
CONDFAC_REVERSED("ConditionIsUsingDoor::CreateAt", 0x0063C860, &CreateAt<CondBase, Vt(0x00937FA8)>);
CONDFAC_REVERSED("ConditionPropIsPlaying::CreateAt", 0x006456D0, &CreateAt<CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00938358)>);
CONDFAC_REVERSED("ConditionAIConditionGroup::CreateAt", 0x00645A50, &CreateAt<CondBase, Vt(0x0093836C)>);
CONDFAC_REVERSED("ConditionHaveIAttacked::CreateAt", 0x00646070, &CreateAt<CondBase, Vt(0x00937FB8)>);

// New (cdecl)
CONDFAC_REVERSED("ConditionWeaponDesired::New", 0x00637410, &NewVia<0x10, 0x0065AD90, 0x0065AE70>);
CONDFAC_REVERSED("ConditionLastWeapon::New", 0x00637600, &NewVia<0x10, 0x0065AE40, 0x0065AE70>);
CONDFAC_REVERSED("ConditionWeaponCount::New", 0x006376C0, &NewVia<0x14, 0x0065AAC0, 0x0065AAE0>);
CONDFAC_REVERSED("ConditionCollisionWeaponType::New", 0x006377A0, &NewVia<0x14, 0x0065AB20, 0x0065AB50>);
CONDFAC_REVERSED("ConditionAltWeaponCollisionType::New", 0x006377E0, &NewVia<0x4, 0x0065AC70, 0x0049A650>);
CONDFAC_REVERSED("ConditionEvaluatedEntityType::New", 0x00637CD0, &NewVia<0xC, 0x0065AA70, 0x005FC3D0>);
CONDFAC_REVERSED("ConditionEntityIsModel::New", 0x00637D90, &New<0xC, CondBase, Vt(0x0093446C), Key(0x4), I32(0x8, 0xFFFFFFFE), Then, ClearName(0x4)>);
CONDFAC_REVERSED("ConditionCutsceneSoundLoaded::New", 0x00638340, &New<0x8, CondBase, Vt(0x0092E718), I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionIsMusicStreamLoaded::New", 0x006383A0, &New<0x8, CondBase, Vt(0x0092E72C), I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionIsSoundBankLoaded::New", 0x00638400, &New<0xC, CondBase, Vt(0x0092E740), Key(0x4), I32(0x8, 0x0)>);
CONDFAC_REVERSED("ConditionIsSpeechBusy::New", 0x00638470, &New<0x8, CondBase, Vt(0x0092E754)>);
CONDFAC_REVERSED("ConditionIsSpeechPreloaded::New", 0x006384C0, &New<0x10, CondBase, Vt(0x0092E77C), Then, Call(0x005FCC70)>);
CONDFAC_REVERSED("ConditionIsSpeechStreamFree::New", 0x00638520, &New<0x4, CondBase, Vt(0x0092E768)>);
CONDFAC_REVERSED("ConditionHasSpeechEvent::New", 0x00638570, &New<0x8, CondBase, Vt(0x0092E7A4), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionNearPoint::New", 0x0063A260, &NewVia<0x10, 0x005FC450, 0x005FC480>);
CONDFAC_REVERSED("CMissionCond::New", 0x0063A2A0, &NewVia<0xC, 0x006A7EB0, 0x0049A650>);
CONDFAC_REVERSED("CErrandCond::New", 0x0063A2E0, &NewVia<0x8, 0x006A72F0, 0x005FD140>);
CONDFAC_REVERSED("CMissionUnlockedCond::New", 0x0063A320, &NewVia<0xC, 0x006A73D0, 0x0049A650>);
CONDFAC_REVERSED("CPeriodCond::New", 0x0063A3F0, &NewVia<0x14, 0x006A7500, 0x006A7530>);
CONDFAC_REVERSED("ConditionMissionSuccessCount::New", 0x0063A6F0, &NewVia<0x14, 0x005FCD40, 0x005FCD70>);
CONDFAC_REVERSED("ConditionIsTargetOutOfReach::New", 0x0063A7F0, &New<0xC, CondBase, Vt(0x0093445C), Then, F32(0x4, 3.0f), F32(0x8, 2.0f)>);
CONDFAC_REVERSED("ConditionLastMission::New", 0x0063C1C0, &NewVia<0x10, 0x006A7330, 0x0061C320>);
CONDFAC_REVERSED("ConditionCameraAllowScriptedChange::New", 0x00644CE0, &New<0x4, CondBase, Vt(0x00936554)>);
CONDFAC_REVERSED("ConditionWeaponDesiredSocket::New", 0x00644D10, &New<0xC, CondBase, Vt(0x00936568), Key(0x4), Key(0x8), Then, Call(0x0065ADC0)>);
CONDFAC_REVERSED("ConditionWeaponDesiredIsEquiped::New", 0x00644D60, &New<0x4, CondBase, Vt(0x0093657C), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionWeaponDesiredIsFocusWeapon::New", 0x00644DA0, &New<0x8, CondBase, Vt(0x00936590), Then, Call(0x007101E0)>);
CONDFAC_REVERSED("ConditionWeaponEquiped::New", 0x00644DE0, &New<0x10, CondBase, Vt(0x009365A4), Key(0x4), Then, Call(0x0065ACE0)>);
CONDFAC_REVERSED("ConditionWeaponEquipedType::New", 0x00644E20, &New<0xC, CondBase, Vt(0x009365B8), Then, Call(0x0065AD20)>);
CONDFAC_REVERSED("ConditionWeaponEquipedSocket::New", 0x00644E60, &New<0x10, CondBase, Vt(0x009365CC), Key(0x8), Then, Call(0x0065AD40)>);
CONDFAC_REVERSED("ConditionWeaponDamaged::New", 0x00644EA0, &New<0x4, CondBase, Vt(0x009365E0), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionWeaponAmmo::New", 0x00644EE0, &New<0x10, CondBase, Vt(0x009365F4), Then, Call(0x0065AAA0)>);
CONDFAC_REVERSED("ConditionPropAttached::New", 0x00644F20, &New<0xC, CondBase, Vt(0x00936608), Key(0x4), Then, Call(0x005FC3D0)>);
CONDFAC_REVERSED("ConditionWeaponModelRequest::New", 0x00644F60, &New<0xC, CondBase, Vt(0x0093661C), Key(0x4), Then, Call(0x0065D710)>);
CONDFAC_REVERSED("ConditionDamageFromExplosion::New", 0x00644FA0, &New<0x4, CondBase, Vt(0x00936630)>);
CONDFAC_REVERSED("ConditionDamageFromBurningFire::New", 0x00644FD0, &New<0x4, CondBase, Vt(0x00936644)>);
CONDFAC_REVERSED("ConditionFireInRange::New", 0x00645000, &New<0x8, CondBase, Vt(0x00936658), Then, Call(0x005FD3B0)>);
CONDFAC_REVERSED("ConditionWeaponIsThrowable::New", 0x00645040, &New<0x4, CondBase, Vt(0x0093666C)>);
CONDFAC_REVERSED("ConditionCameraBlending::New", 0x00645070, &New<0x4, CondBase, Vt(0x0093667C)>);
CONDFAC_REVERSED("ConditionTargetProp::New", 0x006450A0, &New<0x2C, CondBase, Vt(0x00936690), Key(0x10), Key(0x14), Key(0x1C), Then, Call(0x0065AF40)>);
CONDFAC_REVERSED("ConditionWeaponNeedInit::New", 0x006450F0, &New<0x4, CondBase, Vt(0x009366A4)>);
CONDFAC_REVERSED("ConditionWeaponIsInventory::New", 0x00645120, &New<0x4, CondBase, Vt(0x009366B4)>);
CONDFAC_REVERSED("ConditionWeaponHudActive::New", 0x00645150, &New<0x4, CondBase, Vt(0x009366C4)>);
CONDFAC_REVERSED("ConditionWeaponHudLocked::New", 0x00645180, &New<0x4, CondBase, Vt(0x009366D4)>);
CONDFAC_REVERSED("ConditionWeaponWheelOn::New", 0x006451B0, &New<0x4, CondBase, Vt(0x009366E4)>);
CONDFAC_REVERSED("ConditionHudComponentVisible::New", 0x006451E0, &New<0x8, CondBase, Vt(0x009366F4), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionPropLifeTime::New", 0x00645220, &New<0xC, CondBase, Vt(0x00936704), Then, Call(0x0065B0F0)>);
CONDFAC_REVERSED("ConditionPropSpecialState::New", 0x00645260, &New<0x8, CondBase, Vt(0x00936718), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionPropTargetHeight::New", 0x006452A0, &New<0x10, CondBase, Vt(0x0093672C), Then, Call(0x0065AEC0)>);
CONDFAC_REVERSED("ConditionTargetProjectile::New", 0x006452E0, &New<0x14, CondBase, Vt(0x00936740), Key(0x8), Then, Call(0x0065AEF0)>);
CONDFAC_REVERSED("ConditionCameraDesired::New", 0x00645320, &New<0x8, CondBase, Vt(0x00936754), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionCameraActive::New", 0x00645360, &New<0x8, CondBase, Vt(0x00936768), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionCameraLocked::New", 0x006453A0, &New<0x4, CondBase, Vt(0x00936778)>);
CONDFAC_REVERSED("ConditionCameraInWidescreen::New", 0x006453D0, &New<0x4, CondBase, Vt(0x0093678C)>);
CONDFAC_REVERSED("ConditionButtonState::New", 0x00645400, &New<0xC, CondBase, Vt(0x0093679C), Then, I32(0x4, 0x0), I32(0x8, 0x0)>);
CONDFAC_REVERSED("ConditionCameraDesiredReset::New", 0x00645440, &New<0x4, CondBase, Vt(0x009367AC)>);
CONDFAC_REVERSED("ConditionHasCash::New", 0x00645470, &New<0x8, CondBase, Vt(0x009367C0), Then, Call(0x0065B110)>);
CONDFAC_REVERSED("ConditionGetProjectileOwner::New", 0x006454B0, &New<0x4, CondBase, Vt(0x009367D4)>);
CONDFAC_REVERSED("ConditionGetPassingPed::New", 0x006454E0, &New<0x8, CondBase, Vt(0x009367E8), Then, U8(0x4, 0x1)>);
CONDFAC_REVERSED("ConditionEntitySpeed::New", 0x00645510, &New<0xC, CondBase, Vt(0x009367FC), Then, Zero(0x4), I32(0x8, 0x0)>);
CONDFAC_REVERSED("CatchProjectileCond::New", 0x00645550, &New<0x24, CondBase, Vt(0x00936810), Then, Call(0x0065B130)>);
CONDFAC_REVERSED("AttackPowerCond::New", 0x00645590, &New<0xC, CondBase, Vt(0x00936824), Then, Call(0x0065B180)>);
CONDFAC_REVERSED("ConditionTargetInteractiveProp::New", 0x006455D0, &New<0x18, CondBase, Vt(0x00936838), Key(0x8), Then, Call(0x00654F90)>);
CONDFAC_REVERSED("ConditionPedNearPropSocket::New", 0x00645610, &New<0x24, CondBase, Vt(0x0093684C), Key(0x10), Key(0x18), Key(0x1C), Key(0x20), Then, Call(0x00654FC0)>);
CONDFAC_REVERSED("ConditionPropHealth::New", 0x00645670, &New<0xC, CondBase, Vt(0x00936860), Then, Call(0x00655010)>);
CONDFAC_REVERSED("ConditionPropFlags::New", 0x00645700, &New<0x8, CondBase, Vt(0x00936874), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionPropTagPercentage::New", 0x00645740, &New<0xC, CondBase, Vt(0x00936888), Then, Call(0x00655150)>);
CONDFAC_REVERSED("ConditionPropModelLoaded::New", 0x00645780, &New<0xC, CondBase, Vt(0x0093689C), Key(0x4), Then, Call(0x0065D710)>);
CONDFAC_REVERSED("ConditionActionNodePlayTime::New", 0x006457C0, &New<0xC, CondBase, Vt(0x009368B0), Then, Call(0x005FC020)>);
CONDFAC_REVERSED("ConditionActionNodePlayTimePast::New", 0x00645800, &New<0x4, CondBase, Vt(0x009368C0)>);
CONDFAC_REVERSED("ConditionActionIntentionActionRequest::New", 0x00645830, &New<0x18, CondBase, Vt(0x009368D4), Then, Call(0x005FBF40)>);
CONDFAC_REVERSED("ConditionIntentionActionNodeRequest::New", 0x00645870, &New<0x4, CondBase, Vt(0x009368E8), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionActionRequestButton::New", 0x006458B0, &New<0x18, CondBase, Vt(0x009368FC), Then, Call(0x005FBF60)>);
CONDFAC_REVERSED("ConditionAcquireAuthorityFocus::New", 0x006458F0, &New<0x4, CondBase, Vt(0x00936910)>);
CONDFAC_REVERSED("ConditionAcquireFactionFocus::New", 0x00645920, &New<0x4, CondBase, Vt(0x00936920)>);
CONDFAC_REVERSED("ConditionAcquirePOI::New", 0x00645950, &New<0x8, CondBase, Vt(0x00936930), Then, Call(0x005FC4E0)>);
CONDFAC_REVERSED("ConditionAcquirePunishableTarget::New", 0x00645990, &New<0x4, CondBase, Vt(0x00936940)>);
CONDFAC_REVERSED("ConditionAcquireRandomTargetInGroup::New", 0x006459C0, &New<0x4, CondBase, Vt(0x00936950)>);
CONDFAC_REVERSED("ConditionAcquireFocus::New", 0x006459F0, &New<0x4, CondBase, Vt(0x00936960)>);
CONDFAC_REVERSED("ConditionAcquireTarget::New", 0x00645A20, &New<0x4, CondBase, Vt(0x00936970)>);
CONDFAC_REVERSED("ConditionAIIsPlaying::New", 0x00645A70, &New<0xC, CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00936980), Then, Call(0x005FC290)>);
CONDFAC_REVERSED("ConditionAnimationGroupLoaded::New", 0x00645AC0, &New<0x10, CondBase, Vt(0x0092E704), Key(0x4), Then, Call(0x005FCB30)>);
CONDFAC_REVERSED("ConditionAnimationParametric::New", 0x00645B00, &New<0xC, CondBase, Vt(0x00936994), Then, Call(0x005FC020)>);
CONDFAC_REVERSED("ConditionAttacked::New", 0x00645B40, &New<0x8, CondBase, Vt(0x009369A8), Then, Zero(0x4)>);
CONDFAC_REVERSED("ConditionAttackRole::New", 0x00645B70, &New<0x8, CondBase, Vt(0x009369BC), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionAttackPhase::New", 0x00645BB0, &New<0xC, CondBase, Vt(0x009369D0), Then, Call(0x005FC9A0)>);
CONDFAC_REVERSED("ConditionAttackTimerPlayTime::New", 0x00645BF0, &New<0x10, CondBase, Vt(0x009369E4), Then, Call(0x005FC600)>);
CONDFAC_REVERSED("ConditionAttacksDisabled::New", 0x00645C30, &New<0x4, CondBase, Vt(0x009369F8)>);
CONDFAC_REVERSED("ConditionBlockTimerPlayTime::New", 0x00645C60, &New<0x10, CondBase, Vt(0x00936A08), Then, Call(0x005FC600)>);
CONDFAC_REVERSED("ConditionEvadeTimerPlayTime::New", 0x00645CA0, &New<0x10, CondBase, Vt(0x00936A1C), Then, Call(0x005FC600)>);
CONDFAC_REVERSED("ConditionCollisionStart::New", 0x00645CE0, &New<0x4, CondBase, Vt(0x00936A30)>);
CONDFAC_REVERSED("ConditionCharacterSize::New", 0x00645D10, &New<0x8, CondBase, Vt(0x00936A44), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionTargetRelativeCharacterSize::New", 0x00645D50, &New<0xC, CondBase, Vt(0x00936A58), Then, Call(0x005FC940)>);
CONDFAC_REVERSED("ConditionHitReactionEnd::New", 0x00645D90, &New<0x4, CondBase, Vt(0x00936A6C)>);
CONDFAC_REVERSED("ConditionCollisionFlag::New", 0x00645DC0, &New<0x8, CondBase, Vt(0x00936A80), Key(0x4)>);
CONDFAC_REVERSED("ConditionGroupCondition::New", 0x00645DF0, &New<0x8, CondBase, Vt(0x0092E140), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionController::New", 0x00645E30, &New<0x8, CondBase, Vt(0x00936A94), Then, U8(0x4, 0x1), U8(0x5, 0x1)>);
CONDFAC_REVERSED("ConditionDistToTarget::New", 0x00645E70, &New<0x8, CondBase, Vt(0x00936AA8), Then, Call(0x005FC9F0)>);
CONDFAC_REVERSED("ConditionEngageDistance::New", 0x00645EB0, &New<0x8, CondBase, Vt(0x00936ABC), Then, Call(0x006D7FE0)>);
CONDFAC_REVERSED("ConditionFactionScan::New", 0x00645EF0, &New<0x10, CondBase, Vt(0x00936AD0), Then, Call(0x005FC810)>);
CONDFAC_REVERSED("ConditionFactionNear::New", 0x00645F30, &New<0x8, CondBase, Vt(0x00936AE4), Then, Call(0x00655150)>);
CONDFAC_REVERSED("ConditionFindTandemHelper::New", 0x00645F70, &New<0x4, CondBase, Vt(0x00936AF8)>);
CONDFAC_REVERSED("ConditionIsTandemHelper::New", 0x00645FA0, &New<0x4, CondBase, Vt(0x00936B0C)>);
CONDFAC_REVERSED("ConditionFalse::New", 0x00645FD0, &New<0x4, CondBase, Vt(0x00936B20)>);
CONDFAC_REVERSED("ConditionFightToken::New", 0x00646000, &New<0x4, CondBase, Vt(0x00936B30), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionHaveIAttacked::New", 0x00646040, &New<0x4, CondBase, Vt(0x00937FB8)>);
CONDFAC_REVERSED("ConditionHavePOI::New", 0x00646090, &New<0x4, CondBase, Vt(0x00936B44)>);
CONDFAC_REVERSED("ConditionHavePOIOfType::New", 0x006460C0, &New<0x8, CondBase, Vt(0x00936B54), Then, Call(0x005FC4F0)>);
CONDFAC_REVERSED("ConditionFightClass::New", 0x00646100, &New<0x8, CondBase, Vt(0x00936B64), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionFocusAttacked::New", 0x00646140, &New<0x8, CondBase, Vt(0x00936B78), Then, Zero(0x4)>);
CONDFAC_REVERSED("ConditionGrapplingHookAnchord::New", 0x00646170, &New<0x10, CondBase, Vt(0x00936B8C), Then, Call(0x005FC9E0)>);
CONDFAC_REVERSED("CClassSkippedCond::New", 0x006461B0, &New<0xC, CondBase, Vt(0x00936BA0), Then, Call(0x006A7460)>);
CONDFAC_REVERSED("ConditionFirstPerson::New", 0x006461F0, &New<0x4, CondBase, Vt(0x00936BB4)>);
CONDFAC_REVERSED("ConditionHavePath::New", 0x00646220, &New<0x4, CondBase, Vt(0x00936BC8)>);
CONDFAC_REVERSED("ConditionHasSweptSphereCollision::New", 0x00646250, &New<0x2C, CondBase, Vt(0x0092E7CC), Then, Call(0x005FCCE0)>);
CONDFAC_REVERSED("ConditionHealth::New", 0x00646290, &New<0xC, CondBase, Vt(0x00936BD8), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionPercentageHealth::New", 0x006462D0, &New<0xC, CondBase, Vt(0x00936BEC), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionPercentageSpecial::New", 0x00646310, &New<0xC, CondBase, Vt(0x00936C00), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionDamage::New", 0x00646350, &New<0xC, CondBase, Vt(0x00936C14), Then, Call(0x00655150)>);
CONDFAC_REVERSED("ConditionDamagePending::New", 0x00646390, &New<0xC, CondBase, Vt(0x00936C28), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionHasActionNode::New", 0x006463D0, &New<0x8, CondBase, Vt(0x0092E790), Key(0x4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionOpeningBranchChildHasChild::New", 0x00646410, &New<0xC, CondBase, Vt(0x0092E7B8), Key(0x4), Key(0x8), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionWallHitDirection::New", 0x00646460, &New<0x8, CondBase, Vt(0x00936C3C), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionPedIsInOutfit::New", 0x006464A0, &New<0x8, CondBase, Vt(0x00936C50), Key(0x4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionHitByPed::New", 0x006464E0, &New<0x8, CondBase, Vt(0x00936C64), Then, Call(0x005FC0C0)>);
CONDFAC_REVERSED("ConditionHitTime::New", 0x00646520, &New<0x10, CondBase, Vt(0x00936C78), Then, Call(0x005FC540)>);
CONDFAC_REVERSED("ConditionHitFlag::New", 0x00646560, &New<0xC, CondBase, Vt(0x00936C8C), Then, Call(0x005FC0F0)>);
CONDFAC_REVERSED("ConditionHitStunTime::New", 0x006465A0, &New<0xC, CondBase, Vt(0x00936CA0), Then, Call(0x005FC020)>);
CONDFAC_REVERSED("ConditionHitStunFinished::New", 0x006465E0, &New<0x4, CondBase, Vt(0x00936CB4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionHitFlagClear::New", 0x00646620, &New<0x4, CondBase, Vt(0x00936CC8)>);
CONDFAC_REVERSED("ConditionIgnoreAttacks::New", 0x00646650, &New<0x4, CondBase, Vt(0x00936CDC)>);
CONDFAC_REVERSED("ConditionInAttackCone::New", 0x00646680, &New<0x4, CondBase, Vt(0x00936CF0)>);
CONDFAC_REVERSED("ConditionInBikeRace::New", 0x006466B0, &New<0x4, CondBase, Vt(0x00936D04)>);
CONDFAC_REVERSED("ConditionVisibleArea::New", 0x006466E0, &New<0x8, CondBase, Vt(0x00936D14), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionLiquidLevel::New", 0x00646720, &New<0xC, CondBase, Vt(0x0092E5E0), Then, Call(0x005FC320)>);
CONDFAC_REVERSED("ConditionLiquidType::New", 0x00646760, &New<0x8, CondBase, Vt(0x0092E5F4), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionSurfaceHardness::New", 0x006467A0, &New<0x8, CondBase, Vt(0x0092E608), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionIsAuthority::New", 0x006467E0, &New<0x4, CondBase, Vt(0x00936D24)>);
CONDFAC_REVERSED("ConditionIsFemale::New", 0x00646810, &New<0x4, CondBase, Vt(0x00936D38)>);
CONDFAC_REVERSED("ConditionIsHangingOut::New", 0x00646840, &New<0x4, CondBase, Vt(0x00936D4C)>);
CONDFAC_REVERSED("ConditionIsHealthAtHumiliationLevel::New", 0x00646870, &New<0x4, CondBase, Vt(0x00936D5C), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionIsInFactionWithStimulusObject::New", 0x006468B0, &New<0x4, CondBase, Vt(0x00936D70)>);
CONDFAC_REVERSED("ConditionIsMissionSpecific::New", 0x006468E0, &New<0x4, CondBase, Vt(0x00936D80)>);
CONDFAC_REVERSED("ConditionIsScriptedAmbient::New", 0x00646910, &New<0x4, CondBase, Vt(0x00936D94)>);
CONDFAC_REVERSED("ConditionIsTargetAttackRightsTaken::New", 0x00646940, &New<0x4, CondBase, Vt(0x00936DA4)>);
CONDFAC_REVERSED("ConditionIsStationary::New", 0x00646970, &New<0x4, CondBase, Vt(0x00936DB4)>);
CONDFAC_REVERSED("ConditionIsStimulusObject::New", 0x006469A0, &New<0x4, CondBase, Vt(0x00936DC4)>);
CONDFAC_REVERSED("ConditionInCombatZone::New", 0x006469D0, &New<0xC, CondBase, Vt(0x00936DD4), Then, Call(0x005FC950)>);
CONDFAC_REVERSED("ConditionJoinCombatZone::New", 0x00646A10, &New<0xC, CondBase, Vt(0x00936DE8), Then, Call(0x005FCD70)>);
CONDFAC_REVERSED("ConditionWeaponLastHitBy::New", 0x00646A50, &New<0x8, CondBase, Vt(0x00936DFC), Key(0x4)>);
CONDFAC_REVERSED("ConditionMePlaying::New", 0x00646A80, &New<0xC, CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00936E0C), Then, Call(0x005FC290)>);
CONDFAC_REVERSED("ConditionMePlayingDefaultNeutralState::New", 0x00646AD0, &New<0x8, CondBase, Vt(0x0092E5CC), Then, Call(0x00700DE0)>);
CONDFAC_REVERSED("ConditionIsDayTime::New", 0x00646B10, &New<0x4, CondBase, Vt(0x00936E20)>);
CONDFAC_REVERSED("ConditionMotionCycle::New", 0x00646B40, &New<0xC, CondBase, Vt(0x00936E34), Then, Call(0x005FC020)>);
CONDFAC_REVERSED("ConditionMotionIntentionDirection::New", 0x00646B80, &New<0xC, CondBase, Vt(0x00936E48), Then, Call(0x005FBFA0)>);
CONDFAC_REVERSED("ConditionMotionIntentionSpeed::New", 0x00646BC0, &New<0xC, CondBase, Vt(0x00936E5C), Then, Call(0x005FC020)>);
CONDFAC_REVERSED("ConditionNearbyDockLocation::New", 0x00646C00, &New<0x4, CondBase, Vt(0x00936E70)>);
CONDFAC_REVERSED("ConditionNearbyFleeDockLocation::New", 0x00646C30, &New<0x8, CondBase, Vt(0x00936E80), Then, F32(0x4, 10.0f)>);
CONDFAC_REVERSED("ConditionNOT::New", 0x00646C70, &New<0x4, CondBase, Vt(0x00936E90)>);
CONDFAC_REVERSED("ConditionOR::New", 0x00646CA0, &New<0x4, CondBase, Vt(0x00936EA0)>);
CONDFAC_REVERSED("ConditionObjective::New", 0x00646CD0, &New<0x8, CondBase, Vt(0x0092E680), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionInCar::New", 0x00646D10, &New<0x4, CondBase, Vt(0x00936EB0)>);
CONDFAC_REVERSED("ConditionOnBike::New", 0x00646D40, &New<0x4, CondBase, Vt(0x00936EC0)>);
CONDFAC_REVERSED("ConditionOnVehicleOfClass::New", 0x00646D70, &New<0x8, CondBase, Vt(0x00936ED0), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionPedFightingSystem::New", 0x00646DB0, &New<0x8, CondBase, Vt(0x00936EE4), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionPedFlags::New", 0x00646DF0, &New<0x8, CondBase, Vt(0x00936EF8), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionSetPedFlags::New", 0x00646E30, &New<0xC, CondBase, Vt(0x00936F0C), Then, Call(0x005FBF90)>);
CONDFAC_REVERSED("ConditionPedModelID::New", 0x00646E70, &New<0x8, CondBase, Vt(0x0092E6DC), Key(0x4), Then, Call(0x005FC3D0)>);
CONDFAC_REVERSED("ConditionPedType::New", 0x00646EB0, &New<0x8, CondBase, Vt(0x0092E66C), Then, Call(0x005FC4B0)>);
CONDFAC_REVERSED("ConditionPedIsDriver::New", 0x00646EF0, &New<0x4, CondBase, Vt(0x00936F20)>);
CONDFAC_REVERSED("ConditionPrint::New", 0x00646F20, &New<0x8, CondBase, Vt(0x0092E12C), Key(0x4), Then, Call(0x005FC3D0)>);
CONDFAC_REVERSED("ConditionProjectile::New", 0x00646F60, &New<0x8, CondBase, Vt(0x00936F34), Then, Call(0x005FD450)>);
CONDFAC_REVERSED("ConditionRandom::New", 0x00646FA0, &New<0x14, CondBase, Vt(0x00936F48), Then, Call(0x005FC6C0)>);
CONDFAC_REVERSED("ConditionWeightedRandom::New", 0x00646FE0, &New<0x10, CondBase, Vt(0x0092E6A8), Then, Call(0x005FC6E0)>);
CONDFAC_REVERSED("ConditionRangeToTarget::New", 0x00647020, &New<0xC, CondBase, Vt(0x00936F5C), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionTimeSincePedalling::New", 0x00647060, &New<0xC, CondBase, Vt(0x00936F70), Then, Call(0x005FCD70)>);
CONDFAC_REVERSED("ConditionBikeSprintAmount::New", 0x006470A0, &New<0xC, CondBase, Vt(0x00936F84), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionRangeToFocus::New", 0x006470E0, &New<0xC, CondBase, Vt(0x00936F98), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionRangeToObjectiveVehicle::New", 0x00647120, &New<0xC, CondBase, Vt(0x00936FAC), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionSensePlayer::New", 0x00647160, &New<0x4, CondBase, Vt(0x00936FC0)>);
CONDFAC_REVERSED("ConditionSensePlayerDuration::New", 0x00647190, &New<0x8, CondBase, Vt(0x00936FD0), Then, Zero(0x4)>);
CONDFAC_REVERSED("ConditionHearPlayerFootsteps::New", 0x006471C0, &New<0x8, CondBase, Vt(0x00936FE0)>);
CONDFAC_REVERSED("ConditionSenseAttacker::New", 0x006471F0, &New<0x4, CondBase, Vt(0x00936FF0)>);
CONDFAC_REVERSED("ConditionSizeOfGroup::New", 0x00647220, &New<0xC, CondBase, Vt(0x00937000), Then, Call(0x005FD290)>);
CONDFAC_REVERSED("ConditionSpecial::New", 0x00647260, &New<0xC, CondBase, Vt(0x00937010), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionSprint::New", 0x006472A0, &New<0xC, CondBase, Vt(0x00937024), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionSpecialPoints::New", 0x006472E0, &New<0xC, CondBase, Vt(0x00937038), Then, Call(0x005FCD70)>);
CONDFAC_REVERSED("ConditionStat::New", 0x00647320, &New<0x18, CondBase, Vt(0x0093704C), Then, Call(0x005FC520)>);
CONDFAC_REVERSED("ConditionFocusConditionGroup::New", 0x00647360, &New<0x8, CondBase, Vt(0x00937060), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionPedConditionGroup::New", 0x006473A0, &New<0x8, CondBase, Vt(0x00937080), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionAttackerConditionGroup::New", 0x006473E0, &New<0x8, CondBase, Vt(0x009370A0), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionStimulus::New", 0x00647420, &New<0x10, CondBase, Vt(0x009370C0), Then, Call(0x005FC670)>);
CONDFAC_REVERSED("ConditionStimulusRepeated::New", 0x00647460, &New<0x8, CondBase, Vt(0x009370D0), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionTargetedByTarget::New", 0x006474A0, &New<0x4, CondBase, Vt(0x009370E0), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionTargetedByPlayer::New", 0x006474E0, &New<0x4, CondBase, Vt(0x009370F4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionTargetPlayersGF::New", 0x00647520, &New<0x4, CondBase, Vt(0x00937108), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionTargetConditionGroup::New", 0x00647560, &New<0xC, CondBase, Vt(0x0093711C), Then, Call(0x0065D710)>);
CONDFAC_REVERSED("ConditionTargetFaction::New", 0x006475A0, &New<0x8, CondBase, Vt(0x0093713C), Then, Call(0x005FD150)>);
CONDFAC_REVERSED("ConditionTargetFriendsAttacker::New", 0x006475E0, &New<0x4, CondBase, Vt(0x00937150)>);
CONDFAC_REVERSED("ConditionTargetMastersAttacker::New", 0x00647610, &New<0x4, CondBase, Vt(0x00937160)>);
CONDFAC_REVERSED("ConditionTargetHealth::New", 0x00647640, &New<0xC, CondBase, Vt(0x0092E5A4), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionTargetIsVisible::New", 0x00647680, &New<0x4, CondBase, Vt(0x00937170)>);
CONDFAC_REVERSED("ConditionTargetPlaying::New", 0x006476B0, &New<0xC, CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00937180), Then, Call(0x005FC290)>);
CONDFAC_REVERSED("ConditionTargetPlayTime::New", 0x00647700, &New<0xC, CondBase, Vt(0x00937194), Then, Call(0x005FC020)>);
CONDFAC_REVERSED("ConditionTargetRelativeOrientation::New", 0x00647740, &New<0x10, CondBase, Vt(0x009371A8), Then, Call(0x005FC220)>);
CONDFAC_REVERSED("ConditionTargetRelativePosition::New", 0x00647780, &New<0x18, CondBase, Vt(0x009371BC), Then, Call(0x005FC1E0)>);
CONDFAC_REVERSED("ConditionTargetRelativePedCollisionSpheres::New", 0x006477C0, &New<0x10, CondBase, Vt(0x009371D0), Then, Call(0x005FC200)>);
CONDFAC_REVERSED("ConditionTargetRelativePositionInsideBox::New", 0x00647800, &New<0x20, CondBase, Vt(0x009371E4), Then, Call(0x005FC240)>);
CONDFAC_REVERSED("ConditionTargetsPunishmentPoints::New", 0x00647840, &New<0xC, CondBase, Vt(0x009371F8), Then, Call(0x005FC020)>);
CONDFAC_REVERSED("ConditionTargetStimulusSubject::New", 0x00647880, &New<0x4, CondBase, Vt(0x00937208)>);
CONDFAC_REVERSED("ConditionTargetStimulusObject::New", 0x006478B0, &New<0x4, CondBase, Vt(0x00937218)>);
CONDFAC_REVERSED("ConditionTargetStimulusEntitySubject::New", 0x006478E0, &New<0x4, CondBase, Vt(0x00937228)>);
CONDFAC_REVERSED("ConditionTargetStimulusEntityObject::New", 0x00647910, &New<0x4, CondBase, Vt(0x00937238)>);
CONDFAC_REVERSED("ConditionTargetVehicle::New", 0x00647940, &New<0x10, CondBase, Vt(0x00937248), Then, Call(0x005FC1A0)>);
CONDFAC_REVERSED("ConditionTargetVehicleCar::New", 0x00647980, &New<0xC, CondBase, Vt(0x0093725C), Then, Call(0x005FC180)>);
CONDFAC_REVERSED("ConditionTargetVehicleBike::New", 0x006479C0, &New<0x10, CondBase, Vt(0x00937270), Then, Call(0x005FC1A0)>);
CONDFAC_REVERSED("ConditionAttemptAnchorToVehicle::New", 0x00647A00, &New<0x4, CondBase, Vt(0x00937284)>);
CONDFAC_REVERSED("ConditionRelativePosition::New", 0x00647A30, &New<0x10, CondBase, Vt(0x00937298), Then, Call(0x005FC1D0)>);
CONDFAC_REVERSED("ConditionVehicleWheelsOnGround::New", 0x00647A70, &New<0x8, CondBase, Vt(0x009372AC), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionTargetVehicleIsPlaying::New", 0x00647AB0, &New<0x8, CondBase, Vt(0x009372C0), Key(0x4), Then, Call(0x005FC1C0)>);
CONDFAC_REVERSED("ConditionInTriggerVolume::New", 0x00647AF0, &New<0x8, CondBase, Vt(0x0092E61C), Key(0x4), Then, Call(0x005FC3D0)>);
CONDFAC_REVERSED("ConditionPlayerInTriggerVolume::New", 0x00647B30, &New<0x8, CondBase, Vt(0x0092E630), Key(0x4), Then, Call(0x005FC3D0)>);
CONDFAC_REVERSED("CTimeCond::New", 0x00647B70, &New<0x14, CondBase, Vt(0x009372D4), Then, Call(0x006A74B0)>);
CONDFAC_REVERSED("CDayCond::New", 0x00647BB0, &New<0xC, CondBase, Vt(0x009372EC), Then, Call(0x006A75C0)>);
CONDFAC_REVERSED("CLuaCond::New", 0x00647BF0, &New<0x14, CondBase, Vt(0x00937300), Key(0xC), Key(0x10), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("CConversationActiveCond::New", 0x00647C40, &New<0x8, CondBase, Vt(0x00937318), Then, Call(0x00700DE0)>);
CONDFAC_REVERSED("ConditionIsPlayer::New", 0x00647C80, &New<0x4, CondBase, Vt(0x00937330)>);
CONDFAC_REVERSED("ConditionPedIsTrigger::New", 0x00647CB0, &New<0x4, CondBase, Vt(0x00937344)>);
CONDFAC_REVERSED("ConditionDriveForce::New", 0x00647CE0, &New<0xC, CondBase, Vt(0x00937358), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionLastSurfaceOn::New", 0x00647D20, &New<0x8, CondBase, Vt(0x0093736C), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionSurfaceGrade::New", 0x00647D60, &New<0xC, CondBase, Vt(0x00937380), Then, Call(0x005FCA00)>);
CONDFAC_REVERSED("ConditionSeason::New", 0x00647DA0, &New<0xC, CondBase, Vt(0x00937394), Then, Call(0x005FCA20)>);
CONDFAC_REVERSED("ConditionOnMission::New", 0x00647DE0, &New<0x8, CondBase, Vt(0x0092E6F0), Key(0x4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionDistToGround::New", 0x00647E20, &New<0xC, CondBase, Vt(0x009373A8), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionIsTargetOnBike::New", 0x00647E60, &New<0x4, CondBase, Vt(0x009373BC)>);
CONDFAC_REVERSED("ConditionIsFocusOnBike::New", 0x00647E90, &New<0x4, CondBase, Vt(0x009373CC)>);
CONDFAC_REVERSED("ConditionIsObjectiveVehicleOwner::New", 0x00647EC0, &New<0x4, CondBase, Vt(0x009373DC)>);
CONDFAC_REVERSED("ConditionLineOfFireClear::New", 0x00647EF0, &New<0x8, CondBase, Vt(0x009373EC), Then, Call(0x00700DE0)>);
CONDFAC_REVERSED("ConditionIsGuarding::New", 0x00647F30, &New<0x4, CondBase, Vt(0x009373FC)>);
CONDFAC_REVERSED("ConditionIsTargetCrouching::New", 0x00647F60, &New<0x4, CondBase, Vt(0x0093740C)>);
CONDFAC_REVERSED("ConditionIsStealthMissionChar::New", 0x00647F90, &New<0x4, CondBase, Vt(0x0093741C)>);
CONDFAC_REVERSED("ConditionSpotted::New", 0x00647FC0, &New<0x8, CondBase, Vt(0x0093742C)>);
CONDFAC_REVERSED("ConditionWantToBeStealthy::New", 0x00647FF0, &New<0x8, CondBase, Vt(0x0093743C), Then, Call(0x00655150)>);
CONDFAC_REVERSED("ConditionIsTethered::New", 0x00648030, &New<0x4, CondBase, Vt(0x0093744C)>);
CONDFAC_REVERSED("ConditionIsInsideTether::New", 0x00648060, &New<0x4, CondBase, Vt(0x0093745C)>);
CONDFAC_REVERSED("ConditionIsTargetInRange::New", 0x00648090, &New<0xC, CondBase, Vt(0x0093746C), Then, Call(0x005FCDE0)>);
CONDFAC_REVERSED("ConditionHaveDynamicPOI::New", 0x006480D0, &New<0x4, CondBase, Vt(0x0093747C), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionClothing::New", 0x00648110, &New<0x10, CondBase, Vt(0x0093748C), Key(0x8), Key(0xC), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionTargetClothing::New", 0x00648160, &New<0xC, CondBase, Vt(0x009374A0), Then, Call(0x005FC860)>);
CONDFAC_REVERSED("ConditionClothingStyle::New", 0x006481A0, &New<0x10, CondBase, Vt(0x009374B4), Then, Call(0x005FC870)>);
CONDFAC_REVERSED("ConditionClothingWarmthFactor::New", 0x006481E0, &New<0xC, CondBase, Vt(0x009374C8), Then, Call(0x005FC8E0)>);
CONDFAC_REVERSED("ConditionClothingSpecialKiss::New", 0x00648220, &New<0x4, CondBase, Vt(0x009374DC)>);
CONDFAC_REVERSED("ConditionTargetRelativeHeight::New", 0x00648250, &New<0x10, CondBase, Vt(0x0092E7F4), Then, Call(0x005FCE30)>);
CONDFAC_REVERSED("ConditionIsTargetProp::New", 0x00648290, &New<0x4, CondBase, Vt(0x009374F0)>);
CONDFAC_REVERSED("ConditionInterval::New", 0x006482C0, &New<0x8, CondBase, Vt(0x0092E82C), Then, Call(0x005FCEB0)>);
CONDFAC_REVERSED("ConditionCheckActionTimer::New", 0x00648300, &New<0x4, CondBase, Vt(0x00937500)>);
CONDFAC_REVERSED("ConditionCanLaunchMission::New", 0x00648330, &New<0x4, CondBase, Vt(0x00937510)>);
CONDFAC_REVERSED("ConditionCanSecondaryMissionPlay::New", 0x00648360, &New<0xC, CondBase, Vt(0x00937520), Then, Call(0x0065AD20)>);
CONDFAC_REVERSED("ConditionInClass::New", 0x006483A0, &New<0x4, CondBase, Vt(0x00937530)>);
CONDFAC_REVERSED("ConditionIsPlayerNearby::New", 0x006483D0, &New<0x8, CondBase, Vt(0x0092E83C), Then, Call(0x005FCF10)>);
CONDFAC_REVERSED("ConditionIsHungUp::New", 0x00648410, &New<0x4, CondBase, Vt(0x00937540)>);
CONDFAC_REVERSED("ConditionNeedsToDive::New", 0x00648440, &New<0x8, CondBase, Vt(0x00937550)>);
CONDFAC_REVERSED("ConditionHasNewAudibleStimulus::New", 0x00648470, &New<0x8, CondBase, Vt(0x00937564), Then, Call(0x005FC680)>);
CONDFAC_REVERSED("ConditionTargetProxyProp::New", 0x006484B0, &New<0x10, CondBase, Vt(0x0092E804), Then, Call(0x005FCE50)>);
CONDFAC_REVERSED("ConditionCanPropSetUsageState::New", 0x006484F0, &New<0x8, CondBase, Vt(0x0092E818), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionStealthBehavior::New", 0x00648530, &New<0x8, CondBase, Vt(0x00937574), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionTargetVehicleIs::New", 0x00648570, &New<0x8, CondBase, Vt(0x00937588), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionTargetVehicleIsLastVehicleUsed::New", 0x006485B0, &New<0x4, CondBase, Vt(0x0093759C)>);
CONDFAC_REVERSED("ConditionTargetVehicleOwnerDifferentFaction::New", 0x006485E0, &New<0x4, CondBase, Vt(0x009375B0)>);
CONDFAC_REVERSED("ConditionTargetVehicleIsAlreadyOccupied::New", 0x00648610, &New<0x4, CondBase, Vt(0x009375C4)>);
CONDFAC_REVERSED("ConditionTargetMyTargetsVehicle::New", 0x00648640, &New<0x4, CondBase, Vt(0x009375D8)>);
CONDFAC_REVERSED("ConditionIsOnSnow::New", 0x00648670, &New<0x4, CondBase, Vt(0x009375EC)>);
CONDFAC_REVERSED("ConditionFindSnow::New", 0x006486A0, &New<0x8, CondBase, Vt(0x009375FC), Then, F32(0x4, 10.0f)>);
CONDFAC_REVERSED("ConditionRunningAttack::New", 0x006486E0, &New<0xC, CondBase, Vt(0x0093760C), Then, One(0x4), F32(0x8, 2.5f)>);
CONDFAC_REVERSED("ConditionIsAllyLeader::New", 0x00648730, &New<0x4, CondBase, Vt(0x0093761C)>);
CONDFAC_REVERSED("ConditionIsAllyFollower::New", 0x00648760, &New<0x4, CondBase, Vt(0x0093762C)>);
CONDFAC_REVERSED("ConditionIsTargetAlly::New", 0x00648790, &New<0x4, CondBase, Vt(0x0093763C)>);
CONDFAC_REVERSED("ConditionCanRecruitTarget::New", 0x006487C0, &New<0x4, CondBase, Vt(0x0093764C)>);
CONDFAC_REVERSED("ConditionShouldSocialize::New", 0x006487F0, &New<0x4, CondBase, Vt(0x0093765C)>);
CONDFAC_REVERSED("ConditionIsSocialTarget::New", 0x00648820, &New<0x4, CondBase, Vt(0x0093766C)>);
CONDFAC_REVERSED("ConditionSocialAnimIs::New", 0x00648850, &New<0x8, CondBase, Vt(0x0093767C), Then, Call(0x005FCF40)>);
CONDFAC_REVERSED("ConditionSocialEmotionIs::New", 0x00648890, &New<0x8, CondBase, Vt(0x0093768C), Then, Call(0x005FCF60)>);
CONDFAC_REVERSED("ConditionHumiliationGroupIs::New", 0x006488D0, &New<0x8, CondBase, Vt(0x0093769C), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionIsSocializing::New", 0x00648910, &New<0x8, CondBase, Vt(0x009376AC), Then, Call(0x00700DE0)>);
CONDFAC_REVERSED("ConditionIsSocializingWithPlayer::New", 0x00648950, &New<0x4, CondBase, Vt(0x009376BC)>);
CONDFAC_REVERSED("ConditionIsWantingToSocialize::New", 0x00648980, &New<0x4, CondBase, Vt(0x009376CC)>);
CONDFAC_REVERSED("ConditionIsEmotionTowardsTarget::New", 0x006489B0, &New<0x8, CondBase, Vt(0x009376DC), Then, Call(0x005FCF60)>);
CONDFAC_REVERSED("ConditionIsEmotionTowardsPlayer::New", 0x006489F0, &New<0x8, CondBase, Vt(0x009376EC), Then, Call(0x005FCF60)>);
CONDFAC_REVERSED("ConditionIsSocialStateWait::New", 0x00648A30, &New<0x4, CondBase, Vt(0x009376FC)>);
CONDFAC_REVERSED("ConditionIsSocialStateAct::New", 0x00648A60, &New<0x4, CondBase, Vt(0x0093770C)>);
CONDFAC_REVERSED("ConditionIsSocialInitialReaction::New", 0x00648A90, &New<0x4, CondBase, Vt(0x0093771C)>);
CONDFAC_REVERSED("ConditionIsSocialInitialAction::New", 0x00648AC0, &New<0x4, CondBase, Vt(0x0093772C)>);
CONDFAC_REVERSED("ConditionIsReadyToBeKissed::New", 0x00648AF0, &New<0x4, CondBase, Vt(0x0093773C)>);
CONDFAC_REVERSED("ConditionSocialTypeFAT::New", 0x00648B20, &New<0x4, CondBase, Vt(0x0093774C)>);
CONDFAC_REVERSED("ConditionSocialTypeGIRL::New", 0x00648B50, &New<0x4, CondBase, Vt(0x0093775C)>);
CONDFAC_REVERSED("ConditionSocialTypeBULLY::New", 0x00648B80, &New<0x4, CondBase, Vt(0x0093776C)>);
CONDFAC_REVERSED("ConditionSocialTypeNERD::New", 0x00648BB0, &New<0x4, CondBase, Vt(0x0093777C)>);
CONDFAC_REVERSED("ConditionSocialTypeAUTH::New", 0x00648BE0, &New<0x4, CondBase, Vt(0x0093778C)>);
CONDFAC_REVERSED("ConditionSocialTypeGEN::New", 0x00648C10, &New<0x4, CondBase, Vt(0x0093779C)>);
CONDFAC_REVERSED("ConditionSocialTypeOLD::New", 0x00648C40, &New<0x4, CondBase, Vt(0x009377AC)>);
CONDFAC_REVERSED("ConditionSocialTypeNERDGIRL::New", 0x00648C70, &New<0x4, CondBase, Vt(0x009377BC)>);
CONDFAC_REVERSED("ConditionIsKissCount::New", 0x00648CA0, &New<0xC, CondBase, Vt(0x009377CC), Then, Call(0x005FCF70)>);
CONDFAC_REVERSED("ConditionTimeOfLastSocialAction::New", 0x00648CE0, &New<0xC, CondBase, Vt(0x009377DC), Then, Call(0x005FCF70)>);
CONDFAC_REVERSED("ConditionIsTargetBusySocializing::New", 0x00648D20, &New<0x4, CondBase, Vt(0x009377EC)>);
CONDFAC_REVERSED("ConditionHaveIAssaultedTargetPed::New", 0x00648D50, &New<0x4, CondBase, Vt(0x009377FC)>);
CONDFAC_REVERSED("ConditionHaveIHarassedTargetPed::New", 0x00648D80, &New<0x4, CondBase, Vt(0x0093780C)>);
CONDFAC_REVERSED("ConditionHaveIDismissedTargetPed::New", 0x00648DB0, &New<0x4, CondBase, Vt(0x0093781C)>);
CONDFAC_REVERSED("ConditionIsStimulusPriority::New", 0x00648DE0, &New<0xC, CondBase, Vt(0x0093782C), Then, Call(0x005FCF70)>);
CONDFAC_REVERSED("ConditionIsStrafing::New", 0x00648E20, &New<0x4, CondBase, Vt(0x0093783C)>);
CONDFAC_REVERSED("ConditionTargetAttacker::New", 0x00648E50, &New<0x4, CondBase, Vt(0x0093784C)>);
CONDFAC_REVERSED("ConditionTargetByType::New", 0x00648E80, &New<0xC, CondBase, Vt(0x0093785C), Then, I32(0x4, 0x0), U8(0x8, 0x1)>);
CONDFAC_REVERSED("ConditionBikeIsLocked::New", 0x00648EC0, &New<0x4, CondBase, Vt(0x0093786C)>);
CONDFAC_REVERSED("ConditionBikeIsUpright::New", 0x00648EF0, &New<0x4, CondBase, Vt(0x0093787C)>);
CONDFAC_REVERSED("ConditionWillAcceptGift::New", 0x00648F20, &New<0x4, CondBase, Vt(0x0093788C)>);
CONDFAC_REVERSED("ConditionPlayerHasRequiredGift::New", 0x00648F50, &New<0xC, CondBase, Vt(0x0093789C), Then, I32(0x4, 0x0), U8(0x8, 0x0)>);
CONDFAC_REVERSED("ConditionStatInt::New", 0x00648F90, &New<0x10, CondBase, Vt(0x009378AC), Then, Call(0x005FD060)>);
CONDFAC_REVERSED("ConditionStatFloat::New", 0x00648FD0, &New<0x10, CondBase, Vt(0x009378BC), Then, Call(0x005FD0B0)>);
CONDFAC_REVERSED("ConditionCheckTargetSemaphore::New", 0x00649010, &New<0x4, CondBase, Vt(0x009378CC)>);
CONDFAC_REVERSED("ConditionDetectGrenade::New", 0x00649040, &New<0x4, CondBase, Vt(0x009378DC)>);
CONDFAC_REVERSED("ConditionDetectProjectile::New", 0x00649070, &New<0x4, CondBase, Vt(0x009378EC)>);
CONDFAC_REVERSED("ConditionInTriggerVolumeOfType::New", 0x006490A0, &New<0x8, CondBase, Vt(0x0092E644), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionIsVehicleInRange::New", 0x006490E0, &New<0xC, CondBase, Vt(0x009378FC), Then, F32(0x4, 20.0f), U8(0x8, 0x1), U8(0x9, 0x1), U8(0xA, 0x1)>);
CONDFAC_REVERSED("ConditionInGuardRadius::New", 0x00649120, &New<0x8, CondBase, Vt(0x0093790C), Then, F32(0x4, 7.0f)>);
CONDFAC_REVERSED("ConditionAllyLeaderConditionGroup::New", 0x00649160, &New<0x8, CondBase, Vt(0x0093791C), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionAllyFollowerConditionGroup::New", 0x006491A0, &New<0x8, CondBase, Vt(0x0093793C), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionMissionActive::New", 0x006491E0, &New<0x4, CondBase, Vt(0x0093795C)>);
CONDFAC_REVERSED("ConditionCanAcquireAlly::New", 0x00649210, &New<0x8, CondBase, Vt(0x0093796C), Then, Call(0x00700DE0)>);
CONDFAC_REVERSED("ConditionSteering::New", 0x00649250, &New<0xC, CondBase, Vt(0x0093797C), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionSteeringFraction::New", 0x00649290, &New<0xC, CondBase, Vt(0x00937990), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionCheckWeatherType::New", 0x006492D0, &New<0x8, CondBase, Vt(0x009379A4), Then, I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionIsSpectator::New", 0x00649300, &New<0x4, CondBase, Vt(0x009379B4)>);
CONDFAC_REVERSED("ConditionIsParametricStatic::New", 0x00649330, &New<0x4, CondBase, Vt(0x009379C4)>);
CONDFAC_REVERSED("ConditionTargetAlly::New", 0x00649360, &New<0x4, CondBase, Vt(0x009379D4)>);
CONDFAC_REVERSED("ConditionFindSurface::New", 0x00649390, &New<0x10, CondBase, Vt(0x009379E4), Then, Call(0x005FCF20)>);
CONDFAC_REVERSED("ConditionInWeaponRange::New", 0x006493D0, &New<0x4, CondBase, Vt(0x009379F4)>);
CONDFAC_REVERSED("ConditionTargetPed::New", 0x00649400, &New<0x10, CondBase, Vt(0x00937A04), Key(0x4), Then, ClearName(0x4), F32(0x8, 2.0f), U8(0xC, 0x0)>);
CONDFAC_REVERSED("ConditionBalanceMeterPosition::New", 0x00649460, &New<0xC, CondBase, Vt(0x00937A14), Then, Call(0x005FD130)>);
CONDFAC_REVERSED("ConditionIsPlayerFatigued::New", 0x006494A0, &New<0x4, CondBase, Vt(0x00937A28)>);
CONDFAC_REVERSED("ConditionIsPlayerTired::New", 0x006494D0, &New<0x4, CondBase, Vt(0x00937A3C)>);
CONDFAC_REVERSED("ConditionTargetColliding::New", 0x00649500, &New<0x4, CondBase, Vt(0x00937A50)>);
CONDFAC_REVERSED("ConditionPedInConversation::New", 0x00649530, &New<0x4, CondBase, Vt(0x00937A60), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionPedConversationStatus::New", 0x00649570, &New<0x8, CondBase, Vt(0x00937A74), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionPedConversationMood::New", 0x006495B0, &New<0x8, CondBase, Vt(0x00937A88), Then, Call(0x005FD150)>);
CONDFAC_REVERSED("CanPathToEntityCond::New", 0x006495F0, &New<0x8, CondBase, Vt(0x0092E588), Then, Call(0x006D7FE0)>);
CONDFAC_REVERSED("ConditionIsPatrolPed::New", 0x00649630, &New<0x4, CondBase, Vt(0x00937A9C)>);
CONDFAC_REVERSED("ConditionPlayerIsInStealthProp::New", 0x00649660, &New<0x4, CondBase, Vt(0x00937AB0), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionHasLineOfSightToPlayer::New", 0x006496A0, &New<0x4, CondBase, Vt(0x00937AC4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionParametricTime::New", 0x006496E0, &New<0xC, CondBase, Vt(0x00937AD8), Then, Call(0x005FD170)>);
CONDFAC_REVERSED("PedGroundDistCond::New", 0x00649720, &New<0xC, CondBase, Vt(0x0092E84C), Then, Call(0x005FD180)>);
CONDFAC_REVERSED("PedVelocityCond::New", 0x00649760, &New<0x10, CondBase, Vt(0x0092E860), Then, Call(0x005FD1C0)>);
CONDFAC_REVERSED("ConditionHasRangedWeapon::New", 0x006497A0, &New<0x4, CondBase, Vt(0x0092E874), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionHasTXDLoaded::New", 0x006497E0, &New<0xC, CondBase, Vt(0x0092E888), Key(0x4), Then, Call(0x0065D710)>);
CONDFAC_REVERSED("ConditionPedSphereHit::New", 0x00649820, &New<0x8, CondBase, Vt(0x00937AEC), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionIsRumbleMode_Ambient::New", 0x00649860, &New<0x4, CondBase, Vt(0x00937B00)>);
CONDFAC_REVERSED("ConditionIsRumbleMode_Mission::New", 0x00649890, &New<0x4, CondBase, Vt(0x00937B14)>);
CONDFAC_REVERSED("ConditionIsScenarioPed::New", 0x006498C0, &New<0x4, CondBase, Vt(0x00937B28)>);
CONDFAC_REVERSED("ConditionHasCompletedScenarioObj::New", 0x006498F0, &New<0x4, CondBase, Vt(0x00937B38)>);
CONDFAC_REVERSED("ConditionSocialScriptCallback::New", 0x00649920, &New<0x4, CondBase, Vt(0x00937B48)>);
CONDFAC_REVERSED("ConditionUseSocialOverride::New", 0x00649950, &New<0x8, CondBase, Vt(0x00937B58), Then, Call(0x005FCF40)>);
CONDFAC_REVERSED("ConditionPlayerUseSocialOverride::New", 0x00649990, &New<0x8, CondBase, Vt(0x00937B68), Then, Call(0x005FCF50)>);
CONDFAC_REVERSED("ConditionSocialActionInterrupted::New", 0x006499D0, &New<0x4, CondBase, Vt(0x00937B78)>);
CONDFAC_REVERSED("ConditionCanGetOffVehicleHereLeft::New", 0x00649A00, &New<0x4, CondBase, Vt(0x00937B88)>);
CONDFAC_REVERSED("ConditionCanGetOffVehicleHere::New", 0x00649A30, &New<0x4, CondBase, Vt(0x00937B9C)>);
CONDFAC_REVERSED("ConditionCanGetOnVehicleHere::New", 0x00649A60, &New<0x4, CondBase, Vt(0x00937BB0)>);
CONDFAC_REVERSED("ConditionCanGetOnVehicleLeftSide::New", 0x00649A90, &New<0x4, CondBase, Vt(0x00937BC4)>);
CONDFAC_REVERSED("ConditionCanGetOnVehicleRightSide::New", 0x00649AC0, &New<0x4, CondBase, Vt(0x00937BD8)>);
CONDFAC_REVERSED("ConditionTestRequiredGiftType::New", 0x00649AF0, &New<0x8, CondBase, Vt(0x00937BEC), Then, I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionTestFutureGiftType::New", 0x00649B20, &New<0x8, CondBase, Vt(0x00937BFC), Then, I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionHasGiftItemOfType::New", 0x00649B50, &New<0x8, CondBase, Vt(0x00937C0C), Then, I32(0x4, 0x0)>);
CONDFAC_REVERSED("ConditionHasPlayerMaxItems::New", 0x00649B80, &New<0xC, CondBase, Vt(0x00937C1C), Key(0x4), Key(0x8)>);
CONDFAC_REVERSED("ConditionPlayerHasItemCount::New", 0x00649BC0, &New<0x10, CondBase, Vt(0x00937C2C), Key(0x4), Then, Call(0x005FD000)>);
CONDFAC_REVERSED("ConditionIsPowerSliding::New", 0x00649C00, &New<0x4, CondBase, Vt(0x00937C3C)>);
CONDFAC_REVERSED("ConditionShouldAllowApology::New", 0x00649C30, &New<0x4, CondBase, Vt(0x00937C4C)>);
CONDFAC_REVERSED("ConditionBeamLateralDeviation::New", 0x00649C60, &New<0xC, CondBase, Vt(0x00937C5C), Then, Call(0x005FC6B0)>);
CONDFAC_REVERSED("ConditionBeamLateralPosition::New", 0x00649CA0, &New<0x8, CondBase, Vt(0x00937C70), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionWeaponFlags::New", 0x00649CE0, &New<0xC, CondBase, Vt(0x00937C84), Then, Call(0x007769A0)>);
CONDFAC_REVERSED("ConditionWeaponCanBeUsedOnBike::New", 0x00649D20, &New<0x4, CondBase, Vt(0x00937C98)>);
CONDFAC_REVERSED("ConditionPropModelID::New", 0x00649D50, &New<0x8, CondBase, Vt(0x00937CAC), Key(0x4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionPropUniqueID::New", 0x00649D90, &New<0x8, CondBase, Vt(0x00937CC0), Key(0x4), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionVehicleHitOrientation::New", 0x00649DD0, &New<0x10, CondBase, Vt(0x00937CD4), Then, Call(0x005FD270)>);
CONDFAC_REVERSED("ConditionPlayerHasPunishmentPoints::New", 0x00649E10, &New<0xC, CondBase, Vt(0x00937CE8), Then, Call(0x005FD290)>);
CONDFAC_REVERSED("ConditionTargetPropClearOfPeds::New", 0x00649E50, &New<0xC, CondBase, Vt(0x00937CFC), Key(0x4), Then, Call(0x005FD2C0)>);
CONDFAC_REVERSED("ConditionPOIClearOfPeds::New", 0x00649E90, &New<0x8, CondBase, Vt(0x00937D10), Then, Call(0x005FD2D0)>);
CONDFAC_REVERSED("ConditionPropDisplayUseText::New", 0x00649ED0, &New<0x4, CondBase, Vt(0x00937D24), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionAllyIsInCombat::New", 0x00649F10, &New<0x4, CondBase, Vt(0x00937D38)>);
CONDFAC_REVERSED("ConditionAllyState::New", 0x00649F40, &New<0x8, CondBase, Vt(0x00937D4C), Then, Call(0x005FC690)>);
CONDFAC_REVERSED("ConditionAllyMorale::New", 0x00649F80, &New<0x8, CondBase, Vt(0x00937D60), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionMissionAvailable::New", 0x00649FC0, &New<0x8, CondBase, Vt(0x00937D74), Key(0x4)>);
CONDFAC_REVERSED("ConditionPropPedIsUsingMe::New", 0x00649FF0, &New<0x4, CondBase, Vt(0x00937D88), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionNumWeapons::New", 0x0064A030, &New<0xC, CondBase, Vt(0x00937D9C), Then, Call(0x005FCF70)>);
CONDFAC_REVERSED("ConditionCameraFading::New", 0x0064A070, &New<0x4, CondBase, Vt(0x00937DB0)>);
CONDFAC_REVERSED("ConditionHasGrappleTarget::New", 0x0064A0A0, &New<0x4, CondBase, Vt(0x00937DC4)>);
CONDFAC_REVERSED("ConditionHasNodeCooldown::New", 0x0064A0D0, &New<0x8, CondBase, Vt(0x00937DD8), Then, Call(0x00655150)>);
CONDFAC_REVERSED("ConditionIsIgnoreTruants::New", 0x0064A110, &New<0x4, CondBase, Vt(0x00937DEC)>);
CONDFAC_REVERSED("ConditionThisNodeIsPlaying::New", 0x0064A140, &New<0x8, CondBase, Vt(0x00937E00), Then, Call(0x005FD3B0)>);
CONDFAC_REVERSED("ConditionThisNodeIsNotPlaying::New", 0x0064A180, &New<0x8, CondBase, Vt(0x00937E14), Then, Call(0x005FD3B0)>);
CONDFAC_REVERSED("ConditionPlayerHasTarget::New", 0x0064A1C0, &New<0x4, CondBase, Vt(0x00937E28)>);
CONDFAC_REVERSED("ConditionTargetMyTargetsProp::New", 0x0064A1F0, &New<0x4, CondBase, Vt(0x00937E3C)>);
CONDFAC_REVERSED("ConditionNumAuthorityInPursuit::New", 0x0064A220, &New<0xC, CondBase, Vt(0x00937E50), Then, Call(0x005FD3C0)>);
CONDFAC_REVERSED("ConditionIsTagWaiting::New", 0x0064A260, &New<0x4, CondBase, Vt(0x00937E64)>);
CONDFAC_REVERSED("ConditionIsTagDrawing::New", 0x0064A290, &New<0x4, CondBase, Vt(0x00937E74)>);
CONDFAC_REVERSED("ConditionIsNISorCutsceneRunning::New", 0x0064A2C0, &New<0x8, CondBase, Vt(0x00937E84), Then, Call(0x005FD410)>);
CONDFAC_REVERSED("ConditionIsPlayerFacingMirror::New", 0x0064A300, &New<0x8, CondBase, Vt(0x00937E98), Then, Call(0x005FD450)>);
CONDFAC_REVERSED("ConditionPropTargetIsTwoHanded::New", 0x0064A340, &New<0x4, CondBase, Vt(0x00937EA8), Then, KeyDtor(0x0)>);
CONDFAC_REVERSED("ConditionObjectiveOfType::New", 0x0064A380, &New<0x8, CondBase, Vt(0x0092E694), Then, Call(0x005FC640)>);
CONDFAC_REVERSED("ConditionSenseTargetNow::New", 0x0064A3C0, &New<0x4, CondBase, Vt(0x00937EBC)>);
CONDFAC_REVERSED("ConditionIsWeaponFiredWhenDropped::New", 0x0064A3F0, &New<0x4, CondBase, Vt(0x00937ECC)>);
CONDFAC_REVERSED("ConditionIsElderly::New", 0x0064A420, &New<0x4, CondBase, Vt(0x00937EE0)>);
CONDFAC_REVERSED("ConditionIsRegisteredAsGirlfriend::New", 0x0064A450, &New<0x4, CondBase, Vt(0x00937EF4)>);
CONDFAC_REVERSED("ConditionIsInFight::New", 0x0064A480, &New<0x4, CondBase, Vt(0x00937F08)>);
CONDFAC_REVERSED("ConditionScriptSaveData::New", 0x0064A4B0, &New<0x10, CondBase, Vt(0x00937F1C), Then, Call(0x005FD460)>);
CONDFAC_REVERSED("ConditionPlayerHasPunishmentOfType::New", 0x0064A4F0, &New<0x8, CondBase, Vt(0x00937F30)>);
CONDFAC_REVERSED("ConditionIsMiniObjectiveComplete::New", 0x0064A520, &New<0x8, CondBase, Vt(0x00937F44), Then, Call(0x005FD140)>);
CONDFAC_REVERSED("ConditionIsAvoiding::New", 0x0064A560, &New<0xC, CondBase, Vt(0x00937F58), Then, Call(0x005FD4D0)>);
CONDFAC_REVERSED("ConditionFrontEndActive::New", 0x0064A5A0, &New<0xC, CondBase, Vt(0x00937F6C), Then, Call(0x005FD4F0)>);
CONDFAC_REVERSED("ConditionPedHasNumAttackers::New", 0x0064A5E0, &New<0xC, CondBase, Vt(0x00937F80), Then, Call(0x005FD290)>);
CONDFAC_REVERSED("ConditionIsTakingPictureFreeze::New", 0x0064A620, &New<0x4, CondBase, Vt(0x00937F94)>);
CONDFAC_REVERSED("ConditionIsUsingDoor::New", 0x0064A650, &New<0x4, CondBase, Vt(0x00937FA8)>);
CONDFAC_REVERSED("ConditionPropIsPlaying::New", 0x0064B0B0, &New<0xC, CondBase, Vt(0x0092E5B8), Key(0x4), Vt(0x00938358), Then, Call(0x00655070)>);
CONDFAC_REVERSED("ConditionAIConditionGroup::New", 0x0064B100, &New<0x8, CondBase, Vt(0x0093836C), Then, KeyDtor(0x0)>);
