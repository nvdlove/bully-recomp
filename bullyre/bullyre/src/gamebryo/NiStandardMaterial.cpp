// NiStandardMaterial (src/gamebryo/NiFragmentMaterial.h): the descriptor a geometry's
// properties and effects make (GenerateDescriptor), the vertex inputs it asks for, the shader
// (the renderer's) and its vertex declaration, the generation context's release, and the
// static tables from a texture map to its name, its texture transform mapping and its sampler
// index, and the texture coordinates' assignment.
//
// The descriptor's words (bits[0..3], after the identifier): bits[0] the skinning (bits 0..1),
// normals (2 normals, 3 normals binormals tangents), specular (4), vertex colors (5), the vertex
// color mode (6..7) and lighting mode (8), lighting (9), fog (10..11), the UV set count
// (12..14), the maps (15..24; the decal count 20..22), the environment map's texgen (25..27),
// the shader maps (28..31); bits[1] the fifth shader map (0), the decals (1..2), per-vertex
// lighting (3), the light counts (4..15), the shadow flags (16..23), the projected lights
// (24..25); bits[2] the projected shadows (0..1); bits[3] bit 30 the alpha test.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiFragmentMaterial.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::frag;

using Self = NiStandardMaterial;

GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);
// NiShaderDeclaration's usage names (POSITION, BLENDWEIGHT, BLENDINDICES, NORMAL, PSIZE, TEXCOORD,
// TANGENT, BINORMAL, TESSFACTOR, POSITIONT, COLOR...)
GAME_ARRAY(NiFixedString, UsageNames, 0x020E31CC);
// NiStandardMaterial's map names past NiTexturingProperty's (shadow maps, projected lights,
// projected shadows)
GAME_ARRAY(NiFixedString, StandardMapNames, 0x020E23F0);

GAME_FN(uint8_t, __thiscall, Geometry_ContainsVertexData, 0x00516B10, void* geometry, int semantic);
GAME_FN(uint8_t, __thiscall, SkinInstance_ContainsVertexData, 0x00776F00, void* skin, int semantic);
GAME_FN(const re::Ptr32<const char>*, __cdecl, TexturingProperty_GetMapNameFromID, 0x00765A00, int id);
// NiMaterialDescriptor.cpp's
GAME_FN(void, __thiscall, StandardDesc_SetTextureUsage, 0x007777A0, NiMaterialDescriptor* self, uint32_t map,
        uint32_t uvSet, uint32_t transform);
GAME_FN(uint8_t, __thiscall, StandardDesc_SetLight, 0x00777B60, NiMaterialDescriptor* self, void* light,
        uint32_t index, void* geometry);
GAME_FN(void, __thiscall, StandardDesc_SetProjectedLight, 0x00777A60, NiMaterialDescriptor* self, uint32_t index,
        int clipped, int type);
GAME_FN(void, __thiscall, StandardDesc_SetProjectedShadow, 0x00777AD0, NiMaterialDescriptor* self, uint32_t index,
        int clipped, int type);
// the program descriptors' setters (another chunk's): the pixel program's UV set for a map, the
// vertex program's output UV (its source set and texgen)
GAME_FN(void, __thiscall, PixelDesc_SetUVSetForMap, 0x007E3690, NiMaterialDescriptor* self, uint32_t map,
        uint32_t uvSet);
GAME_FN(void, __thiscall, VertexDesc_SetOutputUV, 0x007E5D00, NiMaterialDescriptor* self, uint32_t output,
        uint32_t uvSet, uint32_t texGen);

// NiShaderDeclaration::ShaderRegisterEntry (0x18 bytes)
struct ShaderRegisterEntry {
    uint32_t preferredRegister;   // +0x0
    uint32_t input;               // +0x4
    uint32_t type;                // +0x8
    re::Ptr32<const char> usage;          // +0xC
    uint32_t usageIndex;          // +0x10 (-1 ends the list)
    uint32_t extraData;           // +0x14
};
static_assert(sizeof(ShaderRegisterEntry) == 0x18);

