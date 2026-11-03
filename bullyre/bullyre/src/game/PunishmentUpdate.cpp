// The punishment system's update (the trouble meter, 0xC0F78C): points for crimes
// seen, points for being where one shouldn't (curfew, class time, off limits), and
// authority figures sent after a ped with enough points. (0x44F480 .. 0x44FFB0)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"

namespace {

using re::At;

// A punishment level (the points it gives, the most it takes a ped to, the HUD text)
struct PunishmentLevel {
    uint8_t unknown00[0x1C];
    int32_t maxPoints;      // +0x1C
    uint8_t unknown20[0x8];
    int32_t points;         // +0x28
    re::Ptr32<const char> text;       // +0x2C
};
static_assert(offsetof(PunishmentLevel, maxPoints) == 0x1C);
static_assert(offsetof(PunishmentLevel, points) == 0x28);
static_assert(offsetof(PunishmentLevel, text) == 0x2C);

struct Punishment {
    uint8_t disabled;              // +0
    uint8_t onlyPlayer;            // +1 (only the player gets authority figures sent)
    uint8_t unknown2;              // +2 (the players' allies get the points of what they see)
    uint8_t unknown3;
    int32_t lastPed;               // +4 the ped slot the round-robin update got to
    uint8_t unknown8[0xC];
    re::Ptr32<void> schoolArea;              // +0x14 trigger "AMB_SCHOOL_AREA" (found when first needed)
    re::Ptr32<void> boysDorm;                // +0x18 "PUN_BDORM"
    re::Ptr32<void> girlsDorm;               // +0x1C "PUN_GDORM"
    re::Ptr32<void> asylum;                  // +0x20 "TINDUST_ASYLUM"
    uint8_t unknown24[0xC];
    int32_t alertLevel1;           // +0x30 points over which authority figures come
    int32_t alertLevel2;           // +0x34
    int32_t alertLevel3;           // +0x38
    int32_t spawnTypes[65];        // +0x3C by area (15 or more: none)
    float flashAngle;              // +0x140 (degrees, 2 a frame: the HUD text's flash)
    re::Ptr32<PunishmentLevel> level144;     // +0x144 (ped +0x154C)
    re::Ptr32<PunishmentLevel> level148;     // +0x148 (ped +0x14B0: late for class)
    re::Ptr32<PunishmentLevel> level14C;     // +0x14C (ped +0x14B4: class in session)
    re::Ptr32<PunishmentLevel> level150;     // +0x150 (ped +0x14AC: off limits)
    re::Ptr32<PunishmentLevel> level154;     // +0x154 (ped +0x1470, area 2)
};
static_assert(offsetof(Punishment, lastPed) == 4);
static_assert(offsetof(Punishment, schoolArea) == 0x14);
static_assert(offsetof(Punishment, asylum) == 0x20);
static_assert(offsetof(Punishment, alertLevel1) == 0x30);
static_assert(offsetof(Punishment, spawnTypes) == 0x3C);
static_assert(offsetof(Punishment, flashAngle) == 0x140);
static_assert(offsetof(Punishment, level144) == 0x144);
static_assert(offsetof(Punishment, level154) == 0x154);

// the ped fields read here (CPed, Ped.h)
constexpr uint32_t PED_POSITION = 0x4, PED_MATRIX = 0x14, PED_TYPE = 0x1310, PED_VEHICLE = 0x1554,
                   PED_PUNISHMENT = 0x1D40, PED_PUNISHMENT_TIME = 0x1D48;

GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, vehiclePool, 0x00C0F5F4);
GAME_ARRAY(float, spawnPosDefault, 0x00BCC564);        // [3]
GAME_VAR(int32_t, modelCopCar, 0x00BF4318);             // (sent after a ped in a vehicle)
GAME_VAR(int32_t, modelCopBike, 0x00BF431C);
GAME_VAR(int32_t, weaponModelFirst, 0x00A136B0);        // the weapons' model range
GAME_VAR(int32_t, weaponModelLast, 0x00A136B4);
GAME_VAR(int32_t, weaponModelBF44D4, 0x00BF44D4);
GAME_VAR(int32_t, actionNode1B65540, 0x01B65540);
GAME_VAR(int32_t, enabledMission, 0x009E51A0);          // -1: none
GAME_VAR(void*, detentionMeter, 0x00C674A0);
GAME_VAR(void*, obj_C2C1A0, 0x00C2C1A0);
GAME_VAR(CPool<uint8_t>*, triggerPool, 0x020C7C48);
GAME_VAR(int32_t, currentClass, 0x020C3638);            // (the class schedule's +0x50)
inline void* const classSchedule = reinterpret_cast<void*>(0x020C35E8);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const obj_20C5F70 = reinterpret_cast<void*>(0x020C5F70);
inline void* const triggers = reinterpret_cast<void*>(0x020C7E98);
GAME_VAR(uint8_t*, population, 0x00C2C108);             // +0x7828, +0x782C: spawn distances

