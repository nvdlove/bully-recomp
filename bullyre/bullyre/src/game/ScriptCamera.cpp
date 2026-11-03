#include "reversed.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

// The script camera's processing (ScriptCameraController::vf7), the look-at matrix
// every controller builds, the conversation-camera spline set up from two points, and
// the spline interpolater itself: points built from a path (each point's w: the
// length of the segment before it, measured along the curve), the speed along it
// (accelerate, then brake to stop at the end) and the position between the
// control points.
//
// The x87 code keeps sums in extended precision between float stores; the values
// here are computed in double and rounded where the game stores them.

#define SCRIPTCAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

struct Vec3 {
    float x, y, z;
};
static_assert(sizeof(Vec3) == 0xC);

// a spline point: the position and the curve's length from the previous point
struct SplinePoint {
    float x, y, z, w;
};
static_assert(sizeof(SplinePoint) == 0x10);

// a growable array of points ({data, capacity, count})
struct SplinePoints {
    re::Ptr32<SplinePoint> data;   // +0
    uint32_t capacity;   // +4
    uint32_t count;      // +8
};
static_assert(sizeof(SplinePoints) == 0xC);

// SplineInterpolater (Camera.h's, with the fields named)
struct Spline {
    re::Ptr32<void> vtable;          // +0 (0x9146FC)
    Vec3 position;         // +4 where it is now
    Vec3 control0;         // +0x10 the current segment's ends, pulled out to
    Vec3 control1;         // +0x1C  the segment's length (0x5086C0)
    int32_t segment;       // +0x28 the segment runs from points[segment + 1] to [+ 2]
    float acceleration;    // +0x2C
    float deceleration;    // +0x30
    float maxSpeed;        // +0x34
    float speed;           // +0x38
    float distance;        // +0x3C along the segment
    float t;               // +0x40 distance / the segment's length
    SplinePoints points;   // +0x44
};
static_assert(offsetof(Spline, segment) == 0x28);
static_assert(offsetof(Spline, points) == 0x44);
static_assert(sizeof(Spline) == 0x50);

struct NiTransform {
    float rotate[9];   // +0
    Vec3 translate;    // +0x24
    float scale;       // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

using re::At;

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, VectorConstructorIterator, 0x00401240, void* array, size_t size, size_t count, void* ctor);
GAME_FN(Vec3*, __cdecl, VectorSub, 0x00414B70, Vec3* out, const Vec3* a, const Vec3* b);   // a - b
GAME_FN(Vec3*, __cdecl, VectorAdd, 0x004129A0, Vec3* out, const Vec3* a, const Vec3* b);
GAME_FN(Vec3*, __cdecl, CrossProduct, 0x00414730, Vec3* out, const Vec3* a, const Vec3* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, Vec3* v);
GAME_FN(float, __thiscall, Vector_Normalise415130, 0x00415130, Vec3* v);
GAME_FN(void, __cdecl, RotateAboutAxis, 0x00415EB0, NiTransform* t, const Vec3* axis, float angle, int mode);
GAME_FN(Vec3*, __thiscall, Matrix_Transform, 0x00450B10, const NiTransform* m, Vec3* out, const Vec3* v);
GAME_FN(void, __thiscall, NiQuaternion_FromRotation, 0x00776040, float* q, const void* rotation);
GAME_FN(float*, __cdecl, NiQuaternion_Slerp, 0x007766E0, float* out, float t, const float* a, const float* b);
GAME_FN(void, __thiscall, NiQuaternion_ToRotation, 0x00413C50, const float* q, void* out);
GAME_FN(Vec3*, __cdecl, CatmullRom, 0x00412400, Vec3* out, const Vec3* p0, const Vec3* p1, const Vec3* p2,
        const Vec3* p3, float t);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(uint8_t*, __cdecl, Script_GetEntity, 0x005C28D0, int handle, int kind, int pedItself, int cutscene);
