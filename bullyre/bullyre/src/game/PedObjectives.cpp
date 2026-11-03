// Ped objectives and the queue they wait in (0x470030 .. 0x471D30).

#include "game/Ped.h"
#include "reversed.h"

namespace {
GAME_VAR(uint32_t, PO_timeInMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(int16_t, PO_nextClassId, 0x00BF3830);        // the next objective class id
GAME_ARRAY(void*, PO_modelInfoPtrs, 0x00C67738);      // CModelInfo::ms_modelInfoPtrs

inline re::Ptr32<void>* const PO_vtObjective = reinterpret_cast<re::Ptr32<void>*>(0x0090C9C0);
inline re::Ptr32<void>* const PO_vtQueue = reinterpret_cast<re::Ptr32<void>*>(0x0090CA74);
inline re::Ptr32<void>* const PO_vtMoveToPos = reinterpret_cast<re::Ptr32<void>*>(0x0090CA7C);
inline re::Ptr32<void>* const PO_vtMoveToStimulus = reinterpret_cast<re::Ptr32<void>*>(0x0090CA94);
inline re::Ptr32<void>* const PO_vtMoveToEntity = reinterpret_cast<re::Ptr32<void>*>(0x0090CAAC);
inline re::Ptr32<void>* const PO_vtFollowEntity = reinterpret_cast<re::Ptr32<void>*>(0x0090CAC4);
inline re::Ptr32<void>* const PO_vtFaceHeading = reinterpret_cast<re::Ptr32<void>*>(0x0090CAD8);
inline re::Ptr32<void>* const PO_vtFacePosition = reinterpret_cast<re::Ptr32<void>*>(0x0090CAF0);
inline re::Ptr32<void>* const PO_vtFaceEntity = reinterpret_cast<re::Ptr32<void>*>(0x0090CB08);
inline re::Ptr32<void>* const PO_vtAttackPed = reinterpret_cast<re::Ptr32<void>*>(0x0090CB20);
inline re::Ptr32<void>* const PO_vtAttackVehicle = reinterpret_cast<re::Ptr32<void>*>(0x0090CB34);
inline re::Ptr32<void>* const PO_vtBreakProp = reinterpret_cast<re::Ptr32<void>*>(0x0090CB48);
inline re::Ptr32<void>* const PO_vtUseProp = reinterpret_cast<re::Ptr32<void>*>(0x0090CB5C);
inline re::Ptr32<void>* const PO_vtFleeOnPathOnBike = reinterpret_cast<re::Ptr32<void>*>(0x0090CB70);
inline re::Ptr32<void>* const PO_vtFleeOnRoadOnBike = reinterpret_cast<re::Ptr32<void>*>(0x0090CB84);
inline re::Ptr32<void>* const PO_vtCover = reinterpret_cast<re::Ptr32<void>*>(0x0090CB98);
inline re::Ptr32<void>* const PO_vtSlinger = reinterpret_cast<re::Ptr32<void>*>(0x0090CBAC);
inline re::Ptr32<void>* const PO_vtPlayCatch = reinterpret_cast<re::Ptr32<void>*>(0x0090CBC0);
inline re::Ptr32<void>* const PO_vtPassBall = reinterpret_cast<re::Ptr32<void>*>(0x0090CBD4);
inline re::Ptr32<void>* const PO_vtReceiveBall = reinterpret_cast<re::Ptr32<void>*>(0x0090CBE8);
inline re::Ptr32<void>* const PO_vtFollowFocus = reinterpret_cast<re::Ptr32<void>*>(0x0090CBFC);
inline re::Ptr32<void>* const PO_vtFollowPath = reinterpret_cast<re::Ptr32<void>*>(0x0090CA24);

// The class ids (guard bits, id) of the objective kinds
GAME_VAR(uint32_t, PO_guardObjective, 0x00C2AFA8);
GAME_VAR(int16_t, PO_idObjective, 0x00C2AFA4);
GAME_VAR(uint32_t, PO_guardMoveToEntity, 0x00C2AFE0);
GAME_VAR(int16_t, PO_idMoveToEntity, 0x00C2AFDC);
GAME_VAR(uint32_t, PO_guardFace, 0x00C2AFE8);
GAME_VAR(int16_t, PO_idFace, 0x00C2AFE4);
GAME_VAR(uint32_t, PO_guardMoveToPos, 0x00C2AFF0);
GAME_VAR(int16_t, PO_idMoveToPos, 0x00C2AFEC);
GAME_VAR(uint32_t, PO_guardMoveToStimulus, 0x00C2AFF8);
GAME_VAR(int16_t, PO_idMoveToStimulus, 0x00C2AFF4);
GAME_VAR(uint32_t, PO_guardFollowEntity, 0x00C2B000);
GAME_VAR(int16_t, PO_idFollowEntity, 0x00C2AFFC);
GAME_VAR(uint32_t, PO_guardBreakProp, 0x00C2B008);
GAME_VAR(int16_t, PO_idBreakProp, 0x00C2B004);
GAME_VAR(uint32_t, PO_guardUseProp, 0x00C2B010);
GAME_VAR(int16_t, PO_idUseProp, 0x00C2B00C);
GAME_VAR(uint32_t, PO_guardFleeOnPathOnBike, 0x00C2B018);
GAME_VAR(int16_t, PO_idFleeOnPathOnBike, 0x00C2B014);
GAME_VAR(uint32_t, PO_guardCover, 0x00C2B020);
GAME_VAR(int16_t, PO_idCover, 0x00C2B01C);
GAME_VAR(uint32_t, PO_guardSlinger, 0x00C2B028);
GAME_VAR(int16_t, PO_idSlinger, 0x00C2B024);
GAME_VAR(uint32_t, PO_guardFollowFocus, 0x00C2B030);
GAME_VAR(int16_t, PO_idFollowFocus, 0x00C2B02C);

GAME_FN(void, __cdecl, PO_Free, 0x005EEAB0, void* p);
// CEntity::RegisterReference / CleanUpOldReference (cdecl wrappers)
GAME_FN(void, __cdecl, PO_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, PO_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(uint8_t, __thiscall, PO_ActionIsValid, 0x005D80E0, ped::ActionHandle* handle);
GAME_FN(void, __thiscall, PO_ActionRelease, 0x005D9C50, ped::ActionHandle* handle);
// the base kind's IsA (the ids at 0xC2AFD4 and 0xC2AFA4), and the root's (vf1)
GAME_FN(uint8_t, __cdecl, PO_IsA_Locomotion, 0x0046FFC0, int id);
GAME_FN(uint8_t, __cdecl, PO_IsA_MoveToPosStatic, 0x00470830, int id);
// the queue tells its action tree an objective of a type has gone
GAME_FN(void, __thiscall, PO_QueueNotifyRemoved, 0x0046FB30, ObjectiveQueue* queue, int type);
// the entries' array: removes one, moving the rest down
GAME_FN(void, __thiscall, PO_ArrayRemoveAt, 0x0046FF90, void* array, uint32_t index);
GAME_FN(void, __thiscall, PO_QueueClear, 0x00470350, ObjectiveQueue* queue);
GAME_FN(uint8_t, __thiscall, PO_QueueRemove, 0x00471560, ObjectiveQueue* queue, Objective* objective);
GAME_FN(uint8_t, __thiscall, PO_QueuePrepend, 0x00471470, ObjectiveQueue* queue, Objective* objective);
GAME_FN(uint8_t, __thiscall, PO_QueueRemoveAt, 0x00470200, ObjectiveQueue* queue, int index);
GAME_FN(void*, __thiscall, PO_StimulusInfo, 0x004A5CF0, CStimulus* stimulus);
GAME_FN(void, __thiscall, PO_CoverInfoConstruct, 0x00487EA0, void* info);
GAME_FN(void, __thiscall, PO_EntityRefSet, 0x00471300, ped::EntityRef* ref, CEntity* entity, int value);
GAME_FN(int, __cdecl, PO_Model_520380, 0x00520380, int index);
// CVector: a - b (cdecl, returns out); Normalise (thiscall)
GAME_FN(ped::CVector*, __cdecl, PO_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(float, __thiscall, PO_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(float, __cdecl, PO_GetATanOfXY, 0x00432FE0, float x, float y);
// the peds' own used here
GAME_FN(uint8_t, __thiscall, PO_Ped_473220, 0x00473220, CPed* ped);
GAME_FN(uint8_t, __thiscall, PO_Ped_IsGangType, 0x0041AA50, CPed* ped);
GAME_FN(uint8_t, __thiscall, PO_Ped_4765C0, 0x004765C0, CPed* ped);
GAME_FN(uint8_t, __cdecl, PO_Global_5BB380, 0x005BB380);
GAME_FN(int, __thiscall, PO_Inventory_GetAmmo, 0x00486280, void* inventory, int model);

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;

// (inlined everywhere in the game): the reference dropped, deleted at none
void Release(Objective* objective) {
    if (--objective->numRefs <= 0) {
        reinterpret_cast<void*(__thiscall*)(Objective*, int)>(static_cast<void*>(objective->vtable[0]))(objective, 1);
    }
}

// An entity's position: its matrix's if it has one (CPlaceable::GetPosition)
const ped::CVector* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

// The heading to face `target` from the ped (0 north), in [0, 2pi)
float HeadingTo(const ped::CVector* target, void* ped) {
    ped::CVector dir;
    PO_VectorSub(&dir, target, EntityPos(ped));
    dir.z = 0.0f;
    PO_VectorNormalise(&dir);
    float heading = static_cast<float>(PO_GetATanOfXY(dir.x, dir.y) - 1.5707963705062866);
    if (heading < 0.0f) {
        heading = static_cast<float>(heading + 6.2831854820251465);
    }
    return heading;
}
} // namespace

// ---------------------------------------------------------------------------------
// Objective

int16_t Objective::ClassId(uint32_t& guard, int16_t& id) {
    if (!(guard & 1)) {
        guard |= 1;
        id = PO_nextClassId;
        PO_nextClassId = static_cast<int16_t>(PO_nextClassId + 1);
    }
    return id;
}

Objective* Objective::Destroy(uint8_t flags) {
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

// The IsA of the kinds: their own id, or a base's
struct ObjectiveIsA : Objective {
    uint8_t MoveToPos(int id);
    uint8_t MoveToStimulus(int id);
    uint8_t MoveToEntity(int id);
    uint8_t FollowEntity(int id);
    uint8_t Face(int id);
    uint8_t BreakProp(int id);
    uint8_t UseProp(int id);
    uint8_t FleeOnPathOnBike(int id);
    uint8_t Cover(int id);
    uint8_t Slinger(int id);
    uint8_t FollowFocus(int id);
    static uint8_t __cdecl MoveToPosStatic(int id);
};

static uint8_t IsOwnOrRoot(int id, uint32_t& guard, int16_t& own) {
    if (static_cast<int16_t>(id) == Objective::ClassId(guard, own)) {
        return 1;
    }
    return static_cast<int16_t>(id) == Objective::ClassId(PO_guardObjective, PO_idObjective);
}

uint8_t ObjectiveIsA::MoveToPos(int id) {
    // (MoveToPos says it's a MoveToEntity too)
    if (static_cast<int16_t>(id) == ClassId(PO_guardMoveToPos, PO_idMoveToPos) ||
        static_cast<int16_t>(id) == ClassId(PO_guardMoveToEntity, PO_idMoveToEntity)) {
        return 1;
    }
    return PO_IsA_Locomotion(id);
}

uint8_t __cdecl ObjectiveIsA::MoveToPosStatic(int id) {
    if (static_cast<int16_t>(id) == ClassId(PO_guardMoveToPos, PO_idMoveToPos) ||
        static_cast<int16_t>(id) == ClassId(PO_guardMoveToEntity, PO_idMoveToEntity)) {
        return 1;
    }
    return PO_IsA_Locomotion(id);
}

uint8_t ObjectiveIsA::MoveToStimulus(int id) {
    if (static_cast<int16_t>(id) == ClassId(PO_guardMoveToStimulus, PO_idMoveToStimulus)) {
        return 1;
    }
    return PO_IsA_MoveToPosStatic(id);
}

uint8_t ObjectiveIsA::MoveToEntity(int id) {
    if (static_cast<int16_t>(id) == ClassId(PO_guardMoveToEntity, PO_idMoveToEntity)) {
        return 1;
    }
    return PO_IsA_Locomotion(id);
}

uint8_t ObjectiveIsA::FollowEntity(int id) {
    if (static_cast<int16_t>(id) == ClassId(PO_guardFollowEntity, PO_idFollowEntity)) {
        return 1;
    }
    return PO_IsA_Locomotion(id);
}

uint8_t ObjectiveIsA::Face(int id) {
    if (static_cast<int16_t>(id) == ClassId(PO_guardFace, PO_idFace)) {
        return 1;
    }
    return PO_IsA_Locomotion(id);
}

uint8_t ObjectiveIsA::BreakProp(int id) {
    return IsOwnOrRoot(id, PO_guardBreakProp, PO_idBreakProp);
}

uint8_t ObjectiveIsA::UseProp(int id) {
    return IsOwnOrRoot(id, PO_guardUseProp, PO_idUseProp);
}

uint8_t ObjectiveIsA::FleeOnPathOnBike(int id) {
    return IsOwnOrRoot(id, PO_guardFleeOnPathOnBike, PO_idFleeOnPathOnBike);
}

uint8_t ObjectiveIsA::Cover(int id) {
    return IsOwnOrRoot(id, PO_guardCover, PO_idCover);
}

uint8_t ObjectiveIsA::Slinger(int id) {
    return IsOwnOrRoot(id, PO_guardSlinger, PO_idSlinger);
}

uint8_t ObjectiveIsA::FollowFocus(int id) {
    return IsOwnOrRoot(id, PO_guardFollowFocus, PO_idFollowFocus);
}

// ---------------------------------------------------------------------------------
// The kinds

MoveToPosObjective* MoveToPosObjective::Construct(CPed* owner, const ped::CVector* pos, float speed, float r,
                                                  int stop, int flag18) {
    moveSpeed = speed;
    ped = owner;
    unknown14 = r;
    type = OBJECTIVE_MOVE_TO_POS;
    numRefs = 0;
    unknown18 = static_cast<uint8_t>(flag18);
    vtable = PO_vtMoveToPos;
    action.node = nullptr;
    target = *pos;
    radius = r;
    stopWhenThere = static_cast<uint8_t>(stop);
    return this;
}

MoveToPosObjective* MoveToPosObjective::Destroy(uint8_t flags) {
    vtable = PO_vtMoveToPos;
    // (released twice when valid: the release, then the handle's destructor)
    if (PO_ActionIsValid(&action)) {
        PO_ActionRelease(&action);
    }
    PO_ActionRelease(&action);
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

MoveToStimulusObjective* MoveToStimulusObjective::Construct(CPed* owner, CStimulus* stimulus) {
    const float speed = 0.3f;
    ped::CVector pos = At<ped::CVector>(stimulus, 0xC);
    moveSpeed = speed;
    ped = owner;
    unknown14 = speed;
    type = OBJECTIVE_MOVE_TO_POS;
    numRefs = 0;
    unknown18 = 0;
    action.node = nullptr;
    radius = speed;
    target = pos;
    stopWhenThere = 0;
    vtable = PO_vtMoveToStimulus;
    stimulusInfo = PO_StimulusInfo(stimulus);
    type = OBJECTIVE_MOVE_TO_STIMULUS;
    return this;
}

MoveToEntityObjective* MoveToEntityObjective::Construct(CPed* owner, CEntity* entity, float speed, float r,
                                                        int stop, int flag18) {
    moveSpeed = speed;
    ped = owner;
    unknown14 = r;
    numRefs = 0;
    unknown18 = static_cast<uint8_t>(flag18);
    type = OBJECTIVE_MOVE_TO_ENTITY;
    vtable = PO_vtMoveToEntity;
    action.node = nullptr;
    radius = r;
    target = entity;
    stopWhenThere = static_cast<uint8_t>(stop);
    return this;
}

MoveToEntityObjective* MoveToEntityObjective::Destroy(uint8_t flags) {
    vtable = PO_vtMoveToEntity;
    if (PO_ActionIsValid(&action)) {
        PO_ActionRelease(&action);
    }
    PO_ActionRelease(&action);
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

ped::CVector* MoveToEntityObjective::GetTargetPos() {
    return const_cast<ped::CVector*>(EntityPos(target));
}

FollowEntityObjective* FollowEntityObjective::Construct(CPed* owner, CEntity* entity, float value) {
    unknown14 = value;
    ped = owner;
    type = OBJECTIVE_MOVE_TO_ENTITY;
    numRefs = 0;
    vtable = PO_vtFollowEntity;
    target = entity;
    return this;
}

FaceHeadingObjective* FaceHeadingObjective::Construct(CPed* owner, float h, int flag) {
    heading = h;
    ped = owner;
    type = OBJECTIVE_FACE_HEADING;
    numRefs = 0;
    unknown10 = static_cast<uint8_t>(flag);
    vtable = PO_vtFaceHeading;
    return this;
}

FacePositionObjective* FacePositionObjective::Construct(CPed* owner, const ped::CVector* pos) {
    ped = owner;
    numRefs = 0;
    unknown10 = 0;
    type = OBJECTIVE_FACE_POSITION;
    vtable = PO_vtFacePosition;
    position = *pos;
    return this;
}

float FacePositionObjective::GetHeading() {
    return HeadingTo(&position, ped);
}

FaceEntityObjective* FaceEntityObjective::Construct(CPed* owner, CEntity* entity, int flag) {
    ped = owner;
    type = OBJECTIVE_FACE_ENTITY;
    numRefs = 0;
    unknown10 = 0;
    vtable = PO_vtFaceEntity;
    target = entity;
    if (entity) {
        PO_RegisterReference(entity, &target);
    }
    unknown18 = static_cast<uint8_t>(flag);
    return this;
}

void FaceEntityObjective::Destruct() {
    vtable = PO_vtFaceEntity;
    if (target) {
        PO_CleanUpOldReference(target, &target);
    }
    vtable = PO_vtObjective;
}

FaceEntityObjective* FaceEntityObjective::Destroy(uint8_t flags) {
    vtable = PO_vtFaceEntity;
    if (target) {
        PO_CleanUpOldReference(target, &target);
    }
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

float FaceEntityObjective::GetHeading() {
    if (!target) {
        return At<float>(ped, 0x1700);
    }
    ped::CVector pos = *EntityPos(target);
    return HeadingTo(&pos, ped);
}

AttackPedObjective* AttackPedObjective::Construct(CPed* owner, CPed* victim, int value) {
    ped = owner;
    type = OBJECTIVE_ATTACK_PED;
    numRefs = 0;
    vtable = PO_vtAttackPed;
    unknown10 = value;
    target = victim;
    if (victim) {
        PO_RegisterReference(victim, &target);
    }
    hadTarget = victim != nullptr;
    return this;
}

AttackPedObjective* AttackPedObjective::Destroy(uint8_t flags) {
    vtable = PO_vtAttackPed;
    if (target) {
        PO_CleanUpOldReference(target, &target);
    }
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

uint8_t AttackPedObjective::IsDone() {
    if (!hadTarget) {
        return 0;
    }
    // (the target gone, dead, ...)
    if (!target || At<uint8_t>(target, 0x1C98) || At<int32_t>(target, 0x14C4) || PO_Ped_473220(target)) {
        return 1;
    }
    if (PO_Ped_IsGangType(ped) && !At<int32_t>(target, 0x14E8)) {
        return 1;
    }
    if (PO_Global_5BB380() && PO_Ped_4765C0(ped) && At<uint8_t>(target, 0x1570) == 2) {
        return 1;
    }
    if (At<int32_t>(ped, 0x14EC)) {
        At<int32_t>(ped, 0x14EC) = 0;
    }
    return 0;
}

CoverObjective* CoverObjective::Construct(CPed* owner, CPed* victim, int value, int value1C) {
    ped = owner;
    type = OBJECTIVE_ATTACK_PED;
    numRefs = 0;
    vtable = PO_vtAttackPed;
    unknown10 = value;
    target = victim;
    if (victim) {
        PO_RegisterReference(victim, &target);
    }
    hadTarget = victim != nullptr;
    vtable = PO_vtCover;
    PO_CoverInfoConstruct(coverInfo);
    action.node = nullptr;
    unknown1C = value1C;
    type = OBJECTIVE_COVER;
    return this;
}

CoverObjective* CoverObjective::Destroy(uint8_t flags) {
    vtable = PO_vtCover;
    PO_ActionRelease(&action);
    vtable = PO_vtAttackPed;
    if (target) {
        PO_CleanUpOldReference(target, &target);
    }
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

AttackVehicleObjective* AttackVehicleObjective::Construct(CPed* owner, CVehicle* vehicle) {
    ped = owner;
    type = OBJECTIVE_ATTACK_VEHICLE;
    numRefs = 0;
    vtable = PO_vtAttackVehicle;
    target = vehicle;
    if (vehicle) {
        PO_RegisterReference(vehicle, &target);
    }
    return this;
}

AttackVehicleObjective* AttackVehicleObjective::Destroy(uint8_t flags) {
    vtable = PO_vtAttackVehicle;
    if (target) {
        PO_CleanUpOldReference(target, &target);
    }
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

uint8_t AttackVehicleObjective::IsDone() {
    if (target) {
        re::Ptr32<void>* vt = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(target);
        if (re::PtrCast<uint8_t(__thiscall*)(CVehicle*)>(vt[0x78 / 4])(target)) {
            return 1;
        }
    }
    return 0;
}

BreakPropObjective* BreakPropObjective::Construct(CPed* owner, CEntity* entity, int value20, float f10, float f14,
                                                  float f18) {
    ped = owner;
    type = OBJECTIVE_BREAK_PROP;
    numRefs = 0;
    vtable = PO_vtBreakProp;
    unknown10 = 0.0f;
    unknown14 = 0.0f;
    unknown18 = 0.0f;
    prop = entity;
    if (entity) {
        PO_RegisterReference(entity, &prop);
    }
    unknown10 = f10;
    unknown20 = value20;
    unknown24 = 0;
    unknown14 = f14;
    unknown18 = f18;
    return this;
}

BreakPropObjective* BreakPropObjective::Destroy(uint8_t flags) {
    vtable = PO_vtBreakProp;
    if (prop) {
        PO_CleanUpOldReference(prop, &prop);
    }
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

uint8_t BreakPropObjective::IsDone() {
    if (!prop) {
        return 1;
    }
    re::Ptr32<void>* vt = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(prop);
    if (re::PtrCast<uint8_t(__thiscall*)(CEntity*)>(vt[0x78 / 4])(prop)) {
        return 1;
    }
    if (!unknown24 || !ped) {
        return 0;
    }
    // out of ammo for the weapon in hand: the targets dropped
    void* weapon = At<void*>(ped, 0x1D0);
    if (PO_Inventory_GetAmmo(Bytes(ped) + 0x668, At<int16_t>(weapon, 0x10E)) > 0) {
        return 0;
    }
    PO_EntityRefSet(reinterpret_cast<ped::EntityRef*>(Bytes(ped) + 0x438), nullptr, 0);
    PO_EntityRefSet(reinterpret_cast<ped::EntityRef*>(Bytes(ped) + 0x444), nullptr, 0);
    return 1;
}

UsePropObjective* UsePropObjective::Construct(CPed* owner, CEntity* entity) {
    ped = owner;
    type = OBJECTIVE_USE_PROP;
    numRefs = 0;
    vtable = PO_vtUseProp;
    prop = entity;
    if (entity) {
        PO_RegisterReference(entity, &prop);
    }
    if (owner && entity) {
        int value = 0;
        if (PO_modelInfoPtrs[At<int16_t>(entity, 0x10E)]) {
            // (the game passes 0, not the model)
            value = PO_Model_520380(0);
        }
        PO_EntityRefSet(reinterpret_cast<ped::EntityRef*>(Bytes(owner) + 0x438), entity, value);
    }
    return this;
}

UsePropObjective* UsePropObjective::Destroy(uint8_t flags) {
    vtable = PO_vtUseProp;
    if (prop) {
        PO_CleanUpOldReference(prop, &prop);
    }
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

uint8_t UsePropObjective::IsDone() {
    if (!prop) {
        return 1;
    }
    re::Ptr32<void>* vt = *re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(prop);
    return re::PtrCast<uint8_t(__thiscall*)(CEntity*)>(vt[0x78 / 4])(prop) != 0;
}

FleeOnPathOnBikeObjective* FleeOnPathOnBikeObjective::Construct(CPed* owner, int) {
    ped = owner;
    type = OBJECTIVE_FLEE_ON_PATH_ON_BIKE;
    vtable = PO_vtFleeOnPathOnBike;
    numRefs = 0;
    unknown18[0] = 0.0f;
    actions[0].node = nullptr;
    unknown18[1] = 0.0f;
    actions[1].node = nullptr;
    unknown18[2] = 0.0f;
    actions[2].node = nullptr;
    unknown10 = -1;
    unknown14 = 0;
    unknown24 = 1;
    unknown28 = 1;
    unknown38 = 0;
    entity = nullptr;
    return this;
}

FleeOnPathOnBikeObjective* FleeOnPathOnBikeObjective::Destroy(uint8_t flags) {
    if (entity) {
        PO_CleanUpOldReference(entity, &entity);
    }
    PO_ActionRelease(&actions[2]);
    PO_ActionRelease(&actions[1]);
    PO_ActionRelease(&actions[0]);
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

FollowPathObjective* FollowPathObjective::Destroy(uint8_t flags) {
    vtable = PO_vtFollowPath;
    PO_ActionRelease(&actions[2]);
    PO_ActionRelease(&actions[1]);
    PO_ActionRelease(&actions[0]);
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

FleeOnRoadOnBikeObjective* FleeOnRoadOnBikeObjective::Construct(CPed* owner, int) {
    ped = owner;
    type = OBJECTIVE_FLEE_ON_ROAD_ON_BIKE;
    numRefs = 0;
    vtable = PO_vtFleeOnRoadOnBike;
    unknown10 = 0;
    return this;
}

SlingerObjective* SlingerObjective::Construct(CPed* owner, int duration) {
    ped = owner;
    type = OBJECTIVE_SLINGER;
    numRefs = 0;
    vtable = PO_vtSlinger;
    if (static_cast<float>(duration) == -1.0f) {
        endTime = 0xFFFFFFFF;
        return this;
    }
    endTime = PO_timeInMs + duration;
    return this;
}

uint8_t SlingerObjective::IsDone() {
    return endTime != 0xFFFFFFFF && PO_timeInMs > endTime;
}

uint8_t BallObjective::IsTimeUp() {
    if (endTime == 0xFFFFFFFF) {
        return 0;
    }
    return endTime < PO_timeInMs;
}

uint8_t BallObjective::IsTimeUpF() {
    if (static_cast<float>(static_cast<int32_t>(endTime)) == -1.0f) {
        return 0;
    }
    return endTime < PO_timeInMs;
}

// (PlayCatch, PassBall and ReceiveBall construct alike)
static BallObjective* ConstructBall(BallObjective* self, re::Ptr32<void>* vt, int32_t kind, CPed* owner, CEntity* entity,
                                    int duration) {
    self->ped = owner;
    self->type = kind;
    self->numRefs = 0;
    self->vtable = vt;
    self->entity = nullptr;
    if (static_cast<float>(duration) == -1.0f) {
        self->endTime = 0xFFFFFFFF;
    } else {
        self->endTime = PO_timeInMs + duration;
    }
    if (entity != self->entity) {
        if (self->entity) {
            PO_CleanUpOldReference(self->entity, &self->entity);
        }
        self->entity = entity;
        if (entity) {
            PO_RegisterReference(entity, &self->entity);
        }
    }
    return self;
}

static BallObjective* DestroyWithRef(Objective* self, re::Ptr32<void>* vt, re::Ptr32<CEntity>* ref, uint8_t flags) {
    self->vtable = vt;
    if (*ref) {
        PO_CleanUpOldReference(*ref, ref);
    }
    self->vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(self);
    }
    return static_cast<BallObjective*>(self);
}

BallObjective* BallObjective::ConstructPlayCatch(CPed* owner, CEntity* e, int duration) {
    return ConstructBall(this, PO_vtPlayCatch, OBJECTIVE_PLAY_CATCH, owner, e, duration);
}
BallObjective* BallObjective::ConstructPassBall(CPed* owner, CEntity* e, int duration) {
    return ConstructBall(this, PO_vtPassBall, OBJECTIVE_PASS_BALL, owner, e, duration);
}
BallObjective* BallObjective::ConstructReceiveBall(CPed* owner, CEntity* e, int duration) {
    return ConstructBall(this, PO_vtReceiveBall, OBJECTIVE_RECEIVE_BALL, owner, e, duration);
}
BallObjective* BallObjective::DestroyPlayCatch(uint8_t flags) {
    return DestroyWithRef(this, PO_vtPlayCatch, &entity, flags);
}
BallObjective* BallObjective::DestroyPassBall(uint8_t flags) {
    return DestroyWithRef(this, PO_vtPassBall, &entity, flags);
}
BallObjective* BallObjective::DestroyReceiveBall(uint8_t flags) {
    return DestroyWithRef(this, PO_vtReceiveBall, &entity, flags);
}

FollowFocusObjective* FollowFocusObjective::Construct(CPed* owner, CEntity* e) {
    ped = owner;
    type = OBJECTIVE_FOLLOW_FOCUS;
    numRefs = 0;
    vtable = PO_vtFollowFocus;
    focus = nullptr;
    if (e != focus) {
        focus = e;
        if (e) {
            PO_RegisterReference(e, &focus);
        }
    }
    return this;
}

FollowFocusObjective* FollowFocusObjective::Destroy(uint8_t flags) {
    // (no vtable of its own set first: FleeOnRoadOnBike's destructor too)
    if (focus) {
        PO_CleanUpOldReference(focus, &focus);
    }
    vtable = PO_vtObjective;
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

// ---------------------------------------------------------------------------------
// ObjectiveQueue

ObjectiveQueue* ObjectiveQueue::Construct(CPed* owner) {
    vtable = PO_vtQueue;
    defaultObjective = nullptr;
    action.node = nullptr;
    ped = owner;
    count = 0;
    return this;
}

void ObjectiveQueue::Destruct() {
    vtable = PO_vtQueue;
    PO_QueueClear(this);
    if (defaultObjective) {
        Release(defaultObjective);
    }
    defaultObjective = nullptr;
    PO_ActionRelease(&action);
}

ObjectiveQueue* ObjectiveQueue::Destroy(uint8_t flags) {
    Destruct();
    if (flags & 1) {
        PO_Free(this);
    }
    return this;
}

uint8_t ObjectiveQueue::Insert(Objective* objective, uint32_t index) {
    if (index > count) {
        return 0;
    }
    objective->numRefs++;
    count++;
    for (uint32_t i = count - 1; i > index; i--) {
        entries[i] = entries[i - 1];
    }
    entries[index] = objective;
    return 1;
}

Objective* ObjectiveQueue::FindOfType(int kind) {
    for (uint32_t i = 0; i < count; i++) {
        if (entries[i]->type == kind) {
            return entries[i];
        }
    }
    return nullptr;
}

uint8_t ObjectiveQueue::RemoveAt(int index) {
    if (count == 0) {
        return 0;
    }
    if (index < 0 || static_cast<uint32_t>(index) >= count) {
        return 0;
    }
    PO_QueueNotifyRemoved(this, entries[index]->type);
    Release(entries[index]);
    PO_ArrayRemoveAt(entries, index);
    return 1;
}

void ObjectiveQueue::RemoveAllOfType(int kind) {
    for (uint32_t i = 0; i < count; i++) {
        // (the one moved into the removed one's place is skipped)
        if (entries[i]->type == kind && count != 0 && static_cast<int>(i) >= 0 && i < count) {
            PO_QueueNotifyRemoved(this, entries[i]->type);
            Release(entries[i]);
            PO_ArrayRemoveAt(entries, i);
        }
    }
}

uint8_t ObjectiveQueue::RemoveFirst() {
    if (count == 0) {
        return 0;
    }
    PO_QueueNotifyRemoved(this, entries[0]->type);
    Release(entries[0]);
    PO_ArrayRemoveAt(entries, 0);
    return 1;
}

void ObjectiveQueue::SetDefault(Objective* objective) {
    if (defaultObjective) {
        PO_QueueNotifyRemoved(this, defaultObjective->type);
        Release(defaultObjective);
    }
    defaultObjective = objective;
    if (objective) {
        objective->numRefs++;
    }
}

Objective* ObjectiveQueue::GetCurrent() {
    return count != 0 ? entries[0] : defaultObjective;
}

void ObjectiveQueue::Clear() {
    while (count != 0) {
        PO_QueueNotifyRemoved(this, entries[0]->type);
        Release(entries[0]);
        for (uint32_t i = 0; i < count - 1; i++) {
            entries[i] = entries[i + 1];
        }
        count--;
    }
}

// Whether two objectives are the same attack or prop breaking (not queued twice)
static bool SameTarget(Objective* a, Objective* b) {
    Objective* attackA = a->type == OBJECTIVE_ATTACK_PED ? a : nullptr;
    Objective* attackB = b && b->type == OBJECTIVE_ATTACK_PED ? b : nullptr;
    if (attackA && attackB &&
        static_cast<AttackPedObjective*>(attackA)->target == static_cast<AttackPedObjective*>(attackB)->target) {
        return true;
    }
    Objective* breakA = a->type == OBJECTIVE_BREAK_PROP ? a : nullptr;
    Objective* breakB = b && b->type == OBJECTIVE_BREAK_PROP ? b : nullptr;
    return breakA && breakB &&
           static_cast<BreakPropObjective*>(breakA)->prop == static_cast<BreakPropObjective*>(breakB)->prop;
}

uint8_t ObjectiveQueue::Append(Objective* objective) {
    int32_t refs = objective->numRefs;
    objective->numRefs = refs + 1;
    if (count != 0 && SameTarget(objective, entries[count - 1])) {
        objective->numRefs = refs;
        if (refs <= 0) {
            reinterpret_cast<void*(__thiscall*)(Objective*, int)>(static_cast<void*>(objective->vtable[0]))(objective, 1);
        }
        return 1;
    }
    if (count < 20) {
        entries[count] = objective;
        count++;
        return 1;
    }
    objective->numRefs = refs;
    if (refs <= 0) {
        reinterpret_cast<void*(__thiscall*)(Objective*, int)>(static_cast<void*>(objective->vtable[0]))(objective, 1);
    }
    return 0;
}

uint8_t ObjectiveQueue::Prepend(Objective* objective) {
    int32_t refs = objective->numRefs;
    objective->numRefs = refs + 1;
    if (count != 0 && SameTarget(objective, entries[0])) {
        objective->numRefs = refs;
        if (refs <= 0) {
            reinterpret_cast<void*(__thiscall*)(Objective*, int)>(static_cast<void*>(objective->vtable[0]))(objective, 1);
        }
        return 1;
    }
    if (count > 19) {
        objective->numRefs = refs;
        if (refs <= 0) {
            reinterpret_cast<void*(__thiscall*)(Objective*, int)>(static_cast<void*>(objective->vtable[0]))(objective, 1);
        }
        return 0;
    }
    count++;
    for (uint32_t i = count - 1; i != 0;) {
        i--;
        entries[i + 1] = entries[i];
    }
    entries[0] = objective;
    return 1;
}

uint8_t ObjectiveQueue::Remove(Objective* objective) {
    uint32_t i = 0;
    for (; i < count; i++) {
        if (entries[i] == objective) {
            return PO_QueueRemoveAt(this, static_cast<int>(i));
        }
    }
    // (not there: al is what was left in eax, the count)
    return static_cast<uint8_t>(i);
}

void ObjectiveQueue::MoveToFront(Objective* objective) {
    if (!objective) {
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (entries[i] == objective) {
            objective->numRefs++;
            PO_QueueRemove(this, objective);
            PO_QueuePrepend(this, objective);
            Release(objective);
            return;
        }
    }
}

// ---------------------------------------------------------------------------------

#define PEDOBJ_REVERSED(name, address, fn) \
    REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)

PEDOBJ_REVERSED("Objective::Destroy", 0x00471630, &Objective::Destroy);
PEDOBJ_REVERSED("MoveToPosObjective::IsA", 0x00470610, &ObjectiveIsA::MoveToPos);
REVERSED("MoveToPosObjective::IsA (static)", 0x00470830, RE_BODY(&ObjectiveIsA::MoveToPosStatic),
         nullptr, nullptr, re::Mode::Original);
PEDOBJ_REVERSED("MoveToStimulusObjective::IsA", 0x004708A0, &ObjectiveIsA::MoveToStimulus);
PEDOBJ_REVERSED("MoveToEntityObjective::IsA", 0x00470030, &ObjectiveIsA::MoveToEntity);
PEDOBJ_REVERSED("FollowEntityObjective::IsA", 0x00470AD0, &ObjectiveIsA::FollowEntity);
PEDOBJ_REVERSED("FaceObjective::IsA", 0x00470080, &ObjectiveIsA::Face);
PEDOBJ_REVERSED("BreakPropObjective::IsA", 0x00470EC0, &ObjectiveIsA::BreakProp);
PEDOBJ_REVERSED("UsePropObjective::IsA", 0x00470F30, &ObjectiveIsA::UseProp);
PEDOBJ_REVERSED("FleeOnPathOnBikeObjective::IsA", 0x00471010, &ObjectiveIsA::FleeOnPathOnBike);
PEDOBJ_REVERSED("CoverObjective::IsA", 0x00471120, &ObjectiveIsA::Cover);
PEDOBJ_REVERSED("SlingerObjective::IsA", 0x00471270, &ObjectiveIsA::Slinger);
PEDOBJ_REVERSED("FollowFocusObjective::IsA", 0x00471C70, &ObjectiveIsA::FollowFocus);

PEDOBJ_REVERSED("MoveToPosObjective::Construct", 0x004705B0, &MoveToPosObjective::Construct);
PEDOBJ_REVERSED("MoveToPosObjective::Destroy", 0x00471590, &MoveToPosObjective::Destroy);
PEDOBJ_REVERSED("MoveToStimulusObjective::Construct", 0x004707D0, &MoveToStimulusObjective::Construct);
PEDOBJ_REVERSED("MoveToEntityObjective::Construct", 0x004708F0, &MoveToEntityObjective::Construct);
PEDOBJ_REVERSED("MoveToEntityObjective::Destroy", 0x004715E0, &MoveToEntityObjective::Destroy);
PEDOBJ_REVERSED("MoveToEntityObjective::GetTargetPos", 0x00470940, &MoveToEntityObjective::GetTargetPos);
PEDOBJ_REVERSED("FollowEntityObjective::Construct", 0x00470AA0, &FollowEntityObjective::Construct);
PEDOBJ_REVERSED("FaceHeadingObjective::Construct", 0x00470B20, &FaceHeadingObjective::Construct);
PEDOBJ_REVERSED("FacePositionObjective::Construct", 0x00470B50, &FacePositionObjective::Construct);
PEDOBJ_REVERSED("FacePositionObjective::GetHeading", 0x00470B90, &FacePositionObjective::GetHeading);
PEDOBJ_REVERSED("FaceEntityObjective::Construct", 0x00470C10, &FaceEntityObjective::Construct);
PEDOBJ_REVERSED("FaceEntityObjective::Destruct", 0x00470C60, &FaceEntityObjective::Destruct);
PEDOBJ_REVERSED("FaceEntityObjective::Destroy", 0x00471650, &FaceEntityObjective::Destroy);
PEDOBJ_REVERSED("FaceEntityObjective::GetHeading", 0x00470C90, &FaceEntityObjective::GetHeading);
PEDOBJ_REVERSED("AttackPedObjective::Construct", 0x00470D60, &AttackPedObjective::Construct);
PEDOBJ_REVERSED("AttackPedObjective::Destroy", 0x00471690, &AttackPedObjective::Destroy);
PEDOBJ_REVERSED("AttackPedObjective::IsDone", 0x004716D0, &AttackPedObjective::IsDone);
PEDOBJ_REVERSED("CoverObjective::Construct", 0x004710B0, &CoverObjective::Construct);
PEDOBJ_REVERSED("CoverObjective::Destroy", 0x00471BC0, &CoverObjective::Destroy);
PEDOBJ_REVERSED("AttackVehicleObjective::Construct", 0x00470DB0, &AttackVehicleObjective::Construct);
PEDOBJ_REVERSED("AttackVehicleObjective::Destroy", 0x00470DF0, &AttackVehicleObjective::Destroy);
PEDOBJ_REVERSED("AttackVehicleObjective::IsDone", 0x00470E30, &AttackVehicleObjective::IsDone);
PEDOBJ_REVERSED("BreakPropObjective::Construct", 0x00470E50, &BreakPropObjective::Construct);
PEDOBJ_REVERSED("BreakPropObjective::Destroy", 0x00471760, &BreakPropObjective::Destroy);
PEDOBJ_REVERSED("BreakPropObjective::IsDone", 0x004717A0, &BreakPropObjective::IsDone);
PEDOBJ_REVERSED("UsePropObjective::Construct", 0x00471810, &UsePropObjective::Construct);
PEDOBJ_REVERSED("UsePropObjective::Destroy", 0x00471880, &UsePropObjective::Destroy);
PEDOBJ_REVERSED("UsePropObjective::IsDone", 0x00470FA0, &UsePropObjective::IsDone);
PEDOBJ_REVERSED("FleeOnPathOnBikeObjective::Construct", 0x00470FC0, &FleeOnPathOnBikeObjective::Construct);
PEDOBJ_REVERSED("FleeOnPathOnBikeObjective::Destroy", 0x00471B30, &FleeOnPathOnBikeObjective::Destroy);
PEDOBJ_REVERSED("FollowPathObjective::Destroy", 0x004700D0, &FollowPathObjective::Destroy);
PEDOBJ_REVERSED("FleeOnRoadOnBikeObjective::Construct", 0x00471080, &FleeOnRoadOnBikeObjective::Construct);
PEDOBJ_REVERSED("SlingerObjective::Construct", 0x00471220, &SlingerObjective::Construct);
PEDOBJ_REVERSED("SlingerObjective::IsDone", 0x004712E0, &SlingerObjective::IsDone);
PEDOBJ_REVERSED("BallObjective::IsTimeUp", 0x004711D0, &BallObjective::IsTimeUp);
PEDOBJ_REVERSED("BallObjective::IsTimeUpF", 0x004711F0, &BallObjective::IsTimeUpF);
PEDOBJ_REVERSED("PlayCatchObjective::Construct", 0x004718C0, &BallObjective::ConstructPlayCatch);
PEDOBJ_REVERSED("PlayCatchObjective::Destroy", 0x00471950, &BallObjective::DestroyPlayCatch);
PEDOBJ_REVERSED("PassBallObjective::Construct", 0x00471990, &BallObjective::ConstructPassBall);
PEDOBJ_REVERSED("PassBallObjective::Destroy", 0x00471A20, &BallObjective::DestroyPassBall);
PEDOBJ_REVERSED("ReceiveBallObjective::Construct", 0x00471A60, &BallObjective::ConstructReceiveBall);
PEDOBJ_REVERSED("ReceiveBallObjective::Destroy", 0x00471AF0, &BallObjective::DestroyReceiveBall);
PEDOBJ_REVERSED("FollowFocusObjective::Construct", 0x00471C10, &FollowFocusObjective::Construct);
PEDOBJ_REVERSED("FollowFocusObjective::Destroy", 0x00471B80, &FollowFocusObjective::Destroy);

PEDOBJ_REVERSED("ObjectiveQueue::Construct", 0x00470160, &ObjectiveQueue::Construct);
PEDOBJ_REVERSED("ObjectiveQueue::Destruct", 0x00471350, &ObjectiveQueue::Destruct);
PEDOBJ_REVERSED("ObjectiveQueue::Destroy", 0x00471CE0, &ObjectiveQueue::Destroy);
PEDOBJ_REVERSED("ObjectiveQueue::Insert", 0x00470180, &ObjectiveQueue::Insert);
PEDOBJ_REVERSED("ObjectiveQueue::FindOfType", 0x004701D0, &ObjectiveQueue::FindOfType);
PEDOBJ_REVERSED("ObjectiveQueue::RemoveAt", 0x00470200, &ObjectiveQueue::RemoveAt);
PEDOBJ_REVERSED("ObjectiveQueue::RemoveAllOfType", 0x00470260, &ObjectiveQueue::RemoveAllOfType);
PEDOBJ_REVERSED("ObjectiveQueue::RemoveFirst", 0x004702C0, &ObjectiveQueue::RemoveFirst);
PEDOBJ_REVERSED("ObjectiveQueue::SetDefault", 0x00470300, &ObjectiveQueue::SetDefault);
PEDOBJ_REVERSED("ObjectiveQueue::GetCurrent", 0x00470340, &ObjectiveQueue::GetCurrent);
PEDOBJ_REVERSED("ObjectiveQueue::Clear", 0x00470350, &ObjectiveQueue::Clear);
PEDOBJ_REVERSED("ObjectiveQueue::Append", 0x00471390, &ObjectiveQueue::Append);
PEDOBJ_REVERSED("ObjectiveQueue::Prepend", 0x00471470, &ObjectiveQueue::Prepend);
PEDOBJ_REVERSED("ObjectiveQueue::Remove", 0x00471560, &ObjectiveQueue::Remove);
PEDOBJ_REVERSED("ObjectiveQueue::MoveToFront", 0x00471D30, &ObjectiveQueue::MoveToFront);
