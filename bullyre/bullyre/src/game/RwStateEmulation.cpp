// 0x5191D0 to 0x51A740: the RenderWare emulation over Gamebryo (the game's code still
// talks RW in places): RwRenderStateGet/Set kept in globals and forwarded to the DX9
// render state, the property set that turns those states into Gamebryo properties, and
// RwRasters wrapping NiTextures (NiDynamicTexture for the lockable ones). Also
// NiStencilProperty's small virtuals.

#include <cstddef>
#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

GAME_FN(void*, __cdecl, NiMalloc, 0x00752300, size_t size);
GAME_FN(void*, __cdecl, NiNew, 0x00752460, size_t size);
GAME_FN(void, __cdecl, NiFreeSized, 0x00752540, void* p, size_t size);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void, __thiscall, NiProperty_Construct, 0x00764550, void* self);
GAME_FN(void, __thiscall, NiProperty_Destruct, 0x00764590, void* self);
GAME_FN(void, __thiscall, NiExtraData_Construct, 0x00765640, void* self);
GAME_FN(void*, __thiscall, NiTexturingProperty_Construct, 0x00765DD0, void* self);
GAME_FN(void*, __thiscall, TexturingMap_Construct, 0x00765BA0, void* self);
GAME_FN(void, __thiscall, TexturingMap_SetTexture, 0x00829350, void* self, void* texture);
GAME_FN(void, __thiscall, NiTArray_SetAt, 0x00833C70, void* self, uint32_t index, re::Ptr32<void>* item);
GAME_FN(void*, __thiscall, NiObjectNET_GetExtraData, 0x00764250, void* object, const re::Ptr32<char>* name);
GAME_FN(uint8_t, __thiscall, NiObjectNET_AddExtraData, 0x00764600, void* object, const re::Ptr32<char>* name, void* data);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<char>* self, const re::Ptr32<char>* other);
GAME_FN(void, __thiscall, NiAVObject_AttachProperty, 0x004099F0, void* object, void* property);
// NiAVObject::RemoveProperty(type): the removed property returned as an NiPointer (hidden
// result pointer first)
GAME_FN(re::Ptr32<void>*, __thiscall, NiAVObject_RemoveProperty, 0x0075E8A0, void* object, re::Ptr32<void>* result, int type);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rttiInfo, void* object);
// NiDynamicTexture::Create(width, height, format prefs, tiled)
GAME_FN(uint8_t*, __cdecl, NiDynamicTexture_Create, 0x00779280, uint32_t width, uint32_t height, void* prefs,
        int tiled);
// NiDX9RenderState: SetRenderState (cached at +0x120 + state * 8; the device at +0x2238)
GAME_FN(void, __thiscall, DX9RenderState_SetRenderState, 0x00406630, void* self, int state, uint32_t value);
// NiDX9RenderState: the fog property's states
GAME_FN(void, __thiscall, DX9RenderState_ApplyFog, 0x008828F0, void* self, void* fog);
GAME_FN(void*, __thiscall, NiFogProperty_Construct, 0x00519130, void* self);
GAME_FN(void, __thiscall, NiFogProperty_SetFog, 0x00519070, void* self, int on);
GAME_FN(uint8_t*, __cdecl, GetFogProperty, 0x00519250);
GAME_FN(uint8_t*, __cdecl, FindGeometry, 0x00519C00, void* object);
GAME_FN(void, __thiscall, PropertySet_AttachTo, 0x0051A2D0, void* self, void* object);

GAME_VAR(uint8_t*, ms_pRenderer, 0x020E2380);
GAME_VAR(uint32_t, ms_NiDX9RendererBit, 0x020E6D20);
GAME_VAR(uint32_t, ms_NiGeometryBit, 0x020E25BC);
GAME_VAR(uint32_t, ms_NiNodeBit, 0x020E2550);
GAME_VAR(uint32_t, ms_NiBinaryExtraDataBit, 0x020E2ECC);
inline void* const ms_NiDynamicTextureRTTI = reinterpret_cast<void*>(0x020E2FA0);
inline const re::Ptr32<char>* const ms_rasterExtraDataName = reinterpret_cast<const re::Ptr32<char>*>(0x00BCC49C);
// NiColor::BLACK (NiFogProperty's default colour)
GAME_ARRAY(uint32_t, ms_fogDefaultColor, 0x020E25E8);

