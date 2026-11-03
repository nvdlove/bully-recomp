// More of CCarAI: vehicles told to go to coordinates or to chase / block an entity.
// (UpdateCarAI is in CarAI.cpp; the AutoPilot fields are listed there.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01carai2 {

struct CVector {
    float x, y, z;
};
using re::At;

GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);

GAME_FN(uint8_t, __cdecl, JoinCarWithRoadSystemGotoCoors_41E6C0, 0x0041E6C0, void* vehicle, CVector target, int);
GAME_FN(void, __cdecl, Fn_41E370, 0x0041E370, void* vehicle, uintptr_t);
GAME_FN(void, __thiscall, RegisteredPointerAssign, 0x005D7F10, void* ref, void* entity);
GAME_FN(float, __cdecl, GetCarToGoToCoors_, 0x0041A170, void* vehicle, const CVector* target);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}
// the flat distance from the vehicle to `target`
float FlatDistance(void* vehicle, const CVector* target) {
    const CVector* v = PositionOf(vehicle);
    double dy = static_cast<double>(v->y) - target->y;
    double dx = static_cast<double>(v->x) - target->x;
    float sum = static_cast<float>(dy * dy + dx * dx);
    return static_cast<float>(std::sqrt(static_cast<double>(std::fabs(sum))));
}
void SetStatus(void* vehicle, int status) {
    reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>(At<re::Ptr32<void>*>(vehicle, 0)[0x18 / 4]))(vehicle, status);
}

// 0x41A150: the stuck timer restarted
void __cdecl ResetStuckTimer(uint8_t* vehicle) {
    At<uint32_t>(vehicle, 0x2CC) = m_snTimeInMilliseconds;
}

// 0x41A170: GetCarToGoToCoors: a new GOTOCOORDS mission (or, already on one, the
// target moved when more than 2 units off); the flat distance to it
float __cdecl GetCarToGoToCoors(uint8_t* vehicle, const CVector* target) {
    uint8_t mission = At<uint8_t>(vehicle, 0x2DA);
    if (mission == 8 || mission == 9) {
        float dx = std::fabs(static_cast<float>(target->x - At<float>(vehicle, 0x2F0)));
        bool far = 2.0f < dx;
        if (!far) {
            float dy = std::fabs(static_cast<float>(target->y - At<float>(vehicle, 0x2F4)));
            far = 2.0f < dy;
        }
        if (far) {
            At<CVector>(vehicle, 0x2F0) = *target;
        }
    } else {
        At<uint8_t>(vehicle, 0x2D9) = 2;
        At<uint8_t>(vehicle, 0x2DB) = 0;
        At<uint8_t>(vehicle, 0x2E4) = 20;
        At<uint32_t>(vehicle, 0x2CC) = m_snTimeInMilliseconds;
        SetStatus(vehicle, 3);
        At<uint8_t>(vehicle, 0x2DA) = static_cast<uint8_t>(8 + (JoinCarWithRoadSystemGotoCoors_41E6C0(vehicle, *target, 0) ? 1 : 0));
    }
    return FlatDistance(vehicle, target);
}

// 0x41A2C0: chase an entity (radius 0: ram the player instead)
void __cdecl ChaseEntity(uint8_t* vehicle, void* entity, float radius, float speed) {
    SetStatus(vehicle, 3);
    At<uint8_t>(vehicle, 0x2DA) = radius == 0.0f ? 3 : 0x1A;
    At<uint8_t>(vehicle, 0x2D9) = 2;
    At<uint8_t>(vehicle, 0x2DB) = 0;
    At<uint8_t>(vehicle, 0x2E4) = static_cast<uint8_t>(static_cast<int>(speed));
    At<float>(vehicle, 0x320) = radius;
    At<uint32_t>(vehicle, 0x2CC) = m_snTimeInMilliseconds;
    RegisteredPointerAssign(vehicle + 0x328, entity);
    Fn_41E370(vehicle, 0);
}

// 0x41A340: follow an entity (mission 0x1B)
void __cdecl FollowEntity(uint8_t* vehicle, void* entity, float radius, float speed) {
    SetStatus(vehicle, 3);
    At<uint8_t>(vehicle, 0x2DA) = 0x1B;
    At<uint8_t>(vehicle, 0x2D9) = 2;
    At<uint8_t>(vehicle, 0x2DB) = 0;
    At<uint8_t>(vehicle, 0x2E4) = static_cast<uint8_t>(static_cast<int>(speed));
    At<float>(vehicle, 0x320) = radius;
    At<uint32_t>(vehicle, 0x2CC) = m_snTimeInMilliseconds;
    RegisteredPointerAssign(vehicle + 0x328, entity);
    Fn_41E370(vehicle, 0);
}

// 0x41A3B0: GetCarToGoToCoors, slowly (cruise 10) and joining the roads after
float __cdecl GetCarToGoToCoorsSlowly(uint8_t* vehicle, const CVector* target) {
    GetCarToGoToCoors_(vehicle, target);
    At<uint8_t>(vehicle, 0x33B) |= 0x80;
    At<uint8_t>(vehicle, 0x2E4) = 10;
    return FlatDistance(vehicle, target);
}

} // namespace c01carai2

// ---- registration ----------------------------------------------------------------------

#define CARAI2_REVERSED_C01(name, address, fn)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01carai2 {
CARAI2_REVERSED_C01("CCarAI::ResetStuckTimer", 0x0041A150, &ResetStuckTimer);
CARAI2_REVERSED_C01("CCarAI::GetCarToGoToCoors", 0x0041A170, &GetCarToGoToCoors);
CARAI2_REVERSED_C01("CCarAI::ChaseEntity", 0x0041A2C0, &ChaseEntity);
CARAI2_REVERSED_C01("CCarAI::FollowEntity", 0x0041A340, &FollowEntity);
CARAI2_REVERSED_C01("CCarAI::GetCarToGoToCoorsSlowly", 0x0041A3B0, &GetCarToGoToCoorsSlowly);
} // namespace c01carai2
