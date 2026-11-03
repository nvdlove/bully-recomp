// 0x516CB0 to 0x517FF0: MdBullyMaterial's second part: the material descriptor built
// from a geometry's properties and effects (Gamebryo's NiStandardMaterial
// GenerateDescriptor, cut down), the material's creation by name, its shader map, the
// shader loaded by feature mask (the "_ERGLS.." name after the base name; the flags
// dropped one by one until a shader exists; the .fx compile path for the dev flags) and
// its NiMaterial virtuals (IsShaderCurrent, GetCurrentShader and vf5).

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(void*, ms_pStringTable, 0x020E216C);
GAME_VAR(uint8_t*, ms_pMaterialMap, 0x020E2704);           // NiMaterial's, by name
GAME_VAR(uint32_t, ms_materialClassIndex, 0x00C67634);     // MdBullyMaterial's bit in the RTTI's class set
GAME_VAR(uint8_t, ms_bSwapProcessing, 0x00C67668);         // set by MdBullyMaterialSwapProcessor
GAME_VAR(uint8_t, ms_bCompileShaders_BCBB41, 0x00BCBB41);
GAME_VAR(uint8_t, ms_bCompileShaders_BCBB42, 0x00BCBB42);
GAME_VAR(const char*, ms_shaderSourceDir, 0x00A764D4);
GAME_VAR(const char*, ms_shaderDir, 0x00A764E0);
GAME_VAR(const char*, ms_shaderDirWorld, 0x00A764E4);       // (for the wd_ / sd_ / pd_ masks)
inline const re::Ptr32<char>* const ms_defaultDescriptorName = reinterpret_cast<const re::Ptr32<char>*>(0x00BCC4B8);
inline void* const ms_shaderDefines = reinterpret_cast<void*>(0x00A76530);   // D3DXMACRO[] ("GAMEBRYO")

GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, StringTable_Release, 0x00754550, void* table, re::Ptr32<char>* handle, uint16_t length);
GAME_FN(char*, __cdecl, FixedString_Create, 0x00754760, const char* s);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, size_t size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __thiscall, NiTMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, NiMaterial_Construct, 0x00762CD0, void* self, const re::Ptr32<char>* name);
GAME_FN(void*, __cdecl, NiMaterial_GetFirstIterator, 0x00762A10);
GAME_FN(uint8_t*, __cdecl, NiMaterial_GetNext, 0x00762B30, re::Ptr32<void>* iterator);
GAME_FN(void, __thiscall, MdBullyMaterial_Destruct, 0x00516940, void* self);
GAME_FN(uint8_t*, __cdecl, NiMaterial_GetMaterial, 0x00516AC0, const re::Ptr32<char>* name);
GAME_FN(void, __cdecl, ShaderNameFromFlags, 0x00516360, uint32_t flags, char* out);
GAME_FN(void, __cdecl, FlagsFromShaderName, 0x005165B0, const char* name, uint32_t* flags);
GAME_FN(uint8_t*, __stdcall, GetDefaultShader, 0x005167B0, int skinned);
GAME_FN(uint8_t*, __stdcall, GetShaderOrDefault, 0x00516810, const char* name);
GAME_FN(int, __cdecl, ClassifyShaderName, 0x00516890, const char* name);
GAME_FN(uint8_t*, __cdecl, GetShaderFrom, 0x005168F0, const char* name, void* library);
GAME_FN(uint8_t, __thiscall, ShaderMap_GetAt, 0x00516A10, void* map, uint32_t key, re::Ptr32<uint8_t>* out);
// NiTMap<unsigned int, NiPointer<NiShader>>::SetAt: the NiPointer by value (released by it)
GAME_FN(void, __thiscall, ShaderMap_SetAt, 0x0087ADD0, void* map, uint32_t key, uint8_t* shader);
GAME_FN(uint8_t, __thiscall, ContainsVertexData, 0x00516B10, void* geometry, int semantic);
GAME_FN(uint8_t, __thiscall, NiSkinInstance_ContainsVertexData, 0x00776F00, void* skin, int semantic);
GAME_FN(uint32_t, __cdecl, FlagsFromDesc, 0x00516B60, void* desc, int features);
GAME_FN(uint8_t, __cdecl, GenerateDescriptor, 0x00516CB0, void* geometry, void* skin, void* properties,
        void* effects, void* desc);
GAME_FN(uint8_t*, __cdecl, LoadShader, 0x00517840, uint32_t flags, const char* baseName, int classification);
GAME_FN(uint8_t*, __thiscall, MdBullyMaterial_GetShader, 0x00517DF0, void* self, uint32_t flags);
GAME_FN(void, __thiscall, MdBullyMaterial_CreateShaders, 0x00517790, void* self);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
// NiMaterialDescriptor
GAME_FN(void, __thiscall, Descriptor_Construct, 0x00777B50, void* self);
GAME_FN(void, __thiscall, Descriptor_SetTextureUsage, 0x007777A0, void* self, uint32_t index, uint32_t uvSet,
        int transform);
