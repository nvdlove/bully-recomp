// A ped's obstacle avoidance (0x5DC3A0..0x5DF660; Ghidra's JitterMovementInfo /
// BalanceMeterTrack names are wrong): the peds and objects near a walking ped, the free
// headings around them, and swept-circle tests of its path against lines (the two
// segments below). The state is global (0xD09518..0xD09640):
//   0xD09520 the ped's position        0xD09568, 0xD0956C the left / right costs
//   0xD09570 its radius                0xD09574 its heading
//   0xD09578 an obstacle's position (+ a w)            0xD09588 the direction it tests
//   0xD095BC the obstacle's radius     0xD095C0 a movement vector
//   0xD095D0 a segment (start, end: CVector + w each), 0xD095F0 another
//   0xD09618 / 0xD0961C the widest free heading on each side (relative), 0xD09620 /
//   0xD09624 their distances; 0xD0962C the obstacles (5), 0xD09640 their count
// A segment's own functions (thiscall on it) are here too (length, a point along it).
//
// The CRT's x87 sqrt / sin / cos / asin / atan2 (0x85C500, 0x85AEF0, 0x85AD80, 0x85D460,
// 0x85CF1A) are the C library's here; results are stored as floats where the game stores
// them.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct PaVec {
    float x, y, z;
};
static_assert(sizeof(PaVec) == 12);

// A segment (0x20 bytes: its ends with a w each)
struct PaSegment {
    float start[4];
    float end[4];
};
static_assert(sizeof(PaSegment) == 0x20);

template <typename T>
re::Field<T>& paAt(void* base, int offset) {
    return *reinterpret_cast<re::Field<T>*>(static_cast<uint8_t*>(base) + offset);
}

template <typename R, typename... A>
R paVCall(void* self, int offset, A... args) {
    using Fn = R(__thiscall*)(void*, re::Arg<A>...);
    Fn fn = re::GameFnAt<Fn>(*static_cast<re::Ptr32<uint8_t>*>(self) + offset);
    return fn(self, args...);
}

// The state
GAME_VAR(PaVec, paPos, 0x00D09520);
GAME_VAR(float, paLeftCost, 0x00D09568);
GAME_VAR(float, paRightCost, 0x00D0956C);
GAME_VAR(float, paRadius, 0x00D09570);
GAME_VAR(float, paHeading, 0x00D09574);
inline float* const paObstaclePos = reinterpret_cast<float*>(0x00D09578);  // 4 floats
GAME_VAR(PaVec, paDir, 0x00D09588);
GAME_VAR(float, paObstacleRadius, 0x00D095BC);
GAME_VAR(PaVec, paMove, 0x00D095C0);
inline PaSegment* const paSeg1 = reinterpret_cast<PaSegment*>(0x00D095D0);
inline PaSegment* const paSeg2 = reinterpret_cast<PaSegment*>(0x00D095F0);
GAME_VAR(float, paMaxAngle, 0x00D09618);
GAME_VAR(float, paMinAngle, 0x00D0961C);
GAME_VAR(float, paMaxDist, 0x00D09620);
GAME_VAR(float, paMinDist, 0x00D09624);
GAME_ARRAY(uint8_t*, paObstacles, 0x00D0962C);
GAME_VAR(int32_t, paObstacleCount, 0x00D09640);

// Vector helpers (the game's)
GAME_FN(PaVec*, __cdecl, paVectorAdd, 0x004129A0, PaVec* out, const PaVec* a, const PaVec* b);
GAME_FN(PaVec*, __cdecl, paVectorSub, 0x00414B70, PaVec* out, const void* a, const void* b);
GAME_FN(float, __cdecl, paDot, 0x00414640, const void* a, const void* b);
GAME_FN(PaVec*, __cdecl, paNormalise2D, 0x0041C590, PaVec* out, const void* in);
GAME_FN(float, __thiscall, paNormalise, 0x004294A0, PaVec* v);
GAME_FN(float, __cdecl, paSqrtAbs, 0x0040DF90, float value);
GAME_FN(PaVec*, __cdecl, paFromHeading, 0x00432DF0, PaVec* out, float heading);
GAME_FN(float, __cdecl, paHeadingOf, 0x00433310, const void* v);
GAME_FN(float, __cdecl, paAngleDiff, 0x00432980, float a, float b);
GAME_FN(uint8_t, __cdecl, paIntersect2D, 0x005753B0, const float* p0, const float* p1, const float* q0, const float* q1, float* t);
// The segment's (thiscall on it)
GAME_FN(float, __thiscall, paSeg_Distance, 0x005E6E60, const PaSegment* seg, const void* point);
GAME_FN(PaVec*, __thiscall, paSeg_Normal2D, 0x005E7220, const PaSegment* seg, PaVec* out);
GAME_FN(PaVec*, __thiscall, paSeg_Closest, 0x005E74A0, const PaSegment* seg, PaVec* out, const void* point, float* t);
GAME_FN(float, __thiscall, paSeg_AngleTo, 0x005E7680, const PaSegment* seg, const PaSegment* other);
GAME_FN(float, __thiscall, paSeg_Length, 0x005DCF40, const PaSegment* seg);
GAME_FN(PaVec*, __thiscall, paSeg_PointAt, 0x005DCFA0, const PaSegment* seg, PaVec* out, float t);
// This file's, through the game
GAME_FN(void, __thiscall, paTrackHeading, 0x005DC3A0, void* self, float angle, float dist);
GAME_FN(void, __thiscall, paAvoidCircle, 0x005DC670, void* self, const float* pos);
GAME_FN(uint8_t, __thiscall, paAvoidPoints, 0x005DC790, void* self, const float* points, int count);
GAME_FN(void, __thiscall, paAddSideCost, 0x005DD010, void* self, float cost);

inline float paSqrt(float v) {
    return float(std::sqrt(double(v)));
}

GAME_ARRAY(uint8_t*, paModelInfoPtrs, 0x00C67738);
inline uint8_t* paModelInfoOf(uint8_t* entity) {
    return paModelInfoPtrs[paAt<int16_t>(entity, 0x10E)];
}

// An entity's position (its matrix's, else the placeable's own)
inline float* paEntityPos(uint8_t* entity) {
    uint8_t* matrix = paAt<uint8_t*>(entity, 0x14);
    return matrix != nullptr ? reinterpret_cast<float*>(matrix + 0x30) : reinterpret_cast<float*>(entity + 4);
}

// 0x5DC3A0 (thiscall, unused): a heading (made relative to the ped's) kept when it's the
// widest free one on its side so far, with its distance
void __fastcall TrackHeading(void*, re::Edx, float heading, float dist) {
    float angle = paAngleDiff(paHeading, heading);
    if (paMinAngle > angle) {
        paMinDist = dist;
        paMinAngle = angle;
    }
    if (paMaxAngle < angle) {
        paMaxAngle = angle;
        paMaxDist = dist;
    }
}

