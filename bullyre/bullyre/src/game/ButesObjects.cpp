// The tuning attribute sets (the *Butes classes: camera, dodgeball, AI, player, props,
// ...) and a few tracks: their deleting destructors and the factories that construct one
// in given memory (null in: null out). A Butes object has the track base (0x61A880:
// +0 vtable, +4 link, +0xC start 0, +0x10 end -1), then its fields; the name keys among
// them start empty (0x66BEA0). 0x62FD50..0x630E50.

#include <cstdint>

#include "game/ActionTracksCommon.h"

using namespace actc;

namespace {

constexpr uint32_t kVtStatSetTrack = 0x009343F0;
constexpr uint32_t kVtGetOnOffVehicleTrack = 0x00934428;
constexpr uint32_t kVtWeaponCallbackTrack = 0x00934440;
constexpr uint32_t kVtConditionEntityIsModel = 0x0093446C;
constexpr uint32_t kVtActionRequestTrack = 0x009348A0;
constexpr uint32_t kVtActivateDoorTrack = 0x009348B8;

GAME_FN(void, __thiscall, TrackBase_Construct, 0x0061A880, uint8_t* track);
GAME_FN(uint32_t*, __thiscall, NameKey_Default, 0x0066BEA0, uint32_t* key);
GAME_FN(void, __thiscall, EffectProxy_Construct, 0x00668DA0, uint8_t* proxy);

// a deleting destructor through the class's own destructor (thiscall at Dtor)
template <uintptr_t Dtor>
uint8_t* __fastcall DeletingDtorVia(uint8_t* self, re::Edx, uint32_t flags) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(Dtor)(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// the track base, the vtable, the name keys at the given offsets
template <uintptr_t Vt, int... Keys>
void Build(uint8_t* p) {
    TrackBase_Construct(p);
    At<uint32_t>(p, 0) = uint32_t(Vt);
    (NameKey_Default(&At<uint32_t>(p, Keys)), ...);
}
// constructed in the memory given (cdecl; null: null)
template <uintptr_t Vt, int... Keys>
uint8_t* __cdecl CreateAt(uint8_t* p) {
    if (!p) {
        return nullptr;
    }
    Build<Vt, Keys...>(p);
    return p;
}
// the same with an effect proxy at +0x2C (BreathTrack)
template <uintptr_t Vt, int... Keys>
uint8_t* __cdecl CreateAtProxy(uint8_t* p) {
    if (!p) {
        return nullptr;
    }
    Build<Vt, Keys...>(p);
    EffectProxy_Construct(p + 0x2C);
    return p;
}
// a constructor (thiscall)
template <uintptr_t Vt, int... Keys>
uint8_t* __fastcall ConstructFields(uint8_t* self, re::Edx) {
    Build<Vt, Keys...>(self);
    return self;
}

// 0x62FDE0 (thiscall): ConditionEntityIsModel's deleting destructor (its name key at +4)
uint8_t* __fastcall ConditionEntityIsModel_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtConditionEntityIsModel;
    NameKey_Destruct(self + 4);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

} // namespace

#define BUTESOBJ_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

BUTESOBJ_REVERSED("StatSetTrack::DeletingDestruct", 0x0062FD50, &TrackDeletingDtor<kVtStatSetTrack>);
BUTESOBJ_REVERSED("GetOnOffVehicleTrack::DeletingDestruct", 0x0062FD80, &TrackDeletingDtor<kVtGetOnOffVehicleTrack>);
BUTESOBJ_REVERSED("WeaponCallbackTrack::DeletingDestruct", 0x0062FDB0, &TrackDeletingDtor<kVtWeaponCallbackTrack>);
BUTESOBJ_REVERSED("ConditionEntityIsModel::DeletingDestruct", 0x0062FDE0, &ConditionEntityIsModel_DeletingDtor);
BUTESOBJ_REVERSED("CoasterCamButes::DeletingDestruct", 0x0062FE10, &DeletingDtorVia<0x0072A5E0>);
BUTESOBJ_REVERSED("CoasterCamButes::Create", 0x0062FE30, &CreateAt<0x00934480>);
BUTESOBJ_REVERSED("VehicleCamButes::Create", 0x0062FE50, &CreateAt<0x00914740>);
BUTESOBJ_REVERSED("FollowCam3Butes::Create", 0x0062FE70, &CreateAt<0x00914580>);
BUTESOBJ_REVERSED("DodgeballCamButes::DeletingDestruct", 0x0062FE90, &DeletingDtorVia<0x0072A610>);
BUTESOBJ_REVERSED("DodgeballCamButes::Create", 0x0062FEB0, &CreateAt<0x00934498>);
BUTESOBJ_REVERSED("DodgeballPhysicsButes::DeletingDestruct", 0x0062FED0, &DeletingDtorVia<0x0072A620>);
BUTESOBJ_REVERSED("DodgeballPhysicsButes::Create", 0x0062FEF0, &CreateAt<0x009344B0>);
BUTESOBJ_REVERSED("DodgeballAIButes::DeletingDestruct", 0x0062FF10, &DeletingDtorVia<0x0072A630>);
BUTESOBJ_REVERSED("DodgeballAIButes::Create", 0x0062FF30, &CreateAt<0x009344C8>);
BUTESOBJ_REVERSED("DodgeballAIRampButes::DeletingDestruct", 0x0062FF50, &DeletingDtorVia<0x0072A640>);
BUTESOBJ_REVERSED("DodgeballAIRampButes::Create", 0x0062FF70, &CreateAt<0x009344E0>);
BUTESOBJ_REVERSED("DodgeballGameButes::DeletingDestruct", 0x0062FF90, &DeletingDtorVia<0x0072A650>);
BUTESOBJ_REVERSED("DodgeballGameButes::Create", 0x0062FFB0, &CreateAt<0x009344F8>);
BUTESOBJ_REVERSED("AIButes::DeletingDestruct", 0x0062FFD0, &DeletingDtorVia<0x0072A660>);
BUTESOBJ_REVERSED("AIButes::Create", 0x0062FFF0, &CreateAt<0x00934510>);
BUTESOBJ_REVERSED("FightCollisionButes::DeletingDestruct", 0x00630010, &DeletingDtorVia<0x0072A670>);
BUTESOBJ_REVERSED("FightCollisionButes::Create", 0x00630030, &CreateAt<0x00934528>);
BUTESOBJ_REVERSED("ButtonTriggerButes::DeletingDestruct", 0x00630050, &DeletingDtorVia<0x0072A680>);
BUTESOBJ_REVERSED("ButtonTriggerButes::Create", 0x00630070, &CreateAt<0x00934540>);
BUTESOBJ_REVERSED("TimePeriodButes::DeletingDestruct", 0x00630090, &DeletingDtorVia<0x0072A690>);
BUTESOBJ_REVERSED("TimePeriodButes::Create", 0x006300B0, &CreateAt<0x00934558, 0x18, 0x1C, 0x20>);
BUTESOBJ_REVERSED("TimeCycleButes::DeletingDestruct", 0x006300F0, &DeletingDtorVia<0x0072A6E0>);
BUTESOBJ_REVERSED("TimeCycleButes::Create", 0x00630110, &CreateAt<0x00934570, 0x1C, 0x20, 0x24, 0x28>);
BUTESOBJ_REVERSED("DodgeballArenaButes::DeletingDestruct", 0x00630150, &DeletingDtorVia<0x0072A750>);
BUTESOBJ_REVERSED("DodgeballArenaButes::Create", 0x00630170, &CreateAt<0x00934588, 0x18, 0x1C, 0x20>);
BUTESOBJ_REVERSED("DebugPlayerButes::DeletingDestruct", 0x006301B0, &DeletingDtorVia<0x0072A7A0>);
BUTESOBJ_REVERSED("DebugPlayerButes::Create", 0x006301D0, &CreateAt<0x009345A0>);
BUTESOBJ_REVERSED("PlayerButes::DeletingDestruct", 0x006301F0, &DeletingDtorVia<0x0072A7B0>);
BUTESOBJ_REVERSED("PlayerButes::Create", 0x00630210, &CreateAt<0x009345B8, 0xFC, 0x10C, 0x114, 0x11C>);
BUTESOBJ_REVERSED("FrisbeeButes::DeletingDestruct", 0x00630260, &DeletingDtorVia<0x0072A830>);
BUTESOBJ_REVERSED("FrisbeeButes::Create", 0x00630280, &CreateAt<0x009345D0>);
BUTESOBJ_REVERSED("FirstPersonCamButes::DeletingDestruct", 0x006302A0, &DeletingDtorVia<0x0072A840>);
BUTESOBJ_REVERSED("FirstPersonCamButes::Create", 0x006302C0, &CreateAt<0x009345E8>);
BUTESOBJ_REVERSED("OffsetCamButes::DeletingDestruct", 0x006302E0, &DeletingDtorVia<0x0072A850>);
BUTESOBJ_REVERSED("OffsetCamButes::Create", 0x00630300, &CreateAt<0x00934600, 0x58, 0x5C, 0x64, 0x80>);
BUTESOBJ_REVERSED("ProjectileButes::Construct", 0x00630350, &ConstructFields<0x00934618, 0x54, 0x58, 0xA4, 0xA8, 0xAC, 0xB4, 0xB8, 0xC0, 0x12C, 0x134, 0x138, 0x13C, 0x140>);
BUTESOBJ_REVERSED("ProjectileButes::DeletingDestruct", 0x006303F0, &DeletingDtorVia<0x0072BD30>);
BUTESOBJ_REVERSED("CrosshairButes::DeletingDestruct", 0x00630420, &DeletingDtorVia<0x0072A8C0>);
BUTESOBJ_REVERSED("CrosshairButes::Create", 0x00630440, &CreateAt<0x00934630, 0x18>);
BUTESOBJ_REVERSED("ActionMapButes::DeletingDestruct", 0x00630470, &DeletingDtorVia<0x0072A8F0>);
BUTESOBJ_REVERSED("ActionMapButes::Create", 0x00630490, &CreateAt<0x00934648>);
BUTESOBJ_REVERSED("PickupButes::Create", 0x006304B0, &CreateAt<0x009053B4>);
BUTESOBJ_REVERSED("ObjectButes::Construct", 0x006304D0, &ConstructFields<0x00934660, 0x1C, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C, 0x74, 0x78, 0x8C, 0x90, 0x94, 0x9C, 0xA0>);
BUTESOBJ_REVERSED("ObjectButes::DeletingDestruct", 0x00630570, &DeletingDtorVia<0x0072A910>);
BUTESOBJ_REVERSED("CinematicButes::DeletingDestruct", 0x006305A0, &DeletingDtorVia<0x0072AA50>);
BUTESOBJ_REVERSED("CinematicButes::Create", 0x006305C0, &CreateAt<0x00934678>);
BUTESOBJ_REVERSED("MenuButes::DeletingDestruct", 0x006305E0, &DeletingDtorVia<0x0072AA60>);
BUTESOBJ_REVERSED("MenuButes::Create", 0x00630600, &CreateAt<0x00934690>);
BUTESOBJ_REVERSED("CinematicShotButes::Create", 0x00630620, &CreateAt<0x00914398>);
BUTESOBJ_REVERSED("CinematicShotGroupButes::Construct", 0x00630640, &ConstructFields<0x009346A8, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C>);
BUTESOBJ_REVERSED("CinematicShotGroupButes::DeletingDestruct", 0x006306B0, &DeletingDtorVia<0x0072AA80>);
BUTESOBJ_REVERSED("StimulusButes::DeletingDestruct", 0x006306E0, &DeletingDtorVia<0x0072AB50>);
BUTESOBJ_REVERSED("StimulusButes::Create", 0x00630700, &CreateAt<0x009346C0>);
BUTESOBJ_REVERSED("PunishmentButes::DeletingDestruct", 0x00630720, &DeletingDtorVia<0x0072AB60>);
BUTESOBJ_REVERSED("PunishmentButes::Create", 0x00630740, &CreateAt<0x009346D8, 0x2C>);
BUTESOBJ_REVERSED("PropButes::Construct", 0x00630770, &ConstructFields<0x009346F0, 0x30, 0x34, 0x3C, 0x40, 0x48, 0x4C, 0x54, 0x58, 0x5C, 0x60, 0x68, 0x6C, 0x70, 0x80>);
BUTESOBJ_REVERSED("PropButes::DeletingDestruct", 0x00630800, &DeletingDtorVia<0x0072AB90>);
BUTESOBJ_REVERSED("PropExtraButes::DeletingDestruct", 0x00630830, &DeletingDtorVia<0x0072ACB0>);
BUTESOBJ_REVERSED("PropExtraButes::Create", 0x00630850, &CreateAt<0x00934708>);
BUTESOBJ_REVERSED("PickupListButes::Construct", 0x00630870, &ConstructFields<0x00934720, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48>);
BUTESOBJ_REVERSED("PickupListButes::DeletingDestruct", 0x006308C0, &DeletingDtorVia<0x0072ACC0>);
BUTESOBJ_REVERSED("BikeButes::DeletingDestruct", 0x006308F0, &DeletingDtorVia<0x0072AD60>);
BUTESOBJ_REVERSED("BikeButes::Create", 0x00630910, &CreateAt<0x00934738>);
BUTESOBJ_REVERSED("PhotoButes::DeletingDestruct", 0x00630930, &DeletingDtorVia<0x0072AD70>);
BUTESOBJ_REVERSED("PhotoButes::Create", 0x00630950, &CreateAt<0x00934750>);
BUTESOBJ_REVERSED("TaggingButes::DeletingDestruct", 0x00630970, &DeletingDtorVia<0x0072AD80>);
BUTESOBJ_REVERSED("TaggingButes::Create", 0x00630990, &CreateAt<0x00934768>);
BUTESOBJ_REVERSED("SurfaceSoundRangeButes::DeletingDestruct", 0x006309B0, &DeletingDtorVia<0x0072AD90>);
BUTESOBJ_REVERSED("SurfaceSoundRangeButes::Create", 0x006309D0, &CreateAt<0x00934780>);
BUTESOBJ_REVERSED("AreaTransitionButes::DeletingDestruct", 0x006309F0, &DeletingDtorVia<0x0072ADA0>);
BUTESOBJ_REVERSED("AreaTransitionButes::Create", 0x00630A10, &CreateAt<0x00934798>);
BUTESOBJ_REVERSED("TrafficButes::DeletingDestruct", 0x00630A30, &DeletingDtorVia<0x0072ADB0>);
BUTESOBJ_REVERSED("TrafficButes::Create", 0x00630A50, &CreateAt<0x009347B0>);
BUTESOBJ_REVERSED("PlayerAwarenessButes::DeletingDestruct", 0x00630A70, &DeletingDtorVia<0x0072ADC0>);
BUTESOBJ_REVERSED("PlayerAwarenessButes::Create", 0x00630A90, &CreateAt<0x009347C8>);
BUTESOBJ_REVERSED("StoreCamButes::DeletingDestruct", 0x00630AB0, &DeletingDtorVia<0x0072ADD0>);
BUTESOBJ_REVERSED("StoreCamButes::Create", 0x00630AD0, &CreateAt<0x009347E0>);
BUTESOBJ_REVERSED("SoccerPButes::DeletingDestruct", 0x00630AF0, &DeletingDtorVia<0x0072ADE0>);
BUTESOBJ_REVERSED("SoccerPButes::Create", 0x00630B10, &CreateAt<0x009347F8>);
BUTESOBJ_REVERSED("RadarButes::DeletingDestruct", 0x00630B30, &DeletingDtorVia<0x0072ADF0>);
BUTESOBJ_REVERSED("RadarButes::Create", 0x00630B50, &CreateAt<0x00934810>);
BUTESOBJ_REVERSED("ArmWrestlingButes::DeletingDestruct", 0x00630B70, &DeletingDtorVia<0x0072AE00>);
BUTESOBJ_REVERSED("ArmWrestlingButes::Create", 0x00630B90, &CreateAt<0x00934828>);
BUTESOBJ_REVERSED("MiscGameButes::DeletingDestruct", 0x00630BB0, &DeletingDtorVia<0x0072AE10>);
BUTESOBJ_REVERSED("MiscGameButes::Create", 0x00630BD0, &CreateAt<0x00934840>);
BUTESOBJ_REVERSED("ArcadeRaceCameraButes::DeletingDestruct", 0x00630BF0, &DeletingDtorVia<0x0072AE20>);
BUTESOBJ_REVERSED("ArcadeRaceCameraButes::Create", 0x00630C10, &CreateAt<0x00934858>);
BUTESOBJ_REVERSED("GlobalSpineControlButes::DeletingDestruct", 0x00630C30, &DeletingDtorVia<0x0072AE30>);
BUTESOBJ_REVERSED("GlobalSpineControlButes::Create", 0x00630C50, &CreateAt<0x00934870>);
BUTESOBJ_REVERSED("SpineControlWeightButes::DeletingDestruct", 0x00630C70, &DeletingDtorVia<0x0072AE40>);
BUTESOBJ_REVERSED("SpineControlWeightButes::Create", 0x00630C90, &CreateAt<0x00934888>);
BUTESOBJ_REVERSED("ActionRequestTrack::DeletingDestruct", 0x00630CB0, &TrackDeletingDtor<kVtActionRequestTrack>);
BUTESOBJ_REVERSED("ActionRequestTrack::Create", 0x00630CE0, &CreateAt<0x009348A0>);
BUTESOBJ_REVERSED("ActionNodeRequestTrack::Create", 0x00630D00, &CreateAt<0x009334DC, 0x18>);
BUTESOBJ_REVERSED("ActivateDoorTrack::DeletingDestruct", 0x00630D30, &TrackDeletingDtor<kVtActivateDoorTrack>);
BUTESOBJ_REVERSED("ActivateDoorTrack::Create", 0x00630D60, &CreateAt<0x009348B8>);
BUTESOBJ_REVERSED("AddAttachedEffectTrack::Create", 0x00630D80, &CreateAt<0x00931414, 0x18, 0x1C>);
BUTESOBJ_REVERSED("DetachEffectTrack::Create", 0x00630DB0, &CreateAt<0x009313FC, 0x18, 0x1C>);
BUTESOBJ_REVERSED("BreathTrack::Create", 0x00630DE0, &CreateAtProxy<0x0093142C, 0x18, 0x1C>);
BUTESOBJ_REVERSED("FXCeilingTrack::Create", 0x00630E20, &CreateAt<0x00931444, 0x18>);
BUTESOBJ_REVERSED("FXDecalTrack::Create", 0x00630E50, &CreateAt<0x0093145C, 0x18, 0x1C>);
