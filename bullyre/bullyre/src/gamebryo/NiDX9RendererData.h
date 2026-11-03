#pragma once

// What the files of chunk 40 (NiDX9Renderer's data classes and helpers, 0x88D930..0x89ADB0)
// share: the shader constant map entry, the operator constants' operand buffers, the texture
// renderer data and 2D buffer data layouts, the D3DX import slots. Everything in ni::c40 so
// nothing clashes with the other Gamebryo files (NiDX9Renderer.h's names are used read-only).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni::c40 {

using ni::dx9::Com;
using ni::sys::At;
using ni::sys::Decrement;
using ni::sys::Import;
using ni::sys::Increment;
using ni::sys::Virtual;

// ---- D3DX, through the game's import slots (the exe's own jmp thunks at 0x85AC84 multiply,
// 0x85AC90 transpose, 0x85ACB4 inverse go through the same slots)
constexpr uintptr_t kD3DXMatrixMultiply = 0x008FC2E4;
constexpr uintptr_t kD3DXMatrixTranspose = 0x008FC2EC;
constexpr uintptr_t kD3DXMatrixInverse = 0x008FC304;
constexpr uintptr_t kD3DXGetShaderVersion = 0x008FC308;
constexpr uintptr_t kD3DXAssembleShader = 0x008FC2B4;
constexpr uintptr_t kD3DXAssembleShaderFromFileA = 0x008FC2B8;
constexpr uintptr_t kD3DXCompileShaderFromFileA = 0x008FC2BC;
constexpr uintptr_t kD3DXGetShaderConstantTable = 0x008FC2C0;
constexpr uintptr_t kD3DXCompileShader = 0x008FC2F0;
constexpr uintptr_t kD3DXGetVertexShaderProfile = 0x008FC314;
constexpr uintptr_t kD3DXGetPixelShaderProfile = 0x008FC318;
constexpr uintptr_t kD3DXCreateTexture = 0x008FC2DC;
constexpr uintptr_t kD3DXLoadSurfaceFromSurface = 0x008FC2E0;

inline void MatrixMultiply(float* out, const float* a, const float* b) {
    Import<float*>(kD3DXMatrixMultiply, out, a, b);
}
inline void MatrixInverse(float* m) {
    Import<float*>(kD3DXMatrixInverse, m, static_cast<float*>(nullptr), m);
}
inline void MatrixTranspose(float* out, const float* m) {
    Import<float*>(kD3DXMatrixTranspose, out, m);
}

// the renderer (NiDX9Renderer::ms_pkRenderer is NiRenderer's)
inline uint8_t* Renderer() {
    return static_cast<uint8_t*>(ni::dx9::ms_pkRenderer);
}

// ---- NiShaderConstantMapEntry (0x34, vtable 0x99C2E4: constructor 0x88B520, destructor
// 0x88B580): one constant a shader program is given
struct ConstantMapEntry {
    re::Ptr32<void> vtable;            // +0x0
    uint32_t refCount;       // +0x4
    re::Ptr32<char> key;               // +0x8 (NiFixedString: the name)
    uint32_t internal;       // +0xC (the predefined / object mapping)
    uint32_t flags;          // +0x10 (low byte: the attribute type bit; byte 1: the object type;
                             //  top nibble: 0x1 constant, 0x2 predefined, 0x3 attribute,
                             //  0x4 global, 0x5 operator, 0x6 object)
    uint32_t extra;          // +0x14 (the object's index; an operator's operands and kind)
    uint32_t shaderRegister; // +0x18
    uint32_t registerCount;  // +0x1C
    re::Ptr32<char> variableName;      // +0x20 (NiFixedString)
    uint32_t dataSize;       // +0x24
    uint32_t dataStride;     // +0x28
    re::Ptr32<void> data;              // +0x2C
    uint8_t ownData;         // +0x30
    uint8_t unknown31;
    uint8_t unknown32;
};
static_assert(offsetof(ConstantMapEntry, extra) == 0x14);
static_assert(offsetof(ConstantMapEntry, data) == 0x2C);
static_assert(sizeof(ConstantMapEntry) == 0x34);

// the attribute types by the entry's low flags byte (NiShaderAttributeDesc::AttributeType:
// 1 bool, 3 unsigned int, 4 float, 5 point2, 6 point3, 7 point4, 8 matrix3, 9 matrix4, 10 color,
// 14 texture) and the object types by its byte 1
GAME_ARRAY(uint32_t, ms_auiAttributeTypes, 0x020E408C);
GAME_ARRAY(uint32_t, ms_auiObjectTypes, 0x020E4040);
inline uint32_t AttributeType(const ConstantMapEntry* entry) {
    return ms_auiAttributeTypes[entry->flags & 0xFF];
}

// the operator constants' operands (two of each: a vector, a matrix) and the object constants'
// value
GAME_ARRAY(float, ms_afOperandVectors, 0x020E6EC0);   // [2][4]
GAME_ARRAY(float, ms_afObjectValue, 0x020E6EF0);      // the object constant's registers
GAME_ARRAY(float, ms_afOperandMatrices, 0x020E6F90);  // [2][16]
// the object mappings: value type, registers, valid (12 bytes each, up to 30)
GAME_ARRAY(uint32_t, ms_akObjectMappings, 0x020E3308);

} // namespace ni::c40
