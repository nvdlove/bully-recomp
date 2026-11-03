// The dodgeball minigame's camera controller, CDodgeballCameraCtrl (0x678C90..0x67A1D5; the
// instance at *0xC3CEC0): it frames the players and the ball its view flags (+0x34, set by
// DodgeballCameraViewTrack) pick, looking across the court from its side, and springs
// towards that view. The camera tuning is at *0x20C8888 (+0x18 / +0x1C the nearest and
// furthest distance back, +0x20 / +0x24 the camera's and target's height, +0x28 the
// framing margin, +0x2C the pitch in degrees, +0x30 the spring, +0x34 the damping, +0x38
// the near clip, +0x3C the fov). The court (DodgeballGame +0x17D4): +0x9C its centre,
// +0xA8 its across axis, +0xB4 its along axis.
//
// The controller (0x38): +0 vtable, +4 position, +0x10 its velocity, +0x1C target, +0x28
// the target's velocity, +0x34 view flags (1 the court's centre, 2 / 4 / 8 the first team's
// players flagged 2 in play, not flagged 2 in play, neither, 0x10 / 0x20 / 0x40 the
// second team's, 0x80 the ball).

#include <cmath>
#include <cstdint>

#include "game/TrackMath.h"
#include "reversed.h"

namespace {

using re::At;

GAME_VAR(uint8_t*, dodgeballGame, 0x01B89150);
GAME_VAR(uint8_t*, cameraTuning_20C8888, 0x020C8888);
GAME_VAR(void*, camera_C3CCF4, 0x00C3CCF4);              // TheCamera's NiCamera
inline uint8_t* const theCamera = reinterpret_cast<uint8_t*>(0x00C3CC68);
GAME_VAR(float, cameraFov_C3CD04, 0x00C3CD04);           // TheCamera +0x9C
GAME_VAR(float, cameraFovOverride_C3CDD0, 0x00C3CDD0);   // TheCamera +0x168
GAME_VAR(int32_t, cameraFovOverridden_C3CDD4, 0x00C3CDD4);   // TheCamera +0x16C
GAME_ARRAY(float, cameraOffset_C3CDE8, 0x00C3CDE8);      // TheCamera +0x180
GAME_ARRAY(float, cameraOffset_C3CDF4, 0x00C3CDF4);      // TheCamera +0x18C
GAME_VAR(uint8_t, cameraFovLocked_C3CF82, 0x00C3CF82);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(float, springRate_B9D944, 0x00B9D944);
GAME_ARRAY(float, upVector_9E33DC, 0x009E33DC);

constexpr uint32_t kVtDodgeballCameraCtrl = 0x0093E490;

GAME_FN(uint8_t*, __thiscall, Dodgeball_PlayerPed, 0x0067B040, uint8_t* game, int i);
GAME_FN(uint8_t*, __thiscall, Dodgeball_Player, 0x0067B1F0, uint8_t* game, int i);
GAME_FN(uint8_t*, __thiscall, Dodgeball_GetBall, 0x0067D780, uint8_t* game);
GAME_FN(void*, __cdecl, Camera_GetNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, Node_WorldTransform, 0x006C9A90, void* node);
GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, CrossProduct, 0x00414730, float* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, float* v);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, void* targeting);
GAME_FN(void, __thiscall, Camera_LookAtPoint, 0x004F2C60, void* camera, const float* position, const float* target);
GAME_FN(void, __thiscall, Camera_SetOffset180, 0x004F0920, void* camera, const float* offset);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float plane);
// the point three planes (normal, distance) meet
GAME_FN(void, __thiscall, Plane_Intersect3, 0x00683C40, const float* self, const float* a, const float* b, float* out);

// ours, through the game's addresses
GAME_FN(int, __thiscall, Camera_GatherPointsFn, 0x00678D90, uint8_t* self, float* points, int max);
GAME_FN(void, __thiscall, Camera_FitViewFn, 0x00679090, uint8_t* self, float* position, float lookX, float lookY,
        float lookZ, const float* points, int count);
GAME_FN(void, __thiscall, Camera_MoveFn, 0x00679690, uint8_t* self, float px, float py, float pz, float tx, float ty,
        float tz);
GAME_FN(void, __thiscall, Camera_ComputeFn, 0x006798E0, uint8_t* self, float* position, float* target);

inline void Copy3(float* dst, const float* src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}
inline float* Pos(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}