GAME_FN(void, __thiscall, Descriptor_SetLight, 0x00777B60, void* self, void* light, uint32_t index, void* geometry);
GAME_FN(void, __thiscall, Descriptor_SetProjectedLight, 0x00777A60, void* self, uint32_t index, int clipped,
        int type);
GAME_FN(void, __thiscall, Descriptor_SetProjectedShadow, 0x00777AD0, void* self, uint32_t index, int clipped,
        int type);
GAME_FN(void, __thiscall, NiPointer_AssignRaw, 0x00800420, re::Ptr32<uint8_t>* self, uint8_t* object);
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
// NiString (the pointer to its characters, the length at -4)
GAME_FN(void, __thiscall, NiString_Construct, 0x00774130, re::Ptr32<char>* self, int capacity);
GAME_FN(void, __thiscall, NiString_Destruct, 0x00774240, re::Ptr32<char>* self);
GAME_FN(uint32_t, __thiscall, NiString_Length, 0x00773D10, re::Ptr32<char>* self);
GAME_FN(const char*, __thiscall, NiString_Chars, 0x00773DB0, re::Ptr32<char>* self);
GAME_FN(int, __cdecl, Snprintf, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __cdecl, StandardisePath, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, DirectoryExists, 0x00753440, const char* path);
GAME_FN(void, __cdecl, CreateDirectoryTree, 0x00753430, const char* path);
GAME_FN(void, __cdecl, CompileEffectFile, 0x00407580, const char* source, const char* target, void* include,
        void* defines);
GAME_FN(uint8_t*, __cdecl, NiD3DXEffectLoader_Get, 0x00748CB0, int create);
GAME_FN(uint8_t*, __thiscall, NiD3DXEffectFile_Construct, 0x00747C50, void* self);
GAME_FN(void, __thiscall, NiD3DXEffectFile_Destruct, 0x00747CD0, void* self);
GAME_FN(uint8_t, __thiscall, NiD3DXEffectFile_LoadFromMemory, 0x007485E0, void* loader, const char* name,
        uint32_t size, const void* data, re::Ptr32<uint8_t>* file);
GAME_FN(void, __thiscall, NiD3DXEffectShaderLib_AddEffect, 0x00748310, void* lib, void* file);
// D3DXCreateEffectCompiler (the import's thunk)
GAME_FN(long, __stdcall, D3DXCreateEffectCompiler, 0x0085AC60, const char* data, uint32_t size, void* defines,
        void* include, uint32_t flags, void** compiler, void** errors);

constexpr uint32_t kMdBullyMaterialVtable = 0x009177C0;
constexpr uint32_t kShaderMapBaseVtable = 0x009177A8;
constexpr uint32_t kShaderMapVtable = 0x009177E4;

// NiMaterialDescriptor as MdBullyMaterial fills it
struct Descriptor {
    re::Ptr32<char> name;       // +0 NiFixedString
    uint32_t bits4;   // +4
    uint32_t bits8;   // +8 (byte +0xA tested on its own)
    uint32_t bitsC;   // +0xC
    uint32_t bits10;  // +0x10
};
static_assert(sizeof(Descriptor) == 0x14);

// NiTMap<unsigned int, NiPointer<NiShader>>
struct ShaderMap {
    uint32_t vtable;    // +0
    uint32_t hashSize;  // +4
    re::Ptr32<re::Ptr32<void>> buckets;     // +8
    uint32_t count;     // +0xC
};
static_assert(sizeof(ShaderMap) == 0x10);

struct MdBullyMaterial {
    uint32_t vtable;          // +0
    int32_t refCount;         // +4
    re::Ptr32<char> name;               // +8 NiFixedString (NiMaterial's)
    int32_t classification;   // +0xC 3 until worked out from the name (0, 1 "BullyDef", 2 "Md..")
    re::Ptr32<ShaderMap> shaders;       // +0x10 by feature mask
    re::Ptr32<uint8_t> defaultShader;   // +0x14 NiPointer (for classification 0)
};
static_assert(sizeof(MdBullyMaterial) == 0x18);

// a COM method (stdcall, `this` on the stack)
template <typename R, typename... A>
R ComCall(void* object, int offset, A... args) {
    return re::ComCall<R>(object, offset / 4, args...);
}

using re::At;

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

void ReleaseString(re::Ptr32<char>* handle) {
    char* s = *handle;
    if (s == ms_nullString) {
        return;
    }
    uint16_t length = *reinterpret_cast<uint16_t*>(s - 2);
    if (vc::Decrement(s - 8) == 1) {
        StringTable_Release(ms_pStringTable, handle, length);
    }
}

// a map entry (NiTexturingProperty::Map) that has a texture (+8)
uint8_t* MapWithTexture(uint8_t* map) {
    return (map && At<void*>(map, 8)) ? map : nullptr;
}

