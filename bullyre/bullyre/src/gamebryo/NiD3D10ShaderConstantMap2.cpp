// NiD3D10ShaderConstantMap's constants' values (NiD3D10ShaderConstantMap.cpp has the class): the
// predefined ones (the renderer's matrices, the skin's, the texture transforms, the material,
// fog, time, ambient light, texture sizes, alpha test...), the attributes (the geometry's extra
// data: through the SCM extra data's cache), the operators (two entries' values combined: four
// cdecl helpers on (a, type, b, type, invert, transpose, result)) and the object constants (a
// light's or texture effect's). Each into the caller's buffer, or a pointer at the data itself.
//
// Types (the global types): 3 an unsigned int, 4 a float, 7 / 10 a float4, 9 a 4x4 matrix. Float
// maths as the original orders it; where the x87 code works on an unsigned int exactly before
// rounding (fild, then the operation), the operation is done in double and rounded once.

#include "gamebryo/NiD3D10C44.h"
#include "reversed.h"

namespace ni::c44 {
namespace {

constexpr float kFar = -1048576.0f;   // (a directional light's "position": its direction times this)

GAME_VAR(uint32_t, ms_uiEntryShift, 0x009A1914);         // the SCM extra data's pass shift
GAME_VAR(const char*, ms_pcTimeExtraData, 0x00BC8B20);  // the extra data the time constants read
GAME_ARRAY(const uint32_t, NiPoint3_ZERO, 0x020E2D44);
GAME_VAR(const double, kDegreesToRadiansA, 0x00900160);   // pi (a float's, as a double)
GAME_VAR(const double, kDegreesToRadiansB, 0x00900158);   // 180
constexpr uintptr_t NiFloatsExtraData_RTTI = 0x020E2ABC;
constexpr uintptr_t NiColorExtraData_RTTI = 0x020E29DC;
constexpr uintptr_t NiIntegersExtraData_RTTI = 0x020E2B60;
constexpr uintptr_t NiAmbientLight_RTTI = 0x020E3480;

// the renderer's (chunk 42's)
GAME_FN(float*, __thiscall, Renderer_GetProjectionMatrix, 0x008A9380, void* self);
GAME_FN(float*, __thiscall, Renderer_GetViewMatrix, 0x008A9390, void* self);
GAME_FN(float*, __thiscall, Renderer_GetInverseViewMatrix, 0x008A93A0, void* self);
GAME_FN(float*, __thiscall, Renderer_GetWorldMatrix, 0x008A93B0, void* self);
GAME_FN(void, __thiscall, Renderer_GetCameraNearAndFar, 0x008A93C0, void* self, float* nearPlane, float* farPlane);
GAME_FN(float, __thiscall, Renderer_GetMaxFogFactor, 0x008A92F0, void* self);
// the maths (NiPoint3 / NiMatrix3 / NiTransform), the D3D matrices from Gamebryo's
GAME_FN(void, __thiscall, Point3_ScaleInPlace, 0x0088A480, float* self, float scale);
GAME_FN(float*, __thiscall, Color_Times, 0x0074A400, const float* self, float* result, float scale);
GAME_FN(NiTransform*, __thiscall, Transform_Invert, 0x00770950, const void* self, NiTransform* out);
GAME_FN(float*, __thiscall, Transform_TimesPoint, 0x00450B10, const NiTransform* self, float* out, const float* p);
GAME_FN(NiTransform*, __thiscall, Transform_Times, 0x00406830, const NiTransform* self, NiTransform* out,
        const void* other);
GAME_FN(float*, __thiscall, Point_Minus, 0x0040D9C0, const void* self, float* out, const float* other);
GAME_FN(float, __thiscall, Point_Unitize, 0x00415130, float* self);
GAME_FN(float, __thiscall, Point_Dot, 0x00468CD0, const float* self, const float* other);
GAME_FN(float*, __thiscall, Matrix3_Transpose, 0x00768C40, const void* self, float* out);
GAME_FN(float*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const float* self, float* out, const float* p);
GAME_FN(float*, __thiscall, Matrix3_TimesMatrix, 0x00768920, const float* self, float* out, const float* other);
GAME_FN(void, __cdecl, MakeD3DMatrix, 0x00406670, float* out, const void* rotate, const void* translate, float scale);
GAME_FN(void, __cdecl, MakeD3DMatrixTransposed, 0x008787C0, float* out, const void* rotate, const void* translate,
        float scale);
GAME_FN(void, __cdecl, TransformToD3D, 0x008B60A0, float* out, const void* transform);
GAME_FN(void, __cdecl, TransformToD3DTransposed, 0x008B60C0, float* out, const void* transform);
// the rest of the library
GAME_FN(uint8_t, __cdecl, IsKindOf, 0x007644E0, uintptr_t rtti, const void* object);
GAME_FN(uint8_t*, __cdecl, DynamicCast, 0x00401FF0, uintptr_t rtti, const void* object);
GAME_FN(uint8_t*, __cdecl, GetDynamicEffectForObject, 0x0077F380, void* effects, uint32_t objectType, uint32_t index);
GAME_FN(uint32_t, __cdecl, GetPredefinedMappingTypeByName, 0x0077F2F0, const char* name);
GAME_FN(const float*, __thiscall, TextureTransform_GetMatrix, 0x0074A440, void* self);
GAME_FN(uint8_t*, __thiscall, TexturingProperty_GetShaderMap, 0x0074A7E0, void* self, uint32_t index);
GAME_FN(uint8_t*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* self, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, AttributeCache_Add, 0x00888790, void* scm, uint32_t reg, uint32_t pass, uint32_t type,
        void* extra, int global);
GAME_FN(uint32_t, __thiscall, ObjectSource_GetConstant, 0x0088A560, void* self, float* out, uintptr_t bytes,
        void* geometry, uintptr_t flags, uintptr_t mapping, void* state, void* effects, const void* world,
        const void* bound, uintptr_t pass);

// NiD3D10ShaderConstantMap.cpp's
GAME_FN(void, __cdecl, Map_MakeTextureTransform, 0x008BB370, float* out, const float* m, int transpose);
// this file's (called where modes apply)
GAME_FN(const float*, __cdecl, Operator_Multiply, 0x008BB5F0, const float* a, uint32_t typeA, const float* b,
        uint32_t typeB, int invert, int transpose, float* result);
GAME_FN(const float*, __cdecl, Operator_Divide, 0x008BB900, const float* a, uint32_t typeA, const float* b,
        uint32_t typeB, int invert, int transpose, float* result);
GAME_FN(const float*, __cdecl, Operator_Add, 0x008BBBA0, const float* a, uint32_t typeA, const float* b,
        uint32_t typeB, int invert, int transpose, float* result);
GAME_FN(const float*, __cdecl, Operator_Subtract, 0x008BBD50, const float* a, uint32_t typeA, const float* b,
        uint32_t typeB, int invert, int transpose, float* result);
GAME_FN(const void*, __thiscall, Map_GetPredefinedConstant, 0x008BC6D0, void* self, ConstantMapEntry* entry,
        void* geometry, void* skin, void* state, void* effects, const void* world, const void* bound, uint32_t pass,
        float* buffer);
GAME_FN(const void*, __thiscall, Map_GetAttributeConstant, 0x008BD6C0, void* self, ConstantMapEntry* entry,
        void* geometry, void* skin, void* state, void* effects, const void* world, const void* bound, uint32_t pass,
        int global, void* extraData, void* buffer);
GAME_FN(uint32_t, __cdecl, Map_ComputeObjectConstant, 0x008BDB60, ConstantMapEntry* entry, uint32_t mapping,
        uint8_t* object, void* geometry, void* skin, void* state, void* effects, const void* world,
        const void* bound, uint32_t pass, float* out);

inline bool IsVector(uint32_t type) {
    return type == 7 || type == 10;
}
inline uint32_t Bits(const float* p) {
    return *reinterpret_cast<const uint32_t*>(p);
}
inline double Exact(const float* p) {   // an unsigned int as the x87 loads it
    return double(Bits(p));
}
inline void CopyMatrix(float* to, const float* from) {
    for (int i = 0; i < 16; i++) {
        to[i] = from[i];
    }
}
inline void Set4(float* out, float x, float y, float z, float w) {
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}
inline void Identity(float* out) {
    out[14] = 0.0f; out[13] = 0.0f; out[12] = 0.0f; out[11] = 0.0f;
    out[9] = 0.0f; out[8] = 0.0f; out[7] = 0.0f; out[6] = 0.0f;
    out[4] = 0.0f; out[3] = 0.0f; out[2] = 0.0f; out[1] = 0.0f;
    out[15] = 1.0f; out[10] = 1.0f; out[5] = 1.0f; out[0] = 1.0f;
}

// a light's type (NiDynamicEffect +0xC5: 2/5 point, 3/6 directional, 4/7 spot, 8 a texture effect)
inline uint8_t EffectType(const uint8_t* object) {
    return At<uint8_t>(object, 0xC5);
}
inline bool IsPoint(const uint8_t* object) {
    return EffectType(object) == 2 || EffectType(object) == 5;
}
inline bool IsDirectional(const uint8_t* object) {
    return EffectType(object) == 3 || EffectType(object) == 6;
}
inline bool IsSpot(const uint8_t* object) {
    return EffectType(object) == 4 || EffectType(object) == 7;
}
inline bool IsTextureEffect(const uint8_t* object) {
    return EffectType(object) == 8;
}
inline void FarPosition(const uint8_t* object, float* p) {
    p[0] = At<float>(object, 0x124) * kFar;
    p[1] = At<float>(object, 0x128) * kFar;
    p[2] = kFar * At<float>(object, 0x12C);
}

// a matrix copied out, inverted and transposed as asked
inline float* FinishMatrix(float* out, const float* m, bool invert, bool transpose) {
    CopyMatrix(out, m);
    if (invert) {
        MatrixInverse(out, out);
    }
    if (transpose) {
        MatrixTranspose(out, out);
    }
    return out;
}

} // namespace

// 0x8BB5F0 (cdecl): the multiply operator (a matrix times a matrix, a scalar (its matrix scaled)
// or a vector (transformed; the result then treated as a matrix, as the original does); a vector
// times a matrix (transformed), a scalar or a vector (by components)); null when the types can't
float* __cdecl ConstantMap_MultiplyOperator(const float* a, uint32_t typeA, const float* b, uint32_t typeB,
                                              int invert, int transpose, float* result) {
    if (typeA == 9) {
        alignas(16) float m[16];
        CopyMatrix(m, a);
        if (typeB == 9) {
            alignas(16) float n[16];
            CopyMatrix(n, b);
            MatrixMultiply(result, m, n);
        } else if (IsVector(typeB)) {
            alignas(16) float v[4];
            Vec4Transform(v, b, m);
            result[0] = v[0];
            result[1] = v[1];
            result[2] = v[2];
            result[3] = v[3];
        } else {
            float s;
            if (typeB == 3) {
                s = float(Exact(b));
            } else if (typeB == 4) {
                s = b[0];
            } else {
                return nullptr;
            }
            alignas(16) float scaled[16];
            for (int i = 0; i < 16; i++) {
                scaled[i] = m[i] * s;
            }
            CopyMatrix(result, scaled);
        }
        if (re::Flag(invert)) {
            MatrixInverse(result, result);
        }
        if (re::Flag(transpose)) {
            MatrixTranspose(result, result);
        }
        return result;
    }
    if (!IsVector(typeA)) {
        return nullptr;
    }
    if (typeB == 9) {
        alignas(16) float n[16];
        CopyMatrix(n, b);
        Vec4Transform(result, a, n);
        return result;
    }
    float s;
    if (typeB == 3) {
        s = float(Exact(b));
    } else if (typeB == 4) {
        s = b[0];
    } else if (IsVector(typeB)) {
        result[0] = a[0] * b[0];
        result[1] = a[1] * b[1];
        result[2] = a[2] * b[2];
        result[3] = a[3] * b[3];
        return result;
    } else {
        return nullptr;
    }
    float r0 = a[0] * s;
    float r1 = a[1] * s;
    float r2 = a[2] * s;
    float r3 = a[3] * s;
    Set4(result, r0, r1, r2, r3);
    return result;
}

// 0x8BB900 (cdecl): the divide operator (a matrix or a vector over a scalar: times its reciprocal;
// a vector over a vector by components)
float* __cdecl ConstantMap_DivideOperator(const float* a, uint32_t typeA, const float* b, uint32_t typeB,
                                            int invert, int transpose, float* result) {
    if (typeA == 9) {
        alignas(16) float m[16];
        CopyMatrix(m, a);
        float r;
        if (typeB == 3) {
            r = float(1.0 / Exact(b));
        } else if (typeB == 4) {
            r = 1.0f / b[0];
        } else {
            return nullptr;
        }
        alignas(16) float scaled[16];
        for (int i = 0; i < 16; i++) {
            scaled[i] = m[i] * r;
        }
        CopyMatrix(result, scaled);
        if (re::Flag(invert)) {
            MatrixInverse(result, result);
        }
        if (re::Flag(transpose)) {
            MatrixTranspose(result, result);
        }
        return result;
    }
    if (!IsVector(typeA)) {
        return nullptr;
    }
    if (typeB == 3 || typeB == 4) {
        float r = typeB == 3 ? float(1.0 / Exact(b)) : 1.0f / b[0];
        float r0 = r * a[0];
        float r1 = a[1] * r;
        float r2 = a[2] * r;
        float r3 = r * a[3];
        Set4(result, r0, r1, r2, r3);
        return result;
    }
    if (IsVector(typeB)) {
        result[0] = a[0] / b[0];
        result[1] = a[1] / b[1];
        result[2] = a[2] / b[2];
        result[3] = a[3] / b[3];
        return result;
    }
    return nullptr;
}

// 0x8BBBA0 (cdecl): the add operator (two unsigned ints as an unsigned int; a scalar with a scalar
// as one float; a scalar with a vector, or a vector with a scalar, to each component; vectors by
// components)
float* __cdecl ConstantMap_AddOperator(const float* a, uint32_t typeA, const float* b, uint32_t typeB, int, int,
                                         float* result) {
    float s;
    const float* v;
    if (typeA == 3) {
        if (typeB == 3) {
            *reinterpret_cast<uint32_t*>(result) = Bits(b) + Bits(a);
            return result;
        }
        if (typeB == 4) {
            result[0] = float(Exact(a) + double(b[0]));
            return result;
        }
        if (!IsVector(typeB)) {
            return nullptr;
        }
        s = float(Exact(a));
        v = b;
    } else if (typeA == 4) {
        float x = a[0];
        if (typeB == 3) {
            result[0] = float(Exact(b) + double(x));
            return result;
        }
        if (typeB == 4) {
            result[0] = b[0] + x;
            return result;
        }
        if (!IsVector(typeB)) {
            return nullptr;
        }
        s = x;
        v = b;
    } else if (IsVector(typeA)) {
        if (typeB == 3) {
            s = float(Exact(b));
        } else if (typeB == 4) {
            s = b[0];
        } else if (IsVector(typeB)) {
            float r0 = a[0] + b[0];
            float r1 = a[1] + b[1];
            float r2 = a[2] + b[2];
            float r3 = a[3] + b[3];
            Set4(result, r0, r1, r2, r3);
            return result;
        } else {
            return nullptr;
        }
        v = a;
    } else {
        return nullptr;
    }
    result[0] = s + v[0];
    result[1] = v[1] + s;
    result[2] = v[2] + s;
    result[3] = s + v[3];
    return result;
}

// 0x8BBD50 (cdecl): the subtract operator (a - b, the same pairings)
float* __cdecl ConstantMap_SubtractOperator(const float* a, uint32_t typeA, const float* b, uint32_t typeB, int,
                                              int, float* result) {
    if (typeA == 3) {
        if (typeB == 3) {
            *reinterpret_cast<uint32_t*>(result) = Bits(a) - Bits(b);
            return result;
        }
        if (typeB == 4) {
            result[0] = float(Exact(a) - double(b[0]));
            return result;
        }
        if (!IsVector(typeB)) {
            return nullptr;
        }
        float s = float(Exact(a));
        result[0] = s - b[0];
        result[1] = s - b[1];
        result[2] = s - b[2];
        result[3] = s - b[3];
        return result;
    }
    if (typeA == 4) {
        float s = a[0];
        if (typeB == 3) {
            result[0] = float(double(s) - Exact(b));
            return result;
        }
        if (typeB == 4) {
            result[0] = s - b[0];
            return result;
        }
        if (!IsVector(typeB)) {
            return nullptr;
        }
        result[0] = s - b[0];
        result[1] = s - b[1];
        result[2] = s - b[2];
        result[3] = s - b[3];
        return result;
    }
    if (IsVector(typeA)) {
        if (typeB == 3 || typeB == 4) {
            float s = typeB == 3 ? float(Exact(b)) : b[0];
            result[0] = a[0] - s;
            result[1] = a[1] - s;
            result[2] = a[2] - s;
            result[3] = a[3] - s;
            return result;
        }
        if (IsVector(typeB)) {
            float r0 = a[0] - b[0];
            float r1 = a[1] - b[1];
            float r2 = a[2] - b[2];
            float r3 = a[3] - b[3];
            Set4(result, r0, r1, r2, r3);
            return result;
        }
    }
    return nullptr;
}


// 0x8BC6D0 (thiscall): a predefined entry's value (into the buffer; or a pointer at the skin's bone
// matrices or the entry's own data; null when there's none). The matrices' four variants: plain,
// inverted, transposed, both (the transposition flipped for a column major variable)
const void* __fastcall ConstantMap_GetPredefinedConstant(void*, re::Edx, ConstantMapEntry* entry, void* geometry,
                                                         uint8_t* skin, uint8_t* state, uint8_t* effects,
                                                         const void*, const void*, uint32_t, float* out) {
    uint8_t* renderer = D3D10Renderer_GetRenderer();
    uint32_t id = entry->mapping;
    // the variants (base, inverse, transpose, both) of a 4-id group
    auto flags = [&](uint32_t inverse1, uint32_t inverse2, uint32_t transpose1, uint32_t transpose2, bool& invert,
                     bool& transpose) {
        invert = id == inverse1 || id == inverse2;
        transpose = id == transpose1 || id == transpose2;
        if (entry->columnMajor != 0) {
            transpose = !transpose;
        }
    };
    bool invert;
    bool transpose;
    switch (id) {
    case 0x1:
    case 0x2:
    case 0xD:
    case 0xE:   // the projection
        flags(2, 0xE, 0xD, 0xE, invert, transpose);
        return FinishMatrix(out, Renderer_GetProjectionMatrix(renderer), invert, transpose);
    case 0x3:
    case 0x4:
    case 0xF:
    case 0x10:   // the view
        flags(4, 0x10, 0xF, 0x10, invert, transpose);
        return FinishMatrix(out, Renderer_GetViewMatrix(renderer), invert, transpose);
    case 0x5:
    case 0x6:
    case 0x11:
    case 0x12:   // the world
        flags(6, 0x12, 0x11, 0x12, invert, transpose);
        return FinishMatrix(out, Renderer_GetWorldMatrix(renderer), invert, transpose);
    case 0x7:
    case 0x8:
    case 0x13:
    case 0x14: {   // world * view
        flags(8, 0x14, 0x13, 0x14, invert, transpose);
        float* world = Renderer_GetWorldMatrix(renderer);
        float* view = Renderer_GetViewMatrix(renderer);
        alignas(16) float m[16];
        MatrixMultiply(m, world, view);
        return FinishMatrix(out, m, invert, transpose);
    }
    case 0x9:
    case 0xA:
    case 0x15:
    case 0x16: {   // view * projection
        flags(0xA, 0x16, 0x15, 0x16, invert, transpose);
        float* view = Renderer_GetViewMatrix(renderer);
        float* projection = Renderer_GetProjectionMatrix(renderer);
        alignas(16) float m[16];
        MatrixMultiply(m, view, projection);
        return FinishMatrix(out, m, invert, transpose);
    }
    case 0xB:
    case 0xC:
    case 0x17:
    case 0x18: {   // world * view * projection
        flags(0xC, 0x18, 0x17, 0x18, invert, transpose);
        float* world = Renderer_GetWorldMatrix(renderer);
        float* view = Renderer_GetViewMatrix(renderer);
        float* projection = Renderer_GetProjectionMatrix(renderer);
        alignas(16) float wv[16];
        MatrixMultiply(wv, world, view);
        alignas(16) float copy[16];
        CopyMatrix(copy, wv);
        alignas(16) float m[16];
        MatrixMultiply(m, copy, projection);
        return FinishMatrix(out, m, invert, transpose);
    }
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:   // the skin's world matrix times the view
        if (skin == nullptr) {
            return nullptr;
        }
        CopyMatrix(out, At<const float*>(skin, 0x2C));
        flags(0x1A, 0x1C, 0x1B, 0x1C, invert, transpose);
        MatrixMultiply(out, out, Renderer_GetViewMatrix(renderer));
        if (invert) {
            MatrixInverse(out, out);
        }
        if (transpose) {
            MatrixTranspose(out, out);
        }
        return out;
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20: {   // the skin's world matrix times the view and the projection
        if (skin == nullptr) {
            return nullptr;
        }
        CopyMatrix(out, At<const float*>(skin, 0x2C));
        flags(0x1E, 0x20, 0x1F, 0x20, invert, transpose);
        float* view = Renderer_GetViewMatrix(renderer);
        float* projection = Renderer_GetProjectionMatrix(renderer);
        MatrixMultiply(out, out, view);
        MatrixMultiply(out, out, projection);
        if (invert) {
            MatrixInverse(out, out);
        }
        if (transpose) {
            MatrixTranspose(out, out);
        }
        return out;
    }
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:   // the skin's world matrix
        if (skin == nullptr) {
            return nullptr;
        }
        CopyMatrix(out, At<const float*>(skin, 0x2C));
        flags(0x61, 0x63, 0x62, 0x63, invert, transpose);
        if (invert) {
            MatrixInverse(out, out);
        }
        if (transpose) {
            MatrixTranspose(out, out);
        }
        return out;
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:   // the skin's bone matrices
        if (skin == nullptr) {
            return nullptr;
        }
        return At<void*>(skin, 0x28);
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x58:
    case 0x59:
    case 0x5A:
    case 0x5B:
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44: {   // a map's texture transform: base, dark, detail, gloss, glow, bump, normal,
                   // parallax, decal (by the entry's index), shader (by its index); the third and
                   // fourth of each transposed (the second's inversion not done)
        const float* matrix = nullptr;
        uint8_t turn = 0;
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint8_t* map;
            uint32_t base;
            if (id >= 0x41 && id <= 0x44) {
                map = TexturingProperty_GetShaderMap(texturing, entry->extra);
                base = 0x41;
            } else {
                re::Ptr32<uint8_t>* maps = At<re::Ptr32<uint8_t>*>(texturing, 0x20);
                if (id >= 0x58 && id <= 0x5B) {
                    map = maps[6];
                    base = 0x58;
                } else if (id >= 0x5C && id <= 0x5F) {
                    map = maps[7];
                    base = 0x5C;
                } else if (id >= 0x3D && id <= 0x40) {
                    map = maps[8 + entry->extra];
                    base = 0x3D;
                } else {
                    base = 0x25 + ((id - 0x25) & ~3u);
                    map = maps[(id - 0x25) / 4];
                }
            }
            if (map != nullptr && At<void*>(map, 0xC) != nullptr) {
                matrix = TextureTransform_GetMatrix(At<void*>(map, 0xC));
                if (id == base + 2 || id == base + 3) {
                    turn = 1;
                }
            }
        }
        if (entry->columnMajor != 0) {
            turn = turn == 0;
        }
        Map_MakeTextureTransform(out, matrix, turn);
        return out;
    }
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C: {   // the material's colours (with its alpha); the buffer as it is without one
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return out;
        }
        int offset = id == 0x49 ? 0x28 : id == 0x4A ? 0x1C : id == 0x4B ? 0x34 : 0x40;
        Set4(out, At<float>(material, offset), At<float>(material, offset + 4), At<float>(material, offset + 8),
             At<float>(material, 0x50));
        return out;
    }
    case 0x4D: {   // the material's shininess (x4)
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return out;
        }
        Set4(out, At<float>(material, 0x4C), At<float>(material, 0x4C), At<float>(material, 0x4C),
             At<float>(material, 0x4C));
        return out;
    }
    case 0x4E:
    case 0x4F: {   // the camera's position, its direction (the inverse view's rows 3, 2)
        const float* inverse = Renderer_GetInverseViewMatrix(renderer);
        const float* row = id == 0x4E ? inverse + 12 : inverse + 8;
        Set4(out, row[0], row[1], row[2], row[3]);
        return out;
    }
    case 0x50:
    case 0x51:   // the bone matrix defaults (the entry's own data)
        return entry->data;
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56: {   // the time (the geometry's float extra data), its sine, cosine, tangent
        re::Ptr32<char> name;
        FixedString_Construct(&name, ms_pcTimeExtraData);
        uint8_t* extra = ObjectNET_GetExtraData(geometry, &name);
        FixedString_Destruct(&name);
        if (extra == nullptr) {
            return nullptr;
        }
        float time = At<float>(extra, 0xC);
        switch (id) {
        case 0x52:
            Set4(out, time, time, time, time);
            break;
        case 0x53: {
            float s = crt::Sinf(time);
            Set4(out, s, s, s, s);
            break;
        }
        case 0x54: {
            float c = crt::Cosf(time);
            Set4(out, c, c, c, c);
            break;
        }
        case 0x55: {
            float t = crt::Tanf(time);
            Set4(out, t, t, t, t);
            break;
        }
        default:
            out[0] = time;
            out[1] = crt::Sinf(time);
            out[2] = crt::Cosf(time);
            out[3] = crt::Tanf(time);
            break;
        }
        return out;
    }
    case 0x57: {   // the ambient lights' colours (times their dimmers) summed
        Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
        if (effects != nullptr) {
            for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, 0xC); node != nullptr;) {
                void* light = node[1];
                node = static_cast<re::Ptr32<void>*>(node[0]);
                uint8_t* ambient = DynamicCast(NiAmbientLight_RTTI, light);
                if (ambient != nullptr) {
                    float color[3];
                    Color_Times(reinterpret_cast<const float*>(ambient + 0xFC), color, At<float>(ambient, 0xF8));
                    out[0] = color[0] + out[0];
                    out[1] = color[1] + out[1];
                    out[2] = out[2] + color[2];
                }
            }
        }
        return out;
    }
    case 0x64: {   // the fog's: 1 / ((far - near) * its depth)
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        float nearPlane;
        float farPlane;
        Renderer_GetCameraNearAndFar(renderer, &nearPlane, &farPlane);
        float value = 1.0f / ((farPlane - nearPlane) * At<float>(fog, 0x1C));
        Set4(out, value, value, value, value);
        return out;
    }
    case 0x65: {   // the fog's start and end
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        float nearPlane;
        float farPlane;
        Renderer_GetCameraNearAndFar(renderer, &nearPlane, &farPlane);
        float range = farPlane - nearPlane;
        range = range * At<float>(fog, 0x1C);
        float start = farPlane - range;
        float end = Renderer_GetMaxFogFactor(renderer) * range + farPlane;
        out[0] = start;
        out[1] = end;
        out[2] = 0.0f;
        out[3] = 0.0f;
        return out;
    }
    case 0x66: {   // the fog's colour
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        Set4(out, At<float>(fog, 0x20), At<float>(fog, 0x24), At<float>(fog, 0x28), 1.0f);
        return out;
    }
    case 0x69: {   // the parallax map's offset
        float offset = 0.0f;
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint8_t* map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[7];
            if (map != nullptr) {
                offset = At<float>(map, 0x10);
            }
        }
        out[0] = offset;
        return out;
    }
    case 0x67: {   // the bump map's matrix
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint8_t* map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[5];
            if (map != nullptr) {
                Set4(out, At<float>(map, 0x18), At<float>(map, 0x1C), At<float>(map, 0x20), At<float>(map, 0x24));
                return out;
            }
        }
        Set4(out, 1.0f, 1.0f, 1.0f, 1.0f);
        return out;
    }
    case 0x68: {   // the bump map's luma offset and scale
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint8_t* map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[5];
            if (map != nullptr) {
                out[0] = At<float>(map, 0x14);
                out[1] = At<float>(map, 0x10);
                return out;
            }
        }
        out[0] = 0.0f;
        out[1] = 1.0f;
        return out;
    }
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x6F:
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73: {   // a map's texture's width and height (the buffer as it is without texturing)
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing == nullptr) {
            return out;
        }
        uint8_t* map;
        if (id == 0x72) {
            map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[8 + entry->extra];
        } else if (id == 0x73) {
            map = TexturingProperty_GetShaderMap(texturing, entry->extra);
        } else {
            map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[id - 0x6A];
        }
        void* texture = map != nullptr ? At<void*>(map, 8) : nullptr;
        if (texture == nullptr) {
            out[0] = 0.0f;
            out[1] = 0.0f;
            return out;
        }
        out[0] = float(double(Virtual<uint32_t>(texture, 18)));   // GetWidth
        out[1] = float(double(Virtual<uint32_t>(texture, 19)));   // GetHeight
        return out;
    }
    case 0x74: {   // the alpha test function as three flags (less, equal, greater: a guess)
        uint32_t v[3] = {NiPoint3_ZERO[0], NiPoint3_ZERO[1], NiPoint3_ZERO[2]};
        float* f = reinterpret_cast<float*>(v);
        uint8_t* alpha = At<uint8_t*>(state, 8);
        if (alpha != nullptr) {
            switch ((At<uint16_t>(alpha, 0x18) >> 10) & 7) {
            case 1:
                f[0] = 1.0f;
                f[2] = 1.0f;
                break;
            case 2:
                f[0] = 1.0f;
                f[1] = 1.0f;
                break;
            case 3:
                f[0] = 1.0f;
                break;
            case 4:
                f[1] = 1.0f;
                f[2] = 1.0f;
                break;
            case 5:
                f[2] = 1.0f;
                break;
            case 6:
                f[1] = 1.0f;
                break;
            case 7:
                f[0] = 1.0f;
                f[1] = 1.0f;
                f[2] = 1.0f;
                break;
            default:
                break;
            }
        }
        out[0] = f[0];
        out[1] = f[1];
        out[2] = f[2];
        return out;
    }
    case 0x75: {   // the alpha test reference (0..1)
        float reference = 0.0f;
        uint8_t* alpha = At<uint8_t*>(state, 8);
        if (alpha != nullptr) {
            reference = float(At<uint8_t>(alpha, 0x1A)) / 255.0f;   // (exact operands: one rounding, as the x87 has it)
        }
        out[0] = reference;
        return out;
    }
    default:
        return nullptr;
    }
}

