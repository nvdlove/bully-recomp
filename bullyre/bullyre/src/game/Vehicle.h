#pragma once

// Vehicles: CVehicle (in CPools::ms_pVehiclePool, 0x8C0 bytes a slot) and its kinds,
// CAutomobile (vtable 0x905D04) and CBike (0x912B24: bikes, and the other things the
// handling calls bikes). CVehicle's vtable is 0x905C0C. A vehicle's seats are a
// CVehicleSeats at +0x1C4.
//
// Only the fields understood so far are named; the base classes (CEntity, CPhysical)
// aren't split out yet: their fields are named here where vehicles use them.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

struct CEntity;
struct CPed;
struct CVehicle;

// (in a namespace: the math types aren't shared yet)
namespace veh {
struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// The game's CMatrix (right, forward, up, pos rows of 4 floats; +0x40 attached)
struct CMatrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(CMatrix) == 0x44);

// A ped's fields the vehicle code uses (CPed isn't laid out here)
inline re::Ptr32<CVehicle>& PedVehicle(CPed* ped) { return *reinterpret_cast<re::Ptr32<CVehicle>*>(reinterpret_cast<uint8_t*>(ped) + 0x1554); }
inline uint8_t& PedInVehicle(CPed* ped) { return *(reinterpret_cast<uint8_t*>(ped) + 0x1558); }        // +0x1558 (in its seat)
inline re::Ptr32<CVehicle>& PedLastVehicle(CPed* ped) { return *reinterpret_cast<re::Ptr32<CVehicle>*>(reinterpret_cast<uint8_t*>(ped) + 0x1560); }
inline uint8_t& PedSeatState(CPed* ped) { return *(reinterpret_cast<uint8_t*>(ped) + 0x1570); }        // +0x1570 (2: see CanBeRemoved)
inline int32_t& PedSeat(CPed* ped) { return *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(ped) + 0x1E10); }  // +0x1E10 -1: none
} // namespace veh

struct CVehicleSeat {
    uint8_t exists;               // +0x0 the vehicle has this seat
    uint8_t unknown1[0x3];
    int32_t unknown04;            // +0x4 (0xF, 0xB, 0x10, 0xC by seat: set by SetUp)
    int32_t door;                 // +0x8 (6 when unset; 2..5 by seat)
    re::Ptr32<CPed> occupant;               // +0xC (registered references)
    re::Ptr32<CPed> unknown10;              // +0x10
    re::Ptr32<CPed> unknown14;              // +0x14
    veh::CVector pos;             // +0x18 (from the model's seat positions; x mirrored for the left seats)
    veh::CVector pos2;            // +0x24

    // 0x4CE400: empty, no one in it (the references let go)
    void Clear();
    // 0x4CE780: cleared, then there, with its two ids
    void Set(int32_t id04, int32_t door);
};
static_assert(offsetof(CVehicleSeat, exists) == 0x0);
static_assert(offsetof(CVehicleSeat, unknown04) == 0x4);
static_assert(offsetof(CVehicleSeat, door) == 0x8);
static_assert(offsetof(CVehicleSeat, occupant) == 0xC);
static_assert(offsetof(CVehicleSeat, unknown10) == 0x10);
static_assert(offsetof(CVehicleSeat, unknown14) == 0x14);
static_assert(offsetof(CVehicleSeat, pos) == 0x18);
static_assert(offsetof(CVehicleSeat, pos2) == 0x24);
static_assert(sizeof(CVehicleSeat) == 0x30);

struct CVehicleSeats {
    re::Ptr32<CVehicle> vehicle;            // +0x0
    int32_t vehicleType;          // +0x4 the vehicle's +0x46C when set up (1: a bike)
    CVehicleSeat seats[4];        // +0x8 driver, front passenger, rear left, rear right
    int32_t numSeats;             // +0xC8

