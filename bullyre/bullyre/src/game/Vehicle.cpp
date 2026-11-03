// CVehicle: construction, the state the base vehicle keeps, and its seats
// (CVehicleSeats, +0x1C4).

#include "game/Vehicle.h"

#include <cmath>

#include "game/ModelInfo.h"
#include "game/Pools.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

using veh::CVector;

// the vehicle's own, called where they are (so bullyre.ini decides which runs)
GAME_FN(void, __thiscall, Seat_Clear, 0x004CE400, CVehicleSeat* seat);
GAME_FN(void, __thiscall, Seat_Set, 0x004CE780, CVehicleSeat* seat, int32_t id04, int32_t door);
GAME_FN(void, __thiscall, Seats_Construct, 0x004CE7A0, CVehicleSeats* seats);
GAME_FN(void, __thiscall, Seats_SetUp, 0x004CEAA0, CVehicleSeats* seats);
GAME_FN(uint8_t, __thiscall, Seats_HasSeat, 0x004CE4F0, CVehicleSeats* seats, uint32_t seat);
GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(uint8_t, __thiscall, Seats_IsOccupied, 0x004CE660, CVehicleSeats* seats, uint32_t seat);
GAME_FN(uint8_t, __thiscall, Seats_IsSeatUnknown14, 0x004CE750, CVehicleSeats* seats, uint32_t seat, CPed* ped);
GAME_FN(void, __thiscall, Seats_RemoveFromSeat, 0x004CE880, CVehicleSeats* seats, uint32_t seat, CPed* ped);
GAME_FN(void, __thiscall, Vehicle_Reset, 0x004CB5B0, CVehicle* vehicle, int unused);

// CPhysical / CEntity
GAME_FN(void, __thiscall, Physical_Construct, 0x00468E90, CVehicle* entity);
GAME_FN(void, __thiscall, Physical_Destruct, 0x004690B0, CVehicle* entity);
GAME_FN(void, __thiscall, Entity_SetModelIndex, 0x00468660, CVehicle* entity, int model, int arg);
GAME_FN(void, __thiscall, Entity_DeleteRwObject, 0x004667A0, CVehicle* entity);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, CVehicle* entity);

// the vehicle's parts
GAME_FN(void, __thiscall, AutoPilot_Construct, 0x0044A070, void* autoPilot);
GAME_FN(void, __thiscall, AutoPilot_418170, 0x00418170, void* autoPilot, int arg, int flag, int arg3);
GAME_FN(void, __thiscall, AnimData_Construct, 0x006BA240, void* animData);
GAME_FN(void, __thiscall, AnimData_Destruct, 0x006B5A80, void* animData);

// the population (0xBCD080) counts the vehicles by model and by who made them
GAME_VAR(void*, population, 0x00BCD080);
GAME_FN(void, __thiscall, Population_VehicleModelSet, 0x0041C190, void* population, int model, int createdBy);
GAME_FN(void, __thiscall, Population_VehicleRemoved, 0x0041C1E0, void* population, int model, int createdBy);
GAME_FN(void, __thiscall, Population_AddRandomVehicle, 0x0041A7A0, void* population, int model);
GAME_FN(void, __thiscall, Population_RemoveRandomVehicle, 0x0041A7E0, void* population, int model);

// the radar (0xC674CC): the blips on a handle cleared
GAME_VAR(void*, radar, 0x00C674CC);
GAME_FN(void, __thiscall, Radar_ClearBlipFor, 0x0054A890, void* radar, int kind, int handle);

// the animation manager (0x20C4910)
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C2070, void* manager, void* animData, int anim, uintptr_t, void* slot, void* group,
        void* driverAnimData, uintptr_t);
GAME_FN(void, __thiscall, AnimManager_Stop, 0x006C2200, void* manager, void* animData, void* anim);
GAME_FN(void*, __thiscall, AnimGroups_Get, 0x00464EA0, void* groups, int group);

GAME_FN(void, __cdecl, Fn_41BED0, 0x0041BED0, CVehicle* vehicle);   // (status 3 set)
GAME_FN(void, __cdecl, Stats_Increment, 0x00458480, int stat, int by);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __thiscall, Ped_476180, 0x00476180, CPed* ped, int);
GAME_FN(CVector*, __cdecl, FindPlayerCoors, 0x004469E0, uintptr_t player);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(int, __cdecl, GetObjectAlpha, 0x005F2B10, void* rwObject);
GAME_FN(void, __cdecl, SetObjectAlpha, 0x005F2B00, void* rwObject, int alpha);

// the vehicle model's seat positions (thiscall on the model info: its +0x7C, +0x94)
GAME_FN(CVector*, __thiscall, VehicleModel_GetFrontSeatPos, 0x00534B30, CBaseModelInfo* mi);
GAME_FN(CVector*, __thiscall, VehicleModel_GetFrontSeatPos2, 0x00534B40, CBaseModelInfo* mi);

GAME_VAR(CPed*, playerPed, 0x00C1AEA8);       // (FindPlayerPed's)
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);     // CTimer's
GAME_VAR(float, timeStep, 0x00C1A9A4);        // CTimer's
GAME_VAR(CVector, vecUnset, 0x00BCC564);      // (what an unset position is: seats start with it)

