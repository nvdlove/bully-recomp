// 0x516360 to 0x516B60: MdBullyMaterial's first part: the shader names Bully builds
// from a feature mask ("_skin_ERGLSN_PPPP_WSxx" and the like) and back, the shader
// look-ups (falling back to "BullyDef"), the material's destructor and its map of
// shaders by mask, and Gamebryo's NiMaterial::GetMaterial.
//
// The mask: 0x80000000 skinned; 0x40000000 "depth", 0x400000 "base_map", 0x800000
// "vert_norm" (with 0x1000 "norm_map"); 0x80000 / 0x100000 / 0x200000 the "wd_" /
// "sd_" / "pd_" prefixes, else E R G L S (0x40000, 0x10000, 0x8000, 0x4000, 0x2000);
// 0x1000 N; 1..8 the four P slots; 0x10..0x80 S and 0x100..0x800 W for four more.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_VAR(char*, ms_nullString, 0x00951028);
GAME_VAR(uint8_t*, ms_pMaterialMap, 0x020E2704);   // NiMaterial's, by name
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
GAME_FN(void, __thiscall, NiTMap_RemoveAll, 0x005AC520, void* map);
GAME_FN(void, __thiscall, NiMaterial_Destruct, 0x00762AC0, void* self);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(uint8_t, __thiscall, NiSkinInstance_ContainsVertexData, 0x00776F00, void* skin, int semantic);
GAME_FN(uint8_t, __thiscall, ShaderDesc_GetEntryType, 0x00777690, void* desc, uint32_t index, int* type);
GAME_FN(uint8_t, __thiscall, ShaderDesc_GetEntryFlag, 0x00777700, void* desc, uint32_t index, uint8_t* flag);

constexpr uint32_t kMdBullyMaterialVtable = 0x009177C0;
constexpr uint32_t kNiTMapBaseShaderVtable = 0x009177A8;

const char kBullyDef[] = "BullyDef";

// NiTMap<unsigned int, NiPointer<NiShader>>
struct ShaderMap {
    struct Item {
        re::Ptr32<Item> next;       // +0
        uint32_t key;     // +4
        re::Ptr32<uint8_t> shader;  // +8 NiPointer
    };
    uint32_t vtable;  // +0
    uint32_t hashSize; // +4
    re::Ptr32<re::Ptr32<Item>> buckets;   // +8
    uint32_t count;   // +0xC
};
static_assert(sizeof(ShaderMap) == 0x10);

struct MdBullyMaterial {
    uint32_t vtable;         // +0
    int32_t refCount;        // +4
    uint8_t unknown8[8];     // +8 NiMaterial's (+8 the name)
    re::Ptr32<ShaderMap> shaders;      // +0x10
    re::Ptr32<uint8_t> shader14;       // +0x14 NiPointer
};
static_assert(offsetof(MdBullyMaterial, shaders) == 0x10);
static_assert(offsetof(MdBullyMaterial, shader14) == 0x14);

