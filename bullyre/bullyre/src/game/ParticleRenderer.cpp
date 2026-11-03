// The effect particle renderer (0x577330..0x57A4F0): one object (at 0x1B83108, *0xB6F1EC)
// that turns an emitter's particles into textured quads in the Im3D buffer (0xBD7800,
// indices at 0xBF0B2C) and draws them: camera-facing or stretched along each particle's
// velocity, in world space or the emitter's own; with the emitter's modifiers (colour and
// size, a sub-rectangle of the quad, a rotation, texture coordinates) applied per group.
// (Ghidra's names for these, "JitterMovementInfo::...", are wrong.)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct Vec3 {
    float x, y, z;
};

// A quad corner (16 bytes; w isn't written: the game fills it from uninitialised stack)
struct Corner {
    float x, y, z, w;
};

// NiTransform (0x34): rotation rows, translation, scale
struct Transform {
    float rot[9];     // +0
    float trans[3];   // +0x24
    float scale;      // +0x30
};
static_assert(sizeof(Transform) == 0x34);

// An Im3D vertex as this renderer fills it (0x24)
struct QuadVertex {
    float x, y, z;         // +0
    float unknown0C[3];    // +0xC (not written)
    uint32_t colour;       // +0x18 ARGB
    float u, v;            // +0x1C
};
static_assert(sizeof(QuadVertex) == 0x24);

// A particle: position, velocity, the next in the emitter's list
struct Particle {
    Vec3 pos;          // +0
    Vec3 velocity;     // +0xC
    re::Ptr32<Particle> next;    // +0x18
};
static_assert(offsetof(Particle, next) == 0x18);

// The renderer (0x16C)
struct ParticleRenderer {
    Corner camPos;            // +0 the camera's position (w: its transform's scale)
    re::Ptr32<void> camera;             // +0x10
    Transform view;           // +0x14 the camera's inverse (columns normalised)
    uint8_t colour[4];        // +0x48 the emitter's RGBA
    uint8_t finalColour[4];   // +0x4C after the modifiers and alpha
    float size;               // +0x50
    float distanceScale;      // +0x54
    float baseSize;           // +0x58
    uint8_t hasUVs;           // +0x5C
    uint8_t pad5D[3];
    float uvs[8];             // +0x60 (u, v) per corner
    uint8_t hasRect;          // +0x80
    uint8_t pad81[3];
    float rect[4];            // +0x84 the quad's extent: +x, +y, -y, -x
    uint8_t unknown94;        // +0x94
    uint8_t hasRotation;      // +0x95
    uint8_t pad96[2];
    float sine;               // +0x98
    float cosine;             // +0x9C
    int32_t alpha;            // +0xA0 percent
    uint8_t brighten;         // +0xA4 by the global brightness
    uint8_t padA5[3];
    Corner unitQuad[4];       // +0xA8 a unit square through the view rotation
    Corner quad[4];           // +0xE8 the aligned quad (before its per-particle frame)
    Corner viewQuad[4];       // +0x128 the camera-facing quad
    int32_t quadCount;        // +0x168
};
static_assert(offsetof(ParticleRenderer, view) == 0x14);
static_assert(offsetof(ParticleRenderer, colour) == 0x48);
static_assert(offsetof(ParticleRenderer, uvs) == 0x60);
static_assert(offsetof(ParticleRenderer, rect) == 0x84);
static_assert(offsetof(ParticleRenderer, sine) == 0x98);
static_assert(offsetof(ParticleRenderer, alpha) == 0xA0);
static_assert(offsetof(ParticleRenderer, unitQuad) == 0xA8);
static_assert(offsetof(ParticleRenderer, quad) == 0xE8);
static_assert(offsetof(ParticleRenderer, viewQuad) == 0x128);
static_assert(offsetof(ParticleRenderer, quadCount) == 0x168);
static_assert(sizeof(ParticleRenderer) == 0x16C);

