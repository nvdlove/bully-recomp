// The missions' vehicles (0xC1AA08; see MissionVehicles.cpp): adding one, its radar
// blip, and removing its vehicle (0x45C210..0x45C2F0).

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

struct MissionVehicle {
    int32_t mission;         // +0
    int32_t id;              // +4
    float pos[3];            // +8
    float heading;           // +0x14
    int32_t model;           // +0x18
    int32_t marker;          // +0x1C
    uint8_t showBlip;        // +0x20
    uint8_t unknown21;       // +0x21
    uint8_t unknown22[2];
    int32_t blipSprite;      // +0x24 (-1: no blip)
    uint8_t unknown28;       // +0x28
    uint8_t unknown29[3];
    re::Ptr32<void> cashTrack;         // +0x2C
    int32_t blip;            // +0x30 (-1: none)
    re::Ptr32<uint8_t> vehicle;        // +0x34
};
static_assert(offsetof(MissionVehicle, blipSprite) == 0x24);
static_assert(offsetof(MissionVehicle, vehicle) == 0x34);
static_assert(sizeof(MissionVehicle) == 0x38);

struct MissionVehicleList {
    re::Ptr32<re::Ptr32<MissionVehicle>> items;  // +0
    uint32_t capacity;       // +4
    uint32_t count;          // +8
};
static_assert(sizeof(MissionVehicleList) == 0xC);

GAME_VAR(void*, mv2_radar, 0x00C674CC);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(int32_t, __thiscall, Radar_AddBlip, 0x0054A410, void* radar, int type, float x, float y, float z, uint32_t colour,
        int, int32_t sprite, int);
GAME_FN(void, __thiscall, Radar_SetBlipScale, 0x00548BF0, void* radar, int32_t blip, int);
GAME_FN(void, __thiscall, Radar_SetBlipVisible, 0x00548B60, void* radar, int32_t blip, int);
GAME_FN(void, __thiscall, Radar_SetBlipMission, 0x00548A40, void* radar, int32_t blip, int32_t mission);
GAME_FN(void*, __thiscall, Seats_GetDriver, 0x004CE560, void* seats, int);
GAME_FN(void, __thiscall, Seats_Fn4CE960, 0x004CE960, void* seats);
GAME_FN(void, __thiscall, Ped_SetActionNode, 0x004740F0, void* ped, const char* node, int);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, Fn45E690, 0x0045E690, void* entity);
GAME_FN(void, __thiscall, List_Reserve, 0x006DB480, MissionVehicleList* self, uint32_t capacity, int);
GAME_FN(void, __thiscall, MissionVehicle_AddBlip, 0x0045C210, MissionVehicle* self);

// 0x45C210 (thiscall): its radar blip (red), when it has one to show
void __fastcall MissionVehicle_AddBlipImpl(MissionVehicle* self) {
    void* radar = mv2_radar;
    if (!self->showBlip || self->blipSprite == -1) {
        return;
    }
    int32_t blip = Radar_AddBlip(radar, 4, self->pos[0], self->pos[1], self->pos[2], 0xFFFF0000, 1,
                                 self->blipSprite, self->unknown21);
    Radar_SetBlipScale(radar, blip, 5);
    Radar_SetBlipVisible(radar, blip, 0);
    Radar_SetBlipMission(radar, blip, self->mission);
    self->blip = blip;
}

// 0x45C290 (cdecl): its vehicle out of the world and deleted (a driver sent back to
// "/Global/Player" first)
void __cdecl MissionVehicle_RemoveVehicle(MissionVehicle* self) {
    uint8_t* vehicle = self->vehicle;
    if (!vehicle) {
        return;
    }
    void* seats = vehicle + 0x1C4;
    void* driver = Seats_GetDriver(seats, 0);
    if (driver) {
        Seats_Fn4CE960(seats);
        Ped_SetActionNode(driver, reinterpret_cast<const char*>(0x0090AE30), 0);
    }
    World_Remove(vehicle);
    Fn45E690(vehicle);
    if (vehicle) {
        vc::Call<void>(vehicle, 0, 1);   // the deleting destructor
    }
}

// 0x45C2F0 (thiscall): a new one, its blip, onto the list (doubling it when full).
// (A failed allocation is written through anyway.)
void __fastcall MissionVehicleList_Add(MissionVehicleList* self, re::Edx, int32_t mission, int32_t id, const float* pos,
                                       float heading, int32_t model, int32_t marker, int showBlip, int flag21,
                                       int32_t blipSprite, int flag28, void* cashTrack) {
    MissionVehicle* v = static_cast<MissionVehicle*>(Malloc(0x38));
    if (v) {
        v->vehicle = nullptr;
        v->pos[0] = 0.0f;
        v->pos[1] = 0.0f;
        v->pos[2] = 0.0f;
    } else {
        v = nullptr;
    }
    v->mission = mission;
    v->id = id;
    v->pos[0] = pos[0];
    v->pos[1] = pos[1];
    v->pos[2] = pos[2];
    v->heading = heading;
    v->model = model;
    v->marker = marker;
    v->showBlip = uint8_t(showBlip);
    v->unknown21 = uint8_t(flag21);
    v->blipSprite = blipSprite;
    v->unknown28 = uint8_t(flag28);
    v->cashTrack = cashTrack;
    v->blip = -1;
    MissionVehicle_AddBlip(v);
    if (self->count == self->capacity) {
        List_Reserve(self, self->capacity ? self->capacity * 2 : 1, 0);
    }
    self->items[self->count] = v;
    self->count++;
}

#define C04E_MV(name, address, fn)                                                             \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04E_MV("MissionVehicle::AddBlip", 0x0045C210, &MissionVehicle_AddBlipImpl);
C04E_MV("MissionVehicle::RemoveVehicle", 0x0045C290, &MissionVehicle_RemoveVehicle);
C04E_MV("MissionVehicleList::Add", 0x0045C2F0, &MissionVehicleList_Add);

} // namespace