// the RW render states' values (rwRENDERSTATE*)
GAME_VAR(uint8_t*, ms_textureRaster, 0x00C67710);      // 1  TEXTURERASTER
GAME_VAR(uint32_t, ms_textureAddressU, 0x00A76958);    // 3  (2 sets both)
GAME_VAR(uint32_t, ms_textureAddressV, 0x00A7695C);    // 4
GAME_VAR(uint32_t, ms_zTestEnable, 0x00C67714);        // 6
GAME_VAR(uint32_t, ms_shadeMode, 0x00A76974);          // 7
GAME_VAR(uint32_t, ms_zWriteEnable, 0x00C67718);       // 8
GAME_VAR(uint32_t, ms_textureFilter, 0x00A76960);      // 9
GAME_VAR(uint32_t, ms_srcBlend, 0x00A76968);           // 10 (as D3DBLEND)
GAME_VAR(uint32_t, ms_destBlend, 0x00A7696C);          // 11
GAME_VAR(uint32_t, ms_vertexAlphaEnable, 0x00C6771C);  // 12
GAME_VAR(uint32_t, ms_alphaTestEnable, 0x00C67720);    // (compared, never set here)
GAME_VAR(uint32_t, ms_cullMode, 0x00A76964);           // 20
GAME_VAR(uint8_t, ms_stencilEnable, 0x00C67728);       // 21
GAME_VAR(uint32_t, ms_alphaTestFunction, 0x00A76970);  // 29 (as D3DCMPFUNC)
GAME_VAR(uint32_t, ms_alphaTestRef, 0x00C67724);       // 30
GAME_VAR(uint8_t*, ms_pFogProperty, 0x00C6770C);

constexpr uintptr_t kNiStencilPropertyVtable = 0x00917AAC;
constexpr uintptr_t kNiAlphaPropertyVtable = 0x008FFB9C;
constexpr uintptr_t kNiFogPropertyVtable = 0x009179FC;
constexpr uintptr_t kNiShadePropertyVtable = 0x00917A54;
constexpr uintptr_t kNiZBufferPropertyVtable = 0x008FFCA4;
constexpr uintptr_t kNiBinaryExtraDataVtable = 0x009179A4;

uint32_t& U(void* p, int offset) {
    return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + offset);
}
uint16_t& W(void* p, int offset) {
    return *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + offset);
}
uint8_t& B(void* p, int offset) {
    return static_cast<uint8_t*>(p)[offset];
}
float& F(void* p, int offset) {
    return *reinterpret_cast<float*>(static_cast<uint8_t*>(p) + offset);
}

uint8_t* IsKindOfBit(void* object, uint32_t bit) {
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? static_cast<uint8_t*>(object) : nullptr;
}

void Release(void* object) {
    if (object && vc::Decrement(static_cast<uint8_t*>(object) + 4) == 0) {
        vc::Call<void>(object, 0x04);
    }
}

// NiPointer assignment
void Assign(re::Ptr32<void>& slot, void* object) {
    if (slot == object) {
        return;
    }
    Release(slot);
    slot = object;
    if (object) {
        vc::Increment(static_cast<uint8_t*>(object) + 4);
    }
}

// 0x518E60 / 0x518EC0 / 0x518F40 / 0x518FC0 / 0x519020 take their value in eax (the
// RW <-> D3D / Gamebryo enum tables)
__declspec(noinline) uint32_t CallEax(uintptr_t fn, uint32_t value) {
#if defined(_M_IX86)
    uint32_t result;
    __asm {
        mov eax, value
        call fn
        mov result, eax
    }
    return result;
#else
    return re::Ours<uint32_t (*)(uint32_t)>(fn)(value);
#endif
}
constexpr uintptr_t kAlphaFunctionFromRw = 0x00518E60;  // RW alpha test function -> D3DCMPFUNC
constexpr uintptr_t kBlendFromRw = 0x00518EC0;          // RW blend -> D3DBLEND (and back)
constexpr uintptr_t kNiAlphaBlend = 0x00518F40;         // D3DBLEND -> NiAlphaProperty's
constexpr uintptr_t kNiAlphaTestFunction = 0x00518FC0;  // D3DCMPFUNC -> NiAlphaProperty's
constexpr uintptr_t kNiFilterMode = 0x00519020;         // RW filter -> NiTexturingProperty's

// the DX9 render state (NiDX9Renderer +0x6B4). (A renderer that isn't an NiDX9Renderer
// reads address 0x6B4: never the case)
uint8_t* RenderState() {
    uint8_t* dx9 = ms_pRenderer ? IsKindOfBit(ms_pRenderer, ms_NiDX9RendererBit) : nullptr;
    return *reinterpret_cast<re::Ptr32<uint8_t>*>(reinterpret_cast<uintptr_t>(dx9) + 0x6B4);
}
uint32_t CachedState(uint8_t* renderState, int d3dState) {
    return U(renderState, 0x120 + d3dState * 8);
}
// the state set on the device when the cache differs
void Refresh(uint8_t* renderState, int d3dState, uint32_t value) {
    if (CachedState(renderState, d3dState) != value) {
        DX9RenderState_SetRenderState(renderState, d3dState, value);
    }
}

} // namespace