// 0x5DC410: the convex hull (gift wrapping, from point 0) of 8 points (16 bytes apart),
// as indices into hull[6]: the hull's size (the first of hull[4], hull[5] still -1, else
// 6). Only hull[0] is cleared here. (A step that finds nothing writes visited[-1]: the
// top byte of the step counter, which ends the loop.)
int __stdcall ConvexHull8(const float* points, int32_t* hull) {
    float previous = -3.1415927f;
    uint8_t visited[8] = {};
    int32_t steps = 0;
    int current = 0;
    bool wrapped = false;
    int slot = 0;
    hull[0] = 0;
    for (;;) {
        float best = 3.4028235e+38f;
        float bestAngle = 0.0f;
        int bestIndex = -1;
        const float* p = points;
        for (int i = 0; i < 8; i++, p += 4) {
            if (i == current || visited[i] != 0) {
                continue;
            }
            const float* from = points + current * 4;
            float dx = p[0] - from[0];
            float dy = p[1] - from[1];
            float a = float(std::atan2(double(dy), double(dx)));
            float d = a - previous;
            if (d < 0.0f) {
                d = float(double(d) + 6.2831854820251465);
            }
            if (best > d) {
                best = d;
                bestIndex = i;
                bestAngle = a;
            }
        }
        int next = (slot + 1) % 6;
        if (hull[slot] == bestIndex || hull[next] == bestIndex || hull[(slot + 2) % 6] == bestIndex) {
            break;
        }
        hull[slot] = bestIndex;
        if (slot > 1) {
            wrapped = true;
        }
        if (slot > 1 || wrapped) {
            if (bestIndex == -1) {
                steps |= 0x01000000;
            } else {
                visited[bestIndex] = 1;
            }
        }
        previous = bestAngle;
        steps++;
        slot = next;
        current = bestIndex;
        if (steps >= 8) {
            break;
        }
    }
    for (int i = 4; i < 6; i++) {
        if (hull[i] == -1) {
            return i;
        }
    }
    return 6;
}

// 0x5DC5A0: a cost from a speed (at least 0.1), an angle and a distance (at least 0.05)
float __stdcall AvoidCost(float speed, float angle, float dist) {
    if (double(speed) < double(0.1f)) {
        speed = 0.1f;
    }
    float inverse = 1.0f / speed;
    float a = std::fabs(angle);
    if (!(double(dist) > double(0.05f))) {  // (NaN too)
        dist = 0.05f;
    }
    return float(double(dist) * 0.5 + (a + a) + double(inverse) * double(0.4f));
}

// 0x5DC630: eight dwords set
void __fastcall Set8(uint32_t* self, re::Edx, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h) {
    self[0] = a;
    self[1] = b;
    self[2] = c;
    self[3] = d;
    self[4] = e;
    self[5] = f;
    self[6] = g;
    self[7] = h;
}

// 0x5DC670: the headings grazing a circle around the obstacle (its radius plus the
// ped's) tracked, with the tangent's length
void __fastcall AvoidCircle(void* self, re::Edx, const float* pos) {
    PaVec d = {pos[0] - paPos.x, pos[1] - paPos.y, pos[2] - paPos.z};
    float dist = paSqrt(std::fabs(d.y * d.y + d.x * d.x));
    float r = paObstacleRadius + paRadius;
    float half;
    if (!(r > dist)) {
        half = float(std::asin(double(r / dist)));
    } else {
        half = 1.5707964f;
    }
    float c = float(std::cos(double(half)));
    float tangent = c * dist;
    float heading = paHeadingOf(&d);
    paTrackHeading(self, heading - half, tangent);
    paTrackHeading(self, half + heading, tangent);
}

// 0x5DC790: the same for a group of points (16 bytes apart): the widest on each side,
// when there's one; true
uint8_t __fastcall AvoidPoints(void* self, re::Edx, const float* points, int count) {
    float maxAngle = 0.0f;
    float minAngle = 0.0f;
    float maxDist = -1.0f;
    float minDist = -1.0f;
    if (count <= 0) {
        return 1;
    }
    const float* p = points;
    for (int n = count; n != 0; n--, p += 4) {
        PaVec d = {p[0] - paPos.x, p[1] - paPos.y, p[2] - paPos.z};
        float heading = paHeadingOf(&d);
        float dist = paSqrt(std::fabs(d.y * d.y + d.x * d.x));
        float half;
        if (!(paRadius > dist)) {
            half = float(std::asin(double(paRadius / dist)));
        } else {
            half = 1.5707964f;
        }
        float a1 = paAngleDiff(paHeading, half + heading);
        float a2 = paAngleDiff(paHeading, heading - half);
        if (maxAngle < a1) {
            maxAngle = a1;
            maxDist = dist;
        }
        if (minAngle > a2) {
            minAngle = a2;
            minDist = dist;
        }
    }
    if (maxDist > 0.0f) {
        paTrackHeading(self, paHeading + maxAngle, maxDist);
    }
    if (minDist > 0.0f) {
        paTrackHeading(self, paHeading + minAngle, minDist);
    }
    return 1;
}

// 0x5DC990: the obstacles near the two widest free headings' lines (their circles
// closer to either than the two radii) avoided too. (The lines' w and the obstacle
// position's w are stack the game never sets: zero here.)
void __fastcall AvoidNearLines(void* self, re::Edx) {
    float minDist = paMinDist;
    PaVec v;
    paFromHeading(&v, paMinAngle + paHeading);
    PaVec left = {minDist * v.x, v.y * minDist, v.z * minDist};
    float maxDist = paMaxDist;
    paFromHeading(&v, paMaxAngle + paHeading);
    PaVec right = {maxDist * v.x, v.y * maxDist, v.z * maxDist};
    PaVec a;
    paVectorAdd(&a, &paPos, &left);
    PaSegment line1 = {{paPos.x, paPos.y, paPos.z, 0.0f}, {a.x, a.y, a.z, 0.0f}};
    PaVec b;
    paVectorAdd(&b, &paPos, &right);
    PaSegment line2 = {{paPos.x, paPos.y, paPos.z, 0.0f}, {b.x, b.y, b.z, 0.0f}};
    for (int i = 0; i < paObstacleCount; i++) {
        float* pos = paEntityPos(paObstacles[i]);
        paObstaclePos[0] = pos[0];
        paObstaclePos[1] = pos[1];
        paObstaclePos[2] = pos[2];
        paObstaclePos[3] = 0.0f;
        paObstacleRadius = paVCall<float>(paObstacles[i], 0x74);
        float reach = paRadius + paObstacleRadius;
        if (reach > paSeg_Distance(&line1, paObstaclePos) || reach > paSeg_Distance(&line2, paObstaclePos)) {
            paAvoidCircle(self, paObstaclePos);
        }
    }
}

