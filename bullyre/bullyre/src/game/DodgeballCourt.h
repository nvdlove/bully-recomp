#pragma once

// The dodgeball court's geometry, shared by DodgeballTracks.cpp and DodgeballGame.cpp: the
// court (DodgeballGame +0x17D4, 0xC0 bytes) has its corners +0 / +0xC / +0x18 / +0x24, the
// players' start points +0x30.. (4 a side) and the referee's +0x90, its centre +0x9C, its
// across axis +0xA8 and its along axis +0xB4. A point of it by fractions (-1..1 across and
// along the half extents) and the inward planes of an area's edges are computed the same
// way in several places: written once here, in the games' operation order (single
// precision).

#include <cmath>
#include <cstdint>

#include "game/TrackMath.h"
#include "reversed.h"

namespace dbcourt {

using re::At;

GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, CrossProduct, 0x00414730, float* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, float* v);

inline void Copy3(float* dst, const float* src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

// the court's axes and their extents (the corner +0xC from +0, along each axis)
struct Frame {
    re::Ptr32<float> origin;   // +0x9C
    re::Ptr32<float> u;        // +0xA8
    re::Ptr32<float> v;        // +0xB4
    float negDu;     // -(u . extent)
    float dv;        // v . extent
};
inline Frame MakeFrame(uint8_t* court) {
    Frame f;
    float w[3];
    VectorSub(w, &At<float>(court, 0xC), reinterpret_cast<float*>(court));
    f.u = &At<float>(court, 0xA8);
    f.v = &At<float>(court, 0xB4);
    f.origin = &At<float>(court, 0x9C);
    float du = (f.u[0] * w[0] + f.u[1] * w[1]) + f.u[2] * w[2];
    f.negDu = -du;
    f.dv = (w[0] * f.v[0] + w[1] * f.v[1]) + w[2] * f.v[2];
    return f;
}
// the court point at fractions (across, along): (origin + across part) + along part
inline void Corner(const Frame& f, float across, float along, float* out) {
    float a[3] = {f.v[0] * f.dv, f.v[1] * f.dv, f.dv * f.v[2]};
    float along3[3] = {a[0] * along, a[1] * along, along * a[2]};
    float b[3] = {f.u[0] * f.negDu, f.u[1] * f.negDu, f.negDu * f.u[2]};
    float across3[3] = {b[0] * across, b[1] * across, across * b[2]};
    float tmp[3];
    float tmp2[3];
    Copy3(out, VectorAdd(tmp2, VectorAdd(tmp, f.origin, across3), along3));
}
// an edge's inward plane (normal: edge x up, normalised; `negate`: outward) and distance
inline void EdgePlane(const float* a, const float* b, bool negate, float* plane) {
    float edge[3];
    VectorSub(edge, b, a);
    float up[3] = {0.0f, 0.0f, 1.0f};
    float n[3];
    CrossProduct(n, edge, up);
    Vector_Normalise(n);
    if (negate) {
        n[0] = -n[0];
        n[1] = -n[1];
        n[2] = -n[2];
        float d = (a[0] * n[0] + a[1] * n[1]) + a[2] * n[2];
        plane[0] = n[0];
        plane[1] = n[1];
        plane[2] = n[2];
        plane[3] = float(double(d) - 0.0);
        return;
    }
    float d = (n[0] * a[0] + n[1] * a[1]) + n[2] * a[2];
    plane[0] = n[0];
    plane[1] = n[1];
    plane[2] = n[2];
    plane[3] = float(double(d) - 0.0);
}
inline float PlaneDistance(const float* plane, const float* p) {
    float dot = (plane[0] * p[0] + plane[1] * p[1]) + plane[2] * p[2];
    return dot - plane[3];
}
// a point pushed back through each plane it's past, in turn
inline void PushInside(float planes[4][4], float* q) {
    for (int i = 0; i < 4; i++) {
        float k = -PlaneDistance(planes[i], q);
        if (k < 0.0f) {
            float c[3] = {planes[i][0] * k, planes[i][1] * k, k * planes[i][2]};
            q[0] = q[0] + c[0];
            q[1] = q[1] + c[1];
            q[2] = q[2] + c[2];
        }
    }
}

} // namespace dbcourt