GAME_FN(void*, __cdecl, GetRwObjectFrame, 0x005EE1A0, void* rwObject);
GAME_FN(uint8_t*, __cdecl, GetHierarchy, 0x006C7710, void* frame);
GAME_FN(Vec3*, __cdecl, Script_GetPosition, 0x005C2970, Vec3* out, int handle, int kind);
GAME_FN(void, __thiscall, Sweep_Construct, 0x00563060, void* self, const Vec3* from, const Vec3* to, float radius,
        int flags, void* hit, void* filter, int a, int b, uintptr_t c);
GAME_FN(int32_t, __thiscall, Sweep_Test, 0x00564080, void* self);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
// the paths (a path store and its paths' points)
GAME_FN(void*, __thiscall, ActFiles_GetCurrent, 0x005DB960, void* files);
GAME_FN(int32_t, __thiscall, PathStore_Find, 0x005ED310, void* store, const char* name);
GAME_FN(void*, __thiscall, PathStore_Get, 0x005ED360, void* store, const char* name);
GAME_FN(void*, __cdecl, Path_New, 0x005EC820, size_t size);
GAME_FN(void*, __thiscall, Path_Construct, 0x005EC5E0, void* path, int value);
GAME_FN(void, __thiscall, Path_SetName, 0x005EC610, void* path, const char* name);
GAME_FN(void, __thiscall, Path_Clear, 0x005ECBA0, void* path);
GAME_FN(void, __thiscall, PathPoint_Construct, 0x005EC570, void* point);
GAME_FN(void, __thiscall, PathPoint_Destruct, 0x005EC7B0, void* point);
GAME_FN(void, __thiscall, Path_AddPoint, 0x005ECB20, void* path, void* point);
GAME_FN(uint32_t, __thiscall, Path_GetCount, 0x005EC670, void* path);
GAME_FN(Vec3*, __thiscall, Path_GetPoint, 0x005EC8C0, void* path, uint32_t index);
// ours, called where modes apply
GAME_FN(void, __thiscall, CameraController_LookAtFn, 0x00507AC0, void* self, float* matrix, Vec3 position,
        Vec3 target);
GAME_FN(void, __thiscall, ScriptCameraController_Unknown507970, 0x00507970, void* self, void* path, int value);
GAME_FN(void, __thiscall, ScriptCameraController_Clear, 0x00507670, void* self);
GAME_FN(void, __thiscall, SplinePoints_Reserve, 0x00508450, SplinePoints* self, uint32_t capacity, int setCount);
GAME_FN(void, __thiscall, Spline_UpdateSpeed, 0x00508530, Spline* self, float dt);
GAME_FN(void, __thiscall, Spline_FitControls, 0x005086C0, void* self, Vec3* control0, Vec3 p1, Vec3 p2,
        Vec3* control1);
GAME_FN(void, __thiscall, Spline_NextSegment, 0x005090E0, Spline* self);
GAME_FN(void, __thiscall, Spline_Advance, 0x00509280, Spline* self, float dt);
GAME_FN(void, __thiscall, Spline_Update, 0x005092F0, Spline* self, float dt);
GAME_FN(int32_t, __thiscall, Spline_GetCount, 0x0051A7B0, Spline* self);
GAME_FN(Vec3*, __thiscall, Spline_GetPosition, 0x00508380, Spline* self, Vec3* out);

GAME_VAR(void*, theNiCamera, 0x00C3CCF4);            // TheCamera.camera
GAME_VAR(uint8_t*, cameraTargetPed, 0x00C3CE14);     // TheCamera.targetPed
GAME_VAR(uint8_t, cameraFovLocked, 0x00C3CF82);      // TheCamera+0x31A
GAME_VAR(int32_t, cameraFovOverridden, 0x00C3CDD4);  // TheCamera.fovOverridden
GAME_VAR(float, cameraFov, 0x00C3CD04);              // TheCamera.fov
GAME_VAR(Vec3, cameraUnknown18C, 0x00C3CDF4);        // TheCamera+0x18C
GAME_VAR(Vec3, cameraLookAt, 0x00C3CE00);            // TheCamera+0x198
GAME_VAR(uint16_t, currentScanCode, 0x00C1AE88);     // CWorld::ms_nCurrentScanCode
GAME_VAR(uint8_t, testingSphere, 0x00C1AE8A);
GAME_VAR(void*, ignoreEntity, 0x00C1AE70);           // CWorld::pIgnoreEntity
GAME_VAR(void*, thePathStore, 0x00D13CB4);
inline void* const colPoints = reinterpret_cast<void*>(0x00C216C0);
inline void* const theActFiles = reinterpret_cast<void*>(0x00D02850);
constexpr uintptr_t kSweepFilter = 0x0045EE90;
constexpr uintptr_t kSplineVtable = 0x009146FC;