GAME_FN(void, __cdecl, PR_SetRenderState, 0x00519790, int state, int value);
GAME_FN(int, __cdecl, PR_GetRaster, 0x0051A120, void* texture);
GAME_FN(int, __thiscall, PR_SrcBlend, 0x00667740, void* self, int mode);
GAME_FN(int, __thiscall, PR_DestBlend, 0x00667750, void* self, int mode);
GAME_FN(Vec3*, __thiscall, PR_MatrixMultiply, 0x00768A70, const void* matrix, Vec3* out, const void* v);
GAME_FN(void*, __cdecl, PR_Im3DTransform, 0x005EFAF0, void* vertices, int count, void* transform, uint32_t flags);
GAME_FN(void, __cdecl, PR_Im3DRenderIndexed, 0x005F0170, int type, uint16_t* indices, int count, int);
GAME_FN(uint8_t, __cdecl, PR_Im3DEnd, 0x005EFB20);
GAME_FN(void, __cdecl, PR_MatrixInvert, 0x00522440, Transform* out, const void* in);
GAME_FN(float, __thiscall, PR_Normalise, 0x00414E10, Vec3* v);
GAME_FN(float, __thiscall, PR_NormaliseSafe, 0x00415130, Vec3* v);
GAME_FN(Vec3*, __cdecl, PR_VectorSub, 0x00414B70, Vec3* out, const Vec3* a, const Vec3* b);
GAME_FN(void*, __cdecl, PR_GetCameraFrame, 0x006C81D0, void* camera);
GAME_FN(uint8_t, __thiscall, PR_ModifierType, 0x00669400, void* link);
GAME_FN(float, __thiscall, PR_EmitterAge, 0x0066A010, void* timer);
GAME_FN(void, __thiscall, PR_EmitterStart, 0x00670870, uint8_t* emitter, void* transform);
GAME_FN(uint8_t*, __thiscall, PR_EmitterTexture, 0x00670180, uint8_t* emitter);
GAME_FN(int, __thiscall, PR_EmitterBlend, 0x00670190, uint8_t* emitter);
GAME_FN(float, __thiscall, PR_EmitterSize, 0x006703E0, uint8_t* emitter);
// ours, through the game's addresses
GAME_FN(void, __thiscall, PR_SetColour, 0x00577330, ParticleRenderer* self, const uint8_t* rgba);
GAME_FN(void, __thiscall, PR_SetTexture, 0x00577610, ParticleRenderer* self, int index);
GAME_FN(void, __thiscall, PR_SetBlend, 0x00577640, ParticleRenderer* self, int mode);
GAME_FN(void, __thiscall, PR_TransformCorners, 0x005776B0, const Corner* self, Corner* out, const Transform* matrix);
GAME_FN(void, __thiscall, PR_Flush, 0x00577750, ParticleRenderer* self);
GAME_FN(void, __thiscall, PR_SetSquare, 0x005777A0, Corner* self, float size);
GAME_FN(void, __thiscall, PR_ScaleCorners, 0x00577860, Corner* self, float scale);
GAME_FN(void, __thiscall, PR_SetMatrix, 0x00577970, ParticleRenderer* self, const Transform* transform);
GAME_FN(void, __thiscall, PR_SetCamera, 0x005779E0, ParticleRenderer* self, void* frame);
GAME_FN(void, __thiscall, PR_ScaledCopy, 0x00577B30, const Corner* self, Corner* out, float scale);
GAME_FN(void, __thiscall, PR_BuildViewQuad, 0x00577C70, ParticleRenderer* self);
GAME_FN(void, __thiscall, PR_BuildQuad, 0x00577EF0, ParticleRenderer* self);
GAME_FN(void, __thiscall, PR_ApplyModifiers, 0x00578420, ParticleRenderer* self, re::Ptr32<void>* list);
GAME_FN(float, __thiscall, PR_DistanceScale, 0x005785B0, ParticleRenderer* self, uint8_t* emitter);
GAME_FN(void, __thiscall, PR_RenderAligned, 0x00578700, ParticleRenderer* self, re::Ptr32<Particle>* list, int count);
GAME_FN(void, __thiscall, PR_RenderFacing, 0x00578F70, ParticleRenderer* self, re::Ptr32<Particle>* list, int count);
GAME_FN(void, __thiscall, PR_RenderAlignedLocal, 0x00579340, ParticleRenderer* self, re::Ptr32<Particle>* list, int count, const Transform* transform);
GAME_FN(void, __thiscall, PR_RenderFacingLocal, 0x00579C20, ParticleRenderer* self, re::Ptr32<Particle>* list, int count, const Transform* transform);
GAME_FN(void, __thiscall, PR_GroupsAligned, 0x0057A090, ParticleRenderer* self, uint8_t* groups, re::Ptr32<Particle>* list);
GAME_FN(void, __thiscall, PR_GroupsFacing, 0x0057A0D0, ParticleRenderer* self, uint8_t* groups, re::Ptr32<Particle>* list);
GAME_FN(void, __thiscall, PR_GroupsAlignedLocal, 0x0057A110, ParticleRenderer* self, uint8_t* groups, re::Ptr32<Particle>* list, const Transform* transform);
GAME_FN(void, __thiscall, PR_GroupsFacingLocal, 0x0057A150, ParticleRenderer* self, uint8_t* groups, re::Ptr32<Particle>* list, const Transform* transform);

GAME_VAR(uint8_t*, g_effectTextures, 0x00B6F1F0);   // the effect texture list (textures at +0x5C, 0xC apart)
GAME_ARRAY(QuadVertex, g_im3dVertices, 0x00BD7800);
GAME_ARRAY(uint16_t, g_im3dIndices, 0x00BF0B2C);
GAME_VAR(int32_t, g_quadVertexCount, 0x00CA2AE8);
GAME_VAR(int32_t, g_quadIndexCount, 0x00CA2AEC);
GAME_VAR(float, g_largestParticle, 0x00AE5F50);     // the largest quad's diagonal drawn
GAME_VAR(float, g_brightness, 0x020C4798);
inline void* const kBlendModes = reinterpret_cast<void*>(0x01B65D48);

// the colour as a D3D ARGB dword
uint32_t Argb(const uint8_t* rgba) {
    return (uint32_t(rgba[3]) << 24) | (uint32_t(rgba[0]) << 16) | (uint32_t(rgba[1]) << 8) | uint32_t(rgba[2]);
}

// a quad's four vertices and six indices (0 3 1, 1 3 2)
void EmitQuad(QuadVertex* v, uint16_t* index, int base, uint32_t colour, const ParticleRenderer* self, const Vec3 (&corners)[4]) {
    for (int i = 0; i < 4; i++) {
        v[i].colour = colour;
        v[i].x = corners[i].x;
        v[i].y = corners[i].y;
        v[i].z = corners[i].z;
        v[i].u = self->uvs[i * 2];
        v[i].v = self->uvs[i * 2 + 1];
    }
    uint16_t b = uint16_t(base);
    index[0] = b;
    index[1] = uint16_t(b + 3);
    index[2] = uint16_t(b + 1);
    index[3] = uint16_t(b + 1);
    index[4] = uint16_t(b + 3);
    index[5] = uint16_t(b + 2);
}

// the largest quad drawn (its diagonal, corner 1 to corner 3)
void TrackLargest(const Vec3& a, const Vec3& b) {
    Vec3 d;
    PR_VectorSub(&d, &a, &b);
    float length = std::sqrt(std::fabs(d.x * d.x + d.y * d.y + d.z * d.z));
    if (g_largestParticle < length) {
        g_largestParticle = length;
    }
}

