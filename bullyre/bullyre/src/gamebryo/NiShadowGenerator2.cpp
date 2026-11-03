// NiShadowGenerator's default callbacks (the single shadow map kept in its extra data pointer:
// assign, retrieve, count, the map for a geometry, cleanup; and the shader constants it hands
// the materials: the depth bias and the VSM power / epsilon), its depth bias test, the
// constructor without a light, CreateClone; and NiShadowManager's technique lookup by name.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiDynamicEffect.h"
#include "gamebryo/NiShadowGenerator.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

constexpr uintptr_t NiShadowGenerator_vtable = 0x00952E20;
constexpr uintptr_t NodeList_vtable = 0x0095145C;

GAME_VAR(uint8_t*, ms_pkShadowManager, 0x020E2410);
GAME_VAR(float, kVSMPower, 0x00900D38);     // 10
GAME_VAR(float, kVSMEpsilon, 0x00900C04);   // 0.001

GAME_FN(void, __thiscall, RefObject_Construct, 0x00755260, void* self);
GAME_FN(void, __thiscall, Object_CopyMembers, 0x00755410, void* self, void* dest, void* cloning);
GAME_FN(void, __thiscall, Generator_MarkShadowMapsDirty, 0x00772930, NiShadowGenerator* self);
GAME_FN(uint32_t, __cdecl, GetMaterialIndex, 0x0040DD70, uint32_t lightType);   // (the game's)
// (this file's, called where modes apply)
GAME_FN(uint8_t, __thiscall, Generator_IsDepthBiasDefault, 0x00772480, NiShadowGenerator* self);
GAME_FN(NiShadowGenerator*, __thiscall, Generator_ConstructDefault, 0x00773070, void* self);

// a technique's default depth biases (+0x20: three per light type, then the back-face ones)
inline float TechniqueBias(void* technique, uint32_t index) {
    return At<float>(technique, 0x20 + int(index) * 4);
}

} // namespace

// 0x757220: NiShadowManager::GetKnownShadowTechnique(name): the one of that name's handle
void* __cdecl NiShadowManager_GetKnownShadowTechnique(const re::Ptr32<const char>* name) {
    uint8_t* manager = ms_pkShadowManager;
    for (uint32_t i = 0; i < 16; i++) {
        void* technique = At<void*>(manager, 0x10 + int(i) * 4);
        if (technique != nullptr && At<const char*>(technique, 4) == *name) {
            return At<void*>(manager, 0x10 + int(i) * 4);
        }
    }
    return nullptr;
}

// 0x772410: the single map retrieved (index 0 only)
void* __cdecl NiShadowGenerator_SingleShadowMapRetrieve(NiShadowGenerator* generator, uint32_t index) {
    if (index > 0) {
        return nullptr;
    }
    return generator->extraData;
}

// 0x772430: the single map's count (0 or 1)
uint32_t __cdecl NiShadowGenerator_SingleShadowMapCount(NiShadowGenerator* generator) {
    return generator->extraData != nullptr ? 1 : 0;
}

// 0x772480: whether the depth bias is the technique's default for the light (the back-face set
// when the generator doesn't render back faces)
uint8_t __fastcall NiShadowGenerator_IsDepthBiasDefault(NiShadowGenerator* self, re::Edx) {
    void* technique = self->technique;
    if (technique == nullptr || self->dynamicEffect == nullptr) {
        return 0;
    }
    uint8_t type = self->dynamicEffect->effectType;
    if (type == 0) {
        return 0;
    }
    uint32_t index = GetMaterialIndex(type);
    if (((uint8_t(self->flags) >> 4) & 1) == 0) {
        index += 3;
    }
    return self->depthBias == TechniqueBias(technique, index) ? 1 : 0;
}

// 0x772510: the single map assigned (index 0 only)
uint8_t __cdecl NiShadowGenerator_SingleShadowMapAssign(NiShadowGenerator* generator, void*, void* map,
                                                        uint32_t index) {
    if (index > 0) {
        return 0;
    }
    generator->extraData = map;
    return 1;
}

// 0x772530: the single map for a geometry (index 0 or 0xFF)
void* __cdecl NiShadowGenerator_SingleShadowMapGet(NiShadowGenerator* generator, void*, uint32_t index) {
    if (index != 0xFF && index > 0) {
        return nullptr;
    }
    return generator->extraData;
}

// 0x772550: the single map forgotten
void __cdecl NiShadowGenerator_SingleShadowMapCleanup(NiShadowGenerator* generator) {
    generator->extraData = nullptr;
}

