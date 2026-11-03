// The RenderWare emulation over Gamebryo: RwCamera, an NiCamera (0x2B8, vtable 0x9428E4)
// with RW's fields after it: +0x104 the NiFrustum, +0x120 / +0x124, +0x13C the RW object
// (type 4; its frame link), +0x140 the frame, +0x14C the frame-sync callback, +0x150 the
// projection (1 perspective, 2 parallel), +0x154 / +0x158 begin / end update callbacks,
// +0x15C the view matrix (+0x180 its translation), +0x18C, +0x190 the raster, +0x194 the
// z raster, +0x198 the view window (+0x1A0 its reciprocal), +0x1A8 the view offset, +0x1B0
// near (+0x1B4 as asked), +0x1B8 far, +0x1BC fog, +0x1C0 / +0x1C4 the z scale and shift,
// +0x1C8.. the frustum planes (normals, distances, sign flags), +0x240 the frustum's
// bounding box, +0x258 its eight corners. Also RwBBoxCalculate. 0x6C81D0..0x6C9A2A.

#include <cstdint>

#include "game/VCall.h"

#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

GAME_VAR(uint8_t*, rwc_current, 0x020C5F5C);
GAME_VAR(uint8_t, rwc_updating, 0x020C5F60);
GAME_VAR(int16_t, rwc_screenHeight, 0x020C4850);
GAME_VAR(uint8_t, rwc_cutsceneRunning, 0x020C5BE4);
inline void* const kFrameSync = reinterpret_cast<void*>(0x006C9500);
inline void* const kBeginUpdate = reinterpret_cast<void*>(0x006C8670);
inline void* const kEndUpdate = reinterpret_cast<void*>(0x006C8690);
inline void* const kRtti = reinterpret_cast<void*>(0x020E273C);

GAME_FN(void, __cdecl, Rwc_PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, Rwc_PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Rwc_NiNew, 0x00752460, uint32_t size);
GAME_FN(void, __cdecl, Rwc_NiFreeSized, 0x00752540, void* p, uint32_t size);
GAME_FN(void, __thiscall, Rwc_NiCameraConstruct, 0x00763C20, void* self);
GAME_FN(void, __thiscall, Rwc_NiCameraDestruct, 0x00762D50, void* self);
GAME_FN(void, __thiscall, Rwc_SetViewFrustum, 0x00762D60, void* camera, const float* frustum);
GAME_FN(void, __thiscall, Rwc_LookAtWorldPoint, 0x00763020, void* camera, const CVector* target, const CVector* up);
GAME_FN(void, __thiscall, Rwc_AVObjectUpdate, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __thiscall, Rwc_MatrixIdentity, 0x00770920, void* self);
GAME_FN(uint8_t, __thiscall, Rwc_TransformDiffers, 0x007709F0, const void* self, const void* other);
GAME_FN(float, __thiscall, Rwc_Normalise, 0x00415130, CVector* v);
GAME_FN(void, __cdecl, Rwc_FrameSyncAll, 0x006C9F70);
GAME_FN(void, __cdecl, Rwc_SetParentFrame, 0x006C9BD0, void* object, void* frame);
GAME_FN(void, __cdecl, Rwc_DetachFromFrame, 0x006C9C20, void* object);
GAME_FN(void, __cdecl, Rwc_FrameUpdateObjects, 0x006C9AF0, void* frame);
GAME_FN(const float*, __cdecl, Rwc_FrameLTM, 0x006C9A90, void* frame);
GAME_FN(void, __thiscall, Rwc_UpdateDepth, 0x006C81E0, void* camera);
GAME_FN(float*, __cdecl, Rwc_BBoxCalculate, 0x006C8380, float* box, const float* points, int count);
GAME_FN(uint8_t*, __cdecl, Rwc_SetViewWindow, 0x006C8920, uint8_t* camera, const float* window);
GAME_FN(uint8_t*, __cdecl, Rwc_SetNearClip, 0x006C8820, uint8_t* camera, float nearClip);
GAME_FN(uint8_t*, __cdecl, Rwc_SetFarClip, 0x006C8780, uint8_t* camera, float farClip);
GAME_FN(uint8_t*, __thiscall, Rwc_Construct, 0x006C98D0, void* self);

