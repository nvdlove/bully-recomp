// The dodgeball minigame's action tree tracks' and conditions' factories and destructors
// (0x672A10..0x6764D0): the track and condition registries' New (cdecl: allocated and
// built, for code; missing from Ghidra's function list: only the registries' tables refer
// to them), CreateAt (cdecl: built in place in the loaded tree, null: none) and deleting
// destructors. The shapes are in ActionFactories.h; the classes' virtuals are in
// DodgeballTracks.cpp.

#include <cstdint>

#include "game/ActionFactories.h"
#include "game/ActionTracksCommon.h"

using namespace actfac;

// (variadic: the template arguments' commas)
#define DODGEFAC35_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

// 0x672B80 (thiscall): DodgeballBaseAITrack's destructor
DODGEFAC35_REVERSED("DodgeballBaseAITrack::Destruct_672B80", 0x00672B80, &actc::TrackDtor<0x0093D9F8>);

DODGEFAC35_REVERSED("DodgeballLeadAITrack::CreateAt_673180", 0x00673180, &CreateAt<TrackBase, Vt(0x0093DCD8), Zero(0x3C), Zero(0x40), Zero(0x44)>);
DODGEFAC35_REVERSED("DodgeballSupportAITrack::CreateAt_6731B0", 0x006731B0, &CreateAt<TrackBase, Vt(0x0093DCF0)>);
DODGEFAC35_REVERSED("DodgeballSidelineLeadAITrack::CreateAt_6731D0", 0x006731D0, &CreateAt<TrackBase, Vt(0x0093DD08)>);
DODGEFAC35_REVERSED("DodgeballSidelineAITrack::CreateAt_6731F0", 0x006731F0, &CreateAt<TrackBase, Vt(0x0093DD20)>);
DODGEFAC35_REVERSED("DodgeballFaceLeadTrack::DeletingDestruct_673210", 0x00673210, &DeletingDtor<Call(0x00682950)>);
DODGEFAC35_REVERSED("DodgeballFaceLeadTrack::CreateAt_673230", 0x00673230, &CreateAt<TrackBase, Vt(0x0093DD38)>);
DODGEFAC35_REVERSED("DodgeballCatchBallTrack::DeletingDestruct_673250", 0x00673250, &DeletingDtor<Call(0x006829C0)>);
DODGEFAC35_REVERSED("DodgeballCatchBallTrack::CreateAt_673270", 0x00673270, &CreateAt<TrackBase, Vt(0x0093DD50)>);
DODGEFAC35_REVERSED("DodgeballGetBallAITrack::CreateAt_673290", 0x00673290, &CreateAt<TrackBase, Vt(0x0093DD68)>);
DODGEFAC35_REVERSED("DodgeballGotoAITrack::DeletingDestruct_6732B0", 0x006732B0, &DeletingDtor<Call(0x00682A10)>);
DODGEFAC35_REVERSED("DodgeballGotoAITrack::CreateAt_6732D0", 0x006732D0, &CreateAt<TrackBase, Vt(0x0093DD80)>);
DODGEFAC35_REVERSED("DodgeballIdleAITrack::DeletingDestruct_6732F0", 0x006732F0, &DeletingDtor<Call(0x00682A30)>);
DODGEFAC35_REVERSED("DodgeballIdleAITrack::CreateAt_673310", 0x00673310, &CreateAt<TrackBase, Vt(0x0093DD98)>);
DODGEFAC35_REVERSED("DodgeballSetPathTrack::New_673330", 0x00673330, &New<0x24, TrackBase, Vt(0x0093DA10), Then, U8(0x8, 0), I32(0x18, 0x0), I32(0x1C, 0x4)>);
DODGEFAC35_REVERSED("DodgeballSetPathTrack::DeletingDestruct_673380", 0x00673380, &DeletingDtor<Vt(0x0093DA10), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballSetPathTrack::CreateAt_6733B0", 0x006733B0, &CreateAt<TrackBase, Vt(0x0093DA10)>);
DODGEFAC35_REVERSED("DodgeballFollowPathTrack::New_6733D0", 0x006733D0, &New<0x24, TrackBase, Vt(0x0093DA28), Then, U8(0x8, 1), F32(0x18, 0.30000001192092896f), U8(0x1C, 0), U8(0x1D, 0)>);
DODGEFAC35_REVERSED("DodgeballFollowPathTrack::DeletingDestruct_673410", 0x00673410, &DeletingDtor<Vt(0x0093DA28), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballFollowPathTrack::CreateAt_673440", 0x00673440, &CreateAt<TrackBase, Vt(0x0093DA28)>);
DODGEFAC35_REVERSED("CDodgeballChangePlayerTrack::New_673460", 0x00673460, &New<0x2c, TrackBase, Vt(0x0093DA40), Then, U8(0x18, 0), U8(0x19, 0), U8(0x1A, 0), U8(0x1B, 0), U8(0x8, 1)>);
DODGEFAC35_REVERSED("CDodgeballChangePlayerTrack::DeletingDestruct_6734B0", 0x006734B0, &DeletingDtor<Vt(0x0093DA40), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballChangePlayerTrack::CreateAt_6734E0", 0x006734E0, &CreateAt<TrackBase, Vt(0x0093DA40)>);
DODGEFAC35_REVERSED("CDodgeballSetPedFlagTrack::New_673500", 0x00673500, &New<0x24, TrackBase, Vt(0x0093DA58), Then, U8(0x8, 0), I32(0x18, 0x0), U8(0x1D, 0), U8(0x1E, 0), U8(0x1C, 1)>);
DODGEFAC35_REVERSED("CDodgeballSetPedFlagTrack::DeletingDestruct_673550", 0x00673550, &DeletingDtor<Vt(0x0093DA58), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballSetPedFlagTrack::CreateAt_673580", 0x00673580, &CreateAt<TrackBase, Vt(0x0093DA58)>);
DODGEFAC35_REVERSED("CDodgeballTargetSetPedFlagTrack::New_6735A0", 0x006735A0, &New<0x24, TrackBase, Vt(0x0093DA70), Then, U8(0x8, 0), I32(0x18, 0x0), U8(0x1D, 0), U8(0x1E, 0), U8(0x1C, 1)>);
DODGEFAC35_REVERSED("CDodgeballTargetSetPedFlagTrack::DeletingDestruct_6735F0", 0x006735F0, &DeletingDtor<Vt(0x0093DA70), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballTargetSetPedFlagTrack::CreateAt_673620", 0x00673620, &CreateAt<TrackBase, Vt(0x0093DA70)>);
DODGEFAC35_REVERSED("CDodgeballTargetTrack::New_673640", 0x00673640, &New<0x24, TrackBase, Vt(0x0093DA88), Then, U8(0x8, 0), I32(0x18, 0x0), U8(0x1D, 1), U8(0x1E, 0)>);
DODGEFAC35_REVERSED("CDodgeballTargetTrack::DeletingDestruct_673680", 0x00673680, &DeletingDtor<Vt(0x0093DA88), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballTargetTrack::CreateAt_6736B0", 0x006736B0, &CreateAt<TrackBase, Vt(0x0093DA88)>);
DODGEFAC35_REVERSED("CDodgeballSetGameFlagTrack::New_6736D0", 0x006736D0, &New<0x24, TrackBase, Vt(0x0093DAA0), Then, U8(0x8, 1), I32(0x18, 0x0), U8(0x1C, 1)>);
DODGEFAC35_REVERSED("CDodgeballSetGameFlagTrack::DeletingDestruct_673710", 0x00673710, &DeletingDtor<Vt(0x0093DAA0), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballSetGameFlagTrack::CreateAt_673740", 0x00673740, &CreateAt<TrackBase, Vt(0x0093DAA0)>);
DODGEFAC35_REVERSED("DodgeballGameIdleTrack::New_673760", 0x00673760, &New<0x20, TrackBase, Vt(0x0093DAB8), Then, U8(0x8, 1), U8(0x18, 0)>);
DODGEFAC35_REVERSED("DodgeballGameIdleTrack::DeletingDestruct_6737A0", 0x006737A0, &DeletingDtor<Vt(0x0093DAB8), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballGameIdleTrack::CreateAt_6737D0", 0x006737D0, &CreateAt<TrackBase, Vt(0x0093DAB8)>);
DODGEFAC35_REVERSED("DodgeballGameTimerTrack::New_6737F0", 0x006737F0, &New<0xf8, TrackBase, Vt(0x0093DAD0), Key(0x18), Then, F32(0x1C, 5.0f), U8(0x8, 1), U8(0x20, 1), U8(0x21, 0), ClearName(0x18), U8(0x22, 1)>);
DODGEFAC35_REVERSED("DodgeballGameTimerTrack::DeletingDestruct_673860", 0x00673860, &DeletingDtor<Vt(0x0093DAD0), KeyDtor(0x18), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballGameTimerTrack::CreateAt_673890", 0x00673890, &CreateAt<TrackBase, Vt(0x0093DAD0), Key(0x18)>);
DODGEFAC35_REVERSED("DodgeballResetTrack::New_6738C0", 0x006738C0, &New<0x20, TrackBase, Vt(0x0093DAE8), Then, U8(0x8, 1), U8(0x18, 1)>);
DODGEFAC35_REVERSED("DodgeballResetTrack::DeletingDestruct_673900", 0x00673900, &DeletingDtor<Vt(0x0093DAE8), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballResetTrack::CreateAt_673930", 0x00673930, &CreateAt<TrackBase, Vt(0x0093DAE8)>);
DODGEFAC35_REVERSED("DodgeballSetTurnTrack::New_673950", 0x00673950, &New<0x20, TrackBase, Vt(0x0093DB00), Then, U8(0x8, 1), I32(0x18, 0x0)>);
DODGEFAC35_REVERSED("DodgeballSetTurnTrack::DeletingDestruct_673990", 0x00673990, &DeletingDtor<Vt(0x0093DB00), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballSetTurnTrack::CreateAt_6739C0", 0x006739C0, &CreateAt<TrackBase, Vt(0x0093DB00)>);
DODGEFAC35_REVERSED("DodgeballPedSetDuckTrack::New_6739E0", 0x006739E0, &New<0x20, TrackBase, Vt(0x0093DB18), Then, Zero(0x18), U8(0x8, 0)>);
DODGEFAC35_REVERSED("DodgeballPedSetDuckTrack::DeletingDestruct_673A20", 0x00673A20, &DeletingDtor<Vt(0x0093DB18), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballPedSetDuckTrack::CreateAt_673A50", 0x00673A50, &CreateAt<TrackBase, Vt(0x0093DB18)>);
DODGEFAC35_REVERSED("CDodgeballLocationCollisionTrack::New_673A70", 0x00673A70, &New<0x30, TrackBase, Vt(0x0093DB30), Then, F32(0x18, -0.5f), U8(0x8, 0), U8(0x28, 1), F32(0x20, 0.5f), U8(0x29, 1), F32(0x24, 0.5f), U8(0x2A, 0), F32(0x1C, -0.5f)>);
DODGEFAC35_REVERSED("CDodgeballLocationCollisionTrack::DeletingDestruct_673AC0", 0x00673AC0, &DeletingDtor<Vt(0x0093DB30), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballLocationCollisionTrack::CreateAt_673AF0", 0x00673AF0, &CreateAt<TrackBase, Vt(0x0093DB30)>);
DODGEFAC35_REVERSED("DodgeballSetMessageTrack::New_673B10", 0x00673B10, &New<0x20, TrackBase, Vt(0x0093DB48), Key(0x18), Then, U8(0x8, 0), ClearName(0x18)>);
DODGEFAC35_REVERSED("DodgeballSetMessageTrack::DeletingDestruct_673B70", 0x00673B70, &DeletingDtor<Vt(0x0093DB48), KeyDtor(0x18), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballSetMessageTrack::CreateAt_673BA0", 0x00673BA0, &CreateAt<TrackBase, Vt(0x0093DB48), Key(0x18)>);
DODGEFAC35_REVERSED("DodgeballCameraViewTrack::New_673BD0", 0x00673BD0, &New<0x2c, TrackBase, Vt(0x0093DB60), Then, U8(0x8, 0), U8(0x18, 1), U8(0x19, 1), U8(0x1A, 1), U8(0x1B, 1), U8(0x1C, 1), U8(0x1D, 1), U8(0x1E, 1), U8(0x1F, 1)>);
DODGEFAC35_REVERSED("DodgeballCameraViewTrack::DeletingDestruct_673C20", 0x00673C20, &DeletingDtor<Vt(0x0093DB60), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballCameraViewTrack::CreateAt_673C50", 0x00673C50, &CreateAt<TrackBase, Vt(0x0093DB60)>);
DODGEFAC35_REVERSED("CDodgeballBallLocationCollisionTrack::New_673C70", 0x00673C70, &New<0x34, TrackBase, Vt(0x0093DB78), Then, U8(0x8, 0), F32(0x18, -0.5f), U8(0x2C, 0), F32(0x20, 0.5f), F32(0x24, 0.5f), F32(0x1C, -0.5f), F32(0x28, 0.009999999776482582f)>);
DODGEFAC35_REVERSED("CDodgeballBallLocationCollisionTrack::DeletingDestruct_673CC0", 0x00673CC0, &DeletingDtor<Vt(0x0093DB78), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballBallLocationCollisionTrack::CreateAt_673CF0", 0x00673CF0, &CreateAt<TrackBase, Vt(0x0093DB78)>);
DODGEFAC35_REVERSED("CDodgeballPickupBallTrack::New_673D10", 0x00673D10, &New<0x48, TrackBase, Vt(0x0093DB90), Then, U8(0x8, 0), F32(0x24, 180.0f), U8(0x40, 0), F32(0x28, 180.0f), U8(0x41, 0), I32(0x18, 0xc), F32(0x2C, 3.0f), F32(0x30, 20.0f), One(0x34), One(0x38), One(0x3C), F32(0x1C, -0.10000000149011612f), F32(0x20, -0.10000000149011612f)>);
DODGEFAC35_REVERSED("CDodgeballPickupBallTrack::DeletingDestruct_673D80", 0x00673D80, &DeletingDtor<Vt(0x0093DB90), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballPickupBallTrack::CreateAt_673DB0", 0x00673DB0, &CreateAt<TrackBase, Vt(0x0093DB90)>);
DODGEFAC35_REVERSED("CDodgeballPedMarkerTrack::New_673DD0", 0x00673DD0, &New<0x3c, TrackBase, Vt(0x0093DBA8), Then, One(0x28), U8(0x8, 0), I32(0x18, 0x0), F32(0x2C, 0.019999999552965164f), I32(0x1C, 0xff), I32(0x20, 0xff), Zero(0x30), I32(0x24, 0xff)>);
DODGEFAC35_REVERSED("CDodgeballPedMarkerTrack::DeletingDestruct_673E30", 0x00673E30, &DeletingDtor<Vt(0x0093DBA8), TrackBaseDtor>);
DODGEFAC35_REVERSED("CDodgeballPedMarkerTrack::CreateAt_673E60", 0x00673E60, &CreateAt<TrackBase, Vt(0x0093DBA8)>);
DODGEFAC35_REVERSED("CDodgeballPedFlagCond::New_673E80", 0x00673E80, &New<0xc, CondBase, Vt(0x0093DBC0), Then, I32(0x4, 0x0), U8(0x8, 1)>);
DODGEFAC35_REVERSED("CDodgeballPedFlagCond::DeletingDestruct_673EC0", 0x00673EC0, &DeletingDtor<Vt(0x0093DBC0)>);
DODGEFAC35_REVERSED("CDodgeballPedFlagCond::CreateAt_673EE0", 0x00673EE0, &CreateAt<CondBase, Vt(0x0093DBC0)>);
DODGEFAC35_REVERSED("CDodgeballGameFlagCond::New_673F00", 0x00673F00, &New<0xc, CondBase, Vt(0x0093DBD4), Then, I32(0x4, 0x0), U8(0x8, 1)>);
DODGEFAC35_REVERSED("CDodgeballGameFlagCond::DeletingDestruct_673F40", 0x00673F40, &DeletingDtor<Vt(0x0093DBD4)>);
DODGEFAC35_REVERSED("CDodgeballGameFlagCond::CreateAt_673F60", 0x00673F60, &CreateAt<CondBase, Vt(0x0093DBD4)>);
DODGEFAC35_REVERSED("CDodgeballPedInAreaCond::New_673F80", 0x00673F80, &New<0xc, CondBase, Vt(0x0093DBE8), Then, U8(0x8, 1), I32(0x4, 0x0), U8(0x9, 0)>);
DODGEFAC35_REVERSED("CDodgeballPedInAreaCond::DeletingDestruct_673FC0", 0x00673FC0, &DeletingDtor<Vt(0x0093DBE8)>);
DODGEFAC35_REVERSED("CDodgeballPedInAreaCond::CreateAt_673FE0", 0x00673FE0, &CreateAt<CondBase, Vt(0x0093DBE8)>);
DODGEFAC35_REVERSED("DodgeballBallStateCond::New_674000", 0x00674000, &New<0xc, CondBase, Vt(0x0093DBFC), Then, I32(0x4, 0x0), U8(0x8, 1)>);
DODGEFAC35_REVERSED("DodgeballBallStateCond::DeletingDestruct_674040", 0x00674040, &DeletingDtor<Vt(0x0093DBFC)>);
DODGEFAC35_REVERSED("DodgeballBallStateCond::CreateAt_674060", 0x00674060, &CreateAt<CondBase, Vt(0x0093DBFC)>);
DODGEFAC35_REVERSED("DodgeballGameOverCond::New_674080", 0x00674080, &New<0x4, CondBase, Vt(0x0093DC10), Then>);
DODGEFAC35_REVERSED("DodgeballGameOverCond::DeletingDestruct_6740B0", 0x006740B0, &DeletingDtor<Vt(0x0093DC10)>);
DODGEFAC35_REVERSED("DodgeballGameOverCond::CreateAt_6740D0", 0x006740D0, &CreateAt<CondBase, Vt(0x0093DC10)>);
DODGEFAC35_REVERSED("DodgeballBallLocationCond::New_6740F0", 0x006740F0, &New<0x18, CondBase, Vt(0x0093DC24), Then, U8(0x14, 1), F32(0x4, -0.5f), U8(0x15, 0), F32(0x8, -0.5f), F32(0xC, 0.5f), F32(0x10, 0.5f)>);
DODGEFAC35_REVERSED("DodgeballBallLocationCond::DeletingDestruct_674140", 0x00674140, &DeletingDtor<Vt(0x0093DC24)>);
DODGEFAC35_REVERSED("DodgeballBallLocationCond::CreateAt_674160", 0x00674160, &CreateAt<CondBase, Vt(0x0093DC24)>);
DODGEFAC35_REVERSED("DodgeballPedLocationCond::New_674180", 0x00674180, &New<0x18, CondBase, Vt(0x0093DC38), Then, U8(0x14, 1), F32(0x4, -0.5f), U8(0x15, 0), F32(0x8, -0.5f), F32(0xC, 0.5f), F32(0x10, 0.5f)>);
DODGEFAC35_REVERSED("DodgeballPedLocationCond::DeletingDestruct_6741D0", 0x006741D0, &DeletingDtor<Vt(0x0093DC38)>);
DODGEFAC35_REVERSED("DodgeballPedLocationCond::CreateAt_6741F0", 0x006741F0, &CreateAt<CondBase, Vt(0x0093DC38)>);
DODGEFAC35_REVERSED("DodgeballBallMessageCond::New_674210", 0x00674210, &New<0x8, CondBase, Vt(0x0093DC4C), Key(0x4), Then, ClearName(0x4)>);
DODGEFAC35_REVERSED("DodgeballBallMessageCond::DeletingDestruct_674270", 0x00674270, &DeletingDtor<Vt(0x0093DC4C), KeyDtor(0x4)>);
DODGEFAC35_REVERSED("DodgeballBallMessageCond::CreateAt_6742A0", 0x006742A0, &CreateAt<CondBase, Vt(0x0093DC4C), Key(0x4)>);
DODGEFAC35_REVERSED("DodgeballAttackerDistCond::New_6742D0", 0x006742D0, &New<0xc, CondBase, Vt(0x0093DC60), Then, F32(0x4, 2.0f), I32(0x8, 0x5)>);
DODGEFAC35_REVERSED("DodgeballAttackerDistCond::DeletingDestruct_674320", 0x00674320, &DeletingDtor<Vt(0x0093DC60)>);
DODGEFAC35_REVERSED("DodgeballAttackerDistCond::CreateAt_674340", 0x00674340, &CreateAt<CondBase, Vt(0x0093DC60)>);
DODGEFAC35_REVERSED("DodgeballPlayersInCond::New_674360", 0x00674360, &New<0x10, CondBase, Vt(0x0093DC74), Then, I32(0x4, 0x4), I32(0xC, 0x5)>);
DODGEFAC35_REVERSED("DodgeballPlayersInCond::DeletingDestruct_6743B0", 0x006743B0, &DeletingDtor<Vt(0x0093DC74)>);
DODGEFAC35_REVERSED("DodgeballPlayersInCond::CreateAt_6743D0", 0x006743D0, &CreateAt<CondBase, Vt(0x0093DC74)>);
DODGEFAC35_REVERSED("DodgeballCanPickupCond::New_6743F0", 0x006743F0, &New<0x8, CondBase, Vt(0x0093DC88), Then, F32(0x4, 5.0f)>);
DODGEFAC35_REVERSED("DodgeballCanPickupCond::DeletingDestruct_674430", 0x00674430, &DeletingDtor<Vt(0x0093DC88)>);
DODGEFAC35_REVERSED("DodgeballCanPickupCond::CreateAt_674450", 0x00674450, &CreateAt<CondBase, Vt(0x0093DC88)>);
DODGEFAC35_REVERSED("DodgeballAttackTimeCond::New_674470", 0x00674470, &New<0xc, CondBase, Vt(0x0093DC9C), Then, F32(0x4, 0.5f), I32(0x8, 0x4)>);
DODGEFAC35_REVERSED("DodgeballAttackTimeCond::DeletingDestruct_6744C0", 0x006744C0, &DeletingDtor<Vt(0x0093DC9C)>);
DODGEFAC35_REVERSED("DodgeballAttackTimeCond::CreateAt_6744E0", 0x006744E0, &CreateAt<CondBase, Vt(0x0093DC9C)>);
DODGEFAC35_REVERSED("DodgeballCanPathToCond::New_674500", 0x00674500, &New<0x8, CondBase, Vt(0x0093DCB0), Then, U8(0x4, 0)>);
DODGEFAC35_REVERSED("DodgeballCanPathToCond::DeletingDestruct_674530", 0x00674530, &DeletingDtor<Vt(0x0093DCB0)>);
DODGEFAC35_REVERSED("DodgeballCanPathToCond::CreateAt_674550", 0x00674550, &CreateAt<CondBase, Vt(0x0093DCB0)>);
DODGEFAC35_REVERSED("DodgeballBallCountCond::New_674570", 0x00674570, &New<0xc, CondBase, Vt(0x0093DCC4), Then, I32(0x4, 0x1), I32(0x8, 0x3)>);
DODGEFAC35_REVERSED("DodgeballBallCountCond::CreateAt_6745C0", 0x006745C0, &CreateAt<CondBase, Vt(0x0093DCC4)>);
DODGEFAC35_REVERSED("DodgeballLeadAITrack::New_6762D0", 0x006762D0, &New<0x54, TrackBase, Vt(0x0093DCD8), Zero(0x3C), Zero(0x40), Zero(0x44), Then, U8(0x8, 1)>);
DODGEFAC35_REVERSED("DodgeballSupportAITrack::New_676310", 0x00676310, &New<0x34, TrackBase, Vt(0x0093DCF0), Then, U8(0x8, 1)>);
DODGEFAC35_REVERSED("DodgeballBaseAITrack::DeletingDestruct_676340", 0x00676340, &DeletingDtor<Vt(0x0093D9F8), TrackBaseDtor>);
DODGEFAC35_REVERSED("DodgeballSidelineLeadAITrack::New_676370", 0x00676370, &New<0x40, TrackBase, Vt(0x0093DD08), Then, U8(0x8, 1)>);
DODGEFAC35_REVERSED("DodgeballSidelineAITrack::New_6763A0", 0x006763A0, &New<0x40, TrackBase, Vt(0x0093DD20), Then, Call(0x00681750)>);
DODGEFAC35_REVERSED("DodgeballFaceLeadTrack::New_6763E0", 0x006763E0, &New<0x34, TrackBase, Vt(0x0093DD38), Then, Call(0x00682940)>);
DODGEFAC35_REVERSED("DodgeballCatchBallTrack::New_676420", 0x00676420, &New<0x40, TrackBase, Vt(0x0093DD50), Then, Call(0x00682990)>);
DODGEFAC35_REVERSED("DodgeballGetBallAITrack::New_676460", 0x00676460, &New<0x2c, TrackBase, Vt(0x0093DD68), Then, U8(0x8, 1)>);
DODGEFAC35_REVERSED("DodgeballGotoAITrack::New_676490", 0x00676490, &New<0x34, TrackBase, Vt(0x0093DD80), Then, Call(0x006829F0)>);
DODGEFAC35_REVERSED("DodgeballIdleAITrack::New_6764D0", 0x006764D0, &New<0x2c, TrackBase, Vt(0x0093DD98), Then, Call(0x00682A20)>);
DODGEFAC35_REVERSED("DodgeballBallCountCond::DeletingDestruct_672A10", 0x00672A10, &DeletingDtor<Vt(0x0093DCC4)>);
