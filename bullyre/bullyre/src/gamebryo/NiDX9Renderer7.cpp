// NiDX9Renderer's immediate rendering (NiDX9Renderer.h): Do_RenderPoints (each particle a camera
// facing quad, rotated or not, into a dynamic vertex buffer), Do_RenderLines (the lines' vertices
// and an index buffer from their connectivity), Do_EndBatch (the batched geometry drawn through
// the batch's shader, pass by pass), DrawSkinnedPrimitive (each skin partition through the
// geometry's shader) and the screen texture's vertex arrays and quad indices.
//
// The shaders (NiD3DShaderInterface) by slot: 6 PreProcessPipeline (0: go on), 7 UpdatePipeline,
// 8 SetupRenderingPass, 9 SetupTransformations, 10 SetupShaderPrograms, 11
// PrepareGeometryForRendering, 12 PostRender, 13 PostProcessPipeline, 15 FirstPass, 16 NextPass.
// The renderer's slot 64 gives the error shader. Colours packed as D3DCOLOR (each channel times
// 255, rounded as fistp does).

#include <cstddef>
#include <cstdint>
#include <xmmintrin.h>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"
#include "runflags.h"

namespace ni::dx9 {
namespace {

inline void* const kShaderInterface_RTTI = reinterpret_cast<void*>(0x020E6E0C);
GAME_VAR(uint32_t, kShaderInterface_ClassBit, 0x020E6E14);

enum ShaderSlot : int {
    kPreProcessPipeline = 6,
    kUpdatePipeline = 7,
    kSetupRenderingPass = 8,
    kSetupTransformations = 9,
    kSetupShaderPrograms = 10,
    kPrepareGeometryForRendering = 11,
    kPostRender = 12,
    kPostProcessPipeline = 13,
    kFirstPass = 15,
    kNextPass = 16,
};

GAME_FN(uint8_t*, __cdecl, IsKindOfObject, 0x00401FF0, void* rtti, void* object);   // NiDynamicCast
GAME_FN(void*, __thiscall, Geometry_GetShader, 0x00761630, void* geometry);
GAME_FN(void*, __thiscall, Geometry_GetActiveMaterial, 0x004028D0, void* geometry);
GAME_FN(uint8_t, __thiscall, Geometry_SetActiveMaterial, 0x00402880, void* geometry, void* material);
GAME_FN(uint8_t, __thiscall, Geometry_ApplyAndSetActiveMaterial, 0x00761F70, void* geometry, void* material,
        uint32_t extraData);
GAME_FN(void, __thiscall, GroupManager_AddObjectToGroup, 0x00899470, void* self, void* group, void* data,
        void* skin, int hwSkinned, uint32_t bonesPerPartition, uint32_t bonesPerVertex);
GAME_FN(void, __thiscall, BufferData_SetStreamCount, 0x00896F10, void* self, uint32_t count);
GAME_FN(void, __thiscall, BufferData_SetFVF, 0x00897010, void* self, uint32_t fvf);
GAME_FN(void, __thiscall, BufferData_ReleaseIB, 0x00896EE0, void* self);
GAME_FN(uint8_t, __thiscall, VBManager_CreateVertexBuffer, 0x00887230, void* self, void* bufferData, uint32_t stream);
GAME_FN(uint8_t*, __thiscall, VBManager_LockVB, 0x00887280, void* self, void* vb, uint32_t offset, uint32_t size,
        uint32_t flags);
GAME_FN(void, __thiscall, VBManager_UnlockVB, 0x00887360, void* self, void* vb);
GAME_FN(uint8_t*, __thiscall, VBManager_PackScreenTextureVB, 0x008877E0, void* self, void* bufferData,
        uint32_t vertices, void* positions, void* colors, void* textures, uint32_t dirty, void* chip, uintptr_t a8);
GAME_FN(void*, __thiscall, IBManager_PackBuffer, 0x00897200, void* self, const void* indices, uint32_t count,
        uint32_t activeCount, void* oldBuffer, uint32_t* size, int writeOnly, uint32_t usage);
GAME_FN(void*, __thiscall, IBManager_CreateLinesIB, 0x00897510, void* self, uint32_t vertices, const void* flags,
        uint32_t* indexCount, int a4, int a5);
GAME_FN(void*, __thiscall, IBManager_GetQuadIB, 0x00897360, void* self, uint32_t vertices, int a2, int a3, int a4);
GAME_FN(void, __thiscall, RenderState_SetBoneCount, 0x00746AA0, void* self, uint32_t bones);
GAME_FN(void, __thiscall, RenderState_SetModelTransform, 0x00882E60, void* self, const void* transform);
GAME_FN(void, __cdecl, MakeD3DMatrix, 0x00406670, void* d3d, const void* rotate, const void* translate, float scale);
GAME_FN(float*, __cdecl, Point3_TimesMatrix, 0x00768AD0, float* result, const float* point, const void* matrix);
GAME_FN(float*, __thiscall, Point3_Cross, 0x007D5660, const float* self, float* result, const float* other);
GAME_FN(float*, __thiscall, Point3_Subtract, 0x0040D9C0, const float* self, float* result, const float* other);
GAME_FN(float*, __thiscall, Point3_Add, 0x004065A0, const float* self, float* result, const float* other);
GAME_FN(float*, __cdecl, ScaleVector, 0x00412320, float* result, float scale, const float* vector);
GAME_FN(void, __thiscall, Renderer_ClearBatch, 0x00877E30, void* self);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);          // NiMemObject::operator new[]
GAME_FN(void, __cdecl, NiDeleteArray, 0x00752570, void* p, uint32_t size);   // operator delete[]
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* constructor);
inline void* const kColorAConstruct = reinterpret_cast<void*>(0x005EFB40);   // NiColorA()

