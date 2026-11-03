// NiD3DXEffectParameter (0x4C bytes, src/gamebryo/NiD3DXEffect.h): one parameter of an effect,
// made from the parser's description (its attribute added to the file's table, or a global shader
// constant registered) and set on the ID3DXEffect each draw: from a predefined mapping (the
// transforms, skinning, texture transforms, material, eye, time, fog, texture sizes...), an
// object (a light's or map's data through NiShaderConstantMap), a global constant, the geometry's
// extra data of its name, or a texture (its own file, a dynamic effect's, the texturing
// property's). The results: 0 set, 0x10 nothing to set, 0x50 failed, 0x60 no such object.

#include "gamebryo/NiD3DXEffect.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::d3dx;

using Matrix = float[16];

GAME_VAR(float*, ms_pfBoneMatrices, 0x020DCEA0);        // the skinning matrices handed to the effect
GAME_VAR(uint32_t, ms_uiBoneMatrices, 0x020DCEA4);
GAME_ARRAY(float, ms_afObjectData, 0x020DCE60);         // [16] an object constant's value
GAME_ARRAY(float, ms_afExpanded, 0x020DCEA8);           // the vector arrays widened to float4
GAME_ARRAY(uint32_t, ms_akObjectMappings, 0x020E3308);  // [31][3]: value type, registers, valid
GAME_VAR(const char*, ms_pcTimeExtraData, 0x00BC8B20);  // the extra data the time parameters read
constexpr uintptr_t NiAmbientLight_RTTI = 0x020E3480;
constexpr uintptr_t NiFloatsExtraData_RTTI = 0x020E2ABC;
constexpr uintptr_t NiColorExtraData_RTTI = 0x020E29DC;
// the sine and cosine series terms (the game's: -0.16161616 for -1/6)
GAME_VAR(float, kSinTerm1, 0x00950A9C);
GAME_VAR(float, kSinTerm2, 0x00950A98);
GAME_VAR(float, kSinTerm3, 0x00950A94);
GAME_VAR(float, kCosTerm0, 0x00900DFC);
GAME_VAR(float, kCosTerm1, 0x00950A90);
GAME_VAR(float, kCosTerm2, 0x00950A8C);
GAME_VAR(float, kCosTerm3, 0x00950A88);

GAME_FN(void, __cdecl, NiFree_, 0x00752380, void* p);
GAME_FN(void*, __cdecl, OperatorNew, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, OperatorDelete, 0x005EEAB0, void* p);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(void, __cdecl, NiD3DUtility_SetString, 0x00888530, re::Ptr32<char>* to, uintptr_t size, const char* from);
GAME_FN(void, __thiscall, FixedString_AssignChars, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<const char>* self);
GAME_FN(void*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<const char>* name);
GAME_FN(uint8_t, __cdecl, IsKindOf_, 0x004066F0, uintptr_t rtti, const void* object);
GAME_FN(void*, __cdecl, DynamicCast, 0x00401FF0, uintptr_t rtti, const void* object);
GAME_FN(void, __thiscall, TextureTransform_UpdateMatrix, 0x0077EB30, void* self);
// NiShaderFactory's global constants
GAME_FN(void, __cdecl, ShaderFactory_RegisterGlobalConstant, 0x00756A60, const re::Ptr32<const char>* name, uint32_t type,
        uint32_t size, const void* value);
GAME_FN(void, __cdecl, ShaderFactory_ReleaseGlobalConstant, 0x00756B40, const re::Ptr32<const char>* name);
GAME_FN(uint8_t, __cdecl, ShaderFactory_RetrieveGlobalConstant, 0x00756C10, const re::Ptr32<const char>* name, uint32_t* size,
        re::Ptr32<const void>* data);
// NiShaderConstantMap's
GAME_FN(void*, __cdecl, ConstantMap_GetDynamicEffectForObject, 0x0077F380, void* effects, uint32_t objectType,
        uint32_t index);
GAME_FN(uint8_t, __cdecl, ConstantMap_GetObjectConstant, 0x0088D930, float* out, uint32_t bytes, uint32_t mapping,
        void* object, void* geometry, void* skin, void* partition, void* rendererData, void* state, void* effects,
        void* world, void* worldBound, uint32_t pass);
// the texture transform's matrix as D3D wants it (2 or 4 rows: whether a vertex shader reads it)
GAME_FN(void, __cdecl, TextureTransform_ToD3D, 0x0088B440, float* out, const void* transform, int invert,
        int transpose, uint32_t rows);
// this library's, called where modes apply
GAME_FN(uint8_t, __cdecl, NiD3DXEffect_ExpandToVector4, 0x007520C0, const void* from, float* to,
        uint32_t registerClass, uint32_t columns, uint32_t count);
GAME_FN(void*, __thiscall, TextureTransform_GetMatrix, 0x0074A440, void* self);
GAME_FN(float*, __thiscall, Color_Times, 0x0074A400, const float* self, float* result, float scale);
GAME_FN(uint32_t, __thiscall, Parameter_SetObjectParameter, 0x0074A480, NiD3DXEffectParameter* self, void* effect,
        void* geometry, void* skin, void* partition, void* rendererData, void* state, void* effects, void* world,
        void* worldBound, uint32_t pass);
GAME_FN(void, __thiscall, Parameter_LoadTexture2D, 0x0074A6B0, NiD3DXEffectParameter* self);
GAME_FN(void, __thiscall, Parameter_LoadTexture3D, 0x0074A710, NiD3DXEffectParameter* self);
GAME_FN(void, __thiscall, Parameter_LoadTextureCube, 0x0074A780, NiD3DXEffectParameter* self);
GAME_FN(void*, __thiscall, TexturingProperty_GetShaderMap, 0x0074A7E0, void* self, uint32_t index);
GAME_FN(uint8_t, __thiscall, Parameter_SetPredefinedParameter, 0x0074B1B0, NiD3DXEffectParameter* self, void* effect,
        void* geometry, void* skin, void* partition, void* rendererData, void* state, void* effects, void* world,
        void* worldBound, uint32_t pass, int vertexShader);
GAME_FN(void*, __thiscall, Parameter_GetD3DTexture, 0x0074C540, NiD3DXEffectParameter* self, void* texturing,
        uint32_t flags);
// the attribute table's (this library's)
GAME_FN(uint8_t, __thiscall, Table_AddBool, 0x00751240, void* table, const char* name, int hidden, int value);
GAME_FN(uint8_t, __thiscall, Table_AddUnsignedInt, 0x007512A0, void* table, const char* name, int hidden,
        uint32_t value, uint32_t low, uint32_t high);
GAME_FN(uint8_t, __thiscall, Table_AddFloat, 0x00751320, void* table, const char* name, int hidden, float value,
        float low, float high);
GAME_FN(uint8_t, __thiscall, Table_AddPoint2, 0x007513B0, void* table, const char* name, int hidden,
        const float* value, const float* low, const float* high);
GAME_FN(uint8_t, __thiscall, Table_AddPoint3, 0x00751470, void* table, const char* name, int hidden,
        const float* value, const float* low, const float* high);
