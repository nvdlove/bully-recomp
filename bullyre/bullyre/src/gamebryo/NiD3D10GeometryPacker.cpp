// NiD3D10GeometryPacker (vtable 0x9A07F4: one slot, the deleting destructor; 4 bytes, the
// renderer's +0x448; the name a guess from Gamebryo 2.5's D3D10 renderer): packs geometry data
// into vertex buffers by shader declaration entry. Its packing functions are kept in a static
// NiTPrimitiveArray (0x20E8F68) of one 0x14-byte holder per D3D type (the type, then an
// NiTPrimitiveArray of the functions by source type), as NiD3DShaderDeclaration's (0x20E7480,
// chunk 41's NiD3DShaderDeclaration2.cpp) are; many of the functions are shared with it (some of
// this range's are in its table too).
//
// The D3D types are the D3DDECLTYPE numbers (0..3 FLOAT1..4, 4 D3DCOLOR, 5 UBYTE4, 6/7 SHORT2/4,
// 8 UBYTE4N, 9/10 SHORT2N/4N, 11/12 USHORT2N/4N, 13 UDEC3, 14 DEC3N, 15/16 FLOAT16_2/4, 17
// unused), mapped to DXGI formats by NiD3D10ShaderDeclaration's table (0x20E8FB8). Source types:
// 1..4 floats, 5..8 32-bit integers, 9..12 unsigned, 13..16 16-bit, 17..20 unsigned, 21..24 bytes,
// 25..28 unsigned bytes, 29..32 (colour floats). Each packing function takes a description and
// returns the bytes it accounted for.

#include <xmmintrin.h>

#include "gamebryo/NiD3D10C43.h"
#include "reversed.h"

