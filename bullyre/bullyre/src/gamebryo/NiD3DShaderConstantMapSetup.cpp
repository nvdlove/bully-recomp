// NiD3DShaderConstantMap (vtable 0x99C348, the D3D shader programs' constant map; 0x44 bytes:
// NiShaderConstantMap's +0x8 the program type, +0xC a primitive array, +0x1C the entries
// (NiTObjectArray<NiShaderConstantMapEntryPtr>), +0x34 the last error, then +0x38 the device,
// +0x3C the renderer, +0x40 the render state), the part of it in this chunk: the object
// constants' values (each light's or texture effect's colours, position, direction, transform,
// attenuation, clipping plane... as a float4 or a 4x4 matrix), SetObjectConstant,
// SetAttributeConstant, SetOperatorConstant and its operands (a predefined value, an attribute, a
// global constant: into the two static operand slots), InsertEntry and the destructor. Plus
// NiShaderConstantMapEntry's deleting destructor.
//
// Its slots (the ones known; the names chunk 39's files use): 11 GetEntryAtIndex, 14 InsertEntry,
// 16 SetDefinedConstant, 17 SetConstantConstant, 18 SetAttributeConstant, 19 SetGlobalConstant,
// 20 SetOperatorConstant, 21 SetupObjectEntry, 22 SetObjectConstant. A program's slot 14
// (NiD3DShaderProgram::SetShaderConstant(entry, data, count)) takes the values.

