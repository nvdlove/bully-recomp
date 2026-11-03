// 0x40D9C0 to 0x40E8C0: small math (vector difference and length, a bound against a
// plane, a random range) and the light manager's pieces: light flicker set-up, NiLight
// fields from the loader's light data, the shadow frustums and which lights and shadows
// affect a node, and small array helpers (NiTArray<NiPointer<...>>, growable arrays).

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

inline int __cdecl rand_() { return crt::Rand(); }
GAME_FN(uint8_t*, __thiscall, Loader_BuildLight, 0x0040CFB0, void* loader, int32_t index);
GAME_FN(uint8_t, __thiscall, FixedString_Contains, 0x00406510, const re::Ptr32<char>* self, const char* s);
GAME_FN(int32_t, __cdecl, GetTimeMs_45BCA0, 0x0045BCA0);
GAME_FN(uint32_t, __cdecl, GetMaterialIndex, 0x0040DD70, uint32_t kind);
GAME_FN(uint8_t, __cdecl, Fn_430320, 0x00430320, int area);
GAME_VAR(int32_t, ms_currArea, 0x00BD1008);
GAME_FN(void*, __cdecl, CMemoryMgr_Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, CMemoryMgr_Free, 0x005EEAB0, void* p);
inline void* __cdecl memset_(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
inline char* __cdecl strncpy_(char* dst, const char* src, size_t count) { return crt::Strncpy(dst, src, count); }
GAME_FN(void, __thiscall, AVObject_Update, 0x0075DB00, void* object, float time, int updateControllers);
GAME_FN(void, __thiscall, NiFrustum_Construct, 0x007718A0, void* self, float left, float right, float top, float bottom,
        float nearPlane, float farPlane, int ortho);
GAME_FN(void, __thiscall, ShadowCamera_SetFrustum, 0x007710D0, void* self, const void* frustum, const void* transform);
GAME_FN(void, __thiscall, NiDynamicEffect_AttachAffectedNode, 0x00771A10, void* self, void* node);
GAME_FN(void*, __cdecl, Fn_7571E0, 0x007571E0, void* a);
GAME_FN(void, __thiscall, Fn_7735F0, 0x007735F0, void* self);
GAME_FN(void, __thiscall, GrowArray16, 0x0040DE70, void* self, uint32_t capacity, int setCount);

GAME_VAR(int32_t, ms_numShadows, 0x00BCC128);
GAME_VAR(int32_t, ms_numLights, 0x00BCC12C);
GAME_VAR(uint8_t, ms_bTwoShadowsOnly, 0x00BCC130);
GAME_VAR(uint32_t, ms_NiNodeBit, 0x020E2550);
GAME_VAR(uint32_t*, ms_sqrtTable, 0x020E2BFC);   // NiFastSqrt's mantissa table
GAME_VAR(uint8_t*, ms_pLights, 0x00BCBBB4);

uint32_t& U(void* p, int offset) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + offset);
}
float& F(void* p, int offset) {
    return *reinterpret_cast<float*>(static_cast<uint8_t*>(p) + offset);
}

// x*x + y*y + z*z the way the game sums it (x87: x, y, then z), rounded to float
float SquaredLength(float x, float y, float z) {
    return float((double(x) * x + double(y) * y) + double(z) * z);
}

// degrees to radians and the cosine, as the game rounds them
float CosDegrees(float degrees) {
    float radians = float(double(degrees) * double(3.14159274f) / 180.0);
    return float(std::cos(double(radians)));
}

// a sphere (+0x1C centre, +0x28 radius) in front of all six planes (+8, 0x10 each) of a
// frustum
bool InFrustum(const uint8_t* planes, const uint8_t* bound) {
    float negRadius = -F(const_cast<uint8_t*>(bound), 0x28);
    for (int i = 0; i < 6; i++) {
        const float* p = reinterpret_cast<const float*>(planes + 8 + i * 0x10);
        const float* c = reinterpret_cast<const float*>(bound + 0x1C);
        float d = float(double(p[0]) * c[0] + double(p[1]) * c[1] + double(p[2]) * c[2]);
        d = d - p[3];
        if (d < negRadius || d == negRadius) {
            return false;
        }
    }
    return true;
}

