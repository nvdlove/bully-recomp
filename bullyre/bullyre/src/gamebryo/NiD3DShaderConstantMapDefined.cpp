// NiD3DShaderConstantMap::SetDefinedConstant (slot 16 of vtable 0x99C348): a predefined mapping's
// value for the geometry being drawn, given to the shader program (its slot 14,
// SetShaderConstant(entry, data, 0); the bone matrices through its slot 15,
// SetShaderConstantArray). By the entry's mapping (+0xC; the ids and names of
// NiShaderConstantMapTables.cpp): the renderer's world / view / projection (+0x750 / +0x790 /
// +0x7D0) and their products, the skin's skin-to-world matrix and its products with them (each
// inverted and / or transposed through D3DX, as named), the bone matrices, the texture maps'
// transforms (0x88B440), the material's colours, the eye (the inverse view +0x810), the time extra
// data and its sine / cosine / tangent (the CRT's _CIsin / _CIcos / _CItan), the ambient lights,
// the fog, the bump and parallax maps' values, the textures' sizes, the alpha test. Float maths in
// the game's order (the main thread's 24-bit x87: each operation a float's).
//
// Returns 0 when set, and when there's nothing to set (no skin for the skin and bone matrices, no
// material for its colours, no texturing property for the texture sizes, the light mappings
// 0x45..0x48 and unknown ids: nothing given to the program); 0x50 when the program refused the
// value; 0x10 when the geometry has no time extra data (the game then reads through the null:
// see the time case).
//
// The state (NiPropertyState): +0x8 the alpha property, +0x10 the fog, +0x14 the material, +0x28
// the texturing property (+0x20 its maps: 0 base, 1 dark, 2 detail, 3 gloss, 4 glow, 5 bump,
// 6 normal, 7 parallax, 8.. the decals; a map's +0x8 texture, +0xC transform). The map: +0x3C the
// renderer, +0x40 its render state (+0x78 / +0x7C / +0x84 the fog values).

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiDX9Renderer.h"
#include "gamebryo/NiDX9RendererData.h"
#include "reversed.h"

