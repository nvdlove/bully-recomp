// NiD3DXEffectParser: an effect's parameters and techniques read into the parser's descriptions
// (src/gamebryo/NiD3DXEffect.h: one static buffer per kind, cleared and returned) from their D3DX
// descriptions, values and annotations (the UI hints, ranges, texture sources, object bindings,
// predefined semantics), and NiD3DXEffectAttributeTable (the NiShaderAttributeDescs made from
// them, by name). The effect is an ID3DXBaseEffect (an effect or a compiler).

#include "gamebryo/NiD3DXEffect.h"

#include <cstring>
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::d3dx;

constexpr uintptr_t IID_ID3DXEffect = 0x009A2104;
constexpr uintptr_t PointerList_vtable = 0x00952E10;
constexpr uintptr_t PointerListBase_vtable = 0x0099C748;

// the object counters (an object annotation without an index takes the next of its type's)
GAME_VAR(uint32_t, ms_uiShaderMapCount, 0x020DDEB4);
GAME_ARRAY(uint32_t, ms_auiObjectCounts, 0x020DDEB8);   // [9]: general .. projected light map
// the object property names and their mappings
struct ObjectMapping {
    uint32_t mapping;
    re::Ptr32<const char> name;
};
GAME_VAR(ObjectMapping*, ms_pkObjectMappings, 0x020DDEDC);
GAME_VAR(uint32_t, ms_uiObjectMappings, 0x00BC74E4);
// the parser's buffers
GAME_ARRAY(uint8_t, ms_kMatrixDesc, 0x020E2020);
GAME_ARRAY(uint8_t, ms_kVectorDesc, 0x020E1FD0);
GAME_ARRAY(uint8_t, ms_kFloatDesc, 0x020E1FB0);
GAME_ARRAY(uint8_t, ms_kBoolDesc, 0x020E1F34);
GAME_ARRAY(uint8_t, ms_kIntegerDesc, 0x020E1F90);
GAME_ARRAY(uint8_t, ms_kTextureDesc, 0x020E1F48);
GAME_ARRAY(uint8_t, ms_kPredefinedDesc, 0x020E1F18);
GAME_ARRAY(uint8_t, ms_kObjectDesc, 0x020E1F00);
GAME_ARRAY(uint8_t, ms_kArrayDesc, 0x020DDEE0);

GAME_FN(void*, __cdecl, NiNew_, 0x00752460, uint32_t size);
GAME_FN(void*, __cdecl, NiMalloc_, 0x00752300, uint32_t size);
GAME_FN(void, __cdecl, NiFree_, 0x00752380, void* p);
GAME_FN(int, __cdecl, NiStricmp, 0x00753820, const char* a, const char* b);
GAME_FN(int, __cdecl, NiSprintf, 0x00402E00, char* buffer, uintptr_t size, const char* format, ...);
GAME_FN(void, __cdecl, NiShaderFactory_ReportError, 0x00756700, int error, int recoverable, const char* format, ...);
GAME_FN(void, __thiscall, NiTPointerList_RemoveAll, 0x0075DB60, PointerList* list);
// NiShaderConstantMap's (another chunk's)
GAME_FN(uint8_t, __cdecl, ConstantMap_LookUpPredefinedMapping, 0x0077F210, const char* name, uint32_t* mapping,
        uint32_t* count);
