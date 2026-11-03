// NiD3D10Renderer (src/gamebryo/NiD3D10Renderer.h): the triangle geometry's packing into data
// streams (shapes, strips, a skin's partitions) and the particles' (each particle a rotated,
// sized quad facing the camera). The stream and partition helpers are the D3D10 geometry data's
// (src/gamebryo/NiD3D10Renderer4.cpp describes them).

#include "gamebryo/NiD3D10Renderer.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using namespace ni::d3d10;

constexpr uintptr_t NiTriShapeData_RTTI = 0x020E3130;
constexpr uintptr_t NiTriStripsData_RTTI = 0x020E3B30;

GAME_FN(uint8_t, __cdecl, IsKindOf, 0x004066F0, uintptr_t rtti, const void* object);
GAME_FN(void*, __thiscall, Object_Get4, 0x0066B5D0, void* self);     // (+0x4)
GAME_FN(void*, __thiscall, Object_GetC, 0x00853140, void* self);     // (+0xC)
GAME_FN(void*, __thiscall, Object_Get18, 0x008B40F0, void* self);    // (+0x18)
GAME_FN(void*, __thiscall, Object_Get1C, 0x0077E8E0, void* self);    // (+0x1C)
GAME_FN(void*, __thiscall, Object_Get34, 0x008A2F70, void* self);    // (+0x34)
GAME_FN(uint32_t, __thiscall, GeometryData_GetStreamCount, 0x0075D400, void* self);
GAME_FN(void, __thiscall, Partitions_Prepare, 0x008B43E0, void* self);
GAME_FN(uint32_t, __thiscall, Partitions_GetStride, 0x008B4510, void* self, uint32_t stream);
GAME_FN(uint32_t, __thiscall, Partitions_GetOffset, 0x008B4540, void* self, uint32_t stream, uint32_t element);
GAME_FN(void, __cdecl, Element_Skip, 0x008B4100, uint32_t value);
GAME_FN(void, __thiscall, Manager448_ConvertElement, 0x008B2BD0, void* self, uint32_t vertices, void* model, void* skin,
        void* element, uint32_t stride, void* destination, int b, uintptr_t c);
GAME_FN(uint32_t, __thiscall, GeometryData_GetDrawCount, 0x008B7300, void* self);
GAME_FN(uint32_t, __thiscall, GeometryData_GetFirstDraw, 0x008B72C0, void* self, uint32_t partition);
GAME_FN(uint32_t, __thiscall, GeometryData_GetDrawsIn, 0x008B72E0, void* self, uint32_t partition);
GAME_FN(void*, __thiscall, GeometryData_GetIndexStream_7460, 0x008B7460, void* self);
GAME_FN(void, __thiscall, GeometryData_SetStrides, 0x008B7470, void* self, const uint32_t* strides, uint32_t count);
GAME_FN(void, __thiscall, GeometryData_SetIndexFormat, 0x008B74C0, void* self, uint32_t draw, uint32_t format);
GAME_FN(void, __thiscall, GeometryData_SetStartVertex, 0x008B74F0, void* self, uint32_t draw, uint32_t value);
GAME_FN(void, __thiscall, GeometryData_SetOffsets, 0x008B7520, void* self, uint32_t draw, const uint32_t* offsets,
        uint32_t count);