namespace ni::c43 {
namespace {

constexpr uintptr_t kPackerVtable = 0x009A07F4;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;

GAME_VAR(uint8_t*, ms_pkPackingFunctions, 0x020E8F68);   // NiTPrimitiveArray<holder*>*

// a packing description (NiD3DShaderDeclarationPack.cpp's PackDesc)
struct PackDesc {
    uint32_t d3dType;         // +0x0
    uint32_t sourceType;      // +0x4 (read as 16 bits, but for 0x8B1F30's range test)
    uint32_t vertices;        // +0x8 (16 bits)
    re::Ptr32<const uint16_t> remap;    // +0xC (vertex indices, or null: in order)
    re::Ptr32<const uint8_t> source;    // +0x10 (null: a default)
    uint32_t sourceSize;      // +0x14
    uint32_t sourceStride;    // +0x18
    uint32_t elementSize;     // +0x1C
    uint32_t destStride;      // +0x20
    re::Ptr32<uint8_t> dest;            // +0x24
};
static_assert(offsetof(PackDesc, dest) == 0x24);
using PackFn = uint32_t(__cdecl*)(PackDesc* desc);

GAME_FN(int32_t, __cdecl, FastFloatToInt, 0x008B6090, float value);
GAME_FN(uint32_t, __cdecl, GetTypeSize, 0x008B4100, uint32_t type);   // NiD3D10ShaderDeclaration's
GAME_FN(const uint8_t*, __thiscall, GeometryData_GetTextureSet, 0x0077A000, void* data, uint32_t set);
GAME_FN(uint8_t, __thiscall, AdditionalData_GetDataStream, 0x007E0F10, void* self, uint32_t index, re::Ptr32<void>* data,
        uint32_t* type, uint32_t* count, uint32_t* unknown, uint32_t* size, uint32_t* stride);
GAME_FN(void, __thiscall, NiTArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
// this file's (called where modes apply)
GAME_FN(void*, __thiscall, PackerHolder_Construct, 0x008B2B80, void* self);
GAME_FN(void, __cdecl, Packer_SetPackingFunction, 0x008B3050, uint32_t d3dType, uint32_t sourceType);
GAME_FN(uint8_t, __cdecl, Packer_InitPackingFunctionTables, 0x008B3490);

inline uint16_t Vertices(const PackDesc* desc) {
    return uint16_t(desc->vertices);
}
inline uint16_t SourceType(const PackDesc* desc) {
    return uint16_t(desc->sourceType);
}

// every vertex's element filled the same way (no source)
template <typename Fill>
uint32_t PackEmpty(PackDesc* desc, Fill fill) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        fill(dest);
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}
inline uint32_t PackZeros(PackDesc* desc) {
    return PackEmpty(desc, [desc](uint8_t* dest) { crt::Memset(dest, 0, desc->elementSize); });
}

// every vertex converted from its source element (in order, or through the remap; the source
// pointer only moves in order)
template <typename Convert>
uint32_t PackVertices(PackDesc* desc, Convert convert) {
    const uint8_t* source = desc->source;
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const uint8_t* from = desc->remap != nullptr
                                  ? desc->source + uint32_t(desc->remap[v]) * desc->sourceStride
                                  : source;
        convert(dest, from);
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

inline float ReadFloat(const uint8_t* p) {
    float value;
    crt::Memcpy(&value, p, 4);
    return value;
}
inline uint16_t ReadU16(const uint8_t* p) {
    uint16_t value;
    crt::Memcpy(&value, p, 2);
    return value;
}
inline void WriteU32(uint8_t* p, uint32_t value) {
    crt::Memcpy(p, &value, 4);
}
inline void WriteU16(uint8_t* p, uint16_t value) {
    crt::Memcpy(p, &value, 2);
}
// a float times 255 (x87: the product of the float and the double 255.0, stored as a float: exact
// in double, so one rounding to float)
inline float Times255(float value) {
    return float(double(value) * 255.0);
}

// bytes (count) copied and the rest of a 4-byte element zeroed
inline void CopyBytesPadded(uint8_t* dest, const uint8_t* from, uint16_t count) {
    crt::Memcpy(dest, from, count);
    if (count < 4) {
        crt::Memset(dest + count, 0, uint16_t(4 - count));
    }
}

// the 16-bit high bytes of a 4-component element packed (0x8B19E0's and 0x8B1B40's expression:
// the alpha's high byte, sign or zero extended, to the top)
inline uint32_t HighBytes(const uint8_t* from, uint32_t alpha) {
    return ((uint32_t(ReadU16(from + 4)) & 0xFF00u) | ((alpha & 0xFFFFFF00u) << 8)) << 8 |
           (uint32_t(ReadU16(from + 2)) & 0xFF00u) | from[1];
}

} // namespace

// 0x8B1570 (thiscall): NiD3D10GeometryPacker::NiD3D10GeometryPacker
void* __fastcall GeometryPacker_Construct(void* self, re::Edx) {
    At<uint32_t>(self, 0) = kPackerVtable;
    return self;
}

// 0x8B1580: unsigned bytes (25..28) to FLOAT1..4: each byte / 255 (min(source components, the
// type's floats); the rest zero); zeros without a source. Also NiD3DShaderDeclaration's.
uint32_t __cdecl GeometryPacker_PackUInt8ToFloat(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t count = uint16_t(SourceType(desc) - 0x18);
    uint16_t out = uint16_t(uint16_t(desc->d3dType) + 1);
    if (out < count) {
        count = out;
    }
    return PackVertices(desc, [count, out](uint8_t* dest, const uint8_t* from) {
        float* to = reinterpret_cast<float*>(dest);
        uint16_t c = 0;
        for (; c < count; c++) {
            to[c] = float(double(int32_t(from[c])) / 255.0);
        }
        for (; c < out; c++) {
            to[c] = 0.0f;
        }
    });
}

// 0x8B1750: floats (3, 4; 29..32) to a colour: R8G8B8A8 from each * 255 rounded (alpha 1 for
// three components; the bytes ORed as integers, as the game does); white without a source
uint32_t __cdecl GeometryPacker_PackFloatToColor(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackEmpty(desc, [](uint8_t* dest) {
            uint32_t a = uint32_t(FastFloatToInt(255.0f));
            uint32_t b = uint32_t(FastFloatToInt(255.0f));
            uint32_t g = uint32_t(FastFloatToInt(255.0f));
            uint32_t r = uint32_t(FastFloatToInt(255.0f));
            WriteU32(dest, r | ((g | ((b | (a << 8)) << 8)) << 8));
        });
    }
    uint16_t type = SourceType(desc);
    return PackVertices(desc, [type](uint8_t* dest, const uint8_t* from) {
        float alpha = type == 3 ? 1.0f : ReadFloat(from + 0xC);
        uint32_t a = uint32_t(FastFloatToInt(Times255(alpha)));
        uint32_t b = uint32_t(FastFloatToInt(Times255(ReadFloat(from + 8))));
        uint32_t g = uint32_t(FastFloatToInt(Times255(ReadFloat(from + 4))));
        uint32_t r = uint32_t(FastFloatToInt(Times255(ReadFloat(from))));
        WriteU32(dest, r | ((g | ((b | (a << 8)) << 8)) << 8));
    });
}

// 0x8B19E0: 16-bit values (16) to a colour: each one's high byte (alpha 0xFF for type 15);
// 0xFFFFFFFF without a source
uint32_t __cdecl GeometryPacker_PackInt16ToColor(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackEmpty(desc, [](uint8_t* dest) { WriteU32(dest, 0xFFFFFFFFu); });
    }
    uint16_t type = SourceType(desc);
    return PackVertices(desc, [type](uint8_t* dest, const uint8_t* from) {
        int16_t alpha = type == 0xF ? int16_t(-1) : int16_t(ReadU16(from + 6));
        WriteU32(dest, HighBytes(from, uint32_t(int32_t(alpha))));
    });
}