// 0x5DCC80 (on a ped pointer): whether the way along `dir` (0.3 further than `dist`) is
// clear of the ped's current nav area's edges (true without one)
GAME_FN(uint8_t, __thiscall, paNavArea_IsValid, 0x005E7AF0, void* area);
GAME_FN(uint8_t, __cdecl, paNavArea_LineClear, 0x005E9F50, void* area, const PaVec* to, int a, uintptr_t b, int c, int d, uintptr_t e);
uint8_t __fastcall IsWayClear(re::Ptr32<uint8_t>* self, re::Edx, const PaVec* dir, float dist) {
    uint8_t* ped = *self;
    if (paAt<uint8_t>(ped, 0x12EB) == 0) {
        return 1;
    }
    uint32_t area[5];
    for (int i = 0; i < 5; i++) {
        area[i] = paAt<uint32_t>(ped, 0x1268 + i * 4);
    }
    if (!paNavArea_IsValid(ped + 0x1268)) {
        return 0;
    }
    dist = float(double(dist) + double(0.3f));
    PaVec start = paPos;
    PaVec step = {dist * dir->x, dir->y * dist, dist * dir->z};
    PaVec to;
    paVectorAdd(&to, &start, &step);
    return paNavArea_LineClear(area, &to, 0, 0, 0, 1, 0) != 0;
}

// 0x5DCD80 (on an avoidance record: +0 its ped, +0x1C 15 entries, +0x58 their count):
// the nearest 15 of the ped list's (0xD12AF8, count 0xD12E18) entries within 20 (the
// height difference counted twice; once full, one replaces the farthest found at the
// last replacement)
void __fastcall GatherNearby(uint8_t* self, re::Edx) {
    int32_t count = *reinterpret_cast<int32_t*>(0x00D12E18);
    re::Ptr32<uint8_t>* list = reinterpret_cast<re::Ptr32<uint8_t>*>(0x00D12AF8);
    paAt<uint8_t>(self, 0x58) = 0;
    float* own = paEntityPos(paAt<uint8_t*>(self, 0));
    float x = own[0];
    float y = own[1];
    float z = own[2];
    float worst = 3.4028235e+38f;
    float dists[15];
    for (int i = 0; i < count; i++) {
        uint8_t* entry = list[i];
        uint8_t* entity = paAt<uint8_t*>(entry, 0x58);
        if (entity == nullptr) {
            continue;
        }
        float* pos = paEntityPos(entity);
        float dx = pos[0] - x;
        float dy = pos[1] - y;
        float dz = pos[2] - z;
        float dz2 = dz * dz;
        float sum = dx * dx + dy * dy + dz2;
        float d2 = sum + dz2;
        if (!(d2 < 400.0)) {
            continue;
        }
        uint8_t n = paAt<uint8_t>(self, 0x58);
        if (n < 0xF) {
            paAt<uint8_t*>(self, 0x1C + n * 4) = entry;
            paAt<uint8_t>(self, 0x58) = uint8_t(n + 1);
            dists[n] = d2;
            continue;
        }
        if (!(d2 < worst)) {
            continue;
        }
        int far = 0;
        float m = dists[0];
        for (int k = 1; k < 0xF; k += 2) {
            if (dists[k] > m) {
                far = k;
                m = dists[k];
            }
            if (dists[k + 1] > m) {
                far = k + 1;
                m = dists[k + 1];
            }
        }
        worst = m;
        if (m > d2) {
            paAt<uint8_t*>(self, 0x1C + far * 4) = entry;
            dists[far] = d2;
        }
    }
}

// 0x5DCF40: the segment's length
float __fastcall Segment_Length(const PaSegment* self, re::Edx) {
    float dx = self->start[0] - self->end[0];
    float dy = self->start[1] - self->end[1];
    float dz = self->start[2] - self->end[2];
    return paSqrt(std::fabs(dx * dx + dy * dy + dz * dz));
}

// 0x5DCFA0: the point at t along the segment
PaVec* __fastcall Segment_PointAt(const PaSegment* self, re::Edx, PaVec* out, float t) {
    float dx = self->end[0] - self->start[0];
    float dy = self->end[1] - self->start[1];
    float dz = self->end[2] - self->start[2];
    float sx = dx * t;
    float sy = dy * t;
    float sz = t * dz;
    out->x = self->start[0] + sx;
    out->y = self->start[1] + sy;
    out->z = self->start[2] + sz;
    return out;
}

// 0x5DD010 (thiscall, unused): a cost added to the side segment 1's normal faces
// (against the tested direction)
void __fastcall AddSideCost(void*, re::Edx, float cost) {
    PaVec n;
    PaVec* normal = paSeg_Normal2D(paSeg1, &n);
    PaVec v = *normal;
    paNormalise(&v);
    float dot = paDir.x * v.x + paDir.y * v.y + paDir.z * v.z;
    cost = std::fabs(dot) + cost;
    if (0.0f < dot) {
        paRightCost = paRightCost + cost;
    } else {
        paLeftCost = paLeftCost + cost;
    }
}

// 0x5DD0C0 (thiscall, unused): where along segment 1 a circle of `radius` around the
// point is first touched (as a fraction, at least 0), -1 when the segment passes
// farther than the radius; past the end, by the movement vector instead
float __stdcall TouchAlongSegment(const float* point, float radius) {
    float dist = paSeg_Distance(paSeg1, point);
    if (radius < dist) {
        return -1.0f;
    }
    PaVec closest;
    float t;
    paSeg_Closest(paSeg1, &closest, point, &t);
    float result;
    if (t < 1.0f) {
        double td = t;
        float r2 = radius * radius;
        float d2 = dist * dist;
        float s = paSqrt(std::fabs(r2 - d2));
        float length = paSeg_Length(paSeg1);
        result = float(td - double(s / length));
    } else {
        PaVec v = paMove;
        paNormalise(&v);
        float nx = -v.x;
        float a = paDir.y * nx + v.y * paDir.x + float(0.0 * double(paDir.z));
        double gy = paMove.y;
        double gx = paMove.x;
        float b = paDir.x * v.x + paDir.y * v.y + paDir.z * v.z;
        float r2 = radius * radius;
        float a2 = a * a;
        float s = paSqrt(std::fabs(r2 - a2));
        float c = b - s;
        float g = paSqrt(std::fabs(float(gx * gx + gy * gy)));
        result = c / g;
    }
    if (result < 0.0f) {
        result = 0.0f;
    }
    return result;
}

