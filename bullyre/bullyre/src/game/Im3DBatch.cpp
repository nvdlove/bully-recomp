// The Im3D emulation's batching (RwIm3DRenderIndexedPrimitive's stand-in): the vertices
// Im3D_Transform kept (0xDA7530, RwIm3DVertex of 0x24 bytes) are copied into big
// position / normal / colour / uv arrays, then handed to a pooled NiTriShape (triangle
// lists, strips and fans: 0x400 of them) or NiLines (line lists and strips: 200) that is
// either drawn at once or queued for the renderer's alpha pass. 0x5F0090..0x5F0A20, with
// the pooled shape's destructor and two Gamebryo leaves (a string hash, a pixel format test).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <malloc.h>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// RwIm3DVertex as the game fills it
struct Im3DVertex {
    float pos[3];     // +0x0
    float normal[3];  // +0xC
    uint8_t b, g, r, a;  // +0x18 (a 0x80: opaque)
    float u, v;       // +0x1C
};
static_assert(sizeof(Im3DVertex) == 0x24);

constexpr int kMaxVertices = 300000;
constexpr int kMaxTriShapes = 0x400;
constexpr int kMaxLineShapes = 200;
constexpr int kMaxLineFlags = 0x800;

// the batch (the state Im3D_Transform and friends set)
GAME_VAR(uint8_t, im3dQueueForAlpha, 0x00DA7518);   // queued as translucent (0x402700's third argument)
GAME_VAR(uint8_t, im3dDrawNow, 0x00DA7519);         // drawn at once and the batch reset
GAME_VAR(uint8_t, im3dInitialised, 0x00DA751A);
GAME_VAR(int32_t, im3dVertexCursor, 0x00DA751C);
GAME_VAR(int32_t, im3dTriShapeCursor, 0x00DA7520);
GAME_VAR(uint16_t, im3dIndexCursor, 0x00DA7524);
GAME_VAR(int32_t, im3dLineShapeCursor, 0x00DA7528);
GAME_VAR(int32_t, im3dLineFlagCursor, 0x00DA752C);
GAME_VAR(Im3DVertex*, im3dVertices, 0x00DA7530);
GAME_VAR(int32_t, im3dNumVertices, 0x00DA7534);
GAME_VAR(uint32_t, im3dFlags, 0x00DA7538);          // 1: with uvs
GAME_VAR(float*, im3dTransform, 0x00DA753C);        // an NiTransform (13 floats); nullptr: identity
GAME_ARRAY(float, im3dPositions, 0x00DA7540);       // [kMaxVertices][3]
GAME_ARRAY(float, im3dNormals, 0x011163C0);         // [kMaxVertices][3]
GAME_ARRAY(float, im3dColors, 0x01485240);          // [kMaxVertices][4] r g b a
GAME_ARRAY(float, im3dUvs, 0x01919040);             // [kMaxVertices][2]
GAME_ARRAY(uint8_t, im3dLineFlags, 0x00D14558);     // per vertex: 1 the line continues (strip)
GAME_ARRAY(uint16_t, im3dIndices, 0x00D14D58);      // [kMaxVertices]
GAME_ARRAY(uint8_t*, im3dTriShapes, 0x01B62F40);    // NiPointer<TriShape>[0x400]
GAME_ARRAY(uint8_t*, im3dLineShapes, 0x01B63F40);   // NiPointer<LineShape>[200]
GAME_ARRAY(float, identityTransform, 0x00B0A38C);   // NiTransform (rotation, translation, scale)
GAME_VAR(void*, niRenderer, 0x020E2380);
GAME_VAR(uint8_t, niStaticRtti_20E30FC, 0x020E30FC);

GAME_FN(void, __thiscall, NiTransform_MakeIdentity, 0x00770920, float* transform);
inline void* __cdecl Memset_85B3B0(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiDelete, 0x00752540, void* p, size_t size);
GAME_FN(void*, __thiscall, TriShape_Construct, 0x005EFF20, void* shape);
GAME_FN(void*, __thiscall, LineShape_Construct, 0x005EFCE0, void* shape);
GAME_FN(void, __thiscall, NiPointer_Assign, 0x00800420, re::Ptr32<uint8_t>* slot, void* object);
GAME_FN(void, __thiscall, TriShapeData_SetTriangles, 0x0077BAB0, void* data, int count, uint16_t* indices);
GAME_FN(void, __thiscall, LinesData_SetFlags, 0x00779C70, void* data, uint8_t* flags);
GAME_FN(void, __thiscall, GeometryData_SetData, 0x0077A030, void* data, int count, float* positions, float* normals,
        float* colors, float* uvs, int uvSets, int);
