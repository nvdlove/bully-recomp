// NiFragmentMaterial's geometry and pixel program slots (the program caches' programs of the
// descriptor's name), AddOutputPredefined, NiTSet<ObjectResourceOffset>::Add, and
// NiStandardMaterial's DropShadowMaps fallback (NiStandardMaterial4.cpp has the same code
// inlined in the fallbacks that start with it).

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiFragmentMaterial.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::frag;

using Self = NiFragmentMaterial;

GAME_FN(int, __cdecl, FormatString, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(uint8_t, __cdecl, ConstantMap_LookUpPredefinedMappingName, 0x0077F610, uint32_t mapping,
        re::Ptr32<const char>* name);
GAME_FN(uint32_t, __cdecl, ConstantMap_GetPredefinedMappingType, 0x0077EF10, uint32_t mapping,
        uint32_t registers);
GAME_FN(void, __thiscall, ObjectOffsets_Realloc, 0x007872C0, NiTSet<ObjectResourceOffset>* self, uint32_t allocated);

// a program cache's program of the descriptor's name (its slot 3, FindCachedProgram(name,
// uniforms, failed&))
inline void* CachedProgram(Self* self, int cache, const NiMaterialDescriptor* desc, void* uniforms) {
    char name[0x104];
    Descriptor_ToString(desc, name, 0x30);
    void* programCache = self->programCaches[cache];
    if (programCache == nullptr) {
        return nullptr;
    }
    uint8_t failed = 0;
    return Virtual<void*>(programCache, 3, name, uniforms, &failed);
}

} // namespace

// 0x7865F0: slot 22, the geometry program (the geometry cache's)
void* __fastcall NiFragmentMaterial_GenerateGeometryShader(Self* self, re::Edx, const NiMaterialDescriptor* desc,
                                                           void* uniforms) {
    return CachedProgram(self, 2, desc, uniforms);
}

// 0x786660: slot 23, the pixel program (the pixel cache's)
void* __fastcall NiFragmentMaterial_GeneratePixelShader(Self* self, re::Edx, const NiMaterialDescriptor* desc,
                                                        void* uniforms) {
    return CachedProgram(self, 1, desc, uniforms);
}

// 0x7875E0: slot 31, AddOutputPredefined(node, mapping, registers, count, extra data): a uniform
// "g_<mapping>" of the mapping's type (source 2); the mapping's name looked up whether or not it
// exists, as the game has it
void* __fastcall NiFragmentMaterial_AddOutputPredefined(Self* self, re::Edx, void* node, uint32_t mapping,
                                                        uint32_t registers, uint32_t count, uint32_t extraData) {
    NiFixedString mappingName = NullFixedString;
    ConstantMap_LookUpPredefinedMappingName(mapping, &mappingName);
    uint32_t attributeType = ConstantMap_GetPredefinedMappingType(mapping, registers);
    NiFixedString type;
    Virtual<re::Ptr32<const char>*>(self, 17, &type, attributeType);
    char variableName[0x100];
    FormatString(variableName, 0x100, "g_%s", mappingName);
    NiFixedString variable = String(variableName);
    NiFixedString label = String("");
    void* resource = Node_AddOutputResource(node, &type, &mappingName, &label, &variable, count, 2, 0, extraData);
    Drop(label);
    Drop(variable);
    Drop(type);
    Drop(mappingName);
    return resource;
}

// 0x7879E0: NiTSet<ObjectResourceOffset>::Add (the set doubled when full, from 1)
void __fastcall NiTSet_ObjectResourceOffset_Add(NiTSet<ObjectResourceOffset>* self, re::Edx,
                                                const ObjectResourceOffset* element) {
    if (self->count == self->allocated) {
        ObjectOffsets_Realloc(self, self->allocated != 0 ? self->allocated * 2 : 1);
    }
    self->base[self->count] = *element;
    self->count++;
}

// 0x790270: NiStandardMaterial's DropShadowMaps fallback: the failed pass's shadow maps dropped
// (its pixel program's shadow mask), when the pixel program failed
uint8_t __cdecl NiStandardMaterial_DropShadowMaps(NiMaterialDescriptor*, uint32_t rc, uint32_t failedPass,
                                                  RenderPassDescriptor* passes, uint32_t, uint32_t*, uint32_t*) {
    if ((rc & 1) == 0) {
        return 0;
    }
    uint8_t* mask = reinterpret_cast<uint8_t*>(&passes[failedPass].pixelDesc.bits[3]) + 1;
    if (*mask == 0) {
        return 0;
    }
    *mask = 0;
    return 1;
}

#define NIFRAGMAT2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIFRAGMAT2_REVERSED("NiFragmentMaterial::GenerateGeometryShader", 0x007865F0, NiFragmentMaterial_GenerateGeometryShader);
NIFRAGMAT2_REVERSED("NiFragmentMaterial::GeneratePixelShader", 0x00786660, NiFragmentMaterial_GeneratePixelShader);
NIFRAGMAT2_REVERSED("NiFragmentMaterial::AddOutputPredefined", 0x007875E0, NiFragmentMaterial_AddOutputPredefined);
NIFRAGMAT2_REVERSED("NiTSet<ObjectResourceOffset>::Add", 0x007879E0, NiTSet_ObjectResourceOffset_Add);
NIFRAGMAT2_REVERSED("NiStandardMaterial::DropShadowMaps", 0x00790270, NiStandardMaterial_DropShadowMaps);

} // namespace ni
