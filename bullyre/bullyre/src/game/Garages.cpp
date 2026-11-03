// The garages (GTA's CGarage / CGarages, cut down): a box with a door (a prop anim of
// the garage door model) that opens for the player and stores up to two vehicles,
// which come back at the garage's two places when it opens again.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Pools.h"
#include "game/Vehicle.h"
#include "reversed.h"

namespace c11garage {

using col::CVector;

enum eDoorState : int32_t {
    DOOR_CLOSED = 0,
    DOOR_OPEN = 1,
    DOOR_CLOSING = 2,
    DOOR_OPENING = 3,
};

// 0x4C bytes, CGarages::aGarages (0xC2DC48)
struct CGarage {
    CVector min;              // +0x0
    CVector max;              // +0xC
    CVector points[2];        // +0x18 where the stored vehicles come back
    float headings[2];        // +0x30
    int32_t numPoints;        // +0x38
    int32_t doorState;        // +0x3C eDoorState
    uint8_t isDeactivated;    // +0x40
    uint8_t unknown41[0x3];
    re::Ptr32<CEntity> door;            // +0x44 a registered reference
    float doorTimer;          // +0x48 0..6 while opening, down to 0 while closing

    // 0x4C8850: in the box grown by the margin
    uint8_t IsPointInside(const CVector* point, float margin);
    // 0x4C88D0
    uint8_t IsPointInside(const CVector* point);
    // 0x4C8D10: the first door model in the box (2 m slack) in a sector list, taken as
    // its door (unless `noOpen`: it's made unpathable and locked, +0x180 set)
    void FindDoorInList(re::Ptr32<uint32_t>* list, int noOpen);
    // 0x4C8E00: its door searched for in the sectors within 100 of it
    void FindDoor();
    // 0x4C9020 / 0x4C9080
    void OpenDoor();
    void CloseDoor();
    // 0x4C90D0: from the entity to the box, squared (x, y only)
    float DistanceSqToEntity(CEntity* entity);
    // 0x4C9190: the entity's position and every collision sphere inside (grown by the margin)
    uint8_t IsEntityEntirelyInside(CEntity* entity, float margin);
    // 0x4C92D0: the entity's bounding sphere and one of its collision spheres touch the box
    uint8_t IsEntityTouching(CEntity* entity);
    // 0x4C9410: neither the player, their vehicle nor the player's +0x2F8 entity is inside
    uint8_t IsPlayerOutside();
    // 0x4C9480
    uint32_t ShouldOpen();
    // 0x4C9680: a vehicle touching it has a sphere outside
    uint8_t IsAnyVehicleStickingOut();
    // 0x4C9750: the vehicles touching it whose position is outside, deleted
    void RemoveVehiclesStickingOut();
    // 0x4C97C0: the vehicles whose position is inside deleted (a stored one's model taken
    // again from it first)
    void StoreVehicles();
    // 0x4C9880
    uint32_t ShouldClose();
    // 0x4C9930: the stored vehicles' models requested; when all are loaded, those not
    // there made at its places (1: all loaded)
    uint8_t RestoreStoredVehicles();
    // 0x4C99D0
    void Update();
};
static_assert(offsetof(CGarage, max) == 0xC);
static_assert(offsetof(CGarage, points) == 0x18);
static_assert(offsetof(CGarage, headings) == 0x30);
static_assert(offsetof(CGarage, numPoints) == 0x38);
static_assert(offsetof(CGarage, doorState) == 0x3C);
static_assert(offsetof(CGarage, isDeactivated) == 0x40);
static_assert(offsetof(CGarage, door) == 0x44);
static_assert(offsetof(CGarage, doorTimer) == 0x48);
static_assert(sizeof(CGarage) == 0x4C);

// a vehicle the garages keep (CGarages::aStoredVehicles, 0xC2DEA8: two)
struct CStoredVehicle {
    int32_t model;        // +0x0 -1: none
    re::Ptr32<CEntity> vehicle;     // +0x4 a registered reference (while it's out)