// 0x5DD2D0 (thiscall, unused): whether a circle of the ped's radius moving along
// segment 1 hits segment 2: *t where along segment 1 (0..1), *u where on segment 2
// (clamped to its ends when it's an end that's hit)
uint8_t __stdcall SweepHitsSegment(float* tOut, float* uOut) {
    float s1[2] = {paSeg1->start[0], paSeg1->start[1]};
    float e1[2] = {paSeg1->end[0], paSeg1->end[1]};
    float s2[2] = {paSeg2->start[0], paSeg2->start[1]};
    float e2[2] = {paSeg2->end[0], paSeg2->end[1]};
    float t;
    if (!paIntersect2D(s1, e1, s2, e2, &t)) {
        return 0;
    }
    if (!(t >= 0.0f)) {
        return 0;
    }
    PaVec cross;
    PaVec q = *paSeg_PointAt(paSeg1, &cross, t);
    float sine = float(std::sin(double(paSeg_AngleTo(paSeg1, paSeg2))));
    float back = paRadius / sine;
    PaVec along;
    PaVec* v = paVectorSub(&along, &q, paSeg1->start);
    float length = paSeg_Length(paSeg1);
    float flat = paSqrt(std::fabs(v->x * v->x + v->y * v->y));
    float tt = (flat - back) / length;
    *tOut = tt;
    if (tt > 1.0f) {
        return 0;
    }
    PaVec at;
    paSeg_PointAt(paSeg1, &at, tt);
    PaVec onSeg2;
    paSeg_Closest(paSeg2, &onSeg2, &at, uOut);
    if (-0.001f < *uOut && 1.001f > *uOut) {
        return 1;
    }
    float length2 = paSeg_Length(paSeg2);
    float over = !(0.0f > *uOut) ? float(double(*uOut) - 1.0) : 0.0f - *uOut;
    over = std::fabs(over * length2);
    if (paRadius < over) {
        return 0;
    }
    const float* end = 0.0f < *uOut ? paSeg2->end : paSeg2->start;
    PaVec e = {end[0], end[1], end[2]};
    PaVec normal;
    PaVec n;
    paNormalise2D(&n, paSeg_Normal2D(paSeg1, &normal));
    PaVec de = {e.x - paSeg1->start[0], e.y - paSeg1->start[1], e.z - paSeg1->start[2]};
    float h = std::fabs(n.y * de.y + n.x * de.x + n.z * de.z);
    if (paRadius < h) {
        return 0;
    }
    PaVec d = {paSeg1->end[0] - paSeg1->start[0], paSeg1->end[1] - paSeg1->start[1], paSeg1->end[2] - paSeg1->start[2]};
    PaVec unit;
    paNormalise2D(&unit, &d);
    double rd = paRadius;
    double hd = h;
    float dd = paDot(&unit, &de);
    float r2 = float(rd * rd);
    float h2 = float(hd * hd);
    float t2 = (dd - paSqrtAbs(r2 - h2)) / length;
    *tOut = t2;
    if (t2 > 1.0f) {
        return 0;
    }
    if (0.0f < *uOut) {
        *uOut = 1.0f;
    } else {
        *uOut = 0.0f;
    }
    return 1;
}

// ---- choosing a side

GAME_VAR(uint32_t, paTimeInMs, 0x00C1A9B4);
inline float* const paOutDir = reinterpret_cast<float*>(0x00D09530);  // the chosen direction (+ a w)
GAME_VAR(PaVec, paFallbackDir, 0x00D09550);
GAME_VAR(float, paSideBias, 0x00D09560);
GAME_VAR(float, paBlockage, 0x00D09628);
GAME_FN(void, __cdecl, paSmoothTowards, 0x004331C0, float* value, float* speed, float target, float maxSpeed, float accel);
inline int __cdecl paRand() { return crt::Rand(); }
GAME_FN(uint8_t, __thiscall, paIsWayClear, 0x005DCC80, void* self, const PaVec* dir, float dist);

// The chosen direction set (its w is stack the game never sets: left as it was here)
void paSetOutDir(const PaVec& v) {
    paOutDir[0] = v.x;
    paOutDir[1] = v.y;
    paOutDir[2] = v.z;
}

// Towards one side's widest free heading: smoothed (the record's +0xC heading, +0x10
// its speed) when asked and the record's flag 2 is set, else straight
void paSteer(uint8_t* self, int smooth, float angle, float maxSpeed, const PaVec& straight) {
    if (re::Flag(smooth) && (paAt<uint8_t>(self, 0x6D) & 2) != 0) {
        paSmoothTowards(&paAt<float>(self, 0xC), &paAt<float>(self, 0x10), angle + paHeading, maxSpeed, 0.01f);
        PaVec v;
        paFromHeading(&v, paAt<float>(self, 0xC));
        paSetOutDir(v);
    } else {
        PaVec v = straight;
        paNormalise(&v);
        paSetOutDir(v);
    }
}

// 0x5DD6B0 (on the avoidance record: +0 the ped, +0xC/+0x10 a smoothed heading and its
// speed, +0x14 reset it, +0x6D flags (0x20 going right, 0x40 no side), +0x70 when the
// side last changed, +0x74 / +0x78 until when the left / right side is out): which way
// round the obstacles, from the costs of each side and whether its way is clear; with
// no side free, the fallback direction when it's blocked enough, else the ped stops
// avoiding (+0x1544)
void __fastcall ChooseSide(uint8_t* self, re::Edx, int smooth) {
    PaVec vecMin;
    paFromHeading(&vecMin, paMinAngle + paHeading);
    PaVec vecMax;
    paFromHeading(&vecMax, paMaxAngle + paHeading);
    uint32_t now = paTimeInMs;
    bool rightOut = now < paAt<uint32_t>(self, 0x78);
    bool leftOut = now < paAt<uint32_t>(self, 0x74);
    uint8_t* ped = paAt<uint8_t*>(self, 0);
    if (paMaxAngle == 0.0f || paMinAngle == 0.0f) {
        if (!rightOut || !leftOut) {
            paAt<uint8_t>(self, 0x6D) &= 0xBF;
            paAt<uint8_t>(self, 0x14) = 1;
        }
    } else {
        float a = std::fabs(paMaxAngle);
        paLeftCost = a + paLeftCost;
        a = std::fabs(paMinAngle);
        paRightCost = a + paRightCost;
        float pedHeading;
        uint8_t* matrix = paAt<uint8_t*>(ped, 0x14);
        if (matrix != nullptr) {
            pedHeading = float(std::atan2(-double(paAt<float>(matrix, 0x10)), double(paAt<float>(matrix, 0x14))));
        } else {
            pedHeading = paAt<float>(ped, 0x10);
        }
        float rel = paAngleDiff(paHeading, pedHeading);
        float a1 = paAngleDiff(rel, paMaxAngle);
        float a2 = paAngleDiff(rel, paMinAngle);
        a1 = std::fabs(a1);
        paLeftCost = float(double(a1) * 1.5 + double(paLeftCost));
        a2 = std::fabs(a2);
        paRightCost = float(1.5 * double(a2) + double(paRightCost));
        if (rightOut && leftOut) {
            paAt<uint8_t>(self, 0x6D) |= 0x40;
        } else {
            paAt<uint8_t>(self, 0x6D) &= 0xBF;
            uint8_t flags = paAt<uint8_t>(self, 0x6D);
            float limit = paSideBias == 0.0f ? 200.0f : float(2.0 / double(paSideBias));
            bool right = ((flags >> 5) & 1) != 0;
            if ((right && !leftOut) || (!right && !rightOut)) {
                float r = paRightCost;
                float l = paLeftCost;
                bool change = now > paAt<uint32_t>(self, 0x70) + 1000;
                if (!change) {
                    float d = std::fabs(r - l);
                    change = limit < d;
                }
                if (change) {
                    if (l > r && !rightOut) {
                        paAt<uint8_t>(self, 0x6D) = uint8_t(flags | 0x20);
                    } else if (!leftOut) {
                        paAt<uint8_t>(self, 0x6D) = uint8_t(flags & 0xDF);
                    } else {
                        paAt<uint8_t>(self, 0x6D) = uint8_t(flags | 0x40);
                    }
                    paAt<uint32_t>(self, 0x70) = now;
                }
            }
            if (paAt<uint8_t>(self, 0x14) != 0) {
                paAt<float>(self, 0x10) = 0.0f;
                paAt<uint8_t>(self, 0x14) = 0;
                paAt<float>(self, 0xC) = pedHeading;
            }
            if ((paAt<uint8_t>(self, 0x6D) & 0x40) == 0) {
                for (;;) {
                    if ((paAt<uint8_t>(self, 0x6D) & 0x20) != 0) {
                        if (!rightOut && paIsWayClear(self, &vecMin, paMinDist)) {
                            paSteer(self, smooth, paMinAngle, -50.0f, vecMin);
                            break;
                        }
                        int r2 = paRand();
                        paAt<uint8_t>(self, 0x6D) &= 0xDF;
                        paAt<uint32_t>(self, 0x78) = uint32_t(r2 * 1999 / 0x7FFF) + now + 2000;
                        rightOut = true;
                        if (leftOut) {
                            paAt<uint8_t>(self, 0x6D) |= 0x40;
                            break;
                        }
                    } else {
                        if (!leftOut && paIsWayClear(self, &vecMax, paMaxDist)) {
                            paSteer(self, smooth, paMaxAngle, 50.0f, vecMax);
                            break;
                        }
                        int r2 = paRand();
                        paAt<uint8_t>(self, 0x6D) |= 0x20;
                        paAt<uint32_t>(self, 0x74) = uint32_t(r2 * 1999 / 0x7FFF) + now + 2000;
                        leftOut = true;
                        if (rightOut) {
                            paAt<uint8_t>(self, 0x6D) |= 0x40;
                            break;
                        }
                    }
                }
            }
        }
    }
    if ((paAt<uint8_t>(self, 0x6D) & 0x40) == 0) {
        paAt<int32_t>(ped, 0x1544) = 0;
        return;
    }
    if (1.5f > paBlockage) {
        paAt<float>(self, 0x18) = 0.0f;
        paAt<int32_t>(ped, 0x1544) = 1;
        return;
    }
    PaVec v = paFallbackDir;
    paNormalise(&v);
    paSetOutDir(v);
}