// |v| as the game rounds it (the squared length and the root stored as floats)
float Length(const Vec3& v) {
    float squared = float(double(v.x) * v.x + double(v.y) * v.y + double(v.z) * v.z);
    squared = std::fabs(squared);
    return float(std::sqrt(double(squared)));
}

int ClampIndex(int i, int last) {
    if (i > last) {
        return last;
    }
    return i < 0 ? 0 : i;
}

void PushPoint(Spline* self, const SplinePoint& point) {
    SplinePoints& points = self->points;
    if (points.count == points.capacity) {
        SplinePoints_Reserve(&points, points.capacity ? points.capacity * 2 : 1, 0);
    }
    points.data[points.count] = point;
    points.count++;
}

// the path point index from a float (clamped to 0..count-1, truncated)
uint32_t PathIndex(void* path, float value) {
    float last = float(int32_t(Path_GetCount(path) - 1));
    float clamped;
    if (value < 0.0f) {
        clamped = 0.0f;
    } else if (last < value) {
        clamped = last;
    } else {
        clamped = value;
    }
    return uint32_t(int32_t(clamped));
}

// the segment's control points from the points around it
void SetControls(Spline* self) {
    int last = int(self->points.count) - 1;
    SplinePoint* data = self->points.data;
    const SplinePoint& a = data[ClampIndex(self->segment, last)];
    self->control0 = {a.x, a.y, a.z};
    const SplinePoint& b = data[ClampIndex(self->segment + 3, last)];
    self->control1 = {b.x, b.y, b.z};
    const SplinePoint& p2 = data[ClampIndex(self->segment + 2, last)];
    const SplinePoint& p1 = data[ClampIndex(self->segment + 1, last)];
    Spline_FitControls(self, &self->control0, Vec3{p1.x, p1.y, p1.z}, Vec3{p2.x, p2.y, p2.z}, &self->control1);
}

} // namespace

// 0x507AC0 (thiscall, `this` unused): a look-at rotation into `matrix` (+0x24: the
// position): forward (column 2) to the target, right (column 0) = up x forward with
// z up, up (column 1) = forward x right
void __fastcall CameraController_LookAt(void*, re::Edx, float* matrix, Vec3 position, Vec3 target) {
    matrix[9] = position.x;
    matrix[10] = position.y;
    matrix[11] = position.z;
    Vec3 tmp;
    Vec3 forward = *VectorSub(&tmp, &target, &position);
    Vector_Normalise(&forward);
    matrix[2] = forward.x;
    matrix[5] = forward.y;
    matrix[8] = forward.z;
    Vec3 up = {0.0f, 0.0f, 1.0f};
    Vec3 right = *CrossProduct(&tmp, &up, &forward);
    Vector_Normalise(&right);
    matrix[0] = right.x;
    matrix[3] = right.y;
    matrix[6] = right.z;
    Vec3 newUp = *CrossProduct(&tmp, &forward, &right);
    Vector_Normalise(&newUp);
    matrix[1] = newUp.x;
    matrix[4] = newUp.y;
    matrix[7] = newUp.z;
}
SCRIPTCAM_REVERSED("CameraController::LookAt_507AC0", 0x00507AC0, RE_BODY(&CameraController_LookAt), nullptr, nullptr, re::Mode::Original);

