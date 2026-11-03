// CCarCtrl slowing vehicles down for what's in front of them (GTA's SlowCarDownFor...
// SectorList): a box ahead of the vehicle (in its own space, stretched by its speed)
// and what's found in it: peds get honked at and step aside, the speed is capped.
// (x87 extended precision in the game; what it stores as floats is kept as floats.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01slow {

struct CVector {
    float x, y, z;
};
struct CMatrix {
    float m[16];
    re::Ptr32<void> attached;
};
static_assert(sizeof(CMatrix) == 0x44);
using re::At;

GAME_VAR(uint16_t, ms_nCurrentScanCode, 0x00C1AE88);
GAME_VAR(uint32_t*, ms_pPtrNodes, 0x00C0F788);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t, ms_currentChapter, 0x00A147E0);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(float, ms_fTimeStep, 0x00C1A9A4);
GAME_VAR(float, ms_fSpeedScale_9E4F90, 0x009E4F90);
GAME_VAR(uint8_t*, ms_pCarCtrlParams, 0x020C8914);   // (+0x20 look-ahead time, +0x24 the speed peds react to)
GAME_VAR(int32_t, ms_ignoredPedModel, 0x00BF42DC);
GAME_VAR(void*, ms_pEventMgr_C2C1A0, 0x00C2C1A0);
GAME_ARRAY(uint32_t*, ms_aSectorPedLists, 0x00C1B184);   // CWorld::ms_aSectors[36 * 36] (0x14 each) .lists[3]

GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, CMatrix* self, const CMatrix* other);
GAME_FN(void, __cdecl, Matrix_Invert, 0x00413770, const CMatrix* in, CMatrix* out);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(CVector*, __thiscall, Placeable_GetPosition, 0x00415460, void* placeable);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_TransformVector(float* out, const CMatrix* m, const CVector* v) { re::CallVec16<0x004146B0, void>(out, m, v); }
GAME_FN(void*, __cdecl, GetEntityInPool, 0x0044A3E0, uint32_t pool, int slot);
GAME_FN(uint8_t, __cdecl, IsPointInBox, 0x0041C3C0, const CVector* point, const float* min, const float* max, const CMatrix* matrix,
        float* across, float* along);
GAME_FN(void*, __thiscall, VehicleSeats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __thiscall, Ped_492590, 0x00492590, void* self);
GAME_FN(uint8_t, __thiscall, Ped_IsStateIdle, 0x0041AA50, void* ped);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(void, __thiscall, EventMgr_4A6670, 0x004A6670, void* self, int event, void* ped, void* driver, uintptr_t, float, float);
GAME_FN(void, __thiscall, Ped_SetPunishmentTime, 0x004745B0, void* ped, int time);
GAME_FN(CVector*, __cdecl, GetEntityPositionCopy, 0x0041AAE0, CVector* out, void* entity);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, Ped_StepAsideForVehicle_49A3A0, 0x0049A3A0, void* ped, void* vehicle);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(void, __cdecl, ClearScanCodes, 0x0045D840);
GAME_FN(void, __cdecl, SlowCarDownForPedsSectorList_, 0x0041C5F0, re::Ptr32<uint32_t>* list, void* vehicle, float x1, float y1, float x2, float y2,
        float* speed, float maxSpeed);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}
bool Visible(uint8_t* entity, uint16_t scanCode) {
    return At<uint16_t>(entity, 0x10A) != scanCode && At<int32_t>(entity, 0x20) != 0 &&
           (At<int32_t>(entity, 0x1C) != 0 || At<int32_t>(entity, 0x60) != 0) &&
           (ms_modelInfoPtrs[At<int16_t>(entity, 0x10E)][0xB] & static_cast<uint8_t>(1 << (ms_currentChapter & 0x1F))) &&
           At<int32_t>(entity, 0xF4) == 0;
}
// the sector range of a box (GTA's GetSectorX/Y, clamped to 0..35)
int SectorLow(float v) {
    float sector = static_cast<float>(static_cast<int>(v / 50.0 + 18.0));
    return static_cast<int>(sector > 0.0f ? sector : 0.0f);
}
int SectorHigh(float v) {
    float sector = static_cast<float>(static_cast<int>(v / 50.0 + 18.0));
    return static_cast<int>(sector < 35.0f ? sector : 35.0f);
}