// 0x8BD6C0 (thiscall): an attribute entry's value: the geometry's extra data of its key (found
// through the SCM extra data's cache when there is one, else looked up and cached), as the
// attribute's type has it (into the buffer, or a pointer at its data); the entry's own data
// without one
const void* __fastcall ConstantMap_GetAttributeConstant(uint8_t* self, re::Edx, ConstantMapEntry* entry,
                                                        void* geometry, void*, void*, void*, const void*,
                                                        const void*, uint32_t pass, int global, uint8_t* scm,
                                                        uint32_t* out) {
    uint8_t* extra;
    if (scm != nullptr) {
        uint32_t type = At<uint32_t>(self, 8);
        uint32_t cursor = At<uint32_t>(scm, 0x18 + int(type) * 4);
        uint32_t* slot = reinterpret_cast<uint32_t*>(At<uint8_t*>(scm, 0x24 + int(type) * 4) + cursor * 8);
        bool cached = false;
        extra = nullptr;
        if (slot[0] == ((pass << (ms_uiEntryShift & 0x1F)) | entry->reg)) {
            extra = reinterpret_cast<uint8_t*>(slot[1]);
            At<uint32_t>(scm, 0x18 + int(type) * 4) = cursor + 1;
            cached = extra != nullptr && At<char*>(extra, 8) == entry->key;
        }
        if (!cached) {
            extra = ObjectNET_GetExtraData(geometry, &entry->key);
            if (extra == nullptr) {
                return entry->data;
            }
            AttributeCache_Add(scm, entry->reg, pass, At<uint32_t>(self, 8), extra, global);
        }
    } else {
        extra = ObjectNET_GetExtraData(geometry, &entry->key);
    }
    if (extra == nullptr) {
        return entry->data;
    }
    switch (ms_auiGlobalTypes[entry->flags & 0xFF]) {
    case 1:
        *out = At<uint8_t>(extra, 0xC);
        return out;
    case 3:
        *out = At<uint32_t>(extra, 0xC);
        return out;
    case 4:
        *reinterpret_cast<float*>(out) = At<float>(extra, 0xC);
        return out;
    case 7:
        if (IsKindOf(NiFloatsExtraData_RTTI, extra) || !IsKindOf(NiColorExtraData_RTTI, extra)) {
            return At<void*>(extra, 0x10);
        }
        [[fallthrough]];
    case 10: {
        float* f = reinterpret_cast<float*>(out);
        f[0] = At<float>(extra, 0xC);
        f[1] = At<float>(extra, 0x10);
        f[2] = At<float>(extra, 0x14);
        f[3] = At<float>(extra, 0x18);
        return out;
    }
    case 5:
    case 6:
    case 8:
    case 9:
    case 12:
    case 13:
        return At<void*>(extra, 0x10);
    case 14:
        if (IsKindOf(NiFloatsExtraData_RTTI, extra) || IsKindOf(NiIntegersExtraData_RTTI, extra)) {
            return At<void*>(extra, 0x10);
        }
        *out = At<uint8_t>(extra, 0xC);
        return out;
    default:
        return nullptr;
    }
}

