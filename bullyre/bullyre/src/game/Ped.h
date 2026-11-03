#pragma once

// Peds: CPed (0x1F44 bytes, in CPools::ms_pPedPool), its objectives (what the AI is
// told to do: move to, face, attack, flee, ...) and the queue they wait in.
//
// Objectives are refcounted (+0xC): the queue holds a reference to each of its own,
// and one drops to nothing through its deleting destructor (vtable[0], flags 1).
// Each objective class has an id assigned on first use (Objective::NextClassId), for
// its IsA (vtable[1]).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

struct CEntity;
struct CVehicle;
struct CStimulus;
struct CPed;

namespace ped {
struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// A handle on a node of an action tree (4 bytes; 0x5D80E0 IsValid, 0x5D9C50 its
// release, used as its destructor)
struct ActionHandle {
    re::Ptr32<void> node;
};
static_assert(sizeof(ActionHandle) == 4);

// An entity pointer the entity knows about (CEntity::RegisterReference), with a value
// beside it (0x471300 sets both)
struct EntityRef {
    re::Ptr32<CEntity> entity;
    int32_t value;
};
static_assert(sizeof(EntityRef) == 8);
} // namespace ped

// ---------------------------------------------------------------------------------
// Objectives

// The types (+8)
enum eObjectiveType : int32_t {
    OBJECTIVE_MOVE_TO_POS = 0,
    OBJECTIVE_MOVE_TO_ENTITY = 1,        // (and FollowEntity)
    OBJECTIVE_FACE_HEADING = 2,
    OBJECTIVE_FACE_POSITION = 3,
    OBJECTIVE_FACE_ENTITY = 4,
    OBJECTIVE_ATTACK_PED = 7,
    OBJECTIVE_BREAK_PROP = 8,
    OBJECTIVE_ATTACK_VEHICLE = 0xB,
    OBJECTIVE_FLEE_ON_PATH_ON_BIKE = 0xD,
    OBJECTIVE_COVER = 0x10,
    OBJECTIVE_FOLLOW_FOCUS = 0x11,
    OBJECTIVE_FLEE_ON_ROAD_ON_BIKE = 0x12,
    OBJECTIVE_PLAY_CATCH = 0x13,
    OBJECTIVE_PASS_BALL = 0x14,
    OBJECTIVE_RECEIVE_BALL = 0x15,
    OBJECTIVE_MOVE_TO_STIMULUS = 0x18,
    OBJECTIVE_USE_PROP = 0x1F,
    OBJECTIVE_SLINGER = 0x20,
};

struct Objective {
    re::Ptr32<re::Ptr32<void>> vtable;      // +0x00 0x90C9C0 (the base's)
    re::Ptr32<CPed> ped;          // +0x04
    int32_t type;       // +0x08 eObjectiveType
    int32_t numRefs;    // +0x0C

    // The class ids: 0xBF3830 is the next one to give out
    static int16_t ClassId(uint32_t& guard, int16_t& id);

    // vf0 0x471630: the (deleting) destructor
    Objective* Destroy(uint8_t flags);
};
static_assert(sizeof(Objective) == 0x10);

// MoveToPos / MoveToStimulus / MoveToEntity share this start
struct MoveObjective : Objective {
    float moveSpeed;          // +0x10 (0.3 for a stimulus)
    float unknown14;          // +0x14
    uint8_t unknown18;        // +0x18
    uint8_t pad19[3];
    ped::ActionHandle action; // +0x1C the move it plays
};
static_assert(sizeof(MoveObjective) == 0x20);

struct MoveToPosObjective : MoveObjective {
    ped::CVector target;      // +0x20
    float radius;             // +0x2C (reached within radius + 0.5)
    uint8_t stopWhenThere;    // +0x30
    uint8_t pad31[3];

    // 0x4705B0 (thiscall)
    MoveToPosObjective* Construct(CPed* ped, const ped::CVector* target, float speed, float radius,
                                  int stopWhenThere, int flag18);
    // vf0 0x471590
    MoveToPosObjective* Destroy(uint8_t flags);
};
static_assert(sizeof(MoveToPosObjective) == 0x34);

struct MoveToStimulusObjective : MoveToPosObjective {
    re::Ptr32<void> stimulusInfo;       // +0x34 (0x4A5CF0 of the stimulus)

    // 0x4707D0
    MoveToStimulusObjective* Construct(CPed* ped, CStimulus* stimulus);
};
static_assert(sizeof(MoveToStimulusObjective) == 0x38);

struct MoveToEntityObjective : MoveObjective {
    re::Ptr32<CEntity> target;          // +0x20 (not a registered reference)
    float radius;             // +0x24
    uint8_t stopWhenThere;    // +0x28
    uint8_t pad29[3];

    // 0x4708F0
    MoveToEntityObjective* Construct(CPed* ped, CEntity* target, float speed, float radius, int stopWhenThere,
                                     int flag18);
    // vf0 0x4715E0
    MoveToEntityObjective* Destroy(uint8_t flags);
    // vf4 0x470940: where the target is
    ped::CVector* GetTargetPos();
};
static_assert(sizeof(MoveToEntityObjective) == 0x2C);

struct FollowEntityObjective : Objective {
    re::Ptr32<CEntity> target;          // +0x10
    float unknown14;          // +0x14

    // 0x470AA0 (its type is 1, MoveToEntity's)
    FollowEntityObjective* Construct(CPed* ped, CEntity* target, float value);
};
static_assert(sizeof(FollowEntityObjective) == 0x18);