// the quad of 0x577EF0/0x577C70 from the rectangle and/or rotation (false: neither set)
bool BuildRotatedRect(const ParticleRenderer* self, Corner* q) {
    const float* r = self->rect;
    float s = self->sine;
    float c = self->cosine;
    float lastY;
    if (self->hasRotation == 1) {
        if (self->hasRect == 1) {
            q[3].x = c * r[1] - s * r[0];
            q[3].y = r[0] * c + s * r[1];
            q[3].z = 0.0f;
            float negLeft = -r[3];
            float negBottom = -r[2];
            float bs = negBottom * s;
            q[1].x = negLeft * c - bs;
            float bc = negBottom * c;
            float ls = negLeft * s;
            q[1].y = ls + bc;
            q[1].z = 0.0f;
            q[2].x = c * r[1] - bs;
            q[2].y = s * r[1] + bc;
            q[2].z = 0.0f;
            q[0].x = negLeft * c - s * r[0];
            lastY = ls + r[0] * c;
        } else {
            float a = c - s;
            q[3].x = a;
            float b = s + c;
            q[3].y = b;
            q[3].z = 0.0f;
            float d = s - c;
            q[1].x = d;
            float e = -c - s;
            q[1].y = e;
            q[1].z = 0.0f;
            q[2].z = 0.0f;
            q[2].x = b;
            q[2].y = d;
            q[0].x = e;
            lastY = a;
        }
    } else {
        if (self->hasRect != 1) {
            return false;
        }
        q[3].x = r[1];
        q[3].y = r[0];
        q[3].z = 0.0f;
        q[1].x = -r[3];
        q[1].y = -r[2];
        q[1].z = 0.0f;
        q[2].x = r[1];
        q[2].y = -r[2];
        q[2].z = 0.0f;
        q[0].x = -r[3];
        lastY = r[0];
    }
    q[0].y = lastY;
    q[0].z = 0.0f;
    return true;
}

} // namespace

