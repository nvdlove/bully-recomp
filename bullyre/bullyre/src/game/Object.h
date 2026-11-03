#pragma once

// Objects: CObject (0x22C bytes, CPools::ms_pObjectPool, vtable 0x9134F4), the cutscene
// objects (CCutsceneObject, 0x33C, vtable 0x913304), the dummies they're made from
// (CDummyObject, vtable 0x9133B4), and the accessories peds wear (CAccessory, 0x2E8,
// vtable 0x912484; a ped's two are a CAccessoryContainer).
//
// As in Vehicle.h, the base classes' fields are named here where these use them.

#include <cstddef>
#include <cstdint>

#include "game/Vehicle.h"
#include "reversed.h"

struct CObject {
    re::Ptr32<re::Ptr32<void>> vtable;                // +0x0
    veh::CVector placementPos;    // +0x4 the position while there's no matrix
    uint8_t unknown10[0x4];
    re::Ptr32<veh::CMatrix> matrix;         // +0x14 nullptr: none (its +0x30 is the position)
    re::Ptr32<uint8_t> rwObject;            // +0x18 its Gamebryo object
    uint8_t unknown1C[0x4];
    int32_t unknown20;            // +0x20
    uint8_t unknown24[0x4];
    int32_t unknown28;            // +0x28 (static, for most kinds: see SetIsStatic)
    uint8_t unknown2C[0x10];
    int32_t unknown3C;            // +0x3C
    uint8_t unknown40[0x8];
    int32_t unknown48;            // +0x48
    uint8_t unknown4C[0x28];
    int32_t unknown74;            // +0x74
    uint8_t unknown78[0x1C];
    int32_t unknown94;            // +0x94 (not drawn while set)
    uint8_t unknown98[0x18];
    int32_t unknownB0;            // +0xB0 (static, for kind 7)
    uint8_t unknownB4[0x38];
    int32_t unknownEC;            // +0xEC
    uint8_t unknownF0[0x18];
    uint8_t typeAndStatus;        // +0x108 low 3 bits the entity type (4: object), the rest its status
    uint8_t unknown109[0x5];
    int16_t modelIndex;           // +0x10E
    uint8_t unknown110[0x1];
    uint8_t unknown111;           // +0x111
    uint8_t unknown112[0x2];
    re::Ptr32<uint8_t> animData;            // +0x114 (the cutscene object's +0x234)
    uint8_t unknown118[0x8];
    int32_t unknown120;           // +0x120
    veh::CVector moveSpeed;       // +0x124
    veh::CVector turnSpeed;       // +0x130
    uint8_t unknown13C[0x1C];
    float unknown158;             // +0x158
    float unknown15C;             // +0x15C
    float unknown160;             // +0x160
    float unknown164;             // +0x164
    uint8_t unknown168[0x18];
    uint8_t unknown180;           // +0x180 (10 while static, else 0)
    uint8_t unknown181[0x3B];
    uint8_t flags1BC;             // +0x1BC
    uint8_t unknown1BD[0x47];
    int32_t unknown204;           // +0x204
    uint8_t objectType;           // +0x208 who made it (1 the game, 4 a cutscene; 2, 4, 5 can't be deleted)
    uint8_t objectFlags;          // +0x209 1 (mission), 0x10 (a weapon model), 0x20 (a sound playing), 0x40 (info id 30000)
    uint8_t objectKind;           // +0x20A (7: a different static flag)
    uint8_t unknown20B;           // +0x20B
    uint32_t sound;               // +0x20C (0x59D110 sets it)
    float unknown210[4];          // +0x210
    int16_t unknown220;           // +0x220 (the object info's id, low 15 bits)
    uint8_t unknown222[0x2];
    int32_t unknown224;           // +0x224
    uint32_t hashId;              // +0x228 (ObjectNameToHashID's)

    // 0x4CF410: the constructor (CPhysical's first; returns this)
    CObject* Construct();
    // 0x4CFBA0: the destructor's body (CPhysical's last)
    void Destruct();
    // 0x4CFCE0: set up for its model (static, its object info's id)
    void Init();
    // 0x4CF2B0: not made by a mission (types 2, 4, 5)
    uint8_t CanBeDeleted();
    // 0x4CF3C0: its object info's +0x24 (0: none)
    int32_t GetInfo24();