// 0x8B1B40: unsigned 16-bit values (20) to a colour: each one's high byte (alpha 0xFF for type
// 19); 0xFFFFFFFF without a source
uint32_t __cdecl GeometryPacker_PackUInt16ToColor(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackEmpty(desc, [](uint8_t* dest) { WriteU32(dest, 0xFFFFFFFFu); });
    }
    uint16_t type = SourceType(desc);
    return PackVertices(desc, [type](uint8_t* dest, const uint8_t* from) {
        uint16_t alpha = type != 0x13 ? ReadU16(from + 6) : uint16_t(0xFFFF);
        WriteU32(dest, HighBytes(from, alpha));
    });
}

// 0x8B1CA0: bytes (21..24) to a colour: the four bytes (alpha 0xFF for type 23); 0xFF bytes
// without a source
uint32_t __cdecl GeometryPacker_PackInt8ToColor(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackEmpty(desc, [desc](uint8_t* dest) { crt::Memset(dest, 0xFF, desc->elementSize); });
    }
    uint16_t type = SourceType(desc);
    return PackVertices(desc, [type](uint8_t* dest, const uint8_t* from) {
        uint8_t alpha = type == 0x17 ? uint8_t(0xFF) : from[3];
        WriteU32(dest, uint32_t(from[0]) | uint32_t(from[1]) << 8 | uint32_t(from[2]) << 16 | uint32_t(alpha) << 24);
    });
}

// 0x8B1DF0: unsigned bytes (25..28) to a colour (alpha 0xFF for type 27)
uint32_t __cdecl GeometryPacker_PackUInt8ToColor(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackEmpty(desc, [desc](uint8_t* dest) { crt::Memset(dest, 0xFF, desc->elementSize); });
    }
    uint16_t type = SourceType(desc);
    return PackVertices(desc, [type](uint8_t* dest, const uint8_t* from) {
        uint8_t alpha = type == 0x1B ? uint8_t(0xFF) : from[3];
        WriteU32(dest, uint32_t(from[0]) | uint32_t(from[1]) << 8 | uint32_t(from[2]) << 16 | uint32_t(alpha) << 24);
    });
}

// 0x8B1F30: floats (1..4, 29..32) to UBYTE4N: each * 255 rounded (its low byte), the rest 255;
// zeros without a source (the source type's range tested as 32 bits)
uint32_t __cdecl GeometryPacker_PackFloatToUByteN(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t base = (desc->sourceType == 0 || desc->sourceType > 4) ? uint16_t(0x1D) : uint16_t(1);
    uint16_t count = uint16_t(uint16_t(SourceType(desc) - base) + 1);
    if (count > 4) {
        count = 4;
    }
    return PackVertices(desc, [count](uint8_t* dest, const uint8_t* from) {
        uint16_t c = 0;
        for (; c < count; c++) {
            dest[c] = uint8_t(FastFloatToInt(Times255(ReadFloat(from + c * 4))));
        }
        for (; c < 4; c++) {
            dest[c] = uint8_t(FastFloatToInt(255.0f));
        }
    });
}

// 0x8B2180: 16-bit values (13..16) to UBYTE4(N): each one's low byte, the rest zero. Also
// NiD3DShaderDeclaration's.
uint32_t __cdecl GeometryPacker_PackInt16ToUByte(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t count = uint16_t(SourceType(desc) - 0xC);
    if (count >= 5) {
        count = 4;
    }
    return PackVertices(desc, [count](uint8_t* dest, const uint8_t* from) {
        for (uint16_t c = 0; c < count; c++) {
            dest[c] = from[c * 2];
        }
        if (count < 4) {
            crt::Memset(dest + count, 0, uint16_t(4 - count));
        }
    });
}