// a channel times 255 (in float), rounded to nearest as fistp does (out of range: 0x80000000)
inline uint32_t Channel(float value) {
    return uint32_t(_mm_cvtss_si32(_mm_set_ss(value * 255.0f)));
}
// NiColorA to D3DCOLOR (the channels or'ed in unmasked)
inline uint32_t PackColor(const float* c) {
    uint32_t a = Channel(c[3]);
    uint32_t r = Channel(c[0]);
    uint32_t g = Channel(c[1]);
    uint32_t b = Channel(c[2]);
    return ((a << 8 | r) << 8 | g) << 8 | b;
}

inline void* Chip0(uint8_t* bufferData) {
    return At<uint32_t>(bufferData, 0x20) != 0 ? At<re::Ptr32<void>*>(bufferData, 0x28)[0] : nullptr;
}
inline uint32_t Stride0(uint8_t* bufferData) {
    return At<uint32_t>(bufferData, 0x20) != 0 ? At<uint32_t*>(bufferData, 0x24)[0] : 0;
}

// the render state's shader constants committed (its constant cache's slot 2)
inline void CommitShaderConstants(uint8_t* self) {
    Virtual<void>(At<void*>(At<uint8_t*>(self, kRenderState), 0x20F0), 2);
}

// a particle's quad (vertex: position, normal, colour, u, v; 0x24 bytes)
inline void WriteQuad(float* v, const float* p, const float* e, const float* f, const float* n, uint32_t color) {
    v[0] = p[0] - e[0];
    v[1] = p[1] - e[1];
    v[2] = p[2] - e[2];
    v[3] = n[0];
    v[4] = n[1];
    v[5] = n[2];
    reinterpret_cast<uint32_t*>(v)[6] = color;
    v[7] = 0.0f;
    v[8] = 1.0f;
    v[9] = p[0] - f[0];
    v[10] = p[1] - f[1];
    v[11] = p[2] - f[2];
    v[12] = n[0];
    v[13] = n[1];
    v[14] = n[2];
    reinterpret_cast<uint32_t*>(v)[15] = color;
    v[16] = 1.0f;
    v[17] = 1.0f;
    v[18] = e[0] + p[0];
    v[19] = e[1] + p[1];
    v[20] = e[2] + p[2];
    v[21] = n[0];
    v[22] = n[1];
    v[23] = n[2];
    reinterpret_cast<uint32_t*>(v)[24] = color;
    v[25] = 1.0f;
    v[26] = 0.0f;
    v[27] = p[0] + f[0];
    v[28] = p[1] + f[1];
    v[29] = p[2] + f[2];
    v[30] = n[0];
    v[31] = n[1];
    v[32] = n[2];
    reinterpret_cast<uint32_t*>(v)[33] = color;
    v[34] = 0.0f;
    v[35] = 0.0f;
}

} // namespace

