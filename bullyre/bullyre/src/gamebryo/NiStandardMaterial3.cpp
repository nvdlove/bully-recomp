// NiStandardMaterial's operator fragments (src/gamebryo/NiFragmentMaterial.h): the node of the
// right width for the resources' HLSL types (NormalizeFloat3, AddFloat4, LerpFloat2...) added to
// the configurator, its inputs bound, its output returned; and the splits (color and opacity,
// RGBA) and the type casts between float, float3 and float4.
//
// Each picks its node by the inputs' types, in the order the game tests them; the temporary node
// names are made and let go the way the game's code does at each place (see Temp).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::frag;

using Self = NiStandardMaterial;
using Resource = void;

// slot 70 NormalizeVector(context, vector&): the vector normalized in place (float4, float3,
// float2)
uint8_t __fastcall NormalizeVector(Self* self, re::Edx, MaterialContext* context, re::Ptr32<Resource>* vector) {
    void* node;
    if (*vector == nullptr) {
        return 0;
    }
    if (TypeIs(*vector, "float4")) {
        node = LibraryNode(self, "NormalizeFloat4");
    } else if (*vector == nullptr) {
        return 0;
    } else if (TypeIs(*vector, "float3")) {
        node = LibraryNode(self, "NormalizeFloat3");
    } else if (*vector == nullptr) {
        return 0;
    } else if (TypeIs(*vector, "float2")) {
        node = LibraryNode(self, "NormalizeFloat2", Temp::Destructed);
    } else {
        return 0;
    }
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, *vector, node, "VectorIn");
    *vector = OutputOf(node, "VectorOut");
    return 1;
}

// the binary operators' node by the two inputs' types (both the same width); null when they
// differ
const char* BinaryNodeName(Resource* a, Resource* b, const char* const names[4]) {
    if (TypeIs(a, "float4") && b != nullptr && TypeIs(b, "float4")) {
        return names[0];
    }
    if (TypeIs(a, "float3") && b != nullptr && TypeIs(b, "float3")) {
        return names[1];
    }
    if (TypeIs(a, "float2") && b != nullptr && TypeIs(b, "float2")) {
        return names[2];
    }
    if (TypeIs(a, "float") && b != nullptr && TypeIs(b, "float")) {
        return names[3];
    }
    return nullptr;
}

// AddVector / MultiplyVector(context, a, b, result&): with one input missing the other is the
// result; with their types differing the first is (no node, true)
uint8_t BinaryOperator(Self* self, MaterialContext* context, Resource* a, Resource* b, re::Ptr32<Resource>* result,
                       const char* const names[4]) {
    if (a == nullptr) {
        if (b == nullptr) {
            return 0;
        }
        *result = b;
        return 1;
    }
    const char* name = BinaryNodeName(a, b, names);
    if (name == nullptr) {
        *result = a;
        return 1;
    }
    void* node = LibraryNode(self, name, Temp::Constructed);
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, a, node, "V1");
    BindInput(context, b, node, "V2");
    *result = OutputOf(node, "Output");
    return 1;
}

constexpr const char* kAddNodes[4] = {"AddFloat4", "AddFloat3", "AddFloat2", "AddFloat"};
constexpr const char* kMultiplyNodes[4] = {"MultiplyFloat4", "MultiplyFloat3", "MultiplyFloat2", "MultiplyFloat"};
constexpr const char* kLerpNodes[4] = {"LerpFloat4", "LerpFloat3", "LerpFloat2", "LerpFloat"};

// slot 71
uint8_t __fastcall AddVector(Self* self, re::Edx, MaterialContext* context, Resource* a, Resource* b,
                             re::Ptr32<Resource>* result) {
    return BinaryOperator(self, context, a, b, result, kAddNodes);
}

// slot 72
uint8_t __fastcall MultiplyVector(Self* self, re::Edx, MaterialContext* context, Resource* a, Resource* b,
                                  re::Ptr32<Resource>* result) {
    return BinaryOperator(self, context, a, b, result, kMultiplyNodes);
}

// slot 73 MultiplyAddVector(context, a, b, c, result&): float3s, each bound through TypeCastBind
uint8_t __fastcall MultiplyAddVector(Self* self, re::Edx, MaterialContext* context, Resource* a, Resource* b,
                                     Resource* c, re::Ptr32<Resource>* result) {
    if (a == nullptr || b == nullptr || c == nullptr) {
        return 0;
    }
    void* node = LibraryNode(self, "MultiplyAddFloat3", Temp::Destructed);
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    Resource* inputs[3] = {a, b, c};
    const char* names[3] = {"V1", "V2", "V3"};
    for (int i = 0; i < 3; i++) {
        NiFixedString s = String(names[i]);
        void* to = Node_GetInputResourceByVariableName(node, &s);
        Virtual<uint8_t>(self, 82, context, inputs[i], to);   // TypeCastBind
        Drop(s);
    }
    *result = OutputOf(node, "Output");
    return 1;
}