namespace ParticleRender {

// 0x577330 (thiscall): the quads' colour: the emitter's times the modifier's times the
// alpha percent (red, green and blue doubled, to 255 at most; brightened when asked)
void __fastcall SetColour(ParticleRenderer* self, re::Edx, const uint8_t* rgba) {
    const float k = 3.9215687e-05f;  // 1 / (255 * 100)
    int alpha = self->alpha;
    float g = float(int(uint32_t(self->colour[1]) * rgba[1] * alpha)) * k;
    float b = float(int(uint32_t(self->colour[2]) * rgba[2] * alpha)) * k;
    float a = float(int(uint32_t(self->colour[3]) * rgba[3] * alpha)) * k;
    float r = float(int(uint32_t(self->colour[0]) * rgba[0] * alpha)) * k;
    r = r * 2.0f;
    if (255.0f < r) {
        r = 255.0f;
    }
    g = g * 2.0f;
    if (255.0f < g) {
        g = 255.0f;
    }
    b = b * 2.0f;
    if (255.0f < b) {
        b = 255.0f;
    }
    if (self->brighten) {
        float f = g_brightness;
        self->finalColour[0] = uint8_t(int(r * f));
        self->finalColour[1] = uint8_t(int(g * f));
        self->finalColour[2] = uint8_t(int(b * f));
    } else {
        self->finalColour[0] = uint8_t(int(r));
        self->finalColour[1] = uint8_t(int(g));
        self->finalColour[2] = uint8_t(int(b));
    }
    self->finalColour[3] = uint8_t(int(a));
}

// 0x5775B0 (thiscall, this unused): the render states for effects
void __fastcall SetRenderStates(ParticleRenderer*, re::Edx) {
    PR_SetRenderState(7, 1);
    PR_SetRenderState(6, 1);
    PR_SetRenderState(8, 0);
    PR_SetRenderState(0xC, 1);
    PR_SetRenderState(0x1D, 8);
    PR_SetRenderState(0xE, 0);
    PR_SetRenderState(2, 1);
    PR_SetRenderState(0x14, 1);
    PR_SetRenderState(9, 2);
}

// 0x577610 (thiscall, this unused): an effect texture as the raster
void __fastcall SetTexture(ParticleRenderer*, re::Edx, int index) {
    void* texture = *reinterpret_cast<re::Ptr32<void>*>(g_effectTextures + index * 0xC + 0x5C);
    PR_SetRenderState(1, PR_GetRaster(texture));
}

// 0x577640 (thiscall, this unused): the blend mode (0: the default source and
// destination, set destination first)
void __fastcall SetBlend(ParticleRenderer*, re::Edx, int mode) {
    if (uint8_t(mode) == 0) {
        PR_SetRenderState(0xB, PR_DestBlend(kBlendModes, 0));
        PR_SetRenderState(0xA, PR_SrcBlend(kBlendModes, 0));
        return;
    }
    PR_SetRenderState(0xA, PR_SrcBlend(kBlendModes, mode));
    PR_SetRenderState(0xB, PR_DestBlend(kBlendModes, mode));
}

// 0x5776B0 (thiscall on four corners): them through the matrix's rotation into out
// (the second, third, first, fourth; out may be the corners themselves)
void __fastcall TransformCorners(const Corner* self, re::Edx, Corner* out, const Transform* matrix) {
    Vec3 tmp;
    Vec3* r = PR_MatrixMultiply(matrix, &tmp, &self[1]);
    out[1].x = r->x;
    out[1].y = r->y;
    out[1].z = r->z;
    r = PR_MatrixMultiply(matrix, &tmp, &self[2]);
    out[2].x = r->x;
    out[2].y = r->y;
    out[2].z = r->z;
    r = PR_MatrixMultiply(matrix, &tmp, &self[0]);
    out[0].x = r->x;
    out[0].y = r->y;
    out[0].z = r->z;
    r = PR_MatrixMultiply(matrix, &tmp, &self[3]);
    out[3].x = r->x;
    out[3].y = r->y;
    out[3].z = r->z;
}

// 0x577750 (thiscall): the quads so far drawn (Im3D, indexed triangle list)
void __fastcall Flush(ParticleRenderer* self, re::Edx) {
    if (self->quadCount > 0) {
        PR_Im3DTransform(g_im3dVertices, self->quadCount * 4, nullptr, 1);
        PR_Im3DRenderIndexed(3, g_im3dIndices, self->quadCount * 6, 0);
        PR_Im3DEnd();
    }
    self->quadCount = 0;
}

// 0x5777A0 (thiscall on four corners): a square of half-size s round the origin
void __fastcall SetSquare(Corner* self, re::Edx, float s) {
    float n = -s;
    self[0].x = n;
    self[0].y = s;
    self[0].z = 0.0f;
    self[1].x = n;
    self[1].y = n;
    self[1].z = 0.0f;
    self[2].x = s;
    self[2].y = n;
    self[2].z = 0.0f;
    self[3].x = s;
    self[3].y = s;
    self[3].z = 0.0f;
}

// 0x577860 (thiscall on four corners)
void __fastcall ScaleCorners(Corner* self, re::Edx, float f) {
    self[0].x = f * self[0].x;
    self[0].y = self[0].y * f;
    self[0].z = self[0].z * f;
    self[1].x = f * self[1].x;
    self[1].y = self[1].y * f;
    self[1].z = self[1].z * f;
    self[2].x = self[2].x * f;
    self[2].y = f * self[2].y;
    self[2].z = self[2].z * f;
    self[3].x = self[3].x * f;
    self[3].y = self[3].y * f;
    self[3].z = f * self[3].z;
}

// 0x5778D0 (thiscall): the constructor
ParticleRenderer* __fastcall Construct(ParticleRenderer* self, re::Edx) {
    self->camera = nullptr;
    self->view.scale = 1.0f;
    self->size = 1.0f;
    self->hasUVs = 0;
    self->distanceScale = 1.0f;
    self->hasRect = 0;
    self->baseSize = 1.0f;
    self->hasRotation = 0;
    self->rect[0] = 1.0f;
    self->quadCount = 0;
    self->rect[1] = 1.0f;
    self->alpha = 100;
    self->rect[2] = 1.0f;
    self->rect[3] = 1.0f;
    self->sine = 0.0f;
    self->cosine = 1.0f;
    self->uvs[0] = 1.0f;
    self->uvs[4] = 1.0f;
    self->uvs[6] = 1.0f;
    self->uvs[7] = 1.0f;
    self->uvs[2] = 0.0f;
    self->uvs[3] = 0.0f;
    self->uvs[5] = 0.0f;
    // (camPos.w: the game's uninitialised stack)
    self->camPos.x = 0.0f;
    self->camPos.y = 0.0f;
    self->camPos.z = 0.0f;
    return self;
}

// 0x577970 (thiscall): the view transform as given (the unit square through it, sizes
// 1, white)
void __fastcall SetMatrix(ParticleRenderer* self, re::Edx, const Transform* transform) {
    self->view = *transform;
    PR_SetSquare(self->unitQuad, 1.0f);
    PR_TransformCorners(self->unitQuad, self->unitQuad, &self->view);
    self->baseSize = 1.0f;
    self->distanceScale = 1.0f;
    self->size = 1.0f;
    uint8_t white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    PR_SetColour(self, white);
}

// 0x5779E0 (thiscall): the view transform from a camera's frame (inverted, the
// rotation's columns normalised)
void __fastcall SetCamera(ParticleRenderer* self, re::Edx, void* frame) {
    PR_MatrixInvert(&self->view, frame);
    float* m = self->view.rot;
    for (int column = 0; column < 3; column++) {
        Vec3 v = {m[column], m[column + 3], m[column + 6]};
        PR_Normalise(&v);
        m[column] = v.x;
        m[column + 3] = v.y;
        m[column + 6] = v.z;
    }
    PR_SetSquare(self->unitQuad, 1.0f);
    PR_TransformCorners(self->unitQuad, self->unitQuad, &self->view);
    self->baseSize = 1.0f;
    self->distanceScale = 1.0f;
    self->size = 1.0f;
    uint8_t white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    PR_SetColour(self, white);
}

// 0x577B30 (thiscall on four corners): them scaled into out (the second, third, first,
// fourth)
void __fastcall ScaledCopy(const Corner* self, re::Edx, Corner* out, float scale) {
    static const int order[4] = {1, 2, 0, 3};
    for (int i : order) {
        out[i].x = self[i].x * scale;
        out[i].y = self[i].y * scale;
        out[i].z = (i == 3) ? scale * self[i].z : self[i].z * scale;
    }
}

// 0x577C70 (thiscall): the camera-facing quad: the rectangle/rotation's, scaled and
// turned to the view; without either the unit square scaled
void __fastcall BuildViewQuad(ParticleRenderer* self, re::Edx) {
    if (!BuildRotatedRect(self, self->viewQuad)) {
        PR_ScaledCopy(self->unitQuad, self->viewQuad, self->size);
        return;
    }
    PR_ScaleCorners(self->viewQuad, self->size);
    PR_TransformCorners(self->viewQuad, self->viewQuad, &self->view);
}

// 0x577EF0 (thiscall): the quad in its own frame (stretched quads turn it per particle):
// the rectangle/rotation's scaled; without either a square of the size
void __fastcall BuildQuad(ParticleRenderer* self, re::Edx) {
    if (!BuildRotatedRect(self, self->quad)) {
        PR_SetSquare(self->quad, self->size);
        return;
    }
    PR_ScaleCorners(self->quad, self->size);
}

// 0x578150 (thiscall): a frame's start: the camera, its position, sizes 1, white, no
// quads
void __fastcall Begin(ParticleRenderer* self, re::Edx, void* camera) {
    self->camera = camera;
    PR_SetCamera(self, PR_GetCameraFrame(camera));
    self->baseSize = 1.0f;
    self->distanceScale = 1.0f;
    self->size = 1.0f;
    self->camPos.x = self->view.trans[0];
    self->camPos.y = self->view.trans[1];
    self->camPos.z = self->view.trans[2];
    self->camPos.w = self->view.scale;
    self->quadCount = 0;
    uint8_t white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    PR_SetColour(self, white);
}

// 0x578420 (thiscall): an emitter's (or group's) modifiers: 5 colour and size, 6 the
// quad's rectangle, 7 its rotation, 13 texture coordinates (a list linked at +4)
void __fastcall ApplyModifiers(ParticleRenderer* self, re::Edx, re::Ptr32<void>* list) {
    re::Ptr32<void>* link = static_cast<re::Ptr32<void>*>(*list);
    if (link == nullptr) {
        return;
    }
    uint8_t* modifier = reinterpret_cast<uint8_t*>(link) - 4;
    while (modifier != nullptr) {
        uint8_t* data = modifier + 8;
        if (PR_ModifierType(link) == 5) {
            PR_SetColour(self, data);
            self->size = self->distanceScale * *reinterpret_cast<float*>(modifier + 0xC) * self->baseSize;
        } else if (PR_ModifierType(link) == 6) {
            self->hasRect = 1;
            const float* v = reinterpret_cast<float*>(data);
            float a = v[0];
            float b = v[1];
            float c = v[2];
            self->rect[0] = v[3];
            self->rect[1] = c;
            self->rect[2] = b;
            self->rect[3] = a;
        } else if (PR_ModifierType(link) == 7) {
            self->hasRotation = 1;
            self->sine = *reinterpret_cast<float*>(modifier + 0x10);
            self->cosine = *reinterpret_cast<float*>(modifier + 0x14);
        } else if (PR_ModifierType(link) == 0xD) {
            self->hasUVs = 1;
            const uint32_t* v = reinterpret_cast<uint32_t*>(data);
            uint32_t* uvs = reinterpret_cast<uint32_t*>(self->uvs);
            for (int i = 0; i < 8; i++) {
                uvs[i] = v[i];
            }
        }
        link = static_cast<re::Ptr32<void>*>(*link);
        if (link == nullptr) {
            break;
        }
        modifier = reinterpret_cast<uint8_t*>(link) - 4;
    }
}

// 0x578540: a within maxDistSq of b
uint8_t __cdecl WithinDistanceSq(const Vec3* a, const Vec3* b, float maxDistSq) {
    float x = a->x - b->x;
    float y = a->y - b->y;
    float z = a->z - b->z;
    float lengthSq = x * x + y * y + z * z;
    return maxDistSq > lengthSq ? 1 : 0;
}

// 0x5785B0 (thiscall): the size scale for the emitter's distance from the camera: 1
// nearer than its range's start, the definition's far scale beyond its end, between
// the two in between
float __fastcall DistanceScale(ParticleRenderer* self, re::Edx, uint8_t* emitter) {
    Vec3 cam = {self->camPos.x, self->camPos.y, self->camPos.z};
    uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(emitter + 0xC);
    Vec3 pos = *reinterpret_cast<Vec3*>(data + 0x2C);
    float x = cam.x - pos.x;
    float y = cam.y - pos.y;
    float z = cam.z - pos.z;
    float dist = std::sqrt(std::fabs(x * x + y * y + z * z));
    const float* range = *reinterpret_cast<re::Ptr32<float>*>(data + 0x50);
    if (range[1] > dist) {
        return 1.0f;
    }
    float farScale = *reinterpret_cast<float*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(emitter + 8) + 0x14);
    if (range[2] < dist) {
        return farScale;
    }
    float span = range[2] - range[1];
    float t = 1.0f;
    if (span > 0.0f) {
        float into = dist - range[1];
        t = 1.0f - into / span;
    }
    return 1.0f + (farScale - 1.0f) * (1.0f - t);
}

