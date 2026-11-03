// NiD3DShaderConstantMap's operator constants (add, subtract, multiply, divide two operands the
// operator entry filled in: vector A at 0x20E6EC0, B at 0x20E6ED0, matrices A at 0x20E6F90, B at
// 0x20E6FD0; results at 0x20E6EB0 (vector) and 0x20E7010 (matrix), set through the shader program's
// slot 14 (entry, data, 0)), the texture transform as a D3D matrix, an attribute's data as
// registers (the static buffers at 0x20E6EE0.. by type) and the register padding helper. Float maths
// in the game's order (the main thread's 24-bit precision: each operation a float's).
//
// Operand types: 3 / 4 vectors (float4), 7 / 10 scalars (as the code reads them), 9 matrices.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "reversed.h"

namespace ni::c39 {
using namespace ni::dx9;
namespace {

GAME_ARRAY(float, ms_afResult, 0x020E6EB0);
GAME_ARRAY(float, ms_afA, 0x020E6EC0);
GAME_ARRAY(float, ms_afB, 0x020E6ED0);
GAME_ARRAY(float, ms_afMatrixA, 0x020E6F90);
GAME_ARRAY(float, ms_afMatrixB, 0x020E6FD0);
GAME_ARRAY(float, ms_afMatrixResult, 0x020E7010);
GAME_ARRAY(uint32_t, ms_auiRegisters, 0x020E6EE0);    // a float4 by type
GAME_ARRAY(uint32_t, ms_auiBoolRegister, 0x020E6F30);
GAME_ARRAY(uint32_t, ms_auiIntRegisters, 0x020E6F34);
GAME_ARRAY(uint32_t, ms_auiMatrix3Registers, 0x020E6F48);
GAME_ARRAY(const uint32_t, ms_auiGlobalTypes, 0x020E408C);
constexpr uintptr_t NiFloatsExtraData_RTTI = 0x020E2ABC;
constexpr uintptr_t NiColorExtraData_RTTI = 0x020E29DC;

constexpr uintptr_t kD3DXVec4Transform = 0x008FC2D4;
constexpr uintptr_t kD3DXMatrixMultiply = 0x008FC2E4;
constexpr uintptr_t kD3DXMatrixTranspose = 0x008FC2EC;
constexpr uintptr_t kD3DXMatrixInverse = 0x008FC304;

GAME_FN(uint8_t, __cdecl, IsKindOf, 0x007644E0, uintptr_t rtti, const void* object);
GAME_FN(uint8_t, __stdcall, Entry_GetDimensions, 0x0088AAC0, void* entry, uint32_t* registers, uint32_t* columns);
GAME_FN(void, __thiscall, ScratchArray_SetSize, 0x007CF4F0, void* self, uint32_t size);

inline bool IsVector(uint32_t type) {
    return type == 3 || type == 4;
}
inline bool IsScalar(uint32_t type) {
    return type == 7 || type == 10;
}

// the program's SetShaderConstant: 0 done, 0x50 failed
inline uint32_t Send(void* program, void* entry, const void* data) {
    return Virtual<uint8_t>(program, 14, entry, data, 0u) ? 0 : 0x50;
}

inline void InvertTranspose(int invert, int transpose) {
    if (uint8_t(invert) != 0) {
        Import<void*>(kD3DXMatrixInverse, ms_afMatrixResult, static_cast<float*>(nullptr), ms_afMatrixResult);
    }
    if (uint8_t(transpose) != 0) {
        Import<void*>(kD3DXMatrixTranspose, ms_afMatrixResult, ms_afMatrixResult);
    }
}

} // namespace

// 0x88A050: the add operator (vector + vector by components; a vector A with a scalar B: A's first
// with each of B's; a scalar A with a vector B: each of A's with B's first)
uint32_t __fastcall ConstantMap_AddOperator(void*, re::Edx, void* program, void* entry, uint32_t typeA, uint32_t typeB, uint32_t,
                                           uint32_t) {
    float* r = ms_afResult;
    const float* a = ms_afA;
    const float* b = ms_afB;
    if (IsVector(typeA)) {
        if (IsVector(typeB)) {
            r[0] = b[0] + a[0]; r[1] = b[1] + a[1]; r[2] = b[2] + a[2]; r[3] = b[3] + a[3];
        } else if (IsScalar(typeB)) {
            r[0] = b[0] + a[0]; r[1] = b[1] + a[0]; r[2] = b[2] + a[0]; r[3] = a[0] + b[3];
        } else {
            return 1;
        }
    } else if (IsScalar(typeA)) {
        if (IsVector(typeB)) {
            r[0] = b[0] + a[0]; r[1] = a[1] + b[0]; r[2] = a[2] + b[0]; r[3] = b[0] + a[3];
        } else if (IsScalar(typeB)) {
            r[0] = b[0] + a[0]; r[1] = b[1] + a[1]; r[2] = b[2] + a[2]; r[3] = b[3] + a[3];
        } else {
            return 1;
        }
    } else {
        return 1;
    }
    return Send(program, entry, ms_afResult);
}

// 0x88A260: the subtract operator (A - B, the same pairings)
uint32_t __fastcall ConstantMap_SubtractOperator(void*, re::Edx, void* program, void* entry, uint32_t typeA, uint32_t typeB, uint32_t,
                                                uint32_t) {
    float* r = ms_afResult;
    const float* a = ms_afA;
    const float* b = ms_afB;
    if (IsVector(typeA)) {
        if (IsVector(typeB)) {
            r[0] = a[0] - b[0]; r[1] = a[1] - b[1]; r[2] = a[2] - b[2]; r[3] = a[3] - b[3];
        } else if (IsScalar(typeB)) {
            r[0] = a[0] - b[0]; r[1] = a[0] - b[1]; r[2] = a[0] - b[2]; r[3] = a[0] - b[3];
        } else {
            return 1;
        }
    } else if (IsScalar(typeA)) {
        if (IsVector(typeB)) {
            r[0] = a[0] - b[0]; r[1] = a[1] - b[0]; r[2] = a[2] - b[0]; r[3] = a[3] - b[0];
        } else if (IsScalar(typeB)) {
            r[0] = a[0] - b[0]; r[1] = a[1] - b[1]; r[2] = a[2] - b[2]; r[3] = a[3] - b[3];
        } else {
            return 1;
        }
    } else {
        return 1;
    }
    return Send(program, entry, ms_afResult);
}

// 0x88AE00: the multiply operator (matrix x matrix, matrix x scalar, matrix x vector (D3DX), and
// the vector / scalar products; the matrix results inverted / transposed when asked)
uint32_t __fastcall ConstantMap_MultiplyOperator(void*, re::Edx, void* program, void* entry, uint32_t typeA, uint32_t typeB,
                                                int invert, int transpose) {
    float* r = ms_afResult;
    const float* a = ms_afA;
    const float* b = ms_afB;
    if (typeA == 9) {
        if (typeB == 9) {
            Import<void*>(kD3DXMatrixMultiply, ms_afMatrixResult, ms_afMatrixA, ms_afMatrixB);
            InvertTranspose(invert, transpose);
            return Send(program, entry, ms_afMatrixResult);
        }
        if (IsVector(typeB)) {
            float m[16];
            for (int i = 0; i < 16; i++) {
                m[i] = ms_afMatrixA[i] * b[0];
            }
            for (int i = 0; i < 16; i++) {
                ms_afMatrixResult[i] = m[i];
            }
            InvertTranspose(invert, transpose);
            return Send(program, entry, ms_afMatrixResult);
        }
        if (IsScalar(typeB)) {
            float v[4] = {b[0], b[1], b[2], b[3]};
            float out[4];
            Import<void*>(kD3DXVec4Transform, out, v, ms_afMatrixA);
            r[0] = out[0]; r[1] = out[1]; r[2] = out[2]; r[3] = out[3];
            return Send(program, entry, ms_afResult);
        }
        return 1;
    }
    if (!IsScalar(typeA)) {
        return 1;
    }
    if (IsVector(typeB)) {
        r[0] = b[0] * a[0]; r[1] = a[1] * b[0]; r[2] = a[2] * b[0]; r[3] = b[0] * a[3];
        return Send(program, entry, ms_afResult);
    }
    if (IsScalar(typeB)) {
        r[0] = a[0] * b[0]; r[1] = a[1] * b[1]; r[2] = a[2] * b[2]; r[3] = a[3] * b[3];
        return Send(program, entry, ms_afResult);
    }
    if (typeB == 9) {
        float v[4] = {a[0], a[1], a[2], a[3]};
        float out[4];
        Import<void*>(kD3DXVec4Transform, out, v, ms_afMatrixB);
        r[0] = out[0]; r[1] = out[1]; r[2] = out[2]; r[3] = out[3];
        return Send(program, entry, ms_afResult);
    }
    return 1;
}

// 0x88B1D0: the divide operator (a matrix by a vector's first: times its reciprocal; A over B's
// first, or by components)
uint32_t __fastcall ConstantMap_DivideOperator(void*, re::Edx, void* program, void* entry, uint32_t typeA, uint32_t typeB,
                                              int invert, int transpose) {
    float* r = ms_afResult;
    const float* a = ms_afA;
    const float* b = ms_afB;
    if (typeA == 9) {
        if (int32_t(typeB) < 3 || int32_t(typeB) > 4) {
            return 1;
        }
        float reciprocal = 1.0f / b[0];
        float m[16];
        for (int i = 0; i < 16; i++) {
            m[i] = ms_afMatrixA[i] * reciprocal;
        }
        for (int i = 0; i < 16; i++) {
            ms_afMatrixResult[i] = m[i];
        }
        InvertTranspose(invert, transpose);
        return Send(program, entry, ms_afMatrixResult);
    }
    if (!IsScalar(typeA)) {
        return 1;
    }
    if (IsVector(typeB)) {
        r[0] = a[0] / b[0]; r[1] = a[1] / b[0]; r[2] = a[2] / b[0]; r[3] = a[3] / b[0];
        return Send(program, entry, ms_afResult);
    }
    if (IsScalar(typeB)) {
        r[0] = a[0] / b[0]; r[1] = a[1] / b[1]; r[2] = a[2] / b[2]; r[3] = a[3] / b[3];
        return Send(program, entry, ms_afResult);
    }
    return 1;
}

// 0x88B440: a texture transform's 3x3 as a D3D texture matrix (the translation in the third or
// fourth row by the coordinate count; the identity without one; inverted / transposed when asked)
void __cdecl TextureTransform_ToD3D(float* out, const float* m, int invert, int transpose, uint32_t coordinates) {
    if (m == nullptr) {
        for (int i = 0; i < 16; i++) {
            out[i] = 0.0f;
        }
        out[15] = 1.0f;
        out[10] = 1.0f;
        out[5] = 1.0f;
        out[0] = 1.0f;
        return;
    }
    out[0] = m[0];
    out[4] = m[1];
    out[1] = m[3];
    out[5] = m[4];
    if (coordinates == 4 || coordinates == 3) {
        out[8] = 0.0f;
        out[12] = m[2];
        out[9] = 0.0f;
        out[13] = m[5];
    } else if (coordinates == 2) {
        out[8] = m[2];
        out[12] = 0.0f;
        out[9] = m[5];
        out[13] = 0.0f;
    }
    out[14] = 0.0f;
    out[10] = 0.0f;
    out[6] = 0.0f;
    out[2] = 0.0f;
    out[15] = 0.0f;
    out[11] = 0.0f;
    out[7] = 0.0f;
    out[3] = 0.0f;
    if (uint8_t(invert) != 0) {
        Import<void*>(kD3DXMatrixInverse, out, static_cast<float*>(nullptr), out);
    }
    if (uint8_t(transpose) != 0) {
        Import<void*>(kD3DXMatrixTranspose, out, out);
    }
}

// 0x88BFB0: an attribute entry's extra data as registers (by the attribute type: a bool, an
// integer or float broadcast, a point2 twice, a point3 with w 1, floats or a color, a matrix3 padded,
// a matrix4 as it is); null for the others
const void* __fastcall ConstantMap_GetAttributeData(void*, re::Edx, uint8_t* entry, uint8_t* extra) {
    uint32_t type = ms_auiGlobalTypes[At<uint32_t>(entry, 0x10) & 0xFF];
    uint32_t* reg = ms_auiRegisters;
    switch (type) {
    case 1:
        ms_auiBoolRegister[0] = At<uint8_t>(extra, 0xC);
        return ms_auiBoolRegister;
    case 3: {
        uint32_t v = At<uint32_t>(extra, 0xC);
        ms_auiIntRegisters[0] = v;
        ms_auiIntRegisters[1] = v;
        ms_auiIntRegisters[2] = v;
        ms_auiIntRegisters[3] = v;
        return ms_auiIntRegisters;
    }
    case 4: {
        uint32_t v = At<uint32_t>(extra, 0xC);
        reg[0] = v; reg[1] = v; reg[2] = v; reg[3] = v;
        return reg;
    }
    case 5: {
        const uint32_t* p = At<const uint32_t*>(extra, 0x10);
        reg[0] = p[0]; reg[1] = p[1]; reg[2] = p[0]; reg[3] = p[1];
        return reg;
    }
    case 6: {
        const uint32_t* p = At<const uint32_t*>(extra, 0x10);
        reg[0] = p[0]; reg[1] = p[1]; reg[2] = p[2]; reg[3] = 0x3F800000;
        return reg;
    }
    case 7:
        if (IsKindOf(NiFloatsExtraData_RTTI, extra)) {
            const uint32_t* p = At<const uint32_t*>(extra, 0x10);
            reg[0] = p[0]; reg[1] = p[1]; reg[2] = p[2]; reg[3] = p[3];
            return reg;
        }
        if (IsKindOf(NiColorExtraData_RTTI, extra)) {
            reg[0] = At<uint32_t>(extra, 0xC);
            reg[1] = At<uint32_t>(extra, 0x10);
            reg[2] = At<uint32_t>(extra, 0x14);
            reg[3] = At<uint32_t>(extra, 0x18);
        }
        return reg;
    case 8: {
        const uint32_t* p = At<const uint32_t*>(extra, 0x10);
        uint32_t* m = ms_auiMatrix3Registers;
        m[0] = p[0]; m[1] = p[1]; m[2] = p[2]; m[3] = 0;
        m[4] = p[3]; m[5] = p[4]; m[6] = p[5]; m[7] = 0;
        m[8] = p[6]; m[9] = p[7]; m[10] = p[8]; m[11] = 0;
        return m;
    }
    case 9:
        return At<const void*>(extra, 0x10);
    case 10:
        reg[0] = At<uint32_t>(extra, 0xC);
        reg[1] = At<uint32_t>(extra, 0x10);
        reg[2] = At<uint32_t>(extra, 0x14);
        reg[3] = At<uint32_t>(extra, 0x18);
        return reg;
    default:
        return nullptr;
    }
}

// 0x88C610 (thiscall): an entry's data padded to four floats a register in the map's scratch array
// (+0xC; unless it's already four columns: the data itself)
const void* __fastcall ConstantMap_PadRegisters(uint8_t* self, re::Edx, uint8_t* entry, const uint8_t* data) {
    uint32_t registers, columns;
    Entry_GetDimensions(entry, &registers, &columns);
    if (columns == 4) {
        return data;
    }
    uint32_t elements = At<uint32_t>(entry, 0x24) / At<uint32_t>(entry, 0x28);
    ScratchArray_SetSize(self + 0xC, 0);
    uint32_t total = elements * registers;
    ScratchArray_SetSize(self + 0xC, total * 4);
    uint8_t* to = At<uint8_t*>(self, 0x10);
    for (uint32_t i = total; i != 0; i--) {
        crt::MemcpyS(to, columns * 4, data, columns * 4);
        data += columns * 4;
        to += 0x10;
    }
    return At<const void*>(self, 0x10);
}

#define C39P_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C39P_REVERSED("NiD3DShaderConstantMap::AddOperator", 0x0088A050, ConstantMap_AddOperator);
C39P_REVERSED("NiD3DShaderConstantMap::SubtractOperator", 0x0088A260, ConstantMap_SubtractOperator);
C39P_REVERSED("NiD3DShaderConstantMap::MultiplyOperator", 0x0088AE00, ConstantMap_MultiplyOperator);
C39P_REVERSED("NiD3DShaderConstantMap::DivideOperator", 0x0088B1D0, ConstantMap_DivideOperator);
C39P_REVERSED("NiD3DUtility::GetD3DTextureTransform", 0x0088B440, TextureTransform_ToD3D);
C39P_REVERSED("NiD3DShaderConstantMap::GetAttributeData", 0x0088BFB0, ConstantMap_GetAttributeData);
C39P_REVERSED("NiD3DShaderConstantMap::PadRegisters", 0x0088C610, ConstantMap_PadRegisters);

} // namespace ni::c39