// 0x8B2330: bytes (21..24) to UBYTE4N: copied, the rest zero
uint32_t __cdecl GeometryPacker_PackInt8ToUByteN(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t count = uint16_t(SourceType(desc) - 0x14);
    if (count >= 5) {
        count = 4;
    }
    return PackVertices(desc, [count](uint8_t* dest, const uint8_t* from) { CopyBytesPadded(dest, from, count); });
}

// 0x8B24A0: unsigned 16-bit values (17..20) to SHORT2/4: copied (as many as the type has), the
// rest zero. Also NiD3DShaderDeclaration's.
uint32_t __cdecl GeometryPacker_PackUInt16ToShort(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t out = uint16_t((desc->d3dType != 6 ? 1u : 0u) * 2 + 2);
    uint16_t count = uint16_t(SourceType(desc) - 0x10);
    if (out < count) {
        count = out;
    }
    return PackVertices(desc, [count, out](uint8_t* dest, const uint8_t* from) {
        uint16_t c = 0;
        for (; c < count; c++) {
            WriteU16(dest + c * 2, ReadU16(from + c * 2));
        }
        for (; c < out; c++) {
            WriteU16(dest + c * 2, 0);
        }
    });
}

// 0x8B2660: unsigned bytes (25..28) to SHORT2/4: zero extended, the rest zero. Also
// NiD3DShaderDeclaration's.
uint32_t __cdecl GeometryPacker_PackUInt8ToShort(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t out = uint16_t((desc->d3dType != 6 ? 1u : 0u) * 2 + 2);
    uint16_t count = uint16_t(SourceType(desc) - 0x18);
    if (out < count) {
        count = out;
    }
    return PackVertices(desc, [count, out](uint8_t* dest, const uint8_t* from) {
        uint16_t c = 0;
        for (; c < count; c++) {
            WriteU16(dest + c * 2, from[c]);
        }
        for (; c < out; c++) {
            WriteU16(dest + c * 2, 0);
        }
    });
}

// 0x8B2830: bytes (21..24) to UBYTE4: copied, the rest zero. Also NiD3DShaderDeclaration's.
uint32_t __cdecl GeometryPacker_PackInt8ToUByte(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t count = uint16_t(SourceType(desc) - 0x14);
    if (count >= 5) {
        count = 4;
    }
    return PackVertices(desc, [count](uint8_t* dest, const uint8_t* from) {
        for (uint16_t c = 0; c < count; c++) {
            dest[c] = from[c];
        }
        if (count < 4) {
            crt::Memset(dest + count, 0, uint16_t(4 - count));
        }
    });
}

// 0x8B29E0: unsigned bytes (25..28) to UBYTE4(N): copied, the rest zero. Also
// NiD3DShaderDeclaration's.
uint32_t __cdecl GeometryPacker_PackUInt8ToUByte(PackDesc* desc) {
    if (desc->source == nullptr) {
        return PackZeros(desc);
    }
    uint16_t count = uint16_t(SourceType(desc) - 0x18);
    if (count >= 5) {
        count = 4;
    }
    return PackVertices(desc, [count](uint8_t* dest, const uint8_t* from) { CopyBytesPadded(dest, from, count); });
}

// 0x8B2B50 (thiscall): NiD3D10GeometryPacker's deleting destructor
void* __fastcall GeometryPacker_DeletingDestructor(void* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kPackerVtable;
    if (flags & 1) {
        NiDelete(self, 4);
    }
    return self;
}

// 0x8B2B80 (thiscall): a packing functions holder made (type 0x11, an empty NiTPrimitiveArray
// growing by 1). Also NiD3DShaderDeclaration's.
uint8_t* __fastcall PackerHolder_ConstructImpl(uint8_t* self, re::Edx) {
    At<uint16_t>(self, 0xC) = 0;
    At<uint16_t>(self, 0x12) = 1;
    At<uint16_t>(self, 0xE) = 0;
    At<uint16_t>(self, 0x10) = 0;
    At<re::Ptr32<void>*>(self, 8) = nullptr;
    At<uint32_t>(self, 4) = kPrimitiveArrayVtable;
    At<uint32_t>(self, 0) = 0x11;
    for (uint16_t i = 0; i < At<uint16_t>(self, 0xE); i++) {   // (RemoveAll inlined: empty here)
        At<re::Ptr32<void>*>(self, 8)[i] = nullptr;
    }
    At<uint16_t>(self, 0xE) = 0;
    At<uint16_t>(self, 0x10) = 0;
    return self;
}