GAME_FN(void, __thiscall, GeometryData_SetIndexCount, 0x008B7590, void* self, uint32_t draw, uint32_t count);
GAME_FN(void, __thiscall, GeometryData_SetStartIndex, 0x008B75C0, void* self, uint32_t draw, uint32_t start);
GAME_FN(void, __thiscall, GeometryData_SetBaseVertex, 0x008B75F0, void* self, uint32_t draw, uint32_t base);
GAME_FN(void, __thiscall, GeometryData_SetInputLayoutKey, 0x008B7620, void* self, uint32_t key);
GAME_FN(uint32_t, __cdecl, InputLayout_MakeKey, 0x008B7630, int a, int b, int c, uint32_t d);
GAME_FN(void*, __thiscall, GeometryData_GetStream, 0x008B7850, void* self, uint32_t stream);
GAME_FN(void*, __thiscall, GeometryData_GetIndexStream, 0x008B7880, void* self, uint32_t draw);
GAME_FN(void, __thiscall, GeometryData_Set78B0, 0x008B78B0, void* self, void* a, void* b);
GAME_FN(void, __thiscall, GeometryData_SetStream, 0x008B7930, void* self, uint32_t stream, void* blob);
GAME_FN(void, __thiscall, GeometryData_SetIndexStream, 0x008B7A00, void* self, uint32_t draw, void* blob);
GAME_FN(void, __thiscall, GeometryData_StreamsChanged, 0x008B7AC0, void* self);
GAME_FN(void, __thiscall, GeometryData_IndicesChanged, 0x008B7B30, void* self);
GAME_FN(void, __thiscall, GeometryData_SetStreamCount, 0x008B7EA0, void* self, uint32_t count);
GAME_FN(void, __cdecl, DataStream_Create, 0x008B84E0, uint32_t size, uint32_t usage, uint32_t kind, re::Ptr32<void>* result);
GAME_FN(void, __thiscall, Renderer_EnsureScratch, 0x008AD1B0, void* self, uint32_t count);

// the streams' common start (prepared; the streams counted again unless they match) and their
// buffers made big enough for `vertices` (false: unchanged and static, nothing to do)
struct StreamSetup {
    bool same;
    uint32_t streams;
    uint32_t usage;
    uint32_t strides[15];
    uint32_t offsets[15];
};

void MakeStreams(uint8_t* data, uint8_t* partitions, StreamSetup& setup, uint32_t volatility, uint32_t vertices) {
    re::Ptr32<void>* buffers = static_cast<re::Ptr32<void>*>(Object_Get34(data));
    setup.usage = 4;
    if (volatility == 0x8000) {
        setup.usage = 0x14;
    } else if (volatility == 0) {
        setup.usage = 6;
    }
    for (uint32_t i = 0; i < setup.streams; i++) {
        uint32_t stride = Partitions_GetStride(partitions, i);
        setup.strides[i] = stride;
        uint32_t size = vertices * stride;
        if (buffers[i] == nullptr ||
            uint32_t(reinterpret_cast<uintptr_t>(Object_GetC(GeometryData_GetStream(data, i)))) < size) {
            re::Ptr32<void> blob = nullptr;
            DataStream_Create(size, setup.usage, 2, &blob);
            GeometryData_SetStream(data, i, blob);
            setup.same = false;
            ReleaseRef(blob);
        }
        setup.offsets[i] = 0;
    }
}

// partitions' element (stream, index) or null
inline uint8_t* ElementOf(uint8_t* partitions, uint32_t stream, uint32_t index) {
    uint8_t* infos = At<uint8_t*>(partitions, 0x10);
    if (stream < At<uint32_t>(partitions, 0xC) && infos != nullptr && index < At<uint32_t>(infos, stream * 0x10)) {
        return At<uint8_t*>(infos, stream * 0x10 + 4) + index * 0x18;
    }
    return nullptr;
}
inline uint32_t ElementCount(uint8_t* partitions, uint32_t stream) {
    uint8_t* infos = At<uint8_t*>(partitions, 0x10);
    if (infos != nullptr && stream < At<uint32_t>(partitions, 0xC)) {
        return At<uint32_t>(infos, stream * 0x10);
    }
    return 0;
}

} // namespace

