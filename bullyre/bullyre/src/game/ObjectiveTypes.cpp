// The objectives' base (vtable 0x90C9C0: its constructor, IsA, refcount) and the simple
// kinds (enter/exit vehicle, idle, wander, follow path, passenger, rooftop attacker, bike
// brawl); the queue's filters and its first-done pop; MoveToPos / MoveToEntity's arrival
// (with its Lua callback); an entity reference's setter; a time window's test.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "game/Ped.h"
#include "reversed.h"

namespace {

inline re::Ptr32<void>* const s_objectiveVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C9C0);
inline re::Ptr32<void>* const s_enterVehicleVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C9D4);
inline re::Ptr32<void>* const s_exitVehicleVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C9E8);
inline re::Ptr32<void>* const s_idleVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090C9FC);
inline re::Ptr32<void>* const s_wanderVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090CA10);
inline re::Ptr32<void>* const s_followPathVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090CA24);
inline re::Ptr32<void>* const s_fpPassengerVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090CA38);
inline re::Ptr32<void>* const s_rooftopAttackerVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090CA4C);
inline re::Ptr32<void>* const s_bikeBrawlVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090CA60);
inline re::Ptr32<void>* const s_entityObjectiveVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090CB20);
inline re::Ptr32<void>* const s_actionObjectiveVtable = reinterpret_cast<re::Ptr32<void>*>(0x0090CB98);
inline void* const s_scripts = reinterpret_cast<void*>(0x00D02850);
inline void* const s_patrolPaths = reinterpret_cast<void*>(0x00D13730);
inline void* const s_clock = reinterpret_cast<void*>(0x020C35E8);
GAME_VAR(void*, s_pathOwner, 0x00D13CB4);
GAME_VAR(int32_t, s_clockArg, 0x020C3638);
GAME_VAR(uint8_t, s_clockHours, 0x00BCEB4B);
GAME_VAR(uint16_t, s_nextClassId, 0x00BF3830);

// the class ids (guard, id)
GAME_VAR(uint32_t, s_guardBase, 0x00C2AFA8);
GAME_VAR(uint16_t, s_idBase, 0x00C2AFA4);
GAME_VAR(uint32_t, s_guardEnterVehicle, 0x00C2AFB0);
GAME_VAR(uint16_t, s_idEnterVehicle, 0x00C2AFAC);
GAME_VAR(uint32_t, s_guardWander, 0x00C2AFB8);
GAME_VAR(uint16_t, s_idWander, 0x00C2AFB4);
GAME_VAR(uint32_t, s_guardFPPassenger, 0x00C2AFC0);
GAME_VAR(uint16_t, s_idFPPassenger, 0x00C2AFBC);
GAME_VAR(uint32_t, s_guardRooftopAttacker, 0x00C2AFC8);
GAME_VAR(uint16_t, s_idRooftopAttacker, 0x00C2AFC4);
GAME_VAR(uint32_t, s_guardBikeBrawl, 0x00C2AFD0);
GAME_VAR(uint16_t, s_idBikeBrawl, 0x00C2AFCC);
GAME_VAR(uint32_t, s_guardLocomotion, 0x00C2AFD8);
GAME_VAR(uint16_t, s_idLocomotion, 0x00C2AFD4);

