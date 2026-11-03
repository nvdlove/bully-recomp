// 0x524FF0 to 0x5251C0: CShadows (GTA's), the part before Shadows.cpp's: a static
// shadow's polys given back, Init (the shadow textures from the "particle" txd, the
// fan's index list, the 20 static shadows and the 380 poly bunches), and
// RenderStaticShadows (grouped by type and texture through the render buffer).

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct PolyBunch {
    float verts[7][3];   // +0
    re::Ptr32<PolyBunch> next;     // +0x54
    int16_t numVerts;    // +0x58
    uint8_t u[7];        // +0x5A texture coordinates * 200
    uint8_t v[7];        // +0x61
};
static_assert(offsetof(PolyBunch, next) == 0x54);
static_assert(sizeof(PolyBunch) == 0x68);

struct StaticShadow {
    uint32_t id;             // +0
    re::Ptr32<PolyBunch> polyBunch;    // +4
    uint8_t unknown8[0x28];  // +8
    re::Ptr32<void> texture;           // +0x30
    int16_t intensity;       // +0x34
    uint8_t type;            // +0x36 1 additive, 2 blended, 3 inverted
    uint8_t red;             // +0x37
    uint8_t green;           // +0x38
    uint8_t blue;            // +0x39
    uint8_t justCreated;     // +0x3A
    uint8_t rendered;        // +0x3B
    uint8_t unknown3C[4];
};
static_assert(offsetof(StaticShadow, texture) == 0x30);
static_assert(sizeof(StaticShadow) == 0x40);

// the render buffer's vertex
struct BufferVertex {
    float pos[3];            // +0
    uint32_t unknownC[3];    // +0xC
    uint32_t color;          // +0x18 ARGB
    float u;                 // +0x1C
    float v;                 // +0x20
};
static_assert(sizeof(BufferVertex) == 0x24);

constexpr int kMaxStaticShadows = 20;
constexpr int kNumPolyBunches = 380;
GAME_ARRAY(StaticShadow, aStaticShadows, 0x00C93BE0);
GAME_ARRAY(PolyBunch, aPolyBunches, 0x00C940E0);
GAME_VAR(PolyBunch*, pEmptyBunchList, 0x00C93760);
GAME_ARRAY(int16_t, ShadowIndexList, 0x00C93730);   // 24: a fan over up to 10 points
GAME_VAR(void*, gpShadowCarTex, 0x00C93720);
GAME_VAR(void*, gpShadowPedTex, 0x00C9372C);
GAME_VAR(void*, gpShadowBikeTex, 0x00C9371C);
GAME_VAR(void*, gpShadowExplosionTex, 0x00C93728);
GAME_VAR(void*, gpShadowHeadLightsTex, 0x00C93724);
GAME_VAR(void*, gpShadowLampTex, 0x00C93718);
GAME_VAR(float, ms_fShadowDarkness_CE755C, 0x00CE755C);

GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);
GAME_FN(void*, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __cdecl, RenderBuffer_Clear, 0x0052AE80);
GAME_FN(void, __cdecl, RenderBuffer_StartStoring, 0x0052AE10, int numIndices, int numVertices, re::Ptr32<uint16_t>* indices,
        re::Ptr32<BufferVertex>* vertices, int32_t* firstIndex);   // (an int: the game writes 4 bytes)
GAME_FN(void, __cdecl, RenderBuffer_StopStoring, 0x0049A650);   // (an empty function)
GAME_FN(void, __cdecl, RenderBuffer_RenderStuffInBuffer, 0x0052AE90);

} // namespace