// 0x8ABCB0 (thiscall): NiD3D10Renderer::PackGeometry(geometry data, model data, skin instance,
// partitions, force) (the draws: a skin's partitions (lists or strips), a shape's triangles or the
// strips; the streams; each element converted (a skinned one's positions, normals, tangents and
// binormals left to the skinning); the input layout's key; the indices copied)
uint8_t __fastcall NiD3D10Renderer_PackGeometry(uint8_t* self, re::Edx, uint8_t* data, uint8_t* model, uint8_t* skin,
                                                uint8_t* partitions, int force) {
    if (data == nullptr || model == nullptr || partitions == nullptr) {
        return 0;
    }
    Partitions_Prepare(partitions);
    StreamSetup setup;
    setup.same = false;
    uint32_t partitionCount = uint32_t(reinterpret_cast<uintptr_t>(Object_Get4(data)));
    setup.streams = At<uint32_t>(partitions, 0xC);
    if (Object_Get34(data) != nullptr && GeometryData_GetStreamCount(data) == setup.streams) {
        setup.same = true;
    } else {
        GeometryData_SetStreamCount(data, setup.streams);
    }
    uint32_t volatility = At<uint16_t>(model, 0x34) & 0xF000;
    if (!re::Flag(force) && setup.same && volatility == 0x4000) {
        return 1;
    }
    void* layoutB = Object_Get1C(partitions);
    GeometryData_Set78B0(data, Object_Get18(partitions), layoutB);
    uint32_t vertexCount = At<uint16_t>(model, 0x30);   // (the converter's count)
    uint32_t vertices = At<uint16_t>(model, 0xC);       // (the streams' size)
    const uint16_t* sourceIndices = nullptr;
    uint8_t* skinPartition = skin != nullptr ? At<uint8_t*>(skin, 0xC) : nullptr;
    uint32_t draws = GeometryData_GetDrawCount(data);
    uint32_t draw = 0;
    uint32_t firstIndex = 0;
    uint32_t indexTotal = 0;     // (the indices the index stream takes)
    uint32_t copyTotal = 0;      // (the indices copied from the model)
    if (skinPartition != nullptr) {
        uint16_t partitionVertices = 0;
        uint32_t firstVertex = 0;
        for (uint32_t i = 0; i < partitionCount; i++) {
            GeometryData_GetFirstDraw(data, i);
            GeometryData_GetDrawsIn(data, i);
            uint8_t* partition = At<uint8_t*>(skinPartition, 0xC) + i * 0x2C;
            uint16_t strips = At<uint16_t>(partition, 0x22);
            partitionVertices = uint16_t(partitionVertices + At<uint16_t>(partition, 0x1C));
            if (strips == 0) {
                uint32_t indices = uint32_t(At<uint16_t>(partition, 0x1E)) * 3;
                copyTotal += indices;
                GeometryData_SetStartVertex(data, draw, 0);
                GeometryData_SetIndexCount(data, draw, indices);
                GeometryData_SetStartIndex(data, draw, firstIndex);
                GeometryData_SetBaseVertex(data, draw, firstVertex);
                firstIndex += indices;
                draw++;
                firstVertex += At<uint16_t>(partition, 0x1C);
            } else {
                copyTotal = At<uint16_t>(partition, 0x1E) + copyTotal + uint32_t(strips) * 2;
                const uint16_t* lengths = At<const uint16_t*>(partition, 0x18);
                for (uint32_t k = 0; k < At<uint16_t>(partition, 0x22); k++) {
                    GeometryData_SetStartVertex(data, draw, 0);
                    GeometryData_SetIndexCount(data, draw, lengths[k]);
                    GeometryData_SetStartIndex(data, draw, firstIndex);
                    GeometryData_SetBaseVertex(data, draw, firstVertex);
                    firstIndex += At<const uint16_t*>(partition, 0x18)[k];
                    draw++;
                }
                firstVertex += At<uint16_t>(partition, 0x1C);
            }
        }
        vertexCount = partitionVertices;   // (the converter's count too)
        vertices = partitionVertices;
        indexTotal = copyTotal;
    } else if (IsKindOf(NiTriShapeData_RTTI, model) != 0) {
        sourceIndices = At<const uint16_t*>(model, 0x48);
        uint32_t indices = uint32_t(At<uint16_t>(model, 0x42)) * 3;
        indexTotal = uint32_t(At<uint16_t>(model, 0x40)) * 3;
        copyTotal = indices;
        GeometryData_SetStartVertex(data, 0, 0);
        GeometryData_SetIndexCount(data, 0, indices);
        GeometryData_SetStartIndex(data, 0, 0);
        GeometryData_SetBaseVertex(data, 0, 0);
    } else if (IsKindOf(NiTriStripsData_RTTI, model) != 0) {
        uint32_t strips = At<uint16_t>(model, 0x44);
        sourceIndices = At<const uint16_t*>(model, 0x4C);
        const uint16_t* lengths = At<const uint16_t*>(model, 0x48);
        copyTotal = At<uint16_t>(model, 0x42) + strips * 2;
        indexTotal = At<uint16_t>(model, 0x40) + strips * 2;
        for (uint32_t k = 0; k < strips; k++, draw++) {
            GeometryData_SetStartVertex(data, draw, 0);
            GeometryData_SetIndexCount(data, draw, lengths[k]);
            GeometryData_SetStartIndex(data, draw, firstIndex);
            GeometryData_SetBaseVertex(data, draw, 0);
            firstIndex += lengths[k];
        }
    }
    uint32_t mask = At<uint16_t>(model, 0x34) & 0xFFF;
    if (skin != nullptr) {
        uint32_t frame = At<uint32_t>(self, 0x9C);
        if (At<uint32_t>(skin, 0x18) != frame) {
            mask |= 3;
            At<uint32_t>(skin, 0x18) = frame;
        }
    }
    if (volatility == 0x8000 || re::Flag(force) || !setup.same) {
        mask = 0xFFF;
    }
    MakeStreams(data, partitions, setup, volatility, vertices & 0xFFFF);
    if (!setup.same) {
        mask |= 0xF;
        GeometryData_SetStrides(data, setup.strides, setup.streams);
        for (uint32_t i = 0; i < draws; i++) {
            GeometryData_SetOffsets(data, i, setup.offsets, setup.streams);
        }
    }
    uint8_t colors = 0, normals = 0, binormals = 0, tangents = 0;
    uint32_t textureSets = 0;
    if ((mask & 0xFFFFFFEF) != 0) {
        bool skinned = skin != nullptr && skinPartition != nullptr;
        for (uint32_t i = 0; i < setup.streams; i++) {
            void* stream = GeometryData_GetStream(data, i);
            auto* locked = Virtual<uint8_t*>(stream, 2, 2u);
            uint32_t elements = ElementCount(partitions, i);
            for (uint32_t j = 0; j < elements; j++) {
                uint8_t* element = ElementOf(partitions, i, j);
                uint8_t* destination = Partitions_GetOffset(partitions, i, j) + locked;
                uint32_t stride = Partitions_GetStride(partitions, i);
                uint32_t semantic = At<uint32_t>(element, 4);
                bool convert = true;
                bool left = (mask & 2) != 0 && skinned;   // (the skinning's)
                if (semantic <= 0xE) {
                    static constexpr uint8_t kCases[15] = {0, 6, 6, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5};
                    switch (kCases[semantic]) {
                    case 0:
                        convert = !((mask & 1) != 0 && skinned);
                        break;
                    case 1:
                        normals = 1;
                        convert = !left;
                        break;
                    case 2:
                        colors = 1;
                        break;
                    case 3:
                        textureSets++;
                        break;
                    case 4:
                        tangents = 1;
                        convert = !left;
                        break;
                    case 5:
                        binormals = 1;
                        convert = !left;
                        break;
                    default:
                        break;
                    }
                }
                if (convert) {
                    Manager448_ConvertElement(At<void*>(self, kManager448), vertexCount, model, skin, element, stride,
                                              destination, 0, 0);
                } else {
                    Element_Skip(At<uint32_t>(element, 8));
                }
            }
            Virtual<void>(stream, 3);
        }
        GeometryData_StreamsChanged(data);
    }
    if (GeometryData_GetIndexStream_7460(data) == nullptr) {
        uint8_t both = binormals != 0 && tangents != 0 ? 1 : 0;
        GeometryData_SetInputLayoutKey(data, InputLayout_MakeKey(colors, normals, both, textureSets));
    }
    if (sourceIndices != nullptr || skinPartition != nullptr) {
        void* indexStream = GeometryData_GetIndexStream(data, 0);
        uint32_t bytes = indexTotal * 2;
        if (indexStream == nullptr || uint32_t(reinterpret_cast<uintptr_t>(Object_GetC(indexStream))) < bytes) {
            re::Ptr32<void> blob = nullptr;
            DataStream_Create(bytes, setup.usage, 1, &blob);
            for (uint32_t i = 0; i < draws; i++) {
                GeometryData_SetIndexStream(data, i, blob);
                GeometryData_SetIndexFormat(data, i, 0x39);   // DXGI_FORMAT_R16_UINT
            }
            mask |= 0x30;
            ReleaseRef(blob);
        }
        if ((mask & 0x30) != 0) {
            void* stream = GeometryData_GetIndexStream(data, 0);
            auto* locked = Virtual<uint8_t*>(stream, 2, 2u);
            if (skinPartition != nullptr) {
                for (uint32_t i = 0; i < partitionCount; i++) {
                    uint8_t* partition = At<uint8_t*>(skinPartition, 0xC) + i * 0x2C;
                    uint16_t strips = At<uint16_t>(partition, 0x22);
                    uint32_t count = strips == 0 ? uint32_t(At<uint16_t>(partition, 0x1E)) * 3
                                                 : At<uint16_t>(partition, 0x1E) + uint32_t(strips) * 2;
                    uint32_t size = count * 2;
                    crt::MemcpyS(locked, size, At<const void*>(partition, 0x14), size);
                    locked += size;
                }
            } else {
                uint32_t size = copyTotal * 2;
                crt::MemcpyS(locked, size, sourceIndices, size);
            }
            Virtual<void>(stream, 3);
            GeometryData_IndicesChanged(data);
        }
    }
    At<uint16_t>(model, 0x34) &= 0xF000;
    return 1;
}

