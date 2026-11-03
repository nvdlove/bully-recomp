// 0x51E2E0: COccluder::ProcessOneOccluder (GTA San Andreas's, with Bully's float
// occluders): an occluder near enough on screen becomes an active occluder. A flat one
// (a zero side) gives the four lines of its rectangle and keeps its plane; a box gives
// the lines of the silhouette edges (between a face turned to the camera and one turned
// away), each clipped to the near plane. Too small on screen (64 x 33.6), off screen or
// without lines: not used.
//
// (x87 code: the sums are done in double here and stored as float where the game
// stores them)

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"

namespace {

struct Occluder {
    float length;      // +0
    float width;       // +4
    float height;      // +8
    float mid[3];      // +0xC
    uint16_t angle;    // +0x18
    int16_t next;      // +0x1A
    uint8_t flag;      // +0x1C
    uint8_t unknown1D[3];
    int32_t area;      // +0x20
};
static_assert(sizeof(Occluder) == 0x24);

struct OccluderLine {
    float x, y, dx, dy, length;
};

struct ActiveOccluder {
    OccluderLine lines[6];  // +0
    uint8_t numLines;       // +0x78
    uint8_t isPlane;        // +0x79
    uint8_t unknown7A[2];
    float distance;         // +0x7C
};
static_assert(sizeof(ActiveOccluder) == 0x80);

struct Corner {
    float world[4];    // +0
    float screen[4];   // +0x10
    uint8_t valid;     // +0x20
    uint8_t unknown21[3];
};
static_assert(sizeof(Corner) == 0x24);

struct Vec4 {
    float x, y, z, w;
};

GAME_VAR(float, gMinXInOccluder, 0x00C89F84);
GAME_VAR(float, gMaxXInOccluder, 0x00C89F80);
GAME_VAR(float, gMinYInOccluder, 0x00C89F7C);
GAME_VAR(float, gMaxYInOccluder, 0x00C89F78);
inline float* const gOccluderCoorsMax = reinterpret_cast<float*>(0x00C8E8C8);
inline float* const gOccluderCoorsMin = reinterpret_cast<float*>(0x00C8E8D8);
inline const float* const TheCameraViewMatrix = reinterpret_cast<const float*>(0x00C3CCB0);
GAME_VAR(void*, ms_pCamera_C3CCF4, 0x00C3CCF4);

GAME_FN(float*, __cdecl, MatrixTimesPoint, 0x0045D4A0, float* out, const float* matrix, const float* point);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __thiscall, Vector_Scale, 0x00413A20, float* self, float* out, float scale);
GAME_FN(uint8_t, __cdecl, IsPointInsideLine, 0x0051CF90, float lineX, float lineY, float dx, float dy, float x,
        float y, float tolerance);
GAME_FN(uint8_t, __stdcall, ProcessLineSegmentView, 0x0051D460, Vec4 a, Vec4 b, ActiveOccluder* active,
        float* centre);
GAME_FN(uint8_t, __stdcall, ProcessLineSegment_x64, 0x0051D780, Corner a, Corner b, ActiveOccluder* active, float cx, float cy, uint32_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t ProcessLineSegment(Corner a0, Corner a1, ActiveOccluder* a2, float a3, float a4, float a5, int a6) {
    return ProcessLineSegment_x64(a0, a1, a2, a3, a4, re::BitsOf(a5), a6);
}

// 0x51D360 takes `out` in ecx and leaves the stack to the caller
__declspec(noinline) uint8_t CallCalcScreenCoors(float* result4, float x, float y, float z, uint32_t w) {
#if defined(_M_IX86)
    uint8_t result;
    __asm {
        push w
        push z
        push y
        push x
        mov ecx, result4
        mov eax, 0x0051D360
        call eax
        add esp, 16
        mov result, al
    }
    return result;
#else
    return re::Ours<uint8_t (*)(float*, float, float, float, uint32_t)>(0x0051D360)(result4, x, y, z, w);
#endif
}

// a corner's world position (x, y, z) from its three parts, stored as float at each
// step
void Add3(float* out, const float* a, const float* b, float signB) {
    for (int i = 0; i < 3; i++) {
        out[i] = float(double(a[i]) + signB * double(b[i]));
    }
}

// (the face flags: v . (v + d) for the face +v, with d the camera-to-centre offset)
bool Facing(const float* v, const float* d) {
    double dot = double(v[2]) * float(v[2] + d[2]) + double(v[1]) * float(v[1] + d[1]) +
                 double(v[0]) * float(v[0] + d[0]);
    return dot < 0.0;
}

} // namespace