// 0x6C81D0: the camera's view matrix
uint8_t* __cdecl Camera_GetViewMatrix(uint8_t* camera) {
    return camera + 0x15C;
}

// 0x6C81E0 (thiscall): the z scale and shift from near and far (reciprocals for a
// perspective camera with both above 0; near 0 taken as 0.0001)
uint8_t* __fastcall Camera_UpdateDepth(uint8_t* self, re::Edx) {
    float n = At<float>(self, 0x1B0);
    float f = At<float>(self, 0x1B8);
    if (n == 0.0f) {
        n = 9.999999747378752e-05f;
    }
    float a = n;
    float b = f;
    if (At<int32_t>(self, 0x150) != 2 && 0.0f < n && 0.0f < f) {
        a = 1.0f / n;
        b = 1.0f / f;
    }
    // (1 - 0.0001 and 0.0001 * that, as the game's x87 rounds them)
    const float t1 = 0x1.fff2e4p-1f;
    const float t2 = 0x1.a36370p-14f;
    float s = (t1 - t2) / (b - a);
    At<float>(self, 0x1C0) = s;
    At<float>(self, 0x1C4) = (t1 + t2) - s * (a + b);
    return self;
}

// 0x6C82D0 / 0x6C82E0: far and near (x87 returns)
float __cdecl Camera_GetFarClip(uint8_t* camera) {
    return At<float>(camera, 0x1B8);
}
float __cdecl Camera_GetNearClip(uint8_t* camera) {
    return At<float>(camera, 0x1B0);
}

// 0x6C82F0: its frame
void* __cdecl Camera_GetFrame(uint8_t* camera) {
    return At<void*>(camera, 0x140);
}

// 0x6C8300: the fog distance from an integer
void __cdecl Camera_SetFogDistance(uint8_t* camera, int32_t distance) {
    At<float>(camera, 0x1BC) = float(distance);
}

// 0x6C8310: the view window
uint8_t* __cdecl Camera_GetViewWindow(uint8_t* camera) {
    return camera + 0x198;
}

// 0x6C8320: RwCameraSetProjection
uint8_t* __cdecl Camera_SetProjection(uint8_t* camera, int32_t projection) {
    At<int32_t>(camera, 0x150) = projection;
    if (At<void*>(camera, 0x140) != nullptr) {
        Rwc_FrameUpdateObjects(At<void*>(camera, 0x140));
    }
    Rwc_UpdateDepth(camera);
    return camera;
}

// 0x6C8350: the camera when it has a raster, else null
uint8_t* __cdecl Camera_IfRaster(uint8_t* camera) {
    return At<void*>(camera, 0x190) != nullptr ? camera : nullptr;
}

// 0x6C8370: the camera being updated
uint8_t* __cdecl Camera_GetCurrent() {
    return rwc_current;
}

// 0x6C8380: RwBBoxCalculate (+0 the largest, +0xC the smallest of the points)
float* __cdecl BBoxCalculate(float* box, const float* points, int count) {
    box[0] = points[0];
    box[1] = points[1];
    box[2] = points[2];
    box[3] = box[0];
    box[4] = box[1];
    box[5] = box[2];
    for (int i = 1; i < count; i++) {
        const float* p = points + i * 3;
        for (int k = 0; k < 3; k++) {
            if (box[3 + k] > p[k]) {
                box[3 + k] = p[k];
            }
            if (box[k] < p[k]) {
                box[k] = p[k];
            }
        }
    }
    return box;
}

// 0x6C8670: the default begin-update: made current, the frames synced
uint8_t* __cdecl Camera_DefaultBeginUpdate(uint8_t* camera) {
    rwc_current = camera;
    Rwc_FrameSyncAll();
    return camera;
}

// 0x6C8690: the default end-update
uint8_t* __cdecl Camera_DefaultEndUpdate(uint8_t* camera) {
    rwc_current = nullptr;
    return camera;
}

// 0x6C86A0: RwCameraBeginUpdate
uint8_t* __cdecl Camera_BeginUpdate(uint8_t* camera) {
    using Callback = void*(__cdecl*)(uint8_t*);
    At<Callback>(camera, 0x154)(camera);
    rwc_updating = 1;
    return camera;
}