// Face* share the flag at +0x10
struct FaceHeadingObjective : Objective {
    uint8_t unknown10;        // +0x10
    uint8_t pad11[3];
    float heading;            // +0x14

    // 0x470B20
    FaceHeadingObjective* Construct(CPed* ped, float heading, int flag);
};
static_assert(sizeof(FaceHeadingObjective) == 0x18);

struct FacePositionObjective : Objective {
    uint8_t unknown10;        // +0x10
    uint8_t pad11[3];
    ped::CVector position;    // +0x14

    // 0x470B50
    FacePositionObjective* Construct(CPed* ped, const ped::CVector* position);
    // vf4 0x470B90: the heading to face it
    float GetHeading();
};
static_assert(sizeof(FacePositionObjective) == 0x20);

struct FaceEntityObjective : Objective {
    uint8_t unknown10;        // +0x10
    uint8_t pad11[3];
    re::Ptr32<CEntity> target;          // +0x14 (registered)
    uint8_t unknown18;        // +0x18
    uint8_t pad19[3];

    // 0x470C10
    FaceEntityObjective* Construct(CPed* ped, CEntity* target, int flag);
    // 0x470C60: the destructor's body
    void Destruct();
    // vf0 0x471650
    FaceEntityObjective* Destroy(uint8_t flags);
    // vf4 0x470C90: the heading to face it (the ped's own when it's gone)
    float GetHeading();
};
static_assert(sizeof(FaceEntityObjective) == 0x1C);

struct AttackPedObjective : Objective {
    int32_t unknown10;        // +0x10
    re::Ptr32<CPed> target;             // +0x14 (registered)
    uint8_t hadTarget;        // +0x18
    uint8_t pad19[3];

    // 0x470D60
    AttackPedObjective* Construct(CPed* ped, CPed* target, int value);
    // vf0 0x471690
    AttackPedObjective* Destroy(uint8_t flags);
    // vf2 0x4716D0: done (the target is gone, dead, ...)
    uint8_t IsDone();
};
static_assert(sizeof(AttackPedObjective) == 0x1C);

struct CoverObjective : AttackPedObjective {
    int32_t unknown1C;        // +0x1C
    uint8_t coverInfo[0x48];  // +0x20 (constructed by 0x487EA0)
    ped::ActionHandle action; // +0x68

    // 0x4710B0
    CoverObjective* Construct(CPed* ped, CPed* target, int value, int value1C);
    // vf0 0x471BC0
    CoverObjective* Destroy(uint8_t flags);
};
static_assert(sizeof(CoverObjective) == 0x6C);

struct AttackVehicleObjective : Objective {
    re::Ptr32<CVehicle> target;         // +0x10 (registered)

    // 0x470DB0
    AttackVehicleObjective* Construct(CPed* ped, CVehicle* target);
    // vf0 0x470DF0
    AttackVehicleObjective* Destroy(uint8_t flags);
    // vf2 0x470E30: done when the vehicle's vf30 says so
    uint8_t IsDone();
};
static_assert(sizeof(AttackVehicleObjective) == 0x14);

struct BreakPropObjective : Objective {
    float unknown10;          // +0x10
    float unknown14;          // +0x14
    float unknown18;          // +0x18
    re::Ptr32<CEntity> prop;            // +0x1C (registered)
    int32_t unknown20;        // +0x20
    uint8_t unknown24;        // +0x24
    uint8_t pad25[3];

    // 0x470E50
    BreakPropObjective* Construct(CPed* ped, CEntity* prop, int value20, float f10, float f14, float f18);
    // vf0 0x471760
    BreakPropObjective* Destroy(uint8_t flags);
    // vf2 0x4717A0
    uint8_t IsDone();
};
static_assert(sizeof(BreakPropObjective) == 0x28);

struct UsePropObjective : Objective {
    re::Ptr32<CEntity> prop;            // +0x10 (registered)

    // 0x471810
    UsePropObjective* Construct(CPed* ped, CEntity* prop);
    // vf0 0x471880
    UsePropObjective* Destroy(uint8_t flags);
    // vf2 0x470FA0
    uint8_t IsDone();
};
static_assert(sizeof(UsePropObjective) == 0x14);

struct FleeOnPathOnBikeObjective : Objective {
    int32_t unknown10;        // +0x10 (-1)
    int32_t unknown14;        // +0x14
    float unknown18[3];       // +0x18
    int32_t unknown24;        // +0x24 (1)
    int32_t unknown28;        // +0x28 (1)
    ped::ActionHandle actions[3];  // +0x2C
    int32_t unknown38;        // +0x38
    re::Ptr32<CEntity> entity;          // +0x3C (registered)

    // 0x470FC0 (the second argument unused)
    FleeOnPathOnBikeObjective* Construct(CPed* ped, int unused);
    // vf0 0x471B30
    FleeOnPathOnBikeObjective* Destroy(uint8_t flags);
};
static_assert(sizeof(FleeOnPathOnBikeObjective) == 0x40);

struct FollowPathObjective : Objective {
    uint8_t unknown10[0x1C];
    ped::ActionHandle actions[3];  // +0x2C

    // vf0 0x4700D0
    FollowPathObjective* Destroy(uint8_t flags);
};
static_assert(sizeof(FollowPathObjective) == 0x38);

struct FleeOnRoadOnBikeObjective : Objective {
    int32_t unknown10;        // +0x10