// 0x507C60: a camera from `from` looking at `to` (the conversation camera): with
// `sweep`, `from` is pulled in to where a 0.3 sphere from `to` hits the world (the
// target ped skipped). The position goes into the "ConversationPath" path (made when
// missing) and the controller follows it
void __fastcall ScriptCameraController_SetFromTo(uint8_t* self, re::Edx, float fromX, float fromY, float fromZ,
                                                 float toX, float toY, float toZ, int value, int sweep) {
    Vec3 from = {fromX, fromY, fromZ};
    if (re::Flag(sweep)) {
        Vec3 to = {toX, toY, toZ};
        Vec3 start = {fromX, fromY, fromZ};
        alignas(4) uint8_t test[0x48];
        testingSphere = 1;
        Sweep_Construct(test, &to, &start, 0.30000001192092896f, 0x8B, colPoints, reinterpret_cast<void*>(kSweepFilter),
                        0, 0, 0);
        At<uint16_t>(cameraTargetPed, 0x10A) = uint16_t(currentScanCode + 1);
        ignoreEntity = nullptr;
        if (Sweep_Test(test) != 0) {
            float fraction = At<float>(test, 0x3C);
            Vec3 tmp;
            const Vec3* d = VectorSub(&tmp, &start, &to);
            Vec3 scaled;
            scaled.y = float(double(fraction) * d->y);
            scaled.z = float(double(d->z) * fraction);
            scaled.x = float(double(fraction) * d->x);
            Vec3 result;
            VectorAdd(&result, &to, &scaled);
            from = result;
        }
        testingSphere = 0;
        EmptyThiscall(test);
    }
    uint8_t* act = static_cast<uint8_t*>(ActFiles_GetCurrent(theActFiles));
    int32_t actValue = -2;
    if (act != nullptr) {
        actValue = At<int32_t>(act, 0x44);
    }
    void* path;
    if (PathStore_Find(thePathStore, "ConversationPath") == -1) {
        void* p = Path_New(0x14);
        if (p != nullptr) {
            path = Path_Construct(p, actValue);
            Path_SetName(path, "ConversationPath");
        } else {
            path = nullptr;
            Path_SetName(path, "ConversationPath");   // (on nullptr, as the game does)
        }
    } else {
        path = PathStore_Get(thePathStore, "ConversationPath");
    }
    Path_Clear(path);
    alignas(4) uint8_t point[0x30];
    PathPoint_Construct(point);
    At<Vec3>(point, 0) = from;
    Path_AddPoint(path, point);
    int byteValue = value & 0xFF;
    ScriptCameraController_Unknown507970(self, path, byteValue);
    ScriptCameraController_Clear(self);
    At<float>(self, 0xC) = 0.0f;
    At<float>(self, 0xFC) = 0.0f;
    At<float>(self, 0x90) = toX;
    At<float>(self, 0x94) = toY;
    At<int32_t>(self, 0x8C) = 1;
    At<float>(self, 0x98) = toZ;
    At<int32_t>(self, 4) = byteValue;
    At<uint8_t>(self, 0x100) = 0;
    PathPoint_Destruct(point);
}
SCRIPTCAM_REVERSED("ScriptCameraController::SetFromTo_507C60", 0x00507C60, RE_BODY(&ScriptCameraController_SetFromTo), nullptr, nullptr, re::Mode::Original);