void Release(uint8_t* object) {
    if (object && vc::Decrement(object + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

// the shader, initialised (vtable[3] IsInitialized, else vtable[4] Initialize); null
// when that fails
uint8_t* Initialised(uint8_t* shader) {
    if (shader && !vc::Call<uint8_t>(shader, 0x0C) && !vc::Call<uint8_t>(shader, 0x10)) {
        return nullptr;
    }
    return shader;
}

// (inlined strcat: from `p` on to the terminator, which after a lone "_" is whatever
// the buffer held)
char* End(char* p) {
    while (*p) {
        p++;
    }
    return p;
}

} // namespace

namespace MdBullyMaterial1 {

// 0x516360 (cdecl): the shader name for a feature mask
static void __cdecl ShaderNameFromFlags(uint32_t flags, char* out) {
    char* p;
    if (flags & 0x80000000) {
        std::memcpy(out, "_skin_", 7);
        p = out + 6;
    } else {
        out[0] = '_';   // (not terminated)
        p = out + 1;
    }
    if (flags & 0x40000000) {
        std::memcpy(End(p), "depth", 6);
        p[5] = '\0';
        return;
    }
    if (flags & 0x400000) {
        std::memcpy(End(p), "base_map", 9);
        p[8] = '\0';
        return;
    }
    if (flags & 0x800000) {
        if (flags & 0x1000) {
            std::memcpy(End(p), "norm_map", 9);
            p[8] = '\0';
        } else {
            std::memcpy(End(p), "vert_norm", 10);
            p[9] = '\0';
        }
        return;
    }
    if (flags & 0x80000) {
        p[0] = 'w';
        p[1] = 'd';
        p[2] = '_';
        p += 2;
    } else if (flags & 0x100000) {
        p[0] = 's';
        p[1] = 'd';
        p[2] = '_';
        p += 2;
    } else if (flags & 0x200000) {
        p[0] = 'p';
        p[1] = 'd';
        p[2] = '_';
        p += 2;
    } else {
        p[0] = (flags & 0x40000) ? 'E' : 'x';
        p[1] = (flags & 0x10000) ? 'R' : 'x';
        p[2] = (flags & 0x8000) ? 'G' : 'x';
        p[3] = (flags & 0x4000) ? 'L' : 'x';
        p[4] = (flags & 0x2000) ? 'S' : 'x';
        p += 4;
    }
    p[1] = (flags & 0x1000) ? 'N' : 'x';
    p[2] = '_';
    p += 3;
    for (int i = 0; i < 4; i++) {
        *p++ = (flags & (1u << i)) ? 'P' : 'x';
    }
    *p++ = '_';
    for (int i = 0; i < 4; i++) {
        if (flags & (0x100u << i)) {
            *p = 'W';
        } else {
            *p = (flags & (0x10u << i)) ? 'S' : 'x';
        }
        p++;
    }
    *p = '\0';
}

// 0x5165B0 (cdecl): the feature mask back from a shader name (from its first "_"; a
// name without one is read from address 1). The single letters are only checked for
// what they are, so "P" or "N" both give 0x1000 after the ERGLS
static void __cdecl FlagsFromShaderName(const char* name, uint32_t* flags) {
    *flags = 0;
    const char* s = std::strchr(name, '_') + 1;
    if (std::strncmp(s, "skin_", 5) == 0) {
        *flags |= 0x80000000;
        s += 5;
    }
    if (std::strncmp(s, "depth", 5) == 0) {
        *flags |= 0x40000000;
        return;
    }
    if (std::strncmp(s, "base_map", 8) == 0) {
        *flags |= 0x400000;
        return;
    }
    if (std::strncmp(s, "norm_map", 8) == 0) {
        *flags |= 0x801000;
        return;
    }
    if (std::strncmp(s, "vert_norm", 9) == 0) {
        *flags |= 0x800000;
        return;
    }
    const char* p;
    bool prefixed = true;
    if (std::strncmp(s, "wd_", 3) == 0) {
        *flags |= 0x80000;
    } else if (std::strncmp(s, "sd_", 3) == 0) {
        *flags |= 0x100000;
    } else if (std::strncmp(s, "pd_", 3) == 0) {
        *flags |= 0x200000;
    } else {
        prefixed = false;
    }
    if (prefixed) {
        p = s + 3;
        if (*p == 'N') {
            *flags |= 0x1000;
        }
    } else {
        if (s[0] == 'E') {
            *flags |= 0x40000;
        }
        if (s[1] == 'R') {
            *flags |= 0x10000;
        }
        if (s[2] == 'G') {
            *flags |= 0x8000;
        }
        if (s[3] == 'L') {
            *flags |= 0x4000;
        }
        if (s[4] == 'S') {
            *flags |= 0x2000;
        }
        p = s + 5;
        if (*p == 'P' || *p == 'N') {
            *flags |= 0x1000;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (p[2 + i] == 'P') {
            *flags |= 1u << i;
        }
    }
    for (int i = 0; i < 4; i++) {
        char c = p[7 + i];
        if (c == 'W') {
            *flags |= 0x100u << i;
        } else if (c == 'S') {
            *flags |= 0x10u << i;
        }
    }
}

// 0x5167B0 (stdcall): "BullyDef" (or "BullyDef_skin"), initialised
static uint8_t* __stdcall GetDefaultShader(int skinned) {
    void* library = ShaderLibrary_Get();
    const char* name = re::Flag(skinned) ? "BullyDef_skin" : kBullyDef;
    return Initialised(vc::Call<uint8_t*>(library, 0x08, name, -1, 1));
}

// 0x516810 (stdcall): the named shader, initialised, else "BullyDef"
static uint8_t* __stdcall GetShaderOrDefault(const char* name) {
    uint8_t* shader = Initialised(vc::Call<uint8_t*>(ShaderLibrary_Get(), 0x08, name, -1, 1));
    if (shader) {
        return shader;
    }
    return Initialised(vc::Call<uint8_t*>(ShaderLibrary_Get(), 0x08, kBullyDef, -1, 1));
}

// 0x516890 (cdecl): 1 for "BullyDef", 2 for names starting "Md", else 0 (and for
// names of 3 characters or fewer)
static int __cdecl ClassifyShaderName(const char* name) {
    if (std::strlen(name) <= 3) {
        return 0;
    }
    // (repe cmpsb over 9 bytes: stops at the first difference)
    bool same = true;
    for (int i = 0; i < 9; i++) {
        if (name[i] != kBullyDef[i]) {
            same = false;
            break;
        }
    }
    if (same) {
        return 1;
    }
    if (name[0] == 'M' && name[1] == 'd') {
        return 2;
    }
    return 0;
}

// 0x5168F0 (cdecl): the named shader from a library (vtable[2]), initialised
static uint8_t* __cdecl GetShaderFrom(const char* name, void* library) {
    return Initialised(vc::Call<uint8_t*>(library, 0x08, name, -1, 1));
}

// 0x516930 (thiscall): GetRTTI
static void* __fastcall GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x00C6762C);
}

// 0x516940 (thiscall): the destructor body: the shader map emptied and deleted, +0x14
// released (twice over: the second, NiPointer's destructor, finds it null)
static void __fastcall Destruct(MdBullyMaterial* self, re::Edx) {
    self->vtable = kMdBullyMaterialVtable;
    if (self->shaders) {
        NiTMap_RemoveAll(self->shaders);
        if (self->shaders) {
            vc::Call<void*>(self->shaders, 0x00, 1);
        }
    }
    self->shaders = nullptr;
    if (self->shader14) {
        Release(self->shader14);
        self->shader14 = nullptr;
    }
    Release(self->shader14);
    NiMaterial_Destruct(self);
}

// 0x5169C0 (thiscall) vf6: the same without destructing: the shaders dropped
static void __fastcall UnloadShaders(MdBullyMaterial* self, re::Edx) {
    if (self->shaders) {
        NiTMap_RemoveAll(self->shaders);
        if (self->shaders) {
            vc::Call<void*>(self->shaders, 0x00, 1);
        }
    }
    self->shaders = nullptr;
    if (self->shader14) {
        Release(self->shader14);
        self->shader14 = nullptr;
    }
}

// 0x516A10 (thiscall): NiTMap<unsigned int, NiPointer<NiShader>>::GetAt
static uint8_t __fastcall ShaderMap_GetAt(ShaderMap* self, re::Edx, uint32_t key, re::Ptr32<uint8_t>* out) {
    ShaderMap::Item* item = self->buckets[key % self->hashSize];
    for (; item; item = item->next) {
        if (item->key == key) {
            break;
        }
    }
    if (!item) {
        return 0;
    }
    if (*out != item->shader) {
        Release(*out);
        *out = item->shader;
        if (*out) {
            vc::Increment(*out + 4);
        }
    }
    return 1;
}

// 0x516A80 (thiscall): NiTMapBase<..., NiPointer<NiShader>>'s deleting destructor
static void* __fastcall ShaderMap_Destroy(ShaderMap* self, re::Edx, int flags) {
    self->vtable = kNiTMapBaseShaderVtable;
    NiTMap_RemoveAll(self);
    NiFree(self->buckets);
    if (flags & 1) {
        NiFreeSized(self, 0x10);
    }
    return self;
}

// 0x516AC0 (cdecl): NiMaterial::GetMaterial: the material by name (none for the null
// string or an empty one). The map is keyed by the fixed string's pointer
static uint8_t* __cdecl GetMaterial(const re::Ptr32<char>* name) {
    char* s = *name;
    if (s == ms_nullString || *reinterpret_cast<uint16_t*>(s - 4) == 0) {
        return nullptr;
    }
    ShaderMap* map = re::PtrCast<ShaderMap*>(ms_pMaterialMap);
    uint32_t key = reinterpret_cast<uint32_t>(s);
    for (ShaderMap::Item* item = map->buckets[key % map->hashSize]; item; item = item->next) {
        if (item->key == key) {
            return item->shader;
        }
    }
    return nullptr;
}

// 0x516B10 (thiscall): NiGeometry::ContainsVertexData: the model data's (+0xCC,
// vtable[19]) or the skin instance's (+0xD0)
static uint8_t __fastcall ContainsVertexData(uint8_t* self, re::Edx, int semantic) {
    if (vc::Call<uint8_t>(*reinterpret_cast<re::Ptr32<void>*>(self + 0xCC), 0x4C, semantic)) {
        return 1;
    }
    void* skin = *reinterpret_cast<re::Ptr32<void>*>(self + 0xD0);
    if (skin) {
        return NiSkinInstance_ContainsVertexData(skin, semantic);
    }
    return 0;
}

// 0x516B50 (thiscall): the word at +0xA of the object at +0x30 (0 without one)
static uint16_t __fastcall Get30Count(uint8_t* self, re::Edx) {
    uint8_t* object = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x30);
    if (!object) {
        return 0;
    }
    return *reinterpret_cast<uint16_t*>(object + 0xA);
}

// 0x516B60 (cdecl): the feature mask for a shader description: skinned from +4's low
// bits, the ERGLSN features (with `features`) from +4's, and the P / S / W slots from
// the entries (type 1: the next P; type 2: the next S, or W when its flag is set)
static uint32_t __cdecl FlagsFromDesc(uint8_t* desc, int features) {
    uint32_t bits = *reinterpret_cast<uint32_t*>(desc + 4);
    uint32_t flags = 0;
    if ((bits & 3) == 1) {
        flags = 0x80000000;
    }
    if (re::Flag(features)) {
        if (bits & 0x20000) {
            flags |= 0x1000;
        }
        if (bits & 0x10) {
            flags |= 0x2000;
        }
        if (bits & 0x800000) {
            flags |= 0x4000;
        }
        if (bits & 0x1000000) {
            flags |= 0x8000;
        }
        if ((bits & 0xE000000) == 0xA000000) {
            flags |= 0x10000;
        }
    }
    if ((bits & 0xC0) != 0x80) {
        return flags | 0x40000;
    }
    uint32_t counts = *reinterpret_cast<uint32_t*>(desc + 8);
    uint32_t entries = ((counts >> 8) & 0xF) + ((counts >> 4) & 0xF);
    uint32_t pSlots = 0;
    uint32_t sSlots = 0;
    int type = 2;   // (kept from one entry to the next when the getter fails)
    for (uint32_t i = 0; i < entries; i++) {
        if (!ShaderDesc_GetEntryType(desc, i + 1, &type)) {
            continue;
        }
        uint8_t flag = 0;
        if (!ShaderDesc_GetEntryFlag(desc, i, &flag)) {
            continue;
        }
        if (type == 1) {
            if (pSlots < 4) {
                flags |= 1u << pSlots;
            }
            pSlots++;
        } else if (type == 2) {
            if (sSlots < 4) {
                flags |= flag ? (0x100u << sSlots) : (0x10u << sSlots);
            }
            sSlots++;
        }
    }
    return flags;
}

} // namespace MdBullyMaterial1