// the NiLight fields every light has (colours, attenuation, transform, scale 1)
void SetLightCommon(uint8_t* light, const uint8_t* data, bool spot) {
    const uint32_t* d = reinterpret_cast<const uint32_t*>(data);
    U(light, 0xFC) = d[1];
    U(light, 0x100) = d[2];
    U(light, 0xD0)++;
    int32_t revision = int32_t(U(light, 0xD0));
    U(light, 0x104) = d[3];
    U(light, 0x108) = d[4];
    U(light, 0x10C) = d[5];
    U(light, 0x110) = d[6];
    U(light, 0xD0) = uint32_t(++revision);
    U(light, 0x114) = d[7];
    U(light, 0x118) = d[8];
    U(light, 0x11C) = d[9];
    U(light, 0xD0) = uint32_t(++revision);
    if (spot) {
        float outer = F(const_cast<uint8_t*>(data), 0x6C);
        F(light, 0x13C) = outer;
        F(light, 0x148) = CosDegrees(outer);
        U(light, 0xD0) = uint32_t(++revision);
        float inner = F(const_cast<uint8_t*>(data), 0x70);
        F(light, 0x140) = inner;
        F(light, 0x14C) = CosDegrees(inner);
        U(light, 0xD0) = uint32_t(++revision);
        F(light, 0x144) = F(const_cast<uint8_t*>(data), 0x74);
        U(light, 0xD0) = uint32_t(++revision);
    }
    F(light, 0x124) = F(const_cast<uint8_t*>(data), 0x5C);
    U(light, 0xD0) = uint32_t(++revision);
    F(light, 0x128) = F(const_cast<uint8_t*>(data), 0x60);
    U(light, 0xD0) = uint32_t(++revision);
    F(light, 0x12C) = F(const_cast<uint8_t*>(data), 0x64);
    U(light, 0xD0) = uint32_t(++revision);
    for (int i = 0; i < 13; i++) {
        U(light, 0x2C + i * 4) = d[0x28 / 4 + i];
    }
    F(light, 0x5C) = 1.0f;
}

} // namespace