namespace {

// the frame a stretched quad takes: its x the side (away x velocity), y back along the
// velocity, z the third; then the quad's corners through it, scaled, from the particle
void AlignedCorners(ParticleRenderer* self, const Vec3& pos, Vec3 dir, const Corner (&quad)[4], Vec3 (&out)[4]) {
    PR_NormaliseSafe(&dir);
    float dx = dir.x - 0.0f;
    float dy = dir.y - 0.0f;
    float dz = dir.z - 0.0f;
    if (dx * dx + dy * dy + dz * dz < 0.001f) {
        dir = {0.0f, 0.0f, 1.0f};
    }
    Vec3 toCamera = {self->camPos.x - pos.x, self->camPos.y - pos.y, self->camPos.z - pos.z};
    Vec3 a = {-dir.x, -dir.y, -dir.z};
    Vec3 b = {-toCamera.x, -toCamera.y, -toCamera.z};
    PR_NormaliseSafe(&b);
    Vec3 c;
    c.x = b.z * a.y - b.y * a.z;
    c.y = a.z * b.x - b.z * a.x;
    c.z = a.x * b.y - b.x * a.y;
    PR_NormaliseSafe(&c);
    Vec3 d;
    d.x = c.y * a.z - c.z * a.y;
    d.y = c.z * a.x - a.z * c.x;
    d.z = a.y * c.x - c.y * a.x;
    PR_NormaliseSafe(&d);
    float m[9] = {c.x, a.x, d.x, c.y, a.y, d.y, c.z, a.z, d.z};
    Corner q1 = quad[1];
    Corner q2 = quad[2];
    Corner q0 = quad[0];
    Corner q3 = quad[3];
    Vec3 tmp;
    Vec3 r1 = *PR_MatrixMultiply(m, &tmp, &q1);
    Vec3 r2 = *PR_MatrixMultiply(m, &tmp, &q2);
    Vec3 r0 = *PR_MatrixMultiply(m, &tmp, &q0);
    Vec3 r3 = *PR_MatrixMultiply(m, &tmp, &q3);
    float s = self->size;
    Vec3 s1 = {r1.x * s, r1.y * s, r1.z * s};
    Vec3 s2 = {r2.x * s, r2.y * s, r2.z * s};
    Vec3 s0 = {r0.x * s, r0.y * s, r0.z * s};
    Vec3 s3 = {r3.x * s, r3.y * s, r3.z * s};
    TrackLargest(s1, s3);
    out[0] = {s0.x + pos.x, s0.y + pos.y, s0.z + pos.z};
    out[1] = {s1.x + pos.x, s1.y + pos.y, s1.z + pos.z};
    out[2] = {s2.x + pos.x, s2.y + pos.y, s2.z + pos.z};
    out[3] = {s3.x + pos.x, s3.y + pos.y, s3.z + pos.z};
}

// a particle's position (and velocity) through the emitter's transform
Vec3 ThroughTransform(const Transform* t, const Vec3& v) {
    Vec3 tmp;
    Vec3* r = PR_MatrixMultiply(t->rot, &tmp, &v);
    float scale = t->scale;
    return {t->trans[0] + r->x * scale, r->y * scale + t->trans[1], scale * r->z + t->trans[2]};
}

} // namespace