    // 0x4C9560: made at the place (a mission vehicle of the player's), remembered
    CVehicle* RestoreVehicle(const CVector* pos, float heading);
};
static_assert(sizeof(CStoredVehicle) == 8);

// the save block
struct CGarageSaveData {
    int32_t version;      // +0x0 1
    int32_t size;         // +0x4 0x28
    int32_t count;        // +0x8 2
    int32_t models[2];    // +0xC
    int32_t numModels;    // +0x14
    uint8_t unknown18[0x10];  // +0x18 (checked as "GarageSaveData", 4 bytes)
};
static_assert(sizeof(CGarageSaveData) == 0x28);

GAME_VAR(uint8_t, numGarages, 0x00C2DC30);
GAME_ARRAY(CGarage, aGarages, 0x00C2DC48);
GAME_ARRAY(CStoredVehicle, aStoredVehicles, 0x00C2DEA8);

// 0x4C8940
void __cdecl SetAllDeactivated(uint8_t deactivated);
// 0x4C8960: the stored vehicle's model (the one out deleted)
void __cdecl SetStoredVehicle(int index, int model);
// 0x4C89B0
int32_t __cdecl GetStoredVehicleModel(int index);
// 0x4C89C0
uint8_t __cdecl IsStoredVehicle(CEntity* vehicle);
// 0x4C89F0: no stored model is waiting to come back
uint8_t __cdecl AreStoredVehiclesOut();
// 0x4C8A20: a stored vehicle is out and off screen
uint8_t __cdecl IsAnyStoredVehicleOffScreen();
// 0x4C8A60: the stored vehicles out and off screen deleted
void __cdecl RemoveOffScreenStoredVehicles();
// 0x4C8AB0 / 0x4C8B00
void __cdecl Save(CGarageSaveData* data);
void __cdecl Load(CGarageSaveData* data);
// 0x4C8B90
void __cdecl ClearAll();
// 0x4C8BA0: returns its index
int32_t __cdecl AddGarage(const CVector* min, const CVector* max, const CVector* points, const float* headings, int count);
// 0x4C9530
void __cdecl Init();
// 0x4C9B40
void __cdecl Update();

} // namespace c11garage

namespace {

using namespace c11garage;

GAME_FN(uint8_t, __thiscall, Garage_IsPointInsideMargin, 0x004C8850, CGarage* g, const CVector* point, float margin);
GAME_FN(uint8_t, __thiscall, Garage_IsPointInside, 0x004C88D0, CGarage* g, const CVector* point);
GAME_FN(void, __thiscall, Garage_FindDoorInList, 0x004C8D10, CGarage* g, re::Ptr32<uint32_t>* list, int noOpen);
GAME_FN(void, __thiscall, Garage_FindDoor, 0x004C8E00, CGarage* g);
GAME_FN(void, __thiscall, Garage_OpenDoor, 0x004C9020, CGarage* g);
GAME_FN(void, __thiscall, Garage_CloseDoor, 0x004C9080, CGarage* g);
GAME_FN(float, __thiscall, Garage_DistanceSqToEntity, 0x004C90D0, CGarage* g, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Garage_IsEntityEntirelyInside, 0x004C9190, CGarage* g, CEntity* entity, float margin);
GAME_FN(uint8_t, __thiscall, Garage_IsEntityTouching, 0x004C92D0, CGarage* g, CEntity* entity);
GAME_FN(uint8_t, __thiscall, Garage_IsPlayerOutside, 0x004C9410, CGarage* g);
GAME_FN(uint32_t, __thiscall, Garage_ShouldOpen, 0x004C9480, CGarage* g);
GAME_FN(CVehicle*, __thiscall, Stored_RestoreVehicle, 0x004C9560, CStoredVehicle* s, const CVector* pos, float heading);
GAME_FN(uint8_t, __thiscall, Garage_IsAnyVehicleStickingOut, 0x004C9680, CGarage* g);
GAME_FN(void, __thiscall, Garage_RemoveVehiclesStickingOut, 0x004C9750, CGarage* g);
// (not in the function list: every vehicle inside stored and deleted)
GAME_FN(void, __thiscall, Garage_StoreVehicles, 0x004C97C0, CGarage* g);
GAME_FN(uint32_t, __thiscall, Garage_ShouldClose, 0x004C9880, CGarage* g);
GAME_FN(uint8_t, __thiscall, Garage_RestoreStoredVehicles, 0x004C9930, CGarage* g);
GAME_FN(void, __thiscall, Garage_Update, 0x004C99D0, CGarage* g);
GAME_FN(uint8_t, __cdecl, Garages_IsStoredVehicle, 0x004C89C0, CEntity* vehicle);
GAME_FN(uint8_t, __cdecl, Garages_IsAnyStoredVehicleOffScreen, 0x004C8A20);
GAME_FN(void, __cdecl, Garages_RemoveOffScreenStoredVehicles, 0x004C8A60);
GAME_FN(void, __thiscall, Vehicle_4CC3D0, 0x004CC3D0, CVehicle* vehicle);

GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<CEntity>* ref, CEntity* entity);
GAME_FN(CEntity*, __thiscall, Ref_Get, 0x00773DB0, re::Ptr32<CEntity>* ref);
GAME_FN(void, __thiscall, CopyInt, 0x005FA880, int32_t* dst, const int32_t* src);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, CEntity* entity);
GAME_FN(void, __cdecl, World_ClearScanCodes, 0x0045D840);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, CEntity* entity);
GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, CEntity* entity);
GAME_FN(CColModel*, __cdecl, GetEntityColModel, 0x0051AF50, CEntity* entity);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* MultiplyPoint(CVector* out, const void* matrix, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, matrix, v); }
GAME_FN(CEntity*, __cdecl, GetEntityFromNode, 0x0044A3E0, int type, int index);
GAME_FN(void, __cdecl, Door_SetPathableToPeds, 0x005EB060, CEntity* door, uint8_t pathable);
GAME_FN(void, __thiscall, PropAnim_SetUnknown17C, 0x004EA3B0, CEntity* prop, int value);
GAME_FN(void, __thiscall, PropAnim_OpenDoor, 0x004EED10, CEntity* prop, int, uintptr_t, int immediate);
GAME_FN(void, __thiscall, PropAnim_CloseDoor, 0x004EEEC0, CEntity* prop, uintptr_t);
GAME_FN(void, __cdecl, Stat_AddToInt, 0x00458480, int stat, int amount);
GAME_FN(CVehicle*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(int16_t, __cdecl, CreateVehicle, 0x005D7870, uintptr_t arg, int model, float x, float y, float z, float heading);
GAME_FN(uint8_t, __cdecl, IsVehicleClass1, 0x0051B3B0, int model);
GAME_FN(void, __thiscall, Vehicle_SetCreatedBy, 0x004CB230, CVehicle* vehicle, int createdBy);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(uint8_t, __cdecl, Save_CheckValue, 0x00733320, int, int, int value, int expected);
GAME_FN(uint8_t, __cdecl, Save_CheckBlock, 0x00733620, const char* name, void* data, int size);

GAME_VAR(CEntity*, playerPed, 0x00C1AEA8);
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);
GAME_VAR(uint32_t*, ptrNodes, 0x00C0F788);        // the sector lists' nodes
GAME_VAR(int32_t, garageDoorModel, 0x00BF42C8);
GAME_VAR(int32_t, modelBF4120, 0x00BF4120);
GAME_VAR(int32_t, modelBF4130, 0x00BF4130);
GAME_VAR(uint8_t, colourBF4120, 0x00906888);      // the colours those two come back in
GAME_VAR(uint8_t, colourBF4130, 0x00906889);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
// the world's sectors (36 x 36, 0x14 bytes: lists at +4 and +0x10)
inline uint8_t* const worldSectors = reinterpret_cast<uint8_t*>(0x00C1B178);
inline const char* const garageSaveDataName = reinterpret_cast<const char*>(0x00912E94);   // "GarageSaveData"

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

