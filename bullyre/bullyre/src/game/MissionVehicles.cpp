// The missions' vehicles (0xC1AA08): vehicles a mission places in the world while
// it's available (a bike, a kart...), with a radar blip and a marker; getting on one
// starts the mission. Made when the player comes within the population's cull
// distance, removed 20 units past it.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "game/Entity.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using col::CVector;

struct MissionVehicle {
    int32_t mission;         // +0 the mission's index (-1: none)
    int32_t id;              // +4 (what Remove finds it by, with the position)
    CVector pos;             // +8
    float heading;           // +0x14
    int32_t model;           // +0x18
    int32_t marker;          // +0x1C a marker's type (0: none)
    uint8_t showBlip;        // +0x20
    uint8_t unknown21;       // +0x21 (for the blip and the marker)
    uint8_t unknown22[2];
    uint8_t unknown24;       // +0x24 (for the marker)
    uint8_t unknown25[3];
    uint8_t unknown28;       // +0x28 (the mission's availability test)
    uint8_t unknown29[3];
    re::Ptr32<void> cashTrack;         // +0x2C played (0x68F580) when the player comes close
    int32_t blip;            // +0x30 (-1: none)
    re::Ptr32<CEntity> vehicle;        // +0x34 (a registered reference)
};
static_assert(offsetof(MissionVehicle, model) == 0x18);
static_assert(offsetof(MissionVehicle, showBlip) == 0x20);
static_assert(offsetof(MissionVehicle, cashTrack) == 0x2C);
static_assert(offsetof(MissionVehicle, vehicle) == 0x34);

// The game's growing pointer array (0x6DC540 removes one)
struct MissionVehicleList {
    re::Ptr32<re::Ptr32<MissionVehicle>> items;  // +0
    uint32_t capacity;       // +4
    uint32_t count;          // +8

    void Process();
    void Remove(int id, const float* pos);
    void Clear();
};
static_assert(sizeof(MissionVehicleList) == 0xC);

GAME_VAR(MissionVehicleList, g_missionVehicles, 0x00C1AA08);
GAME_VAR(uint32_t, g_missionVehiclesInit, 0x00C1AA14);
GAME_VAR(void*, g_radar, 0x00C674CC);
GAME_VAR(uint8_t*, g_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, g_population, 0x00C2C108);  // +0x7834 its cull distance
GAME_VAR(void*, g_hudText, 0x00C67548);
inline void* const g_missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline CVector* const g_zeroVector = reinterpret_cast<CVector*>(0x00BCC564);
inline const char* const s_globalPlayer = reinterpret_cast<const char*>(0x0090AE30);  // "/Global/Player"

GAME_FN(uint8_t*, __thiscall, MissionMgr_GetTaskFlags, 0x006AA7C0, void* mgr, int mission);
GAME_FN(uint8_t, __thiscall, MissionMgr_IsAvailable, 0x006AA9A0, void* mgr, int mission, uint8_t arg);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetMission, 0x006AA660, void* mgr, int mission);
GAME_FN(void, __thiscall, MissionMgr_Start, 0x006AD100, void* mgr, int mission, uint8_t arg);
GAME_FN(void, __thiscall, Radar_RemoveBlip, 0x0054A940, void* radar, int blip);
GAME_FN(void, __thiscall, Radar_SetBlipVisible, 0x00548B60, void* radar, int blip, uint8_t on);
GAME_FN(void, __thiscall, MissionVehicle_AddBlip, 0x0045C210, MissionVehicle* v);
GAME_FN(void, __cdecl, MissionVehicle_RemoveVehicle, 0x0045C290, MissionVehicle* v);
GAME_FN(void*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __thiscall, Seats_Fn4CE960, 0x004CE960, void* seats);
GAME_FN(void, __thiscall, Ped_PlayAction, 0x004740F0, void* ped, const char* node, const char* file);
GAME_FN(uint8_t, __thiscall, Ped_Fn473FD0, 0x00473FD0, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_Fn4CE690, 0x004CE690, void* ped);  // (in a vehicle)
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedIn, 0x004CE6B0, void* ped, CEntity* vehicle);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, CEntity* entity);
GAME_FN(void, __cdecl, Entity_Fn45E690, 0x0045E690, CEntity* entity);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);  // +4 the load state
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(int16_t, __cdecl, CreateVehicle, 0x005D7870, uintptr_t arg, int model, float x, float y, float z, float heading);
GAME_FN(CEntity*, __cdecl, GetVehicleFromHandle, 0x0044A580, int handle);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, CEntity* entity, re::Ptr32<CEntity>* ref);
GAME_FN(void, __cdecl, Markers_Place, 0x0052A600, int type, int a, int b, CVector* pos, float size, CVector* dir, int c, int d, int e,
        int f, CEntity* entity, int h, int i);