// 0x507ED0 (vtable[7]): the target (a point, a script entity's position, or the first
// spline), the position along the second spline (plus the offset +0xF4 points at,
// turned by +0xF8 about z around the target), then the camera: looking straight at
// it, or (while +4 is 0 and +0xC under 1) turning towards that view by 0.05 a frame
void __fastcall ScriptCameraController_Process(uint8_t* self, re::Edx) {
    Vec3 target = {0.0f, 0.0f, 0.0f};
    if (At<int32_t>(self, 0x8C) != 0) {
        target = At<Vec3>(self, 0x90);
    } else {
        int32_t handle = At<int32_t>(self, 0x9C);
        if (handle == -1) {
            Spline* spline = reinterpret_cast<Spline*>(self + 0x3C);
            if (Spline_GetCount(spline) <= 0) {
                return;
            }
            Spline_Update(spline, 0.0333000011742115f);
            Vec3 tmp;
            target = *Spline_GetPosition(spline, &tmp);
        } else if (At<uint8_t>(self, 0x100) != 0) {
            uint8_t* entity = Script_GetEntity(handle, At<int32_t>(self, 0xA0), 0, 0);
            uint8_t* hierarchy = GetHierarchy(GetRwObjectFrame(At<void*>(entity, 0x18)));
            target = At<Vec3>(hierarchy, 0x8C);
        } else {
            Vec3 tmp;
            const Vec3* p = Script_GetPosition(&tmp, handle, At<int32_t>(self, 0xA0));
            target.x = p->x;
            target.y = p->y;
            target.z = float(double(p->z) + 0.5);
        }
    }
    Spline* path = reinterpret_cast<Spline*>(self + 0xA4);
    Spline_Update(path, 0.0333000011742115f);
    Vec3 position;
    Spline_GetPosition(path, &position);
    if (const Vec3* offset = At<Vec3*>(self, 0xF4)) {
        Vec3 tmp1, tmp2;
        const Vec3* d = VectorSub(&tmp1, &position, offset);
        position = *VectorAdd(&tmp2, d, &target);
    }
    if (At<float>(self, 0xF8) != 0.0f) {
        Vec3 tmp;
        Vec3 d = *VectorSub(&tmp, &target, &position);
        Vec3 axis = {0.0f, 0.0f, 1.0f};
        NiTransform m;
        m.scale = 1.0f;
        RotateAboutAxis(&m, &axis, At<float>(self, 0xF8), 0);
        Vec3 tmp2;
        d = *Matrix_Transform(&m, &tmp2, &d);
        Vec3 tmp3;
        target = *VectorAdd(&tmp3, &position, &d);
    }
    target.z = At<float>(self, 0xFC) + target.z;
    float* transform = GetLocalTransform(GetCameraNode(theNiCamera));
    Vec3 lookAt;
    if (At<int32_t>(self, 4) == 0 && 1.0f > At<float>(self, 0xC)) {
        NiTransform m;
        m.scale = 1.0f;
        CameraController_LookAtFn(self, m.rotate, position, target);
        float from[4], to[4];
        NiQuaternion_FromRotation(from, transform);
        NiQuaternion_FromRotation(to, &m);
        float t = float(double(At<float>(self, 0xC)) + 0.05000000074505806);
        At<float>(self, 0xC) = t;
        float out[4];
        const float* q = NiQuaternion_Slerp(out, t, from, to);
        float blended[4] = {q[0], q[1], q[2], q[3]};
        NiQuaternion_ToRotation(blended, transform);
        transform[12] = 1.0f;
        transform[9] = position.x;
        transform[10] = position.y;
        transform[11] = position.z;
        Vec3 forward = {transform[2], transform[5], transform[8]};
        Vec3 tmp;
        float distance = Length(*VectorSub(&tmp, &target, &position));
        Vec3 scaled;
        scaled.x = float(double(distance) * forward.x);
        scaled.y = float(double(forward.y) * distance);
        scaled.z = float(double(distance) * forward.z);
        Vec3 tmp2;
        lookAt = *VectorAdd(&tmp2, &position, &scaled);
    } else {
        CameraController_LookAtFn(self, transform, position, target);
        lookAt = target;
    }
    if (cameraFovLocked == 0) {
        cameraFovOverridden = 0;
        cameraFov = At<float>(self, 0x104);
    }
    cameraLookAt = lookAt;
    cameraUnknown18C = {0.0f, 0.0f, 0.0f};
}
SCRIPTCAM_REVERSED("ScriptCameraController::vf7_507ED0", 0x00507ED0, RE_BODY(&ScriptCameraController_Process), nullptr, nullptr, re::Mode::Original);

// ---- the spline