// 0x5DDC90 (on the avoidance record): a point pushed out of the nearby boxes (the
// record's entries: 4 corners of 3 floats, +0x4C..+0x50 their height range, +0x54 a
// radius, +0x58 their entity; not the record's own +4): for the first box whose height
// range has the point and whose circle it's in, when the point is inside or nearer an
// edge than `radius`, *out is the point moved off the nearest edge (along its normal)
// to `radius` + 0.05 beyond it; false when none
GAME_FN(uint8_t, __thiscall, paBox_Contains, 0x005E52B0, void* box, const PaVec* point);
uint8_t __fastcall PushOutOfBoxes(uint8_t* self, re::Edx, PaVec* out, PaVec pos, float radius) {
    for (int i = 0; i < paAt<uint8_t>(self, 0x58); i++) {
        float* box = paAt<float*>(self, 0x1C + i * 4);
        uint8_t* entity = paAt<uint8_t*>(box, 0x58);
        if (entity == nullptr || entity == paAt<uint8_t*>(self, 4)) {
            continue;
        }
        float z = pos.z;
        if (!(box[0x13] < z) || !(box[0x14] > z)) {
            continue;
        }
        float* p = paEntityPos(entity);
        PaVec e = {p[0], p[1], p[2]};
        PaVec d;
        paVectorSub(&d, &e, &pos);
        float reach = box[0x15] + radius;
        float flat = d.x * d.x + d.y * d.y;
        float reach2 = reach * reach;
        if (!(reach2 > flat)) {
            continue;
        }
        uint8_t inside = 0;
        if (paBox_Contains(box, &pos)) {
            inside = 1;
        }
        float best = 3.4028235e+38f;
        PaSegment bestEdge;
        const float* corner = box;
        for (int k = 0; k < 4; k++, corner += 3) {
            int next = (k + 1) % 4;
            // (the edge's w are stack the game never sets: zero here)
            PaSegment edge = {{corner[0], corner[1], 0.0f, 0.0f}, {box[next * 3], box[next * 3 + 1], 0.0f, 0.0f}};
            float dist = paSeg_Distance(&edge, &pos);
            if (best > dist) {
                best = dist;
                bestEdge = edge;
            }
        }
        if (!inside && !(radius > best)) {
            continue;
        }
        PaVec normal;
        PaVec n = *paSeg_Normal2D(&bestEdge, &normal);
        paNormalise(&n);
        float t = inside ? best + radius : radius - best;
        t = float(double(t) + double(0.05f));
        PaVec step = {n.x * t, n.y * t, t * n.z};
        PaVec* moved = paVectorAdd(&n, &pos, &step);
        out->x = moved->x;
        out->y = moved->y;
        out->z = moved->z;
        return 1;
    }
    return 0;
}

// 0x5DDFA0 (thiscall, passed on): the nodes of a ring (+0x5C the next; +0x44 its
// position (x, y), +0x54 its cost, corners (x, y) at +0, +0xC, +0x18, +0x24), from the
// one after `first` round to it: each becomes the obstacle (on the ground plane), its
// side cost is added, and when it's ahead within segment 1's length its corners are
// avoided. (The w values the game copies from unset stack are zero here.)
void __fastcall AvoidRing(void* self, re::Edx, uint8_t* first) {
    float dx0 = paSeg1->start[0] - paSeg1->end[0];
    float dy0 = paSeg1->start[1] - paSeg1->end[1];
    float dz0 = paSeg1->start[2] - paSeg1->end[2];
    float length = paSqrt(std::fabs(dx0 * dx0 + dy0 * dy0 + dz0 * dz0));
    PaVec along = {paSeg1->end[0] - paSeg1->start[0], paSeg1->end[1] - paSeg1->start[1], paSeg1->end[2] - paSeg1->start[2]};
    paNormalise(&along);
    uint8_t* node = first;
    do {
        node = paAt<uint8_t*>(node, 0x5C);
        float x = paAt<float>(node, 0x44);
        float y = paAt<float>(node, 0x48);
        paObstaclePos[0] = x;
        paObstaclePos[1] = y;
        paObstaclePos[2] = 0.0f;
        paObstaclePos[3] = 0.0f;
        paDir.x = x - paPos.x;
        paDir.y = y - paPos.y;
        *reinterpret_cast<float*>(0x00D09594) = 0.0f;
        paDir.z = 0.0f - paPos.z;
        float ahead = paDir.y * along.y + paDir.x * along.x + paDir.z * along.z;
        paAddSideCost(self, paAt<float>(node, 0x54));
        if (length > ahead) {
            const float* n = reinterpret_cast<const float*>(node);
            float points[16] = {n[0], n[1], 0.0f, 0.0f, n[3], n[4], 0.0f, 0.0f, n[6], n[7], 0.0f, 0.0f, n[9], n[10], 0.0f, 0.0f};
            paAvoidPoints(self, points, 4);
        }
    } while (node != first);
}