uint8_t* Bytes(void* p) {
    return static_cast<uint8_t*>(p);
}
const CVector& Position(CEntity* entity) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(Bytes(entity) + 0x14);
    return *reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : Bytes(entity) + 4);
}
void* MatrixOf(CEntity* entity) {
    return *reinterpret_cast<re::Ptr32<void>*>(Bytes(entity) + 0x14);
}
int16_t ModelOf(CEntity* entity) {
    return *reinterpret_cast<int16_t*>(Bytes(entity) + 0x10E);
}
uint8_t TypeOf(CEntity* entity) {
    return Bytes(entity)[0x108] & 7;
}

// (inlined: IsPointInside with the margin, on the entity's position)
bool IsInsideBox(const CGarage* g, const CVector& p, float margin) {
    if (p.x < double(g->min.x) - margin) return false;
    if (double(g->max.x) + margin < p.x) return false;
    if (double(g->min.y) - margin > p.y) return false;
    if (double(g->max.y) + margin < p.y) return false;
    if (double(g->min.z) - margin > p.z) return false;
    if (p.z > double(g->max.z) + margin) return false;
    return true;
}

CVehicle* VehicleAt(CPool<CVehicle>* pool, int slot) {
    return reinterpret_cast<CVehicle*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot);
}

void DeleteEntity(CEntity* entity) {
    World_Remove(entity);
    CallVirtual<void>(entity, 0, 1);
}

} // namespace