// whether the geometry's data, or its skin instance, holds that vertex data
inline bool HasVertexData(void* geometry, int semantic) {
    if (Virtual<uint8_t>(At<void*>(geometry, 0xCC), 19, semantic)) {
        return true;
    }
    void* skin = At<void*>(geometry, 0xD0);
    return skin != nullptr && SkinInstance_ContainsVertexData(skin, semantic) != 0;
}

// a texturing property's map that has a texture
inline bool HasTexture(void* map) {
    return map != nullptr && At<void*>(map, 8) != nullptr;
}

// NiFixedString's assignment, inlined (through the import slots)
inline void FixedStringSet(re::Ptr32<const char>* slot, NiFixedString value) {
    if (*slot == value) {
        return;
    }
    if (value != NullFixedString) {
        ni::sys::Increment(re::PtrCast<char*>(value) - 8);
    }
    NiFixedString old = *slot;
    if (old != NullFixedString) {
        uint32_t length = *reinterpret_cast<const uint16_t*>(old - 2);
        if (ni::sys::Decrement(re::PtrCast<char*>(old) - 8) == 1) {
            ni::scene::StringTable_RemoveString(ni::scene::GlobalStringTable, slot, length);
        }
    }
    *slot = value;
}

inline void SetEntry(ShaderRegisterEntry* entry, uint32_t input, uint32_t type, const re::Ptr32<const char>* usage) {
    entry->preferredRegister = 0;
    entry->input = input;
    entry->type = type;
    FixedString_Assign(&entry->usage, usage);
    entry->usageIndex = 0;
    entry->extraData = 0;
}

// slot 5 GetVertexInputSemantics(geometry, (unused), entries): position, the blend weights and
// indices of a skin (the indices with a bone palette), normal, color, the UV sets, binormal and
// tangent (the data's flags asking for them); the list ended by a usage index of -1
uint8_t __fastcall GetVertexInputSemantics(Self*, re::Edx, void* geometry, uint32_t, ShaderRegisterEntry* entries) {
    SetEntry(&entries[0], 0, 2, &UsageNames[0]);
    uint32_t count = 1;
    void* skin = At<void*>(geometry, 0xD0);
    if (skin != nullptr) {
        void* partition = At<void*>(skin, 0xC);
        if (partition != nullptr) {
            void* partitions = At<void*>(partition, 0xC);
            SetEntry(&entries[1], 1, 2, &UsageNames[1]);
            count = 2;
            if (At<uint32_t>(partition, 8) != 0 && partitions != nullptr && At<uint32_t>(partitions, 0x10) != 0) {
                SetEntry(&entries[2], 2, 5, &UsageNames[2]);
                count = 3;
            }
        }
    }
    if (HasVertexData(geometry, 3)) {
        SetEntry(&entries[count], 3, 2, &UsageNames[3]);
        count++;
    }
    if (HasVertexData(geometry, 4)) {
        SetEntry(&entries[count], 4, 4, &UsageNames[10]);
        count++;
    }
    for (uint32_t semantic = 5; semantic - 5 < 8; semantic++) {
        if (!HasVertexData(geometry, int(semantic))) {
            continue;
        }
        ShaderRegisterEntry* entry = &entries[count];
        entry->preferredRegister = 0;
        entry->input = semantic;
        entry->type = 1;
        FixedStringSet(&entry->usage, UsageNames[5]);
        entry->usageIndex = semantic - 5;
        entry->extraData = 0;
        count++;
    }
    if ((At<uint16_t>(At<void*>(geometry, 0xCC), 0x32) & 0xF000) != 0 && HasVertexData(geometry, 0xE) &&
        HasVertexData(geometry, 0xD)) {
        SetEntry(&entries[count], 0xE, 2, &UsageNames[7]);
        count++;
        SetEntry(&entries[count], 0xD, 2, &UsageNames[6]);
        count++;
    }
    entries[count].usageIndex = 0xFFFFFFFF;
    return 1;
}