// 0x578700 (thiscall): count quads stretched along the particles' velocities (from the
// list, which is advanced)
void __fastcall RenderAligned(ParticleRenderer* self, re::Edx, re::Ptr32<Particle>* list, int count) {
    int base = g_quadVertexCount;
    QuadVertex* v = &g_im3dVertices[g_quadVertexCount];
    uint16_t* index = &g_im3dIndices[g_quadIndexCount];
    g_quadVertexCount = g_quadVertexCount + count * 4;
    g_quadIndexCount = g_quadIndexCount + count * 6;
    PR_BuildQuad(self);
    Corner quad[4] = {self->quad[0], self->quad[1], self->quad[2], self->quad[3]};
    if (count < 1) {
        self->quadCount += count;
        return;
    }
    uint32_t colour = Argb(self->finalColour);
    for (int i = 0; i < count; i++) {
        Particle* p = *list;
        Vec3 pos = p->pos;
        Vec3 corners[4];
        AlignedCorners(self, pos, p->velocity, quad, corners);
        EmitQuad(v, index, base + i * 4, colour, self, corners);
        *list = (*list)->next;
        v += 4;
        index += 6;
    }
    self->quadCount += count;
}

// 0x578F70 (thiscall): count camera-facing quads
void __fastcall RenderFacing(ParticleRenderer* self, re::Edx, re::Ptr32<Particle>* list, int count) {
    PR_BuildViewQuad(self);
    const Corner* q = self->viewQuad;
    uint32_t colour = Argb(self->finalColour);
    int base = g_quadVertexCount;
    uint16_t* index = &g_im3dIndices[g_quadIndexCount];
    g_quadIndexCount = g_quadIndexCount + count * 6;
    QuadVertex* v = &g_im3dVertices[g_quadVertexCount];
    g_quadVertexCount = g_quadVertexCount + count * 4;
    Vec3 c1 = {q[1].x, q[1].y, q[1].z};
    Vec3 c3 = {q[3].x, q[3].y, q[3].z};
    TrackLargest(c1, c3);
    for (int i = 0; i < count; i++) {
        Vec3 pos = (*list)->pos;
        Vec3 corners[4] = {{pos.x + q[0].x, pos.y + q[0].y, pos.z + q[0].z},
                           {q[1].x + pos.x, q[1].y + pos.y, q[1].z + pos.z},
                           {q[2].x + pos.x, q[2].y + pos.y, q[2].z + pos.z},
                           {q[3].x + pos.x, pos.y + q[3].y, pos.z + q[3].z}};
        EmitQuad(v, index, base + i * 4, colour, self, corners);
        *list = (*list)->next;
        index += 6;
        v += 4;
    }
    self->quadCount += count;
}

// 0x579340 (thiscall): stretched quads for particles in the emitter's space
void __fastcall RenderAlignedLocal(ParticleRenderer* self, re::Edx, re::Ptr32<Particle>* list, int count, const Transform* transform) {
    int base = g_quadVertexCount;
    QuadVertex* v = &g_im3dVertices[g_quadVertexCount];
    uint16_t* index = &g_im3dIndices[g_quadIndexCount];
    g_quadVertexCount = g_quadVertexCount + count * 4;
    g_quadIndexCount = g_quadIndexCount + count * 6;
    PR_BuildQuad(self);
    Corner quad[4] = {self->quad[0], self->quad[1], self->quad[2], self->quad[3]};
    if (count < 1) {
        self->quadCount += count;
        return;
    }
    uint32_t colour = Argb(self->finalColour);
    for (int i = 0; i < count; i++) {
        Particle* p = *list;
        Vec3 pos = ThroughTransform(transform, p->pos);
        Vec3 tmp;
        Vec3 dir = *PR_MatrixMultiply(transform->rot, &tmp, &p->velocity);
        Vec3 corners[4];
        AlignedCorners(self, pos, dir, quad, corners);
        EmitQuad(v, index, base + i * 4, colour, self, corners);
        *list = (*list)->next;
        v += 4;
        index += 6;
    }
    self->quadCount += count;
}