// the Lua callback handles (ped::ActionHandle)
GAME_FN(uint8_t, __thiscall, Callback_IsValid, 0x005D80E0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, Callback_GetState, 0x005D9C20, ped::ActionHandle* handle);
GAME_FN(int, __thiscall, Callback_GetRef, 0x005D80D0, ped::ActionHandle* handle);
GAME_FN(int, __thiscall, Callback_GetScript, 0x005D80B0, ped::ActionHandle* handle);
GAME_FN(void, __thiscall, Callback_Release, 0x005D9C50, ped::ActionHandle* handle);
GAME_FN(void, __cdecl, Lua_Begin, 0x005D8270, void* state);
GAME_FN(void, __cdecl, Lua_PushFunction, 0x005D8670, void* state, int function);
GAME_FN(void, __cdecl, Lua_PushInt, 0x005D85C0, void* state, int value);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, void* state, int args, int results);
GAME_FN(void*, __thiscall, Scripts_GetCurrent, 0x005DB960, void* scripts);
GAME_FN(void*, __thiscall, Scripts_Find, 0x005DB8F0, void* scripts, int script);
GAME_FN(void, __thiscall, Scripts_SetCurrent, 0x005DBDA0, void* scripts, void* script);
GAME_FN(float, __cdecl, Distance2D, 0x00470110, const ped::CVector* a, const ped::CVector* b);
GAME_FN(uint8_t, __thiscall, Ped_IsAmbient, 0x004765C0, void* ped);
GAME_FN(void, __thiscall, ActionController_Stop, 0x005F40A0, void* controller);
GAME_FN(uint32_t, __thiscall, PathOwner_Count, 0x005ED0E0, void* owner);
GAME_FN(void*, __thiscall, PatrolPath_Get, 0x005ED3A0, void* owner, int path);
GAME_FN(uint32_t, __thiscall, Path_GetNumNodes, 0x005EC670, void* path);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(int, __thiscall, Clock_GetTime, 0x006A8CC0, void* clock, int arg);
GAME_FN(void, __thiscall, Queue_RemoveAllOfType, 0x00470260, ObjectiveQueue* queue, int type);
GAME_FN(void, __thiscall, PatrolPaths_ReleasePed, 0x005EC0E0, void* paths, void* ped);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(Objective*, __thiscall, WanderObjective_Construct_, 0x0046FC80, void* self, void* ped, float speed);
GAME_FN(void, __thiscall, Queue_SetDefault, 0x00470300, ObjectiveQueue* queue, Objective* objective);
GAME_FN(Objective*, __thiscall, Queue_GetCurrent, 0x00470340, ObjectiveQueue* queue);
GAME_FN(void, __thiscall, Queue_NotifyRemoved, 0x0046FB30, ObjectiveQueue* queue, int type);
GAME_FN(void, __thiscall, ObjectiveArray_RemoveAt, 0x0046FF90, void* array, uint32_t index);

template <typename T>
re::Field<T>& Field(void* object, size_t offset) {
    return *reinterpret_cast<re::Field<T>*>(reinterpret_cast<uint8_t*>(object) + offset);
}

uint16_t ClassId(uint32_t& guard, uint16_t& id) {
    if (!(guard & 1)) {
        guard |= 1;
        id = s_nextClassId;
        s_nextClassId = uint16_t(s_nextClassId + 1);
    }
    return id;
}

bool IsBase(uint16_t id) {
    return id == ClassId(s_guardBase, s_idBase);
}

// an objective's refcount dropped (its deleting destructor at 0)
void Release(Objective* objective) {
    if (--objective->numRefs <= 0) {
        reinterpret_cast<void(__thiscall*)(Objective*, int)>(static_cast<void*>(objective->vtable[0]))(objective, 1);
    }
}

// the callback called with one int
void CallCallback(ped::ActionHandle* handle, int value) {
    void* state = Callback_GetState(handle);
    Lua_Begin(state);
    Lua_PushFunction(state, Callback_GetRef(handle));
    Lua_PushInt(state, value);
    Lua_Call(state, 1, 0);
}

// 0x46FA90 (thiscall)
Objective* __fastcall ObjectiveConstruct(Objective* self, re::Edx, int type, CPed* ped) {
    self->vtable = s_objectiveVtable;
    self->ped = ped;
    self->type = type;
    self->numRefs = 0;
    return self;
}

// 0x46FAB0 vf1 (thiscall)
uint8_t __fastcall ObjectiveIsA(Objective*, re::Edx, uint16_t id) {
    return IsBase(id);
}

// 0x46FAF0 (thiscall)
void __fastcall ObjectiveDestruct(Objective* self, re::Edx) {
    self->vtable = s_objectiveVtable;
}

// 0x46FB00 (thiscall)
void __fastcall ObjectiveAddRef(Objective* self, re::Edx) {
    self->numRefs++;
}