// 0x5DE2A0 (on the avoidance record: +0x5C entities, +0x6C their count): the collision
// boxes of the nearby entities (within 10, with +0x184) whose circle reaches segment
// 1: when the ped's swept circle hits one of a box's edges facing it, its corners are
// avoided and the record's flag 0x10 set. (The corners' w, and the w of segment 2's
// ends the game copies from unset stack, are left alone / zero here.)
GAME_FN(float*, __thiscall, paCollisionBox_Get, 0x004EA210, void* boxes, void* entity, int index);
inline void* const paCollisionBoxes = reinterpret_cast<void*>(0x00C3B808);
GAME_FN(uint8_t, __stdcall, paSweepHitsSegment, 0x005DD2D0, float* t, float* u);
void __fastcall AvoidEntityBoxes(uint8_t* self, re::Edx) {
    for (int i = 0; i < paAt<uint8_t>(self, 0x6C); i++) {
        uint8_t* entity = paAt<uint8_t*>(self, 0x5C + i * 4);
        if (entity == nullptr || paAt<int32_t>(entity, 0x184) == 0) {
            continue;
        }
        float* pos = paEntityPos(entity);
        float px = pos[0];
        float py = pos[1];
        float pz = pos[2];
        paObstaclePos[0] = px;
        paObstaclePos[1] = py;
        paObstaclePos[2] = pz;
        paDir.x = px - paPos.x;
        paDir.y = py - paPos.y;
        paDir.z = pz - paPos.z;
        float dist = paSqrt(std::fabs(paDir.x * paDir.x + paDir.y * paDir.y + paDir.z * paDir.z));
        if (!(dist < 10.0f)) {
            continue;
        }
        uint8_t* col = paAt<uint8_t*>(paModelInfoOf(entity), 0x80);
        if (col == nullptr) {
            continue;
        }
        for (int k = 0; k < paAt<int8_t>(paAt<uint8_t*>(col, 0x38), 0x1F); k++) {
            float* box = paCollisionBox_Get(paCollisionBoxes, entity, k);
            float center[3] = {box[2], box[3], 0.0f};
            float d = paSeg_Distance(paSeg1, center);
            if (!(box[4] + paRadius > d)) {
                continue;
            }
            float corners[16] = {box[5], box[6], 0.0f, 0.0f, box[7], box[8], 0.0f, 0.0f,
                                 box[9], box[10], 0.0f, 0.0f, box[11], box[12], 0.0f, 0.0f};
            float best = 3.4028235e+38f;
            for (int j = 0; j < 4; j++) {
                const float* c = corners + j * 4;
                PaVec rel = {c[0] - paPos.x, c[1] - paPos.y, c[2] - paPos.z};
                paSeg2->start[0] = c[0];
                paSeg2->start[1] = c[1];
                paSeg2->start[2] = c[2];
                const float* n = corners + ((j + 1) % 4) * 4;
                paSeg2->end[0] = n[0];
                paSeg2->end[1] = n[1];
                paSeg2->end[2] = n[2];
                PaVec normal;
                paSeg_Normal2D(paSeg2, &normal);
                float facing = normal.y * rel.y + normal.x * rel.x + normal.z * rel.z;
                if (!(facing < 0.0f)) {
                    continue;
                }
                float t;
                float u;
                if (!paSweepHitsSegment(&t, &u)) {
                    continue;
                }
                if (best > t) {
                    if (0.0f > t) {
                        best = 0.0f;
                        break;
                    }
                    best = t;
                }
            }
            if (best < 3.4028235e+38f) {
                paAvoidPoints(self, corners, 4);
                paAt<uint8_t>(self, 0x6D) |= 0x10;
            }
        }
    }
}

// 0x5DE7C0 (thiscall, passed on): a moving obstacle: over the time until one of the two
// would have covered the gap (at most 10), segment 1 becomes the ped's movement
// relative to the obstacle's; when that brings them into touch, the time to it
// (0xD09614) and a cost added to the side the obstacle passes on; false when it doesn't.
// (The w values the game copies from unset stack are left alone here.)
GAME_VAR(PaVec, paOwnVelocity, 0x00D09550);
GAME_VAR(float, paOwnSpeed, 0x00D09564);
GAME_VAR(PaVec, paObstacleVelocity, 0x00D095A8);
GAME_VAR(float, paObstacleSpeed, 0x00D095B8);
GAME_VAR(float, paTouchTime, 0x00D09614);
GAME_FN(float, __stdcall, paTouchAlongSegment, 0x005DD0C0, const float* point, float radius);
GAME_FN(float, __stdcall, paAvoidCost, 0x005DC5A0, float speed, float angle, float dist);
uint8_t __fastcall AvoidMovingObstacle(void*, re::Edx) {
    float gapTime;
    if (paObstacleSpeed == 0.0f) {
        gapTime = 3.4028235e+38f;
    } else {
        float flat = paSqrt(std::fabs(paObstacleVelocity.x * paObstacleVelocity.x + paObstacleVelocity.y * paObstacleVelocity.y));
        gapTime = flat / paObstacleSpeed;
    }
    float ownSpeed = paOwnSpeed;
    float t = gapTime > ownSpeed ? ownSpeed : gapTime;
    if (!(t < 10.0f)) {
        t = 10.0f;
    }
    PaVec a = paOwnVelocity;
    PaVec b = paObstacleVelocity;
    if (ownSpeed != 0.0f) {
        float k = t / ownSpeed;
        a.x = a.x * k;
        a.y = a.y * k;
        a.z = k * a.z;
    }
    if (gapTime != 0.0f) {
        float k = t / gapTime;
        b.x = b.x * k;
        b.y = b.y * k;
        b.z = k * b.z;
    }
    paMove.x = a.x - b.x;
    paMove.y = a.y - b.y;
    paMove.z = a.z - b.z;
    float px = paPos.x;
    float py = paPos.y;
    float pz = paPos.z;
    paSeg1->start[0] = px;
    paSeg1->start[1] = py;
    paSeg1->start[2] = pz;
    paSeg1->end[0] = px + paMove.x;
    paSeg1->end[1] = py + paMove.y;
    paSeg1->end[2] = pz + paMove.z;
    float reach = paObstacleRadius + paRadius;
    float f = paTouchAlongSegment(paObstaclePos, reach);
    if (f < 0.0f) {
        return 0;
    }
    paTouchTime = f * t;
    PaVec normal;
    PaVec n = *paSeg_Normal2D(paSeg1, &normal);
    paNormalise(&n);
    float side = n.y * paDir.y + n.x * paDir.x + n.z * paDir.z;
    float cost = paAvoidCost(paTouchTime, side, paObstacleSpeed);
    if (side > 0.0f) {
        paRightCost = cost + paRightCost;
    } else {
        paLeftCost = cost + paLeftCost;
    }
    return 1;
}