    // 0x4CE7A0: the constructor's body (every seat empty, numSeats 1)
    void Construct();
    // 0x4CE470: for a vehicle (nothing for nullptr); the seats set up when it has its
    // handling
    void Init(CVehicle* vehicle);
    // 0x4CEAA0: which seats there are (a bike only the driver's) and where, from the
    // model's seat positions
    void SetUp();
    // 0x4CE4C0: the seat if it's there, else nullptr
    CVehicleSeat* GetSeat(uint32_t seat);
    // 0x4CE4F0: (a seat past 3 reads the one at -1, before the array)
    uint8_t HasSeat(uint32_t seat);
    // 0x4CE510: someone is in it (or getting in, with `orEntering`)
    uint8_t IsSeatTaken(uint32_t seat, int orEntering);
    // 0x4CE560: who's in it (nullptr: no one, or no such seat)
    CPed* GetOccupant(uint32_t seat);
    // 0x4CE590: puts the ped in (the ped told its seat)
    void SetOccupant(uint32_t seat, CPed* ped);
    // 0x4CE5F0: the front passenger's seat is taken (0 / 1 in all of eax)
    uint32_t HasFrontPassenger();
    // 0x4CE600: SetOccupant for the driver's seat
    void SetDriver(CPed* ped);
    // 0x4CE640
    uint8_t HasAnyOccupant();
    // 0x4CE660
    uint8_t IsOccupied(uint32_t seat);
    // 0x4CE6D0
    uint8_t IsInside(CPed* ped);
    // 0x4CE720: -1: not in it
    int32_t GetSeatIndex(CPed* ped);
    // 0x4CE750: the ped is the seat's +0x14 (a seat past 3: the one at -1)
    uint8_t IsSeatUnknown14(uint32_t seat, CPed* ped);
    // 0x4CE880: the ped out of the seat (its seat forgotten unless it's still getting
    // in there)
    void RemoveFromSeat(uint32_t seat, CPed* ped);
    // 0x4CE900 (and its thunk 0x4CEA60): out of whichever seat
    void Remove(CPed* ped);
    // 0x4CE960: the driver out; the vehicle abandoned (status 4) when that leaves no one
    void RemoveDriver();
    // 0x4CEA70
    uint8_t IsDriver(CPed* ped);
};
static_assert(offsetof(CVehicleSeats, vehicle) == 0x0);
static_assert(offsetof(CVehicleSeats, vehicleType) == 0x4);
static_assert(offsetof(CVehicleSeats, seats) == 0x8);
static_assert(offsetof(CVehicleSeats, numSeats) == 0xC8);
static_assert(sizeof(CVehicleSeats) == 0xCC);

