// 0x518E60 to 0x5191C0: the RenderWare emulation's enum tables (RW alpha functions,
// blends and filters to D3D's and Gamebryo's; the value in eax, so not hooked), the
// raster destroy, and the small Gamebryo pieces next to them: NiAlphaProperty's
// blending flag, NiBinaryExtraData's destructor, NiFogProperty's constructor and
// destructor, and NiShadeProperty's RTTI and type.

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_ARRAY(uint32_t, ms_defaultFogColour, 0x020E25E8);   // NiFogProperty's default (r, g, b)

GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __thiscall, NiProperty_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, NiProperty_Destruct, 0x00764590, void* self);
GAME_FN(void, __thiscall, NiExtraData_Destruct, 0x00765670, void* self);

constexpr uint32_t kBinaryExtraDataVtable = 0x009179A4;
constexpr uint32_t kFogPropertyVtable = 0x009179FC;

using re::At;

} // namespace

namespace RwEnumTables {

// 0x518E60 (value in eax): RW alpha test function (1..8) -> D3DCMPFUNC (the same
// numbers; 8 for anything else)
[[maybe_unused]] static uint32_t AlphaFunctionFromRw(uint32_t value) {
    return (value >= 1 && value <= 8) ? value : 8;
}
RE_UNHOOKED_FN("RwEmu::AlphaFunctionFromRw", 0x00518E60, &AlphaFunctionFromRw);

// 0x518EC0 (value in eax): RW blend (1..11) -> D3DBLEND (the same numbers; 3 for
// anything else)
[[maybe_unused]] static uint32_t BlendFromRw(uint32_t value) {
    return (value >= 1 && value <= 11) ? value : 3;
}
RE_UNHOOKED_FN("RwEmu::BlendFromRw", 0x00518EC0, &BlendFromRw);

// 0x518F40 (value in eax): D3DBLEND -> NiAlphaProperty's AlphaFunction (0 for 2 and
// anything unknown)
[[maybe_unused]] static uint32_t NiAlphaBlend(uint32_t value) {
    static const uint32_t kTable[11] = {1, 0, 2, 3, 6, 7, 8, 9, 4, 5, 10};
    return (value >= 1 && value <= 11) ? kTable[value - 1] : 0;
}
RE_UNHOOKED_FN("RwEmu::NiAlphaBlend", 0x00518F40, &NiAlphaBlend);

// 0x518FC0 (value in eax): D3DCMPFUNC -> NiAlphaProperty's TestFunction (0 for 8 and
// anything unknown)
[[maybe_unused]] static uint32_t NiAlphaTestFunction(uint32_t value) {
    static const uint32_t kTable[8] = {7, 1, 2, 3, 4, 5, 6, 0};
    return (value >= 1 && value <= 8) ? kTable[value - 1] : 0;
}
RE_UNHOOKED_FN("RwEmu::NiAlphaTestFunction", 0x00518FC0, &NiAlphaTestFunction);

// 0x519020 (value in eax): RW filter -> NiTexturingProperty's FilterMode (1 for 2
// and anything unknown)
[[maybe_unused]] static uint32_t NiFilterMode(uint32_t value) {
    static const uint32_t kTable[6] = {0, 1, 3, 4, 5, 2};
    return (value >= 1 && value <= 6) ? kTable[value - 1] : 1;
}
RE_UNHOOKED_FN("RwEmu::NiFilterMode", 0x00519020, &NiFilterMode);

// 0x519070 (thiscall): NiAlphaProperty::SetAlphaBlending (flag bit 0)
static void __fastcall SetAlphaBlending(uint8_t* self, re::Edx, int on) {
    if (re::Flag(on)) {
        At<uint16_t>(self, 0x18) |= 1;
    } else {
        At<uint16_t>(self, 0x18) &= 0xFFFE;
    }
}

// 0x519090 (cdecl): RwRasterDestroy: a raster with a texture (+0x34) only drops the
// texture (the raster itself isn't freed then); one without is freed. TRUE
static int __cdecl RasterDestroy(uint8_t* raster) {
    if (!raster) {
        return 1;
    }
    uint8_t* texture = At<uint8_t*>(raster, 0x34);
    if (texture) {
        if (vc::Decrement(texture + 4) == 0) {
            vc::Call<void>(texture, 0x04);
        }
        At<uint8_t*>(raster, 0x34) = nullptr;
        return 1;
    }
    NiFree(raster);
    return 1;
}

// 0x5190E0 (thiscall): NiBinaryExtraData::GetRTTI
static void* __fastcall BinaryExtraData_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2EC4);
}

