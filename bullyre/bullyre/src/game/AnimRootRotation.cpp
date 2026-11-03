// AM_ root motion for anims with an extra root joint (the skeleton's last): the rotation
// from the sampled root to the model's stored one, and the travel in the stored root's
// frame. One skeleton (hash 0x6B6F323) turns its stored rotation by the frame time in
// two anims (0x23F12AD6, 0x44A81D7). 0x6B6A60..0x6B6FAC.

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};
struct Xform {
    float rotation[9];
    CVector translation;
    float scale;
};

inline void* const kAnimPose = reinterpret_cast<void*>(0x020C4910);

GAME_FN(void, __thiscall, Pose_SampleJoint, 0x006BE3B0, void* sampler, uint8_t* anim, float time, int joint,
        uint8_t* hierarchy, CVector* translation, float* rotation);
GAME_FN(float, __thiscall, Hxd_GetTranslation, 0x006B3E20, uint8_t* hxd, int animIndex, int joint, CVector* out,
        int key);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint32_t, __cdecl, GetFrameMs, 0x00446870);
GAME_FN(void, __thiscall, Quat_ToNiTransform, 0x00413E60, const float* q, Xform* out);
GAME_FN(void, __thiscall, Matrix_FromTransform, 0x00412A70, uint8_t* matrix, const Xform* t);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A80, uint8_t* matrix);
// (0x004146B0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* Multiply3x3(CVector* out, const uint8_t* m, const CVector* v) { return re::CallVec16<0x004146B0, CVector*>(out, m, v); }

inline float FrameTurn() {
    return float(float(double(GetFrameMs()) * 0.0010000000474974513) * 1.3329999446868896);
}

// 0x6B6A60: the root's rotation (quatOut: the sampled root's inverse times the stored,
// w first) and travel (posOut) for a player; over a loop's wrap only the travel
void __stdcall RootRotation(uint8_t* model, uint8_t* player, uint8_t* starts, float* quatOut, CVector* posOut) {
    uint8_t* hierarchy = At<uint8_t*>(model, 0x94);
    uint8_t* hxd = At<uint8_t*>(hierarchy, 0x10);
    CVector storedPos{0.0f, 0.0f, 0.0f};  // F-0x90
    CVector sampledPos{0.0f, 0.0f, 0.0f}; // F-0x84
    int joint = int(At<uint32_t>(hxd, 0x5C)) - 1;
    float sampled[4];  // F-0xA0
    Pose_SampleJoint(kAnimPose, At<uint8_t*>(player, 0), At<float>(player, 8), joint, hierarchy, &sampledPos, sampled);
    uint8_t* record = starts + At<int32_t>(starts, 0x24) * joint;
    storedPos = *reinterpret_cast<CVector*>(record + 0x64);
    float stored[4];  // F-0xD8
    for (int i = 0; i < 4; i++) {
        stored[i] = At<float>(record, 0x54 + i * 4);
    }
    if (At<float>(player, 4) < At<float>(player, 8) && (At<uint8_t>(player, 0x10) & 2)) {
        uint8_t* anim = At<uint8_t*>(player, 0);
        CVector end{0.0f, 0.0f, 0.0f};
        Hxd_GetTranslation(hxd, At<uint16_t>(anim, 0x32), joint, &end, -1);
        CVector first{0.0f, 0.0f, 0.0f};
        Hxd_GetTranslation(hxd, At<uint16_t>(anim, 0x32), joint, &first, 0);
        CVector t;
        end = *VectorSub(&t, &end, &first);
        CVector sum;
        const CVector* r = VectorAdd(reinterpret_cast<CVector*>(stored), VectorSub(&t, &end, &sampledPos), &storedPos);
        sum = *r;
        *posOut = sum;
        return;
    }
    if (At<uint32_t>(hierarchy, 0x14) == 0x06B6F323) {
        uint32_t animHash = At<uint32_t>(At<uint8_t*>(player, 0), 0x14);
        float sign;
        float w;
        if (animHash == 0x23F12AD6) {
            float turn = FrameTurn();
            float magnitude = std::fabs(sampled[3]);
            float s = turn + magnitude;
            if (!(s < 1.0f)) {
                stored[3] = 1.0f;
                sign = 1.0f;
            } else {
                stored[3] = s;
                sign = s > 0.7f ? 1.0f : -1.0f;
            }
        } else if (animHash == 0x044A81D7) {
            float magnitude = std::fabs(sampled[3]);
            float turn = FrameTurn();
            float s = magnitude - turn;
            if (!(s > 0.0f)) {
                stored[3] = 0.0f;
                sign = -1.0f;
            } else {
                stored[3] = s;
                sign = s > 0.7f ? 1.0f : -1.0f;
            }
        } else {
            goto product;
        }
        stored[3] = sign * stored[3];
        w = std::sqrt(1.0f - stored[3] * stored[3]);
        stored[0] = -sign * w;
    }
product:
    // the sampled rotation's inverse (left as it was on the stack when it's zero)
    float inv[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // (F-0xC8 w, F-0xC4, F-0xC0, F-0xBC)
    float n = ((sampled[1] * sampled[1] + sampled[2] * sampled[2]) + sampled[3] * sampled[3]) + sampled[0] * sampled[0];
    if (n > 0.0f) {
        float r = 1.0f / n;
        inv[0] = r * sampled[0];
        float neg = -r;
        inv[1] = neg * sampled[1];
        inv[2] = neg * sampled[2];
        inv[3] = neg * sampled[3];
    }
    const float d0 = stored[0], d1 = stored[1], d2 = stored[2], d3 = stored[3];
    quatOut[0] = inv[0] * d0 - ((d1 * inv[1] + d2 * inv[2]) + inv[3] * d3);
    quatOut[1] = inv[1] * d0 + ((inv[2] * d3 - d2 * inv[3]) + d1 * inv[0]);
    quatOut[2] = ((d1 * inv[3] - inv[1] * d3) + d2 * inv[0]) + inv[2] * d0;
    quatOut[3] = d0 * inv[3] + (inv[0] * d3 + (inv[1] * d2 - inv[2] * d1));
    CVector travel;
    *posOut = *VectorSub(&travel, &storedPos, &sampledPos);
    // the travel turned by the inverse rotation (the height kept)
    float x = posOut->x, y = posOut->y;
    Xform m;  // F-0x34
    m.scale = 1.0f;
    uint8_t matrix[0x48];  // F-0x78 (a CMatrix attached to m)
    *reinterpret_cast<re::Ptr32<Xform>*>(matrix + 0x40) = &m;
    float q[4] = {inv[1], inv[2], inv[3], inv[0]};
    Quat_ToNiTransform(q, &m);
    Matrix_FromTransform(matrix, &m);
    Matrix_UpdateRW(matrix);
    float z = posOut->z;
    CVector flat{x, y, 0.0f};
    CVector turned;
    Multiply3x3(&turned, matrix, &flat);
    posOut->x = turned.x;
    posOut->y = turned.y;
    posOut->z = z;
}

} // namespace

#define AMROOTROT_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AMROOTROT_REVERSED("AM::RootRotation", 0x006B6A60, &RootRotation);