// 0x6C86C0: RwCameraEndUpdate
uint8_t* __cdecl Camera_EndUpdate(uint8_t* camera) {
    using Callback = void*(__cdecl*)(uint8_t*);
    At<Callback>(camera, 0x158)(camera);
    rwc_updating = 0;
    return camera;
}

// 0x6C86E0: RwCameraDestroy (detached, its reference let go of; returns 1)
int __cdecl Camera_Destroy(uint8_t* camera) {
    Rwc_DetachFromFrame(camera + 0x13C);
    if (vc::Decrement(camera + 4) == 0) {
        vc::Call<void>(camera, 4);
    }
    return 1;
}

// 0x6C8720: RwCameraSetFrame
uint8_t* __cdecl Camera_SetFrame(uint8_t* camera, void* frame) {
    Rwc_SetParentFrame(camera + 0x13C, frame);
    return camera;
}

// 0x6C8740 / 0x6C8750 / 0x6C8760 / 0x6C8770: the raster and z raster
void* __cdecl Camera_GetRaster(uint8_t* camera) {
    return At<void*>(camera, 0x190);
}
void __cdecl Camera_SetRaster(uint8_t* camera, void* raster) {
    At<void*>(camera, 0x190) = raster;
}
void __cdecl Camera_SetZRaster(uint8_t* camera, void* raster) {
    At<void*>(camera, 0x194) = raster;
}
void* __cdecl Camera_GetZRaster(uint8_t* camera) {
    return At<void*>(camera, 0x194);
}

// the NiCamera frustum set from a copy (the screen height at +0x124) and near read back
inline void ApplyFrustum(uint8_t* camera, float* frustum) {
    At<float>(camera, 0x124) = float(int32_t(rwc_screenHeight));
    Rwc_SetViewFrustum(camera, frustum);
    for (int i = 0; i < 7; i++) {
        frustum[i] = At<float>(camera, 0x104 + i * 4);
    }
    At<float>(camera, 0x1B0) = frustum[4];
}

// 0x6C8780: RwCameraSetFarClipPlane
uint8_t* __cdecl Camera_SetFarClip(uint8_t* camera, float farClip) {
    At<float>(camera, 0x1B8) = farClip;
    Rwc_UpdateDepth(camera);
    if (At<void*>(camera, 0x140) != nullptr) {
        Rwc_FrameUpdateObjects(At<void*>(camera, 0x140));
    }
    float frustum[7];
    for (int i = 0; i < 7; i++) {
        frustum[i] = At<float>(camera, 0x104 + i * 4);
    }
    frustum[5] = farClip;
    frustum[4] = At<float>(camera, 0x1B4);
    ApplyFrustum(camera, frustum);
    return camera;
}

// 0x6C8820: RwCameraSetNearClipPlane: kept at least far / screen height and 0.05
uint8_t* __cdecl Camera_SetNearClip(uint8_t* camera, float nearClip) {
    At<float>(camera, 0x1B4) = nearClip;
    float lowest = At<float>(camera, 0x1B8) / float(int32_t(rwc_screenHeight));
    float clamped = nearClip;
    if (lowest > nearClip) {
        clamped = lowest;
    }
    if (clamped < 0.05000000074505806f) {
        clamped = 0.05000000074505806f;
    }
    At<float>(camera, 0x1B0) = clamped;
    Rwc_UpdateDepth(camera);
    if (At<void*>(camera, 0x140) != nullptr) {
        Rwc_FrameUpdateObjects(At<void*>(camera, 0x140));
    }
    float frustum[7];
    for (int i = 0; i < 7; i++) {
        frustum[i] = At<float>(camera, 0x104 + i * 4);
    }
    frustum[4] = clamped;
    At<float>(camera, 0x124) = float(int32_t(rwc_screenHeight));
    At<float>(camera, 0x120) = 0.05000000074505806f;
    Rwc_SetViewFrustum(camera, frustum);
    for (int i = 0; i < 7; i++) {
        frustum[i] = At<float>(camera, 0x104 + i * 4);
    }
    At<float>(camera, 0x1B0) = frustum[4];
    return camera;
}