// the models whose throwing at a ped gets it hit back and no points (0x44FA60)
GAME_VAR(int32_t, projectileModel0, 0x00BF3E84);
GAME_VAR(int32_t, projectileModel1, 0x00BF440C);
GAME_VAR(int32_t, projectileModel2, 0x00BF3E80);
GAME_VAR(int32_t, projectileModel3, 0x00BF3E7C);
GAME_VAR(int32_t, projectileModel4, 0x00BF4294);
GAME_VAR(int32_t, projectileModel5, 0x00BF4290);
GAME_VAR(int32_t, projectileModel6, 0x00BF428C);
GAME_VAR(int32_t, projectileModel7, 0x00BF4288);
GAME_VAR(int32_t, projectileModel8, 0x00BF4284);
GAME_VAR(int32_t, projectileModel9, 0x00BF4280);
GAME_VAR(int32_t, projectileModel10, 0x00BF4278);
GAME_VAR(int32_t, projectileModel11, 0x00BF4274);
GAME_VAR(int32_t, projectileModel12, 0x00BF4254);
GAME_VAR(int32_t, projectileModel13, 0x00BF4250);

// the punishment's own
GAME_FN(void, __thiscall, Punishment_44E280, 0x0044E280, Punishment* self, uint8_t* ped);
GAME_FN(float, __thiscall, Punishment_44E360, 0x0044E360, Punishment* self);
GAME_FN(void, __thiscall, Punishment_44E780, 0x0044E780, Punishment* self, uint8_t* vehicle);
GAME_FN(void, __thiscall, Punishment_AddPoints, 0x0044E850, Punishment* self, uint8_t* ped, int32_t* points, PunishmentLevel* level,
        int arg4, int arg5);
GAME_FN(uint8_t, __thiscall, Punishment_44E8E0, 0x0044E8E0, Punishment* self, uint8_t* ped);
GAME_FN(void, __thiscall, Punishment_SpawnVehicle, 0x0044EAE0, Punishment* self, int model);
GAME_FN(PunishmentLevel*, __thiscall, Punishment_GetLevel, 0x0044F2B0, Punishment* self, uint8_t* event);
GAME_FN(void, __thiscall, Punishment_44E2D0, 0x0044E2D0, Punishment* self, uint8_t* ped, int32_t* points, int32_t* value);
GAME_FN(void, __thiscall, Punishment_SpawnAuthorities, 0x0044F480, Punishment* self, uint8_t* ped);
GAME_FN(float, __thiscall, Punishment_GetClassProgress, 0x0044FE10, Punishment* self, int32_t* minutesLeft);
GAME_FN(void, __thiscall, MissionData_Construct, 0x0044F390, void* data, uintptr_t arg);
GAME_FN(void, __thiscall, MissionData_Destruct, 0x0044E0D0, void* data);
// the class schedule, the missions
GAME_FN(int, __thiscall, Schedule_GetCurrent, 0x006A8CF0, void* schedule);
GAME_FN(int, __thiscall, Schedule_GetState, 0x006A8CC0, void* schedule, int cls);
GAME_FN(uint8_t, __thiscall, Schedule_6A8BB0, 0x006A8BB0, void* schedule, int cls, int, int);
GAME_FN(void, __thiscall, Schedule_GetStart, 0x006A8DC0, void* schedule, int cls, int* hours, int* minutes);
GAME_FN(void, __thiscall, Schedule_GetEnd, 0x006A8DF0, void* schedule, int cls, int* hours, int* minutes);
GAME_FN(uint8_t, __thiscall, Schedule_6A8D20, 0x006A8D20, void* schedule);
GAME_FN(uint8_t*, __thiscall, Missions_Get, 0x006AA7C0, void* missions, int id);
GAME_FN(int, __thiscall, Missions_GetRunning, 0x006AA030, void* missions);
GAME_FN(int32_t*, __thiscall, Missions_6AA680, 0x006AA680, void* missions);
GAME_FN(int, __thiscall, Missions_GetId, 0x006AA660, void* missions, int index);
GAME_FN(uint8_t, __thiscall, Missions_6AA840, 0x006AA840, void* missions);
GAME_FN(uint8_t, __thiscall, Obj20C5F70_6CAA20, 0x006CAA20, void* self);
GAME_FN(uint8_t, __thiscall, Obj20C5F70_6CAA60, 0x006CAA60, void* self);
// triggers
GAME_FN(void*, __thiscall, Triggers_GetByName, 0x00431740, void* triggers, const char* name);
GAME_FN(int, __thiscall, Triggers_FindByName, 0x006D7A10, void* triggers, const char* name);   // -1: none
GAME_FN(uint8_t, __thiscall, Trigger_IsPointInside2D, 0x006D6EB0, void* trigger, const float* xy);
// population
GAME_FN(int, __thiscall, Population_CountOfType, 0x0049C2D0, uint8_t* population, int type);
GAME_FN(uint8_t, __thiscall, Population_FindSpawnPoint, 0x0049D860, uint8_t* population, const float* from, float* out, int, int* out2,
        float minDistance, float maxDistance);
