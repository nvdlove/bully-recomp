// NiD3D10Renderer (src/gamebryo/NiD3D10Renderer.h): the geometry's renderer data (made per
// geometry kind and kept in its model data's list), the render of a geometry through its shader's
// pipeline, the lines' packing into data streams, the screen textures' packing into a screen
// elements node.
//
// The D3D10 geometry data (NiD3D10GeometryData, 0x4C bytes; its code is not this chunk's): data
// streams by stream (+0x34 the array), a strides and offsets set, an index stream; the
// partitions' description passed with it: +0xC its stream count, +0x10 for each stream (0x10) its
// element count and elements (0x18 each: +0x4 the semantic, +0x8 what the skipped ones are given).

#include "gamebryo/NiD3D10Renderer.h"
#include "reversed.h"

namespace ni::c42 {
namespace {

using namespace ni::d3d10;

GAME_VAR(uint32_t, NiTriShape_ClassBit, 0x020E3104);
GAME_VAR(uint32_t, NiD3D10Shader_ClassBit, 0x020E9178);
constexpr uintptr_t NiTriStrips_RTTI = 0x020E3B64;
constexpr uintptr_t NiParticles_RTTI = 0x020E3F84;   // (names guesses, by the topology they get)
constexpr uintptr_t NiLines_RTTI = 0x020E3040;
constexpr uintptr_t NiD3D10ShaderInterface_RTTI = 0x020E9170;
constexpr uintptr_t kVertexColorPropertyVtable = 0x0092D0D4;
constexpr uintptr_t kAlphaPropertyVtable = 0x008FFB9C;
constexpr uintptr_t kZBufferPropertyVtable = 0x008FFCA4;

// ID3D10Device slots
enum : int {
    kDrawIndexed = 8,
    kIASetInputLayout = 11,
    kIASetVertexBuffers = 12,
    kIASetIndexBuffer = 13,
    kIASetPrimitiveTopology = 18,
};

GAME_FN(uint8_t, __cdecl, IsKindOf, 0x004066F0, uintptr_t rtti, const void* object);
GAME_FN(void*, __cdecl, DynamicCast, 0x00401FF0, uintptr_t rtti, void* object);
GAME_FN(void, __thiscall, AVObject_AttachProperty, 0x004099F0, void* self, void* property);
GAME_FN(void, __thiscall, Geometry_SetActiveMaterial, 0x00402880, void* geometry, void* material);
GAME_FN(void*, __thiscall, Geometry_GetActiveShader, 0x00761630, void* geometry);
GAME_FN(void, __thiscall, Geometry_ApplyActiveMaterial, 0x00761F70, void* geometry, void* material, int data);
GAME_FN(void, __thiscall, ObjectNET_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* self, float time, int updateControllers);
GAME_FN(void, __thiscall, AVObject_UpdateProperties, 0x0075E040, void* self);
GAME_FN(void*, __thiscall, ScreenElementsData_Construct, 0x007893A0, void* self, int a, int b, int c, uint32_t polygons,
        int d, uint32_t vertices, int e, uint32_t triangles, int f);
GAME_FN(void*, __thiscall, ScreenElements_Construct, 0x007AB380, void* self, void* data);
GAME_FN(void, __thiscall, ScreenElementsData_RemoveAll, 0x00789010, void* self);
GAME_FN(int32_t, __thiscall, ScreenElementsData_Insert, 0x00788AA0, void* self, uint16_t vertices, uint16_t triangles,
        uintptr_t a);
GAME_FN(void, __thiscall, ScreenElementsData_SetRectangle, 0x00789060, void* self, int32_t polygon, float left,
        float top, float width, float height);
GAME_FN(void, __thiscall, ScreenElementsData_SetColors, 0x00789160, void* self, int32_t polygon, const void* color);
GAME_FN(void, __thiscall, ScreenElementsData_SetTextures, 0x00789300, void* self, int32_t polygon, int set,
        float left, float top, float right, float bottom);
GAME_FN(void*, __thiscall, Object_Get4, 0x0066B5D0, void* self);     // (+0x4)
GAME_FN(void*, __thiscall, Object_GetC, 0x00853140, void* self);     // (+0xC)
GAME_FN(void*, __thiscall, Object_Get18, 0x008B40F0, void* self);    // (+0x18)
GAME_FN(void*, __thiscall, Object_Get1C, 0x0077E8E0, void* self);    // (+0x1C)
GAME_FN(void*, __thiscall, Object_Get34, 0x008A2F70, void* self);    // (+0x34)
GAME_FN(void*, __thiscall, Object_Get38, 0x008A30A0, void* self);    // (+0x38)
// the geometry data's and the partitions' (not this chunk's)
GAME_FN(uint32_t, __thiscall, GeometryData_GetStreamCount, 0x0075D400, void* self);
GAME_FN(void, __thiscall, Partitions_Prepare, 0x008B43E0, void* self);
GAME_FN(uint32_t, __thiscall, Partitions_GetStride, 0x008B4510, void* self, uint32_t stream);
GAME_FN(uint32_t, __thiscall, Partitions_GetOffset, 0x008B4540, void* self, uint32_t stream, uint32_t element);
GAME_FN(void, __cdecl, Element_Skip, 0x008B4100, uint32_t value);
GAME_FN(void, __thiscall, Manager448_ConvertElement, 0x008B2BD0, void* self, uint32_t vertices, void* model, uintptr_t a,
        void* element, uint32_t stride, void* destination, int b, uintptr_t c);
GAME_FN(uint32_t, __thiscall, GeometryData_GetDrawCount, 0x008B7300, void* self);
GAME_FN(uint32_t, __thiscall, GeometryData_GetFirstDraw, 0x008B72C0, void* self, uint32_t partition);
GAME_FN(uint32_t, __thiscall, GeometryData_GetDrawsIn, 0x008B72E0, void* self, uint32_t partition);
GAME_FN(void*, __thiscall, GeometryData_GetIndexBuffer, 0x008B7310, void* self, uint32_t draw);
GAME_FN(uint32_t, __thiscall, GeometryData_GetIndexFormat, 0x008B7340, void* self, uint32_t draw);
GAME_FN(uint32_t, __thiscall, GeometryData_GetIndexOffset, 0x008B7370, void* self, uint32_t draw);
GAME_FN(const uint32_t*, __thiscall, GeometryData_GetVertexOffsets, 0x008B73A0, void* self, uint32_t draw);
GAME_FN(uint32_t, __thiscall, GeometryData_GetIndexCount, 0x008B73D0, void* self, uint32_t draw);
GAME_FN(uint32_t, __thiscall, GeometryData_GetStartIndex, 0x008B7400, void* self, uint32_t draw);
GAME_FN(int32_t, __thiscall, GeometryData_GetBaseVertex, 0x008B7430, void* self, uint32_t draw);
GAME_FN(void*, __thiscall, GeometryData_GetIndexStream_7460, 0x008B7460, void* self);
GAME_FN(void, __thiscall, GeometryData_SetStrides, 0x008B7470, void* self, const uint32_t* strides, uint32_t count);
GAME_FN(void, __thiscall, GeometryData_SetIndexFormat, 0x008B74C0, void* self, uint32_t draw, uint32_t format);
GAME_FN(void, __thiscall, GeometryData_Set74F0, 0x008B74F0, void* self, uint32_t a, uint32_t b);
GAME_FN(void, __thiscall, GeometryData_SetOffsets, 0x008B7520, void* self, uint32_t draw, const uint32_t* offsets,
        uint32_t count);
GAME_FN(void, __thiscall, GeometryData_Set7590, 0x008B7590, void* self, uint32_t a, uint32_t b);
GAME_FN(void, __thiscall, GeometryData_Set75C0, 0x008B75C0, void* self, uint32_t a, uint32_t b);
GAME_FN(void, __thiscall, GeometryData_Set75F0, 0x008B75F0, void* self, uint32_t a, uint32_t b);
GAME_FN(void, __thiscall, GeometryData_SetInputLayoutKey, 0x008B7620, void* self, uint32_t key);
GAME_FN(uint32_t, __cdecl, InputLayout_MakeKey, 0x008B7630, int a, int b, int c, uint32_t d);
GAME_FN(void*, __thiscall, GeometryData_GetOwner, 0x008B7680, void* self);
GAME_FN(void, __thiscall, GeometryData_SetNext, 0x008B7690, void* self, void* next);
GAME_FN(void*, __thiscall, GeometryData_GetStream, 0x008B7850, void* self, uint32_t stream);
GAME_FN(void*, __thiscall, GeometryData_GetIndexStream, 0x008B7880, void* self, uint32_t draw);
GAME_FN(void, __thiscall, GeometryData_Set78B0, 0x008B78B0, void* self, void* a, void* b);
GAME_FN(void, __thiscall, GeometryData_SetStream, 0x008B7930, void* self, uint32_t stream, void* blob);
GAME_FN(void, __thiscall, GeometryData_SetIndexStream, 0x008B7A00, void* self, uint32_t draw, void* blob);
GAME_FN(void, __thiscall, GeometryData_StreamsChanged, 0x008B7AC0, void* self);
GAME_FN(void, __thiscall, GeometryData_IndicesChanged, 0x008B7B30, void* self);
GAME_FN(void*, __thiscall, GeometryData_Construct, 0x008B7BB0, void* self, uint32_t strips);
GAME_FN(void*, __thiscall, GeometryData_ConstructPartitions, 0x008B7C90, void* self, void* skin, uint32_t count,
        const uint32_t* strips);
GAME_FN(void, __thiscall, GeometryData_SetStreamCount, 0x008B7EA0, void* self, uint32_t count);
GAME_FN(void, __cdecl, DataStream_Create, 0x008B84E0, uint32_t size, uint32_t usage, uint32_t kind, re::Ptr32<void>* result);
GAME_FN(void, __thiscall, GeometryData_SetTopology, 0x008C00F0, void* self, uint32_t topology);
GAME_FN(void*, __thiscall, GeometryData_Next, 0x00749E60, void* self);
GAME_FN(void, __thiscall, RenderStateManager_Apply, 0x008B1530, void* self);
// this chunk's
GAME_FN(void*, __thiscall, Renderer_CreateGeometryData, 0x008ABAD0, void* self, void* geometry);
GAME_FN(void, __thiscall, ShaderConstantManager_Apply, 0x008AFC60, void* self);

inline uint8_t* Device(uint8_t* self) {
    return At<uint8_t*>(self, kDevice);
}

} // namespace

// 0x8ABAD0 (thiscall; `this` unused): NiD3D10Renderer::CreateGeometryData (by the geometry's kind:
// its topology; a partitioned skin's strips counts; put at its model data's list's end)
void* __fastcall NiD3D10Renderer_CreateGeometryData(void*, re::Edx, uint8_t* geometry) {
    if (geometry == nullptr || At<void*>(geometry, 0xCC) == nullptr) {
        return nullptr;
    }
    uint32_t topology = 0;
    if (HasClassBit(geometry, NiTriShape_ClassBit)) {
        topology = 4;   // D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    } else if (IsKindOf(NiTriStrips_RTTI, geometry) != 0) {
        topology = 5;   // TRIANGLESTRIP
    } else if (IsKindOf(NiParticles_RTTI, geometry) != 0) {
        topology = 4;
    } else if (IsKindOf(NiLines_RTTI, geometry) != 0) {
        topology = 2;   // LINELIST
    }
    void* data;
    uint8_t* skin = At<uint8_t*>(geometry, 0xD0);
    if (skin != nullptr && At<void*>(skin, 0xC) != nullptr) {
        uint8_t* partition = At<uint8_t*>(skin, 0xC);
        uint32_t count = At<uint32_t>(partition, 8);
        uint8_t* partitions = At<uint8_t*>(partition, 0xC);
        topology = (At<uint16_t>(partitions, 0x22) != 0 ? 1u : 0u) + 4;
        uint32_t strips[0x100];
        for (uint32_t i = 0; i < count; i++) {
            uint16_t stripCount = At<uint16_t>(partitions + i * 0x2C, 0x22);
            strips[i] = stripCount == 0 ? 1 : stripCount;
        }
        void* memory = NiNew(0x4C);
        data = memory != nullptr ? GeometryData_ConstructPartitions(memory, skin, count, strips) : nullptr;
    } else if (IsKindOf(NiTriStrips_RTTI, geometry) != 0) {
        uint8_t* model = At<uint8_t*>(geometry, 0xCC);
        void* memory = NiNew(0x4C);
        data = memory != nullptr ? GeometryData_Construct(memory, At<uint16_t>(model, 0x44)) : nullptr;
    } else {
        void* memory = NiNew(0x4C);
        data = memory != nullptr ? GeometryData_Construct(memory, 1) : nullptr;
    }
    GeometryData_SetTopology(data, topology);
    uint8_t* model = At<uint8_t*>(geometry, 0xCC);
    void* last = At<void*>(model, 0x3C);
    if (last == nullptr) {
        At<void*>(model, 0x3C) = data;
        return data;
    }
    while (GeometryData_Next(last) != nullptr) {
        last = GeometryData_Next(last);
    }
    GeometryData_SetNext(last, data);
    return data;
}

// 0x8ACA80 (thiscall): NiD3D10Renderer::RenderGeometry(geometry, kind 0..3) (its shader (its own
// D3D10 one, else the default material's, else the error shader) run over its passes and
// partitions: each partition's draws (index buffer, vertex buffers, indexed draw). Quirks kept:
// the partition pointer isn't reset between passes; after the first partition the buffer data the
// shader prepared is what the next calls get)
void __fastcall NiD3D10Renderer_RenderGeometry(uint8_t* self, re::Edx, uint8_t* geometry, uint32_t kind) {
    if (At<void*>(self, 0xC) == nullptr) {
        Warning(Text(0x0099FBA8), At<const char*>(geometry, 8));
        return;
    }
    void* shader = nullptr;
    if (kind == 0 || kind == 1) {
        void* active = Geometry_GetActiveShader(geometry);
        if (active != nullptr && HasClassBit(active, NiD3D10Shader_ClassBit)) {
            shader = active;
        }
    }
    if (shader == nullptr) {
        uint32_t index = At<uint32_t>(geometry, 0xE0);
        void* previous = index < At<uint32_t>(geometry, 0xDC) ? At<void*>(At<uint8_t*>(geometry, 0xD4) + index * 16, 4)
                                                              : nullptr;
        Geometry_ApplyActiveMaterial(geometry, At<void*>(self, 0x18), -1);
        shader = DynamicCast(NiD3D10ShaderInterface_RTTI, Geometry_GetActiveShader(geometry));
        Geometry_SetActiveMaterial(geometry, previous);
        if (shader == nullptr) {
            Warning(Text(0x0099FB70), At<const char*>(geometry, 8));
            shader = Virtual<void*>(self, 64);   // GetErrorShader
        }
    }
    uint8_t* skin = At<uint8_t*>(geometry, 0xD0);
    uint8_t* skinPartition = skin != nullptr ? At<uint8_t*>(skin, 0xC) : nullptr;
    uint32_t partitions = skinPartition != nullptr ? At<uint32_t>(skinPartition, 8) : 1;
    uint8_t* partition = skinPartition != nullptr ? At<uint8_t*>(skinPartition, 0xC) : nullptr;
    void* data = At<void*>(At<uint8_t*>(geometry, 0xCC), 0x3C);
    while (data != nullptr && GeometryData_GetOwner(data) != skin) {
        data = GeometryData_Next(data);
    }
    if (data == nullptr) {
        data = Renderer_CreateGeometryData(self, geometry);
    }
    void* state = At<void*>(self, 0xC);
    void* effects = At<void*>(self, 0x10);
    uint8_t* world = geometry + 0x60;
    uint8_t* bound = geometry + 0x1C;
    if (Virtual<uint32_t>(shader, 6, geometry, skin, data, state, effects, world, bound) != 0) {   // PreProcessPipeline
        Warning(Text(0x0099FB28), At<const char*>(geometry, 8));
        return;
    }
    Virtual<uint32_t>(shader, 7, geometry, skin, data, state, effects, world, bound);   // UpdatePipeline
    if (Virtual<uint32_t>(shader, 15) != 0) {   // FirstPass
        do {
            Virtual<uint32_t>(shader, 8, geometry, skin, data, state, effects, world, bound);   // SetupRenderingPass
            for (uint32_t pass = 0; pass < partitions; pass++) {
                Virtual<uint32_t>(shader, 9, geometry, skin, partition, data, state, effects, world, bound);
                if (kind == 2) {
                    data = Virtual<void*>(shader, 30, geometry, partition, data, state);
                } else if (kind == 3) {
                    data = Virtual<void*>(shader, 31, geometry, partition, data, state);
                } else {
                    data = Virtual<void*>(shader, 11, geometry, partition, data, state);   // PrepareGeometryForRendering
                }
                Virtual<uint32_t>(shader, 10, geometry, skin, partition, data, state, effects, world, bound);
                RenderStateManager_Apply(At<void*>(self, kRenderStateManager));
                ShaderConstantManager_Apply(At<void*>(self, kShaderResourceManager));
                if (pass == 0) {
                    Com<long>(Device(self), kIASetPrimitiveTopology, Object_GetC(data));
                    Com<long>(Device(self), kIASetInputLayout, Object_Get18(data));
                }
                uint32_t draw = GeometryData_GetFirstDraw(data, pass);
                uint32_t end = draw + GeometryData_GetDrawsIn(data, pass);
                for (; draw < end; draw++) {
                    uint32_t offset = GeometryData_GetIndexOffset(data, draw);
                    uint32_t format = GeometryData_GetIndexFormat(data, draw);
                    void* indexBuffer = GeometryData_GetIndexBuffer(data, draw);
                    Com<long>(Device(self), kIASetIndexBuffer, indexBuffer, format, offset);
                    const uint32_t* offsets = GeometryData_GetVertexOffsets(data, draw);
                    void* strides = Object_Get38(data);
                    void* buffers = Object_Get34(data);
                    uint32_t streams = GeometryData_GetStreamCount(data);
                    Com<long>(Device(self), kIASetVertexBuffers, 0u, streams, buffers, strides, offsets);
                    int32_t baseVertex = GeometryData_GetBaseVertex(data, draw);
                    uint32_t start = GeometryData_GetStartIndex(data, draw);
                    uint32_t indexCount = GeometryData_GetIndexCount(data, draw);
                    Com<long>(Device(self), kDrawIndexed, indexCount, start, baseVertex);
                }
                Virtual<uint32_t>(shader, 12, geometry, skin, partition, data, state, effects, world, bound);   // PostRender
                if (partition != nullptr) {
                    partition += 0x2C;
                }
            }
        } while (Virtual<uint32_t>(shader, 16) != 0);   // NextPass
    }
    Virtual<uint32_t>(shader, 13, geometry, skin, data, state, effects, world, bound);   // PostProcessPipeline
}

// 0x8A9DE0 (thiscall): NiD3D10Renderer::PackLines(geometry data, model data, partitions, force)
// (the streams made big enough, each element converted (or skipped when unchanged), the input
// layout's key, the line list's indices from the connectivity flags)
uint8_t __fastcall NiD3D10Renderer_PackLines(uint8_t* self, re::Edx, uint8_t* data, uint8_t* model, uint8_t* partitions,
                                            int force) {
    if (data == nullptr || model == nullptr || partitions == nullptr) {
        return 0;
    }
    Partitions_Prepare(partitions);
    bool same = false;
    Object_Get4(data);
    uint32_t streams = At<uint32_t>(partitions, 0xC);
    if (Object_Get34(data) != nullptr && GeometryData_GetStreamCount(data) == streams) {
        same = true;
    } else {
        GeometryData_SetStreamCount(data, streams);
    }
    uint32_t volatility = At<uint16_t>(model, 0x34) & 0xF000;
    if (!re::Flag(force) && same && volatility == 0x4000) {
        return 1;
    }
    void* layoutA = Object_Get1C(partitions);
    GeometryData_Set78B0(data, Object_Get18(partitions), layoutA);
    uint32_t connections = At<uint16_t>(model, 0xC);
    uint32_t vertices = At<uint16_t>(model, 0x30);
    const uint8_t* flags = At<const uint8_t*>(model, 0x40);
    uint32_t lines = 0;
    for (uint32_t i = 0; i < connections; i++) {
        if (flags[i] != 0) {
            lines++;
        }
    }
    uint32_t draws = GeometryData_GetDrawCount(data);
    GeometryData_Set74F0(data, 0, 0);
    GeometryData_Set7590(data, 0, (lines & 0xFFFF) * 2);
    GeometryData_Set75C0(data, 0, 0);
    GeometryData_Set75F0(data, 0, 0);
    uint32_t mask = At<uint16_t>(model, 0x34) & 0xFFF;
    if (volatility == 0x8000 || re::Flag(force) || !same) {
        mask = 0xFFF;
    }
    re::Ptr32<void>* buffers = static_cast<re::Ptr32<void>*>(Object_Get34(data));
    uint32_t usage = 4;
    if (volatility == 0x8000) {
        usage = 0x14;
    } else if (volatility == 0) {
        usage = 6;
    }
    uint32_t strides[15];
    uint32_t offsets[15];
    for (uint32_t i = 0; i < streams; i++) {
        uint32_t stride = Partitions_GetStride(partitions, i);
        strides[i] = stride;
        uint32_t size = connections * stride;
        if (buffers[i] == nullptr ||
            uint32_t(reinterpret_cast<uintptr_t>(Object_GetC(GeometryData_GetStream(data, i)))) < size) {
            re::Ptr32<void> blob = nullptr;
            DataStream_Create(size, usage, 2, &blob);
            GeometryData_SetStream(data, i, blob);
            same = false;
            ReleaseRef(blob);
        }
        offsets[i] = 0;
    }
    if (!same) {
        mask |= 0xF;
        GeometryData_SetStrides(data, strides, streams);
        for (uint32_t i = 0; i < draws; i++) {
            GeometryData_SetOffsets(data, i, offsets, streams);
        }
    }
    uint8_t binormals = 0, tangents = 0, colors = 0, normals = 0;
    uint32_t textureSets = 0;
    if ((mask & 0xFFFFFFEF) != 0) {
        for (uint32_t i = 0; i < streams; i++) {
            void* stream = GeometryData_GetStream(data, i);
            auto* locked = Virtual<uint8_t*>(stream, 2, 2u);
            uint8_t* infos = At<uint8_t*>(partitions, 0x10);
            uint32_t elements = infos != nullptr && i < At<uint32_t>(partitions, 0xC) ? At<uint32_t>(infos, i * 0x10) : 0;
            for (uint32_t j = 0; j < elements; j++) {
                infos = At<uint8_t*>(partitions, 0x10);
                uint8_t* element = i < At<uint32_t>(partitions, 0xC) && infos != nullptr &&
                                           j < At<uint32_t>(infos, i * 0x10)
                                       ? At<uint8_t*>(infos, i * 0x10 + 4) + j * 0x18
                                       : nullptr;
                uint8_t* destination = Partitions_GetOffset(partitions, i, j) + locked;
                uint32_t stride = Partitions_GetStride(partitions, i);
                uint32_t semantic = At<uint32_t>(element, 4);
                bool convert = true;
                if (semantic <= 0xE) {
                    static constexpr uint8_t kCases[15] = {0, 6, 6, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5};
                    switch (kCases[semantic]) {
                    case 0:
                        convert = (mask & 1) != 0;
                        break;
                    case 1:
                        convert = (mask & 2) != 0;
                        if (convert) {
                            normals = 1;
                        }
                        break;
                    case 2:
                        convert = (mask & 4) != 0;
                        if (convert) {
                            colors = 1;
                        }
                        break;
                    case 3:
                        convert = (mask & 8) != 0;
                        if (convert) {
                            textureSets++;
                        }
                        break;
                    case 4:
                        convert = (mask & 2) != 0;
                        if (convert) {
                            tangents = 1;
                        }
                        break;
                    case 5:
                        convert = (mask & 2) != 0;
                        if (convert) {
                            binormals = 1;
                        }
                        break;
                    default:
                        break;
                    }
                }
                if (convert) {
                    Manager448_ConvertElement(At<void*>(self, kManager448), vertices, model, 0, element, stride,
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
    void* indexStream = GeometryData_GetIndexStream(data, 0);
    uint32_t indexBytes = connections * 4;
    if (indexStream == nullptr || uint32_t(reinterpret_cast<uintptr_t>(Object_GetC(indexStream))) < indexBytes) {
        re::Ptr32<void> blob = nullptr;
        DataStream_Create(indexBytes, usage, 1, &blob);
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
        uint32_t last = connections - 1;
        const uint8_t* flag = flags;
        for (uint32_t i = 0; i < last; i++, flag++) {
            if (*flag != 0) {
                indices[0] = uint16_t(i);
                indices[1] = uint16_t(i + 1);
                indices += 2;
            }
        }
        if (*flag != 0) {
            indices[0] = uint16_t(last);
            indices[1] = 0;
        }
        Virtual<void>(stream, 3);
        GeometryData_IndicesChanged(data);
    }
    At<uint16_t>(model, 0x34) &= 0xF000;
    return 1;
}

// 0x8AE870 (thiscall): NiD3D10Renderer::PackScreenTexture (the screen elements node made the
// first time (its data, vertex colour, alpha and z-buffer properties) or grown; each rectangle a
// polygon: its position wrapped onto the target, half a pixel off, its colours and texture
// coordinates. Quirk kept: a top past the height is wrapped by the width)
void __fastcall NiD3D10Renderer_PackScreenTexture(uint8_t* self, re::Edx, uint8_t* screenTexture) {
    uint32_t count = At<uint32_t>(screenTexture, 0x10);
    re::Ptr32<void>& node = At<void*>(self, 0x468);
    if (node == nullptr) {
        void* memory = NiNew(0x70);
        auto* data = static_cast<uint8_t*>(memory != nullptr ? ScreenElementsData_Construct(memory, 0, 1, 1, count, 1,
                                                                                           count * 4, 1, count * 2, 1)
                                                             : nullptr);
        At<uint16_t>(data, 0x34) = uint16_t((At<uint16_t>(data, 0x34) & 0xFFF) | 0x8000);
        memory = NiNew(0xEC);
        void* elements = memory != nullptr ? ScreenElements_Construct(memory, data) : nullptr;
        AssignRef(node, elements);
        auto* vertexColor = static_cast<uint8_t*>(NiNew(0x1C));
        if (vertexColor != nullptr) {
            ObjectNET_Construct(vertexColor);
            At<uint32_t>(vertexColor, 0) = kVertexColorPropertyVtable;
            At<uint16_t>(vertexColor, 0x18) = 8;
        }
        At<uint16_t>(vertexColor, 0x18) = uint16_t((At<uint16_t>(vertexColor, 0x18) & 0xFFD7) | 0x10);
        AVObject_AttachProperty(node, vertexColor);
        auto* alpha = static_cast<uint8_t*>(NiNew(0x1C));
        if (alpha != nullptr) {
            ObjectNET_Construct(alpha);
            At<uint32_t>(alpha, 0) = kAlphaPropertyVtable;
            At<uint8_t>(alpha, 0x1A) = 0;
            At<uint16_t>(alpha, 0x18) = 0xEC;
        }
        At<uint16_t>(alpha, 0x18) = uint16_t((At<uint16_t>(alpha, 0x18) & 0xFEED) | 0xED);
        AVObject_AttachProperty(node, alpha);
        auto* zbuffer = static_cast<uint8_t*>(NiNew(0x30));
        if (zbuffer != nullptr) {
            ObjectNET_Construct(zbuffer);
            At<float>(zbuffer, 0x1C) = 0.0f;
            At<uint32_t>(zbuffer, 0) = kZBufferPropertyVtable;
            At<float>(zbuffer, 0x20) = 0.0f;
            At<uint32_t>(zbuffer, 0x24) = 0;
            At<uint16_t>(zbuffer, 0x18) = 0xF;
            At<float>(zbuffer, 0x28) = 1.0f;
            At<float>(zbuffer, 0x2C) = 1.0f;
        }
        At<uint16_t>(zbuffer, 0x18) &= 0xFFFC;
        AVObject_AttachProperty(node, zbuffer);
        AVObject_Update(node, 0.0f, 1);
        AVObject_UpdateProperties(node);
    } else {
        uint8_t* data = At<uint8_t*>(node, 0xCC);
        uint32_t polygons = At<uint16_t>(data, 0x60);
        if (count > polygons) {
            int32_t grow = int32_t(count - polygons);
            if (grow <= 0) {
                grow = 1;
            }
            At<uint16_t>(data, 0x62) = uint16_t(grow);
            data = At<uint8_t*>(node, 0xCC);
            grow = int32_t(count * 4 - At<uint16_t>(data, 0xC));
            if (grow <= 0) {
                grow = 1;
            }
            At<uint16_t>(data, 0x68) = uint16_t(grow);
            data = At<uint8_t*>(node, 0xCC);
            grow = int32_t(count * 2 - At<uint16_t>(data, 0x40));
            At<uint16_t>(data, 0x6C) = uint16_t(grow > 0 ? grow * 3 : 3);
        }
        ScreenElementsData_RemoveAll(At<uint8_t*>(node, 0xCC));
    }
    uint8_t* data = At<uint8_t*>(node, 0xCC);
    uint16_t width = uint16_t(Virtual<uint32_t>(Virtual<void*>(self, 30), 18, 0u));
    uint16_t height = uint16_t(Virtual<uint32_t>(Virtual<void*>(self, 30), 19, 0u));
    int32_t widthInt = width;
    int32_t heightInt = height;
    float invWidth = 1.0f / float(widthInt);
    float invHeight = 1.0f / float(heightInt);
    uint8_t* textureBuffers = At<uint8_t*>(At<uint8_t*>(screenTexture, 0x14), 0x20);
    void* head = At<void*>(textureBuffers, 0);
    void* buffer = head != nullptr ? At<void*>(head, 8) : nullptr;
    float invTextureWidth = 1.0f / float(Virtual<uint32_t>(buffer, 18));
    float invTextureHeight = 1.0f / float(Virtual<uint32_t>(buffer, 19));
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t* rect = At<const uint8_t*>(screenTexture, 8) + i * 0x1C;
        int32_t polygon = ScreenElementsData_Insert(data, 4, 0, 0);
        uint16_t left = At<uint16_t>(rect, 2);
        while (int16_t(left) < 0) {
            left = uint16_t(left + width);
        }
        while (int16_t(left) > widthInt) {
            left = uint16_t(left - width);
        }
        uint16_t top = At<uint16_t>(rect, 0);
        while (int16_t(top) < 0) {
            top = uint16_t(top + height);
        }
        while (int16_t(top) > heightInt) {
            top = uint16_t(top - width);
        }
        uint32_t rectHeight = At<uint16_t>(rect, 6);
        uint32_t rectWidth = At<uint16_t>(rect, 4);
        float h = float(int32_t(rectHeight)) * invHeight;
        float w = float(int32_t(rectWidth)) * invWidth;
        float y = (float(int32_t(int16_t(top))) - 0.5f) * invHeight;
        float x = (float(int32_t(int16_t(left))) - 0.5f) * invWidth;
        ScreenElementsData_SetRectangle(data, polygon, x, y, w, h);
        ScreenElementsData_SetColors(data, int32_t(i), rect + 0xC);
        uint32_t textureTop = At<uint16_t>(rect, 8);
        uint32_t textureLeft = At<uint16_t>(rect, 0xA);
        float bottom = float(int32_t(rectHeight + textureTop)) * invTextureHeight;
        float right = float(int32_t(rectWidth + textureLeft)) * invTextureWidth;
        float topCoord = float(int32_t(textureTop)) * invTextureHeight;
        float leftCoord = invTextureWidth * float(int32_t(textureLeft));
        ScreenElementsData_SetTextures(data, int32_t(i), 0, leftCoord, topCoord, right, bottom);
    }
    At<uint16_t>(screenTexture, 0x18) = 0;
}

#define C42I_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C42I_REVERSED("NiD3D10Renderer::PackLines", 0x008A9DE0, NiD3D10Renderer_PackLines);
C42I_REVERSED("NiD3D10Renderer::CreateGeometryData", 0x008ABAD0, NiD3D10Renderer_CreateGeometryData);
C42I_REVERSED("NiD3D10Renderer::RenderGeometry", 0x008ACA80, NiD3D10Renderer_RenderGeometry);
C42I_REVERSED("NiD3D10Renderer::PackScreenTexture", 0x008AE870, NiD3D10Renderer_PackScreenTexture);

} // namespace ni::c42