// slot 74 LerpVector(context, a, b, amount, result&): a and b of the same width (false
// otherwise)
uint8_t __fastcall LerpVector(Self* self, re::Edx, MaterialContext* context, Resource* a, Resource* b,
                              Resource* amount, re::Ptr32<Resource>* result) {
    if (a == nullptr) {
        return 0;
    }
    const char* name = BinaryNodeName(a, b, kLerpNodes);
    if (name == nullptr) {
        return 0;
    }
    void* node = LibraryNode(self, name, Temp::Constructed);
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, a, node, "V1");
    BindInput(context, b, node, "V2");
    BindInput(context, amount, node, "LerpAmount");
    *result = OutputOf(node, "Output");
    return 1;
}

// slot 75 ScaleVector(context, vector, scale, result&): a vector by a float (without a scale the
// vector is the result). A float by a float takes MultiplyFloat, whose second input isn't
// "Scale": that binding fails, as the game has it
uint8_t __fastcall ScaleVector(Self* self, re::Edx, MaterialContext* context, Resource* vector, Resource* scale,
                               re::Ptr32<Resource>* result) {
    if (scale == nullptr) {
        if (vector == nullptr) {
            return 0;
        }
        *result = vector;
        return 1;
    }
    if (vector == nullptr) {
        return 0;
    }
    const char* name;
    if (TypeIs(vector, "float4") && TypeIs(scale, "float")) {
        name = "ScaleFloat4";
    } else if (TypeIs(vector, "float3") && TypeIs(scale, "float")) {
        name = "ScaleFloat3";
    } else if (TypeIs(vector, "float2") && TypeIs(scale, "float")) {
        name = "ScaleFloat2";
    } else if (TypeIs(vector, "float") && TypeIs(scale, "float")) {
        name = "MultiplyFloat";
    } else {
        return 0;
    }
    void* node = LibraryNode(self, name, Temp::Constructed);
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, vector, node, "V1");
    BindInput(context, scale, node, "Scale");
    *result = OutputOf(node, "Output");
    return 1;
}

// slot 76 SaturateVector(context, vector, result&)
uint8_t __fastcall SaturateVector(Self* self, re::Edx, MaterialContext* context, Resource* vector, re::Ptr32<Resource>* result) {
    if (vector == nullptr) {
        return 0;
    }
    void* node;
    if (TypeIs(vector, "float4")) {
        node = LibraryNode(self, "SaturateFloat4");
    } else if (TypeIs(vector, "float3")) {
        node = LibraryNode(self, "SaturateFloat3");
    } else if (TypeIs(vector, "float2")) {
        node = LibraryNode(self, "SaturateFloat2", Temp::Destructed);
    } else if (TypeIs(vector, "float")) {
        node = LibraryNode(self, "SaturateFloat", Temp::Destructed);
    } else {
        return 0;
    }
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, vector, node, "V1");
    *result = OutputOf(node, "Output");
    return 1;
}

// slot 77 MultiplyScalarSatAddVector(context, a, b, scalar, result&): float3s (either input may
// be missing; the first present one decides the type)
uint8_t __fastcall MultiplyScalarSatAddVector(Self* self, re::Edx, MaterialContext* context, Resource* a, Resource* b,
                                              Resource* scalar, re::Ptr32<Resource>* result) {
    if (!(a != nullptr && TypeIs(a, "float3"))) {
        if (b == nullptr || !TypeIs(b, "float3")) {
            return 0;
        }
    }
    void* node = LibraryNode(self, "MultiplyScalarSatAddFloat3");
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    if (a != nullptr) {
        BindInput(context, a, node, "V1");
    }
    if (b != nullptr) {
        BindInput(context, b, node, "V2");
    }
    if (scalar != nullptr) {
        BindInput(context, scalar, node, "Scalar");
    }
    *result = OutputOf(node, "Output");
    return 1;
}

