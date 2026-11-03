// Buoyancy (GTA's cBuoyancy): an entity's bounding box sampled on a grid against the
// water level; the samples under water give a force and the point it acts at.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace c11buoy {

using col::CVector;

struct cBuoyancy {
    CVector pos;                  // +0x0 the entity's position
    col::CMatrix matrix;          // +0xC the entity's matrix (copied)
    uint32_t ownsAttached;        // +0x50 (the matrix's: GTA's m_bOwnsAttachedMatrix)
    CVector entityZ;              // +0x54 (0, 0, the entity's z)
    float waterLevel;             // +0x60
    float waterLevelOffset;       // +0x64 (added to +0x60 by PreCalcSetup)
    float buoyancy;               // +0x68
    CVector boundMax;             // +0x6C
    CVector boundMin;             // +0x78
    float numCheckedPoints;       // +0x84 (1 upwards: the running average's weight)
    uint8_t unknown88[0xC];
    uint8_t inWater;              // +0x94
    uint8_t unknown95[0x3];
    CVector step;                 // +0x98 the grid's steps (the box's size / +0xB0)
    CVector stepRatio;            // +0xA4 the steps over the largest
    float gridDivisions;          // +0xB0 (2 while processing)
    uint8_t flipCentre;           // +0xB4 the centre's contributions negated
    uint8_t unknownB5;
    uint8_t squareDepth;          // +0xB6 depths squared (and scaled); also: always "in water"
    uint8_t unknownB7;
    float volume;                 // +0xB8 the summed depths
    CVector centre;               // +0xBC where the force acts (entity space)

    // 0x4C81B0: a sample's depth summed and its point averaged into the centre;
    // the depth (0: above the water)
    float SimpleSumBuoyancyData(CVector* point, int state);
    // 0x4C83E0: the box, the grid, the sums reset for the entity
    void PreCalcSetup(CEntity* entity, float buoyancyValue);
    // 0x4C8580: the centre (world axes) and the force for this frame (0: not in water)
    uint8_t CalcBuoyancyForce(CEntity* entity, CVector* outCentre, CVector* outForce);
    // 0x4C8610: a grid point's depth below the water (clamped to the box): the state 0
    // under the box, 1 inside, 2 above
    void FindWaterLevel(CVector* entityPos, CVector* point, int* state);
    // 0x4C86C0: every grid point summed, the volume normalised
    void SimpleCalcBuoyancy();
    // 0x4C87C0
    uint8_t ProcessBuoyancy(CEntity* entity, float buoyancyValue, CVector* outCentre, CVector* outForce);
};
static_assert(offsetof(cBuoyancy, matrix) == 0xC);
static_assert(offsetof(cBuoyancy, entityZ) == 0x54);
static_assert(offsetof(cBuoyancy, waterLevel) == 0x60);
static_assert(offsetof(cBuoyancy, buoyancy) == 0x68);
static_assert(offsetof(cBuoyancy, boundMax) == 0x6C);
static_assert(offsetof(cBuoyancy, boundMin) == 0x78);
static_assert(offsetof(cBuoyancy, numCheckedPoints) == 0x84);
static_assert(offsetof(cBuoyancy, inWater) == 0x94);
static_assert(offsetof(cBuoyancy, step) == 0x98);
static_assert(offsetof(cBuoyancy, stepRatio) == 0xA4);
static_assert(offsetof(cBuoyancy, gridDivisions) == 0xB0);
static_assert(offsetof(cBuoyancy, flipCentre) == 0xB4);
static_assert(offsetof(cBuoyancy, squareDepth) == 0xB6);
static_assert(offsetof(cBuoyancy, volume) == 0xB8);
static_assert(offsetof(cBuoyancy, centre) == 0xBC);
static_assert(sizeof(cBuoyancy) == 0xC8);

} // namespace c11buoy

