// The standard material's descriptor (five 32-bit bit fields that key a shader: 0x14 bytes,
// zeroed by NiMaterialDescriptor's constructor 0x7D5470): the light counts (point bits 4..7 of
// the third word, spot 8..11, directional 12..15) and their shadow flags (bits 16..23), the
// projected lights and shadows, the texture maps' UV sets and transforms. Also
// NiSkinInstance::ContainsVertexData, which the material's vertex-data checks call.
//
// The bit-field writes keep the game's quirks: values are or'ed in unmasked (a value too wide for
// its field spills into the next), and a light index of 8 or more shifts out of its byte.

#include <cstddef>
#include <cstdint>

#include "gamebryo/NiCore.h"
#include "gamebryo/NiSceneCommon.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::scene;

struct Descriptor {
    uint32_t bits[5];   // +0x0
};
static_assert(sizeof(Descriptor) == 0x14);

GAME_VAR(uint32_t, NiSpotLight_RTTIIndex, 0x020E2CA4);    // NiSpotLight's RTTI's kind-of index
GAME_VAR(uint32_t, NiPointLight_RTTIIndex, 0x020E2CD8);
GAME_VAR(uint8_t, NiDirectionalLight_RTTI, 0x020E39AC);   // (its RTTI, as IsKindOf takes it)
GAME_VAR(void*, NiShadowManager_ms_pkShadowManager, 0x020E2410);

GAME_FN(void, __thiscall, MaterialDescriptor_Construct, 0x007D5470, Descriptor* self);
GAME_FN(uint8_t, __cdecl, IsKindOf, 0x004066F0, const void* rtti, const void* object);
// NiShadowGenerator's (another chunk's)
GAME_FN(uint8_t, __thiscall, ShadowGenerator_IsUnaffectedReceiverNode, 0x007731D0, void* self, void* geometry);
GAME_FN(uint32_t, __thiscall, ShadowGenerator_GetShadowMapCount, 0x00772440, void* self);
GAME_FN(void*, __thiscall, ShadowGenerator_GetShadowMap, 0x00772460, void* self, uint32_t index);
// (this file's, called where modes apply)
GAME_FN(void, __thiscall, Descriptor_SetShadowFlag, 0x00777750, Descriptor* self, uint32_t index, int flag);

// whether an object's class is a kind of the one with this RTTI index (the RTTI's bit set,
// inlined)
inline bool KindOfIndex(void* object, uint32_t index) {
    const uint32_t* bits = reinterpret_cast<const uint32_t*>(Virtual<uint8_t*>(object, 2) + 0xC);
    return (bits[index >> 5] & (1u << (index & 31))) != 0;
}

// ---- NiSkinInstance

// ContainsVertexData(parameter): blend weights with a partition, blend indices with more than 4
// bones in its first partition, else the first partition's buffer data's (slot 1, a tail call)
uint8_t __fastcall SkinContainsVertexData(void* self, re::Edx, int parameter) {
    if (parameter == 1) {
        return At<void*>(self, 0xC) != nullptr ? 1 : 0;
    }
    if (parameter == 2) {
        void* partition = At<void*>(self, 0xC);
        if (partition != nullptr) {
            void* partitions = At<void*>(partition, 0xC);
            if (partitions != nullptr && At<uint16_t>(partitions, 0x20) > 4) {
                return 1;
            }
        }
        return 0;
    }
    void* partition = At<void*>(self, 0xC);
    if (partition != nullptr && At<uint32_t>(partition, 8) != 0) {
        void* buffer = At<void*>(At<void*>(partition, 0xC), 0x28);
        if (buffer != nullptr) {
            return Virtual<uint8_t>(buffer, 1, parameter);
        }
    }
    return 0;
}

// ---- the descriptor

// GetLightType(index, type&): the lights are numbered point, spot, then directional (with the
// game's off-by-one: index == the point count is still a point light); false past them all
uint8_t __fastcall GetLightType(Descriptor* self, re::Edx, uint32_t index, uint32_t* type) {
    uint32_t d = self->bits[2];
    uint32_t spot = (d >> 8) & 0xF;
    uint32_t directional = (d >> 12) & 0xF;
    uint32_t point = (d >> 4) & 0xF;
    if (index > spot + directional + point) {
        return 0;
    }
    if (index > directional + point) {
        *type = 2;
        return 1;
    }
    if (index > point) {
        *type = 0;
        return 1;
    }
    *type = 1;
    return 1;
}

// GetShadowFlag(index, flag&)
uint8_t __fastcall GetShadowFlag(Descriptor* self, re::Edx, uint32_t index, uint8_t* flag) {
    uint32_t d = self->bits[2];
    if (index > ((d >> 12) & 0xF) + ((d >> 8) & 0xF) + ((d >> 4) & 0xF)) {
        return 0;
    }
    uint8_t mask = uint8_t(1u << (index & 31));
    *flag = (uint8_t(d >> 16) & mask) != 0 ? 1 : 0;
    return 1;
}

void __fastcall SetShadowFlag(Descriptor* self, re::Edx, uint32_t index, int flag) {
    uint32_t mask = 1u << (index & 31);
    uint32_t value = uint8_t(self->bits[2] >> 16);
    if (uint8_t(flag) != 0) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    self->bits[2] = (self->bits[2] & 0xFF00FFFF) | (value << 16);
}