// the model ids (from the ides) the vehicle class tells apart
GAME_VAR(int32_t, modelBF4110, 0x00BF4110);
GAME_VAR(int32_t, modelBF4118, 0x00BF4118);
GAME_VAR(int32_t, modelBF4120, 0x00BF4120);
GAME_VAR(int32_t, modelBF4124, 0x00BF4124);
GAME_VAR(int32_t, modelBF4128, 0x00BF4128);
GAME_VAR(int32_t, modelBF412C, 0x00BF412C);
GAME_VAR(int32_t, modelBF4130, 0x00BF4130);
GAME_VAR(int32_t, modelBF4134, 0x00BF4134);
GAME_VAR(int32_t, modelBF4138, 0x00BF4138);
GAME_VAR(int32_t, modelBF413C, 0x00BF413C);
GAME_VAR(int32_t, modelBF4310, 0x00BF4310);
GAME_VAR(int32_t, modelBF4314, 0x00BF4314);
GAME_VAR(int32_t, modelSiren1, 0x00BF4318);   // (the models with a siren)
GAME_VAR(int32_t, modelSiren2, 0x00BF431C);
GAME_VAR(int32_t, modelBF4454, 0x00BF4454);

// the model extras CVehicleModelInfo::CreateInstance picked last (0xFF: none)
GAME_VAR(uint8_t, lastExtra1, 0x00CE6D24);
GAME_VAR(uint8_t, lastExtra2, 0x00CE6D25);

inline void* const vehicleVtable = reinterpret_cast<void*>(0x00905C0C);

re::Ptr32<CEntity>* Ref(re::Ptr32<CPed>& ped) {
    return reinterpret_cast<re::Ptr32<CEntity>*>(&ped);
}
re::Ptr32<CEntity>* Ref(re::Ptr32<CEntity>& entity) {
    return &entity;
}

// the vehicle's handle (CPools::GetVehicleRef inlined)
int VehicleHandle(CVehicle* vehicle) {
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    int slot = int((reinterpret_cast<uint8_t*>(vehicle) - re::PtrCast<uint8_t*>(pool->objects)) / pool->elemSize);
    return (slot << 8) + pool->flags[slot];
}

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

// (a seat index past 3 is -1: the seat before the array)
CVehicleSeat& SeatOrBefore(CVehicleSeats* seats, uint32_t seat) {
    int index = seat > 3 ? -1 : int(seat);
    return *reinterpret_cast<CVehicleSeat*>(reinterpret_cast<uint8_t*>(seats) + 8 + index * int(sizeof(CVehicleSeat)));
}

} // namespace

// ---- CVehicleSeat ---------------------------------------------------------------------

void CVehicleSeat::Clear() {
    exists = 0;
    unknown04 = 0;
    door = 6;
    veh::SetRef(Ref(occupant), nullptr);
    veh::SetRef(Ref(unknown10), nullptr);
    veh::SetRef(Ref(unknown14), nullptr);
    pos = vecUnset;
    pos2 = vecUnset;
}

void CVehicleSeat::Set(int32_t id04, int32_t doorId) {
    Seat_Clear(this);
    exists = 1;
    unknown04 = id04;
    door = doorId;
}

// ---- CVehicleSeats --------------------------------------------------------------------

void CVehicleSeats::Construct() {
    for (int i = 0; i < 4; i++) {
        CVehicleSeat& seat = seats[i];
        seat.pos = {0.0f, 0.0f, 0.0f};
        seat.pos2 = {0.0f, 0.0f, 0.0f};
        seat.occupant = nullptr;
        seat.unknown10 = nullptr;
        seat.unknown14 = nullptr;
        // (what Clear does, inlined)
        seat.exists = 0;
        seat.unknown04 = 0;
        seat.door = 6;
        veh::SetRef(Ref(seat.occupant), nullptr);
        veh::SetRef(Ref(seat.unknown10), nullptr);
        veh::SetRef(Ref(seat.unknown14), nullptr);
        seat.pos = vecUnset;
        seat.pos2 = vecUnset;
    }
    numSeats = 1;
}

void CVehicleSeats::Init(CVehicle* newVehicle) {
    if (!newVehicle) {
        return;
    }
    vehicle = newVehicle;
    vehicleType = newVehicle->vehicleType;
    for (int i = 0; i < 4; i++) {
        Seat_Clear(&seats[i]);
    }
    if (vehicle->handling) {
        Seats_SetUp(this);
    }
}

void CVehicleSeats::SetUp() {
    Seat_Set(&seats[0], 0xF, 2);
    if (vehicle->vehicleType != 1) {
        Seat_Set(&seats[1], 0xB, 3);
        Seat_Set(&seats[2], 0x10, 4);
        Seat_Set(&seats[3], 0xC, 5);
    }
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[vehicle->modelIndex];
    if (!mi) {
        return;
    }
    // (the model's +0x7C / +0x94 front seat, +0x88 / +0xA0 rear seat; x mirrored on the left)
    auto modelVector = [mi](int offset) { return *reinterpret_cast<CVector*>(reinterpret_cast<uint8_t*>(mi) + offset); };
    uint8_t count = 0;
    for (int i = 0; i < 4; i++) {
        CVehicleSeat& seat = seats[i];
        if (!seat.exists) {
            continue;
        }
        count++;
        switch (i) {
        case 0:
            seat.pos = *VehicleModel_GetFrontSeatPos(mi);
            seat.pos.x = -seat.pos.x;
            seat.pos2 = *VehicleModel_GetFrontSeatPos2(mi);
            seat.pos2.x = -seat.pos2.x;
            break;
        case 1:
            seat.pos = *VehicleModel_GetFrontSeatPos(mi);
            seat.pos2 = *VehicleModel_GetFrontSeatPos2(mi);
            break;
        case 2:
            seat.pos = modelVector(0x88);
            seat.pos.x = -seat.pos.x;
            seat.pos2 = modelVector(0xA0);
            seat.pos2.x = -seat.pos2.x;
            break;
        case 3:
            seat.pos = modelVector(0x88);
            seat.pos2 = modelVector(0xA0);
            break;
        }
    }
    numSeats = count;
}