// 0x5190F0 (thiscall): NiBinaryExtraData's deleting destructor (the data at +0xC freed)
static void* __fastcall BinaryExtraData_Destroy(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = kBinaryExtraDataVtable;
    NiFree(At<void*>(self, 0xC));
    At<void*>(self, 0xC) = nullptr;
    NiExtraData_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x14);
    }
    return self;
}

// 0x519130 (thiscall): NiFogProperty's constructor: off, depth 1, the default colour
static uint8_t* __fastcall FogProperty_Construct(uint8_t* self, re::Edx) {
    NiProperty_Construct(self);
    At<uint32_t>(self, 0) = kFogPropertyVtable;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    At<uint16_t>(self, 0x18) = 0;
    At<float>(self, 0x1C) = 1.0f;
    At<uint32_t>(self, 0x20) = ms_defaultFogColour[0];
    At<uint32_t>(self, 0x24) = ms_defaultFogColour[1];
    At<uint32_t>(self, 0x28) = ms_defaultFogColour[2];
    return self;
}

// 0x519180 (thiscall): NiFogProperty::GetRTTI
static void* __fastcall FogProperty_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2EF8);
}

// 0x519190 (thiscall): NiFogProperty's deleting destructor
static void* __fastcall FogProperty_Destroy(uint8_t* self, re::Edx, int flags) {
    NiProperty_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x2C);
    }
    return self;
}

// 0x5191B0 (thiscall): NiShadeProperty::GetRTTI
static void* __fastcall ShadeProperty_GetRTTI(void*, re::Edx) {
    return reinterpret_cast<void*>(0x020E2F30);
}

// 0x5191C0 (thiscall) vf18: NiShadeProperty::Type (SHADE: 5)
static int __fastcall ShadeProperty_Type(void*, re::Edx) {
    return 5;
}

} // namespace RwEnumTables

// ---- registration ----------------------------------------------------------------------

#define C16_RWENUM_REVERSED(name, fn, address)                                                       \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace RwEnumTables {

C16_RWENUM_REVERSED("NiAlphaProperty::SetAlphaBlending", SetAlphaBlending, 0x00519070);
C16_RWENUM_REVERSED("RwEmu::RasterDestroy", RasterDestroy, 0x00519090);
C16_RWENUM_REVERSED("NiBinaryExtraData::GetRTTI", BinaryExtraData_GetRTTI, 0x005190E0);
C16_RWENUM_REVERSED("NiBinaryExtraData::Destroy", BinaryExtraData_Destroy, 0x005190F0);
C16_RWENUM_REVERSED("NiFogProperty::Construct", FogProperty_Construct, 0x00519130);
C16_RWENUM_REVERSED("NiFogProperty::GetRTTI", FogProperty_GetRTTI, 0x00519180);
C16_RWENUM_REVERSED("NiFogProperty::Destroy", FogProperty_Destroy, 0x00519190);
C16_RWENUM_REVERSED("NiShadeProperty::GetRTTI", ShadeProperty_GetRTTI, 0x005191B0);
C16_RWENUM_REVERSED("NiShadeProperty::Type", ShadeProperty_Type, 0x005191C0);

} // namespace RwEnumTables