namespace LightManagerParts {

// 0x40D9C0 (thiscall): out = this - other
static float* __fastcall Subtract(const float* self, re::Edx, float* out, const float* other) {
    out[0] = self[0] - other[0];
    out[1] = self[1] - other[1];
    out[2] = self[2] - other[2];
    return out;
}

// 0x40D9F0 (thiscall): the squared length
static float __fastcall SquaredLength_(const float* self, re::Edx) {
    return SquaredLength(self[0], self[1], self[2]);
}

// 0x40DA20 (thiscall): which side of `plane` (normal, constant) a sphere (centre,
// radius) is: 2 behind, 1 in front, 0 across
static uint32_t __fastcall WhichSide(const float* self, re::Edx, const float* plane) {
    float d = float(double(plane[1]) * self[1] + double(plane[0]) * self[0] + double(plane[2]) * self[2]);
    d = d - plane[3];
    if (!(-self[3] < d)) {
        return 2;
    }
    if (self[3] < d || self[3] == d) {
        return 1;
    }
    return 0;
}

// 0x40DA80: a random integer from min to max - 1 (rand's scale)
static int32_t __cdecl RandomRange(int32_t min, int32_t max) {
    return rand_() * (max - min - 1) / 0x7FFF + min;
}

// 0x40DAB0, 0x40DAE0, 0x40DB10 (thiscall): an NiLight's ambient, diffuse or specular
// colour set (its revision bumped)
static void __fastcall SetAmbient(uint8_t* self, re::Edx, const uint32_t* colour) {
    U(self, 0xFC) = colour[0];
    U(self, 0x100) = colour[1];
    U(self, 0xD0)++;
    U(self, 0x104) = colour[2];
}
static void __fastcall SetDiffuse(uint8_t* self, re::Edx, const uint32_t* colour) {
    U(self, 0x108) = colour[0];
    U(self, 0x10C) = colour[1];
    U(self, 0xD0)++;
    U(self, 0x110) = colour[2];
}
static void __fastcall SetSpecular(uint8_t* self, re::Edx, const uint32_t* colour) {
    U(self, 0x114) = colour[0];
    U(self, 0x118) = colour[1];
    U(self, 0xD0)++;
    U(self, 0x11C) = colour[2];
}

// 0x40DB40 (stdcall): a flickering light's state (0x24) for the loader's light `index`:
// its flags from the name (cs_, co_, no _N_, flicker), the flicker rate from its spot
// exponent (whole tenths, and the rest), its scale kept and set to 1
static uint8_t __stdcall InitFlicker(uint8_t* state, void* loader, int32_t index) {
    for (int i = 0; i < 9; i++) {
        U(state, i * 4) = 0;
    }
    uint8_t* light = Loader_BuildLight(loader, index);
    U(state, 0x1C) = re::Addr32(light);
    if (!light) {
        return 0;
    }
    *reinterpret_cast<uint16_t*>(light + 0x7A) = 0x400;
    const re::Ptr32<char>* name = reinterpret_cast<const re::Ptr32<char>*>(light + 0x84);
    state[0x11] = FixedString_Contains(name, "cs_") != 0;
    state[0x18] = FixedString_Contains(name, "co_") != 0;
    state[0x19] = FixedString_Contains(name, "_N_") == 0;
    double exponent = F(light, 0x74);
    int32_t whole = int32_t(exponent * 10.0 + 0.5);   // (_ftol2: truncating)
    float wholeF = float(whole);
    int32_t rest = int32_t(0.5 + (exponent * 1000.0 - 100.0 * wholeF));
    F(state, 4) = float(wholeF * double(0.1f));
    F(state, 8) = float(double(0.1f) * rest);
    uint8_t flicker = FixedString_Contains(name, "flicker");
    U(state, 0x14) = 0;
    if (flicker) {
        state[0x12] = 1;
        state[0x13] = 1;
    } else {
        state[0x12] = 0;
    }
    F(state, 0) = F(light, 0x58);
    F(light, 0x58) = 1.0f;
    return 1;
}

// 0x40DC90 (thiscall): the next flicker time: now + (30..299, or 200..1998 when +0x13 is
// set) / (1..4)
static void __fastcall NextFlickerTime(uint8_t* self, re::Edx) {
    int32_t base;
    if (self[0x13]) {
        base = rand_() * 0x707 / 0x7FFF + 200;
    } else {
        base = rand_() * 0x10D / 0x7FFF + 30;
    }
    int32_t divisor = rand_() * 4 / 0x7FFF + 1;
    int32_t now = GetTimeMs_45BCA0();
    *reinterpret_cast<int32_t*>(self + 0x14) = now + base / divisor;
}

// 0x40DD20 (stdcall): out = a * s + b * t (two vectors (a: +4.., b: +0x14..), weights)
static float* __stdcall Blend(float* out, float a0, float a1, float a2, float s, float b0, float b1, float b2, float t) {
    out[0] = float(double(a0) * s + double(b0) * t);
    out[1] = float(double(a1) * s + double(b1) * t);
    out[2] = float(double(s) * a2 + double(b2) * t);
    return out;
}

// 0x40DD70: a material kind's index (2, 5: 1; 3, 6: 0; 4, 7: 2; others 3)
static uint32_t __cdecl GetMaterialIndex_(uint32_t kind) {
    switch (kind) {
    case 2:
    case 5:
        return 1;
    case 3:
    case 6:
        return 0;
    case 4:
    case 7:
        return 2;
    default:
        return 3;
    }
}

// the material value (+0x54) from the table at +0xC (+0x20: six floats) by +0x4C's kind
static void UpdateMaterialValue(uint8_t* self) {
    uint8_t* table = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xC);
    uint8_t* info = *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x4C);
    if (!table || !info || !info[0xC5]) {
        return;
    }
    uint32_t index = GetMaterialIndex(info[0xC5]);
    if (!((self[8] >> 4) & 1)) {
        index += 3;
    }
    F(self, 0x54) = F(table, 0x20 + int(index) * 4);
}

// 0x40DDB0 (thiscall)
static void __fastcall UpdateMaterialValue_(uint8_t* self, re::Edx) {
    UpdateMaterialValue(self);
}