// 0x678C90 (thiscall)
uint8_t* __fastcall DodgeballCamera_Ctor(uint8_t* self, re::Edx) {
    for (int at = 4; at <= 0x30; at += 4) {
        At<float>(self, at) = 0.0f;
    }
    At<uint32_t>(self, 0) = kVtDodgeballCameraCtrl;
    return self;
}

// 0x678CC0 (thiscall): vf4, started from the camera's view (its target 2 ahead) and its
// velocities
void __fastcall DodgeballCamera_Start(uint8_t* self, re::Edx) {
    float* m = Node_WorldTransform(Camera_GetNode(camera_C3CCF4));
    float* position = &At<float>(self, 4);
    position[0] = m[9];
    position[1] = m[10];
    position[2] = m[11];
    float forward[3] = {m[2], m[5], m[8]};
    float ahead[3] = {forward[0] * 2.0f, forward[1] * 2.0f, 2.0f * forward[2]};
    Copy3(&At<float>(self, 0x1C), VectorAdd(forward, position, ahead));
    Copy3(&At<float>(self, 0x10), cameraOffset_C3CDE8);
    Copy3(&At<float>(self, 0x28), cameraOffset_C3CDF4);
}

// the players of a team (from `first`) whose flags pass, their positions appended
template <typename Pass>
inline int GatherPlayers(float* points, int n, int first, Pass pass) {
    for (int i = first; i < first + 4; i++) {
        uint32_t flags = At<uint32_t>(Dodgeball_Player(dodgeballGame, i), 4);
        if (!pass(flags)) {
            continue;
        }
        Copy3(points + n * 3, Pos(Dodgeball_PlayerPed(dodgeballGame, i)));
        n++;
    }
    return n;
}

// 0x678D90 (thiscall): the points its view flags pick; their number (the maximum unused)
int __fastcall DodgeballCamera_GatherPoints(uint8_t* self, re::Edx, float* points, int) {
    int n = 0;
    if (self[0x34] & 1) {
        Copy3(points, &At<float>(At<uint8_t*>(dodgeballGame, 0x17D4), 0x9C));
        n = 1;
    }
    auto inPlay2 = [](uint32_t f) { return (f & 2) && (f & 1); };
    auto inPlay = [](uint32_t f) { return !(f & 2) && (f & 1); };
    auto out = [](uint32_t f) { return (f & 3) == 0; };
    if (self[0x34] & 2) {
        n = GatherPlayers(points, n, 0, inPlay2);
    }
    if (self[0x34] & 4) {
        n = GatherPlayers(points, n, 0, inPlay);
    }
    if (self[0x34] & 8) {
        n = GatherPlayers(points, n, 0, out);
    }
    if (self[0x34] & 0x10) {
        n = GatherPlayers(points, n, 4, inPlay2);
    }
    if (self[0x34] & 0x20) {
        n = GatherPlayers(points, n, 4, inPlay);
    }
    if (self[0x34] & 0x40) {
        n = GatherPlayers(points, n, 4, out);
    }
    if (self[0x34] & 0x80) {
        uint8_t* ball = Dodgeball_GetBall(dodgeballGame);
        if (ball) {
            Copy3(points + n * 3, Pos(ball));
            n++;
        }
    }
    return n;
}