    // 0x471080 (the second argument unused)
    FleeOnRoadOnBikeObjective* Construct(CPed* ped, int unused);
};
static_assert(sizeof(FleeOnRoadOnBikeObjective) == 0x14);

// Slinger, PlayCatch, PassBall, ReceiveBall: a time limit (-1: none)
struct SlingerObjective : Objective {
    uint32_t endTime;         // +0x10 (CTimer ms)

    // 0x471220
    SlingerObjective* Construct(CPed* ped, int duration);
    // vf2 0x4712E0: the time is up
    uint8_t IsDone();
};
static_assert(sizeof(SlingerObjective) == 0x14);

struct BallObjective : Objective {
    re::Ptr32<CEntity> entity;          // +0x10 (registered)
    uint32_t endTime;         // +0x14 (-1: none)

    // 0x4711D0: the time is up
    uint8_t IsTimeUp();
    // 0x4711F0: the same, testing -1 as a float
    uint8_t IsTimeUpF();
    // 0x471950 / 0x471A20 / 0x471AF0: vf0 (the same, but for the vtable)
    BallObjective* DestroyPlayCatch(uint8_t flags);
    BallObjective* DestroyPassBall(uint8_t flags);
    BallObjective* DestroyReceiveBall(uint8_t flags);
    // 0x4718C0 / 0x471990 / 0x471A60
    BallObjective* ConstructPlayCatch(CPed* ped, CEntity* entity, int duration);
    BallObjective* ConstructPassBall(CPed* ped, CEntity* entity, int duration);
    BallObjective* ConstructReceiveBall(CPed* ped, CEntity* entity, int duration);
};
static_assert(sizeof(BallObjective) == 0x18);

struct FollowFocusObjective : Objective {
    re::Ptr32<CEntity> focus;           // +0x10 (registered)

    // 0x471C10
    FollowFocusObjective* Construct(CPed* ped, CEntity* focus);
    // vf0 0x471B80
    FollowFocusObjective* Destroy(uint8_t flags);
};
static_assert(sizeof(FollowFocusObjective) == 0x14);

// The objectives a ped has: up to 20 in order (the first is the current one), and one
// it falls back to when there are none.
struct ObjectiveQueue {
    re::Ptr32<re::Ptr32<void>> vtable;                // +0x00 0x90CA74
    re::Ptr32<CPed> ped;                    // +0x04
    re::Ptr32<Objective> entries[20];       // +0x08 (with the count, an array class of its own: 0x46FF90)
    uint32_t count;               // +0x58
    re::Ptr32<Objective> defaultObjective;  // +0x5C
    ped::ActionHandle action;     // +0x60

    // 0x470160
    ObjectiveQueue* Construct(CPed* owner);
    // 0x471350: the destructor's body
    void Destruct();
    // vf0 0x471CE0
    ObjectiveQueue* Destroy(uint8_t flags);
    // 0x470180: at a place (0 .. count); 0 if past the end
    uint8_t Insert(Objective* objective, uint32_t index);
    // 0x4701D0: the first of a type
    Objective* FindOfType(int type);
    // 0x470200
    uint8_t RemoveAt(int index);
    // 0x470260: every one of a type (skipping the one after each it removes)
    void RemoveAllOfType(int type);
    // 0x4702C0
    uint8_t RemoveFirst();
    // 0x470300
    void SetDefault(Objective* objective);
    // 0x470340: the first, or the default
    Objective* GetCurrent();
    // 0x470350
    void Clear();
    // 0x471390: at the end (not if the last one attacks the same ped or breaks the
    // same prop; not if full)
    uint8_t Append(Objective* objective);
    // 0x471470: at the front (the same checks against the first)
    uint8_t Prepend(Objective* objective);
    // 0x471560
    uint8_t Remove(Objective* objective);
    // 0x471D30
    void MoveToFront(Objective* objective);
};
static_assert(offsetof(ObjectiveQueue, count) == 0x58);
static_assert(offsetof(ObjectiveQueue, defaultObjective) == 0x5C);
static_assert(sizeof(ObjectiveQueue) == 0x64);

// ---------------------------------------------------------------------------------
// CPed

// An anim blended in for a while (0x1DE8)
struct PedBlendSlot {
    int32_t id;     // -1: free
    float time;     // counts down to 0, then it's blended out
};
static_assert(sizeof(PedBlendSlot) == 8);