// ---- registration ----------------------------------------------------------------------

#define C16B_MAT_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace MdBullyMaterial1 {

static void(__cdecl* Original_FlagsFromShaderName)(const char*, uint32_t*);
static int(__cdecl* Original_ClassifyShaderName)(const char*);

static void __cdecl Shadow_FlagsFromShaderName(const char* name, uint32_t* flags) {
    Original_FlagsFromShaderName(name, flags);
    uint32_t ours = 0;
    FlagsFromShaderName(name, &ours);
    static uint32_t compared = 0;
    re::Compared("MdBullyMaterial::FlagsFromShaderName", compared);
    if (ours != *flags) {
        re::Mismatch("MdBullyMaterial::FlagsFromShaderName", "%s: game %08X, ours %08X", name, *flags, ours);
    }
}

static int __cdecl Shadow_ClassifyShaderName(const char* name) {
    int theirs = Original_ClassifyShaderName(name);
    int ours = ClassifyShaderName(name);
    static uint32_t compared = 0;
    re::Compared("MdBullyMaterial::ClassifyShaderName", compared);
    if (theirs != ours) {
        re::Mismatch("MdBullyMaterial::ClassifyShaderName", "%s: game %d, ours %d", name, theirs, ours);
    }
    return theirs;
}

C16B_MAT_REVERSED("MdBullyMaterial::ShaderNameFromFlags", ShaderNameFromFlags, 0x00516360);
REVERSED("MdBullyMaterial::FlagsFromShaderName", 0x005165B0, RE_BODY(&FlagsFromShaderName),
         reinterpret_cast<void*>(&Shadow_FlagsFromShaderName),
         reinterpret_cast<void**>(&Original_FlagsFromShaderName), re::Mode::Original);