namespace c11garage {

uint8_t CGarage::IsPointInside(const CVector* point, float margin) {
    return IsInsideBox(this, *point, margin) ? 1 : 0;
}

uint8_t CGarage::IsPointInside(const CVector* point) {
    if (min.x > point->x) return 0;
    if (max.x < point->x) return 0;
    if (min.y > point->y) return 0;
    if (max.y < point->y) return 0;
    if (min.z > point->z) return 0;
    if (max.z < point->z) return 0;
    return 1;
}

void __cdecl SetAllDeactivated(uint8_t deactivated) {
    for (int i = numGarages; i > 0; i--) {
        aGarages[numGarages - i].isDeactivated = deactivated;
    }
}

void __cdecl SetStoredVehicle(int index, int model) {
    CStoredVehicle& stored = aStoredVehicles[index];
    if (stored.vehicle) {
        World_Remove(stored.vehicle);
        if (stored.vehicle) {
            CallVirtual<void>(stored.vehicle, 0, 1);
        }
    }
    SetRef(&stored.vehicle, nullptr);
    stored.model = model;
}

int32_t __cdecl GetStoredVehicleModel(int index) {
    return aStoredVehicles[index].model;
}

uint8_t __cdecl IsStoredVehicle(CEntity* vehicle) {
    for (int i = 0; i < 2; i++) {
        if (aStoredVehicles[i].vehicle == vehicle) {
            return 1;
        }
    }
    return 0;
}

uint8_t __cdecl AreStoredVehiclesOut() {
    uint8_t out = 1;
    if (aStoredVehicles[0].model >= 0 && aStoredVehicles[0].vehicle == nullptr) {
        out = 0;
    }
    if (aStoredVehicles[1].model >= 0 && aStoredVehicles[1].vehicle == nullptr) {
        out = 0;
    }
    return out;
}

uint8_t __cdecl IsAnyStoredVehicleOffScreen() {
    for (int i = 0; i < 2; i++) {
        if (aStoredVehicles[i].vehicle) {
            CEntity* vehicle = Ref_Get(&aStoredVehicles[i].vehicle);
            if (vehicle && !Entity_GetIsOnScreen(vehicle)) {
                return 1;
            }
        }
    }
    return 0;
}

void __cdecl RemoveOffScreenStoredVehicles() {
    for (int i = 0; i < 2; i++) {
        if (aStoredVehicles[i].vehicle) {
            CEntity* vehicle = Ref_Get(&aStoredVehicles[i].vehicle);
            if (vehicle && !Entity_GetIsOnScreen(vehicle)) {
                DeleteEntity(vehicle);
            }
        }
    }
}

void __cdecl Save(CGarageSaveData* data) {
    data->version = 1;
    data->size = 0x28;
    data->count = 2;
    for (int i = 0; i < 2; i++) {
        CopyInt(&data->models[i], &aStoredVehicles[i].model);
    }
    data->numModels = 2;
}

void __cdecl Load(CGarageSaveData* data) {
    if (!Save_CheckValue(0, 0, data->version, 1) || !Save_CheckValue(0, 0, data->size, 0x28) ||
        !Save_CheckValue(0, 0, data->count, 2) || !Save_CheckBlock(garageSaveDataName, data->unknown18, 4)) {
        return;
    }
    for (int i = 0; i < data->numModels; i++) {
        aStoredVehicles[i].model = data->models[i];
    }
}

void __cdecl ClearAll() {
    numGarages = 0;
}

int32_t __cdecl AddGarage(const CVector* boxMin, const CVector* boxMax, const CVector* points, const float* headings,
                          int count) {
    CGarage& g = aGarages[numGarages];
    g.min = *boxMin;
    g.max = *boxMax;
    // (more than two places run over into the fields after them, as in the game)
    for (int i = 0; i < count; i++) {
        g.points[i] = points[i];
        g.headings[i] = headings[i];
    }
    numGarages++;
    g.numPoints = count;
    g.doorState = DOOR_CLOSED;
    g.isDeactivated = 0;
    return int32_t(numGarages) - 1;
}

void CGarage::FindDoorInList(re::Ptr32<uint32_t>* list, int noOpen) {
    uint32_t* node = *list;
    while (node) {
        uint32_t value = *node;
        CEntity* entity = GetEntityFromNode(value & 0xF, (value >> 4) & 0x3FFF);
        node = (value >> 18) == 0x3FFF ? nullptr : &ptrNodes[value >> 18];
        uint16_t& scanCode = *reinterpret_cast<uint16_t*>(Bytes(entity) + 0x10A);
        if (scanCode == currentScanCode) {
            continue;
        }
        scanCode = currentScanCode;
        if (ModelOf(entity) != garageDoorModel) {
            continue;
        }
        if (!Garage_IsPointInsideMargin(this, &Position(entity), 2.0f) || door) {
            continue;
        }
        SetRef(&door, entity);
        if (uint8_t(noOpen)) {
            continue;
        }
        CEntity* d = Ref_Get(&door);
        if (d) {
            Door_SetPathableToPeds(d, 0);
            PropAnim_SetUnknown17C(d, 1);
            *reinterpret_cast<int32_t*>(Bytes(d) + 0x180) = 1;
        }
    }
}

void CGarage::FindDoor() {
    SetRef(&door, nullptr);
    double centreX = (double(max.x) + min.x) * 0.5;
    float right = float(centreX + 100.0);
    double centreY = (double(max.y) + min.y) * 0.5;
    float bottom = float(centreY - 100.0);
    float top = float(centreY + 100.0);
    float left = float(centreX - 100.0);

    // the sector range (GetSectorX / Y, clamped to 0..35)
    float value = float(int(double(left) / 50.0 + 18.0));
    int x1 = int(value > 0.0f ? value : 0.0f);
    value = float(int(double(bottom) / 50.0 + 18.0));
    int y1 = int(value > 0.0f ? value : 0.0f);
    value = float(int(double(right) / 50.0 + 18.0));
    int x2 = int(value < 35.0f ? value : 35.0f);
    value = float(int(double(top) / 50.0 + 18.0));
    int y2 = int(value < 35.0f ? value : 35.0f);

    if (currentScanCode < 0xFFFF) {
        currentScanCode++;
    } else {
        World_ClearScanCodes();
        currentScanCode = 1;
    }
    for (int y = y1; y <= y2; y++) {
        if (door) {
            break;
        }
        for (int x = x1; x <= x2 && !door; x++) {
            uint8_t* sector = worldSectors + (y * 36 + x) * 0x14;
            Garage_FindDoorInList(this, reinterpret_cast<re::Ptr32<uint32_t>*>(sector + 4), 0);
            Garage_FindDoorInList(this, reinterpret_cast<re::Ptr32<uint32_t>*>(sector + 0x10), 1);
        }
    }
    if (door) {
        *reinterpret_cast<int32_t*>(Bytes(door) + 0x20) = 1;
    }
}

void CGarage::OpenDoor() {
    if ((doorState == DOOR_CLOSED || doorState == DOOR_CLOSING) && door && TypeOf(door) == 6) {
        doorState = DOOR_OPENING;
        CEntity* d = Ref_Get(&door);
        if (d) {
            Door_SetPathableToPeds(d, 1);
            PropAnim_OpenDoor(d, 0, 0, 0);
            Stat_AddToInt(0x34, 1);
        }
    }
}

void CGarage::CloseDoor() {
    if (doorState == DOOR_OPEN || doorState == DOOR_OPENING) {
        doorState = DOOR_CLOSING;
        if (door && TypeOf(door) == 6) {
            CEntity* d = Ref_Get(&door);
            if (d) {
                Door_SetPathableToPeds(d, 0);
                PropAnim_CloseDoor(d, 0);
            }
        }
    }
}

float CGarage::DistanceSqToEntity(CEntity* entity) {
    float x = Position(entity).x;
    float y = Position(entity).y;
    float dx;
    if (min.x > x) {
        dx = x - min.x;
    } else if (max.x < x) {
        dx = x - max.x;
    } else {
        dx = 0.0f;
    }
    float dy;
    if (min.y > y) {
        dy = y - min.y;
    } else if (max.y < y) {
        dy = y - max.y;
    } else {
        dy = 0.0f;
    }
    return float(double(dx) * dx + double(dy) * dy);
}

uint8_t CGarage::IsEntityEntirelyInside(CEntity* entity, float margin) {
    if (!IsInsideBox(this, Position(entity), margin)) {
        return 0;
    }
    CCollisionData* data = GetEntityColModel(entity)->data;
    int numSpheres = int8_t(data->numSpheres);
    void* matrix = MatrixOf(entity);
    CColSphere* sphere = data->spheres;
    for (int i = 0; i < numSpheres; i++, sphere++) {
        struct { CVector v; float pad; } pointPad16;   // (the game's transform writes a 4th float)
        CVector& point = pointPad16.v;
        MultiplyPoint(&point, matrix, &sphere->center);
        if (!Garage_IsPointInsideMargin(this, &point, margin - sphere->radius)) {
            return 0;
        }
    }
    return 1;
}

uint8_t CGarage::IsEntityTouching(CEntity* entity) {
    float radius = Entity_GetBoundRadius(entity);
    if (!IsInsideBox(this, Position(entity), radius)) {
        return 0;
    }
    CCollisionData* data = GetEntityColModel(entity)->data;
    int numSpheres = int8_t(data->numSpheres);
    void* matrix = MatrixOf(entity);
    CColSphere* sphere = data->spheres;
    for (int i = 0; i < numSpheres; i++, sphere++) {
        struct { CVector v; float pad; } pointPad16;   // (the game's transform writes a 4th float)
        CVector& point = pointPad16.v;
        MultiplyPoint(&point, matrix, &sphere->center);
        if (Garage_IsPointInsideMargin(this, &point, sphere->radius)) {
            return 1;
        }
    }
    return 0;
}

uint8_t CGarage::IsPlayerOutside() {
    CEntity* ped = playerPed;
    CEntity* other = *reinterpret_cast<re::Ptr32<CEntity>*>(Bytes(ped) + 0x2F8);
    uint8_t outside = 1;
    CVehicle* vehicle = FindPlayerVehicle(0);
    if (vehicle && Garage_IsEntityEntirelyInside(this, reinterpret_cast<CEntity*>(vehicle), 1.0f)) {
        outside = 0;
    }
    if (Garage_IsEntityEntirelyInside(this, ped, 1.0f)) {
        outside = 0;
    }
    if (other && Garage_IsEntityEntirelyInside(this, other, 1.0f)) {
        return 0;
    }
    return outside;
}

uint32_t CGarage::ShouldOpen() {
    float distSq = Garage_DistanceSqToEntity(this, playerPed);
    uint8_t out = 1;
    if (aStoredVehicles[0].model >= 0 && aStoredVehicles[0].vehicle == nullptr) {
        out = 0;
    }
    bool secondWaiting = aStoredVehicles[1].model >= 0 && aStoredVehicles[1].vehicle == nullptr;
    if (secondWaiting || !out || Garages_IsAnyStoredVehicleOffScreen()) {
        // on foot and close
        if (!FindPlayerVehicle(0) && distSq <= 9.0f) {
            return 1;
        }
    }
    CVehicle* vehicle = FindPlayerVehicle(0);
    if (vehicle) {
        vehicle = FindPlayerVehicle(0);
        for (int i = 0; i < 2; i++) {
            if (aStoredVehicles[i].vehicle == reinterpret_cast<CEntity*>(vehicle)) {
                // in a stored vehicle, a bit further
                return distSq <= 64.0f ? 1 : 0;
            }
        }
    }
    return 0;
}

CVehicle* CStoredVehicle::RestoreVehicle(const CVector* pos, float heading) {
    int16_t handle = CreateVehicle(0, uint16_t(model), pos->x, pos->y, pos->z, heading);
    if (handle == -1) {
        return nullptr;
    }
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    int slot = int(handle) >> 8;
    if (pool->flags[slot] != uint8_t(handle)) {
        return nullptr;
    }
    CVehicle* v = VehicleAt(pool, slot);
    if (!v) {
        return nullptr;
    }
    if (IsVehicleClass1(model)) {
        reinterpret_cast<uint8_t*>(v)[0x6B8] |= 2;
    }
    CallVirtual<void>(v, 6, 4);   // SetStatus
    v->unknown354 = 1;
    Vehicle_SetCreatedBy(v, 4);
    Vehicle_4CC3D0(v);
    veh::SetRef(&v->owner, playerPed);
    if (model == modelBF4120) {
        v->colour1 = colourBF4120;
        v->colour2 = colourBF4120;
    } else if (model == modelBF4130) {
        v->colour1 = colourBF4130;
        v->colour2 = colourBF4130;
    }
    SetRef(&vehicle, reinterpret_cast<CEntity*>(v));
    return v;
}

uint8_t CGarage::IsAnyVehicleStickingOut() {
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        CVehicle* v = VehicleAt(pool, slot);
        if (!v || !Garage_IsEntityTouching(this, reinterpret_cast<CEntity*>(v))) {
            continue;
        }
        CCollisionData* data = (*reinterpret_cast<re::Ptr32<CColModel>*>(modelInfoPtrs[v->modelIndex] + 0xC))->data;
        int numSpheres = int8_t(data->numSpheres);
        CColSphere* sphere = data->spheres;
        for (int i = 0; i < numSpheres; i++, sphere++) {
            struct { CVector v; float pad; } pointPad16;   // (the game's transform writes a 4th float)
            CVector& point = pointPad16.v;
            MultiplyPoint(&point, v->matrix, &sphere->center);
            if (!Garage_IsPointInsideMargin(this, &point, sphere->radius)) {
                return 1;
            }
        }
    }
    return 0;
}