// The ped entity (CPhysical's fields first; only what the functions here use is named).
// The player's (CPlayerPed) fields are in the same 0x1F44 bytes, noted as such.
struct CPed {
    re::Ptr32<re::Ptr32<void>> vtable;                          // +0x0
    ped::CVector position;                  // +0x4 (CPlaceable's, without a matrix)
    float heading;                          // +0x10
    re::Ptr32<uint8_t> matrix;                        // +0x14 CMatrix: +0 right, +0x30 position
    re::Ptr32<void> rwObject;                         // +0x18 its scene node
    uint8_t unknown1C_[0x5C];
    int32_t unknown78;                      // +0x78
    uint8_t unknown7C_[0x28];
    int32_t unknownA4;                      // +0xA4
    uint8_t unknownA8_[0x60];
    uint8_t typeFlags;                      // +0x108 low 3 bits: the entity type (2 vehicle, 3 ped)
    uint8_t unknown109_[0x5];
    int16_t modelIndex;                     // +0x10E
    uint8_t unknown110_[0x4];
    re::Ptr32<uint8_t> animBlend;                     // +0x114 +0x94 -> +0x10: the clump its anims play on
    uint8_t unknown118_[0xB0];
    re::Ptr32<void> targeting;                        // +0x1C8 CTargetingSystem
    uint8_t unknown1CC_[0x4];
    re::Ptr32<CEntity> weapon;                        // +0x1D0 the object in hand (+0x10E its model, -1: none)
    uint8_t unknown1D4_[0x10C];
    re::Ptr32<uint8_t> intelligence;                  // +0x2E0 +0 move direction, +0xC, +0x1C, +0x300, +0x408
    uint8_t unknown2E4_[0x4];
    re::Ptr32<void> attitudeSet;                      // +0x2E8 CAttitudeSet
    re::Ptr32<void> unknown2EC;                       // +0x2EC
    re::Ptr32<void> unknown2F0;                       // +0x2F0
    re::Ptr32<CPed> unknown2F4;                       // +0x2F4 (peds chained through their +0x2F4)
    re::Ptr32<CPed> unknown2F8;                       // +0x2F8 (chained through +0x2F8)
    uint8_t unknown2FC_[0xC0];
    uint8_t unknown3BC[0x74];               // +0x3BC (0x48C070 reads +0x180 of it)
    re::Ptr32<CEntity> unknown430;                    // +0x430 (registered)
    uint8_t unknown434_[0x4];
    ped::EntityRef entityRef438;            // +0x438
    uint8_t unknown440_[0x4];
    ped::EntityRef entityRef444;            // +0x444
    uint8_t unknown44C_[0xA4];
    re::Ptr32<CPed> unknown4F0[5];                    // +0x4F0 (indexed by +0x504)
    int32_t unknown504;                     // +0x504
    uint8_t unknown508_[0x30];
    re::Ptr32<CEntity> unknown538;                    // +0x538
    uint8_t unknown53C_[0x8];
    re::Ptr32<CEntity> unknown544;                    // +0x544
    int32_t unknown548;                     // +0x548
    uint8_t unknown54C_[0xA4];
    ObjectiveQueue objectives;              // +0x5F0
    uint8_t unknown654_[0x14];
    uint8_t inventory[4];                   // +0x668 (the weapon inventory starts here: 0x486280 is its ammo)
    uint8_t unknown66C_[0x3F8];
    uint8_t unknownA64[4];                  // +0xA64 (0x48D470 is called on it)
    float unknownA68;                       // +0xA68 stat 2, in radians
    float unknownA6C;                       // +0xA6C stat 3
    uint8_t unknownA70_[0x140];
    int32_t unknownBB0;                     // +0xBB0
    re::Ptr32<CEntity> unknownBB4;                    // +0xBB4
    uint8_t unknownBB8_[0x1B8];
    re::Ptr32<CEntity> unknownD70;                    // +0xD70
    uint8_t unknownD74_[0x478];
    re::Ptr32<CEntity> unknown11EC;                   // +0x11EC
    uint8_t unknown11F0_[0x120];
    int32_t pedType;                        // +0x1310 13: the player
    uint8_t unknown1314_[0x8];
    int32_t unknown131C;                    // +0x131C
    int32_t unknown1320;                    // +0x1320
    uint8_t unknown1324_[0x8];
    int32_t unknown132C;                    // +0x132C
    uint8_t unknown1330_[0xC];
    int32_t unknown133C;                    // +0x133C
    uint8_t unknown1340_[0x8];
    int32_t unknown1348;                    // +0x1348
    uint8_t unknown134C_[0x30];
    int32_t unknown137C;                    // +0x137C
    uint8_t unknown1380_[0x4];
    int32_t unknown1384;                    // +0x1384 (set with the alpha)
    uint8_t unknown1388_[0xB4];
    int32_t unknown143C;                    // +0x143C
    uint8_t unknown1440_[0x30];
    int32_t unknown1470;                    // +0x1470
    uint8_t unknown1474_[0x18];
    int32_t unknown148C;                    // +0x148C
    uint8_t unknown1490_[0x18];
    int32_t unknown14A8;                    // +0x14A8
    uint8_t unknown14AC_[0x18];
    int32_t unknown14C4;                    // +0x14C4
    uint8_t unknown14C8_[0x4];
    int32_t unknown14CC;                    // +0x14CC
    uint8_t unknown14D0_[0x18];
    int32_t unknown14E8;                    // +0x14E8
    int32_t unknown14EC;                    // +0x14EC
    uint8_t unknown14F0_[0x10];
    int32_t unknown1500;                    // +0x1500
    uint8_t unknown1504_[0x1C];
    int32_t unknown1520;                    // +0x1520
    uint8_t unknown1524_[0x30];
    re::Ptr32<CVehicle> vehicle;                      // +0x1554
    uint8_t inVehicleSeat;                  // +0x1558 (in its seat, see Vehicle.h)
    uint8_t unknown1559;                    // +0x1559
    uint8_t unknown155A_[0x2];
    re::Ptr32<CPed> partner;                          // +0x155C (which has this one as its own)
    re::Ptr32<CVehicle> lastVehicle;                  // +0x1560 (registered)
    uint8_t unknown1564_[0xC];
    uint8_t seatState;                      // +0x1570 (1 .. 4; 2: see Vehicle.h, 0x4765C0)
    uint8_t unknown1571_[0xF];
    int32_t unknown1580;                    // +0x1580
    uint8_t unknown1584_[0x64];
    float unknown15E8[3];                   // +0x15E8 (1000 when the 0x1E24 flags change)
    uint8_t unknown15F4_[0x28];
    int32_t unknown161C;                    // +0x161C (an anim, 0: none)
    uint8_t unknown1620_[0x4];
    uint8_t unknown1624;                    // +0x1624
    uint8_t unknown1625_[0xB];
    uint8_t unknown1630[0xD0];              // +0x1630
    float desiredHeading;                   // +0x1700
    uint8_t unknown1704_[0x1BC];
    float unknown18C0;                      // +0x18C0 stat 0x16
    float unknown18C4;                      // +0x18C4
    int32_t unknown18C8;                    // +0x18C8 stat 0x17 (0 .. 5)
    uint8_t unknown18CC_[0x4];
    float unknown18D0;                      // +0x18D0 stat 0x10
    float unknown18D4;                      // +0x18D4
    uint8_t unknown18D8_[0x28];
    int32_t stats[25];                      // +0x1900 GetStat / SetStat
    uint8_t unknown1964_[0x334];
    uint8_t unknown1C98;                    // +0x1C98 (cleared when the max health drops below the health)
    uint8_t unknown1C99_[0x7];
    uint32_t money;                         // +0x1CA0 cents, in 25s, up to 10,000,000
    float maxHealth;                        // +0x1CA4 stat 4
    uint8_t unknown1CA8_[0x4];
    float healthRegen;                      // +0x1CAC stat 5 (the % of 0.02 a step)
    uint8_t unknown1CB0_[0x8];
    float health;                           // +0x1CB8
    float unknown1CBC;                      // +0x1CBC
    uint8_t unknown1CC0_[0x60];
    re::Ptr32<void> group;                            // +0x1D20 CPedGroup (cleared when the group goes)
    re::Ptr32<uint8_t> unknown1D24;                   // +0x1D24 (+0x70: 3 ...)
    re::Ptr32<uint8_t> unknown1D28;                   // +0x1D28 (the one before it)
    uint8_t unknown1D2C_[0x14];
    int32_t punishment;                     // +0x1D40 the trouble points
    int8_t unknown1D44;                     // +0x1D44
    uint8_t unknown1D45_[0x3];
    uint32_t punishmentTime;                // +0x1D48 (CTimer ms)
    uint8_t unknown1D4C_[0x70];
    int32_t unknown1DBC;                    // +0x1DBC
    float unknown1DC0;                      // +0x1DC0
    uint8_t unknown1DC4_[0x23];
    uint8_t alpha;                          // +0x1DE7
    PedBlendSlot blendSlots[4];             // +0x1DE8 (id -1: free)
    uint8_t unknown1E08_[0x4];
    float unknown1E0C;                      // +0x1E0C (seconds, against CTimer's)
    int32_t seat;                           // +0x1E10 (-1: none)
    uint8_t unknown1E14;                    // +0x1E14
    uint8_t unknown1E15_[0x3];
    ped::CVector unknown1E18;               // +0x1E18
    uint8_t flags1E24;                      // +0x1E24
    uint8_t unknown1E25;                    // +0x1E25
    uint8_t unknown1E26;                    // +0x1E26
    uint8_t unknown1E27;                    // +0x1E27
    int32_t unknown1E28;                    // +0x1E28
    int32_t giftIndex;                      // +0x1E2C (-1: none)
    uint8_t unknown1E30_[0x8];
    int32_t historyIndex;                   // +0x1E38 (-1: reset)
    ped::CVector history[4];                // +0x1E3C positions, filled then a ring
    int32_t historyCount;                   // +0x1E6C
    uint8_t unknown1E70_[0x21];
    uint8_t unknown1E91;                    // +0x1E91 (CPlayerPed)
    uint8_t unknown1E92_[0x2A];
    int32_t minPunishment;                  // +0x1EBC (CPlayerPed)
    uint8_t unknown1EC0_[0x80];
    uint32_t unknown1F40;                   // +0x1F40 (CPlayerPed: a time)