// 0x508450: the point array to `capacity` (points copied over; the count set to it
// when asked)
void __fastcall SplinePoints_ReserveOurs(SplinePoints* self, re::Edx, uint32_t capacity, int setCount) {
    SplinePoint* data = nullptr;
    self->capacity = capacity;
    if (capacity != 0) {
        uint64_t bytes = uint64_t(capacity) * sizeof(SplinePoint);
        size_t size = (bytes >> 32) != 0 ? 0xFFFFFFFFu : size_t(bytes);
        data = static_cast<SplinePoint*>(Malloc(size));
        if (data != nullptr) {
            VectorConstructorIterator(data, sizeof(SplinePoint), capacity, reinterpret_cast<void*>(0x00508410));
        } else {
            data = nullptr;
        }
    }
    if (self->data != nullptr) {
        for (uint32_t i = 0; i < self->count; i++) {
            data[i] = self->data[i];
        }
        Free(self->data);
    }
    self->data = data;
    if (re::Flag(setCount)) {
        self->count = self->capacity;
    }
}
SCRIPTCAM_REVERSED("SplinePoints::Reserve_508450", 0x00508450, RE_BODY(&SplinePoints_ReserveOurs), nullptr, nullptr, re::Mode::Original);

// 0x508530: the speed: accelerating while the stopping distance is under what's left
// of the path, braking otherwise, kept to 0..maxSpeed. (What's left counts the
// next point's length twice.)
void __fastcall Spline_UpdateSpeedOurs(Spline* self, re::Edx, float dt) {
    int last = int(self->points.count) - 1;
    int i = self->segment + 2;
    const SplinePoint* data = self->points.data;
    float left = float(double(data[ClampIndex(i, last)].w) - self->distance);
    if (last - i >= 4) {
        do {
            left = float(double(data[ClampIndex(i, last)].w) + left);
            left = float(double(data[ClampIndex(i + 1, last)].w) + left);
            left = float(double(data[ClampIndex(i + 2, last)].w) + left);
            left = float(double(data[ClampIndex(i + 3, last)].w) + left);
            i += 4;
        } while (i < last - 3);
    }
    if (i < last) {
        int last2 = int(self->points.count) - 1;
        const SplinePoint* data2 = self->points.data;
        do {
            left = float(double(data2[ClampIndex(i, last2)].w) + left);
            i++;
        } while (i < last);
    }
    float q = float(double(self->speed) / self->deceleration);
    double half = double(self->deceleration) * 0.5;
    float stopping = float(double(q) * self->speed + double(q) * half * q);
    double a;
    if (!(stopping >= left)) {
        a = self->acceleration;
    } else {
        a = -double(self->deceleration);
    }
    self->speed = float(a * dt + self->speed);
    if (self->maxSpeed < self->speed) {
        self->speed = self->maxSpeed;
    } else if (0.0f > self->speed) {
        self->speed = 0.0f;
    }
}
SCRIPTCAM_REVERSED("SplineInterpolater::UpdateSpeed_508530", 0x00508530, RE_BODY(&Spline_UpdateSpeedOurs), nullptr, nullptr, re::Mode::Original);

// 0x5086C0 (thiscall, `this` unused): both controls pulled (along their direction
// from their end) to the segment's length from p1 / p2 (a zero-length pull stays)
void __fastcall Spline_FitControlsOurs(void*, re::Edx, Vec3* control0, Vec3 p1, Vec3 p2, Vec3* control1) {
    Vec3 tmp;
    float length = Length(*VectorSub(&tmp, &p1, &p2));
    if (Length(*VectorSub(&tmp, control0, &p1)) != 0.0f) {
        Vec3 base = p1;
        Vec3 dir = *VectorSub(&tmp, control0, &p1);
        Vector_Normalise415130(&dir);
        Vec3 scaled;
        scaled.x = float(double(length) * dir.x);
        scaled.y = float(double(dir.y) * length);
        scaled.z = float(double(length) * dir.z);
        control0->x = float(double(scaled.x) + base.x);
        control0->y = float(double(base.y) + scaled.y);
        control0->z = float(double(base.z) + scaled.z);
    }
    if (Length(*VectorSub(&tmp, control1, &p2)) != 0.0f) {
        Vec3 base = p2;
        Vec3 dir = *VectorSub(&tmp, control1, &p2);
        Vector_Normalise415130(&dir);
        Vec3 scaled;
        scaled.x = float(double(length) * dir.x);
        scaled.y = float(double(dir.y) * length);
        scaled.z = float(double(length) * dir.z);
        control1->x = float(double(scaled.x) + base.x);
        control1->y = float(double(base.y) + scaled.y);
        control1->z = float(double(base.z) + scaled.z);
    }
}
SCRIPTCAM_REVERSED("SplineInterpolater::FitControls_5086C0", 0x005086C0, RE_BODY(&Spline_FitControlsOurs), nullptr, nullptr, re::Mode::Original);

