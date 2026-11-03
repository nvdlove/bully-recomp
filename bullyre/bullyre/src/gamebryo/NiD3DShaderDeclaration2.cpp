// The D3D renderers' shader declaration base (vtable 0x99DBF0, 0x2C bytes, NiShaderDeclaration's
// RTTI; NiD3DShaderDeclaration.cpp's class, vtable 0x99BEB8, derives from it: there it's called
// NiD3DShaderDeclaration, so this one keeps that name with its own method names): +0x18 the
// renderer, +0x1C its vertex buffer manager, +0x20 its device (held), +0x24 / +0x28 zero from the
// constructor (the derived class's element capacity and count). Its Initialize makes the streams
// (0x10 each) and their entries (NiShaderDeclaration::ShaderRegisterEntry, 0x18 each).
//
// Also the packing functions the entries pack geometry data with (chosen by SetPackingFunction
// 0x8862F0 by D3D type and source type, kept in an NiTPrimitiveArray at 0x20E7480 of one 0x14-byte
// object per D3D type: the type, then an NiTPrimitiveArray of the functions by source type), the
// D3D types' sizes (0x20E7438), and PackEntry, which fills a packing description for an entry from
// the geometry data (or the skin partition's weights and bone indices, or additional geometry
// data's streams) and calls the function. Source types: 1..4 floats, 5..8 32-bit integers, 9..12
// unsigned, 13..16 16-bit, 17..20 unsigned, 21..24 bytes, 25..28 unsigned, 29..32 (a float copy).

#include <cstddef>
#include <cstdint>
#include <xmmintrin.h>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c41 {
using namespace ni::dx9;
namespace {

constexpr uintptr_t kDeclarationBaseVtable = 0x0099DBF0;
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;
constexpr uintptr_t kStreamConstructor = 0x008A1200;
constexpr uint32_t kDeclarationBaseSize = 0x2C;

GAME_ARRAY(uint32_t, ms_auiTypeSizes, 0x020E7438);   // bytes by D3D type (0x11)
GAME_VAR(uint8_t*, ms_pkPackingFunctions, 0x020E7480);

GAME_FN(void*, __thiscall, ShaderDeclaration_ConstructBase, 0x0077C420, void* self);
GAME_FN(void, __thiscall, ShaderDeclaration_DestructBase, 0x0077D6E0, void* self);
GAME_FN(void, __thiscall, ShaderRegisterEntry_Construct, 0x0077C5F0, void* self);
GAME_FN(void*, __cdecl, NiNewArray, 0x007524D0, uint32_t size);
GAME_FN(void, __stdcall, VectorConstructor, 0x00401240, void* p, uint32_t size, uint32_t count, uintptr_t ctor);
GAME_FN(const void*, __thiscall, GeometryData_GetTextureSet, 0x0077A000, void* data, uint32_t set);
GAME_FN(uint8_t, __thiscall, AdditionalData_GetDataStream, 0x007E0F10, void* self, uint32_t index, re::Ptr32<void>* data,
        uint32_t* type, uint32_t* count, uint32_t* unknown, uint32_t* size, uint32_t* stride);
GAME_FN(void, __thiscall, PackerHolder_Construct, 0x008B2B80, void* self);
GAME_FN(void, __thiscall, NiTArray_SetSize, 0x0084A110, void* array, uint32_t size);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* array, uint32_t index, const re::Ptr32<void>* value);
GAME_FN(void, __cdecl, Declaration_SetPackingFunction, 0x008862F0, uint32_t d3dType, uint32_t sourceType);

// this file's (called where modes apply)
GAME_FN(void, __thiscall, DeclarationBase_Destruct, 0x008A1220, void* self);
GAME_FN(uint8_t, __cdecl, Declaration_InitPackingFunctionTables, 0x008A1D90);