CVehicleSeat* CVehicleSeats::GetSeat(uint32_t seat) {
    if (seat > 3 || seat == 0xFFFFFFFFu) {
        return nullptr;
    }
    return seats[seat].exists ? &seats[seat] : nullptr;
}

uint8_t CVehicleSeats::HasSeat(uint32_t seat) {
    return SeatOrBefore(this, seat).exists != 0;
}

uint8_t CVehicleSeats::IsSeatTaken(uint32_t seat, int orEntering) {
    if (seat > 3 || int(seat) < 0 || int(seat) >= 4 || seat == 0xFFFFFFFFu || !seats[seat].exists) {
        return 0;
    }
    if (seats[seat].occupant) {
        return 1;
    }
    return re::Flag(orEntering) && seats[seat].unknown10 ? 1 : 0;
}

CPed* CVehicleSeats::GetOccupant(uint32_t seat) {
    if (seat > 3 || seat == 0xFFFFFFFFu || !seats[seat].exists) {
        return nullptr;
    }
    return seats[seat].occupant;
}

void CVehicleSeats::SetOccupant(uint32_t seat, CPed* ped) {
    if (seat > 3 || int(seat) < 0 || int(seat) >= 4 || seat == 0xFFFFFFFFu || !seats[seat].exists) {
        return;
    }
    re::Ptr32<CPed>& occupant = seats[seat].occupant;
    veh::SetRef(Ref(occupant), reinterpret_cast<CEntity*>(ped));
    if (ped) {
        veh::PedSeat(ped) = int32_t(seat);
        veh::PedInVehicle(ped) = 1;
    }
    // (registered a second time: SetRef did already)
    veh::RegisterReference(re::PtrCast<CEntity*>(occupant), Ref(occupant));
}

uint32_t CVehicleSeats::HasFrontPassenger() {
    return seats[1].occupant != nullptr ? 1 : 0;
}

void CVehicleSeats::SetDriver(CPed* ped) {
    if (!seats[0].exists) {
        return;
    }
    re::Ptr32<CPed>& occupant = seats[0].occupant;
    veh::SetRef(Ref(occupant), reinterpret_cast<CEntity*>(ped));
    if (ped) {
        veh::PedSeat(ped) = 0;
        veh::PedInVehicle(ped) = 1;
    }
    veh::RegisterReference(re::PtrCast<CEntity*>(occupant), Ref(occupant));
}

uint8_t CVehicleSeats::HasAnyOccupant() {
    for (int i = 0; i < 4; i++) {
        if (seats[i].occupant) {
            return 1;
        }
    }
    return 0;
}

uint8_t CVehicleSeats::IsOccupied(uint32_t seat) {
    if (seat < 4) {
        return seats[seat].occupant != nullptr;
    }
    return 0;
}

uint8_t CVehicleSeats::IsInside(CPed* ped) {
    for (int i = 0; i < 4; i++) {
        if (seats[i].occupant == ped) {
            return 1;   // (the seat's address, never null)
        }
    }
    return 0;
}

int32_t CVehicleSeats::GetSeatIndex(CPed* ped) {
    for (int i = 0; i < 4; i++) {
        if (seats[i].occupant == ped) {
            return i;
        }
    }
    return -1;
}

uint8_t CVehicleSeats::IsSeatUnknown14(uint32_t seat, CPed* ped) {
    if (!ped) {
        return 0;
    }
    return SeatOrBefore(this, seat).unknown14 == ped;
}

void CVehicleSeats::RemoveFromSeat(uint32_t seat, CPed* ped) {
    if (seat > 3) {
        seat = 0xFFFFFFFFu;
    }
    CVehicleSeat& slot = SeatOrBefore(this, seat);
    if (slot.occupant == ped) {
        veh::SetRef(Ref(slot.occupant), nullptr);
    }
    if (ped && uint32_t(veh::PedSeat(ped)) == seat) {
        // (the seat's +0x10 looked up again, -1 unless 0..3)
        CVehicleSeat& again = SeatOrBefore(this, (int(seat) < 0 || int(seat) >= 4) ? 0xFFFFFFFFu : seat);
        if (again.unknown10 != ped && !Seats_IsSeatUnknown14(this, seat, ped)) {
            veh::PedSeat(ped) = -1;
            veh::PedInVehicle(ped) = 0;
        }
    }
}