struct CVehicle {
    re::Ptr32<re::Ptr32<void>> vtable;                // +0x0
    veh::CVector placementPos;    // +0x4 the position while there's no matrix
    uint8_t unknown10[0x4];
    re::Ptr32<veh::CMatrix> matrix;         // +0x14 nullptr: none (its +0x30 is the position)
    re::Ptr32<void> rwObject;               // +0x18 its Gamebryo object
    uint32_t entityFlags;         // +0x1C (VehicleSetEntityFlag)
    int32_t unknown20;            // +0x20 (0 while hidden: see SetHidden)
    uint8_t unknown24[0x20];
    int32_t unknown44;            // +0x44
    uint8_t unknown48[0x20];
    int32_t unknown68;            // +0x68
    int32_t unknown6C;            // +0x6C
    int32_t unknown70;            // +0x70
    uint8_t unknown74[0x5C];
    float unknownD0;              // +0xD0 (1 while hidden, else 0: see SetHidden; its health-like float in InflictDamage)
    uint8_t unknownD4[0x34];
    uint8_t typeAndStatus;        // +0x108 low 3 bits the entity type (2: vehicle, 3: ped, 4: object), the rest its status
    uint8_t unknown109[0x5];
    int16_t modelIndex;           // +0x10E
    uint8_t unknown110[0x1];
    uint8_t unknown111;           // +0x111
    uint8_t unknown112[0x2];
    re::Ptr32<void> animData;               // +0x114 (points at +0x47C)
    uint8_t unknown118[0xC];
    veh::CVector moveSpeed;       // +0x124
    veh::CVector turnSpeed;       // +0x130
    uint8_t unknown13C[0x30];
    veh::CVector centreOfMass;    // +0x16C (the handling's +0x1C)
    re::Ptr32<void> collisionList;          // +0x178 (nodes: +8 the entity, +0x10 the next)
    uint8_t unknown17C[0x4];
    uint8_t unknown180;           // +0x180
    uint8_t unknown181[0x1F];
    float damageImpulse;          // +0x1A0
    re::Ptr32<CEntity> damageEntity;        // +0x1A4 (a registered reference)
    veh::CVector damagePos;       // +0x1A8
    uint16_t damagePiece;         // +0x1B4
    uint8_t unknown1B6[0x7];
    uint8_t unknown1BD;           // +0x1BD
    uint8_t unknown1BE[0x6];
    CVehicleSeats seats;          // +0x1C4
    uint8_t createdBy;            // +0x290 1 random, 2 mission, 3 (as random), 4 (see SetCreatedBy)
    uint8_t unknown291[0xB];
    float unknown29C;             // +0x29C
    float steerAngle;             // +0x2A0
    re::Ptr32<uint8_t> handling;            // +0x2A4 (its +0xCC: the flags; 0x70000 the kind of vehicle)
    uint8_t autoPilot;            // +0x2A8 the autopilot starts here (its constructor 0x44A070)
    uint8_t unknown2A9[0x23];
    uint32_t autoPilotTimer;      // +0x2CC
    uint8_t unknown2D0[0x9];
    uint8_t drivingStyle;         // +0x2D9
    uint8_t mission;              // +0x2DA
    uint8_t unknown2DB;           // +0x2DB
    uint8_t unknown2DC[0x4];
    float cruiseSpeedNow;         // +0x2E0
    int8_t cruiseSpeed;           // +0x2E4
    uint8_t unknown2E5[0x7];
    uint8_t autoPilotFlags;       // +0x2EC
    uint8_t unknown2ED;           // +0x2ED
    uint8_t unknown2EE[0x3A];
    re::Ptr32<CEntity> autoPilotTarget;     // +0x328 (a registered reference)
    uint8_t unknown32C[0x8];
    uint8_t colour1;              // +0x334
    uint8_t colour2;              // +0x335
    uint8_t extra1;               // +0x336 the model's extras picked for it (CVehicleModelInfo's last pick)
    uint8_t extra2;               // +0x337
    int16_t unknown338;           // +0x338
    uint8_t flags33A;             // +0x33A 0x20 fading out, 0x40 takes damage
    uint8_t flags33B;             // +0x33B
    uint8_t flags33C;             // +0x33C 4 out of the world's bounds, 8 (status), 0x10 hidden locked
    uint8_t unknown33D[0x3];
    float health;                 // +0x340
    uint8_t unknown344;           // +0x344
    uint8_t unknown345[0x3];
    int32_t unknown348;           // +0x348
    float unknown34C;             // +0x34C
    float unknown350;             // +0x350
    int32_t unknown354;           // +0x354
    re::Ptr32<CEntity> owner;               // +0x358 (a registered reference)
    float unknown35C;             // +0x35C
    float unknown360;             // +0x360
    float accelerationMult;       // +0x364
    int32_t unknown368;           // +0x368
    uint8_t unknown36C;           // +0x36C
    uint8_t siren;                // +0x36D
    uint8_t unknown36E;           // +0x36E
    uint8_t unknown36F[0x1];
    int32_t unknown370;           // +0x370
    uint8_t unknown374[0x64];
    int32_t unknown3D8;           // +0x3D8
    uint8_t unknown3DC;           // +0x3DC
    uint8_t unknown3DD[0x67];
    int32_t unknown444;           // +0x444
    uint8_t unknown448;           // +0x448
    uint8_t unknown449[0x3];
    uint8_t unknown44C;           // +0x44C (a stat counted once: see 0x4CC4A0)
    uint8_t unknown44D;           // +0x44D (a stat counted once)
    uint8_t unknown44E[0x2];
    re::Ptr32<CEntity> unknown450;          // +0x450 (a registered reference)
    uint8_t unknown454;           // +0x454
    uint8_t unknown455[0x3];
    veh::CVector unknown458;      // +0x458
    uint8_t unknown464;           // +0x464
    uint8_t unknown465[0x3];
    re::Ptr32<void> unknown468;             // +0x468 (an animation on its animData)
    int32_t vehicleType;          // +0x46C 0 an automobile, 1 a bike
    float unknown470;             // +0x470
    uint8_t unknown474[0x4];
    int32_t unknown478;           // +0x478
    uint8_t animDataStorage;      // +0x47C (what +0x114 points at; its constructor 0x6BA240)
    uint8_t unknown47D[0x103];