namespace {

// an operator's operand: its type and value (null when it can't have one)
const void* OperatorOperand(void* self, ConstantMapEntry* operand, uint32_t& type, void* geometry, void* skin,
                            void* state, void* effects, const void* world, const void* bound, uint32_t pass,
                            int global, void* extra, float* buffer, bool& known) {
    known = true;
    switch (operand->flags & 0xF0000000u) {
    case 0x20000000u:
        type = GetPredefinedMappingTypeByName(operand->key);
        return Map_GetPredefinedConstant(self, operand, geometry, skin, state, effects, world, bound, pass, buffer);
    case 0x40000000u:
    case 0x10000000u:
        return operand->data;
    case 0x30000000u:
        return Map_GetAttributeConstant(self, operand, geometry, skin, state, effects, world, bound, pass, global,
                                        extra, buffer);
    default:
        known = false;
        return nullptr;
    }
}

} // namespace

// 0x8BD870 (thiscall): an operator entry's value: its two operand entries' (by index: the extra's
// low 12 bits and the next 12) combined by its operation (the top nibble: 1 multiply, 2 divide, 3
// add, 4 subtract; bit 24 invert, bit 25 transpose the result)
const void* __fastcall ConstantMap_GetOperatorConstant(void* self, re::Edx, ConstantMapEntry* entry, void* geometry,
                                                       void* skin, void* state, void* effects, const void* world,
                                                       const void* bound, uint32_t pass, int global, void* extra,
                                                       float* out) {
    uint32_t operands = entry->extra;
    uint32_t operation = operands & 0xF0000000u;
    uint8_t transpose = uint8_t((operands >> 25) & 1);
    uint8_t invert = uint8_t((operands >> 24) & 1);
    if (entry->columnMajor != 0) {
        transpose = transpose == 0;
    }
    ConstantMapEntry* a = Virtual<ConstantMapEntry*>(self, 11, operands & 0xFFF);
    ConstantMapEntry* b = Virtual<ConstantMapEntry*>(self, 11, (operands >> 12) & 0xFFF);
    if (a == nullptr || b == nullptr) {
        return nullptr;
    }
    uint32_t typeA = ms_auiGlobalTypes[a->flags & 0xFF];
    uint32_t typeB = ms_auiGlobalTypes[b->flags & 0xFF];
    alignas(16) float bufferA[16];
    alignas(16) float bufferB[16];
    bool known;
    const void* dataA = OperatorOperand(self, a, typeA, geometry, skin, state, effects, world, bound, pass, global,
                                        extra, bufferA, known);
    if (!known || dataA == nullptr) {
        return nullptr;
    }
    const void* dataB = OperatorOperand(self, b, typeB, geometry, skin, state, effects, world, bound, pass, global,
                                        extra, bufferB, known);
    if (!known || dataB == nullptr || typeA == 0 || typeB == 0) {
        return nullptr;
    }
    const float* fa = static_cast<const float*>(dataA);
    const float* fb = static_cast<const float*>(dataB);
    switch (operation) {
    case 0x10000000u:
        return Operator_Multiply(fa, typeA, fb, typeB, invert, transpose, out);
    case 0x20000000u:
        return Operator_Divide(fa, typeA, fb, typeB, invert, transpose, out);
    case 0x30000000u:
        return Operator_Add(fa, typeA, fb, typeB, invert, transpose, out);
    case 0x40000000u:
        return Operator_Subtract(fa, typeA, fb, typeB, invert, transpose, out);
    default:
        return nullptr;
    }
}

