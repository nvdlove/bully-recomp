// NiTextureTransform (0x48 bytes, NiTexturingProperty's maps own one): the translation, rotation,
// scale and center of a texture map's coordinates, the 3x3 matrix made from them (by the
// method: Maya's, Max's, or the plain one), streaming, comparison.
//
// Main-thread float maths at the game's 24-bit x87 precision: each product and sum below is a
// float operation in the game's order. The sine and cosine come from one fsincos (here the C
// runtime's, rounded to float as the game stores them).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct NiTextureTransform {
    float translate[2];    // +0x0
    float rotate;          // +0x8 (radians)
    float scale[2];        // +0xC
    float center[2];       // +0x14
    uint8_t matrixDirty;   // +0x1C
    uint8_t pad1D[3];
    float matrix[3][3];    // +0x20
    uint32_t method;       // +0x44 (0 Maya, 1 Max, 2 the plain one)
};
static_assert(offsetof(NiTextureTransform, center) == 0x14);
static_assert(offsetof(NiTextureTransform, matrix) == 0x20);
static_assert(offsetof(NiTextureTransform, method) == 0x44);
static_assert(sizeof(NiTextureTransform) == 0x48);

GAME_FN(uint8_t, __thiscall, Point2_NotEqual, 0x00779F50, const float* self, const float* other);
GAME_FN(void, __thiscall, Point2_LoadBinary, 0x007D39F0, float* self, void* stream);
GAME_FN(void, __thiscall, Point2_SaveBinary, 0x007D3A50, float* self, void* stream);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, TextureTransform_UpdateMatrix, 0x0077EB30, NiTextureTransform* self);

// operator!=: rotation, translation, scale, method, center
uint8_t __fastcall NotEqual(const NiTextureTransform* self, re::Edx, const NiTextureTransform* other) {
    if (self->rotate != other->rotate || self->translate[0] != other->translate[0] ||
        self->translate[1] != other->translate[1]) {
        return 1;
    }
    if (Point2_NotEqual(self->scale, other->scale) || self->method != other->method) {
        return 1;
    }
    if (Point2_NotEqual(self->center, other->center)) {
        return 1;
    }
    return 0;
}

// UpdateMatrix: rotation and scale about the center, then the translation (an unknown method
// leaves the matrix as it was); clean afterwards
void __fastcall UpdateMatrix(NiTextureTransform* self, re::Edx) {
    float angle = self->rotate;
    float c = float(crt::Cos(angle));
    float s = float(crt::Sin(angle));
    float(&m)[3][3] = self->matrix;
    switch (self->method) {
    case 0: {
        float sx = self->scale[0] * s;
        m[0][0] = self->scale[0] * c;
        m[1][0] = sx;
        m[2][0] = 0.0f;
        float cy = self->scale[1] * c;
        float ns = -s;
        m[0][1] = self->scale[1] * ns;
        m[1][1] = cy;
        m[2][1] = 0.0f;
        float dy = self->translate[1] - self->center[1];
        float dx = self->translate[0] - self->center[0];
        float row1 = c * dy;
        row1 = row1 + s * dx;
        row1 = row1 + self->center[1];
        float row0 = dx * c;
        row0 = row0 + ns * dy;
        m[0][2] = row0 + self->center[0];
        m[1][2] = row1;
        m[2][2] = 1.0f;
        break;
    }
    case 1: {
        float a = -s * self->scale[1];
        m[0][0] = self->scale[0] * c;
        m[1][0] = a;
        m[2][0] = 0.0f;
        float b = self->scale[1] * c;
        m[0][1] = self->scale[0] * s;
        m[1][1] = b;
        m[2][1] = 0.0f;
        float dy = self->translate[1] - self->center[1];
        float e = -self->center[0] - self->translate[0];
        float row1 = c * dy;
        row1 = row1 - s * e;
        row1 = row1 * self->scale[1];
        row1 = row1 + self->center[1];
        float row0 = c * e;
        row0 = s * dy + row0;
        row0 = row0 * self->scale[0];
        m[0][2] = row0 + self->center[0];
        m[1][2] = row1;
        m[2][2] = 1.0f;
        break;
    }
    case 2: {
        float ns = -s;
        float a = self->scale[0] * ns;
        m[0][0] = self->scale[0] * c;
        m[1][0] = a;
        m[2][0] = 0.0f;
        float b = -c * self->scale[1];
        m[0][1] = ns * self->scale[1];
        m[1][1] = b;
        m[2][1] = 0.0f;
        float g = self->translate[1] + self->center[1];
        g = 1.0f - g;
        float h = self->center[0] - self->translate[0];
        float row1 = h * s;
        row1 = row1 + c * g;
        row1 = row1 + self->center[1];
        float k = self->translate[0] - self->center[0];
        float row0 = c * k;
        row0 = s * g + row0;
        m[0][2] = row0 + self->center[0];
        m[1][2] = row1;
        m[2][2] = 1.0f;
        break;
    }
    default:
        break;
    }
    self->matrixDirty = 0;
}

// NiTextureTransform(translate, rotate, scale, center, method)
NiTextureTransform* __fastcall Construct(NiTextureTransform* self, re::Edx, const float* translate, float rotate,
                                         const float* scale, const float* center, uint32_t method) {
    self->translate[0] = translate[0];
    self->rotate = rotate;
    self->translate[1] = translate[1];
    self->scale[0] = scale[0];
    self->scale[1] = scale[1];
    self->center[0] = center[0];
    self->center[1] = center[1];
    self->method = method;
    TextureTransform_UpdateMatrix(self);
    return self;
}

// LoadBinary: translation, scale, rotation, method, center (the matrix made when next needed)
void __fastcall LoadBinary(NiTextureTransform* self, re::Edx, void* stream) {
    Point2_LoadBinary(self->translate, stream);
    Point2_LoadBinary(self->scale, stream);
    LoadBytes(stream, &self->rotate, 4);
    uint32_t method;
    LoadBytes(stream, &method, 4);
    self->method = method;
    Point2_LoadBinary(self->center, stream);
    self->matrixDirty = 1;
}

void __fastcall SaveBinary(NiTextureTransform* self, re::Edx, void* stream) {
    Point2_SaveBinary(self->translate, stream);
    Point2_SaveBinary(self->scale, stream);
    SaveBytes(stream, &self->rotate, 4);
    SaveBytes(stream, &self->method, 4);
    Point2_SaveBinary(self->center, stream);
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NITEXXFORM_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NITEXXFORM_REVERSED("NiTextureTransform::operator!=", 0x0077EAC0, &ni::NotEqual);
// (callers rely on ecx: the constructor 0x77ED00 returns ecx as it is after the call; turn on
// together with NiTextureTransform::NiTextureTransform)
NITEXXFORM_REVERSED("NiTextureTransform::UpdateMatrix", 0x0077EB30, &ni::UpdateMatrix);
NITEXXFORM_REVERSED("NiTextureTransform::NiTextureTransform", 0x0077ED00, &ni::Construct);
NITEXXFORM_REVERSED("NiTextureTransform::LoadBinary", 0x0077ED50, &ni::LoadBinary);
NITEXXFORM_REVERSED("NiTextureTransform::SaveBinary", 0x0077EDD0, &ni::SaveBinary);