#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c40 {
namespace {

using Matrix = float[16];

constexpr uintptr_t kConstantMapVtable = 0x0099C348;
constexpr uintptr_t kObjectArrayVtable = 0x009893C0;     // NiTObjectArray<NiShaderConstantMapEntryPtr>
constexpr uintptr_t kPrimitiveArrayVtable = 0x00961988;  // NiTPrimitiveArray
constexpr uintptr_t kRefObjectVtable = 0x008FEFD4;
constexpr uintptr_t NiAmbientLight_RTTI = 0x020E3480;
constexpr uintptr_t NiFloatsExtraData_RTTI = 0x020E2ABC;
constexpr uintptr_t NiIntegersExtraData_RTTI = 0x020E2B60;
constexpr float kFar = -1048576.0f;   // (a directional light's "position": its direction times this)

GAME_VAR(uint32_t, ms_uiObjects, 0x020E2168);           // NiRefObject's count
GAME_VAR(const char*, ms_pcTimeExtraData, 0x00BC8B20);  // the extra data the time constants read
GAME_ARRAY(const float, NiPoint3_ZERO, 0x020E2D44);

// the maths (NiPoint3 / NiMatrix3 / NiTransform), at their addresses
GAME_FN(void, __thiscall, Color_TimesEquals, 0x0088A480, float* self, float scale);
GAME_FN(float*, __thiscall, Color_Times, 0x0074A400, const float* self, float* result, float scale);
GAME_FN(NiTransform*, __thiscall, Transform_Invert, 0x00770950, const NiTransform* self, NiTransform* out);
GAME_FN(float*, __thiscall, Transform_TimesPoint, 0x00450B10, const NiTransform* self, float* out, const float* p);
GAME_FN(NiTransform*, __thiscall, Transform_Times, 0x00406830, const NiTransform* self, NiTransform* out,
        const NiTransform* other);
GAME_FN(float*, __thiscall, Point_Minus, 0x0040D9C0, const float* self, float* out, const float* other);
GAME_FN(float, __thiscall, Point_Unitize, 0x00415130, float* self);
GAME_FN(float, __thiscall, Point_Dot, 0x00468CD0, const float* self, const float* other);
GAME_FN(float*, __thiscall, Matrix3_Transpose, 0x00768C40, const float* self, float* out);
GAME_FN(float*, __thiscall, Matrix3_TimesPoint, 0x00768A70, const float* self, float* out, const float* p);
GAME_FN(float*, __thiscall, Matrix3_TimesMatrix, 0x00768920, const float* self, float* out, const float* other);
// a transform as a D3D matrix (the rotation transposed, times the scale; the translation)
GAME_FN(void, __cdecl, TransformToD3D, 0x008B60A0, float* out, const NiTransform* transform);
GAME_FN(void, __cdecl, PartsToD3D, 0x00406670, float* out, const float* rotate, const float* translate, float scale);
// a texture transform's matrix for D3D (rows 2 or 4)
GAME_FN(void, __cdecl, TextureTransform_ToD3D, 0x0088B440, float* out, const float* matrix, int invert,
        int transpose, uint32_t rows);
GAME_FN(const float*, __thiscall, TextureTransform_GetMatrix, 0x0074A440, void* self);
GAME_FN(uint8_t*, __thiscall, TexturingProperty_GetShaderMap, 0x0074A7E0, void* self, uint32_t index);
// the rest of the library
GAME_FN(void*, __cdecl, GetDynamicEffectForObject, 0x0077F380, void* effects, uint32_t objectType, uint32_t index);
GAME_FN(uint32_t, __cdecl, GetPredefinedMappingTypeByName, 0x0077F2F0, const char* name);
GAME_FN(uint8_t*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(void, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(uint8_t*, __cdecl, DynamicCast, 0x00401FF0, uintptr_t rtti, const void* object);
GAME_FN(uint32_t, __cdecl, ShaderFactory_ReportError, 0x00756700, int error, int recoverable, const char* format, ...);
GAME_FN(void, __cdecl, ShaderFactory_ReleaseGlobalConstant, 0x00756B40, const re::Ptr32<char>* name);
GAME_FN(uint32_t, __thiscall, ObjectArray_AddFirstEmpty, 0x007606C0, void* array, const re::Ptr32<ConstantMapEntry>* entry);
GAME_FN(void, __thiscall, ObjectArray_RemoveAll, 0x0040E7A0, void* array);
GAME_FN(void, __cdecl, NiDeleteArray, 0x00836F80, void* p);
GAME_FN(void, __thiscall, Entry_Destruct, 0x0088B580, ConstantMapEntry* self);
GAME_FN(void, __thiscall, ShaderConstantMap_Construct, 0x0077F0C0, void* self, uint32_t programType);   // NiShaderConstantMap's
GAME_FN(void, __thiscall, Map_SetD3DRenderer, 0x0088A5A0, void* self, void* renderer);   // (the other chunk's)
// NiD3DShaderConstantMap's own (the other chunk's)
GAME_FN(void*, __thiscall, Map_GetAttributeData, 0x0088BFB0, void* self, ConstantMapEntry* entry, void* extra);
GAME_FN(void*, __thiscall, Map_GetConstantData, 0x0088C200, void* self, ConstantMapEntry* entry);
GAME_FN(uint32_t, __thiscall, Map_SetEntryData, 0x0088BF50, void* self, void* program, ConstantMapEntry* entry,
        uintptr_t data);
GAME_FN(uint32_t, __thiscall, Map_PadRegisters, 0x0088C610, void* self, ConstantMapEntry* entry, void* data);
// the operators (multiply, divide, add, subtract by the operator kind)
using Operator = uint32_t(__thiscall*)(void* self, void* program, ConstantMapEntry* entry, uint32_t type0,
                                       uint32_t type1, int inverse, int transpose);
GAME_FN(uint32_t, __thiscall, Map_OperatorMultiply, 0x0088AE00, void* self, void* program, ConstantMapEntry* entry,
        uint32_t type0, uint32_t type1, int inverse, int transpose);
GAME_FN(uint32_t, __thiscall, Map_OperatorDivide, 0x0088B1D0, void* self, void* program, ConstantMapEntry* entry,
        uint32_t type0, uint32_t type1, int inverse, int transpose);
GAME_FN(uint32_t, __thiscall, Map_OperatorAdd, 0x0088A050, void* self, void* program, ConstantMapEntry* entry,
        uint32_t type0, uint32_t type1, int inverse, int transpose);
GAME_FN(uint32_t, __thiscall, Map_OperatorSubtract, 0x0088A260, void* self, void* program, ConstantMapEntry* entry,
        uint32_t type0, uint32_t type1, int inverse, int transpose);
// the shader attribute cache a program may pass (the other chunk's)
GAME_FN(uint8_t*, __thiscall, AttributeCache_Find, 0x0088A4B0, void* self, uint32_t shaderRegister, uint32_t pass,
        uint32_t programType, uint32_t arg4);
GAME_FN(void, __thiscall, AttributeCache_Add, 0x00888790, void* self, uint32_t shaderRegister, uint32_t pass,
        uint32_t programType, void* extra, uint32_t arg5);
// the Bully object's constant source (+0xD4 of the object: its +0x44 function)
GAME_FN(uint8_t, __thiscall, ObjectSource_GetConstant, 0x0088A560, void* self, float* out, uintptr_t bytes,
        void* geometry, uintptr_t mask, uintptr_t mapping, void* state, void* effects, const NiTransform* world,
        const NiBound* worldBound, uintptr_t pass);
// this file's (called where modes apply)
GAME_FN(uint8_t, __cdecl, Map_GetObjectConstant, 0x0088D930, float* out, uint32_t bytes, uint32_t mapping,
        uint8_t* object, void* geometry, void* skin, void* partition, void* rendererData, void* state, void* effects,
        const NiTransform* world, const NiBound* worldBound, uint32_t pass);
GAME_FN(uint32_t, __thiscall, Map_SetupPredefinedOperand, 0x0088E900, void* self, uint32_t index,
        ConstantMapEntry* entry, void* geometry, uint8_t* skin, uint8_t* state, uint8_t* effects,
        const NiTransform* world, const NiBound* worldBound);
GAME_FN(uint32_t, __thiscall, Map_SetupGlobalOperand, 0x0088F7F0, void* self, uint32_t index,
        ConstantMapEntry* entry, uint32_t pass);
GAME_FN(uint32_t, __thiscall, Map_SetupAttributeOperand, 0x0088FA40, void* self, uint32_t index,
        ConstantMapEntry* entry, void* geometry, void* skin, void* state, const NiTransform* world,
        const NiBound* worldBound, uint32_t pass);
GAME_FN(uint32_t, __thiscall, Map_GetAttributeTextureData, 0x0088FD00, void* self, ConstantMapEntry* entry,
        uint8_t* extra);

inline float* OperandVector(uint32_t index) {
    return ms_afOperandVectors + index * 4;
}
inline float* OperandMatrix(uint32_t index) {
    return ms_afOperandMatrices + index * 16;
}
inline void Set4(float* out, float x, float y, float z, float w) {
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}
inline void CopyMatrix(float* to, const float* from) {
    for (int i = 0; i < 16; i++) {
        to[i] = from[i];
    }
}

// a light's type (NiDynamicEffect: +0xC5; 2/5 point, 3/6 directional, 4/7 spot: the second of
// each its shadowed kind)
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
inline const NiTransform* WorldTransform(const uint8_t* object) {   // NiAVObject's world (+0x60)
    return reinterpret_cast<const NiTransform*>(object + 0x60);
}
inline const float* WorldTranslate(const uint8_t* object) {
    return reinterpret_cast<const float*>(object + 0x84);
}
// a directional light's "position": its world direction times -2^20
inline void FarPosition(const uint8_t* object, float* p) {
    p[0] = At<float>(object, 0x124) * kFar;
    p[1] = At<float>(object, 0x128) * kFar;
    p[2] = At<float>(object, 0x12C) * kFar;
}
// a light's world direction: towards the bound for a point light (unitized), a directional
// light's (+0x124), a spot's (+0x130)
inline void LightDirection(const uint8_t* object, const NiBound* worldBound, float* d) {
    if (IsPoint(object)) {
        float t[3];
        Point_Minus(reinterpret_cast<const float*>(worldBound), t, WorldTranslate(object));
        d[0] = t[0];
        d[1] = t[1];
        d[2] = t[2];
        Point_Unitize(d);
    } else if (IsDirectional(object)) {
        d[0] = At<float>(object, 0x124);
        d[1] = At<float>(object, 0x128);
        d[2] = At<float>(object, 0x12C);
    } else {
        d[0] = At<float>(object, 0x130);
        d[1] = At<float>(object, 0x134);
        d[2] = At<float>(object, 0x138);
    }
}

} // namespace

// 0x88D930 (cdecl): NiD3DShaderConstantMap::GetObjectConstant (an object mapping's value from the
// light or texture effect: written to out, true when there was an object; with none, the
// mapping's default). 1 dimmer, 2..4 ambient / diffuse / specular, 5..7 the same times the dimmer,
// 8 / 9 the world / model position, 10 / 11 the world / model direction, 12 / 13 the world /
// model transform, 14 a spot's (exponent, cutoff...), 15 a directional light's direction, 16 / 17
// a texture effect's world / model projection matrix, 18 / 19 its world / model projection
// translation, 20 / 21 its world / model clipping plane, 22 its texture type, 23 / 24 its world
// / model projection transform, 25 / 27 / 28 / 29 Bully's (the object's source at +0xD4: a
// matrix, a texture's size, its own function); anything else zeros
uint8_t __cdecl NiD3DShaderConstantMap_GetObjectConstant(float* out, uint32_t bytes, uint32_t mapping,
                                                          uint8_t* object, void* geometry, void*, void*, void*,
                                                          void* state, void* effects, const NiTransform* world,
                                                          const NiBound* worldBound, uint32_t pass) {
    alignas(16) Matrix identity;
    crt::Memset(identity, 0, sizeof(identity));
    identity[15] = 1.0f;
    identity[10] = 1.0f;
    identity[5] = 1.0f;
    identity[0] = 1.0f;
    alignas(16) Matrix m;
    float p[3];
    NiTransform inverse;
    switch (mapping) {
    case 1:   // the dimmer
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        Set4(out, At<float>(object, 0xF8), 0.0f, 0.0f, 0.0f);
        return 1;
    case 2:
    case 3:
    case 4: {   // the ambient, diffuse, specular colour
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
            return 0;
        }
        int offset = mapping == 2 ? 0xFC : mapping == 3 ? 0x108 : 0x114;
        Set4(out, At<float>(object, offset), At<float>(object, offset + 4), At<float>(object, offset + 8), 1.0f);
        return 1;
    }
    case 5:
    case 6:
    case 7: {   // the same times the dimmer
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
            return 0;
        }
        int offset = mapping == 5 ? 0xFC : mapping == 6 ? 0x108 : 0x114;
        p[0] = At<float>(object, offset);
        p[1] = At<float>(object, offset + 4);
        p[2] = At<float>(object, offset + 8);
        Color_TimesEquals(p, At<float>(object, 0xF8));
        Set4(out, p[0], p[1], p[2], 1.0f);
        return 1;
    }
    case 8:   // the world position
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
            return 0;
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
            Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
            return 0;
        }
        Transform_Invert(world, &inverse);
        float r[3];
        if (IsDirectional(object)) {
            float far[3];
            FarPosition(object, far);
            Transform_TimesPoint(&inverse, r, far);
        } else {
            Transform_TimesPoint(&inverse, r, WorldTranslate(object));
        }
        Set4(out, r[0], r[1], r[2], 1.0f);
        return 1;
    }
    case 10:   // the world direction
        if (object == nullptr) {
            Set4(out, 1.0f, 0.0f, 0.0f, 1.0f);
            return 0;
        }
        LightDirection(object, worldBound, p);
        Set4(out, p[0], p[1], p[2], 1.0f);
        return 1;
    case 11: {   // the model direction (the world rotation's transpose times it)
        if (object == nullptr) {
            Set4(out, 1.0f, 0.0f, 0.0f, 1.0f);
            return 0;
        }
        LightDirection(object, worldBound, p);
        float t[9];
        float r[3];
        Matrix3_TimesPoint(Matrix3_Transpose(world->rotate.m[0], t), r, p);
        Set4(out, r[0], r[1], r[2], 1.0f);
        return 1;
    }
    case 12:   // the world transform
        if (object == nullptr) {
            CopyMatrix(out, identity);
            return 0;
        }
        if (!IsDirectional(object)) {
            TransformToD3D(m, WorldTransform(object));
        } else {
            FarPosition(object, p);
            PartsToD3D(m, WorldTransform(object)->rotate.m[0], p, WorldTransform(object)->scale);
        }
        CopyMatrix(out, m);
        return 1;
    case 13: {   // the model transform (the inverse world times its world)
        if (object == nullptr) {
            CopyMatrix(out, identity);
            return 0;
        }
        Transform_Invert(world, &inverse);
        NiTransform product;
        if (IsDirectional(object)) {
            NiTransform light = *WorldTransform(object);
            FarPosition(object, &light.translate.x);
            // (the original builds this product over its identity matrix's stack space)
            Transform_Times(&inverse, &product, &light);
        } else {
            Transform_Times(&inverse, &product, WorldTransform(object));
        }
        TransformToD3D(m, &product);
        CopyMatrix(out, m);
        return 1;
    }
    case 14:   // a spot's (+0x14C, +0x148, +0x144)
        if (object == nullptr) {
            Set4(out, -1.0f, -1.0f, 0.0f, 0.0f);
            return 0;
        }
        if (!IsSpot(object)) {
            Set4(out, -1.0f, -1.0f, 0.0f, 0.0f);
            return 1;
        }
        Set4(out, At<float>(object, 0x14C), At<float>(object, 0x148), At<float>(object, 0x144), 0.0f);
        return 1;
    case 15:   // (+0x124, a point light's)
        if (object == nullptr) {
            Set4(out, 1.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        if (!IsPoint(object)) {
            Set4(out, 1.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        }
        Set4(out, At<float>(object, 0x124), At<float>(object, 0x128), At<float>(object, 0x12C), 0.0f);
        return 1;
    case 16:   // the world projection matrix (+0x128)
        if (object == nullptr) {
            CopyMatrix(out, identity);
            return 0;
        }
        PartsToD3D(m, reinterpret_cast<const float*>(object + 0x128), NiPoint3_ZERO, 1.0f);
        CopyMatrix(out, m);
        return 1;
    case 17: {   // the model projection matrix (the world rotation's transpose times it)
        if (object == nullptr) {
            CopyMatrix(out, identity);
            return 0;
        }
        float t[9];
        float r[9];
        Matrix3_TimesMatrix(Matrix3_Transpose(world->rotate.m[0], t), r, reinterpret_cast<const float*>(object + 0x128));
        PartsToD3D(m, r, NiPoint3_ZERO, 1.0f);
        CopyMatrix(out, m);
        return 1;
    }
    case 18:   // the world projection translation (+0x14C)
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
            return 0;
        }
        Set4(out, At<float>(object, 0x14C), At<float>(object, 0x150), At<float>(object, 0x154), 1.0f);
        return 1;
    case 19: {   // the model projection translation
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 1.0f);
            return 0;
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
        if (object == nullptr) {
            Set4(out, 1.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        if (At<uint8_t>(object, 0x16C) == 0) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        }
        Set4(out, At<float>(object, 0x180), At<float>(object, 0x184), At<float>(object, 0x188), At<float>(object, 0x18C));
        return 1;
    case 21: {   // the model clipping plane: the normal rotated, the constant from a point on it
        if (object == nullptr) {
            Set4(out, 1.0f, 0.0f, 0.0f, 0.0f);
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
    case 22:   // the texture type (+0x168): 2 for 0 / 1, 3 for 2 / 3, 1 for 4
        if (object == nullptr) {
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 0;
        }
        switch (At<uint32_t>(object, 0x168)) {
        case 0:
        case 1:
            Set4(out, 2.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        case 2:
        case 3:
            Set4(out, 3.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        case 4:
            Set4(out, 1.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        default:
            Set4(out, 0.0f, 0.0f, 0.0f, 0.0f);
            return 1;
        }
    case 23:   // the world projection transform (+0x128 rotation, +0x14C translation)
        if (object == nullptr) {
            CopyMatrix(out, identity);
            return 0;
        }
        PartsToD3D(m, reinterpret_cast<const float*>(object + 0x128), reinterpret_cast<const float*>(object + 0x14C),
                   1.0f);
        CopyMatrix(out, m);
        return 1;
    case 24: {   // the model projection transform
        if (object == nullptr) {
            CopyMatrix(out, identity);
            return 0;
        }
        Transform_Invert(world, &inverse);
        p[0] = At<float>(object, 0x14C);
        p[1] = At<float>(object, 0x150);
        p[2] = At<float>(object, 0x154);
        float translate[3];
        Transform_TimesPoint(&inverse, translate, p);
        float t[9];
        float r[9];
        Matrix3_TimesMatrix(Matrix3_Transpose(world->rotate.m[0], t), r, reinterpret_cast<const float*>(object + 0x128));
        PartsToD3D(m, r, translate, 1.0f);
        CopyMatrix(out, m);
        return 1;
    }
    case 25: {   // Bully's: the matrix its source finds for the geometry (transposed)
        if (object == nullptr) {
            CopyMatrix(out, identity);
            return 0;
        }
        uint8_t* source = At<uint8_t*>(object, 0xD4);
        auto find = re::PtrCast<uint8_t*(__cdecl*)(void*, void*, uint32_t)>(At<void*>(source, 0x30));
        uint8_t* found = find(source, geometry, 0xFF);
        uint8_t* holder = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(found, 0xC), 0x50), 8), 0x20);
        alignas(16) Matrix copy;
        crt::MemcpyS(copy, sizeof(copy), holder + 0xC4, 0x40);
        alignas(16) Matrix transposed;
        MatrixTranspose(transposed, copy);
        CopyMatrix(out, transposed);
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
        out[0] = float(Virtual<uint32_t>(At<void*>(found, 8), 18));   // NiTexture::GetWidth
        out[1] = float(Virtual<uint32_t>(At<void*>(found, 8), 19));   // GetHeight
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
                                        world, worldBound, pass);
    default:
        crt::Memset(out, 0, bytes);
        return 0;
    }
}

// 0x88E830 (thiscall, the map unused): NiD3DShaderConstantMap::SetObjectConstant (the object the
// entry names found among the dynamic effects, its value given to the program: 0 set, 0x10 a
// mapping that isn't valid, 0x60 no such object, 0x50 the program refused it)
uint32_t __fastcall NiD3DShaderConstantMap_SetObjectConstant(void*, re::Edx, void* program, ConstantMapEntry* entry,
                                                            void* geometry, void* skin, void* partition,
                                                            void* rendererData, void* state, void* effects,
                                                            const NiTransform* world, const NiBound* worldBound,
                                                            uint32_t pass) {
    uint32_t result = 0;
    uint8_t* object = static_cast<uint8_t*>(
        GetDynamicEffectForObject(effects, ms_auiObjectTypes[(entry->flags >> 8) & 0xFF], entry->extra));
    uint32_t mapping = entry->internal;
    uint32_t row = int(mapping) < 0x1E ? mapping : 0x1E;
    if (ms_akObjectMappings[row * 3 + 2] == 0) {
        return 0x10;
    }
    if (Map_GetObjectConstant(ms_afObjectValue, ms_akObjectMappings[row * 3 + 1] * 16, mapping, object, geometry,
                              skin, partition, rendererData, state, effects, world, worldBound, pass) == 0) {
        result = 0x60;
    }
    if (Virtual<uint8_t>(program, 14, entry, static_cast<float*>(ms_afObjectValue), 0) == 0) {
        result = 0x50;
    }
    return result;
}

// 0x88E900 (thiscall): NiD3DShaderConstantMap::SetupPredefinedOperand (an operator constant's
// operand from a predefined mapping, into operand slot `index`; returns its attribute type: 9 a
// matrix, 7 a point4, 10 a colour, 4 a float, 0 none, 0x10 no time extra data). The renderer's
// world / view / projection and their products (as is, inverted, transposed), the skin's world
// matrix, the texture transforms, the material's colours, the camera, the time, the ambient
// lights, the fog, the alpha blend and test
uint32_t __fastcall NiD3DShaderConstantMap_SetupPredefinedOperand(uint8_t* self, re::Edx, uint32_t index,
                                                                  ConstantMapEntry* entry, void* geometry,
                                                                  uint8_t* skin, uint8_t* state, uint8_t* effects,
                                                                  const NiTransform*, const NiBound*) {
    uint32_t mapping = entry->internal;
    uint8_t* renderer = At<uint8_t*>(self, 0x3C);
    float* matrix = OperandMatrix(index);
    float* vector = OperandVector(index);
    alignas(16) Matrix a;
    alignas(16) Matrix b;
    alignas(16) Matrix c;
    alignas(16) Matrix ab;
    bool invert = false;
    bool transpose = false;
    switch (mapping) {
    case 0x1:
    case 0x2:
    case 0xD:
    case 0xE:   // the projection
        invert = mapping == 0x2 || mapping == 0xE;
        transpose = mapping == 0xD || mapping == 0xE;
        CopyMatrix(a, reinterpret_cast<const float*>(renderer + 0x7D0));
        CopyMatrix(matrix, a);
        break;
    case 0x3:
    case 0x4:
    case 0xF:
    case 0x10:   // the view
        invert = mapping == 0x4 || mapping == 0x10;
        transpose = mapping == 0xF || mapping == 0x10;
        CopyMatrix(a, reinterpret_cast<const float*>(renderer + 0x790));
        CopyMatrix(matrix, a);
        break;
    case 0x5:
    case 0x6:
    case 0x11:
    case 0x12:   // the world
        invert = mapping == 0x6 || mapping == 0x12;
        transpose = mapping == 0x11 || mapping == 0x12;
        CopyMatrix(matrix, reinterpret_cast<const float*>(renderer + 0x750));
        break;
    case 0x7:
    case 0x8:
    case 0x13:
    case 0x14:   // world * view
        invert = mapping == 0x8 || mapping == 0x14;
        transpose = mapping == 0x13 || mapping == 0x14;
        CopyMatrix(a, reinterpret_cast<const float*>(renderer + 0x750));
        CopyMatrix(b, reinterpret_cast<const float*>(renderer + 0x790));
        MatrixMultiply(ab, a, b);
        CopyMatrix(matrix, ab);
        break;
    case 0x9:
    case 0xA:
    case 0x15:
    case 0x16:   // view * projection
        invert = mapping == 0xA || mapping == 0x16;
        transpose = mapping == 0x15 || mapping == 0x16;
        CopyMatrix(a, reinterpret_cast<const float*>(renderer + 0x790));
        CopyMatrix(b, reinterpret_cast<const float*>(renderer + 0x7D0));
        MatrixMultiply(ab, a, b);
        CopyMatrix(matrix, ab);
        break;
    case 0xB:
    case 0xC:
    case 0x17:
    case 0x18: {   // world * view * projection
        invert = mapping == 0xC || mapping == 0x18;
        transpose = mapping == 0x17 || mapping == 0x18;
        CopyMatrix(a, reinterpret_cast<const float*>(renderer + 0x750));
        CopyMatrix(b, reinterpret_cast<const float*>(renderer + 0x790));
        CopyMatrix(c, reinterpret_cast<const float*>(renderer + 0x7D0));
        MatrixMultiply(ab, a, b);
        alignas(16) Matrix abc;
        MatrixMultiply(abc, ab, c);
        CopyMatrix(matrix, abc);
        break;
    }
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:   // the skin's world matrix times the view
        if (skin == nullptr) {
            return 0;
        }
        CopyMatrix(a, At<const float*>(skin, 0x2C));
        CopyMatrix(matrix, a);
        invert = mapping == 0x1A || mapping == 0x1C;
        transpose = mapping == 0x1B || mapping == 0x1C;
        CopyMatrix(b, reinterpret_cast<const float*>(renderer + 0x790));
        MatrixMultiply(matrix, matrix, b);
        break;
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:   // the skin's world matrix times the view and the projection
        if (skin == nullptr) {
            return 0;
        }
        CopyMatrix(a, At<const float*>(skin, 0x2C));
        CopyMatrix(matrix, a);
        invert = mapping == 0x1E || mapping == 0x20;
        transpose = mapping == 0x1F || mapping == 0x20;
        CopyMatrix(b, reinterpret_cast<const float*>(renderer + 0x790));
        CopyMatrix(c, reinterpret_cast<const float*>(renderer + 0x7D0));
        MatrixMultiply(matrix, matrix, b);
        MatrixMultiply(matrix, matrix, c);
        break;
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:   // the skin's world matrix
        if (skin == nullptr) {
            return 0;
        }
        CopyMatrix(a, At<const float*>(skin, 0x2C));
        CopyMatrix(matrix, a);
        invert = mapping == 0x61 || mapping == 0x63;
        transpose = mapping == 0x62 || mapping == 0x63;
        break;
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
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44: {   // a map's texture transform (4 rows): base, dark (the bump map's, as the
                   // original has it), detail, gloss, glow, bump, decal (by the entry's index),
                   // shader (by its index); the second of each inverted, the third transposed
        const float* transform = nullptr;
        int flip = 0;
        int turn = 0;
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint32_t base = (mapping - 0x25) & ~3u;
            uint8_t* map;
            if (mapping >= 0x41) {
                map = TexturingProperty_GetShaderMap(texturing, entry->extra);
            } else if (mapping >= 0x3D) {
                map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[8 + entry->extra];
            } else {
                static const int kMaps[7] = {0, 5, 2, 3, 4, 5};
                map = At<re::Ptr32<uint8_t>*>(texturing, 0x20)[kMaps[base / 4]];
            }
            if (map != nullptr && At<void*>(map, 0xC) != nullptr) {
                transform = TextureTransform_GetMatrix(At<void*>(map, 0xC));
                uint32_t kind = (mapping - 0x25) & 3;
                flip = kind == 1 || kind == 3;
                turn = kind == 2 || kind == 3;
            }
        }
        TextureTransform_ToD3D(matrix, transform, flip, turn, 4);
        return 9;
    }
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C: {   // the material's colours (with its alpha)
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return 0;
        }
        int offset = mapping == 0x49 ? 0x28 : mapping == 0x4A ? 0x1C : mapping == 0x4B ? 0x34 : 0x40;
        Set4(vector, At<float>(material, offset), At<float>(material, offset + 4), At<float>(material, offset + 8),
             At<float>(material, 0x50));
        return 10;
    }
    case 0x4D: {   // the material's shininess (x4)
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return 0;
        }
        float s = At<float>(material, 0x4C);
        Set4(vector, s, s, s, s);
        return 10;
    }
    case 0x4E:
    case 0x4F: {   // the camera's position, its direction (the inverse view's rows 3, 2)
        CopyMatrix(a, reinterpret_cast<const float*>(renderer + 0x810));
        const float* row = mapping == 0x4E ? a + 12 : a + 8;
        Set4(vector, row[0], row[1], row[2], row[3]);
        return 7;
    }
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
            return 0x10;
        }
        float time = At<float>(extra, 0xC);
        float value;
        switch (mapping) {
        case 0x52:
            value = time;
            break;
        case 0x53:
            value = crt::Sinf(time);
            break;
        case 0x54:
            value = crt::Cosf(time);
            break;
        case 0x55:
            value = crt::Tanf(time);
            break;
        default:
            vector[0] = time;
            vector[1] = crt::Sinf(time);
            vector[2] = crt::Cosf(time);
            vector[3] = crt::Tanf(time);
            return 7;
        }
        Set4(vector, value, value, value, value);
        return 7;
    }
    case 0x57: {   // the ambient lights' colours (times their dimmers) summed
        Set4(vector, 0.0f, 0.0f, 0.0f, 1.0f);
        if (effects != nullptr) {
            for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, 0xC); node != nullptr;) {
                void* light = node[1];
                node = static_cast<re::Ptr32<void>*>(node[0]);
                uint8_t* ambient = DynamicCast(NiAmbientLight_RTTI, light);
                if (ambient != nullptr && At<uint8_t>(ambient, 0xC4) != 0) {
                    float color[3];
                    Color_Times(reinterpret_cast<const float*>(ambient + 0xFC), color, At<float>(ambient, 0xF8));
                    vector[0] = vector[0] + color[0];
                    vector[1] = vector[1] + color[1];
                    vector[2] = color[2] + vector[2];
                }
            }
        }
        return 7;
    }
    case 0x64: {   // the fog's depth scale (1 / ((far - near) * density))
        uint8_t* renderState = At<uint8_t*>(self, 0x40);
        float scale = 1.0f / ((At<float>(renderState, 0x7C) - At<float>(renderState, 0x78)) *
                              At<float>(At<uint8_t*>(state, 0x10), 0x1C));
        Set4(vector, scale, scale, scale, scale);
        return 7;
    }
    case 0x65: {   // the fog's start and end
        uint8_t* renderState = At<uint8_t*>(self, 0x40);
        float end = At<float>(renderState, 0x7C);
        vector[0] = At<float>(renderState, 0x78);
        vector[1] = end;
        vector[2] = 0.0f;
        vector[3] = 0.0f;
        return 7;
    }
    case 0x66: {   // the fog's colour
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        Set4(vector, At<float>(fog, 0x20), At<float>(fog, 0x24), At<float>(fog, 0x28), 1.0f);
        return 10;
    }
    case 0x74: {   // the alpha blend's source function as (r, g, b) flags
        float v[3] = {NiPoint3_ZERO[0], NiPoint3_ZERO[1], NiPoint3_ZERO[2]};
        uint8_t* alpha = At<uint8_t*>(state, 8);
        if (alpha != nullptr) {
            switch ((At<uint16_t>(alpha, 0x18) >> 10) & 7) {
            case 1:
                v[0] = 1.0f;
                v[2] = 1.0f;
                break;
            case 2:
                v[0] = 1.0f;
                v[1] = 1.0f;
                break;
            case 3:
                v[0] = 1.0f;
                break;
            case 4:
                v[1] = 1.0f;
                v[2] = 1.0f;
                break;
            case 5:
                v[2] = 1.0f;
                break;
            case 6:
                v[1] = 1.0f;
                break;
            case 7:
                v[0] = 1.0f;
                v[1] = 1.0f;
                v[2] = 1.0f;
                break;
            default:
                break;
            }
        }
        Set4(vector, v[0], v[1], v[2], 1.0f);
        return 7;
    }
    case 0x75: {   // the alpha test's reference (/ 255)
        float value = 0.0f;
        uint8_t* alpha = At<uint8_t*>(state, 8);
        if (alpha != nullptr) {
            value = float(int(At<uint8_t>(alpha, 0x1A))) / 255.0f;
        }
        Set4(vector, value, value, value, value);
        return 4;
    }
    default:
        return 0;
    }
    if (invert) {
        MatrixInverse(matrix);
    }
    if (transpose) {
        MatrixTranspose(matrix, matrix);
    }
    return 9;
}