    // 0x470110: the distance between two points on the ground (cdecl)
    static float __cdecl Distance2D(const ped::CVector* a, const ped::CVector* b);
    // 0x474EE0: the entity if it's a vehicle (cdecl)
    static CVehicle* __cdecl AsVehicle(CEntity* entity);
    // 0x477D90: a ped with +0x1580 above 0 (cdecl)
    static uint8_t __cdecl IsPedWith1580(CEntity* entity);

    // 0x472D40: `in`, normalised (cdecl)
    static ped::CVector* __cdecl NormalisedCopy(ped::CVector* out, const ped::CVector* in);
    // 0x472D00: the squared distance (cdecl)
    static float __cdecl DistanceSquared(const ped::CVector* a, const ped::CVector* b);

    // Money (cents, in 25s)
    // 0x4730E0
    uint8_t CanAfford(uint32_t amount);
    // 0x473100: down to 0
    void TakeMoney(uint32_t amount);
    // 0x4752A0: the player's gains go to the stats; the HUD is told
    void SetMoney(uint32_t amount);
    // 0x475320: up to the cap (the player's gains to the stats)
    void GiveMoney(uint32_t amount);

    // Stats
    // 0x477060: (6 is 0 with some weapons in hand)
    int32_t GetStat(int stat);
    // 0x4770F0: some are copied into fields they're used from
    void SetStat(int stat, int value);

    // Health
    // 0x473140: by healthRegen, up to the max
    void RegenerateHealth();
    // 0x474CC0: up to the max
    void AddHealth(float amount);