// 0x8B2BD0 (thiscall, `this` unused; 8 arguments): NiD3D10GeometryPacker::PackEntry: an entry of
// a shader declaration packed for every skin partition (or once without a skin instance): its
// source by its parameter (position, blend weights and bone indices from the partition by its
// bones per vertex, normal, colour, texture coordinate sets, tangents and binormals after the
// normals, an additional data stream), or the override source given (its stride; the element size
// as its size), the packing function for its D3D type (D3DCOLOR as UBYTE4 for bone indices,
// UBYTE4N otherwise) and source type; the vertices remapped through the partition's vertex map
// (not for weights and indices) and counted by the partition; the destination moved on by each
// partition's vertices. 1 when all were packed, 0 when one couldn't be.
//
// As the game: the source type, size and stride live across partitions (a parameter past the
// known ones that isn't a data stream keeps the last ones; the first time, the source type is
// the stride argument's stack slot and the rest uninitialised: here the stride and zeros), and
// a data stream's count is written over the vertex count argument's slot (unread after).
uint32_t __fastcall GeometryPacker_PackEntry(void*, re::Edx, uint32_t vertices, uint8_t* data, uint8_t* skin,
                                             const uint8_t* entry, uint32_t stride, uint8_t* dest,
                                             uint32_t overrideStride, const uint8_t* overrideSource) {
    uint32_t d3dType = At<uint32_t>(entry, 8);
    if (d3dType == 0x11) {
        return 0;
    }
    if (d3dType == 4) {
        d3dType = (At<uint32_t>(entry, 4) != 2 ? 3u : 0u) + 5;
    }
    PackDesc desc;
    desc.vertices = vertices;   // (its low 16 bits are what's read)
    desc.destStride = stride;
    desc.dest = dest;
    uint32_t elementSize = GetTypeSize(d3dType);
    desc.elementSize = elementSize;
    uint8_t* holder = At<re::Ptr32<uint8_t>*>(ms_pkPackingFunctions, 4)[d3dType];
    if (holder == nullptr) {
        return 0;
    }
    void* additional = At<void*>(data, 0x38);
    uint8_t* skinPartition = skin != nullptr ? At<uint8_t*>(skin, 0xC) : nullptr;
    uint8_t* partition = skinPartition != nullptr ? At<uint8_t*>(skinPartition, 0xC) : nullptr;
    uint32_t partitions = skinPartition != nullptr ? At<uint32_t>(skinPartition, 8) : 1;
    uint32_t sourceType = stride;
    const uint8_t* source = nullptr;
    uint32_t sourceSize = 0;
    uint32_t sourceStride = 0;
    uint32_t streamCount = vertices;   // (the vertex count argument's slot)
    uint32_t streamUnknown = 0;        // (a stack local)
    for (uint32_t i = 0; i < partitions; i++) {
        uint32_t parameter = At<uint32_t>(entry, 4);
        switch (parameter) {
        case 0:   // position
            source = At<uint8_t*>(data, 0x20);
            sourceSize = 0xC;
            sourceStride = 0xC;
            sourceType = 3;
            break;
        case 1:   // blend weights
            if (partition == nullptr) {
                return 0;
            }
            switch (At<uint16_t>(partition, 0x24)) {
            case 1:
                sourceType = 1;
                sourceSize = sourceStride = 4;
                break;
            case 2:
                sourceType = 2;
                sourceSize = sourceStride = 8;
                break;
            case 3:
                sourceSize = sourceStride = 0xC;
                sourceType = 3;
                break;
            case 4:
                sourceStride = 0x10;
                sourceSize = 0xC;
                sourceType = 3;
                break;
            default:
                break;
            }
            source = At<uint8_t*>(partition, 8);
            break;
        case 2:   // bone indices
            if (partition == nullptr || At<uint8_t*>(partition, 0x10) == nullptr) {
                return 0;
            }
            switch (At<uint16_t>(partition, 0x24)) {
            case 1:
                sourceType = 0x19;
                sourceSize = sourceStride = 1;
                break;
            case 2:
                sourceType = 0x1A;
                sourceSize = sourceStride = 2;
                break;
            case 3:
                sourceType = 0x1B;
                sourceSize = sourceStride = 3;
                break;
            case 4:
                sourceType = 0x1C;
                sourceSize = sourceStride = 4;
                break;
            default:
                break;
            }
            source = At<uint8_t*>(partition, 0x10);
            break;
        case 3:   // normal
            source = At<uint8_t*>(data, 0x24);
            sourceSize = 0xC;
            sourceStride = 0xC;
            sourceType = 3;
            break;
        case 4:   // colour
            source = At<uint8_t*>(data, 0x28);
            sourceSize = 0x10;
            sourceStride = 0x10;
            sourceType = 4;
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:   // texture coordinate sets
            source = GeometryData_GetTextureSet(data, uint16_t(parameter - 5));
            sourceSize = 8;
            sourceStride = 8;
            sourceType = 2;
            break;
        case 13:   // tangents (after the normals and binormals)
        case 14:   // binormals (after the normals)
            if (At<uint8_t*>(data, 0x24) == nullptr || (At<uint16_t>(data, 0x32) & 0xF000) == 0) {
                source = nullptr;
            } else {
                source = At<uint8_t*>(data, 0x24) + uint32_t(At<uint16_t>(data, 0xC)) * (parameter == 13 ? 0x18u : 0xCu);
            }
            sourceSize = 0xC;
            sourceStride = 0xC;
            sourceType = 3;
            break;
        case 0xFFFFFFFF:
            return 0;
        default:
            if (int32_t(parameter) < 0) {
                re::Ptr32<void> stream = skin;   // (the skin instance argument's slot takes the stream)
                if (AdditionalData_GetDataStream(additional, parameter & 0x7FFFFFFF, &stream, &sourceType,
                                                 &streamCount, &streamUnknown, &sourceSize, &sourceStride) != 0) {
                    source = static_cast<const uint8_t*>(stream);
                } else {
                    sourceSize = 0;
                    sourceStride = 0;
                    sourceType = 0;
                    source = nullptr;
                }
                skin = static_cast<uint8_t*>(stream);
            }
            break;
        }
        if (overrideSource != nullptr) {
            source = overrideSource;
            sourceSize = elementSize;
            sourceStride = overrideStride;
        }
        PackFn pack = re::GameFnAt<PackFn>(&At<re::Ptr32<void>*>(holder, 8)[sourceType]);
        if (pack == nullptr) {
            return 0;
        }
        uint32_t parameterNow = At<uint32_t>(entry, 4);
        if (partition == nullptr || parameterNow == 1 || parameterNow == 2) {
            desc.remap = nullptr;
        } else {
            desc.remap = At<const uint16_t*>(partition, 0xC);
        }
        if (partition != nullptr) {
            desc.vertices = (desc.vertices & 0xFFFF0000u) | At<uint16_t>(partition, 0x1C);
        }
        desc.sourceType = sourceType;
        desc.d3dType = d3dType;
        desc.source = source;
        desc.sourceSize = sourceSize;
        desc.sourceStride = sourceStride;
        if (pack(&desc) == 0) {
            return 0;
        }
        if (partition != nullptr) {
            partition += 0x2C;
        }
        desc.dest += uint32_t(uint16_t(desc.vertices)) * desc.destStride;
    }
    return 1;
}

