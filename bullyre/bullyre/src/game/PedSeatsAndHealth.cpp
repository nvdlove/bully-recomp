// CPed (0x476D70 .. 0x478590): the swim nodes, its place in a vehicle seat, the seat
// state (+0x1570), the speed from root motion (vf20), sphere distances, the entity it
// uses (+0x16D8), the attack-response ids from its stats, health and the death action,
// and entity reference helpers.

#include <cmath>

#include "game/Ped.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
const ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

constexpr int PED_TYPE_PLAYER = 0xD;

GAME_VAR(CPed*, PH_player, 0x00C1AEA8);               // FindPlayerPed
GAME_VAR(int32_t, PH_currArea, 0x00BD1008);           // CGame::currArea
GAME_VAR(ped::CVector, PH_zeroVector, 0x00BCC564);
GAME_VAR(uint8_t*, PH_population, 0x00C2C108);
GAME_VAR(float, PH_seatOffsetY, 0x0090CF0C);          // -0.95
GAME_VAR(float, PH_seatOffsetZ, 0x0090CF08);          // -0.3
GAME_VAR(float, PH_f_0_3, 0x00911E30);
GAME_VAR(float, PH_minRootMotion, 0x0090CF10);        // 0.02
GAME_VAR(float, PH_stepSeconds, 0x00A3396C);          // 0.02
GAME_VAR(float, PH_timeStep, 0x00C1A9A4);             // CTimer::ms_fTimeStep
GAME_VAR(int32_t, PH_rootMotionOn, 0x00A339C4);
GAME_VAR(float, PH_f_1e8, 0x0090CF14);
GAME_VAR(double, PH_d_0_05, 0x00900C18);
GAME_VAR(float, PH_f_0_5, 0x009055A0);
GAME_VAR(float, PH_f_5_5, 0x0090137C);
inline void* const PH_actionContexts = reinterpret_cast<void*>(0x00D02850);
inline void* const PH_giftMgr = reinterpret_cast<void*>(0x020C3698);
inline void* const PH_itemMgr = reinterpret_cast<void*>(0x020C3CA0);