// 0x87CD00 (thiscall): NiDX9Renderer::Do_RenderPoints(particles) (the shader from the legacy
// default material; the active particles' quads: rotated by their angles (cos / sin) or not, the
// right and up directions in model space from +0x450 / +0x45C, the normal their cross product;
// drawn with a quad index buffer)
void __fastcall Renderer_Do_RenderPoints(uint8_t* self, re::Edx, uint8_t* particles) {
    if (self[kDeviceLost] != 0) {
        return;
    }
    uint8_t* data = At<uint8_t*>(particles, 0xCC);
    alignas(4) uint8_t world[0x34];
    crt::Memcpy(world, particles + 0x60, 0x34);
    float bound[4];
    crt::Memcpy(bound, particles + 0x1C, 0x10);
    uint32_t active = At<uint16_t>(data, 0x30);
    uint32_t vertexCount = At<uint16_t>(data, 0xC);
    if (active == 0) {
        return;
    }
    uint32_t materialIndex = At<uint32_t>(particles, 0xE0);
    void* activeMaterial = materialIndex < At<uint32_t>(particles, 0xDC)
                               ? At<void*>(At<uint8_t*>(particles, 0xD4) + materialIndex * 0x10, 4)
                               : nullptr;
    Geometry_ApplyAndSetActiveMaterial(particles, At<void*>(self, kLegacyDefaultMaterial), 0xFFFFFFFF);
    void* shader = IsKindOfObject(kShaderInterface_RTTI, Geometry_GetShader(particles));
    Geometry_SetActiveMaterial(particles, activeMaterial);
    if (shader == nullptr) {
        NiOutputDebugString("Do_RenderPoints> No shader found for object %s!\nUsing Error Shader!\n",
                            At<const char*>(particles, 8));
        shader = Virtual<void*>(self, 64);
    }
    GroupManager_AddObjectToGroup(At<void*>(self, kManager6A8), At<void*>(self, 0x6B0), data, nullptr, 0, 0, 0);
    uint8_t* bufferData = At<uint8_t*>(data, 0x3C);
    BufferData_SetStreamCount(bufferData, 1);
    At<uint32_t>(bufferData, 0x1C) = vertexCount * 4;
    At<uint32_t>(bufferData, 0x18) = active * 4;
    At<uint32_t>(bufferData, 0x40) = active * 2;
    At<uint32_t>(bufferData, 0x44) = vertexCount * 2;
    At<void*>(bufferData, 0x50) = nullptr;
    At<void*>(bufferData, 0x4C) = nullptr;
    At<uint32_t>(bufferData, 0x48) = 1;
    At<uint32_t>(bufferData, 4) = 0x1C00000;
    void* indexBuffer = IBManager_GetQuadIB(At<void*>(self, kIBManager), active * 4, 0, 0, 1);
    if (indexBuffer == nullptr) {
        return;
    }
    const float* radii = At<const float*>(data, 0x44);
    const float* rotations = At<const float*>(data, 0x4C);
    const float* points = At<const float*>(data, 0x20);
    const float* colors = At<const float*>(data, 0x28);
    const float* sizes = At<const float*>(data, 0x40);
    RenderState_SetBoneCount(At<void*>(self, kRenderState), 0);
    MakeD3DMatrix(self + kWorld, world, world + 0x24, At<float>(world, 0x30));
    Com<long>(At<void*>(self, kDevice), kDevSetTransform, 0x100u, self + kWorld);
    RenderState_SetModelTransform(At<void*>(self, kRenderState), world);
    float temp[3];
    const float* p = Point3_TimesMatrix(temp, reinterpret_cast<const float*>(self + 0x450), world);
    At<float>(self, 0x468) = p[0];
    At<float>(self, 0x46C) = p[1];
    At<float>(self, 0x470) = p[2];
    p = Point3_TimesMatrix(temp, reinterpret_cast<const float*>(self + 0x45C), world);
    At<float>(self, 0x474) = p[0];
    At<float>(self, 0x478) = p[1];
    At<float>(self, 0x47C) = p[2];
    const float* right = reinterpret_cast<const float*>(self + 0x468);
    const float* up = reinterpret_cast<const float*>(self + 0x474);
    float normal[3];
    Point3_Cross(right, normal, up);
    BufferData_SetFVF(bufferData, 0x152);   // XYZ | NORMAL | DIFFUSE | TEX1
    if (At<uint32_t>(bufferData, 0x20) != 0) {
        At<uint32_t*>(bufferData, 0x24)[0] = 0x24;
    }
    VBManager_CreateVertexBuffer(At<void*>(self, kVBManager), bufferData, 0);
    uint8_t* chip = static_cast<uint8_t*>(Chip0(bufferData));
    if (At<void*>(chip, 8) == nullptr) {
        return;
    }
    float* v = reinterpret_cast<float*>(VBManager_LockVB(At<void*>(self, kVBManager), At<void*>(chip, 8),
                                                         At<uint32_t>(chip, 0xC), active * 4 * 0x24,
                                                         At<uint32_t>(chip, 0x10)));
    if (v == nullptr) {
        return;
    }
    if (rotations != nullptr) {
        for (uint32_t i = 0; i < active; i++) {
            uint32_t color = colors != nullptr ? PackColor(colors) : 0xFFFFFFFF;
            float size = *radii * *sizes;
            float angle = *rotations;
            float c = crt::Cosf(angle);   // (fsincos in the game: ours may differ in the last place)
            float s = crt::Sinf(angle);
            float a = (c + s) * size;
            float b = (c - s) * size;   // (fsubrp: cos - sin)
            float r1[3];
            float r2[3];
            float r3[3];
            float r4[3];
            ScaleVector(r1, b, up);
            ScaleVector(r2, a, right);
            float e[3] = {r1[0] + r2[0], r1[1] + r2[1], r1[2] + r2[2]};
            ScaleVector(r3, a, up);
            ScaleVector(r4, -b, right);
            float f[3] = {r3[0] + r4[0], r3[1] + r4[1], r3[2] + r4[2]};
            WriteQuad(v, points, e, f, normal, color);
            v += 36;
            points += 3;
            if (colors != nullptr) {
                colors += 4;
            }
            radii++;
            rotations++;
            sizes++;
        }
    } else {
        float minusRight[3];   // up - right
        float plus[3];         // right + up
        Point3_Subtract(up, minusRight, right);
        Point3_Add(up, plus, right);
        for (uint32_t i = 0; i < active; i++) {
            uint32_t color = colors != nullptr ? PackColor(colors) : 0xFFFFFFFF;
            float size = *radii * *sizes;
            float e[3] = {plus[0] * size, plus[1] * size, plus[2] * size};
            float f[3] = {minusRight[0] * size, minusRight[1] * size, size * minusRight[2]};
            WriteQuad(v, points, e, f, normal, color);
            v += 36;
            points += 3;
            if (colors != nullptr) {
                colors += 4;
            }
            sizes++;
            radii++;
        }
    }
    VBManager_UnlockVB(At<void*>(self, kVBManager), At<void*>(chip, 8));
    void* const& state = At<void*>(self, 0xC);     // (read at each call, as the game does)
    void* const& effects = At<void*>(self, 0x10);
    if (Virtual<uint32_t>(shader, kPreProcessPipeline, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound)) != 0) {
        return;
    }
    Virtual<uint32_t>(shader, kUpdatePipeline, nullptr, nullptr, bufferData, state, effects, static_cast<void*>(world),
                      static_cast<void*>(bound));
    uint32_t passes = Virtual<uint32_t>(shader, kFirstPass);
    Com<long>(At<void*>(self, kDevice), kDevSetStreamSource, 0u, At<void*>(chip, 8), 0u, Stride0(bufferData));
    Com<long>(At<void*>(self, kDevice), kDevSetIndices, indexBuffer);
    while (passes != 0) {
        Virtual<uint32_t>(shader, kSetupRenderingPass, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound));
        Virtual<uint32_t>(shader, kSetupShaderPrograms, nullptr, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound));
        CommitShaderConstants(self);
        re::CountDraw(Com<long>(At<void*>(self, kDevice), kDevDrawIndexedPrimitive, At<uint32_t>(bufferData, 0x3C),
                  At<uint32_t>(bufferData, 0x38), 0u, At<uint32_t>(bufferData, 0x18), 0u, At<uint32_t>(bufferData, 0x40)), 0);
        Virtual<uint32_t>(shader, kPostRender, nullptr, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound));
        passes = Virtual<uint32_t>(shader, kNextPass);
    }
    Virtual<uint32_t>(shader, kPostProcessPipeline, nullptr, nullptr, bufferData, state, effects,
                      static_cast<void*>(world), static_cast<void*>(bound));
}