// slot 18 GenerateDescriptor(geometry, skin, property state, effect state, descriptor): false
// without properties. As the game has it: a gloss map's test reads the specular property without
// checking it's there, the UV set of a map is clamped to the set count (not one less), and the
// projected lights' clipped flag is passed as a byte in a dword whose upper bytes are left over
// (the callee reads the byte)
uint8_t __fastcall GenerateDescriptor(Self* self, re::Edx, void* geometry, void* skin, void* state, void* effects,
                                      NiMaterialDescriptor* desc) {
    if (state == nullptr) {
        return 0;
    }
    FixedString_Assign(&desc->identifier, &self->descriptorName);
    uint32_t* bits = desc->bits;

    // the skinning: 1 with a bone palette in the first partition, 2 without
    void* partition = skin != nullptr ? At<void*>(skin, 0xC) : nullptr;
    if (partition == nullptr) {
        bits[0] &= ~3u;
    } else {
        void* partitions = At<void*>(partition, 0xC);
        if (At<uint32_t>(partition, 8) != 0 && partitions != nullptr && At<uint32_t>(partitions, 0x10) != 0) {
            bits[0] = (bits[0] & ~2u) | 1;
        } else {
            bits[0] = (bits[0] & ~1u) | 2;
        }
    }

    // the normals
    if ((At<uint16_t>(At<void*>(geometry, 0xCC), 0x32) & 0xF000) != 0 && Geometry_ContainsVertexData(geometry, 3) &&
        Geometry_ContainsVertexData(geometry, 0xE) && Geometry_ContainsVertexData(geometry, 0xD)) {
        bits[0] = (bits[0] & ~4u) | 8;
    } else if (HasVertexData(geometry, 3)) {
        bits[0] = (bits[0] & ~8u) | 4;
    } else {
        bits[0] &= ~0xCu;
    }
    bits[0] &= 0xFFFF8FFF;
    bool colors = HasVertexData(geometry, 4);
    bits[0] = (bits[0] & ~0x20u) | (uint32_t(colors ? 1 : 0) << 5);

    void* vertexColor = At<void*>(state, 0x2C);
    void* texturing = At<void*>(state, 0x28);
    void* specular = At<void*>(state, 0x20);
    void* fog = At<void*>(state, 0x10);
    void* alpha = At<void*>(state, 0x8);

    // the vertex color property: its source mode, its lighting mode
    if (vertexColor != nullptr && colors) {
        uint16_t mode = uint16_t((At<uint8_t>(vertexColor, 0x18) >> 4) & 3);
        if (mode == 2) {
            bits[0] = (bits[0] & ~0x40u) | 0x80;
        } else if (mode == 1) {
            bits[0] = (bits[0] & ~0x80u) | 0x40;
        } else if (mode == 0) {
            bits[0] &= ~0xC0u;
        }
    } else {
        bits[0] &= ~0xC0u;
    }
    if (vertexColor != nullptr) {
        bits[0] ^= ((uint32_t(At<uint16_t>(vertexColor, 0x18)) << 5) ^ bits[0]) & 0x100;
    }
    if (specular != nullptr) {
        if ((At<uint8_t>(specular, 0x18) & 1) != 0) {
            bits[0] |= 0x10;
        } else {
            bits[0] &= ~0x10u;
        }
    }
    if (fog != nullptr) {
        uint8_t flags = At<uint8_t>(fog, 0x18);
        uint32_t function = (flags >> 1) & 3;
        if ((flags & 1) != 0 && function == 0) {
            bits[0] = (bits[0] & ~0x800u) | 0x400;
        } else if ((flags & 1) != 0 && function == 1) {
            bits[0] = (bits[0] & ~0x400u) | 0x800;
        } else {
            bits[0] &= 0xFFFFF3FF;
        }
    }
    if (alpha != nullptr) {
        uint16_t flags = At<uint16_t>(alpha, 0x18);
        if (((flags >> 9) & 1) != 0 && (flags & 0x1C00) != 0) {
            bits[3] |= 0x40000000;
        } else {
            bits[3] &= 0xBFFFFFFF;
        }
    }

    // the texture maps
    if (texturing != nullptr) {
        // 0..7 the fixed maps, 8..12 the shader maps, 13..15 the decals. (More than three decals
        // write past the 16: in the game into the per-set counts, cleared after, and from the
        // 33rd on over its return address; kept to the 32 here)
        void* slots[32];
        crt::Memset(slots, 0, 16 * sizeof(void*));
        uint32_t uvSets = 0;
        for (int semantic = 5; semantic < 13; semantic++) {
            if (HasVertexData(geometry, semantic)) {
                uvSets++;
            }
        }
        if ((At<uint8_t>(texturing, 0x18) & 0xE) != 0) {
            bits[0] |= 0x200;
        } else {
            bits[0] &= ~0x200u;
        }
        if (uvSets != 0) {
            re::Ptr32<void>* maps = At<re::Ptr32<void>*>(texturing, 0x20);
            if (HasTexture(maps[7])) {   // parallax
                bits[0] |= 0x8000;
                slots[0] = maps[7];
            }
            if (HasTexture(maps[0])) {   // base
                bits[0] |= 0x10000;
                slots[1] = maps[0];
            }
            if (HasTexture(maps[6])) {   // normal (it asks for binormals and tangents)
                bits[0] |= 0x20000;
                bits[0] &= 0xFFF3FFFF;
                bits[0] = (bits[0] & ~4u) | 8;
                slots[2] = maps[6];
            }
            if (HasTexture(maps[1])) {   // dark
                bits[0] |= 0x100000;
                slots[3] = maps[1];
            }
            if (HasTexture(maps[2])) {   // detail
                bits[0] |= 0x200000;
                slots[4] = maps[2];
            }
            bool environment = effects != nullptr && At<void*>(effects, 0x18) != nullptr;
            if (HasTexture(maps[5]) && environment) {   // bump
                bits[0] |= 0x400000;
                slots[5] = maps[5];
            }
            if (HasTexture(maps[3]) && ((At<uint8_t>(specular, 0x18) & 1) != 0 || environment)) {   // gloss
                bits[0] |= 0x800000;
                slots[6] = maps[3];
            }
            if (HasTexture(maps[4])) {   // glow
                bits[0] |= 0x1000000;
                slots[7] = maps[4];
            }

            // the decals (from maps[8] on while the array has them; 3 at most)
            uint32_t decals = (At<uint16_t>(texturing, 0x18) >> 4) & 0xFF;
            if (decals != 0) {
                uint32_t arraySize = At<uint16_t>(texturing, 0x26);
                for (uint32_t i = 0; i < decals; i++) {
                    void* map = i + 8 < arraySize ? maps[8 + i] : nullptr;
                    if (!HasTexture(map)) {
                        decals--;
                    } else if ((bits[1] & 6) == 6) {
                        decals = 3;
                        break;
                    } else if (13 + i < 32) {
                        slots[13 + i] = map;
                    }
                }
            }
            bits[1] = (bits[1] & ~6u) | (decals * 2);

            // the shader maps (5 at most)
            void* shaderMaps = At<void*>(texturing, 0x30);
            uint32_t count = shaderMaps != nullptr ? At<uint16_t>(shaderMaps, 0xA) : 0;
            for (uint32_t i = 0; i < count && i < 5; i++) {
                shaderMaps = At<void*>(texturing, 0x30);
                if (shaderMaps == nullptr || i >= At<uint16_t>(shaderMaps, 0xA)) {
                    continue;
                }
                void* map = At<re::Ptr32<void>*>(shaderMaps, 4)[i];
                if (!HasTexture(map)) {
                    continue;
                }
                switch (i) {
                case 0:
                    bits[0] |= 0x10000000;
                    break;
                case 1:
                    bits[0] |= 0x20000000;
                    break;
                case 2:
                    bits[0] |= 0x40000000;
                    break;
                case 3:
                    bits[0] |= 0x80000000;
                    break;
                case 4:
                    bits[1] |= 1;
                    break;
                default:
                    break;
                }
                slots[8 + i] = map;
            }
        }

        // each map's UV set and transform, in slot order (the per-set counts the game keeps are
        // never read)
        uint32_t perSet[16];
        crt::Memset(perSet, 0, sizeof(perSet));
        uint32_t texture = 0;
        for (int i = 0; i < 16; i++) {
            void* map = slots[i];
            if (map == nullptr || uvSets == 0) {
                continue;
            }
            uint32_t uvSet = At<uint8_t>(map, 4);
            if (uvSet > uvSets) {
                uvSet = uvSets;
            }
            uint32_t transform = 0;
            if (At<void*>(map, 0xC) != nullptr) {
                transform = 1;
            } else {
                perSet[uvSet]++;
            }
            StandardDesc_SetTextureUsage(desc, texture, uvSet, transform);
            texture++;
        }
        bits[0] = (bits[0] & 0xFFFF8FFF) | (uvSets << 12);
    }

    // the dynamic effects
    if (effects != nullptr) {
        if ((bits[0] & 0x200) != 0) {
            // the lights by kind (2 and 5, then 3 and 6, then 4 and 7)
            static const uint8_t kinds[3][2] = {{2, 5}, {3, 6}, {4, 7}};
            uint32_t index = 0;
            for (int pass = 0; pass < 3; pass++) {
                for (void* node = At<void*>(effects, 0xC); node != nullptr;) {
                    void* light = At<void*>(node, 4);
                    node = At<void*>(node, 0);
                    if (light == nullptr) {
                        continue;
                    }
                    uint8_t kind = At<uint8_t>(light, 0xC5);
                    if (kind == kinds[pass][0] || kind == kinds[pass][1]) {
                        StandardDesc_SetLight(desc, light, index, geometry);
                        index++;
                    }
                }
            }
        }
        // the environment map's texgen
        bits[0] &= 0xF1FFFFFF;
        void* environment = At<void*>(effects, 0x18);
        if (environment != nullptr && At<uint8_t>(environment, 0xC4) == 1 && At<uint32_t>(environment, 0x164) == 2) {
            switch (At<uint32_t>(environment, 0x168)) {
            case 0:
                bits[0] = (bits[0] & 0xF3FFFFFF) | 0x2000000;
                break;
            case 1:
                bits[0] = (bits[0] & 0xF5FFFFFF) | 0x4000000;
                break;
            case 2:
                bits[0] = (bits[0] & 0xF7FFFFFF) | 0x6000000;
                break;
            case 3:
                bits[0] = (bits[0] & 0xFBFFFFFF) | 0xA000000;
                break;
            case 4:
                bits[0] = (bits[0] & 0xF9FFFFFF) | 0x8000000;
                break;
            default:
                break;
            }
        }
        // the projected lights and shadows (3 of each at most; texgen 0 a type 1, 1 a type 2)
        for (int shadows = 0; shadows < 2; shadows++) {
            uint32_t index = 0;
            for (void* node = At<void*>(effects, shadows == 0 ? 0x10 : 0x14); node != nullptr && index < 3;) {
                void* effect = At<void*>(node, 4);
                node = At<void*>(node, 0);
                if (effect == nullptr || At<uint8_t>(effect, 0xC4) != 1 ||
                    At<uint32_t>(effect, 0x164) != uint32_t(shadows)) {
                    continue;
                }
                int clipped = At<uint8_t>(effect, 0x16C);
                uint32_t texGen = At<uint32_t>(effect, 0x168);
                if (texGen != 0 && texGen != 1) {
                    continue;
                }
                int type = texGen == 0 ? 1 : 2;
                if (shadows == 0) {
                    StandardDesc_SetProjectedLight(desc, index, clipped, type);
                } else {
                    StandardDesc_SetProjectedShadow(desc, index, clipped, type);
                }
                index++;
            }
            if (shadows == 0) {
                bits[1] = (bits[1] & 0xFCFFFFFF) | (index << 24);
            } else {
                bits[2] = (bits[2] & ~3u) | index;
            }
        }
    }

    // per-vertex lighting: none with shadows or the material's switch (+0x3BC); else with lights
    // only when there's no parallax or normal map
    bool lights = (bits[1] & 0xF0) != 0 || (bits[1] & 0xFF00) != 0;
    if (uint8_t(bits[1] >> 16) == 0 && self->unknown3BC == 0 && (!lights || (bits[0] & 0x28000) == 0)) {
        bits[1] |= 8;
    } else {
        bits[1] &= ~8u;
    }
    // without normals: no lights, and no texgen that needs them
    if ((bits[0] & 0xC) == 0) {
        bits[1] &= 0xFFFF000F;
        uint32_t texGen = (bits[0] >> 25) & 7;
        if (texGen == 3 || texGen == 5 || texGen == 4) {
            bits[0] &= 0xF1FFFFFF;
        }
    }
    return 1;
}