void CVehicleSeats::Remove(CPed* ped) {
    if (ped) {
        int32_t seat = veh::PedSeat(ped);
        if (seat >= 0 && seat < 4 && Seats_GetOccupant(this, uint32_t(seat)) == ped) {
            Seats_RemoveFromSeat(this, uint32_t(seat), ped);
            return;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (seats[i].occupant == ped) {
            veh::SetRef(Ref(seats[i].occupant), nullptr);
        }
    }
}

void CVehicleSeats::RemoveDriver() {
    if ((vehicle->typeAndStatus & 0xF8) != 0x28) {
        CallVirtual<void>(vehicle, 6, 4);   // SetStatus(4) unless it's status 5
    }
    if (seats[0].exists && seats[0].occupant) {
        CPed* driver = seats[0].occupant;
        *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(driver) + 0x132C) = 0;
        Ped_476180(driver, 0);
        CPed* ped = seats[0].exists ? seats[0].occupant : nullptr;
        // (the driver's seat's three pointers compared with it)
        if (seats[0].occupant == ped) {
            veh::SetRef(Ref(seats[0].occupant), nullptr);
        }
        if (seats[0].unknown10 == ped) {
            veh::SetRef(Ref(seats[0].unknown10), nullptr);
        }
        if (seats[0].unknown14 == ped) {
            veh::SetRef(Ref(seats[0].unknown14), nullptr);
        }
        if (ped && veh::PedSeat(ped) == 0) {
            veh::PedSeat(ped) = -1;
            veh::PedInVehicle(ped) = 0;
        }
    }
    if (vehicle->unknown468) {
        AnimManager_Stop(animManager, vehicle->animData, vehicle->unknown468);
        vehicle->unknown468 = nullptr;
    }
    CVehicle* v = vehicle;
    for (int i = 0; i < 4; i++) {
        if (v->seats.seats[i].occupant) {
            return;
        }
    }
    CallVirtual<void>(v, 6, 4);
    if (vehicle->vehicleType == 1) {
        // (a bike's registered pointer at +0x6BC)
        veh::SetRef(reinterpret_cast<re::Ptr32<CEntity>*>(re::PtrCast<uint8_t*>(vehicle) + 0x6BC), nullptr);
    }
}

uint8_t CVehicleSeats::IsDriver(CPed* ped) {
    if (!ped) {
        return 0;
    }
    if (!seats[0].exists) {
        return ped == nullptr;
    }
    return ped == seats[0].occupant;
}

// 0x4CE690 (CPed, thiscall): in a vehicle and seated
static uint8_t __fastcall Ped_IsSeatedInVehicle(CPed* ped, re::Edx) {
    if (!veh::PedVehicle(ped)) {
        return 0;
    }
    return veh::PedInVehicle(ped);
}

// 0x4CE6B0 (CPed, thiscall): seated in this vehicle
static uint8_t __fastcall Ped_IsSeatedIn(CPed* ped, re::Edx, CVehicle* vehicle) {
    if (veh::PedVehicle(ped) != vehicle) {
        return 0;
    }
    return veh::PedInVehicle(ped);
}

// The seats' names (0x9131BC, 3 of them)
inline const re::Ptr32<const char>* const seatKindNames = reinterpret_cast<const re::Ptr32<const char>*>(0x009131BC);
GAME_FN(int, __cdecl, RegisterEnum, 0x0073AC90, const re::Ptr32<const char>* names, int count, void* target, int, int);

// 0x4CE3C0
static const char* __cdecl GetSeatKindName(uint32_t kind) {
    if (kind < 3) {
        return seatKindNames[kind];
    }
    return reinterpret_cast<const char*>(0x00903168);   // "INVALID"
}

// 0x4CE3E0: the seat kind named `target` (0x73AC90 on the names): its index, -1 for none
static int __cdecl RegisterSeatKinds(void* target) {
    return RegisterEnum(seatKindNames, 3, target, -1, 0);
}

// ---- CVehicle -------------------------------------------------------------------------

CVehicle* CVehicle::Construct(uint8_t by) {
    Physical_Construct(this);
    Seats_Construct(&seats);
    vtable = reinterpret_cast<re::Ptr32<void>*>(vehicleVtable);
    AutoPilot_Construct(&autoPilot);
    owner = nullptr;
    unknown3D8 = -1;
    unknown3DC = 0;
    unknown444 = -1;
    unknown448 = 0;
    unknown458 = {0.0f, 0.0f, 0.0f};
    unknown450 = nullptr;
    AnimData_Construct(&animDataStorage);
    unknown35C = 0.5f;
    unknown470 = 0.0f;
    unknown370 = 0;
    createdBy = by;
    animData = &animDataStorage;
    unknown478 = 0;
    Vehicle_Reset(this, 1);
    return this;
}

void CVehicle::Destruct() {
    vtable = reinterpret_cast<re::Ptr32<void>*>(vehicleVtable);
    Population_VehicleRemoved(population, modelIndex, createdBy);
    World_Remove(this);
    Radar_ClearBlipFor(radar, 1, VehicleHandle(this));
    Radar_ClearBlipFor(radar, 0xC, VehicleHandle(this));
    for (uint32_t i = 0; i < 4; i++) {
        if (Seats_HasSeat(&seats, i) && Seats_IsOccupied(&seats, i)) {
            CallVirtual<void>(Seats_GetOccupant(&seats, i), 24);
        }
    }
    Entity_DeleteRwObject(this);
    animData = nullptr;
    AnimData_Destruct(&animDataStorage);
    veh::SetRef(&unknown450, nullptr);
    veh::SetRef(&owner, nullptr);
    veh::SetRef(&autoPilotTarget, nullptr);
    for (int i = 3; i >= 0; i--) {
        veh::SetRef(Ref(seats.seats[i].unknown14), nullptr);
        veh::SetRef(Ref(seats.seats[i].unknown10), nullptr);
        veh::SetRef(Ref(seats.seats[i].occupant), nullptr);
    }
    Physical_Destruct(this);
}

