// The Butes objects' factories 0x64FA20 registers (ActionTracks::RegisterButes,
// TrackRegistryInit.cpp) that no other pass had: each one's +4 New (cdecl: allocated,
// built, then its fields' defaults; the call after Then also runs on a failed allocation,
// at null, as the game's do) and five +8 CreateAt (cdecl: the constructor on the memory
// given, null: none). None is in Ghidra's function list: only the table refers to them.
// The shapes are in ActionFactories.h.

#include <cstdint>

#include "game/ActionFactories.h"

using namespace actfac;

// (variadic: the template arguments' commas)
#define BUTESFAC_REVERSED(name, address, ...)     REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

// CreateAt (cdecl, the memory)
BUTESFAC_REVERSED("ProjectileButes::CreateAt", 0x00630410, &CreateVia<0x00630350>);
BUTESFAC_REVERSED("ObjectButes::CreateAt", 0x00630590, &CreateVia<0x006304D0>);
BUTESFAC_REVERSED("CinematicShotGroupButes::CreateAt", 0x006306D0, &CreateVia<0x00630640>);
BUTESFAC_REVERSED("PropButes::CreateAt", 0x00630820, &CreateVia<0x00630770>);
BUTESFAC_REVERSED("PickupListButes::CreateAt", 0x006308E0, &CreateVia<0x00630870>);