    // the virtuals
    // vf7 0x4CF2E0
    void SetIsStatic(int isStatic);
    // vf18 0x4CF320
    void Render();
    // vf30 0x4CF360 (0 / 1 in all of eax)
    uint32_t IsUnknownVf30();
    // vf31 0x4CF380 (0 / 1 in all of eax)
    uint32_t IsUnknownVf31();
    // vf32 0x4CF3B0
    float GetUnknown220();
};
static_assert(offsetof(CObject, vtable) == 0x0);
static_assert(offsetof(CObject, placementPos) == 0x4);
static_assert(offsetof(CObject, matrix) == 0x14);
static_assert(offsetof(CObject, rwObject) == 0x18);
static_assert(offsetof(CObject, unknown20) == 0x20);
static_assert(offsetof(CObject, unknown28) == 0x28);
static_assert(offsetof(CObject, unknown3C) == 0x3C);
static_assert(offsetof(CObject, unknown48) == 0x48);
static_assert(offsetof(CObject, unknown74) == 0x74);
static_assert(offsetof(CObject, unknown94) == 0x94);
static_assert(offsetof(CObject, unknownB0) == 0xB0);
static_assert(offsetof(CObject, unknownEC) == 0xEC);
static_assert(offsetof(CObject, typeAndStatus) == 0x108);
static_assert(offsetof(CObject, modelIndex) == 0x10E);
static_assert(offsetof(CObject, unknown111) == 0x111);
static_assert(offsetof(CObject, animData) == 0x114);
static_assert(offsetof(CObject, unknown120) == 0x120);
static_assert(offsetof(CObject, moveSpeed) == 0x124);
static_assert(offsetof(CObject, turnSpeed) == 0x130);
static_assert(offsetof(CObject, unknown158) == 0x158);
static_assert(offsetof(CObject, unknown15C) == 0x15C);
static_assert(offsetof(CObject, unknown160) == 0x160);
static_assert(offsetof(CObject, unknown164) == 0x164);
static_assert(offsetof(CObject, unknown180) == 0x180);
static_assert(offsetof(CObject, flags1BC) == 0x1BC);
static_assert(offsetof(CObject, unknown204) == 0x204);
static_assert(offsetof(CObject, objectType) == 0x208);
static_assert(offsetof(CObject, objectFlags) == 0x209);
static_assert(offsetof(CObject, objectKind) == 0x20A);
static_assert(offsetof(CObject, unknown20B) == 0x20B);
static_assert(offsetof(CObject, sound) == 0x20C);
static_assert(offsetof(CObject, unknown210) == 0x210);
static_assert(offsetof(CObject, unknown220) == 0x220);
static_assert(offsetof(CObject, unknown224) == 0x224);
static_assert(offsetof(CObject, hashId) == 0x228);
static_assert(sizeof(CObject) == 0x22C);

struct CCutsceneObject : CObject {
    re::Ptr32<void> attachedTo;             // +0x22C a node it follows (nullptr: none)
    int32_t attachedIsFrame;      // +0x230 (the node itself, else 0x6CA0A0's of it)
    uint8_t animDataStorage;      // +0x234 (what +0x114 points at)
    uint8_t unknown235[0x103];
    int32_t boneIndex;            // +0x338 -1: none

    // 0x4CEE60: the constructor (returns this)
    CCutsceneObject* Construct();
    // 0x4CEEF0: the destructor's body (CObject's last)
    void Destruct();

    // the virtuals
    // vf8 0x4CEF20: the model set; a skinned one moves with its speed
    void SetModelIndex(int model, int arg);
    // vf12 0x4CF0C0: follows its node, else its bone's (or its root's) position
    void ProcessControl();
    // vf17 0x4CF040: follows its node
    void PreRender();
    // vf18 0x4CEF70: drawn with twice the draw distance
    void Render();
};
static_assert(offsetof(CCutsceneObject, attachedTo) == 0x22C);
static_assert(offsetof(CCutsceneObject, attachedIsFrame) == 0x230);
static_assert(offsetof(CCutsceneObject, animDataStorage) == 0x234);
static_assert(offsetof(CCutsceneObject, boneIndex) == 0x338);
static_assert(sizeof(CCutsceneObject) == 0x33C);

struct CAccessory {
    re::Ptr32<re::Ptr32<void>> vtable;                // +0x0
    uint8_t unknown4[0x14];
    re::Ptr32<uint8_t> rwObject;            // +0x18
    uint8_t unknown1C[0x4];
    int32_t unknown20;            // +0x20
    uint8_t unknown24[0x18];
    int32_t unknown3C;            // +0x3C
    uint8_t unknown40[0xC8];
    uint8_t typeAndStatus;        // +0x108
    uint8_t unknown109[0xB];
    re::Ptr32<uint8_t> animData;            // +0x114 (its +0x1CC)
    uint8_t unknown118[0xC];
    veh::CVector moveSpeed;       // +0x124
    veh::CVector turnSpeed;       // +0x130
    uint8_t unknown13C[0x1C];
    float unknown158;             // +0x158
    float unknown15C;             // +0x15C
    float unknown160;             // +0x160
    float unknown164;             // +0x164
    uint8_t unknown168[0x54];
    uint8_t flags1BC;             // +0x1BC
    uint8_t unknown1BD[0x7];
    int32_t model;                // +0x1C4 -1: none
    re::Ptr32<void> anim;                   // +0x1C8 its animation on the wearer
    uint8_t animDataStorage;      // +0x1CC
    uint8_t unknown1CD[0x103];
    re::Ptr32<uint8_t> wearerAnimData;      // +0x2D0 (the animation data it's played on)
    int32_t animId;               // +0x2D4 -1: none
    int32_t animGroup;            // +0x2D8 -1: none (an index into the table at 0xC2C1B0)
    int32_t unknown2DC;           // +0x2DC
    uint32_t unknown2E0;          // +0x2E0 (constructed by 0x669FD0)
    re::Ptr32<uint8_t> info;                // +0x2E4