// 0x8ADA50 (thiscall): NiD3D10Renderer::PackParticles(geometry data, particles data, partitions,
// camera right, camera up, force) (one draw of two triangles a particle; each a quad of the
// particle's radius times size, rotated by its rotation when it has them, built in the scratch
// buffers: positions, texture coordinates (0,1) (1,1) (1,0) (0,0), colours four times; the
// normal (right x up), binormal (-up) and tangent (right) the elements of their kind take. Quirk
// kept: an element of an unhandled kind gets the source stride of the one before)
uint8_t __fastcall NiD3D10Renderer_PackParticles(uint8_t* self, re::Edx, uint8_t* data, uint8_t* model,
                                                 uint8_t* partitions, const float* right, const float* up, int force) {
    if (data == nullptr || model == nullptr || partitions == nullptr) {
        return 0;
    }
    Partitions_Prepare(partitions);
    StreamSetup setup;
    setup.same = false;
    Object_Get4(data);
    setup.streams = At<uint32_t>(partitions, 0xC);
    if (Object_Get34(data) != nullptr && GeometryData_GetStreamCount(data) == setup.streams) {
        setup.same = true;
    } else {
        GeometryData_SetStreamCount(data, setup.streams);
    }
    uint32_t volatility = At<uint16_t>(model, 0x34) & 0xF000;
    if (!re::Flag(force) && setup.same && volatility == 0x4000) {
        return 1;
    }
    void* layoutB = Object_Get1C(partitions);
    GeometryData_Set78B0(data, Object_Get18(partitions), layoutB);
    uint32_t particles = At<uint16_t>(model, 0x30);
    uint32_t active = At<uint16_t>(model, 0xC);
    uint32_t vertexCount = uint16_t(particles * 4);       // (the converter's count)
    uint32_t vertices = uint16_t(active * 4);             // (the streams' size and the scratch's)
    uint32_t indexCount = uint32_t(uint16_t(active * 2)) * 3;
    uint32_t draws = GeometryData_GetDrawCount(data);
    GeometryData_SetStartVertex(data, 0, 0);
    GeometryData_SetIndexCount(data, 0, uint32_t(uint16_t(particles * 2)) * 3);
    GeometryData_SetStartIndex(data, 0, 0);
    GeometryData_SetBaseVertex(data, 0, 0);
    uint32_t mask = At<uint16_t>(model, 0x34) & 0xFFF;
    if (volatility == 0x8000 || re::Flag(force) || !setup.same) {
        mask = 0xFFF;
    }
    MakeStreams(data, partitions, setup, volatility, vertices);
    if (!setup.same) {
        mask |= 0xF;
        GeometryData_SetStrides(data, setup.strides, setup.streams);
        for (uint32_t i = 0; i < draws; i++) {
            GeometryData_SetOffsets(data, i, setup.offsets, setup.streams);
        }
    }
    // the normal, binormal and tangent all the particles share
    float tangent[3] = {right[0], right[1], right[2]};
    float normal[3];
    normal[0] = right[1] * up[2] - right[2] * up[1];
    normal[1] = right[2] * up[0] - right[0] * up[2];
    normal[2] = up[1] * right[0] - up[0] * right[1];
    float binormal[3] = {-up[0], -up[1], -up[2]};
    Renderer_EnsureScratch(self, vertices);
    const float* sizes = At<const float*>(model, 0x40);
    const float* radii = At<const float*>(model, 0x44);
    const float* rotations = At<const float*>(model, 0x4C);
    const float* positions = At<const float*>(model, 0x20);
    const uint32_t* colors = At<const uint32_t*>(model, 0x28);
    auto vectors = At<uint32_t*>(self, 0x45C);
    auto texture = At<float*>(self, 0x460);
    auto position = At<float*>(self, 0x458);
    for (uint32_t k = 0; k < particles && rotations != nullptr; k++, position += 12, texture += 8, positions += 3) {
        float z = radii[k] * sizes[k];
        float angle = rotations[k];
        float c = float(crt::Cos(angle));   // (fsincos)
        float s = float(crt::Sin(angle));
        float a = (s + c) * z;
        float b = (c - s) * z;
        float p[3], q[3];
        for (int i = 0; i < 3; i++) {
            p[i] = right[i] * a + up[i] * b;
        }
        float negB = -b;
        for (int i = 0; i < 3; i++) {
            q[i] = right[i] * negB + up[i] * a;
        }
        for (int i = 0; i < 3; i++) {
            position[i] = positions[i] - p[i];
            position[3 + i] = positions[i] - q[i];
            position[6 + i] = p[i] + positions[i];
            position[9 + i] = q[i] + positions[i];
        }
        texture[0] = 0.0f, texture[1] = 1.0f;
        texture[2] = 1.0f, texture[3] = 1.0f;
        texture[4] = 1.0f, texture[5] = 0.0f;
        texture[6] = 0.0f, texture[7] = 0.0f;
    }
    for (uint32_t k = 0; k < particles && rotations == nullptr; k++, position += 12, texture += 8, positions += 3) {
        float z = radii[k] * sizes[k];
        float sum[3] = {right[0] + up[0], up[1] + right[1], right[2] + up[2]};
        float p[3] = {sum[0] * z, sum[1] * z, sum[2] * z};
        float difference[3] = {up[0] - right[0], up[1] - right[1], up[2] - right[2]};
        float q[3] = {difference[0] * z, difference[1] * z, z * difference[2]};
        for (int i = 0; i < 3; i++) {
            position[i] = positions[i] - p[i];
            position[3 + i] = positions[i] - q[i];
            position[6 + i] = p[i] + positions[i];
            position[9 + i] = q[i] + positions[i];
        }
        texture[0] = 0.0f, texture[1] = 1.0f;
        texture[2] = 1.0f, texture[3] = 1.0f;
        texture[4] = 1.0f, texture[5] = 0.0f;
        texture[6] = 0.0f, texture[7] = 0.0f;
    }
    if (colors != nullptr) {
        for (uint32_t k = 0; k < particles; k++, colors += 4) {
            for (int corner = 0; corner < 4; corner++, vectors += 4) {
                vectors[0] = colors[0], vectors[1] = colors[1], vectors[2] = colors[2], vectors[3] = colors[3];
            }
        }
    }
    uint8_t colorsUsed = 0, normals = 0, binormals = 0, tangents = 0;
    uint32_t textureSets = 0;
    if ((mask & 0xFFFFFFEF) != 0) {
        uint32_t sourceStride = indexCount;   // (what an unhandled kind gets)
        for (uint32_t i = 0; i < setup.streams; i++) {
            void* stream = GeometryData_GetStream(data, i);
            auto* locked = Virtual<uint8_t*>(stream, 2, 2u);
            uint32_t elements = ElementCount(partitions, i);
            for (uint32_t j = 0; j < elements; j++) {
                uint8_t* element = ElementOf(partitions, i, j);
                uint8_t* destination = Partitions_GetOffset(partitions, i, j) + locked;
                uint32_t stride = Partitions_GetStride(partitions, i);
                uint32_t semantic = At<uint32_t>(element, 4);
                const void* source = nullptr;
                bool convert = true;
                if (semantic <= 0xE) {
                    static constexpr uint8_t kCases[15] = {0, 6, 6, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5};
                    switch (kCases[semantic]) {
                    case 0:
                        convert = (mask & 1) != 0;
                        if (convert) {
                            source = At<void*>(self, 0x458);
                            sourceStride = 0xC;
                        }
                        break;
                    case 1:
                        convert = (mask & 2) != 0;
                        if (convert) {
                            normals = 1;
                            source = normal;
                            sourceStride = 0;
                        }
                        break;
                    case 5:
                        convert = (mask & 2) != 0;
                        if (convert) {
                            binormals = 1;
                            source = binormal;
                            sourceStride = 0;
                        }
                        break;
                    case 4:
                        convert = (mask & 2) != 0;
                        if (convert) {
                            tangents = 1;
                            source = tangent;
                            sourceStride = 0;
                        }
                        break;
                    case 2:
                        convert = (mask & 4) != 0;
                        if (convert) {
                            source = At<void*>(self, 0x45C);
                            sourceStride = colors != nullptr ? 0x10 : 0;
                            colorsUsed = 1;
                        }
                        break;
                    case 3:
                        convert = (mask & 8) != 0;
                        if (convert) {
                            textureSets++;
                            source = At<void*>(self, 0x460);
                            sourceStride = 8;
                        }
                        break;
                    default:
                        break;
                    }
                }
                if (convert) {
                    Manager448_ConvertElement(At<void*>(self, kManager448), vertexCount, model, nullptr, element,
                                              stride, destination, int(reinterpret_cast<uintptr_t>(source)),
                                              int(sourceStride));
                } else {
                    Element_Skip(At<uint32_t>(element, 8));
                }
            }
            Virtual<void>(stream, 3);
        }
        GeometryData_StreamsChanged(data);
    }
    if (GeometryData_GetIndexStream_7460(data) == nullptr) {
        uint8_t both = binormals != 0 && tangents != 0 ? 1 : 0;
        GeometryData_SetInputLayoutKey(data, InputLayout_MakeKey(colorsUsed, normals, both, textureSets));
    }
    void* indexStream = GeometryData_GetIndexStream(data, 0);
    uint32_t bytes = indexCount * 2;
    if (indexStream == nullptr || uint32_t(reinterpret_cast<uintptr_t>(Object_GetC(indexStream))) < bytes) {
        re::Ptr32<void> blob = nullptr;
        DataStream_Create(bytes, setup.usage, 1, &blob);
        for (uint32_t i = 0; i < draws; i++) {
            GeometryData_SetIndexStream(data, i, blob);
            GeometryData_SetIndexFormat(data, i, 0x39);   // DXGI_FORMAT_R16_UINT
        }
        mask |= 0x30;
        ReleaseRef(blob);
    }
    if ((mask & 0x30) != 0) {
        void* stream = GeometryData_GetIndexStream(data, 0);
        auto* indices = Virtual<uint16_t*>(stream, 2, 2u);
        for (uint32_t k = 0; k < particles; k++) {
            uint16_t base = uint16_t(k * 4);
            *indices++ = base;
            *indices++ = uint16_t(base + 1);
            *indices++ = uint16_t(base + 2);
            *indices++ = base;
            *indices++ = uint16_t(base + 2);
            *indices++ = uint16_t(base + 3);
        }
        Virtual<void>(stream, 3);
        GeometryData_IndicesChanged(data);
    }
    At<uint16_t>(model, 0x34) &= 0xF000;
    return 1;
}

#define C42J_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42J_REVERSED("NiD3D10Renderer::PackGeometry", 0x008ABCB0, NiD3D10Renderer_PackGeometry);
C42J_REVERSED("NiD3D10Renderer::PackParticles", 0x008ADA50, NiD3D10Renderer_PackParticles);

} // namespace ni::c42