// 0x8B3050 (cdecl): NiD3D10GeometryPacker::SetPackingFunction: the function for a D3D type and a
// source type put in the type's holder (source type 0: one that packs nothing, for every type);
// types past 8 get none
void __cdecl GeometryPacker_SetPackingFunction(uint32_t d3dType, uint32_t sourceType) {
    uint8_t* holder = At<re::Ptr32<uint8_t>*>(ms_pkPackingFunctions, 4)[d3dType];
    uintptr_t fn = 0;
    constexpr uintptr_t kNothing = 0x00824EC0;   // returns 0
    if (sourceType == 0) {
        fn = kNothing;
        NiTArray_SetAt(holder + 4, 0, re::PtrCast<const re::Ptr32<void>*>(&fn));
        return;
    }
    if (sourceType - 1 > 0x1F) {
        return;
    }
    uint32_t group = (sourceType - 1) / 4;   // 0 floats .. 7 colour floats
    if (d3dType <= 3) {
        static const uintptr_t kFloat[8] = {0, 0x008A0210, 0x008A0420, 0x008A05F0, 0x008A0820, 0x008A09F0,
                                            0x008B1580, 0x008A1430};
        if (group == 0) {
            if (sourceType - 1 == d3dType) {
                fn = 0x008A1580;   // copy
            } else if (sourceType - 1 < d3dType) {
                fn = 0x008A1430;   // extend
            } else {
                fn = 0x008A1350;   // truncate
            }
        } else {
            fn = kFloat[group];
        }
    } else if (d3dType == 4) {
        switch (sourceType) {
        case 3:
        case 4:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x20:
            fn = 0x008B1750;
            break;
        case 5:
        case 9:
            fn = 0x008A0C20;
            break;
        case 0x10:
            fn = 0x008B19E0;
            break;
        case 0x14:
            fn = 0x008B1B40;
            break;
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
            fn = 0x008B1CA0;
            break;
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
            fn = 0x008B1DF0;
            break;
        default:
            fn = kNothing;
            break;
        }
    } else if (d3dType == 8) {
        static const uintptr_t kUByteN[8] = {0x008B1F30, 0x00885930, 0x00885AE0, 0x008B2180,
                                             0x00885C90, 0x008B2330, 0x008B29E0, 0x008B1F30};
        fn = kUByteN[group];
    } else if (d3dType == 6 || d3dType == 7) {
        static const uintptr_t kShort[8] = {0x00884E70, 0x00885050, 0x00885210, 0x008853D0,
                                            0x008B24A0, 0x00885570, 0x008B2660, kNothing};
        fn = kShort[group];
    } else if (d3dType == 5) {
        static const uintptr_t kUByte[8] = {0x00885740, 0x00885930, 0x00885AE0, 0x008B2180,
                                            0x00885C90, 0x008B2830, 0x008B29E0, kNothing};
        fn = kUByte[group];
    } else {
        return;
    }
    if (fn != 0) {
        NiTArray_SetAt(holder + 4, sourceType, re::PtrCast<const re::Ptr32<void>*>(&fn));
    }
}

