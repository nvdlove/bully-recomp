// CGeneral: the game's angle and interpolation helpers (GTA's CGeneral where the code
// matches: GetATanOfXY, GetRadianAngleBetweenPoints, LimitRadianAngle), and the
// steppers the HUD and the cameras smooth values with. 0x432860..0x4334F0.
//
// The CRT's x87 helpers they call (sqrt 0x85C500, atan 0x85CF70, acos 0x85CE00, asin
// 0x85D460, sin 0x85AEF0, cos 0x85AD80, floor 0x85D570) take their argument in ST0 and
// are replaced by the C library's; the results are stored as floats as the game does.

#include <cmath>
#include <cstdint>

#include "reversed.h"

#define GENERAL_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

struct GmVector {
    float x, y, z;
};

GAME_VAR(uint8_t, gm_smoothingOn, 0x009F4790);   // the steppers step (else they jump)
GAME_VAR(float, gm_twoPi, 0x009F4794);           // (data: 2 pi)
GAME_VAR(float, gm_accelScale, 0x009F4798);      // (0.02: AccelerateTowards' time step scale)
GAME_VAR(float, gm_timeStep, 0x00C1A9A4);        // CTimer's time step
GAME_VAR(float, gm_bigAngle, 0x00BD3E70);        // LimitRadianAngle's threshold for the modulo route
inline void* const gm_random = reinterpret_cast<void*>(0x00CF356C);

GAME_FN(float, __thiscall, GM_Random_GetFloat, 0x00577300, void* random);
GAME_FN(uint8_t, __thiscall, GM_Random_Chance, 0x00432860, void* random, float chance);
GAME_FN(void, __cdecl, GM_StepShortTowards, 0x00432880, int16_t* out, int16_t* state, int16_t target);
GAME_FN(float, __cdecl, GM_WrapTriangle, 0x00432E70, float x, int n);
GAME_FN(float, __cdecl, GM_LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, GM_GetRadianAngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __thiscall, GM_Normalise, 0x00414E10, GmVector* v);
GAME_FN(GmVector*, __cdecl, GM_Sub, 0x00414B70, GmVector* out, const GmVector* a, const GmVector* b);

float Stored(double v) {
    return float(v);
}

} // namespace

