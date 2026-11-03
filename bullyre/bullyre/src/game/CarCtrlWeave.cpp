// CCarCtrl's weaving (GTA's WeaveThroughCarsSectorList and friends): the angles to
// steer round other vehicles, peds and one kind of object ahead, narrowed by each one
// found in the box ahead of the vehicle.
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01weave {

struct CVector {
    float x, y, z;
};
using re::At;

GAME_VAR(uint8_t*, PED_player, 0x00C1AEA8);
GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_VAR(uint32_t*, ms_pPtrNodes, 0x00C0F788);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);
GAME_VAR(int32_t, ms_weaveObjectModel, 0x00BF44FC);

GAME_FN(uint8_t*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, GetATanOfXY, 0x00432FE0, float x, float y);
GAME_FN(int32_t*, __thiscall, GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(float*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(CVector*, __thiscall, Entity_GetBoundCentre, 0x00466C70, void* entity, CVector* out);
GAME_FN(void*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(void*, __thiscall, VehicleSeats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __cdecl, WeaveForOtherCar, 0x0041D260, void* other, void* vehicle, float* left, float* right);
GAME_FN(void, __cdecl, WeaveForPed, 0x0041D6D0, void* ped, void* vehicle, float* left, float* right);
GAME_FN(void, __cdecl, WeaveForObject, 0x0041D930, void* object, void* vehicle, float* left, float* right);

const double kMinusPi = -3.1415927410125732;
const double kPi = 3.1415927410125732;
const float kTwoPi = 6.2831854820251465f;

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}
float SqrtAbs(float value) {
    return static_cast<float>(std::sqrt(static_cast<double>(std::fabs(value))));
}
float Sin(float a) {
    return static_cast<float>(std::sin(static_cast<double>(a)));
}
float Cos(float a) {
    return static_cast<float>(std::cos(static_cast<double>(a)));
}
float* ColModel(void* entity) {
    return At<float*>(ms_modelInfoPtrs[At<int16_t>(entity, 0x10E)], 0xC);
}
// an angle difference brought into -pi..pi (a float after each step)
float Limit(float angle) {
    while (angle < kMinusPi) {
        angle = angle + kTwoPi;
    }
    while (angle > kPi) {
        angle = angle - kTwoPi;
    }
    return angle;
}
// the weave: an obstacle at `angle` spanning `span` (its angular width) pushes the
// left and right angles out of it
void Weave(float angle, float span, float* left, float* right) {
    double half = span * 0.5;
    if (std::fabs(Limit(static_cast<float>(angle - *left))) < half) {
        *left = static_cast<float>(angle - half);
        while (*left < kMinusPi) {
            *left = *left + kTwoPi;
        }
    }
    if (!(std::fabs(Limit(static_cast<float>(angle - *right))) < half)) {
        return;
    }
    *right = static_cast<float>(angle + half);
    while (*right > kPi) {
        *right = *right - kTwoPi;
    }
}

// 0x41D260: round another vehicle coming closer (not the one being rammed)
void __cdecl WeaveForOtherCarImpl(uint8_t* other, uint8_t* vehicle, float* left, float* right) {
    if (vehicle[0x2DA] == 3 && other == FindPlayerVehicle(0)) {
        return;
    }
    if (vehicle[0x2DA] == 0xF && other == At<uint8_t*>(vehicle, 0x324)) {
        return;
    }
    CVector diff;
    VectorSub(&diff, PositionOf(other), PositionOf(vehicle));
    float dx = diff.x;
    float dy = diff.y;
    float angle = GetATanOfXY(dx, dy);
    float dist = SqrtAbs(static_cast<float>(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy));
    if (1.0f > dist) {
        return;
    }
    int32_t kind;
    bool bike = *GetVehicleClass(vehicle, &kind) == 7 || *GetVehicleClass(other, &kind) == 7;
    float* vehicleCol = ColModel(vehicle);
    float* otherCol = ColModel(other);
    float dvy = At<float>(vehicle, 0x128) - At<float>(other, 0x128);
    float dvx = At<float>(vehicle, 0x124) - At<float>(other, 0x124);
    float closing = static_cast<float>(static_cast<double>(dvy) * dy + static_cast<double>(dvx) * dx);
    if (closing * 110.0 - otherCol[3] - vehicleCol[3] < dist) {
        return;
    }
    CVector forward;
    Placeable_GetForward(vehicle, &forward);
    float fx = forward.x;
    float fy = forward.y;
    float length = SqrtAbs(static_cast<float>(static_cast<double>(fx) * fx + static_cast<double>(fy) * fy));
    float nx, ny = fy;
    if (length == 0.0f) {
        nx = 1.0f;
    } else {
        nx = fx / length;
        ny = fy / length;
    }
    float angleDiff = static_cast<float>(static_cast<double>(angle) - GetATanOfXY(nx, ny));
    float width = std::fabs(static_cast<float>(Cos(angleDiff) * otherCol[8]));
    float length2 = std::fabs(static_cast<float>(Sin(angleDiff) * otherCol[9]));
    float sides = static_cast<float>((static_cast<double>(length2) + width) * 2.0);
    double factor = bike ? 1.25 : 3.4000000953674316;
    float span = static_cast<float>(vehicleCol[8] * factor + sides);
    Weave(angle, span / dist, left, right);
}

// 0x41D6D0: round a ped (not the player when ramming it)
void __cdecl WeaveForPedImpl(uint8_t* ped, uint8_t* vehicle, float* left, float* right) {
    if (vehicle[0x2DA] == 3 && ped == PED_player) {
        return;
    }
    float dx = PositionOf(ped)->x - PositionOf(vehicle)->x;
    float dy = PositionOf(ped)->y - PositionOf(vehicle)->y;
    float angle = GetATanOfXY(dx, dy);
    float width = static_cast<float>(ColModel(vehicle)[8] * 2.4000000953674316 + 0.800000011920929);
    float dist = SqrtAbs(static_cast<float>(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy));
    Weave(angle, width / dist, left, right);
}

// 0x41D930: round the one object model that's weaved round (its position a little
// ahead of it)
void __cdecl WeaveForObjectImpl(uint8_t* object, uint8_t* vehicle, float* left, float* right) {
    if (At<int16_t>(object, 0x10E) != ms_weaveObjectModel) {
        return;
    }
    CVector pos = *PositionOf(object);
    const float* matrix = Entity_GetMatrix(object);
    float rx = matrix[0];
    float ry = matrix[1];
    CVector forward;
    float x = static_cast<float>(Placeable_GetForward(object, &forward)->x * 0.14699999988079071 + rx * 2.9570000171661377 + pos.x);
    float y = static_cast<float>(Placeable_GetForward(object, &forward)->y * 0.14699999988079071 + ry * 2.9570000171661377 + pos.y);
    float dx = x - PositionOf(vehicle)->x;
    float dy = y - PositionOf(vehicle)->y;
    float angle = GetATanOfXY(dx, dy);
    float width = static_cast<float>(ColModel(vehicle)[8] * 2.4000000953674316 + 0.30000001192092896);
    float dist = SqrtAbs(static_cast<float>(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy));
    Weave(angle, width / dist, left, right);
}

// the checks the sector-list walks share: not seen this scan, shown in this chapter
bool Visible(uint8_t* entity, uint16_t scanCode) {
    return At<uint16_t>(entity, 0x10A) != scanCode && At<int32_t>(entity, 0x20) != 0 &&
           (At<int32_t>(entity, 0x1C) != 0 || At<int32_t>(entity, 0x60) != 0) &&
           (ms_modelInfoPtrs[At<int16_t>(entity, 0x10E)][0xB] & static_cast<uint8_t>(1 << (ms_currentChapter & 0x1F))) &&
           At<int32_t>(entity, 0xF4) == 0;
}
bool InBox(float x, float y, float x1, float y1, float x2, float y2) {
    return x1 < x && x2 > x && y1 < y && y2 > y;
}
uint32_t* Next(uint32_t* node) {
    return (*node >> 18) == 0x3FFF ? nullptr : ms_pPtrNodes + (*node >> 18);
}

// 0x41F400: the vehicles in a sector list (inside the box, within 4 in height)
void __cdecl WeaveThroughCarsSectorList(re::Ptr32<uint32_t>* list, uint8_t* vehicle, void* ignored, float x1, float y1, float x2, float y2,
                                        float* left, float* right) {
    uint32_t* node = *list;
    if (!node) {
        return;
    }
    uint16_t scanCode = ms_nCurrentScanCode;
    float z = PositionOf(vehicle)->z;
    do {
        uint8_t* entity = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
        node = Next(node);
        if (!Visible(entity, scanCode) || entity == ignored) {
            continue;
        }
        At<uint16_t>(entity, 0x10A) = scanCode;
        CVector centre;
        Entity_GetBoundCentre(entity, &centre);
        if (!InBox(centre.x, centre.y, x1, y1, x2, y2)) {
            continue;
        }
        if (std::fabs(static_cast<float>(PositionOf(entity)->z - z)) < 4.0f && entity != vehicle && (entity[0x108] & 7) == 2) {
            WeaveForOtherCar(entity, vehicle, left, right);
        }
    } while (node);
}

// 0x41F5A0: the peds in a sector list (not its driver)
void __cdecl WeaveThroughPedsSectorList(re::Ptr32<uint32_t>* list, uint8_t* vehicle, void* ignored, float x1, float y1, float x2, float y2,
                                        float* left, float* right) {
    uint32_t* node = *list;
    if (!node) {
        return;
    }
    uint16_t scanCode = ms_nCurrentScanCode;
    float z = PositionOf(vehicle)->z;
    do {
        uint8_t* entity = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
        node = Next(node);
        if (!Visible(entity, scanCode) || entity == ignored) {
            continue;
        }
        At<uint16_t>(entity, 0x10A) = scanCode;
        if (entity == VehicleSeats_GetOccupant(vehicle + 0x1C4, 0)) {
            continue;
        }
        CVector pos = *PositionOf(entity);
        if (InBox(pos.x, pos.y, x1, y1, x2, y2) && std::fabs(static_cast<float>(pos.z - z)) < 4.0f) {
            WeaveForPed(entity, vehicle, left, right);
        }
    } while (node);
}

// 0x41F740: the objects in a sector list (within 8 in height, standing up)
void __cdecl WeaveThroughObjectsSectorList(re::Ptr32<uint32_t>* list, uint8_t* vehicle, float x1, float y1, float x2, float y2, float* left,
                                           float* right) {
    uint32_t* node = *list;
    if (!node) {
        return;
    }
    uint16_t scanCode = ms_nCurrentScanCode;
    float z = PositionOf(vehicle)->z;
    do {
        uint8_t* entity = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
        node = Next(node);
        if (!Visible(entity, scanCode)) {
            continue;
        }
        At<uint16_t>(entity, 0x10A) = scanCode;
        CVector pos = *PositionOf(entity);
        if (InBox(pos.x, pos.y, x1, y1, x2, y2) && std::fabs(static_cast<float>(pos.z - z)) < 8.0f &&
            Placeable_GetMatrix(entity)[10] > 0.8999999761581421) {
            WeaveForObject(entity, vehicle, left, right);
        }
    } while (node);
}

} // namespace c01weave

// ---- registration ----------------------------------------------------------------------

#define WEAVE_REVERSED_C01(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01weave {
WEAVE_REVERSED_C01("CCarCtrl::WeaveForOtherCar", 0x0041D260, &WeaveForOtherCarImpl);
WEAVE_REVERSED_C01("CCarCtrl::WeaveForPed", 0x0041D6D0, &WeaveForPedImpl);
WEAVE_REVERSED_C01("CCarCtrl::WeaveForObject", 0x0041D930, &WeaveForObjectImpl);
WEAVE_REVERSED_C01("CCarCtrl::WeaveThroughCarsSectorList", 0x0041F400, &WeaveThroughCarsSectorList);
WEAVE_REVERSED_C01("CCarCtrl::WeaveThroughPedsSectorList", 0x0041F5A0, &WeaveThroughPedsSectorList);
WEAVE_REVERSED_C01("CCarCtrl::WeaveThroughObjectsSectorList", 0x0041F740, &WeaveThroughObjectsSectorList);
} // namespace c01weave