// slot 27 CreateShader(descriptor): the renderer's (its slot 75)
void* __fastcall CreateShader(Self*, re::Edx, NiMaterialDescriptor* desc) {
    void* renderer = NiRenderer_ms_pkRenderer;
    if (renderer == nullptr) {
        return nullptr;
    }
    return Virtual<void*>(renderer, 75, desc);
}

// slot 28 SetupPackingRequirements(shader, descriptor, passes, count): the shader's vertex
// declaration (its slot 18: entries, streams) filled as the descriptor asks (the declaration's
// slot 18: SetEntry(entry, input, type, stream)), held through the import's interlocked calls
uint8_t __fastcall SetupPackingRequirements(Self*, re::Edx, void* shader, NiMaterialDescriptor* desc, void*, uint32_t) {
    NiRefObject* declaration = Virtual<NiRefObject*>(shader, 18, 0xFu, 1u);
    if (declaration == nullptr) {
        return 0;
    }
    ni::sys::Increment(&declaration->refCount);
    auto set = [&](uint32_t entry, uint32_t input, uint32_t type) {
        Virtual<void>(declaration, 18, entry, input, type, 0u);
    };
    set(0, 0, 2);
    uint32_t entry = 1;
    uint32_t skinning = desc->bits[0] & 3;
    if (skinning == 1) {
        set(1, 1, 3);
        set(2, 2, 5);
        entry = 3;
    } else if (skinning == 2) {
        set(1, 1, 3);
        entry = 2;
    }
    if ((uint8_t(desc->bits[0]) & 0xC) != 0) {
        set(entry, 3, 2);
        entry++;
    }
    if ((uint8_t(desc->bits[0]) & 0x20) != 0) {
        set(entry, 4, 4);
        entry++;
    }
    uint32_t uvSets = (desc->bits[0] >> 12) & 7;
    for (uint32_t i = 0; i < 8 && i < uvSets; i++) {
        set(entry, i + 5, 1);
        entry++;
    }
    if ((uint8_t(desc->bits[0]) & 0xC) == 8) {
        set(entry, 0xE, 2);
        entry++;
        set(entry, 0xD, 2);
    }
    if (ni::sys::Decrement(&declaration->refCount) == 0) {
        Virtual<void>(declaration, 1);
    }
    return 1;
}