// 0x40DDF0, 0x40DE30 (thiscall): a spot light's outer / inner angle (degrees; its cosine
// kept too)
static void __fastcall SetOuterAngle(uint8_t* self, re::Edx, float degrees) {
    F(self, 0x13C) = degrees;
    float c = CosDegrees(degrees);
    U(self, 0xD0)++;
    F(self, 0x148) = c;
}
static void __fastcall SetInnerAngle(uint8_t* self, re::Edx, float degrees) {
    F(self, 0x140) = degrees;
    float c = CosDegrees(degrees);
    U(self, 0xD0)++;
    F(self, 0x14C) = c;
}

// 0x40DE70, 0x40DF00 (thiscall): a growable array (+0 items, +4 capacity, +8 count)
// reallocated (16 or 0x24 byte items); the count set to the capacity when asked
template <int Size>
static void Grow(uint8_t* self, uint32_t capacity, int setCount) {
    U(self, 4) = capacity;
    uint8_t* items = nullptr;
    if (capacity) {
        uint64_t bytes = uint64_t(capacity) * Size;
        items = static_cast<uint8_t*>(CMemoryMgr_Malloc((bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes)));
    }
    uint8_t* old = *reinterpret_cast<re::Ptr32<uint8_t>*>(self);
    if (old) {
        for (uint32_t i = 0; i < U(self, 8); i++) {
            for (int j = 0; j < Size / 4; j++) {
                reinterpret_cast<uint32_t*>(items + i * Size)[j] = reinterpret_cast<uint32_t*>(old + i * Size)[j];
            }
        }
        CMemoryMgr_Free(old);
    }
    *reinterpret_cast<re::Ptr32<uint8_t>*>(self) = items;
    if (re::Flag(setCount)) {
        U(self, 8) = U(self, 4);
    }
}
static void __fastcall GrowArray16_(uint8_t* self, re::Edx, uint32_t capacity, int setCount) {
    Grow<0x10>(self, capacity, setCount);
}
static void __fastcall GrowArray24(uint8_t* self, re::Edx, uint32_t capacity, int setCount) {
    Grow<0x24>(self, capacity, setCount);
}

// 0x40DF80: 0x430320 on the current area (its result returned)
static uint8_t __cdecl UpdateCurrentArea() {
    return Fn_430320(ms_currArea);
}

// 0x40DF90: sqrt(|x|) as a float
static float __cdecl SqrtAbs(float x) {
    float a = std::fabs(x);
    return float(std::sqrt(double(a)));
}

// 0x40DFB0 (thiscall): the length
static float __fastcall Length(const float* self, re::Edx) {
    float s = std::fabs(SquaredLength(self[0], self[1], self[2]));
    return float(std::sqrt(double(s)));
}

// 0x40DFF0 (thiscall): a light record (0xE0) from a flicker state: its first 7 dwords, the
// name (63 characters), the light data's 0x84 bytes
static uint8_t* __fastcall LightRecord_Construct(uint8_t* self, re::Edx, const uint8_t* state) {
    for (int i = 1; i <= 9; i++) {
        F(self, 0x5C + i * 4) = 0.0f;
    }
    memset_(self, 0, 0xE0);
    for (int i = 0; i < 7; i++) {
        U(self, i * 4) = reinterpret_cast<const uint32_t*>(state)[i];
    }
    const uint8_t* light = *reinterpret_cast<const re::Ptr32<uint8_t>*>(state + 0x1C);
    strncpy_(reinterpret_cast<char*>(self + 0x1C), *reinterpret_cast<const re::Ptr32<char>*>(light + 0x84), 0x3F);
    light = *reinterpret_cast<const re::Ptr32<uint8_t>*>(state + 0x1C);
    for (int i = 0; i < 0x21; i++) {
        U(self, 0x5C + i * 4) = reinterpret_cast<const uint32_t*>(light)[i];
    }
    return self;
}