// 0x679090 (thiscall): the camera position framing the points looking along `look`: the
// view's two side planes (at half the fov either side, pitched by look x up) pushed out to
// keep every point the tuning's margin inside, met with the plane through the position
// across the view
void __fastcall DodgeballCamera_FitView(uint8_t*, re::Edx, float* position, float lookX, float lookY, float lookZ,
                                        const float* points, int count) {
    float fov = cameraFovOverridden_C3CDD4 ? cameraFovOverride_C3CDD0 : cameraFov_C3CD04;
    float half = float(float(double(fov) * 0.017453293005625408) * 0.5);
    float c = trk::CrtCos(half);
    float s = trk::CrtSin(half);
    float look[3] = {lookX, lookY, lookZ};
    float up[3] = {0.0f, 0.0f, 1.0f};
    float side[3];
    float tmp[3];
    Copy3(side, CrossProduct(tmp, look, up));
    Vector_Normalise(side);
    float ls[3] = {look[0] * s, look[1] * s, s * look[2]};
    float sc[3] = {side[0] * c, side[1] * c, c * side[2]};
    float n1[3];
    VectorSub(n1, sc, ls);
    float ls2[3] = {look[0] * s, look[1] * s, s * look[2]};
    float negC = -c;
    float m[3] = {side[0] * negC, side[1] * negC, negC * side[2]};
    float n2[3];
    VectorSub(n2, m, ls2);
    float plane1[4] = {n1[0], n1[1], n1[2], 0.0f};
    float plane2[4] = {n2[0], n2[1], n2[2], 0.0f};
    float dot1 = (position[0] * n1[0] + position[1] * n1[1]) + position[2] * n1[2];
    plane1[3] = dot1 - 0.0f;
    float dot2 = (position[0] * n2[0] + position[1] * n2[1]) + position[2] * n2[2];
    plane2[3] = dot2 - 0.0f;
    float axis[3];
    CrossProduct(axis, side, look);
    float plane3[4] = {axis[0], axis[1], axis[2], 0.0f};
    float dot3 = (position[0] * axis[0] + position[1] * axis[1]) + position[2] * axis[2];
    plane3[3] = dot3 - 0.0f;
    float margin = At<float>(cameraTuning_20C8888, 0x28);
    for (int i = 0; i < count; i++) {
        const float* p = points + i * 3;
        float d1 = (p[1] * plane1[1] + p[0] * plane1[0]) + p[2] * plane1[2];
        if (d1 - plane1[3] < margin) {
            plane1[3] = (d1 - 0.0f) - margin;
        }
        float d2 = (p[1] * plane2[1] + p[0] * plane2[0]) + p[2] * plane2[2];
        if (d2 - plane2[3] < margin) {
            plane2[3] = (d2 - 0.0f) - margin;
        }
    }
    Plane_Intersect3(plane3, plane1, plane2, position);
}

// 0x679690 (thiscall): the position and target sprung towards the wanted ones (their
// velocities: the spring times 0.02 s x the time step, x1.3 without a lock-on, then damped)
void __fastcall DodgeballCamera_Move(uint8_t* self, re::Edx, float px, float py, float pz, float tx, float ty, float tz) {
    float k = springRate_B9D944 * timeStep;
    float f = Targeting_GetTarget(At<uint8_t*>(playerPed, 0x1C8)) ? 1.0f : 1.3f;
    float spring = At<float>(cameraTuning_20C8888, 0x30);
    float* position = &At<float>(self, 4);
    float* target = &At<float>(self, 0x1C);
    float wantedPosition[3] = {px, py, pz};
    float d[3];
    VectorSub(d, wantedPosition, position);
    float a[3] = {d[0] * spring, d[1] * spring, spring * d[2]};
    float b[3] = {a[0] * f, a[1] * f, f * a[2]};
    float spring2 = At<float>(cameraTuning_20C8888, 0x30);
    float wantedTarget[3] = {tx, ty, tz};
    float e[3];
    VectorSub(e, wantedTarget, target);
    float g[3] = {e[0] * spring2, e[1] * spring2, spring2 * e[2]};
    float h[3] = {g[0] * f, g[1] * f, f * g[2]};
    float* velocity = &At<float>(self, 0x10);
    float i3[3] = {b[0] * k, b[1] * k, b[2] * k};
    velocity[0] = velocity[0] + i3[0];
    velocity[1] = i3[1] + velocity[1];
    velocity[2] = velocity[2] + i3[2];
    float* velocity2 = &At<float>(self, 0x28);
    float j3[3] = {h[0] * k, h[1] * k, k * h[2]};
    velocity2[0] = velocity2[0] + j3[0];
    velocity2[1] = velocity2[1] + j3[1];
    velocity2[2] = velocity2[2] + j3[2];
    float damping = At<float>(cameraTuning_20C8888, 0x34);
    float v1[3] = {velocity[0] * damping, velocity[1] * damping, damping * velocity[2]};
    Copy3(velocity, v1);
    float damping2 = At<float>(cameraTuning_20C8888, 0x34);
    float v2[3] = {velocity2[0] * damping2, velocity2[1] * damping2, damping2 * velocity2[2]};
    Copy3(velocity2, v2);
    float newPosition[3];
    VectorAdd(newPosition, position, velocity);
    float tmp[3];
    Copy3(target, VectorAdd(tmp, target, velocity2));
    Copy3(position, newPosition);
}