// 0x87DD40 (thiscall): NiDX9Renderer::Do_RenderLines(lines) (positions, colours when there are
// any, texture coordinates when there are any; a line list index buffer from the connectivity)
void __fastcall Renderer_Do_RenderLines(uint8_t* self, re::Edx, uint8_t* lines) {
    if (self[kDeviceLost] != 0) {
        return;
    }
    uint8_t* data = At<uint8_t*>(lines, 0xCC);
    uint32_t vertices = At<uint16_t>(data, 0xC);
    if (vertices == 0) {
        return;
    }
    alignas(4) uint8_t world[0x34];
    crt::Memcpy(world, lines + 0x60, 0x34);
    float bound[4];
    crt::Memcpy(bound, lines + 0x1C, 0x10);
    GroupManager_AddObjectToGroup(At<void*>(self, kManager6A8), At<void*>(self, 0x6B0), data, nullptr, 0, 0, 0);
    const float* points = At<const float*>(data, 0x20);
    const float* uvs = At<const float*>(data, 0x2C);
    const float* colors = At<const float*>(data, 0x28);
    uint32_t indexCount = 0;
    void* indexBuffer =
        IBManager_CreateLinesIB(At<void*>(self, kIBManager), vertices, At<void*>(data, 0x40), &indexCount, 0, 1);
    if (indexBuffer == nullptr || indexCount == 0) {
        return;
    }
    uint32_t fvf = 2;   // XYZ
    uint32_t stride = 0xC;
    if (uvs != nullptr) {
        fvf = 0x102;   // | TEX1
        stride = 0x14;
    }
    if (colors != nullptr) {
        fvf |= 0x40;   // DIFFUSE
        stride += 4;
    }
    void* activeMaterial = Geometry_GetActiveMaterial(lines);
    Geometry_ApplyAndSetActiveMaterial(lines, At<void*>(self, kLegacyDefaultMaterial), 0xFFFFFFFF);
    void* shader = IsKindOfObject(kShaderInterface_RTTI, Geometry_GetShader(lines));
    Geometry_SetActiveMaterial(lines, activeMaterial);
    if (shader == nullptr) {
        NiOutputDebugString("Do_RenderLines> No shader found for object %s!\nUsing Error Shader!\n",
                            At<const char*>(lines, 8));
        shader = Virtual<void*>(self, 64);
    }
    uint8_t* bufferData = At<uint8_t*>(data, 0x3C);
    BufferData_SetStreamCount(bufferData, 1);
    At<uint32_t>(bufferData, 0x18) = vertices;
    At<uint32_t>(bufferData, 0x1C) = vertices;
    BufferData_SetFVF(bufferData, fvf);
    if (At<uint32_t>(bufferData, 0x20) != 0) {
        At<uint32_t*>(bufferData, 0x24)[0] = stride;
    }
    VBManager_CreateVertexBuffer(At<void*>(self, kVBManager), bufferData, 0);
    uint8_t* chip = static_cast<uint8_t*>(Chip0(bufferData));
    if (At<void*>(chip, 8) == nullptr) {
        return;
    }
    float* out = reinterpret_cast<float*>(VBManager_LockVB(At<void*>(self, kVBManager), At<void*>(chip, 8),
                                                           At<uint32_t>(chip, 0xC), At<uint32_t>(chip, 0x14),
                                                           At<uint32_t>(chip, 0x10)));
    if (out == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < vertices; i++) {
        out[0] = points[0];
        out[1] = points[1];
        out[2] = points[2];
        points += 3;
        out += 3;
        if (colors != nullptr) {
            *reinterpret_cast<uint32_t*>(out) = PackColor(colors);
            colors += 4;
            out++;
        }
        if (uvs != nullptr) {
            out[0] = uvs[0];
            out[1] = uvs[1];
            uvs += 2;
            out += 2;
        }
    }
    VBManager_UnlockVB(At<void*>(self, kVBManager), At<void*>(chip, 8));
    void* const& state = At<void*>(self, 0xC);     // (read at each call, as the game does)
    void* const& effects = At<void*>(self, 0x10);
    if (Virtual<uint32_t>(shader, kPreProcessPipeline, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound)) != 0) {
        return;
    }
    Virtual<uint32_t>(shader, kUpdatePipeline, nullptr, nullptr, bufferData, state, effects, static_cast<void*>(world),
                      static_cast<void*>(bound));
    uint32_t passes = Virtual<uint32_t>(shader, kFirstPass);
    Com<long>(At<void*>(self, kDevice), kDevSetStreamSource, 0u, At<void*>(chip, 8), 0u, Stride0(bufferData));
    Com<long>(At<void*>(self, kDevice), kDevSetIndices, indexBuffer);
    while (passes != 0) {
        Virtual<uint32_t>(shader, kSetupRenderingPass, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound));
        Virtual<uint32_t>(shader, kSetupTransformations, nullptr, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound));
        Virtual<uint32_t>(shader, kSetupShaderPrograms, nullptr, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound));
        CommitShaderConstants(self);
        re::CountDraw(Com<long>(At<void*>(self, kDevice), kDevDrawIndexedPrimitive, 2u, At<uint32_t>(bufferData, 0x38), 0u,
                  At<uint32_t>(bufferData, 0x18), 0u, indexCount >> 1), 1);   // D3DPT_LINELIST
        Virtual<uint32_t>(shader, kPostRender, nullptr, nullptr, nullptr, bufferData, state, effects,
                          static_cast<void*>(world), static_cast<void*>(bound));
        passes = Virtual<uint32_t>(shader, kNextPass);
    }
    Virtual<uint32_t>(shader, kPostProcessPipeline, nullptr, nullptr, bufferData, state, effects,
                      static_cast<void*>(world), static_cast<void*>(bound));
}