// 0x40E070 (thiscall): each light's bound (+0x1C.. its world translation, +0x28 the
// radius: 20 / the linear attenuation, or 10000 without attenuation; kept from the one
// before when only quadratic, uninitialised for the first: the game's)
static void __fastcall UpdateLightBounds(uint8_t* self, re::Edx) {
    float radius = 0.0f;
    for (int32_t i = 0; i < ms_numLights; i++) {
        uint8_t* light = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x24))[i];
        float linear = F(light, 0x128);
        if (linear == 0.0f && F(light, 0x12C) == 0.0f) {
            radius = 10000.0f;
        } else if (linear != 0.0f) {
            radius = float(20.0 / linear);
        }
        U(light, 0x1C) = U(light, 0x50);
        U(light, 0x20) = U(light, 0x54);
        U(light, 0x24) = U(light, 0x58);
        F(light, 0x28) = radius;
    }
}

// 0x40E120 (thiscall): each shadow's camera updated and its frustum set from the
// light's field of view (+0x13C) and range (+0x58 near, +0x5C far, at most 350; 0.1 and
// 350 without light data)
static void __fastcall UpdateShadowFrustums(uint8_t* self, re::Edx) {
    for (int32_t i = 0; i < ms_numShadows; i++) {
        uint8_t* shadow = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x14))[i];
        uint8_t* camera = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow + 4);
        AVObject_Update(camera, 0.0f, 1);
        float angle = float(double(F(camera, 0x13C)) * double(3.14159274f) / 180.0);
        float t = float(std::tan(double(angle)));
        uint8_t* data = *reinterpret_cast<re::Ptr32<uint8_t>*>(shadow);
        float nearPlane;
        float farPlane;
        if (data) {
            nearPlane = F(data, 0x58);
            farPlane = F(data, 0x5C);
            if (350.0f < farPlane) {
                farPlane = 350.0f;
            }
        } else {
            farPlane = 350.0f;
            nearPlane = 0.1f;
        }
        uint8_t frustum[0x1C];
        NiFrustum_Construct(frustum, -t, t, t, -t, nearPlane, farPlane, 0);
        ShadowCamera_SetFrustum(shadow + 8, frustum, camera + 0x2C);
        U(shadow, 0x68) = 0x3F;
    }
}

// 0x40E230 (thiscall): a bound inside any shadow frustum
static uint8_t __fastcall IsInShadowFrustum(uint8_t* self, re::Edx, const uint8_t* bound) {
    if (!bound) {
        return 0;
    }
    for (int32_t i = 0; i < ms_numShadows; i++) {
        uint8_t* planes = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x14))[i];
        if (InFrustum(planes, bound)) {
            return 1;
        }
    }
    return 0;
}

// 0x40E2D0 (stdcall): a spot light set from the loader's light data (the light passed as
// an NiPointer by value: released)
static void __stdcall SetSpotLight(uint8_t* light, const uint8_t* data) {
    SetLightCommon(light, data, true);
    if (vc::Decrement(light + 4) == 0) {
        vc::Call<void>(light, 0x04);
    }
}

// 0x40E450 (stdcall): a point light likewise
static void __stdcall SetPointLight(uint8_t* light, const uint8_t* data) {
    SetLightCommon(light, data, false);
    if (vc::Decrement(light + 4) == 0) {
        vc::Call<void>(light, 0x04);
    }
}