// 0x508930: the points from a path: a lead-in 5 back from the first point, the
// first point, then each further point with its segment's length along the curve
// (sampled in 2, 3, ... steps until it changes by 0.1 or less); then the first
// segment's controls
void __fastcall Spline_Build(Spline* self, re::Edx, void* path) {
    Vec3 first = *Path_GetPoint(path, 0);
    Vec3* a = Path_GetPoint(path, 0);
    Vec3* b = Path_GetPoint(path, 1);
    Vec3 tmp;
    Vec3 dir = *VectorSub(&tmp, b, a);
    Vector_Normalise415130(&dir);
    Vec3 back;
    back.x = float(double(dir.x) * -1.0);
    back.y = float(double(dir.y) * -1.0);
    back.z = float(double(dir.z) * -1.0);
    Vec3 back5;
    back5.x = float(double(back.x) * 5.0);
    back5.y = float(double(back.y) * 5.0);
    back5.z = float(double(back.z) * 5.0);
    SplinePoint lead;
    lead.x = float(double(back5.x) + first.x);
    lead.y = float(double(first.y) + back5.y);
    lead.z = float(double(first.z) + back5.z);
    lead.w = 0.0f;
    PushPoint(self, lead);
    Vec3* p = Path_GetPoint(path, 0);
    PushPoint(self, SplinePoint{p->x, p->y, p->z, 5.0f});
    uint32_t i = 1;
    if (Path_GetCount(path) > 1) {
        do {
            float steps = 2.0f;
            Vec3* prev = Path_GetPoint(path, i - 1);
            Vec3* cur = Path_GetPoint(path, i);
            float previous = Length(*VectorSub(&tmp, cur, prev));
            if (0.0f == previous) {
                Path_GetCount(path);
                Vec3* q = Path_GetPoint(path, i);
                PushPoint(self, SplinePoint{q->x, q->y, q->z, 0.0f});
            } else {
                float sum;
                for (;;) {
                    int j = 0;
                    sum = 0.0f;
                    float step = 0.0f;
                    double stepsD = steps;
                    if (stepsD > 0.0) {
                        float at0 = float(int32_t(i) - 2);
                        float at1 = float(int32_t(i - 1));
                        float at2 = float(int32_t(i));
                        float at3 = float(int32_t(i + 1));
                        float inv = float(1.0 / double(steps));
                        do {
                            uint32_t i0 = PathIndex(path, at0);
                            uint32_t i1 = PathIndex(path, at1);
                            uint32_t i2 = PathIndex(path, at2);
                            uint32_t i3 = PathIndex(path, at3);
                            Vec3 c0 = *Path_GetPoint(path, i0);
                            Vec3 c1 = *Path_GetPoint(path, i3);
                            Vec3* q2 = Path_GetPoint(path, i2);
                            Vec3* q1 = Path_GetPoint(path, i1);
                            Spline_FitControls(self, &c0, *q1, *q2, &c1);
                            float u = float(double(step) * inv);
                            Vec3* r2 = Path_GetPoint(path, i2);
                            Vec3* r1 = Path_GetPoint(path, i1);
                            Vec3 point0;
                            CatmullRom(&point0, &c0, r1, r2, &c1, u);
                            j++;
                            float u2 = float(double(j) * inv);
                            r2 = Path_GetPoint(path, i2);
                            r1 = Path_GetPoint(path, i1);
                            Vec3 point1;
                            CatmullRom(&point1, &c0, r1, r2, &c1, u2);
                            float length = Length(*VectorSub(&tmp, &point0, &point1));
                            sum = float(double(length) + sum);
                            step = float(j);
                        } while (step < stepsD);
                    }
                    float change = float(double(sum) - previous);
                    if (change >= -0.10000000149011612 && change <= 0.10000000149011612) {
                        break;
                    }
                    previous = sum;
                    steps = float(double(steps) + 1.0);
                }
                Vec3* q = Path_GetPoint(path, i);
                PushPoint(self, SplinePoint{q->x, q->y, q->z, sum});
            }
            i++;
        } while (i < Path_GetCount(path));
    }
    SetControls(self);
}
SCRIPTCAM_REVERSED("SplineInterpolater::Build_508930", 0x00508930, RE_BODY(&Spline_Build), nullptr, nullptr, re::Mode::Original);