// 0x46FB10 (thiscall)
void __fastcall ObjectiveRelease(Objective* self, re::Edx) {
    Release(self);
}

// 0x46FB30 (thiscall): the queue's callback told a type was removed
void __fastcall QueueNotifyRemoved(ObjectiveQueue* self, re::Edx, int type) {
    if (Callback_IsValid(&self->action)) {
        CallCallback(&self->action, type);
    }
}

// 0x46FB80 (MoveToPos vf4: its target) and 0x46FB90 (+0x18): too short to hook
ped::CVector* MoveToPosGetTarget(MoveToPosObjective* self) {
    return &self->target;
}
uint8_t MoveObjectiveGet18(MoveObjective* self) {
    return self->unknown18;
}
RE_UNHOOKED_FN("MoveToPosObjective::GetTarget", 0x0046FB80, &MoveToPosGetTarget);
// 0x0046FB90: [8a 41 18 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0046FB90(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint8_t*>(self + 0x18);
}
RE_UNHOOKED_FN("MoveObjective::Get18", 0x0046FB90, &Tiny_0046FB90);

// 0x46FBA0 (thiscall): EnterVehicle (type 5)
Objective* __fastcall EnterVehicleConstruct(Objective* self, re::Edx, CPed* ped, CEntity* vehicle) {
    self->ped = ped;
    self->type = 5;
    self->numRefs = 0;
    self->vtable = s_enterVehicleVtable;
    Field<CEntity*>(self, 0x10) = vehicle;
    return self;
}

// 0x46FBD0 EnterVehicle vf1
uint8_t __fastcall EnterVehicleIsA(Objective*, re::Edx, uint16_t id) {
    if (id == ClassId(s_guardEnterVehicle, s_idEnterVehicle)) {
        return 1;
    }
    return IsBase(id);
}

// 0x46FC40 (thiscall): ExitVehicle (type 6)
Objective* __fastcall ExitVehicleConstruct(Objective* self, re::Edx, CPed* ped) {
    self->ped = ped;
    self->type = 6;
    self->numRefs = 0;
    self->vtable = s_exitVehicleVtable;
    return self;
}

// 0x46FC60 (thiscall): Idle (type 9)
Objective* __fastcall IdleConstruct(Objective* self, re::Edx, CPed* ped) {
    self->ped = ped;
    self->type = 9;
    self->numRefs = 0;
    self->vtable = s_idleVtable;
    return self;
}

// 0x46FC80 (thiscall): Wander (type 10) at a speed
Objective* __fastcall WanderConstruct(Objective* self, re::Edx, CPed* ped, float speed) {
    Field<float>(self, 0x10) = speed;
    self->ped = ped;
    self->type = 10;
    self->numRefs = 0;
    self->vtable = s_wanderVtable;
    return self;
}

// 0x46FCB0 Wander vf1
uint8_t __fastcall WanderIsA(Objective*, re::Edx, uint16_t id) {
    if (id == ClassId(s_guardWander, s_idWander)) {
        return 1;
    }
    return IsBase(id);
}

// 0x46FD20 (thiscall): a patrol path node reference {path, node} that exists
uint8_t __fastcall PathNodeRefIsValid(int32_t* self, re::Edx) {
    if (self[0] < 0) {
        return 0;
    }
    uint32_t path = uint32_t(self[0]);
    if (path >= PathOwner_Count(s_pathOwner)) {
        return 0;
    }
    int nodes = int(Path_GetNumNodes(PatrolPath_Get(s_pathOwner, int(path))));
    return self[1] >= 0 && self[1] < nodes;
}

// 0x46FD70 (thiscall): FollowPath (type 12; 0x40 bytes)
Objective* __fastcall FollowPathConstruct(Objective* self, re::Edx, CPed* ped) {
    self->ped = ped;
    self->type = 12;
    self->vtable = s_followPathVtable;
    self->numRefs = 0;
    Field<float>(self, 0x18) = 0.0f;
    Field<float>(self, 0x1C) = 0.0f;
    Field<int32_t>(self, 0x2C) = 0;
    Field<float>(self, 0x20) = 0.0f;
    Field<int32_t>(self, 0x30) = 0;
    Field<int32_t>(self, 0x34) = 0;
    Field<int32_t>(self, 0x10) = -1;
    Field<int32_t>(self, 0x14) = 0;
    Field<int32_t>(self, 0x24) = 1;
    Field<int32_t>(self, 0x28) = 1;
    Field<int32_t>(self, 0x38) = 0;
    Field<float>(self, 0x3C) = 0.3f;
    return self;
}