void CGarage::RemoveVehiclesStickingOut() {
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        CVehicle* v = VehicleAt(pool, slot);
        CEntity* entity = reinterpret_cast<CEntity*>(v);
        if (!v || !Garage_IsEntityTouching(this, entity)) {
            continue;
        }
        if (!Garage_IsPointInside(this, &Position(entity))) {
            DeleteEntity(entity);
        }
    }
}

void CGarage::StoreVehicles() {
    CPool<CVehicle>* pool = CPools::ms_pVehiclePool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        CVehicle* v = VehicleAt(pool, slot);
        CEntity* entity = reinterpret_cast<CEntity*>(v);
        if (!v || !Garage_IsPointInside(this, &Position(entity))) {
            continue;
        }
        for (int i = 0; i < 2; i++) {
            CStoredVehicle& stored = aStoredVehicles[i];
            if (stored.vehicle == entity) {
                SetRef(&stored.vehicle, nullptr);
                stored.model = -1;
                stored.model = ModelOf(entity);
                SetRef(&stored.vehicle, entity);
            }
        }
        DeleteEntity(entity);
    }
}

uint32_t CGarage::ShouldClose() {
    float distSq = Garage_DistanceSqToEntity(this, playerPed);
    bool checkFar = Garage_IsAnyVehicleStickingOut(this) != 0;
    if (!checkFar) {
        if (64.0f < distSq) {
            return Garage_IsPlayerOutside(this) ? 1 : 0;
        }
        if (!FindPlayerVehicle(0) && 9.0f < distSq) {
            return Garage_IsPlayerOutside(this) ? 1 : 0;
        }
        if (FindPlayerVehicle(0) && !Garages_IsStoredVehicle(reinterpret_cast<CEntity*>(FindPlayerVehicle(0)))) {
            return Garage_IsPlayerOutside(this) ? 1 : 0;
        }
    }
    if (!(144.0f < distSq)) {
        return 0;
    }
    return Garage_IsPlayerOutside(this) ? 1 : 0;
}