// 0x87E420 (thiscall): NiDX9Renderer::Do_EndBatch (the batch list (+0x43C: geometry, partition,
// buffer data, next) through the batch shader (+0x44C) with the batch's property / effect states;
// each geometry's dirty flags cleared; the batch let go). (A partition with more bones than the
// hardware takes under the error shader is reported and retried forever: the game's loop doesn't
// move on.)
void __fastcall Renderer_Do_EndBatch(uint8_t* self, re::Edx) {
    uint8_t* head = At<uint8_t*>(self, 0x43C);
    re::Ptr32<void>& shaderSlot = At<void*>(self, 0x44C);
    if (head == nullptr || self[kDeviceLost] != 0) {
        if (shaderSlot != nullptr) {
            ReleaseRef(shaderSlot);
            shaderSlot = nullptr;
        }
        At<void*>(self, 0x444) = nullptr;
        At<void*>(self, 0x448) = nullptr;
        Renderer_ClearBatch(self);
        return;
    }
    uint8_t* geometry = At<uint8_t*>(head, 0);
    void* skin = At<void*>(geometry, 0xD0);
    void* data = At<void*>(head, 8);
    if (Virtual<uint32_t>(shaderSlot, kPreProcessPipeline, geometry, skin, data, At<void*>(self, 0x444),
                          At<void*>(self, 0x448), geometry + 0x60, geometry + 0x1C) != 0) {
        if (shaderSlot != nullptr) {
            ReleaseRef(shaderSlot);
            shaderSlot = nullptr;
        }
        At<void*>(self, 0x444) = nullptr;
        At<void*>(self, 0x448) = nullptr;
        Renderer_ClearBatch(self);
        return;
    }
    Virtual<uint32_t>(shaderSlot, kUpdatePipeline, geometry, skin, data, At<void*>(self, 0x444), At<void*>(self, 0x448),
                      geometry + 0x60, geometry + 0x1C);
    uint32_t passes = Virtual<uint32_t>(shaderSlot, kFirstPass);
    while (passes != 0) {
        uint8_t* first = At<uint8_t*>(self, 0x43C);
        uint8_t* firstGeometry = At<uint8_t*>(first, 0);
        Virtual<uint32_t>(shaderSlot, kSetupRenderingPass, firstGeometry, At<void*>(firstGeometry, 0xD0),
                          At<void*>(first, 8), At<void*>(self, 0x444), At<void*>(self, 0x448), firstGeometry + 0x60,
                          firstGeometry + 0x1C);
        uint8_t* entry = first;
        while (entry != nullptr) {
            geometry = At<uint8_t*>(entry, 0);
            skin = At<void*>(geometry, 0xD0);
            data = At<void*>(entry, 8);
            uint8_t* partition = At<uint8_t*>(entry, 4);
            void* shader = shaderSlot;
            if (shader == At<void*>(self, 0x8A0) && partition != nullptr &&
                At<uint16_t>(partition, 0x20) > At<uint32_t>(self, kHWBones)) {
                NiOutputDebugString("EndBatch> Cannot render partition with %d bones without a shader!\n",
                                    uint32_t(At<uint16_t>(partition, 0x20)));
                continue;
            }
            void* const& state = At<void*>(self, 0x444);
            void* const& effects = At<void*>(self, 0x448);
            Virtual<uint32_t>(shader, kSetupTransformations, geometry, skin, partition, data, state, effects,
                              geometry + 0x60, geometry + 0x1C);
            uint8_t* prepared = Virtual<uint8_t*>(shaderSlot, kPrepareGeometryForRendering, geometry, partition, data,
                                                  At<void*>(self, 0x444));
            data = prepared;
            Virtual<uint32_t>(shaderSlot, kSetupShaderPrograms, geometry, skin, partition, prepared,
                              At<void*>(self, 0x444), At<void*>(self, 0x448), geometry + 0x60, geometry + 0x1C);
            CommitShaderConstants(self);
            if (At<void*>(prepared, 0x34) == nullptr) {
                Com<long>(At<void*>(self, kDevice), kDevDrawPrimitive, At<uint32_t>(prepared, 0x3C),
                          At<uint32_t>(prepared, 0x38), At<uint32_t>(prepared, 0x40));
            } else {
                uint32_t start = 0;
                for (uint32_t k = 0; k < At<uint32_t>(prepared, 0x48); k++) {
                    uint16_t* lengths = At<uint16_t*>(prepared, 0x4C);
                    uint32_t count = lengths != nullptr ? uint32_t(lengths[k]) - 2 : At<uint32_t>(prepared, 0x40);
                    re::CountDraw(Com<long>(At<void*>(self, kDevice), kDevDrawIndexedPrimitive, At<uint32_t>(prepared, 0x3C),
                              At<uint32_t>(prepared, 0x38), 0u, At<uint32_t>(prepared, 0x18), start, count), 2);
                    start += count + 2;
                }
            }
            Virtual<uint32_t>(shaderSlot, kPostRender, geometry, skin, partition, prepared, At<void*>(self, 0x444),
                              At<void*>(self, 0x448), geometry + 0x60, geometry + 0x1C);
            entry = At<uint8_t*>(entry, 0xC);
        }
        passes = Virtual<uint32_t>(shaderSlot, kNextPass);
    }
    for (uint8_t* entry = At<uint8_t*>(self, 0x43C); entry != nullptr; entry = At<uint8_t*>(entry, 0xC)) {
        At<uint16_t>(At<uint8_t*>(At<uint8_t*>(entry, 0), 0xCC), 0x34) &= 0xF000;
    }
    // (the last geometry's: as the game's registers hold them)
    Virtual<uint32_t>(shaderSlot, kPostProcessPipeline, geometry, skin, data, At<void*>(self, 0x444),
                      At<void*>(self, 0x448), geometry + 0x60, geometry + 0x1C);
    if (shaderSlot != nullptr) {
        ReleaseRef(shaderSlot);
        shaderSlot = nullptr;
    }
    At<void*>(self, 0x444) = nullptr;
    At<void*>(self, 0x448) = nullptr;
    Renderer_ClearBatch(self);
    At<uint8_t>(At<void*>(self, kRenderState), 0x20F5) = 0;
}