GAME_FN(uint8_t, __thiscall, Table_AddPoint4, 0x00751540, void* table, const char* name, int hidden,
        const float* value, const float* low, const float* high);
GAME_FN(uint8_t, __thiscall, Table_AddMatrix3, 0x007515C0, void* table, const char* name, int hidden,
        const float* value);
GAME_FN(uint8_t, __thiscall, Table_AddMatrix4, 0x00751670, void* table, const char* name, int hidden,
        const float* value);
GAME_FN(uint8_t, __thiscall, Table_AddColor, 0x007516D0, void* table, const char* name, int hidden,
        const float* value, const float* low, const float* high);
GAME_FN(uint8_t, __thiscall, Table_AddColorA, 0x007517A0, void* table, const char* name, int hidden,
        const float* value, const float* low, const float* high);
GAME_FN(uint8_t, __thiscall, Table_AddTexture, 0x00751880, void* table, const char* name, int hidden,
        uint32_t index, const char* file);
GAME_FN(uint8_t, __thiscall, Table_AddArray, 0x007518E0, void* table, const char* name, int hidden,
        const float* value, uint32_t elements, uint32_t elementSize, uint32_t attributeType, const float* low,
        const float* high);
GAME_FN(uint32_t, __thiscall, VectorDesc_GetType, 0x00751CE0, VectorDesc* self);
GAME_FN(uint32_t, __thiscall, ArrayDesc_GetType, 0x00751D20, ArrayDesc* self);
GAME_FN(uint32_t, __thiscall, ArrayDesc_GetElementSize, 0x00751D80, ArrayDesc* self);

inline uint8_t* Renderer() {
    return NiDX9Renderer_ms_pkRenderer;
}
inline uint8_t* RenderState(uint8_t* renderer) {
    return At<uint8_t*>(renderer, 0x6B4);
}
inline const float* RendererMatrix(int offset) {   // +0x750 world, +0x790 view, +0x7D0 projection
    return reinterpret_cast<const float*>(Renderer() + offset);
}
inline void CopyMatrix(float* to, const float* from) {
    crt::Memcpy(to, from, 64);
}
inline void Multiply(float* out, const float* a, const float* b) {
    Import<float*>(kD3DXMatrixMultiply, out, a, b);
}
inline void Invert(float* m) {
    Import<float*>(kD3DXMatrixInverse, m, static_cast<float*>(nullptr), m);
}

// the matrix set as it is or transposed
inline void SetMatrix(void* effect, const char* handle, const float* m, bool transpose) {
    Com<long>(effect, transpose ? kSetMatrixTranspose : kSetMatrix, handle, m);
}

// the texturing property's map (NiTexturingProperty: the maps' array at +0x20)
inline uint8_t* Map(uint8_t* texturing, int index) {
    return At<re::Ptr32<uint8_t>*>(texturing, 0x20)[index];
}

// a 3x3 or 3x4 matrix (rows of 3 or 4: the parameter's) widened to a 4x4 one
inline void WidenMatrix(float* m, const float* d, uint32_t rows, uint32_t columns) {
    m[0] = d[0];
    m[1] = d[1];
    m[2] = d[2];
    if (rows == 3) {
        m[3] = 0.0f;
        m[4] = d[3];
        m[5] = d[4];
        m[6] = d[5];
        m[7] = 0.0f;
        m[8] = d[6];
        m[9] = d[7];
        m[10] = d[8];
        m[11] = 0.0f;
        if (columns == 3) {
            m[12] = 0.0f;
            m[13] = 0.0f;
            m[14] = 0.0f;
        } else {
            m[12] = d[9];
            m[13] = d[10];
            m[14] = d[11];
        }
    } else {
        for (int i = 3; i < 12; i++) {
            m[i] = d[i];
        }
        m[12] = 0.0f;
        m[13] = 0.0f;
        m[14] = 0.0f;
    }
    m[15] = 1.0f;
}

// a float array set: plain floats, float4 vectors, or narrower vectors widened first
inline uint32_t SetFloatArray(void* effect, const char* handle, const float* data, uint32_t floats,
                              uint32_t vectors) {
    D3DXParameterDesc desc;
    Com<long>(effect, kGetParameterDesc, handle, &desc);
    if (desc.parameterType != 3) {   // D3DXPT_FLOAT
        return 0x10;
    }
    if (desc.parameterClass == 0) {   // D3DXPC_SCALAR
        Com<long>(effect, kSetFloatArray, handle, data, floats);
        return 0;
    }
    if (desc.parameterClass != 1) {   // D3DXPC_VECTOR
        return 0x10;
    }
    if (desc.columns == 4 && desc.rows == 1) {
        Com<long>(effect, kSetVectorArray, handle, data, vectors);
        return 0;
    }
    NiD3DXEffect_ExpandToVector4(data, ms_afExpanded, desc.rows, desc.columns, desc.elements);
    Com<long>(effect, kSetVectorArray, handle, static_cast<const float*>(ms_afExpanded), desc.elements * desc.rows);
    return 0;
}

// the skinning matrices buffer grown to the parameter's count
inline void GrowBoneMatrices(uint32_t count) {
    if (ms_uiBoneMatrices < count) {
        OperatorDelete(ms_pfBoneMatrices);
        uint64_t bytes = uint64_t(count) * 0x40;
        ms_pfBoneMatrices = static_cast<float*>(OperatorNew(bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : uint32_t(bytes)));
        ms_uiBoneMatrices = count;
    }
}

} // namespace

// 0x74A3C0: ReleaseTexture
void __fastcall NiD3DXEffectParameter_ReleaseTexture(NiD3DXEffectParameter* self, re::Edx) {
    if (self->texture != nullptr) {
        Com<unsigned long>(self->texture, kRelease);
        self->texture = nullptr;
    }
}

// 0x74A400: NiColor::operator* (a scale)
float* __fastcall NiColor_Times(const float* self, re::Edx, float* result, float scale) {
    result[0] = self[0];
    result[1] = self[1];
    result[2] = self[2];
    result[0] = scale * result[0];
    result[1] = scale * result[1];
    result[2] = scale * result[2];
    return result;
}

// 0x74A440: NiTextureTransform::GetMatrix (updated when it changed)
void* __fastcall NiTextureTransform_GetMatrix(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 0x1C) != 0) {
        TextureTransform_UpdateMatrix(self);
    }
    return self + 0x20;
}

// 0x74A460: LinkHandle (by name)
void __fastcall NiD3DXEffectParameter_LinkHandle(NiD3DXEffectParameter* self, re::Edx, void* effect) {
    self->handle = Com<const char*>(effect, kGetParameterByName, static_cast<const char*>(nullptr), self->name);
}

