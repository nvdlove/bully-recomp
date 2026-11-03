// 0x51CC50 to 0x51DDB0: COcclusion (GTA San Andreas's, with Bully's float occluders):
// the line tests on screen, Init and AddOne, the screen projection (a 640x480
// virtual screen, the near plane at 0.25), the active occluders' lines built from an
// occluder's projected edges (ProcessLineSegment in two forms), NearCamera, and
// CEntity::IsEntityOccluded.
//
// (x87 code: the sums are done in double here and stored as float where the game
// stores them; the game keeps them in 80-bit registers in between)

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "reversed.h"
#include "runflags.h"

namespace {

// COccluder (0x24 bytes)
struct Occluder {
    float length;      // +0
    float width;       // +4
    float height;      // +8
    float mid[3];      // +0xC
    uint16_t angle;    // +0x18 (0..0xFFFF for 0..360 degrees)
    int16_t next;      // +0x1A in its list
    uint8_t flag;      // +0x1C
    uint8_t unknown1D[3];
    int32_t area;      // +0x20
};
static_assert(sizeof(Occluder) == 0x24);

struct OccluderLine {
    float x, y;        // +0 the start on screen
    float dx, dy;      // +8 the direction
    float length;      // +0x10
};
static_assert(sizeof(OccluderLine) == 0x14);

// CActiveOccluder (0x80 bytes). A plane occluder (+0x79) keeps its plane at +0x64
// (normal, distance), over the sixth line
struct ActiveOccluder {
    OccluderLine lines[6];  // +0
    uint8_t numLines;       // +0x78
    uint8_t isPlane;        // +0x79
    uint8_t unknown7A[2];
    float distance;         // +0x7C
};
static_assert(sizeof(ActiveOccluder) == 0x80);

// a corner of an occluder: its position, projected, and whether it's in front
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

constexpr int kMaxOccluders = 350;
GAME_ARRAY(Occluder, aOccluders, 0x00C8B790);
GAME_ARRAY(ActiveOccluder, aActiveOccluders, 0x00C89F90);
GAME_VAR(int32_t, NumOccludersOnMap, 0x00C89F8C);
GAME_VAR(int32_t, NumActiveOccluders, 0x00C89F88);
GAME_VAR(int16_t, FarAwayList, 0x00A76B44);
GAME_VAR(int16_t, NearbyList, 0x00A76B48);
GAME_VAR(int16_t, ListWalkThroughFA, 0x00A76B4C);
GAME_VAR(int16_t, PreviousListWalkThroughFA, 0x00A76B50);
GAME_VAR(int16_t, FreeList, 0x00A76B54);
GAME_VAR(float, gMinXInOccluder, 0x00C89F84);
GAME_VAR(float, gMaxXInOccluder, 0x00C89F80);
GAME_VAR(float, gMinYInOccluder, 0x00C89F7C);
GAME_VAR(float, gMaxYInOccluder, 0x00C89F78);
inline float* const gOccluderCoorsMax = reinterpret_cast<float*>(0x00C8E8C8);
inline float* const gOccluderCoorsMin = reinterpret_cast<float*>(0x00C8E8D8);
inline const float* const TheCameraViewMatrix = reinterpret_cast<const float*>(0x00C3CCB0);
GAME_VAR(void*, ms_pCamera_C3CCF4, 0x00C3CCF4);
GAME_VAR(float, ms_fovOverride, 0x00C3CDD0);
GAME_VAR(int32_t, ms_bFovOverride, 0x00C3CDD4);
GAME_VAR(float, ms_fov, 0x00C3CD04);

GAME_FN(float*, __cdecl, MatrixTimesPoint, 0x0045D4A0, float* out, const float* matrix, const float* point);
// (0x0045D500 stores 16 bytes through `out4`: called through re::CallVec16)
static inline float* MatrixTimesPoint4(float* out4, const void* matrix, const float* point) { return re::CallVec16<0x0045D500, float*>(out4, matrix, point); }
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(void, __thiscall, Entity_GetBoundCentre, 0x00466CA0, void* entity, float* out);
GAME_FN(uint8_t*, __cdecl, GetColModel, 0x0051AF50, void* entity);
GAME_FN(void*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, void* self, const void* other);
GAME_FN(uint8_t, __cdecl, DoesInfiniteLineCrossFiniteLine, 0x0051CC50, float x1, float y1, float x2, float y2,
        float lineX, float lineY, float lineDX, float lineDY);
GAME_FN(uint8_t, __cdecl, DoesInfiniteLineTouchScreen, 0x0051CCB0, float x, float y, float dx, float dy);
GAME_FN(uint8_t, __thiscall, IsPointWithinOcclusionArea, 0x0051D070, void* active, float x, float y, float radius);
GAME_FN(void, __thiscall, Vector_Max, 0x0051D0D0, float* self, const float* other);
GAME_FN(void, __thiscall, Vector_Min, 0x0051D150, float* self, const float* other);

constexpr double kNear = 0.25;
constexpr double kClipNear = double(0.275f);   // (the float, widened)

// 0x51D360 / 0x51D210 take `out` in ecx and leave the stack to the caller: called
// through these
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

__declspec(noinline) uint8_t CallCalcScreenCoorsSize(float* result4, float x, float y, float z, uint32_t w, float* sizeX,
                                float* sizeY) {
#if defined(_M_IX86)
    uint8_t result;
    __asm {
        push sizeY
        push sizeX
        push w
        push z
        push y
        push x
        mov ecx, result4
        mov eax, 0x0051D210
        call eax
        add esp, 24
        mov result, al
    }
    return result;
#else
    return re::Ours<uint8_t (*)(float*, float, float, float, uint32_t, float*, float*)>(0x0051D210)(result4, x, y, z, w, sizeX, sizeY);
#endif
}

// the view matrix applied to a point (x, y, z out; w copied)
void ToView(float* out, float x, float y, float z, uint32_t w) {
    const float* m = TheCameraViewMatrix;
    out[0] = float(((double(x) * m[0] + double(y) * m[4]) + double(z) * m[8]) + m[12]);
    out[1] = float(((double(m[1]) * x + double(m[5]) * y) + double(m[9]) * z) + m[13]);
    out[2] = float(((double(m[2]) * x + double(m[6]) * y) + double(m[10]) * z) + m[14]);
    std::memcpy(&out[3], &w, 4);
}

float Sqrt(float value) {
    return float(std::sqrt(double(value)));
}

const float* CameraPosition() {
    return reinterpret_cast<const float*>(GetLocalTransform(GetCameraNode(ms_pCamera_C3CCF4)) + 0x24);
}

} // namespace