uint8_t CGarage::RestoreStoredVehicles() {
    uint8_t allLoaded = 1;
    for (int i = 0; i < 2; i++) {
        CStoredVehicle& stored = aStoredVehicles[i];
        if (stored.model >= 0 && !stored.vehicle) {
            RequestModel(stored.model, 4);
            if (GetStreamingInfo(stored.model)[4] != 1) {
                allLoaded = 0;
            }
        }
    }
    if (!allLoaded) {
        return 0;
    }
    for (int i = 0; i < 2; i++) {
        CStoredVehicle& stored = aStoredVehicles[i];
        if (stored.model >= 0 && !stored.vehicle) {
            Stored_RestoreVehicle(&stored, &points[i], headings[i]);
        }
    }
    return allLoaded;
}

void CGarage::Update() {
    if (!door) {
        Garage_FindDoor(this);
    }
    if (isDeactivated) {
        if (doorState == DOOR_CLOSED) {
            return;
        }
        switch (doorState) {
        case DOOR_OPEN:
        case DOOR_OPENING:
            Garage_CloseDoor(this);
            return;
        case DOOR_CLOSING:
            doorTimer = float(doorTimer - timeStep * 0.10000000149011612);
            if (doorTimer <= 0.0f) {
                doorTimer = 0.0f;
                doorState = DOOR_CLOSED;
            }
            return;
        default:
            return;
        }
    }
    if (!door) {
        return;
    }
    switch (doorState) {
    case DOOR_CLOSED:
        if (Garage_ShouldOpen(this)) {
            Garages_RemoveOffScreenStoredVehicles();
            if (Garage_RestoreStoredVehicles(this)) {
                Garage_OpenDoor(this);
            }
        }
        break;
    case DOOR_OPEN:
        if (Garage_ShouldClose(this)) {
            Garage_RemoveVehiclesStickingOut(this);
            Garage_CloseDoor(this);
        }
        break;
    case DOOR_CLOSING:
        doorTimer = float(doorTimer - timeStep * 0.10000000149011612);
        if (!Garage_IsPlayerOutside(this)) {
            Garage_OpenDoor(this);
        } else if (doorTimer <= 0.0f) {
            doorTimer = 0.0f;
            doorState = DOOR_CLOSED;
            Garage_StoreVehicles(this);
        }
        break;
    case DOOR_OPENING:
        doorTimer = float(timeStep * 0.10000000149011612 + doorTimer);
        if (doorTimer >= 6.0f) {
            doorTimer = 6.0f;
            doorState = DOOR_OPEN;
        }
        break;
    default:
        break;
    }
}