// 0x74A480: SetObjectParameter: an object's constant (the mapping's value type: a float, a vector
// or a matrix) through NiShaderConstantMap; 0x10 for a mapping it doesn't know, 0x60 when the
// object isn't there (defaults set), 0x50 for a value type it can't set
uint32_t __fastcall NiD3DXEffectParameter_SetObjectParameter(NiD3DXEffectParameter* self, re::Edx, void* effect,
                                                              void* geometry, void* skin, void* partition,
                                                              void* rendererData, void* state, void* effects,
                                                              void* world, void* worldBound, uint32_t pass) {
    void* object = ConstantMap_GetDynamicEffectForObject(effects, self->objectType, self->index);
    int32_t mapping = int32_t(self->objectMapping);
    int32_t entry = mapping < 0x1E ? mapping : 0x1E;
    uint32_t valueType = ms_akObjectMappings[entry * 3];
    if (ms_akObjectMappings[entry * 3 + 2] == 0) {
        return 0x10;
    }
    uint8_t ok = ConstantMap_GetObjectConstant(ms_afObjectData, ms_akObjectMappings[entry * 3 + 1] * 16,
                                               self->objectMapping, object, geometry, skin, partition, rendererData,
                                               state, effects, world, worldBound, pass);
    uint32_t result = ok ? 0 : 0x60;
    switch (valueType) {
    case 1:
        Com<long>(effect, kSetFloat, self->handle, ms_afObjectData[0]);
        return result;
    case 4: {
        float v[4] = {ms_afObjectData[0], ms_afObjectData[1], ms_afObjectData[2], ms_afObjectData[3]};
        Com<long>(effect, kSetVector, self->handle, v);
        return result;
    }
    case 12:
    case 16: {
        Matrix m;
        for (int i = 0; i < 16; i++) {
            m[i] = ms_afObjectData[i];
        }
        Com<long>(effect, kSetMatrix, self->handle, m);
        return result;
    }
    default:
        return 0x50;
    }
}

// 0x74A6B0 / 0x74A710 / 0x74A780: the texture made from its file (unless it is a procedural one:
// a target)
void __fastcall NiD3DXEffectParameter_LoadTexture2D(NiD3DXEffectParameter* self, re::Edx) {
    void* device = At<void*>(Renderer(), 0xA4);
    void* texture = nullptr;
    if (self->textureTarget != nullptr) {
        return;
    }
    long hr = 0;
    if (self->textureFile != nullptr) {
        hr = Import<long>(kD3DXCreateTextureFromFileExA, device, self->textureFile, self->rows, self->columns,
                          0xFFFFFFFFu, 0u, 0u, 1u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u, static_cast<void*>(nullptr),
                          static_cast<void*>(nullptr), &texture);
    }
    if (hr >= 0) {
        self->texture = texture;
    }
}
void __fastcall NiD3DXEffectParameter_LoadTexture3D(NiD3DXEffectParameter* self, re::Edx) {
    void* device = At<void*>(Renderer(), 0xA4);
    void* texture = nullptr;
    if (self->textureTarget != nullptr) {
        return;
    }
    long hr = 0;
    if (self->textureFile != nullptr) {
        hr = Import<long>(kD3DXCreateVolumeTextureFromFileExA, device, self->textureFile, self->rows, self->columns,
                          self->depth, 1u, 0u, 0u, 1u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u, static_cast<void*>(nullptr),
                          static_cast<void*>(nullptr), &texture);
    }
    if (hr >= 0) {
        self->texture = texture;
    }
}
void __fastcall NiD3DXEffectParameter_LoadTextureCube(NiD3DXEffectParameter* self, re::Edx) {
    void* device = At<void*>(Renderer(), 0xA4);
    void* texture = nullptr;
    if (self->textureTarget != nullptr) {
        return;
    }
    long hr = 0;
    if (self->textureFile != nullptr) {
        hr = Import<long>(kD3DXCreateCubeTextureFromFileExA, device, self->textureFile, self->rows, 0xFFFFFFFFu, 0u,
                          0u, 1u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u, static_cast<void*>(nullptr),
                          static_cast<void*>(nullptr), &texture);
    }
    if (hr >= 0) {
        self->texture = texture;
    }
}

// 0x74A7E0: NiTexturingProperty::GetShaderMap
void* __fastcall NiTexturingProperty_GetShaderMap(uint8_t* self, re::Edx, uint32_t index) {
    NiTArray<void*>* maps = At<NiTArray<void*>*>(self, 0x30);
    if (maps != nullptr && index < maps->size) {
        return maps->base[index];
    }
    return nullptr;
}

// 0x74A810: the constructor (the used flag left unset)
NiD3DXEffectParameter* __fastcall NiD3DXEffectParameter_Construct(NiD3DXEffectParameter* self, re::Edx) {
    self->handle = nullptr;
    self->name = ni::sys::kNullFixedString;
    self->global = 0;
    self->predefined = 0;
    self->objectMapping = 0;
    self->type = 0;
    self->textureType = 0;
    self->arrayCount = 0;
    self->maxCount = 0;
    self->index = 0;
    self->textureFile = nullptr;
    self->textureTarget = nullptr;
    self->textureFlags = 0;
    self->texture = nullptr;
    self->rows = 0;
    self->columns = 0;
    self->depth = 0;
    self->objectType = 0;
    return self;
}

// 0x74A850: the destructor: the texture, the global constant, the strings
void __fastcall NiD3DXEffectParameter_Destruct(NiD3DXEffectParameter* self, re::Edx) {
    if (self->texture != nullptr) {
        Com<unsigned long>(self->texture, kRelease);
        self->texture = nullptr;
    }
    if (self->global) {
        ShaderFactory_ReleaseGlobalConstant(&self->name);
    }
    NiFree_(self->textureFile);
    NiFree_(self->textureTarget);
    ni::sys::FixedStringRelease(re::PtrCast<re::Ptr32<char>*>(&self->name));
}