    // 0x4CB730: the constructor (CPhysical's first; thiscall, returns this)
    CVehicle* Construct(uint8_t createdBy);
    // 0x4CD130: the destructor's body (CPhysical's last)
    void Destruct();
    // 0x4CB5B0: its state reset (the argument is unused)
    void Reset(int unused);
    // 0x4CAEE0: from the handling flags: 1 car, 2 (0x10000), 3 (0x20000), 4 (0x40000); 0 else
    int32_t GetHandlingKind();
    // 0x4CAF30: which kind of vehicle it is, by its kind and its model (0..14)
    int32_t* GetVehicleClass(int32_t* out);
    // 0x4CB1F0: a model with a siren
    uint8_t HasSirenModel();
    // 0x4CB210: +0x354 is 0, 1 or 7
    uint8_t IsUnknown354Normal();
    // 0x4CB230: the population told when it stops or starts being a random vehicle
    void SetCreatedBy(int createdBy);
    // 0x4CB480: its status (2: with cruise speed, else 3), then the autopilot's 0x418170
    void SetUnknownAutoPilot(int arg, int flag, int arg3);
    // 0x4CB4E0: a stat counted, once (automobiles)
    void CountStat16();
    // 0x4CB510 / 0x4CB520
    float GetUnknown470();
    int32_t GetUnknown478();
    // 0x4CB530: hidden or shown, its occupants too (unless locked by flags33C 0x10)
    void SetHidden(int hidden);
    // 0x4CB7F0: a random vehicle nobody (player or seat state 2) is using
    uint8_t CanBeRemoved();
    // 0x4CC270: fades its alpha out (flags33A 0x20) or back in (at once when close)
    void UpdateFade();

