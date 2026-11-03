// CCurves: the curves cars on rails drive between two path points (GTA's CCurves;
// Bully's weighs the two directions linearly, with half of each). 0x428DA0..0x428FC1.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#define CURVES_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

struct CurveVector {
    float x, y, z;
};
static_assert(sizeof(CurveVector) == 0xC);

GAME_FN(CurveVector*, __cdecl, CV_Sub, 0x00414B70, CurveVector* out, const CurveVector* a, const CurveVector* b);
GAME_FN(float, __cdecl, CV_CalcSpeedScaleFactor, 0x00428DA0, const CurveVector* start, const CurveVector* end, float dir1X, float dir1Y,
        float dir2X, float dir2Y);

} // namespace

namespace CCurves {

// 0x428DA0: the length of the curve: the distance (2D), plus the sideways offset of
// the points along the first direction when the directions are nearly the same (a
// lane change)
float __cdecl CalcSpeedScaleFactor(const CurveVector* start, const CurveVector* end, float dir1X, float dir1Y, float dir2X, float dir2Y) {
    CurveVector d;
    CV_Sub(&d, start, end);
    float sq = d.x * d.x + d.y * d.y;
    // (the game's square root takes the fabs first)
    float distance = std::sqrt(std::fabs(sq));
    double dot = double(dir1X) * dir2X + double(dir1Y) * dir2Y;
    if (!(dot > double(0.9f))) {
        return distance;
    }
    float a = float(double(start->x) * dir1Y - double(start->y) * dir1X);
    float b = float(double(end->x) * dir1Y - double(end->y) * dir1X);
    float offset = std::fabs(a - b);
    return offset + distance;
}

// 0x428E70: the point `between` (0..1) along the curve and the speed there (per second,
// the curve taking `timeOnCurve` ms); z 0 in both
void __cdecl CalcCurvePoint(const CurveVector* start, const CurveVector* end, const CurveVector* startDir, const CurveVector* endDir,
                            float between, int32_t timeOnCurve, CurveVector* outPos, CurveVector* outSpeed) {
    float a1 = float(startDir->x * 0.5);
    float b1 = float(startDir->y * 0.5);
    float a2 = float(endDir->x * 0.5);
    float b2 = float(0.5 * endDir->y);
    float factor = CV_CalcSpeedScaleFactor(start, end, a1, b1, a2, b2);
    float A1 = a1 * factor;
    float B1 = b1 * factor;
    float A2 = a2 * factor;
    float B2 = factor * b2;
    float t = between;
    float u = float(1.0 - t);
    float p1y = start->y + t * B1;
    float q2y = float(end->y - B2 * double(u));
    float q2x = float(end->x - A2 * double(u));
    float p1x = float(start->x + A1 * double(t));
    outPos->x = float(p1x * double(u) + q2x * double(t));
    outPos->y = float(q2y * double(t) + p1y * double(u));
    outPos->z = 0.0f;
    double seconds = timeOnCurve * 0.0010000000474974513;
    outSpeed->x = float((A2 * double(t) + A1 * double(u)) / seconds);
    outSpeed->y = float((B2 * double(t) + B1 * double(u)) / seconds);
    outSpeed->z = 0.0f;
}

CURVES_REVERSED("CCurves::CalcSpeedScaleFactor", 0x00428DA0, &CalcSpeedScaleFactor);
CURVES_REVERSED("CCurves::CalcCurvePoint", 0x00428E70, &CalcCurvePoint);

} // namespace CCurves