GAME_FN(void, __thiscall, PropertyState_Update, 0x00519CB0, void* state);
GAME_FN(void, __thiscall, AVObject_AttachProperty, 0x00762080, void* object, void* property, int);
GAME_FN(void, __cdecl, Renderer_QueueGeometry, 0x00402700, void* geometry, int opaque, int translucent);
GAME_FN(void, __thiscall, PropertyState_Destruct, 0x005EFC30, void* state);
GAME_FN(void, __thiscall, TriShapeBase_Destruct, 0x007793C0, void* shape);
// (passed by address: the game's NiPointer slots of two properties)
inline void* const propertySlot_BCC4B4 = reinterpret_cast<void*>(0x00BCC4B4);
inline void* const propertySlot_BCC4B0 = reinterpret_cast<void*>(0x00BCC4B0);

float ColorChannel(uint8_t value) {
    return float(value / 255.0);
}
float AlphaChannel(uint8_t value) {
    float a = float(value * 0.0078125);   // (0x80: 1)
    return a > 1.0f ? 1.0f : a;
}

// vertex `from` of the Im3D buffer into slot `to` of the arrays
void CopyVertex(const Im3DVertex& v, int to) {
    im3dPositions[to * 3 + 0] = v.pos[0];
    im3dPositions[to * 3 + 1] = v.pos[1];
    im3dPositions[to * 3 + 2] = v.pos[2];
    im3dNormals[to * 3 + 0] = v.normal[0];
    im3dNormals[to * 3 + 1] = v.normal[1];
    im3dNormals[to * 3 + 2] = v.normal[2];
    im3dColors[to * 4 + 0] = ColorChannel(v.r);
    im3dColors[to * 4 + 1] = ColorChannel(v.g);
    im3dColors[to * 4 + 2] = ColorChannel(v.b);
    im3dColors[to * 4 + 3] = AlphaChannel(v.a);
    im3dUvs[to * 2 + 0] = v.u;
    im3dUvs[to * 2 + 1] = v.v;
}

void ReleaseRef(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 4);   // DeleteThis
    }
}