// the generation context's destructor: its nodes and configurator let go, last first
void __fastcall ContextDestruct(MaterialContext* self, re::Edx) {
    re::Ptr32<NiRefObject>* pointers = &self->configurator;
    for (int i = 4; i >= 0; i--) {
        NiRefObject* object = pointers[i];
        if (object != nullptr && ni::sys::Decrement(&object->refCount) == 0) {
            Virtual<void>(object, 1);
        }
    }
}

// ---- the static tables

// GetTextureTransformMappingFromEnum(map, mapping&, occurrence&): the map's texture transform's
// predefined mapping
uint8_t __cdecl GetTextureTransformMappingFromEnum(uint32_t map, uint32_t* mapping, uint32_t* occurrence) {
    static const uint32_t kMappings[16][2] = {
        {0x5C, 0}, {0x25, 0}, {0x58, 0}, {0x29, 0}, {0x2D, 0}, {0x39, 0}, {0x31, 0}, {0x35, 0},
        {0x41, 0}, {0x41, 1}, {0x41, 2}, {0x41, 3}, {0x41, 4}, {0x3D, 0}, {0x3D, 1}, {0x3D, 2},
    };
    if (map > 15) {
        return 0;
    }
    *mapping = kMappings[map][0];
    *occurrence = kMappings[map][1];
    return 1;
}