// 0x41C5F0: the peds of a sector list in the box ahead of the vehicle
void __cdecl SlowCarDownForPedsSectorList(re::Ptr32<uint32_t>* list, uint8_t* vehicle, float x1, float y1, float x2, float y2, float* speed,
                                          float maxSpeed) {
    uint32_t* node = *list;
    if (!node) {
        return;
    }
    const float* col = At<float*>(ms_modelInfoPtrs[At<int16_t>(vehicle, 0x10E)], 0xC);
    // the box ahead: x across the bounding box, y from its front to 2 beyond
    float boxMin[3] = {col[4], col[9], col[6]};
    float boxMax[3] = {col[8], static_cast<float>(col[9] + 2.0), col[10]};
    if (At<int32_t>(vehicle, 0x46C) == 1) {
        boxMin[0] = static_cast<float>(boxMin[0] * 1.5);
        boxMax[0] = static_cast<float>(1.5 * boxMax[0]);
    }
    CMatrix matrix;
    Matrix_CopyConstruct(&matrix, reinterpret_cast<CMatrix*>(Entity_GetMatrix(vehicle)));
    CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(&matrix, &inverse);
    CMatrix toLocal;
    Matrix_CopyConstruct(&toLocal, &inverse);
    CVector forward;
    const CVector* f = Placeable_GetForward(vehicle, &forward);
    const CVector& moveSpeed = At<CVector>(vehicle, 0x124);
    float speedAlong = static_cast<float>(static_cast<double>(f->y) * moveSpeed.y + static_cast<double>(f->x) * moveSpeed.x +
                                          static_cast<double>(f->z) * moveSpeed.z);
    float step = ms_fSpeedScale_9E4F90 * ms_fTimeStep;
    if (step == 0.0f) {
        step = 0.10000000149011612f;
    }
    uint8_t* params = ms_pCarCtrlParams;
    float lookAhead = At<float>(params, 0x20);
    CVector speedCopy = moveSpeed;
    float local[4];
    Matrix_TransformVector(local, &toLocal, &speedCopy);
    float inverseStep = 1.0f / step;
    float ahead = local[1] * inverseStep;
    ahead = ahead * lookAhead;
    bool fastEnough = At<float>(params, 0x24) < speedAlong;
    float front = static_cast<float>(boxMax[1] + ahead);
    boxMax[1] = front >= boxMax[1] || std::isnan(front) ? front : boxMax[1];
    float back = static_cast<float>(boxMin[1] + ahead);
    boxMin[1] = back > boxMin[1] ? boxMin[1] : back;
    uint16_t scanCode = ms_nCurrentScanCode;
    float z = PositionOf(vehicle)->z;
    do {
        uint8_t* ped = static_cast<uint8_t*>(GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF));
        node = (*node >> 18) == 0x3FFF ? nullptr : ms_pPtrNodes + (*node >> 18);
        if (!Visible(ped, scanCode)) {
            continue;
        }
        At<uint16_t>(ped, 0x10A) = scanCode;
        if ((ped[0x108] & 7) == 3 && At<int16_t>(ped, 0x10E) == ms_ignoredPedModel) {
            continue;
        }
        CVector pos = *PositionOf(ped);
        if (!(x1 < pos.x) || !(x2 > pos.x) || !(y1 < pos.y) || !(y2 > pos.y) || !(std::fabs(static_cast<float>(pos.z - z)) < 4.0f)) {
            continue;
        }
        float along;
        if (!IsPointInBox(&pos, boxMin, boxMax, &matrix, nullptr, &along)) {
            continue;
        }
        void* seats = vehicle + 0x1C4;
        int8_t style = static_cast<int8_t>(vehicle[0x2D9]);
        bool reacts = true;
        if ((style < 2 || style > 3) && VehicleSeats_GetOccupant(seats, 0) &&
            At<int32_t>(VehicleSeats_GetOccupant(seats, 0), 0x1310) != 0xD && At<int32_t>(vehicle, 0x46C) != 1) {
            reacts = false;
        }
        Ped_492590(ped + 0xB44);
        // a still driver honks at a still ped (every 5 seconds)
        if (VehicleSeats_GetOccupant(seats, 0) && Ped_IsStateIdle(VehicleSeats_GetOccupant(seats, 0)) &&
            Vector_MagnitudeSqr(reinterpret_cast<CVector*>(ped + 0x124)) < 0.009999999776482582f) {
            uint8_t* pedVehicle = At<uint8_t*>(ped, 0x1554);
            bool still;
            if (pedVehicle) {
                still = Vector_MagnitudeSqr(reinterpret_cast<CVector*>(pedVehicle + 0x124)) < 0.009999999776482582f;
            } else {
                still = Vector_MagnitudeSqr(reinterpret_cast<CVector*>(ped + 0x124)) < 0.009999999776482582f;
            }
            if (still && m_snTimeInMilliseconds - At<uint32_t>(vehicle, 0x2D0) > 5000) {
                EventMgr_4A6670(ms_pEventMgr_C2C1A0, 0x4D, ped, VehicleSeats_GetOccupant(seats, 0), 0, -1.0f, -1.0f);
                Ped_SetPunishmentTime(ped, 5000);
            }
        }
        CVector pedPos;
        CVector* pp = GetEntityPositionCopy(&pedPos, ped);
        CVector* vp = Placeable_GetPosition(vehicle);
        CVector away;
        VectorSub(&away, vp, pp);
        Vector_Normalise(&away);
        if (reacts && At<int32_t>(ped, 0x16D8) == 0 && fastEnough) {
            CVector pedForward;
            if (DotProduct(&away, Placeable_GetForward(ped, &pedForward)) > -0.7070000171661377f) {
                Ped_StepAsideForVehicle_49A3A0(ped, vehicle);
            }
        }
        if (!(speedAlong < 10.0f) || !(3.0f > along)) {
            continue;
        }
        if (style != 0 && style != 4 && style != 1) {
            continue;
        }
        float slower = static_cast<float>(Max(0.0f, static_cast<float>(speedAlong - 1.0)) / 10.0 * maxSpeed);
        *speed = Min(*speed, slower);
        vehicle[0x2EC] |= 2;
        if (2.0f > speedAlong && vehicle[0x2DB] != 1) {
            vehicle[0x2DB] = 1;
            At<uint32_t>(vehicle, 0x2DC) = m_snTimeInMilliseconds + 3000;
        }
    } while (node);
}