// 0x87E7D0 (thiscall): NiDX9Renderer::DrawSkinnedPrimitive(geometry, data, skin) (the geometry's
// shader, the legacy default material's when it has none of its own, the error shader at worst;
// each partition drawn as an indexed triangle list or strips, the data's dirty flags cleared each
// pass; a partition the hardware can't skin under the error shader is reported and skipped)
void __fastcall Renderer_DrawSkinnedPrimitive(uint8_t* self, re::Edx, uint8_t* geometry, uint8_t* data, uint8_t* skin) {
    uint8_t* skinPartition = At<uint8_t*>(skin, 0xC);
    uint8_t* partitions = At<uint8_t*>(skinPartition, 0xC);
    uint32_t primitive = At<uint16_t>(partitions, 0x22) != 0 ? 5u : 4u;   // TRIANGLESTRIP / LIST
    void* shader = nullptr;
    if (geometry != nullptr) {
        void* own = Geometry_GetShader(geometry);
        shader = own != nullptr && HasClassBit(own, kShaderInterface_ClassBit) ? own : nullptr;
        if (shader == nullptr) {
            uint32_t materialIndex = At<uint32_t>(geometry, 0xE0);
            void* activeMaterial = materialIndex < At<uint32_t>(geometry, 0xDC)
                                       ? At<void*>(At<uint8_t*>(geometry, 0xD4) + materialIndex * 0x10, 4)
                                       : nullptr;
            Geometry_ApplyAndSetActiveMaterial(geometry, At<void*>(self, 0x18), 0xFFFFFFFF);
            shader = IsKindOfObject(kShaderInterface_RTTI, Geometry_GetShader(geometry));
            Geometry_SetActiveMaterial(geometry, activeMaterial);
        }
    }
    if (shader == nullptr) {
        NiOutputDebugString("DrawSkinnedPrimitive> No shader found for object %s!\nUsing Error Shader!\n",
                            At<const char*>(geometry, 8));
        shader = Virtual<void*>(self, 64);
    }
    void* bufferData = At<void*>(partitions, 0x28);
    void* const& state = At<void*>(self, 0xC);     // (read at each call, as the game does)
    void* const& effects = At<void*>(self, 0x10);
    if (Virtual<uint32_t>(shader, kPreProcessPipeline, geometry, skin, bufferData, state, effects, geometry + 0x60,
                          geometry + 0x1C) != 0) {
        return;
    }
    Virtual<uint32_t>(shader, kUpdatePipeline, geometry, skin, bufferData, At<void*>(self, 0xC), At<void*>(self, 0x10),
                      geometry + 0x60, geometry + 0x1C);
    uint32_t count = At<uint32_t>(skinPartition, 8);
    uint32_t draws = 0;
    uint32_t passes = Virtual<uint32_t>(shader, kFirstPass);
    while (passes != 0) {
        Virtual<uint32_t>(shader, kSetupRenderingPass, geometry, skin, bufferData, At<void*>(self, 0xC),
                          At<void*>(self, 0x10), geometry + 0x60, geometry + 0x1C);
        uint8_t* partition = partitions;
        for (uint32_t i = 0; i < count; i++, partition += 0x2C) {
            if (shader == At<void*>(self, 0x8A0) && At<uint16_t>(partition, 0x20) > At<uint32_t>(self, kHWBones)) {
                NiOutputDebugString(
                    "DrawSkinnedPrimitive> Cannot render partition with %d bones without a shader!\n",
                    uint32_t(At<uint16_t>(partition, 0x20)));
                continue;
            }
            bufferData = At<void*>(partition, 0x28);
            Virtual<uint32_t>(shader, kSetupTransformations, geometry, skin, partition, bufferData, At<void*>(self, 0xC),
                              At<void*>(self, 0x10), geometry + 0x60, geometry + 0x1C);
            uint8_t* prepared = Virtual<uint8_t*>(shader, kPrepareGeometryForRendering, geometry, partition, bufferData,
                                                  At<void*>(self, 0xC));
            bufferData = prepared;
            if (prepared == nullptr) {
                return;
            }
            Virtual<uint32_t>(shader, kSetupShaderPrograms, geometry, skin, partition, prepared, At<void*>(self, 0xC),
                              At<void*>(self, 0x10), geometry + 0x60, geometry + 0x1C);
            CommitShaderConstants(self);
            uint32_t start = 0;
            for (uint32_t k = 0; k < At<uint32_t>(prepared, 0x48); k++) {
                uint16_t* lengths = At<uint16_t*>(prepared, 0x4C);
                uint32_t primitives = lengths != nullptr ? uint32_t(lengths[k]) - 2 : At<uint32_t>(prepared, 0x40);
                re::CountDraw(Com<long>(At<void*>(self, kDevice), kDevDrawIndexedPrimitive, primitive, At<uint32_t>(prepared, 0x38), 0u,
                          uint32_t(At<uint16_t>(partition, 0x1C)), start, primitives), 3);
                draws++;
                start += primitives + 2;
            }
            Virtual<uint32_t>(shader, kPostRender, geometry, skin, partition, prepared, At<void*>(self, 0xC),
                              At<void*>(self, 0x10), geometry + 0x60, geometry + 0x1C);
        }
        At<uint16_t>(data, 0x34) &= 0xF000;
        passes = Virtual<uint32_t>(shader, kNextPass);
    }
    (void)draws;
    Virtual<uint32_t>(shader, kPostProcessPipeline, geometry, skin, bufferData, At<void*>(self, 0xC),
                      At<void*>(self, 0x10), geometry + 0x60, geometry + 0x1C);
    At<uint8_t>(At<void*>(self, kRenderState), 0x20F5) = 0;
}