// 0x8B33D0 (cdecl): NiD3D10GeometryPacker's packing function tables deleted (each holder's array
// emptied and freed, the holder deleted; the array emptied and deleted, the pointer left)
void __cdecl GeometryPacker_ShutdownPackingFunctions() {
    uint8_t* array = ms_pkPackingFunctions;
    for (uint32_t i = 0; i < At<uint16_t>(array, 0xA); i++) {
        uint8_t* holder = At<re::Ptr32<uint8_t>*>(array, 4)[i];
        if (holder != nullptr) {
            for (uint16_t j = 0; j < At<uint16_t>(holder, 0xE); j++) {
                At<re::Ptr32<void>*>(holder, 8)[j] = nullptr;
            }
            At<uint16_t>(holder, 0xE) = 0;
            At<uint16_t>(holder, 0x10) = 0;
            At<uint32_t>(holder, 4) = kPrimitiveArrayVtable;
            NiFree(At<void*>(holder, 8));
            NiDelete(holder, 0x14);
            array = ms_pkPackingFunctions;
        }
    }
    for (uint16_t i = 0; i < At<uint16_t>(array, 0xA); i++) {
        At<re::Ptr32<void>*>(array, 4)[i] = nullptr;
    }
    At<uint16_t>(array, 0xC) = 0;
    At<uint16_t>(array, 0xA) = 0;
    if (ms_pkPackingFunctions != nullptr) {
        Virtual<void>(ms_pkPackingFunctions, 0, 1u);
    }
}

// 0x8B3490 (cdecl): NiD3D10GeometryPacker's tables made, once (a holder per D3D type with 38
// slots, then every function set); true
uint8_t __cdecl GeometryPacker_InitPackingFunctionTables() {
    if (At<uint16_t>(ms_pkPackingFunctions, 0xA) == 0x12) {
        return 1;
    }
    for (uint32_t i = 0; i < 0x12; i++) {
        void* memory = NiNew(0x14);
        uint8_t* holder = memory != nullptr ? static_cast<uint8_t*>(PackerHolder_Construct(memory)) : nullptr;
        At<uint32_t>(holder, 0) = i;
        NiTArray_SetSize(holder + 4, 0x26);
        uint8_t* array = ms_pkPackingFunctions;
        if (i >= At<uint16_t>(array, 8)) {
            NiTArray_SetSize(array, At<uint16_t>(array, 0xE) + i);
        }
        NiTArray_SetAt(array, i, re::PtrCast<const re::Ptr32<void>*>(&holder));
    }
    for (uint32_t d3dType = 0; d3dType < 0x12; d3dType++) {
        for (uint32_t sourceType = 0; sourceType < 0x21; sourceType++) {
            Packer_SetPackingFunction(d3dType, sourceType);
        }
    }
    return 1;
}