// MapIndexFromTextureEnum(map, pixel descriptor): the map's sampler index, the maps before it
// (and it) that the descriptor has, counted from -1
int32_t __cdecl MapIndexFromTextureEnum(uint32_t map, const NiMaterialDescriptor* desc) {
    if (map > 15) {
        return -1;
    }
    uint32_t d = desc->bits[0];
    int32_t index = -1;
    switch (map) {
    case 15:
        if ((d & 0x700000) > 0x200000) {
            index = 0;
        }
        [[fallthrough]];
    case 14:
        if ((d & 0x700000) > 0x100000) {
            index++;
        }
        [[fallthrough]];
    case 13:
        if ((d & 0x700000) != 0) {
            index++;
        }
        [[fallthrough]];
    case 12:
        index += (d >> 19) & 1;
        [[fallthrough]];
    case 11:
        index += (d >> 18) & 1;
        [[fallthrough]];
    case 10:
        index += (d >> 17) & 1;
        [[fallthrough]];
    case 9:
        index += (d >> 16) & 1;
        [[fallthrough]];
    case 8:
        index += (d >> 15) & 1;
        [[fallthrough]];
    case 7:
        index += (d >> 14) & 1;
        [[fallthrough]];
    case 6:
        index += (d >> 13) & 1;
        [[fallthrough]];
    case 5:
        index += (d >> 12) & 1;
        [[fallthrough]];
    case 4:
        index += (d >> 11) & 1;
        [[fallthrough]];
    case 3:
        index += (d >> 10) & 1;
        [[fallthrough]];
    case 2:
        index += (d >> 9) & 1;
        [[fallthrough]];
    case 1:
        index += (d >> 8) & 1;
        [[fallthrough]];
    case 0:
        index += (d >> 7) & 1;
        break;
    default:
        break;
    }
    return index;
}