// 0x40E540 (thiscall): the child nodes of `node` attached to the shadows whose frustum
// holds them (the first two only, with 0xBCC130) and to the lights they're in range of
// (distance by NiFastSqrt)
static void __fastcall AttachAffectedNodes(uint8_t* self, re::Edx, uint8_t* node) {
    for (uint32_t c = 0; c < *reinterpret_cast<uint16_t*>(node + 0xCE); c++) {
        uint8_t* child = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(node + 0xC8))[c];
        if (!child) {
            continue;
        }
        uint8_t* rtti = vc::Call<uint8_t*>(child, 0x08);
        if (!(*reinterpret_cast<uint32_t*>(rtti + 0xC + (ms_NiNodeBit >> 5) * 4) & (1u << (ms_NiNodeBit & 0x1F)))) {
            continue;
        }
        for (int32_t i = 0; i < ms_numShadows; i++) {
            if (ms_bTwoShadowsOnly && i >= 2) {
                break;
            }
            uint8_t* shadow = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x14))[i];
            if (InFrustum(shadow, child)) {
                NiDynamicEffect_AttachAffectedNode(*reinterpret_cast<re::Ptr32<void>*>(shadow + 4), child);
            }
        }
        for (int32_t i = 0; i < ms_numLights; i++) {
            uint8_t* light = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 0x24))[i];
            float dx = F(light, 0x1C) - F(child, 0x50);
            float dy = F(light, 0x20) - F(child, 0x54);
            float dz = F(light, 0x24) - F(child, 0x58);
            float sq = SquaredLength(dx, dy, dz);
            uint32_t bits;
            std::memcpy(&bits, &sq, 4);
            float distance;
            if (bits == 0) {
                distance = 0.0f;
            } else {
                int16_t exponent = int16_t(uint16_t((bits >> 23) - 0x7F));
                uint32_t mantissa = bits & 0x7FFFFF;
                if (exponent & 1) {
                    mantissa |= 0x800000;
                }
                uint32_t result = uint32_t((int32_t(int16_t(exponent >> 1)) + 0x7F) << 23) | ms_sqrtTable[mantissa >> 16];
                std::memcpy(&distance, &result, 4);
            }
            if (distance < F(child, 0x28) + F(light, 0x28)) {
                NiDynamicEffect_AttachAffectedNode(light, child);
            }
        }
    }
}

// 0x40E740 (thiscall): +0xC from 0x7571E0, the material value updated when asked, then
// 0x7735F0
static void __fastcall SetMaterialTable(uint8_t* self, re::Edx, void* source, int update) {
    uint8_t* table = static_cast<uint8_t*>(Fn_7571E0(source));
    *reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0xC) = table;
    if (re::Flag(update) && table) {
        UpdateMaterialValue(self);
    }
    Fn_7735F0(self);
}

// 0x40E7A0 (thiscall): NiTArray<NiPointer<...>>::RemoveAll
static void __fastcall PointerArray_RemoveAll(uint8_t* self, re::Edx) {
    uint16_t& size = *reinterpret_cast<uint16_t*>(self + 0xA);
    for (uint16_t i = 0; i < size; i++) {
        re::Ptr32<uint8_t>& item = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4))[i];
        if (item) {
            if (vc::Decrement(item + 4) == 0) {
                vc::Call<void>(item, 0x04);
            }
            item = nullptr;
        }
    }
    *reinterpret_cast<uint16_t*>(self + 0xC) = 0;
    size = 0;
}

// 0x40E800 (thiscall): NiTArray<NiPointer<...>>::SetAt (the used size and count kept)
static void __fastcall PointerArray_SetAt(uint8_t* self, re::Edx, uint32_t index, const re::Ptr32<uint8_t>* item) {
    uint16_t& size = *reinterpret_cast<uint16_t*>(self + 0xA);
    uint16_t& count = *reinterpret_cast<uint16_t*>(self + 0xC);
    re::Ptr32<uint8_t>* items = *reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4);
    if (index >= size) {
        size = uint16_t(index + 1);
        if (*item) {
            count++;
        }
    } else if (*item) {
        if (!items[index]) {
            count++;
        }
    } else if (items[index]) {
        count--;
    }
    re::Ptr32<uint8_t>& slot = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t>>*>(self + 4))[index];
    if (slot == *item) {
        return;
    }
    if (slot && vc::Decrement(slot + 4) == 0) {
        vc::Call<void>(slot, 0x04);
    }
    slot = *item;
    if (slot) {
        vc::Increment(slot + 4);
    }
}

// 0x40E8A0: a new reference to the Lights node (0xBCBBB4)
static re::Ptr32<uint8_t>* __cdecl GetLightsNode(re::Ptr32<uint8_t>* out) {
    uint8_t* lights = ms_pLights;
    *out = lights;
    if (lights) {
        vc::Increment(lights + 4);
    }
    return out;
}