// 0x46FDD0 (thiscall): FPPassenger (type 0x1D)
Objective* __fastcall FPPassengerConstruct(Objective* self, re::Edx, CPed* ped, int arg) {
    self->ped = ped;
    self->type = 0x1D;
    self->numRefs = 0;
    self->vtable = s_fpPassengerVtable;
    Field<int32_t>(self, 0x10) = arg;
    return self;
}

// 0x46FE00 FPPassenger vf1
uint8_t __fastcall FPPassengerIsA(Objective*, re::Edx, uint16_t id) {
    if (id == ClassId(s_guardFPPassenger, s_idFPPassenger)) {
        return 1;
    }
    return IsBase(id);
}

// 0x46FE70 (thiscall): RooftopAttacker (type 0x1E)
Objective* __fastcall RooftopAttackerConstruct(Objective* self, re::Edx, CPed* ped) {
    self->ped = ped;
    self->type = 0x1E;
    self->numRefs = 0;
    self->vtable = s_rooftopAttackerVtable;
    return self;
}

// 0x46FE90 RooftopAttacker vf1
uint8_t __fastcall RooftopAttackerIsA(Objective*, re::Edx, uint16_t id) {
    if (id == ClassId(s_guardRooftopAttacker, s_idRooftopAttacker)) {
        return 1;
    }
    return IsBase(id);
}

// 0x46FF00 (thiscall): BikeBrawl (type 0x21)
Objective* __fastcall BikeBrawlConstruct(Objective* self, re::Edx, CPed* ped) {
    self->ped = ped;
    self->type = 0x21;
    self->numRefs = 0;
    self->vtable = s_bikeBrawlVtable;
    return self;
}

// 0x46FF20 BikeBrawl vf1
uint8_t __fastcall BikeBrawlIsA(Objective*, re::Edx, uint16_t id) {
    if (id == ClassId(s_guardBikeBrawl, s_idBikeBrawl)) {
        return 1;
    }
    return IsBase(id);
}

// the queue's entries with their count (+0x50 of the array: the queue's +0x58)
struct ObjectiveArray {
    re::Ptr32<Objective> entries[20];  // +0
    uint32_t count;          // +0x50

    void RemoveAt(uint32_t index);
};
static_assert(sizeof(ObjectiveArray) == 0x54);

// 0x46FF90 (thiscall)
void ObjectiveArray::RemoveAt(uint32_t index) {
    for (uint32_t i = index; i < count - 1; i++) {
        entries[i] = entries[i + 1];
    }
    count--;
}

// 0x46FFC0: the locomotion objectives' IsA
uint8_t __cdecl LocomotionIsA(uint16_t id) {
    if (id == ClassId(s_guardLocomotion, s_idLocomotion)) {
        return 1;
    }
    return IsBase(id);
}

// (0x470400 / 0x470470) the entries the filter picks removed (their type announced,
// released). (An index out of range still steps back)
template <bool removeWhen>
void RemoveWhere(ObjectiveQueue* self, uint8_t(__cdecl* filter)(Objective*)) {
    for (uint32_t i = 0; i < self->count; i++) {
        if ((filter(self->entries[i]) != 0) != removeWhen) {
            continue;
        }
        if (self->count > 0 && int(i) >= 0 && i < self->count) {
            Queue_NotifyRemoved(self, self->entries[i]->type);
            Release(self->entries[i]);
            ObjectiveArray_RemoveAt(self->entries, i);
        }
        i--;
    }
}

// 0x470400 (thiscall): those the filter rejects
void __fastcall QueueRemoveUnless(ObjectiveQueue* self, re::Edx, uint8_t(__cdecl* filter)(Objective*)) {
    RemoveWhere<false>(self, filter);
}