// 0x41EE50: the peds within 11 of the vehicle checked (the sectors round it); its
// flag bit 1 (+0x2EC) kept as it was
void __cdecl SlowCarDownForPeds(uint8_t* vehicle) {
    uint8_t wasSlowed = (vehicle[0x2EC] >> 1) & 1;
    CVector pos = *PositionOf(vehicle);
    float xMin = static_cast<float>(pos.x - 11.0);
    float xMax = static_cast<float>(pos.x + 11.0);
    float yMin = static_cast<float>(pos.y - 11.0);
    float yMax = static_cast<float>(pos.y + 11.0);
    int minX = SectorLow(xMin);
    int minY = SectorLow(yMin);
    int maxX = SectorHigh(xMax);
    int maxY = SectorHigh(yMax);
    if (ms_nCurrentScanCode < 0xFFFF) {
        ms_nCurrentScanCode++;
    } else {
        ClearScanCodes();
        ms_nCurrentScanCode = 1;
    }
    float speed = static_cast<float>(static_cast<int8_t>(vehicle[0x2E4]));
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float cruise = static_cast<float>(static_cast<int8_t>(vehicle[0x2E4]));
            SlowCarDownForPedsSectorList_(&ms_aSectorPedLists[(y * 36 + x) * 5], vehicle, xMin, yMin, xMax, yMax, &speed, cruise);
        }
    }
    vehicle[0x33A] |= 0x10;
    vehicle[0x2EC] ^= ((wasSlowed * 2) ^ vehicle[0x2EC]) & 2;
}

// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline void Matrix_TransformPoint(float* out, const CMatrix* m, const CVector* point) { re::CallVec16<0x0041A9A0, void>(out, m, point); }

