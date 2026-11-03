// CPlayerInfo (CWorld::Players at 0xC1AEA8; +0 the player ped), continued: its +0xA6
// state (0..3, stepping down over time; +0xA7 the previous), its small tables (+0x110,
// +0x1AC bits, +0x1B0, +0x1EC / +0x1EE words, +0x2B4 flags), and the FindPlayer helpers:
// the player's vehicle (+0x1554) or ped, its position and heading.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"
#include "runflags.h"

namespace {

using re::At;

struct Vec3 {
    float x, y, z;
};

// A placeable's position (its matrix's, else its placement's)
Vec3* PositionOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<Vec3*>(matrix + 0x30) : reinterpret_cast<Vec3*>(entity + 4);
}
// Its heading: from its matrix's forward (the CRT's x87 atan2, 0x85CF1A), else its placement's
float HeadingOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    if (matrix) {
        return float(std::atan2(double(-At<float>(matrix, 0x10)), double(At<float>(matrix, 0x14))));
    }
    return At<float>(entity, 0x10);
}

constexpr int32_t PED_TYPE_PLAYER = 0xD;

GAME_VAR(uint8_t*, PI2_playerPed, 0x00C1AEA8);  // Players[0].ped
GAME_VAR(uint8_t, PI2_cutsceneRunning, 0x020C5BE4);
GAME_VAR(float, PI2_timeStep, 0x00C1A9A4);
GAME_VAR(float, PI2_moveScale, 0x00A14550);
GAME_VAR(float, PI2_vehicleSpeed, 0x00C1AF88);
inline void* const PI2_camera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(int, __thiscall, PI2_Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(int, __cdecl, PI2_GetTimeStepInMs, 0x0045B830);
GAME_FN(uint8_t, __cdecl, PI2_Fn425E40, 0x00425E40, int, int, int, int);
GAME_FN(float, __cdecl, PI2_LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(void, __thiscall, PI2_SetState, 0x004464A0, uint8_t* self, int state);
GAME_FN(void, __thiscall, PI2_SetTiredness, 0x00445E30, uint8_t* self, int level);
GAME_FN(uint8_t, __thiscall, PI2_Fn5D80E0, 0x005D80E0, void* object);
GAME_FN(void*, __thiscall, PI2_Fn5D9C20, 0x005D9C20, void* object);
GAME_FN(int, __thiscall, PI2_Fn5D80D0, 0x005D80D0, void* object);
GAME_FN(void, __cdecl, PI2_Fn5D8270, 0x005D8270, void* a);
GAME_FN(void, __cdecl, PI2_Fn5D8670, 0x005D8670, void* a, int b);
GAME_FN(void, __cdecl, PI2_Fn5D85C0, 0x005D85C0, void* a, int index);
GAME_FN(void, __cdecl, PI2_Fn5D82A0, 0x005D82A0, void* a, int b, int c);

// 0x4464A0 (thiscall): the +0xA6 state (clamped to 0..3), the old one into +0xA7
void __fastcall SetState(uint8_t* self, re::Edx, int state) {
    float f = float(int(int8_t(state)));
    if (f < 0.0f) {
        f = 0.0f;
    } else if (f > 3.0f) {
        f = 3.0f;
    }
    self[0xA7] = self[0xA6];
    self[0xA6] = uint8_t(int(f));  // (0x85C720: _ftol)
}

// 0x446500 (thiscall): the state stepped down while it's settled (+0xA7 == +0xA6; the
// timer +0xC0 restarts on a change): levels 1 and 2 after 30 s each; level 3 after 10 s
// sets the ped's +0x1460 (cleared on entering it) and drops to 0. Not in cutscenes or
// camera fades
void __fastcall ProcessState(uint8_t* self, re::Edx) {
    if (self[0xA7] == 0 || At<void*>(self, 0) == nullptr || PI2_cutsceneRunning != 0 ||
        PI2_Camera_GetFadeState(PI2_camera) != 0) {
        return;
    }
    int8_t state = int8_t(self[0xA6]);
    switch (state) {
    case 0:
        if (self[0xA7] != 0) {
            self[0xA7] = 0;
            At<int32_t>(self, 0xC0) = 0;
            PI2_SetState(self, 0);
        }
        return;
    case 1:
    case 2:
        if (self[0xA7] != uint8_t(state)) {
            At<int32_t>(self, 0xC0) = 0;
            self[0xA7] = uint8_t(state);
            return;
        }
        At<int32_t>(self, 0xC0) += PI2_GetTimeStepInMs();
        if (float(double(At<int32_t>(self, 0xC0)) * double(3.33333337e-05f)) > 1.0f) {
            PI2_SetState(self, int8_t(self[0xA6] - 1));
        }
        return;
    case 3:
        if (self[0xA7] != 3) {
            At<int32_t>(self, 0xC0) = 0;
            self[0xA7] = 3;
            At<int32_t>(PI2_playerPed, 0x1460) = 0;
            return;
        }
        At<int32_t>(self, 0xC0) += PI2_GetTimeStepInMs();
        if (float(double(At<int32_t>(self, 0xC0)) * double(1e-4f)) > 1.0f) {
            At<int32_t>(PI2_playerPed, 0x1460) = 1;
            PI2_SetState(self, 0);
        }
        return;
    default:
        return;
    }
}

// 0x4466D0 (thiscall): +0x110[index] (index up to 28)
void __fastcall SetInt110(uint8_t* self, re::Edx, uint32_t index, int32_t value) {
    if (index < 0x1D) {
        At<int32_t>(self, 0x110 + index * 4) = value;
    }
}

// 0x4466F0 (thiscall): +0x110[index], -1 out of range
int32_t __fastcall GetInt110(uint8_t* self, re::Edx, uint32_t index) {
    if (index < 0x1D) {
        return At<int32_t>(self, 0x110 + index * 4);
    }
    return -1;
}

// 0x446710 (thiscall): bit `bit` of the +0x1AC byte
uint8_t __fastcall GetBit1AC(uint8_t* self, re::Edx, int bit) {
    return (self[0x1AC] & uint8_t(1u << (bit & 31))) != 0;
}

// 0x446730 (thiscall): the same set or cleared (8-bit shift: bits 8+ are none)
void __fastcall SetBit1AC(uint8_t* self, re::Edx, int bit, int on) {
    uint8_t mask = uint8_t(1u << (bit & 31));
    if (re::Flag(on)) {
        self[0x1AC] |= mask;
        return;
    }
    self[0x1AC] &= uint8_t(~mask);
}

// 0x446760 / 0x446780 (thiscall): +0x1B0[index]
void __fastcall SetInt1B0(uint8_t* self, re::Edx, int index, int32_t value) {
    At<int32_t>(self, 0x1B0 + index * 4) = value;
}
int32_t __fastcall GetInt1B0(uint8_t* self, re::Edx, int index) {
    return At<int32_t>(self, 0x1B0 + index * 4);
}

// 0x446790 / 0x4467B0 / 0x4467C0 / 0x4467E0 (thiscall): the two words of +0x1EC[index]
// (4 bytes each)
void __fastcall SetWord1EC(uint8_t* self, re::Edx, int index, int value) {
    At<uint16_t>(self, 0x1EC + index * 4) = uint16_t(value);
}
uint32_t __fastcall GetWord1EC(uint8_t* self, re::Edx, int index) {
    return At<uint16_t>(self, 0x1EC + index * 4);
}
void __fastcall SetWord1EE(uint8_t* self, re::Edx, int index, int value) {
    At<uint16_t>(self, 0x1EE + index * 4) = uint16_t(value);
}
uint32_t __fastcall GetWord1EE(uint8_t* self, re::Edx, int index) {
    return At<uint16_t>(self, 0x1EE + index * 4);
}

// 0x4467F0 (thiscall): the +0x2B4 flag `index`; set while its +0x2CC object is ready,
// that object's current entry started for it (0x5D8xxx)
void __fastcall SetFlag2B4(uint8_t* self, re::Edx, int index, int on) {
    self[0x2B4 + index] = uint8_t(on);
    if (!re::Flag(on)) {
        return;
    }
    void* object = self + 0x2CC;
    if (!PI2_Fn5D80E0(object)) {
        return;
    }
    void* entry = PI2_Fn5D9C20(object);
    PI2_Fn5D8270(entry);
    PI2_Fn5D8670(entry, PI2_Fn5D80D0(object));
    PI2_Fn5D85C0(entry, index);
    PI2_Fn5D82A0(entry, 1, 0);
}

// 0x446860 (thiscall): the +0x2B4 flag `index`
uint8_t __fastcall GetFlag2B4(uint8_t* self, re::Edx, int index) {
    return self[0x2B4 + index];
}

// 0x446870: the time step scaled by 0xA14550, in whole milliseconds (truncated; the
// products rounded as the x87 does in single precision)
int32_t __cdecl GetScaledTimeStepMs() {
    return int32_t(int64_t(float(double(PI2_moveScale * PI2_timeStep) * 1000.0)));
}

// 0x4468B0 (thiscall): the position of its ped's vehicle, or of its ped
Vec3* __fastcall GetPlayerCoors(uint8_t* self, re::Edx, Vec3* out) {
    uint8_t* ped = At<uint8_t*>(self, 0);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    *out = *PositionOf(vehicle ? vehicle : ped);
    return out;
}

// 0x446900: FindPlayerCoors: the player's vehicle's position, or the player's
Vec3* __cdecl FindPlayerCoors(Vec3* out) {
    uint8_t* ped = PI2_playerPed;
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    *out = *PositionOf(vehicle ? vehicle : ped);
    return out;
}

// 0x446970: FindPlayerVehicle: the vehicle of the ped (the player when null) if it's a
// player ped
uint8_t* __cdecl FindPlayerVehicle(uint8_t* ped) {
    if (!ped) {
        ped = PI2_playerPed;
        if (!ped) {
            return nullptr;
        }
    }
    if (At<int32_t>(ped, 0x1310) != PED_TYPE_PLAYER) {
        return nullptr;
    }
    return At<uint8_t*>(ped, 0x1554);
}

// 0x4469A0: the player in a vehicle moving (0xC1AF88 over 0.01)
uint8_t __cdecl IsPlayerInMovingVehicle() {
    uint8_t* ped = PI2_playerPed;
    if (ped && At<int32_t>(ped, 0x1310) == PED_TYPE_PLAYER && At<uint8_t*>(ped, 0x1554)) {
        return 0.01f < PI2_vehicleSpeed ? 1 : 0;
    }
    return 0;
}

// 0x4469E0: FindPlayerCentreOfWorld's position: the ped's (the player when null) vehicle's
// when it's a player ped in one, else the ped's
Vec3* __cdecl FindPlayerCentreOfWorld(uint8_t* ped) {
    if (const float* centre = ped ? nullptr : re::PopulationCentre()) {
        // (the remaster's second population pass, runflags.h: the place the player is mapped to
        // through a seamless door, in the area behind it)
        static Vec3 there;
        std::memcpy(&there, centre, sizeof(there));
        return &there;
    }
    uint8_t* entity = ped ? ped : static_cast<uint8_t*>(PI2_playerPed);
    if (At<int32_t>(entity, 0x1310) == PED_TYPE_PLAYER && At<uint8_t*>(entity, 0x1554)) {
        entity = At<uint8_t*>(entity, 0x1554);
    }
    return PositionOf(entity);
}

// 0x446A40: the player's vehicle's position, or the player's
Vec3* __cdecl FindPlayerPosition() {
    uint8_t* ped = PI2_playerPed;
    if (ped && At<int32_t>(ped, 0x1310) == PED_TYPE_PLAYER && At<uint8_t*>(ped, 0x1554)) {
        return PositionOf(At<uint8_t*>(ped, 0x1554));
    }
    return PositionOf(ped);
}

// 0x446A80: FindPlayerHeading: its vehicle's, or the ped's (turned round while +0x1488
// with +0x1474 or +0x1470)
float __cdecl FindPlayerHeading() {
    uint8_t* ped = PI2_playerPed;
    if (ped && At<int32_t>(ped, 0x1310) == PED_TYPE_PLAYER && At<uint8_t*>(ped, 0x1554)) {
        return HeadingOf(At<uint8_t*>(ped, 0x1554));
    }
    float heading = HeadingOf(ped);
    if ((At<int32_t>(ped, 0x1474) != 0 || At<int32_t>(ped, 0x1470) != 0) && At<int32_t>(ped, 0x1488) != 0) {
        heading = PI2_LimitRadianAngle(float(heading + double(3.14159274f)));
    }
    return heading;
}

// 0x446B40 (thiscall): up one state unless 0x425E40(2, 7, 0, 0) (+0xC4 bit 2 cleared,
// +0xB8 0, +0xBC -1); the tiredness (+0xA4) down one if not done already (+0xC4 bit 0).
// 1 when that happened
uint8_t __fastcall RaiseState(uint8_t* self, re::Edx) {
    if (PI2_Fn425E40(2, 7, 0, 0)) {
        return 0;
    }
    self[0xA6] = uint8_t(self[0xA6] + 1);
    self[0xC4] &= 0xFB;
    At<int32_t>(self, 0xB8) = 0;
    At<int32_t>(self, 0xBC) = -1;
    PI2_SetState(self, self[0xA6]);
    if (self[0xA4] == 0 || (self[0xC4] & 1)) {
        return 0;
    }
    self[0xA4] = uint8_t(self[0xA4] - 1);
    PI2_SetTiredness(self, self[0xA4]);
    self[0xC4] |= 1;
    return 1;
}

} // namespace

#define PLAYERINFO2_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PLAYERINFO2_REVERSED("CPlayerInfo::SetState", 0x004464A0, &SetState);
PLAYERINFO2_REVERSED("CPlayerInfo::ProcessState", 0x00446500, &ProcessState);
PLAYERINFO2_REVERSED("CPlayerInfo::SetInt110", 0x004466D0, &SetInt110);
PLAYERINFO2_REVERSED("CPlayerInfo::GetInt110", 0x004466F0, &GetInt110);
PLAYERINFO2_REVERSED("CPlayerInfo::GetBit1AC", 0x00446710, &GetBit1AC);
PLAYERINFO2_REVERSED("CPlayerInfo::SetBit1AC", 0x00446730, &SetBit1AC);
PLAYERINFO2_REVERSED("CPlayerInfo::SetInt1B0", 0x00446760, &SetInt1B0);
PLAYERINFO2_REVERSED("CPlayerInfo::GetInt1B0", 0x00446780, &GetInt1B0);
PLAYERINFO2_REVERSED("CPlayerInfo::SetWord1EC", 0x00446790, &SetWord1EC);
PLAYERINFO2_REVERSED("CPlayerInfo::GetWord1EC", 0x004467B0, &GetWord1EC);
PLAYERINFO2_REVERSED("CPlayerInfo::SetWord1EE", 0x004467C0, &SetWord1EE);
PLAYERINFO2_REVERSED("CPlayerInfo::GetWord1EE", 0x004467E0, &GetWord1EE);
PLAYERINFO2_REVERSED("CPlayerInfo::SetFlag2B4", 0x004467F0, &SetFlag2B4);
PLAYERINFO2_REVERSED("CPlayerInfo::GetFlag2B4", 0x00446860, &GetFlag2B4);
PLAYERINFO2_REVERSED("GetScaledTimeStepMs", 0x00446870, &GetScaledTimeStepMs);
PLAYERINFO2_REVERSED("CPlayerInfo::GetPlayerCoors", 0x004468B0, &GetPlayerCoors);
PLAYERINFO2_REVERSED("FindPlayerCoors", 0x00446900, &FindPlayerCoors);
PLAYERINFO2_REVERSED("FindPlayerVehicle", 0x00446970, &FindPlayerVehicle);
PLAYERINFO2_REVERSED("IsPlayerInMovingVehicle", 0x004469A0, &IsPlayerInMovingVehicle);
PLAYERINFO2_REVERSED("FindPlayerCentreOfWorld", 0x004469E0, &FindPlayerCentreOfWorld);
PLAYERINFO2_REVERSED("FindPlayerPosition", 0x00446A40, &FindPlayerPosition);
PLAYERINFO2_REVERSED("FindPlayerHeading", 0x00446A80, &FindPlayerHeading);
PLAYERINFO2_REVERSED("CPlayerInfo::RaiseState", 0x00446B40, &RaiseState);