// GetTextureNameFromTextureEnum(map, name&, occurrence&): the texturing property's map names for
// the fixed, shader and decal maps (0..15), NiStandardMaterial's for the shadow maps, projected
// lights and projected shadows (23..46, 8 of each); false for the rest
uint8_t __cdecl GetTextureNameFromTextureEnum(uint32_t map, re::Ptr32<const char>* name, uint32_t* occurrence) {
    static const int8_t kIds[16] = {7, 0, 6, 1, 2, 5, 3, 4, 9, 9, 9, 9, 9, 8, 8, 8};
    static const uint8_t kOccurrences[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 0, 1, 2};
    if (map <= 15) {
        FixedString_Assign(name, TexturingProperty_GetMapNameFromID(kIds[map]));
        *occurrence = kOccurrences[map];
        return 1;
    }
    if (map >= 23 && map <= 46) {
        FixedString_Assign(name, &StandardMapNames[(map - 23) / 8]);
        *occurrence = (map - 23) % 8;
        return 1;
    }
    return 0;
}

// AssignTextureCoordinates(uvSets[], texGens[], count, vertex descriptor, pixel descriptor): the
// maps' UV sets packed (the sets used, renumbered in order), each generated one given an output
// of its own after them; the vertex program's output count in bits 24..27. (A UV set past 15
// writes past the game's table on its stack; it's left out here.)
uint8_t __cdecl AssignTextureCoordinates(const int32_t* uvSets, const int32_t* texGens, uint32_t count,
                                         NiMaterialDescriptor* vertexDesc, NiMaterialDescriptor* pixelDesc) {
    int32_t remapped[16];
    int32_t used[16];
    crt::Memset(remapped, -1, sizeof(remapped));
    crt::Memset(used, 0, sizeof(used));
    int32_t highest = -1;
    for (uint32_t i = 0; i < count; i++) {
        if (texGens[i] != 0) {
            continue;
        }
        int32_t uvSet = uvSets[i];
        remapped[i] = uvSet;
        if (uvSet > highest) {
            highest = uvSet;
        }
        if (uint32_t(uvSet) < 16) {
            used[uvSet] = 1;
        }
    }
    int32_t next = -1;
    if (highest != -1) {
        for (uint32_t set = 0; set <= uint32_t(highest); set++) {
            if (used[set] == 0) {
                continue;
            }
            next++;
            for (uint32_t i = 0; i < count; i++) {
                if (remapped[i] == int32_t(set)) {
                    remapped[i] = next;
                }
            }
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        int32_t texGen = texGens[i];
        int32_t uvSet = uvSets[i];
        if (texGen == 0) {
            PixelDesc_SetUVSetForMap(pixelDesc, i, uint32_t(remapped[i]));
            VertexDesc_SetOutputUV(vertexDesc, uint32_t(remapped[i]), uint32_t(uvSet), 0);
        } else {
            next++;
            PixelDesc_SetUVSetForMap(pixelDesc, i, uint32_t(next));
            VertexDesc_SetOutputUV(vertexDesc, uint32_t(next), uint32_t(uvSet), uint32_t(texGen));
        }
    }
    if (next != -1) {
        vertexDesc->bits[0] = (uint32_t(next + 1) << 24) | (vertexDesc->bits[0] & 0xF0FFFFFF);
    } else {
        vertexDesc->bits[0] &= 0xF0FFFFFF;
    }
    return 1;
}

} // namespace
} // namespace ni