GAME_FN(uint8_t*, __thiscall, Population_AddPed_x64, 0x004A09E0, uint8_t* population, int type, re::Float3, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Population_AddPed(uint8_t* a0, int a1, float a2, float a3, float a4, int a5) {
    return Population_AddPed_x64(a0, a1, re::Float3{a2, a3, a4}, a5);
}
GAME_FN(uint8_t, __cdecl, FindSpawnPoint48B510, 0x0048B510, float* pos, float* heading, uintptr_t);
GAME_FN(uint8_t, __cdecl, IsPositionClear48C990, 0x0048C990, const float* pos, float radius, int, uintptr_t);
GAME_FN(float, __cdecl, FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int);
GAME_FN(uint8_t, __cdecl, RequestModelIfNotLoaded, 0x00533030, int model, int flags);
// peds, vehicles
GAME_FN(uint8_t, __thiscall, Ped_4CE690, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_41AA50, 0x0041AA50, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, PedObjectives_GetCurrent, 0x00470340, void* objectives);   // +8 its type
GAME_FN(uint8_t*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(uint8_t, __thiscall, Vehicle_4CB7F0, 0x004CB7F0, uint8_t* vehicle);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x00475730, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Ped_SetPunishmentTime, 0x004745B0, uint8_t* ped, int msFromNow);
GAME_FN(uint8_t, __thiscall, Ped_485530, 0x00485530, void* part2F0);
GAME_FN(uint8_t, __thiscall, Ped_48D470, 0x0048D470, void* partA64, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, Ped_GetIntelligence408, 0x00474BE0, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_44E400, 0x0044E400, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_474040, 0x00474040, uint8_t* ped);
GAME_FN(void, __thiscall, Obj_C2C1A0_4A67D0, 0x004A67D0, void* self, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Event_GetOffender, 0x004A5CB0, uint8_t* event);
GAME_FN(uint8_t*, __thiscall, Event_GetVictim, 0x004A5CD0, uint8_t* event);
GAME_FN(uint8_t*, __cdecl, GetWeaponInfo, 0x0051B450, int model);   // +0x6C, +0x70: point factors
GAME_FN(void, __thiscall, HUDDetention_SetText, 0x0053D650, void* hud, const char* text, int value, int force);
GAME_FN(uint8_t, __cdecl, IsTimeInRange, 0x00425E40, int fromHours, int toHours, int fromMinutes, int toMinutes);
GAME_FN(float, __cdecl, LimitRadianAngle, 0x00432EE0, float angle);

const float* PedPosition(uint8_t* ped) {
    uint8_t* matrix = At<uint8_t*>(ped, PED_MATRIX);
    return matrix ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(ped + PED_POSITION);
}

// the ped's vf45 (AddPunishment)
void AddPunishment(uint8_t* ped, int points) {
    using Fn = void(__thiscall*)(uint8_t*, int);
    re::GameFnAt<Fn>(At<uint8_t*>(ped, 0) + 0xB4)(ped, points);
}

// _ftol2 (truncation) of an int by a float, as the game's fild / fmul
int32_t Scale(int32_t value, float factor) {
    return int32_t(double(value) * double(factor));
}

// 0x44F480 (thiscall): authority figures (and their vehicles) sent after a ped whose
// points are over the alert levels
void __fastcall SpawnAuthorities(Punishment* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return;
    }
    int32_t points = At<int32_t>(ped, PED_PUNISHMENT);
    int32_t vehiclesWanted = 0;
    int32_t maxPeds;
    if (points >= self->alertLevel3) {
        Punishment_44E280(self, ped);
        maxPeds = 2;
        vehiclesWanted = 1;
    } else if (points > self->alertLevel2) {
        Punishment_44E280(self, ped);
        maxPeds = 1;
        vehiclesWanted = 0;
    } else if (points > self->alertLevel1) {
        maxPeds = 0;
    } else {
        return;
    }
    int32_t type = self->spawnTypes[currArea];
    if (type >= 15) {
        return;
    }
    if (self->onlyPlayer && At<int32_t>(ped, PED_TYPE) != 13) {
        return;
    }
    uint8_t onGrounds = 0;
    switch (currArea) {
    case 0: {
        if (!self->schoolArea && !(self->schoolArea = Triggers_GetByName(triggers, "AMB_SCHOOL_AREA"))) {
            return;
        }
        if (!self->boysDorm && !(self->boysDorm = Triggers_GetByName(triggers, "PUN_BDORM"))) {
            return;
        }
        if (!self->girlsDorm && !(self->girlsDorm = Triggers_GetByName(triggers, "PUN_GDORM"))) {
            return;
        }
        const float* pos = PedPosition(ped);
        float xy[2] = {pos[0], pos[1]};
        if (!Trigger_IsPointInside2D(self->schoolArea, xy)) {
            break;
        }
        onGrounds = 1;
        type = 0;
        int state = Schedule_GetState(classSchedule, currentClass);
        vehiclesWanted = 0;
        if (state != 3) {
            break;
        }
        goto alert3;
    }
    case 2:
        onGrounds = 1;
        if (Schedule_GetState(classSchedule, currentClass) == 2) {
            goto alert3;
        }
        if (Schedule_GetState(classSchedule, currentClass) != 3) {
            break;
        }
        goto alert3;
    case 0xE:
    case 0x14:
        return;
    default:
        break;
    }
    goto spawn;
alert3:
    if (points >= self->alertLevel3) {
        type = 0;
    } else {
        maxPeds = 0;
    }
spawn:
    if (currArea != 0) {
        vehiclesWanted = 0;
    }
    int32_t count = Population_CountOfType(population, type);
    int32_t vehicles = 0;
    if (vehiclesWanted > 0 && currArea == 0 && type != 0) {
        int32_t model = (At<void*>(ped, PED_VEHICLE) && Ped_4CE690(ped)) ? modelCopCar : modelCopBike;
        CPool<uint8_t>* pool = vehiclePool;
        // (slot 0 is never looked at)
        for (int slot = pool->size - 1; slot != 0; slot--) {
            if (pool->IsFreeSlot(slot)) {
                continue;
            }
            uint8_t* vehicle = pool->objects + pool->elemSize * slot;
            if (!vehicle || !Vehicle_4CB7F0(vehicle)) {
                continue;
            }
            int32_t vehicleModel = At<int16_t>(vehicle, 0x10E);
            if (vehicleModel != modelCopBike && vehicleModel != modelCopCar) {
                continue;
            }
            void* seats = vehicle + 0x1C4;
            bool chase = false;
            if (vehicleModel == modelCopBike && Seats_GetOccupant(seats, 0) && Ped_41AA50(Seats_GetOccupant(seats, 0))) {
                chase = At<uint8_t>(vehicle, 0x2DA) == 1;
            } else if (At<int16_t>(vehicle, 0x10E) == modelCopCar && Seats_GetOccupant(seats, 0) &&
                       Ped_41AA50(Seats_GetOccupant(seats, 0))) {
                uint8_t* objective = PedObjectives_GetCurrent(Seats_GetOccupant(seats, 0) + 0x5F0);
                chase = objective && At<int32_t>(objective, 8) != 7;
            }
            if (chase) {
                Punishment_44E780(self, vehicle);
            }
            vehicles++;
        }
        if (RequestModelIfNotLoaded(model, 0) && vehicles < vehiclesWanted) {
            for (int n = vehiclesWanted - vehicles; n != 0; n--) {
                Punishment_SpawnVehicle(self, model);
            }
        }
    }

    float pos[3] = {spawnPosDefault[0], spawnPosDefault[1], spawnPosDefault[2]};
    float heading = 0.0f;
    int32_t spawnOut = 0;
    // (the time as unsigned, the ped's as signed)
    if (!(double(timeInMs) <= double(At<int32_t>(ped, PED_PUNISHMENT_TIME)))) {
        return;
    }
    if (count >= maxPeds) {
        return;
    }
    uint8_t haveSpot = 0;
    do {
        if (onGrounds) {
            haveSpot = FindSpawnPoint48B510(pos, &heading, 0);
            if (!haveSpot) {
                goto aroundPlayer;
            }
            if (IsPositionClear48C990(pos, 5.0f, -1, 0)) {
                float xy[2] = {pos[0], pos[1]};
                bool inDorm = self->boysDorm && Trigger_IsPointInside2D(self->boysDorm, xy);
                if (!inDorm && self->girlsDorm) {
                    float xy2[2] = {pos[0], pos[1]};
                    inDorm = Trigger_IsPointInside2D(self->girlsDorm, xy2) != 0;
                }
                if (inDorm) {
                    haveSpot = 0;
                    goto aroundPlayer;
                }
            }
        }
        if (haveSpot) {
            goto found;
        }
    aroundPlayer: {
        float maxDistance = At<float>(population, 0x7828);
        float minDistance = At<float>(population, 0x782C);
        if (!Population_FindSpawnPoint(population, PedPosition(playerPed), pos, 1, &spawnOut, minDistance,
                                       float(double(maxDistance) - 1.0))) {
            return;
        }
    }
    found: {
        float xy[2] = {pos[0], pos[1]};
        if (self->schoolArea && Trigger_IsPointInside2D(self->schoolArea, xy) && type != 0) {
            type = 0;
        }
        uint8_t groundFound = 0;
        float groundZ = FindGroundZ(pos[0], pos[1], float(double(pos[2]) + 1.0), &groundFound, 1);
        pos[2] = groundFound ? float(double(groundZ) + double(0.01f)) : float(double(pos[2]) + double(0.35f));
        uint8_t* spawned = Population_AddPed(population, type, pos[0], pos[1], pos[2], 1);
        if (haveSpot) {
            // (the punished ped's heading, not the new one's: the game's)
            Ped_SetHeading(ped, heading);
        }
        if (!spawned) {
            return;
        }
    }
    } while (++count < maxPeds);
}