namespace {

using namespace c11buoy;

GAME_FN(float, __thiscall, Buoy_SimpleSumBuoyancyData, 0x004C81B0, cBuoyancy* b, CVector* point, int state);
GAME_FN(void, __thiscall, Buoy_PreCalcSetup, 0x004C83E0, cBuoyancy* b, CEntity* entity, float buoyancyValue);
GAME_FN(uint8_t, __thiscall, Buoy_CalcBuoyancyForce, 0x004C8580, cBuoyancy* b, CEntity* entity, CVector* outCentre,
        CVector* outForce);
GAME_FN(void, __thiscall, Buoy_FindWaterLevel, 0x004C8610, cBuoyancy* b, CVector* entityPos, CVector* point, int* state);
GAME_FN(void, __thiscall, Buoy_SimpleCalcBuoyancy, 0x004C86C0, cBuoyancy* b);

// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3(CVector* out, const col::CMatrix* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Matrix_Copy, 0x00412720, col::CMatrix* dst, const void* src);
GAME_FN(void*, __thiscall, Entity_GetMatrix, 0x008B6120, CEntity* entity);

GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_VAR(float, timeStep, 0x00C1A9A4);

// the sum's scratch (the game's statics)
GAME_VAR(float, depthScale, 0x00A357AC);          // (1: set before every sample)
GAME_VAR(float, sumKeep, 0x00C2DB38);
GAME_VAR(float, sumWeight, 0x00C2DB3C);
GAME_VAR(float, sumDepth, 0x00C2DB40);
GAME_VAR(CVector, sumPoint, 0x00C2DB44);
GAME_VAR(uint32_t, sumPointInit, 0x00C2DB50);     // bit 0: sumPoint zeroed once

} // namespace

namespace c11buoy {

float cBuoyancy::SimpleSumBuoyancyData(CVector* point, int) {
    if (!(sumPointInit & 1)) {
        sumPointInit |= 1;
        sumPoint = {0.0f, 0.0f, 0.0f};
    }
    float depth = std::fabs(point->z - boundMin.z);
    sumDepth = float(double(depth) - (1.0 - double(depthScale)));
    if (sumDepth < 0.0f) {
        return 0.0f;
    }
    if (squareDepth) {
        sumDepth = float(double(depthScale) * sumDepth);
        sumDepth = float(double(sumDepth) * sumDepth);
    }
    float amount = sumDepth;
    volume = float(double(volume) + amount);
    sumPoint.z = float((double(boundMin.z) + point->z) * 0.5 * stepRatio.z);
    sumPoint.x = stepRatio.x * point->x;
    sumPoint.y = stepRatio.y * point->y;
    if (flipCentre) {
        sumPoint.x = float(sumPoint.x * -1.0);
        sumPoint.y = float(sumPoint.y * -1.0);
        sumPoint.z = float(sumPoint.z * -1.0);
    }
    sumWeight = float(1.0 / numCheckedPoints);
    double weight = sumWeight;
    sumKeep = float(1.0 - weight);
    CVector added = {float(weight * sumPoint.x), float(weight * sumPoint.y), float(weight * sumPoint.z)};
    CVector scaled = {float(double(amount) * added.x), float(double(amount) * added.y), float(double(amount) * added.z)};
    CVector kept = {sumKeep * centre.x, centre.y * sumKeep, sumKeep * centre.z};
    CVector result;
    CVector* sum = VectorAdd(&result, &kept, &scaled);
    centre = *sum;
    numCheckedPoints = float(numCheckedPoints + 1.0);
    inWater = 1;
    return sumDepth;
}

void cBuoyancy::PreCalcSetup(CEntity* entity, float buoyancyValue) {
    squareDepth = 0;
    int16_t model = *reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(entity) + 0x10E);
    CColModel* col = *reinterpret_cast<re::Ptr32<CColModel>*>(modelInfoPtrs[model] + 0xC);
    boundMin = col->boxMin;
    boundMax = col->boxMax;
    step.x = float((double(boundMax.x) - boundMin.x) / gridDivisions);
    step.y = float((double(boundMax.y) - boundMin.y) / gridDivisions);
    step.z = float((double(boundMax.z) - boundMin.z) / gridDivisions);
    float x = step.x;
    float y = step.y;
    float z = step.z;
    if (z > x && z > y) {
        stepRatio.x = float(double(x) / z);
        stepRatio.y = float(double(y) / z);
        stepRatio.z = 1.0f;
    } else {
        float largest;
        if (y > x && y > z) {
            stepRatio.x = float(double(x) / y);
            stepRatio.y = 1.0f;
            largest = y;
        } else {
            stepRatio.x = 1.0f;
            stepRatio.y = float(double(y) / x);
            largest = x;
        }
        stepRatio.z = float(double(z) / largest);
    }
    numCheckedPoints = 1.0f;
    inWater = 0;
    volume = 0.0f;
    centre = {0.0f, 0.0f, 0.0f};
    uint8_t* e = reinterpret_cast<uint8_t*>(entity);
    uint8_t* m = *reinterpret_cast<re::Ptr32<uint8_t>*>(e + 0x14);
    pos = *reinterpret_cast<CVector*>(m ? m + 0x30 : e + 4);
    entityZ.x = 0.0f;
    entityZ.y = 0.0f;
    entityZ.z = pos.z;
    buoyancy = buoyancyValue;
    waterLevel = waterLevelOffset + waterLevel;
}