// 0x5DEAF0 (thiscall, passed on): an entity's bounding box (its 8 corners by its
// matrix; the convex hull of them on the ground): when the ped's swept circle hits an
// edge of the hull facing it, the time to it (0xD09614, by the factor at 0xD09610) and
// the hull, moved on by the obstacle's velocity over that time, avoided; false when
// it doesn't. (The w values the game copies from unset stack are zero / left alone.)
GAME_FN(float*, __cdecl, paEntity_GetBoundingBox, 0x0051AF50, void* entity);
GAME_FN(void*, __thiscall, paEntity_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(void, __cdecl, paMatrix_TransformPoints, 0x00414B10, float* out, int count, void* matrix, const float* in);
GAME_FN(int, __stdcall, paConvexHull8, 0x005DC410, const float* points, int32_t* hull);
GAME_VAR(float, paTouchTimeScale, 0x00D09610);
uint8_t __fastcall AvoidEntityHull(void* self, re::Edx, void* entity) {
    float* box = paEntity_GetBoundingBox(entity);
    void* matrix = paEntity_GetMatrix(entity);
    PaVec unused;
    paSeg_Normal2D(paSeg1, &unused);
    float x0 = box[4], y0 = box[5], z0 = box[6];
    float x1 = box[8], y1 = box[9], z1 = box[10];
    float corners[32] = {x0, y0, z0, 0.0f, x1, y0, z0, 0.0f, x0, y1, z0, 0.0f, x1, y1, z0, 0.0f,
                         x0, y0, z1, 0.0f, x1, y0, z1, 0.0f, x0, y1, z1, 0.0f, x1, y1, z1, 0.0f};
    float moved[32];
    paMatrix_TransformPoints(moved, 8, matrix, corners);
    for (int i = 0; i < 8; i++) {
        corners[i * 4 + 2] = 0.0f;
    }
    int32_t hull[6] = {-1, -1, -1, -1, -1, -1};
    int count = paConvexHull8(moved, hull);
    float best = 3.4028235e+38f;
    for (int i = 0; i < count; i++) {
        const float* p = moved + hull[i] * 4;
        PaVec rel = {p[0] - paPos.x, p[1] - paPos.y, p[2] - paPos.z};
        paSeg2->start[0] = p[0];
        paSeg2->start[1] = p[1];
        paSeg2->start[2] = p[2];
        const float* n = moved + hull[(i + 1) % count] * 4;
        paSeg2->end[0] = n[0];
        paSeg2->end[1] = n[1];
        paSeg2->end[2] = n[2];
        PaVec normal;
        paSeg_Normal2D(paSeg2, &normal);
        float facing = normal.x * rel.x + normal.y * rel.y + normal.z * rel.z;
        if (!(facing < 0.0f)) {
            continue;
        }
        float t;
        float u;
        if (!paSweepHitsSegment(&t, &u)) {
            continue;
        }
        if (best > t) {
            if (0.0f > t) {
                best = 0.0f;
                break;
            }
            best = t;
        }
    }
    if (!(best < 3.4028235e+38f)) {
        return 0;
    }
    paTouchTime = best * paTouchTimeScale;
    PaVec v = paObstacleVelocity;
    paNormalise(&v);
    float d = paObstacleSpeed * paTouchTime;
    PaVec step = {v.x * d, v.y * d, d * v.z};
    float mx = paObstaclePos[0] + step.x;
    float my = paObstaclePos[1] + step.y;
    float mz = paObstaclePos[2] + step.z;
    float ox = mx - paObstaclePos[0];
    float oy = my - paObstaclePos[1];
    float oz = mz - paObstaclePos[2];
    for (int i = 0; i < count; i++) {
        const float* p = moved + hull[i] * 4;
        corners[i * 4 + 0] = ox + p[0];
        corners[i * 4 + 1] = p[1] + oy;
        corners[i * 4 + 2] = p[2] + oz;
        corners[i * 4 + 3] = 0.0f;
    }
    paAvoidPoints(self, corners, count);
    return 1;
}

// 0x5DF1E0 (on the avoidance record: +0 its ped, +0x5C 4 entity references, +0x6C
// their count): the nearest 4 entities of type 6 whose model passes 0x520440 within 20
// of the ped, from the world's sectors within 400 (the height difference counted twice;
// once full, one replaces the farthest found at the last replacement)
GAME_FN(uint8_t*, __cdecl, paGetEntityFromNode, 0x0044A3E0, int pool, int index);
GAME_VAR(uint32_t*, paNodeBase, 0x00C0F788);
GAME_FN(uint8_t, __thiscall, paModelInfo_520440, 0x00520440, void* modelInfo);
GAME_FN(void, __cdecl, paCleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __cdecl, paRegisterReference, 0x004657D0, void* entity, void* ref);

// A sector index from a coordinate (truncated), clamped to 0..35 as a float
int paSectorOf(float coord, double offset) {
    float f = float(int((double(coord) + offset) / 50.0 + 18.0));
    if (f > 35.0f) {
        f = 35.0f;
    }
    if (f < 0.0f) {
        f = 0.0f;
    }
    return int(f);
}

void paSetReference(re::Ptr32<uint8_t>* slot, uint8_t* entity) {
    uint8_t* old = *slot;
    if (entity != old) {
        if (old != nullptr) {
            paCleanUpOldReference(old, slot);
        }
        *slot = entity;
        paRegisterReference(entity, slot);
    }
}

void __fastcall GatherNearbyObjects(uint8_t* self, re::Edx) {
    paAt<uint8_t>(self, 0x6C) = 0;
    float* own = paEntityPos(paAt<uint8_t*>(self, 0));
    float x = own[0];
    float y = own[1];
    float z = own[2];
    int xStart = paSectorOf(x, -400.0);
    int xEnd = paSectorOf(x, 400.0);
    int yStart = paSectorOf(y, -400.0);
    int yEnd = paSectorOf(y, 400.0);
    float worst = 3.4028235e+38f;
    float dists[4];
    for (int xi = xStart; xi <= xEnd; xi++) {
        for (int yi = yStart; yi <= yEnd; yi++) {
            uint32_t* node = *reinterpret_cast<re::Ptr32<uint32_t>*>(0x00C1B17C + (xi + yi * 36) * 0x14);
            while (node != nullptr) {
                uint32_t packed = *node;
                uint8_t* entity = paGetEntityFromNode(int(packed & 0xF), int((packed >> 4) & 0x3FFF));
                node = (*node >> 18) == 0x3FFF ? nullptr : paNodeBase + (*node >> 18);
                if (entity == nullptr || (paAt<uint8_t>(entity, 0x108) & 7) != 6 || !paModelInfo_520440(paModelInfoOf(entity))) {
                    continue;
                }
                float* pos = paEntityPos(entity);
                float dx = pos[0] - x;
                float dy = pos[1] - y;
                float dz = pos[2] - z;
                float dz2 = dz * dz;
                float sum = dx * dx + dy * dy + dz2;
                float d2 = sum + dz2;
                if (!(d2 < 400.0)) {
                    continue;
                }
                uint8_t n = paAt<uint8_t>(self, 0x6C);
                if (n < 4) {
                    paSetReference(&paAt<uint8_t*>(self, 0x5C + n * 4), entity);
                    n = paAt<uint8_t>(self, 0x6C);
                    paAt<uint8_t>(self, 0x6C) = uint8_t(n + 1);
                    dists[n] = d2;
                    continue;
                }
                if (!(worst > d2)) {
                    continue;
                }
                worst = dists[0];
                int far = 0;
                if (dists[1] > worst) {
                    worst = dists[1];
                    far = 1;
                }
                if (worst < dists[2]) {
                    worst = dists[2];
                    far = 2;
                }
                if (worst < dists[3]) {
                    worst = dists[3];
                    far = 3;
                }
                if (worst > d2) {
                    paSetReference(&paAt<uint8_t*>(self, 0x5C + far * 4), entity);
                    dists[far] = d2;
                }
            }
        }
    }
}

// 0x5DF660 (on the avoidance record: +0x1C its boxes, +0x58 their count; not the
// record's own +4 entity): the nearby boxes (within 10 on the ground, the ped's height
// inside theirs, their circle reaching segment 1) whose edge facing the ped its swept
// circle hits: their side cost added, their corners avoided, the record's flag 8 set;
// a box in a group (+0x60, -1 none) has its whole ring avoided, once per group
GAME_FN(void, __thiscall, paAvoidRing, 0x005DDFA0, void* self, void* first);
void __fastcall AvoidBoxes(uint8_t* self, re::Edx) {
    uint32_t groups = 0;
    for (int i = 0; i < paAt<uint8_t>(self, 0x58); i++) {
        float* box = paAt<float*>(self, 0x1C + i * 4);
        uint8_t* entity = paAt<uint8_t*>(box, 0x58);
        if (entity == nullptr || entity == paAt<uint8_t*>(self, 4)) {
            continue;
        }
        int group = paAt<int8_t>(box, 0x60);
        if (group != -1 && (groups & (1u << (group & 0x1F))) != 0) {
            continue;
        }
        float* pos = paEntityPos(entity);
        float px = pos[0];
        float py = pos[1];
        float pz = pos[2];
        paObstaclePos[0] = px;
        paObstaclePos[1] = py;
        paObstaclePos[2] = pz;
        paDir.x = px - paPos.x;
        paDir.y = py - paPos.y;
        paDir.z = pz - paPos.z;
        float dist = paSqrt(std::fabs(paDir.x * paDir.x + paDir.y * paDir.y));
        if (!(dist < 10.0f)) {
            continue;
        }
        if (!(box[0x13] < paPos.z) || !(box[0x14] > paPos.z)) {
            continue;
        }
        float center[3] = {box[0x11], box[0x12], 0.0f};
        float d = paSeg_Distance(paSeg1, center);
        float radius = box[0x15];
        if (!(paRadius + radius > d)) {
            continue;
        }
        float corners[16] = {box[0], box[1], 0.0f, 0.0f, box[3], box[4], 0.0f, 0.0f,
                             box[6], box[7], 0.0f, 0.0f, box[9], box[10], 0.0f, 0.0f};
        float best = 3.4028235e+38f;
        for (int j = 0; j < 4; j++) {
            const float* c = corners + j * 4;
            PaVec rel = {c[0] - paPos.x, c[1] - paPos.y, c[2] - paPos.z};
            paSeg2->start[0] = c[0];
            paSeg2->start[1] = c[1];
            paSeg2->start[2] = c[2];
            const float* n = corners + ((j + 1) % 4) * 4;
            paSeg2->end[0] = n[0];
            paSeg2->end[1] = n[1];
            paSeg2->end[2] = n[2];
            PaVec normal;
            paSeg_Normal2D(paSeg2, &normal);
            float facing = normal.y * rel.y + normal.x * rel.x + normal.z * rel.z;
            if (!(facing < 0.0f)) {
                continue;
            }
            float t;
            float u;
            if (!paSweepHitsSegment(&t, &u)) {
                continue;
            }
            if (best > t) {
                if (0.0f > t) {
                    best = 0.0f;
                    break;
                }
                best = t;
            }
        }
        if (!(best < 3.4028235e+38f)) {
            continue;
        }
        paAddSideCost(self, radius);
        paAvoidPoints(self, corners, 4);
        paAt<uint8_t>(self, 0x6D) |= 8;
        if (paAt<int8_t>(box, 0x60) != -1) {
            paAvoidRing(self, box);
            groups |= 1u << (paAt<uint8_t>(box, 0x60) & 0x1F);
        }
    }
}

} // namespace