GAME_FN(uint8_t, __thiscall, PH_IsNodePlaying, 0x005F4800, void* controller, void* node, int a, int b);
GAME_FN(void*, __thiscall, PH_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(void*, __thiscall, PH_Fn8B6120, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, PH_MatrixCopy, 0x00412A10, void* self, const void* from);
GAME_FN(void*, __thiscall, PH_GetSeatInfo, 0x004CE4C0, void* seats, int seat);
GAME_FN(ped::CVector*, __cdecl, PH_Multiply3x3, 0x0041C490, ped::CVector* out, const void* matrix, const void* v);
GAME_FN(void, __thiscall, PH_SetMatrix, 0x0046DE90, CPed* ped, const void* matrix, int a);
GAME_FN(void, __thiscall, PH_ActionContextsFn5DBA60, 0x005DBA60, void* contexts, CPed* ped);
GAME_FN(void, __cdecl, PH_Fn52FF40, 0x0052FF40, int model);
GAME_FN(uint8_t, __thiscall, PH_Is4765C0, 0x004765C0, CPed* ped);
GAME_FN(void, __thiscall, PH_PopulationFn49FDF0, 0x0049FDF0, void* population, int model);
GAME_FN(void, __thiscall, PH_PopulationFn49FE40, 0x0049FE40, void* population, int model);
GAME_FN(void*, __cdecl, PH_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(Objective*, __thiscall, PH_IdleConstruct, 0x0046FC60, void* self, CPed* ped);
GAME_FN(Objective*, __thiscall, PH_WanderConstruct, 0x0046FC80, void* self, CPed* ped, float speed);
GAME_FN(Objective*, __thiscall, PH_ExitVehicleConstruct, 0x0046FC40, void* self, CPed* ped);
GAME_FN(void, __thiscall, PH_QueueSetDefault, 0x00470300, ObjectiveQueue* queue, Objective* objective);
GAME_FN(uint8_t, __thiscall, PH_QueuePrepend, 0x00471470, ObjectiveQueue* queue, Objective* objective);
GAME_FN(int, __thiscall, PH_GiftFor, 0x006A9A20, void* gifts, CPed* ped);
GAME_FN(void, __thiscall, PH_SetGiftIndex, 0x00474B40, CPed* ped, int index);
GAME_FN(void, __thiscall, PH_EntityVf20, 0x00468190, CPed* ped);
GAME_FN(void*, __cdecl, PH_Fn6C7370, 0x006C7370, void* object);
GAME_FN(void, __thiscall, PH_EntityFn465AB0, 0x00465AB0, void* entity);
GAME_FN(ped::CVector*, __cdecl, PH_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(float, __thiscall, PH_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(ped::CVector*, __thiscall, PH_GetSphereCentre, 0x00474770, CPed* ped, ped::CVector* out, int index);
GAME_FN(void, __cdecl, PH_RegisterReference, 0x004657D0, void* entity, void* ref);
GAME_FN(void, __cdecl, PH_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void*, __cdecl, PH_MatrixInvert, 0x00413770, const void* matrix, void* out);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline ped::CVector* PH_MatrixTransform(ped::CVector* out, const void* matrix, const ped::CVector* v) { return re::CallVec16<0x0041A9A0, ped::CVector*>(out, matrix, v); }
GAME_FN(void, __thiscall, PH_Fn48D020, 0x0048D020, void* self, ped::CVector* out);
GAME_FN(int, __thiscall, PH_Fn4154A0, 0x004154A0, CPed* ped, void* v);
GAME_FN(int, __cdecl, PH_Fn52A600, 0x0052A600, int kind, int a, int b, ped::CVector* pos, float radius, uintptr_t c,
        int d, int e, int f, int g, CPed* ped, int h, int i);
GAME_FN(void*, __thiscall, PH_SeatsGetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __thiscall, PH_Fn4A4310, 0x004A4310, void* self, CPed* ped);
GAME_FN(int, __thiscall, PH_GetStat, 0x00477060, CPed* ped, int stat);
GAME_FN(uint8_t*, __thiscall, PH_ItemInfo, 0x006AA7C0, void* items, int item);
GAME_FN(int, __thiscall, PH_GetAttackResponses, 0x00477E40, CPed* ped, int* out);
inline int __cdecl PH_rand() { return crt::Rand(); }
GAME_FN(uint8_t*, __cdecl, PH_Fn51B450, 0x0051B450, int id);
GAME_FN(void, __cdecl, PH_Fn5280C0, 0x005280C0, CPed* ped, void* v, float x, float y, int a, int b);
GAME_FN(uint8_t, __thiscall, PH_EntityVf22, 0x00465740, CPed* ped);
GAME_FN(uint8_t, __thiscall, PH_ActionIsValid, 0x005D80E0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PH_ActionGetNode, 0x005D80B0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PH_ActionGetNode2, 0x005D80D0, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PH_ActionGetInstance, 0x005D9C20, ped::ActionHandle* handle);
GAME_FN(void, __thiscall, PH_ActionRelease, 0x005D9C50, ped::ActionHandle* handle);
GAME_FN(void*, __thiscall, PH_ContextsFind, 0x005DB8F0, void* contexts, void* node);
GAME_FN(void*, __thiscall, PH_ContextsGetCurrent, 0x005DB960, void* contexts);
GAME_FN(void, __thiscall, PH_ContextsSetCurrent, 0x005DBDA0, void* contexts, void* context);
GAME_FN(void, __cdecl, PH_Instance5D8270, 0x005D8270, void* instance);
GAME_FN(void, __cdecl, PH_Instance5D8670, 0x005D8670, void* instance, void* node);
GAME_FN(void, __cdecl, PH_Instance5D85C0, 0x005D85C0, void* instance, int value);
GAME_FN(void, __cdecl, PH_Instance5D82A0, 0x005D82A0, void* instance, int a, int b);

// 0x476D70 (thiscall): swimming (one of the three nodes), not with +0x548
uint8_t __fastcall CPed_IsSwimming(CPed* self, re::Edx) {
    if (self->unknown548 != 0) {
        return 0;
    }
    void* actions = self->intelligence + 0x258;
    if (PH_IsNodePlaying(actions, reinterpret_cast<void*>(0x01B652B8), 1, 1) ||
        PH_IsNodePlaying(actions, reinterpret_cast<void*>(0x01B652BC), 1, 1) ||
        PH_IsNodePlaying(actions, reinterpret_cast<void*>(0x01B652C0), 1, 1)) {
        return 1;
    }
    return 0;
}

// 0x476DD0 (thiscall): put in its seat (the vehicle's matrix moved by the seat's offset),
// or the player at +0x1E94's; stopped
void __fastcall CPed_PlaceInSeat(CPed* self, re::Edx) {
    alignas(4) uint8_t matrix[0x44];  // CMatrix
    // (the offset's fourth float is uninitialised stack in the game; and with no seat the
    // whole offset is: ours are zero)
    float offset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    uint8_t* vehicle = re::PtrCast<uint8_t*>(self->vehicle);
    if (vehicle && self->inVehicleSeat != 0 &&
        (At<int32_t>(vehicle, 0x46C) != 1 || (At<uint8_t>(vehicle, 0x1BC) & 0x10) == 0)) {
        PH_MatrixCopy(matrix, PH_GetMatrix(vehicle));
        if (self->seat != -1 && PH_GetSeatInfo(vehicle + 0x1C4, self->seat)) {
            uint8_t* info = static_cast<uint8_t*>(PH_GetSeatInfo(vehicle + 0x1C4, self->seat));
            offset[0] = At<float>(info, 0x18);
            offset[1] = At<float>(info, 0x1C);
            offset[2] = At<float>(info, 0x20);
        }
        ped::CVector moved;
        PH_Multiply3x3(&moved, matrix, offset);
        ped::CVector& pos = At<ped::CVector>(matrix, 0x30);
        pos.x = pos.x + moved.x;
        pos.y = pos.y + moved.y;
        pos.z = pos.z + moved.z;
        uint8_t* vehicleMatrix = At<uint8_t*>(self->vehicle, 0x14);
        float heading;
        if (vehicleMatrix) {
            heading = static_cast<float>(std::atan2(static_cast<double>(-At<float>(vehicleMatrix, 0x10)),
                                                    static_cast<double>(At<float>(vehicleMatrix, 0x14))));
        } else {
            heading = At<float>(self->vehicle, 0x10);
        }
        self->desiredHeading = heading;
        PH_SetMatrix(self, matrix, 0);
        At<ped::CVector>(self, 0x1564) = pos;
        At<ped::CVector>(self, 0x124) = PH_zeroVector;
        return;
    }
    if (self->pedType == PED_TYPE_PLAYER && At<void*>(self, 0x1E94)) {
        PH_MatrixCopy(matrix, PH_Fn8B6120(At<void*>(self, 0x1E94)));
        offset[0] = 0.0f;
        offset[1] = PH_seatOffsetY;
        offset[2] = PH_seatOffsetZ;
        ped::CVector moved;
        PH_Multiply3x3(&moved, matrix, offset);
        ped::CVector& pos = At<ped::CVector>(matrix, 0x30);
        pos.x = pos.x + moved.x;
        pos.y = pos.y + moved.y;
        pos.z = pos.z + moved.z;
        PH_SetMatrix(self, matrix, 0);
        At<ped::CVector>(self, 0x1564) = pos;
        At<ped::CVector>(self, 0x124) = PH_zeroVector;
        At<int32_t>(self, 0x1318) = 1;
    }
}

// 0x4771F0 (thiscall): the seat state (+0x1570; not for the player's model unless 2).
// 1: the population takes it back and it idles (kind 9) or wanders, `restore` its model
// released, `gift` a gift chosen
void __fastcall CPed_Set1570(CPed* self, re::Edx, int state, int kind, int restore, int gift) {
    uint8_t s = static_cast<uint8_t>(state);
    // (the player isn't checked for null)
    if (self->modelIndex == PH_player->modelIndex && s != 2) {
        return;
    }
    uint8_t old = self->seatState;
    if (old == s) {
        return;
    }
    if (s != 1) {
        if (PH_Is4765C0(self)) {
            PH_PopulationFn49FE40(PH_population, self->modelIndex);
        }
        self->seatState = s;
        return;
    }
    if (old == 2) {
        PH_ActionContextsFn5DBA60(PH_actionContexts, self);
        if (re::Flag(restore)) {
            PH_Fn52FF40(self->modelIndex);
        }
    }
    if (!PH_Is4765C0(self)) {
        PH_PopulationFn49FDF0(PH_population, self->modelIndex);
    }
    Objective* objective;
    if (kind == 9) {
        void* memory = PH_Malloc(0x10);
        objective = memory ? PH_IdleConstruct(memory, self) : nullptr;
    } else {
        void* memory = PH_Malloc(0x14);
        objective = memory ? PH_WanderConstruct(memory, self, PH_f_0_3) : nullptr;
    }
    PH_QueueSetDefault(&self->objectives, objective);
    if (re::Flag(gift)) {
        PH_SetGiftIndex(self, PH_GiftFor(PH_giftMgr, self));
    }
    self->seatState = s;
}

// vf20 0x477310: CEntity's, then (with +0x1624 and 0xA339C4) the speed its root motion
// gives, from how far it moved since +0x1564
void __fastcall CPed_Vf20(CPed* self, re::Edx) {
    PH_EntityVf20(self);
    if (!self->rwObject) {
        return;
    }
    void* weaponObject = At<void*>(self->weapon, 0x18);
    if (weaponObject && PH_Fn6C7370(weaponObject)) {
        PH_EntityFn465AB0(self->weapon);
    }
    if (!self->unknown1624 || PH_rootMotionOn == 0) {
        return;
    }
    ped::CVector d;
    PH_VectorSub(&d, PosOf(self), &At<ped::CVector>(self, 0x1564));
    d.z = 0.0f;
    float dist = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(d.y * d.y + d.x * d.x + 0.0f * 0.0f))));
    auto vf16 = [](CPed* ped) {
        return reinterpret_cast<uint8_t*(__thiscall*)(CPed*)>(static_cast<void*>(ped->vtable[0x40 / 4]))(ped);
    };
    if (!(dist > PH_minRootMotion)) {
        At<float>(At<uint8_t*>(vf16(self), 0x14), 0x1C) = 0.0f;
        return;
    }
    float step = PH_stepSeconds * PH_timeStep;
    float speed = dist / step;
    PH_VectorNormalise(&d);
    At<float>(At<uint8_t*>(vf16(self), 0x14), 0x1C) = speed;
    At<ped::CVector>(At<uint8_t*>(vf16(self), 0x14), 0x24) = d;
    At<ped::CVector>(self->intelligence, 0) = d;
}

// vf26 0x477820: how far a point is from its nearest sphere's surface
float __fastcall CPed_Vf26(CPed* self, re::Edx, const ped::CVector* point) {
    float best = PH_f_1e8;
    for (int i = 0; i < 3; i++) {
        ped::CVector centre;
        ped::CVector d;
        ped::CVector* v = PH_VectorSub(&d, PH_GetSphereCentre(self, &centre, i), point);
        float dist = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(v->x * v->x + v->y * v->y + v->z * v->z))));
        dist = dist - At<float>(self, 0x1D7C + i * 4);
        if (best > dist) {
            best = dist;
        }
    }
    return best;
}