GAME_FN(void, __thiscall, HUDText_AddString, 0x0055B070, void* hud, int slot, uintptr_t text);
GAME_FN(void, __cdecl, CashTrack_Play, 0x0068F580, void* track, float volume);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, PtrArray_RemoveAt, 0x006DC540, MissionVehicleList* list, uint32_t index);
inline int __cdecl AtExit(void(__cdecl* fn)()) { return crt::Atexit((void(__cdecl*)())fn); }
inline void(__cdecl* const MissionVehicles_Destroy)() = reinterpret_cast<void(__cdecl*)()>(0x008F2150);

// 0x8F2150: the list's storage freed (the exit function the static initializer registers)
void __cdecl MissionVehicles_DestroyImpl() {
    if (g_missionVehicles.items != nullptr) {
        Free(g_missionVehicles.items);
        g_missionVehicles.items = nullptr;
    }
}

CVector* PositionOf(uint8_t* entity) {
    CPlaceable* p = reinterpret_cast<CPlaceable*>(entity);
    return &p->GetPosition();
}

// (0x45C290 inlined): the driver thrown off, the vehicle out of the world and deleted.
// (Its reference isn't cleared here: deleting it does that)
void DeleteVehicle(CEntity* vehicle) {
    void* seats = reinterpret_cast<uint8_t*>(vehicle) + 0x1C4;
    void* driver = Seats_GetOccupant(seats, 0);
    if (driver) {
        Seats_Fn4CE960(seats);
        Ped_PlayAction(driver, s_globalPlayer, nullptr);
    }
    World_Remove(vehicle);
    Entity_Fn45E690(vehicle);
    if (vehicle) {
        reinterpret_cast<void(__thiscall*)(CEntity*, int)>(static_cast<void*>(vehicle->vtable[0]))(vehicle, 1);
    }
}

// 0x45C3C0 (thiscall): every frame
void MissionVehicleList::Process() {
    void* radar = g_radar;  // (read once)
    for (uint32_t i = 0; i < count; i++) {
        MissionVehicle* v = items[i];
        if (v->blip != -1 && v->mission >= 0 && (MissionMgr_GetTaskFlags(g_missionMgr, v->mission)[4] & 0x40)) {
            Radar_RemoveBlip(radar, v->blip);
            v->blip = -1;
        }
        if (v->showBlip && v->blip == -1 &&
            (v->mission < 0 || !(MissionMgr_GetTaskFlags(g_missionMgr, v->mission)[4] & 0x40))) {
            MissionVehicle_AddBlip(v);
        }
        if (!MissionMgr_IsAvailable(g_missionMgr, v->mission, v->unknown28)) {
            if (v->blip != -1) {
                Radar_SetBlipVisible(radar, v->blip, 0);
            }
            if (v->vehicle) {
                DeleteVehicle(v->vehicle);
            }
            continue;
        }
        float far = float(double(*reinterpret_cast<float*>(g_population + 0x7834)) + 20.0);
        CVector diff;
        CVector* d;
        if (v->vehicle) {
            d = VectorSub(&diff, PositionOf(g_playerPed), &v->vehicle->GetPosition());
        } else {
            d = VectorSub(&diff, PositionOf(g_playerPed), &v->pos);
        }
        float lenSq = float(double(d->x) * d->x + double(d->y) * d->y);
        float dist = float(std::sqrt(double(std::fabs(lenSq))));
        if (v->blip != -1) {
            Radar_SetBlipVisible(radar, v->blip, 1);
        }
        if (!(*reinterpret_cast<float*>(g_population + 0x7834) > dist)) {
            if (far < dist) {
                MissionVehicle_RemoveVehicle(v);
            }
            continue;
        }
        if (!v->vehicle) {
            if (GetStreamingInfo(v->model)[4] == 1) {
                int16_t handle = CreateVehicle(0, uint16_t(v->model), v->pos.x, v->pos.y, v->pos.z, v->heading);
                CEntity* vehicle = GetVehicleFromHandle(handle);
                if (vehicle != v->vehicle) {
                    if (v->vehicle) {
                        CleanUpOldReference(v->vehicle, &v->vehicle);
                    }
                    v->vehicle = vehicle;
                    if (vehicle) {
                        RegisterReference(vehicle, &v->vehicle);
                    }
                }
            } else if (GetStreamingInfo(v->model)[4] != 2) {
                RequestModel(v->model, 0);
            }
            continue;
        }
        if (v->marker) {
            CVector pos = v->vehicle->GetPosition();
            Markers_Place(v->marker, v->unknown24, v->unknown21, &pos, 0.5f, g_zeroVector, v->mission, 0, 0x72, 0xBC, v->vehicle, 1, 5);
        }
        if (Ped_Fn473FD0(g_playerPed)) {
            continue;
        }
        if (Ped_Fn4CE690(g_playerPed) && !Ped_IsSeatedIn(g_playerPed, v->vehicle)) {
            continue;
        }
        if (2.0f > dist && v->cashTrack) {
            uint8_t* mission = MissionMgr_GetMission(g_missionMgr, v->mission);
            int text = *reinterpret_cast<int*>(mission + 0x20);
            if (text) {
                HUDText_AddString(g_hudText, 3, text);
            }
            CashTrack_Play(v->cashTrack, 1.0f);
        }
        if (Ped_IsSeatedIn(g_playerPed, v->vehicle)) {
            MissionMgr_Start(g_missionMgr, v->mission, 0);
        }
    }
}