// one axis of the moving-rect test: when (as t goes 0..1) the point `v` moving by
// `speed` per unit t is within -back..front; `enter` and `leave` narrowed
void AxisEntry(float v, float speed, float front, float back, float* enter, float* leave) {
    if (v <= front) {
        if (v < -back) {
            if (0.0f < speed) {
                *enter = static_cast<float>(-((static_cast<double>(back) + v) / speed));
                if (1.0f <= *enter) {
                    *enter = 1.0f;
                    return;
                }
                float out = static_cast<float>(*enter + (static_cast<double>(back) + front) / speed);
                if (out < 1.0f) {
                    *leave = out;
                }
            } else {
                *leave = 1.0f;
                *enter = 1.0f;
            }
        } else if (speed <= 0.0f) {
            if (speed < 0.0f) {
                *leave = static_cast<float>(-((static_cast<double>(back) + v) / speed));
            }
        } else {
            *leave = static_cast<float>((static_cast<double>(front) - v) / speed);
        }
        return;
    }
    if (0.0f <= speed) {
        *leave = 1.0f;
        *enter = 1.0f;
        return;
    }
    *enter = static_cast<float>(-((static_cast<double>(v) - front) / speed));
    if (1.0f <= *enter) {
        *enter = 1.0f;
        return;
    }
    float out = static_cast<float>(*enter - (static_cast<double>(back) + front) / speed);
    if (out < 1.0f) {
        *leave = out;
    }
}

// 0x41CC70: GTA's TestCollisionBetween2MovingRects: the fraction (0..1) of the
// projection step at which vehicle A's corners enter B's rectangle; 2 when A's
// position is inside B already. (A kind-1 vehicle's rectangle is 0.6 of its box;
// B's rectangle is symmetric across, from -back to front along.)
float __cdecl TestCollisionBetween2MovingRects(uint8_t* a, uint8_t* b, float projectionX, float projectionY, const CVector* forwardA,
                                               const CVector* forwardB) {
    CVector posA = *PositionOf(a);
    const float* colB = At<float*>(ms_modelInfoPtrs[At<int16_t>(b, 0x10E)], 0xC);
    float widthB = colB[8];
    float lengthB = colB[9];
    if (At<int32_t>(b, 0x46C) == 1) {
        widthB = static_cast<float>(widthB * 0.6000000238418579);
        lengthB = static_cast<float>(0.6000000238418579 * lengthB);
    }
    CMatrix inverse;
    inverse.attached = nullptr;
    Matrix_Invert(reinterpret_cast<CMatrix*>(Entity_GetMatrix(b)), &inverse);
    CMatrix toLocal;
    Matrix_CopyConstruct(&toLocal, &inverse);
    float local[4];
    Matrix_TransformPoint(local, &toLocal, &posA);
    if (colB[4] < local[0] && widthB > local[0] && colB[5] < local[1] && lengthB > local[1]) {
        return 2.0f;
    }
    CVector posB = *PositionOf(b);
    const float* colA = At<float*>(ms_modelInfoPtrs[At<int16_t>(a, 0x10E)], 0xC);
    float widthA = colA[8];
    float lengthA = colA[9];
    if (At<int32_t>(a, 0x46C) == 1) {
        widthA = static_cast<float>(widthA * 0.6000000238418579);
        lengthA = static_cast<float>(0.6000000238418579 * lengthA);
    }
    float dx = posA.x - posB.x;
    float dy = posA.y - posB.y;
    float backB = -colB[5];
    float backA = -colA[5];
    float best = 1.0f;
    float projectionAcross = static_cast<float>(static_cast<double>(forwardA->y) * projectionX - static_cast<double>(forwardA->x) * projectionY);
    float projectionAlong = static_cast<float>(static_cast<double>(projectionX) * forwardA->x + static_cast<double>(projectionY) * forwardA->y);
    float fx = forwardB->x;
    float fy = forwardB->y;
    for (int corner = 0; corner < 4; corner++) {
        float cx, cy;
        switch (corner) {
        case 0:
            cx = static_cast<float>(static_cast<double>(fy) * widthA + static_cast<double>(lengthA) * fx + dx);
            cy = static_cast<float>(dy + static_cast<double>(fy) * lengthA - static_cast<double>(widthA) * fx);
            break;
        case 1:
            cx = static_cast<float>(static_cast<double>(lengthA) * fx + dx - static_cast<double>(fy) * widthA);
            cy = static_cast<float>(static_cast<double>(widthA) * fx + dy + static_cast<double>(fy) * lengthA);
            break;
        case 2:
            cx = static_cast<float>(static_cast<double>(fy) * widthA + (dx - static_cast<double>(fx) * backA));
            cy = static_cast<float>(dy - static_cast<double>(fy) * backA - static_cast<double>(widthA) * fx);
            break;
        default:
            cx = static_cast<float>(dx - static_cast<double>(fx) * backA - static_cast<double>(fy) * widthA);
            cy = static_cast<float>(static_cast<double>(widthA) * fx + (dy - static_cast<double>(fy) * backA));
            break;
        }
        float enterAcross = 0.0f;
        float leaveAcross = 1.0f;
        float across = static_cast<float>(static_cast<double>(forwardA->y) * cx - static_cast<double>(forwardA->x) * cy);
        AxisEntry(across, projectionAcross, widthB, widthB, &enterAcross, &leaveAcross);
        float enterAlong = 0.0f;
        float leaveAlong = 1.0f;
        float along = static_cast<float>(static_cast<double>(forwardA->x) * cx + static_cast<double>(forwardA->y) * cy);
        AxisEntry(along, projectionAlong, lengthB, backB, &enterAlong, &leaveAlong);
        if (enterAlong < enterAcross) {
            enterAlong = enterAcross;
        }
        if (enterAlong < leaveAcross && enterAlong < leaveAlong && enterAlong <= best) {
            best = enterAlong;
        }
    }
    return best;
}