GAME_FN(uint8_t, __cdecl, ConstantMap_IsObjectMappingValidForType, 0x0077F000, uint32_t mapping, uint32_t objectType);
GAME_FN(TechniqueDesc*, __cdecl, NiD3DXEffect_ClearTechniqueDesc, 0x007522E0);
// NiShaderAttributeDesc's (another chunk's)
GAME_FN(void*, __thiscall, AttributeDesc_Construct, 0x00784580, void* self);
GAME_FN(void, __thiscall, AttributeDesc_SetType, 0x00784620, void* self, uint32_t type);
GAME_FN(void, __thiscall, AttributeDesc_SetHidden, 0x007849A0, void* self, int hidden);
GAME_FN(void, __thiscall, AttributeDesc_SetRanged, 0x007849C0, void* self, int ranged);
GAME_FN(void, __thiscall, AttributeDesc_SetValueBool, 0x00784690, void* self, int value);
GAME_FN(void, __thiscall, AttributeDesc_SetValueUnsignedInt, 0x00784700, void* self, uint32_t value);
GAME_FN(void, __thiscall, AttributeDesc_SetValueFloat_x64, 0x00784770, void* self, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AttributeDesc_SetValueFloat(void* a0, float a1) {
    return AttributeDesc_SetValueFloat_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, AttributeDesc_SetValuePoint2, 0x007847D0, void* self, const float* value);
GAME_FN(void, __thiscall, AttributeDesc_SetValuePoint3, 0x00784840, void* self, const float* value);
GAME_FN(void, __thiscall, AttributeDesc_SetValueColor, 0x007848B0, void* self, const float* value);
GAME_FN(void, __thiscall, AttributeDesc_SetValueColorA, 0x00784920, void* self, const float* value);
GAME_FN(void, __thiscall, AttributeDesc_SetValuePoint4, 0x00784DE0, void* self, const float* value);
GAME_FN(void, __thiscall, AttributeDesc_SetValueMatrix3, 0x00784E50, void* self, const float* value);
GAME_FN(void, __thiscall, AttributeDesc_SetValueMatrix4, 0x00784EE0, void* self, const float* value);
GAME_FN(void, __thiscall, AttributeDesc_SetValueTexture, 0x00784F50, void* self, uint32_t index, const char* file);
GAME_FN(void, __thiscall, AttributeDesc_SetValueArray, 0x00785070, void* self, const float* value,
        uint32_t attributeType, uint32_t elementSize, uint32_t elements);
GAME_FN(void, __thiscall, AttributeDesc_SetRangeUnsignedInt, 0x007849E0, void* self, uint32_t low, uint32_t high);
GAME_FN(void, __thiscall, AttributeDesc_SetRangeFloat_x64, 0x00784A10, void* self, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AttributeDesc_SetRangeFloat(void* a0, float a1, float a2) {
    return AttributeDesc_SetRangeFloat_x64(a0, re::BitsOf(a1), re::BitsOf(a2));
}
GAME_FN(void, __thiscall, AttributeDesc_SetRangePoint2, 0x00784A40, void* self, const float* low, const float* high);
GAME_FN(void, __thiscall, AttributeDesc_SetRangePoint3, 0x00784A80, void* self, const float* low, const float* high);
GAME_FN(void, __thiscall, AttributeDesc_SetRangePoint4, 0x00784AD0, void* self, const float* low, const float* high);
GAME_FN(void, __thiscall, AttributeDesc_SetRangeColor, 0x00784B30, void* self, const float* low, const float* high);
GAME_FN(void, __thiscall, AttributeDesc_SetRangeColorA, 0x00784B80, void* self, const float* low, const float* high);
GAME_FN(void, __thiscall, AttributeDesc_SetRangeArray, 0x00785110, void* self, uint32_t elementSize, uint32_t elements,
        const float* low, const float* high);
// this file's, called where modes apply
GAME_FN(const char*, __cdecl, Parser_GetAnnotationString, 0x0074EDB0, void* effect, const char* handle,
        const D3DXParameterDesc* desc, const char* name);
GAME_FN(uint32_t, __cdecl, Parser_GetObjectType, 0x0074EE40, const char* name, uint32_t* index, int indexGiven);
GAME_FN(uint8_t, __cdecl, Parser_AdjustObjectMapping, 0x0074F070, void* effect, const char* handle,
        const D3DXParameterDesc* desc, uint32_t objectType, uint32_t* mapping);
GAME_FN(TechniqueDesc*, __cdecl, Parser_ParseTechnique, 0x0074F390, void* effect, const char* handle);
GAME_FN(uint8_t, __cdecl, Parser_LookUpObjectMapping, 0x0074FB70, const char* name, uint32_t* mapping);
GAME_FN(ObjectDesc*, __cdecl, Parser_ParseObjectParameter, 0x0074FBE0, void* effect, const char* handle,
        const D3DXParameterDesc* desc, const char* property);
GAME_FN(PredefinedDesc*, __cdecl, Parser_ParsePredefinedParameter, 0x00750860, const char* semantic,
        int matrixColumns, uint32_t count);
GAME_FN(ParameterDesc*, __cdecl, Parser_ParseParameter, 0x00750AA0, void* effect, const char* handle);
GAME_FN(void, __cdecl, Parser_ParseMatrixAnnotations, 0x0074DF60, void* effect, const D3DXParameterDesc* desc,
        const char* handle, MatrixDesc* out);
GAME_FN(void, __cdecl, Parser_ParseArrayAnnotations, 0x0074E090, void* effect, const D3DXParameterDesc* desc,
        const char* handle, ArrayDesc* out);
GAME_FN(void, __cdecl, Parser_ParseFloatAnnotations, 0x0074E520, void* effect, const D3DXParameterDesc* desc,
        const char* handle, FloatDesc* out);
GAME_FN(void, __cdecl, Parser_ParseBoolAnnotations, 0x0074E6D0, void* effect, const D3DXParameterDesc* desc,
        const char* handle, BoolDesc* out);
GAME_FN(void, __cdecl, Parser_ParseIntegerAnnotations, 0x0074E800, void* effect, const D3DXParameterDesc* desc,
        const char* handle, IntegerDesc* out);
GAME_FN(void, __cdecl, Parser_ParsePredefinedAnnotations, 0x0074E9D0, void* effect, const D3DXParameterDesc* desc,
        const char* handle, PredefinedDesc* out);
GAME_FN(void, __cdecl, Parser_ParseTechniqueAnnotations, 0x0074EA50, void* effect, const D3DXTechniqueDesc* desc,
        const char* handle, TechniqueDesc* out);
GAME_FN(void, __cdecl, Parser_ParseVectorAnnotations, 0x0074F470, void* effect, const D3DXParameterDesc* desc,
        const char* handle, VectorDesc* out);
GAME_FN(void, __cdecl, Parser_ParseTextureAnnotations, 0x0074F740, void* effect, const D3DXParameterDesc* desc,
        const char* handle, TextureDesc* out);
GAME_FN(uint8_t, __cdecl, NiD3DXEffect_CompactFromVector4, 0x00751DD0, const float* from, float* to,
        uint32_t rows, uint32_t columns, uint32_t count);
GAME_FN(void*, __thiscall, Table_CreateAttribute, 0x00751090, NiD3DXEffectAttributeTable* self, const char* name,
        uint32_t type, int hidden);
GAME_FN(void*, __thiscall, Table_FindAttribute, 0x00751180, NiD3DXEffectAttributeTable* self, const char* name);
GAME_FN(uint8_t, __thiscall, Table_AddAttribute, 0x007511E0, NiD3DXEffectAttributeTable* self, void* attribute);
GAME_FN(MatrixDesc*, __cdecl, Parser_GetMatrixDesc, 0x00751B00);
GAME_FN(VectorDesc*, __cdecl, Parser_GetVectorDesc, 0x00751B30);
GAME_FN(FloatDesc*, __cdecl, Parser_GetFloatDesc, 0x00751B60);
GAME_FN(BoolDesc*, __cdecl, Parser_GetBoolDesc, 0x00751BA0);
GAME_FN(IntegerDesc*, __cdecl, Parser_GetIntegerDesc, 0x00751BD0);
GAME_FN(TextureDesc*, __cdecl, Parser_GetTextureDesc, 0x00751C10);
GAME_FN(PredefinedDesc*, __cdecl, Parser_GetPredefinedDesc, 0x00751C40);
GAME_FN(ObjectDesc*, __cdecl, Parser_GetObjectDesc, 0x00751C80);
GAME_FN(ArrayDesc*, __cdecl, Parser_GetArrayDesc, 0x00751CB0);
GAME_FN(uint32_t, __thiscall, ArrayDesc_GetType, 0x00751D20, ArrayDesc* self);

inline bool Is(const char* name, const char* key) {
    return NiStricmp(name, key) == 0;
}
inline bool IsDescription(const char* name) {
    return Is(name, "desc") || Is(name, "description") || Is(name, "uihelp");
}
inline bool IsMin(const char* name) {
    return Is(name, "min") || Is(name, "uimin");
}
inline bool IsMax(const char* name) {
    return Is(name, "max") || Is(name, "uimax");
}

// an annotation of a parameter and its description
inline const char* Annotation(void* effect, const char* handle, uint32_t index, D3DXParameterDesc* desc) {
    const char* annotation = Com<const char*>(effect, kGetAnnotation, handle, index);
    Com<long>(effect, kGetParameterDesc, annotation, desc);
    return annotation;
}
inline const char* GetString(void* effect, const char* annotation) {
    const char* value = nullptr;
    Com<long>(effect, kGetString, annotation, &value);
    return value;
}

// "uiwidget" = "none": hidden
inline void UiWidgetNone(void* effect, const char* annotation, uint8_t* hidden) {
    const char* widget = GetString(effect, annotation);
    if (widget != nullptr && Is(widget, "none")) {
        *hidden = 1;
    }
}

// a description buffer cleared, its type set
template <typename T>
inline T* Cleared(uint8_t* buffer, uint32_t size, uint32_t type) {
    crt::Memset(buffer, 0, size);
    *reinterpret_cast<uint32_t*>(buffer) = type;
    return reinterpret_cast<T*>(buffer);
}

// an attribute added unless the name is taken (deleted when the table won't take it)
inline uint8_t Add(NiD3DXEffectAttributeTable* table, void* attribute) {
    if (Table_AddAttribute(table, attribute)) {
        return 1;
    }
    if (attribute != nullptr) {
        Virtual<void>(attribute, 0, 1);
    }
    return 0;
}

} // namespace

// ---- the effect

// 0x74DE70: the parameter and technique counts (D3DXEFFECT_DESC)
uint8_t __cdecl NiD3DXEffectParser_GetEffectDesc(void* effect, uint32_t* parameters, uint32_t* techniques) {
    struct {
        const char* creator;
        uint32_t parameters, techniques, functions;
    } desc;   // D3DXEFFECT_DESC (the system's layout)
    if (Com<long>(effect, kGetDesc, static_cast<void*>(&desc)) < 0) {
        return 0;
    }
    *parameters = desc.parameters;
    *techniques = desc.techniques;
    return 1;
}

// 0x74DEB0: whether a technique (by name) uses a parameter (by name); true when the effect isn't an
// ID3DXEffect
uint8_t __cdecl NiD3DXEffectParser_IsParameterUsed(void* baseEffect, const char* parameter, const char* technique) {
    void* effect;
    if (Com<long>(baseEffect, kQueryInterface, reinterpret_cast<const void*>(IID_ID3DXEffect), &effect) < 0) {
        return 1;
    }
    const char* parameterHandle = Com<const char*>(effect, kGetParameterByName, static_cast<const char*>(nullptr),
                                                   parameter);
    const char* techniqueHandle = Com<const char*>(effect, 13, technique);   // GetTechniqueByName
    uint8_t used = Com<long>(effect, kIsParameterUsed, parameterHandle, techniqueHandle) != 0 ? 1 : 0;
    Com<unsigned long>(effect, kRelease);
    return used;
}

// 0x74DF20: the object counters reset (a new effect)
void __cdecl NiD3DXEffectParser_ResetCounts() {
    ms_uiShaderMapCount = 0;
    for (int i = 0; i < 9; i++) {
        ms_auiObjectCounts[i] = 0;
    }
}

// ---- the annotations

// 0x74DF60: a matrix's (the description, hidden)
void __cdecl NiD3DXEffectParser_ParseMatrixAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                       const char* handle, MatrixDesc* out) {
    int value = 0;   // (uninitialised in the game)
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc annotationDesc;
        const char* annotation = Annotation(effect, handle, i, &annotationDesc);
        const char* name = annotationDesc.name;
        if (IsDescription(name)) {
            Com<long>(effect, kGetString, annotation, &out->description);
        } else if (Is(name, "hidden")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->hidden = value != 0;
        } else if (Is(name, "uiwidget")) {
            UiWidgetNone(effect, annotation, &out->hidden);
        }
    }
}