// 0x40E8C0 (thiscall): a 16 byte item added to the array at +8 (doubling)
static void __fastcall AddItem16(uint8_t* self, re::Edx, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    uint32_t capacity = U(self, 0xC);
    if (U(self, 0x10) == capacity) {
        GrowArray16(self + 8, capacity ? capacity * 2 : 1, 0);
    }
    uint32_t* item = reinterpret_cast<uint32_t*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(self + 8) + U(self, 0x10) * 0x10);
    item[0] = a;
    item[1] = b;
    item[2] = c;
    item[3] = d;
    U(self, 0x10)++;
}

} // namespace LightManagerParts

// ---- registration ----------------------------------------------------------------------

#define C00_LIGHTS_REVERSED(name, fn, address)                                                      \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace LightManagerParts {
C00_LIGHTS_REVERSED("NiPoint3::Subtract", Subtract, 0x0040D9C0);
C00_LIGHTS_REVERSED("NiPoint3::SqrLength", SquaredLength_, 0x0040D9F0);
C00_LIGHTS_REVERSED("NiBound::WhichSide", WhichSide, 0x0040DA20);
C00_LIGHTS_REVERSED("RandomRange", RandomRange, 0x0040DA80);
C00_LIGHTS_REVERSED("NiLight::SetAmbient", SetAmbient, 0x0040DAB0);
C00_LIGHTS_REVERSED("NiLight::SetDiffuse", SetDiffuse, 0x0040DAE0);
C00_LIGHTS_REVERSED("NiLight::SetSpecular", SetSpecular, 0x0040DB10);
C00_LIGHTS_REVERSED("LightFlicker::Init", InitFlicker, 0x0040DB40);
C00_LIGHTS_REVERSED("LightFlicker::NextTime", NextFlickerTime, 0x0040DC90);
C00_LIGHTS_REVERSED("Blend", Blend, 0x0040DD20);
C00_LIGHTS_REVERSED("GetMaterialIndex", GetMaterialIndex_, 0x0040DD70);
C00_LIGHTS_REVERSED("UpdateMaterialValue", UpdateMaterialValue_, 0x0040DDB0);
C00_LIGHTS_REVERSED("NiSpotLight::SetOuterAngle", SetOuterAngle, 0x0040DDF0);
C00_LIGHTS_REVERSED("NiSpotLight::SetInnerAngle", SetInnerAngle, 0x0040DE30);
C00_LIGHTS_REVERSED("Array16::Grow", GrowArray16_, 0x0040DE70);
C00_LIGHTS_REVERSED("Array24::Grow", GrowArray24, 0x0040DF00);
C00_LIGHTS_REVERSED("UpdateCurrentArea", UpdateCurrentArea, 0x0040DF80);
C00_LIGHTS_REVERSED("SqrtAbs", SqrtAbs, 0x0040DF90);
C00_LIGHTS_REVERSED("NiPoint3::Length", Length, 0x0040DFB0);
C00_LIGHTS_REVERSED("LightRecord::Construct", LightRecord_Construct, 0x0040DFF0);
C00_LIGHTS_REVERSED("LightManager::UpdateLightBounds", UpdateLightBounds, 0x0040E070);
C00_LIGHTS_REVERSED("LightManager::UpdateShadowFrustums", UpdateShadowFrustums, 0x0040E120);
C00_LIGHTS_REVERSED("LightManager::IsInShadowFrustum", IsInShadowFrustum, 0x0040E230);
C00_LIGHTS_REVERSED("LightManager::SetSpotLight", SetSpotLight, 0x0040E2D0);
C00_LIGHTS_REVERSED("LightManager::SetPointLight", SetPointLight, 0x0040E450);
C00_LIGHTS_REVERSED("LightManager::AttachAffectedNodes", AttachAffectedNodes, 0x0040E540);
C00_LIGHTS_REVERSED("SetMaterialTable", SetMaterialTable, 0x0040E740);
C00_LIGHTS_REVERSED("NiTArray<NiPointer>::RemoveAll", PointerArray_RemoveAll, 0x0040E7A0);
C00_LIGHTS_REVERSED("NiTArray<NiPointer>::SetAt", PointerArray_SetAt, 0x0040E800);
C00_LIGHTS_REVERSED("LightManager::GetLightsNode", GetLightsNode, 0x0040E8A0);
C00_LIGHTS_REVERSED("LightManager::AddItem16", AddItem16, 0x0040E8C0);
} // namespace LightManagerParts