// 0x74A8D0: Initialize from the parser's description: the attribute (with its range when there is
// one) added to the file's table, or the value registered as a global constant
uint8_t __fastcall NiD3DXEffectParameter_Initialize(NiD3DXEffectParameter* self, re::Edx, ParameterDesc* desc,
                                                    void* table) {
    if (desc == nullptr) {
        return 0;
    }
    self->type = desc->type;
    self->global = desc->global;
    switch (desc->type) {
    case 1: {   // matrix
        MatrixDesc* d = static_cast<MatrixDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->rows = d->columns;
        self->columns = d->rows;
        self->arrayCount = self->rows * d->rows;
        if (d->rows == 3 && d->columns == 3) {
            if (self->global) {
                ShaderFactory_RegisterGlobalConstant(&self->name, 8, 0x30, d->value);
                return 1;
            }
            Table_AddMatrix3(table, self->name, d->hidden, d->value);
            return 1;
        }
        if (self->global) {
            ShaderFactory_RegisterGlobalConstant(&self->name, 9, 0x40, d->value);
            return 1;
        }
        Table_AddMatrix4(table, self->name, d->hidden, d->value);
        return 1;
    }
    case 2: {   // vector: a range when every maximum is at least its minimum and one is above
        VectorDesc* d = static_cast<VectorDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        uint32_t vectorType = VectorDesc_GetType(d);
        self->arrayCount = d->elements;
        uint32_t count = d->elements;
        const float* low = d->minimum;
        const float* high = d->maximum;
        bool equal = true;
        uint32_t i = 0;
        for (; i < count; i++) {
            if (d->maximum[i] < d->minimum[i]) {
                break;
            }
            if (d->maximum[i] > d->minimum[i]) {
                equal = false;
            }
        }
        if (count == 0 || equal || i < count) {
            low = nullptr;
            high = nullptr;
        }
        if (d->color) {
            if (self->global) {
                ShaderFactory_RegisterGlobalConstant(&self->name, 10, count * 4, d->value);
                return 1;
            }
            if (count == 3) {
                Table_AddColor(table, self->name, d->hidden, d->value, low, high);
            } else if (count == 4) {
                Table_AddColorA(table, self->name, d->hidden, d->value, low, high);
            }
            return 1;
        }
        if (vectorType == 0) {
            if (self->global) {
                ShaderFactory_RegisterGlobalConstant(&self->name, 5, 8, d->value);
                return 1;
            }
            Table_AddPoint2(table, self->name, d->hidden, d->value, low, high);
        } else if (vectorType == 1) {
            if (self->global) {
                ShaderFactory_RegisterGlobalConstant(&self->name, 6, 0xC, d->value);
                return 1;
            }
            Table_AddPoint3(table, self->name, d->hidden, d->value, low, high);
        } else if (vectorType == 2) {
            if (self->global) {
                ShaderFactory_RegisterGlobalConstant(&self->name, 7, 0x10, d->value);
                return 1;
            }
            Table_AddPoint4(table, self->name, d->hidden, d->value, low, high);
        }
        return 1;
    }
    case 3: {   // float (no range when the bounds are equal)
        FloatDesc* d = static_cast<FloatDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->arrayCount = 1;
        if (self->global) {
            ShaderFactory_RegisterGlobalConstant(&self->name, 4, 4, &d->value);
            return 1;
        }
        if (d->maximum == d->minimum) {
            Table_AddFloat(table, self->name, d->hidden, d->value, 0.0f, 0.0f);
            return 1;
        }
        Table_AddFloat(table, self->name, d->hidden, d->value, d->minimum, d->maximum);
        return 1;
    }
    case 4: {   // bool
        BoolDesc* d = static_cast<BoolDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->arrayCount = 1;
        if (self->global) {
            ShaderFactory_RegisterGlobalConstant(&self->name, 1, 1, &d->value);
            return 1;
        }
        Table_AddBool(table, self->name, d->hidden, d->value);
        return 1;
    }
    case 5: {   // unsigned int
        IntegerDesc* d = static_cast<IntegerDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->arrayCount = 1;
        if (self->global) {
            ShaderFactory_RegisterGlobalConstant(&self->name, 3, 4, &d->value);
            return 1;
        }
        uint32_t low = d->minimum;
        uint32_t high = d->maximum;
        if (low == high) {
            low = 0;
            high = 0;
        }
        Table_AddUnsignedInt(table, self->name, d->hidden, d->value, low, high);
        return 1;
    }
    case 6: {   // texture: an object's, a texturing property map (by its NTM name), a file or a
                // procedural one (a function and target)
        TextureDesc* d = static_cast<TextureDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->arrayCount = 1;
        if (self->global) {
            ShaderFactory_RegisterGlobalConstant(&self->name, 0xB, 4, d->texture);
        }
        self->objectType = d->objectType;
        self->index = d->objectIndex;
        if (d->objectType != 0) {
            return 1;
        }
        if (d->ntm != nullptr) {
            static const struct {
                const char* name;
                uint32_t flags;
            } kMaps[] = {{"base", 0x200000}, {"dark", 0x300000},   {"detail", 0x400000}, {"gloss", 0x500000},
                         {"glow", 0x600000}, {"bump", 0x700000},   {"normal", 0x800000}, {"parallax", 0x900000}};
            for (const auto& map : kMaps) {
                if (NiStricmp(d->ntm, map.name) == 0) {
                    self->textureFlags = map.flags;
                    return 1;
                }
            }
            if (NiStricmp(d->ntm, "decal") == 0) {
                self->textureFlags = d->ntmIndex | 0x1000000;
                return 1;
            }
            self->textureFlags = d->ntmIndex | 0x2000000;
            Table_AddTexture(table, self->name, d->hidden, d->ntmIndex, d->file);
            return 1;
        }
        if (d->objectType != 0 && d->objectType >= 1 && d->objectType <= 7) {
            return 1;
        }
        if (d->file != nullptr) {
            re::Ptr32<char> file = nullptr;
            NiD3DUtility_SetString(&file, 0, d->file);
            self->textureFile = file;
            self->textureType = d->textureType;
            return 1;
        }
        if (d->function == nullptr) {
            return 0;
        }
        re::Ptr32<char> function = nullptr;
        NiD3DUtility_SetString(&function, 0, d->function);
        self->textureFile = function;
        re::Ptr32<char> target = nullptr;
        NiD3DUtility_SetString(&target, 0, d->target != nullptr ? static_cast<const char*>(d->target) : "tx_1_0");
        self->textureTarget = target;
        self->textureType = d->textureType;
        return 1;
    }
    case 7: {   // predefined
        PredefinedDesc* d = reinterpret_cast<PredefinedDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->arrayCount = 1;
        self->maxCount = d->count;
        self->predefined = d->mapping;
        self->index = d->ntmIndex;
        return 1;
    }
    case 8: {   // object
        ObjectDesc* d = reinterpret_cast<ObjectDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->arrayCount = 1;
        self->objectMapping = d->mapping;
        self->objectType = d->objectType;
        self->index = d->objectIndex;
        return 1;
    }
    case 9: {   // array (a range as for vectors)
        ArrayDesc* d = static_cast<ArrayDesc*>(desc);
        FixedString_AssignChars(&self->name, d->name);
        self->arrayCount = d->elements;
        uint32_t count = d->columns * d->rows * d->elements;
        const float* low = d->minimum;
        const float* high = d->maximum;
        bool equal = true;
        uint32_t i = 0;
        for (; i < count; i++) {
            if (d->maximum[i] < d->minimum[i]) {
                break;
            }
            if (d->maximum[i] > d->minimum[i]) {
                equal = false;
            }
        }
        if (count == 0 || equal || i < count) {
            low = nullptr;
            high = nullptr;
        }
        if (self->global) {
            ShaderFactory_RegisterGlobalConstant(&self->name, 0xE, ArrayDesc_GetElementSize(d) * d->elements,
                                                 d->value);
            return 1;
        }
        uint32_t attributeType = 0;
        switch (ArrayDesc_GetType(d)) {
        case 0:
            attributeType = 4;
            break;
        case 1:
            attributeType = 5;
            break;
        case 2:
            attributeType = 6;
            break;
        case 3:
            attributeType = 7;
            break;
        case 4:
            attributeType = 10;
            break;
        default:
            break;
        }
        Table_AddArray(table, self->name, d->hidden, d->value, d->elements, ArrayDesc_GetElementSize(d), attributeType,
                       low, high);
        return 1;
    }
    default:
        return 0;
    }
}