// RwRaster, as the emulation keeps it: RW's fields and the NiTexture behind it
struct RwRaster {
    re::Ptr32<RwRaster> parent;         // +0x00
    re::Ptr32<uint8_t> cpPixels;        // +0x04 (while locked; ours keep it in LockedPixels)
    re::Ptr32<uint8_t> palette;         // +0x08
    int32_t width;            // +0x0C
    int32_t height;           // +0x10
    int32_t depth;            // +0x14
    int32_t stride;           // +0x18
    int16_t nOffsetX;         // +0x1C
    int16_t nOffsetY;         // +0x1E
    uint8_t cType;            // +0x20 (rwRASTERTYPE*)
    uint8_t cFlags;           // +0x21
    uint8_t privateFlags;     // +0x22
    uint8_t cFormat;          // +0x23
    re::Ptr32<uint8_t> originalPixels;  // +0x24
    int32_t originalWidth;    // +0x28
    int32_t originalHeight;   // +0x2C
    int32_t originalStride;   // +0x30
    re::Ptr32<uint8_t> texture;         // +0x34 NiTexture
};
static_assert(sizeof(RwRaster) == 0x38);
static_assert(offsetof(RwRaster, cType) == 0x20);
static_assert(offsetof(RwRaster, texture) == 0x34);

// The Gamebryo properties the RW states become (one set per user), NiPointers
struct RwPropertySet {
    re::Ptr32<void> alpha;      // +0x00 NiAlphaProperty
    re::Ptr32<void> fog;        // +0x04 NiFogProperty
    re::Ptr32<void> shade;      // +0x08 NiShadeProperty
    re::Ptr32<void> stencil;    // +0x0C NiStencilProperty
    re::Ptr32<void> texturing;  // +0x10 NiTexturingProperty
    re::Ptr32<void> baseMap;    // +0x14 its base map (not a reference)
    re::Ptr32<void> zbuffer;    // +0x18 NiZBufferProperty
};
static_assert(sizeof(RwPropertySet) == 0x1C);

