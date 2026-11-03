// The overlay shapes drawn on a bone of the player (0x4E8CE0 draws one each frame while
// 0xC3B700 is set: the offset camera controller (0x5050D0) picks the bone, 0xC3B6F8, and
// the shape, 0xC3B6FC). Shapes 0 and 1 are the same code with other sizes (2 and 4
// middle sections): a box of RwIm3DVertex triangles, black at alpha 0x7F fading to 0 at
// the bevels, built in the bone's frame, then turned 180 degrees about X and Y about
// the bone's position and drawn through the Im3D pipe. Shape 2 is 0x4E48E0.
//
// The game unrolls everything and recomputes each vector per component; here each is
// computed once (the same float operations in the same order, so the same values).
// Arithmetic is float, as the game's x87 code in single precision (D3D9's default FPU
// mode; its constants are floats widened to double). Checked bit for bit against the
// original in an emulator; with the FPU in double precision the game's inline
// three-term sums (the ring corners) would round once, 1 ulp off ours.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace c12overlay {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// RwIm3DVertex: only the position and colour are written here
struct Im3DVertex {
    CVector pos;       // +0
    CVector normal;    // +0xC
    uint32_t color;    // +0x18 (ARGB)
    float u, v;        // +0x1C
};
static_assert(sizeof(Im3DVertex) == 0x24);
static_assert(offsetof(Im3DVertex, color) == 0x18);

// Gamebryo NiTransform (0x34 bytes)
struct NiTransform {
    float rotate[9];      // +0
    CVector translate;    // +0x24
    float scale;          // +0x30
};
static_assert(sizeof(NiTransform) == 0x34);

// the shared scratch buffer (also the handling/IPL file buffer)
GAME_ARRAY(Im3DVertex, vertexBuffer, 0x00BD7800);
GAME_VAR(int32_t, overlayVertexCount, 0x00CA2AE8);
GAME_VAR(int32_t, overlayUnknown, 0x00CA2AEC);

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(NiTransform*, __cdecl, CombineRotation, 0x00415EB0, NiTransform* out, const CVector* axis, float degrees, int mode);
GAME_FN(NiTransform*, __cdecl, CombineTranslation, 0x00415F50, NiTransform* out, const CVector* translation, int mode);
// the Im3D pipe: Transform keeps the arguments (flags | 0x18) and returns the vertices
GAME_FN(void*, __cdecl, Im3D_Transform, 0x005EFAF0, void* vertices, int count, NiTransform* transform, uint32_t flags);
GAME_FN(void, __cdecl, Im3D_SetFlag, 0x005EFAE0, uint8_t value);   // (byte at 0xDA7518)
GAME_FN(void, __cdecl, Im3D_RenderPrimitive, 0x005F09D0, int type);
GAME_FN(uint8_t, __cdecl, Im3D_End, 0x005EFB20);

constexpr uint32_t kShade = 0x7F000000;   // black, half alpha
constexpr uint32_t kClear = 0x00000000;

CVector Add(const CVector& a, const CVector& b) {
    CVector out;
    VectorAdd(&out, &a, &b);
    return out;
}
CVector Sub(const CVector& a, const CVector& b) {
    CVector out;
    VectorSub(&out, &a, &b);
    return out;
}
CVector Scale(const CVector& v, float k) {
    return {v.x * k, v.y * k, v.z * k};
}

// the sizes (A, B, C: the bone matrix's columns)
struct ShapeParams {
    float back;        // B: the box's centre back from the bone
    float halfWidth;   // A and C: half the outer box
    float depth;       // C: the ring's centre, half of it forward
    float sideC;       // C: the ring's half width
    float outerA;      // A: the ring's far ends
    float innerA;      // A: the ring's near ends
    float bevelC;      // C
    float bevelA;      // A (near end)
    float bevelA2;     // A (far end)
    float step;        // C: the step of each middle section
    int32_t sections;  // the middle sections
    float farB;        // B: the outer box's length
    int32_t vertices;  // written in all (0x9C, 0xF0)
};