// 0x44FA60 (thiscall): a crime seen (an event: +0 its kind, +4 a vehicle, +8 the object
// used, +0x29 set when points were given): points to its offender
void __fastcall OnCrime(Punishment* self, re::Edx, uint8_t* event) {
    if (self->disabled) {
        return;
    }
    uint8_t* offender = Event_GetOffender(event);
    uint8_t* victim = Event_GetVictim(event);
    if (offender == victim) {
        return;
    }
    if (offender) {
        int32_t pedType = At<int32_t>(offender, PED_TYPE);
        if (pedType == 0 || pedType == 8 || pedType == 7) {
            return;
        }
    } else {
        uint8_t* vehicle = At<uint8_t*>(event, 4);
        if (!vehicle || (At<uint8_t>(vehicle, 0x108) & 7) != 2) {
            return;
        }
        offender = Seats_GetOccupant(vehicle + 0x1C4, 0);
        if (!offender) {
            return;
        }
    }
    PunishmentLevel* level = Punishment_GetLevel(self, event);
    if (!level) {
        return;
    }
    int32_t points = level->points;
    uint8_t outOfLine = 0;   // (pushed as a dword: its upper bytes are uninitialised stack in the game)
    int32_t ammo = 0;
    if (offender && victim) {
        int32_t weaponModel = At<int16_t>(offender, 0x10E);
        if (At<int16_t>(Ped_GetIntelligence408(victim), 0x5E) >= weaponModelFirst &&
            At<int16_t>(Ped_GetIntelligence408(victim), 0x5E) <= weaponModelLast) {
            weaponModel = At<int16_t>(Ped_GetIntelligence408(victim), 0x5E);
            ammo = At<int32_t>(Ped_GetIntelligence408(victim), 0x60);
        }
        uint8_t* info = GetWeaponInfo(weaponModel);
        if (info) {
            int32_t kind = At<int32_t>(event, 0);
            if (kind == 4) {
                points = Scale(points, At<float>(info, 0x6C));
                uint8_t* vehicle = At<uint8_t*>(victim, PED_VEHICLE);
                if (vehicle && At<int32_t>(vehicle, 0x46C) == 0) {
                    points = 0;
                }
                int32_t node = actionNode1B65540;
                if (At<int32_t>(Ped_GetIntelligence408(victim), 0x64) == node) {
                    int32_t since = int32_t(timeInMs - At<uint32_t>(victim, 0x4CC));
                    double f = since;
                    if (since < 0) {
                        f += 4294967296.0;
                    }
                    float elapsed = float(std::fabs(f));
                    if (elapsed < 500.0) {
                        int32_t value = self->alertLevel2;
                        Punishment_44E2D0(self, offender, &points, &value);
                    }
                }
                if (weaponModel == weaponModelBF44D4 && ammo != 1) {
                    int32_t value = self->alertLevel2;
                    Punishment_44E2D0(self, offender, &points, &value);
                }
            } else if (kind == 0x3C) {
                points = Scale(points, At<float>(info, 0x6C));
            }
            if (weaponModel >= weaponModelFirst && weaponModel <= weaponModelLast) {
                if (1.0f < At<float>(info, 0x6C)) {
                    outOfLine = 1;
                } else {
                    outOfLine = 0;
                    if (At<uint8_t>(Ped_GetModelInfo(victim), 0x50) || Ped_44E400(victim)) {
                        int32_t value = self->alertLevel2;
                        Punishment_44E2D0(self, offender, &points, &value);
                    }
                }
            }
        }
    } else if (At<int32_t>(event, 0) == 0) {
        int32_t model = At<int16_t>(At<uint8_t*>(offender, 0x1D0), 0x10E);
        if (model >= weaponModelFirst && model <= weaponModelLast) {
            uint8_t* info = GetWeaponInfo(model);
            if (info) {
                points = Scale(points, At<float>(info, 0x70));
            }
        } else {
            points = 0;
        }
    }
    if (At<int32_t>(offender, 0x14C8) && (At<int32_t>(event, 0) == 0x3D || At<int32_t>(event, 0) == 0x48)) {
        points = 0;
    }
    uint8_t* object = At<uint8_t*>(event, 8);
    if (object) {
        int32_t model = At<int16_t>(object, 0x10E);
        if (model == projectileModel0 || model == projectileModel1 || model == projectileModel2 || model == projectileModel3 ||
            model == projectileModel4 || model == projectileModel5 || model == projectileModel6 || model == projectileModel7 ||
            model == projectileModel8 || model == projectileModel9 || model == projectileModel10 || model == projectileModel11 ||
            model == projectileModel12 || model == projectileModel13) {
            Obj_C2C1A0_4A67D0(obj_C2C1A0, offender);
            points = 0;
        }
    }
    if (!Ped_474040(offender) && points != 0) {
        Punishment_AddPoints(self, offender, &points, level, 1, outOfLine);
        if (points > 0) {
            At<uint8_t>(event, 0x29) = 1;
        }
    }
}