void CVehicle::Reset(int) {
    unknown29C = 1.0f;
    unknown360 = 0.0f;
    accelerationMult = 1.0f;
    health = 100000.0f;
    unknown344 = 1;
    typeAndStatus = (typeAndStatus & 0xFA) | 2;
    unknown338 = 0;
    unknown468 = nullptr;
    unknown348 = 0;
    flags33A = (flags33A & 0x14) | 0x42;
    siren = 0;
    unknown368 = 0;
    unknown36C = 0;
    unknown36E = 0;
    flags33B = 0;
    // (a random bit that's never set: a 0..1 fraction compared < 0)
    float fraction = float(double(Rand()) / 32767.0 * (1.0 - 0.0) + 0.0);
    flags33C = uint8_t((flags33C & 0xF0) | (fraction < 0.0f ? 1 : 0));
    veh::SetRef(&owner, nullptr);
    unknown350 = 0.0f;
    unknown34C = 0.0f;
    unknown354 = 1;
    unknown3DC = 0;
    unknown448 = 0;
    mission = 0;
    unknown2DB = 0;
    autoPilotFlags &= 0xEB;
    flags33C = uint8_t((flags33C & 0xDF) | 0x10);
    autoPilotTimer = timeInMs;
    unknown2ED = 0x14;
    unknown44C = 0;
    unknown44D = 0;
    veh::SetRef(&unknown450, nullptr);
    unknown454 = 0;
    unknown458 = {0.0f, 0.0f, 0.0f};
    unknown464 = 0;
}

int32_t CVehicle::GetHandlingKind() {
    uint32_t kind = *reinterpret_cast<uint32_t*>(handling + 0xCC) & 0x70000;
    switch (kind) {
    case 0:
        return 1;
    case 0x10000:
        return 2;
    case 0x20000:
        return 3;
    case 0x40000:
        return 4;
    default:
        return 0;
    }
}

int32_t* CVehicle::GetVehicleClass(int32_t* out) {
    int model = modelIndex;
    // (the kind is GetHandlingKind's, inlined)
    uint32_t flags = *reinterpret_cast<uint32_t*>(handling + 0xCC) & 0x70000;
    int kind = flags == 0 ? 1 : flags == 0x10000 ? 2 : flags == 0x20000 ? 3 : flags == 0x40000 ? 4 : 0;
    auto bikeClass = [&](int32_t fallback) -> int32_t {
        if (model == modelSiren1) {
            return 3;
        }
        if (model == modelBF4130) {
            return 8;
        }
        if (model == modelBF4454) {
            return 0xB;
        }
        if (model == modelBF412C) {
            return 0xC;
        }
        if (model == modelBF4128) {
            return 0xD;
        }
        if (model == modelBF4124 || model == modelBF4110) {
            return 0xE;
        }
        if (model == modelBF4120) {
            return 9;
        }
        return fallback;
    };
    switch (kind) {
    case 1:
        if (model == modelBF4314) {
            *out = 6;
        } else if (model == modelBF4310) {
            *out = 7;
        } else if (model == modelBF413C || model == modelBF4138 || model == modelBF4134) {
            *out = 0xA;
        } else {
            *out = 1;
        }
        return out;
    case 2:
        *out = bikeClass(2);
        return out;
    case 3:
        *out = (model == modelBF4118 || model == modelSiren2) ? 4 : 5;
        return out;
    case 4:
        if (model == modelSiren1) {
            *out = 3;
        } else if (model == modelBF4314) {
            *out = 6;
        } else if (model == modelBF4310) {
            *out = 7;
        } else if (model == modelBF413C || model == modelBF4138 || model == modelBF4134) {
            *out = 0xA;
        } else {
            *out = bikeClass(0);
        }
        return out;
    default:
        *out = 0;
        return out;
    }
}

uint8_t CVehicle::HasSirenModel() {
    int model = modelIndex;
    return model == modelSiren1 || model == modelSiren2;
}

uint8_t CVehicle::IsUnknown354Normal() {
    return unknown354 == 0 || unknown354 == 1 || unknown354 == 7;
}

void CVehicle::SetCreatedBy(int by) {
    uint8_t newBy = uint8_t(by);
    bool newRandom = newBy == 1 || newBy == 3;
    bool wasRandom = createdBy == 1 || createdBy == 3;
    if (newRandom && !wasRandom) {
        Population_AddRandomVehicle(population, modelIndex);
    } else if (!newRandom && wasRandom) {
        Population_RemoveRandomVehicle(population, modelIndex);
    }
    createdBy = newBy;
}

void CVehicle::SetUnknownAutoPilot(int arg, int flag, int arg3) {
    if (uint8_t(flag)) {
        CallVirtual<void>(this, 6, 2);
        unknown2DB = 0;
        cruiseSpeedNow = float(int(cruiseSpeed));
    } else {
        CallVirtual<void>(this, 6, 3);
    }
    AutoPilot_418170(&autoPilot, arg, flag, arg3);
}

void CVehicle::CountStat16() {
    if (!unknown44D && vehicleType == 0) {
        unknown44D = 1;
        Stats_Increment(0x16, 1);
    }
}

float CVehicle::GetUnknown470() {
    return unknown470;
}

int32_t CVehicle::GetUnknown478() {
    return unknown478;
}

void CVehicle::SetHidden(int hidden) {
    if (flags33C & 0x10) {
        return;
    }
    bool isHidden = uint8_t(hidden) != 0;
    unknown20 = isHidden ? 0 : 1;
    // (+0xD0 as an int here)
    *reinterpret_cast<int32_t*>(&unknownD0) = isHidden ? 1 : 0;
    for (uint32_t i = 0; i < 4; i++) {
        if (Seats_IsOccupied(&seats, i)) {
            CPed* ped = Seats_GetOccupant(&seats, i);
            *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(ped) + 0xD0) = isHidden ? 1 : 0;
        }
    }
}

uint8_t CVehicle::CanBeRemoved() {
    for (uint32_t i = 0; i < 4; i++) {
        if (Seats_IsOccupied(&seats, i) && veh::PedSeatState(Seats_GetOccupant(&seats, i)) == 2) {
            return 0;
        }
    }
    if (playerPed && veh::PedLastVehicle(playerPed) == this) {
        return 0;
    }
    return createdBy == 1 || createdBy == 3;
}