namespace COcclusion3 {

// 0x51E2E0 (thiscall): COccluder::ProcessOneOccluder
static uint8_t __fastcall ProcessOneOccluder(Occluder* self, re::Edx, ActiveOccluder* active) {
    bool isPlane = self->length == 0.0f || self->width == 0.0f || self->height == 0.0f;
    active->numLines = 0;
    active->isPlane = isPlane ? 1 : 0;
    float mid[3] = {self->mid[0], self->mid[1], self->mid[2]};
    float screen[4];
    uint32_t garbage = 0;   // (w: whatever was on the stack, here and below)
    if (!CallCalcScreenCoors(screen, mid[0], mid[1], mid[2], garbage) && !isPlane) {
        return 0;
    }
    if (300.0f < screen[2]) {
        return 0;
    }
    float angle = float(double(self->angle) * 9.58738019107841e-05);
    float s = float(std::sin(double(angle)));
    float c = float(std::cos(double(angle)));
    // the half sides: along the length, the width and the height
    float v1[4] = {float(double(self->length) * 0.5 * s), float(double(-self->length) * 0.5 * c), 0.0f, 0.0f};
    float v2[4] = {float(double(c) * (double(self->width) * 0.5)), float(double(self->width) * 0.5 * s), 0.0f,
                   0.0f};
    float v3[4] = {0.0f, 0.0f, float(0.5 * self->height), 0.0f};
    const float* cameraPos = reinterpret_cast<const float*>(GetLocalTransform(GetCameraNode(ms_pCamera_C3CCF4)) + 0x24);
    float cam[3] = {cameraPos[0], cameraPos[1], cameraPos[2]};
    float d[3] = {mid[0] - cam[0], mid[1] - cam[1], mid[2] - cam[2]};

    if (isPlane) {
        // the rectangle's two half sides
        const float* p;
        const float* q;
        if (self->length == 0.0f) {
            p = v2;
            q = v3;
        } else if (self->width == 0.0f) {
            p = v1;
            q = v3;
        } else {
            p = v1;
            q = v2;
        }
        float plusP[3];
        float minusP[3];
        Add3(plusP, mid, p, 1.0);
        Add3(minusP, mid, p, -1.0);
        float corners[4][4];   // (+p +q), (-p +q), (+p -q), (-p -q)
        Add3(corners[0], plusP, q, 1.0);
        Add3(corners[1], minusP, q, 1.0);
        Add3(corners[2], plusP, q, -1.0);
        Add3(corners[3], minusP, q, -1.0);
        for (float* corner : corners) {
            std::memcpy(&corner[3], &garbage, 4);
        }

        gOccluderCoorsMin[0] = 999999.875f;
        gOccluderCoorsMin[1] = 999999.875f;
        gOccluderCoorsMin[2] = 0.0f;
        gOccluderCoorsMax[0] = -999999.875f;
        gOccluderCoorsMax[1] = -999999.875f;
        gOccluderCoorsMax[2] = 0.0f;
        Vec4 view[4];
        for (int i = 0; i < 4; i++) {
            float out[4];
            MatrixTimesPoint(out, TheCameraViewMatrix, corners[i]);
            view[i] = {out[0], out[1], out[2], 0.0f};
        }
        float centre[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        bool any = ProcessLineSegmentView(view[0], view[2], active, centre) != 0;
        any |= ProcessLineSegmentView(view[1], view[3], active, centre) != 0;
        any |= ProcessLineSegmentView(view[0], view[1], active, centre) != 0;
        any |= ProcessLineSegmentView(view[2], view[3], active, centre) != 0;

        float maxX = gOccluderCoorsMax[0];
        float minX = gOccluderCoorsMin[0];
        float maxY = gOccluderCoorsMax[1];
        float minY = gOccluderCoorsMin[1];
        float width = maxX - minX;
        float height = maxY - minY;
        if (64.0f > width || 33.6f > height) {
            return 0;
        }
        if (maxX < 0.0f && minX < 0.0f) {
            return 0;
        }
        if (maxX > 640.0f && minX > 640.0f) {
            return 0;
        }
        if (maxY < 0.0f && minY < 0.0f) {
            return 0;
        }
        if (double(maxY) > 480.0 && double(minY) > 480.0) {
            return 0;
        }
        if (!any) {
            return 0;
        }
        float average[3];
        float scale = float(1.0 / double(int32_t(active->numLines * 2)));
        Vector_Scale(centre, average, scale);
        for (int i = 0; i < active->numLines; i++) {
            OccluderLine& line = active->lines[i];
            if (IsPointInsideLine(line.x, line.y, line.dx, line.dy, centre[0], centre[1], 0.0f)) {
                line.x = line.dx + line.x;
                line.y = line.dy + line.y;
                line.dx = -line.dx;
                line.dy = -line.dy;
            }
            line.dx = line.dx / line.length;
            line.dy = line.dy / line.length;
        }

        // the plane through corners 1, 2 and 3
        const float* c1 = corners[1];
        const float* c2 = corners[2];
        const float* c3 = corners[3];
        float nx = float((double(c3[2]) - c1[2]) * c2[1] + (double(c2[2]) - c3[2]) * c1[1] +
                         (double(c1[2]) - c2[2]) * c3[1]);
        float ny = float((double(c3[0]) - c1[0]) * c2[2] + (double(c2[0]) - c3[0]) * c1[2] +
                         (double(c1[0]) - c2[0]) * c3[2]);
        float nz = float((double(c2[1]) - c3[1]) * c1[0] + (double(c3[1]) - c1[1]) * c2[0] +
                         (double(c1[1]) - c2[1]) * c3[0]);
        double k1 = double(c3[2]) * c2[1] - double(c3[1]) * c2[2];
        double k2 = double(c3[1]) * c1[2] - double(c3[2]) * c1[1];
        double k3 = double(c1[1]) * c2[2] - double(c1[2]) * c2[1];
        float dPlane = float(((k1 * c1[0] + k2 * c2[0]) + k3 * c3[0]) * -1.0);
        double distance = ((double(nx) * cam[0] + double(ny) * cam[1]) + double(nz) * cam[2]) + dPlane;
        float lengthSq = float((double(nx) * nx + double(ny) * ny) + double(nz) * nz);
        float length = float(std::sqrt(double(lengthSq)));
        active->distance = std::fabs(float(distance / length));
        float* plane = &active->lines[5].x;   // (+0x64)
        plane[0] = nx;
        plane[1] = ny;
        plane[2] = nz;
        plane[3] = dPlane;
        return 1;
    }

    // a box: its distance, the faces turned to the camera, the eight corners projected
    float side = (self->width < self->length) ? self->length : self->width;
    active->distance = float(double(side) * double(0.35f) + screen[2]);
    float minusV1[3] = {-v1[0], -v1[1], -v1[2]};
    float minusV2[3] = {-v2[0], -v2[1], -v2[2]};
    float minusV3[3] = {-v3[0], -v3[1], -v3[2]};
    uint8_t facing[6] = {
        uint8_t(Facing(v1, d)), uint8_t(Facing(minusV1, d)), uint8_t(Facing(v2, d)),
        uint8_t(Facing(minusV2, d)), uint8_t(Facing(v3, d)), uint8_t(Facing(minusV3, d)),
    };
    // A..H: (+-v1, +-v2, +-v3) = +++, -++, +-+, --+, ++-, -+-, +--, ---
    Corner corners[8];
    static const int8_t kSigns[8][3] = {{1, 1, 1},  {-1, 1, 1},  {1, -1, 1},  {-1, -1, 1},
                                        {1, 1, -1}, {-1, 1, -1}, {1, -1, -1}, {-1, -1, -1}};
    for (int i = 0; i < 8; i++) {
        float a[3];
        float b[3];
        Add3(a, mid, v1, kSigns[i][0]);
        Add3(b, a, v2, kSigns[i][1]);
        Add3(corners[i].world, b, v3, kSigns[i][2]);
        std::memcpy(&corners[i].world[3], &garbage, 4);
    }
    for (Corner& corner : corners) {
        corner.valid = CallCalcScreenCoors(corner.screen, corner.world[0], corner.world[1], corner.world[2], garbage);
    }
    gMinYInOccluder = 999999.875f;
    gMinXInOccluder = 999999.875f;
    gMaxYInOccluder = -999999.875f;
    gMaxXInOccluder = -999999.875f;

    // the edges: the two faces, then the corners (first, second)
    static const uint8_t kEdges[12][4] = {
        {0, 2, 0, 4}, {0, 3, 2, 6}, {0, 4, 0, 2}, {0, 5, 4, 6}, {1, 2, 1, 5}, {1, 3, 3, 7},
        {4, 1, 1, 3}, {5, 1, 5, 7}, {4, 2, 0, 1}, {4, 3, 2, 3}, {5, 3, 6, 7}, {5, 2, 4, 5},
    };
    for (const auto& edge : kEdges) {
        if (facing[edge[0]] == facing[edge[1]]) {
            continue;
        }
        if (ProcessLineSegment(corners[edge[2]], corners[edge[3]], active, screen[0], screen[1], screen[2], 0)) {
            return 0;
        }
    }
    if (double(gMaxXInOccluder) - gMinXInOccluder < 64.0) {
        return 0;
    }
    if (double(gMaxYInOccluder) - gMinYInOccluder < double(33.6f)) {
        return 0;
    }
    return active->numLines != 0 ? 1 : 0;
}

} // namespace COcclusion3

// ---- registration ----------------------------------------------------------------------

#define C16_OCC3_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace COcclusion3 {

C16_OCC3_REVERSED("COccluder::ProcessOneOccluder", ProcessOneOccluder, 0x0051E2E0);

} // namespace COcclusion3