// a packing description (as NiD3DShaderDeclarationPack.cpp's PackDesc)
struct PackDesc {
    uint32_t d3dType;         // +0x0 (read as 16 bits)
    uint32_t sourceType;      // +0x4 (read as 16 bits)
    uint32_t vertices;        // +0x8 (16 bits)
    re::Ptr32<const uint16_t> remap;    // +0xC (vertex indices, or null: in order)
    re::Ptr32<const uint8_t> source;    // +0x10 (null: a default)
    uint32_t sourceSize;      // +0x14 (the bytes a source element has)
    uint32_t sourceStride;    // +0x18
    uint32_t elementSize;     // +0x1C
    uint32_t destStride;      // +0x20
    re::Ptr32<uint8_t> dest;            // +0x24
};
static_assert(offsetof(PackDesc, dest) == 0x24);
using PackFn = uint32_t(__cdecl*)(PackDesc* desc);

inline uint16_t Vertices(const PackDesc* desc) {
    return uint16_t(desc->vertices);
}
inline const uint8_t* RemappedSource(const PackDesc* desc, uint16_t v) {
    return desc->source + desc->remap[v] * desc->sourceStride;
}

// the source components to floats (min(source components, the D3D type's), the rest zero);
// zeros without a source
template <typename Src, float (*Convert)(Src), uint16_t Base>
uint32_t PackToFloat(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            crt::Memset(dest, 0, desc->elementSize);
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    uint16_t components = uint16_t(uint16_t(desc->sourceType) - Base);
    uint16_t out = uint16_t(uint16_t(desc->d3dType) + 1);
    if (out < components) {
        components = out;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const Src* from = reinterpret_cast<const Src*>(desc->remap != nullptr ? RemappedSource(desc, v) : source);
        float* to = reinterpret_cast<float*>(dest);
        uint16_t c = 0;
        for (; c < components; c++) {
            to[c] = Convert(from[c]);
        }
        for (; c < out; c++) {
            to[c] = 0.0f;
        }
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// signed values scaled into -1..1 (clamped below at -1), unsigned into 0..1; the scale's product
// or quotient rounded to float once, as at the main thread's precision
inline float FromInt32(int32_t value) {
    float f = float(value) * 4.6566128730773926e-10f;   // 2^-31
    return f < -1.0f ? -1.0f : f;
}
inline float FromUInt32(uint32_t value) {
    return float(double(value)) * 2.3283064365386963e-10f;   // 2^-32
}
inline float FromInt16(int16_t value) {
    float f = float(int32_t(value)) / 32767.0f;
    return f < -1.0f ? -1.0f : f;
}
inline float FromUInt16(uint16_t value) {
    return float(uint32_t(value)) / 65535.0f;
}
inline float FromInt8(int8_t value) {
    float f = float(int32_t(value)) / 127.0f;
    return f < -1.0f ? -1.0f : f;
}

// four 16-bit channels (R, G, B, A) to D3DCOLOR by their high bytes; A opaque for the 3-channel
// type
template <uint16_t ThreeChannels>
uint32_t PackShortColor(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            *reinterpret_cast<uint32_t*>(dest) = 0xFFFFFFFF;
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const uint16_t* from = reinterpret_cast<const uint16_t*>(
            desc->remap != nullptr ? RemappedSource(desc, v) : source);
        uint32_t alpha = uint16_t(desc->sourceType) == ThreeChannels ? 0xFFFFu : from[3];
        *reinterpret_cast<uint32_t*>(dest) = ((alpha & 0xFF00) << 8 | (from[0] & 0xFF00)) << 8 | (from[1] & 0xFF00) |
                                             (from[2] >> 8);
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// four byte channels (R, G, B, A) to D3DCOLOR; A opaque for the 3-channel type; 0xFF bytes without
// a source
template <uint16_t ThreeChannels>
uint32_t PackByteColor(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            crt::Memset(dest, 0xFF, desc->elementSize);
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const uint8_t* from = desc->remap != nullptr ? RemappedSource(desc, v) : source;
        uint32_t alpha = uint16_t(desc->sourceType) == ThreeChannels ? 0xFFu : from[3];
        *reinterpret_cast<uint32_t*>(dest) =
            alpha << 24 | uint32_t(from[0]) << 16 | uint32_t(from[1]) << 8 | from[2];
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// a float to an integer with fistp in the default rounding mode
inline int32_t RoundToInt(float value) {
    return _mm_cvtss_si32(_mm_set_ss(value));
}

} // namespace

// 0x8A01D0 (thiscall): the declaration base's constructor
uint8_t* __fastcall DeclarationBase_Construct(uint8_t* self, re::Edx) {
    ShaderDeclaration_ConstructBase(self);
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    At<uint32_t>(self, 0x20) = 0;
    At<uint32_t>(self, 0x24) = 0;
    At<uint32_t>(self, 0x28) = 0;
    At<uint32_t>(self, 0) = kDeclarationBaseVtable;
    return self;
}

// 0x8A0200 (thiscall, slot 22, three arguments): true
uint8_t __fastcall DeclarationBase_ReturnTrue(void*, re::Edx, uint32_t, uint32_t, uint32_t) {
    return 1;
}

// 0x8A0210, 0x8A0420, 0x8A05F0, 0x8A0820, 0x8A09F0: 32-bit, unsigned 32-bit, 16-bit, unsigned
// 16-bit and 8-bit integers to FLOAT1..4 (normalized)
uint32_t __cdecl Pack_Int32ToFloat(PackDesc* desc) {
    return PackToFloat<int32_t, FromInt32, 4>(desc);
}
uint32_t __cdecl Pack_UInt32ToFloat(PackDesc* desc) {
    return PackToFloat<uint32_t, FromUInt32, 8>(desc);
}
uint32_t __cdecl Pack_Int16ToFloat(PackDesc* desc) {
    return PackToFloat<int16_t, FromInt16, 12>(desc);
}
uint32_t __cdecl Pack_UInt16ToFloat(PackDesc* desc) {
    return PackToFloat<uint16_t, FromUInt16, 16>(desc);
}
uint32_t __cdecl Pack_Int8ToFloat(PackDesc* desc) {
    return PackToFloat<int8_t, FromInt8, 20>(desc);
}

// 0x8A0C20: a 32-bit colour copied as D3DCOLOR (white without a source)
uint32_t __cdecl Pack_CopyColor(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            *reinterpret_cast<uint32_t*>(dest) = 0xFFFFFFFF;
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const uint8_t* from = desc->remap != nullptr ? RemappedSource(desc, v) : source;
        *reinterpret_cast<uint32_t*>(dest) = *reinterpret_cast<const uint32_t*>(from);
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// 0x8A0CC0 / 0x8A0E10: 16-bit / unsigned 16-bit channels to D3DCOLOR
uint32_t __cdecl Pack_Int16ToColor(PackDesc* desc) {
    return PackShortColor<0xF>(desc);
}
uint32_t __cdecl Pack_UInt16ToColor(PackDesc* desc) {
    return PackShortColor<0x13>(desc);
}

// 0x8A0F70 / 0x8A10C0: 8-bit / unsigned 8-bit channels to D3DCOLOR
uint32_t __cdecl Pack_Int8ToColor(PackDesc* desc) {
    return PackByteColor<0x17>(desc);
}
uint32_t __cdecl Pack_UInt8ToColor(PackDesc* desc) {
    return PackByteColor<0x1B>(desc);
}

// 0x8A1200 (thiscall): a declaration stream's constructor
void __fastcall DeclarationStream_Construct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint8_t>(self, 0xC) = 0;
    At<uint8_t>(self, 0xD) = 0;
}

// 0x8A1220 (thiscall): the declaration base's destructor (the device let go)
void __fastcall DeclarationBase_DestructImpl(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kDeclarationBaseVtable;
    At<uint32_t>(self, 0x18) = 0;
    Com<long>(At<void*>(self, 0x20), kRelease);
    At<void*>(self, 0x20) = nullptr;
    ShaderDeclaration_DestructBase(self);
}

// the bytes of `count` objects of `size` (0xFFFFFFFF on overflow), NiNew[]'s count before them
// when `header`
inline uint32_t CheckedSize(uint32_t count, uint32_t size, bool header) {
    uint64_t bytes = uint64_t(count) * size;
    uint32_t total = (bytes >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(bytes);
    if (header) {
        total = total > 0xFFFFFFFBu ? 0xFFFFFFFFu : total + 4;
    }
    return total;
}

// 0x8A1250 (thiscall): the declaration's renderer, vertex buffer manager and device (held), its
// streams and their entries made; true
uint8_t __fastcall DeclarationBase_Initialize(uint8_t* self, re::Edx, uint8_t* renderer, uint32_t entries,
                                              uint32_t streams) {
    At<uint8_t*>(self, 0x18) = renderer;
    At<void*>(self, 0x1C) = At<void*>(renderer, kVBManager);
    void* device = At<void*>(renderer, kDevice);
    At<void*>(self, 0x20) = device;
    Com<long>(device, kAddRef);
    At<uint32_t>(self, 8) = entries;
    At<uint32_t>(self, 0xC) = streams;
    void* streamArray = NiNewArray(CheckedSize(streams, 0x10, false));
    if (streamArray != nullptr) {
        VectorConstructor(streamArray, 0x10, streams, kStreamConstructor);
    } else {
        streamArray = nullptr;
    }
    At<void*>(self, 0x10) = streamArray;
    for (uint32_t s = 0; s < At<uint32_t>(self, 0xC); s++) {
        uint32_t count = At<uint32_t>(self, 8);
        uint32_t* memory = static_cast<uint32_t*>(NiNewArray(CheckedSize(count, 0x18, true)));
        uint8_t* entryArray = nullptr;
        if (memory != nullptr) {
            *memory = count;
            entryArray = reinterpret_cast<uint8_t*>(memory + 1);
            uint8_t* entry = entryArray;
            for (int32_t i = int32_t(count) - 1; i >= 0; i--) {
                ShaderRegisterEntry_Construct(entry);
                entry += 0x18;
            }
        }
        At<uint8_t*>(At<uint8_t*>(self, 0x10), int(s) * 0x10 + 4) = entryArray;
    }
    return 1;
}

// 0x8A1350: floats with more components than the D3D type: the element's bytes copied
uint32_t __cdecl Pack_FloatTruncate(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            crt::Memset(dest, 0, desc->elementSize);
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const uint8_t* from = desc->remap != nullptr ? RemappedSource(desc, v) : source;
        crt::MemcpyS(dest, desc->elementSize, from, desc->elementSize);
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// 0x8A1430: floats with fewer components than the D3D type: the source's bytes copied, the rest
// zeroed (the D3D type less the source type plus one, in 16 bits: a huge count for the types past
// the floats)
uint32_t __cdecl Pack_FloatExtend(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint16_t zeros = uint16_t(uint16_t(desc->d3dType) - uint16_t(desc->sourceType) + 1);
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            crt::Memset(dest, 0, desc->elementSize);
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const uint8_t* from = desc->remap != nullptr ? RemappedSource(desc, v) : source;
        crt::MemcpyS(dest, desc->sourceSize, from, desc->sourceSize);
        uint32_t* rest = reinterpret_cast<uint32_t*>(dest + desc->sourceSize);
        for (uint32_t i = zeros; i != 0; i--) {
            *rest++ = 0;
        }
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// 0x8A1580: floats as many as the D3D type: copied
uint32_t __cdecl Pack_FloatCopy(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            crt::Memset(dest, 0, desc->elementSize);
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const uint8_t* from = desc->remap != nullptr ? RemappedSource(desc, v) : source;
        crt::MemcpyS(dest, desc->sourceSize, from, desc->sourceSize);
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// 0x8A1670: float channels (R, G, B, A; A 1.0 for 3) to D3DCOLOR: each times 255.0 (a float at
// the main thread's precision), rounded, not masked
uint32_t __cdecl Pack_FloatToColor(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    uint32_t total = 0;
    if (desc->source == nullptr) {
        for (uint16_t v = 0; v < Vertices(desc); v++) {
            *reinterpret_cast<uint32_t*>(dest) = 0xFFFFFFFF;
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    const uint8_t* source = desc->source;
    for (uint16_t v = 0; v < Vertices(desc); v++) {
        const float* from = reinterpret_cast<const float*>(
            desc->remap != nullptr ? RemappedSource(desc, v) : source);
        float alpha = uint16_t(desc->sourceType) == 3 ? 1.0f : from[3];
        int32_t a = RoundToInt(float(double(alpha) * 255.0));
        int32_t r = RoundToInt(float(double(from[0]) * 255.0));
        int32_t g = RoundToInt(float(double(from[1]) * 255.0));
        int32_t b = RoundToInt(float(double(from[2]) * 255.0));
        *reinterpret_cast<uint32_t*>(dest) = uint32_t((((a << 8 | r) << 8 | g) << 8) | b);
        if (desc->remap == nullptr) {
            source += desc->sourceStride;
        }
        dest += desc->destStride;
        total += desc->elementSize;
    }
    return total;
}

// 0x8A18C0 (thiscall, slot 0): the declaration base's deleting destructor
uint8_t* __fastcall DeclarationBase_DeletingDestructor(uint8_t* self, re::Edx, uint32_t flags) {
    DeclarationBase_Destruct(self);
    if (flags & 1) {
        NiDelete(self, kDeclarationBaseSize);
    }
    return self;
}

// 0x8A1910 (thiscall, `this` unused): an entry of the declaration packed: its source by its
// parameter (position, blend weights and bone indices from the partition by its bones per
// vertex, normal, colour, texture coordinate sets, tangent and binormal (after the normals), an
// additional data stream), the packing function for its D3D type (D3DCOLOR made UBYTE4 for bone
// indices) and the source type, the vertices remapped through the partition's vertex map (not for
// weights and indices); the bytes packed, 0 when there's nothing to pack. (Bones per vertex past
// 4, and parameters past the known ones that aren't data streams, leave the game reading its
// uninitialised stack: here the stride argument as the source type, the rest zero.)
uint32_t __fastcall Declaration_PackEntry(void*, re::Edx, uint32_t vertices, uint8_t* data, void*, uint8_t* partition,
                                          uint8_t* entry, uint32_t stride, uint8_t* dest) {
    uint32_t type = At<uint32_t>(entry, 8);
    if (type == 0x11) {
        return 0;
    }
    uint32_t d3dType = type;
    if (type == 4 && At<uint32_t>(entry, 4) == 2) {
        d3dType = 5;
    }
    PackDesc desc;
    desc.elementSize = ms_auiTypeSizes[d3dType];
    desc.destStride = stride;
    desc.dest = dest;
    uint8_t* holder = At<re::Ptr32<uint8_t>*>(ms_pkPackingFunctions, 4)[d3dType];
    if (holder == nullptr) {
        return 0;
    }
    uint32_t parameter = At<uint32_t>(entry, 4);
    const uint8_t* source = nullptr;
    uint32_t sourceType = stride;
    uint32_t sourceSize = 0;
    uint32_t sourceStride = 0;
    switch (parameter) {
    case 0:   // position
        source = At<uint8_t*>(data, 0x20);
        sourceType = 3;
        sourceSize = sourceStride = 0xC;
        break;
    case 1:   // blend weights
        if (partition == nullptr) {
            return 0;
        }
        source = At<uint8_t*>(partition, 8);
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
            sourceType = 3;
            sourceSize = sourceStride = 0xC;
            break;
        case 4:
            sourceType = 3;
            sourceSize = 0xC;
            sourceStride = 0x10;
            break;
        default:
            break;
        }
        break;
    case 2:   // bone indices
        if (partition == nullptr) {
            return 0;
        }
        source = At<uint8_t*>(partition, 0x10);
        if (source == nullptr) {
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
        break;
    case 3:   // normal
        source = At<uint8_t*>(data, 0x24);
        sourceType = 3;
        sourceSize = sourceStride = 0xC;
        break;
    case 4:   // colour
        source = At<uint8_t*>(data, 0x28);
        sourceType = 4;
        sourceSize = sourceStride = 0x10;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:   // texture coordinate sets
        source = static_cast<const uint8_t*>(GeometryData_GetTextureSet(data, uint16_t(parameter - 5)));
        sourceType = 2;
        sourceSize = sourceStride = 8;
        break;
    case 13:   // tangents (after the normals and binormals)
    case 14:   // binormals (after the normals)
        if (At<uint8_t*>(data, 0x24) == nullptr || (At<uint16_t>(data, 0x32) & 0xF000) == 0) {
            source = nullptr;
        } else {
            source = At<uint8_t*>(data, 0x24) + uint32_t(At<uint16_t>(data, 0xC)) * (parameter == 13 ? 0x18 : 0xC);
        }
        sourceType = 3;
        sourceSize = sourceStride = 0xC;
        break;
    case 0xFFFFFFFF:
        return 0;
    default:
        if (int32_t(parameter) < 0) {
            re::Ptr32<void> stream = nullptr;
            uint32_t count = 0;
            uint32_t unknown = 0;
            if (AdditionalData_GetDataStream(At<void*>(data, 0x38), parameter & 0x7FFFFFFF, &stream, &sourceType,
                                             &count, &unknown, &sourceSize, &sourceStride) != 0) {
                source = static_cast<const uint8_t*>(stream);
            } else {
                source = nullptr;
                sourceType = 0;
                sourceSize = 0;
                sourceStride = 0;
            }
        }
        break;
    }
    PackFn pack = re::GameFnAt<PackFn>(&At<re::Ptr32<void>*>(holder, 8)[sourceType]);
    if (pack == nullptr) {
        return 0;
    }
    if (partition == nullptr || parameter == 1 || parameter == 2) {
        desc.remap = nullptr;
    } else {
        desc.remap = At<const uint16_t*>(partition, 0xC);
    }
    desc.d3dType = d3dType;
    desc.sourceType = sourceType;
    desc.vertices = uint16_t(vertices);
    desc.source = source;
    desc.sourceSize = sourceSize;
    desc.sourceStride = sourceStride;
    return pack(&desc);
}

// 0x8A1CD0: the packing functions' tables deleted (each type's array freed and its holder
// deleted, the array emptied and deleted; the pointer left as it was)
void __cdecl Declaration_ShutdownPackingFunctions() {
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

// 0x8A1D90: the D3D types' sizes and a function array per type (38 slots) made, each function set
// (once: when the array hasn't the 18 types yet); true
uint8_t __cdecl Declaration_InitPackingFunctionTablesImpl() {
    if (At<uint16_t>(ms_pkPackingFunctions, 0xA) == 0x12) {
        return 1;
    }
    static const uint32_t kSizes[0x11] = {4, 8, 0xC, 0x10, 4, 4, 4, 8, 4, 4, 8, 4, 8, 4, 4, 4, 8};
    for (uint32_t i = 0; i < 0x11; i++) {
        ms_auiTypeSizes[i] = kSizes[i];
    }
    for (uint32_t i = 0; i < 0x12; i++) {
        void* memory = NiNew(0x14);
        uint8_t* holder = nullptr;
        if (memory != nullptr) {
            PackerHolder_Construct(memory);
            holder = static_cast<uint8_t*>(memory);
        }
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
            Declaration_SetPackingFunction(d3dType, sourceType);
        }
    }
    return 1;
}

// 0x8A1EA0: the packing functions' array made (an NiTPrimitiveArray growing by 1) and filled
void __cdecl Declaration_InitPackingFunctions() {
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
    Declaration_InitPackingFunctionTables();
}

#define C41F_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C41F_REVERSED("NiD3DShaderDeclaration::NiD3DShaderDeclaration", 0x008A01D0, DeclarationBase_Construct);
C41F_REVERSED("NiD3DShaderDeclaration::ReturnTrue", 0x008A0200, DeclarationBase_ReturnTrue);
C41F_REVERSED("NiD3DShaderDeclaration::PackInt32ToFloat", 0x008A0210, Pack_Int32ToFloat);
C41F_REVERSED("NiD3DShaderDeclaration::PackUInt32ToFloat", 0x008A0420, Pack_UInt32ToFloat);
C41F_REVERSED("NiD3DShaderDeclaration::PackInt16ToFloat", 0x008A05F0, Pack_Int16ToFloat);
C41F_REVERSED("NiD3DShaderDeclaration::PackUInt16ToFloat", 0x008A0820, Pack_UInt16ToFloat);
C41F_REVERSED("NiD3DShaderDeclaration::PackInt8ToFloat", 0x008A09F0, Pack_Int8ToFloat);
C41F_REVERSED("NiD3DShaderDeclaration::PackCopyColor", 0x008A0C20, Pack_CopyColor);
C41F_REVERSED("NiD3DShaderDeclaration::PackInt16ToColor", 0x008A0CC0, Pack_Int16ToColor);
C41F_REVERSED("NiD3DShaderDeclaration::PackUInt16ToColor", 0x008A0E10, Pack_UInt16ToColor);
C41F_REVERSED("NiD3DShaderDeclaration::PackInt8ToColor", 0x008A0F70, Pack_Int8ToColor);
C41F_REVERSED("NiD3DShaderDeclaration::PackUInt8ToColor", 0x008A10C0, Pack_UInt8ToColor);
C41F_REVERSED("NiD3DShaderDeclaration::ShaderRegisterStream::ShaderRegisterStream", 0x008A1200,
              DeclarationStream_Construct);
C41F_REVERSED("NiD3DShaderDeclaration::~NiD3DShaderDeclaration", 0x008A1220, DeclarationBase_DestructImpl);
C41F_REVERSED("NiD3DShaderDeclaration::Initialize", 0x008A1250, DeclarationBase_Initialize);
C41F_REVERSED("NiD3DShaderDeclaration::PackFloatTruncate", 0x008A1350, Pack_FloatTruncate);
C41F_REVERSED("NiD3DShaderDeclaration::PackFloatExtend", 0x008A1430, Pack_FloatExtend);
C41F_REVERSED("NiD3DShaderDeclaration::PackFloatCopy", 0x008A1580, Pack_FloatCopy);
C41F_REVERSED("NiD3DShaderDeclaration::PackFloatToColor", 0x008A1670, Pack_FloatToColor);
C41F_REVERSED("NiD3DShaderDeclaration::DeletingDestructorBase", 0x008A18C0, DeclarationBase_DeletingDestructor);
C41F_REVERSED("NiD3DShaderDeclaration::PackEntry", 0x008A1910, Declaration_PackEntry);
C41F_REVERSED("NiD3DShaderDeclaration::ShutdownPackingFunctions", 0x008A1CD0, Declaration_ShutdownPackingFunctions);
C41F_REVERSED("NiD3DShaderDeclaration::InitPackingFunctionTables", 0x008A1D90,
              Declaration_InitPackingFunctionTablesImpl);
C41F_REVERSED("NiD3DShaderDeclaration::InitPackingFunctions", 0x008A1EA0, Declaration_InitPackingFunctions);

} // namespace ni::c41
