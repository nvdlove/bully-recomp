// The packing functions NiD3DShaderDeclaration's entries pack geometry data with (picked by
// SetPackingFunction 0x8862F0 by D3D type and source type): the ones in this range turn the
// source components into SHORT2 / SHORT4 (zero padded) or UBYTE4 (zero padded) elements. Each
// takes a description and returns the bytes it accounted for (the element size per vertex). The
// source type's components are its value less its group's base (1..4 floats, 5..8 and 9..12
// 32-bit integers, 13..16 16-bit, 17..20 16-bit for bytes, 21..24 bytes).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "reversed.h"

namespace ni::c39 {
namespace {

struct PackDesc {
    uint32_t d3dType;            // +0x0 (6 SHORT2: two components, else four)
    uint16_t sourceType;         // +0x4
    uint16_t pad6;
    uint16_t vertices;           // +0x8
    uint16_t padA;
    re::Ptr32<const uint16_t> remap;       // +0xC (vertex indices, or null: in order)
    re::Ptr32<const uint8_t> source;       // +0x10 (null: zeros)
    uint32_t unknown14;
    uint32_t sourceStride;       // +0x18
    uint32_t elementSize;        // +0x1C
    uint32_t destStride;         // +0x20
    re::Ptr32<uint8_t> dest;               // +0x24
};
static_assert(offsetof(PackDesc, dest) == 0x24);

// float to short through __ftol2 (truncated through 64 bits, the low half's low word)
inline int16_t FloatToShort(const uint8_t* p) {
    float value;
    crt::Memcpy(&value, p, 4);
    return int16_t(crt::FloatToInt(double(value)));
}
// float to byte through fistp with the chop mode (32 bits; out of range the integer indefinite)
inline uint8_t FloatToByte(const uint8_t* p) {
    float value;
    crt::Memcpy(&value, p, 4);
    return uint8_t(int32_t(value));
}
inline int16_t LowWord(const uint8_t* p) {
    int16_t value;
    crt::Memcpy(&value, p, 2);
    return value;
}
inline int16_t SignedByte(const uint8_t* p) {
    return int16_t(int8_t(*p));
}
inline uint8_t LowByte(const uint8_t* p) {
    return *p;
}

// the common loop: per vertex (in order or remapped) min(components, out) converted, the rest of
// the element zeroed; zeros for every vertex when there's no source
template <typename Dest, Dest (*Convert)(const uint8_t*), uint16_t Base, uint32_t Step, bool Short>
uint32_t Pack(PackDesc* desc) {
    uint8_t* dest = desc->dest;
    const uint8_t* source = desc->source;
    uint32_t total = 0;
    if (source == nullptr) {
        for (uint16_t v = 0; v < desc->vertices; v++) {
            crt::Memset(dest, 0, desc->elementSize);
            dest += desc->destStride;
            total += desc->elementSize;
        }
        return total;
    }
    uint16_t out = Short ? uint16_t(desc->d3dType != 6 ? 4 : 2) : uint16_t(4);
    uint16_t components = uint16_t(desc->sourceType - Base);
    if (components > out) {
        components = out;
    }
    for (uint16_t v = 0; v < desc->vertices; v++) {
        const uint8_t* from = desc->remap != nullptr ? desc->source + desc->remap[v] * desc->sourceStride : source;
        Dest* to = reinterpret_cast<Dest*>(dest);
        uint16_t c = 0;
        for (; c < components; c++) {
            to[c] = Convert(from + c * Step);
        }
        for (; c < out; c++) {
            to[c] = 0;
        }
        total += desc->elementSize;
        dest += desc->destStride;
        source += desc->sourceStride;
    }
    return total;
}

} // namespace

// 0x884E70: floats to SHORT2/4
uint32_t __cdecl Pack_FloatToShort(PackDesc* desc) {
    return Pack<int16_t, FloatToShort, 0, 4, true>(desc);
}
// 0x885050: 32-bit integers (5..8) to SHORT2/4 (their low words)
uint32_t __cdecl Pack_Int32ToShort(PackDesc* desc) {
    return Pack<int16_t, LowWord, 4, 4, true>(desc);
}
// 0x885210: 32-bit integers (9..12) to SHORT2/4
uint32_t __cdecl Pack_UInt32ToShort(PackDesc* desc) {
    return Pack<int16_t, LowWord, 8, 4, true>(desc);
}
// 0x8853D0: 16-bit values (13..16) to SHORT2/4 (copied: memcpy in the game)
uint32_t __cdecl Pack_Int16ToShort(PackDesc* desc) {
    return Pack<int16_t, LowWord, 12, 2, true>(desc);
}
// 0x885570: bytes (21..24) to SHORT2/4, sign extended
uint32_t __cdecl Pack_Int8ToShort(PackDesc* desc) {
    return Pack<int16_t, SignedByte, 20, 1, true>(desc);
}
// 0x885740: floats to UBYTE4
uint32_t __cdecl Pack_FloatToUByte(PackDesc* desc) {
    return Pack<uint8_t, FloatToByte, 0, 4, false>(desc);
}
// 0x885930: 32-bit integers (5..8) to UBYTE4 (their low bytes)
uint32_t __cdecl Pack_Int32ToUByte(PackDesc* desc) {
    return Pack<uint8_t, LowByte, 4, 4, false>(desc);
}
// 0x885AE0: 32-bit integers (9..12) to UBYTE4
uint32_t __cdecl Pack_UInt32ToUByte(PackDesc* desc) {
    return Pack<uint8_t, LowByte, 8, 4, false>(desc);
}
// 0x885C90: 16-bit values (17..20) to UBYTE4 (their low bytes)
uint32_t __cdecl Pack_UInt16ToUByte(PackDesc* desc) {
    return Pack<uint8_t, LowByte, 16, 2, false>(desc);
}

#define C39G_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39G_REVERSED("NiD3DShaderDeclaration::PackFloatToShort", 0x00884E70, Pack_FloatToShort);
C39G_REVERSED("NiD3DShaderDeclaration::PackInt32ToShort", 0x00885050, Pack_Int32ToShort);
C39G_REVERSED("NiD3DShaderDeclaration::PackUInt32ToShort", 0x00885210, Pack_UInt32ToShort);
C39G_REVERSED("NiD3DShaderDeclaration::PackInt16ToShort", 0x008853D0, Pack_Int16ToShort);
C39G_REVERSED("NiD3DShaderDeclaration::PackInt8ToShort", 0x00885570, Pack_Int8ToShort);
C39G_REVERSED("NiD3DShaderDeclaration::PackFloatToUByte", 0x00885740, Pack_FloatToUByte);
C39G_REVERSED("NiD3DShaderDeclaration::PackInt32ToUByte", 0x00885930, Pack_Int32ToUByte);
C39G_REVERSED("NiD3DShaderDeclaration::PackUInt32ToUByte", 0x00885AE0, Pack_UInt32ToUByte);
C39G_REVERSED("NiD3DShaderDeclaration::PackUInt16ToUByte", 0x00885C90, Pack_UInt16ToUByte);

} // namespace ni::c39
