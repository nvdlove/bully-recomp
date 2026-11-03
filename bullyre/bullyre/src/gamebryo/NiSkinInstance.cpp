// NiSkinInstance::Deform (0x7D48C0): software skinning. Each bone's world transform, taken
// into the skin's space (the root parent's inverse world, the skin data's root-parent-to-skin
// transform) and through the bone's skin-to-bone transform, moves the vertices it weights;
// the results are summed by weight into the outputs (cleared first). Normals (and, with both
// output arrays, binormals and tangents: in the source after the normals, a vertex count
// apart) use the unscaled rotation and are unitized at the end.
//
// NiSkinInstance: +0x8 the skin data, +0x10 the root parent, +0x14 the bones. NiSkinData
// (0x48 bytes): +0xC the root-parent-to-skin transform, +0x40 the bone data (0x4C each),
// +0x44 their count. Main-thread maths: float, in the game's order.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni {
namespace {

struct BoneVertData {
    uint16_t vertex;   // +0x0
    uint16_t pad2;
    float weight;      // +0x4
};
static_assert(sizeof(BoneVertData) == 8);

struct BoneData {
    NiTransform skinToBone;     // +0x0
    NiBound bound;              // +0x34
    re::Ptr32<BoneVertData> weights;      // +0x44
    uint16_t vertexCount;       // +0x48
    uint16_t pad4A;
};
static_assert(offsetof(BoneData, weights) == 0x44);
static_assert(sizeof(BoneData) == 0x4C);

struct NiSkinData : NiObject {
    re::Ptr32<NiObject> partition;          // +0x8
    NiTransform rootParentToSkin; // +0xC
    re::Ptr32<BoneData> bones;              // +0x40
    uint32_t boneCount;           // +0x44
};
static_assert(offsetof(NiSkinData, bones) == 0x40);
static_assert(sizeof(NiSkinData) == 0x48);

struct NiSkinInstance : NiObject {
    re::Ptr32<NiSkinData> skinData;     // +0x8
    re::Ptr32<NiObject> partition;      // +0xC
    re::Ptr32<NiAVObject> rootParent;   // +0x10
    re::Ptr32<re::Ptr32<NiAVObject>> bones;       // +0x14
};

GAME_VAR(NiPoint3, NiPoint3_ZERO, 0x020E2D44);

GAME_FN(NiTransform*, __thiscall, Transform_Invert, 0x00770950, const NiTransform* self, NiTransform* result);
GAME_FN(NiTransform*, __thiscall, Transform_Multiply, 0x00406830, const NiTransform* self, NiTransform* result,
        const NiTransform* other);
GAME_FN(NiMatrix3*, __thiscall, Matrix3_Scale, 0x00768A10, const NiMatrix3* self, NiMatrix3* result, float scale);
GAME_FN(void, __cdecl, Point3_UnitizeVectors, 0x00770B10, NiPoint3* points, uint32_t count, uint32_t stride);

inline NiPoint3* Strided(NiPoint3* base, uint32_t index, uint32_t stride) {
    return reinterpret_cast<NiPoint3*>(reinterpret_cast<uint8_t*>(base) + index * stride);
}

// the outputs cleared (memset when packed, else NiPoint3::ZERO at the stride)
inline void Clear(NiPoint3* points, uint32_t count, uint32_t stride) {
    if (stride == 0xC) {
        crt::Memset(points, 0, count * 0xC);
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        *Strided(points, i, stride) = NiPoint3_ZERO;
    }
}

// the bone's transform into skin space: (skin * bone world) * skin-to-bone, and its rotation
// scaled
inline void BoneTransform(const NiTransform& skin, NiAVObject* bone, const BoneData& data, NiTransform& m,
                          NiMatrix3& scaled) {
    NiTransform first;
    NiTransform second;
    NiTransform* t = Transform_Multiply(&skin, &first, &bone->world);
    t = Transform_Multiply(t, &second, &data.skinToBone);
    m = *t;
    Matrix3_Scale(&m.rotate, &scaled, m.scale);
}

// a point through the scaled rotation and translation
inline NiPoint3 TransformPoint(const NiMatrix3& r, const NiPoint3& t, const NiPoint3& p) {
    NiPoint3 out;
    out.x = r.m[0][0] * p.x + r.m[0][1] * p.y + r.m[0][2] * p.z + t.x;
    out.y = r.m[1][0] * p.x + r.m[1][1] * p.y + r.m[1][2] * p.z + t.y;
    out.z = r.m[2][0] * p.x + r.m[2][1] * p.y + r.m[2][2] * p.z + t.z;
    return out;
}

// a direction through the rotation
inline NiPoint3 RotateVector(const NiMatrix3& r, const NiPoint3& v) {
    NiPoint3 out;
    out.x = r.m[0][0] * v.x + r.m[0][1] * v.y + r.m[0][2] * v.z;
    out.y = r.m[1][0] * v.x + r.m[1][1] * v.y + r.m[1][2] * v.z;
    out.z = r.m[2][0] * v.x + r.m[2][1] * v.y + r.m[2][2] * v.z;
    return out;
}

// out += v * weight (the products rounded first)
inline void Accumulate(NiPoint3* out, const NiPoint3& v, float weight) {
    float x = v.x * weight;
    float y = v.y * weight;
    float z = v.z * weight;
    out->x = x + out->x;
    out->y = y + out->y;
    out->z = z + out->z;
}

// 0x7D48C0 Deform(sourceVerts, sourceNormals, count, verts, normals, binormals, tangents,
// stride)
void __fastcall SkinInstance_DeformImpl(NiSkinInstance* self, re::Edx, const NiPoint3* sourceVerts,
                                        const NiPoint3* sourceNormals, uint32_t count, NiPoint3* verts,
                                        NiPoint3* normals, NiPoint3* binormals, NiPoint3* tangents,
                                        uint32_t stride) {
    NiTransform inverseRoot;
    Transform_Invert(&self->rootParent->world, &inverseRoot);
    NiTransform product;
    NiTransform skin = *Transform_Multiply(&self->skinData->rootParentToSkin, &product, &inverseRoot);
    BoneData* boneData = self->skinData->bones;
    uint32_t boneCount = self->skinData->boneCount;

    if (sourceNormals != nullptr && normals != nullptr) {
        if (binormals != nullptr && tangents != nullptr) {
            const NiPoint3* sourceBinormals =
                reinterpret_cast<const NiPoint3*>(reinterpret_cast<const uint8_t*>(sourceNormals) + count * 0xC);
            const NiPoint3* sourceTangents =
                reinterpret_cast<const NiPoint3*>(reinterpret_cast<const uint8_t*>(sourceBinormals) + count * 0xC);
            if (stride == 0xC) {
                crt::Memset(verts, 0, count * 0xC);
                crt::Memset(normals, 0, count * 0xC);
                crt::Memset(binormals, 0, count * 0xC);
                crt::Memset(tangents, 0, count * 0xC);
            } else {
                for (uint32_t i = 0; i < count; i++) {
                    *Strided(verts, i, stride) = NiPoint3_ZERO;
                    *Strided(normals, i, stride) = NiPoint3_ZERO;
                    *Strided(binormals, i, stride) = NiPoint3_ZERO;
                    *Strided(tangents, i, stride) = NiPoint3_ZERO;
                }
            }
            for (uint32_t b = 0; b < boneCount; b++) {
                const BoneData& data = boneData[b];
                uint32_t weightCount = data.vertexCount;
                NiTransform m;
                NiMatrix3 scaled;
                BoneTransform(skin, self->bones[b], data, m, scaled);
                const BoneVertData* weights = data.weights;
                for (uint32_t j = 0; j < weightCount; j++) {
                    uint32_t vertex = weights[j].vertex;
                    float weight = weights[j].weight;
                    NiPoint3 p = TransformPoint(scaled, m.translate, sourceVerts[vertex]);
                    NiPoint3 n = RotateVector(m.rotate, sourceNormals[vertex]);
                    NiPoint3 bn = RotateVector(m.rotate, sourceBinormals[vertex]);
                    NiPoint3 t = RotateVector(m.rotate, sourceTangents[vertex]);
                    Accumulate(Strided(verts, vertex, stride), p, weight);
                    Accumulate(Strided(normals, vertex, stride), n, weight);
                    Accumulate(Strided(binormals, vertex, stride), bn, weight);
                    Accumulate(Strided(tangents, vertex, stride), t, weight);
                }
            }
            Point3_UnitizeVectors(normals, count, stride);
            Point3_UnitizeVectors(binormals, count, stride);
            Point3_UnitizeVectors(tangents, count, stride);
            return;
        }
        Clear(verts, count, stride);
        Clear(normals, count, stride);
        for (uint32_t b = 0; b < boneCount; b++) {
            const BoneData& data = boneData[b];
            uint32_t weightCount = data.vertexCount;
            NiTransform m;
            NiMatrix3 scaled;
            BoneTransform(skin, self->bones[b], data, m, scaled);
            const BoneVertData* weights = data.weights;
            for (uint32_t j = 0; j < weightCount; j++) {
                uint32_t vertex = weights[j].vertex;
                float weight = weights[j].weight;
                NiPoint3 p = TransformPoint(scaled, m.translate, sourceVerts[vertex]);
                NiPoint3 n = RotateVector(m.rotate, sourceNormals[vertex]);
                Accumulate(Strided(verts, vertex, stride), p, weight);
                Accumulate(Strided(normals, vertex, stride), n, weight);
            }
        }
        Point3_UnitizeVectors(normals, count, stride);
        return;
    }

    Clear(verts, count, stride);
    for (uint32_t b = 0; b < boneCount; b++) {
        const BoneData& data = boneData[b];
        uint32_t weightCount = data.vertexCount;
        NiTransform m;
        NiMatrix3 scaled;
        BoneTransform(skin, self->bones[b], data, m, scaled);
        const BoneVertData* weights = data.weights;
        for (uint32_t j = 0; j < weightCount; j++) {
            uint32_t vertex = weights[j].vertex;
            float weight = weights[j].weight;
            NiPoint3 p = TransformPoint(scaled, m.translate, sourceVerts[vertex]);
            Accumulate(Strided(verts, vertex, stride), p, weight);
        }
    }
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISKININST_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

NISKININST_REVERSED("NiSkinInstance::Deform", 0x007D48C0, ni::SkinInstance_DeformImpl);