// SetTextureUsage(map, uvSet, transform): a 3-bit UV set and a transform bit per map, 4 bits a
// map from bit 8 of the fourth word (maps 0..5) and from bit 0 of the fifth (6..11)
void __fastcall SetTextureUsage(Descriptor* self, re::Edx, uint32_t map, uint32_t uvSet, uint32_t transform) {
    if (map > 11) {
        return;
    }
    uint32_t& word = map < 6 ? self->bits[3] : self->bits[4];
    uint32_t shift = map < 6 ? 8 + map * 4 : (map - 6) * 4;
    uint32_t value = (word & ~(7u << shift)) | (uvSet << shift);
    word = value;
    word = (value & ~(8u << shift)) | (transform << (shift + 3));
}

// the count of the vertex inputs the second and third words ask for
uint32_t __fastcall GetInputCount(Descriptor* self, re::Edx) {
    uint32_t a = self->bits[1];
    uint32_t b = self->bits[2];
    uint32_t count = (b >> 1) & 3;
    static const uint32_t kBits[] = {30, 29, 28, 24, 23, 22, 21, 20, 17, 16, 15};
    for (uint32_t bit : kBits) {
        count += (a >> bit) & 1;
    }
    count += b & 1;
    count += a >> 31;
    return count;
}

// SetProjectedLight(index, clipped, type): the clipped bits 29..31 of the third word, the
// type bits 26..28 (set unless the type is 1)
void __fastcall SetProjectedLight(Descriptor* self, re::Edx, uint32_t index, int clipped, int type) {
    uint32_t mask = 1u << (index & 31);
    uint32_t value = self->bits[2] >> 29;
    if (uint8_t(clipped) != 0) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    self->bits[2] = (self->bits[2] & 0x1FFFFFFF) | (value << 29);
    value = (self->bits[2] >> 26) & 7;
    if (type != 1) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    self->bits[2] = (self->bits[2] & 0xE3FFFFFF) | (value << 26);
}

// SetProjectedShadow(index, clipped, type): bits 5..7 and 2..4 of the fourth word
void __fastcall SetProjectedShadow(Descriptor* self, re::Edx, uint32_t index, int clipped, int type) {
    uint32_t mask = 1u << (index & 31);
    uint32_t value = (self->bits[3] >> 5) & 7;
    if (uint8_t(clipped) != 0) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    self->bits[3] = (self->bits[3] & 0xFFFFFF1F) | (value << 5);
    value = (self->bits[3] >> 2) & 7;
    if (type != 1) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    self->bits[3] = (value * 4) | (self->bits[3] & 0xFFFFFFE3);
}

Descriptor* __fastcall Construct(Descriptor* self, re::Edx) {
    MaterialDescriptor_Construct(self);
    return self;
}

// SetLight(light, index, geometry): the light counted by its kind (spot, point, directional; a
// null light is read past anyway: the game's crash), and, for a light whose shadow generator
// casts on this geometry, the shadow flag set and the shadow map's count kept (bits 28..29 of
// the fifth word)
uint8_t __fastcall SetLight(Descriptor* self, re::Edx, void* light, uint32_t index, void* geometry) {
    if (index >= 8) {
        return 0;
    }
    uint8_t counted = 0;
    if (light != nullptr && KindOfIndex(light, NiSpotLight_RTTIIndex)) {
        self->bits[2] = ((self->bits[2] & 0xF00) + 0x100) | (self->bits[2] & 0xFFFFF0FF);
        counted = 1;
    } else if (light != nullptr && KindOfIndex(light, NiPointLight_RTTIIndex)) {
        self->bits[2] = ((self->bits[2] & 0xF0) + 0x10) | (self->bits[2] & 0xFFFFFF0F);
        counted = 1;
    } else if (IsKindOf(&NiDirectionalLight_RTTI, light)) {
        self->bits[2] = ((self->bits[2] & 0xF000) + 0x1000) | (self->bits[2] & 0xFFFF0FFF);
        counted = 1;
    }
    void* generator = At<void*>(light, 0xD4);
    if (!counted || generator == nullptr || NiShadowManager_ms_pkShadowManager == nullptr ||
        At<uint8_t>(NiShadowManager_ms_pkShadowManager, 0xBC) == 0) {
        return counted;
    }
    if (((At<uint8_t>(generator, 8) >> 3) & 1) != 0 && !ShadowGenerator_IsUnaffectedReceiverNode(generator, geometry)) {
        uint32_t count = ShadowGenerator_GetShadowMapCount(generator);
        for (uint32_t i = 0; i < count; i++) {
            if (ShadowGenerator_GetShadowMap(generator, i) != nullptr) {
                void* shadowMap = At<void*>(generator, 0xC);
                if (shadowMap == nullptr) {
                    return counted;
                }
                uint32_t value = At<uint16_t>(shadowMap, 0x44);
                Descriptor_SetShadowFlag(self, index, 1);
                if (value > ((self->bits[4] >> 28) & 3)) {
                    self->bits[4] = (self->bits[4] & 0xCFFFFFFF) | (value << 28);
                }
                return counted;
            }
        }
    }
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NIMATDESC_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NIMATDESC_REVERSED("NiSkinInstance::ContainsVertexData", 0x00776F00, &ni::SkinContainsVertexData);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::GetLightType", 0x00777690, &ni::GetLightType);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::GetShadowFlag", 0x00777700, &ni::GetShadowFlag);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::SetShadowFlag", 0x00777750, &ni::SetShadowFlag);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::SetTextureUsage", 0x007777A0, &ni::SetTextureUsage);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::GetInputCount", 0x007779D0, &ni::GetInputCount);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::SetProjectedLight", 0x00777A60, &ni::SetProjectedLight);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::SetProjectedShadow", 0x00777AD0, &ni::SetProjectedShadow);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::NiStandardMaterialDescriptor", 0x00777B50, &ni::Construct);
NIMATDESC_REVERSED("NiStandardMaterialDescriptor::SetLight", 0x00777B60, &ni::SetLight);