    // the virtuals
    // vf6 0x4CB2F0
    void SetStatus(int status);
    // vf8 0x4CB190: the model set (the population told), then the extras the model
    // picked
    void SetModelIndex(int model, int arg);
    // vf54 0x4CCBB0: minus the collision box's min z
    float GetHeightAboveRoad();
    // vf57 0x4CB1E0
    float GetUnknownVf57();
    // vf58 0x4CB400: an animation on the driver's animation data (both ids given)
    void PlayDriverAnim(int group, int anim);
};
static_assert(offsetof(CVehicle, vtable) == 0x0);
static_assert(offsetof(CVehicle, placementPos) == 0x4);
static_assert(offsetof(CVehicle, matrix) == 0x14);
static_assert(offsetof(CVehicle, rwObject) == 0x18);
static_assert(offsetof(CVehicle, entityFlags) == 0x1C);
static_assert(offsetof(CVehicle, unknown20) == 0x20);
static_assert(offsetof(CVehicle, unknown44) == 0x44);
static_assert(offsetof(CVehicle, unknown68) == 0x68);
static_assert(offsetof(CVehicle, unknown6C) == 0x6C);
static_assert(offsetof(CVehicle, unknown70) == 0x70);
static_assert(offsetof(CVehicle, unknownD0) == 0xD0);
static_assert(offsetof(CVehicle, typeAndStatus) == 0x108);
static_assert(offsetof(CVehicle, modelIndex) == 0x10E);
static_assert(offsetof(CVehicle, unknown111) == 0x111);
static_assert(offsetof(CVehicle, animData) == 0x114);
static_assert(offsetof(CVehicle, moveSpeed) == 0x124);
static_assert(offsetof(CVehicle, turnSpeed) == 0x130);
static_assert(offsetof(CVehicle, centreOfMass) == 0x16C);
static_assert(offsetof(CVehicle, collisionList) == 0x178);
static_assert(offsetof(CVehicle, unknown180) == 0x180);
static_assert(offsetof(CVehicle, damageImpulse) == 0x1A0);
static_assert(offsetof(CVehicle, damageEntity) == 0x1A4);
static_assert(offsetof(CVehicle, damagePos) == 0x1A8);
static_assert(offsetof(CVehicle, damagePiece) == 0x1B4);
static_assert(offsetof(CVehicle, unknown1BD) == 0x1BD);
static_assert(offsetof(CVehicle, seats) == 0x1C4);
static_assert(offsetof(CVehicle, createdBy) == 0x290);
static_assert(offsetof(CVehicle, unknown29C) == 0x29C);
static_assert(offsetof(CVehicle, steerAngle) == 0x2A0);
static_assert(offsetof(CVehicle, handling) == 0x2A4);
static_assert(offsetof(CVehicle, autoPilot) == 0x2A8);
static_assert(offsetof(CVehicle, autoPilotTimer) == 0x2CC);
static_assert(offsetof(CVehicle, drivingStyle) == 0x2D9);
static_assert(offsetof(CVehicle, mission) == 0x2DA);
static_assert(offsetof(CVehicle, unknown2DB) == 0x2DB);
static_assert(offsetof(CVehicle, cruiseSpeedNow) == 0x2E0);
static_assert(offsetof(CVehicle, cruiseSpeed) == 0x2E4);
static_assert(offsetof(CVehicle, autoPilotFlags) == 0x2EC);
static_assert(offsetof(CVehicle, unknown2ED) == 0x2ED);
static_assert(offsetof(CVehicle, autoPilotTarget) == 0x328);
static_assert(offsetof(CVehicle, colour1) == 0x334);
static_assert(offsetof(CVehicle, colour2) == 0x335);
static_assert(offsetof(CVehicle, extra1) == 0x336);
static_assert(offsetof(CVehicle, extra2) == 0x337);
static_assert(offsetof(CVehicle, unknown338) == 0x338);
static_assert(offsetof(CVehicle, flags33A) == 0x33A);
static_assert(offsetof(CVehicle, flags33B) == 0x33B);
static_assert(offsetof(CVehicle, flags33C) == 0x33C);
static_assert(offsetof(CVehicle, health) == 0x340);
static_assert(offsetof(CVehicle, unknown344) == 0x344);
static_assert(offsetof(CVehicle, unknown348) == 0x348);
static_assert(offsetof(CVehicle, unknown34C) == 0x34C);
static_assert(offsetof(CVehicle, unknown350) == 0x350);
static_assert(offsetof(CVehicle, unknown354) == 0x354);
static_assert(offsetof(CVehicle, owner) == 0x358);
static_assert(offsetof(CVehicle, unknown35C) == 0x35C);
static_assert(offsetof(CVehicle, unknown360) == 0x360);
static_assert(offsetof(CVehicle, accelerationMult) == 0x364);
static_assert(offsetof(CVehicle, unknown368) == 0x368);
static_assert(offsetof(CVehicle, unknown36C) == 0x36C);
static_assert(offsetof(CVehicle, siren) == 0x36D);
static_assert(offsetof(CVehicle, unknown36E) == 0x36E);
static_assert(offsetof(CVehicle, unknown370) == 0x370);
static_assert(offsetof(CVehicle, unknown3D8) == 0x3D8);
static_assert(offsetof(CVehicle, unknown3DC) == 0x3DC);
static_assert(offsetof(CVehicle, unknown444) == 0x444);
static_assert(offsetof(CVehicle, unknown448) == 0x448);
static_assert(offsetof(CVehicle, unknown44C) == 0x44C);
static_assert(offsetof(CVehicle, unknown44D) == 0x44D);
static_assert(offsetof(CVehicle, unknown450) == 0x450);
static_assert(offsetof(CVehicle, unknown454) == 0x454);
static_assert(offsetof(CVehicle, unknown458) == 0x458);
static_assert(offsetof(CVehicle, unknown464) == 0x464);
static_assert(offsetof(CVehicle, unknown468) == 0x468);
static_assert(offsetof(CVehicle, vehicleType) == 0x46C);
static_assert(offsetof(CVehicle, unknown470) == 0x470);
static_assert(offsetof(CVehicle, unknown478) == 0x478);
static_assert(offsetof(CVehicle, animDataStorage) == 0x47C);
static_assert(sizeof(CVehicle) == 0x580);


