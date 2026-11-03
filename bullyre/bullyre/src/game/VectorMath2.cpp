// Vector helpers, second part: CVector::NormaliseAndMag, and a transform stood on the
// ground (its up the ground's normal, facing a direction). 0x4294A0..0x4297F7.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

#define VECMATH2_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

struct VmVector {
    float x, y, z;
};
static_assert(sizeof(VmVector) == 0xC);

// A collision point (see World.h's CColPoint)
struct VmColPoint {
    VmVector point;    // +0
    re::Ptr32<void> entity;      // +0xC (a registered reference)
    VmVector normal;   // +0x10
    int32_t piece;     // +0x1C
    uint32_t surfaces; // +0x20
    float depth;       // +0x24
};
static_assert(sizeof(VmColPoint) == 0x28);

GAME_FN(float, __thiscall, VM_NormaliseAndMag, 0x004294A0, VmVector* v);
GAME_FN(uint8_t, __cdecl, VM_ProcessVerticalLine, 0x004629C0, const VmVector* start, float zEnd, VmColPoint* point, re::Ptr32<void>* entity,
        int buildings, int vehicles);
GAME_FN(void, __cdecl, VM_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);

} // namespace

namespace CVectorMath2 {

// 0x4294A0 (thiscall): the vector made unit length (all 0 when it's 0); its length
// before. (The game's square root takes the fabs first)
float __fastcall NormaliseAndMag(VmVector* self, re::Edx) {
    float sq = self->y * self->y + self->x * self->x + self->z * self->z;
    float length = std::sqrt(std::fabs(sq));
    float inverse = length == 0.0f ? 0.0f : float(1.0 / length);
    self->x = self->x * inverse;
    self->y = inverse * self->y;
    self->z = inverse * self->z;
    return length;
}

// 0x429520: a transform (3x3 rotation by columns right, up, back; then the position)
// at a position dropped onto the ground below it (from 1 unit above, 100 down; raised
// by zOffset when there is ground), up the ground's normal (else z), facing `dir`
// (the 4th floats of both vectors are padding)
void __cdecl GroundTransform(float* out, float x, float y, float z, uint32_t /*w*/, float dirX, float dirY, float dirZ, uint32_t /*dirW*/,
                             float zOffset) {
    VmVector up = {0.0f, 0.0f, 1.0f};
    VmColPoint point;
    point.point = {0.0f, 0.0f, 0.0f};
    point.entity = nullptr;
    point.normal = {0.0f, 0.0f, 0.0f};
    point.piece = 0;     // (not written by the game)
    point.surfaces = 0;  // (not written by the game)
    point.depth = 3.40282347e+38f;
    VmVector start = {x, y, float(z + 1.0)};
    float zEnd = float(start.z - 100.0);
    re::Ptr32<void> entity = nullptr;
    float posX = x;
    float posY = y;
    float posZ = z;
    if (VM_ProcessVerticalLine(&start, zEnd, &point, &entity, 1, 0)) {
        posX = point.point.x;
        posY = point.point.y;
        up = point.normal;
        VM_NormaliseAndMag(&up);
        posZ = point.point.z + zOffset;
    }
    VmVector back = {dirX, dirY, dirZ};
    VM_NormaliseAndMag(&back);
    back.x = float(back.x * -1.0);
    back.y = float(back.y * -1.0);
    back.z = float(-1.0 * back.z);
    VmVector right;
    right.x = back.y * up.z - up.y * back.z;
    right.y = up.x * back.z - back.x * up.z;
    right.z = back.x * up.y - up.x * back.y;
    VM_NormaliseAndMag(&right);
    back.x = right.y * up.z - up.y * right.z;
    back.y = up.x * right.z - right.x * up.z;
    back.z = right.x * up.y - up.x * right.y;
    VM_NormaliseAndMag(&back);
    out[9] = posX;
    out[10] = posY;
    out[11] = posZ;
    out[2] = back.x;
    out[5] = back.y;
    out[8] = back.z;
    out[0] = right.x;
    out[3] = right.y;
    out[6] = right.z;
    out[1] = up.x;
    out[4] = up.y;
    out[7] = up.z;
    if (point.entity) {
        VM_CleanUpOldReference(point.entity, &point.entity);
    }
}

VECMATH2_REVERSED("CVector::NormaliseAndMag", 0x004294A0, &NormaliseAndMag);
VECMATH2_REVERSED("GroundTransform", 0x00429520, &GroundTransform);

} // namespace CVectorMath2