namespace RwEmulation {

// ---- NiStencilProperty -----------------------------------------------------------------

// 0x5191D0 (thiscall): the constructor (draw mode CCW, test off, ref 0, mask ~0)
static void* __fastcall StencilProperty_Construct(uint8_t* self, re::Edx) {
    NiProperty_Construct(self);
    U(self, 0x00) = kNiStencilPropertyVtable;
    U(self, 0x1C) = 0;
    U(self, 0x20) = 0xFFFFFFFF;
    W(self, 0x18) = 0x4180;
    return self;
}

// 0x519200: GetRTTI
static void* __cdecl StencilProperty_GetRTTI() {
    return reinterpret_cast<void*>(0x020E2F68);
}

// 0x519210: the property type (STENCIL)
static int __cdecl StencilProperty_Type() {
    return 7;
}

// 0x519220 (thiscall): the deleting destructor (flags 1: freed)
static void* __fastcall StencilProperty_Destroy(uint8_t* self, re::Edx, uint32_t flags) {
    U(self, 0x00) = kNiStencilPropertyVtable;
    NiProperty_Destruct(self);
    if (flags & 1) {
        NiFreeSized(self, 0x24);
    }
    return self;
}

// ---- render states ---------------------------------------------------------------------

// 0x519250: the shared fog property, made (fog off) on first use. (A failed allocation
// writes to address 0x18)
static uint8_t* __cdecl FogProperty() {
    if (!ms_pFogProperty) {
        void* memory = NiNew(0x2C);
        if (memory) {
            uint8_t* fog = static_cast<uint8_t*>(NiFogProperty_Construct(memory));
            W(fog, 0x18) &= 0xFFFE;
            ms_pFogProperty = fog;
            return fog;
        }
        *reinterpret_cast<volatile uint16_t*>(uintptr_t(0x18)) &= 0xFFFE;
        ms_pFogProperty = nullptr;
    }
    return ms_pFogProperty;
}

// 0x519290: RwRenderStateGet. Those the DX9 state caches are pushed to the device first
// when the cache differs (so what's returned is also what's set). Always 1 but for
// TEXTUREPERSPECTIVE and BORDERCOLOR (0, nothing written)
static uint32_t __cdecl RenderStateGet(int state, uint32_t* value) {
    uint8_t* rs = RenderState();
    switch (state) {
    case 1:
        *value = re::PtrCast<uint32_t>(ms_textureRaster);
        return 1;
    case 2:
    case 3:
        *value = ms_textureAddressU;
        return 1;
    case 4:
        *value = ms_textureAddressV;
        return 1;
    case 5:
    case 13:
        return 0;
    case 6:
        Refresh(rs, 7, ms_zTestEnable);
        *value = ms_zTestEnable;
        return 1;
    case 7:
        *value = ms_shadeMode;
        return 1;
    case 8:
        Refresh(rs, 0xE, ms_zWriteEnable);
        *value = ms_zWriteEnable;
        return 1;
    case 9:
        *value = ms_textureFilter;
        return 1;
    case 10:
        Refresh(rs, 0x13, ms_srcBlend);
        *value = CallEax(kBlendFromRw, ms_srcBlend);
        return 1;
    case 11:
        Refresh(rs, 0x14, ms_destBlend);
        *value = CallEax(kBlendFromRw, ms_destBlend);
        return 1;
    case 12:
        Refresh(rs, 0x1B, ms_vertexAlphaEnable);
        Refresh(rs, 0xF, ms_alphaTestEnable);
        *value = ms_vertexAlphaEnable;
        return 1;
    case 14:
        *value = B(FogProperty(), 0x18) & 1;
        return 1;
    case 15: {
        // the colour's channels * 255 truncated, packed R, G, B; the top byte is the red
        // integer's (0 in range). (x87 at the renderer's precision)
        uint8_t* fog = FogProperty();
        float r = F(fog, 0x20);
        float g = F(fog, 0x24);
        float b = F(fog, 0x28);
        uint32_t ri = uint32_t(int32_t(double(r) * 255.0));
        uint32_t gi = uint32_t(int32_t(double(g) * 255.0));
        uint32_t bi = uint32_t(int32_t(double(b) * 255.0));
        *value = (ri & 0xFF000000u) | (ri & 0xFF) | ((gi & 0xFF) << 8) | ((bi & 0xFF) << 16);
        return 1;
    }
    case 16: {
        uint32_t mode = (B(FogProperty(), 0x18) >> 1) & 3;
        *value = mode == 1 ? 3 : 1;
        return 1;
    }
    case 17:
        *value = 0;
        return 1;
    case 20:
        Refresh(rs, 0x16, ms_cullMode);
        *value = (ms_cullMode == 2 || ms_cullMode == 3) ? ms_cullMode : 1;
        return 1;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
        // the stencil states straight from the cache (D3DRS_STENCILENABLE.. 0x34..0x3B)
        *value = CachedState(rs, 0x34 + (state - 21));
        return 1;
    case 29:
        Refresh(rs, 0x19, ms_alphaTestFunction);
        *value = CallEax(kAlphaFunctionFromRw, ms_alphaTestFunction);
        return 1;
    case 30:
        Refresh(rs, 0x18, ms_alphaTestRef);
        *value = ms_alphaTestRef;
        return 1;
    default:
        return 1;
    }
}

// 0x519790: RwRenderStateSet: kept, and the device's state set where D3D has one.
// Always 1 but for TEXTUREPERSPECTIVE and BORDERCOLOR (0)
static uint32_t __cdecl RenderStateSet(int state, uint32_t value) {
    uint8_t* rs = RenderState();
    switch (state) {
    case 1:
        ms_textureRaster = reinterpret_cast<uint8_t*>(value);
        return 1;
    case 2:
        ms_textureAddressV = value;
        ms_textureAddressU = value;
        return 1;
    case 3:
        ms_textureAddressU = value;
        return 1;
    case 4:
        ms_textureAddressV = value;
        return 1;
    case 5:
    case 13:
        return 0;
    case 6:
        ms_zTestEnable = value;
        DX9RenderState_SetRenderState(rs, 7, value);
        return 1;
    case 7:
        ms_shadeMode = value;
        return 1;
    case 8:
        ms_zWriteEnable = value;
        DX9RenderState_SetRenderState(rs, 0xE, value);
        return 1;
    case 9:
        ms_textureFilter = value;
        return 1;
    case 10:
        ms_srcBlend = CallEax(kBlendFromRw, value);
        DX9RenderState_SetRenderState(rs, 0x13, ms_srcBlend);
        return 1;
    case 11:
        ms_destBlend = CallEax(kBlendFromRw, value);
        DX9RenderState_SetRenderState(rs, 0x14, ms_destBlend);
        return 1;
    case 12:
        ms_vertexAlphaEnable = value;
        DX9RenderState_SetRenderState(rs, 0x1B, value);
        return 1;
    case 14: {
        // (the flag pushed is eax's low byte over the case index 0xD: 0xD00 | on; the
        // setter reads the byte)
        int on = 0xD00 | (value != 0 ? 1 : 0);
        NiFogProperty_SetFog(FogProperty(), on);
        DX9RenderState_ApplyFog(rs, FogProperty());
        return 1;
    }
    case 15: {
        float r = float(double(value & 0xFF) / 255.0);
        float g = float(double((value >> 8) & 0xFF) / 255.0);
        float b = float(double((value >> 16) & 0xFF) / 255.0);
        uint8_t* fog = FogProperty();
        F(fog, 0x20) = r;
        F(fog, 0x24) = g;
        F(fog, 0x28) = b;
        DX9RenderState_ApplyFog(rs, FogProperty());
        return 1;
    }
    case 16: {
        // RW's EXPONENTIAL / EXPONENTIAL2 (2, 3) -> Gamebryo's FOG_RANGE_SQ (1); else 0
        int32_t type = int32_t(value);
        uint16_t mode = (type != 1 && type > 1 && type <= 3) ? 1 : 0;
        uint8_t* fog = FogProperty();
        W(fog, 0x18) = uint16_t((W(fog, 0x18) & 0xFFF9) | (mode * 2));
        DX9RenderState_ApplyFog(rs, FogProperty());
        return 1;
    }
    case 20: {
        uint32_t mode = (value == 2 || value == 3) ? value : 1;
        ms_cullMode = mode;
        DX9RenderState_SetRenderState(rs, 0x16, mode);
        return 1;
    }
    case 21:
        ms_stencilEnable = value != 0 ? 1 : 0;
        DX9RenderState_SetRenderState(rs, 0x34, value);
        return 1;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
        DX9RenderState_SetRenderState(rs, 0x35 + (state - 22), value);
        return 1;
    case 29:
        ms_alphaTestFunction = CallEax(kAlphaFunctionFromRw, value);
        DX9RenderState_SetRenderState(rs, 0x19, ms_alphaTestFunction);
        return 1;
    case 30:
        ms_alphaTestRef = value;
        DX9RenderState_SetRenderState(rs, 0x18, value);
        return 1;
    default:
        return 1;
    }
}

// 0x519C00: the first NiGeometry in `object`'s tree (depth first through NiNodes'
// children), or null
static uint8_t* __cdecl FindFirstGeometry(uint8_t* object) {
    if (!object) {
        return nullptr;
    }
    if (IsKindOfBit(object, ms_NiGeometryBit)) {
        return object;
    }
    uint8_t* node = IsKindOfBit(object, ms_NiNodeBit);
    if (!node) {
        return nullptr;
    }
    // the children: NiTArray at +0xC4 (+0xC8 the array, +0xCE the count)
    for (uint32_t i = 0; i < W(node, 0xCE); i++) {
        uint8_t* child = reinterpret_cast<re::Ptr32<uint8_t>*>(U(node, 0xC8))[i];
        uint8_t* found = FindGeometry(child);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

// 0x519CB0 (thiscall): the properties brought up to the current RW states
static void __fastcall PropertySet_Update(RwPropertySet* self, re::Edx) {
    if (uint8_t* alpha = static_cast<uint8_t*>(self->alpha)) {
        if (ms_vertexAlphaEnable) {
            W(alpha, 0x18) |= 1;
            uint8_t* a = static_cast<uint8_t*>(self->alpha);
            W(a, 0x18) = uint16_t((CallEax(kNiAlphaBlend, ms_srcBlend) * 2) | (W(a, 0x18) & 0xFFE1));
            a = static_cast<uint8_t*>(self->alpha);
            W(a, 0x18) = uint16_t((CallEax(kNiAlphaBlend, ms_destBlend) << 5) | (W(a, 0x18) & 0xFE1F));
        } else {
            W(alpha, 0x18) &= 0xFFFE;
        }
        uint32_t test = CallEax(kNiAlphaTestFunction, ms_alphaTestFunction);
        uint8_t* a = static_cast<uint8_t*>(self->alpha);
        if (test) {
            W(a, 0x18) |= 0x200;
            a = static_cast<uint8_t*>(self->alpha);
            W(a, 0x18) = uint16_t((W(a, 0x18) & 0xE3FF) | (test << 10));
            B(self->alpha, 0x1A) = uint8_t(ms_alphaTestRef);
        } else {
            W(a, 0x18) &= 0xFDFF;
        }
    }
    if (self->fog) {
        // (FogProperty inlined)
        if (!ms_pFogProperty) {
            void* memory = NiNew(0x2C);
            uint8_t* fog = memory ? static_cast<uint8_t*>(NiFogProperty_Construct(memory)) : nullptr;
            // (null: writes to address 0x18)
            *reinterpret_cast<volatile uint16_t*>(reinterpret_cast<uintptr_t>(fog) + 0x18) &= 0xFFFE;
            ms_pFogProperty = fog;
        }
        if (B(ms_pFogProperty, 0x18) & 1) {
            W(self->fog, 0x18) |= 1;
            uint8_t shared = B(FogProperty(), 0x18);
            W(self->fog, 0x18) ^= uint16_t((shared ^ B(self->fog, 0x18)) & 6);
            uint8_t* source = FogProperty();
            U(self->fog, 0x20) = U(source, 0x20);
            U(self->fog, 0x24) = U(source, 0x24);
            U(self->fog, 0x28) = U(source, 0x28);
            float density = F(FogProperty(), 0x1C);
            F(self->fog, 0x1C) = !(density >= 0.0f) ? 0.0f : density;   // (NaN too)
        } else {
            W(self->fog, 0x18) &= 0xFFFE;
        }
    }
    if (self->shade) {
        if (ms_shadeMode == 2) {
            W(self->shade, 0x18) |= 1;
        } else {
            W(self->shade, 0x18) &= 0xFFFE;
        }
    }
    if (self->stencil) {
        // the draw mode from the cull mode (NONE: both, BACK: CCW... as Gamebryo has them)
        uint16_t drawMode;
        if (ms_cullMode == 2) {
            drawMode = 2;
        } else if (ms_cullMode == 3) {
            drawMode = 1;
        } else {
            drawMode = 3;
        }
        W(self->stencil, 0x18) = uint16_t((W(self->stencil, 0x18) & 0xF3FF) | (drawMode << 10));
    }
    if (self->texturing) {
        uint8_t* map = static_cast<uint8_t*>(self->baseMap);
        if (map) {
            if (!ms_textureRaster) {
                void* texture = *reinterpret_cast<re::Ptr32<void>*>(map + 8);
                if (texture) {
                    Release(texture);
                    *reinterpret_cast<re::Ptr32<void>*>(map + 8) = nullptr;
                }
            } else {
                TexturingMap_SetTexture(map, *reinterpret_cast<re::Ptr32<void>*>(ms_textureRaster + 0x34));
                // the clamp mode: CLAMP_S_CLAMP_T 0 .. WRAP_S_WRAP_T 3 (RW's CLAMP is 3)
                uint16_t clamp = ms_textureAddressU == 3 ? uint16_t(ms_textureAddressV != 3)
                                                         : uint16_t((ms_textureAddressV != 3) + 2);
                uint8_t* m = static_cast<uint8_t*>(self->baseMap);
                W(m, 4) = uint16_t((W(m, 4) & 0xCFFF) | (clamp << 12));
                m = static_cast<uint8_t*>(self->baseMap);
                uint8_t filter = uint8_t(CallEax(kNiFilterMode, ms_textureFilter));
                W(m, 4) = uint16_t((uint16_t(filter) << 8) | (W(m, 4) & 0xF0FF));
            }
        }
    }
    if (self->zbuffer) {
        if (ms_zTestEnable) {
            W(self->zbuffer, 0x18) |= 1;
        } else {
            W(self->zbuffer, 0x18) &= 0xFFFE;
        }
        if (ms_zWriteEnable) {
            W(self->zbuffer, 0x18) |= 2;
            return;
        }
        W(self->zbuffer, 0x18) &= 0xFFFD;
    }
}

// the raster's NiBinaryExtraData on its texture (named at 0xBCC49C), made and added
static void AddRasterExtraData(RwRaster* raster) {
    uint8_t* data = static_cast<uint8_t*>(NiNew(0x14));
    if (data) {
        NiExtraData_Construct(data);
        U(data, 0x00) = kNiBinaryExtraDataVtable;
        U(data, 0x10) = 0x38;
        *reinterpret_cast<re::Ptr32<RwRaster>*>(data + 0xC) = raster;
    } else {
        data = nullptr;
    }
    // (a failed allocation assigns the name at address 8)
    FixedString_Assign(reinterpret_cast<re::Ptr32<char>*>(data + 8), ms_rasterExtraDataName);
    NiObjectNET_AddExtraData(raster->texture, ms_rasterExtraDataName, data);
}

// 0x519F60: RwRasterCreate(width, height, depth (unused), flags). A lockable texture
// (TEXTURE with a 0x500 format) gets an NiDynamicTexture (8888), kept with the raster
static RwRaster* __cdecl RasterCreate(int32_t width, int32_t height, int32_t, uint32_t flags) {
    PushMemoryTag(0x3A);
    RwRaster* raster = static_cast<RwRaster*>(NiMalloc(0x38));
    PopMemoryTag();
    raster->texture = nullptr;
    switch (flags & 7) {
    case 1:   // ZBUFFER
        raster->cType = 1;
        raster->cFormat = 8;
        raster->cFlags = 0;
        raster->privateFlags = 0;
        break;
    case 2:   // CAMERA
        raster->cType = 2;
        raster->cFlags = 0;
        raster->privateFlags = 0;
        break;
    case 4:   // TEXTURE
        raster->cType = 4;
        raster->cFlags = 0;
        raster->privateFlags = 0;
        raster->cFormat = 5;
        if (flags & 0x500) {
            uint32_t prefs[3] = {2, 2, 0};   // FormatPrefs: TRUE_COLOR_32, SMOOTH, NO mipmaps
            uint8_t* texture = NiDynamicTexture_Create(width, height, prefs, 0);
            vc::Increment(texture + 4);
            raster->texture = texture;
        }
        break;
    case 5:   // CAMERATEXTURE
        raster->cType = 5;
        raster->cFormat = 5;
        raster->cFlags = 0;
        raster->privateFlags = 0;
        break;
    default:
        break;
    }
    uint8_t* texture = raster->texture;
    raster->parent = nullptr;
    raster->cpPixels = nullptr;
    raster->palette = nullptr;
    raster->width = width;
    raster->height = height;
    // the depth from the texture's renderer data (+0x24; its +0x15), else 32
    uint8_t* rendererData = texture ? *reinterpret_cast<re::Ptr32<uint8_t>*>(texture + 0x24) : nullptr;
    if (texture && rendererData && reinterpret_cast<uintptr_t>(rendererData) + 0x14 != 0) {
        raster->depth = rendererData[0x15];
    } else {
        raster->depth = 0x20;
    }
    int32_t stride = raster->depth * width;
    raster->stride = stride;
    raster->nOffsetX = 0;
    raster->nOffsetY = 0;
    raster->originalPixels = nullptr;
    raster->originalWidth = width;
    raster->originalHeight = height;
    raster->originalStride = stride;
    if (texture) {
        uint8_t* existing = static_cast<uint8_t*>(NiObjectNET_GetExtraData(texture, ms_rasterExtraDataName));
        if (!existing || !IsKindOfBit(existing, ms_NiBinaryExtraDataBit)) {
            AddRasterExtraData(raster);
        }
    }
    return raster;
}

// 0x51A120: the RwRaster of an NiTexture: the one kept in its extra data, or a new
// TEXTURE raster (its depth from the renderer data's +0x15)
static RwRaster* __cdecl RasterFromTexture(uint8_t* texture) {
    if (!texture) {
        return nullptr;
    }
    uint8_t* existing = static_cast<uint8_t*>(NiObjectNET_GetExtraData(texture, ms_rasterExtraDataName));
    if (existing) {
        uint8_t* data = IsKindOfBit(existing, ms_NiBinaryExtraDataBit);
        if (data) {
            return *reinterpret_cast<re::Ptr32<RwRaster>*>(data + 0xC);
        }
    }
    PushMemoryTag(0x3A);
    RwRaster* raster = static_cast<RwRaster*>(NiMalloc(0x38));
    PopMemoryTag();
    raster->parent = nullptr;
    raster->cpPixels = nullptr;
    raster->palette = nullptr;
    uint8_t* rendererData = *reinterpret_cast<re::Ptr32<uint8_t>*>(texture + 0x24);
    uint8_t* format = rendererData ? rendererData + 0x14 : nullptr;   // (null: reads address 1)
    raster->width = vc::Call<int32_t>(texture, 0x48);    // GetWidth
    int32_t height = vc::Call<int32_t>(texture, 0x4C);   // GetHeight
    raster->height = height;
    int32_t depth = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(format) + 1)[0];
    int32_t stride = raster->width * depth;
    raster->originalHeight = height;
    raster->depth = depth;
    raster->stride = stride;
    raster->nOffsetX = 0;
    raster->nOffsetY = 0;
    raster->cType = 4;
    raster->cFlags = 0;
    raster->privateFlags = 0;
    raster->cFormat = 5;
    raster->originalPixels = nullptr;
    raster->originalWidth = raster->width;
    raster->originalStride = stride;
    raster->texture = texture;
    AddRasterExtraData(raster);
    return raster;
}

// a raster's cpPixels: the dynamic texture's memory as D3D locked it (in the 64-bit build
// anywhere, so kept as a native pointer: re::NativeField; only our code reads it)
inline uint8_t*& LockedPixels(RwRaster* raster) {
    return re::NativeField<uint8_t>(raster, 4);
}

// 0x51A250: RwRasterLock: an NiDynamicTexture's pixels (Lock, its pitch into a scratch
// int), kept in cpPixels
static uint8_t* __cdecl RasterLock(RwRaster* raster) {
    if (!raster || !raster->texture) {
        return nullptr;
    }
    uint8_t* texture = IsKindOf(ms_NiDynamicTextureRTTI, raster->texture);
    if (!texture) {
        return nullptr;
    }
    // (the game passes its own argument's slot for the pitch)
    int32_t pitch = 0;
    uint8_t* pixels = vc::Call<uint8_t*>(texture, 0x50, &pitch);
    LockedPixels(raster) = pixels;
    return pixels;
}

// 0x51A290: RwRasterUnlock
static RwRaster* __cdecl RasterUnlock(RwRaster* raster) {
    if (!raster || !raster->texture) {
        return nullptr;
    }
    uint8_t* texture = IsKindOf(ms_NiDynamicTextureRTTI, raster->texture);
    if (!texture) {
        return nullptr;
    }
    LockedPixels(raster) = nullptr;
    vc::Call<void>(texture, 0x54);
    return raster;
}

// 0x51A2D0 (thiscall): the set's properties attached to `object` (replacing those of
// the same types)
static void __fastcall PropertySet_AttachProperties(RwPropertySet* self, re::Edx, uint8_t* object) {
    if (!object) {
        return;
    }
    struct Slot {
        re::Ptr32<void> RwPropertySet::*member;
        int type;
    };
    static const Slot slots[] = {
        {&RwPropertySet::alpha, 0},   {&RwPropertySet::fog, 2},        {&RwPropertySet::shade, 5},
        {&RwPropertySet::stencil, 7}, {&RwPropertySet::texturing, 8}, {&RwPropertySet::zbuffer, 0xB},
    };
    for (const Slot& slot : slots) {
        if (self->*slot.member) {
            re::Ptr32<void> removed = nullptr;
            NiAVObject_RemoveProperty(object, &removed, slot.type);
            Release(removed);
            NiAVObject_AttachProperty(object, self->*slot.member);
        }
    }
}

// 0x51A460 (thiscall): the constructor: a property of each kind (Gamebryo's defaults),
// the texturing property's base map, then attached to `object` when given
static RwPropertySet* __fastcall PropertySet_Construct(RwPropertySet* self, re::Edx, uint8_t* object) {
    self->alpha = nullptr;
    self->fog = nullptr;
    self->shade = nullptr;
    self->stencil = nullptr;
    self->texturing = nullptr;
    self->zbuffer = nullptr;

    uint8_t* alpha = static_cast<uint8_t*>(NiNew(0x1C));
    if (alpha) {
        NiProperty_Construct(alpha);
        U(alpha, 0x00) = kNiAlphaPropertyVtable;
        B(alpha, 0x1A) = 0;
        W(alpha, 0x18) = 0xEC;
    }
    Assign(self->alpha, alpha);

    uint8_t* fog = static_cast<uint8_t*>(NiNew(0x2C));
    if (fog) {
        NiProperty_Construct(fog);
        U(fog, 0x00) = kNiFogPropertyVtable;
        F(fog, 0x20) = 0.0f;
        F(fog, 0x24) = 0.0f;
        F(fog, 0x28) = 0.0f;
        W(fog, 0x18) = 0;
        F(fog, 0x1C) = 1.0f;
        U(fog, 0x20) = ms_fogDefaultColor[0];
        U(fog, 0x24) = ms_fogDefaultColor[1];
        U(fog, 0x28) = ms_fogDefaultColor[2];
    }
    Assign(self->fog, fog);

    uint8_t* shade = static_cast<uint8_t*>(NiNew(0x1C));
    if (shade) {
        NiProperty_Construct(shade);
        U(shade, 0x00) = kNiShadePropertyVtable;
        W(shade, 0x18) = 1;
    }
    Assign(self->shade, shade);

    uint8_t* stencil = static_cast<uint8_t*>(NiNew(0x24));
    if (stencil) {
        NiProperty_Construct(stencil);
        U(stencil, 0x00) = kNiStencilPropertyVtable;
        U(stencil, 0x1C) = 0;
        U(stencil, 0x20) = 0xFFFFFFFF;
        W(stencil, 0x18) = 0x4180;
    }
    Assign(self->stencil, stencil);

    void* memory = NiNew(0x34);
    void* texturing = memory ? NiTexturingProperty_Construct(memory) : nullptr;
    Assign(self->texturing, texturing);

    memory = NiNew(0x10);
    re::Ptr32<void> map = memory ? TexturingMap_Construct(memory) : nullptr;
    self->baseMap = map;
    uint8_t* texturingProperty = static_cast<uint8_t*>(self->texturing);
    void* current = **reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(texturingProperty + 0x20);   // maps[0]
    if (map != current) {
        if (current) {
            vc::Call<void>(current, 0x00, 1);   // deleting destructor
        }
        NiTArray_SetAt(texturingProperty + 0x1C, 0, &map);
    }

    uint8_t* zbuffer = static_cast<uint8_t*>(NiNew(0x30));
    if (zbuffer) {
        NiProperty_Construct(zbuffer);
        F(zbuffer, 0x1C) = 0.0f;
        U(zbuffer, 0x00) = kNiZBufferPropertyVtable;
        F(zbuffer, 0x20) = 0.0f;
        U(zbuffer, 0x24) = 0;
        W(zbuffer, 0x18) = 0xF;
        F(zbuffer, 0x28) = 1.0f;
        F(zbuffer, 0x2C) = 1.0f;
    }
    Assign(self->zbuffer, zbuffer);

    if (object) {
        PropertySet_AttachTo(self, object);
    }
    return self;
}

// 0x51A740 (thiscall): attached to the first geometry under `object` (or to `object`)
static void __fastcall PropertySet_AttachToGeometry(RwPropertySet* self, re::Edx, uint8_t* object) {
    uint8_t* target = FindGeometry(object);
    if (!target) {
        target = object;
    }
    PropertySet_AttachTo(self, target);
}

} // namespace RwEmulation

// ---- registration ----------------------------------------------------------------------

#define C16D_RW_REVERSED(name, fn, address)                                                         \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace RwEmulation {
C16D_RW_REVERSED("NiStencilProperty::Construct", StencilProperty_Construct, 0x005191D0);
C16D_RW_REVERSED("NiStencilProperty::GetRTTI", StencilProperty_GetRTTI, 0x00519200);
C16D_RW_REVERSED("NiStencilProperty::Type", StencilProperty_Type, 0x00519210);
C16D_RW_REVERSED("NiStencilProperty::Destroy", StencilProperty_Destroy, 0x00519220);
C16D_RW_REVERSED("RwEmulation::FogProperty", FogProperty, 0x00519250);
C16D_RW_REVERSED("RwEmulation::RwRenderStateGet", RenderStateGet, 0x00519290);
C16D_RW_REVERSED("RwEmulation::RwRenderStateSet", RenderStateSet, 0x00519790);
C16D_RW_REVERSED("RwEmulation::FindFirstGeometry", FindFirstGeometry, 0x00519C00);
C16D_RW_REVERSED("RwPropertySet::Update", PropertySet_Update, 0x00519CB0);
C16D_RW_REVERSED("RwEmulation::RwRasterCreate", RasterCreate, 0x00519F60);
C16D_RW_REVERSED("RwEmulation::RwRasterFromTexture", RasterFromTexture, 0x0051A120);
C16D_RW_REVERSED("RwEmulation::RwRasterLock", RasterLock, 0x0051A250);
C16D_RW_REVERSED("RwEmulation::RwRasterUnlock", RasterUnlock, 0x0051A290);
C16D_RW_REVERSED("RwPropertySet::AttachTo", PropertySet_AttachProperties, 0x0051A2D0);
C16D_RW_REVERSED("RwPropertySet::Construct", PropertySet_Construct, 0x0051A460);
C16D_RW_REVERSED("RwPropertySet::AttachToGeometry", PropertySet_AttachToGeometry, 0x0051A740);
} // namespace RwEmulation