// 0x8B3530 (cdecl): NiD3D10GeometryPacker::_SDMInit's part: the tables' array made (an
// NiTPrimitiveArray growing by 1) and filled
uint8_t __cdecl GeometryPacker_InitPackingFunctions() {
    uint8_t* array = static_cast<uint8_t*>(NiNew(0x10));
    if (array != nullptr) {
        At<uint16_t>(array, 8) = 0;
        At<uint16_t>(array, 0xE) = 1;
        At<uint16_t>(array, 0xA) = 0;
        At<uint16_t>(array, 0xC) = 0;
        At<void*>(array, 4) = nullptr;
        At<uint32_t>(array, 0) = kPrimitiveArrayVtable;
    }
    ms_pkPackingFunctions = array;
    return Packer_InitPackingFunctionTables();   // (a tail jump)
}

// 0x8B6090 (cdecl): a float to an integer by fistp (the current rounding: to nearest)
int32_t __cdecl GeometryPacker_FastFloatToInt(float value) {
    return _mm_cvtss_si32(_mm_set_ss(value));
}

#define GB43B_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB43B_REVERSED("NiD3D10GeometryPacker::NiD3D10GeometryPacker", 0x008B1570, GeometryPacker_Construct);
GB43B_REVERSED("NiD3D10GeometryPacker::PackUInt8ToFloat", 0x008B1580, GeometryPacker_PackUInt8ToFloat);
GB43B_REVERSED("NiD3D10GeometryPacker::PackFloatToColor", 0x008B1750, GeometryPacker_PackFloatToColor);
GB43B_REVERSED("NiD3D10GeometryPacker::PackInt16ToColor", 0x008B19E0, GeometryPacker_PackInt16ToColor);
GB43B_REVERSED("NiD3D10GeometryPacker::PackUInt16ToColor", 0x008B1B40, GeometryPacker_PackUInt16ToColor);
GB43B_REVERSED("NiD3D10GeometryPacker::PackInt8ToColor", 0x008B1CA0, GeometryPacker_PackInt8ToColor);
GB43B_REVERSED("NiD3D10GeometryPacker::PackUInt8ToColor", 0x008B1DF0, GeometryPacker_PackUInt8ToColor);
GB43B_REVERSED("NiD3D10GeometryPacker::PackFloatToUByteN", 0x008B1F30, GeometryPacker_PackFloatToUByteN);
GB43B_REVERSED("NiD3D10GeometryPacker::PackInt16ToUByte", 0x008B2180, GeometryPacker_PackInt16ToUByte);
GB43B_REVERSED("NiD3D10GeometryPacker::PackInt8ToUByteN", 0x008B2330, GeometryPacker_PackInt8ToUByteN);
GB43B_REVERSED("NiD3D10GeometryPacker::PackUInt16ToShort", 0x008B24A0, GeometryPacker_PackUInt16ToShort);
GB43B_REVERSED("NiD3D10GeometryPacker::PackUInt8ToShort", 0x008B2660, GeometryPacker_PackUInt8ToShort);
GB43B_REVERSED("NiD3D10GeometryPacker::PackInt8ToUByte", 0x008B2830, GeometryPacker_PackInt8ToUByte);
GB43B_REVERSED("NiD3D10GeometryPacker::PackUInt8ToUByte", 0x008B29E0, GeometryPacker_PackUInt8ToUByte);
GB43B_REVERSED("NiD3D10GeometryPacker::DeletingDestructor", 0x008B2B50, GeometryPacker_DeletingDestructor);
GB43B_REVERSED("NiD3D10GeometryPacker::PackFunctionHolder::PackFunctionHolder", 0x008B2B80,
               PackerHolder_ConstructImpl);
GB43B_REVERSED("NiD3D10GeometryPacker::PackEntry", 0x008B2BD0, GeometryPacker_PackEntry);
GB43B_REVERSED("NiD3D10GeometryPacker::SetPackingFunction", 0x008B3050, GeometryPacker_SetPackingFunction);
GB43B_REVERSED("NiD3D10GeometryPacker::ShutdownPackingFunctions", 0x008B33D0,
               GeometryPacker_ShutdownPackingFunctions);
GB43B_REVERSED("NiD3D10GeometryPacker::InitPackingFunctionTables", 0x008B3490,
               GeometryPacker_InitPackingFunctionTables);
GB43B_REVERSED("NiD3D10GeometryPacker::InitPackingFunctions", 0x008B3530, GeometryPacker_InitPackingFunctions);
GB43B_REVERSED("NiD3D10GeometryPacker::FastFloatToInt", 0x008B6090, GeometryPacker_FastFloatToInt);

} // namespace ni::c43