namespace COcclusion2 {

// 0x51CC50 (cdecl): whether the infinite line crosses the segment (x1, y1)-(x2, y2):
// the two ends on either side
static uint8_t __cdecl LineCrossesSegment(float x1, float y1, float x2, float y2, float lineX, float lineY,
                                          float lineDX, float lineDY) {
    double a = (double(x1) - lineX) * lineDY - (double(y1) - lineY) * lineDX;
    double b = (double(x2) - lineX) * lineDY - (double(y2) - lineY) * lineDX;
    return a * b < 0.0 ? 1 : 0;
}

// 0x51CCB0 (cdecl): DoesInfiniteLineTouchScreen: a start on the 640x480 screen, or
// the line crossing its left, top, right or bottom edge
static uint8_t __cdecl LineTouchesScreen(float x, float y, float dx, float dy) {
    if (x > 0.0f && y > 0.0f && x < 640.0f && double(y) < 480.0) {
        return 1;
    }
    double a = (0.0 - x) * dy;
    double b = (0.0 - y) * dx;
    double c = a - b;
    if (((640.0 - x) * dy - b) * c < 0.0) {
        return 1;
    }
    if ((a - (480.0 - y) * dx) * c < 0.0) {
        return 1;
    }
    if (DoesInfiniteLineCrossFiniteLine(640.0f, 0.0f, 640.0f, 480.0f, x, y, dx, dy)) {
        return 1;
    }
    return DoesInfiniteLineCrossFiniteLine(0.0f, 480.0f, 640.0f, 480.0f, x, y, dx, dy) ? 1 : 0;
}

// 0x51CE00 (cdecl): Init: no occluders, all 350 on the free list
static void __cdecl Init() {
    NumOccludersOnMap = 0;
    FarAwayList = -1;
    NearbyList = -1;
    ListWalkThroughFA = -1;
    PreviousListWalkThroughFA = -1;
    FreeList = 0;
    for (int i = 0; i < kMaxOccluders; i++) {
        aOccluders[i].next = int16_t(i + 1);
    }
    aOccluders[kMaxOccluders - 1].next = -1;
}

// 0x51CE50 (cdecl): AddOne: an occluder from the free list onto the far-away list.
// (The free list's end is tested against 0xFFFF after sign extension: never true; the
// count's limit is what stops it)
static void __cdecl AddOne(float midX, float midY, float midZ, float width, float length, float height,
                           float angle, int32_t area, int flag) {
    if (NumOccludersOnMap >= kMaxOccluders) {
        return;
    }
    int32_t index = FreeList;
    if (index == 0xFFFF) {
        return;
    }
    Occluder& occluder = aOccluders[index];
    occluder.mid[0] = midX;
    FreeList = occluder.next;
    occluder.mid[1] = midY;
    occluder.mid[2] = midZ;
    occluder.length = length;
    occluder.width = width;
    occluder.height = height;
    while (angle < 0.0f) {
        angle = float(angle + 360.0);
    }
    while (angle > 360.0f) {
        angle = float(angle - 360.0);
    }
    NumOccludersOnMap++;
    occluder.angle = uint16_t(int32_t(double(angle) * 182.04444885253906));
    occluder.area = area;
    occluder.flag = uint8_t(flag);
    occluder.next = FarAwayList;
    FarAwayList = int16_t(index);
}

// 0x51CF90 (cdecl): IsPointInsideLine
static uint8_t __cdecl IsPointInsideLine(float lineX, float lineY, float dx, float dy, float x, float y,
                                         float tolerance) {
    double side = (double(x) - lineX) * dy - (double(y) - lineY) * dx;
    return side < tolerance ? 1 : 0;
}

// 0x51CFC0 (cdecl): OccluderHidesBehind: every line of `active` (its start and end)
// on the inner side of every line of `other`
static uint8_t __cdecl OccluderHidesBehind(ActiveOccluder* active, ActiveOccluder* other) {
    for (int i = 0; i < active->numLines; i++) {
        const OccluderLine& line = active->lines[i];
        for (int j = 0; j < other->numLines; j++) {
            const OccluderLine& o = other->lines[j];
            if ((double(line.x) - o.x) * o.dy - (double(line.y) - o.y) * o.dx < 0.0) {
                return 0;
            }
            float endX = float(double(line.dx) * line.length + line.x);
            float endY = float(double(line.dy) * line.length + line.y);
            if ((double(endX) - o.x) * o.dy - (double(endY) - o.y) * o.dx < 0.0) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x51D070 (thiscall): CActiveOccluder::IsPointWithinOcclusionArea
static uint8_t __fastcall PointWithinOcclusionArea(ActiveOccluder* self, re::Edx, float x, float y, float radius) {
    for (int i = 0; i < self->numLines; i++) {
        const OccluderLine& line = self->lines[i];
        if ((double(x) - line.x) * line.dy - (double(y) - line.y) * line.dx < radius) {
            return 0;
        }
    }
    return 1;
}

// 0x51D0D0 (thiscall): the per-axis maximum, in place
static void __fastcall Max(float* self, re::Edx, const float* other) {
    for (int i = 0; i < 3; i++) {
        self[i] = (other[i] < self[i]) ? self[i] : other[i];
    }
}

// 0x51D150 (thiscall): the per-axis minimum, in place
static void __fastcall Min(float* self, re::Edx, const float* other) {
    for (int i = 0; i < 3; i++) {
        self[i] = (self[i] < other[i]) ? self[i] : other[i];
    }
}

// 0x51D1D0 (thiscall): a corner copied (the w's left out)
static Corner* __fastcall CopyCorner(Corner* self, re::Edx, const Corner* other) {
    self->world[0] = other->world[0];
    self->world[1] = other->world[1];
    self->world[2] = other->world[2];
    self->screen[0] = other->screen[0];
    self->screen[1] = other->screen[1];
    self->screen[2] = other->screen[2];
    self->valid = other->valid;
    return self;
}

// 0x51D210 (`out` in ecx, the caller pops): CalcScreenCoors with the sprite size: the
// point in view space (w copied), false at or behind 0.25; else x and y scaled to the
// 640x480 screen and the sizes for the field of view (70 degrees as 1)
[[maybe_unused]] static uint8_t CalcScreenCoorsSize(float* out, float x, float y, float z, uint32_t w,
                                                    float* sizeX, float* sizeY) {
    ToView(out, x, y, z, w);
    if (!(kNear < out[2])) {
        return 0;
    }
    float recip = float(1.0 / out[2]);
    float sx = float(640.0 * recip);
    float sy = float(recip * 480.0);
    float fov = ms_bFovOverride ? ms_fovOverride : ms_fov;
    float scale = float(70.0 / fov);
    out[0] = float(double(out[0]) * sx);
    out[1] = float(double(out[1]) * sy);
    *sizeX = float(double(scale) * sx);
    *sizeY = float(double(sy) * scale);
    return 1;
}
RE_UNHOOKED_FN("COcclusion::CalcScreenCoorsSize", 0x0051D210, &CalcScreenCoorsSize);

// 0x51D360 (`out` in ecx, the caller pops): CalcScreenCoors
[[maybe_unused]] static uint8_t CalcScreenCoors(float* out, float x, float y, float z, uint32_t w) {
    ToView(out, x, y, z, w);
    if (!(kNear < out[2])) {
        return 0;
    }
    float recip = float(1.0 / out[2]);
    float sx = float(640.0 * recip);
    float sy = float(recip * 480.0);
    out[0] = float(double(out[0]) * sx);
    out[1] = float(double(out[1]) * sy);
    return 1;
}
RE_UNHOOKED_FN("COcclusion::CalcScreenCoors", 0x0051D360, &CalcScreenCoors);

// 0x51D460 (stdcall): a line of an active occluder from two view-space points: both
// behind 0.25 gives nothing (false); one behind is moved along the segment to 0.25.
// Both are projected, the global box of projected points grown (0xC8E8C8 / 0xC8E8D8),
// the centre sum (`centre`) given both, and the line added
static uint8_t __stdcall ProcessLineSegmentView(Vec4 a, Vec4 b, ActiveOccluder* active, float* centre) {
    if (a.z < kNear && b.z < kNear) {
        return 0;
    }
    if (!(a.z < kNear)) {
        if (b.z < kNear) {
            float num = float(kNear - a.z);
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float dz = b.z - a.z;
            double t = double(num) / dz;
            b.x = float(double(a.x) + dx * t);
            b.y = float(double(a.y) + dy * t);
            b.z = 0.25f;
        }
    } else {
        float num = float(kNear - a.z);
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float dz = b.z - a.z;
        double t = double(num) / dz;
        a.x = float(double(a.x) + dx * t);
        a.y = float(double(a.y) + dy * t);
        a.z = 0.25f;
    }
    float ra = float(1.0 / a.z);
    float sxa = float(ra * 640.0);
    float sya = float(ra * 480.0);
    Vec4 pa = {float(double(sxa) * a.x), float(double(sya) * a.y), a.z, a.w};
    float rb = float(1.0 / b.z);
    float sxb = float(640.0 * rb);
    float syb = float(rb * 480.0);
    Vec4 pb = {float(double(sxb) * b.x), float(double(syb) * b.y), b.z, b.w};
    Vector_Max(gOccluderCoorsMax, &pa.x);
    Vector_Max(gOccluderCoorsMax, &pb.x);
    Vector_Min(gOccluderCoorsMin, &pa.x);
    Vector_Min(gOccluderCoorsMin, &pb.x);
    centre[0] = float(double(float(double(centre[0]) + pa.x)) + pb.x);
    centre[1] = float(double(float(double(centre[1]) + pa.y)) + pb.y);
    centre[2] = float(double(float(double(centre[2]) + pa.z)) + pb.z);
    float dx = pb.x - pa.x;
    float dy = pb.y - pa.y;
    float length = Sqrt(std::fabs(float(double(dx) * dx + double(dy) * dy)));
    OccluderLine& line = active->lines[active->numLines];
    line.length = length;
    line.x = pa.x;
    line.y = pa.y;
    line.dx = dx;
    line.dy = dy;
    active->numLines++;
    return 1;
}

// the point at the near plane between two corners, weighted as the game does
void Interpolate(float* out, const Corner& a, float weightA, const Corner& b, float weightB) {
    float bx = float(double(b.world[0]) * weightB);
    float by = float(double(b.world[1]) * weightB);
    float bz = float(double(weightB) * b.world[2]);
    float ax = float(double(a.world[0]) * weightA);
    float ay = float(double(a.world[1]) * weightA);
    float az = float(double(weightA) * a.world[2]);
    out[0] = ax + bx;
    out[1] = ay + by;
    out[2] = az + bz;
}

// |view z - 0.275| of a corner
float ClipDistance(const Corner& corner) {
    float view[4];
    float z = MatrixTimesPoint(view, TheCameraViewMatrix, corner.world)[2];
    return std::fabs(float(z - kClipNear));
}

// 0x51D780 (stdcall): COccluder::ProcessLineSegment for two corners: a corner behind
// the camera is moved to the near plane (0.275) and projected (true when that fails);
// the screen box (0xC89F78..84) grown; the line added pointing away from the centre
// (cx, cy) when it touches the screen (false). A line off screen gives true when the
// screen's centre is outside it, else false. Nothing for two corners behind (false).
// (The second corner is clipped with the weights the wrong way round. The last two
// arguments are unused)
static uint8_t __stdcall ProcessLineSegment(Corner a, Corner b, ActiveOccluder* active, float cx, float cy, int,
                                            int) {
    float screenA[4];
    float screenB[4];
    if (!a.valid) {
        if (!b.valid) {
            return 0;
        }
        float d1 = ClipDistance(a);
        float d2 = ClipDistance(b);
        float t = float(double(d2) / (double(d1) + d2));
        float u = float(1.0 - t);
        float p[3];
        Interpolate(p, a, t, b, u);
        uint32_t w = reinterpret_cast<uint32_t>(&a);   // (whatever was on the stack)
        if (!CallCalcScreenCoors(screenA, p[0], p[1], p[2], w)) {
            return 1;
        }
    } else {
        std::memcpy(screenA, a.screen, sizeof(screenA));
    }
    if (b.valid) {
        std::memcpy(screenB, b.screen, sizeof(screenB));
    } else {
        float d1 = ClipDistance(a);
        float d2 = ClipDistance(b);
        float s = float(double(d1) / (double(d2) + d1));
        float v = float(1.0 - s);
        float p[3];
        Interpolate(p, a, s, b, v);
        uint32_t w = reinterpret_cast<uint32_t>(&a);
        if (!CallCalcScreenCoors(screenB, p[0], p[1], p[2], w)) {
            return 1;
        }
    }

    float m = (gMinXInOccluder < screenA[0]) ? gMinXInOccluder : screenA[0];
    gMinXInOccluder = (m < screenB[0]) ? m : screenB[0];
    m = (gMaxXInOccluder > screenA[0]) ? gMaxXInOccluder : screenA[0];
    gMaxXInOccluder = (m > screenB[0]) ? m : screenB[0];
    m = (gMinYInOccluder < screenA[1]) ? gMinYInOccluder : screenA[1];
    gMinYInOccluder = (m < screenB[1]) ? m : screenB[1];
    m = (gMaxYInOccluder > screenA[1]) ? gMaxYInOccluder : screenA[1];
    gMaxYInOccluder = (m > screenB[1]) ? m : screenB[1];

    float startX = screenA[0];
    float startY = screenA[1];
    float dx = screenB[0] - screenA[0];
    float dy = screenB[1] - screenA[1];
    if ((double(cx) - startX) * dy - (double(cy) - startY) * dx < 0.0) {
        startX = startX + dx;
        startY = startY + dy;
        dx = -dx;
        dy = -dy;
    }
    float length = Sqrt(std::fabs(float(double(dx) * dx + double(dy) * dy)));
    OccluderLine& line = active->lines[active->numLines];
    line.length = length;
    float ndx = dx / line.length;
    float ndy = dy / line.length;
    line.x = startX;
    line.y = startY;
    line.dx = ndx;
    line.dy = ndy;
    if (DoesInfiniteLineTouchScreen(startX, startY, ndx, ndy)) {
        active->numLines++;
        return 0;
    }
    return ((320.0 - startX) * ndy - (240.0 - startY) * ndx < 0.0) ? 1 : 0;
}

// 0x51DCE0 (thiscall): COccluder::NearCamera: within 250 of the camera, less half its
// larger side
static uint8_t __fastcall NearCamera(Occluder* self, re::Edx) {
    float centre[3] = {self->mid[0], self->mid[1], self->mid[2]};
    float d[3];
    VectorSub(d, CameraPosition(), centre);
    float distance = Sqrt(std::fabs(float((double(d[0]) * d[0] + double(d[1]) * d[1]) + double(d[2]) * d[2])));
    float side = (self->width < self->length) ? self->length : self->width;
    return (double(distance) - side * 0.5 < 250.0) ? 1 : 0;
}

// 0x51DDB0 (thiscall): CEntity::IsEntityOccluded: the bound centre on screen, then
// per active occluder in front of it (or whose plane the view line crosses far
// enough out): inside all its lines with a margin of 0.9 of the projected radius, or
// inside with the four projected corners of the bounding box (worked out once) all
// inside every line
static uint8_t __fastcall IsEntityOccluded(uint8_t* self, re::Edx) {
    // (the remaster's seamless doors scan another area from another camera: the active occluders
    // are the main camera's, on its screen, not that one's: runflags.h)
    if (NumActiveOccluders == 0 || *reinterpret_cast<uint32_t*>(self + 0xEC) != 0 || re::PortalScan()) {
        return 0;
    }
    float pos[3];
    Entity_GetBoundCentre(self, pos);
    float screen[4];
    float sizeX;
    float sizeY;
    uint32_t garbage = 0;   // (w: whatever was on the stack)
    if (!CallCalcScreenCoorsSize(screen, pos[0], pos[1], pos[2], garbage, &sizeX, &sizeY)) {
        return 0;
    }
    const uint8_t* colModel = GetColModel(self);
    const float* boxMin = reinterpret_cast<const float*>(colModel + 0x10);
    const float* boxMax = reinterpret_cast<const float*>(colModel + 0x20);
    float min[3] = {boxMin[0], boxMin[1], boxMin[2]};
    float max[3] = {boxMax[0], boxMax[1], boxMax[2]};
    float radius = *reinterpret_cast<const float*>(colModel + 0xC);
    float size = (sizeY < sizeX) ? sizeX : sizeY;
    float margin = float(double(size) * radius * double(0.9f));
    const float* camera = CameraPosition();
    float cam[3] = {camera[0], camera[1], camera[2]};
    float dx = cam[0] - pos[0];
    float dy = cam[1] - pos[1];
    float dz = cam[2] - pos[2];
    float distance = Sqrt(std::fabs(float((double(dx) * dx + double(dy) * dy) + double(dz) * dz)));
    float nearZ = float(double(screen[2]) - radius * double(0.85f));

    bool cornersDone = false;
    float corners[4][4];
    for (int i = 0; i < NumActiveOccluders; i++) {
        ActiveOccluder& active = aActiveOccluders[i];
        if (!active.isPlane) {
            if (!(active.distance < nearZ)) {
                continue;
            }
        } else {
            const float* plane = &active.lines[5].x;   // (+0x64)
            float nx = plane[0];
            float ny = plane[1];
            float nz = plane[2];
            float d = plane[3];
            float denominator = float((double(nx) * dx + double(ny) * dy) + double(nz) * dz);
            if (denominator == 0.0f) {
                continue;
            }
            float numerator = float((double(ny) * cam[1] + double(nx) * cam[0]) + double(nz) * cam[2]);
            float t = float((double(numerator) + d) / denominator);
            if (!(t > 0.0f) || !(t < 1.0f)) {
                continue;
            }
            if (!(radius < double(distance) - double(t) * distance)) {
                continue;
            }
        }
        if (IsPointWithinOcclusionArea(&active, screen[0], screen[1], margin)) {
            return 1;
        }
        if (!IsPointWithinOcclusionArea(&active, screen[0], screen[1], 0.0f)) {
            continue;
        }
        if (!cornersDone) {
            uint8_t matrix[0x44];
            Matrix_CopyConstruct(matrix, Placeable_GetMatrix(self));
            float worldMin[4];
            float worldMax[4];
            MatrixTimesPoint4(worldMin, matrix, min);
            MatrixTimesPoint4(worldMax, matrix, max);
            CallCalcScreenCoors(corners[0], worldMin[0], worldMin[1], worldMin[2], garbage);
            CallCalcScreenCoors(corners[1], worldMax[0], worldMax[1], worldMin[2], garbage);
            CallCalcScreenCoors(corners[2], worldMax[0], worldMin[1], worldMax[2], garbage);
            CallCalcScreenCoors(corners[3], worldMin[0], worldMax[1], worldMax[2], garbage);
            cornersDone = true;
        }
        if (active.numLines == 0) {
            return 1;
        }
        bool inside = true;
        for (int j = 0; j < active.numLines && inside; j++) {
            const OccluderLine& line = active.lines[j];
            inside = false;
            bool all = true;
            for (const float* corner : corners) {
                if ((double(corner[0]) - line.x) * line.dy - (double(corner[1]) - line.y) * line.dx < 0.0) {
                    all = false;
                    break;
                }
            }
            inside = all;
        }
        if (inside) {
            return 1;
        }
    }
    return 0;
}

} // namespace COcclusion2

// ---- registration ----------------------------------------------------------------------

#define C16_OCC2_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace COcclusion2 {

C16_OCC2_REVERSED("COcclusion::DoesInfiniteLineCrossFiniteLine", LineCrossesSegment, 0x0051CC50);
C16_OCC2_REVERSED("COcclusion::DoesInfiniteLineTouchScreen", LineTouchesScreen, 0x0051CCB0);
C16_OCC2_REVERSED("COcclusion::Init", Init, 0x0051CE00);
C16_OCC2_REVERSED("COcclusion::AddOne", AddOne, 0x0051CE50);
C16_OCC2_REVERSED("COcclusion::IsPointInsideLine", IsPointInsideLine, 0x0051CF90);
C16_OCC2_REVERSED("COcclusion::OccluderHidesBehind", OccluderHidesBehind, 0x0051CFC0);
C16_OCC2_REVERSED("CActiveOccluder::IsPointWithinOcclusionArea", PointWithinOcclusionArea, 0x0051D070);
C16_OCC2_REVERSED("CVector::Max_51D0D0", Max, 0x0051D0D0);
C16_OCC2_REVERSED("CVector::Min_51D150", Min, 0x0051D150);
C16_OCC2_REVERSED("COccluderCorner::Copy", CopyCorner, 0x0051D1D0);
C16_OCC2_REVERSED("COccluder::ProcessLineSegmentView", ProcessLineSegmentView, 0x0051D460);
C16_OCC2_REVERSED("COccluder::ProcessLineSegment", ProcessLineSegment, 0x0051D780);
C16_OCC2_REVERSED("COccluder::NearCamera", NearCamera, 0x0051DCE0);
C16_OCC2_REVERSED("CEntity::IsEntityOccluded", IsEntityOccluded, 0x0051DDB0);

} // namespace COcclusion2