// 0x4778E0 (thiscall, TrackOrCondition::sub_4778E0): the entity it uses (+0x16D8,
// registered), state 2; not while +0x16D4 is negative
void __fastcall CPed_SetUsedEntity(CPed* self, re::Edx, CEntity* entity) {
    if (At<int8_t>(self, 0x16D4) < 0) {
        return;
    }
    re::Ptr32<CEntity>* ref = &At<CEntity*>(self, 0x16D8);
    At<int8_t>(self, 0x16D4) = 2;
    if (entity != *ref) {
        if (*ref) {
            PH_CleanUpOldReference(*ref, ref);
        }
        *ref = entity;
        if (entity) {
            PH_RegisterReference(entity, ref);
        }
    }
}

// 0x477930 (thiscall): the used entity turns left (its +0x2A0 below 0), or when it's
// barely turning, the ped is on its left
int __fastcall CPed_IsOnUsedEntitysLeft(CPed* self, re::Edx) {
    uint8_t* entity = At<uint8_t*>(self, 0x16D8);
    if (!entity) {
        return 0;
    }
    float turn = At<float>(entity, 0x2A0);
    float side;
    if (!(std::fabs(turn) < PH_d_0_05)) {
        side = turn;
    } else {
        alignas(4) uint8_t inverse[0x44];
        alignas(4) uint8_t local[0x44];
        At<void*>(inverse, 0x40) = nullptr;
        PH_MatrixCopy(local, PH_MatrixInvert(PH_GetMatrix(entity), inverse));
        struct { ped::CVector v; float pad; } pPad16;   // (the game's transform writes a 4th float)
        ped::CVector& p = pPad16.v;
        PH_MatrixTransform(&p, local, PosOf(self));
        side = p.x;
    }
    return 0.0f > side ? 1 : 0;
}