// 0x8BDB60 (cdecl): an object mapping's value from the light or texture effect, into out (whether
// there was one; with none, the mapping's default). 1 dimmer, 2..4 ambient / diffuse / specular,
// 5..7 the same times the dimmer, 8 / 9 the world / model position, 10 / 11 the world / model
// direction, 12 / 13 the world / model transform, 14 a spot's (cos outer, cos inner, exponent),
// 15 a point light's (+0x124), 16 / 17 a texture effect's world / model projection matrix, 18 / 19
// its world / model projection translation, 20 / 21 its world / model clipping plane, 22 its
// texture type, 23 / 24 its world / model projection transform, 25 / 27 / 28 / 29 Bully's (the
// object's source at +0xD4: a matrix, a texture's size, its own function). The matrices made
// transposed for a column major variable (Bully's 25 the other way round)
uint32_t __cdecl ConstantMap_ComputeObjectConstant(ConstantMapEntry* entry, uint32_t mapping, uint8_t* object,
                                                   void* geometry, void*, void* state, void* effects,
                                                   const NiTransform* world, const float* bound, uint32_t pass,
                                                   float* out) {
    float p[3];
    NiTransform inverse;
    bool columnMajor = entry->columnMajor != 0;
    switch (mapping) {
    case 1:   // the dimmer
        if (object != nullptr && !IsTextureEffect(object)) {
            out[0] = At<float>(object, 0xF8);
            return 1;
        }
        out[0] = 0.0f;
        return 0;
    case 2:
    case 3:
    case 4: {   // the ambient, diffuse, specular colour
        if (object == nullptr || IsTextureEffect(object)) {
            break;
        }
        int offset = mapping == 2 ? 0xFC : mapping == 3 ? 0x108 : 0x114;
        Set4(out, At<float>(object, offset), At<float>(object, offset + 4), At<float>(object, offset + 8), 1.0f);
        return 1;
    }
    case 5:
    case 6:
    case 7: {   // the same times the dimmer
        if (object == nullptr || IsTextureEffect(object)) {
            break;
        }
        int offset = mapping == 5 ? 0xFC : mapping == 6 ? 0x108 : 0x114;
        p[0] = At<float>(object, offset);
        p[1] = At<float>(object, offset + 4);
        p[2] = At<float>(object, offset + 8);
        Point3_ScaleInPlace(p, At<float>(object, 0xF8));
        Set4(out, p[0], p[1], p[2], 1.0f);
        return 1;
    }
    case 8:   // the world position (a directional light's far along its direction)
        if (object == nullptr) {
            break;
        }
        if (!IsDirectional(object)) {
            Set4(out, At<float>(object, 0x84), At<float>(object, 0x88), At<float>(object, 0x8C), 1.0f);
            return 1;
        }
        FarPosition(object, p);
        Set4(out, p[0], p[1], p[2], 1.0f);
        return 1;
    case 9: {   // the model position
        if (object == nullptr) {
            break;
        }
        Transform_Invert(world, &inverse);
        float r[3];
        if (IsDirectional(object)) {
            float farPoint[3];
            FarPosition(object, farPoint);
            Transform_TimesPoint(&inverse, r, farPoint);
        } else {
            Transform_TimesPoint(&inverse, r, reinterpret_cast<const float*>(object + 0x84));
        }
        Set4(out, r[0], r[1], r[2], 1.0f);
        return 1;
    }
    case 10:
    case 11: {   // the world direction (towards the bound for a point light, unitized); the model
                 // direction (the world rotation's transpose times it)
        if (object == nullptr || IsTextureEffect(object)) {
            Set4(out, 1.0f, 0.0f, 0.0f, 1.0f);
            return 0;
        }
        if (IsPoint(object)) {
            float t[3];
            Point_Minus(bound, t, reinterpret_cast<const float*>(object + 0x84));
            p[0] = t[0];
            p[1] = t[1];
            p[2] = t[2];
            Point_Unitize(p);
        } else if (IsDirectional(object)) {
            p[0] = At<float>(object, 0x124);
            p[1] = At<float>(object, 0x128);
            p[2] = At<float>(object, 0x12C);
        } else {
            p[0] = At<float>(object, 0x130);
            p[1] = At<float>(object, 0x134);
            p[2] = At<float>(object, 0x138);
        }
        if (mapping == 11) {
            float t[9];
            float r[3];
            Matrix3_TimesPoint(Matrix3_Transpose(world, t), r, p);
            p[0] = r[0];
            p[1] = r[1];
            p[2] = r[2];
        }
        Set4(out, p[0], p[1], p[2], 1.0f);
        return 1;
    }
    case 12:   // the world transform
        if (object == nullptr) {
            Identity(out);
            return 0;
        }
        if (!IsDirectional(object)) {
            if (columnMajor) {
                TransformToD3DTransposed(out, object + 0x60);
            } else {
                TransformToD3D(out, object + 0x60);
            }
            return 1;
        }
        FarPosition(object, p);
        if (columnMajor) {
            MakeD3DMatrixTransposed(out, object + 0x60, p, At<float>(object, 0x90));
        } else {
            MakeD3DMatrix(out, object + 0x60, p, At<float>(object, 0x90));
        }
        return 1;
    case 13: {   // the model transform (the inverse world times its world)
        if (object == nullptr) {
            Identity(out);
            return 0;
        }
        Transform_Invert(world, &inverse);
        NiTransform product;
        if (!IsDirectional(object)) {
            Transform_Times(&inverse, &product, object + 0x60);
        } else {
            NiTransform light = At<NiTransform>(object, 0x60);
            FarPosition(object, &light.translate.x);
            Transform_Times(&inverse, &product, &light);
        }
        if (columnMajor) {
            TransformToD3DTransposed(out, &product);
        } else {
            TransformToD3D(out, &product);
        }
        return 1;
    }
    case 14:   // a spot's: the cosines of its outer and inner angles, its exponent
        if (object != nullptr && IsSpot(object)) {
            float angle = At<float>(object, 0x140) * float(kDegreesToRadiansA);
            angle = angle / float(kDegreesToRadiansB);
            out[0] = crt::Cosf(angle);
            angle = At<float>(object, 0x13C) * float(kDegreesToRadiansA);
            angle = angle / float(kDegreesToRadiansB);
            out[1] = crt::Cosf(angle);
            out[2] = At<float>(object, 0x144);
            out[3] = 0.0f;
            return 1;
        }
        Set4(out, -1.0f, -1.0f, 0.0f, 0.0f);
        return 0;
    case 15:   // a point light's (+0x124)
        if (object != nullptr && IsPoint(object)) {
            Set4(out, At<float>(object, 0x124), At<float>(object, 0x128), At<float>(object, 0x12C), 0.0f);
            return 1;
        }
        Set4(out, 1.0f, 0.0f, 0.0f, 0.0f);
        return 0;
    case 16:   // the world projection matrix (+0x128)
        if (object == nullptr || !IsTextureEffect(object)) {
            Identity(out);
            return 0;
        }
        if (columnMajor) {
            MakeD3DMatrixTransposed(out, object + 0x128, NiPoint3_ZERO, 1.0f);
        } else {
            MakeD3DMatrix(out, object + 0x128, NiPoint3_ZERO, 1.0f);
        }
        return 1;
    case 17: {   // the model projection matrix (the world rotation's transpose times it)
        if (object == nullptr || !IsTextureEffect(object)) {
            Identity(out);
            return 0;
        }
        float t[9];
        float r[9];
        Matrix3_TimesMatrix(Matrix3_Transpose(world, t), r, reinterpret_cast<const float*>(object + 0x128));
        if (columnMajor) {
            MakeD3DMatrixTransposed(out, r, NiPoint3_ZERO, 1.0f);
        } else {
            MakeD3DMatrix(out, r, NiPoint3_ZERO, 1.0f);
        }
        return 1;
    }
    case 18:   // the world projection translation (+0x14C)
        if (object == nullptr || !IsTextureEffect(object)) {
            break;
        }
        Set4(out, At<float>(object, 0x14C), At<float>(object, 0x150), At<float>(object, 0x154), 1.0f);
        return 1;
    case 19: {   // the model projection translation
        if (object == nullptr || !IsTextureEffect(object)) {
            break;
        }
        Transform_Invert(world, &inverse);
        p[0] = At<float>(object, 0x14C);
        p[1] = At<float>(object, 0x150);
        p[2] = At<float>(object, 0x154);
        float r[3];
        Transform_TimesPoint(&inverse, r, p);
        Set4(out, r[0], r[1], r[2], 1.0f);
        return 1;
    }
    case 20:   // the world clipping plane (when it clips)
        if (object == nullptr || !IsTextureEffect(object)) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        if (At<uint8_t>(object, 0x16C) == 0) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        }
        Set4(out, At<float>(object, 0x180), At<float>(object, 0x184), At<float>(object, 0x188), At<float>(object, 0x18C));
        return 1;
    case 21: {   // the model clipping plane: the normal rotated, the constant from a point on it
        if (object == nullptr || !IsTextureEffect(object)) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        if (At<uint8_t>(object, 0x16C) == 0) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        }
        Transform_Invert(world, &inverse);
        float plane[4] = {At<float>(object, 0x180), At<float>(object, 0x184), At<float>(object, 0x188),
                          At<float>(object, 0x18C)};
        float normal[3];
        Matrix3_TimesPoint(inverse.rotate.m[0], normal, plane);
        float point[3] = {plane[3] * plane[0], plane[1] * plane[3], plane[3] * plane[2]};
        float modelPoint[3];
        Transform_TimesPoint(&inverse, modelPoint, point);
        out[0] = normal[0];
        out[1] = normal[1];
        out[2] = normal[2];
        out[3] = Point_Dot(normal, modelPoint);
        return 1;
    }
    case 22:   // the texture type (+0x168): 2 for 0 / 1, 3 for 2 / 3, 1 for 4 (only the first float)
        if (object == nullptr || !IsTextureEffect(object)) {
            out[0] = 0.0f;
            return 0;
        }
        switch (At<uint32_t>(object, 0x168)) {
        case 0:
        case 1:
            out[0] = 2.0f;
            return 1;
        case 2:
        case 3:
            out[0] = 3.0f;
            return 1;
        case 4:
            out[0] = 1.0f;
            return 1;
        default:
            out[0] = 0.0f;
            return 1;
        }
    case 23:   // the world projection transform (+0x128 rotation, +0x14C translation)
        if (object == nullptr || !IsTextureEffect(object)) {
            Identity(out);
            return 0;
        }
        if (columnMajor) {
            MakeD3DMatrixTransposed(out, object + 0x128, object + 0x14C, 1.0f);
        } else {
            MakeD3DMatrix(out, object + 0x128, object + 0x14C, 1.0f);
        }
        return 1;
    case 24: {   // the model projection transform
        if (object == nullptr || !IsTextureEffect(object)) {
            Identity(out);
            return 0;
        }
        Transform_Invert(world, &inverse);
        p[0] = At<float>(object, 0x14C);
        p[1] = At<float>(object, 0x150);
        p[2] = At<float>(object, 0x154);
        float translate[3];
        Transform_TimesPoint(&inverse, translate, p);
        p[0] = translate[0];
        p[1] = translate[1];
        p[2] = translate[2];
        float t[9];
        float r[9];
        Matrix3_TimesMatrix(Matrix3_Transpose(world, t), r, reinterpret_cast<const float*>(object + 0x128));
        if (columnMajor) {
            MakeD3DMatrixTransposed(out, r, p, 1.0f);
        } else {
            MakeD3DMatrix(out, r, p, 1.0f);
        }
        return 1;
    }
    case 25: {   // Bully's: the matrix its source finds for the geometry (transposed for a row
                 // major variable)
        if (object == nullptr) {
            Identity(out);
            return 0;
        }
        uint8_t* source = At<uint8_t*>(object, 0xD4);
        auto find = re::PtrCast<uint8_t*(__cdecl*)(void*, void*, uint32_t)>(At<void*>(source, 0x30));
        uint8_t* found = find(source, geometry, 0xFF);
        const float* matrix = reinterpret_cast<const float*>(
            At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(found, 0xC), 0x50), 8), 0x20) + 0xC4);
        if (columnMajor) {
            CopyMatrix(out, matrix);
        } else {
            MatrixTranspose(out, matrix);
        }
        return 1;
    }
    case 27: {   // Bully's: the size of the texture its source finds for the geometry
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        uint8_t* source = At<uint8_t*>(object, 0xD4);
        auto find = re::PtrCast<uint8_t*(__cdecl*)(void*, void*, uint32_t)>(At<void*>(source, 0x30));
        uint8_t* found = find(source, geometry, 0xFF);
        out[0] = float(double(Virtual<uint32_t>(At<void*>(found, 8), 18)));   // NiTexture::GetWidth
        out[1] = float(double(Virtual<uint32_t>(At<void*>(found, 8), 19)));   // GetHeight
        out[2] = 0.0f;
        out[3] = 0.0f;
        return 1;
    }
    case 28:
    case 29:   // Bully's: its source's own
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        return ObjectSource_GetConstant(At<void*>(object, 0xD4), out, 0x10, geometry, 0xFF, mapping, state, effects,
                                        world, bound, pass);
    default:   // (26 and anything else: nothing written)
        return 0;
    }
    Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
    return 0;
}