// 0x44FE10 (thiscall): how far into the current class (0 .. 1, 0 when none or a mission
// is on); `minutesLeft` the minutes to its end (5 before the bell) when it's on
float __fastcall GetClassProgress(Punishment*, re::Edx, int32_t* minutesLeft) {
    float progress = 0.0f;
    int cls = Schedule_GetCurrent(classSchedule);
    if (enabledMission != -1 && At<uint16_t>(Missions_Get(missionMgr, enabledMission), 2) == 0) {
        return 0.0f;
    }
    int running = Missions_GetRunning(missionMgr);
    if (running == -1) {
        if (*Missions_6AA680(missionMgr) - 1 != enabledMission) {
            return 0.0f;
        }
    } else {
        alignas(4) uint8_t data[0x4C];   // a CMissionData
        MissionData_Construct(data, Missions_GetId(missionMgr, running));
        if (data[0x2C]) {
            MissionData_Destruct(data);
            return 0.0f;
        }
        MissionData_Destruct(data);
    }
    if (cls < 0 || Missions_6AA840(missionMgr)) {
        return 0.0f;
    }
    if (!Schedule_6A8BB0(classSchedule, cls, 0, -1) || Schedule_GetState(classSchedule, cls) != 1) {
        return progress;
    }
    int startHours = 0, startMinutes = 0, endHours = 0, endMinutes = 0;
    Schedule_GetStart(classSchedule, cls, &startHours, &startMinutes);
    Schedule_GetEnd(classSchedule, cls, &endHours, &endMinutes);
    int start = startMinutes + startHours * 60;
    int end = endMinutes - 5 + endHours * 60;
    int length = end - start;
    if (start > end) {
        length += 1440;
    }
    int now = clockMinutes + clockHours * 60;
    int left = now > end ? end - now + 1440 : end - now;
    if (length > 0) {
        progress = 1.0f - float(left) / float(length);
    }
    if (length > left) {
        *minutesLeft = length - left;
    }
    return progress;
}