// 0x772AC0: the default shader constants: 0x1C the depth bias (the technique's own one for the
// light when the bias is the default and the receiver's alpha test is on), 0x1D the VSM power and
// epsilon
uint8_t __cdecl NiShadowGenerator_DefaultGetShaderConstant(float* data, uint32_t, NiShadowGenerator* generator,
                                                           void*, void*, uint32_t constant, void* entry) {
    if (constant == 0x1C) {
        float bias = generator->depthBias;
        if ((At<uint16_t>(At<void*>(entry, 0x24), 0x18) & 0xC00) == 0xC00 &&
            ((uint8_t(generator->flags) >> 4) & 1) != 0 && Generator_IsDepthBiasDefault(generator)) {
            uint32_t index = GetMaterialIndex(generator->dynamicEffect->effectType);
            bias = At<float>(generator->technique, 0x2C + int(index) * 4);
        }
        data[0] = bias;
        data[1] = 0.0f;
        data[2] = 0.0f;
        data[3] = 0.0f;
        return 1;
    }
    if (constant == 0x1D) {
        data[0] = kVSMPower;
        data[1] = kVSMEpsilon;
        data[2] = 0.0f;
        data[3] = 0.0f;
        return 1;
    }
    return 0;
}

// 0x773070: the constructor without a light (active, no maps yet)
NiShadowGenerator* __fastcall NiShadowGenerator_ConstructDefault(NiShadowGenerator* self, re::Edx) {
    RefObject_Construct(self);
    self->technique = nullptr;
    self->vtable = reinterpret_cast<void*>(NiShadowGenerator_vtable);
    self->unaffectedCasters.count = 0;
    self->unaffectedCasters.head = nullptr;
    self->unaffectedCasters.tail = nullptr;
    self->unaffectedCasters.vtable = reinterpret_cast<void*>(NodeList_vtable);
    self->unaffectedReceivers.count = 0;
    self->unaffectedReceivers.head = nullptr;
    self->unaffectedReceivers.tail = nullptr;
    self->unaffectedReceivers.vtable = reinterpret_cast<void*>(NodeList_vtable);
    self->depthBias = 0.0f;
    self->flags |= 2;
    self->unknown58 = 0.0f;
    self->callback30 = nullptr;
    self->unknown5C = 0.0f;
    self->assignShadowMap = nullptr;
    self->unknown60 = 0.0f;
    self->retrieveShadowMap = nullptr;
    self->shadowMapCount = nullptr;
    self->callback40 = nullptr;
    self->callback44 = nullptr;
    self->cleanup = nullptr;
    self->dynamicEffect = nullptr;
    self->extraData = nullptr;
    self->sizeHint = 0x400;
    if (((uint8_t(self->flags) >> 2) & 1) == 0) {
        Generator_MarkShadowMapsDirty(self);
    }
    self->flags = uint16_t((self->flags & 0xFFDB) | 0x3D8);
    return self;
}

// 0x7732D0: CreateClone (slot 3): the technique, flags, biases and size hint (not the light)
NiShadowGenerator* __fastcall NiShadowGenerator_CreateClone(NiShadowGenerator* self, re::Edx, void* cloning) {
    void* memory = NiNew(sizeof(NiShadowGenerator));
    NiShadowGenerator* clone = memory != nullptr ? Generator_ConstructDefault(memory) : nullptr;
    Object_CopyMembers(self, clone, cloning);
    clone->technique = self->technique;
    clone->flags = self->flags;
    clone->depthBias = self->depthBias;
    clone->unknown58 = self->unknown58;
    clone->unknown5C = self->unknown5C;
    clone->unknown60 = self->unknown60;
    clone->sizeHint = self->sizeHint;
    return clone;
}

#define NISHADOWGEN2_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NISHADOWGEN2_REVERSED("NiShadowManager::GetKnownShadowTechnique", 0x00757220, NiShadowManager_GetKnownShadowTechnique);
NISHADOWGEN2_REVERSED("NiShadowGenerator::SingleShadowMapRetrieve", 0x00772410, NiShadowGenerator_SingleShadowMapRetrieve);
NISHADOWGEN2_REVERSED("NiShadowGenerator::SingleShadowMapCount", 0x00772430, NiShadowGenerator_SingleShadowMapCount);
NISHADOWGEN2_REVERSED("NiShadowGenerator::IsDepthBiasDefault", 0x00772480, NiShadowGenerator_IsDepthBiasDefault);
NISHADOWGEN2_REVERSED("NiShadowGenerator::SingleShadowMapAssign", 0x00772510, NiShadowGenerator_SingleShadowMapAssign);
NISHADOWGEN2_REVERSED("NiShadowGenerator::SingleShadowMapGet", 0x00772530, NiShadowGenerator_SingleShadowMapGet);
NISHADOWGEN2_REVERSED("NiShadowGenerator::SingleShadowMapCleanup", 0x00772550, NiShadowGenerator_SingleShadowMapCleanup);
NISHADOWGEN2_REVERSED("NiShadowGenerator::DefaultGetShaderConstant", 0x00772AC0, NiShadowGenerator_DefaultGetShaderConstant);
NISHADOWGEN2_REVERSED("NiShadowGenerator::NiShadowGeneratorDefault", 0x00773070, NiShadowGenerator_ConstructDefault);
NISHADOWGEN2_REVERSED("NiShadowGenerator::CreateClone", 0x007732D0, NiShadowGenerator_CreateClone);

} // namespace ni