struct Writer {
    Im3DVertex* next;
    void Put(const CVector& pos, uint32_t color) {
        next->pos = pos;
        next->color = color;
        next++;
    }
};

// the ring about `centre`: its four ends and their bevels, 24 vertices
struct Ring {
    CVector nearA0;   // centre + C - innerA
    CVector farA0;    // centre - C - outerA
    CVector farA1;    // centre - C + outerA
    CVector nearA1;   // centre + C + innerA
    CVector bevel0, bevel1, bevel2, bevel3;
};

Ring MakeRing(const CVector& centre, const CVector& sideC, const CVector& innerA, const CVector& outerA,
              const CVector& bevelA, const CVector& bevelA2, const CVector& bevelC) {
    Ring r;
    r.nearA0 = Sub(Add(centre, sideC), innerA);
    r.farA0 = Sub(Sub(centre, sideC), outerA);
    r.farA1 = Add(Sub(centre, sideC), outerA);
    r.nearA1 = Add(Add(centre, sideC), innerA);
    r.bevel0 = Sub(Add(r.nearA0, bevelA), bevelC);
    r.bevel1 = Add(Add(r.farA0, bevelA2), bevelC);
    r.bevel2 = Add(Sub(r.farA1, bevelA2), bevelC);
    r.bevel3 = Sub(Sub(r.nearA1, bevelA), bevelC);
    return r;
}

// (the corners pulled in by the nudges, inline x87 sums in the game)
void PutRing(Writer& w, const Ring& r, const CVector& nudgeC, const CVector& nudgeA) {
    CVector c0{(r.nearA0.x - nudgeC.x) - nudgeA.x, (r.nearA0.y - nudgeC.y) - nudgeA.y, (r.nearA0.z - nudgeC.z) - nudgeA.z};
    CVector c1{(r.farA0.x + nudgeC.x) - nudgeA.x, (r.farA0.y + nudgeC.y) - nudgeA.y, (r.farA0.z + nudgeC.z) - nudgeA.z};
    CVector c2{r.farA1.x + nudgeC.x + nudgeA.x, r.farA1.y + nudgeC.y + nudgeA.y, r.farA1.z + nudgeC.z + nudgeA.z};
    CVector c3{(r.nearA1.x - nudgeC.x) + nudgeA.x, (r.nearA1.y - nudgeC.y) + nudgeA.y, (r.nearA1.z - nudgeC.z) + nudgeA.z};
    w.Put(c0, kShade);
    w.Put(r.bevel0, kClear);
    w.Put(c1, kShade);
    w.Put(r.bevel0, kClear);
    w.Put(c1, kShade);
    w.Put(r.bevel1, kClear);
    w.Put(c1, kShade);
    w.Put(r.bevel1, kClear);
    w.Put(r.bevel2, kClear);
    w.Put(c1, kShade);
    w.Put(r.bevel2, kClear);
    w.Put(c2, kShade);
    w.Put(c2, kShade);
    w.Put(c3, kShade);
    w.Put(r.bevel2, kClear);
    w.Put(c3, kShade);
    w.Put(r.bevel2, kClear);
    w.Put(r.bevel3, kClear);
    w.Put(c3, kShade);
    w.Put(r.bevel3, kClear);
    w.Put(r.bevel0, kClear);
    w.Put(c3, kShade);
    w.Put(r.bevel0, kClear);
    w.Put(c0, kShade);
}