// 0x74E090: an array's (the description, hidden, colour, the ranges: read only when they have the
// array's shape; narrower vectors read as float4 and compacted; a failed read clears the range).
// (A range of another class keeps the last read's result: uninitialised stack in the game)
void __cdecl NiD3DXEffectParser_ParseArrayAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                      const char* handle, ArrayDesc* out) {
    int value = 0;
    long maxResult = 0;
    long minResult = 0;
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        const char* name = a.name;
        if (IsDescription(name)) {
            Com<long>(effect, kGetString, annotation, &out->description);
        } else if (Is(name, "hidden")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->hidden = value != 0;
        } else if (Is(name, "uiwidget")) {
            const char* widget = GetString(effect, annotation);
            if (widget == nullptr) {
                continue;
            }
            if (Is(widget, "color") || Is(widget, "colorswatch")) {
                if (out->columns == 4 || out->rows == 1) {
                    out->color = 1;
                }
            } else if (widget != nullptr && Is(widget, "none")) {
                out->hidden = 1;
            }
        } else if (Is(name, "color")) {
            if (out->columns == 4 || out->rows == 1) {
                Com<long>(effect, kGetBool, annotation, &value);
                out->color = value != 0;
            }
        } else if (Is(name, "uitype")) {
            const char* type = GetString(effect, annotation);
            if (type != nullptr && (Is(type, "color") || Is(type, "colorswatch")) &&
                (out->columns == 4 || out->rows == 1)) {
                out->color = 1;
            }
        } else {
            bool isMin = IsMin(name);
            if (!isMin && !IsMax(name)) {
                continue;
            }
            if (a.columns != out->columns || a.rows != out->rows || a.elements != out->elements) {
                continue;
            }
            float* range = isMin ? out->minimum : out->maximum;
            long& result = isMin ? minResult : maxResult;
            bool compact = false;
            if (a.parameterClass == 0) {
                result = Com<long>(effect, kGetFloatArray, annotation, range, out->elements);
            } else if (a.parameterClass == 1) {
                if (a.rows == 1 && a.columns == 4) {
                    result = Com<long>(effect, kGetVectorArray, annotation, range, out->elements);
                } else {
                    result = Com<long>(effect, kGetVectorArray, annotation, out->scratch, out->elements);
                    compact = true;
                }
            }
            if (result < 0) {
                crt::Memset(range, 0, 0x1000);
            } else if (compact) {
                NiD3DXEffect_CompactFromVector4(out->scratch, range, out->rows, out->columns, out->elements);
            }
        }
    }
}

// 0x74E520: a float's (the description, hidden, the range)
void __cdecl NiD3DXEffectParser_ParseFloatAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                      const char* handle, FloatDesc* out) {
    int value = 0;   // (uninitialised in the game)
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        const char* name = a.name;
        if (IsDescription(name)) {
            Com<long>(effect, kGetString, annotation, &out->description);
        } else if (Is(name, "hidden")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->hidden = value != 0;
        } else if (Is(name, "uiwidget")) {
            UiWidgetNone(effect, annotation, &out->hidden);
        } else if (IsMin(name)) {
            Com<long>(effect, kGetFloat, annotation, &out->minimum);
        } else if (IsMax(name)) {
            Com<long>(effect, kGetFloat, annotation, &out->maximum);
        }
    }
}

// 0x74E6D0: a bool's (the description, hidden)
void __cdecl NiD3DXEffectParser_ParseBoolAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                     const char* handle, BoolDesc* out) {
    int value = 0;
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        const char* name = a.name;
        if (IsDescription(name)) {
            Com<long>(effect, kGetString, annotation, &out->description);
        } else if (Is(name, "hidden")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->hidden = value != 0;
        } else if (Is(name, "uiwidget")) {
            UiWidgetNone(effect, annotation, &out->hidden);
        }
    }
}

// 0x74E800: an integer's (the description, hidden, the range)
void __cdecl NiD3DXEffectParser_ParseIntegerAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                        const char* handle, IntegerDesc* out) {
    uint32_t number = 0;
    int value = 0;   // (uninitialised in the game)
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        const char* name = a.name;
        if (IsDescription(name)) {
            Com<long>(effect, kGetString, annotation, &out->description);
        } else if (Is(name, "hidden")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->hidden = value != 0;
        } else if (Is(name, "uiwidget")) {
            UiWidgetNone(effect, annotation, &out->hidden);
        } else if (IsMin(name)) {
            Com<long>(effect, kGetInt, annotation, &number);
            out->minimum = number;
        } else if (IsMax(name)) {
            Com<long>(effect, kGetInt, annotation, &number);
            out->maximum = number;
        }
    }
}

// 0x74E9D0: a predefined one's (its NTM index)
void __cdecl NiD3DXEffectParser_ParsePredefinedAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                           const char* handle, PredefinedDesc* out) {
    uint32_t number = 0;
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        if (Is(a.name, "NTMIndex")) {
            Com<long>(effect, kGetInt, annotation, &number);
            out->ntmIndex = number;
        }
    }
}

// 0x74EA50: a technique's (its shader name, description, implementation, versions, bones, the
// binormal-tangent method and UV source, the state switches)
void __cdecl NiD3DXEffectParser_ParseTechniqueAnnotations(void* effect, const D3DXTechniqueDesc* technique,
                                                          const char* handle, TechniqueDesc* out) {
    int value = 0;
    const char* nbtMethod = nullptr;
    uint32_t number = uint32_t(uintptr_t(out));   // (the game's is the argument slot)
    for (uint32_t i = 0; i < technique->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        const char* name = a.name;
        if (IsDescription(name)) {
            Com<long>(effect, kGetString, annotation, &out->description);
        } else if (Is(name, "shadername")) {
            Com<long>(effect, kGetString, annotation, &out->shaderName);
        } else if (Is(name, "implementation")) {
            Com<long>(effect, kGetInt, annotation, &number);
            out->implementation = number;
        } else if (Is(name, "userversion")) {
            Com<long>(effect, kGetInt, annotation, &number);
            out->userVersion = number;
        } else if (Is(name, "bonesperpartition")) {
            Com<long>(effect, kGetInt, annotation, &number);
            out->bonesPerPartition = number;
        } else if (Is(name, "nbtmethod")) {
            Com<long>(effect, kGetString, annotation, &nbtMethod);
        } else if (Is(name, "nbtsourceuv")) {
            if (a.parameterType == 4) {   // a string
                const char* source = GetString(effect, annotation);
                if (Is(source, "nbtuvdefault")) {
                    out->nbtSourceUV = 0xFFFF;
                }
            } else {
                    Com<long>(effect, kGetInt, annotation, &number);
                out->nbtSourceUV = number;
            }
        } else if (Is(name, "usesnirenderstate")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->usesNiRenderState = value != 0;
        } else if (Is(name, "usesnilightstate")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->usesNiLightState = value != 0;
        } else if (Is(name, "softwarevprequired")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->softwareVPRequired = value != 0;
        } else if (Is(name, "softwarevpfallback")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->softwareVPFallback = value != 0;
        }
    }
    out->nbtMethod = 0;
    if (nbtMethod == nullptr) {
        return;
    }
    if (Is(nbtMethod, "NDL")) {
        out->nbtMethod = 0x1000;
        return;
    }
    if (Is(nbtMethod, "MAX")) {
        NiShaderFactory_ReportError(1, 1,
                                    "NiD3DXEffectParser Warning: deprecated \"MAX\" value assigned to the "
                                    "\"NBTMethod\" annotation specified on \"%s\" technique",
                                    technique->name);
        out->nbtMethod = 0x1000;
        return;
    }
    if (Is(nbtMethod, "ATI")) {
        out->nbtMethod = 0x3000;
    }
}

// 0x74EDB0: the string of the annotation of a name (none: null)
const char* __cdecl NiD3DXEffectParser_GetAnnotationString(void* effect, const char* handle,
                                                           const D3DXParameterDesc* desc, const char* key) {
    const char* value = nullptr;
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        if (Is(a.name, key)) {
            Com<long>(effect, kGetString, annotation, &value);
            return value;
        }
    }
    return value;
}

// 0x74EE40: an object's type by name (1 general light .. 7 shadow spot light, 8 environment map, 9
// projected shadow, 10 projected light, 11 fog map); unless given, its index is the next of its type
uint32_t __cdecl NiD3DXEffectParser_GetObjectType(const char* name, uint32_t* index, int indexGiven) {
    static const char* const kTypes[] = {"GeneralLight",     "PointLight",       "DirectionalLight",
                                         "SpotLight",        "ShadowPointLight", "ShadowDirectionalLight",
                                         "ShadowSpotLight"};
    for (uint32_t type = 1; type <= 7; type++) {
        if (Is(name, kTypes[type - 1])) {
            if (!re::Flag(indexGiven)) {
                *index = ms_auiObjectCounts[type - 1]++;
            }
            return type;
        }
    }
    if (Is(name, "EnvironmentMap")) {
        return 8;
    }
    if (Is(name, "ProjectedShadowMap")) {
        if (!re::Flag(indexGiven)) {
            *index = ms_auiObjectCounts[7]++;
        }
        return 9;
    }
    if (Is(name, "ProjectedLightMap")) {
        if (!re::Flag(indexGiven)) {
            *index = ms_auiObjectCounts[8]++;
        }
        return 10;
    }
    if (Is(name, "FogMap")) {
        return 11;
    }
    return 0;
}