// 0x74B1B0: SetPredefinedParameter (by the NiShaderConstantMap mapping: each base matrix mapping
// comes as is, inverted, transposed, or both). Quirks kept: the "dark" texture transforms read the
// bump map's; the skinned world matrices' inverse / transpose tests compare with the bone
// mappings' numbers (never true); 0x50 also sets 0x51's value, 0x66 0x69's and 0x74 0x75's on the
// same handle (no break). True unless the time extra data is missing
uint8_t __fastcall NiD3DXEffectParameter_SetPredefinedParameter(NiD3DXEffectParameter* self, re::Edx, void* effect,
                                                                void* geometry, uint8_t* skin, uint8_t* partition,
                                                                void*, uint8_t* state, uint8_t* effects, void*, void*,
                                                                uint32_t, int vertexShader) {
    uint32_t mapping = self->predefined;
    const char* handle = self->handle;
    alignas(16) Matrix m;
    float v[4];
    // the texture transform mappings: which map, then 4 kinds
    int transformMap = -1;
    switch (mapping) {
    case 0x1:
    case 0x2:
    case 0xD:
    case 0xE:
    case 0x3:
    case 0x4:
    case 0xF:
    case 0x10:
    case 0x5:
    case 0x6:
    case 0x11:
    case 0x12: {   // world, view, projection
        int offset = (mapping == 1 || mapping == 2 || mapping == 0xD || mapping == 0xE)    ? 0x7D0
                     : (mapping == 3 || mapping == 4 || mapping == 0xF || mapping == 0x10) ? 0x790
                                                                                          : 0x750;
        uint32_t base = (mapping - 1) % 12 + 1;   // (1..6)
        bool invert = (base & 1) == 0;
        bool transpose = mapping >= 0xD;
        CopyMatrix(m, RendererMatrix(offset));
        if (invert) {
            Invert(m);
        }
        SetMatrix(effect, handle, m, transpose);
        return 1;
    }
    case 0x7:
    case 0x8:
    case 0x13:
    case 0x14: {   // world * view
        bool invert = mapping == 8 || mapping == 0x14;
        bool transpose = mapping == 0x13 || mapping == 0x14;
        Matrix a, b;
        CopyMatrix(a, RendererMatrix(0x750));
        CopyMatrix(b, RendererMatrix(0x790));
        Multiply(m, a, b);
        if (invert) {
            Invert(m);
        }
        SetMatrix(effect, handle, m, transpose);
        return 1;
    }
    case 0x9:
    case 0xA:
    case 0x15:
    case 0x16: {   // view * projection
        bool invert = mapping == 0xA || mapping == 0x16;
        bool transpose = mapping == 0x15 || mapping == 0x16;
        Matrix a, b;
        CopyMatrix(a, RendererMatrix(0x790));
        CopyMatrix(b, RendererMatrix(0x7D0));
        Multiply(m, a, b);
        if (invert) {
            Invert(m);
        }
        SetMatrix(effect, handle, m, transpose);
        return 1;
    }
    case 0xB:
    case 0xC:
    case 0x17:
    case 0x18: {   // world * view * projection
        bool invert = mapping == 0xC || mapping == 0x18;
        bool transpose = mapping == 0x17 || mapping == 0x18;
        Matrix a, b, c, ab;
        CopyMatrix(a, RendererMatrix(0x750));
        CopyMatrix(b, RendererMatrix(0x790));
        CopyMatrix(c, RendererMatrix(0x7D0));
        Multiply(ab, a, b);
        Multiply(m, ab, c);
        if (invert) {
            Invert(m);
        }
        SetMatrix(effect, handle, m, transpose);
        return 1;
    }
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20: {   // the skin's world * view (* projection)
        if (skin == nullptr) {
            return 1;
        }
        CopyMatrix(m, At<const float*>(skin, 0x2C));
        bool invert = mapping == 0x1A || mapping == 0x1C || mapping == 0x1E || mapping == 0x20;
        bool transpose = mapping == 0x1B || mapping == 0x1C || mapping == 0x1F || mapping == 0x20;
        Matrix view;
        CopyMatrix(view, RendererMatrix(0x790));
        Multiply(m, m, view);
        if (mapping >= 0x1D) {
            Matrix projection;
            CopyMatrix(projection, RendererMatrix(0x7D0));
            Multiply(m, m, projection);
        }
        if (invert) {
            Invert(m);
        }
        SetMatrix(effect, handle, m, transpose);
        return 1;
    }
    case 0x21:
    case 0x23:
    case 0x22:
    case 0x24: {   // the partition's bone matrices (3x4 ones given a last row of 0 0 0 1)
        if (skin == nullptr) {
            return 1;
        }
        uint32_t stride = At<uint32_t>(skin, 0x20);
        const float* bones = At<const float*>(skin, 0x28);
        GrowBoneMatrices(self->maxCount);
        uint16_t count = At<uint16_t>(partition, 0x20);
        const uint16_t* indices = At<const uint16_t*>(partition, 4);
        uint16_t i = 0;
        for (; i < count; i++) {
            const float* from = bones + indices[i] * stride * 4;
            float* to = ms_pfBoneMatrices + i * 16;
            if (mapping == 0x21 || mapping == 0x23) {
                for (int k = 0; k < 12; k++) {
                    to[k] = from[k];
                }
                to[12] = 0.0f;
                to[13] = 0.0f;
                to[14] = 0.0f;
                to[15] = 1.0f;
            } else {
                crt::Memcpy(to, from, 64);
            }
        }
        Com<long>(effect, kSetMatrixTransposeArray, handle, static_cast<const float*>(ms_pfBoneMatrices), uint32_t(i));
        return 1;
    }
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
        transformMap = 0;   // base
        break;
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
        transformMap = 5;   // dark (the bump map's: the game's)
        break;
    case 0x2D:
    case 0x2E:
    case 0x2F:
    case 0x30:
        transformMap = 2;   // detail
        break;
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
        transformMap = 3;   // gloss
        break;
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
        transformMap = 4;   // glow
        break;
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
        transformMap = 5;   // bump
        break;
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40:
        transformMap = 8 + int(self->index);   // decal
        break;
    case 0x58:
    case 0x59:
    case 0x5A:
    case 0x5B:
        transformMap = 6;   // normal
        break;
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
        transformMap = 7;   // parallax
        break;
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44: {   // a shader map's
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        void* transform = nullptr;
        bool invert = false;
        bool transpose = false;
        if (texturing != nullptr) {
            uint8_t* map = static_cast<uint8_t*>(TexturingProperty_GetShaderMap(texturing, self->index));
            if (map != nullptr && At<void*>(map, 0xC) != nullptr) {
                transform = TextureTransform_GetMatrix(At<void*>(map, 0xC));
                invert = mapping == 0x42 || mapping == 0x44;
                transpose = mapping == 0x43 || mapping == 0x44;
            }
        }
        TextureTransform_ToD3D(m, transform, invert, 0, re::Flag(vertexShader) ? 4 : 2);
        SetMatrix(effect, handle, m, transpose);
        return 1;
    }
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C: {   // the material's colours (ambient, diffuse, specular, emissive), its alpha
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return 1;
        }
        int offset = mapping == 0x49 ? 0x28 : mapping == 0x4A ? 0x1C : mapping == 0x4B ? 0x34 : 0x40;
        v[0] = At<float>(material, offset);
        v[1] = At<float>(material, offset + 4);
        v[2] = At<float>(material, offset + 8);
        v[3] = At<float>(material, 0x50);
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    }
    case 0x4D: {   // the material's shininess
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return 1;
        }
        Com<long>(effect, kSetFloat, handle, At<float>(material, 0x4C));
        return 1;
    }
    case 0x4E:
    case 0x4F: {   // the eye's position, its direction (the inverse view's rows)
        CopyMatrix(m, RendererMatrix(0x810));
        int row = mapping == 0x4E ? 12 : 8;
        v[0] = m[row];
        v[1] = m[row + 1];
        v[2] = m[row + 2];
        v[3] = m[row + 3];
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    }
    case 0x50:   // the sine terms, then (no break) the cosine terms
        v[0] = 1.0f;
        v[1] = kSinTerm1;
        v[2] = kSinTerm2;
        v[3] = kSinTerm3;
        Com<long>(effect, kSetVector, handle, v);
        [[fallthrough]];
    case 0x51:
        v[0] = kCosTerm0;
        v[1] = kCosTerm1;
        v[2] = kCosTerm2;
        v[3] = kCosTerm3;
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56: {   // the time (the geometry's float extra data), its sine, cosine, tangent
        re::Ptr32<const char> name;
        FixedString_Construct(&name, ms_pcTimeExtraData);
        uint8_t* extra = static_cast<uint8_t*>(ObjectNET_GetExtraData(geometry, &name));
        FixedString_Destruct(&name);
        if (extra == nullptr) {
            return 0;
        }
        float time = At<float>(extra, 0xC);
        switch (mapping) {
        case 0x52:
            Com<long>(effect, kSetFloat, handle, time);
            return 1;
        case 0x53:
            Com<long>(effect, kSetFloat, handle, crt::Sinf(time));
            return 1;
        case 0x54:
            Com<long>(effect, kSetFloat, handle, crt::Cosf(time));
            return 1;
        case 0x55:
            Com<long>(effect, kSetFloat, handle, crt::Tanf(time));
            return 1;
        default:
            v[0] = time;
            v[1] = crt::Sinf(time);
            v[2] = crt::Cosf(time);
            v[3] = crt::Tanf(time);
            Com<long>(effect, kSetVector, handle, v);
            return 1;
        }
    }
    case 0x57: {   // the ambient lights' colours (times their dimmers) summed
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        v[3] = 1.0f;
        if (effects != nullptr) {
            for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, 0xC); node != nullptr;) {
                void* light = node[1];
                node = static_cast<re::Ptr32<void>*>(node[0]);
                uint8_t* ambient = static_cast<uint8_t*>(DynamicCast(NiAmbientLight_RTTI, light));
                if (ambient != nullptr) {
                    float color[3];
                    Color_Times(reinterpret_cast<const float*>(ambient + 0xFC), color, At<float>(ambient, 0xF8));
                    v[0] = v[0] + color[0];
                    v[1] = v[1] + color[1];
                    v[2] = v[2] + color[2];
                }
            }
        }
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    }
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63: {   // the skin's world matrix (the tests compare with 0x1A..0x1C: as is)
        if (skin == nullptr) {
            return 1;
        }
        CopyMatrix(m, At<const float*>(skin, 0x2C));
        bool invert = mapping == 0x1A || mapping == 0x1C;
        bool transpose = mapping == 0x1B || mapping == 0x1C;
        if (invert) {
            Invert(m);
        }
        SetMatrix(effect, handle, m, transpose);
        return 1;
    }
    case 0x64: {   // the fog's depth scale (1 / ((far - near) * density))
        uint8_t* renderState = RenderState(Renderer());
        float scale = 1.0f / ((At<float>(renderState, 0x7C) - At<float>(renderState, 0x78)) *
                              At<float>(At<uint8_t*>(state, 0x10), 0x1C));
        v[0] = scale;
        v[1] = scale;
        v[2] = scale;
        v[3] = scale;
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    }
    case 0x65: {   // the fog's start and end
        uint8_t* renderState = RenderState(Renderer());
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        float far = At<float>(renderState, 0x7C);
        float depth = far - At<float>(renderState, 0x78);
        depth = depth * At<float>(fog, 0x1C);
        float factor = At<float>(renderState, 0x84);
        v[0] = far - depth;
        v[1] = depth * factor + far;
        v[2] = 0.0f;
        v[3] = 0.0f;
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    }
    case 0x66: {   // the fog's colour, then (no break) the parallax offset
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        v[0] = At<float>(fog, 0x20);
        v[1] = At<float>(fog, 0x24);
        v[2] = At<float>(fog, 0x28);
        v[3] = 1.0f;
        Com<long>(effect, kSetVector, handle, v);
        [[fallthrough]];
    }
    case 0x69: {   // the parallax map's offset
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        float offset = 0.0f;
        if (texturing != nullptr && Map(texturing, 7) != nullptr) {
            offset = At<float>(Map(texturing, 7), 0x10);
        }
        Com<long>(effect, kSetFloat, handle, offset);
        return 1;
    }
    case 0x67: {   // the bump map's matrix
        float bump[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr && Map(texturing, 5) != nullptr) {
            uint8_t* map = Map(texturing, 5);
            bump[0] = At<float>(map, 0x18);
            bump[1] = At<float>(map, 0x1C);
            bump[2] = At<float>(map, 0x20);
            bump[3] = At<float>(map, 0x24);
        }
        v[0] = bump[0];
        v[1] = bump[1];
        v[2] = bump[2];
        v[3] = bump[3];
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    }
    case 0x68: {   // the bump map's luma scale and offset
        v[0] = 0.0f;
        v[1] = 1.0f;
        v[2] = 0.0f;
        v[3] = 0.0f;
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr && Map(texturing, 5) != nullptr) {
            uint8_t* map = Map(texturing, 5);
            v[0] = At<float>(map, 0x14);
            v[1] = At<float>(map, 0x10);
        }
        Com<long>(effect, kSetVector, handle, v);
        return 1;
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
    case 0x73: {   // a map's texture size
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing == nullptr) {
            return 1;
        }
        uint8_t* map;
        if (mapping <= 0x71) {
            map = Map(texturing, int(mapping - 0x6A));
        } else if (mapping == 0x72) {
            map = Map(texturing, 8 + int(self->index));
        } else {
            map = static_cast<uint8_t*>(TexturingProperty_GetShaderMap(texturing, self->index));
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        v[3] = 0.0f;
        if (map != nullptr && At<void*>(map, 8) != nullptr) {
            void* texture = At<void*>(map, 8);
            v[0] = static_cast<float>(Virtual<uint32_t>(texture, 18));   // GetWidth
            v[1] = static_cast<float>(Virtual<uint32_t>(texture, 19));   // GetHeight
        }
        Com<long>(effect, kSetVector, handle, v);
        return 1;
    }
    case 0x74:   // the alpha test function, then (no break) it over 255
        Com<long>(effect, kSetInt, handle, uint32_t((At<uint16_t>(At<uint8_t*>(state, 8), 0x18) >> 10) & 7));
        [[fallthrough]];
    case 0x75: {
        uint32_t function = (At<uint16_t>(At<uint8_t*>(state, 8), 0x18) >> 10) & 7;
        Com<long>(effect, kSetFloat, handle, static_cast<float>(int32_t(function)) / 255.0f);
        return 1;
    }
    default:
        return 1;
    }
    // the texture transform mappings (4 per map: as is, inverted, transposed, both)
    uint8_t* texturing = At<uint8_t*>(state, 0x28);
    void* transform = nullptr;
    bool invert = false;
    bool transpose = false;
    if (texturing != nullptr) {
        uint8_t* map = Map(texturing, transformMap);
        if (map != nullptr && At<void*>(map, 0xC) != nullptr) {
            transform = TextureTransform_GetMatrix(At<void*>(map, 0xC));
            uint32_t kind = (mapping >= 0x58 ? mapping - 0x58 : mapping - 0x25) & 3;
            invert = kind == 1 || kind == 3;
            transpose = kind == 2 || kind == 3;
        }
    }
    TextureTransform_ToD3D(m, transform, invert, 0, re::Flag(vertexShader) ? 4 : 2);
    SetMatrix(effect, handle, m, transpose);
    return 1;
}