void CVehicle::UpdateFade() {
    CVector player = *FindPlayerCoors(0);
    int alpha = GetObjectAlpha(rwObject);
    if (flags33A & 0x20) {
        alpha -= 0x10;
        if (alpha < 0) {
            SetObjectAlpha(rwObject, 0);
            return;
        }
    } else if (alpha < 0xFF) {
        alpha += 0x10;
        if (alpha > 0xFF) {
            alpha = 0xFF;
        } else {
            CVector diff;
            const CVector* pos = matrix ? reinterpret_cast<const CVector*>(&matrix->m[12]) : &placementPos;
            CVector* d = VectorSub(&diff, pos, &player);
            float lengthSq = float(double(d->x) * d->x + double(d->y) * d->y);
            lengthSq = std::fabs(lengthSq);
            float length = float(std::sqrt(double(lengthSq)));
            if (length < 32.5f) {
                alpha = 0xFF;   // (close: at once)
            }
        }
    }
    SetObjectAlpha(rwObject, alpha);
}

void CVehicle::SetStatus(int status) {
    if (status == 4) {
        uint8_t old = typeAndStatus >> 3;
        if (old == 3 || old == 0) {
            flags33C |= 8;
        } else if (old == 2) {
            flags33C &= 0xF7;
        }
    } else if (status == 3) {
        Fn_41BED0(this);
    }
    typeAndStatus = uint8_t((typeAndStatus & 7) | (status << 3));
}

void CVehicle::SetModelIndex(int model, int arg) {
    Population_VehicleModelSet(population, model, createdBy);
    Entity_SetModelIndex(this, model, arg);
    extra1 = lastExtra1;
    extra2 = lastExtra2;
}

float CVehicle::GetHeightAboveRoad() {
    CBaseModelInfo* mi = CModelInfo::ms_modelInfoPtrs[modelIndex];
    const float* box = reinterpret_cast<const float*>(re::PtrCast<uint8_t*>(mi->colModel) + 0x10);
    return float(double(box[2]) * -1.0);
}

float CVehicle::GetUnknownVf57() {
    return 250.0f;
}

void CVehicle::PlayDriverAnim(int group, int anim) {
    if (group == -1 || anim == -1) {
        return;
    }
    if (!Seats_GetOccupant(&seats, 0)) {
        return;
    }
    auto pedAnimData = [](CPed* ped) { return *reinterpret_cast<re::Ptr32<uint8_t>*>(reinterpret_cast<uint8_t*>(ped) + 0x114); };
    void* groups = *reinterpret_cast<re::Ptr32<void>*>(pedAnimData(Seats_GetOccupant(&seats, 0)) + 0x94);
    void* animGroup = AnimGroups_Get(groups, group);
    void* driverAnimData = pedAnimData(Seats_GetOccupant(&seats, 0));
    AnimManager_Play(animManager, animData, anim, 0, &unknown468, animGroup, driverAnimData, 0);
}

// 0x4CB590 (cdecl): a fixed-point 1/128 value as a float
static float __cdecl FixedToFloat(int16_t value) {
    return float(double(int(value)) * 0.0078125);
}

// 0x4CB870 (stdcall): an impulse along the normal for a kind of collision (1: -1.1,
// 3: 0), scaled by the time step
static float __fastcall CollisionImpulse(void*, re::Edx, int kind, const CVector* a, const CVector* b, float divisor) {
    float value;
    if (kind == 1) {
        value = -1.1f;
    } else if (kind == 3) {
        value = 0.0f;
    } else {
        float dot = a->y * b->y + a->x * b->x + a->z * b->z;
        value = -dot / divisor;
    }
    return timeStep * value;
}

// ---- registration ---------------------------------------------------------------------

#define VEHICLE_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
#define VEHICLE_MEMBER(name, address, member) VEHICLE_REVERSED(name, address, member)