namespace Shadows2 {

// 0x524FF0 (thiscall): CStaticShadow::Free: its polys back on the free list
static void __fastcall StaticShadow_Free(StaticShadow* self, re::Edx) {
    PolyBunch* first = self->polyBunch;
    if (first) {
        PolyBunch* last = first;
        PolyBunch* empty = pEmptyBunchList;
        pEmptyBunchList = first;
        while (last->next) {
            last = last->next;
        }
        last->next = empty;
    }
    self->id = 0;
    self->polyBunch = nullptr;
}

// 0x525030 (cdecl): CShadows::Init
static void __cdecl Init() {
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(Txd_FindTxdSlot("particle", 0));
    gpShadowCarTex = Txd_FindTexture("shad_car", nullptr);
    gpShadowPedTex = Txd_FindTexture("shad_ped", nullptr);
    gpShadowBikeTex = Txd_FindTexture("shad_bike", nullptr);
    gpShadowExplosionTex = Txd_FindTexture("shad_exp", nullptr);
    gpShadowHeadLightsTex = Txd_FindTexture("headlight", nullptr);
    gpShadowLampTex = Txd_FindTexture("lamp_shad_64", nullptr);
    Txd_PopCurrentTxd();
    static const int16_t kIndices[24] = {0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 5, 4, 0, 6, 5, 0, 7, 6, 0, 8, 7, 0, 9, 8};
    for (int i = 0; i < 24; i++) {
        ShadowIndexList[i] = kIndices[i];
    }
    for (int i = 0; i < kMaxStaticShadows; i++) {
        aStaticShadows[i].id = 0;
        aStaticShadows[i].polyBunch = nullptr;
    }
    pEmptyBunchList = aPolyBunches;
    for (int i = 0; i < kNumPolyBunches - 1; i++) {
        aPolyBunches[i].next = &aPolyBunches[i + 1];
    }
    aPolyBunches[kNumPolyBunches - 1].next = nullptr;
}

// 0x5251C0 (cdecl): CShadows::RenderStaticShadows: for each static shadow not drawn
// yet, its blend and raster, then every shadow of the same type and texture (from it
// on) put in the render buffer (alpha: its intensity times 1 - darkness / 2; 3 cm up)
// and marked drawn
static void __cdecl RenderStaticShadows() {
    SetRenderState(8, 0);
    SetRenderState(6, 1);
    SetRenderState(0xC, 1);
    SetRenderState(0xE, 0);
    for (int i = 0; i < kMaxStaticShadows; i++) {
        aStaticShadows[i].rendered = 0;
    }
    for (int i = 0; i < kMaxStaticShadows; i++) {
        StaticShadow& shadow = aStaticShadows[i];
        if (!shadow.polyBunch || shadow.rendered) {
            continue;
        }
        switch (shadow.type) {
            case 1:
                SetRenderState(0xA, 5);
                SetRenderState(0xB, 6);
                break;
            case 2:
                SetRenderState(0xA, 2);
                SetRenderState(0xB, 2);
                break;
            case 3:
                SetRenderState(0xA, 1);
                SetRenderState(0xB, 4);
                break;
            default:
                break;
        }
        SetRenderState(1, int(reinterpret_cast<intptr_t>(GetRaster(shadow.texture))));
        RenderBuffer_Clear();
        for (int j = i; j < kMaxStaticShadows; j++) {
            StaticShadow& other = aStaticShadows[j];
            PolyBunch* poly = other.polyBunch;
            if (!poly || shadow.type != other.type || shadow.texture != other.texture) {
                continue;
            }
            for (; poly; poly = poly->next) {
                re::Ptr32<uint16_t> indices;
                re::Ptr32<BufferVertex> vertices;
                int32_t first;
                RenderBuffer_StartStoring(poly->numVerts * 3 - 6, poly->numVerts, &indices, &vertices, &first);
                int32_t intensity = other.intensity;
                int32_t alpha = int32_t((1.0 - double(ms_fShadowDarkness_CE755C) * 0.5) * intensity);
                for (int k = 0; k < poly->numVerts; k++) {
                    BufferVertex& vertex = vertices[k];
                    vertex.color = uint32_t((((uint32_t(alpha) << 8 | other.red) << 8) | other.green) << 8 | other.blue);
                    vertex.u = float(double(poly->u[k]) * 0.004999999888241291);
                    vertex.v = float(double(poly->v[k]) * 0.004999999888241291);
                    vertex.pos[0] = poly->verts[k][0];
                    vertex.pos[1] = poly->verts[k][1];
                    vertex.pos[2] = float(double(poly->verts[k][2]) + 0.029999999329447746);
                }
                for (int k = 0; k < (poly->numVerts - 2) * 3; k++) {
                    indices[k] = uint16_t(ShadowIndexList[k] + first);
                }
                RenderBuffer_StopStoring();
            }
            other.rendered = 1;
        }
        RenderBuffer_RenderStuffInBuffer();
    }
    SetRenderState(0xC, 0);
    SetRenderState(8, 1);
}

} // namespace Shadows2

// ---- registration ----------------------------------------------------------------------

#define C16_SHADOWS2_REVERSED(name, fn, address)                                                     \
    static void* Original_##fn;                                                                      \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace Shadows2 {

C16_SHADOWS2_REVERSED("CStaticShadow::Free", StaticShadow_Free, 0x00524FF0);
C16_SHADOWS2_REVERSED("CShadows::Init", Init, 0x00525030);
C16_SHADOWS2_REVERSED("CShadows::RenderStaticShadows", RenderStaticShadows, 0x005251C0);

} // namespace Shadows2