// 0x4779E0 (thiscall): the pending event (+0x1E30) raised: 1 .. 5 at the ped's spot
// (+0x1E34), 6 clears it and gives the player; others give back the value less one
intptr_t __fastcall CPed_RaisePendingEvent(CPed* self, re::Edx) {
    int index = At<int32_t>(self, 0x1E30) - 1;
    int kind;
    float radius;
    switch (index) {
    case 0:
        kind = 2;
        radius = PH_f_0_5;
        break;
    case 1:
        kind = 3;
        radius = PH_f_0_5;
        break;
    case 2:
        kind = 10;
        radius = PH_f_5_5;
        break;
    case 3:
        kind = 11;
        radius = PH_f_5_5;
        break;
    case 4:
        kind = 12;
        radius = PH_f_5_5;
        break;
    case 5: {
        CPed* player = PH_player;
        At<int32_t>(self, 0x1E30) = 0;
        return reinterpret_cast<intptr_t>(player);
    }
    default:
        return index;
    }
    ped::CVector pos;
    PH_Fn48D020(self->unknownA64, &pos);
    uint8_t scratch[0xC];
    int c = PH_Fn4154A0(self, scratch);
    int result = PH_Fn52A600(kind, -1, 1, &pos, radius, c, -1, 0, 0x72, 0xBC, self, 1, 5);
    At<int32_t>(self, 0x1E34) = result;
    return result;
}