// a value kept at most `limit` (the limit on ties; NaN: the limit)
inline float AtMost(float v, float limit) {
    return (limit < v || limit == v || limit != limit || v != v) ? limit : v;
}
// kept at least `limit` (the limit on ties; NaN: the limit)
inline float AtLeast(float v, float limit) {
    return (limit < v) ? v : limit;
}

// 0x6798E0 (thiscall): the wanted view: the points' extent across (within 4 past the
// court's sides) and along (within 10 past its ends) centred (the court's centre without
// points), looked at from the side furthest out: from 30 out across, pitched by the
// tuning's angle, framed by FitView; then its distance back and sideways offset clamped,
// raised with the height (up to 6)
void __fastcall DodgeballCamera_Compute(uint8_t* self, re::Edx, float* position, float* target) {
    uint8_t* game = dodgeballGame;
    if (!game) {
        return;
    }
    uint8_t* court = At<uint8_t*>(game, 0x17D4);
    float* u = &At<float>(court, 0xA8);
    float* v = &At<float>(court, 0xB4);
    float* origin = &At<float>(court, 0x9C);
    float points[20][3];
    for (int i = 0; i < 20; i++) {
        points[i][0] = 0.0f;
        points[i][1] = 0.0f;
        points[i][2] = 0.0f;
    }
    int count = Camera_GatherPointsFn(self, &points[0][0], 20);
    float maxX = -100000.0f;
    float minX = 100000.0f;
    float minY = 100000.0f;
    float maxY = -100000.0f;
    float best = -10000.0f;
    float far[3] = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < count; i++) {
        float* p = points[i];
        float negU[3] = {-u[0], -u[1], -u[2]};
        float d[3];
        VectorSub(d, p, origin);
        float out = (d[1] * negU[1] + d[0] * negU[0]) + d[2] * negU[2];
        if (best < out) {
            best = out;
            Copy3(far, p);
        }
        float x = AtMost(p[0], At<float>(court, 0x24) + 4.0f);
        if (maxX < x) {
            maxX = x;
        }
        x = AtLeast(p[0], At<float>(court, 0x18) - 4.0f);
        if (minX > x) {
            minX = x;
        }
        float y = AtMost(p[1], At<float>(court, 0x28) + 10.0f);
        if (maxY < y) {
            maxY = y;
        }
        y = AtLeast(p[1], At<float>(court, 0x1C) - 10.0f);
        if (minY > y) {
            minY = y;
        }
    }
    Copy3(position, origin);
    float pitch = At<float>(cameraTuning_20C8888, 0x2C);
    float centre[3] = {origin[0], origin[1], origin[2]};
    if (count > 0) {
        centre[0] = (minX + maxX) * 0.5f;
        centre[1] = (minY + maxY) * 0.5f;
    }
    float unused[3];
    VectorSub(unused, far, origin);
    float across[3] = {u[0] * 30.0f, u[1] * 30.0f, 30.0f * u[2]};
    float eye[3];
    VectorSub(eye, origin, across);
    float direction[3];
    VectorSub(direction, centre, eye);
    Vector_Normalise(direction);
    pitch = pitch * 3.14159274f / 180.0f;
    float raise[3] = {0.0f, 0.0f, trk::CrtSin(pitch)};
    float c = trk::CrtCos(pitch);
    float back[3] = {-direction[0], -direction[1], -direction[2]};
    float scaled[3] = {back[0] * c, back[1] * c, c * back[2]};
    float look[3];
    VectorAdd(look, scaled, raise);
    Vector_Normalise(look);
    float e[3];
    VectorSub(e, centre, origin);
    float side = (u[1] * e[1] + u[0] * e[0]) + u[2] * e[2];
    if (side < 0.0f) {
        look[0] = -look[0];
    }
    Copy3(position, centre);
    Camera_FitViewFn(self, position, look[0], look[1], look[2], &points[0][0], count);
    uint8_t* tuning = cameraTuning_20C8888;
    float negU[3] = {-u[0], -u[1], -u[2]};
    float f[3];
    VectorSub(f, position, centre);
    float distance = (f[1] * negU[1] + f[0] * negU[0]) + f[2] * negU[2];
    distance = AtMost(distance, At<float>(tuning, 0x1C));
    distance = AtLeast(distance, At<float>(tuning, 0x18));
    float g[3];
    VectorSub(g, centre, origin);
    float s = (g[1] * negU[1] + negU[0] * g[0]) + g[2] * negU[2];
    float t = s + distance;
    if (At<float>(tuning, 0x1C) < t) {
        distance = distance - (t - At<float>(tuning, 0x1C));
    }
    float h[3];
    VectorSub(h, position, centre);
    float sideways = (h[1] * v[1] + v[0] * h[0]) + h[2] * v[2];
    float lo = -At<float>(tuning, 0x1C) * 0.33000001311302185f;
    float hi = 0.33000001311302185f * At<float>(tuning, 0x1C);
    if (lo > sideways) {
        sideways = lo;
    } else if (hi < sideways) {
        sideways = hi;
    }
    float height = position[2] - At<float>(court, 0xA4);
    if (!(height < 6.0f)) {
        height = 6.0f;
    }
    float upward[3] = {height * upVector_9E33DC[0], height * upVector_9E33DC[1], height * upVector_9E33DC[2]};
    float a[3] = {-u[0] * distance, -u[1] * distance, distance * -u[2]};
    float b[3] = {v[0] * sideways, v[1] * sideways, sideways * v[2]};
    float r1[3];
    float r2[3];
    float r3[3];
    Copy3(position, VectorAdd(r3, VectorAdd(r2, VectorAdd(r1, centre, b), a), upward));
    position[2] = At<float>(cameraTuning_20C8888, 0x20) + position[2];
    Copy3(target, centre);
    target[2] = At<float>(cameraTuning_20C8888, 0x24) + target[2];
}