// 0x87EE80 (thiscall): NiDX9Renderer::GenerateScreenTextureBuffers(screen texture, dirty flags) (4
// vertices a rectangle: pixel positions half a pixel off, wrapped into the target, the base
// texture's coordinates, the rectangle's colour; with dirty bit 8 the arrays grown and the quad
// indices rebuilt; a vertex buffer too small repacks everything (0xF))
void __fastcall Renderer_GenerateScreenTextureBuffers(uint8_t* self, re::Edx, uint8_t* screenTexture, uint32_t dirty) {
    uint32_t rects = At<uint16_t>(screenTexture, 0x10);
    uint8_t* bufferData = At<uint8_t*>(screenTexture, 0x1C);
    uint32_t vertexCount = uint16_t(rects * 4);
    int32_t indexCount = int32_t(vertexCount * 3) / 2;
    bool indices = ((dirty >> 3) & 1) != 0;
    At<uint32_t>(bufferData, 0x40) = rects * 2;
    At<uint32_t>(bufferData, 0x44) = rects * 2;
    At<uint32_t>(bufferData, 0x18) = vertexCount;
    At<uint32_t>(bufferData, 0x1C) = vertexCount;
    At<void*>(bufferData, 0x50) = nullptr;
    At<void*>(bufferData, 0x4C) = nullptr;
    At<uint32_t>(bufferData, 0x48) = 1;
    uint8_t* chip = At<uint32_t>(bufferData, 0x20) != 0 ? At<re::Ptr32<uint8_t>*>(bufferData, 0x28)[0] : nullptr;
    if (indices) {
        if (uint16_t(vertexCount) > At<uint16_t>(self, 0x85C)) {
            NiDeleteArray(At<void*>(self, 0x850), 8);
            NiDeleteArray(At<void*>(self, 0x854), 0x10);
            NiDeleteArray(At<void*>(self, 0x858), 8);
            auto bytes = [](uint32_t count, uint32_t size) {
                uint64_t total = uint64_t(count) * size;
                return total > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(total);
            };
            At<void*>(self, 0x850) = NiNewArray(bytes(vertexCount, 8));
            void* colors = NiNewArray(bytes(vertexCount, 0x10));
            if (colors != nullptr) {
                VectorConstruct(colors, 0x10, int(vertexCount), kColorAConstruct);
            }
            At<void*>(self, 0x854) = colors;
            At<void*>(self, 0x858) = NiNewArray(bytes(vertexCount, 8));
            At<uint16_t>(self, 0x85C) = uint16_t(vertexCount);
        }
        if (chip != nullptr) {
            uint32_t stride = At<uint32_t>(bufferData, 0x20) != 0 ? At<uint32_t*>(bufferData, 0x24)[0] : 0;
            if (At<uint32_t>(chip, 0x14) / stride < vertexCount) {
                dirty = 0xF;
            }
        }
    }
    if ((uint16_t(dirty) | 0xFFFFFFF7u) != 0) {   // (always)
        uint32_t targetWidth = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 18, 0u);
        uint32_t targetHeight = Virtual<uint32_t>(At<void*>(self, kCurrentRenderTargetGroup), 19, 0u);
        void* map = *At<re::Ptr32<void>*>(At<uint8_t*>(screenTexture, 0x14), 0x20);
        void* texture = map != nullptr ? At<void*>(map, 8) : nullptr;
        float invWidth = 1.0f / float(Virtual<uint16_t>(texture, 18));
        map = *At<re::Ptr32<void>*>(At<uint8_t*>(screenTexture, 0x14), 0x20);
        texture = map != nullptr ? At<void*>(map, 8) : nullptr;
        float invHeight = 1.0f / float(Virtual<uint16_t>(texture, 19));
        float* positions = At<float*>(self, 0x850);
        uint32_t* colors = At<uint32_t*>(self, 0x854);
        float* textures = At<float*>(self, 0x858);
        for (uint16_t i = 0; i < uint16_t(rects); i++) {
            const uint8_t* rect = At<uint8_t*>(screenTexture, 8) + i * 0x1C;
            int16_t x = int16_t(At<uint16_t>(rect, 2));
            while (x < 0) {
                x = int16_t(x + targetWidth);
            }
            while (int32_t(x) >= int32_t(targetWidth)) {
                x = int16_t(x - targetWidth);
            }
            int16_t y = int16_t(At<uint16_t>(rect, 0));
            while (y < 0) {
                y = int16_t(y + targetHeight);
            }
            while (int32_t(y) >= int32_t(targetHeight)) {
                y = int16_t(y - targetHeight);
            }
            float x0 = float(x) - 0.5f;
            float y0 = float(y) - 0.5f;
            float width = float(At<uint16_t>(rect, 4));
            float height = float(At<uint16_t>(rect, 6));
            float x1 = x0 + width;
            float y1 = y0 + height;
            float u0 = float(At<uint16_t>(rect, 0xA)) * invWidth;
            float v0 = float(At<uint16_t>(rect, 8)) * invHeight;
            float u1 = u0 + width * invWidth;
            float v1 = v0 + height * invHeight;
            uint16_t index = uint16_t(i * 4);
            for (uint16_t j = 0; j < 4; j++, index++) {
                bool second = (j >> 1) != 0;
                bool odd = (j & 1) != 0;
                positions[index * 2] = second ? x1 : x0;
                positions[index * 2 + 1] = odd ? y1 : y0;
                crt::Memcpy(&colors[index * 4], rect + 0xC, 0x10);
                textures[index * 2] = second ? u1 : u0;
                textures[index * 2 + 1] = odd ? v1 : v0;
            }
        }
        if (VBManager_PackScreenTextureVB(At<void*>(self, kVBManager), bufferData, vertexCount, positions,
                                          At<void*>(self, 0x854), textures, dirty, chip, 0) == 0) {
            return;
        }
    }
    if (!indices) {
        return;
    }
    if (uint32_t(indexCount) > At<uint32_t>(self, 0x864)) {
        NiFree(At<void*>(self, 0x860));
        At<void*>(self, 0x860) = NiMalloc(uint32_t(indexCount) * 2);
        At<uint32_t>(self, 0x864) = uint32_t(indexCount);
        uint16_t* list = At<uint16_t*>(self, 0x860);
        uint32_t k = 0;
        for (uint32_t v = 0; v < vertexCount; v += 4) {
            uint16_t base = uint16_t(v);
            list[uint16_t(k)] = base;
            list[uint16_t(k + 1)] = uint16_t(base + 1);
            list[uint16_t(k + 2)] = uint16_t(base + 2);
            list[uint16_t(k + 3)] = uint16_t(base + 2);
            list[uint16_t(k + 4)] = uint16_t(base + 1);
            list[uint16_t(k + 5)] = uint16_t(base + 3);
            k += 6;
        }
    }
    uint32_t size = At<uint32_t>(bufferData, 0x30);
    void* old = At<void*>(bufferData, 0x34);
    void* buffer = IBManager_PackBuffer(At<void*>(self, kIBManager), At<void*>(self, 0x860), uint32_t(indexCount),
                                        uint32_t(indexCount), old, &size, 1, bufferData[0x14] != 0 ? 0x10u : 0u);
    if (old != buffer) {
        BufferData_ReleaseIB(bufferData);
        At<void*>(bufferData, 0x34) = buffer;
        At<uint32_t>(bufferData, 0x2C) = uint32_t(indexCount);
        At<uint32_t>(bufferData, 0x30) = size;
    }
}

#define DX9REND38G_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

DX9REND38G_REVERSED("NiDX9Renderer::Do_RenderPoints", 0x0087CD00, Renderer_Do_RenderPoints);
DX9REND38G_REVERSED("NiDX9Renderer::Do_RenderLines", 0x0087DD40, Renderer_Do_RenderLines);
DX9REND38G_REVERSED("NiDX9Renderer::Do_EndBatch", 0x0087E420, Renderer_Do_EndBatch);
DX9REND38G_REVERSED("NiDX9Renderer::DrawSkinnedPrimitive", 0x0087E7D0, Renderer_DrawSkinnedPrimitive);
DX9REND38G_REVERSED("NiDX9Renderer::GenerateScreenTextureBuffers", 0x0087EE80, Renderer_GenerateScreenTextureBuffers);

} // namespace ni::dx9