// (CAutomobile and CBike are sized as the pool's slot: their own sizes aren't known)
struct CAutomobile : CVehicle {
    uint8_t unknown580[0xF4];
    re::Ptr32<void> frames[20];             // +0x674 (found by FindFrames)
    uint8_t unknown6C4[0x178];
    re::Ptr32<CEntity> entities[4];         // +0x83C
    uint8_t unknown84C[0x24];
    uint8_t effects[32];          // +0x870 four, 8 bytes each (0x668BA0 resets one)
    uint32_t effectIds[4];        // +0x890
    uint8_t unknown8A0[0x20];

    // 0x4A8410: the hardest hit so far kept (by whom, where, which piece)
    void SetDamage(int piece, float impulse, CEntity* entity, const veh::CVector* pos);
    // 0x4A84E0: its four effects (+0x870) reset, their ids 0xFFFF
    void ResetEffects();
    // 0x4AAAA0: its frames found (0x510B40 fills them from its Gamebryo object)
    void FindFrames();

    // the virtuals
    // vf8 0x4ADFC0: the model set, then its frames, vf39 with its matrix, its effects
    void SetModelIndex(int model, int arg);
    // vf44 0x4A8520
    uint8_t HasFrame(int frame);
    // vf48 0x4A8940: the entity forgotten wherever it's one of its four
    void ForgetEntity(CEntity* entity);
};
static_assert(offsetof(CAutomobile, frames) == 0x674);
static_assert(offsetof(CAutomobile, entities) == 0x83C);
static_assert(offsetof(CAutomobile, effects) == 0x870);
static_assert(offsetof(CAutomobile, effectIds) == 0x890);
static_assert(sizeof(CAutomobile) == 0x8C0);

struct CBike : CVehicle {
    re::Ptr32<void> frames[8];              // +0x580
    uint8_t unknown5A0[0xF0];
    float unknown690;             // +0x690
    uint8_t unknown694[0x50];
    re::Ptr32<CEntity> entities[2];         // +0x6E4
    uint8_t unknown6EC[0x38];
    uint32_t unknown724;          // +0x724 a time
    uint32_t unknown728;          // +0x728 the one before
    uint8_t unknown72C[0x4C];
    float unknown778;             // +0x778
    float jump;                   // +0x77C (BikeJump)
    uint8_t unknown780[0x140];