// 0x470470 (thiscall): those the filter accepts
void __fastcall QueueRemoveIf(ObjectiveQueue* self, re::Edx, uint8_t(__cdecl* filter)(Objective*)) {
    RemoveWhere<true>(self, filter);
}

// 0x4704E0 (thiscall): the first ones popped while done (vf2)
void __fastcall QueuePopDone(ObjectiveQueue* self, re::Edx) {
    Objective* current = Queue_GetCurrent(self);
    while (current) {
        if (!reinterpret_cast<uint8_t(__thiscall*)(Objective*)>(static_cast<void*>(current->vtable[2]))(current) || self->count == 0) {
            return;
        }
        int type = self->entries[0]->type;
        if (Callback_IsValid(&self->action)) {
            CallCallback(&self->action, type);
        }
        Release(self->entries[0]);
        for (uint32_t i = 0; self->count - 1 != 0 && i < self->count - 1; i++) {
            self->entries[i] = self->entries[i + 1];
        }
        self->count--;
        current = Queue_GetCurrent(self);
    }
}

// (MoveToPos / MoveToEntity's arrival) the callback, with the ped's vf43, in its script
void ArrivalCallback(ped::ActionHandle* handle, CPed* pedPtr) {
    void* saved = Scripts_GetCurrent(s_scripts);
    void* script = Scripts_Find(s_scripts, Callback_GetScript(handle));
    if (script) {
        Scripts_SetCurrent(s_scripts, script);
        void* state = Callback_GetState(handle);
        Lua_Begin(state);
        Lua_PushFunction(state, Callback_GetRef(handle));
        int value = reinterpret_cast<int(__thiscall*)(CPed*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(pedPtr))[43]))(pedPtr);
        Lua_PushInt(state, value);
        Lua_Call(state, 1, 0);
        Scripts_SetCurrent(s_scripts, saved);
    }
    Callback_Release(handle);
}

// (both) once there: the callback (if the ped's +0x2E0's +0x1C is 0), and with
// stopWhenThere an ambient ped stops its actions and leaves the world
uint8_t Arrive(Objective* self, ped::ActionHandle* action, uint8_t stopWhenThere) {
    uint8_t* pedPtr = re::PtrCast<uint8_t*>(self->ped);
    if (Callback_IsValid(action) && 0.0f == Field<float>(Field<uint8_t*>(pedPtr, 0x2E0), 0x1C)) {
        ArrivalCallback(action, self->ped);
    }
    if (!stopWhenThere || !Ped_IsAmbient(self->ped)) {
        return 0;
    }
    ActionController_Stop(Field<uint8_t*>(pedPtr, 0x2E0) + 0x258);
    Field<uint32_t>(pedPtr, 0x78) = 1;
    return 1;
}

// 0x470690 MoveToPos vf2 (thiscall)
uint8_t __fastcall MoveToPosUpdate(MoveToPosObjective* self, re::Edx) {
    if (!Callback_IsValid(&self->action) && !self->stopWhenThere) {
        return 0;
    }
    CEntity* pedEntity = re::PtrCast<CEntity*>(self->ped);
    float dist = Distance2D(reinterpret_cast<ped::CVector*>(&pedEntity->GetPosition()), &self->target);
    if (!(double(self->radius) + 0.5 > dist)) {
        return 0;
    }
    return Arrive(self, &self->action, self->stopWhenThere);
}

// 0x470960 MoveToEntity vf2 (thiscall)
uint8_t __fastcall MoveToEntityUpdate(MoveToEntityObjective* self, re::Edx) {
    if (!Callback_IsValid(&self->action) && !self->stopWhenThere) {
        return 0;
    }
    CEntity* pedEntity = re::PtrCast<CEntity*>(self->ped);
    ped::CVector* pedPos = reinterpret_cast<ped::CVector*>(&pedEntity->GetPosition());
    ped::CVector* target = reinterpret_cast<ped::CVector*(__thiscall*)(Objective*)>(static_cast<void*>(self->vtable[4]))(self);
    float dist = Distance2D(pedPos, target);
    if (!(double(self->radius) + 0.5 > dist)) {
        return 0;
    }
    return Arrive(self, &self->action, self->stopWhenThere);
}