// 0x5F0170 (cdecl): the batch as `type` (1 line list, 2 line strip, 3 triangle list, 4
// strip, 5 fan, 6 points: drawn as one-vertex lines); with `indices` the vertices by
// them (all of them copied for the triangle types, only the indexed ones for lines).
// `twoSided`: the shape's flags 0x10. 0: the arrays or pools are full
uint8_t __cdecl RenderIndexedPrimitive(int type, uint16_t* indices, int numIndices, int twoSided) {
    for (;;) {
        if (!im3dInitialised) {
            im3dInitialised = 1;
            NiTransform_MakeIdentity(identityTransform);
            Memset_85B3B0(im3dLineFlags, 1, kMaxLineFlags);
        }
        int count = im3dNumVertices;
        if (!im3dVertices) {
            return 0;
        }
        int base = im3dVertexCursor;
        if (!indices || type == 3 || type == 5 || type == 4) {
            if (uint32_t(base + count) > uint32_t(kMaxVertices)) {
                return 0;
            }
            for (int i = 0; i < count; i++) {
                CopyVertex(im3dVertices[i], base + i);
            }
        } else {
            if (uint32_t(base + numIndices) > uint32_t(kMaxVertices)) {
                return 0;
            }
            count = numIndices;
            for (int i = 0; i < numIndices; i++) {
                CopyVertex(im3dVertices[indices[i]], base + i);
            }
        }
        float* normals = &im3dNormals[im3dVertexCursor * 3];
        uint8_t* shape;
        uint8_t* data;
        switch (type) {
        case 1:
        case 2: {
            int flagBase = im3dLineFlagCursor;
            if (flagBase + count > kMaxLineFlags || im3dLineShapeCursor + 1 > kMaxLineShapes) {
                return 0;
            }
            for (int i = 1; i < count; i += 2) {
                im3dLineFlags[flagBase + i] = type == 2;
            }
            int slot = im3dLineShapeCursor;
            if (!im3dLineShapes[slot]) {
                void* p = NiNew(0x10C);
                NiPointer_Assign(&im3dLineShapes[im3dLineShapeCursor], p ? LineShape_Construct(p) : nullptr);
                flagBase = im3dLineFlagCursor;
                slot = im3dLineShapeCursor;
            }
            shape = im3dLineShapes[slot];
            im3dLineShapeCursor = slot + 1;
            LinesData_SetFlags(vc::At<void*>(shape, 0xEC), &im3dLineFlags[flagBase]);
            im3dLineFlagCursor += count;
            data = vc::At<uint8_t*>(shape, 0xEC);
            normals = nullptr;
            break;
        }
        case 3:
        case 4:
        case 5: {
            int n = indices ? numIndices : count;
            uint32_t numTris = type == 3 ? uint16_t(n / 3) : uint16_t(n - 2);
            uint32_t at = im3dIndexCursor;
            uint32_t numIndexed = uint16_t(numTris * 3);
            if (int(numIndexed + at) > kMaxVertices || im3dTriShapeCursor + 1 > kMaxTriShapes) {
                return 0;
            }
            uint16_t* out = &im3dIndices[at];
            if (type == 3) {
                for (int i = 0; i < int(numIndexed); i++) {
                    out[i] = indices ? indices[i] : uint16_t(i);
                }
            } else if (type == 5) {
                for (int i = 0; i < int(numTris); i++) {
                    out[i * 3 + 0] = indices ? indices[0] : uint16_t(0);
                    out[i * 3 + 1] = indices ? indices[i + 2] : uint16_t(i + 2);
                    out[i * 3 + 2] = indices ? indices[i + 1] : uint16_t(i + 1);
                }
            } else {
                for (int i = 0; i < int(numTris); i++) {
                    if (indices) {
                        out[i * 3 + 0] = indices[i];
                        out[i * 3 + 1] = indices[(uint32_t(i) >> 1) * 2 + 2];
                        out[i * 3 + 2] = indices[((i + 1) / 2) * 2 + 1];
                    } else {
                        out[i * 3 + 0] = uint16_t(i);
                        out[i * 3 + 1] = uint16_t((i / 2) * 2 + 2);
                        out[i * 3 + 2] = uint16_t(((i + 1) / 2) * 2 + 1);
                    }
                }
            }
            if (!im3dTriShapes[im3dTriShapeCursor]) {
                void* p = NiNew(0x10C);
                NiPointer_Assign(&im3dTriShapes[im3dTriShapeCursor], p ? TriShape_Construct(p) : nullptr);
            }
            shape = im3dTriShapes[im3dTriShapeCursor];
            im3dTriShapeCursor++;
            TriShapeData_SetTriangles(vc::At<void*>(shape, 0xEC), int(numTris), &im3dIndices[im3dIndexCursor]);
            im3dIndexCursor = uint16_t(im3dIndexCursor + uint16_t(numIndexed));
            data = vc::At<uint8_t*>(shape, 0xEC);
            break;
        }
        case 6: {
            // (a stack buffer of (i, i) pairs: each point a line to itself)
            uint16_t* pairs = static_cast<uint16_t*>(_alloca(size_t(count) * 4));
            for (int i = 0; i < count; i++) {
                pairs[i * 2] = uint16_t(i);
                pairs[i * 2 + 1] = uint16_t(i);
            }
            indices = pairs;
            numIndices = count * 2;
            type = 1;
            twoSided = 0;
            continue;
        }
        default:
            // (the game goes on with an uninitialised shape pointer; its callers only pass 1..6)
            return 0;
        }
        // the transform (the identity without one), its scale made positive
        const float* transform = im3dTransform ? static_cast<float*>(im3dTransform) : identityTransform;
        float* local = &vc::At<float>(shape, 0x2C);
        for (int i = 0; i < 13; i++) {
            local[i] = transform[i];
        }
        vc::At<float>(shape, 0x5C) = std::fabs(vc::At<float>(shape, 0x5C));
        uint8_t* state = shape + 0xF0;
        PropertyState_Update(state);
        AVObject_AttachProperty(shape, vc::At<int32_t>(vc::At<uint8_t*>(state, 0x14), 8) == 0 ? propertySlot_BCC4B4 : propertySlot_BCC4B0,
                                -1);
        int base2 = im3dVertexCursor;
        GeometryData_SetData(data, count, &im3dPositions[base2 * 3], normals, &im3dColors[base2 * 4],
                             (im3dFlags & 1) ? &im3dUvs[base2 * 2] : nullptr, int(im3dFlags & 1), 0);
        vc::At<uint16_t>(data, 0x34) |= 0xFFF;
        im3dVertexCursor += count;
        vc::At<uint32_t>(shape, 0xB0) = re::Flag(twoSided) ? 0x10 : 0;
        if (!im3dDrawNow) {
            Renderer_QueueGeometry(shape, im3dQueueForAlpha == 0, im3dQueueForAlpha != 0);
            return 1;
        }
        // drawn now: the world transform the identity
        float* world = &vc::At<float>(shape, 0x60);
        for (int i = 0; i < 9; i++) {
            world[i] = identityTransform[i];
        }
        for (int i = 9; i < 13; i++) {
            world[i] = identityTransform[i];
        }
        vc::Call<void>(shape, 0x88, niRenderer);
        im3dTriShapeCursor = 0;
        im3dIndexCursor = 0;
        im3dLineShapeCursor = 0;
        im3dLineFlagCursor = 0;
        im3dVertexCursor = 0;
        return 1;
    }
}