    // 0x4B9C20
    void SetJump(float jump);
    // 0x4BCCA0: how long since +0x724's time
    int32_t GetTimeSince724();
    // 0x4BCCB0: +0x724's time moved on to now (the old one kept), +0x778 increased
    void AddUnknown778(float amount);
    // 0x4C0EC0: its collision's +0x38's +0x1E: 8 with a driver, else 5
    void UpdateColSurface();

    // the virtuals
    // vf0 0x4BD830: the deleting destructor (its body 0x4B9E80)
    CBike* Destroy(uint8_t flags);
    // vf7 0x4B9720
    void SetIsStatic(int isStatic);
    // vf34 0x4B9FF0: someone's in the driver's seat (0 / 1 in all of eax)
    uint32_t HasDriver();
    // vf43 0x4BA020: a frame's position (left as it is when there's no such frame)
    void GetFramePosition(int frame, veh::CVector* out);
    // vf44 0x4B95A0
    uint8_t HasFrame(int frame);
    // vf48 0x4B9CB0: the entity forgotten wherever it's one of its two
    void ForgetEntity(CEntity* entity);
    // vf54 0x4BA010
    float GetUnknown690();
};
static_assert(offsetof(CBike, frames) == 0x580);
static_assert(offsetof(CBike, unknown690) == 0x690);
static_assert(offsetof(CBike, entities) == 0x6E4);
static_assert(offsetof(CBike, unknown724) == 0x724);
static_assert(offsetof(CBike, unknown728) == 0x728);
static_assert(offsetof(CBike, unknown778) == 0x778);
static_assert(offsetof(CBike, jump) == 0x77C);
static_assert(sizeof(CBike) == 0x8C0);


// CVehicleModelInfo's whole layout, as the vehicle code sees it (ModelInfo.h's
// CVehicleModelInfo is the same object with fewer fields named; vtable 0x918C94).
struct CVehicleModel {
    re::Ptr32<re::Ptr32<void>> vtable;                // +0x0
    uint8_t unknown4[0x4];
    uint8_t type;                 // +0x8
    uint8_t unknown9[0x3];
    re::Ptr32<uint8_t> colModel;            // +0xC
    uint8_t unknown10[0x10];
    re::Ptr32<void> clump;                  // +0x20
    uint8_t unknown24[0x24];
    re::Ptr32<void> animGroups;             // +0x48
    uint8_t lastColour1;          // +0x4C
    uint8_t lastColour2;          // +0x4D
    uint8_t unknown4E[0x2];
    int32_t vehicleClass;         // +0x50 (indexes the frame lists at 0xA95AC8)
    uint8_t unknown54[0x4];
    float unknown58;              // +0x58
    int16_t wheelModel;           // +0x5C -1: none
    uint8_t unknown5E[0x2];
    int8_t unknown60;             // +0x60
    int8_t numExtras;             // +0x61
    uint8_t unknown62[0x2];
    veh::CVector dummyPos[10];    // +0x64 (seats: +0x7C front, +0x88 rear, +0x94 / +0xA0 their second positions)
    uint32_t colourFlags;         // +0xDC weather colour rules (nibbles; see GetAllowedColours)
    float unknownE0;              // +0xE0
    re::Ptr32<void> unknownE4[27];          // +0xE4
    re::Ptr32<void> unknown150[23];         // +0x150
    uint8_t colours1[8];          // +0x1AC
    uint8_t colours2[8];          // +0x1B4
    uint8_t numColours;           // +0x1BC
    uint8_t lastColourIndex;      // +0x1BD
    uint8_t currentColour1;       // +0x1BE
    uint8_t currentColour2;       // +0x1BF
    re::Ptr32<void> extras[6];              // +0x1C0
    int32_t animFile2;            // +0x1D8 vf8
    int32_t animGroup;            // +0x1DC -1: none