bool HasVertexData(uint8_t* geometry, int semantic) {
    if (vc::Call<uint8_t>(At<void*>(geometry, 0xCC), 0x4C, semantic)) {
        return true;
    }
    void* skin = At<void*>(geometry, 0xD0);
    return skin && NiSkinInstance_ContainsVertexData(skin, semantic);
}

void ClassifyIfUnknown(MdBullyMaterial* self) {
    if (self->classification == 3) {
        self->classification = ClassifyShaderName(self->name);
    }
}

} // namespace

namespace MdBullyMaterial2 {

// 0x516CB0 (cdecl): the descriptor for a geometry: skinned, normals (tangent space
// when +0x32 of the model data says it and binormals and tangents are there), vertex
// colours, the vertex colour, specular, fog and alpha test properties, the textures
// (the UV sets at +4's bits 12..14, a usage per map), the lights by type, the
// environment effect and the projected lights and shadows. False without properties
static uint8_t __cdecl GenerateDescriptorImpl(uint8_t* geometry, void* skin, uint8_t* properties, uint8_t* effects,
                                              Descriptor* desc) {
    if (!properties) {
        return 0;
    }
    FixedString_Assign(&desc->name, ms_defaultDescriptorName);
    if (skin) {
        desc->bits4 = (desc->bits4 & ~2u) | 1;
    } else {
        desc->bits4 &= ~3u;
    }
    if ((At<uint16_t>(At<uint8_t*>(geometry, 0xCC), 0x32) & 0xF000) && ContainsVertexData(geometry, 3) &&
        ContainsVertexData(geometry, 0xE) && ContainsVertexData(geometry, 0xD)) {
        desc->bits4 = (desc->bits4 & ~4u) | 8;
    } else if (HasVertexData(geometry, 3)) {
        desc->bits4 = (desc->bits4 & ~8u) | 4;
    } else {
        desc->bits4 &= ~0xCu;
    }
    desc->bits4 &= 0xFFFF8FFF;
    bool colours = HasVertexData(geometry, 4);
    desc->bits4 = (desc->bits4 & ~0x20u) | (colours ? 0x20u : 0u);

    uint8_t* vertexColor = At<uint8_t*>(properties, 0x2C);
    uint8_t* texturing = At<uint8_t*>(properties, 0x28);
    uint8_t* specular = At<uint8_t*>(properties, 0x20);
    uint8_t* fog = At<uint8_t*>(properties, 0x10);
    uint8_t* alpha = At<uint8_t*>(properties, 0x08);

    // the vertex colour source mode (bits 4..5: 2 ambient/diffuse, 1 emissive)
    uint16_t sourceMode = 0xFFFF;
    if (vertexColor && colours) {
        sourceMode = uint16_t((vertexColor[0x18] >> 4) & 3);
    }
    if (sourceMode == 2) {
        desc->bits4 = (desc->bits4 & ~0x40u) | 0x80;
    } else if (sourceMode == 1) {
        desc->bits4 = (desc->bits4 & ~0x80u) | 0x40;
    } else if (sourceMode == 0 || sourceMode == 0xFFFF) {
        desc->bits4 &= ~0xC0u;
    }
    if (vertexColor) {
        desc->bits4 ^= ((uint32_t(At<uint16_t>(vertexColor, 0x18)) << 5) ^ desc->bits4) & 0x100;
    }
    if (specular) {
        if (specular[0x18] & 1) {
            desc->bits4 |= 0x10;
        } else {
            desc->bits4 &= ~0x10u;
        }
    }
    if (fog) {
        uint32_t function = (fog[0x18] >> 1) & 3;
        if (!(fog[0x18] & 1)) {
            desc->bits4 &= 0xFFFFF3FF;
        } else if (function == 0) {
            desc->bits4 = (desc->bits4 & ~0x800u) | 0x400;
        } else if (function == 1) {
            desc->bits4 = (desc->bits4 & ~0x400u) | 0x800;
        } else {
            desc->bits4 &= 0xFFFFF3FF;
        }
    }
    if (alpha) {
        uint16_t flags = At<uint16_t>(alpha, 0x18);
        if (((flags >> 9) & 1) && (flags & 0x1C00)) {
            desc->bits10 |= 0x40000000;
        } else {
            desc->bits10 &= 0xBFFFFFFF;
        }
    }

    if (texturing) {
        uint8_t* slots[16];   // 0..7 the fixed maps, 8..12 the shader maps, 13.. the decals
        std::memset(slots, 0, sizeof(slots));
        uint32_t uvSets = 0;
        for (int semantic = 5; semantic < 13; semantic++) {
            if (HasVertexData(geometry, semantic)) {
                uvSets++;
            }
        }
        if (texturing[0x18] & 0xE) {
            desc->bits4 |= 0x200;
        } else {
            desc->bits4 &= ~0x200u;
        }
        if (uvSets) {
            re::Ptr32<uint8_t>* maps = At<re::Ptr32<uint8_t>*>(texturing, 0x20);
            if (uint8_t* map = MapWithTexture(maps[7])) {
                desc->bits4 |= 0x8000;
                slots[0] = map;
            }
            if (uint8_t* map = MapWithTexture(maps[0])) {
                desc->bits4 |= 0x10000;
                slots[1] = map;
            }
            if (uint8_t* map = MapWithTexture(maps[6])) {
                desc->bits4 |= 0x20000;
                desc->bits4 &= 0xFFF3FFFF;
                desc->bits4 = (desc->bits4 & ~4u) | 8;
                slots[2] = map;
            }
            if (uint8_t* map = MapWithTexture(maps[1])) {
                desc->bits4 |= 0x100000;
                slots[3] = map;
            }
            if (uint8_t* map = MapWithTexture(maps[2])) {
                desc->bits4 |= 0x200000;
                slots[4] = map;
            }
            bool environment = effects && At<void*>(effects, 0x18);
            if (uint8_t* map = MapWithTexture(maps[5]); map && environment) {
                desc->bits4 |= 0x400000;
                slots[5] = map;
            }
            // (the specular property read without checking it's there)
            if (uint8_t* map = MapWithTexture(maps[3]); map && ((specular[0x18] & 1) || environment)) {
                desc->bits4 |= 0x800000;
                slots[6] = map;
            }
            if (uint8_t* map = MapWithTexture(maps[4])) {
                desc->bits4 |= 0x1000000;
                slots[7] = map;
            }

            // the decals (from maps[8] on, while the array has them; 3 at most)
            uint32_t decals = (At<uint16_t>(texturing, 0x18) >> 4) & 0xFF;
            uint32_t arraySize = At<uint16_t>(texturing, 0x26);
            for (uint32_t i = 0; i < decals; i++) {
                uint8_t* map = (i + 8 < arraySize) ? MapWithTexture(maps[8 + i]) : nullptr;
                if (!map) {
                    decals--;
                } else if ((desc->bits8 & 6) == 6) {
                    decals = 3;
                    break;
                } else {
                    slots[13 + i] = map;
                }
            }
            desc->bits8 = (desc->bits8 & ~6u) | (decals * 2);

            // the shader maps (5 at most)
            uint8_t* shaderMaps = At<uint8_t*>(texturing, 0x30);
            uint32_t count = shaderMaps ? At<uint16_t>(shaderMaps, 0xA) : 0;
            for (uint32_t i = 0; i < count && i < 5; i++) {
                shaderMaps = At<uint8_t*>(texturing, 0x30);
                if (!shaderMaps || i >= At<uint16_t>(shaderMaps, 0xA)) {
                    continue;
                }
                uint8_t* map = MapWithTexture(At<re::Ptr32<uint8_t>*>(shaderMaps, 4)[i]);
                if (!map) {
                    continue;
                }
                switch (i) {
                    case 0: desc->bits4 |= 0x10000000; break;
                    case 1: desc->bits4 |= 0x20000000; break;
                    case 2: desc->bits4 |= 0x40000000; break;
                    case 3: desc->bits4 |= 0x80000000; break;
                    case 4: desc->bits8 |= 1; break;
                }
                slots[8 + i] = map;
            }
        }
        // the usage of each map present: its UV set (clamped to the count) and whether it
        // has a transform (+0xC). (Counted per set and never read)
        uint32_t perSet[16];
        std::memset(perSet, 0, sizeof(perSet));
        uint32_t index = 0;
        for (uint8_t* map : slots) {
            if (map && uvSets) {
                uint32_t set = map[4];
                if (set > uvSets) {
                    set = uvSets;
                }
                int transform = At<void*>(map, 0xC) ? 1 : 0;
                if (!transform) {
                    perSet[set]++;
                }
                Descriptor_SetTextureUsage(desc, index, set, transform);
                index++;
            }
        }
        desc->bits4 = (desc->bits4 & 0xFFFF8FFF) | (uvSets << 12);
    }

    if (effects) {
        if (desc->bits4 & 0x200) {
            // the lights, by type: 2/5, then 3/6, then 4/7
            static const uint8_t kTypes[3][2] = {{2, 5}, {3, 6}, {4, 7}};
            uint32_t index = 0;
            for (const auto& types : kTypes) {
                for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, 0xC); node;) {
                    uint8_t* light = static_cast<uint8_t*>(node[1]);
                    node = static_cast<re::Ptr32<void>*>(node[0]);
                    if (light && (light[0xC5] == types[0] || light[0xC5] == types[1])) {
                        Descriptor_SetLight(desc, light, index, geometry);
                        index++;
                    }
                }
            }
        }
        desc->bits4 &= 0xF1FFFFFF;
        uint8_t* environment = At<uint8_t*>(effects, 0x18);
        if (environment && environment[0xC4] == 1 && At<int32_t>(environment, 0x164) == 2) {
            switch (At<int32_t>(environment, 0x168)) {
                case 0: desc->bits4 = (desc->bits4 & 0xF3FFFFFF) | 0x2000000; break;
                case 1: desc->bits4 = (desc->bits4 & 0xF5FFFFFF) | 0x4000000; break;
                case 2: desc->bits4 = (desc->bits4 & 0xF7FFFFFF) | 0x6000000; break;
                case 3: desc->bits4 = (desc->bits4 & 0xFBFFFFFF) | 0xA000000; break;
                case 4: desc->bits4 = (desc->bits4 & 0xF9FFFFFF) | 0x8000000; break;
                default: break;
            }
        }
        // the projected lights (+0x10) and shadows (+0x14), 3 of each at most
        uint32_t lights = 0;
        for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, 0x10); node && lights < 3;) {
            uint8_t* effect = static_cast<uint8_t*>(node[1]);
            node = static_cast<re::Ptr32<void>*>(node[0]);
            if (!effect || effect[0xC4] != 1 || At<int32_t>(effect, 0x164) != 0) {
                continue;
            }
            int32_t type = At<int32_t>(effect, 0x168);
            if (type != 0 && type != 1) {
                continue;
            }
            Descriptor_SetProjectedLight(desc, lights, effect[0x16C], type == 0 ? 1 : 2);
            lights++;
        }
        desc->bits8 = (desc->bits8 & 0xFCFFFFFF) | (lights << 24);
        uint32_t shadows = 0;
        for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, 0x14); node && shadows < 3;) {
            uint8_t* effect = static_cast<uint8_t*>(node[1]);
            node = static_cast<re::Ptr32<void>*>(node[0]);
            if (!effect || effect[0xC4] != 1 || At<int32_t>(effect, 0x164) != 1) {
                continue;
            }
            int32_t type = At<int32_t>(effect, 0x168);
            if (type != 0 && type != 1) {
                continue;
            }
            Descriptor_SetProjectedShadow(desc, shadows, effect[0x16C], type == 0 ? 1 : 2);
            shadows++;
        }
        desc->bitsC = (desc->bitsC & ~3u) | shadows;
    }

    bool mapped = (desc->bits8 & 0xF0) || (desc->bits8 & 0xFF00);
    if (reinterpret_cast<uint8_t*>(desc)[0xA] == 0 && (!mapped || !(desc->bits4 & 0x28000))) {
        desc->bits8 |= 8;
    } else {
        desc->bits8 &= ~8u;
    }
    if (!(desc->bits4 & 0xC)) {
        desc->bits8 &= 0xFFFF000F;
        uint32_t environmentType = (desc->bits4 >> 25) & 7;
        if (environmentType == 3 || environmentType == 5 || environmentType == 4) {
            desc->bits4 &= 0xF1FFFFFF;
        }
    }
    return 1;
}