// 0x88F7F0 (thiscall, the map unused): NiD3DShaderConstantMap::SetupGlobalOperand (an operator's
// operand from the entry's own data, by its attribute type, into operand slot `index`)
uint32_t __fastcall NiD3DShaderConstantMap_SetupGlobalOperand(void*, re::Edx, uint32_t index, ConstantMapEntry* entry,
                                                              uint32_t) {
    const float* data = static_cast<const float*>(entry->data);
    uint32_t type = AttributeType(entry);
    float* vector = OperandVector(index);
    float* matrix = OperandMatrix(index);
    switch (type) {
    case 3:
    case 4:
        vector[0] = data[0];
        return type;
    case 5:
        vector[0] = data[0];
        vector[1] = data[1];
        return 5;
    case 6:
        vector[0] = data[0];
        vector[1] = data[1];
        vector[2] = data[2];
        return 6;
    case 7:
    case 10:
        vector[0] = data[0];
        vector[1] = data[1];
        vector[2] = data[2];
        vector[3] = data[3];
        return type;
    case 8:   // a 3x3 widened
        matrix[0] = data[0];
        matrix[1] = data[1];
        matrix[2] = data[2];
        matrix[3] = 0.0f;
        matrix[4] = data[3];
        matrix[5] = data[4];
        matrix[6] = data[5];
        matrix[7] = 0.0f;
        matrix[8] = data[6];
        matrix[9] = data[7];
        matrix[10] = data[8];
        matrix[11] = 0.0f;
        matrix[12] = 0.0f;
        matrix[13] = 0.0f;
        matrix[14] = 0.0f;
        matrix[15] = 1.0f;
        return 8;
    case 9:
        for (int i = 0; i < 16; i++) {
            matrix[i] = data[i];
        }
        return 9;
    default:
        return 0;
    }
}