// 0x74F070: an object mapping checked against the object's type, then turned into its variant the
// annotations ask for ("Usage" = "Undimmed", "Space" = "Model", "Attenuation" = "Spot")
uint8_t __cdecl NiD3DXEffectParser_AdjustObjectMapping(void* effect, const char* handle,
                                                       const D3DXParameterDesc* desc, uint32_t objectType,
                                                       uint32_t* mapping) {
    if (!ConstantMap_IsObjectMappingValidForType(*mapping, objectType)) {
        NiShaderFactory_ReportError(1, 0,
                                    "NiD3DXEffectParser Error: Semantic specified for incompatible object type "
                                    "on \"%s\" parameter\n",
                                    desc->name);
        return 0;
    }
    const char* key;
    const char* wanted;
    uint32_t variant;
    switch (*mapping) {
    case 5:
        key = "Usage", wanted = "Undimmed", variant = 2;
        break;
    case 6:
        key = "Usage", wanted = "Undimmed", variant = 3;
        break;
    case 7:
        key = "Usage", wanted = "Undimmed", variant = 4;
        break;
    case 8:
        key = "Space", wanted = "Model", variant = 9;
        break;
    case 10:
        key = "Space", wanted = "Model", variant = 0xB;
        break;
    case 12:
        key = "Space", wanted = "Model", variant = 0xD;
        break;
    case 15:
        key = "Attenuation", wanted = "Spot", variant = 0xE;
        break;
    case 16:
        key = "Space", wanted = "Model", variant = 0x11;
        break;
    case 18:
        key = "Space", wanted = "Model", variant = 0x13;
        break;
    case 20:
        key = "Space", wanted = "Model", variant = 0x15;
        break;
    default:
        return 1;
    }
    const char* value = Parser_GetAnnotationString(effect, handle, desc, key);
    if (value != nullptr && Is(value, wanted)) {
        *mapping = variant;
    }
    return 1;
}

// 0x74F390: a technique described (the parser's buffer): its annotations, the highest vertex and
// pixel shader versions its passes use; the shader name defaults to the technique's
TechniqueDesc* __cdecl NiD3DXEffectParser_ParseTechnique(void* effect, const char* handle) {
    D3DXTechniqueDesc technique;
    Com<long>(effect, kGetTechniqueDesc, handle, &technique);
    TechniqueDesc* desc = NiD3DXEffect_ClearTechniqueDesc();
    desc->techniqueName = technique.name;
    desc->nbtSourceUV = 0xFFFF;
    if (technique.annotations != 0) {
        Parser_ParseTechniqueAnnotations(effect, &technique, handle, desc);
    }
    if (desc->shaderName == nullptr) {
        desc->shaderName = desc->techniqueName;
    }
    for (uint32_t i = 0; i < technique.passes; i++) {
        const char* pass = Com<const char*>(effect, kGetPass, handle, i);
        D3DXPassDesc passDesc;
        Com<long>(effect, kGetPassDesc, pass, &passDesc);
        uint32_t vertex = 0;
        uint32_t pixel = 0;
        if (passDesc.vertexShaderFunction != nullptr) {
            vertex = Import<uint32_t>(kD3DXGetShaderVersion, passDesc.vertexShaderFunction);
        }
        if (passDesc.pixelShaderFunction != nullptr) {
            pixel = Import<uint32_t>(kD3DXGetShaderVersion, passDesc.pixelShaderFunction);
        }
        if (desc->vertexShaderVersion < vertex) {
            desc->vertexShaderVersion = vertex;
        }
        if (desc->pixelShaderVersion < pixel) {
            desc->pixelShaderVersion = pixel;
        }
    }
    return desc;
}

// 0x74F470: a vector's (the description, hidden, colour; the range annotations are read into a copy
// and dropped: the game's)
void __cdecl NiD3DXEffectParser_ParseVectorAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                       const char* handle, VectorDesc* out) {
    int value = 0;   // (uninitialised in the game)
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* annotation = Annotation(effect, handle, i, &a);
        const char* name = a.name;
        if (IsDescription(name)) {
            Com<long>(effect, kGetString, annotation, &out->description);
        } else if (Is(name, "hidden")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->hidden = value != 0;
        } else if (Is(name, "uiwidget")) {
            const char* widget = GetString(effect, annotation);
            if (widget == nullptr) {
                continue;
            }
            if (Is(widget, "color") || Is(widget, "colorswatch")) {
                out->color = 1;
            } else if (widget != nullptr && Is(widget, "none")) {
                out->hidden = 1;
            }
        } else if (Is(name, "color")) {
            Com<long>(effect, kGetBool, annotation, &value);
            out->color = value != 0;
        } else if (Is(name, "uitype")) {
            const char* type = GetString(effect, annotation);
            if (type != nullptr && (Is(type, "color") || Is(type, "colorswatch"))) {
                out->color = 1;
            }
        } else if (IsMin(name)) {
            float copy[4] = {out->minimum[0], out->minimum[1], out->minimum[2], out->minimum[3]};
            Com<long>(effect, kGetVector, annotation, copy);
        } else if (IsMax(name)) {
            float copy[4] = {out->maximum[0], out->maximum[1], out->maximum[2], out->maximum[3]};
            Com<long>(effect, kGetVector, annotation, copy);
        }
    }
}

// 0x74F740: a texture's (the description, hidden, file name, procedural function and target, size,
// type, its NTM map or the object it comes from). A file that doesn't exist is dropped (CreateFileA,
// through the import); a texture with no source is a shader map, numbered in turn
void __cdecl NiD3DXEffectParser_ParseTextureAnnotations(void* effect, const D3DXParameterDesc* desc,
                                                        const char* handle, TextureDesc* out) {
    out->width = 0xFFFFFFFF;
    out->height = 0xFFFFFFFF;
    out->depth = 0xFFFFFFFF;
    out->ntmIndex = 0xFFFFFFFF;
    out->objectType = 0;
    uint32_t number = 0;
    int value = 0;
    const char* type = nullptr;
    const char* object = nullptr;
    uint32_t objectIndex = 0;
    uint8_t indexGiven = 0;
    if (desc->annotations != 0) {
        for (uint32_t i = 0; i < desc->annotations; i++) {
            D3DXParameterDesc a;
            const char* annotation = Annotation(effect, handle, i, &a);
            const char* name = a.name;
            if (IsDescription(name)) {
                Com<long>(effect, kGetString, annotation, &out->description);
            } else if (Is(name, "hidden")) {
                Com<long>(effect, kGetBool, annotation, &value);
                out->hidden = value != 0;
            } else if (Is(name, "uiwidget")) {
                UiWidgetNone(effect, annotation, &out->hidden);
            } else if (Is(name, "name")) {
                Com<long>(effect, kGetString, annotation, &out->file);
            } else if (Is(name, "function")) {
                Com<long>(effect, kGetString, annotation, &out->function);
            } else if (Is(name, "target")) {
                Com<long>(effect, kGetString, annotation, &out->target);
            } else if (Is(name, "width")) {
                Com<long>(effect, kGetInt, annotation, &number);
                out->width = number;
            } else if (Is(name, "height")) {
                Com<long>(effect, kGetInt, annotation, &number);
                out->height = number;
            } else if (Is(name, "depth")) {
                Com<long>(effect, kGetInt, annotation, &number);
                out->depth = number;
            } else if (Is(name, "type")) {
                Com<long>(effect, kGetString, annotation, &type);
            } else if (Is(name, "NTM")) {
                Com<long>(effect, kGetString, annotation, &out->ntm);
            } else if (Is(name, "NTMIndex")) {
                Com<long>(effect, kGetInt, annotation, &number);
                out->ntmIndex = number;
            } else if (Is(name, "Object")) {
                Com<long>(effect, kGetString, annotation, &object);
            } else if (Is(name, "ObjectIndex")) {
                Com<long>(effect, kGetInt, annotation, &number);
                objectIndex = number;
                indexGiven = 1;
            }
        }
        if (object != nullptr) {
            out->objectType = Parser_GetObjectType(object, &objectIndex, indexGiven);
            out->objectIndex = objectIndex;
            out->ntm = nullptr;
        }
        if (type != nullptr) {
            if (Is(type, "volume")) {
                out->textureType = 2;
            } else if (Is(type, "cube")) {
                out->textureType = 3;
            }
        } else {
            out->textureType = 1;
        }
    } else {
        out->textureType = 1;
    }
    if (out->file != nullptr) {
        void* file = Import<void*>(kCreateFileA, out->file, 0x80000000u, 1u, static_cast<void*>(nullptr), 3u, 0u,
                                   static_cast<void*>(nullptr));
        if (file == reinterpret_cast<void*>(intptr_t(-1))) {
            out->file = nullptr;
        } else {
            Import<int>(ni::sys::kCloseHandle, file);
        }
    }
    if (out->file == nullptr && out->function == nullptr && object == nullptr) {
        if (out->ntm == nullptr) {
            out->ntm = "shader";
        }
        if (std::strcmp(out->ntm, "shader") == 0 && out->ntmIndex == 0xFFFFFFFF) {
            out->ntmIndex = ms_uiShaderMapCount++;
        }
    }
}