GAME_FN(float, __cdecl, RecipSqrt, 0x00414C20, float numerator, float value);
GAME_FN(float, __cdecl, TestCollisionBetween2MovingRects_, 0x0041CC70, void* a, void* b, float projectionX, float projectionY,
        const CVector* forwardA, const CVector* forwardB, int id);

// a matrix's forward flattened and normalised
CVector FlatForward(void* entity) {
    const float* m = Entity_GetMatrix(entity);
    float fx = m[4];
    float fy = m[5];
    float scale = RecipSqrt(1.0f, static_cast<float>(static_cast<double>(fx) * fx + static_cast<double>(fy) * fy));
    return {scale * fx, scale * fy, 0.0f};
}

// 0x41F0C0: GTA's SlowCarDownForOtherCar: another vehicle ahead on a collision course
// caps the speed (the course tested both ways, the nearer taken)
void __cdecl SlowCarDownForOtherCar(uint8_t* other, uint8_t* vehicle, float* speed, float curSpeed) {
    CVector forwardA = FlatForward(vehicle);
    const CVector* v = PositionOf(vehicle);
    const CVector* o = PositionOf(other);
    double dot = (static_cast<double>(o->y) - v->y) * forwardA.y + (static_cast<double>(o->x) - v->x) * forwardA.x;
    if (dot < 0.0) {
        return;
    }
    float otherX = static_cast<float>(At<float>(other, 0x124) * 30.0);
    float otherY = static_cast<float>(30.0 * At<float>(other, 0x128));
    float ownX = forwardA.x * curSpeed;
    float ownY = forwardA.y * curSpeed;
    CVector forwardB = FlatForward(other);
    float projectionX = otherX - ownX;
    float projectionY = otherY - ownY;
    float first = TestCollisionBetween2MovingRects_(other, vehicle, projectionX, projectionY, &forwardA, &forwardB, 0);
    float second = TestCollisionBetween2MovingRects_(vehicle, other, -projectionX, -projectionY, &forwardB, &forwardA, 1);
    float proximity = second > first ? first : second;
    if (!(proximity >= 0.0f) || !(1.5f > proximity)) {
        return;
    }
    proximity = static_cast<float>((proximity - 0.20000000298023224) * 0.7692307829856873);
    if (proximity < 0.0) {
        proximity = 0.0f;
    }
    vehicle[0x2EC] |= 1;
    float old = *speed;
    float capped = proximity * curSpeed;
    *speed = capped > old ? old : capped;
}

} // namespace c01slow

// ---- registration ----------------------------------------------------------------------

#define SLOW_REVERSED_C01(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01slow {
SLOW_REVERSED_C01("CCarCtrl::SlowCarDownForPedsSectorList", 0x0041C5F0, &SlowCarDownForPedsSectorList);
SLOW_REVERSED_C01("CCarCtrl::SlowCarDownForPeds", 0x0041EE50, &SlowCarDownForPeds);
SLOW_REVERSED_C01("CCarCtrl::TestCollisionBetween2MovingRects", 0x0041CC70, &TestCollisionBetween2MovingRects);
SLOW_REVERSED_C01("CCarCtrl::SlowCarDownForOtherCar", 0x0041F0C0, &SlowCarDownForOtherCar);
} // namespace c01slow