    // Punishment (the trouble points)
    // 0x477520: (not for some models) and when it next drops
    void SetPunishment(int points);
    // vf45 0x4775D0
    void AddPunishment(int points);
    // vf44 0x477680: drops with time (not while an authority figure sees the ped)
    void UpdatePunishment();
    // 0x4745D0
    void Update1D44();
    // 0x4745B0
    void SetPunishmentTime(int msFromNow);

    // Gifts
    // 0x474A30
    uint8_t WantsGift();
    // 0x474AA0: returns out
    int* GetGift(int* out);
    // 0x474B30
    void Set1E28(int value);
    // 0x474B40: (counts the requests of each gift)
    void SetGiftIndex(int index);
    // 0x474B80
    uint8_t Has0GiftRequirement();
    // 0x474A90
    void Set1E26(int value);
    // 0x47D1B0: (1 also counts and puts a blip)
    void Set1E27(int value);

    // 0x473200: remembers the old one in +0x1D28
    void Push1D24(uint8_t* value);
    // 0x473220
    uint8_t Is1D24Kind3();
    // 0x473250: both cleared; `notify`: each is told (0x4475D0)
    uint8_t Clear1D24(int notify);
    // 0x4732A0
    void SetAlpha(int alpha, int flag);
    // 0x4734C0
    void SetDesiredHeading(float heading);
    // 0x475730: its heading, the matrix's, and the intelligence's directions
    void SetHeading(float heading);
    // 0x474570: the intelligence's move direction from its forward, flat
    void UpdateMoveDirection();
    // 0x4734D0
    uint8_t* Get1630();
    // 0x4736F0: the matrix's right (made from the heading without one)
    ped::CVector* GetRight(ped::CVector* out);
    // vf25 0x474870 (the second argument unused)
    ped::CVector* GetPosition(ped::CVector* out, int unused);   // (returns out)
    // 0x4777B0
    float DistanceToPlayer();
    // 0x4731D0: its attitude towards a ped type (the set's vf1; nothing without a set)
    void SetAttitude(int pedType, int attitude);
    // 0x4731F0: every attitude back to 5 (no null check)
    void ResetAttitudes();
    // 0x475380: the attitude towards a ped type (the set's, or the default's when 5)
    int* GetAttitude(int* out, int pedType);
    // 0x475400
    int* GetAttitudeTo(int* out, CPed* other);
    // 0x473E10
    uint8_t Has1470OrWeapon4330();
    // vf24 0x473F60
    void Vf24();
    // 0x473F80
    uint32_t Get473F80();
    // 0x473F10: the player has the flag 0x1B of this model
    uint8_t Is473F10();
    // 0x474340
    void Set18C4(float value);
    // 0x474350: +0x18C0 the remainder of 200, +0x18C8 the 200s (0 .. 5)
    void Set18C0And18C8(int value);
    // 0x4743D0
    void Add18C8(int value);
    // 0x474430
    void Set18D4(float value);
    // 0x474440
    void Set1559(int value, int notify);
    // vf28 0x474920
    float Vf28();
    // vf29 0x474940
    float Vf29();
    // vf21 0x474970
    uint8_t Vf21();
    // 0x474990: +0x1E0C = now (seconds) + `seconds`
    void Set1E0C(float seconds);
    // 0x4749C0: +0x1E0C is still ahead
    uint8_t Is1E0CAhead();
    // 0x474A00
    void Set1E18(const ped::CVector* value);
    // vf16 0x474BD0
    uint8_t* GetIntelligence300();
    // 0x474BE0
    uint8_t* GetIntelligence408();
    // 0x474BF0
    uint8_t HasWeapon();
    // 0x474C10
    void SetFlags1E24(int set, int second);
    // 0x476180: registered as +0x1554, +0x1560 and +0x430
    void SetVehicle(CVehicle* vehicle);
    // 0x476640
    void EnterVehicle(CVehicle* vehicle);
    // 0x476200
    uint8_t Is476200(CEntity* other);
    // 0x4762B0: it's in the vehicle, or the vehicle's +0x450 is this one
    uint8_t IsInVehicle(CEntity* vehicle);
    // 0x4765C0
    uint8_t Is4765C0();
    // 0x476600
    uint8_t Is476600();
    // 0x477AC0
    void RecordPosition();
    // 0x477B70
    void ResetPositionHistory();
    // 0x477B90: moved less than 0.2 over the history
    uint8_t HasntMoved();
    // 0x477DC0
    void* GetModelInfo();
    // 0x477C00: one of the entities it's involved with
    CEntity* GetEntity(int kind, int* out);
    // 0x497100
    CEntity* GetTarget();
    // 0x478180
    float Get478180(float value);
    // 0x47B7E0: 0x4771F0 with 10
    void Set1570(int state, int a, int b);
    // 0x47B940
    uint8_t Is47B940(float a, int b);

    // 0x4744B0: registered, unless one of the two drives a vehicle of kind other than 6 or 7
    void SetPartner(CPed* other);
    // 0x477470: the partner, its anim and its targeting dropped
    void ClearPartner();
    // 0x498B10: 0x497E70 with no offset
    void SetTargetEntity(CEntity* target);
    // The intelligence's (+0x2E0) own, forwarded
    // 0x474090
    void Intelligence484E60();
    // 0x4740C0
    void Intelligence4843E0(int a, int b);
    // 0x4740D0
    void Intelligence4849B0(int a, int b);
    // 0x4740E0
    uint8_t Intelligence484A70(int a, int b);
    // 0x4740F0
    uint8_t Intelligence484AF0(int a, int b);
    // 0x474110
    void Intelligence484480();