// slot 80 SplitColorAndOpacity(context, colorAndOpacity, color&, opacity&): a float4 split
uint8_t __fastcall SplitColorAndOpacity(Self* self, re::Edx, MaterialContext* context, Resource* colorAndOpacity,
                                        re::Ptr32<Resource>* color, re::Ptr32<Resource>* opacity) {
    if (colorAndOpacity == nullptr || !TypeIs(colorAndOpacity, "float4")) {
        return 0;
    }
    void* node = LibraryNode(self, "SplitColorAndOpacity");
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, colorAndOpacity, node, "ColorAndOpacity");
    *color = OutputOf(node, "Color");
    *opacity = OutputOf(node, "Opacity");
    return 1;
}

// slot 81 SplitRGBA(context, rgba, red&, green&, blue&, alpha&)
uint8_t __fastcall SplitRGBA(Self* self, re::Edx, MaterialContext* context, Resource* rgba, re::Ptr32<Resource>* red,
                             re::Ptr32<Resource>* green, re::Ptr32<Resource>* blue, re::Ptr32<Resource>* alpha) {
    if (rgba == nullptr || !TypeIs(rgba, "float4")) {
        return 0;
    }
    void* node = LibraryNode(self, "SplitRGBA");
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, rgba, node, "ColorAndOpacity");
    *red = OutputOf(node, "Red");
    *green = OutputOf(node, "Green");
    *blue = OutputOf(node, "Blue");
    *alpha = OutputOf(node, "Alpha");
    return 1;
}

// slot 82 TypeCastBind(context, from, to): bound through a conversion node when the widths are a
// float and a float3 / float4 (or back); otherwise directly when the configurator's slot 4 says
// the types bind
uint8_t __fastcall TypeCastBind(Self* self, re::Edx, MaterialContext* context, Resource* from, Resource* to) {
    if (from == nullptr) {
        return 0;
    }
    const char* name = nullptr;
    if (TypeIs(from, "float") && to != nullptr && TypeIs(to, "float3")) {
        name = "FloatToFloat3";
    } else if (TypeIs(from, "float") && to != nullptr && TypeIs(to, "float4")) {
        name = "FloatToFloat4";
    } else if (TypeIs(from, "float4") && to != nullptr && TypeIs(to, "float")) {
        name = "Float4ToFloat";
    } else if (TypeIs(from, "float3")) {
        if (to == nullptr) {
            return 0;
        }
        if (TypeIs(to, "float")) {
            name = "Float3ToFloat";
        }
    }
    if (name == nullptr) {
        if (to == nullptr) {
            return 0;
        }
        void* configurator = context->configurator;
        const re::Ptr32<const char>* toType = Resource_GetType(to);
        const re::Ptr32<const char>* fromType = Resource_GetType(from);
        if (!Virtual<uint8_t>(configurator, 4, fromType, toType)) {   // CanBindTypes
            return 0;
        }
        return Configurator_AddBinding(context->configurator, from, to);
    }
    void* node = LibraryNode(self, name, Temp::Constructed);
    if (node == nullptr) {
        return 0;
    }
    AddNode(context, node);
    BindInput(context, from, node, "Input");
    NiFixedString s = String("Output");
    void* configurator = context->configurator;
    Configurator_AddBinding(configurator, Node_GetOutputResourceByVariableName(node, &s), to);
    Drop(s);
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTDMAT3_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTDMAT3_REVERSED("NiStandardMaterial::NormalizeVector", 0x0078D310, &ni::NormalizeVector);
NISTDMAT3_REVERSED("NiStandardMaterial::AddVector", 0x0078D530, &ni::AddVector);
NISTDMAT3_REVERSED("NiStandardMaterial::MultiplyVector", 0x0078D820, &ni::MultiplyVector);
NISTDMAT3_REVERSED("NiStandardMaterial::MultiplyAddVector", 0x0078DB10, &ni::MultiplyAddVector);
NISTDMAT3_REVERSED("NiStandardMaterial::LerpVector", 0x0078DCF0, &ni::LerpVector);
NISTDMAT3_REVERSED("NiStandardMaterial::ScaleVector", 0x0078DFF0, &ni::ScaleVector);
NISTDMAT3_REVERSED("NiStandardMaterial::SaturateVector", 0x0078E2B0, &ni::SaturateVector);
NISTDMAT3_REVERSED("NiStandardMaterial::MultiplyScalarSatAddVector", 0x0078E510, &ni::MultiplyScalarSatAddVector);
NISTDMAT3_REVERSED("NiStandardMaterial::SplitColorAndOpacity", 0x0078E740, &ni::SplitColorAndOpacity);
NISTDMAT3_REVERSED("NiStandardMaterial::SplitRGBA", 0x0078E8D0, &ni::SplitRGBA);
NISTDMAT3_REVERSED("NiStandardMaterial::TypeCastBind", 0x0078EB00, &ni::TypeCastBind);