// 0x74FB70: an object property's mapping by name
uint8_t __cdecl NiD3DXEffectParser_LookUpObjectMapping(const char* name, uint32_t* mapping) {
    for (uint32_t i = 0; i < ms_uiObjectMappings; i++) {
        ObjectMapping* entry = &ms_pkObjectMappings[i];
        if (Is(entry->name, name)) {
            *mapping = entry->mapping;
            return 1;
        }
    }
    return 0;
}

// 0x74FBC0: the technique of an index described
TechniqueDesc* __cdecl NiD3DXEffectParser_GetTechnique(void* effect, uint32_t index) {
    const char* handle = Com<const char*>(effect, kGetTechnique, index);
    return Parser_ParseTechnique(effect, handle);
}

// 0x74FBE0: an object parameter (an "Object" annotation: its type and index, the property's mapping
// from "ObjectProperty" or the semantic)
ObjectDesc* __cdecl NiD3DXEffectParser_ParseObjectParameter(void* effect, const char* handle,
                                                            const D3DXParameterDesc* desc, const char* property) {
    const char* object = Parser_GetAnnotationString(effect, handle, desc, "Object");
    if (object == nullptr) {
        return nullptr;
    }
    const char* annotation = Com<const char*>(effect, kGetAnnotationByName, handle, "ObjectProperty");
    if (annotation != nullptr) {
        const char* value = nullptr;
        if (Com<long>(effect, kGetString, annotation, &value) >= 0 && value != nullptr) {
            property = value;
        }
    }
    if (property == nullptr) {
        return nullptr;
    }
    uint32_t index = 0;
    uint32_t indexGiven = 0;
    for (uint32_t i = 0; i < desc->annotations; i++) {
        D3DXParameterDesc a;
        const char* found = Annotation(effect, handle, i, &a);
        if (Is(a.name, "ObjectIndex")) {
            index = 0;
            Com<long>(effect, kGetInt, found, &index);
            indexGiven = 1;
            break;
        }
    }
    uint32_t type = Parser_GetObjectType(object, &index, int(indexGiven));
    if (type == 0) {
        return nullptr;
    }
    uint32_t mapping;
    if (!Parser_LookUpObjectMapping(property, &mapping)) {
        return nullptr;
    }
    if (!Parser_AdjustObjectMapping(effect, handle, desc, type, &mapping)) {
        return nullptr;
    }
    ObjectDesc* out = Parser_GetObjectDesc();
    out->objectType = type;
    out->mapping = mapping;
    out->objectIndex = index;
    return out;
}

// 0x750860: a predefined parameter by its semantic, as it is or with its suffixes and prefix
// normalised ("...it", "...trans", "...transpose" and "inv...", "...i", "...inverse" become
// "Inv...Transpose"); the bone matrix mappings keep the element count
PredefinedDesc* __cdecl NiD3DXEffectParser_ParsePredefinedParameter(const char* semantic, int, uint32_t count) {
    uint32_t mapping = 0;
    uint32_t defaultCount = 0;
    if (!ConstantMap_LookUpPredefinedMapping(semantic, &mapping, &defaultCount)) {
        uint32_t length = uint32_t(std::strlen(semantic));
        char* lower = static_cast<char*>(NiMalloc_(length + 1));
        for (uint32_t i = 0; i < length; i++) {
            lower[i] = char(crt::Tolower(semantic[i]));
        }
        char* end = lower + length;
        bool transpose = false;
        bool inverse = false;
        *end = '\0';
        if (length > 2 && crt::Strncmp(end - 2, "it", 2) == 0) {
            inverse = true;
            length -= 2;
            end -= 2;
            transpose = true;
            *end = '\0';
        } else if (length > 5 && crt::Strncmp(end - 5, "trans", 5) == 0) {
            length -= 5;
            end -= 5;
            transpose = true;
            *end = '\0';
        } else if (length > 9 && crt::Strncmp(end - 9, "transpose", 9) == 0) {
            length -= 9;
            end -= 9;
            transpose = true;
            *end = '\0';
        }
        if (length > 3 && crt::Strncmp(lower, "inv", 3) == 0) {
            uint32_t rest = length - 3;
            inverse = true;
            for (uint32_t i = 0; i < rest; i++) {
                lower[i] = lower[i + 3];
            }
            length = rest;
            end[-3] = '\0';
        } else if (length > 1 && crt::Strncmp(end - 1, "i", 1) == 0) {
            length -= 1;
            end[-1] = '\0';
            inverse = true;
        } else if (length > 7 && crt::Strncmp(end - 7, "inverse", 7) == 0) {
            length -= 7;
            end[-7] = '\0';
            inverse = true;
        }
        uint32_t size = (transpose ? 9 : 0) + (inverse ? 3 : 0) + length + 1;
        char* name = static_cast<char*>(NiMalloc_(size));
        NiSprintf(name, size, "%s%s%s", inverse ? "Inv" : "", lower, transpose ? "Transpose" : "");
        NiFree_(lower);
        uint8_t found = ConstantMap_LookUpPredefinedMapping(name, &mapping, &defaultCount);
        NiFree_(name);
        if (!found) {
            return nullptr;
        }
    }
    PredefinedDesc* desc = Parser_GetPredefinedDesc();
    desc->semantic = semantic;
    desc->mapping = mapping;
    desc->count = mapping - 0x21 <= 3 ? count : 1;
    return desc;
}