// 0x6C8920: RwCameraSetViewWindow (reciprocals 0 for a 0 window)
uint8_t* __cdecl Camera_SetViewWindow(uint8_t* camera, const float* window) {
    At<float>(camera, 0x198) = window[0];
    At<float>(camera, 0x19C) = window[1];
    At<float>(camera, 0x1A0) = At<float>(camera, 0x198) == 0.0f ? 0.0f : 1.0f / At<float>(camera, 0x198);
    At<float>(camera, 0x1A4) = At<float>(camera, 0x19C) == 0.0f ? 0.0f : 1.0f / At<float>(camera, 0x19C);
    if (At<void*>(camera, 0x140) != nullptr) {
        Rwc_FrameUpdateObjects(At<void*>(camera, 0x140));
    }
    float frustum[7];
    for (int i = 0; i < 7; i++) {
        frustum[i] = At<float>(camera, 0x104 + i * 4);
    }
    frustum[1] = window[0];
    frustum[0] = -window[0];
    frustum[2] = window[1];
    frustum[3] = -window[1];
    At<float>(camera, 0x124) = float(int32_t(rwc_screenHeight));
    Rwc_SetViewFrustum(camera, frustum);
    return camera;
}

inline CVector Scale(const CVector& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}
inline CVector Sub(const CVector& a, const CVector& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline CVector Add(const CVector& a, const CVector& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
// the cross products the planes use (the two operand orders the game has)
inline CVector CrossA(const CVector& u, const CVector& v) {
    return {v.z * u.y - v.y * u.z, u.z * v.x - v.z * u.x, u.x * v.y - v.x * u.y};
}
inline CVector CrossB(const CVector& u, const CVector& v) {
    return {v.y * u.z - v.z * u.y, v.z * u.x - u.z * v.x, u.y * v.x - v.y * u.x};
}
inline uint8_t NotNegative(float f) {
    return f >= 0.0f ? 1 : 0;
}

// 0x6C8A00 (the camera in esi: not hookable): the frustum's corners (near at +0x258, far at
// +0x288), its planes (the view direction and its opposite, then four sides: normals
// normalised, distances through a corner) and each normal's component signs
void UpdateFrustum(uint8_t* camera) {
    uint8_t* ltm = At<uint8_t*>(camera, 0x140) + 0x48;
    const float* m = reinterpret_cast<const float*>(ltm);
    CVector a = {m[0], m[3], m[6]};
    CVector b = {m[1], m[4], m[7]};
    CVector c = {m[2], m[5], m[8]};
    CVector pos = {m[9], m[10], m[11]};
    float wx = At<float>(camera, 0x198);
    float wy = At<float>(camera, 0x19C);
    CVector corners[4];
    corners[0] = Add(Add(Scale(a, wx), c), Scale(b, wy));
    corners[1] = Add(Sub(c, Scale(a, wx)), Scale(b, wy));
    corners[2] = Sub(Sub(c, Scale(a, wx)), Scale(b, wy));
    corners[3] = Sub(Add(Scale(a, wx), c), Scale(b, wy));
    float oy = At<float>(camera, 0x1AC);
    float ox = -At<float>(camera, 0x1A8);
    CVector offset = Add(Scale(a, ox), Scale(b, oy));
    CVector* nearCorners = reinterpret_cast<CVector*>(camera + 0x258);
    CVector* farCorners = reinterpret_cast<CVector*>(camera + 0x288);
    for (int i = 0; i < 4; i++) {
        float keep = 1.0f - At<float>(camera, 0x1B0);
        CVector shift = Scale(offset, keep);
        float n = At<float>(camera, 0x1B0);
        CVector along = Scale(corners[i], n);
        nearCorners[i] = Add(Add(pos, along), shift);
        keep = 1.0f - At<float>(camera, 0x1B8);
        shift = Scale(offset, keep);
        float f = At<float>(camera, 0x1B8);
        along = Scale(corners[i], f);
        farCorners[i] = Add(Add(along, pos), shift);
    }
    At<CVector>(camera, 0x1C8) = c;
    const float* far0 = &farCorners[0].x;
    At<float>(camera, 0x1D4) = (far0[1] * c.y + far0[0] * c.x) + far0[2] * c.z;
    At<float>(camera, 0x1DC) = -At<float>(camera, 0x1C8);
    At<float>(camera, 0x1E0) = -At<float>(camera, 0x1CC);
    At<float>(camera, 0x1E4) = -At<float>(camera, 0x1D0);
    const float* near0 = &nearCorners[0].x;
    At<float>(camera, 0x1E8) = (near0[1] * At<float>(camera, 0x1E0) + near0[0] * At<float>(camera, 0x1DC)) +
                               near0[2] * At<float>(camera, 0x1E4);
    // the four sides: each from a near corner's edge and a far edge
    CVector u = Sub(nearCorners[1], farCorners[1]);
    CVector v = Sub(farCorners[2], farCorners[1]);
    CVector* plane = reinterpret_cast<CVector*>(camera + 0x1F0);
    *plane = CrossA(u, v);
    Rwc_Normalise(plane);
    At<float>(camera, 0x1FC) = (plane->y * nearCorners[1].y + nearCorners[1].x * plane->x) + plane->z * nearCorners[1].z;
    v = Sub(farCorners[0], farCorners[1]);
    plane = reinterpret_cast<CVector*>(camera + 0x204);
    *plane = CrossB(u, v);
    Rwc_Normalise(plane);
    At<float>(camera, 0x210) = (nearCorners[1].x * plane->x + plane->y * nearCorners[1].y) + plane->z * nearCorners[1].z;
    u = Sub(nearCorners[3], farCorners[3]);
    v = Sub(farCorners[0], farCorners[3]);
    plane = reinterpret_cast<CVector*>(camera + 0x218);
    *plane = CrossA(u, v);
    Rwc_Normalise(plane);
    At<float>(camera, 0x224) = (plane->y * nearCorners[3].y + plane->x * nearCorners[3].x) + plane->z * nearCorners[3].z;
    v = Sub(farCorners[2], farCorners[3]);
    plane = reinterpret_cast<CVector*>(camera + 0x22C);
    *plane = CrossB(u, v);
    Rwc_Normalise(plane);
    At<float>(camera, 0x238) = (plane->y * nearCorners[3].y + plane->x * nearCorners[3].x) + plane->z * nearCorners[3].z;
    static const int kSigns[][2] = {{0x1C8, 0x1D8}, {0x1DC, 0x1EC}, {0x1F0, 0x200},
                                    {0x204, 0x214}, {0x218, 0x228}, {0x22C, 0x23C}};
    for (const auto& s : kSigns) {
        for (int k = 0; k < 3; k++) {
            At<uint8_t>(camera, s[1] + k) = NotNegative(At<float>(camera, s[0] + k * 4));
        }
    }
}

// 0x6C9500 (the frame-sync callback, given the camera's RW object): for a perspective
// camera the view matrix (+0x15C) from the frame and the view window and offset, and the
// frustum; always the frustum's box; then the NiCamera placed on the frame (looking down
// its third axis) when the frame moved or a cutscene runs
uint8_t* __cdecl Camera_FrameSync(uint8_t* object) {
    uint8_t* camera = object - 0x13C;
    if (At<int32_t>(camera, 0x150) == 1) {
        const float* m = reinterpret_cast<const float*>(At<uint8_t*>(camera, 0x140) + 0x48);
        CVector a = {m[0], m[3], m[6]};
        CVector b = {m[1], m[4], m[7]};
        CVector c = {m[2], m[5], m[8]};
        CVector pos = {m[9], m[10], m[11]};
        float rx = At<float>(camera, 0x1A0);
        CVector p = Scale(a, rx);
        float k = At<float>(camera, 0x1A8) * At<float>(camera, 0x1A0);
        k = k + 1.0f;
        CVector r = Sub(Scale(c, k), p);
        CVector s = Scale(r, 0.5f);
        float t = At<float>(camera, 0x1A0) * -0.5f;
        t = t * At<float>(camera, 0x1A8);
        float w0 = t - ((s.y * pos.y + s.x * pos.x) + s.z * pos.z);
        float ry = At<float>(camera, 0x1A4);
        CVector p2 = Scale(b, ry);
        float k2 = 1.0f - At<float>(camera, 0x1A4) * At<float>(camera, 0x1AC);
        CVector r2 = Sub(Scale(c, k2), p2);
        CVector s2 = Scale(r2, 0.5f);
        float t2 = 0.5f * At<float>(camera, 0x1A4);
        t2 = t2 * At<float>(camera, 0x1AC);
        float w1 = t2 - ((s2.y * pos.y + s2.x * pos.x) + s2.z * pos.z);
        float w2 = -((pos.y * c.y + c.x * pos.x) + c.z * pos.z);
        At<float>(camera, 0x15C) = s.x;
        At<float>(camera, 0x168) = s2.x;
        At<float>(camera, 0x174) = c.x;
        At<float>(camera, 0x160) = s.y;
        At<float>(camera, 0x16C) = s2.y;
        At<float>(camera, 0x178) = c.y;
        At<float>(camera, 0x164) = s.z;
        At<float>(camera, 0x170) = s2.z;
        At<float>(camera, 0x17C) = c.z;
        At<float>(camera, 0x180) = w0;
        At<float>(camera, 0x184) = w1;
        At<float>(camera, 0x188) = w2;
        UpdateFrustum(camera);
    }
    Rwc_BBoxCalculate(reinterpret_cast<float*>(camera + 0x240), reinterpret_cast<float*>(camera + 0x258), 8);
    const float* ltm = Rwc_FrameLTM(At<void*>(camera, 0x140));
    float local[13];   // (only its rotation and scale set, as the game has it)
    local[12] = 1.0f;
    Rwc_MatrixIdentity(local);
    if (rwc_cutsceneRunning == 0 && Rwc_TransformDiffers(ltm, local) == 0) {
        return object;
    }
    At<float>(camera, 0x84) = ltm[9];
    At<float>(camera, 0x88) = ltm[10];
    At<float>(camera, 0x8C) = ltm[11];
    At<float>(camera, 0x50) = ltm[9];
    At<float>(camera, 0x54) = ltm[10];
    At<float>(camera, 0x58) = ltm[11];
    CVector up = {ltm[1], ltm[4], ltm[7]};
    CVector dir = {ltm[2], ltm[5], ltm[8]};
    CVector target = {dir.x + ltm[9], ltm[10] + dir.y, ltm[11] + dir.z};
    Rwc_LookAtWorldPoint(camera, &target, &up);
    Rwc_AVObjectUpdate(camera, 0.0f, 1);
    return object;
}

// 0x6C98D0 (thiscall): the constructor: perspective, a 1 x 1 window, near 0.05, far 10,
// fog 5
uint8_t* __fastcall Camera_Construct(uint8_t* self, re::Edx) {
    Rwc_NiCameraConstruct(self);
    At<uint32_t>(self, 0) = 0x009428E4;
    At<float>(self, 0x18C) = 1.0f;
    At<uint8_t>(self, 0x13D) = 0;
    At<uint8_t>(self, 0x13E) = 0;
    At<uint8_t>(self, 0x13F) = 0;
    At<uint32_t>(self, 4) = 0;
    At<void*>(self, 0x194) = nullptr;
    At<void*>(self, 0x190) = nullptr;
    At<uint8_t>(self, 0x13C) = 4;
    At<void*>(self, 0x14C) = kFrameSync;
    At<int32_t>(self, 0x150) = 1;
    At<void*>(self, 0x154) = kBeginUpdate;
    At<void*>(self, 0x158) = kEndUpdate;
    float window[2] = {1.0f, 1.0f};
    Rwc_SetViewWindow(self, window);
    At<float>(self, 0x1AC) = 0.0f;
    At<float>(self, 0x1A8) = 0.0f;
    Rwc_SetNearClip(self, 0.05000000074505806f);
    Rwc_SetFarClip(self, 10.0f);
    At<float>(self, 0x1BC) = 5.0f;
    Rwc_UpdateDepth(self);
    return self;
}

// 0x6C99A0 (vf 2): its RTTI
void* __fastcall Camera_GetRtti(void*, re::Edx) {
    return kRtti;
}

// 0x6C99B0 (vf 0): the deleting destructor
uint8_t* __fastcall Camera_Delete(uint8_t* self, re::Edx, int flags) {
    Rwc_NiCameraDestruct(self);
    if (flags & 1) {
        Rwc_NiFreeSized(self, 0x2B8);
    }
    return self;
}

// 0x6C99E0: RwCameraCreate (memory tag 1; referenced once)
uint8_t* __cdecl Camera_Create() {
    Rwc_PushMemoryTag(1);
    void* p = Rwc_NiNew(0x2B8);
    uint8_t* camera = p != nullptr ? Rwc_Construct(p) : nullptr;
    Rwc_PopMemoryTag();
    vc::Increment(camera + 4);
    return camera;
}

} // namespace

RE_UNHOOKED("RwCamera::UpdateFrustum", 0x006C8A00);

#define RWCAMERA_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

RWCAMERA_REVERSED("RwCamera::GetViewMatrix", 0x006C81D0, &Camera_GetViewMatrix);
RWCAMERA_REVERSED("RwCamera::UpdateDepth", 0x006C81E0, &Camera_UpdateDepth);
RWCAMERA_REVERSED("RwCamera::GetFarClipPlane", 0x006C82D0, &Camera_GetFarClip);
RWCAMERA_REVERSED("RwCamera::GetNearClipPlane", 0x006C82E0, &Camera_GetNearClip);
RWCAMERA_REVERSED("RwCamera::GetFrame", 0x006C82F0, &Camera_GetFrame);
RWCAMERA_REVERSED("RwCamera::SetFogDistance", 0x006C8300, &Camera_SetFogDistance);
RWCAMERA_REVERSED("RwCamera::GetViewWindow", 0x006C8310, &Camera_GetViewWindow);
RWCAMERA_REVERSED("RwCamera::SetProjection", 0x006C8320, &Camera_SetProjection);
RWCAMERA_REVERSED("RwCamera::IfRaster", 0x006C8350, &Camera_IfRaster);
RWCAMERA_REVERSED("RwCamera::GetCurrent", 0x006C8370, &Camera_GetCurrent);
RWCAMERA_REVERSED("RwBBox::Calculate", 0x006C8380, &BBoxCalculate);
RWCAMERA_REVERSED("RwCamera::DefaultBeginUpdate", 0x006C8670, &Camera_DefaultBeginUpdate);
RWCAMERA_REVERSED("RwCamera::DefaultEndUpdate", 0x006C8690, &Camera_DefaultEndUpdate);
RWCAMERA_REVERSED("RwCamera::BeginUpdate", 0x006C86A0, &Camera_BeginUpdate);
RWCAMERA_REVERSED("RwCamera::EndUpdate", 0x006C86C0, &Camera_EndUpdate);
RWCAMERA_REVERSED("RwCamera::Destroy", 0x006C86E0, &Camera_Destroy);
RWCAMERA_REVERSED("RwCamera::SetFrame", 0x006C8720, &Camera_SetFrame);
RWCAMERA_REVERSED("RwCamera::GetRaster", 0x006C8740, &Camera_GetRaster);
RWCAMERA_REVERSED("RwCamera::SetRaster", 0x006C8750, &Camera_SetRaster);
RWCAMERA_REVERSED("RwCamera::SetZRaster", 0x006C8760, &Camera_SetZRaster);
RWCAMERA_REVERSED("RwCamera::GetZRaster", 0x006C8770, &Camera_GetZRaster);
RWCAMERA_REVERSED("RwCamera::SetFarClipPlane", 0x006C8780, &Camera_SetFarClip);
RWCAMERA_REVERSED("RwCamera::SetNearClipPlane", 0x006C8820, &Camera_SetNearClip);
RWCAMERA_REVERSED("RwCamera::SetViewWindow", 0x006C8920, &Camera_SetViewWindow);
RWCAMERA_REVERSED("RwCamera::FrameSync", 0x006C9500, &Camera_FrameSync);
RWCAMERA_REVERSED("RwCamera::Construct", 0x006C98D0, &Camera_Construct);
RWCAMERA_REVERSED("RwCamera::GetRtti", 0x006C99A0, &Camera_GetRtti);
RWCAMERA_REVERSED("RwCamera::Delete", 0x006C99B0, &Camera_Delete);
RWCAMERA_REVERSED("RwCamera::Create", 0x006C99E0, &Camera_Create);