// (inlined three times in 0x44FFB0) a level's points to a ped, up to the level's most:
// what was given
int32_t GiveLevelPoints(uint8_t* ped, int32_t points, int32_t maxPoints) {
    int32_t current = At<int32_t>(ped, PED_PUNISHMENT);
    if (current > maxPoints) {
        return 0;
    }
    if (current + points > maxPoints) {
        points = maxPoints - current;
    }
    if (points > 0) {
        AddPunishment(ped, points);
    }
    return points;
}

// 0x44FFB0 (thiscall): every ped's authority figures, and the points of one ped a frame
// (round-robin, from the last one down) for where it is
void __fastcall Update(Punishment* self, re::Edx) {
    if (self->disabled) {
        return;
    }
    CPool<uint8_t>* pool = pedPool;
    int32_t size = pool->size;
    int32_t minutesLeft = 0;
    float classProgress = Punishment_GetClassProgress(self, &minutesLeft);
    Punishment_44E360(self);
    self->flashAngle = float(double(self->flashAngle) + 2.0);
    uint8_t lateSoon = uint32_t(minutesLeft - 1) <= 29;
    float radians = float(double(self->flashAngle) * double(3.1415927f) / 180.0);
    float flash = std::fabs(float(std::sin(double(LimitRadianAngle(radians)))));

    for (int slot = size; slot != 0;) {
        slot--;
        if (!pool->IsFreeSlot(slot)) {
            uint8_t* ped = pool->objects + pool->elemSize * slot;
            if (ped) {
                Punishment_SpawnAuthorities(self, ped);
            }
        }
    }

    int32_t slot = self->lastPed - 1;
    if (slot == self->lastPed) {
        return;
    }
    for (;;) {
        if (slot < 0) {
            slot = size - 1;
        }
        uint8_t* ped = nullptr;
        if (!pool->IsFreeSlot(slot)) {
            ped = pool->objects + pool->elemSize * slot;
        }
        if (ped) {
            int32_t pedType = At<int32_t>(ped, PED_TYPE);
            if (pedType == 0 || pedType == 8 || pedType == 7) {
                // an authority figure: the points of those it sees
                int32_t numSeen = At<int32_t>(ped, 0x3C0);
                for (int i = 0; i < numSeen; i++) {
                    uint8_t* seen = At<uint8_t*>(ped, 0x3C4 + 4 * i);
                    if (seen && Ped_48D470(ped + 0xA64, seen) && self->unknown2) {
                        AddPunishment(seen, self->alertLevel2);
                    }
                    numSeen = At<int32_t>(ped, 0x3C0);
                }
                self->lastPed = slot;
                return;
            }
            uint8_t offLimits = Punishment_44E8E0(self, ped);
            pedType = At<int32_t>(ped, PED_TYPE);
            bool student = ((pedType > 0 && pedType < 7) || pedType == 0xB || pedType == 0xD);
            uint8_t skipping = student && At<int32_t>(ped, 0x14E8) && 0.0f < classProgress;
            uint8_t inClass = student && At<int32_t>(ped, 0x14E8) && Schedule_6A8D20(classSchedule) && currArea != 0xE;
            bool clear = false;
            if (At<int32_t>(ped, PED_TYPE) != 0xD) {
                if (Ped_485530(ped + 0x2F0)) {
                    skipping = 0;
                }
            }
            if (At<int32_t>(ped, PED_TYPE) == 0xD) {
                if (Obj20C5F70_6CAA20(obj_20C5F70) || At<uint8_t>(playerPed, 0x1E91)) {
                    skipping = 0;
                    offLimits = 0;
                    inClass = 0;
                }
                if (At<int32_t>(ped, PED_TYPE) == 0xD && Obj20C5F70_6CAA60(obj_20C5F70)) {
                    if (!self->asylum) {
                        int index = Triggers_FindByName(triggers, "TINDUST_ASYLUM");
                        CPool<uint8_t>* triggerSlots = triggerPool;
                        self->asylum = (index == -1 || triggerSlots->IsFreeSlot(index)) ? nullptr
                                                                                        : triggerSlots->objects + triggerSlots->elemSize * index;
                        if (!self->asylum) {
                            return;   // (lastPed not stored)
                        }
                    }
                    if (currArea == 0x26) {
                        clear = true;
                    } else if (self->asylum) {
                        const float* pos = PedPosition(ped);
                        float xy[2] = {pos[0], pos[1]};
                        clear = Trigger_IsPointInside2D(self->asylum, xy) != 0;
                    }
                }
            }
            if (clear) {
                skipping = 0;
                inClass = 0;
            }
            if (!clear && offLimits) {
                if (!At<int32_t>(ped, 0x14AC)) {
                    flash = 1.0f;
                }
                At<int32_t>(ped, 0x14AC) = 1;
            } else if (At<int32_t>(ped, 0x14AC)) {
                At<int32_t>(ped, 0x14AC) = 0;
            }
            if (skipping) {
                At<int32_t>(ped, 0x154C) = lateSoon;
                if (!lateSoon) {
                    if (!At<int32_t>(ped, 0x14B0)) {
                        flash = 1.0f;
                    }
                    At<int32_t>(ped, 0x14B0) = 1;
                }
            } else if (At<int32_t>(ped, 0x14B0) || At<int32_t>(ped, 0x154C)) {
                At<int32_t>(ped, 0x154C) = 0;
                At<int32_t>(ped, 0x14B0) = 0;
            }
            if (inClass) {
                if (!At<int32_t>(ped, 0x14B4)) {
                    flash = 1.0f;
                }
                At<int32_t>(ped, 0x14B4) = 1;
            } else if (At<int32_t>(ped, 0x14B4)) {
                At<int32_t>(ped, 0x14B4) = 0;
            }

            if (At<int32_t>(ped, 0x14AC)) {
                PunishmentLevel* level = self->level150;
                if (!level) {
                    At<int32_t>(ped, 0x14AC) = 0;
                } else {
                    int32_t points = level->points;
                    int32_t maxPoints;
                    bool haveMax = true;
                    if (self->level14C && !IsTimeInRange(7, 0x13, 0, 0) && currArea == 2) {
                        // (the points are the class level's most, the game's)
                        points = self->level14C->maxPoints;
                        maxPoints = points;
                    } else if (self->level150) {
                        maxPoints = self->level150->maxPoints;
                    } else {
                        haveMax = false;
                        maxPoints = 0;
                    }
                    if (haveMax) {
                        points = GiveLevelPoints(ped, points, maxPoints);
                    } else if (points > 0) {
                        AddPunishment(ped, points);
                    }
                    if (At<int32_t>(ped, PED_PUNISHMENT) <= self->level150->maxPoints) {
                        Ped_SetPunishmentTime(ped, 5000);
                    }
                    if (At<int32_t>(ped, PED_TYPE) == 0xD && flash >= 0.99f) {
                        HUDDetention_SetText(detentionMeter, self->level150->text, points, 0);
                    }
                }
            } else if (At<int32_t>(ped, 0x154C)) {
                if (!self->level144) {
                    At<int32_t>(ped, 0x154C) = 0;
                }
            } else if (At<int32_t>(ped, 0x14B0)) {
                PunishmentLevel* level = self->level148;
                if (!level) {
                    At<int32_t>(ped, 0x14B0) = 0;
                } else {
                    int32_t points = GiveLevelPoints(ped, level->points, level->maxPoints);
                    if (At<int32_t>(ped, PED_PUNISHMENT) <= self->level148->maxPoints) {
                        Ped_SetPunishmentTime(ped, 5000);
                    }
                    if (At<int32_t>(ped, PED_TYPE) == 0xD && flash >= 0.98f) {
                        HUDDetention_SetText(detentionMeter, self->level148->text, points, 0);
                    }
                }
            } else if (At<int32_t>(ped, 0x14B4)) {
                PunishmentLevel* level = self->level14C;
                if (!level) {
                    At<int32_t>(ped, 0x14B4) = 0;
                } else {
                    int32_t points = GiveLevelPoints(ped, level->points, level->maxPoints);
                    if (At<int32_t>(ped, PED_PUNISHMENT) <= self->level14C->maxPoints) {
                        Ped_SetPunishmentTime(ped, 5000);
                    }
                    if (At<int32_t>(ped, PED_TYPE) == 0xD && flash >= 0.98f) {
                        HUDDetention_SetText(detentionMeter, self->level14C->text, points, 0);
                    }
                }
            } else if (At<int32_t>(ped, 0x1470) && currArea == 2 && self->level154) {
                PunishmentLevel* level = self->level154;
                int32_t points = level->points;
                if (At<int32_t>(ped, PED_PUNISHMENT) > level->maxPoints) {
                    points = 0;
                }
                Punishment_AddPoints(self, ped, &points, level, 0, 0);
                if (At<int32_t>(ped, PED_TYPE) == 0xD) {
                    HUDDetention_SetText(detentionMeter, self->level154->text, points, 0);
                }
            }
        }
        slot--;
        if (slot == self->lastPed) {
            return;
        }
    }
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define C04B_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04B_REVERSED("CPunishment::SpawnAuthorities", 0x0044F480, &SpawnAuthorities);
C04B_REVERSED("CPunishment::OnCrime", 0x0044FA60, &OnCrime);
C04B_REVERSED("CPunishment::GetClassProgress", 0x0044FE10, &GetClassProgress);
C04B_REVERSED("CPunishment::Update", 0x0044FFB0, &Update);