// 0x5174A0 (cdecl): the material by name, or a new MdBullyMaterial (classification 3:
// not worked out yet)
static MdBullyMaterial* __cdecl Create(const char* name) {
    re::Ptr32<char> key = name ? FixedString_Create(name) : static_cast<char*>(ms_nullString);
    uint8_t* found = NiMaterial_GetMaterial(&key);
    ReleaseString(&key);
    if (found) {
        return reinterpret_cast<MdBullyMaterial*>(found);
    }
    auto* material = static_cast<MdBullyMaterial*>(NiNew(sizeof(MdBullyMaterial)));
    if (!material) {
        return nullptr;
    }
    re::Ptr32<char> materialName = name ? FixedString_Create(name) : static_cast<char*>(ms_nullString);
    NiMaterial_Construct(material, &materialName);
    material->vtable = kMdBullyMaterialVtable;
    material->classification = 3;
    material->shaders = nullptr;
    material->defaultShader = nullptr;
    ReleaseString(&materialName);
    return material;
}

// 0x5175B0 (thiscall) vf0: the deleting destructor
static void* __fastcall Destroy(MdBullyMaterial* self, re::Edx, int flags) {
    MdBullyMaterial_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, sizeof(MdBullyMaterial));
    }
    return self;
}

// 0x5175D0 (thiscall) vf3: IsShaderCurrent: always for classification 0 once there are
// shaders, else whether GetCurrentShader (vf4) gives `shader`
static uint8_t __fastcall IsShaderCurrent(MdBullyMaterial* self, re::Edx, void* shader, void* geometry, void* skin,
                                          void* properties, void* effects, uint32_t extra) {
    if (!shader) {
        return 0;
    }
    ClassifyIfUnknown(self);
    if (!self->shaders && !self->defaultShader) {
        return 0;
    }
    if (self->classification == 0) {
        return 1;
    }
    return vc::Call<void*>(self, 0x10, geometry, skin, properties, effects, extra) == shader;
}