#define PA_REVERSED(name, address, fn)                   \
    static void* RE_CONCAT(paOriginal_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(paOriginal_, __LINE__), re::Mode::Original)

PA_REVERSED("PedAvoidance::TrackHeading", 0x005DC3A0, &TrackHeading);
PA_REVERSED("PedAvoidance::ConvexHull8", 0x005DC410, &ConvexHull8);
PA_REVERSED("PedAvoidance::AvoidCost", 0x005DC5A0, &AvoidCost);
PA_REVERSED("PedAvoidance::Set8", 0x005DC630, &Set8);
PA_REVERSED("PedAvoidance::AvoidCircle", 0x005DC670, &AvoidCircle);
PA_REVERSED("PedAvoidance::AvoidPoints", 0x005DC790, &AvoidPoints);
PA_REVERSED("PedAvoidance::AvoidNearLines", 0x005DC990, &AvoidNearLines);
PA_REVERSED("PedAvoidance::IsWayClear", 0x005DCC80, &IsWayClear);
PA_REVERSED("PedAvoidance::GatherNearby", 0x005DCD80, &GatherNearby);
PA_REVERSED("AvoidSegment::Length", 0x005DCF40, &Segment_Length);
PA_REVERSED("AvoidSegment::PointAt", 0x005DCFA0, &Segment_PointAt);
PA_REVERSED("PedAvoidance::AddSideCost", 0x005DD010, &AddSideCost);
PA_REVERSED("PedAvoidance::TouchAlongSegment", 0x005DD0C0, &TouchAlongSegment);
PA_REVERSED("PedAvoidance::SweepHitsSegment", 0x005DD2D0, &SweepHitsSegment);
PA_REVERSED("PedAvoidance::ChooseSide", 0x005DD6B0, &ChooseSide);
PA_REVERSED("PedAvoidance::PushOutOfBoxes", 0x005DDC90, &PushOutOfBoxes);
PA_REVERSED("PedAvoidance::AvoidRing", 0x005DDFA0, &AvoidRing);
PA_REVERSED("PedAvoidance::AvoidEntityBoxes", 0x005DE2A0, &AvoidEntityBoxes);
PA_REVERSED("PedAvoidance::AvoidMovingObstacle", 0x005DE7C0, &AvoidMovingObstacle);
PA_REVERSED("PedAvoidance::AvoidEntityHull", 0x005DEAF0, &AvoidEntityHull);
PA_REVERSED("PedAvoidance::GatherNearbyObjects", 0x005DF1E0, &GatherNearbyObjects);
PA_REVERSED("PedAvoidance::AvoidBoxes", 0x005DF660, &AvoidBoxes);