// 0x477DD0 (cdecl): the vehicle's driver told to get out
void __cdecl Vehicle_DriverExit(uint8_t* vehicle) {
    if (!vehicle) {
        return;
    }
    void* seats = vehicle + 0x1C4;
    if (!PH_SeatsGetOccupant(seats, 0)) {
        return;
    }
    void* memory = PH_Malloc(0x10);
    Objective* objective =
        memory ? PH_ExitVehicleConstruct(memory, static_cast<CPed*>(PH_SeatsGetOccupant(seats, 0))) : nullptr;
    CPed* driver = static_cast<CPed*>(PH_SeatsGetOccupant(seats, 0));
    PH_QueuePrepend(&driver->objectives, objective);
}

// 0x477E30 (thiscall): its +0xD70's 0x4A4310 with itself
void __fastcall CPed_FnD70_4A4310(CPed* self, re::Edx) {
    PH_Fn4A4310(Bytes(self) + 0xD70, self);
}

// 0x477E40 (thiscall): up to 4 (id, a, weight) triples from stats 0x2D .. 0x3C: each
// whose id is set, and whose item (the 4th stat) has +2 or isn't set
int __fastcall CPed_GetAttackResponses(CPed* self, re::Edx, int* out) {
    int count = 0;
    for (int group = 0; group < 4; group++) {
        int first = 0x2D + group * 4;
        if (PH_GetStat(self, first) == -1) {
            continue;
        }
        if (PH_GetStat(self, first + 3) != -1 &&
            At<uint16_t>(PH_ItemInfo(PH_itemMgr, PH_GetStat(self, first + 3)), 2) == 0) {
            continue;
        }
        int* entry = out + count * 3;
        entry[0] = PH_GetStat(self, first);
        entry[1] = PH_GetStat(self, first + 1);
        entry[2] = PH_GetStat(self, first + 2);
        count++;
    }
    return count;
}