namespace CGeneral {

// 0x432860 (thiscall, on the random generator 0xCF356C): a chance of `chance` (0..1)
uint8_t __fastcall RandomChance(void* random, re::Edx, float chance) {
    float r = GM_Random_GetFloat(random);
    return !(chance < r) && !std::isnan(chance) && !std::isnan(r);
}

// 0x432880: *out towards `target` a step at a time, at random (half the frames);
// straight there when smoothing is off, the state unset (-0x54D) or 0xFF either way
void __cdecl StepShortTowards(int16_t* out, int16_t* state, int16_t target) {
    int16_t current = *state;
    if (!gm_smoothingOn || current == -0x54D || current == target || current == 0xFF) {
        *out = target;
        *state = target;
        return;
    }
    if (target == 0xFF) {
        *out = 0xFF;
        return;
    }
    *out = current;
    if (GM_Random_Chance(gm_random, 0.5f)) {
        if (target > *state) {
            (*out)++;
            *state = *out;
            return;
        }
        (*out)--;
    }
    *state = *out;
}

// 0x432910
float __cdecl Floor(float x) {
    return float(std::floor(double(x)));
}

// 0x432930: into 0..2 pi
float __cdecl LimitAngle0To2Pi(float angle) {
    int turns = int(angle * 0.15915493866300567);
    float r = Stored(angle - double(turns) * gm_twoPi);
    if (r < 0.0f) {
        return Stored(double(r) + gm_twoPi);
    }
    return r;
}

// 0x432980: b - a, both into 0..2 pi first, the result into -pi..pi
float __cdecl AngleDifference(float a, float b) {
    float ra = Stored(a - double(int(a * 0.15915493866300567)) * gm_twoPi);
    if (ra < 0.0f) {
        ra = Stored(double(ra) + gm_twoPi);
    }
    float rb = Stored(b - double(int(b * 0.15915493866300567)) * gm_twoPi);
    if (rb < 0.0f) {
        rb = Stored(double(rb) + gm_twoPi);
    }
    float d = rb - ra;
    if (d > 3.1415927410125732) {
        d = Stored(d - 6.2831854820251465);
    }
    if (d < -3.1415927410125732) {
        return Stored(d + 6.2831854820251465);
    }
    return d;
}

// 0x432A60: |a - b| <= eps (a whole int)
int __cdecl IsNearlyEqual(float a, float b, float eps) {
    float d = std::fabs(a - b);
    return !(eps < d) && !std::isnan(d) && !std::isnan(eps) ? 1 : 0;
}

// 0x432A90: StepShortTowards on ints (the state's low 16 bits, both results sign-extended)
void __cdecl StepIntTowards(int32_t* out, int32_t* state, int32_t target) {
    int16_t o = 0;
    int16_t s = int16_t(uint16_t(*state));
    GM_StepShortTowards(&o, &s, int16_t(target));
    *out = o;
    *state = s;
}

// 0x432AD0: *out towards `target` by at most maxStep (straight there when smoothing is
// off or the state is unset, 13571113)
void __cdecl StepFloatTowards(float* out, float* state, float target, float maxStep) {
    float d = target - *state;   // (24-bit on the main thread: d and |d| compare equal when d >= 0)
    float ad = std::fabs(d);
    if (*state != 13571113.0 && gm_smoothingOn && !(maxStep >= ad)) {
        float sign = 1.0f;
        if (d < ad) {
            sign = -1.0f;
        }
        *out = maxStep * sign + *state;
        *state = *out;
        return;
    }
    *out = target;
    *state = *out;
}

// 0x432B60: y at x on the line through (x0, y0) and (x1, y1) (y0 when x0 and x1 meet)
float __cdecl LinearMap(float x, float y0, float x0, float y1, float x1) {
    double dx = double(x1) - x0;
    if (float(std::fabs(float(dx))) >= 9.999999747378752e-06f) {
        float t1 = x - x0;
        float t2 = y1 - y0;
        float q = Stored(double(t1) * t2 / dx);
        return y0 + q;
    }
    return y0 + 0.0f;
}

// 0x432BE0: the angle from a to b (0..2 pi), the direction by the side of a's
// perpendicular b is on (flipped with `flag`); pi when opposite, 0 when the same
float __cdecl AngleBetweenVectors(const GmVector* a, const GmVector* b, int flag) {
    GmVector perp = {a->y, -a->x, 0.0f};
    GM_Normalise(&perp);
    float dot = a->x * b->x + b->y * a->y + a->z * b->z;
    float side = b->y * perp.y + b->x * perp.x + b->z * perp.z;
    float result = 0.0f;
    float nearOpposite = std::fabs(float(dot - -1.0));
    if (!(9.999999747378752e-06f < nearOpposite)) {
        return 3.1415927410125732f;
    }
    float nearSame = std::fabs(float(dot - 1.0));
    if (!(9.999999747378752e-06f < nearSame)) {
        return result;
    }
    result = float(std::acos(double(dot)));
    bool flip = re::Flag(flag);
    if (side >= 0.0f) {
        return flip ? result : Stored(6.2831854820251465 - result);
    }
    return flip ? Stored(6.2831854820251465 - result) : result;
}

// 0x432D10: GTA's GetRadianAngleBetweenPoints (dx == 0 with dy > 0 gives -pi)
float __cdecl GetRadianAngleBetweenPoints(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    if (dy == 0.0f) {
        return 0.0f < dx ? 1.5707963705062866f : -1.5707963705062866f;
    }
    if (dy < 0.0f) {
        float a = float(std::atan(double(float(double(dx) / dy))));
        return -a;
    }
    float q = float(double(dx) / dy);
    float a = float(std::atan(double(q)));
    if (dx > 0.0f) {
        return Stored(3.1415927410125732 - a);
    }
    return Stored(-3.1415927410125732 - a);
}

// 0x432DF0: a heading's direction (-sin, cos, 0)
GmVector* __cdecl HeadingToVector(GmVector* out, float heading) {
    float s = float(std::sin(double(heading)));
    out->x = -s;
    out->y = float(std::cos(double(heading)));
    out->z = 0.0f;
    return out;
}

// 0x432E30: (cos, sin, 0)
GmVector* __cdecl AngleToVector(GmVector* out, float angle) {
    out->x = float(std::cos(double(angle)));
    out->y = float(std::sin(double(angle)));
    out->z = 0.0f;
    return out;
}

// 0x432E70: x folded into -n..n (a sawtooth of period 2n), the sign kept
float __cdecl WrapTriangle(float x, int n) {
    float ax = std::fabs(x);
    int whole = int(ax);
    float frac = float(double(ax) - whole);
    int r = whole % (n * 2);
    float y;
    if (r > n) {
        y = Stored(double(r - n * 2) - frac);
    } else {
        y = Stored(double(r) + frac);
    }
    if (x < 0.0f) {
        y = -y;
    }
    return y;
}

// 0x432EE0: GTA's LimitRadianAngle; big angles (0xBD3E70 and up) through degrees and
// WrapTriangle instead of the loops
float __cdecl LimitRadianAngle(float angle) {
    float a = std::fabs(angle);
    if (!(gm_bigAngle > a)) {
        float degrees = Stored(angle * 180.0 / 3.1415927410125732);
        return Stored(GM_WrapTriangle(degrees, 180) * 3.1415927410125732 / 180.0);
    }
    while (angle > 3.1415927410125732) {
        angle = Stored(double(angle) - gm_twoPi);
    }
    while (angle < -3.1415927410125732) {
        angle = Stored(double(angle) + gm_twoPi);
    }
    return angle;
}

// 0x432FB0: into 0..2 pi through LimitRadianAngle
float __cdecl LimitRadianAngle0To2Pi(float angle) {
    return Stored(GM_LimitRadianAngle(Stored(angle - 3.1415927410125732)) + 3.1415927410125732);
}

// 0x432FE0: GTA's GetATanOfXY (0..2 pi, 0 for the zero vector)
float __cdecl GetATanOfXY(float x, float y) {
    if (x == 0.0f && y == 0.0f) {
        return 0.0f;
    }
    float xabs = std::fabs(x);
    float yabs = std::fabs(y);
    if (xabs < yabs) {
        if (y > 0.0f) {
            if (x > 0.0f) {
                return Stored(1.5707963705062866 - float(std::atan(double(float(double(x) / y)))));
            }
            return Stored(float(std::atan(double(float(-double(x) / y)))) + 1.5707963705062866);
        }
        if (x > 0.0f) {
            return Stored(float(std::atan(double(float(double(x) / -y)))) + 4.71238911151886);
        }
        return Stored(4.71238911151886 - float(std::atan(double(float(double(x) / y)))));
    }
    if (y > 0.0f) {
        if (x > 0.0f) {
            return float(std::atan(double(float(double(y) / x))));
        }
        return Stored(3.1415927410125732 - float(std::atan(double(float(double(y) / -x)))));
    }
    if (x > 0.0f) {
        return Stored(6.2831854820251465 - float(std::atan(double(float(-double(y) / x)))));
    }
    return Stored(float(std::atan(double(float(double(y) / x)))) + 3.1415927410125732);
}

// 0x433190: GetRadianAngleBetweenPoints from the origin to (v.x, v.y)
float __cdecl GetAngleOfVector2D(const GmVector* v) {
    return GM_GetRadianAngleBetweenPoints(0.0f, 0.0f, v->x, v->y);
}

// 0x4331C0: *value towards `target` with a velocity accelerated by `accel` (a time
// step of 0.02 x CTimer's): the velocity zeroed when it points away, the step capped
// at the distance, and there at once within eps. (The last test compares the step,
// not the new value, with the target.)
void __cdecl AccelerateTowards(float* value, float* velocity, float target, float accel, float eps) {
    float d = target - *value;
    float dt = gm_accelScale * gm_timeStep;
    if (d < 0.0f) {
        accel = -accel;
    }
    float ad = std::fabs(float(d - 0.0));
    if (eps >= ad) {
        *value = target;
        *velocity = 0.0f;
        return;
    }
    int dirD = d < 0.0f ? -1 : 1;
    int dirV = *velocity < 0.0f ? -1 : 1;
    if (dirD != dirV) {
        *velocity = 0.0f;
    }
    float dt2 = dt * dt;
    float step = Stored(double(*velocity) * dt + 0.5 * accel * dt2);
    *velocity = Stored(double(accel) * dt + *velocity);
    float absD = std::fabs(d);
    if (std::fabs(step) > absD) {
        step = d;
    }
    float off = std::fabs(step - target);
    if (off < eps || off == eps) {
        *value = target;
        return;
    }
    *value = step + *value;
}

// 0x433310: the heading of a vector (LimitRadianAngle of the angle from it to the origin)
float __cdecl GetHeadingOfVector(const GmVector* v) {
    return GM_LimitRadianAngle(GM_GetRadianAngleBetweenPoints(v->x, v->y, 0.0f, 0.0f));
}

// 0x433350: the heading from a to b
float __cdecl GetHeadingFromTo(const GmVector* a, const GmVector* b) {
    GmVector d;
    GmVector* diff = GM_Sub(&d, b, a);
    return GM_LimitRadianAngle(GM_GetRadianAngleBetweenPoints(diff->x, diff->y, 0.0f, 0.0f));
}

// 0x4333A0: an angle's heading (through its cos and sin)
float __cdecl AngleToHeading(float angle) {
    float c = float(std::cos(double(angle)));
    float s = float(std::sin(double(angle)));
    return GM_LimitRadianAngle(GM_GetRadianAngleBetweenPoints(c, s, 0.0f, 0.0f));
}

// 0x433400: a x^2 + b x + c = 0's roots (false: none)
uint8_t __cdecl SolveQuadratic(float a, float b, float c, float* root1, float* root2) {
    float disc = Stored(double(b) * b - a * 4.0 * c);
    if (disc < 0.0f) {
        return 0;
    }
    float q = std::sqrt(std::fabs(disc));
    *root2 = Stored((double(q) - b) * 0.5 / a);
    *root1 = Stored((-double(b) - q) * 0.5 / a);
    return 1;
}

// 0x433490: a vector's pitch (asin of z over its length; 0 for the zero vector)
float __cdecl GetPitchOfVector(const GmVector* v) {
    float sq = v->y * v->y + v->x * v->x + v->z * v->z;
    float len = std::sqrt(std::fabs(sq));
    if (len == 0.0f) {
        return 0.0f;
    }
    return float(std::asin(double(float(double(v->z) / len))));
}

GENERAL_REVERSED("CGeneral::RandomChance", 0x00432860, &RandomChance);
GENERAL_REVERSED("CGeneral::StepShortTowards", 0x00432880, &StepShortTowards);
GENERAL_REVERSED("CGeneral::Floor", 0x00432910, &Floor);
GENERAL_REVERSED("CGeneral::LimitAngle0To2Pi", 0x00432930, &LimitAngle0To2Pi);
GENERAL_REVERSED("CGeneral::AngleDifference", 0x00432980, &AngleDifference);
GENERAL_REVERSED("CGeneral::IsNearlyEqual", 0x00432A60, &IsNearlyEqual);
GENERAL_REVERSED("CGeneral::StepIntTowards", 0x00432A90, &StepIntTowards);
GENERAL_REVERSED("CGeneral::StepFloatTowards", 0x00432AD0, &StepFloatTowards);
GENERAL_REVERSED("CGeneral::LinearMap", 0x00432B60, &LinearMap);
GENERAL_REVERSED("CGeneral::AngleBetweenVectors", 0x00432BE0, &AngleBetweenVectors);
GENERAL_REVERSED("CGeneral::GetRadianAngleBetweenPoints", 0x00432D10, &GetRadianAngleBetweenPoints);
GENERAL_REVERSED("CGeneral::HeadingToVector", 0x00432DF0, &HeadingToVector);
GENERAL_REVERSED("CGeneral::AngleToVector", 0x00432E30, &AngleToVector);
GENERAL_REVERSED("CGeneral::WrapTriangle", 0x00432E70, &WrapTriangle);
GENERAL_REVERSED("CGeneral::LimitRadianAngle", 0x00432EE0, &LimitRadianAngle);
GENERAL_REVERSED("CGeneral::LimitRadianAngle0To2Pi", 0x00432FB0, &LimitRadianAngle0To2Pi);
GENERAL_REVERSED("CGeneral::GetATanOfXY", 0x00432FE0, &GetATanOfXY);
GENERAL_REVERSED("CGeneral::GetAngleOfVector2D", 0x00433190, &GetAngleOfVector2D);
GENERAL_REVERSED("CGeneral::AccelerateTowards", 0x004331C0, &AccelerateTowards);
GENERAL_REVERSED("CGeneral::GetHeadingOfVector", 0x00433310, &GetHeadingOfVector);
GENERAL_REVERSED("CGeneral::GetHeadingFromTo", 0x00433350, &GetHeadingFromTo);
GENERAL_REVERSED("CGeneral::AngleToHeading", 0x004333A0, &AngleToHeading);
GENERAL_REVERSED("CGeneral::SolveQuadratic", 0x00433400, &SolveQuadratic);
GENERAL_REVERSED("CGeneral::GetPitchOfVector", 0x00433490, &GetPitchOfVector);

} // namespace CGeneral