    // Anims blended in for a while
    // 0x4739E0: (nothing if there already or no slot free)
    void AddBlend(int id);
    // 0x473A50
    void RemoveBlend(int id);
    // 0x473AC0
    void ClearBlends();
    // 0x473B10
    void UpdateBlends();

    // CPlayerPed
    // 0x49A830
    void SubPunishment(int points);
    // 0x49A840
    void ResetPunishment();
    // vf44 0x49A7F0: not below the minimum
    void PlayerUpdatePunishment();
    // 0x49AD40: (+0x1F40 - now) / 0xA35054, 0 .. 1
    float Player49AD40();
};
static_assert(sizeof(CPed) == 0x1F44);
static_assert(offsetof(CPed, vtable) == 0x0);
static_assert(offsetof(CPed, position) == 0x4);
static_assert(offsetof(CPed, heading) == 0x10);
static_assert(offsetof(CPed, matrix) == 0x14);
static_assert(offsetof(CPed, rwObject) == 0x18);
static_assert(offsetof(CPed, unknown78) == 0x78);
static_assert(offsetof(CPed, unknownA4) == 0xA4);
static_assert(offsetof(CPed, typeFlags) == 0x108);
static_assert(offsetof(CPed, modelIndex) == 0x10E);
static_assert(offsetof(CPed, animBlend) == 0x114);
static_assert(offsetof(CPed, targeting) == 0x1C8);
static_assert(offsetof(CPed, weapon) == 0x1D0);
static_assert(offsetof(CPed, intelligence) == 0x2E0);
static_assert(offsetof(CPed, attitudeSet) == 0x2E8);
static_assert(offsetof(CPed, unknown2EC) == 0x2EC);
static_assert(offsetof(CPed, unknown2F0) == 0x2F0);
static_assert(offsetof(CPed, unknown2F4) == 0x2F4);
static_assert(offsetof(CPed, unknown2F8) == 0x2F8);
static_assert(offsetof(CPed, unknown3BC) == 0x3BC);
static_assert(offsetof(CPed, unknown430) == 0x430);
static_assert(offsetof(CPed, entityRef438) == 0x438);
static_assert(offsetof(CPed, entityRef444) == 0x444);
static_assert(offsetof(CPed, unknown4F0) == 0x4F0);
static_assert(offsetof(CPed, unknown504) == 0x504);
static_assert(offsetof(CPed, unknown538) == 0x538);
static_assert(offsetof(CPed, unknown544) == 0x544);
static_assert(offsetof(CPed, unknown548) == 0x548);
static_assert(offsetof(CPed, objectives) == 0x5F0);
static_assert(offsetof(CPed, inventory) == 0x668);
static_assert(offsetof(CPed, unknownA64) == 0xA64);
static_assert(offsetof(CPed, unknownA68) == 0xA68);
static_assert(offsetof(CPed, unknownA6C) == 0xA6C);
static_assert(offsetof(CPed, unknownBB0) == 0xBB0);
static_assert(offsetof(CPed, unknownBB4) == 0xBB4);
static_assert(offsetof(CPed, unknownD70) == 0xD70);
static_assert(offsetof(CPed, unknown11EC) == 0x11EC);
static_assert(offsetof(CPed, pedType) == 0x1310);
static_assert(offsetof(CPed, unknown131C) == 0x131C);
static_assert(offsetof(CPed, unknown1320) == 0x1320);
static_assert(offsetof(CPed, unknown132C) == 0x132C);
static_assert(offsetof(CPed, unknown133C) == 0x133C);
static_assert(offsetof(CPed, unknown1348) == 0x1348);
static_assert(offsetof(CPed, unknown137C) == 0x137C);
static_assert(offsetof(CPed, unknown1384) == 0x1384);
static_assert(offsetof(CPed, unknown143C) == 0x143C);
static_assert(offsetof(CPed, unknown1470) == 0x1470);
static_assert(offsetof(CPed, unknown148C) == 0x148C);
static_assert(offsetof(CPed, unknown14A8) == 0x14A8);
static_assert(offsetof(CPed, unknown14C4) == 0x14C4);
static_assert(offsetof(CPed, unknown14CC) == 0x14CC);
static_assert(offsetof(CPed, unknown14E8) == 0x14E8);
static_assert(offsetof(CPed, unknown14EC) == 0x14EC);
static_assert(offsetof(CPed, unknown1500) == 0x1500);
static_assert(offsetof(CPed, unknown1520) == 0x1520);
static_assert(offsetof(CPed, vehicle) == 0x1554);
static_assert(offsetof(CPed, inVehicleSeat) == 0x1558);
static_assert(offsetof(CPed, unknown1559) == 0x1559);
static_assert(offsetof(CPed, partner) == 0x155C);
static_assert(offsetof(CPed, lastVehicle) == 0x1560);
static_assert(offsetof(CPed, seatState) == 0x1570);
static_assert(offsetof(CPed, unknown1580) == 0x1580);
static_assert(offsetof(CPed, unknown15E8) == 0x15E8);
static_assert(offsetof(CPed, unknown161C) == 0x161C);
static_assert(offsetof(CPed, unknown1624) == 0x1624);
static_assert(offsetof(CPed, unknown1630) == 0x1630);
static_assert(offsetof(CPed, desiredHeading) == 0x1700);
static_assert(offsetof(CPed, unknown18C0) == 0x18C0);
static_assert(offsetof(CPed, unknown18C4) == 0x18C4);
static_assert(offsetof(CPed, unknown18C8) == 0x18C8);
static_assert(offsetof(CPed, unknown18D0) == 0x18D0);
static_assert(offsetof(CPed, unknown18D4) == 0x18D4);
static_assert(offsetof(CPed, stats) == 0x1900);
static_assert(offsetof(CPed, unknown1C98) == 0x1C98);
static_assert(offsetof(CPed, money) == 0x1CA0);
static_assert(offsetof(CPed, maxHealth) == 0x1CA4);
static_assert(offsetof(CPed, healthRegen) == 0x1CAC);
static_assert(offsetof(CPed, health) == 0x1CB8);
static_assert(offsetof(CPed, unknown1CBC) == 0x1CBC);
static_assert(offsetof(CPed, group) == 0x1D20);
static_assert(offsetof(CPed, unknown1D24) == 0x1D24);
static_assert(offsetof(CPed, unknown1D28) == 0x1D28);
static_assert(offsetof(CPed, punishment) == 0x1D40);
static_assert(offsetof(CPed, unknown1D44) == 0x1D44);
static_assert(offsetof(CPed, punishmentTime) == 0x1D48);
static_assert(offsetof(CPed, unknown1DBC) == 0x1DBC);
static_assert(offsetof(CPed, unknown1DC0) == 0x1DC0);
static_assert(offsetof(CPed, alpha) == 0x1DE7);
static_assert(offsetof(CPed, blendSlots) == 0x1DE8);
static_assert(offsetof(CPed, unknown1E0C) == 0x1E0C);
static_assert(offsetof(CPed, seat) == 0x1E10);
static_assert(offsetof(CPed, unknown1E14) == 0x1E14);
static_assert(offsetof(CPed, unknown1E18) == 0x1E18);
static_assert(offsetof(CPed, flags1E24) == 0x1E24);
static_assert(offsetof(CPed, unknown1E25) == 0x1E25);
static_assert(offsetof(CPed, unknown1E26) == 0x1E26);
static_assert(offsetof(CPed, unknown1E27) == 0x1E27);
static_assert(offsetof(CPed, unknown1E28) == 0x1E28);
static_assert(offsetof(CPed, giftIndex) == 0x1E2C);
static_assert(offsetof(CPed, historyIndex) == 0x1E38);
static_assert(offsetof(CPed, history) == 0x1E3C);
static_assert(offsetof(CPed, historyCount) == 0x1E6C);
static_assert(offsetof(CPed, unknown1E91) == 0x1E91);
static_assert(offsetof(CPed, minPunishment) == 0x1EBC);
static_assert(offsetof(CPed, unknown1F40) == 0x1F40);