// 0x517650 (cdecl): every MdBullyMaterial's shaders dropped (the map emptied and
// deleted, the default shader released)
static void __cdecl UnloadAllShaders() {
    if (!ms_pMaterialMap) {
        return;
    }
    re::Ptr32<void> iterator = NiMaterial_GetFirstIterator();
    for (uint8_t* material = NiMaterial_GetNext(&iterator); material; material = NiMaterial_GetNext(&iterator)) {
        const uint8_t* rtti = vc::Call<const uint8_t*>(material, 0x08);
        uint32_t index = ms_materialClassIndex;
        uint32_t bits = *reinterpret_cast<const uint32_t*>(rtti + 0xC + (index >> 5) * 4);
        if (!(bits & (1u << (index & 0x1F)))) {
            continue;
        }
        auto* self = reinterpret_cast<MdBullyMaterial*>(material);
        if (self->shaders) {
            NiTMap_RemoveAll(self->shaders);
            if (self->shaders) {
                vc::Call<void*>(self->shaders, 0x00, 1);
            }
        }
        self->shaders = nullptr;
        if (self->defaultShader) {
            Release(self->defaultShader);
            self->defaultShader = nullptr;
        }
    }
}

// 0x517710 (thiscall): NiTPointerMap<unsigned int, NiPointer<NiShader>>'s deleting
// destructor
static void* __fastcall ShaderMap_Destroy(ShaderMap* self, re::Edx, int flags) {
    self->vtable = kShaderMapVtable;
    NiTMap_RemoveAll(self);
    self->vtable = kShaderMapBaseVtable;
    NiTMap_RemoveAll(self);
    NiFree(self->buckets);
    if (flags & 1) {
        NiFreeSized(self, sizeof(ShaderMap));
    }
    return self;
}