// 0x750AA0: a parameter described: textures; then (unless "StandardsGlobal") by its "VarType"
// (ATTRIBUTE, GLOBAL, OBJECT, PREDEFINED with its "DefinedMapping") or semantic an object or
// predefined one, else by class and type an array, matrix, vector, float, bool or integer one
// with its value; null for anything else
ParameterDesc* __cdecl NiD3DXEffectParser_ParseParameter(void* effect, const char* handle) {
    D3DXParameterDesc pd;
    Com<long>(effect, kGetParameterDesc, handle, &pd);
    uint32_t parameterClass = pd.parameterClass;
    const char* semantic = pd.semantic;
    uint32_t type = pd.parameterType;
    if (type == 5 || type == 7 || type == 8 || type == 9) {   // the texture types
        TextureDesc* desc = Parser_GetTextureDesc();
        desc->name = pd.name;
        Parser_ParseTextureAnnotations(effect, &pd, handle, desc);
        void* texture;
        if (Com<long>(effect, kGetTexture, handle, &texture) < 0) {
            desc->texture = nullptr;
        } else {
            desc->texture = texture;
        }
        return desc;
    }
    if (type != 1 && type != 2 && type != 3) {
        return nullptr;
    }
    if (semantic != nullptr && Is(semantic, "StandardsGlobal")) {
        return nullptr;
    }
    bool isAttribute = false;
    uint8_t isGlobal = 0;
    bool varTypeGiven = false;
    const char* varType = nullptr;
    const char* annotation = Com<const char*>(effect, kGetAnnotationByName, handle, "VarType");
    if (annotation != nullptr) {
        if (Com<long>(effect, kGetString, annotation, &varType) >= 0) {
            isAttribute = Is(varType, "ATTRIBUTE");
            isGlobal = Is(varType, "GLOBAL") ? 1 : 0;
            Is(varType, "OBJECT");
            Is(varType, "PREDEFINED");
            varTypeGiven = true;
        }
        if (Is(varType, "PREDEFINED")) {
            const char* mappingAnnotation = Com<const char*>(effect, kGetAnnotationByName, handle, "DefinedMapping");
            if (mappingAnnotation != nullptr) {
                const char* defined = nullptr;
                if (Com<long>(effect, kGetString, mappingAnnotation, &defined) >= 0 && defined != nullptr) {
                    semantic = defined;
                }
            }
        }
    }
    if (semantic != nullptr && !varTypeGiven) {
        isAttribute = std::strcmp(semantic, "ATTRIBUTE") == 0;
        isGlobal = std::strcmp(semantic, "GLOBAL") == 0 ? 1 : 0;
    }
    ParameterDesc* result = nullptr;
    if (!isAttribute && !isGlobal) {
        result = reinterpret_cast<ParameterDesc*>(Parser_ParseObjectParameter(effect, handle, &pd, semantic));
        if (result == nullptr && semantic != nullptr) {
            PredefinedDesc* predefined = Parser_ParsePredefinedParameter(semantic, parameterClass == 3, pd.elements);
            if (predefined != nullptr) {
                Parser_ParsePredefinedAnnotations(effect, &pd, handle, predefined);
            }
            result = reinterpret_cast<ParameterDesc*>(predefined);
        }
        if (result != nullptr) {
            if (result->name == nullptr || result->name[0] == '\0') {
                result->name = pd.name;
            }
            return result;
        }
    }
    // one-column vectors and one-element matrix arrays count as scalars (the game's)
    if (parameterClass == 1) {
        if (pd.columns == 1) {
            parameterClass = 0;
        }
    } else if ((parameterClass == 2 || parameterClass == 3) && pd.elements == 1) {
        parameterClass = 0;
    }
    if (type == 3 && pd.elements > 1 && (parameterClass == 1 || parameterClass == 0)) {
        ArrayDesc* desc = Parser_GetArrayDesc();
        desc->rows = pd.rows;
        desc->columns = pd.columns;
        desc->elements = pd.elements;
        if (pd.elements > 0x100) {
            return nullptr;
        }
        ArrayDesc_GetType(desc);
        Parser_ParseArrayAnnotations(effect, &pd, handle, desc);
        bool compact = false;
        long hr = 0;   // (another class: the game reads a stale argument slot, never negative)
        if (pd.parameterClass == 0) {
            hr = Com<long>(effect, kGetFloatArray, handle, desc->value, desc->elements);
        } else if (pd.parameterClass == 1) {
            if (pd.rows == 1 && pd.columns == 4) {
                hr = Com<long>(effect, kGetVectorArray, handle, desc->value, desc->elements);
            } else {
                hr = Com<long>(effect, kGetVectorArray, handle, desc->scratch, desc->elements);
                compact = true;
            }
        }
        if (hr < 0) {
            crt::Memset(desc->value, 0, 0x1000);
        } else if (compact) {
            NiD3DXEffect_CompactFromVector4(desc->scratch, desc->value, desc->rows, desc->columns, desc->elements);
        }
        result = desc;
    } else if (parameterClass == 2 || parameterClass == 3) {
        if (type != 3) {
            return nullptr;
        }
        MatrixDesc* desc = Parser_GetMatrixDesc();
        desc->rows = pd.rows;
        desc->columns = pd.columns;
        Parser_ParseMatrixAnnotations(effect, &pd, handle, desc);
        long hr = parameterClass == 3 ? Com<long>(effect, kGetMatrix, handle, desc->value)
                                      : Com<long>(effect, kGetMatrixTranspose, handle, desc->value);
        if (hr < 0) {
            for (int i = 0; i < 16; i++) {
                desc->value[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            }
        }
        result = desc;
    } else if (parameterClass == 1) {
        if (type != 3) {
            return nullptr;
        }
        VectorDesc* desc = Parser_GetVectorDesc();
        desc->rows = pd.rows;
        desc->columns = pd.columns;
        desc->elements = pd.columns;
        if (pd.columns > 4) {
            return nullptr;
        }
        Parser_ParseVectorAnnotations(effect, &pd, handle, desc);
        if (Com<long>(effect, kGetVector, handle, desc->value) < 0) {
            desc->value[0] = 0.0f;
            desc->value[1] = 0.0f;
            desc->value[2] = 0.0f;
            desc->value[3] = 0.0f;
        }
        result = desc;
    } else if (parameterClass == 0) {
        if (type == 3) {
            FloatDesc* desc = Parser_GetFloatDesc();
            Parser_ParseFloatAnnotations(effect, &pd, handle, desc);
            float value;
            desc->value = Com<long>(effect, kGetFloat, handle, &value) >= 0 ? value : 0.0f;
            result = desc;
        } else if (type == 1) {
            BoolDesc* desc = Parser_GetBoolDesc();
            Parser_ParseBoolAnnotations(effect, &pd, handle, desc);
            int value;
            desc->value = Com<long>(effect, kGetBool, handle, &value) >= 0 ? (value != 0 ? 1 : 0) : 0;
            result = desc;
        } else if (type == 2) {
            IntegerDesc* desc = Parser_GetIntegerDesc();
            Parser_ParseIntegerAnnotations(effect, &pd, handle, desc);
            uint32_t value = 0;
            long hr = Com<long>(effect, kGetInt, handle, &value);
            desc->value = hr < 0 ? 0 : value;
            result = desc;
        }
    }
    if (result == nullptr) {
        return nullptr;
    }
    result->global = isGlobal;
    if (result->name == nullptr || result->name[0] == '\0') {
        result->name = pd.name;
    }
    return result;
}

// 0x751070: the parameter of an index described
ParameterDesc* __cdecl NiD3DXEffectParser_GetParameter(void* effect, uint32_t index) {
    const char* handle = Com<const char*>(effect, kGetParameter, static_cast<const char*>(nullptr), index);
    return Parser_ParseParameter(effect, handle);
}

// ---- NiD3DXEffectAttributeTable

// 0x751090: an NiShaderAttributeDesc made (its name, type, hidden flag). (`this` isn't used)
void* __fastcall NiD3DXEffectAttributeTable_CreateAttribute(NiD3DXEffectAttributeTable*, re::Edx, const char* name,
                                                           uint32_t type, int hidden) {
    void* memory = NiNew_(0xE4);
    if (memory == nullptr) {
        return nullptr;
    }
    void* attribute = AttributeDesc_Construct(memory);
    if (attribute == nullptr) {
        return nullptr;
    }
    Virtual<void>(attribute, 4, name);   // SetName
    AttributeDesc_SetType(attribute, type);
    AttributeDesc_SetHidden(attribute, hidden);
    return attribute;
}

// 0x7510F0: the destructor (the attributes deleted)
void __fastcall NiD3DXEffectAttributeTable_Destruct(NiD3DXEffectAttributeTable* self, re::Edx) {
    self->iterator = self->attributes.head;
    while (self->iterator != nullptr) {
        ListNode* node = self->iterator;
        self->iterator = node->next;
        if (node->element != nullptr) {
            Virtual<void>(node->element, 0, 1);
        }
    }
    NiTPointerList_RemoveAll(&self->attributes);
    self->attributes.vtable = reinterpret_cast<void*>(PointerList_vtable);
    NiTPointerList_RemoveAll(&self->attributes);
    self->attributes.vtable = reinterpret_cast<void*>(PointerListBase_vtable);
}

// 0x751140 / 0x751160: GetFirstAttribute / GetNextAttribute
void* __fastcall NiD3DXEffectAttributeTable_GetFirst(NiD3DXEffectAttributeTable* self, re::Edx) {
    ListNode* node = self->attributes.head;
    self->iterator = node;
    if (node == nullptr) {
        return nullptr;
    }
    self->iterator = node->next;
    return node->element;
}
void* __fastcall NiD3DXEffectAttributeTable_GetNext(NiD3DXEffectAttributeTable* self, re::Edx) {
    ListNode* node = self->iterator;
    if (node == nullptr) {
        return nullptr;
    }
    self->iterator = node->next;
    return node->element;
}

// 0x751180: FindAttribute (by name, ignoring case; the walk stops at a null element)
void* __fastcall NiD3DXEffectAttributeTable_FindAttribute(NiD3DXEffectAttributeTable* self, re::Edx,
                                                         const char* name) {
    ListNode* node = self->attributes.head;
    self->iterator = node;
    if (node == nullptr) {
        return nullptr;
    }
    self->iterator = node->next;
    for (void* attribute = node->element; attribute != nullptr;) {
        if (NiStricmp(Virtual<const char*>(attribute, 2), name) == 0) {   // GetName
            return attribute;
        }
        node = self->iterator;
        if (node == nullptr) {
            return nullptr;
        }
        self->iterator = node->next;
        attribute = node->element;
    }
    return nullptr;
}

// 0x7511E0: AddAttribute (at the tail)
uint8_t __fastcall NiD3DXEffectAttributeTable_AddAttribute(NiD3DXEffectAttributeTable* self, re::Edx, void* attribute) {
    if (attribute == nullptr) {
        return 0;
    }
    PointerList* list = &self->attributes;
    ListNode* node = Virtual<ListNode*>(list, 1);
    node->element = attribute;
    node->next = nullptr;
    node->prev = list->tail;
    if (list->tail != nullptr) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->count += 1;
    return 1;
}

// 0x751240 .. 0x7518E0: the attributes of each kind (none when the name is taken), with a range
// when one is given
uint8_t __fastcall NiD3DXEffectAttributeTable_AddBool(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                     int hidden, int value) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, 1, hidden);
    AttributeDesc_SetValueBool(attribute, value);
    return Add(self, attribute);
}