// 0x88FA40 (thiscall): NiD3DShaderConstantMap::SetupAttributeOperand (an operator's operand from
// the geometry's extra data of the entry's name: an unsigned int made a float, the rest copied;
// 1 without a geometry, 0x10 without the extra data)
uint32_t __fastcall NiD3DShaderConstantMap_SetupAttributeOperand(void* self, re::Edx, uint32_t index,
                                                                 ConstantMapEntry* entry, void* geometry, void*,
                                                                 void*, const NiTransform*, const NiBound*,
                                                                 uint32_t) {
    if (geometry == nullptr) {
        return 1;
    }
    uint8_t* extra = ObjectNET_GetExtraData(geometry, &entry->key);
    if (extra == nullptr) {
        return 0x10;
    }
    const uint32_t* data = static_cast<const uint32_t*>(Map_GetAttributeData(self, entry, extra));
    uint32_t type = AttributeType(entry);
    uint32_t* vector = reinterpret_cast<uint32_t*>(OperandVector(index));
    uint32_t* matrix = reinterpret_cast<uint32_t*>(OperandMatrix(index));
    switch (type) {
    case 3:
        OperandVector(index)[0] = float(data[0]);
        return 3;
    case 4:
        vector[0] = data[0];
        return 4;
    case 5:
        vector[0] = data[0];
        vector[1] = data[1];
        return 5;
    case 6:
        vector[0] = data[0];
        vector[1] = data[1];
        vector[2] = data[2];
        return 6;
    case 7:
    case 10:
        vector[0] = data[0];
        vector[1] = data[1];
        vector[2] = data[2];
        vector[3] = data[3];
        return type;
    case 8:   // a 3x3 widened
        matrix[0] = data[0];
        matrix[1] = data[1];
        matrix[2] = data[2];
        matrix[3] = 0;
        matrix[4] = data[3];
        matrix[5] = data[4];
        matrix[6] = data[5];
        matrix[7] = 0;
        matrix[8] = data[6];
        matrix[9] = data[7];
        matrix[10] = data[8];
        matrix[11] = 0;
        matrix[12] = 0;
        matrix[13] = 0;
        matrix[14] = 0;
        OperandMatrix(index)[15] = 1.0f;
        return 8;
    case 9:
        for (int i = 0; i < 16; i++) {
            matrix[i] = data[i];
        }
        return 9;
    default:
        return 0;
    }
}