// ---------------------------------------------------------------------------------
// CPedGroup: up to 10 peds (registered pointers; each member's +0x1D20 is its group)

struct CPedGroup {
    re::Ptr32<re::Ptr32<void>> vtable;             // +0x00 0x90FAA8
    re::Ptr32<CPed> members[10];         // +0x04 (registered)
    uint32_t count;            // +0x2C the slots in use (freed ones stay)
    int32_t maxMembers;        // +0x30 (up to 10)
    int32_t unknown34;         // +0x34
    int32_t unknown38;         // +0x38 (vf2)
    int32_t unknown3C;         // +0x3C
    int32_t unknown40;         // +0x40
    uint8_t unknown44;         // +0x44
    uint8_t pad45[3];
    uint32_t bits48;           // +0x48 (bits 9 .. 24 set when made)
    uint32_t bits4C;           // +0x4C (low 11 bits cleared)
    uint8_t flags50;           // +0x50
    uint8_t unknown51;         // +0x51 (0xF8)
    uint8_t flags52;           // +0x52
    uint8_t pad53;
    uint32_t bits54;           // +0x54
    int32_t unknown58;         // +0x58
    uint8_t unknown5C[8];      // +0x5C
    uint8_t flags64;           // +0x64
    uint8_t pad65[3];
    float unknown68[3];        // +0x68
    int32_t unknown74;         // +0x74 (-1)
    int32_t unknown78;         // +0x78
    re::Ptr32<CPed> leader;              // +0x7C (registered)

    // 0x489520
    CPedGroup* Construct(int maxMembers, int value34, int value38, int value3C);
    // 0x489670: the destructor's body (the members let go)
    void Destruct();
    // vf0 0x489F30
    CPedGroup* Destroy(uint8_t flags);
    // vf1 0x489F00: as many members as it takes
    uint8_t IsFull();
    // vf2 0x489660
    int32_t* Get38(int32_t* out);   // (returns out)
    // 0x489700: the slots with a member
    int CountMembers();
    // 0x489770: its slot emptied (not compacted)
    void RemoveMember(CPed* ped);
};
static_assert(offsetof(CPedGroup, count) == 0x2C);
static_assert(offsetof(CPedGroup, bits48) == 0x48);
static_assert(offsetof(CPedGroup, unknown58) == 0x58);
static_assert(offsetof(CPedGroup, flags64) == 0x64);
static_assert(offsetof(CPedGroup, unknown74) == 0x74);
static_assert(offsetof(CPedGroup, leader) == 0x7C);
static_assert(sizeof(CPedGroup) == 0x80);
