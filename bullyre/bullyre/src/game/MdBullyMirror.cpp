// 0x518710 to 0x518AB0: MdBullyMirror, the planar mirror the main view render click
// draws the scene through once more: a plane (from three points), its own camera
// reflected from the main camera's, and an enabled flag.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

inline void* const ms_numRefObjects = reinterpret_cast<void*>(0x020E2168);

GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(uint8_t*, __thiscall, NiCamera_Construct, 0x00763C20, void* self);
GAME_FN(void, __thiscall, NiCamera_SetViewFrustum, 0x00762D60, void* camera, const float* frustum);
GAME_FN(void, __thiscall, NiAVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(float, __thiscall, CVector_Normalise, 0x004294A0, void* vector);
// d3dx9 (the imports' thunks)
GAME_FN(void*, __stdcall, D3DXMatrixReflect, 0x0085AC72, float* out, const float* plane);
GAME_FN(void*, __stdcall, D3DXVec4Transform, 0x0085AC6C, float* out, const float* in, const float* matrix);
GAME_FN(void*, __stdcall, D3DXVec3Normalize, 0x0085AC66, float* out, const float* in);

constexpr uint32_t kNiRefObjectVtable = 0x008FEFD4;
constexpr uint32_t kMirrorVtable = 0x009178D4;

struct Mirror {
    uint32_t vtable;     // +0
    int32_t refCount;    // +4
    float plane[4];      // +8 normal and distance (D3DXPLANE)
    re::Ptr32<uint8_t> camera;     // +0x18 NiPointer<NiCamera>
    uint8_t enabled;     // +0x1C
    uint8_t unknown1D[3];
};
static_assert(sizeof(Mirror) == 0x20);
static_assert(offsetof(Mirror, camera) == 0x18);

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

float& F(uint8_t* object, int offset) {
    return *reinterpret_cast<float*>(object + offset);
}

struct Vector {
    float x, y, z;
};

} // namespace

namespace MdBullyMirror {

// 0x518710 (thiscall, 4 bytes): IsEnabled
[[maybe_unused]] static uint8_t __fastcall IsEnabled(Mirror* self, re::Edx) {
    return self->enabled;
}
RE_UNHOOKED_FN("MdBullyMirror::IsEnabled", 0x00518710, &IsEnabled);

// 0x518720 (thiscall): Disable
static void __fastcall Disable(Mirror* self, re::Edx) {
    self->enabled = 0;
}

// 0x518730 (thiscall): the constructor (NiRefObject's inlined)
static Mirror* __fastcall Construct(Mirror* self, re::Edx) {
    self->vtable = kNiRefObjectVtable;
    self->refCount = 0;
    vc::Increment(ms_numRefObjects);
    self->vtable = kMirrorVtable;
    self->camera = nullptr;
    self->enabled = 0;
    return self;
}

// 0x518760 (thiscall): GetRTTI
static void* __fastcall GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x00C676A4);
}

// 0x518770 (thiscall): the mirror's camera (+0x120 0.1, +0x124 1000)
static void __fastcall Initialise(Mirror* self, re::Edx) {
    void* memory = NiNew(0x13C);
    uint8_t* camera = memory ? NiCamera_Construct(memory) : nullptr;
    if (self->camera != camera) {
        Release(self->camera);
        self->camera = camera;
        if (camera) {
            vc::Increment(camera + 4);
        }
    }
    F(self->camera, 0x124) = 1000.0f;
    F(self->camera, 0x120) = 0.1f;
}