void __cdecl Init() {
    numGarages = 0;
    for (int i = 0; i < 2; i++) {
        SetRef(&aStoredVehicles[i].vehicle, nullptr);
        aStoredVehicles[i].model = -1;
    }
}

void __cdecl Update() {
    for (int i = 0; i < numGarages; i++) {
        Garage_Update(&aGarages[i]);
    }
}

} // namespace c11garage

// ---- registration ---------------------------------------------------------------------

#define GARAGE_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
#define GARAGE_MEMBER(name, address, member) GARAGE_REVERSED(name, address, member)

using c11garage::CGarage;

GARAGE_MEMBER("CGarage::IsPointInsideMargin", 0x004C8850,
              static_cast<uint8_t (CGarage::*)(const col::CVector*, float)>(&CGarage::IsPointInside));
GARAGE_MEMBER("CGarage::IsPointInside", 0x004C88D0,
              static_cast<uint8_t (CGarage::*)(const col::CVector*)>(&CGarage::IsPointInside));
GARAGE_REVERSED("CGarages::SetAllDeactivated", 0x004C8940, &c11garage::SetAllDeactivated);
GARAGE_REVERSED("CGarages::SetStoredVehicle", 0x004C8960, &c11garage::SetStoredVehicle);
GARAGE_REVERSED("CGarages::GetStoredVehicleModel", 0x004C89B0, &c11garage::GetStoredVehicleModel);
GARAGE_REVERSED("CGarages::IsStoredVehicle", 0x004C89C0, &c11garage::IsStoredVehicle);
GARAGE_REVERSED("CGarages::AreStoredVehiclesOut", 0x004C89F0, &c11garage::AreStoredVehiclesOut);
GARAGE_REVERSED("CGarages::IsAnyStoredVehicleOffScreen", 0x004C8A20, &c11garage::IsAnyStoredVehicleOffScreen);
GARAGE_REVERSED("CGarages::RemoveOffScreenStoredVehicles", 0x004C8A60, &c11garage::RemoveOffScreenStoredVehicles);
GARAGE_REVERSED("CGarages::Save", 0x004C8AB0, &c11garage::Save);
GARAGE_REVERSED("CGarages::Load", 0x004C8B00, &c11garage::Load);
GARAGE_REVERSED("CGarages::ClearAll", 0x004C8B90, &c11garage::ClearAll);
GARAGE_REVERSED("CGarages::AddGarage", 0x004C8BA0, &c11garage::AddGarage);
GARAGE_MEMBER("CGarage::FindDoorInList", 0x004C8D10, &CGarage::FindDoorInList);
GARAGE_MEMBER("CGarage::FindDoor", 0x004C8E00, &CGarage::FindDoor);
GARAGE_MEMBER("CGarage::OpenDoor", 0x004C9020, &CGarage::OpenDoor);
GARAGE_MEMBER("CGarage::CloseDoor", 0x004C9080, &CGarage::CloseDoor);
GARAGE_MEMBER("CGarage::DistanceSqToEntity", 0x004C90D0, &CGarage::DistanceSqToEntity);
GARAGE_MEMBER("CGarage::IsEntityEntirelyInside", 0x004C9190, &CGarage::IsEntityEntirelyInside);
GARAGE_MEMBER("CGarage::IsEntityTouching", 0x004C92D0, &CGarage::IsEntityTouching);
GARAGE_MEMBER("CGarage::IsPlayerOutside", 0x004C9410, &CGarage::IsPlayerOutside);
GARAGE_MEMBER("CGarage::ShouldOpen", 0x004C9480, &CGarage::ShouldOpen);
GARAGE_REVERSED("CGarages::Init", 0x004C9530, &c11garage::Init);
GARAGE_MEMBER("CStoredVehicle::RestoreVehicle", 0x004C9560, &c11garage::CStoredVehicle::RestoreVehicle);
GARAGE_MEMBER("CGarage::IsAnyVehicleStickingOut", 0x004C9680, &CGarage::IsAnyVehicleStickingOut);
GARAGE_MEMBER("CGarage::RemoveVehiclesStickingOut", 0x004C9750, &CGarage::RemoveVehiclesStickingOut);
GARAGE_MEMBER("CGarage::StoreVehicles", 0x004C97C0, &CGarage::StoreVehicles);
GARAGE_MEMBER("CGarage::ShouldClose", 0x004C9880, &CGarage::ShouldClose);
GARAGE_MEMBER("CGarage::RestoreStoredVehicles", 0x004C9930, &CGarage::RestoreStoredVehicles);
GARAGE_MEMBER("CGarage::Update", 0x004C99D0, &CGarage::Update);
GARAGE_REVERSED("CGarages::Update", 0x004C9B40, &c11garage::Update);