// 0x5F09D0 (cdecl): RwIm3DRenderPrimitive
void __cdecl RenderPrimitive(int type) {
    reinterpret_cast<uint8_t(__cdecl*)(int, uint16_t*, int, int)>(0x005F0170)(type, nullptr, 0, 0);
}

// 0x5F09F0 (cdecl): NiTStringMap's hash (h * 33 + c over the name) into `tableSize`
uint32_t __cdecl StringMap_KeyToHashIndex(const char* key, uint32_t tableSize) {
    uint32_t hash = 0;
    for (; *key; key++) {
        hash = hash * 33 + uint32_t(int(*key));
    }
    return hash % tableSize;
}

// 0x5F0A20 (thiscall): NiPixelFormat::IsCompressed (formats 4..6: DXT1/3/5)
uint32_t __fastcall PixelFormat_IsCompressed(uint8_t* self, re::Edx) {
    int32_t format = vc::At<int32_t>(self, 4);
    return format >= 4 && format <= 6 ? 1 : 0;
}

// 0x5F0090: the pooled shapes' GetRTTI
void* __cdecl TriShape_GetRTTI() {
    return &niStaticRtti_20E30FC;
}

// 0x5F00A0 (thiscall): the pooled triangle shape's destructor (its data emptied first)
void __fastcall TriShape_Destruct(uint8_t* self, re::Edx) {
    vc::At<uint32_t>(self, 0) = 0x0092D1CC;
    TriShapeData_SetTriangles(vc::At<void*>(self, 0xEC), 0, nullptr);
    GeometryData_SetData(vc::At<void*>(self, 0xEC), 0, nullptr, nullptr, nullptr, nullptr, 0, 0);
    if (uint8_t* data = vc::At<uint8_t*>(self, 0xEC)) {
        ReleaseRef(data);
        vc::At<uint8_t*>(self, 0xEC) = nullptr;
    }
    PropertyState_Destruct(self + 0xF0);
    ReleaseRef(vc::At<uint8_t*>(self, 0xEC));   // (the NiPointer's own destructor: nothing left)
    TriShapeBase_Destruct(self);
}

// 0x5F0140 (thiscall): the scalar deleting destructor
uint8_t* __fastcall TriShape_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x005F00A0)(self);
    if (flags & 1) {
        NiDelete(self, 0x10C);
    }
    return self;
}

} // namespace

#define IM3DB_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

IM3DB_REVERSED("TriShape::GetRTTI_5F0090", 0x005F0090, &TriShape_GetRTTI);
IM3DB_REVERSED("TriShape::Destruct_5F00A0", 0x005F00A0, &TriShape_Destruct);
IM3DB_REVERSED("TriShape::DeletingDestruct_5F0140", 0x005F0140, &TriShape_DeletingDestruct);
IM3DB_REVERSED("Im3D::RenderIndexedPrimitive", 0x005F0170, &RenderIndexedPrimitive);
IM3DB_REVERSED("Im3D::RenderPrimitive", 0x005F09D0, &RenderPrimitive);
IM3DB_REVERSED("NiTStringMap::KeyToHashIndex", 0x005F09F0, &StringMap_KeyToHashIndex);
IM3DB_REVERSED("NiPixelFormat::IsCompressed", 0x005F0A20, &PixelFormat_IsCompressed);