// 0x5187F0 (thiscall): the mirror's camera for `camera`: its world position (+0x84)
// reflected in the plane as the local translation, its direction and up (world
// rotation columns 0 and 1) reflected (the reflection's translation cleared) and
// normalised, the third axis their cross product; then updated
static uint8_t* __fastcall GetCamera(Mirror* self, re::Edx, uint8_t* camera) {
    float reflect[16];
    D3DXMatrixReflect(reflect, self->plane);

    float position[4] = {F(camera, 0x84), F(camera, 0x88), F(camera, 0x8C), 1.0f};
    float reflected[4];
    D3DXVec4Transform(reflected, position, reflect);
    uint8_t* mirrorCamera = self->camera;
    F(mirrorCamera, 0x50) = reflected[0];
    F(mirrorCamera, 0x54) = reflected[1];
    F(mirrorCamera, 0x58) = reflected[2];
    reflect[12] = 0.0f;
    reflect[13] = 0.0f;
    reflect[14] = 0.0f;

    float direction[4] = {F(camera, 0x60), F(camera, 0x6C), F(camera, 0x78), 1.0f};
    float a[4];
    D3DXVec4Transform(a, direction, reflect);
    D3DXVec3Normalize(a, a);
    float up[4] = {F(camera, 0x64), F(camera, 0x70), F(camera, 0x7C), 1.0f};
    float b[4];
    D3DXVec4Transform(b, up, reflect);
    D3DXVec3Normalize(b, b);
    float c[3] = {
        a[2] * b[1] - a[1] * b[2],
        b[2] * a[0] - a[2] * b[0],
        b[0] * a[1] - b[1] * a[0],
    };
    D3DXVec3Normalize(c, c);

    // the local rotation, the three vectors as columns
    float rotation[9] = {a[0], b[0], c[0], a[1], b[1], c[1], a[2], b[2], c[2]};
    std::memcpy(mirrorCamera + 0x2C, rotation, sizeof(rotation));
    NiAVObject_Update(self->camera, 0.0f, 1);
    return self->camera;
}

// 0x518A50 (thiscall): the deleting destructor
static void* __fastcall Destroy(Mirror* self, re::Edx, int flags) {
    self->vtable = kMirrorVtable;
    Release(self->camera);
    self->vtable = kNiRefObjectVtable;
    vc::Decrement(ms_numRefObjects);
    if (flags & 1) {
        NiFreeSized(self, sizeof(Mirror));
    }
    return self;
}

// 0x518AB0 (thiscall): the plane from four points: the normal (b - a) x (b - d)
// through the middle of b and c, the mirror camera given `camera`'s frustum, and the
// mirror enabled. (The last argument unused)
static void __fastcall SetPlane(Mirror* self, re::Edx, const Vector* a, const Vector* b, const Vector* c,
                                const Vector* d, uint8_t* camera, int) {
    Vector middle = {(b->x + c->x) * 0.5f, (b->y + c->y) * 0.5f, (b->z + c->z) * 0.5f};
    Vector d1 = {b->x - d->x, b->y - d->y, b->z - d->z};
    CVector_Normalise(&d1);
    Vector d2 = {b->x - a->x, b->y - a->y, b->z - a->z};
    CVector_Normalise(&d2);
    Vector n = {
        d2.y * d1.z - d2.z * d1.y,
        d2.z * d1.x - d2.x * d1.z,
        d1.y * d2.x - d2.y * d1.x,
    };
    CVector_Normalise(&n);
    self->plane[0] = n.x;
    self->plane[1] = n.y;
    self->plane[2] = n.z;
    self->plane[3] = float(-(double(n.x) * middle.x + double(n.y) * middle.y + double(n.z) * middle.z));
    NiCamera_SetViewFrustum(self->camera, reinterpret_cast<const float*>(camera + 0x104));
    self->enabled = 1;
}

} // namespace MdBullyMirror

// ---- registration ----------------------------------------------------------------------

#define C16_MIRROR_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace MdBullyMirror {

C16_MIRROR_REVERSED("MdBullyMirror::Disable", Disable, 0x00518720);
C16_MIRROR_REVERSED("MdBullyMirror::Construct", Construct, 0x00518730);
C16_MIRROR_REVERSED("MdBullyMirror::GetRTTI", GetRTTI, 0x00518760);
C16_MIRROR_REVERSED("MdBullyMirror::Initialise", Initialise, 0x00518770);
C16_MIRROR_REVERSED("MdBullyMirror::GetCamera", GetCamera, 0x005187F0);
C16_MIRROR_REVERSED("MdBullyMirror::Destroy", Destroy, 0x00518A50);
C16_MIRROR_REVERSED("MdBullyMirror::SetPlane", SetPlane, 0x00518AB0);

} // namespace MdBullyMirror