VEHICLE_MEMBER("CVehicleSeat::Clear", 0x004CE400, &CVehicleSeat::Clear);
VEHICLE_MEMBER("CVehicleSeat::Set", 0x004CE780, &CVehicleSeat::Set);
VEHICLE_MEMBER("CVehicleSeats::Construct", 0x004CE7A0, &CVehicleSeats::Construct);
VEHICLE_MEMBER("CVehicleSeats::Init", 0x004CE470, &CVehicleSeats::Init);
VEHICLE_MEMBER("CVehicleSeats::SetUp", 0x004CEAA0, &CVehicleSeats::SetUp);
VEHICLE_MEMBER("CVehicleSeats::GetSeat", 0x004CE4C0, &CVehicleSeats::GetSeat);
VEHICLE_MEMBER("CVehicleSeats::HasSeat", 0x004CE4F0, &CVehicleSeats::HasSeat);
VEHICLE_MEMBER("CVehicleSeats::IsSeatTaken", 0x004CE510, &CVehicleSeats::IsSeatTaken);
VEHICLE_MEMBER("CVehicleSeats::GetOccupant", 0x004CE560, &CVehicleSeats::GetOccupant);
VEHICLE_MEMBER("CVehicleSeats::SetOccupant", 0x004CE590, &CVehicleSeats::SetOccupant);
VEHICLE_MEMBER("CVehicleSeats::HasFrontPassenger", 0x004CE5F0, &CVehicleSeats::HasFrontPassenger);
VEHICLE_MEMBER("CVehicleSeats::SetDriver", 0x004CE600, &CVehicleSeats::SetDriver);
VEHICLE_MEMBER("CVehicleSeats::HasAnyOccupant", 0x004CE640, &CVehicleSeats::HasAnyOccupant);
VEHICLE_MEMBER("CVehicleSeats::IsOccupied", 0x004CE660, &CVehicleSeats::IsOccupied);
VEHICLE_MEMBER("CVehicleSeats::IsInside", 0x004CE6D0, &CVehicleSeats::IsInside);
VEHICLE_MEMBER("CVehicleSeats::GetSeatIndex", 0x004CE720, &CVehicleSeats::GetSeatIndex);
VEHICLE_MEMBER("CVehicleSeats::IsSeatUnknown14", 0x004CE750, &CVehicleSeats::IsSeatUnknown14);
VEHICLE_MEMBER("CVehicleSeats::RemoveFromSeat", 0x004CE880, &CVehicleSeats::RemoveFromSeat);
VEHICLE_MEMBER("CVehicleSeats::Remove", 0x004CE900, &CVehicleSeats::Remove);
VEHICLE_MEMBER("CVehicleSeats::Remove (thunk)", 0x004CEA60, &CVehicleSeats::Remove);
VEHICLE_MEMBER("CVehicleSeats::RemoveDriver", 0x004CE960, &CVehicleSeats::RemoveDriver);
VEHICLE_MEMBER("CVehicleSeats::IsDriver", 0x004CEA70, &CVehicleSeats::IsDriver);
VEHICLE_REVERSED("CPed::IsSeatedInVehicle", 0x004CE690, &Ped_IsSeatedInVehicle);
VEHICLE_REVERSED("CPed::IsSeatedIn", 0x004CE6B0, &Ped_IsSeatedIn);
VEHICLE_REVERSED("CVehicleSeats::GetKindName", 0x004CE3C0, &GetSeatKindName);
VEHICLE_REVERSED("CVehicleSeats::RegisterKinds", 0x004CE3E0, &RegisterSeatKinds);

VEHICLE_MEMBER("CVehicle::Construct", 0x004CB730, &CVehicle::Construct);
VEHICLE_MEMBER("CVehicle::Destruct", 0x004CD130, &CVehicle::Destruct);
VEHICLE_MEMBER("CVehicle::Reset", 0x004CB5B0, &CVehicle::Reset);
VEHICLE_MEMBER("CVehicle::GetHandlingKind", 0x004CAEE0, &CVehicle::GetHandlingKind);
VEHICLE_MEMBER("CVehicle::GetVehicleClass", 0x004CAF30, &CVehicle::GetVehicleClass);
VEHICLE_MEMBER("CVehicle::HasSirenModel", 0x004CB1F0, &CVehicle::HasSirenModel);
VEHICLE_MEMBER("CVehicle::IsUnknown354Normal", 0x004CB210, &CVehicle::IsUnknown354Normal);
VEHICLE_MEMBER("CVehicle::SetCreatedBy", 0x004CB230, &CVehicle::SetCreatedBy);
VEHICLE_MEMBER("CVehicle::SetUnknownAutoPilot", 0x004CB480, &CVehicle::SetUnknownAutoPilot);
VEHICLE_MEMBER("CVehicle::CountStat16", 0x004CB4E0, &CVehicle::CountStat16);
VEHICLE_MEMBER("CVehicle::GetUnknown470", 0x004CB510, &CVehicle::GetUnknown470);
VEHICLE_MEMBER("CVehicle::GetUnknown478", 0x004CB520, &CVehicle::GetUnknown478);
VEHICLE_MEMBER("CVehicle::SetHidden", 0x004CB530, &CVehicle::SetHidden);
VEHICLE_MEMBER("CVehicle::CanBeRemoved", 0x004CB7F0, &CVehicle::CanBeRemoved);
VEHICLE_MEMBER("CVehicle::UpdateFade", 0x004CC270, &CVehicle::UpdateFade);
VEHICLE_MEMBER("CVehicle::SetStatus", 0x004CB2F0, &CVehicle::SetStatus);
VEHICLE_MEMBER("CVehicle::SetModelIndex", 0x004CB190, &CVehicle::SetModelIndex);
VEHICLE_MEMBER("CVehicle::GetHeightAboveRoad", 0x004CCBB0, &CVehicle::GetHeightAboveRoad);
VEHICLE_MEMBER("CVehicle::GetUnknownVf57", 0x004CB1E0, &CVehicle::GetUnknownVf57);
VEHICLE_MEMBER("CVehicle::PlayDriverAnim", 0x004CB400, &CVehicle::PlayDriverAnim);
VEHICLE_REVERSED("CVehicle::FixedToFloat", 0x004CB590, &FixedToFloat);
VEHICLE_REVERSED("CVehicle::CollisionImpulse", 0x004CB870, &CollisionImpulse);

// ---- CAutomobile ----------------------------------------------------------------------

namespace {
GAME_FN(void, __thiscall, Entity_RegisterReference, 0x00450880, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __thiscall, Effect_Reset, 0x00668BA0, void* effect);
GAME_FN(void, __cdecl, Clump_FindFrames, 0x00510B40, void* rwObject, re::Ptr32<void>* frames);
GAME_FN(void, __thiscall, Vehicle_SetModelIndex, 0x004CB190, CVehicle* vehicle, int model, int arg);
GAME_FN(void, __thiscall, Automobile_FindFrames, 0x004AAAA0, CAutomobile* automobile);
GAME_FN(void, __thiscall, Automobile_ResetEffects, 0x004A84E0, CAutomobile* automobile);
GAME_FN(void*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Bike_Destruct, 0x004B9E80, CBike* bike);
GAME_FN(void, __cdecl, Vehicle_Delete, 0x004CD270, void* vehicle);   // (operator delete)
GAME_FN(uint8_t*, __cdecl, Node_6CA0A0, 0x006CA0A0, void* node);
} // namespace