uint8_t __fastcall NiD3DXEffectAttributeTable_AddUnsignedInt(NiD3DXEffectAttributeTable* self, re::Edx,
                                                            const char* name, int hidden, uint32_t value,
                                                            uint32_t low, uint32_t high) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, 3, hidden);
    AttributeDesc_SetValueUnsignedInt(attribute, value);
    if (high != 0) {
        AttributeDesc_SetRangeUnsignedInt(attribute, low, high);
        AttributeDesc_SetRanged(attribute, 1);
    }
    return Add(self, attribute);
}

uint8_t __fastcall NiD3DXEffectAttributeTable_AddFloat(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                      int hidden, float value, float low, float high) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, 4, hidden);
    AttributeDesc_SetValueFloat(attribute, value);
    if (high != 0.0f) {
        AttributeDesc_SetRangeFloat(attribute, low, high);
        AttributeDesc_SetRanged(attribute, 1);
    }
    return Add(self, attribute);
}

namespace {
// the vector kinds: the value (copied as `n` floats, or passed as it is), the range when there is one
template <int N>
inline uint8_t AddVector(NiD3DXEffectAttributeTable* self, const char* name, int hidden, uint32_t type,
                         const float* value, const float* low, const float* high,
                         void(__thiscall* setValue)(void*, const float*),
                         void(__thiscall* setRange)(void*, const float*, const float*), bool copy) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, type, hidden);
    if (copy) {
        float valueCopy[N];
        for (int i = 0; i < N; i++) {
            valueCopy[i] = value[i];
        }
        setValue(attribute, valueCopy);
    } else {
        setValue(attribute, value);
    }
    if (high != nullptr) {
        if (copy) {
            float lowCopy[N];
            float highCopy[N];
            for (int i = 0; i < N; i++) {
                lowCopy[i] = low[i];
                highCopy[i] = high[i];
            }
            setRange(attribute, lowCopy, highCopy);
        } else {
            setRange(attribute, low, high);
        }
        AttributeDesc_SetRanged(attribute, 1);
    }
    return Add(self, attribute);
}
} // namespace

uint8_t __fastcall NiD3DXEffectAttributeTable_AddPoint2(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                       int hidden, const float* value, const float* low,
                                                       const float* high) {
    return AddVector<2>(self, name, hidden, 5, value, low, high, AttributeDesc_SetValuePoint2,
                        AttributeDesc_SetRangePoint2, true);
}
uint8_t __fastcall NiD3DXEffectAttributeTable_AddPoint3(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                       int hidden, const float* value, const float* low,
                                                       const float* high) {
    return AddVector<3>(self, name, hidden, 6, value, low, high, AttributeDesc_SetValuePoint3,
                        AttributeDesc_SetRangePoint3, true);
}
uint8_t __fastcall NiD3DXEffectAttributeTable_AddPoint4(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                       int hidden, const float* value, const float* low,
                                                       const float* high) {
    return AddVector<4>(self, name, hidden, 7, value, low, high, AttributeDesc_SetValuePoint4,
                        AttributeDesc_SetRangePoint4, false);
}

uint8_t __fastcall NiD3DXEffectAttributeTable_AddMatrix3(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                        int hidden, const float* value) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, 8, hidden);
    float matrix[9] = {value[0], value[1], value[2], value[4], value[5], value[6], value[8], value[9], value[10]};
    AttributeDesc_SetValueMatrix3(attribute, matrix);
    AttributeDesc_SetRanged(attribute, 0);
    return Add(self, attribute);
}

uint8_t __fastcall NiD3DXEffectAttributeTable_AddMatrix4(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                        int hidden, const float* value) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, 9, hidden);
    AttributeDesc_SetValueMatrix4(attribute, value);
    AttributeDesc_SetRanged(attribute, 0);
    return Add(self, attribute);
}

uint8_t __fastcall NiD3DXEffectAttributeTable_AddColor(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                      int hidden, const float* value, const float* low,
                                                      const float* high) {
    return AddVector<3>(self, name, hidden, 10, value, low, high, AttributeDesc_SetValueColor,
                        AttributeDesc_SetRangeColor, true);
}
uint8_t __fastcall NiD3DXEffectAttributeTable_AddColorA(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                       int hidden, const float* value, const float* low,
                                                       const float* high) {
    return AddVector<4>(self, name, hidden, 10, value, low, high, AttributeDesc_SetValueColorA,
                        AttributeDesc_SetRangeColorA, true);
}

uint8_t __fastcall NiD3DXEffectAttributeTable_AddTexture(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                        int hidden, uint32_t index, const char* file) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, 0xB, hidden);
    AttributeDesc_SetValueTexture(attribute, index, file);
    return Add(self, attribute);
}

uint8_t __fastcall NiD3DXEffectAttributeTable_AddArray(NiD3DXEffectAttributeTable* self, re::Edx, const char* name,
                                                      int hidden, const float* value, uint32_t elements,
                                                      uint32_t elementSize, uint32_t attributeType, const float* low,
                                                      const float* high) {
    if (Table_FindAttribute(self, name) != nullptr) {
        return 0;
    }
    void* attribute = Table_CreateAttribute(self, name, 0xE, hidden);
    AttributeDesc_SetValueArray(attribute, value, attributeType, elementSize, elements);
    if (low != nullptr && high != nullptr) {
        AttributeDesc_SetRangeArray(attribute, elementSize, elements, low, high);
        AttributeDesc_SetRanged(attribute, 1);
    } else {
        AttributeDesc_SetRanged(attribute, 0);
    }
    return Add(self, attribute);
}

// 0x751980: the constructor
NiD3DXEffectAttributeTable* __fastcall NiD3DXEffectAttributeTable_Construct(NiD3DXEffectAttributeTable* self, re::Edx) {
    self->attributes.count = 0;
    self->attributes.head = nullptr;
    self->attributes.tail = nullptr;
    self->attributes.vtable = reinterpret_cast<void*>(PointerList_vtable);
    NiTPointerList_RemoveAll(&self->attributes);
    return self;
}

// ---- the parser's description buffers, cleared

// 0x751B00 .. 0x751CB0
MatrixDesc* __cdecl NiD3DXEffectParser_GetMatrixDesc() {
    return Cleared<MatrixDesc>(ms_kMatrixDesc, sizeof(MatrixDesc), 1);
}
VectorDesc* __cdecl NiD3DXEffectParser_GetVectorDesc() {
    return Cleared<VectorDesc>(ms_kVectorDesc, sizeof(VectorDesc), 2);
}
FloatDesc* __cdecl NiD3DXEffectParser_GetFloatDesc() {
    return Cleared<FloatDesc>(ms_kFloatDesc, sizeof(FloatDesc), 3);
}
BoolDesc* __cdecl NiD3DXEffectParser_GetBoolDesc() {
    return Cleared<BoolDesc>(ms_kBoolDesc, sizeof(BoolDesc), 4);
}
IntegerDesc* __cdecl NiD3DXEffectParser_GetIntegerDesc() {
    return Cleared<IntegerDesc>(ms_kIntegerDesc, sizeof(IntegerDesc), 5);
}
TextureDesc* __cdecl NiD3DXEffectParser_GetTextureDesc() {
    return Cleared<TextureDesc>(ms_kTextureDesc, sizeof(TextureDesc), 6);
}
PredefinedDesc* __cdecl NiD3DXEffectParser_GetPredefinedDesc() {
    return Cleared<PredefinedDesc>(ms_kPredefinedDesc, sizeof(PredefinedDesc), 7);
}
ObjectDesc* __cdecl NiD3DXEffectParser_GetObjectDesc() {
    return Cleared<ObjectDesc>(ms_kObjectDesc, sizeof(ObjectDesc), 8);
}
ArrayDesc* __cdecl NiD3DXEffectParser_GetArrayDesc() {
    return Cleared<ArrayDesc>(ms_kArrayDesc, sizeof(ArrayDesc), 9);
}

