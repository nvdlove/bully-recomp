// Helpers of the car control code (GTA's CCarAI / CCarCtrl area): missions reset or
// switched, parked-vehicle counting, points against a box in a matrix's space, and
// small CPlaceable / vector bits.
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01carctrl {

struct CVector {
    float x, y, z;
};
struct CMatrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(CMatrix) == 0x44);
using re::At;

GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint32_t*, ms_listMovingEntityPtrs, 0x00C1AEA4);   // CWorld's moving list (its first node)
GAME_VAR(uint32_t*, ms_pPtrNodes, 0x00C0F788);
GAME_VAR(uint8_t*, ms_pCarCtrlParams, 0x020C8914);         // (+0x1C: how long parked before it counts)
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);

GAME_FN(void*, __thiscall, VehicleSeats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(uint8_t, __thiscall, HasSirenModel, 0x004CB1F0, void* vehicle);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const CMatrix* in, CMatrix* out);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, CMatrix* self, const CMatrix* other);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_TransformPoint(float* out, const CMatrix* m, const CVector* point) { re::CallVec16<0x0041A9A0, void>(out, m, point); }
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_TransformVector(float* out, const CMatrix* m, const CVector* v) { re::CallVec16<0x004146B0, void>(out, m, v); }
GAME_FN(float, __cdecl, RecipSqrt, 0x00414C20, float numerator, float value);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}

// 0x41BB60: the steering outputs zeroed (and the handbrake on); further than 10 from
// (x, y), the mission switched to block the player (5), or 0x11 from 0x12
void __cdecl StopAndSwitchWhenFar(uint8_t* vehicle, float x, float y, float, float, float* steer, float* accel, float* brake,
                                  uint8_t* handbrake) {
    *steer = 0.0f;
    *accel = 0.0f;
    *brake = 1.0f;
    *handbrake = 1;
    CVector pos = *PositionOf(vehicle);
    double dx = static_cast<double>(pos.x) - x;
    double dy = static_cast<double>(pos.y) - y;
    float distSq = static_cast<float>(dy * dy + dx * dx);
    if (distSq > 100.0) {
        At<uint8_t>(vehicle, 0x2DA) = At<uint8_t>(vehicle, 0x2DA) != 0x12 ? 5 : 0x11;
    }
}

// 0x41BED0: back to cruising (1) with a driver who's on it (+0x1570), none without
void __cdecl ResetMission(uint8_t* vehicle) {
    void* seats = vehicle + 0x1C4;
    if (VehicleSeats_GetOccupant(seats, 0)) {
        uint8_t* driver = static_cast<uint8_t*>(VehicleSeats_GetOccupant(seats, 0));
        At<uint8_t>(vehicle, 0x2DA) = driver[0x1570] == 1;
    } else {
        At<uint8_t>(vehicle, 0x2DA) = 0;
    }
    At<uint8_t>(vehicle, 0x2DB) = 0;
    At<uint32_t>(vehicle, 0x2CC) = m_snTimeInMilliseconds;
    At<uint32_t>(vehicle, 0x2D0) = m_snTimeInMilliseconds;
}

// 0x41C320: the moving vehicles parked long enough (kind 0, no siren going)
int __cdecl CountParkedVehicles() {
    int count = 0;
    uint32_t* node = ms_listMovingEntityPtrs;
    if (!node) {
        return 0;
    }
    do {
        uint8_t* entity = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
        if ((entity[0x108] & 7) == 2 && At<int32_t>(entity, 0x46C) == 0 && (!HasSirenModel(entity) || entity[0x36D] == 0) &&
            At<uint32_t>(entity, 0x2CC) + At<uint32_t>(ms_pCarCtrlParams, 0x1C) <= m_snTimeInMilliseconds) {
            count++;
        }
        if ((*node >> 18) == 0x3FFF) {
            break;
        }
        node = ms_pPtrNodes + (*node >> 18);
    } while (node);
    return count;
}