// 0x471190 (thiscall): the destructor's body of an objective with an entity (+0x14) and
// a callback (+0x68)
void __fastcall ActionObjectiveDestruct(Objective* self, re::Edx) {
    self->vtable = s_actionObjectiveVtable;
    Callback_Release(&Field<ped::ActionHandle>(self, 0x68));
    self->vtable = s_entityObjectiveVtable;
    re::Ptr32<CEntity>& entity = Field<CEntity*>(self, 0x14);
    if (entity) {
        CleanUpOldReference(entity, &entity);
    }
    self->vtable = s_objectiveVtable;
}

// 0x471300 (thiscall): the entity (references moved) and the value
void __fastcall EntityRefSet(ped::EntityRef* self, re::Edx, CEntity* entity, int32_t value) {
    if (entity != self->entity) {
        if (self->entity) {
            CleanUpOldReference(self->entity, &self->entity);
        }
        self->entity = entity;
        if (entity) {
            RegisterReference(entity, &self->entity);
        }
    }
    self->value = value;
}

bool InWindow(int from, int to, int now) {
    if (from < to) {
        return from <= now && now < to;
    }
    if (from == to) {
        return false;
    }
    return now < to || now >= from;
}

// 0x471D90 (thiscall): now in the window (the clock's time with +0xD, else the hour;
// wrapping past midnight)
uint8_t __fastcall TimeWindowContains(uint8_t* self, re::Edx) {
    if (self[0xD]) {
        int now = Clock_GetTime(s_clock, s_clockArg);
        return InWindow(Field<int32_t>(self, 0x14), Field<int32_t>(self, 0x18), now);
    }
    return InWindow(Field<int32_t>(self, 0x1C), Field<int32_t>(self, 0x20), s_clockHours);
}

// 0x471E00 (thiscall)
void __fastcall SetByte1(uint8_t* self, re::Edx, uint8_t value) {
    self[1] = value;
}

// 0x471E10: the objective if it's a FollowPath (type 12)
Objective* __cdecl AsFollowPath(Objective* objective) {
    if (!objective) {
        return nullptr;
    }
    return objective->type == 12 ? objective : nullptr;
}

// 0x471E70 (thiscall): the path left: the ped's actions stopped (unless its +0x1D24's
// +0x70 is 3..15), its path objectives removed, a wander by default
void __fastcall FollowPathAbandon(Objective* self, re::Edx) {
    uint8_t* pedPtr = re::PtrCast<uint8_t*>(self->ped);
    if (!pedPtr) {
        return;
    }
    uint8_t* x = Field<uint8_t*>(pedPtr, 0x1D24);
    if (!x || Field<int32_t>(x, 0x70) <= 2 || Field<int32_t>(x, 0x70) >= 0x10) {
        ActionController_Stop(Field<uint8_t*>(pedPtr, 0x2E0) + 0x290);
        ActionController_Stop(Field<uint8_t*>(re::PtrCast<uint8_t*>(self->ped), 0x2E0) + 0x258);
    }
    Queue_RemoveAllOfType(reinterpret_cast<ObjectiveQueue*>(re::PtrCast<uint8_t*>(self->ped) + 0x5F0), 12);
    PatrolPaths_ReleasePed(s_patrolPaths, self->ped);
    void* mem = Malloc(0x14);
    Objective* wander = mem ? WanderObjective_Construct_(mem, self->ped, 0.3f) : nullptr;
    Queue_SetDefault(reinterpret_cast<ObjectiveQueue*>(re::PtrCast<uint8_t*>(self->ped) + 0x5F0), wander);
    Field<uint8_t>(self->ped, 0x1C99) = 1;
    Field<uint32_t>(self->ped, 0xE4) = 0;
    Field<uint32_t>(self->ped, 0x1398) = 0;
}

} // namespace

#define OBJT_C05_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