uint8_t cBuoyancy::CalcBuoyancyForce(CEntity*, CVector* outCentre, CVector* outForce) {
    if (!inWater) {
        return 0;
    }
    CVector c = centre;
    struct { CVector v; float pad; } rotatedPad16;   // (the game's transform writes a 4th float)
    CVector& rotated = rotatedPad16.v;
    Multiply3x3(&rotated, &matrix, &c);
    *outCentre = rotated;
    outForce->x = 0.0f;
    outForce->y = 0.0f;
    outForce->z = float(double(volume) * buoyancy * timeStep);
    return 1;
}

void cBuoyancy::FindWaterLevel(CVector* entityPos, CVector* point, int* state) {
    *state = 1;
    CVector local = *point;
    struct { CVector v; float pad; } rotatedPad16;   // (the game's transform writes a 4th float)
    CVector& rotated = rotatedPad16.v;
    Multiply3x3(&rotated, &matrix, &local);
    point->z = 0.0f;
    float z = float(0.0 - (double(entityPos->z) + rotated.z));
    point->z = z;
    if (boundMax.z < z) {
        point->z = boundMax.z;
        *state = 2;
        return;
    }
    if (boundMin.z > z) {
        point->z = boundMin.z;
        *state = 0;
    }
}

void cBuoyancy::SimpleCalcBuoyancy() {
    CVector point = {0.0f, 0.0f, 0.0f};
    for (float x = boundMin.x; x <= boundMax.x; x = step.x + x) {
        for (float y = boundMin.y; y <= boundMax.y; y = y + step.y) {
            int state;
            point.x = x;
            point.y = y;
            point.z = 0.0f;
            Buoy_FindWaterLevel(this, &entityZ, &point, &state);
            depthScale = 1.0f;
            if (state) {
                Buoy_SimpleSumBuoyancyData(this, &point, state);
            }
        }
    }
    double divisions = double(gridDivisions) + 1.0;
    volume = float(volume / ((double(boundMax.z) - boundMin.z) * (divisions * divisions)));
}

uint8_t cBuoyancy::ProcessBuoyancy(CEntity* entity, float buoyancyValue, CVector* outCentre, CVector* outForce) {
    gridDivisions = 2.0f;
    waterLevel = 0.0f;
    Matrix_Copy(&matrix, Entity_GetMatrix(entity));
    Buoy_PreCalcSetup(this, entity, buoyancyValue);
    Buoy_SimpleCalcBuoyancy(this);
    uint8_t inWaterNow = Buoy_CalcBuoyancyForce(this, entity, outCentre, outForce);
    if (squareDepth) {
        return 1;
    }
    return float(inWaterNow) != 0.0f ? 1 : 0;
}

} // namespace c11buoy

// ---- registration ---------------------------------------------------------------------

#define BUOY_REVERSED(name, address, member)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, re::MemberAddress(member), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

BUOY_REVERSED("cBuoyancy::SimpleSumBuoyancyData", 0x004C81B0, &c11buoy::cBuoyancy::SimpleSumBuoyancyData);
BUOY_REVERSED("cBuoyancy::PreCalcSetup", 0x004C83E0, &c11buoy::cBuoyancy::PreCalcSetup);
BUOY_REVERSED("cBuoyancy::CalcBuoyancyForce", 0x004C8580, &c11buoy::cBuoyancy::CalcBuoyancyForce);
BUOY_REVERSED("cBuoyancy::FindWaterLevel", 0x004C8610, &c11buoy::cBuoyancy::FindWaterLevel);
BUOY_REVERSED("cBuoyancy::SimpleCalcBuoyancy", 0x004C86C0, &c11buoy::cBuoyancy::SimpleCalcBuoyancy);
BUOY_REVERSED("cBuoyancy::ProcessBuoyancy", 0x004C87C0, &c11buoy::cBuoyancy::ProcessBuoyancy);