// New (cdecl)
BUTESFAC_REVERSED("CoasterCamButes::New", 0x0063C8E0, &New<0x88, TrackBase, Vt(0x00934480), Then, Call(0x007286E0)>);
BUTESFAC_REVERSED("VehicleCamButes::New", 0x0063C920, &New<0x88, TrackBase, Vt(0x00914740), Then, Call(0x007287A0)>);
BUTESFAC_REVERSED("FollowCam3Butes::New", 0x0063C960, &New<0xF8, TrackBase, Vt(0x00914580), Then, Call(0x00728880)>);
BUTESFAC_REVERSED("DodgeballCamButes::New", 0x0063C9A0, &New<0x40, TrackBase, Vt(0x00934498), Then, Call(0x00728A60)>);
BUTESFAC_REVERSED("DodgeballPhysicsButes::New", 0x0063C9E0, &New<0x54, TrackBase, Vt(0x009344B0), Then, Call(0x00728AB0)>);
BUTESFAC_REVERSED("DodgeballAIButes::New", 0x0063CA20, &New<0xBC, TrackBase, Vt(0x009344C8), Then, Call(0x00728B40)>);
BUTESFAC_REVERSED("DodgeballAIRampButes::New", 0x0063CA60, &New<0x28, TrackBase, Vt(0x009344E0), Then, Call(0x00728C50)>);
BUTESFAC_REVERSED("DodgeballGameButes::New", 0x0063CAA0, &New<0x5C, TrackBase, Vt(0x009344F8), Then, Call(0x00728C70)>);
BUTESFAC_REVERSED("AIButes::New", 0x0063CAE0, &New<0xA8, TrackBase, Vt(0x00934510), Then, Call(0x00728CD0)>);
BUTESFAC_REVERSED("FightCollisionButes::New", 0x0063CB20, &New<0x48, TrackBase, Vt(0x00934528), Then, Call(0x00728DE0)>);
BUTESFAC_REVERSED("ButtonTriggerButes::New", 0x0063CB60, &New<0x28, TrackBase, Vt(0x00934540), Then, Call(0x00728E30)>);
BUTESFAC_REVERSED("TimePeriodButes::New", 0x0063CBA0, &New<0x38, TrackBase, Vt(0x00934558), Key(0x18), Key(0x1C), Key(0x20), Then, Call(0x00728E50)>);
BUTESFAC_REVERSED("TimeCycleButes::New", 0x0063CBF0, &New<0x2C, TrackBase, Vt(0x00934570), Key(0x1C), Key(0x20), Key(0x24), Key(0x28), Then, Call(0x00728F00)>);
BUTESFAC_REVERSED("DodgeballArenaButes::New", 0x0063CC50, &New<0x34, TrackBase, Vt(0x00934588), Key(0x18), Key(0x1C), Key(0x20), Then, Call(0x00728FD0)>);
BUTESFAC_REVERSED("DebugPlayerButes::New", 0x0063CCA0, &New<0x30, TrackBase, Vt(0x009345A0), Then, Call(0x00729080)>);
BUTESFAC_REVERSED("PlayerButes::New", 0x0063CCE0, &New<0x124, TrackBase, Vt(0x009345B8), Key(0xFC), Key(0x10C), Key(0x114), Key(0x11C), Then, Call(0x007290D0)>);
BUTESFAC_REVERSED("FrisbeeButes::New", 0x0063CD50, &New<0x40, TrackBase, Vt(0x009345D0), Then, Call(0x00729380)>);
BUTESFAC_REVERSED("FirstPersonCamButes::New", 0x0063CD90, &New<0x3C, TrackBase, Vt(0x009345E8), Then, Call(0x007293E0)>);
BUTESFAC_REVERSED("OffsetCamButes::New", 0x0063CDD0, &New<0x9C, TrackBase, Vt(0x00934600), Key(0x58), Key(0x5C), Key(0x64), Key(0x80), Then, Call(0x00729430)>);
BUTESFAC_REVERSED("ProjectileButes::New", 0x0063CE30, &NewVia<0x158, 0x00630350, 0x0072B910>);
BUTESFAC_REVERSED("CrosshairButes::New", 0x0063CE70, &New<0x44, TrackBase, Vt(0x00934630), Key(0x18), Then, Call(0x007295D0)>);
BUTESFAC_REVERSED("ActionMapButes::New", 0x0063CEB0, &New<0x68, TrackBase, Vt(0x00934648), Then, Call(0x00729630)>);
BUTESFAC_REVERSED("PickupButes::New", 0x0063CEF0, &New<0x30, TrackBase, Vt(0x009053B4), Then, Call(0x00729670)>);
BUTESFAC_REVERSED("ObjectButes::New", 0x0063CF30, &NewVia<0xA4, 0x006304D0, 0x007296A0>);
BUTESFAC_REVERSED("CinematicButes::New", 0x0063CF70, &New<0x5C, TrackBase, Vt(0x00934678), Then, Call(0x00729910)>);
BUTESFAC_REVERSED("MenuButes::New", 0x0063CFB0, &New<0x8C, TrackBase, Vt(0x00934690), Then, Call(0x00729980)>);
BUTESFAC_REVERSED("CinematicShotButes::New", 0x0063CFF0, &New<0x54, TrackBase, Vt(0x00914398), Then, Call(0x00729A30)>);
BUTESFAC_REVERSED("CinematicShotGroupButes::New", 0x0063D030, &NewVia<0x40, 0x00630640, 0x00729A70>);
BUTESFAC_REVERSED("StimulusButes::New", 0x0063D060, &New<0x38, TrackBase, Vt(0x009346C0), Then, Call(0x00729C40)>);
BUTESFAC_REVERSED("PunishmentButes::New", 0x0063D0A0, &New<0x30, TrackBase, Vt(0x009346D8), Key(0x2C), Then, Call(0x00729C70)>);
BUTESFAC_REVERSED("PropButes::New", 0x0063D0E0, &NewVia<0x90, 0x00630770, 0x00729CC0>);
BUTESFAC_REVERSED("PropExtraButes::New", 0x0063D120, &New<0x44, TrackBase, Vt(0x00934708), Then, Call(0x00729E50)>);
BUTESFAC_REVERSED("PickupListButes::New", 0x0063D160, &NewVia<0x50, 0x00630870, 0x00729E80>);
BUTESFAC_REVERSED("BikeButes::New", 0x0063D190, &New<0x8C, TrackBase, Vt(0x00934738), Then, Call(0x00729FE0)>);
BUTESFAC_REVERSED("PhotoButes::New", 0x0063D1D0, &New<0x88, TrackBase, Vt(0x00934750), Then, Call(0x0072A0B0)>);
BUTESFAC_REVERSED("TaggingButes::New", 0x0063D210, &New<0x40, TrackBase, Vt(0x00934768), Then, Call(0x0072A150)>);
BUTESFAC_REVERSED("SurfaceSoundRangeButes::New", 0x0063D250, &New<0x120, TrackBase, Vt(0x00934780), Then, Call(0x0072A1B0)>);
BUTESFAC_REVERSED("AreaTransitionButes::New", 0x0063D290, &New<0x28, TrackBase, Vt(0x00934798), Then, Call(0x0072A320)>);
BUTESFAC_REVERSED("TrafficButes::New", 0x0063D2D0, &New<0x28, TrackBase, Vt(0x009347B0), Then, Call(0x0072A350)>);
BUTESFAC_REVERSED("PlayerAwarenessButes::New", 0x0063D310, &New<0x38, TrackBase, Vt(0x009347C8), Then, Call(0x0072A380)>);
BUTESFAC_REVERSED("StoreCamButes::New", 0x0063D350, &New<0x34, TrackBase, Vt(0x009347E0), Then, Call(0x0072A3C0)>);
BUTESFAC_REVERSED("SoccerPButes::New", 0x0063D390, &New<0x5C, TrackBase, Vt(0x009347F8), Then, Call(0x0072A3F0)>);
BUTESFAC_REVERSED("RadarButes::New", 0x0063D3D0, &New<0x30, TrackBase, Vt(0x00934810), Then, Call(0x0072A460)>);
BUTESFAC_REVERSED("ArmWrestlingButes::New", 0x0063D410, &New<0x28, TrackBase, Vt(0x00934828), Then, Call(0x0072A490)>);
BUTESFAC_REVERSED("MiscGameButes::New", 0x0063D450, &New<0x1C, TrackBase, Vt(0x00934840), Then, Call(0x0072A4B0)>);
BUTESFAC_REVERSED("ArcadeRaceCameraButes::New", 0x0063D490, &New<0x24, TrackBase, Vt(0x00934858), Then, Call(0x0072A4C0)>);
BUTESFAC_REVERSED("GlobalSpineControlButes::New", 0x0063D4D0, &New<0x7C, TrackBase, Vt(0x00934870), Then, Call(0x0072A4E0)>);
BUTESFAC_REVERSED("SpineControlWeightButes::New", 0x0063D510, &New<0x48, TrackBase, Vt(0x00934888), Then, Call(0x0072A580)>);
BUTESFAC_REVERSED("FightCamButes::New", 0x0064A6E0, &New<0xD0, TrackBase, Vt(0x00937FCC), Then, Call(0x00728570)>);