    // 0x535110: the constructor (CClumpModelInfo's with type 7; returns this)
    CVehicleModel* Construct();
    // 0x534B40: the front seat's second position
    veh::CVector* GetFrontSeatPos2();
    // 0x534B00: its second animation file by name ("null": left as it is)
    void SetAnimFile2(const char* name);
    // 0x534CD0: a frame's position (by the frame table at 0xA95AD0)
    void GetFramePosition(int frame, veh::CVector* out);
    // 0x534ED0 / 0x534F90: a colour for a new vehicle (-1: its own); a forced one
    // (0xA95810 / 0xA95811) once
    int32_t ChooseColour1();
    int32_t ChooseColour2();
    // 0x535060: another pair than the last (8 tries), remembered as the last
    void AvoidLastColours(uint8_t* colour1, uint8_t* colour2);
    // 0x5350F0: +0x58 plus the global 0xA95814
    float GetUnknown58();
    // 0x535D90: the next of its colour pairs (not the player's vehicle's, when it's
    // this model)
    void ChooseVehicleColour(uint8_t* colour1, uint8_t* colour2);

    // the virtuals
    // vf0 0x535380: the deleting destructor (CClumpModelInfo's body, then freed)
    CVehicleModel* Destroy(uint8_t flags);
    // vf1 0x534A30: the name, and its animation group ("models\vehicles\<name>", else
    // the default one's)
    void SetName(const char* name);
    // vf3 0x534B50: its extras' objects too
    void DeleteObjects();
    // vf8 0x535370
    int32_t GetDependency8();
    // vf13 0x535E70: its scene graph set: frames, extras, what's left
    void SetClump(void* clump, int arg);
};
static_assert(offsetof(CVehicleModel, vtable) == 0x0);
static_assert(offsetof(CVehicleModel, type) == 0x8);
static_assert(offsetof(CVehicleModel, colModel) == 0xC);
static_assert(offsetof(CVehicleModel, clump) == 0x20);
static_assert(offsetof(CVehicleModel, animGroups) == 0x48);
static_assert(offsetof(CVehicleModel, lastColour1) == 0x4C);
static_assert(offsetof(CVehicleModel, lastColour2) == 0x4D);
static_assert(offsetof(CVehicleModel, vehicleClass) == 0x50);
static_assert(offsetof(CVehicleModel, unknown58) == 0x58);
static_assert(offsetof(CVehicleModel, wheelModel) == 0x5C);
static_assert(offsetof(CVehicleModel, unknown60) == 0x60);
static_assert(offsetof(CVehicleModel, numExtras) == 0x61);
static_assert(offsetof(CVehicleModel, dummyPos) == 0x64);
static_assert(offsetof(CVehicleModel, colourFlags) == 0xDC);
static_assert(offsetof(CVehicleModel, unknownE0) == 0xE0);
static_assert(offsetof(CVehicleModel, unknownE4) == 0xE4);
static_assert(offsetof(CVehicleModel, unknown150) == 0x150);
static_assert(offsetof(CVehicleModel, colours1) == 0x1AC);
static_assert(offsetof(CVehicleModel, colours2) == 0x1B4);
static_assert(offsetof(CVehicleModel, numColours) == 0x1BC);
static_assert(offsetof(CVehicleModel, lastColourIndex) == 0x1BD);
static_assert(offsetof(CVehicleModel, currentColour1) == 0x1BE);
static_assert(offsetof(CVehicleModel, currentColour2) == 0x1BF);
static_assert(offsetof(CVehicleModel, extras) == 0x1C0);
static_assert(offsetof(CVehicleModel, animFile2) == 0x1D8);
static_assert(offsetof(CVehicleModel, animGroup) == 0x1DC);
static_assert(sizeof(CVehicleModel) == 0x1E0);


// Not reversed yet: called where they are
namespace veh {
// CEntity's registered pointers (the entity clears them when it goes)
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
// (thiscall on the pointer): drops the old one's registration, registers the new one
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<CEntity>* ref, CEntity* entity);
}