// the ring's two sides joined to the section's face, 12 vertices
void PutRingSides(Writer& w, const Ring& r, const CVector& centre, const CVector& sideC, const CVector& halfA,
                  const CVector& nudgeC) {
    CVector nearFace0 = Sub(Add(centre, sideC), halfA);
    CVector nearFace1 = Add(Add(centre, sideC), halfA);
    w.Put(nearFace0, kShade);
    w.Put(r.nearA0, kShade);
    w.Put(Add(r.farA0, nudgeC), kShade);
    w.Put(nearFace0, kShade);
    w.Put(Add(r.farA0, nudgeC), kShade);
    w.Put(Add(Sub(Sub(centre, sideC), halfA), nudgeC), kShade);
    w.Put(nearFace1, kShade);
    w.Put(r.nearA1, kShade);
    w.Put(Add(r.farA1, nudgeC), kShade);
    w.Put(nearFace1, kShade);
    w.Put(Add(r.farA1, nudgeC), kShade);
    w.Put(Add(Add(Sub(centre, sideC), halfA), nudgeC), kShade);
}

// 0x4DC0F0 / 0x4E04E0 (`m`: the bone's 3x4 matrix, rows, translation at [9])
void RenderShape(const float* m, const ShapeParams& p) {
    overlayVertexCount = p.vertices;
    overlayUnknown = 0;
    CVector axisA{m[0], m[3], m[6]};
    CVector axisB{m[1], m[4], m[7]};
    CVector axisC{m[2], m[5], m[8]};
    CVector pos{m[9], m[10], m[11]};

    CVector zeroC = Scale(axisC, 0.0f);   // (kept: its sign reaches the sums)
    CVector back = Scale(axisB, p.back);
    CVector base = Add(Add(pos, back), zeroC);
    CVector halfC = Scale(axisC, p.halfWidth);
    CVector halfA = Scale(axisA, p.halfWidth);
    // the outer box's corners at the bone
    CVector corner0 = Add(Add(base, halfA), halfC);
    CVector corner1 = Add(Sub(base, halfA), halfC);
    CVector corner2 = Sub(Add(base, halfA), halfC);
    CVector corner3 = Sub(Sub(base, halfA), halfC);

    CVector depthC = Scale(axisC, p.depth);
    CVector halfDepth = Scale(depthC, 0.5f);
    CVector centre = Sub(base, halfDepth);
    CVector sideC = Scale(axisC, p.sideC);
    CVector outerA = Scale(axisA, p.outerA);
    CVector innerA = Scale(axisA, p.innerA);
    CVector bevelC = Scale(axisC, p.bevelC);
    CVector bevelA = Scale(axisA, p.bevelA);
    CVector bevelA2 = Scale(axisA, p.bevelA2);
    CVector nudgeC = Scale(axisC, -0.0001f);
    CVector nudgeA = Scale(axisA, 0.0001f);

    Writer w{vertexBuffer};
    Ring ring = MakeRing(centre, sideC, innerA, outerA, bevelA, bevelA2, bevelC);
    PutRing(w, ring, nudgeC, nudgeA);

    // the first section's face
    w.Put(corner0, kShade);
    w.Put(corner1, kShade);
    w.Put(Add(Sub(Add(centre, sideC), halfA), nudgeC), kShade);
    w.Put(corner0, kShade);
    w.Put(Add(Add(Add(centre, sideC), halfA), nudgeC), kShade);
    w.Put(Add(Sub(Add(centre, sideC), halfA), nudgeC), kShade);
    PutRingSides(w, ring, centre, sideC, halfA, nudgeC);

    // the middle sections, each a step further along C
    CVector step = Scale(axisC, p.step);
    centre = {centre.x + step.x, centre.y + step.y, centre.z + step.z};
    for (int32_t i = p.sections; i != 0; i--) {
        CVector back0 = Sub(Sub(centre, sideC), step);
        w.Put(Sub(back0, halfA), kShade);
        w.Put(Add(back0, halfA), kShade);
        w.Put(Add(Sub(Add(centre, sideC), halfA), nudgeC), kShade);
        w.Put(Add(back0, halfA), kShade);
        w.Put(Add(Sub(Add(centre, sideC), halfA), nudgeC), kShade);
        w.Put(Add(Add(Add(centre, sideC), halfA), nudgeC), kShade);
        ring = MakeRing(centre, sideC, innerA, outerA, bevelA, bevelA2, bevelC);
        PutRing(w, ring, nudgeC, nudgeA);
        PutRingSides(w, ring, centre, sideC, halfA, nudgeC);
        centre = {centre.x + step.x, centre.y + step.y, centre.z + step.z};
    }

    // the last face (the other operand order than the sections'), then the outer box
    // along B
    CVector last = Sub(Sub(centre, step), sideC);
    w.Put(Sub(last, halfA), kShade);
    w.Put(Add(last, halfA), kShade);
    w.Put(corner2, kShade);
    w.Put(Sub(last, halfA), kShade);
    w.Put(corner3, kShade);
    w.Put(corner2, kShade);
    CVector nudgeB = Scale(axisB, -0.0001f);
    CVector farB = Scale(axisB, p.farB);
    w.Put(Add(corner0, nudgeB), kShade);
    w.Put(Add(corner2, nudgeB), kShade);
    w.Put(Add(corner2, farB), kShade);
    w.Put(Add(corner0, nudgeB), kShade);
    w.Put(Add(corner2, farB), kShade);
    w.Put(Add(corner0, farB), kShade);
    w.Put(Add(corner1, nudgeB), kShade);
    w.Put(Add(corner3, nudgeB), kShade);
    w.Put(Add(corner3, farB), kShade);
    w.Put(Add(corner1, nudgeB), kShade);
    w.Put(Add(corner3, farB), kShade);
    w.Put(Add(corner1, farB), kShade);
    w.Put(corner1, kShade);
    w.Put(corner0, kShade);
    w.Put(Add(corner0, farB), kShade);
    w.Put(corner1, kShade);
    w.Put(Add(corner0, farB), kShade);
    w.Put(Add(corner1, farB), kShade);
    w.Put(corner3, kShade);
    w.Put(corner2, kShade);
    w.Put(Add(corner2, farB), kShade);
    w.Put(corner3, kShade);
    w.Put(Add(corner2, farB), kShade);
    w.Put(Add(corner3, farB), kShade);

    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(0xE, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    SetRenderState(9, 2);
    SetRenderState(1, 0);
    SetRenderState(0x14, 1);

    // turned 180 degrees about X and Y (so about Z) around the bone
    NiTransform transform;   // (only the scale set before the first combine replaces it)
    transform.scale = 1.0f;
    CVector toOrigin{-m[9], -m[10], -m[11]};
    CVector unitX{1.0f, 0.0f, 0.0f};
    CVector unitY{0.0f, 1.0f, 0.0f};
    CombineTranslation(&transform, &toOrigin, 0);
    CombineRotation(&transform, &unitX, 180.0f, 2);
    CombineRotation(&transform, &unitY, 180.0f, 2);
    CombineTranslation(&transform, &pos, 2);
    if (Im3D_Transform(vertexBuffer, p.vertices, &transform, 0x18)) {
        Im3D_SetFlag(1);
        Im3D_RenderPrimitive(3);
        Im3D_End();
        Im3D_SetFlag(0);
    }
    SetRenderState(6, 1);
}

constexpr ShapeParams kShape0{-0.062f, 0.12f, -0.04f, 0.0075f, 0.1f, 0.0925f, 0.003f, 0.003f, 0.006f, -0.02f, 2, 0.12f, 0x9C};
constexpr ShapeParams kShape1{-0.25f, 0.36f, -0.212f, 0.0115f, 0.185f, 0.2f, 0.006f, 0.012f, 0.006f, -0.053f, 4, 0.36f, 0xF0};

void __cdecl RenderShape0(const float* boneMatrix) {
    RenderShape(boneMatrix, kShape0);
}

void __cdecl RenderShape1(const float* boneMatrix) {
    RenderShape(boneMatrix, kShape1);
}

#define OVERLAY_C12_REVERSED(name, address, fn)    \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)
OVERLAY_C12_REVERSED("BoneOverlay::RenderShape0", 0x004DC0F0, &RenderShape0);
OVERLAY_C12_REVERSED("BoneOverlay::RenderShape1", 0x004E04E0, &RenderShape1);

} // namespace c12overlay