// ---- registration ---------------------------------------------------------------------

#define NISTDMAT_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(Original_, __LINE__);                                               \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), \
             re::Mode::Original)

NISTDMAT_REVERSED("NiStandardMaterial::GetVertexInputSemantics", 0x0078FDE0, &ni::GetVertexInputSemantics);
NISTDMAT_REVERSED("NiStandardMaterial::GenerateDescriptor", 0x0078F450, &ni::GenerateDescriptor);
NISTDMAT_REVERSED("NiStandardMaterial::CreateShader", 0x0078A010, &ni::CreateShader);
NISTDMAT_REVERSED("NiStandardMaterial::SetupPackingRequirements", 0x0078FC90, &ni::SetupPackingRequirements);
NISTDMAT_REVERSED("NiFragmentMaterial::Context::~Context", 0x0078A030, &ni::ContextDestruct);
NISTDMAT_REVERSED("NiStandardMaterial::GetTextureTransformMappingFromEnum", 0x00789B30,
                  &ni::GetTextureTransformMappingFromEnum);
NISTDMAT_REVERSED("NiStandardMaterial::MapIndexFromTextureEnum", 0x0078EDC0, &ni::MapIndexFromTextureEnum);
NISTDMAT_REVERSED("NiStandardMaterial::GetTextureNameFromTextureEnum", 0x0078EEE0, &ni::GetTextureNameFromTextureEnum);
NISTDMAT_REVERSED("NiStandardMaterial::AssignTextureCoordinates", 0x007900F0, &ni::AssignTextureCoordinates);