C16B_MAT_REVERSED("MdBullyMaterial::GetDefaultShader", GetDefaultShader, 0x005167B0);
C16B_MAT_REVERSED("MdBullyMaterial::GetShaderOrDefault", GetShaderOrDefault, 0x00516810);
REVERSED("MdBullyMaterial::ClassifyShaderName", 0x00516890, RE_BODY(&ClassifyShaderName),
         reinterpret_cast<void*>(&Shadow_ClassifyShaderName),
         reinterpret_cast<void**>(&Original_ClassifyShaderName), re::Mode::Original);
C16B_MAT_REVERSED("MdBullyMaterial::GetShaderFrom", GetShaderFrom, 0x005168F0);
C16B_MAT_REVERSED("MdBullyMaterial::GetRTTI", GetRTTI, 0x00516930);
C16B_MAT_REVERSED("MdBullyMaterial::Destruct", Destruct, 0x00516940);
C16B_MAT_REVERSED("MdBullyMaterial::UnloadShaders", UnloadShaders, 0x005169C0);
C16B_MAT_REVERSED("NiTMap<uint,NiPointer<NiShader>>::GetAt", ShaderMap_GetAt, 0x00516A10);
C16B_MAT_REVERSED("NiTMapBase<uint,NiPointer<NiShader>>::Destroy", ShaderMap_Destroy, 0x00516A80);
C16B_MAT_REVERSED("NiMaterial::GetMaterial", GetMaterial, 0x00516AC0);
C16B_MAT_REVERSED("NiGeometry::ContainsVertexData", ContainsVertexData, 0x00516B10);
C16B_MAT_REVERSED("MdBullyMaterial::Get30Count_516B50", Get30Count, 0x00516B50);
C16B_MAT_REVERSED("MdBullyMaterial::FlagsFromDesc", FlagsFromDesc, 0x00516B60);

} // namespace MdBullyMaterial1