// 0x517750 (thiscall): NiFixedString's destructor
static void __fastcall FixedStringDestruct(re::Ptr32<char>* self, re::Edx) {
    ReleaseString(self);
}

// 0x517790 (thiscall): the default shader (classification 0: the material's name, else
// "BullyDef") and an empty shader map of 0x407 buckets
static void __fastcall CreateShaders(MdBullyMaterial* self, re::Edx) {
    if (self->classification == 0) {
        uint8_t* shader = GetShaderOrDefault(self->name);
        if (self->defaultShader != shader) {
            Release(self->defaultShader);
            self->defaultShader = shader;
            if (shader) {
                vc::Increment(shader + 4);
            }
        }
    }
    auto* map = static_cast<ShaderMap*>(NiNew(sizeof(ShaderMap)));
    if (!map) {
        self->shaders = nullptr;
        return;
    }
    map->vtable = kShaderMapBaseVtable;
    map->hashSize = 0x407;
    map->count = 0;
    map->buckets = static_cast<re::Ptr32<void>*>(NiMalloc(0x101C));
    std::memset(map->buckets, 0, map->hashSize * 4);
    map->vtable = kShaderMapVtable;
    self->shaders = map;
}

// the shader name: the base name then the feature mask's part
void BuildName(char* out, const char* baseName, uint32_t flags) {
    char features[0x80];
    std::memset(features, 0, sizeof(features));
    ShaderNameFromFlags(flags, features);
    std::strcpy(out, baseName);
    std::strcat(out, features);
}

bool Usable(uint8_t* shader) {
    return shader && (vc::Call<uint8_t>(shader, 0x0C) || vc::Call<uint8_t>(shader, 0x10));
}