// 0x74C540: GetD3DTexture: the texturing property's map the flags name (a standard map, a decal or
// a shader map), prepared by the renderer's texture manager. (`this` isn't used)
void* __fastcall NiD3DXEffectParameter_GetD3DTexture(NiD3DXEffectParameter*, re::Edx, uint8_t* texturing,
                                                     uint32_t flags) {
    uint8_t* map = nullptr;
    uint32_t kind = flags & 0xF000000;
    if (kind == 0) {
        switch (flags & 0xF00000) {
        case 0x200000:
            map = Map(texturing, 0);
            break;
        case 0x300000:
            map = Map(texturing, 1);
            break;
        case 0x400000:
            map = Map(texturing, 2);
            break;
        case 0x500000:
            map = Map(texturing, 3);
            break;
        case 0x600000:
            map = Map(texturing, 4);
            break;
        case 0x700000:
            map = Map(texturing, 5);
            break;
        case 0x800000:
            map = Map(texturing, 6);
            break;
        case 0x900000:
            map = Map(texturing, 7);
            break;
        default:
            return nullptr;
        }
    } else if (kind == 0x1000000) {
        uint32_t index = flags & 0xFFFF;
        if (index >= uint32_t(int32_t(At<uint16_t>(texturing, 0x26)) - 8)) {
            return nullptr;
        }
        map = Map(texturing, 8 + int(index));
    } else if (kind == 0x2000000) {
        uint32_t index = flags & 0xFFFF;
        NiTArray<void*>* shaderMaps = At<NiTArray<void*>*>(texturing, 0x30);
        if (shaderMaps == nullptr || index >= shaderMaps->size) {
            return nullptr;
        }
        map = static_cast<uint8_t*>(TexturingProperty_GetShaderMap(texturing, index));
    } else {
        return nullptr;
    }
    if (map == nullptr || At<void*>(map, 8) == nullptr) {
        return nullptr;
    }
    uint8_t changed = 0;   // (the game's byte is stale stack)
    uint32_t a = flags;    // (the argument slots, overwritten)
    uint32_t b = uint32_t(uintptr_t(texturing));
    return Virtual<void*>(At<void*>(Renderer(), 0x6C0), 3, At<void*>(map, 8), &changed, &a, &b);
}