// 0x88FCE0 (thiscall): NiShaderConstantMapEntry's deleting destructor (the D3D library's copy)
ConstantMapEntry* __fastcall NiShaderConstantMapEntry_DeletingDestructor(ConstantMapEntry* self, re::Edx, int flags) {
    Entry_Destruct(self);
    if (flags & 1) {
        NiDelete(self, 0x34);
    }
    return self;
}

// 0x88FD00 (thiscall): NiD3DShaderConstantMap::GetAttributeTextureData (a texture attribute's
// data: the entry's own without extra data, a floats' or integers' extra data's array; none for
// any other extra data)
uint32_t __fastcall NiD3DShaderConstantMap_GetAttributeTextureData(void* self, re::Edx, ConstantMapEntry* entry,
                                                                   uint8_t* extra) {
    if (extra == nullptr) {
        return Map_PadRegisters(self, entry, entry->data);
    }
    if (Virtual<uintptr_t>(extra, 2) == NiFloatsExtraData_RTTI) {
        return Map_PadRegisters(self, entry, At<void*>(extra, 0x10));
    }
    if (Virtual<uintptr_t>(extra, 2) == NiIntegersExtraData_RTTI) {
        return Map_PadRegisters(self, entry, At<void*>(extra, 0x10));
    }
    return 0;
}