// 0x517840 (cdecl): the shader for a base name and feature mask (`classification`
// unused). Normally: the exact one from the library, else the mask with bits 0x10000,
// 0x8000, .. 1 dropped one after another (cumulatively) until one exists and
// initialises. With the dev flags (0xBCBB41 / 0xBCBB42): compiled from source into
// <dir>\<base>_<mask>.fxb and loaded as an NiD3DXEffectFile. (The source compiled is an
// empty NiString, and the .fxb built from a buffer that is never filled: uninitialised
// stack. Leftovers of a tool build)
static uint8_t* __cdecl LoadShaderImpl(uint32_t flags, const char* baseName, int) {
    char fxbPath[0x104];
    char sourcePath[0x104];
    char unfilledSource[0x104];   // (never written)
    char directory[0x104];
    char fileName[0x100];
    char name[0x100];

    BuildName(name, baseName, flags);
    std::sprintf(fileName, "%s_%8.8X", baseName, flags);
    void* library = ShaderLibrary_Get();
    if (!ms_bCompileShaders_BCBB41 && !ms_bCompileShaders_BCBB42) {
        if (uint8_t* shader = GetShaderFrom(name, library)) {
            return shader;
        }
    }
    Snprintf(sourcePath, 0x104, "%s\\%s", ms_shaderSourceDir, baseName);
    StandardisePath(sourcePath);
    re::Ptr32<char> source = nullptr;
    NiString_Construct(&source, 0x400);
    uint8_t* shader = nullptr;

    if (!ms_bCompileShaders_BCBB41 && !ms_bCompileShaders_BCBB42) {
        uint32_t mask = flags;
        uint32_t bit = 0x10000;
        do {
            mask &= ~bit;
            bit >>= 1;
            BuildName(name, baseName, mask);
            shader = vc::Call<uint8_t*>(library, 0x08, name, -1, 1);
            if (Usable(shader)) {
                NiString_Destruct(&source);
                return shader;
            }
        } while (bit);
        NiString_Destruct(&source);
        return nullptr;
    }

    void* errors = nullptr;
    void* compiler = nullptr;
    uint32_t length = NiString_Length(&source);
    long hr = D3DXCreateEffectCompiler(NiString_Chars(&source), length, ms_shaderDefines, nullptr, 0, &compiler,
                                       &errors);
    if (hr < 0) {
        if (errors) {
            ComCall<void*>(errors, 0x0C);   // GetBufferPointer (unused)
            if (errors) {
                ComCall<uint32_t>(errors, 0x08);
                errors = nullptr;
            }
        }
        NiString_Destruct(&source);
        return nullptr;
    }
    if (errors) {
        ComCall<uint32_t>(errors, 0x08);
        errors = nullptr;
    }
    void* effect = nullptr;
    hr = ComCall<long>(compiler, 0xEC, 0, &effect, &errors);   // CompileEffect
    if (hr < 0) {
        if (errors) {
            ComCall<void*>(errors, 0x0C);
            if (errors) {
                ComCall<uint32_t>(errors, 0x08);
                errors = nullptr;
            }
        }
        NiString_Destruct(&source);
        return nullptr;
    }
    if (errors) {
        ComCall<uint32_t>(errors, 0x08);
        errors = nullptr;
    }
    Snprintf(directory, 0x104, "%s", (flags & 0xF80000) ? ms_shaderDirWorld : ms_shaderDir);
    StandardisePath(directory);
    if (!DirectoryExists(directory)) {
        CreateDirectoryTree(directory);
    }
    Snprintf(fxbPath, 0x104, "%s\\%s.fxb", directory, fileName);
    StandardisePath(fxbPath);
    CompileEffectFile(unfilledSource, fxbPath, nullptr, ms_shaderDefines);
    uint8_t* loader = NiD3DXEffectLoader_Get(0);
    void* memory = NiNew(0x54);
    re::Ptr32<uint8_t> file = memory ? NiD3DXEffectFile_Construct(memory) : nullptr;
    const void* data = ComCall<const void*>(effect, 0x0C);   // GetBufferPointer
    uint32_t size = ComCall<uint32_t>(effect, 0x10);         // GetBufferSize
    uint8_t loaded = NiD3DXEffectFile_LoadFromMemory(loader, name, size, data, &file);
    if (effect) {
        ComCall<uint32_t>(effect, 0x08);
        effect = nullptr;
    }
    if (!loaded) {
        if (file) {
            NiD3DXEffectFile_Destruct(file);
            NiFreeSized(file, 0x54);
        }
        NiString_Destruct(&source);
        return nullptr;
    }
    At<int32_t>(file, 4) = 2;
    vc::Call<void>(loader, 0x04, file);
    void* effectLib = vc::Call<void*>(library, 0x2C, "NiD3DXEffectShaderLib");
    NiD3DXEffectShaderLib_AddEffect(effectLib, file);
    shader = vc::Call<uint8_t*>(library, 0x08, name, -1, 1);
    NiString_Destruct(&source);
    if (!shader || Usable(shader)) {
        return shader;
    }
    return nullptr;
}

// 0x517CE0 (cdecl): a shader loaded from its full name (mask from the name, base name
// up to the first "_"). Returns 0. (Terminates the base name one character late: the
// buffer was zeroed anyway)
static int __cdecl LoadShaderByName(const char* fullName) {
    uint32_t flags = 0;
    FlagsFromShaderName(fullName, &flags);
    char baseName[0x32];
    baseName[0] = '\0';
    std::memset(baseName + 1, 0, 0x31);
    int length = int(std::strchr(fullName, '_') - fullName);
    std::strncpy(baseName, fullName, length);
    baseName[length + 1] = '\0';
    LoadShader(flags, baseName, ClassifyShaderName(baseName));
    return 0;
}

// 0x517D60 (cdecl): the same for "<base>_<mask as 8 hex digits>". Returns 0
static int __cdecl LoadShaderByFileName(const char* fileName) {
    uint32_t flags = 0;
    int length = int(std::strlen(fileName));
    std::sscanf(fileName + length - 8, "%X", &flags);
    char baseName[0x32];
    baseName[0] = '\0';
    std::memset(baseName + 1, 0, 0x31);
    std::strncpy(baseName, fileName, length - 9);
    baseName[length - 9] = '\0';
    LoadShader(flags, baseName, ClassifyShaderName(baseName));
    return 0;
}