// 0x74C690: SetParameter: predefined, object, texture (its own, an object's or a map's), then the
// global constant or the geometry's extra data of its name by type
uint32_t __fastcall NiD3DXEffectParameter_SetParameter(NiD3DXEffectParameter* self, re::Edx, void* effect,
                                                       uint8_t* geometry, void* skin, void* partition,
                                                       void* rendererData, uint8_t* state, uint8_t* effects,
                                                       void* world, void* worldBound, uint32_t pass,
                                                       int vertexShader) {
    if (self->predefined != 0) {
        return Parameter_SetPredefinedParameter(self, effect, geometry, skin, partition, rendererData, state, effects,
                                                world, worldBound, pass, vertexShader)
                   ? 0
                   : 0x50;
    }
    if (self->objectMapping != 0) {
        return Parameter_SetObjectParameter(self, effect, geometry, skin, partition, rendererData, state, effects,
                                            world, worldBound, pass);
    }
    void* texture = nullptr;
    if (self->objectType != 0 || self->textureFlags != 0) {
        // an object's texture: a shadow map, the environment map, a projected shadow or light, the fog map
        void* source = nullptr;
        bool found = false;
        uint8_t changed = 0;   // (the game passes argument slots for the outputs)
        uint32_t a = 0;
        uint32_t b = 0;
        switch (self->objectType) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7: {
            uint8_t* light =
                static_cast<uint8_t*>(ConstantMap_GetDynamicEffectForObject(effects, self->objectType, self->index));
            if (light == nullptr) {
                break;
            }
            uint8_t* generator = At<uint8_t*>(light, 0xD4);
            if (generator == nullptr) {
                break;
            }
            auto getShadowMap = At<uint8_t*(__cdecl*)(void*, void*, uint32_t)>(generator, 0x30);
            source = At<void*>(getShadowMap(generator, geometry, pass), 8);
            if (source == nullptr) {
                break;
            }
            a = uint32_t(uintptr_t(world));
            b = uint32_t(uintptr_t(worldBound));
            changed = uint8_t(uintptr_t(rendererData));
            found = true;
            break;
        }
        case 8:
        case 9:
        case 10:
        case 11: {
            uint8_t* textureEffect = nullptr;
            if (self->objectType == 8) {
                textureEffect = At<uint8_t*>(effects, 0x18);
            } else if (self->objectType == 11) {
                textureEffect = At<uint8_t*>(effects, 0x1C);
            } else {
                re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, self->objectType == 9 ? 0x14 : 0x10);
                bool atIndex = false;
                for (uint32_t i = 0; node != nullptr; i++) {
                    void* element = node[1];
                    node = static_cast<re::Ptr32<void>*>(node[0]);
                    if (i == self->index) {
                        textureEffect = static_cast<uint8_t*>(element);
                        atIndex = true;
                        break;
                    }
                }
                if (!atIndex) {
                    break;
                }
            }
            if (textureEffect == nullptr) {
                break;
            }
            source = At<void*>(textureEffect, 0x158);
            if (source == nullptr) {
                break;
            }
            a = uint32_t(uintptr_t(geometry));
            b = uint32_t(uintptr_t(effects));
            changed = uint8_t(pass);
            found = true;
            break;
        }
        default:
            break;
        }
        if (found) {
            texture = Virtual<void*>(At<void*>(Renderer(), 0x6C0), 3, source, &changed, &a, &b);
            if (texture != nullptr) {
                Com<long>(effect, kSetTexture, self->handle, texture);
                return 0;
            }
        }
        if (self->textureFlags == 0) {
            return 0x10;
        }
        texture = Parameter_GetD3DTexture(self, At<void*>(state, 0x28), self->textureFlags);
    } else {
        if (self->type == 0) {
            return 0x10;
        }
        if (self->type == 6) {   // the parameter's own texture, made on first use
            if (self->textureType == 2) {
                if (self->texture == nullptr) {
                    Parameter_LoadTexture3D(self);
                }
            } else if (self->textureType == 3) {
                if (self->texture == nullptr) {
                    Parameter_LoadTextureCube(self);
                }
            } else if (self->textureType == 1 && self->texture == nullptr) {
                Parameter_LoadTexture2D(self);
            }
            texture = self->texture;
        } else {
            if (geometry == nullptr) {
                return 0x10;
            }
            const char* handle = self->handle;
            if (self->global) {
                uint32_t size = 0;
                re::Ptr32<const void> data = nullptr;
                if (!ShaderFactory_RetrieveGlobalConstant(&self->name, &size, &data)) {
                    return 0x10;
                }
                const float* d = static_cast<const float*>(data);
                switch (self->type) {
                case 3:
                    if (size != 4) {
                        return 0x10;
                    }
                    Com<long>(effect, kSetFloat, handle, d[0]);
                    return 0;
                case 2: {
                    float v[4];
                    if (size == 8) {
                        v[0] = d[0];
                        v[1] = d[1];
                        v[2] = 0.0f;
                        v[3] = 1.0f;
                    } else if (size == 0xC) {
                        v[0] = d[0];
                        v[1] = d[1];
                        v[2] = d[2];
                        v[3] = 1.0f;
                    } else {
                        v[0] = d[0];
                        v[1] = d[1];
                        v[2] = d[2];
                        v[3] = d[3];
                    }
                    Com<long>(effect, kSetVector, handle, v);
                    return 0;
                }
                case 1: {
                    Matrix m;
                    if (size == 0x40) {
                        crt::Memcpy(m, d, 64);
                    } else {
                        WidenMatrix(m, d, self->rows, self->columns);
                    }
                    Com<long>(effect, kSetMatrix, handle, m);
                    return 0;
                }
                case 5:
                    if (size != 4) {
                        return 0x10;
                    }
                    Com<long>(effect, kSetInt, handle, *static_cast<const uint32_t*>(data));
                    return 0;
                case 4:
                    if (size != 1) {
                        return 0x10;
                    }
                    Com<long>(effect, kSetBool, handle, int(*static_cast<const uint8_t*>(data)));
                    return 0;
                case 9:
                    return SetFloatArray(effect, handle, d, size >> 2, size >> 4);
                default:
                    return 0x10;
                }
            }
            uint8_t* extra = static_cast<uint8_t*>(ObjectNET_GetExtraData(geometry, &self->name));
            if (extra == nullptr) {
                return 0x10;
            }
            switch (self->type) {
            case 3:
                Com<long>(effect, kSetFloat, handle, At<float>(extra, 0xC));
                return 0;
            case 2: {
                float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};   // (uninitialised in the game for other data)
                if (IsKindOf_(NiFloatsExtraData_RTTI, extra)) {
                    uint32_t count = At<uint32_t>(extra, 0xC);
                    const float* d = At<const float*>(extra, 0x10);
                    v[0] = d[0];
                    v[1] = d[1];
                    if (count == 2) {
                        v[2] = 0.0f;
                        v[3] = 1.0f;
                    } else if (count == 3) {
                        v[2] = d[2];
                        v[3] = 1.0f;
                    } else {
                        v[2] = d[2];
                        v[3] = d[3];
                    }
                } else if (IsKindOf_(NiColorExtraData_RTTI, extra)) {
                    v[0] = At<float>(extra, 0xC);
                    v[1] = At<float>(extra, 0x10);
                    v[2] = At<float>(extra, 0x14);
                    v[3] = self->arrayCount == 3 ? 1.0f : At<float>(extra, 0x18);
                }
                Com<long>(effect, kSetVector, handle, v);
                return 0;
            }
            case 1: {
                Matrix m;
                const float* d = At<const float*>(extra, 0x10);
                if (At<uint32_t>(extra, 0xC) == 0x10) {
                    crt::Memcpy(m, d, 64);
                } else {
                    WidenMatrix(m, d, self->rows, self->columns);
                }
                Com<long>(effect, kSetMatrix, handle, m);
                return 0;
            }
            case 5:
                Com<long>(effect, kSetInt, handle, At<uint32_t>(extra, 0xC));
                return 0;
            case 4:
                Com<long>(effect, kSetBool, handle, int(At<uint8_t>(extra, 0xC)));
                return 0;
            case 9: {
                if (!IsKindOf_(NiFloatsExtraData_RTTI, extra)) {
                    return 0x10;
                }
                uint32_t count = At<uint32_t>(extra, 0xC);
                return SetFloatArray(effect, handle, At<const float*>(extra, 0x10), count, count >> 2);
            }
            default:
                return 0x10;
            }
        }
    }
    if (texture == nullptr) {
        return 0x10;
    }
    Com<long>(effect, kSetTexture, self->handle, texture);
    return 0;
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NID3DXPARAM_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NID3DXPARAM_REVERSED("NiD3DXEffectParameter::ReleaseTexture", 0x0074A3C0, &ni::NiD3DXEffectParameter_ReleaseTexture);
NID3DXPARAM_REVERSED("NiColor::operator*_74A400", 0x0074A400, &ni::NiColor_Times);
NID3DXPARAM_REVERSED("NiTextureTransform::GetMatrix", 0x0074A440, &ni::NiTextureTransform_GetMatrix);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::LinkHandle", 0x0074A460, &ni::NiD3DXEffectParameter_LinkHandle);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::SetObjectParameter", 0x0074A480, &ni::NiD3DXEffectParameter_SetObjectParameter);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::LoadTexture2D", 0x0074A6B0, &ni::NiD3DXEffectParameter_LoadTexture2D);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::LoadTexture3D", 0x0074A710, &ni::NiD3DXEffectParameter_LoadTexture3D);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::LoadTextureCube", 0x0074A780, &ni::NiD3DXEffectParameter_LoadTextureCube);
NID3DXPARAM_REVERSED("NiTexturingProperty::GetShaderMap", 0x0074A7E0, &ni::NiTexturingProperty_GetShaderMap);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::NiD3DXEffectParameter", 0x0074A810, &ni::NiD3DXEffectParameter_Construct);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::~NiD3DXEffectParameter", 0x0074A850, &ni::NiD3DXEffectParameter_Destruct);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::Initialize", 0x0074A8D0, &ni::NiD3DXEffectParameter_Initialize);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::SetPredefinedParameter", 0x0074B1B0, &ni::NiD3DXEffectParameter_SetPredefinedParameter);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::GetD3DTexture", 0x0074C540, &ni::NiD3DXEffectParameter_GetD3DTexture);
NID3DXPARAM_REVERSED("NiD3DXEffectParameter::SetParameter", 0x0074C690, &ni::NiD3DXEffectParameter_SetParameter);