// 0x88FD80 (thiscall): NiD3DShaderConstantMap::SetAttributeConstant (the geometry's extra data of
// the entry's name, through the program's attribute cache when it has one, given to the program;
// a texture attribute through the texture path; the entry's default without the extra data:
// 0 set, 1 no geometry, 0x10 no data, 0x40 data that doesn't fit, 0x50 the program refused it)
uint32_t __fastcall NiD3DShaderConstantMap_SetAttributeConstant(uint8_t* self, re::Edx, void* program,
                                                               ConstantMapEntry* entry, void* geometry, void*, void*,
                                                               void*, void*, void*, const NiTransform*,
                                                               const NiBound*, uint32_t pass, uint32_t arg11,
                                                               void* cache) {
    if (geometry == nullptr) {
        return 1;
    }
    uint8_t* extra;
    if (cache != nullptr) {
        extra = AttributeCache_Find(cache, entry->shaderRegister, pass, At<uint32_t>(self, 8), arg11);
        if (extra == nullptr || At<char*>(extra, 8) != entry->key) {
            extra = ObjectNET_GetExtraData(geometry, &entry->key);
            if (extra != nullptr) {
                AttributeCache_Add(cache, entry->shaderRegister, pass, At<uint32_t>(self, 8), extra, arg11);
            }
        }
    } else {
        extra = ObjectNET_GetExtraData(geometry, &entry->key);
    }
    if (AttributeType(entry) == 14) {   // a texture
        uint32_t data = Map_GetAttributeTextureData(self, entry, extra);
        return Map_SetEntryData(self, program, entry, data);
    }
    void* data;
    if (extra == nullptr) {
        data = Map_GetConstantData(self, entry);
        if (data == nullptr) {
            return 0x10;
        }
    } else {
        data = Map_GetAttributeData(self, entry, extra);
        if (data == nullptr) {
            return 0x40;
        }
    }
    switch (AttributeType(entry)) {
    case 9:
        if (Virtual<uint8_t>(program, 14, entry, data, 4) == 0) {
            return 0x50;
        }
        break;
    case 6:
    case 7:
    case 5:
    case 4:
    case 10:
    case 3:
    case 1:
        if (Virtual<uint8_t>(program, 14, entry, data, 0) == 0) {
            return 0x50;
        }
        break;
    case 8:
        if (Virtual<uint8_t>(program, 14, entry, data, 3) == 0) {
            return 0x50;
        }
        break;
    default:
        break;
    }
    return 0;
}