// 0x517DF0 (thiscall): the shader for a mask: from the map, else loaded (else
// "BullyDef", skinned or not) and added to the map. (The name built on the way is
// unused)
static uint8_t* __fastcall GetShader(MdBullyMaterial* self, re::Edx, uint32_t flags) {
    re::Ptr32<uint8_t> shader = nullptr;
    if (!ShaderMap_GetAt(self->shaders, flags, &shader)) {
        uint8_t* loaded = LoadShader(flags, self->name, self->classification);
        if (shader != loaded) {
            Release(shader);
            shader = loaded;
            if (loaded) {
                vc::Increment(loaded + 4);
            }
        }
        if (!shader) {
            char unused[0x64];
            unused[0] = '\0';
            std::memset(unused + 1, 0, 0x63);
            ShaderNameFromFlags(flags, unused);
            NiPointer_AssignRaw(&shader, GetDefaultShader((flags & 0x80000000) ? 1 : 0));
        }
        if (shader) {
            vc::Increment(shader + 4);   // (the NiPointer passed by value)
        }
        ShaderMap_SetAt(self->shaders, flags, shader);
    }
    Release(shader);
    return shader;
}

// 0x517F00 (thiscall) vf4: GetCurrentShader: the default shader for classification 0;
// while the swap processor draws, the depth shader (skinned or not); else the shader
// for the descriptor's mask (just skinned or not when there's no descriptor)
static uint8_t* __fastcall GetCurrentShader(MdBullyMaterial* self, re::Edx, uint8_t* geometry, void* skin,
                                            void* properties, void* effects, uint32_t) {
    ClassifyIfUnknown(self);
    if (!self->shaders && !self->defaultShader) {
        MdBullyMaterial_CreateShaders(self);
    }
    if (self->classification == 0) {
        return self->defaultShader;
    }
    if (ms_bSwapProcessing) {
        return MdBullyMaterial_GetShader(self, At<void*>(geometry, 0xD0) ? 0xC0000000 : 0x40000000);
    }
    Descriptor desc;
    Descriptor_Construct(&desc);
    uint32_t flags;
    if (GenerateDescriptor(geometry, skin, properties, effects, &desc)) {
        flags = FlagsFromDesc(&desc, self->classification == 1 ? 1 : 0);
    } else {
        flags = At<void*>(geometry, 0xD0) ? 0x80000000 : 0;
    }
    uint8_t* shader = MdBullyMaterial_GetShader(self, flags);
    FixedString_Destruct(&desc.name);
    return shader;
}

// 0x517FF0 (thiscall) vf5: the current shader's vtable[14] with the third argument
// (the default shader for classification 0, else the one for skinned or not)
static void __fastcall Vf5(MdBullyMaterial* self, re::Edx, uint8_t* geometry, void*, void* argument) {
    ClassifyIfUnknown(self);
    if (!self->shaders && !self->defaultShader) {
        MdBullyMaterial_CreateShaders(self);
    }
    if (self->classification == 0) {
        vc::Call<void>(self->defaultShader, 0x38, argument);
        return;
    }
    uint8_t* shader = MdBullyMaterial_GetShader(self, At<void*>(geometry, 0xD0) ? 0x80000000 : 0);
    vc::Call<void>(shader, 0x38, argument);
}

} // namespace MdBullyMaterial2

// ---- registration ----------------------------------------------------------------------

#define C16_MAT2_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace MdBullyMaterial2 {

C16_MAT2_REVERSED("MdBullyMaterial::GenerateDescriptor", GenerateDescriptorImpl, 0x00516CB0);
C16_MAT2_REVERSED("MdBullyMaterial::Create", Create, 0x005174A0);
C16_MAT2_REVERSED("MdBullyMaterial::Destroy", Destroy, 0x005175B0);
C16_MAT2_REVERSED("MdBullyMaterial::IsShaderCurrent", IsShaderCurrent, 0x005175D0);
C16_MAT2_REVERSED("MdBullyMaterial::UnloadAllShaders", UnloadAllShaders, 0x00517650);
C16_MAT2_REVERSED("NiTPointerMap<uint,NiPointer<NiShader>>::Destroy", ShaderMap_Destroy, 0x00517710);
C16_MAT2_REVERSED("NiFixedString::Destruct_517750", FixedStringDestruct, 0x00517750);
C16_MAT2_REVERSED("MdBullyMaterial::CreateShaders", CreateShaders, 0x00517790);
C16_MAT2_REVERSED("MdBullyMaterial::LoadShader", LoadShaderImpl, 0x00517840);
C16_MAT2_REVERSED("MdBullyMaterial::LoadShaderByName", LoadShaderByName, 0x00517CE0);
C16_MAT2_REVERSED("MdBullyMaterial::LoadShaderByFileName", LoadShaderByFileName, 0x00517D60);
C16_MAT2_REVERSED("MdBullyMaterial::GetShader", GetShader, 0x00517DF0);
C16_MAT2_REVERSED("MdBullyMaterial::GetCurrentShader", GetCurrentShader, 0x00517F00);
C16_MAT2_REVERSED("MdBullyMaterial::Vf5", Vf5, 0x00517FF0);

} // namespace MdBullyMaterial2