// 0x41C3C0: `point` inside the flat box min..max in the matrix's space; where it is
// across (from the box's middle) and along (from min) as asked
uint8_t __cdecl IsPointInBox(const CVector* point, const float* min, const float* max, const CMatrix* matrix, float* across,
                             float* along) {
    CMatrix inverse;
    CMatrix local;
    local.attached = nullptr;
    Matrix_Invert(matrix, &inverse);
    Matrix_CopyConstruct(&local, &inverse);
    float p[4];
    Matrix_TransformPoint(p, &local, point);
    if (across) {
        *across = static_cast<float>(p[0] - (static_cast<double>(min[0]) + max[0]) * 0.5);
    }
    if (along) {
        *along = p[1] - min[1];
    }
    if (!(min[0] < p[0]) || !(max[0] > p[0]) || !(min[1] < p[1]) || !(max[1] > p[1])) {
        return 0;
    }
    return 1;
}

// 0x41C490: a direction through a matrix (a copy of it first)
CVector* __cdecl TransformDirection(CVector* out, const CMatrix* matrix, const CVector* v) {
    CVector copy = *v;
    float result[4];
    Matrix_TransformVector(result, matrix, &copy);
    out->x = result[0];
    out->y = result[1];
    out->z = result[2];
    return out;
}

// 0x41C4E0 (thiscall): CPlaceable::SetPosition (the matrix's too; the game also
// writes uninitialised stack into the matrix's +0x3C, left here)
void __fastcall SetPosition(uint8_t* self, re::Edx, float x, float y, float z) {
    if (uint8_t* matrix = At<uint8_t*>(self, 0x14)) {
        At<float>(matrix, 0x30) = x;
        At<float>(matrix, 0x34) = y;
        At<float>(matrix, 0x38) = z;
    }
    At<float>(self, 4) = x;
    At<float>(self, 8) = y;
    At<float>(self, 0xC) = z;
}

// 0x41C540 (thiscall): shown in the current chapter (+0x20 set, +0x1C or +0x60, the
// model's chapter bits, +0xF4 clear)
uint32_t __fastcall IsInCurrentChapter(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x20) != 0 && (At<int32_t>(self, 0x1C) != 0 || At<int32_t>(self, 0x60) != 0) &&
        (ms_modelInfoPtrs[At<int16_t>(self, 0x10E)][0xB] & static_cast<uint8_t>(1 << (ms_currentChapter & 0x1F))) &&
        At<int32_t>(self, 0xF4) == 0) {
        return 1;
    }
    return 0;
}

// 0x41C590: out = v flattened and normalised
CVector* __cdecl FlatNormalised(CVector* out, const CVector* v) {
    float scale = RecipSqrt(1.0f, static_cast<float>(static_cast<double>(v->x) * v->x + static_cast<double>(v->y) * v->y));
    out->x = scale * v->x;
    out->y = scale * v->y;
    out->z = 0.0f;
    return out;
}

GAME_FN(void, __thiscall, Placeable_SetPosition, 0x0041C4E0, void* self, float x, float y, float z);

// 0x41EE20 (thiscall): CPlaceable::SetPosition from a vector
void __fastcall SetPositionVector(uint8_t* self, re::Edx, const CVector* pos) {
    Placeable_SetPosition(self, pos->x, pos->y, pos->z);
}

GAME_FN(CVector*, __cdecl, FindPlayerCentreOfWorld, 0x004469E0, uintptr_t player);
GAME_FN(int, __cdecl, GetObjectAlpha_5F2B10, 0x005F2B10, void* rwObject);
GAME_FN(uint8_t, __thiscall, Vehicle_CanBeRemoved, 0x004CB7F0, void* vehicle);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, void* ped);
GAME_FN(uint8_t, __thiscall, PedTypeMgr_49EA30, 0x0049EA30, void* self, void* ped);
GAME_FN(int, __cdecl, CountParkedVehicles_, 0x0041C320);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, void* entity);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_VAR(void*, pedTypeMgr, 0x00C2C108);

