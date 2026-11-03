// Script natives for vehicles (0x5D5DC0..0x5D7E40). Each is `int __cdecl Native_X(lua_State* L)`,
// returning how many results it pushed; registered under its script name (lua_natives.csv).
//
// A vehicle handle is looked up the game's inlined way (snPoolAt: the slot's flag byte must
// match; nullptr otherwise, which most natives then use anyway). Several natives take the
// handle as 16 bits (noted).

#include <cmath>
#include <cstdint>

#include "game/ScriptNatives.h"
#include "reversed.h"

namespace {

// The vehicle of argument i (the full int, or its low 16 bits)
inline uint8_t* snvVehicleArg(lua_State* L, int i) {
    return snPoolAt(snVehiclePool, Script::GetInt(L, i));
}

GAME_FN(uint8_t, __cdecl, snvLoadModelNow, 0x00533030, int model, int flags);
GAME_FN(void, __thiscall, snvScript_AddModel, 0x005D9450, void* script, int model);
GAME_FN(void, __thiscall, snvScript_RemoveVehicle, 0x005D9A00, void* script, int handle);

// model (16 bits)[, priority]: loaded now (without priority the running script keeps it)
int __cdecl Native_VehicleRequestModel(lua_State* L) {
    int model = uint16_t(Script::GetInt(L, 0));
    uint8_t loaded;
    if (!Script::GetBoolOpt(L, 1, 0)) {
        loaded = snvLoadModelNow(model, 2);
        snvScript_AddModel(snCurrentScript(snLurStore), model);
    } else {
        loaded = snvLoadModelNow(model, 1);
    }
    Script::PushBool(L, loaded != 0);
    return 1;
}

GAME_FN(uint8_t, __cdecl, snvIsEntityInArea, 0x005C2A70, int is3D, int handle, int kind, float x, float y, float z, float rx,
        float ry, float rz, int blip, const SnVec* rotation, int blip2, int a13, int a14, int a15, int a16);

// vehicle (16 bits), x, y, z, radius, blip[, heading (degrees)]
int __cdecl Native_VehicleIsInAreaXYZ(lua_State* L) {
    int handle = uint16_t(Script::GetInt(L, 0));
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float r = Script::GetFloat(L, 4);
    int blip = Script::GetInt(L, 5);
    float heading = 0.0f;
    if (Script::GetArgCount(L) >= 7) {
        heading = float(double(Script::GetFloat(L, 6)) * double(3.1415927f) / 180.0);
    }
    SnVec rotation = {0.0f, 0.0f, heading};
    Script::PushBool(L, snvIsEntityInArea(1, handle, 1, x, y, z, r, r, r, blip, &rotation, -1, -1, 0, 0, 1));
    return 1;
}

// The vehicle population (0xBCD080)
GAME_VAR(uint8_t*, snvVehiclePopulation, 0x00BCD080);
GAME_FN(void, __thiscall, snvPopulation_OverrideAmbient, 0x0041A820, void* pop, const int32_t* models);
GAME_FN(uint8_t, __thiscall, snvPopulation_IsPointOkForCar, 0x0041FF20, void* pop, const SnVec* pos);

// model[, model, model, model (0)]
int __cdecl Native_VehicleOverrideAmbient(lua_State* L) {
    int32_t models[4];
    models[0] = Script::GetInt(L, 0);
    for (int i = 1; i < 4; i++) {
        models[i] = Script::GetIntOpt(L, i, 0);
    }
    snvPopulation_OverrideAmbient(snvVehiclePopulation, models);
    return 0;
}

GAME_FN(int32_t, __cdecl, snvCreateCarGenerator, 0x004C74E0, float x, float y, float z, float heading, int model, int col1,
        int col2, int force, int unused, int alarm, int minDelay, int maxDelay, int upright);

// x, y, z, heading, model, colours, 3 flags, the delays, upright (the flags go as full
// eax values, from the byte results)
int __cdecl Native_VehicleCreateCarGenerator(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    int upright = Script::GetBoolOpt(L, 12, 0);
    int maxDelay = Script::GetInt(L, 11);
    int minDelay = Script::GetInt(L, 10);
    int alarm = Script::GetBool(L, 9);
    int unused = Script::GetBool(L, 8);
    int force = Script::GetBool(L, 7);
    int col2 = Script::GetInt(L, 6);
    int col1 = Script::GetInt(L, 5);
    int model = Script::GetInt(L, 4);
    float heading = Script::GetFloat(L, 3);
    Script::PushInt(L, snvCreateCarGenerator(x, y, z, heading, model, col1, col2, force, unused, alarm, minDelay, maxDelay, upright));
    return 1;
}

int __cdecl Native_CanCreateVehicleNearXYZ(lua_State* L) {
    SnVec pos;
    pos.x = Script::GetFloat(L, 0);
    pos.y = Script::GetFloat(L, 1);
    pos.z = Script::GetFloat(L, 2);
    Script::PushBool(L, snvPopulation_IsPointOkForCar(snvVehiclePopulation, &pos));
    return 1;
}

int __cdecl Native_GetCurrentNumOfAmbientBikes(lua_State* L) {
    Script::PushInt(L, snAt<int32_t>(snvVehiclePopulation, 0x1D18));
    return 1;
}

GAME_FN(void, __cdecl, snvWorld_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, snvWorld_RemoveFromCollisionRecords, 0x0045E690, void* entity);

// Out of the world and deleted (the virtual destructor, +0, with 1)
void snvDeleteVehicle(int handle) {
    uint8_t* vehicle = snPoolAt(snVehiclePool, handle);
    if (vehicle != nullptr) {
        snvWorld_Remove(vehicle);
        snvWorld_RemoveFromCollisionRecords(vehicle);
        snVCall<void>(vehicle, 0, 1);
    }
}

// 0x5D6180 (cdecl, the handle as 16 bits)
void __cdecl DeleteVehicle(int handle) {
    snvDeleteVehicle(uint16_t(handle));
}

GAME_FN(SnVec*, __cdecl, snvVectorSub, 0x00414B70, SnVec* out, const SnVec* a, const SnVec* b);
GAME_FN(float, __cdecl, snvHeadingFromVector, 0x00433310, const SnVec* v);
GAME_FN(float, __thiscall, snvVector_Normalise, 0x00414E10, SnVec* v);
GAME_FN(SnVec*, __cdecl, snvCrossProduct, 0x00414730, SnVec* out, const SnVec* a, const SnVec* b);
// the up vector (0, 0, 1)
inline SnVec* const snvUp = reinterpret_cast<SnVec*>(0x009E33DC);

// vehicle, from x, y, z, towards x, y, z[, teleport (true)]: moved there facing the
// second point (the vehicle's virtual +0xD4), or only turned to it (its matrix: forward
// the normalised direction, right forward x up, up (0, 0, 1); the game fills the
// matrix's padding (+0x1C, +0x2C) with uninitialised stack, left alone here)
int __cdecl Native_VehicleSetPosXYZ(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    uint8_t teleport = 1;
    if (lua::gettop(L) > 7) {
        teleport = Script::GetBool(L, 7);
    }
    SnVec from, to;
    from.x = Script::GetFloat(L, 1);
    from.y = Script::GetFloat(L, 2);
    from.z = Script::GetFloat(L, 3);
    to.x = Script::GetFloat(L, 4);
    to.y = Script::GetFloat(L, 5);
    to.z = Script::GetFloat(L, 6);
    SnVec direction;
    float heading = snvHeadingFromVector(snvVectorSub(&direction, &to, &from));
    if (teleport) {
        snVCall<void>(vehicle, 0xD4, from.x, from.y, from.z, heading);
        return 0;
    }
    float* matrix = snEntity_GetMatrix(vehicle);
    SnVec forward;
    snvVectorSub(&forward, &to, &from);
    snvVector_Normalise(&forward);
    matrix[4] = forward.x;
    matrix[5] = forward.y;
    matrix[6] = forward.z;
    SnVec right1, right2, right3;
    snvCrossProduct(&right1, &forward, snvUp);
    snvCrossProduct(&right2, &forward, snvUp);
    snvCrossProduct(&right3, &forward, snvUp);
    matrix[0] = right3.x;
    matrix[1] = right2.y;
    matrix[2] = right1.z;
    matrix[8] = snvUp->x;
    matrix[9] = snvUp->y;
    matrix[10] = snvUp->z;
    return 0;
}

// (16 bits) deleted, and dropped by the running script whether or not it existed
int __cdecl Native_VehicleDelete(lua_State* L) {
    int handle = uint16_t(Script::GetInt(L, 0));
    snvDeleteVehicle(handle);
    snvScript_RemoveVehicle(snCurrentScript(snLurStore), handle);
    return 0;
}

GAME_FN(void, __cdecl, snvSetModelNotNeededAmbient, 0x0052FE40, int model);

int __cdecl Native_VehicleModelNotNeededAmbient(lua_State* L) {
    uint8_t* vehicle = snPoolAt(snVehiclePool, uint16_t(Script::GetInt(L, 0)));
    if (vehicle != nullptr) {
        snvSetModelNotNeededAmbient(snAt<int16_t>(vehicle, 0x10E));
    }
    return 0;
}

GAME_FN(void, __thiscall, snvVehicle_SetCreatedBy, 0x004CB230, void* vehicle, int createdBy);

// (16 bits) made a mission vehicle (created by 1; nullptr for a stale handle, used anyway)
// and dropped by the running script
int __cdecl Native_VehicleDontCleanup(lua_State* L) {
    int handle = uint16_t(Script::GetInt(L, 0));
    snvVehicle_SetCreatedBy(snPoolAt(snVehiclePool, handle), 1);
    snvScript_RemoveVehicle(snCurrentScript(snLurStore), handle);
    return 0;
}

GAME_FN(uint8_t*, __thiscall, snvSeats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void*, __thiscall, snvMoveToPosObjective_Construct, 0x004705B0, void* self, uint8_t* ped, const ScriptVector* target,
        float speed, float radius, int, int);
GAME_FN(float, __cdecl, snvGetCarToGoToCoors, 0x0041A170, void* vehicle, const ScriptVector* target);

// vehicle, x, y, z[, speed (0.5)]: a bike's rider gets a move objective (its +0x10 the
// speed, written through a failed allocation too), a car drives there
int __cdecl Native_VehicleMoveToXYZ(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    uint8_t* driver = snvSeats_GetOccupant(vehicle + 0x1C4, 0);
    float speed = *reinterpret_cast<const float*>(0x009055A0); // (0.5)
    if (lua::gettop(L) == 5) {
        speed = Script::GetFloatOpt(L, 4, *reinterpret_cast<const float*>(0x009055A0));
    }
    ScriptVector target;
    Script::GetVector(&target, L, 1);
    if (snAt<int32_t>(vehicle, 0x46C) == 1) {
        void* mem = snMalloc(0x34);
        float r = *reinterpret_cast<const float*>(0x00911E30); // (0.3)
        uint8_t* objective = mem != nullptr ? static_cast<uint8_t*>(snvMoveToPosObjective_Construct(mem, driver, &target, r, r, 0, 0))
                                            : nullptr;
        snAt<float>(objective, 0x10) = speed;
        snObjectives_Add(driver + 0x5F0, objective);
        return 0;
    }
    snvGetCarToGoToCoors(vehicle, &target);
    return 0;
}

GAME_FN(void, __thiscall, snvVehicle_FollowPath, 0x004CB480, void* vehicle, int path, int flag, int flag2);

// vehicle, path[, flag]: (the flag goes as a dword with uninitialised upper bytes)
int __cdecl Native_VehicleFollowPath(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    uint8_t flag = 0;
    if (lua::gettop(L) > 2 && Script::IsBoolean(L, 2)) {
        flag = Script::GetBool(L, 2);
    }
    uint8_t flag2 = Script::GetBoolOpt(L, 2, 0);
    snvVehicle_FollowPath(vehicle, Script::GetInt(L, 1), flag, flag2);
    return 0;
}

int __cdecl Native_VehicleSetCruiseSpeed(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snAt<uint8_t>(vehicle, 0x2E4) = uint8_t(Script::GetInt(L, 1));
    return 0;
}

int __cdecl Native_VehicleGetPosXYZ(lua_State* L) {
    const float* pos = snEntityPos(snvVehicleArg(L, 0));
    float x = pos[0], y = pos[1], z = pos[2];
    Script::PushFloat(L, x);
    Script::PushFloat(L, y);
    Script::PushFloat(L, z);
    return 3;
}

GAME_VAR(CPool<uint8_t>*, snvTriggerPool, 0x020C7C48);
GAME_FN(uint8_t, __thiscall, snvTrigger_Contains, 0x006D6FA0, void* trigger, const float* pos);

// vehicle, trigger slot (16 bits)
int __cdecl Native_VehicleIsInTrigger(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    int slot = uint16_t(Script::GetInt(L, 1));
    CPool<uint8_t>* pool = snvTriggerPool;
    uint8_t* trigger = (pool->flags[slot] & 0x80) != 0 ? nullptr : pool->objects + pool->elemSize * slot;
    Script::PushBool(L, snvTrigger_Contains(trigger, snEntityPos(vehicle)));
    return 1;
}

// (+0x2DA, +0x2DB cleared; a stale handle writes to address 0x2DA as the game does)
int __cdecl Native_VehicleStop(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snAt<uint8_t>(vehicle, 0x2DA) = 0;
    snAt<uint8_t>(vehicle, 0x2DB) = 0;
    return 0;
}

GAME_FN(void, __thiscall, snvMatrix_SetRotateZOnly, 0x00412C20, void* matrix, float heading);

// vehicle, heading (whole degrees)
int __cdecl Native_VehicleFaceHeading(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    float heading = float(double(Script::GetInt(L, 1)) * double(3.1415927f) / 180.0);
    if (snAt<void*>(vehicle, 0x14) != nullptr) {
        snvMatrix_SetRotateZOnly(snAt<void*>(vehicle, 0x14), heading);
    }
    snAt<float>(vehicle, 0x10) = heading;
    return 0;
}

int __cdecl Native_VehicleSetColours(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snAt<uint8_t>(vehicle, 0x334) = uint8_t(Script::GetInt(L, 1));
    snAt<uint8_t>(vehicle, 0x335) = uint8_t(Script::GetInt(L, 2));
    return 0;
}

GAME_FN(void, __cdecl, snvCarAI_Fn41E370, 0x0041E370, void* vehicle, uint8_t* out);
inline void* const snvPaths = reinterpret_cast<void*>(0x00BF6228);
GAME_FN(SnVec*, __thiscall, snvPaths_GetNodePosition, 0x00442270, void* paths, SnVec* out, int node);
GAME_FN(void, __cdecl, snvPlaceCarBetweenNodes, 0x0041FF70, void* vehicle, float between, float z, int flag);

// vehicle[, drive (true)]: a mission-owned vehicle (created by 1) made to drive like an
// ambient one (+0x2E0 20, +0x2DA 1, +0x2E4 speed 20); a car is put back between its
// nodes (+0x2A8, +0x2AC) at its projected place (0..1); dropped by the running script
int __cdecl Native_VehicleMakeAmbient(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    uint8_t drive = Script::GetBoolOpt(L, 1, 1);
    snvVehicle_SetCreatedBy(vehicle, 1);
    if (drive) {
        uint8_t result = 0;
        snvCarAI_Fn41E370(vehicle, &result);
        snAt<float>(vehicle, 0x2E0) = *reinterpret_cast<const float*>(0x0090CE58); // (20)
        snAt<uint8_t>(vehicle, 0x2DA) = 1;
        snAt<uint8_t>(vehicle, 0x2E4) = 0x14;
        if (snAt<int32_t>(vehicle, 0x46C) == 0) {
            SnVec a, b;
            snvPaths_GetNodePosition(snvPaths, &a, snAt<int32_t>(vehicle, 0x2A8));
            snvPaths_GetNodePosition(snvPaths, &b, snAt<int32_t>(vehicle, 0x2AC));
            const float* p = snEntityPos(vehicle);
            SnVec pos = {p[0], p[1], p[2]};
            SnVec d;
            snvVectorSub(&d, &b, &a);
            d.z = 0.0f;
            float lengthSq = std::fabs(d.x * d.x + d.y * d.y);
            snvVector_Normalise(&d);
            SnVec pa;
            snvVectorSub(&pa, &pos, &a);
            float dot = pa.x * d.x + pa.y * d.y + pa.z * d.z;
            float length = float(std::sqrt(double(lengthSq)));
            float t = dot / length;
            float between;
            if (t < 0.0f) {
                between = 0.0f;
            } else if (1.0f < t) {
                between = 1.0f;
            } else {
                between = t;
            }
            snvPlaceCarBetweenNodes(vehicle, between, 0.0f, 0);
        }
    }
    snvScript_RemoveVehicle(snCurrentScript(snLurStore), Script::GetInt(L, 0));
    return 0;
}

int __cdecl Native_VehicleIsModel(lua_State* L) {
    int model = snAt<int16_t>(snvVehicleArg(L, 0), 0x10E);
    Script::PushBool(L, model == Script::GetInt(L, 1));
    return 1;
}

int __cdecl Native_VehicleGetModelId(lua_State* L) {
    Script::PushInt(L, snAt<int16_t>(snvVehicleArg(L, 0), 0x10E));
    return 1;
}

GAME_FN(void, __thiscall, snvRef_Set, 0x005D7F10, re::Ptr32<void>* ref, void* entity);

// vehicle, ped: the owner (+0x358, a reference)
int __cdecl Native_VehicleSetOwner(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    uint8_t* ped = snPedArg(L, 1);
    if (ped != nullptr) {
        snvRef_Set(&snAt<void*>(vehicle, 0x358), ped);
    }
    return 0;
}

int __cdecl Native_VehicleSetAccelerationMult(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snAt<float>(vehicle, 0x364) = Script::GetFloat(L, 1);
    return 0;
}

GAME_FN(void*, __thiscall, snvBikeAttachment_Construct, 0x00464EF0, void* self);
GAME_FN(void, __thiscall, snvBikeAttachment_SetModel, 0x00465010, void* attachment, int model);

// bike (ignored), model (16 bits): an attachment made, given the model and deleted again
// (SetModel called on nullptr after a failed allocation)
int __cdecl Native_BikeAddAttachment(lua_State* L) {
    Script::GetInt(L, 0);
    int model = uint16_t(Script::GetInt(L, 1));
    void* mem = snMalloc(0x234);
    void* attachment = mem != nullptr ? snvBikeAttachment_Construct(mem) : nullptr;
    snvBikeAttachment_SetModel(attachment, model);
    if (attachment != nullptr) {
        snVCall<void>(attachment, 0, 1);
    }
    return 0;
}

int __cdecl Native_VehicleSetDrivingMode(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snAt<uint8_t>(vehicle, 0x2D9) = uint8_t(Script::GetInt(L, 1));
    return 0;
}

int __cdecl Native_VehicleSetMission(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snAt<uint8_t>(vehicle, 0x2DA) = uint8_t(Script::GetInt(L, 1));
    return 0;
}

// (the vehicle's virtual +0x18)
int __cdecl Native_VehicleSetStatus(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snVCall<void>(vehicle, 0x18, Script::GetInt(L, 1));
    return 0;
}

// A flag byte's bit from a bool (the game's xor-and-xor)
inline void snvSetBit(uint8_t& flags, uint8_t value, int bit) {
    flags ^= uint8_t((uint8_t(value << bit) ^ flags) & (1 << bit));
}

GAME_FN(void*, __cdecl, snvGetEntityFromHandle, 0x005C28D0, int handle, int kind, int pedItself, int cutscene);
GAME_FN(void, __cdecl, snvCarAI_FollowEntity, 0x0041A2C0, void* vehicle, void* entity, float distance, float);

// vehicle, handle, kind, distance (the game passes 15 after it)
int __cdecl Native_VehicleFollowEntity(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    int kind = Script::GetInt(L, 2);
    void* entity = snvGetEntityFromHandle(Script::GetInt(L, 1), kind, 0, 0);
    float distance = Script::GetFloat(L, 3);
    snvCarAI_FollowEntity(vehicle, entity, distance, *reinterpret_cast<const float*>(0x00900E6C)); // (15)
    return 0;
}

// (+0x6B8 bit 3)
int __cdecl Native_BikeSetPadlock(lua_State* L) {
    uint8_t* bike = snvVehicleArg(L, 0);
    snvSetBit(snAt<uint8_t>(bike, 0x6B8), Script::GetBool(L, 1), 3);
    return 0;
}

GAME_FN(uint8_t, __thiscall, snvVehicle_HasSirenModel, 0x004CB1F0, void* vehicle);

// (+0x36D, for a model with a siren)
int __cdecl Native_VehicleEnableSiren(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    if (snvVehicle_HasSirenModel(vehicle)) {
        snAt<uint8_t>(vehicle, 0x36D) = Script::GetBool(L, 1);
    }
    return 0;
}

int __cdecl Native_VehicleSetColor(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    uint8_t col1 = uint8_t(Script::GetInt(L, 1));
    uint8_t col2 = uint8_t(Script::GetInt(L, 2));
    snAt<uint8_t>(vehicle, 0x334) = col1;
    snAt<uint8_t>(vehicle, 0x335) = col2;
    return 0;
}

// (+0x33C bit 5, for a model with a siren)
int __cdecl Native_VehicleSirenAllwaysOn(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    if (snvVehicle_HasSirenModel(vehicle)) {
        snvSetBit(snAt<uint8_t>(vehicle, 0x33C), Script::GetBool(L, 1), 5);
    }
    return 0;
}

// A number that is a live vehicle's handle (its slot in range)
int __cdecl Native_VehicleIsValid(lua_State* L) {
    if (!Script::IsNumber(L, 0)) {
        Script::PushBool(L, 0);
        return 1;
    }
    int handle = Script::GetInt(L, 0);
    int slot = handle >> 8;
    CPool<uint8_t>* pool = snVehiclePool;
    if (handle >= 0 && slot < pool->size && pool->flags[slot] == (handle & 0xFF) && pool->flags[slot] == (handle & 0xFF) &&
        pool->objects + pool->elemSize * slot != nullptr) {
        Script::PushBool(L, 1);
        return 1;
    }
    Script::PushBool(L, 0);
    return 1;
}

GAME_FN(int, __thiscall, snvAutomobile_CountDamagedComponents, 0x004A8990, void* vehicle);

int __cdecl Native_CarGetDamageNumber(lua_State* L) {
    Script::PushInt(L, snvAutomobile_CountDamagedComponents(snvVehicleArg(L, 0)));
    return 1;
}

GAME_FN(void, __thiscall, snvBike_Jump, 0x004B9C20, void* bike, float strength);

// bike, strength (x 0.21)
int __cdecl Native_BikeJump(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    float strength = Script::GetFloat(L, 1);
    uint8_t* bike = snPoolAt(snVehiclePool, handle);
    snvBike_Jump(bike, float(double(strength) * double(0.21f)));
    return 0;
}

// (+0x33A bit 1; default on)
int __cdecl Native_VehicleEnableEngine(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t on = Script::GetBoolOpt(L, 1, 1);
    uint8_t* vehicle = snPoolAt(snVehiclePool, handle);
    if (vehicle != nullptr) {
        snvSetBit(snAt<uint8_t>(vehicle, 0x33A), on, 1);
    }
    return 0;
}

int __cdecl Native_VehicleGetHealth(lua_State* L) {
    Script::PushFloat(L, snAt<float>(snvVehicleArg(L, 0), 0x340));
    return 1;
}

// (the vehicle's virtual +0x1C, then +0x124 and +0x130 set to the "area clear" point;
// says 1 result without pushing one)
int __cdecl Native_VehicleSetStatic(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snVCall<void>(vehicle, 0x1C, int(Script::GetBool(L, 1)));
    const SnVec* clear = reinterpret_cast<const SnVec*>(0x00BCC564);
    snAt<SnVec>(vehicle, 0x124) = *clear;
    snAt<SnVec>(vehicle, 0x130) = *clear;
    return 1;
}

// (+0x33C bit 4)
int __cdecl Native_VehicleSetNISSafe(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    snvSetBit(snAt<uint8_t>(vehicle, 0x33C), Script::GetBool(L, 1), 4);
    return 0;
}

// vehicle, flag, on: the entity's dword flags from +0x1C (the index unchecked)
int __cdecl Native_VehicleSetEntityFlag(lua_State* L) {
    uint8_t* vehicle = snvVehicleArg(L, 0);
    int flag = Script::GetInt(L, 1);
    snAt<int32_t>(vehicle, 0x1C + flag * 4) = Script::GetBool(L, 2);
    return 0;
}

// (+0x6B9 bit 2)
int __cdecl Native_VehicleBikeForceBoundingSphereUpdate(lua_State* L) {
    uint8_t* bike = snvVehicleArg(L, 0);
    snvSetBit(snAt<uint8_t>(bike, 0x6B9), Script::GetBool(L, 1), 2);
    return 0;
}

GAME_VAR(void*, snvCamera, 0x00C3CCF4);
GAME_FN(void*, __cdecl, snvGetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, snvGetLocalTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t, __thiscall, snvPaths_NewGenerateCarCreationCoors_x64, 0x00440870, void* paths, float, float, float, float, float, float, float range, float minRange, int flag, SnVec* out, re::Ptr32<uint8_t>* node1, re::Ptr32<uint8_t>* node2, float* between, uintptr_t, uintptr_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t snvPaths_NewGenerateCarCreationCoors(void* a0, SnVec a1, SnVec a2, float a3, float a4, int a5, SnVec* a6, re::Ptr32<uint8_t>* a7, re::Ptr32<uint8_t>* a8, float* a9, uintptr_t a10, uintptr_t a11) {
    return snvPaths_NewGenerateCarCreationCoors_x64(a0, re::FloatAt(a1, 0), re::FloatAt(a1, 1), re::FloatAt(a1, 2), re::FloatAt(a2, 0), re::FloatAt(a2, 1), re::FloatAt(a2, 2), a3, a4, a5, a6, a7, a8, a9, a10, a11);
}
GAME_FN(float, __cdecl, snvFindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flags);
GAME_FN(void, __cdecl, snvFindObjectsInRange, 0x0045E3C0, const SnVec* pos, float range, int twoD, int16_t* count, int16_t maxCount,
        re::Ptr32<void>* out, uint32_t lists);

// model: a spot for a car ahead of the camera around the player (65 away), on a road with
// lanes, on the ground (the model's bounding box bottom on it) and clear of anything
// within twice its radius + 10; else (9999, 9999, 9999)
int __cdecl Native_VehicleFindRandomSpawnPosition(lua_State* L) {
    int model = Script::GetInt(L, 0);
    float none = *reinterpret_cast<const float*>(0x0090D1C8); // (9999)
    SnVec fallback = {none, none, none};
    float between = 0.0f;
    SnVec pos = *reinterpret_cast<const SnVec*>(0x00BCC564);
    re::Ptr32<uint8_t> node1 = nullptr;
    re::Ptr32<uint8_t> node2 = nullptr;
    uint8_t* transform = snvGetLocalTransform(snvGetCameraNode(snvCamera));
    SnVec dir = {snAt<float>(transform, 8), snAt<float>(transform, 0x14), 0.0f};
    const float* p = snEntityPos(snPlayerPed);
    SnVec playerPos = {p[0], p[1], p[2]};
    if (!snvPaths_NewGenerateCarCreationCoors(snvPaths, playerPos, dir, *reinterpret_cast<const float*>(0x00900E1C),
                                              *reinterpret_cast<const float*>(0x009055A0), 1, &pos, &node1, &node2, &between, 0, 0)) {
        Script::PushVector(L, reinterpret_cast<ScriptVector*>(&fallback));
        return 3;
    }
    float lanes1 = float(node1[0x16] & 0xF);
    float lanes2 = float(node2[0x16] & 0xF);
    float lanes = lanes2 < lanes1 ? lanes1 : lanes2;
    if (int(lanes) == 0) {
        Script::PushVector(L, reinterpret_cast<ScriptVector*>(&fallback));
        return 3;
    }
    uint8_t found;
    float groundZ = snvFindGroundZFor3DCoord(pos.x, pos.y, float(pos.z + 2.0), &found, 1);
    if (!found) {
        Script::PushVector(L, reinterpret_cast<ScriptVector*>(&fallback));
        return 3;
    }
    uint8_t* col = snAt<uint8_t*>(snModelInfoPtrs[model], 0xC);
    pos.z = float(double(snAt<float>(col, 0x18)) * -1.0 + groundZ);
    float radius = snAt<float>(snAt<uint8_t*>(snModelInfoPtrs[model], 0xC), 0xC);
    int32_t count = 0; // (an int16 the game clears as a dword)
    re::Ptr32<void> objects[8];
    snvFindObjectsInRange(&pos, float(double(radius + radius) + 10.0), 1, reinterpret_cast<int16_t*>(&count), 8, objects, 6);
    if (int16_t(count) > 0) {
        Script::PushVector(L, reinterpret_cast<ScriptVector*>(&fallback));
        return 3;
    }
    Script::PushVector(L, reinterpret_cast<ScriptVector*>(&pos));
    return 3;
}

GAME_FN(uint8_t*, __thiscall, snvRoutes_Get, 0x006D6450, void* routes, int id);
GAME_FN(uint8_t*, __thiscall, snvRoute_First, 0x006D59B0, void* route);
GAME_FN(uint8_t*, __thiscall, snvRoute_Next, 0x006D59F0, void* route);
inline void* const snvRoutes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(void, __thiscall, snvMatrix_SetRotateY, 0x00412CC0, void* matrix, float angle);
GAME_FN(void, __thiscall, snvMatrix_Multiply, 0x00412800, float* matrix, const float* by);

// CMatrix (as far as it's set here: 0x40 bytes of rows, +0x40 the attached RwMatrix)
struct SnvMatrix {
    float m[16];        // +0
    re::Ptr32<void> attached;     // +0x40
    uint8_t owns;       // +0x44
    uint8_t unknown45[3];
};
static_assert(sizeof(SnvMatrix) == 0x48);

// The route point (and its index) the vehicle is put at, facing the point's heading;
// [, place (true): through the vehicle's virtual +0xD4 again; tilt: a bike's roll in
// degrees]. Says 1 result without pushing one. (The matrix's +0x3C padding gets
// uninitialised stack in the game, left alone here)
int __cdecl Native_VehicleSetPosPoint(lua_State* L) {
    bool hasIndex = false;
    int index = 0; // (only read with the argument)
    if (lua::gettop(L) >= 3) {
        hasIndex = true;
        index = Script::GetInt(L, 2);
    }
    int handle = Script::GetInt(L, 0);
    uint8_t* route = snvRoutes_Get(snvRoutes, uint16_t(Script::GetInt(L, 1)));
    uint8_t* point = snvRoute_First(route);
    if (hasIndex) {
        for (int i = 1; i < index; i++) {
            uint8_t* next = snvRoute_Next(route);
            if (next == nullptr) {
                break;
            }
            point = next;
        }
    }
    float x = snAt<float>(point, 0), y = snAt<float>(point, 4), z = snAt<float>(point, 8);
    float pointHeading = snAt<float>(point, 0x1C);
    uint8_t* vehicle = snPoolAt(snVehiclePool, handle);
    float* matrix = snAt<float*>(vehicle, 0x14);
    if (matrix != nullptr) {
        matrix[12] = x;
        matrix[13] = y;
        matrix[14] = z;
    }
    snAt<float>(vehicle, 4) = x;
    snAt<float>(vehicle, 8) = y;
    snAt<float>(vehicle, 0xC) = z;
    float heading = float((double(pointHeading) - 90.0) * double(3.1415927f) / 180.0);
    if (snAt<void*>(vehicle, 0x14) != nullptr) {
        snvMatrix_SetRotateZOnly(snAt<void*>(vehicle, 0x14), heading);
    }
    snAt<float>(vehicle, 0x10) = heading;
    if (lua::gettop(L) < 4 || Script::GetBool(L, 3)) {
        float* m = snAt<float*>(vehicle, 0x14);
        float current = m != nullptr ? float(std::atan2(-double(m[4]), double(m[5]))) : snAt<float>(vehicle, 0x10);
        snVCall<void>(vehicle, 0xD4, x, y, z, current);
    }
    if (lua::gettop(L) >= 5) {
        float tilt = Script::GetFloat(L, 4);
        if (snAt<int32_t>(vehicle, 0x46C) == 1) {
            SnvMatrix roll;
            roll.attached = nullptr;
            snvMatrix_SetRotateY(&roll, float(double(tilt) * double(3.1415927f) / 180.0));
            snvMatrix_Multiply(snEntity_GetMatrix(vehicle), roll.m);
        }
    }
    return 1;
}

GAME_FN(uint8_t, __cdecl, snvIsBikeModel, 0x0051B3B0, int model);
GAME_FN(uint8_t*, __cdecl, snvGetStreamingInfo, 0x0052DD30, int model);
GAME_FN(void, __cdecl, snvRequestModel, 0x00532020, int model, int flags);
GAME_FN(void, __cdecl, snvLoadAllRequestedModels, 0x00533EE0, int onlyPriority);
GAME_FN(void, __cdecl, snvLoadingScreenMessage, 0x0043C4F0, const char* a, const char* b, uintptr_t);
GAME_FN(void*, __cdecl, snvVehicleNew, 0x004CC490, uintptr_t size);
GAME_FN(uint8_t*, __thiscall, snvBike_Construct, 0x004C6F70, void* mem, int model, int createdBy);
GAME_FN(uint8_t*, __thiscall, snvAutomobile_Construct, 0x004B7B80, void* mem, int model, int createdBy);
GAME_FN(float, __cdecl, snvFindGroundZ2D, 0x004634A0, float x, float y, int);
GAME_FN(float, __thiscall, snvEntity_GetDistanceFromCentreOfMassToBase, 0x00466110, void* entity);
GAME_FN(void, __cdecl, snvWorld_Add, 0x0045D6B0, void* entity);
GAME_VAR(int32_t, snvCurrArea, 0x00BD1008);
// the lowest z taken as given (below: the ground's)
GAME_VAR(float, snvCreateMinZ, 0x00A14A8C);

// 0x5D7870: a mission vehicle (created by 2) of the model (16 bits) at x, y, z (the
// ground's at or below the minimum), its base on it, facing `heading`; the model loaded
// at once (its +0x5C extra model first). Not with no free vehicle slot, nor for a car
// with anything within its radius. A bike is made static (its speeds zeroed) and kept in
// the special areas 0x1F, 0x2B, 0x3E. Returns the handle (16 bits), -1 for none
int16_t __cdecl CreateVehicle(lua_State*, int modelArg, float x, float y, float z, float heading) {
    CPool<uint8_t>* pool = snVehiclePool;
    int used = 0;
    for (int i = 0; i < pool->size; i++) {
        if ((pool->flags[i] & 0x80) == 0) {
            used++;
        }
    }
    if (pool->size - used <= 0) {
        return -1;
    }
    int model = uint16_t(modelArg);
    if (!snvIsBikeModel(model)) {
        float radius = snAt<float>(snAt<uint8_t*>(snModelInfoPtrs[model], 0xC), 0xC);
        SnVec pos = {x, y, z};
        int32_t count = 0; // (an int16 the game clears as a dword)
        re::Ptr32<void> objects[8];
        snvFindObjectsInRange(&pos, radius, 1, reinterpret_cast<int16_t*>(&count), 8, objects, 2);
        if (int16_t(count) > 0) {
            return -1;
        }
    }
    if (snvGetStreamingInfo(model)[4] != 1) {
        int extra = snAt<int16_t>(snModelInfoPtrs[model], 0x5C);
        if (extra > -1) {
            snvRequestModel(extra, 0x17);
            snvLoadingScreenMessage("LuaCreateVehicle()- 1", "CStreaming::LoadAllRequestedModels()", 0);
            snvLoadAllRequestedModels(0);
        }
        snvRequestModel(model, 0x16);
        snvScript_AddModel(snCurrentScript(snLurStore), model);
        snvLoadingScreenMessage("LuaCreateVehicle()- 2", "CStreaming::LoadAllRequestedModels()", 0);
        snvLoadAllRequestedModels(0);
    }
    float groundZ = z;
    uint8_t* vehicle;
    uint8_t* bike = nullptr;
    if (snvIsBikeModel(model)) {
        void* mem = snvVehicleNew(0x7A4);
        vehicle = mem != nullptr ? snvBike_Construct(mem, model, 2) : nullptr;
        snAt<uint8_t>(vehicle, 0x6B8) |= 2;
        bike = vehicle;
        int area = snvCurrArea;
        if (area == 0x1F || area == 0x2B || area == 0x3E) {
            snAt<uint8_t>(vehicle, 0x111) = uint8_t(area);
        }
    } else {
        void* mem = snvVehicleNew(0x8C0);
        vehicle = mem != nullptr ? snvAutomobile_Construct(mem, model, 2) : nullptr;
    }
    if (!(snvCreateMinZ < z)) {
        groundZ = snvFindGroundZ2D(x, y, 1);
    }
    groundZ = snvEntity_GetDistanceFromCentreOfMassToBase(vehicle) + groundZ;
    snVCall<void>(vehicle, 0xD4, x, y, groundZ, heading);
    snVCall<void>(vehicle, 0x18, 4);
    if (bike != nullptr) {
        snVCall<void>(vehicle, 0x1C, 1);
        const SnVec* clear = reinterpret_cast<const SnVec*>(0x00BCC564);
        snAt<SnVec>(vehicle, 0x124) = *clear;
        snAt<SnVec>(vehicle, 0x130) = *clear;
    }
    snAt<uint8_t>(vehicle, 0x33A) |= 1;
    snvCarAI_Fn41E370(vehicle, nullptr);
    snAt<uint8_t>(vehicle, 0x33A) &= 0xFD;
    snAt<float>(vehicle, 0x2E0) = *reinterpret_cast<const float*>(0x0090CE58); // (20)
    snAt<uint8_t>(vehicle, 0x2DA) = 0;
    snAt<uint8_t>(vehicle, 0x2DB) = 0;
    snAt<uint8_t>(vehicle, 0x2D9) = 0;
    snAt<uint8_t>(vehicle, 0x2E4) = 0x14;
    snAt<uint8_t>(vehicle, 0x2D7) = 0;
    snAt<uint8_t>(vehicle, 0x2D8) = 0;
    snvWorld_Add(vehicle);
    pool = snVehiclePool;
    int slot = int(vehicle - pool->objects) / pool->elemSize;
    return int16_t(uint16_t(pool->flags[slot] + (slot << 8)));
}

GAME_FN(int16_t, __cdecl, snvCreateVehicle, 0x005D7870, lua_State* L, int model, float x, float y, float z, float heading);
GAME_FN(void, __thiscall, snvScript_AddVehicle, 0x005D99E0, void* script, int handle);

// model (16 bits), route (16 bits)[, point]: facing the point's heading
int __cdecl Native_VehicleCreatePoint(lua_State* L) {
    bool hasIndex = false;
    int index = 0; // (only read with the argument)
    if (lua::gettop(L) == 3) {
        hasIndex = true;
        index = Script::GetInt(L, 2);
    }
    int model = uint16_t(Script::GetInt(L, 0));
    uint8_t* route = snvRoutes_Get(snvRoutes, uint16_t(Script::GetInt(L, 1)));
    uint8_t* point = snvRoute_First(route);
    if (hasIndex) {
        for (int i = 1; i < index; i++) {
            uint8_t* next = snvRoute_Next(route);
            if (next == nullptr) {
                break;
            }
            point = next;
        }
    }
    float heading = float((double(snAt<float>(point, 0x1C)) - 90.0) * double(3.1415927f) / 180.0);
    int handle = snvCreateVehicle(L, model, snAt<float>(point, 0), snAt<float>(point, 4), snAt<float>(point, 8), heading);
    if (handle != -1) {
        snvScript_AddVehicle(snCurrentScript(snLurStore), handle);
    }
    Script::PushInt(L, handle);
    return 1;
}

// [ped (0: the player)]: its vehicle's handle (16 bits), nothing for none
int __cdecl Native_VehicleFromDriver(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t* ped = handle != 0 ? snGetPed(handle, SN_HANDLE_PED) : static_cast<uint8_t*>(snPlayerPed);
    if (ped == nullptr || snAt<uint8_t*>(ped, 0x1554) == nullptr) {
        return 0;
    }
    CPool<uint8_t>* pool = snVehiclePool;
    int slot = int(snAt<uint8_t*>(ped, 0x1554) - pool->objects) / pool->elemSize;
    Script::PushInt(L, uint16_t(pool->flags[slot] + (slot << 8)));
    return 1;
}

// x, y, z, radius[, bikes only]: a table of the vehicles closer than the radius (3D,
// the last slot first), nil for none
int __cdecl Native_VehicleFindInAreaXYZ(lua_State* L) {
    ScriptVector center;
    Script::GetVector(&center, L, 0);
    float radius = Script::GetFloat(L, 3);
    uint8_t bikesOnly = Script::GetBoolOpt(L, 4, 0);
    CPool<uint8_t>* pool = snVehiclePool;
    int count = 0;
    for (int i = pool->size; i != 0;) {
        i--;
        if ((pool->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* vehicle = pool->objects + pool->elemSize * i;
        if (vehicle == nullptr || (bikesOnly && snAt<int32_t>(vehicle, 0x46C) != 1)) {
            continue;
        }
        SnVec d;
        snvVectorSub(&d, reinterpret_cast<const SnVec*>(snEntityPos(vehicle)), reinterpret_cast<const SnVec*>(&center));
        float lengthSq = std::fabs(d.x * d.x + d.y * d.y + d.z * d.z);
        float distance = float(std::sqrt(double(lengthSq)));
        if (distance < radius) {
            if (count == 0) {
                Script::NewTable(L);
            }
            int slot = int(vehicle - pool->objects) / pool->elemSize;
            Script::PushInt(L, pool->flags[slot] + slot * 0x100);
            count++;
            Script::RawSetI(L, -2, count);
        }
    }
    if (count == 0) {
        Script::PushNil(L);
    }
    return 1;
}

// model (16 bits), x, y, z: facing -90 degrees
int __cdecl Native_VehicleCreateXYZ(lua_State* L) {
    int model = uint16_t(Script::GetInt(L, 0));
    float z = Script::GetFloat(L, 3);
    float y = Script::GetFloat(L, 2);
    float x = Script::GetFloat(L, 1);
    int handle = snvCreateVehicle(L, model, x, y, z, *reinterpret_cast<const float*>(0x00900FB8)); // (-pi/2)
    if (handle != -1) {
        snvScript_AddVehicle(snCurrentScript(snLurStore), handle);
    }
    Script::PushInt(L, handle);
    return 1;
}

} // namespace

#define SNV_REVERSED(name, address)       \
    static void* Original_Native_##name; \
    REVERSED("Native_" #name, address, RE_BODY(Native_##name), nullptr, &Original_Native_##name, re::Mode::Original)

SNV_REVERSED(VehicleRequestModel, 0x005D5DC0);
SNV_REVERSED(VehicleIsInAreaXYZ, 0x005D5E70);
SNV_REVERSED(VehicleOverrideAmbient, 0x005D5F80);
SNV_REVERSED(VehicleCreateCarGenerator, 0x005D5FE0);
SNV_REVERSED(CanCreateVehicleNearXYZ, 0x005D60F0);
SNV_REVERSED(GetCurrentNumOfAmbientBikes, 0x005D6160);
static void* Original_DeleteVehicle;
REVERSED("Script::DeleteVehicle", 0x005D6180, RE_BODY(DeleteVehicle), nullptr, &Original_DeleteVehicle, re::Mode::Original);
SNV_REVERSED(VehicleSetPosXYZ, 0x005D61C0);
SNV_REVERSED(VehicleDelete, 0x005D63F0);
SNV_REVERSED(VehicleModelNotNeededAmbient, 0x005D6460);
SNV_REVERSED(VehicleDontCleanup, 0x005D64B0);
SNV_REVERSED(VehicleMoveToXYZ, 0x005D6510);
SNV_REVERSED(VehicleFollowPath, 0x005D6610);
SNV_REVERSED(VehicleSetCruiseSpeed, 0x005D66A0);
SNV_REVERSED(VehicleGetPosXYZ, 0x005D66F0);
SNV_REVERSED(VehicleIsInTrigger, 0x005D6790);
SNV_REVERSED(VehicleStop, 0x005D6820);
SNV_REVERSED(VehicleFaceHeading, 0x005D6870);
SNV_REVERSED(VehicleSetColours, 0x005D68F0);
SNV_REVERSED(VehicleMakeAmbient, 0x005D6950);
SNV_REVERSED(VehicleIsModel, 0x005D6B40);
SNV_REVERSED(VehicleGetModelId, 0x005D6BA0);
SNV_REVERSED(VehicleSetOwner, 0x005D6BF0);
SNV_REVERSED(VehicleSetAccelerationMult, 0x005D6C60);
SNV_REVERSED(BikeAddAttachment, 0x005D6CB0);
SNV_REVERSED(VehicleSetDrivingMode, 0x005D6D10);
SNV_REVERSED(VehicleSetMission, 0x005D6D60);
SNV_REVERSED(VehicleSetStatus, 0x005D6DB0);
SNV_REVERSED(VehicleFollowEntity, 0x005D6E00);
SNV_REVERSED(BikeSetPadlock, 0x005D6E80);
SNV_REVERSED(VehicleEnableSiren, 0x005D6EE0);
SNV_REVERSED(VehicleSetColor, 0x005D6F40);
SNV_REVERSED(VehicleSirenAllwaysOn, 0x005D6FA0);
SNV_REVERSED(VehicleIsValid, 0x005D7000);
SNV_REVERSED(CarGetDamageNumber, 0x005D70A0);
SNV_REVERSED(BikeJump, 0x005D70F0);
SNV_REVERSED(VehicleEnableEngine, 0x005D7150);
SNV_REVERSED(VehicleGetHealth, 0x005D71A0);
SNV_REVERSED(VehicleSetStatic, 0x005D71F0);
SNV_REVERSED(VehicleSetNISSafe, 0x005D7290);
SNV_REVERSED(VehicleSetEntityFlag, 0x005D72F0);
SNV_REVERSED(VehicleBikeForceBoundingSphereUpdate, 0x005D7350);
SNV_REVERSED(VehicleFindRandomSpawnPosition, 0x005D73B0);
SNV_REVERSED(VehicleSetPosPoint, 0x005D7640);
static void* Original_CreateVehicle;
REVERSED("Script::CreateVehicle", 0x005D7870, RE_BODY(CreateVehicle), nullptr, &Original_CreateVehicle, re::Mode::Original);
SNV_REVERSED(VehicleCreatePoint, 0x005D7B60);
SNV_REVERSED(VehicleFromDriver, 0x005D7C80);
SNV_REVERSED(VehicleFindInAreaXYZ, 0x005D7CF0);
SNV_REVERSED(VehicleCreateXYZ, 0x005D7E40);