OBJT_C05_REVERSED("Objective::Objective", 0x0046FA90, &ObjectiveConstruct);
OBJT_C05_REVERSED("Objective::IsA", 0x0046FAB0, &ObjectiveIsA);
OBJT_C05_REVERSED("Objective::~Objective", 0x0046FAF0, &ObjectiveDestruct);
OBJT_C05_REVERSED("Objective::AddRef", 0x0046FB00, &ObjectiveAddRef);
OBJT_C05_REVERSED("Objective::Release", 0x0046FB10, &ObjectiveRelease);
OBJT_C05_REVERSED("ObjectiveQueue::NotifyRemoved", 0x0046FB30, &QueueNotifyRemoved);
OBJT_C05_REVERSED("EnterVehicleObjective::EnterVehicleObjective", 0x0046FBA0, &EnterVehicleConstruct);
OBJT_C05_REVERSED("EnterVehicleObjective::IsA", 0x0046FBD0, &EnterVehicleIsA);
OBJT_C05_REVERSED("ExitVehicleObjective::ExitVehicleObjective", 0x0046FC40, &ExitVehicleConstruct);
OBJT_C05_REVERSED("IdleObjective::IdleObjective", 0x0046FC60, &IdleConstruct);
OBJT_C05_REVERSED("WanderObjective::WanderObjective", 0x0046FC80, &WanderConstruct);
OBJT_C05_REVERSED("WanderObjective::IsA", 0x0046FCB0, &WanderIsA);
OBJT_C05_REVERSED("PathNodeRef::IsValid", 0x0046FD20, &PathNodeRefIsValid);
OBJT_C05_REVERSED("FollowPathObjective::FollowPathObjective", 0x0046FD70, &FollowPathConstruct);
OBJT_C05_REVERSED("FPPassengerObjective::FPPassengerObjective", 0x0046FDD0, &FPPassengerConstruct);
OBJT_C05_REVERSED("FPPassengerObjective::IsA", 0x0046FE00, &FPPassengerIsA);
OBJT_C05_REVERSED("RooftopAttackerObjective::RooftopAttackerObjective", 0x0046FE70, &RooftopAttackerConstruct);
OBJT_C05_REVERSED("RooftopAttackerObjective::IsA", 0x0046FE90, &RooftopAttackerIsA);
OBJT_C05_REVERSED("BikeBrawlObjective::BikeBrawlObjective", 0x0046FF00, &BikeBrawlConstruct);
OBJT_C05_REVERSED("BikeBrawlObjective::IsA", 0x0046FF20, &BikeBrawlIsA);
REVERSED("ObjectiveArray::RemoveAt", 0x0046FF90, re::MemberAddress(&ObjectiveArray::RemoveAt), nullptr, nullptr, re::Mode::Original);
OBJT_C05_REVERSED("LocomotionObjective::IsA", 0x0046FFC0, &LocomotionIsA);
OBJT_C05_REVERSED("ObjectiveQueue::RemoveUnless", 0x00470400, &QueueRemoveUnless);
OBJT_C05_REVERSED("ObjectiveQueue::RemoveIf", 0x00470470, &QueueRemoveIf);
OBJT_C05_REVERSED("ObjectiveQueue::PopDone", 0x004704E0, &QueuePopDone);
OBJT_C05_REVERSED("MoveToPosObjective::Update", 0x00470690, &MoveToPosUpdate);
OBJT_C05_REVERSED("MoveToEntityObjective::Update", 0x00470960, &MoveToEntityUpdate);
OBJT_C05_REVERSED("ActionObjective::Destruct", 0x00471190, &ActionObjectiveDestruct);
OBJT_C05_REVERSED("EntityRef::Set", 0x00471300, &EntityRefSet);
OBJT_C05_REVERSED("TimeWindow::Contains", 0x00471D90, &TimeWindowContains);
OBJT_C05_REVERSED("SetByte1", 0x00471E00, &SetByte1);
OBJT_C05_REVERSED("AsFollowPathObjective", 0x00471E10, &AsFollowPath);
OBJT_C05_REVERSED("FollowPathObjective::Abandon", 0x00471E70, &FollowPathAbandon);