// 0x478090 (thiscall)
int __fastcall CPed_HasAttackResponse(CPed* self, re::Edx, int id) {
    int entries[12];
    int count = PH_GetAttackResponses(self, entries);
    for (int i = 0; i < count; i++) {
        if (entries[i * 3] == id) {
            return 1;
        }
    }
    return 0;
}

// 0x4780D0 (thiscall): one picked at random by weight (not one flagged +0x69 in area 2)
uint8_t __fastcall CPed_PickAttackResponse(CPed* self, re::Edx, int* outId, int* outA) {
    int entries[12];
    int count = PH_GetAttackResponses(self, entries);
    if (count <= 0) {
        return 0;
    }
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += entries[i * 3 + 2];
    }
    if (total == 0) {
        return 0;
    }
    int pick = (PH_rand() * (total - 1)) / 0x7FFF;
    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += entries[i * 3 + 2];
        if (pick <= sum) {
            *outId = entries[i * 3];
            *outA = entries[i * 3 + 1];
            if (PH_currArea != 2) {
                return 1;
            }
            if (PH_Fn51B450(*outId)[0x69] == 0) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x4781E0 (thiscall): with +0x1474 and +0x1758, for some kinds (+0x176E) the point at
// +0x174C passed on to 0x5280C0
void __fastcall CPed_Fn4781E0(CPed* self, re::Edx) {
    if (At<int32_t>(self, 0x1474) == 0 || At<int32_t>(self, 0x1758) == 0) {
        return;
    }
    uint8_t kind = At<uint8_t>(self, 0x176E);
    switch (kind) {
    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
    case 0x09: case 0x13: case 0x1B: case 0x1C: case 0x1D: case 0x1F: case 0x2B: case 0x2D:
    case 0x36: case 0x37: case 0x39: case 0x3A: case 0x3B: case 0x3D: {
        float a[3] = {At<float>(self, 0x174C), At<float>(self, 0x1750), At<float>(self, 0x1754)};
        // (the fourth float is uninitialised stack in the game)
        float b[4] = {a[0], a[1], a[2], 0.0f};
        PH_Fn4154A0(self, a);
        PH_Fn5280C0(self, b, a[0], a[1], 3, 2);
        break;
    }
    default:
        break;
    }
}

// (inlined, as in 0x474D20 without `force`): the death action at +0x18E8 run
void RunDeathAction(CPed* self) {
    if (self->unknown1C98 != 0 || At<int32_t>(self, 0x141C) == 0) {
        return;
    }
    ped::ActionHandle* handle = &At<ped::ActionHandle>(self, 0x18E8);
    if (!PH_ActionIsValid(handle)) {
        return;
    }
    void* context = PH_ContextsFind(PH_actionContexts, PH_ActionGetNode(handle));
    if (!context) {
        PH_ActionRelease(handle);
        return;
    }
    void* saved = PH_ContextsGetCurrent(PH_actionContexts);
    PH_ContextsSetCurrent(PH_actionContexts, context);
    void* instance = PH_ActionGetInstance(handle);
    PH_Instance5D8270(instance);
    PH_Instance5D8670(instance, PH_ActionGetNode2(handle));
    int value = reinterpret_cast<int(__thiscall*)(CPed*)>(static_cast<void*>(self->vtable[0xAC / 4]))(self);
    PH_Instance5D85C0(instance, value);
    PH_Instance5D82A0(instance, 1, 0);
    PH_ContextsSetCurrent(PH_actionContexts, saved);
    PH_ActionRelease(handle);
}

// 0x4782E0 (thiscall): not below +0x1CA8; the death action at 0
void __fastcall CPed_SetHealth(CPed* self, re::Edx, float health) {
    float minimum = At<float>(self, 0x1CA8);
    if (minimum > health) {
        health = minimum;
    }
    self->health = health;
    if (!(0.0f < health)) {
        RunDeathAction(self);
    }
}

// 0x4783F0 (thiscall)
void __fastcall CPed_TakeHealth(CPed* self, re::Edx, float amount) {
    float health = self->health - amount;
    self->health = health;
    if (0.0f > health) {
        self->health = 0.0f;
    }
    if (At<float>(self, 0x1CA8) > self->health) {
        self->health = At<float>(self, 0x1CA8);
    }
    if (!(0.0f < self->health)) {
        RunDeathAction(self);
    }
}

// vf22 0x478520
uint8_t __fastcall CPed_Vf22(CPed* self, re::Edx) {
    if (!PH_EntityVf22(self) && self->unknown548 == 0 && self->partner == nullptr) {
        return 0;
    }
    return 1;
}

// 0x478550 (thiscall, TrackOrCondition::sub_478550): a registered entity pointer set
re::Ptr32<CEntity>* __fastcall EntityPtr_Set(re::Ptr32<CEntity>* self, re::Edx, CEntity* entity) {
    if (entity != *self) {
        if (*self) {
            PH_CleanUpOldReference(*self, self);
        }
        *self = entity;
        if (entity) {
            PH_RegisterReference(entity, self);
        }
    }
    return self;
}

// 0x478590 (thiscall, TrackOrCondition::sub_478590): a counted list of registered entity
// pointers emptied
void __fastcall EntityPtrList_Clear(int32_t* self, re::Edx) {
    for (int i = 0; i < self[0]; i++) {
        re::Ptr32<CEntity>& entity = reinterpret_cast<re::Ptr32<CEntity>*>(self + 1)[i];
        if (entity) {
            PH_CleanUpOldReference(entity, &entity);
            entity = nullptr;
        }
    }
    self[0] = 0;
}

#define PEDSEAT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDSEAT_REVERSED("CPed::IsSwimming", 0x00476D70, &CPed_IsSwimming);
PEDSEAT_REVERSED("CPed::PlaceInSeat", 0x00476DD0, &CPed_PlaceInSeat);
PEDSEAT_REVERSED("CPed::Set1570_4771F0", 0x004771F0, &CPed_Set1570);
PEDSEAT_REVERSED("CPed::vf20", 0x00477310, &CPed_Vf20);
PEDSEAT_REVERSED("CPed::vf26", 0x00477820, &CPed_Vf26);
PEDSEAT_REVERSED("CPed::SetUsedEntity", 0x004778E0, &CPed_SetUsedEntity);
PEDSEAT_REVERSED("CPed::IsOnUsedEntitysLeft", 0x00477930, &CPed_IsOnUsedEntitysLeft);
PEDSEAT_REVERSED("CPed::RaisePendingEvent", 0x004779E0, &CPed_RaisePendingEvent);
PEDSEAT_REVERSED("CVehicle::DriverExit", 0x00477DD0, &Vehicle_DriverExit);
PEDSEAT_REVERSED("CPed::FnD70_4A4310", 0x00477E30, &CPed_FnD70_4A4310);
PEDSEAT_REVERSED("CPed::GetAttackResponses", 0x00477E40, &CPed_GetAttackResponses);
PEDSEAT_REVERSED("CPed::HasAttackResponse", 0x00478090, &CPed_HasAttackResponse);
PEDSEAT_REVERSED("CPed::PickAttackResponse", 0x004780D0, &CPed_PickAttackResponse);
PEDSEAT_REVERSED("CPed::Fn4781E0", 0x004781E0, &CPed_Fn4781E0);
PEDSEAT_REVERSED("CPed::SetHealth", 0x004782E0, &CPed_SetHealth);
PEDSEAT_REVERSED("CPed::TakeHealth", 0x004783F0, &CPed_TakeHealth);
PEDSEAT_REVERSED("CPed::vf22", 0x00478520, &CPed_Vf22);
PEDSEAT_REVERSED("EntityPtr::Set", 0x00478550, &EntityPtr_Set);
PEDSEAT_REVERSED("EntityPtrList::Clear", 0x00478590, &EntityPtrList_Clear);

} // namespace