    // 0x4A6E40: the constructor (CPhysical's first; returns this)
    CAccessory* Construct();
    // 0x4A6BE0: the destructor's body (CPhysical's last)
    void Destruct();
    // 0x4A6C20 / 0x4A6C30
    void SetInfo(uint8_t* newInfo);
    uint8_t* GetInfo();
    // 0x4A6C40: its animation stopped, its model let go
    void StopAnim();
    // 0x4A6CA0: its animation played on the wearer's animation data
    void PlayAnim();
    // 0x4A6DD0: (unless +0x2DC) 0x469840, then vf3
    void Fn_4A6DD0();
    // 0x4A6E00
    void SetTurnSpeed(float x, float y, float z);
    // 0x4A6EF0: where its object is
    void GetNodePosition(veh::CVector* out);
    // 0x4A6FA0: worn: the model (requested, referenced), the animation
    void Set(int newModel, int group, uint8_t* wearer, int anim, int arg5);
    // 0x4A7040: the model let go, the animation stopped
    void Clear();
    // 0x4A71A0: knocked off: where it was, flying off with a random speed
    void KnockOff();

    // the virtuals
    // vf17 0x4A6DC0 (CEntity's)
    void PreRender();
    // vf18 0x4A6D70
    void Render();
};
static_assert(offsetof(CAccessory, vtable) == 0x0);
static_assert(offsetof(CAccessory, rwObject) == 0x18);
static_assert(offsetof(CAccessory, unknown20) == 0x20);
static_assert(offsetof(CAccessory, unknown3C) == 0x3C);
static_assert(offsetof(CAccessory, typeAndStatus) == 0x108);
static_assert(offsetof(CAccessory, animData) == 0x114);
static_assert(offsetof(CAccessory, moveSpeed) == 0x124);
static_assert(offsetof(CAccessory, turnSpeed) == 0x130);
static_assert(offsetof(CAccessory, unknown158) == 0x158);
static_assert(offsetof(CAccessory, unknown15C) == 0x15C);
static_assert(offsetof(CAccessory, unknown160) == 0x160);
static_assert(offsetof(CAccessory, unknown164) == 0x164);
static_assert(offsetof(CAccessory, flags1BC) == 0x1BC);
static_assert(offsetof(CAccessory, model) == 0x1C4);
static_assert(offsetof(CAccessory, anim) == 0x1C8);
static_assert(offsetof(CAccessory, animDataStorage) == 0x1CC);
static_assert(offsetof(CAccessory, wearerAnimData) == 0x2D0);
static_assert(offsetof(CAccessory, animId) == 0x2D4);
static_assert(offsetof(CAccessory, animGroup) == 0x2D8);
static_assert(offsetof(CAccessory, unknown2DC) == 0x2DC);
static_assert(offsetof(CAccessory, unknown2E0) == 0x2E0);
static_assert(offsetof(CAccessory, info) == 0x2E4);
static_assert(sizeof(CAccessory) == 0x2E8);


// The two accessories a ped wears (the accessory container pool's 8 bytes)
struct CAccessoryContainer {
    re::Ptr32<CAccessory> accessories[2];

    // 0x4A7370: both deleted (the table at 0xC2C730 told first)
    void Clear();
    // 0x4A73B0: into the first free place (nothing if both are taken)
    void Add(CAccessory* accessory);
    // 0x4A73D0: a place is free
    uint8_t HasRoom();
    // 0x4A73F0
    uint8_t Count();
    // 0x4A7410: nullptr past 1
    CAccessory* Get(uint32_t index);
    // 0x4A7430 / 0x4A7460: each one's vf17 / vf18
    void PreRender();
    void Render();
    // 0x4A7490: each one's 0x466160
    void Fn_4A7490(int arg);
    // 0x4A74C0: each knocked off with the chance (0..1; not the ones whose info says no)
    void KnockOff(float chance);
};
static_assert(sizeof(CAccessoryContainer) == 8);

// What a CObject leaves in the world while it's streamed out (CDummy's 0x128 bytes)
struct CDummyObject {
    re::Ptr32<re::Ptr32<void>> vtable;
    uint8_t unknown04[0x120];
    uint32_t hashId;         // +0x124 the object's +0x228

    // 0x4CF200 / 0x4CF220: the constructors (the second from an object, which gives
    // it its model, its Gamebryo object and its id)
    CDummyObject* Construct();
    CDummyObject* Construct(CObject* object);
    // vf0 0x4CF280: the deleting destructor
    CDummyObject* Destroy(uint8_t flags);
};
static_assert(offsetof(CDummyObject, hashId) == 0x124);
static_assert(sizeof(CDummyObject) == 0x128);