// 0x67A080 (thiscall): vf7, the process: the wanted view computed and sprung to, the
// camera placed, its near clip and (unless locked) fov from the tuning
void __fastcall DodgeballCamera_Process(uint8_t* self, re::Edx) {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float target[3] = {0.0f, 0.0f, 0.0f};
    Camera_ComputeFn(self, position, target);
    Camera_MoveFn(self, position[0], position[1], position[2], target[0], target[1], target[2]);
    Camera_LookAtPoint(theCamera, &At<float>(self, 4), &At<float>(self, 0x1C));
    Camera_SetOffset180(theCamera, &At<float>(self, 0x10));
    Copy3(cameraOffset_C3CDF4, &At<float>(self, 0x28));
    Camera_SetNearClip(camera_C3CCF4, At<float>(cameraTuning_20C8888, 0x38));
    if (!cameraFovLocked_C3CF82) {
        float fov = At<float>(cameraTuning_20C8888, 0x3C);
        cameraFovOverridden_C3CDD4 = 0;
        cameraFov_C3CD04 = fov;
    }
}

// 0x67A170 (thiscall): vf3, the reset: straight to the wanted view, every view flag set,
// the velocities zeroed
void __fastcall DodgeballCamera_Reset(uint8_t* self, re::Edx) {
    Camera_ComputeFn(self, &At<float>(self, 4), &At<float>(self, 0x1C));
    At<int32_t>(self, 0x34) = -1;
    static constexpr int kZeroed[] = {0x10, 0x14, 0x18, 0x28, 0x2C, 0x30};
    for (int at : kZeroed) {
        At<float>(self, at) = 0.0f;
    }
}

} // namespace

#define DODGECAM35_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

DODGECAM35_REVERSED("CDodgeballCameraCtrl::Construct_678C90", 0x00678C90, &DodgeballCamera_Ctor);
DODGECAM35_REVERSED("CDodgeballCameraCtrl::Start_678CC0", 0x00678CC0, &DodgeballCamera_Start);
DODGECAM35_REVERSED("CDodgeballCameraCtrl::GatherPoints_678D90", 0x00678D90, &DodgeballCamera_GatherPoints);
DODGECAM35_REVERSED("CDodgeballCameraCtrl::FitView_679090", 0x00679090, &DodgeballCamera_FitView);
DODGECAM35_REVERSED("CDodgeballCameraCtrl::Move_679690", 0x00679690, &DodgeballCamera_Move);
DODGECAM35_REVERSED("CDodgeballCameraCtrl::Compute_6798E0", 0x006798E0, &DodgeballCamera_Compute);
DODGECAM35_REVERSED("CDodgeballCameraCtrl::Process_67A080", 0x0067A080, &DodgeballCamera_Process);
DODGECAM35_REVERSED("CDodgeballCameraCtrl::Reset_67A170", 0x0067A170, &DodgeballCamera_Reset);