// 0x45C7B0 (thiscall): the one with this id and position, deleted
void MissionVehicleList::Remove(int id, const float* pos) {
    for (uint32_t i = 0; i < count; i++) {
        MissionVehicle* v = items[i];
        if (v->id == id && pos[0] == v->pos.x && pos[1] == v->pos.y && pos[2] == v->pos.z) {
            MissionVehicle_RemoveVehicle(v);
            if (v->blip != -1) {
                Radar_RemoveBlip(g_radar, v->blip);
            }
            if (v->vehicle) {
                CleanUpOldReference(v->vehicle, &v->vehicle);
            }
            Free(v);
            PtrArray_RemoveAt(this, i);
            return;
        }
    }
}

// 0x45C870 (thiscall): all of them deleted
void MissionVehicleList::Clear() {
    for (uint32_t i = 0; i < count; i++) {
        if (items[i]->vehicle) {
            DeleteVehicle(items[i]->vehicle);
        }
        MissionVehicle* v = items[i];
        if (v->blip != -1) {
            Radar_RemoveBlip(g_radar, v->blip);
        }
        if (v->vehicle) {
            CleanUpOldReference(v->vehicle, &v->vehicle);
        }
        Free(v);
    }
    if (items) {
        Free(items);
        items = nullptr;
    }
    count = 0;
    capacity = 0;
}

// 0x45C950: the list (made empty on first use, destroyed at exit)
MissionVehicleList* __cdecl GetMissionVehicles() {
    if (!(g_missionVehiclesInit & 1)) {
        g_missionVehiclesInit |= 1;
        g_missionVehicles.items = nullptr;
        g_missionVehicles.capacity = 0;
        g_missionVehicles.count = 0;
        AtExit(MissionVehicles_Destroy);
    }
    return &g_missionVehicles;
}

} // namespace

#define MV_C05_REVERSED(name, address, fn) REVERSED(name, address, fn, nullptr, nullptr, re::Mode::Original)

MV_C05_REVERSED("MissionVehicles::Process", 0x0045C3C0, re::MemberAddress(&MissionVehicleList::Process));
MV_C05_REVERSED("MissionVehicles::Remove", 0x0045C7B0, re::MemberAddress(&MissionVehicleList::Remove));
MV_C05_REVERSED("MissionVehicles::Clear", 0x0045C870, re::MemberAddress(&MissionVehicleList::Clear));
MV_C05_REVERSED("MissionVehicles::Get", 0x0045C950, RE_BODY(&GetMissionVehicles));

// 0x45CF30: a lone jmp to 0x45CCC0 (CWeaponInventory's)
RE_UNHOOKED("CWeaponInventory::thunk_45CCC0", 0x0045CF30);
RE_UNHOOKED_FN("MissionVehicles::Destroy (at exit)", 0x008F2150, &MissionVehicles_DestroyImpl);