// 0x5090E0: on to the next segment
void __fastcall Spline_NextSegmentOurs(Spline* self, re::Edx) {
    self->segment++;
    self->distance = 0.0f;
    self->t = 0.0f;
    SetControls(self);
}
SCRIPTCAM_REVERSED("SplineInterpolater::NextSegment_5090E0", 0x005090E0, RE_BODY(&Spline_NextSegmentOurs), nullptr, nullptr, re::Mode::Original);

// 0x5091C0 (+0x40 left as it was)
Spline* __fastcall Spline_Construct(Spline* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kSplineVtable);
    self->position = {0.0f, 0.0f, 0.0f};
    self->control0 = {0.0f, 0.0f, 0.0f};
    self->control1 = {0.0f, 0.0f, 0.0f};
    self->points.data = nullptr;
    self->points.capacity = 0;
    self->points.count = 0;
    self->acceleration = 5.0f;
    self->deceleration = 5.0f;
    self->segment = 0;
    self->maxSpeed = 30.0f;
    self->position = {0.0f, 0.0f, 0.0f};
    self->speed = 0.0f;
    self->distance = 0.0f;
    return self;
}
SCRIPTCAM_REVERSED("SplineInterpolater::SplineInterpolater_5091C0", 0x005091C0, RE_BODY(&Spline_Construct), nullptr, nullptr, re::Mode::Original);

// 0x509280: moves along by speed * dt (into the next segment when past this one)
void __fastcall Spline_AdvanceOurs(Spline* self, re::Edx, float dt) {
    float distance = float(double(self->speed) * dt + self->distance);
    self->distance = distance;
    int last = int(self->points.count) - 1;
    float length = self->points.data[ClampIndex(self->segment + 2, last)].w;
    if (length < distance) {
        float excess = float(double(distance) - length);
        Spline_NextSegment(self);
        self->distance = float(double(self->distance) + excess);
    }
}
SCRIPTCAM_REVERSED("SplineInterpolater::Advance_509280", 0x00509280, RE_BODY(&Spline_AdvanceOurs), nullptr, nullptr, re::Mode::Original);

// 0x5092F0: the speed, the distance, then the position on the curve (nothing
// without points)
void __fastcall Spline_UpdateOurs(Spline* self, re::Edx, float dt) {
    if (self->points.count == 0) {
        return;
    }
    Spline_UpdateSpeed(self, dt);
    Spline_Advance(self, dt);
    int segment = self->segment;
    int last = int(self->points.count) - 1;
    const SplinePoint* data = self->points.data;
    float length = data[ClampIndex(segment + 2, last)].w;
    if (length > 0.0f) {
        self->t = float(double(self->distance) / length);
    }
    const SplinePoint& p2 = data[ClampIndex(segment + 2, last)];
    const SplinePoint& p1 = data[ClampIndex(segment + 1, last)];
    Vec3 out;
    const Vec3* r = CatmullRom(&out, &self->control0, reinterpret_cast<const Vec3*>(&p1),
                               reinterpret_cast<const Vec3*>(&p2), &self->control1, self->t);
    self->position = *r;
}
SCRIPTCAM_REVERSED("SplineInterpolater::Update_5092F0", 0x005092F0, RE_BODY(&Spline_UpdateOurs), nullptr, nullptr, re::Mode::Original);