// 0x579C20 (thiscall): camera-facing quads for particles in the emitter's space
void __fastcall RenderFacingLocal(ParticleRenderer* self, re::Edx, re::Ptr32<Particle>* list, int count, const Transform* transform) {
    PR_BuildViewQuad(self);
    int base = g_quadVertexCount;
    Transform t = *transform;
    uint16_t* index = &g_im3dIndices[g_quadIndexCount];
    g_quadIndexCount = g_quadIndexCount + count * 6;
    uint32_t colour = Argb(self->finalColour);
    const Corner* q = self->viewQuad;
    QuadVertex* v = &g_im3dVertices[g_quadVertexCount];
    g_quadVertexCount = g_quadVertexCount + count * 4;
    Vec3 c1 = {q[1].x, q[1].y, q[1].z};
    Vec3 c3 = {q[3].x, q[3].y, q[3].z};
    TrackLargest(c1, c3);
    if (count < 1) {
        self->quadCount += count;
        return;
    }
    for (int i = 0; i < count; i++) {
        float scale = t.scale;
        Vec3 tmp;
        Vec3* r = PR_MatrixMultiply(t.rot, &tmp, &(*list)->pos);
        Vec3 scaled = {scale * r->x, r->y * scale, scale * r->z};
        Vec3 pos = {t.trans[0] + scaled.x, t.trans[1] + scaled.y, t.trans[2] + scaled.z};
        Vec3 corners[4] = {{pos.x + q[0].x, pos.y + q[0].y, pos.z + q[0].z},
                           {q[1].x + pos.x, q[1].y + pos.y, q[1].z + pos.z},
                           {q[2].x + pos.x, q[2].y + pos.y, q[2].z + pos.z},
                           {q[3].x + pos.x, pos.y + q[3].y, pos.z + q[3].z}};
        EmitQuad(v, index, base + i * 4, colour, self, corners);
        *list = (*list)->next;
        v += 4;
        index += 6;
    }
    self->quadCount += count;
}

// 0x57A090/0x57A0D0/0x57A110/0x57A150 (thiscall): a group list (linked at +0; the
// group's modifiers at +4, its particle count at +0x10), each group's modifiers then its
// particles
void __fastcall GroupsAligned(ParticleRenderer* self, re::Edx, uint8_t* groups, re::Ptr32<Particle>* list) {
    for (; groups != nullptr; groups = *reinterpret_cast<re::Ptr32<uint8_t>*>(groups)) {
        PR_ApplyModifiers(self, reinterpret_cast<re::Ptr32<void>*>(groups + 4));
        PR_RenderAligned(self, list, groups[0x10]);
    }
}

void __fastcall GroupsFacing(ParticleRenderer* self, re::Edx, uint8_t* groups, re::Ptr32<Particle>* list) {
    for (; groups != nullptr; groups = *reinterpret_cast<re::Ptr32<uint8_t>*>(groups)) {
        PR_ApplyModifiers(self, reinterpret_cast<re::Ptr32<void>*>(groups + 4));
        PR_RenderFacing(self, list, groups[0x10]);
    }
}

void __fastcall GroupsAlignedLocal(ParticleRenderer* self, re::Edx, uint8_t* groups, re::Ptr32<Particle>* list, const Transform* transform) {
    for (; groups != nullptr; groups = *reinterpret_cast<re::Ptr32<uint8_t>*>(groups)) {
        PR_ApplyModifiers(self, reinterpret_cast<re::Ptr32<void>*>(groups + 4));
        PR_RenderAlignedLocal(self, list, groups[0x10], transform);
    }
}

void __fastcall GroupsFacingLocal(ParticleRenderer* self, re::Edx, uint8_t* groups, re::Ptr32<Particle>* list, const Transform* transform) {
    for (; groups != nullptr; groups = *reinterpret_cast<re::Ptr32<uint8_t>*>(groups)) {
        PR_ApplyModifiers(self, reinterpret_cast<re::Ptr32<void>*>(groups + 4));
        PR_RenderFacingLocal(self, list, groups[0x10], transform);
    }
}

// 0x57A190 (thiscall): an emitter's particles (the definition at +8: flags +0x1C (1 world
// space, 2 stretched, 4 in the emitter's own frame), far scale +0x14; the instance at
// +0xC: its transform at +8)
void __fastcall Render(ParticleRenderer* self, re::Edx, uint8_t* emitter) {
    uint8_t* definition = *reinterpret_cast<re::Ptr32<uint8_t>*>(emitter + 8);
    uint8_t* instance = *reinterpret_cast<re::Ptr32<uint8_t>*>(emitter + 0xC);
    Transform* transform = reinterpret_cast<Transform*>(instance + 8);
    uint8_t ownFrame = (definition[0x1C] >> 2) & 1;
    if (ownFrame) {
        // the view as the emitter's frame (its columns 0, 2, 1, negated)
        const float* m = transform->rot;
        Transform view;
        view.rot[0] = -m[0];
        view.rot[1] = -m[2];
        view.rot[2] = -m[1];
        view.rot[3] = -m[3];
        view.rot[4] = -m[5];
        view.rot[5] = -m[4];
        view.rot[6] = -m[6];
        view.rot[7] = -m[8];
        view.rot[8] = -m[7];
        view.trans[0] = 0.0f;
        view.trans[1] = 0.0f;
        view.trans[2] = 0.0f;
        view.scale = 1.0f;
        PR_SetMatrix(self, &view);
    }
    if (!(emitter[0x15] & 0x80)) {
        float age = PR_EmitterAge(emitter + 4) - *reinterpret_cast<float*>(definition + 8);
        if (age >= 0.0f) {
            PR_EmitterStart(emitter, transform);
        }
    }
    self->uvs[0] = 0.0f;
    self->uvs[2] = 0.0f;
    self->uvs[3] = 0.0f;
    self->brighten = (*reinterpret_cast<re::Ptr32<uint8_t>*>(instance + 0x50))[0x1C] & 1;
    self->uvs[5] = 0.0f;
    self->hasRotation = 0;
    self->uvs[1] = 1.0f;
    self->hasRect = 0;
    self->uvs[4] = 1.0f;
    self->hasUVs = 0;
    self->uvs[6] = 1.0f;
    self->uvs[7] = 1.0f;
    PR_SetTexture(self, *PR_EmitterTexture(emitter) >> 1);
    PR_SetBlend(self, PR_EmitterBlend(emitter));
    self->alpha = int(*reinterpret_cast<float*>(instance + 0x48) * 100.0f);
    re::Ptr32<Particle> head = *reinterpret_cast<re::Ptr32<Particle>*>(emitter + 0x18);
    uint8_t white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    PR_SetColour(self, white);
    *reinterpret_cast<uint32_t*>(self->colour) = *reinterpret_cast<uint32_t*>(emitter + 0x44);
    float baseSize = PR_EmitterSize(emitter);
    self->baseSize = baseSize;
    self->distanceScale = 1.0f;
    if (1.0f != *reinterpret_cast<float*>(definition + 0x14)) {
        self->distanceScale = PR_DistanceScale(self, emitter);
    }
    self->size = self->distanceScale * baseSize;
    uint8_t* groups = *reinterpret_cast<re::Ptr32<uint8_t>*>(emitter + 0x28);
    int count = emitter[0x16];
    uint8_t stretched = (definition[0x1C] >> 1) & 1;
    PR_ApplyModifiers(self, reinterpret_cast<re::Ptr32<void>*>(emitter + 0x20));
    g_quadVertexCount = 0;
    g_quadIndexCount = 0;
    if (definition[0x1C] & 1) {
        if (groups != nullptr) {
            if (stretched) {
                PR_GroupsAligned(self, groups, &head);
            } else {
                PR_GroupsFacing(self, groups, &head);
            }
        } else if (stretched) {
            PR_RenderAligned(self, &head, count);
        } else {
            PR_RenderFacing(self, &head, count);
        }
    } else if (groups != nullptr) {
        if (stretched) {
            PR_GroupsAlignedLocal(self, groups, &head, transform);
        } else {
            PR_GroupsFacingLocal(self, groups, &head, transform);
        }
    } else if (stretched) {
        PR_RenderAlignedLocal(self, &head, count, transform);
    } else {
        PR_RenderFacingLocal(self, &head, count, transform);
    }
    if (ownFrame) {
        PR_SetCamera(self, PR_GetCameraFrame(self->camera));
    }
    PR_Flush(self);
}

} // namespace ParticleRender