// 0x41EC70 (the argument popped; ecx unused): GTA's PossiblyRemoveVehicle: deleted
// when not visible (or not drawn at all) and further from the player than 95 (21.7
// for an empty parked one of the player's kind, 32.5 when too many are parked)
void __fastcall PossiblyRemoveVehicle(void*, re::Edx, uint8_t* vehicle) {
    CVector player = *FindPlayerCentreOfWorld(0);
    auto destroy = [vehicle]() { reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>(At<re::Ptr32<void>*>(vehicle, 0)[0]))(vehicle, 1); };
    if (GetObjectAlpha_5F2B10(At<void*>(vehicle, 0x18)) == 0) {
        destroy();
        return;
    }
    if (!Vehicle_CanBeRemoved(vehicle)) {
        return;
    }
    uint8_t* driver = At<uint8_t*>(vehicle, 0x358);
    if (driver && !Ped_IsSeatedInVehicle(driver) && PedTypeMgr_49EA30(pedTypeMgr, driver)) {
        return;
    }
    float limit = 95.0f;
    bool parkedCheck;
    if (At<int32_t>(vehicle, 0x46C) == 1) {
        if (!driver) {
            if (vehicle[0x290] != 3) {
                limit = 21.666667938232422f;
            }
            parkedCheck = false;
        } else {
            parkedCheck = true;
        }
    } else {
        parkedCheck = driver != nullptr;
    }
    if (parkedCheck) {
        uint32_t maxParked = At<uint32_t>(ms_pCarCtrlParams, 0x18);
        if (static_cast<uint32_t>(CountParkedVehicles_()) >= maxParked && At<int32_t>(vehicle, 0x34) != 0 &&
            !Entity_GetIsOnScreen(vehicle)) {
            limit = 32.5f;
        }
    }
    CVector diff;
    const CVector* d = VectorSub(&diff, &player, PositionOf(vehicle));
    float distSq = static_cast<float>(static_cast<double>(d->x) * d->x + static_cast<double>(d->y) * d->y);
    float dist = static_cast<float>(std::sqrt(static_cast<double>(std::fabs(distSq))));
    if (limit < dist) {
        if (Entity_GetIsOnScreen(vehicle)) {
            vehicle[0x33A] |= 0x20;
            return;
        }
        destroy();
        return;
    }
    if (vehicle[0x33A] & 0x20) {
        vehicle[0x33A] &= 0xDF;
    }
}

} // namespace c01carctrl

// ---- registration ----------------------------------------------------------------------

#define CARCTRL_REVERSED_C01(name, address, fn)                                              \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01carctrl {
CARCTRL_REVERSED_C01("CCarAI::StopAndSwitchWhenFar", 0x0041BB60, &StopAndSwitchWhenFar);
CARCTRL_REVERSED_C01("CCarAI::ResetMission", 0x0041BED0, &ResetMission);
CARCTRL_REVERSED_C01("CCarCtrl::CountParkedVehicles", 0x0041C320, &CountParkedVehicles);
CARCTRL_REVERSED_C01("CCarCtrl::IsPointInBox", 0x0041C3C0, &IsPointInBox);
CARCTRL_REVERSED_C01("CMatrix::TransformDirection", 0x0041C490, &TransformDirection);
CARCTRL_REVERSED_C01("CPlaceable::SetPosition", 0x0041C4E0, &SetPosition);
CARCTRL_REVERSED_C01("CEntity::IsInCurrentChapter", 0x0041C540, &IsInCurrentChapter);
CARCTRL_REVERSED_C01("CVector::FlatNormalised", 0x0041C590, &FlatNormalised);
CARCTRL_REVERSED_C01("CPlaceable::SetPosition (vector)", 0x0041EE20, &SetPositionVector);
CARCTRL_REVERSED_C01("CCarCtrl::PossiblyRemoveVehicle", 0x0041EC70, &PossiblyRemoveVehicle);
} // namespace c01carctrl