void CAutomobile::SetDamage(int piece, float impulse, CEntity* entity, const CVector* pos) {
    if (!entity || !(damageImpulse < impulse)) {
        return;
    }
    damageImpulse = impulse;
    damagePiece = uint16_t(piece);
    damageEntity = entity;
    Entity_RegisterReference(entity, &damageEntity);
    damagePos = *pos;
}

void CAutomobile::ResetEffects() {
    for (int i = 0; i < 4; i++) {
        effectIds[i] = 0xFFFF;
        Effect_Reset(&effects[i * 8]);
    }
}

void CAutomobile::FindFrames() {
    for (re::Ptr32<void>& frame : frames) {
        frame = nullptr;
    }
    Clump_FindFrames(rwObject, frames);
}

void CAutomobile::SetModelIndex(int model, int arg) {
    Vehicle_SetModelIndex(this, model, arg);
    Automobile_FindFrames(this);
    void* matrixOf = Entity_GetMatrix(this);
    CallVirtual<void>(this, 39, matrixOf);
    Automobile_ResetEffects(this);
}

uint8_t CAutomobile::HasFrame(int frame) {
    return frames[frame] != nullptr;
}

void CAutomobile::ForgetEntity(CEntity* entity) {
    for (re::Ptr32<CEntity>& known : entities) {
        if (known == entity) {
            known = nullptr;
        }
    }
}

// ---- CBike ----------------------------------------------------------------------------

void CBike::SetJump(float value) {
    jump = value;
}

int32_t CBike::GetTimeSince724() {
    return int32_t(timeInMs - unknown724);
}

void CBike::AddUnknown778(float amount) {
    unknown728 = unknown724;
    unknown724 = timeInMs;
    unknown778 = unknown778 + amount;
}

void CBike::UpdateColSurface() {
    uint8_t* colModel = static_cast<uint8_t*>(CModelInfo::ms_modelInfoPtrs[modelIndex]->colModel);
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(colModel + 0x38);
    data[0x1E] = Seats_GetOccupant(&seats, 0) ? 8 : 5;
}

CBike* CBike::Destroy(uint8_t flags) {
    Bike_Destruct(this);
    if (flags & 1) {
        Vehicle_Delete(this);
    }
    return this;
}

void CBike::SetIsStatic(int isStatic) {
    uint8_t flag = uint8_t(isStatic);
    *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(this) + 0xB0) = flag;   // (CEntity's +0xB0)
    unknown180 = flag ? 10 : 0;
}

uint32_t CBike::HasDriver() {
    return Seats_GetOccupant(&seats, 0) != nullptr ? 1 : 0;
}

void CBike::GetFramePosition(int frame, CVector* out) {
    if (frames[frame]) {
        uint8_t* transform = Node_6CA0A0(frames[frame]);
        *out = *reinterpret_cast<CVector*>(transform + 0x24);
    }
}

uint8_t CBike::HasFrame(int frame) {
    return frames[frame] != nullptr;
}

void CBike::ForgetEntity(CEntity* entity) {
    for (re::Ptr32<CEntity>& known : entities) {
        if (known == entity) {
            known = nullptr;
        }
    }
}

float CBike::GetUnknown690() {
    return unknown690;
}

VEHICLE_MEMBER("CAutomobile::SetDamage", 0x004A8410, &CAutomobile::SetDamage);
VEHICLE_MEMBER("CAutomobile::ResetEffects", 0x004A84E0, &CAutomobile::ResetEffects);
VEHICLE_MEMBER("CAutomobile::FindFrames", 0x004AAAA0, &CAutomobile::FindFrames);
VEHICLE_MEMBER("CAutomobile::SetModelIndex", 0x004ADFC0, &CAutomobile::SetModelIndex);
VEHICLE_MEMBER("CAutomobile::HasFrame", 0x004A8520, &CAutomobile::HasFrame);
VEHICLE_MEMBER("CAutomobile::ForgetEntity", 0x004A8940, &CAutomobile::ForgetEntity);
VEHICLE_MEMBER("CBike::SetJump", 0x004B9C20, &CBike::SetJump);
VEHICLE_MEMBER("CBike::GetTimeSince724", 0x004BCCA0, &CBike::GetTimeSince724);
VEHICLE_MEMBER("CBike::AddUnknown778", 0x004BCCB0, &CBike::AddUnknown778);
VEHICLE_MEMBER("CBike::UpdateColSurface", 0x004C0EC0, &CBike::UpdateColSurface);
VEHICLE_MEMBER("CBike::Destroy", 0x004BD830, &CBike::Destroy);
VEHICLE_MEMBER("CBike::SetIsStatic", 0x004B9720, &CBike::SetIsStatic);
VEHICLE_MEMBER("CBike::HasDriver", 0x004B9FF0, &CBike::HasDriver);
VEHICLE_MEMBER("CBike::GetFramePosition", 0x004BA020, &CBike::GetFramePosition);
VEHICLE_MEMBER("CBike::HasFrame", 0x004B95A0, &CBike::HasFrame);
VEHICLE_MEMBER("CBike::ForgetEntity", 0x004B9CB0, &CBike::ForgetEntity);
VEHICLE_MEMBER("CBike::GetUnknown690", 0x004BA010, &CBike::GetUnknown690);