// 0x88FF10 (thiscall): NiD3DShaderConstantMap::SetOperatorConstant (the two operand entries (the
// entry's extra: their indices in its low 24 bits, inverse bit 24, transpose bit 25, the operator
// in the top nibble) evaluated into the operand slots, then the operator applied. Quirk kept: a
// constant first operand is read as an attribute, a constant second one as a global. 1 when an
// entry or operand is missing, reported)
uint32_t __fastcall NiD3DShaderConstantMap_SetOperatorConstant(uint8_t* self, re::Edx, void* program,
                                                              ConstantMapEntry* entry, void* geometry, uint8_t* skin,
                                                              uint8_t* state, uint8_t* effects,
                                                              const NiTransform* world, const NiBound* worldBound,
                                                              uint32_t pass) {
    uint32_t extra = entry->extra;
    int transpose = (extra >> 25) & 1;
    int inverse = (extra >> 24) & 1;
    uint32_t op = extra & 0xF0000000;
    ConstantMapEntry* first = Virtual<ConstantMapEntry*>(self, 11, extra & 0xFFF);
    ConstantMapEntry* second = Virtual<ConstantMapEntry*>(self, 11, (extra >> 12) & 0xFFF);
    if (first == nullptr || second == nullptr) {
        ShaderFactory_ReportError(1, 0, "Invalid entries in OperatorConstant\n");
        return 1;
    }
    uint32_t type0 = AttributeType(first);
    uint32_t type1 = AttributeType(second);
    switch (first->flags & 0xF0000000) {
    case 0x20000000:
        type0 = GetPredefinedMappingTypeByName(first->key);
        Map_SetupPredefinedOperand(self, 0, first, geometry, skin, state, effects, world, worldBound);
        break;
    case 0x40000000:
        Map_SetupGlobalOperand(self, 0, first, pass);
        break;
    case 0x30000000:
    case 0x10000000:
        Map_SetupAttributeOperand(self, 0, first, geometry, skin, state, world, worldBound, pass);
        break;
    default:
        break;
    }
    switch (second->flags & 0xF0000000) {
    case 0x20000000:
        type1 = GetPredefinedMappingTypeByName(second->key);
        Map_SetupPredefinedOperand(self, 1, second, geometry, skin, state, effects, world, worldBound);
        break;
    case 0x30000000:
        Map_SetupAttributeOperand(self, 1, second, geometry, skin, state, world, worldBound, pass);
        break;
    case 0x40000000:
    case 0x10000000:
        Map_SetupGlobalOperand(self, 1, second, pass);
        break;
    default:
        break;
    }
    if (type0 == 0 || type1 == 0) {
        ShaderFactory_ReportError(1, 0, "Invalid operands in OperatorConstant\n");
        return 1;
    }
    switch (op) {
    case 0x10000000:
        return Map_OperatorMultiply(self, program, entry, type0, type1, inverse, transpose);
    case 0x20000000:
        return Map_OperatorDivide(self, program, entry, type0, type1, inverse, transpose);
    case 0x30000000:
        return Map_OperatorAdd(self, program, entry, type0, type1, inverse, transpose);
    case 0x40000000:
        return Map_OperatorSubtract(self, program, entry, type0, type1, inverse, transpose);
    default:
        return 1;
    }
}