namespace ni::c88 {
namespace {

using ni::c40::ConstantMapEntry;
using ni::sys::At;
using ni::sys::Virtual;

using Matrix = float[16];

// the predefined mappings this function knows (NiShaderConstantMapTables.cpp's kTable1)
enum Mapping : uint32_t {
    kProj = 0x1,
    kInvProj = 0x2,
    kView = 0x3,
    kInvView = 0x4,
    kWorld = 0x5,
    kInvWorld = 0x6,
    kWorldView = 0x7,
    kInvWorldView = 0x8,
    kViewProj = 0x9,
    kInvViewProj = 0xA,
    kWorldViewProj = 0xB,
    kInvWorldViewProj = 0xC,
    kProjTranspose = 0xD,
    kInvProjTranspose = 0xE,
    kViewTranspose = 0xF,
    kInvViewTranspose = 0x10,
    kWorldTranspose = 0x11,
    kInvWorldTranspose = 0x12,
    kWorldViewTranspose = 0x13,
    kInvWorldViewTranspose = 0x14,
    kViewProjTranspose = 0x15,
    kInvViewProjTranspose = 0x16,
    kWorldViewProjTranspose = 0x17,
    kInvWorldViewProjTranspose = 0x18,
    kSkinWorldView = 0x19,
    kInvSkinWorldView = 0x1A,
    kSkinWorldViewTranspose = 0x1B,
    kInvSkinWorldViewTranspose = 0x1C,
    kSkinWorldViewProj = 0x1D,
    kInvSkinWorldViewProj = 0x1E,
    kSkinWorldViewProjTranspose = 0x1F,
    kInvSkinWorldViewProjTranspose = 0x20,
    kBoneMatrix3 = 0x21,
    kBoneMatrix4 = 0x22,
    kSkinBoneMatrix3 = 0x23,
    kSkinBoneMatrix4 = 0x24,
    kTexTransformBase = 0x25,
    kInvTexTransformBase = 0x26,
    kTexTransformBaseTranspose = 0x27,
    kInvTexTransformBaseTranspose = 0x28,
    kTexTransformDark = 0x29,
    kInvTexTransformDark = 0x2A,
    kTexTransformDarkTranspose = 0x2B,
    kInvTexTransformDarkTranspose = 0x2C,
    kTexTransformDetail = 0x2D,
    kInvTexTransformDetail = 0x2E,
    kTexTransformDetailTranspose = 0x2F,
    kInvTexTransformDetailTranspose = 0x30,
    kTexTransformGloss = 0x31,
    kInvTexTransformGloss = 0x32,
    kTexTransformGlossTranspose = 0x33,
    kInvTexTransformGlossTranspose = 0x34,
    kTexTransformGlow = 0x35,
    kInvTexTransformGlow = 0x36,
    kTexTransformGlowTranspose = 0x37,
    kInvTexTransformGlowTranspose = 0x38,
    kTexTransformBump = 0x39,
    kInvTexTransformBump = 0x3A,
    kTexTransformBumpTranspose = 0x3B,
    kInvTexTransformBumpTranspose = 0x3C,
    kTexTransformDecal = 0x3D,
    kInvTexTransformDecal = 0x3E,
    kTexTransformDecalTranspose = 0x3F,
    kInvTexTransformDecalTranspose = 0x40,
    kTexTransformShader = 0x41,
    kInvTexTransformShader = 0x42,
    kTexTransformShaderTranspose = 0x43,
    kInvTexTransformShaderTranspose = 0x44,
    kLightPosWS = 0x45,
    kLightDirWS = 0x46,
    kLightPosOS = 0x47,
    kLightDirOS = 0x48,
    kMaterialDiffuse = 0x49,
    kMaterialAmbient = 0x4A,
    kMaterialSpecular = 0x4B,
    kMaterialEmissive = 0x4C,
    kMaterialPower = 0x4D,
    kEyePos = 0x4E,
    kEyeDir = 0x4F,
    kTaylorSin = 0x50,
    kTaylorCos = 0x51,
    kTime = 0x52,
    kSinTime = 0x53,
    kCosTime = 0x54,
    kTanTime = 0x55,
    kTimeSinCosTan = 0x56,
    kAmbientLight = 0x57,
    kTexTransformNormal = 0x58,
    kInvTexTransformNormal = 0x59,
    kTexTransformNormalTranspose = 0x5A,
    kInvTexTransformNormalTranspose = 0x5B,
    kTexTransformParallax = 0x5C,
    kInvTexTransformParallax = 0x5D,
    kTexTransformParallaxTranspose = 0x5E,
    kInvTexTransformParallaxTranspose = 0x5F,
    kSkinWorld = 0x60,
    kInvSkinWorld = 0x61,
    kSkinWorldTranspose = 0x62,
    kInvSkinWorldTranspose = 0x63,
    kFogDensity = 0x64,
    kFogNearFar = 0x65,
    kFogColor = 0x66,
    kBumpMatrix = 0x67,
    kBumpLumaAndScale = 0x68,
    kParallaxOffset = 0x69,
    kTexSizeBase = 0x6A,
    kTexSizeDark = 0x6B,
    kTexSizeDetail = 0x6C,
    kTexSizeGloss = 0x6D,
    kTexSizeGlow = 0x6E,
    kTexSizeBump = 0x6F,
    kTexSizeNormal = 0x70,
    kTexSizeParallax = 0x71,
    kTexSizeDecal = 0x72,
    kTexSizeShader = 0x73,
    kAlphaTestFunction = 0x74,
    kAlphaTestRef = 0x75,
};

// the texturing property's maps by index (and the two found another way)
enum MapIndex : uint32_t {
    kBaseMap = 0,
    kDarkMap = 1,
    kDetailMap = 2,
    kGlossMap = 3,
    kGlowMap = 4,
    kBumpMap = 5,
    kNormalMap = 6,
    kParallaxMap = 7,
    kDecalMaps = 8,          // 8 + the entry's extra
    kShaderMap = 0xFFFFFFFF, // GetShaderMap(the entry's extra)
};

constexpr uintptr_t NiAmbientLight_RTTI = 0x020E3480;
GAME_VAR(const char*, ms_pcTimeExtraData, 0x00BC8B20);   // the time constants' float extra data
GAME_ARRAY(const float, NiPoint3_ZERO, 0x020E2D44);

GAME_FN(void, __thiscall, FixedString_Construct, 0x00401050, re::Ptr32<char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Destruct, 0x00517750, re::Ptr32<char>* self);
GAME_FN(uint8_t*, __thiscall, ObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(uint8_t*, __cdecl, DynamicCast, 0x00401FF0, uintptr_t rtti, const void* object);
GAME_FN(float*, __thiscall, Color_Times, 0x0074A400, const float* self, float* result, float scale);
GAME_FN(const float*, __thiscall, TextureTransform_GetMatrix, 0x0074A440, void* self);
GAME_FN(uint8_t*, __thiscall, TexturingProperty_GetShaderMap, 0x0074A7E0, void* self, uint32_t index);
// a texture transform's 3x3 as a D3D texture matrix (the identity for null; inverted / transposed
// when the flags' low bytes say)
GAME_FN(void, __cdecl, TextureTransform_ToD3D, 0x0088B440, float* out, const float* matrix, int invert,
        int transpose, uint32_t coordinates);

// a matrix copied (the game's rep movsd into its own stack copy)
inline void CopyMatrix(float* to, const float* from) {
    for (int i = 0; i < 16; i++) {
        to[i] = from[i];
    }
}

inline const float* RendererMatrix(const uint8_t* renderer, int field) {
    return reinterpret_cast<const float*>(renderer + field);
}

// inverted, then transposed, in place (D3DX), as asked
inline void InvertTranspose(float* m, bool invert, bool transpose) {
    if (invert) {
        ni::c40::MatrixInverse(m);
    }
    if (transpose) {
        ni::c40::MatrixTranspose(m, m);
    }
}

// the texturing property's map at an index (no bounds check, as the game's)
inline uint8_t* Map(const uint8_t* texturing, uint32_t index) {
    return At<re::Ptr32<uint8_t>*>(texturing, 0x20)[index];
}

// a texture's width / height (slots 18 / 19) as a float: the game's fild of the unsigned value,
// 2^32 added when it's negative, one rounding to a float (what the exact double rounded is)
inline float UnsignedToFloat(uint32_t value) {
    return float(double(value));
}

// a texture map's transform constant into `m`: the map's transform when the texturing property has
// the map and the map a transform (the second / fourth of each group of four inverted, the third /
// fourth transposed: only with a transform), else the identity. (The game passes its flags as
// dwords with only the low byte set, the rest its stack's leftovers: 0x88B440 reads the low byte.)
void TextureTransformConstant(float* m, uint8_t* state, const ConstantMapEntry* entry, uint32_t mapping,
                              uint32_t first, uint32_t index) {
    const float* transform = nullptr;
    int invert = 0;
    int transpose = 0;
    uint8_t* texturing = At<uint8_t*>(state, 0x28);
    if (texturing != nullptr) {
        uint8_t* map;
        if (index == kShaderMap) {
            map = TexturingProperty_GetShaderMap(texturing, entry->extra);
        } else if (index == kDecalMaps) {
            map = Map(texturing, kDecalMaps + entry->extra);
        } else {
            map = Map(texturing, index);
        }
        if (map != nullptr && At<void*>(map, 0xC) != nullptr) {
            transform = TextureTransform_GetMatrix(At<void*>(map, 0xC));
            uint32_t kind = mapping - first;
            if (kind == 1 || kind == 3) {
                invert = 1;
            }
            if (kind == 2 || kind == 3) {
                transpose = 1;
            }
        }
    }
    TextureTransform_ToD3D(m, transform, invert, transpose, 4);
}

} // namespace

// 0x88C6B0 (thiscall, slot 16): NiD3DShaderConstantMap::SetDefinedConstant
uint32_t __fastcall NiD3DShaderConstantMap_SetDefinedConstant(uint8_t* self, re::Edx, void* program,
                                                              ConstantMapEntry* entry, void* geometry,
                                                              uint8_t* skin, uint8_t* partition, void*,
                                                              uint8_t* state, uint8_t* effects, const NiTransform*,
                                                              const NiBound*, uint32_t) {
    uint32_t result = 0;
    uint32_t mapping = entry->internal;
    alignas(16) Matrix m;   // the matrix values
    alignas(16) Matrix a;   // the renderer's matrices copied
    alignas(16) Matrix b;
    alignas(16) Matrix c;
    alignas(16) Matrix ab;
    // the vector values: where the game sends four floats of which it wrote fewer, the rest are its
    // stack's leftovers (uninitialised): 0 here
    float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const void* data = nullptr;
    bool invert = false;
    bool transpose = false;
    switch (mapping) {
    case kProj:
    case kInvProj:
    case kProjTranspose:
    case kInvProjTranspose:
        invert = mapping == kInvProj || mapping == kInvProjTranspose;
        transpose = mapping == kProjTranspose || mapping == kInvProjTranspose;
        CopyMatrix(m, RendererMatrix(At<uint8_t*>(self, 0x3C), ni::dx9::kProjection));
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    case kView:
    case kInvView:
    case kViewTranspose:
    case kInvViewTranspose:
        invert = mapping == kInvView || mapping == kInvViewTranspose;
        transpose = mapping == kViewTranspose || mapping == kInvViewTranspose;
        CopyMatrix(m, RendererMatrix(At<uint8_t*>(self, 0x3C), ni::dx9::kView));
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    case kWorld:
    case kInvWorld:
    case kWorldTranspose:
    case kInvWorldTranspose:
        invert = mapping == kInvWorld || mapping == kInvWorldTranspose;
        transpose = mapping == kWorldTranspose || mapping == kInvWorldTranspose;
        CopyMatrix(m, RendererMatrix(At<uint8_t*>(self, 0x3C), ni::dx9::kWorld));
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    case kWorldView:
    case kInvWorldView:
    case kWorldViewTranspose:
    case kInvWorldViewTranspose: {
        invert = mapping == kInvWorldView || mapping == kInvWorldViewTranspose;
        transpose = mapping == kWorldViewTranspose || mapping == kInvWorldViewTranspose;
        const uint8_t* renderer = At<uint8_t*>(self, 0x3C);
        CopyMatrix(a, RendererMatrix(renderer, ni::dx9::kWorld));
        CopyMatrix(b, RendererMatrix(renderer, ni::dx9::kView));
        ni::c40::MatrixMultiply(m, a, b);
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    }
    case kViewProj:
    case kInvViewProj:
    case kViewProjTranspose:
    case kInvViewProjTranspose: {
        invert = mapping == kInvViewProj || mapping == kInvViewProjTranspose;
        transpose = mapping == kViewProjTranspose || mapping == kInvViewProjTranspose;
        const uint8_t* renderer = At<uint8_t*>(self, 0x3C);
        CopyMatrix(a, RendererMatrix(renderer, ni::dx9::kView));
        CopyMatrix(b, RendererMatrix(renderer, ni::dx9::kProjection));
        ni::c40::MatrixMultiply(m, a, b);
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    }
    case kWorldViewProj:
    case kInvWorldViewProj:
    case kWorldViewProjTranspose:
    case kInvWorldViewProjTranspose: {
        invert = mapping == kInvWorldViewProj || mapping == kInvWorldViewProjTranspose;
        transpose = mapping == kWorldViewProjTranspose || mapping == kInvWorldViewProjTranspose;
        const uint8_t* renderer = At<uint8_t*>(self, 0x3C);
        CopyMatrix(a, RendererMatrix(renderer, ni::dx9::kWorld));
        CopyMatrix(b, RendererMatrix(renderer, ni::dx9::kView));
        CopyMatrix(c, RendererMatrix(renderer, ni::dx9::kProjection));
        ni::c40::MatrixMultiply(ab, a, b);
        ni::c40::MatrixMultiply(m, ab, c);
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    }
    case kSkinWorldView:
    case kInvSkinWorldView:
    case kSkinWorldViewTranspose:
    case kInvSkinWorldViewTranspose:   // the skin's skin-to-world matrix (+0x2C) times the view
        if (skin == nullptr) {
            return result;
        }
        CopyMatrix(m, At<const float*>(skin, 0x2C));
        invert = mapping == kInvSkinWorldView || mapping == kInvSkinWorldViewTranspose;
        transpose = mapping == kSkinWorldViewTranspose || mapping == kInvSkinWorldViewTranspose;
        CopyMatrix(b, RendererMatrix(At<uint8_t*>(self, 0x3C), ni::dx9::kView));
        ni::c40::MatrixMultiply(m, m, b);
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    case kSkinWorldViewProj:
    case kInvSkinWorldViewProj:
    case kSkinWorldViewProjTranspose:
    case kInvSkinWorldViewProjTranspose: {   // ... times the view and the projection
        if (skin == nullptr) {
            return result;
        }
        CopyMatrix(m, At<const float*>(skin, 0x2C));
        invert = mapping == kInvSkinWorldViewProj || mapping == kInvSkinWorldViewProjTranspose;
        transpose = mapping == kSkinWorldViewProjTranspose || mapping == kInvSkinWorldViewProjTranspose;
        const uint8_t* renderer = At<uint8_t*>(self, 0x3C);
        CopyMatrix(b, RendererMatrix(renderer, ni::dx9::kView));
        CopyMatrix(c, RendererMatrix(renderer, ni::dx9::kProjection));
        ni::c40::MatrixMultiply(m, m, b);
        ni::c40::MatrixMultiply(m, m, c);
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    }
    case kSkinWorld:
    case kInvSkinWorld:
    case kSkinWorldTranspose:
    case kInvSkinWorldTranspose:   // the skin's skin-to-world matrix
        if (skin == nullptr) {
            return result;
        }
        CopyMatrix(m, At<const float*>(skin, 0x2C));
        invert = mapping == kInvSkinWorld || mapping == kInvSkinWorldTranspose;
        transpose = mapping == kSkinWorldTranspose || mapping == kInvSkinWorldTranspose;
        InvertTranspose(m, invert, transpose);
        data = m;
        break;
    case kBoneMatrix3:
    case kBoneMatrix4:
    case kSkinBoneMatrix3:
    case kSkinBoneMatrix4:   // the skin's bone matrices (+0x28; +0x20 registers each) as an array, by
                             // the partition's bones (+0x20 the count, +0x4 the indices; the
                             // partition isn't checked)
        if (skin == nullptr) {
            return result;
        }
        if (Virtual<uint8_t>(program, 15, entry, At<const void*>(skin, 0x28), uint32_t(At<uint16_t>(partition, 0x20)),
                             At<uint32_t>(skin, 0x20), At<const uint16_t*>(partition, 4)) == 0) {
            result = 0x50;
        }
        return result;
    case kTexTransformBase:
    case kInvTexTransformBase:
    case kTexTransformBaseTranspose:
    case kInvTexTransformBaseTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformBase, kBaseMap);
        data = m;
        break;
    case kTexTransformDark:
    case kInvTexTransformDark:
    case kTexTransformDarkTranspose:
    case kInvTexTransformDarkTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformDark, kDarkMap);
        data = m;
        break;
    case kTexTransformDetail:
    case kInvTexTransformDetail:
    case kTexTransformDetailTranspose:
    case kInvTexTransformDetailTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformDetail, kDetailMap);
        data = m;
        break;
    case kTexTransformGloss:
    case kInvTexTransformGloss:
    case kTexTransformGlossTranspose:
    case kInvTexTransformGlossTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformGloss, kGlossMap);
        data = m;
        break;
    case kTexTransformGlow:
    case kInvTexTransformGlow:
    case kTexTransformGlowTranspose:
    case kInvTexTransformGlowTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformGlow, kGlowMap);
        data = m;
        break;
    case kTexTransformBump:
    case kInvTexTransformBump:
    case kTexTransformBumpTranspose:
    case kInvTexTransformBumpTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformBump, kBumpMap);
        data = m;
        break;
    case kTexTransformNormal:
    case kInvTexTransformNormal:
    case kTexTransformNormalTranspose:
    case kInvTexTransformNormalTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformNormal, kNormalMap);
        data = m;
        break;
    case kTexTransformParallax:
    case kInvTexTransformParallax:
    case kTexTransformParallaxTranspose:
    case kInvTexTransformParallaxTranspose:
        TextureTransformConstant(m, state, entry, mapping, kTexTransformParallax, kParallaxMap);
        data = m;
        break;
    case kTexTransformDecal:
    case kInvTexTransformDecal:
    case kTexTransformDecalTranspose:
    case kInvTexTransformDecalTranspose:   // the decal map of the entry's index
        TextureTransformConstant(m, state, entry, mapping, kTexTransformDecal, kDecalMaps);
        data = m;
        break;
    case kTexTransformShader:
    case kInvTexTransformShader:
    case kTexTransformShaderTranspose:
    case kInvTexTransformShaderTranspose:   // the shader map of the entry's index
        TextureTransformConstant(m, state, entry, mapping, kTexTransformShader, kShaderMap);
        data = m;
        break;
    case kMaterialDiffuse:
    case kMaterialAmbient:
    case kMaterialSpecular:
    case kMaterialEmissive: {   // the material's colour (+0x28 / +0x1C / +0x34 / +0x40) with its
                                // alpha (+0x50)
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return result;
        }
        int offset = mapping == kMaterialDiffuse   ? 0x28
                     : mapping == kMaterialAmbient ? 0x1C
                     : mapping == kMaterialSpecular ? 0x34
                                                    : 0x40;
        v[0] = At<float>(material, offset);
        v[1] = At<float>(material, offset + 4);
        v[2] = At<float>(material, offset + 8);
        v[3] = At<float>(material, 0x50);
        data = v;
        break;
    }
    case kMaterialPower: {   // its shininess (+0x4C) four times
        uint8_t* material = At<uint8_t*>(state, 0x14);
        if (material == nullptr) {
            return result;
        }
        v[0] = At<float>(material, 0x4C);
        v[1] = At<float>(material, 0x4C);
        v[2] = At<float>(material, 0x4C);
        v[3] = At<float>(material, 0x4C);
        data = v;
        break;
    }
    case kEyePos:
    case kEyeDir: {   // the inverse view's row 3 (the eye's position) or row 2 (its direction)
        CopyMatrix(a, RendererMatrix(At<uint8_t*>(self, 0x3C), ni::dx9::kInvView));
        const float* row = mapping == kEyePos ? a + 12 : a + 8;
        v[0] = row[0];
        v[1] = row[1];
        v[2] = row[2];
        v[3] = row[3];
        data = v;
        break;
    }
    case kTaylorSin:
    case kTaylorCos:   // no data: the program takes the entry's own (the defaults SetupPredefinedEntry
                       // gave it)
        data = nullptr;
        break;
    case kTime:
    case kSinTime:
    case kCosTime:
    case kTanTime:
    case kTimeSinCosTan: {   // the geometry's time float extra data (+0xC), its sine, cosine, tangent
        re::Ptr32<char> name;
        FixedString_Construct(&name, ms_pcTimeExtraData);
        uint8_t* extra = ObjectNET_GetExtraData(geometry, &name);
        FixedString_Destruct(&name);
        if (extra == nullptr) {
            result = 0x10;
        }
        // (without the extra data the game reads the time through the null all the same: it faults
        // there, as this read does)
        float time = *reinterpret_cast<const volatile float*>(reinterpret_cast<uintptr_t>(extra) + 0xC);
        switch (mapping) {
        case kTime:
            v[0] = time;
            v[1] = time;
            v[2] = time;
            v[3] = time;
            break;
        case kSinTime: {
            float s = crt::Sinf(time);
            v[0] = s;
            v[1] = s;
            v[2] = s;
            v[3] = s;
            break;
        }
        case kCosTime: {
            float s = crt::Cosf(time);
            v[0] = s;
            v[1] = s;
            v[2] = s;
            v[3] = s;
            break;
        }
        case kTanTime: {
            float s = crt::Tanf(time);
            v[0] = s;
            v[1] = s;
            v[2] = s;
            v[3] = s;
            break;
        }
        case kTimeSinCosTan:
            v[0] = time;
            v[1] = crt::Sinf(time);
            v[2] = crt::Cosf(time);
            v[3] = crt::Tanf(time);
            break;
        default:   // (unreachable: the game's inner switch has it)
            return 0x10;
        }
        data = v;
        break;
    }
    case kAmbientLight:   // the ambient lights' colours (+0xFC) times their dimmers (+0xF8) summed (on
                          // or not), alpha 1
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        v[3] = 1.0f;
        if (effects != nullptr) {
            for (re::Ptr32<void>* node = At<re::Ptr32<void>*>(effects, 0xC); node != nullptr;) {
                void* light = node[1];
                node = static_cast<re::Ptr32<void>*>(node[0]);
                uint8_t* ambient = DynamicCast(NiAmbientLight_RTTI, light);
                if (ambient != nullptr) {
                    float color[3];
                    Color_Times(reinterpret_cast<const float*>(ambient + 0xFC), color, At<float>(ambient, 0xF8));
                    v[0] = color[0] + v[0];
                    v[1] = color[1] + v[1];
                    v[2] = color[2] + v[2];
                }
            }
        }
        data = v;
        break;
    case kFogDensity: {   // 1 / ((+0x7C - +0x78) * the fog's depth (+0x1C)), four times
        uint8_t* renderState = At<uint8_t*>(self, 0x40);
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        float density = 1.0f / ((At<float>(renderState, 0x7C) - At<float>(renderState, 0x78)) * At<float>(fog, 0x1C));
        v[3] = density;
        v[2] = density;
        v[1] = density;
        v[0] = density;
        data = v;
        break;
    }
    case kFogNearFar: {   // the fog's start (+0x7C less the depth) and end (+0x84 times the depth, plus
                          // +0x7C), 0, 0 (as NiDX9RenderState::ApplyFog's, with +0x7C - +0x78 for
                          // its depth scale)
        uint8_t* renderState = At<uint8_t*>(self, 0x40);
        uint8_t* fog = At<uint8_t*>(state, 0x10);
        float start = At<float>(renderState, 0x7C);
        float range = start - At<float>(renderState, 0x78);
        float depth = range * At<float>(fog, 0x1C);
        float endScale = At<float>(renderState, 0x84);
        v[0] = start - depth;
        v[1] = depth * endScale + start;
        v[2] = 0.0f;
        v[3] = 0.0f;
        data = v;
        break;
    }
    case kFogColor:   // the fog's colour (+0x20) as it lies: four floats from there
        data = At<uint8_t*>(state, 0x10) + 0x20;
        break;
    case kBumpMatrix: {   // the bump map's matrix (+0x18..+0x24); 1s without one
        v[0] = 1.0f;
        v[1] = 1.0f;
        v[2] = 1.0f;
        v[3] = 1.0f;
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint8_t* map = Map(texturing, kBumpMap);
            if (map != nullptr) {
                v[0] = At<float>(map, 0x18);
                v[1] = At<float>(map, 0x1C);
                v[2] = At<float>(map, 0x20);
                v[3] = At<float>(map, 0x24);
            }
        }
        data = v;
        break;
    }
    case kBumpLumaAndScale: {   // the bump map's luma offset (+0x14) and scale (+0x10); 0, 1 without one
        v[0] = 0.0f;
        v[1] = 1.0f;
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint8_t* map = Map(texturing, kBumpMap);
            if (map != nullptr) {
                v[0] = At<float>(map, 0x14);
                v[1] = At<float>(map, 0x10);
            }
        }
        data = v;   // (v[2], v[3]: uninitialised in the game)
        break;
    }
    case kParallaxOffset: {   // the parallax map's offset (+0x10); 0 without one
        v[0] = 0.0f;
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing != nullptr) {
            uint8_t* map = Map(texturing, kParallaxMap);
            if (map != nullptr) {
                v[0] = At<float>(map, 0x10);
            }
        }
        data = v;   // (v[1..3]: uninitialised in the game)
        break;
    }
    case kTexSizeBase:
    case kTexSizeDark:
    case kTexSizeDetail:
    case kTexSizeGloss:
    case kTexSizeGlow:
    case kTexSizeBump:
    case kTexSizeNormal:
    case kTexSizeParallax:
    case kTexSizeDecal:
    case kTexSizeShader: {   // the map's texture's width and height; 0s without them
        uint8_t* texturing = At<uint8_t*>(state, 0x28);
        if (texturing == nullptr) {
            return result;
        }
        uint8_t* map;
        if (mapping == kTexSizeShader) {
            map = TexturingProperty_GetShaderMap(texturing, entry->extra);
        } else if (mapping == kTexSizeDecal) {
            map = Map(texturing, kDecalMaps + entry->extra);
        } else {
            map = Map(texturing, mapping - kTexSizeBase);
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        if (map != nullptr) {
            void* texture = At<void*>(map, 8);
            if (texture != nullptr) {
                v[0] = UnsignedToFloat(Virtual<uint32_t>(texture, 18));   // GetWidth
                v[1] = UnsignedToFloat(Virtual<uint32_t>(texture, 19));   // GetHeight
            }
        }
        data = v;   // (v[2], v[3]: uninitialised in the game)
        break;
    }
    case kAlphaTestFunction: {   // the alpha property's test function (+0x18 bits 10..12) as flags:
                                 // (1, 0, 1) less, (1, 1, 0) equal, (1, 0, 0) less or equal,
                                 // (0, 1, 1) greater, (0, 0, 1) not equal, (0, 1, 0) greater or
                                 // equal, (1, 1, 1) never; NiPoint3::ZERO for always or no property
        v[0] = NiPoint3_ZERO[0];
        v[1] = NiPoint3_ZERO[1];
        v[2] = NiPoint3_ZERO[2];
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
        data = v;   // (v[3]: uninitialised in the game)
        break;
    }
    case kAlphaTestRef: {   // the alpha property's test reference (+0x1A) / 255; 0 without one
        v[0] = 0.0f;
        uint8_t* alpha = At<uint8_t*>(state, 8);
        if (alpha != nullptr) {
            v[0] = float(int(At<uint8_t>(alpha, 0x1A))) / 255.0f;
        }
        data = v;   // (v[1..3]: uninitialised in the game)
        break;
    }
    default:   // the light mappings (0x45..0x48: object constants now) and unknown ids: nothing set
        return result;
    }
    if (Virtual<uint8_t>(program, 14, entry, data, 0u) == 0) {
        result = 0x50;
    }
    return result;
}

#define C88_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(&fn), nullptr, nullptr, re::Mode::Original)

// (called only through the vtable: no callers keeping registers across it)
C88_REVERSED("NiD3DShaderConstantMap::SetDefinedConstant", 0x0088C6B0, NiD3DShaderConstantMap_SetDefinedConstant);

} // namespace ni::c88