// 0x8BF190 (thiscall): an object entry's value (the effect it names from the dynamic effects;
// null for a mapping without a type, else the buffer, whatever was written)
const void* __fastcall ConstantMap_GetObjectConstant(void*, re::Edx, ConstantMapEntry* entry, void* geometry,
                                                     void* skin, void* state, void* effects, const void* world,
                                                     const void* bound, uint32_t pass, float* out) {
    uint8_t* object =
        GetDynamicEffectForObject(effects, ms_auiObjectTypes[(entry->flags >> 8) & 0xFF], entry->extra);
    uint32_t mapping = entry->mapping;
    int32_t index = int32_t(mapping) < 0x1E ? int32_t(mapping) : 0x1E;
    if (ms_auiObjectMappings[index * 3 + 1] == 0) {
        return nullptr;
    }
    Map_ComputeObjectConstant(entry, mapping, object, geometry, skin, state, effects, world, bound, pass, out);
    return out;
}

#define GB44N_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

GB44N_REVERSED("NiD3D10ShaderConstantMap::MultiplyOperator", 0x008BB5F0, ConstantMap_MultiplyOperator);
GB44N_REVERSED("NiD3D10ShaderConstantMap::DivideOperator", 0x008BB900, ConstantMap_DivideOperator);
GB44N_REVERSED("NiD3D10ShaderConstantMap::AddOperator", 0x008BBBA0, ConstantMap_AddOperator);
GB44N_REVERSED("NiD3D10ShaderConstantMap::SubtractOperator", 0x008BBD50, ConstantMap_SubtractOperator);
GB44N_REVERSED("NiD3D10ShaderConstantMap::GetPredefinedConstant", 0x008BC6D0, ConstantMap_GetPredefinedConstant);
GB44N_REVERSED("NiD3D10ShaderConstantMap::GetAttributeConstant", 0x008BD6C0, ConstantMap_GetAttributeConstant);
GB44N_REVERSED("NiD3D10ShaderConstantMap::GetOperatorConstant", 0x008BD870, ConstantMap_GetOperatorConstant);
GB44N_REVERSED("NiD3D10ShaderConstantMap::ComputeObjectConstant", 0x008BDB60, ConstantMap_ComputeObjectConstant);
GB44N_REVERSED("NiD3D10ShaderConstantMap::GetObjectConstant", 0x008BF190, ConstantMap_GetObjectConstant);
} // namespace ni::c44