// ---- registration: all original until tested ---------------------------------------

#define PARTREND_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(partrend_original_, __LINE__);                                    \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(partrend_original_, __LINE__), re::Mode::Original)

PARTREND_REVERSED("ParticleRenderer::SetColour", 0x00577330, &ParticleRender::SetColour);
PARTREND_REVERSED("ParticleRenderer::SetRenderStates", 0x005775B0, &ParticleRender::SetRenderStates);
PARTREND_REVERSED("ParticleRenderer::SetTexture", 0x00577610, &ParticleRender::SetTexture);
PARTREND_REVERSED("ParticleRenderer::SetBlend", 0x00577640, &ParticleRender::SetBlend);
PARTREND_REVERSED("ParticleRenderer::TransformCorners", 0x005776B0, &ParticleRender::TransformCorners);
PARTREND_REVERSED("ParticleRenderer::Flush", 0x00577750, &ParticleRender::Flush);
PARTREND_REVERSED("ParticleRenderer::SetSquare", 0x005777A0, &ParticleRender::SetSquare);
PARTREND_REVERSED("ParticleRenderer::ScaleCorners", 0x00577860, &ParticleRender::ScaleCorners);
PARTREND_REVERSED("ParticleRenderer::ParticleRenderer", 0x005778D0, &ParticleRender::Construct);
PARTREND_REVERSED("ParticleRenderer::SetMatrix", 0x00577970, &ParticleRender::SetMatrix);
PARTREND_REVERSED("ParticleRenderer::SetCamera", 0x005779E0, &ParticleRender::SetCamera);
PARTREND_REVERSED("ParticleRenderer::ScaledCopy", 0x00577B30, &ParticleRender::ScaledCopy);
PARTREND_REVERSED("ParticleRenderer::BuildViewQuad", 0x00577C70, &ParticleRender::BuildViewQuad);
PARTREND_REVERSED("ParticleRenderer::BuildQuad", 0x00577EF0, &ParticleRender::BuildQuad);
PARTREND_REVERSED("ParticleRenderer::Begin", 0x00578150, &ParticleRender::Begin);
PARTREND_REVERSED("ParticleRenderer::ApplyModifiers", 0x00578420, &ParticleRender::ApplyModifiers);
PARTREND_REVERSED("ParticleRenderer::WithinDistanceSq", 0x00578540, &ParticleRender::WithinDistanceSq);
PARTREND_REVERSED("ParticleRenderer::DistanceScale", 0x005785B0, &ParticleRender::DistanceScale);
PARTREND_REVERSED("ParticleRenderer::RenderAligned", 0x00578700, &ParticleRender::RenderAligned);
PARTREND_REVERSED("ParticleRenderer::RenderFacing", 0x00578F70, &ParticleRender::RenderFacing);
PARTREND_REVERSED("ParticleRenderer::RenderAlignedLocal", 0x00579340, &ParticleRender::RenderAlignedLocal);
PARTREND_REVERSED("ParticleRenderer::RenderFacingLocal", 0x00579C20, &ParticleRender::RenderFacingLocal);
PARTREND_REVERSED("ParticleRenderer::GroupsAligned", 0x0057A090, &ParticleRender::GroupsAligned);
PARTREND_REVERSED("ParticleRenderer::GroupsFacing", 0x0057A0D0, &ParticleRender::GroupsFacing);
PARTREND_REVERSED("ParticleRenderer::GroupsAlignedLocal", 0x0057A110, &ParticleRender::GroupsAlignedLocal);
PARTREND_REVERSED("ParticleRenderer::GroupsFacingLocal", 0x0057A150, &ParticleRender::GroupsFacingLocal);
PARTREND_REVERSED("ParticleRenderer::Render", 0x0057A190, &ParticleRender::Render);