// 0x751CE0: a vector's kind (0 point2, 1 point3, 2 point4)
uint32_t __fastcall VectorDesc_GetTypeImpl(VectorDesc* self, re::Edx) {
    if (self->rows == 1 && self->columns == 2 && self->elements == 2) {
        return 0;
    }
    if (self->rows == 1 && self->columns == 3 && self->elements == 3) {
        return 1;
    }
    return 2;
}

// 0x751D20: an array's element kind (0 float, 1 point2, 2 point3, 4 colour, 3 point4)
uint32_t __fastcall ArrayDesc_GetTypeImpl(ArrayDesc* self, re::Edx) {
    if (self->rows == 1) {
        if (self->columns == 1) {
            return 0;
        }
        if (self->columns == 2) {
            return 1;
        }
        if (self->columns == 3) {
            return 2;
        }
        if (self->columns == 4 && self->color) {
            return 4;
        }
    }
    return 3;
}

// 0x751D80: an array's element size in bytes
uint32_t __fastcall ArrayDesc_GetElementSizeImpl(ArrayDesc* self, re::Edx) {
    if (self->rows == 1) {
        if (self->columns == 1) {
            return 4;
        }
        if (self->columns == 2) {
            return 8;
        }
        if (self->columns == 3) {
            return 0xC;
        }
    }
    return 0x10;
}

// 0x751DD0: float4 rows compacted to float2 / float3 / float4 (one register class only: 1); whether
// it did. (ExpandToVector4's inverse; copies the bits)
uint8_t __cdecl NiD3DXEffect_CompactFromVector4Impl(const float* from, float* to, int rows, uint32_t columns,
                                                    uint32_t count) {
    if (rows != 1 || columns < 2 || columns > 4) {
        return 0;
    }
    const uint32_t* source = reinterpret_cast<const uint32_t*>(from);
    uint32_t* target = reinterpret_cast<uint32_t*>(to);
    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t k = 0; k < columns; k++) {
            target[i * columns + k] = source[i * 4 + k];
        }
    }
    return 1;
}

} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NID3DXPARSER_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                                 \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetEffectDesc", 0x0074DE70, &ni::NiD3DXEffectParser_GetEffectDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::IsParameterUsed", 0x0074DEB0, &ni::NiD3DXEffectParser_IsParameterUsed);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ResetCounts", 0x0074DF20, &ni::NiD3DXEffectParser_ResetCounts);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseMatrixAnnotations", 0x0074DF60, &ni::NiD3DXEffectParser_ParseMatrixAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseArrayAnnotations", 0x0074E090, &ni::NiD3DXEffectParser_ParseArrayAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseFloatAnnotations", 0x0074E520, &ni::NiD3DXEffectParser_ParseFloatAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseBoolAnnotations", 0x0074E6D0, &ni::NiD3DXEffectParser_ParseBoolAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseIntegerAnnotations", 0x0074E800, &ni::NiD3DXEffectParser_ParseIntegerAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParsePredefinedAnnotations", 0x0074E9D0, &ni::NiD3DXEffectParser_ParsePredefinedAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseTechniqueAnnotations", 0x0074EA50, &ni::NiD3DXEffectParser_ParseTechniqueAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetAnnotationString", 0x0074EDB0, &ni::NiD3DXEffectParser_GetAnnotationString);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetObjectType", 0x0074EE40, &ni::NiD3DXEffectParser_GetObjectType);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::AdjustObjectMapping", 0x0074F070, &ni::NiD3DXEffectParser_AdjustObjectMapping);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseTechnique", 0x0074F390, &ni::NiD3DXEffectParser_ParseTechnique);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseVectorAnnotations", 0x0074F470, &ni::NiD3DXEffectParser_ParseVectorAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseTextureAnnotations", 0x0074F740, &ni::NiD3DXEffectParser_ParseTextureAnnotations);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::LookUpObjectMapping", 0x0074FB70, &ni::NiD3DXEffectParser_LookUpObjectMapping);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetTechnique", 0x0074FBC0, &ni::NiD3DXEffectParser_GetTechnique);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseObjectParameter", 0x0074FBE0, &ni::NiD3DXEffectParser_ParseObjectParameter);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParsePredefinedParameter", 0x00750860, &ni::NiD3DXEffectParser_ParsePredefinedParameter);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::ParseParameter", 0x00750AA0, &ni::NiD3DXEffectParser_ParseParameter);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetParameter", 0x00751070, &ni::NiD3DXEffectParser_GetParameter);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::CreateAttribute", 0x00751090, &ni::NiD3DXEffectAttributeTable_CreateAttribute);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::~NiD3DXEffectAttributeTable", 0x007510F0, &ni::NiD3DXEffectAttributeTable_Destruct);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::GetFirstAttribute", 0x00751140, &ni::NiD3DXEffectAttributeTable_GetFirst);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::GetNextAttribute", 0x00751160, &ni::NiD3DXEffectAttributeTable_GetNext);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::FindAttribute", 0x00751180, &ni::NiD3DXEffectAttributeTable_FindAttribute);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribute", 0x007511E0, &ni::NiD3DXEffectAttributeTable_AddAttribute);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Bool", 0x00751240, &ni::NiD3DXEffectAttributeTable_AddBool);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_UnsignedInt", 0x007512A0, &ni::NiD3DXEffectAttributeTable_AddUnsignedInt);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Float", 0x00751320, &ni::NiD3DXEffectAttributeTable_AddFloat);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Point2", 0x007513B0, &ni::NiD3DXEffectAttributeTable_AddPoint2);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Point3", 0x00751470, &ni::NiD3DXEffectAttributeTable_AddPoint3);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Point4", 0x00751540, &ni::NiD3DXEffectAttributeTable_AddPoint4);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Matrix3", 0x007515C0, &ni::NiD3DXEffectAttributeTable_AddMatrix3);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Matrix4", 0x00751670, &ni::NiD3DXEffectAttributeTable_AddMatrix4);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Color", 0x007516D0, &ni::NiD3DXEffectAttributeTable_AddColor);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_ColorA", 0x007517A0, &ni::NiD3DXEffectAttributeTable_AddColorA);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Texture", 0x00751880, &ni::NiD3DXEffectAttributeTable_AddTexture);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::AddAttribs_Array", 0x007518E0, &ni::NiD3DXEffectAttributeTable_AddArray);
NID3DXPARSER_REVERSED("NiD3DXEffectAttributeTable::NiD3DXEffectAttributeTable", 0x00751980, &ni::NiD3DXEffectAttributeTable_Construct);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetMatrixDesc", 0x00751B00, &ni::NiD3DXEffectParser_GetMatrixDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetVectorDesc", 0x00751B30, &ni::NiD3DXEffectParser_GetVectorDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetFloatDesc", 0x00751B60, &ni::NiD3DXEffectParser_GetFloatDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetBoolDesc", 0x00751BA0, &ni::NiD3DXEffectParser_GetBoolDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetIntegerDesc", 0x00751BD0, &ni::NiD3DXEffectParser_GetIntegerDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetTextureDesc", 0x00751C10, &ni::NiD3DXEffectParser_GetTextureDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetPredefinedDesc", 0x00751C40, &ni::NiD3DXEffectParser_GetPredefinedDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetObjectDesc", 0x00751C80, &ni::NiD3DXEffectParser_GetObjectDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParser::GetArrayDesc", 0x00751CB0, &ni::NiD3DXEffectParser_GetArrayDesc);
NID3DXPARSER_REVERSED("NiD3DXEffectParameterVectorDesc::GetVectorType", 0x00751CE0, &ni::VectorDesc_GetTypeImpl);
NID3DXPARSER_REVERSED("NiD3DXEffectParameterArrayDesc::GetArrayType", 0x00751D20, &ni::ArrayDesc_GetTypeImpl);
NID3DXPARSER_REVERSED("NiD3DXEffectParameterArrayDesc::GetElementSize", 0x00751D80, &ni::ArrayDesc_GetElementSizeImpl);
NID3DXPARSER_REVERSED("NiD3DXEffect::CompactFromVector4", 0x00751DD0, &ni::NiD3DXEffect_CompactFromVector4Impl);