// 0x890200 (thiscall): NiD3DShaderConstantMap's destructor (the global constants its entries
// registered released, the entries, the device, the arrays)
void __fastcall NiD3DShaderConstantMap_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kConstantMapVtable;
    for (uint32_t i = 0; i < At<uint16_t>(self, 0x24); i++) {
        ConstantMapEntry* entry = At<re::Ptr32<ConstantMapEntry>*>(self, 0x20)[i];
        if (entry == nullptr) {
            continue;
        }
        Increment(&entry->refCount);
        if ((entry->flags & 0xF0000000) == 0x40000000) {
            re::Ptr32<char> name = ni::sys::FixedStringAddRef(entry->key);
            ShaderFactory_ReleaseGlobalConstant(&name);
            ni::sys::FixedStringRelease(&name);
        }
        if (Decrement(&entry->refCount) == 0) {
            Virtual<void>(entry, 1);
        }
    }
    ObjectArray_RemoveAll(self + 0x1C);
    void* device = At<void*>(self, 0x38);
    At<void*>(self, 0x3C) = nullptr;
    if (device != nullptr) {
        Com<long>(device, 2);   // Release
    }
    At<void*>(self, 0x38) = nullptr;
    At<void*>(self, 0x40) = nullptr;
    At<uint32_t>(self, 0x1C) = kObjectArrayVtable;
    NiDeleteArray(At<void*>(self, 0x20));
    At<uint32_t>(self, 0xC) = kPrimitiveArrayVtable;
    NiFree(At<void*>(self, 0x10));
    At<uint32_t>(self, 0) = kRefObjectVtable;
    Decrement(&ms_uiObjects);
}

// 0x890330 (thiscall): its deleting destructor
uint8_t* __fastcall NiD3DShaderConstantMap_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    NiD3DShaderConstantMap_Destruct(self, nullptr);
    if (flags & 1) {
        NiDelete(self, 0x44);
    }
    return self;
}

// 0x890350 (thiscall): NiD3DShaderConstantMap::InsertEntry (added at the first empty slot; the
// last error 0x30 when it can't be; returns the last error)
uint32_t __fastcall NiD3DShaderConstantMap_InsertEntry(uint8_t* self, re::Edx, ConstantMapEntry* entry) {
    re::Ptr32<ConstantMapEntry> held = entry;   // (the NiPointer the argument is)
    if (entry != nullptr) {
        Increment(&entry->refCount);
    }
    bool failed = ObjectArray_AddFirstEmpty(self + 0x1C, &held) == 0xFFFFFFFF;
    if (entry != nullptr && Decrement(&entry->refCount) == 0) {
        Virtual<void>(entry, 1);
    }
    if (failed) {
        At<uint32_t>(self, 0x34) = 0x30;
    }
    return At<uint32_t>(self, 0x34);
}

// 0x894150 (thiscall): NiD3DShaderConstantMap's constructor (NiShaderConstantMap's for the program
// type; the arrays empty; pointed at the renderer)
void* __fastcall NiD3DShaderConstantMap_Construct(uint8_t* self, re::Edx, void* renderer, uint32_t programType) {
    ShaderConstantMap_Construct(self, programType);
    At<uint32_t>(self, 0) = kConstantMapVtable;
    At<uint16_t>(self, 0x14) = 0;
    At<uint16_t>(self, 0x16) = 0;
    At<uint16_t>(self, 0x18) = 0;
    At<void*>(self, 0x10) = nullptr;
    At<uint32_t>(self, 0xC) = kPrimitiveArrayVtable;
    At<uint16_t>(self, 0x1A) = 1;
    At<uint16_t>(self, 0x2A) = 1;
    At<uint16_t>(self, 0x24) = 0;
    At<uint16_t>(self, 0x26) = 0;
    At<uint16_t>(self, 0x28) = 0;
    At<void*>(self, 0x20) = nullptr;
    At<uint32_t>(self, 0x1C) = kObjectArrayVtable;
    At<uint8_t>(self, 0x2C) = 1;
    At<uint32_t>(self, 0x30) = 0;
    At<uint32_t>(self, 0x34) = 0;
    At<uint32_t>(self, 0x38) = 0;
    At<uint32_t>(self, 0x3C) = 0;
    At<uint32_t>(self, 0x40) = 0;
    Map_SetD3DRenderer(self, renderer);
    ObjectArray_RemoveAll(self + 0x1C);
    return self;
}

#define C40A_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

C40A_REVERSED("NiD3DShaderConstantMap::GetObjectConstant", 0x0088D930, NiD3DShaderConstantMap_GetObjectConstant);
C40A_REVERSED("NiD3DShaderConstantMap::SetObjectConstant", 0x0088E830, NiD3DShaderConstantMap_SetObjectConstant);
C40A_REVERSED("NiD3DShaderConstantMap::SetupPredefinedOperand", 0x0088E900,
              NiD3DShaderConstantMap_SetupPredefinedOperand);
C40A_REVERSED("NiD3DShaderConstantMap::SetupGlobalOperand", 0x0088F7F0, NiD3DShaderConstantMap_SetupGlobalOperand);
C40A_REVERSED("NiD3DShaderConstantMap::SetupAttributeOperand", 0x0088FA40,
              NiD3DShaderConstantMap_SetupAttributeOperand);
C40A_REVERSED("NiShaderConstantMapEntry::DeletingDestructor_88FCE0", 0x0088FCE0,
              NiShaderConstantMapEntry_DeletingDestructor);
C40A_REVERSED("NiD3DShaderConstantMap::GetAttributeTextureData", 0x0088FD00,
              NiD3DShaderConstantMap_GetAttributeTextureData);
C40A_REVERSED("NiD3DShaderConstantMap::SetAttributeConstant", 0x0088FD80, NiD3DShaderConstantMap_SetAttributeConstant);
C40A_REVERSED("NiD3DShaderConstantMap::SetOperatorConstant", 0x0088FF10, NiD3DShaderConstantMap_SetOperatorConstant);
C40A_REVERSED("NiD3DShaderConstantMap::~NiD3DShaderConstantMap", 0x00890200, NiD3DShaderConstantMap_Destruct);
C40A_REVERSED("NiD3DShaderConstantMap::DeletingDestructor", 0x00890330, NiD3DShaderConstantMap_DeletingDestructor);
C40A_REVERSED("NiD3DShaderConstantMap::InsertEntry", 0x00890350, NiD3DShaderConstantMap_InsertEntry);
C40A_REVERSED("NiD3DShaderConstantMap::NiD3DShaderConstantMap", 0x00894150, NiD3DShaderConstantMap_Construct);

} // namespace ni::c40
