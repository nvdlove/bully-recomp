// CScreenFX (0x55DFE0..0x560E00): the full-screen post effects, done with 13 D3DX
// effects (Shaders\PostProcess\ScrFX_*.fx, compiled to the shader binary folder):
// the glow/bloom, the depth of field, the out-of-focus blur, the motion blur, the
// wobble (being hit), the monochrome filter (the camera's), the colour modulation, the
// heat haze (the gym's fire), mud and tiredness, each from its own settings (some per
// area, from Config\effects.dat). The render targets are Gamebryo's (NiRenderedTexture
// and NiRenderTargetGroup, kept in NiPointers). Also the streaks (10 small bursts of
// lines from a point: 0xCF1714), updated here each frame.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "game/Hud.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CVector {
    float x, y, z;
};

// ---- D3DX (COM: stdcall, `this` first) ----
template <typename R, typename... A>
R Com(void* object, int offset, A... args) {
    return re::ComCall<R>(object, offset / 4, args...);
}
// ID3DXEffect's (by vtable offset)
enum {
    FX_RELEASE = 0x08,
    FX_GET_TECHNIQUE_DESC = 0x14,
    FX_GET_PARAMETER_BY_NAME = 0x24,
    FX_GET_TECHNIQUE_BY_NAME = 0x34,
    FX_GET_INT = 0x6C,
    FX_SET_FLOAT = 0x78,
    FX_SET_FLOAT_ARRAY = 0x80,
    FX_SET_VECTOR = 0x88,
    FX_SET_VECTOR_ARRAY = 0x90,
    FX_SET_MATRIX = 0x98,
    FX_SET_TEXTURE = 0xD0,
    FX_SET_TECHNIQUE = 0xE8,
    FX_BEGIN = 0xFC,
    FX_BEGIN_PASS = 0x100,
    FX_COMMIT_CHANGES = 0x104,
    FX_END_PASS = 0x108,
    FX_END = 0x10C,
};
constexpr int DEVICE_TEST_COOPERATIVE_LEVEL = 0x0C;

void* Param(void* fx, const char* name) {
    return Com<void*>(fx, FX_GET_PARAMETER_BY_NAME, static_cast<void*>(nullptr), name);
}
void SetFloat(void* fx, void* handle, float value) {
    Com<long>(fx, FX_SET_FLOAT, handle, value);
}
void SetTexture(void* fx, void* handle, void* texture) {
    Com<long>(fx, FX_SET_TEXTURE, handle, texture);
}
void SetVector(void* fx, void* handle, const float* v) {
    Com<long>(fx, FX_SET_VECTOR, handle, v);
}

// the effects, by index into ms_effects/ms_effectNames
enum {
    FX_COMBINE, FX_GLOW, FX_LUMINANCE, FX_MODULATE, FX_DEPTH_OF_FIELD, FX_HEAT_HAZE, FX_GAUSSIAN_BLUR,
    FX_BLOOM, FX_OUT_OF_FOCUS, FX_MOTION_BLUR, FX_MONOCHROME, FX_WOBBLE, FX_PASS_THRU, FX_COUNT
};

// ---- the game's ----
GAME_FN(void*, __stdcall, D3DXMatrixMultiply_, 0x0085AC84, float* out, const float* a, const float* b);
GAME_FN(long, __stdcall, D3DXCreateEffect_, 0x0085AC8A, void* device, const void* data, uint32_t size,
        const void* defines, void* include, uint32_t flags, void* pool, void** effect, void** errors);
GAME_FN(float, __cdecl, GetTimeSeconds, 0x00425710);
GAME_FN(uint8_t*, __cdecl, GetDisplaySettings, 0x00405BA0, int32_t* out);  // (+0 width, +4 height; returns `out`)
GAME_FN(uint8_t*, __cdecl, RenderedTexture_Create, 0x0075D960, int width, int height, void* renderer,
        const int32_t* format, int flags);
GAME_FN(uint8_t*, __cdecl, RenderTargetGroup_Create, 0x0075D720, void* buffer, void* renderer, int, int);
GAME_FN(uint8_t*, __cdecl, IsKindOf, 0x00401FF0, void* rtti, void* object);
GAME_FN(void, __cdecl, Renderer_EndTarget, 0x004096C0);
GAME_FN(void, __cdecl, Renderer_BeginTarget, 0x004094E0, void* group, int clear);
GAME_FN(void, __cdecl, Renderer_409520, 0x00409520, int, int);
GAME_FN(void, __cdecl, Renderer_409960, 0x00409960);
GAME_FN(void, __cdecl, Renderer_CopyTarget, 0x004073F0, void* to, void* from);
GAME_FN(void, __cdecl, DrawScreenQuad, 0x0073A550, int, int, void* texture);
GAME_FN(void, __cdecl, DrawScreenQuad2, 0x0073A590, int, int);
GAME_FN(void, __cdecl, DrawScreenQuad3, 0x00739C90, int, int, uintptr_t);
GAME_FN(void*, __cdecl, ShaderLibrary_Get, 0x00756620);
GAME_FN(void, __cdecl, GetFileName_888680, 0x00888680, const char* path, char* out, int size);
GAME_FN(uint8_t*, __thiscall, ShaderLib_Find, 0x00748130, void* library, const char* name);
GAME_FN(void*, __cdecl, NiLoadFile_749590, 0x00749590, const char* path, uint32_t* size);
GAME_FN(void, __cdecl, NiFree, 0x00752380, void* p);
GAME_FN(void, __cdecl, FixPath, 0x00752C20, char* path);
GAME_FN(uint8_t, __cdecl, DirectoryExists, 0x00753440, const char* path);
GAME_FN(void, __cdecl, CreateDirectoryTree, 0x00753430, const char* path);
GAME_FN(uint8_t, __cdecl, FileExists, 0x007533C0, const char* path, int);
GAME_FN(int, __cdecl, FormatString, 0x00402E00, char* buffer, size_t size, const char* format, ...);
GAME_FN(void, __cdecl, CompileEffectFile, 0x00407580, const char* source, const char* target, void* include, uintptr_t);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline void* __cdecl Memcpy(void* to, const void* from, size_t size) { return crt::Memcpy(to, from, size); }
GAME_FN(void, __cdecl, PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, PopCurrentTxd, 0x005F0AA0);
GAME_FN(int, __cdecl, FindTxdSlot, 0x005F22D0, const char* name, int);
GAME_FN(void, __cdecl, SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(uint8_t*, __cdecl, FindTexture, 0x005F1770, const char* name, int);  // (+0x24 its NiTexture, whose +0x5C is the D3D one)
GAME_FN(uint8_t, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(uint8_t*, __cdecl, Camera_GetRaster, 0x006C8740, uintptr_t camera);  // +0xC width, +0x10 height, +0x14 depth
GAME_FN(void*, __cdecl, RwRasterCreate, 0x00519F60, int width, int height, int depth, int flags);
GAME_FN(void, __cdecl, RwRasterDestroy, 0x00519090, void* raster);
GAME_FN(void*, __cdecl, File_Open, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(char*, __cdecl, File_ReadLine, 0x00429C10, void* file);
GAME_FN(int, __cdecl, File_Close, 0x0042D450, void* file);
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(int, __cdecl, GetAreaFromName, 0x004305D0, const char* name);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(void, __cdecl, Empty_49A650, 0x0049A650, uint32_t);   // (just a ret)
GAME_FN(float, __cdecl, Min, 0x00414470, float a, float b);
GAME_FN(void, __cdecl, Photo_GetViewScale, 0x00543D30, float* x, float* y);
inline double __cdecl Crt_85DBF0(double x) { return crt::Ceil(x); }    // (CRT math: rounds up)
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, NormaliseSafe, 0x00415130, CVector* v);
GAME_FN(int32_t, __thiscall, Camera_ProjectToScreen, 0x004F2240, void* camera, const CVector* point, float* out,
        int normalised);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
// CScreenFX's own, where modes apply
GAME_FN(void, __cdecl, FX_Reset, 0x0055DFE0);
GAME_FN(void, __cdecl, FX_LoadAreaSettings, 0x0055E100);
GAME_FN(void, __cdecl, FX_GetEffect, 0x0055E880, int effect);
GAME_FN(void, __cdecl, FX_CreateRasters, 0x0055E8F0);
GAME_FN(void, __cdecl, FX_MakeGaussianKernel, 0x0055E970, uint32_t width, uint32_t height, float* offsets,
        float* weights, float scale);
GAME_FN(void, __cdecl, FX_CreatePhotoTarget, 0x0055EF80);
GAME_FN(void, __cdecl, FX_ReloadEffects, 0x0055F100);
GAME_FN(void, __cdecl, FX_Draw, 0x0055F5B0, int effect, const char* technique, int a, int b, int c);
GAME_FN(void, __cdecl, FX_ReleaseTargets, 0x0055F790);
GAME_FN(void, __cdecl, FX_Modulate, 0x0055F910, uint32_t colour);
GAME_FN(void, __cdecl, FX_HeatHaze, 0x0055F9C0, int strong);
GAME_FN(void, __cdecl, FX_CompileEffect, 0x0055FBB0, const char* name);
GAME_FN(void, __cdecl, FX_LoadEffect, 0x0055FC60, int effect);
GAME_FN(void, __cdecl, FX_GaussianBlur, 0x0055FDE0);
GAME_FN(void, __cdecl, FX_MotionBlur, 0x0055FFE0, int strength, float, void*, int, float, int);
GAME_FN(void, __cdecl, FX_DepthOfField, 0x005601F0, int strength);
GAME_FN(void, __cdecl, FX_OutOfFocus, 0x00560380, int strength, uint8_t* mask, int, int);
GAME_FN(void, __cdecl, FX_Bloom, 0x00560480, int threshold, int glow, int scale, int, int);
GAME_FN(void, __cdecl, FX_Wobble, 0x005608D0, float speed, float strength);
GAME_FN(void, __cdecl, FX_Monochrome, 0x00560A30, const CRGBA* colour);
GAME_FN(void, __cdecl, FX_ClearStreaks, 0x00560BE0);
GAME_FN(void, __cdecl, FX_UpdateStreaks, 0x00560E00);

// ---- game data ----
GAME_VAR(uint8_t*, g_renderer, 0x020E2380);            // NiDX9Renderer (+0xA4 the device, +0x4F8 device lost,
                                                        // +0x790 view, +0x7D0 projection)
GAME_VAR(uint32_t, g_rendererRtti, 0x020E6D20);         // (its class's bit)
GAME_VAR(uint32_t, g_renderedTextureRtti, 0x020E2250);  // (NiRenderedTexture's bit)
inline void* const kRenderedTextureRtti = reinterpret_cast<void*>(0x020E2248);
GAME_VAR(uint8_t*, g_mainTarget, 0x00BCBBAC);            // NiPointers: the scene's render target group
GAME_VAR(uint8_t*, g_sceneTarget, 0x00BCBBB0);           // (+0x24 the texture it renders to)
GAME_VAR(const char*, g_shaderBinaryDirectory, 0x009D3BA8);
GAME_VAR(int32_t, g_currArea, 0x00BD1008);
GAME_VAR(int32_t, g_currentCamera, 0x00C3CCF4);
GAME_VAR(uint8_t, g_bcbb53, 0x00BCBB53);                // (no post effects while set)
GAME_VAR(uint8_t, g_c1a998, 0x00C1A998);
GAME_VAR(uint8_t, g_c936f8, 0x00C936F8);                // not 0xFF: the default glow
GAME_VAR(uint8_t, g_brightness1, 0x020C4824);           // the options' (the glow scaled by them)
GAME_VAR(uint8_t, g_brightness2, 0x020C4825);
GAME_VAR(uint8_t, g_modulateOption, 0x020C4832);
GAME_VAR(uint32_t, g_modulateOptionColour, 0x020C4833);
GAME_VAR(uint8_t*, g_hudPhoto, 0x00C674E0);             // (+0x28 active)

// the settings (the scripts set them)
GAME_VAR(int32_t, ms_glowOn, 0x00AC7608);
GAME_VAR(int32_t, ms_glowThreshold, 0x00AC760C);
GAME_VAR(int32_t, ms_glowStrength, 0x00AC7610);
GAME_VAR(int32_t, ms_glowScale, 0x00AC7614);
GAME_VAR(int32_t, ms_glow4, 0x00AC7618);
GAME_VAR(int32_t, ms_blurStrength, 0x00AC761C);
GAME_VAR(float, ms_blurFloat, 0x00AC7620);
GAME_VAR(int32_t, ms_dofOn, 0x00AC7624);
GAME_VAR(uint8_t, ms_dofStrength, 0x00AC7628);
GAME_VAR(uint32_t, ms_wobbleColour, 0x00AC762C);        // (a CRGBA; its alpha from the wobble)
GAME_VAR(uint32_t, ms_modulateColour, 0x00AC7630);
GAME_VAR(float, ms_gymFireAlpha, 0x00AC7634);
GAME_VAR(CRGBA, ms_filterColour, 0x00AC7638);
GAME_VAR(float, ms_tirednessSpeed, 0x00AC763C);
GAME_VAR(uint32_t, ms_gymFireColour, 0x00AC7640);
GAME_VAR(uint8_t, ms_gymFireAlphaByte, 0x00AC7643);
GAME_VAR(uint8_t, ms_resetMotionBlur, 0x00AC76AC);
GAME_ARRAY(const char*, ms_effectNames, 0x00AC7678);   // [13]
GAME_VAR(int32_t, ms_heatTxd, 0x00AC76F4);
GAME_VAR(float*, ms_kernel, 0x00AC76F8);
GAME_VAR(int32_t, ms_wobbleTxd, 0x00AC76FC);
GAME_VAR(int32_t, ms_blurOn, 0x00CF1068);
GAME_VAR(int32_t, ms_blurTexture, 0x00CF106C);          // <0: one of the masks (-1, -2)
GAME_VAR(int32_t, ms_outOfFocusOn, 0x00CF1070);
GAME_VAR(int32_t, ms_outOfFocusStrength, 0x00CF1074);
GAME_VAR(int32_t, ms_outOfFocusMask, 0x00CF1078);
GAME_VAR(int32_t, ms_wobbleOn, 0x00CF107C);
GAME_VAR(float, ms_wobble, 0x00CF1080);
GAME_VAR(int32_t, ms_modulateMode, 0x00CF1084);
GAME_VAR(int32_t, ms_int_CF1088, 0x00CF1088);
GAME_VAR(int32_t, ms_filterSet, 0x00CF108C);
GAME_VAR(int32_t, ms_gymFireOn, 0x00CF1090);
GAME_VAR(int32_t, ms_blindedByMud, 0x00CF1094);
GAME_VAR(int32_t, ms_blindedByMudTime, 0x00CF1098);
GAME_VAR(int32_t, ms_tirednessOn, 0x00CF109C);
GAME_VAR(float, ms_tiredness, 0x00CF10A0);
GAME_ARRAY(uint32_t, ms_wordsCF10A0, 0x00CF10A0);        // (a negative texture index reads here: -1 and -2 are the masks)
GAME_ARRAY(uint8_t*, ms_masks, 0x00CF10A4);             // [2] zoommask, zoommask2
GAME_VAR(float, ms_tirednessPhase, 0x00CF10B0);
GAME_ARRAY(float, ms_blurOffsets, 0x00CF10D0);          // [13][4] the Gaussian's sample offsets
GAME_ARRAY(float, ms_blurWeights, 0x00CF11A0);          // [13][4] and weights
GAME_ARRAY(void*, ms_effects, 0x00CF1270);              // [13] ID3DXEffect*
GAME_VAR(int32_t, ms_int_CF12A4, 0x00CF12A4);
GAME_VAR(int32_t, ms_int_CF12A8, 0x00CF12A8);
GAME_VAR(int32_t, ms_int_CF12AC, 0x00CF12AC);
GAME_VAR(int32_t, ms_int_CF12B0, 0x00CF12B0);
GAME_VAR(int32_t, ms_int_CF12B4, 0x00CF12B4);
GAME_VAR(int32_t, ms_rasterHeight, 0x00CF12B8);
GAME_VAR(int32_t, ms_rasterWidth, 0x00CF12BC);
GAME_VAR(int32_t, ms_int_CF12C0, 0x00CF12C0);
GAME_VAR(int32_t, ms_int_CF12C4, 0x00CF12C4);
GAME_VAR(void*, ms_raster1, 0x00CF12C8);
GAME_VAR(void*, ms_raster2, 0x00CF12CC);
GAME_VAR(void*, ms_sceneTexture, 0x00CF12DC);           // the D3D texture the effects read
GAME_VAR(uint8_t, ms_blurDone, 0x00CF12E0);             // the Gaussian's pass done this frame
// the render targets (NiPointers)
GAME_VAR(uint8_t*, ms_halfTexture, 0x00CF12EC);
GAME_VAR(uint8_t*, ms_halfTarget, 0x00CF12F0);
GAME_VAR(uint8_t*, ms_quarterTexture, 0x00CF12F4);
GAME_VAR(uint8_t*, ms_quarterTexture2, 0x00CF12F8);
GAME_VAR(uint8_t*, ms_quarterTarget2, 0x00CF12FC);
GAME_VAR(uint8_t*, ms_fullTexture, 0x00CF1300);
GAME_VAR(uint8_t*, ms_backBufferTarget, 0x00CF1304);
GAME_VAR(uint8_t*, ms_photoTexture, 0x00CF1308);
GAME_VAR(uint8_t*, ms_photoTarget, 0x00CF130C);

// the per-area glow, from Config\effects.dat (64 areas)
struct AreaGlow {
    int32_t enabled;    // +0
    int32_t absolute;   // +4 0: the first two are scaled by the brightness options
    int32_t threshold;  // +8
    int32_t strength;   // +0xC
    int32_t scale;      // +0x10
    int32_t unknown14;  // +0x14
};
static_assert(sizeof(AreaGlow) == 0x18);
GAME_ARRAY(AreaGlow, ms_areaGlow, 0x00CF0A68);

// the streaks
struct Streak {
    CVector origin;      // +0x00
    CVector points[10];  // +0x0C the lines' ends, moving out
    float lengths[10];   // +0x84 each line's step (shrinking)
    uint8_t unknownAC;   // +0xAC (0x80)
    uint8_t padAD[3];
    float size;          // +0xB0
    CVector dir;         // +0xB4 (normalised)
    CVector spread;      // +0xC0 (1 - |dir|) * 2, per axis (the given dir's)
    float maxLength;     // +0xCC
    int32_t count;       // +0xD0 lines (4..10), 0: free
};
static_assert(offsetof(Streak, lengths) == 0x84);
static_assert(offsetof(Streak, size) == 0xB0);
static_assert(offsetof(Streak, count) == 0xD0);
static_assert(sizeof(Streak) == 0xD4);
GAME_VAR(int32_t, ms_numStreaks, 0x00CF1714);
GAME_ARRAY(Streak, ms_streaks, 0x00CF1718);          // [10]
GAME_VAR(uint8_t*, ms_streakTexture, 0x00CF1710);

// ---- NiPointer handling (the game's inlined NiRefObject counts, through its imports) ----
void DeleteThis(uint8_t* object) {
    vc::Call<void>(object, 0x04);
}
void Assign(re::Ptr32<uint8_t>& slot, uint8_t* value) {
    uint8_t* old = slot;
    if (old != value) {
        if (old && vc::Decrement(old + 4) == 0) {
            DeleteThis(old);
        }
        slot = value;
        if (value) {
            vc::Increment(value + 4);
        }
    }
}
void ReleaseSlot(re::Ptr32<uint8_t>& slot) {
    uint8_t* old = slot;
    if (old) {
        if (vc::Decrement(old + 4) == 0) {
            DeleteThis(old);
        }
        slot = nullptr;
    }
}
// a temporary NiPointer's copy made and dropped
void Touch(uint8_t* object) {
    if (object) {
        vc::Increment(object + 4);
        if (vc::Decrement(object + 4) == 0) {
            DeleteThis(object);
        }
    }
}
// the render target group begun through a temporary NiPointer
void BeginMainTarget() {
    uint8_t* target = g_mainTarget;
    if (target) {
        vc::Increment(target + 4);
    }
    Renderer_BeginTarget(target, 0);
    if (target && vc::Decrement(target + 4) == 0) {
        DeleteThis(target);
    }
}
// NiObject::IsKindOf inlined (the class's bit in the object's RTTI)
uint8_t* KindOf(uint8_t* object, uint32_t bit) {
    if (!object) {
        return nullptr;
    }
    uint8_t* rtti = vc::Call<uint8_t*>(object, 0x08);
    uint32_t bits = *reinterpret_cast<uint32_t*>(rtti + 0xC + (bit >> 5) * 4);
    return (bits & (1u << (bit & 0x1F))) ? object : nullptr;
}
// the scene target's texture (a NiRenderedTexture; its +0x5C the D3D one)
void* SceneD3DTexture() {
    uint8_t* target = g_sceneTarget;
    Touch(target);
    uint8_t* texture = KindOf(*reinterpret_cast<re::Ptr32<uint8_t>*>(target + 0x24), g_renderedTextureRtti);
    return *reinterpret_cast<re::Ptr32<void>*>(texture + 0x5C);
}
void* D3DTextureOf(uint8_t* niTexture) {
    return *reinterpret_cast<re::Ptr32<void>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(niTexture + 0x24) + 0x5C);
}
// a texture's NiTexture's D3D one (FindTexture's result)
void* D3DTextureOfTexture(uint8_t* texture) {
    return *reinterpret_cast<re::Ptr32<void>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(texture + 0x24) + 0x5C);
}
void EndEffect() {
    Renderer_EndTarget();
    Renderer_409520(0, 0);
}
uint8_t* CreateTarget(uint8_t* texture) {
    void* buffer = vc::Call<void*>(texture, 0x50);   // its Ni2DBuffer
    return RenderTargetGroup_Create(buffer, g_renderer, 1, 0);
}

// ---- 0x55DFE0: the settings to their defaults
void __cdecl Reset() {
    ms_blurFloat = 0.1f;
    ms_wobble = 0.0f;
    ms_glowOn = 1;
    ms_glow4 = 1;
    ms_gymFireAlpha = 1.0f;
    ms_dofOn = 1;
    ms_tiredness = 0.0f;
    ms_wobbleColour = 0xFFD2FFBE;  // (0xBE, 0xFF, 0xD2, 0xFF)
    ms_glowThreshold = 0xE6;
    ms_glowStrength = 0x50;
    ms_glowScale = 4;
    ms_blurOn = 0;
    ms_blurStrength = 0x5A;
    ms_dofStrength = 0x78;
    ms_outOfFocusOn = 0;
    ms_outOfFocusStrength = 0;
    ms_wobbleOn = 0;
    ms_modulateMode = 0;
    ms_int_CF1088 = 0;
    ms_modulateColour = 0xFFFFFFFF;
    ms_filterSet = 0;
    ms_gymFireOn = 0;
    ms_blindedByMud = 0;
    ms_blindedByMudTime = 0;
    ms_tirednessOn = 0;
    *reinterpret_cast<uint32_t*>(&ms_filterColour) = 0xFFFFFFFF;
}

// ---- 0x55E0D0: the targets and the two masks let go
uint32_t __cdecl Shutdown() {
    FX_ReleaseTargets();
    for (int i = 0; i < 2; i++) {
        NiRelease(ms_masks[i]);
    }
    return 1;
}

// ---- 0x55E100: Config\effects.dat: per area, "name on absolute threshold strength
// scale x" (# comments)
void __cdecl LoadAreaSettings() {
    void* file = File_Open("Config\\effects.dat", "r", 1);
    int32_t on = 0, absolute = 0, threshold = 0, strength = 0, scale = 0, x = 0;
    char name[0x30];
    for (char* line = File_ReadLine(file); line; line = File_ReadLine(file)) {
        if (*line == '\0' || *line == '#') {
            continue;
        }
        Sscanf(line, "%s %d %d %d %d %d %d", name, &on, &threshold, &strength, &scale, &x, &absolute);
        int area = GetAreaFromName(name);
        if (area >= 0) {
            AreaGlow& g = ms_areaGlow[area];
            g.threshold = threshold;
            g.scale = scale;
            g.enabled = on != 0;
            g.strength = strength;
            g.unknown14 = x;
            g.absolute = absolute != 0;
        }
    }
    File_Close(file);
}

// ---- 0x55E210: the masks (particle.txd), the rasters, the streaks and the area settings
uint32_t __cdecl Initialise() {
    PushCurrentTxd();
    SetCurrentTxd(FindTxdSlot("particle", 0));
    char names[2][10] = {"zoommask", "zoommask2"};
    for (int i = 0; i < 2; i++) {
        ms_masks[i] = FindTexture(names[i], 0);
    }
    PopCurrentTxd();
    FX_CreateRasters();
    FX_ClearStreaks();
    FX_LoadAreaSettings();
    return 1;
}

uint32_t MaskTexture(int32_t index) {
    // (a negative index: the mask at CF10A0 + |index| * 4)
    return index < 0 ? ms_wordsCF10A0[std::abs(index)] : uint32_t(index);
}

// ---- 0x55E2C0: the frame's effects, then the scene drawn to the screen
void __cdecl Render() {
    if (g_bcbb53 || g_c1a998) {
        return;
    }
    FX_ReloadEffects();
    // (ours: the texture the effects read, made the scene's now. The game's Modulate and
    // Monochrome read whatever the last frame left (the scene's, by the glow's end), and a
    // device reset since (a resized window: ApplyVideoMode) has let that texture go: drawing
    // with it crashed in d3d9)
    ms_sceneTexture = SceneD3DTexture();
    ms_int_CF12C0 = 0;
    ms_int_CF12C4 = 0;
    ms_int_CF12AC = 0;
    ms_int_CF12A8 = 0;
    ms_blurDone = 0;
    int32_t threshold, strength, scale, glow4;
    if (uint32_t(g_currArea) <= 0x3F) {
        const AreaGlow& g = ms_areaGlow[g_currArea];
        ms_glowOn = g.enabled;
        if (g.enabled) {
            if (g.absolute == 0) {
                threshold = int(double(g.threshold) * (double(g_brightness1) / 255.0));
                strength = int(double(g.strength) * (double(g_brightness2) / 255.0));
            } else {
                threshold = g.threshold;
                strength = g.strength;
            }
            scale = g.scale;
            glow4 = g.unknown14;
        } else {
            glow4 = ms_glow4;
            scale = ms_glowScale;
            strength = ms_glowStrength;
            threshold = ms_glowThreshold;
        }
    } else {
        ms_glowOn = 0;
        glow4 = ms_glow4;
        scale = ms_glowScale;
        strength = ms_glowStrength;
        threshold = ms_glowThreshold;
    }
    if (g_c936f8 != 0xFF) {
        strength = 0x50;
        threshold = 0xE6;
        scale = 4;
        glow4 = 1;
    }
    ms_glowThreshold = threshold & 0xFF;
    ms_glowStrength = strength & 0xFF;
    ms_glowScale = scale > 0 ? scale & 0xFF : 1;
    ms_glow4 = glow4 & 0xFF;
    ms_modulateMode = g_modulateOption;
    ms_modulateColour = g_modulateOptionColour;
    uint8_t photoActive = g_hudPhoto[0x28];
    FX_UpdateStreaks();

    if (ms_gymFireOn) {
        ms_gymFireAlphaByte = uint8_t(int(double(ms_gymFireAlpha) * 255.0));
        FX_Modulate(ms_gymFireColour);
        FX_HeatHaze(!(0.5f > ms_gymFireAlpha));
    }
    if (ms_filterSet) {
        FX_Monochrome(&ms_filterColour);
    } else if (ms_modulateMode == 1) {
        FX_Modulate(ms_modulateColour);
    } else if (ms_modulateMode == 2) {
        Empty_49A650(ms_modulateColour);
    }
    if (ms_dofOn && IsAreaInterior(g_currArea)) {
        FX_DepthOfField(ms_dofStrength);
    }
    if (ms_outOfFocusOn) {
        FX_OutOfFocus(ms_outOfFocusStrength, reinterpret_cast<uint8_t*>(uintptr_t(MaskTexture(ms_outOfFocusMask))), 0x50,
                      0);
    }
    if (ms_glowOn) {
        FX_Bloom(ms_glowThreshold, ms_glowStrength, ms_glowScale, ms_glow4, 0);
    }
    if (ms_wobbleOn && ms_wobble > 0.01f) {
        // being hit: the wobble, a motion blur and the colour
        float w = ms_wobble;
        FX_Wobble(float(double(w) * 6.0), float(double(w) * 1.5));
        float blur = float(double(ms_wobble) * 0.10000000149011612);
        FX_MotionBlur(int(double(ms_wobble) * 100.0), blur, nullptr, 1, 0.0f, 0);
        uint32_t colour = ms_wobbleColour;
        colour = (colour & 0x00FFFFFF) | (uint32_t(uint8_t(int(double(ms_wobble) * 255.0))) << 24);
        FX_Modulate(colour);
    } else if (ms_blindedByMud) {
        ms_blindedByMudTime -= 2;
        if (ms_blindedByMudTime == 0) {
            ms_blindedByMud = 0;
        } else {
            CRGBA mud;
            mud.r = 0xC8;
            mud.g = 0xA0;
            mud.b = 0x6E;
            mud.a = uint8_t(int(Min(float(ms_blindedByMudTime * 3), 255.0f)));
            FX_Monochrome(&mud);
            Empty_49A650(uint32_t(ms_blindedByMudTime));
            uint32_t packed;
            std::memcpy(&packed, &mud, 4);
            FX_Modulate(packed);
        }
    } else if (ms_tirednessOn && !photoActive) {
        // tired: a motion blur swinging between 0 and 0.1 on mask 2
        float tiredness = ms_tiredness;
        float speed = ms_tirednessSpeed;
        ms_tirednessPhase = float(double(ms_tirednessPhase) + double(speed) * double(tiredness));
        float phase = ms_tirednessPhase;
        if (double(phase) > 0.10000000149011612 || !(0.0f < phase)) {
            ms_tirednessSpeed = -speed;
            ms_tirednessPhase = phase + ms_tirednessSpeed;
            phase = ms_tirednessPhase;
        }
        void* mask = reinterpret_cast<void*>(uintptr_t(ms_wordsCF10A0[2]));
        float blur = Min(float(double(tiredness) * 130.0), 100.0f);
        FX_MotionBlur(int(blur), phase, mask, 1, 0.0f, 0);
    } else if (ms_blurOn) {
        void* texture = reinterpret_cast<void*>(uintptr_t(MaskTexture(ms_blurTexture)));
        FX_MotionBlur(ms_blurStrength, ms_blurFloat, texture, 1, 0.0f, 0);
    }

    uint8_t* target = g_sceneTarget;
    Touch(target);
    vc::Call<void>(target, 0x50);
    uint8_t* texture = IsKindOf(kRenderedTextureRtti, *reinterpret_cast<re::Ptr32<void>*>(target + 0x24));
    DrawScreenQuad(0, 0, *reinterpret_cast<re::Ptr32<void>*>(texture + 0x5C));
}

// ---- 0x55E850: the motion blur starts afresh next time
void __cdecl ResetMotionBlur() {
    ms_resetMotionBlur = 1;
}

// ---- 0x55E860 (ID3DXInclude::Close, stdcall)
long __stdcall ShaderInclude_Close(void*, void* data) {
    if (data) {
        Free(data);
    }
    return 0;
}

// ---- 0x55E880: an effect from Gamebryo's effect shader library (when not made yet)
void __cdecl GetEffect(int effect) {
    re::Ptr32<void>& slot = ms_effects[effect];
    if (slot) {
        return;
    }
    void* library = vc::Call<void*>(ShaderLibrary_Get(), 0x2C, "NiD3DXEffectShaderLib");
    char name[0x104];
    GetFileName_888680(ms_effectNames[effect], name, 0x104);
    uint8_t* found = ShaderLib_Find(library, name);
    if (found) {
        slot = *reinterpret_cast<re::Ptr32<void>*>(found + 0x4C);
    }
}

// ---- 0x55E8F0: the camera's raster size and two small rasters
void __cdecl CreateRasters() {
    uint8_t* raster = Camera_GetRaster(g_currentCamera);
    ms_rasterWidth = *reinterpret_cast<int32_t*>(raster + 0xC);
    ms_rasterHeight = *reinterpret_cast<int32_t*>(raster + 0x10);
    ms_int_CF12A4 = 0x60;
    ms_int_CF12B0 = 0;
    ms_int_CF12B4 = (*reinterpret_cast<int32_t*>(raster + 0x14) != 0x20) + 1;
    ms_raster1 = RwRasterCreate(0x10, 0x10, 0x20, 0x504);
    ms_raster2 = RwRasterCreate(8, 2, 0x20, 0x504);
}

// ---- 0x55E970: a 13-tap Gaussian (the diamond |x| + |y| <= 2): each tap's offset in
// texels and weight, the weights normalised to `scale`
void __cdecl MakeGaussianKernel(uint32_t width, uint32_t height, float* offsets, float* weights, float scale) {
    float texelX = float(1.0 / double(width));
    float texelY = float(1.0 / double(height));
    float sum = 0.0f;
    int n = 0;   // (in floats)
    for (int x = -2; x <= 2; x++) {
        float fx = float(x);
        float ax = std::fabs(fx);
        for (int y = -2; y <= 2; y++) {
            float fy = float(y);
            float ay = std::fabs(fy);
            if (double(ay) + double(ax) > 2.0) {
                continue;
            }
            offsets[n + 0] = fx * texelX;
            offsets[n + 1] = fy * texelY;
            offsets[n + 2] = 0.0f;
            offsets[n + 3] = 0.0f;
            float e = float(std::exp(double(float((double(fx) * fx + double(fy) * fy) * -0.5))));
            float root = float(std::sqrt(6.2831854820251465));
            float w = float(double(e) / double(root));
            weights[n + 0] = w;
            weights[n + 1] = w;
            weights[n + 2] = w;
            weights[n + 3] = w;
            sum = float(double(w) + sum);
            n += 4;
        }
    }
    for (int i = 0; i < n; i++) {
        weights[i] = float(double(weights[i]) / sum * scale);
    }
}

const int32_t kTargetFormat[3] = {6, 2, 2};   // (the NiTexture::FormatPrefs the targets are made with)

// ---- 0x55EB70: the render targets (half, quarter and full size, the back buffer's
// group), the effects cleared, the Gaussian's kernel for the half size
void __cdecl CreateTargets() {
    int32_t format[3] = {kTargetFormat[0], kTargetFormat[1], kTargetFormat[2]};
    int32_t a[6], b[6];
    int32_t w = *reinterpret_cast<int32_t*>(GetDisplaySettings(a));
    int32_t h = *reinterpret_cast<int32_t*>(GetDisplaySettings(b) + 4);
    Assign(ms_halfTexture, RenderedTexture_Create(w / 2, h / 2, g_renderer, format, 0x100));
    Assign(ms_halfTarget, CreateTarget(ms_halfTexture));
    w = *reinterpret_cast<int32_t*>(GetDisplaySettings(b));
    h = *reinterpret_cast<int32_t*>(GetDisplaySettings(a) + 4);
    Assign(ms_quarterTexture, RenderedTexture_Create(w / 4, h / 4, g_renderer, format, 0x100));
    w = *reinterpret_cast<int32_t*>(GetDisplaySettings(b));
    h = *reinterpret_cast<int32_t*>(GetDisplaySettings(a) + 4);
    Assign(ms_quarterTexture2, RenderedTexture_Create(w / 4, h / 4, g_renderer, format, 0x100));
    Assign(ms_quarterTarget2, CreateTarget(ms_quarterTexture2));
    w = *reinterpret_cast<int32_t*>(GetDisplaySettings(b));
    h = *reinterpret_cast<int32_t*>(GetDisplaySettings(a) + 4);
    Assign(ms_fullTexture, RenderedTexture_Create(w, h, g_renderer, format, 0x100));
    uint8_t* renderer = KindOf(g_renderer, g_rendererRtti);
    void* backBuffer = vc::Call<void*>(renderer, 0x80);
    Assign(ms_backBufferTarget, RenderTargetGroup_Create(backBuffer, g_renderer, 1, 0));
    for (int i = 0; i < FX_COUNT; i++) {
        ms_effects[i] = nullptr;
    }
    w = *reinterpret_cast<int32_t*>(GetDisplaySettings(b));
    h = *reinterpret_cast<int32_t*>(GetDisplaySettings(a) + 4);
    FX_MakeGaussianKernel(uint32_t(w / 2), uint32_t(h / 2), ms_blurOffsets, ms_blurWeights, 1.0f);
    FX_GetEffect(0);
}

// ---- 0x55EF60: the effects not had yet, from the library
void __cdecl GetMissingEffects() {
    for (int i = 0; i < FX_COUNT; i++) {
        if (!ms_effects[i]) {
            FX_GetEffect(i);
        }
    }
}

// ---- 0x55EF80: the camera's target at its view scale (sizes rounded up to 8s)
void __cdecl CreatePhotoTarget() {
    int32_t format[3] = {kTargetFormat[0], kTargetFormat[1], kTargetFormat[2]};
    float sx, sy;
    Photo_GetViewScale(&sx, &sy);
    int32_t settings[6];
    float fw = float(double(*reinterpret_cast<int32_t*>(GetDisplaySettings(settings))) * sx);
    int32_t w = int(float(Crt_85DBF0(double(fw))));
    float fh = float(double(*reinterpret_cast<int32_t*>(GetDisplaySettings(settings) + 4)) * sy);
    int32_t h = int(float(Crt_85DBF0(double(fh))));
    if (int32_t r = w % 8) {
        w += 8 - r;
    }
    if (int32_t r = h % 8) {
        h += 8 - r;
    }
    Assign(ms_photoTexture, RenderedTexture_Create(w, h, g_renderer, format, 0x100));
    Assign(ms_photoTarget, CreateTarget(ms_photoTexture));
}

// ---- 0x55F100: every effect got again
void __cdecl ReloadEffects() {
    for (int i = 0; i < FX_COUNT; i++) {
        ms_effects[i] = nullptr;
        FX_GetEffect(i);
    }
}

// ---- 0x55F130: the targets let go and made again (a device reset)
void __cdecl RecreateTargets() {
    ReleaseSlot(ms_photoTarget);
    ReleaseSlot(ms_photoTexture);
    ReleaseSlot(ms_halfTarget);
    ReleaseSlot(ms_halfTexture);
    ReleaseSlot(ms_backBufferTarget);
    ReleaseSlot(ms_fullTexture);
    ReleaseSlot(ms_quarterTarget2);
    ReleaseSlot(ms_quarterTexture2);
    ReleaseSlot(ms_quarterTexture);
    FX_CreatePhotoTarget();
    int32_t format[3] = {kTargetFormat[0], kTargetFormat[1], kTargetFormat[2]};
    int32_t a[6], b[6];
    int32_t w = *reinterpret_cast<int32_t*>(GetDisplaySettings(a));
    int32_t h = *reinterpret_cast<int32_t*>(GetDisplaySettings(b) + 4);
    Assign(ms_halfTexture, RenderedTexture_Create(w / 2, h / 2, g_renderer, format, 0x100));
    Assign(ms_halfTarget, CreateTarget(ms_halfTexture));
    w = *reinterpret_cast<int32_t*>(GetDisplaySettings(b));
    h = *reinterpret_cast<int32_t*>(GetDisplaySettings(a) + 4);
    Assign(ms_quarterTexture, RenderedTexture_Create(w / 4, h / 4, g_renderer, format, 0x100));
    w = *reinterpret_cast<int32_t*>(GetDisplaySettings(b));
    h = *reinterpret_cast<int32_t*>(GetDisplaySettings(a) + 4);
    Assign(ms_quarterTexture2, RenderedTexture_Create(w / 4, h / 4, g_renderer, format, 0x100));
    Assign(ms_quarterTarget2, CreateTarget(ms_quarterTexture2));
    w = *reinterpret_cast<int32_t*>(GetDisplaySettings(b));
    h = *reinterpret_cast<int32_t*>(GetDisplaySettings(a) + 4);
    Assign(ms_fullTexture, RenderedTexture_Create(w, h, g_renderer, format, 0x100));
    void* backBuffer = vc::Call<void*>(g_renderer, 0x80);   // (no IsKindOf here)
    Assign(ms_backBufferTarget, RenderTargetGroup_Create(backBuffer, g_renderer, 1, 0));
    ms_sceneTexture = nullptr;   // (ours: the old one let go by the reset; Render takes the new)
}

// ---- 0x55F5B0: an effect drawn over the screen (a technique, default
// "DefaultTechnique"), reading the scene texture; each pass draws a quad (a, b: its
// arguments; c: the other quad's)
void __cdecl DrawEffect(int effect, const char* technique, int a, int b, int c) {
    uint8_t* renderer = g_renderer;
    void* scene = ms_sceneTexture;
    if (!scene || !*reinterpret_cast<re::Ptr32<void>*>(scene) || renderer[0x4F8]) {
        return;
    }
    void* device = *reinterpret_cast<re::Ptr32<void>*>(renderer + 0xA4);
    if (Com<long>(device, DEVICE_TEST_COOPERATIVE_LEVEL) != 0) {
        return;
    }
    void* fx = ms_effects[effect];
    void* tech = Com<void*>(fx, FX_GET_TECHNIQUE_BY_NAME, technique ? technique : "DefaultTechnique");
    uint32_t desc[3];   // (D3DXTECHNIQUE_DESC, unused)
    Com<long>(fx, FX_GET_TECHNIQUE_DESC, tech, desc);
    void* h = Param(fx, "gSceneTexture");
    if (Com<long>(fx, FX_SET_TEXTURE, h, ms_sceneTexture) < 0) {
        return;
    }
    void* hTime = Param(fx, "gTime");
    if (hTime) {
        SetFloat(fx, hTime, GetTimeSeconds());
    }
    Renderer_409960();
    void* hViewProjection = Param(fx, "gViewProjection");
    alignas(16) float view[16];
    alignas(16) float projection[16];
    alignas(16) float viewProjection[16];
    std::memcpy(view, g_renderer + 0x790, sizeof(view));
    std::memcpy(projection, g_renderer + 0x7D0, sizeof(projection));
    D3DXMatrixMultiply_(viewProjection, view, projection);
    Com<long>(fx, FX_SET_MATRIX, hViewProjection, viewProjection);
    Com<long>(fx, FX_SET_TECHNIQUE, tech);
    uint32_t passes = 0;
    Com<long>(fx, FX_BEGIN, &passes, 0u);
    for (uint32_t i = 0; i < passes; i++) {
        Com<long>(fx, FX_BEGIN_PASS, i);
        Com<long>(fx, FX_COMMIT_CHANGES);
        if (c) {
            DrawScreenQuad3(a, b, c);
        } else {
            DrawScreenQuad2(a, b);
        }
        Com<long>(fx, FX_END_PASS);
    }
    Com<long>(fx, FX_END);
}

// ---- 0x55F790: the rasters and targets let go
void __cdecl ReleaseTargets() {
    RwRasterDestroy(ms_raster2);
    RwRasterDestroy(ms_raster1);
    ReleaseSlot(ms_photoTarget);
    ReleaseSlot(ms_photoTexture);
    ReleaseSlot(ms_halfTarget);
    ReleaseSlot(ms_halfTexture);
    ReleaseSlot(ms_backBufferTarget);
    ReleaseSlot(ms_fullTexture);
    ReleaseSlot(ms_quarterTarget2);
    ReleaseSlot(ms_quarterTexture2);
    ReleaseSlot(ms_quarterTexture);
    ms_sceneTexture = nullptr;
}

void ColourVector(const uint8_t* c, float* v) {
    for (int i = 0; i < 4; i++) {
        v[i] = float(double(c[i]) / 255.0);
    }
}

// ---- 0x55F910: the screen multiplied by a colour
void __cdecl Modulate(uint32_t colour) {
    void* fx = ms_effects[FX_MODULATE];
    void* h = Param(fx, "gColor");
    float v[4];
    ColourVector(reinterpret_cast<const uint8_t*>(&colour), v);
    SetVector(fx, h, v);
    FX_Draw(FX_MODULATE, nullptr, 0, 0, 0);
    EndEffect();
}

// ---- 0x55F9C0: the heat haze (the gym's fire): two ripple textures, stronger when set
void __cdecl HeatHaze(int strong) {
    void* fx = ms_effects[FX_HEAT_HAZE];
    PushCurrentTxd();
    if (ms_heatTxd == -1) {
        ms_heatTxd = FindTxdSlot("EffectTextures", 0);
    }
    SetCurrentTxd(ms_heatTxd);
    uint8_t* ripple1 = FindTexture("HeatRipple_01", 0);
    uint8_t* ripple2 = FindTexture("HeatRipple_02", 0);
    void* h1 = Param(fx, "gPrimaryHeatHazeTexture");
    void* h2 = Param(fx, "gSecondaryHeatHazeTexture");
    SetTexture(fx, h1, D3DTextureOfTexture(ripple1));
    SetTexture(fx, h2, D3DTextureOfTexture(ripple2));
    void* h = Param(fx, "gHeatIntensity");
    SetFloat(fx, h, strong ? 0.01f : 0.005f);
    FX_Draw(FX_HEAT_HAZE, nullptr, 0, 0, 0);
    EndEffect();
    PopCurrentTxd();
}

// ---- 0x55FAD0 (ID3DXInclude::Open, stdcall): an include from Shaders\PostProcess, read
// whole (up to 32K) into a new block. (A failed read leaves the file open.)
long __stdcall ShaderInclude_Open(void*, int, const char* name, const void*, re::Ptr32<void>* data, uint32_t* bytes) {
    char path[0x104];
    char buffer[0x7FFF];
    FormatString(path, 0x104, "%s\\%s", "Shaders\\PostProcess", name);
    FixPath(path);
    void* file = vc::Import<void*>(0x008FC064, static_cast<const char*>(path), 0x80000000u, 0u, static_cast<void*>(nullptr),
                                   3u, 0u, static_cast<void*>(nullptr));   // CreateFileA
    if (file == reinterpret_cast<void*>(intptr_t(-1))) {
        return int32_t(0x80004005);
    }
    uint32_t read = 0;
    if (!vc::Import<int>(0x008FC06C, file, static_cast<void*>(buffer), 0x7FFFu, &read,
                         static_cast<void*>(nullptr))) {   // ReadFile
        return int32_t(0x80004005);
    }
    void* block = Malloc(read);
    *data = block;
    Memcpy(block, buffer, read);
    *bytes = read;
    vc::Import<int>(0x008FC050, file);   // CloseHandle
    return 0;
}

// the include object CompileEffectFile is given (just the vtable: Open, Close)
struct ShaderInclude {
    void* vtable;
};

// ---- 0x55FBB0: Shaders\PostProcess\<name>.fxl compiled to <binary folder>\<name>.fxb
void __cdecl CompileEffect(const char* name) {
    char source[0x104];
    FormatString(source, 0x104, "%s\\%s.fxl", "Shaders\\PostProcess", name);
    FixPath(source);
    if (!DirectoryExists(g_shaderBinaryDirectory)) {
        CreateDirectoryTree(g_shaderBinaryDirectory);
    }
    char target[0x104];
    FormatString(target, 0x104, "%s\\%s.fxb", g_shaderBinaryDirectory, name);
    FixPath(target);
    ShaderInclude include = {reinterpret_cast<void*>(0x0091B838)};
    CompileEffectFile(source, target, &include, 0);
}

// ---- 0x55FC60: an effect made from its .fxb (compiled first when there's none)
void __cdecl LoadEffect(int effect) {
    void* device = *reinterpret_cast<re::Ptr32<void>*>(g_renderer + 0xA4);
    re::Ptr32<void>* slot = &ms_effects[effect];
    if (*slot) {
        Com<unsigned long>(*slot, FX_RELEASE);
        *slot = nullptr;
    }
    char path[0x104];
    FormatString(path, 0x104, "%s\\%s.fxb", g_shaderBinaryDirectory, ms_effectNames[effect]);
    FixPath(path);
    if (!FileExists(path, 0)) {
        FX_CompileEffect(ms_effectNames[effect]);
        if (!FileExists(path, 0)) {
            return;
        }
    }
    uint32_t size = 0;
    void* data = NiLoadFile_749590(path, &size);
    if (!data) {
        return;
    }
    re::Ptr32<void> errors = nullptr;
    PushMemoryTag(0x3C);
    long hr = D3DXCreateEffect_(device, data, size, nullptr, nullptr, 0, nullptr, re::OutPtr(slot), re::OutPtr(&errors));
    PopMemoryTag();
    if (hr < 0) {
        *slot = nullptr;
    }
    if (errors) {
        Com<unsigned long>(errors, FX_RELEASE);
        errors = nullptr;
    }
    NiFree(data);
}

// ---- 0x55FD90: every effect compiled and made again
void __cdecl RebuildEffects() {
    for (int i = 0; i < FX_COUNT; i++) {
        if (ms_effects[i]) {
            Com<unsigned long>(ms_effects[i], FX_RELEASE);
            ms_effects[i] = nullptr;
        }
        FX_CompileEffect(ms_effectNames[i]);
        FX_LoadEffect(i);
    }
}

// ---- 0x55FDE0: the scene blurred into the half-size target (once a frame: the depth
// of field, the out-of-focus blur and the wobble read it)
void __cdecl GaussianBlur() {
    Renderer_EndTarget();
    uint8_t* target = g_sceneTarget;
    Touch(target);
    uint8_t* texture = KindOf(*reinterpret_cast<re::Ptr32<uint8_t>*>(target + 0x24), g_renderedTextureRtti);
    ms_sceneTexture = *reinterpret_cast<re::Ptr32<void>*>(texture + 0x5C);
    Renderer_BeginTarget(ms_halfTarget, 7);
    void* fx = ms_effects[FX_GAUSSIAN_BLUR];
    void* hOffsets = Param(fx, "gSceneTextureSampleOffsets");
    void* hWeights = Param(fx, "gGaussianWeights");
    Com<long>(fx, FX_SET_VECTOR_ARRAY, hOffsets, static_cast<const float*>(ms_blurOffsets), 0xDu);
    Com<long>(fx, FX_SET_VECTOR_ARRAY, hWeights, static_cast<const float*>(ms_blurWeights), 0xDu);
    FX_Draw(FX_GAUSSIAN_BLUR, nullptr, 0, 0, 0);
    ms_blurDone = 1;
}

// ---- 0x55FED0: the scene drawn into the camera's target through a rectangle (the
// background colour black meanwhile)
void __cdecl PassThru(int rect) {
    Renderer_EndTarget();
    float saved[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    vc::Call<void>(g_renderer, 0x60, saved);   // GetBackgroundColor
    float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    vc::Call<void>(g_renderer, 0x58, static_cast<const float*>(black));   // SetBackgroundColor
    Renderer_BeginTarget(ms_photoTarget, 7);
    uint8_t* target = g_sceneTarget;
    Touch(target);
    uint8_t* texture = KindOf(*reinterpret_cast<re::Ptr32<uint8_t>*>(target + 0x24), g_renderedTextureRtti);
    ms_sceneTexture = *reinterpret_cast<re::Ptr32<void>*>(texture + 0x5C);
    FX_Draw(FX_PASS_THRU, nullptr, 0, 0, rect);
    Renderer_EndTarget();
    vc::Call<void>(g_renderer, 0x58, static_cast<const float*>(saved));
    Renderer_409520(0, 0);
}

void MotionBlurPass(void* fx, float strength) {
    SetTexture(fx, Param(fx, "gMotionBlurTexture"), D3DTextureOf(ms_fullTexture));
    SetFloat(fx, Param(fx, "gMotionBlurStrength"), strength);
    FX_Draw(FX_MOTION_BLUR, "AccumulateTechnique", 0, 0, 0);
    Renderer_EndTarget();
    Renderer_CopyTarget(ms_backBufferTarget, ms_fullTexture);
}

// ---- 0x55FFE0: the motion blur (the frames accumulated in the full-size target;
// started afresh after ResetMotionBlur). Only the strength is read of its arguments.
void __cdecl MotionBlur(int strength, float, void*, int, float, int) {
    void* fx = ms_effects[FX_MOTION_BLUR];
    Renderer_EndTarget();
    if (ms_resetMotionBlur) {
        Renderer_BeginTarget(ms_backBufferTarget, 0);
        MotionBlurPass(fx, 0.0f);
        Renderer_BeginTarget(ms_backBufferTarget, 0);
        ms_resetMotionBlur = 0;
    } else {
        Renderer_BeginTarget(ms_backBufferTarget, 0);
    }
    MotionBlurPass(fx, float(double(strength) / 200.0));
    BeginMainTarget();
    uint8_t* target = g_sceneTarget;
    Touch(target);
    vc::Call<void>(target, 0x50);
    uint8_t* texture = KindOf(*reinterpret_cast<re::Ptr32<uint8_t>*>(target + 0x24), g_renderedTextureRtti);
    DrawScreenQuad(0, 0, *reinterpret_cast<re::Ptr32<void>*>(texture + 0x5C));
    FX_Draw(FX_MOTION_BLUR, nullptr, 0, 0, 0);
    EndEffect();
}

// the Gaussian's pass if not done this frame, then back to the main target
void BlurredStart() {
    if (!ms_blurDone) {
        FX_GaussianBlur();
    }
    Renderer_EndTarget();
    BeginMainTarget();
}

// ---- 0x5601F0: the depth of field (interiors). (Both pixel sizes are set to the
// first parameter's handle.)
void __cdecl DepthOfField(int strength) {
    void* fx = ms_effects[FX_DEPTH_OF_FIELD];
    BlurredStart();
    SetTexture(fx, Param(fx, "gBlurTexture"), D3DTextureOf(ms_halfTexture));
    void* hScene = Param(fx, "gSceneTexturePixelSize");
    Param(fx, "gBlurTexturePixelSize");
    int32_t settings[6];
    float v1[4], v2[4];
    v1[0] = float(0.25 / double(*reinterpret_cast<int32_t*>(GetDisplaySettings(settings))));
    v1[1] = float(0.25 / double(*reinterpret_cast<int32_t*>(GetDisplaySettings(settings) + 4)));
    v1[2] = 0.0f;
    v1[3] = 0.0f;
    v2[0] = float(1.0 / double(*reinterpret_cast<int32_t*>(GetDisplaySettings(settings))));
    v2[1] = float(1.0 / double(*reinterpret_cast<int32_t*>(GetDisplaySettings(settings) + 4)));
    v2[2] = 0.0f;
    v2[3] = 0.0f;
    SetVector(fx, hScene, v1);
    SetVector(fx, hScene, v2);
    SetFloat(fx, Param(fx, "gBlurStrength"), float(double(uint8_t(strength)) / 255.0));
    FX_Draw(FX_DEPTH_OF_FIELD, nullptr, 0, 0, 0);
    EndEffect();
}

// ---- 0x560380: the out-of-focus blur through a mask texture (the last two arguments
// aren't read)
void __cdecl OutOfFocus(int strength, uint8_t* mask, int, int) {
    void* fx = ms_effects[FX_OUT_OF_FOCUS];
    BlurredStart();
    SetTexture(fx, Param(fx, "gBlurTexture"), D3DTextureOf(ms_halfTexture));
    void* maskTexture = mask ? D3DTextureOf(mask) : nullptr;
    SetTexture(fx, Param(fx, "gBlurMaskTexture"), maskTexture);
    SetFloat(fx, Param(fx, "gBlurStrength"), float(double(strength) * 0.0078125));
    FX_Draw(FX_OUT_OF_FOCUS, nullptr, 0, 0, 0);
    EndEffect();
}

// the bloom's kernel: the taps' offsets along x or y in quarter-size texels. (It fills
// four times as many floats as there are samples; the effect is given `samples`.)
void FillKernel(int samples, bool vertical) {
    int32_t settings[6];
    for (int i = 0; i < samples * 16; i += 4) {
        float* k = ms_kernel;
        if (vertical) {
            k[i] = 0.0f;
        }
        double offset = double(i / 4 - samples / 2);
        int32_t size = *reinterpret_cast<int32_t*>(GetDisplaySettings(settings) + (vertical ? 4 : 0)) / 4;
        k = ms_kernel;
        k[i + (vertical ? 1 : 0)] = float(offset / double(size));
        if (!vertical) {
            k[i + 1] = 0.0f;
        }
        k[i + 2] = 0.0f;
        k[i + 3] = 0.0f;
    }
}

// ---- 0x560480: the glow: the bright parts (over the threshold) to a quarter-size
// target, blurred across and down, added back at the glow's strength
void __cdecl Bloom(int threshold, int glow, int scale, int, int) {
    void* fx = ms_effects[FX_LUMINANCE];
    Renderer_EndTarget();
    Renderer_BeginTarget(ms_quarterTarget2, 0);
    ms_sceneTexture = SceneD3DTexture();
    SetFloat(fx, Param(fx, "gGlowThreshold"), float(double(threshold) / 255.0));
    FX_Draw(FX_LUMINANCE, nullptr, 0, 0, 0);

    fx = ms_effects[FX_BLOOM];
    Renderer_EndTarget();
    Renderer_CopyTarget(ms_quarterTarget2, ms_quarterTexture);
    Renderer_BeginTarget(ms_quarterTarget2, 0);
    ms_sceneTexture = D3DTextureOf(ms_quarterTexture);
    void* hSamples = Param(fx, "BLUR_SAMPLES");
    int32_t samples = 0;
    Com<long>(fx, FX_GET_INT, hSamples, &samples);
    FillKernel(samples, false);
    Com<long>(fx, FX_SET_FLOAT_ARRAY, Param(fx, "gHorizontalKernel"), static_cast<const float*>(ms_kernel),
              uint32_t(samples));
    SetFloat(fx, Param(fx, "gBloomScale"), float(double(scale) * 0.5));
    FX_Draw(FX_BLOOM, "BloomHorizontalTechnique", 0, 0, 0);
    Renderer_EndTarget();
    Renderer_CopyTarget(ms_quarterTarget2, ms_quarterTexture);
    Renderer_BeginTarget(ms_quarterTarget2, 0);
    ms_sceneTexture = D3DTextureOf(ms_quarterTexture);
    FillKernel(samples, true);
    Com<long>(fx, FX_SET_FLOAT_ARRAY, Param(fx, "gVerticalKernel"), static_cast<const float*>(ms_kernel),
              uint32_t(samples));
    FX_Draw(FX_BLOOM, "BloomVerticalTechnique", 0, 0, 0);
    Renderer_EndTarget();
    Renderer_CopyTarget(ms_quarterTarget2, ms_quarterTexture);

    BeginMainTarget();
    ms_sceneTexture = SceneD3DTexture();
    fx = ms_effects[FX_GLOW];
    SetTexture(fx, Param(fx, "gGlowTexture"), D3DTextureOf(ms_quarterTexture));
    SetFloat(fx, Param(fx, "gGlowStrength"), float(double(glow) / 255.0));
    FX_Draw(FX_GLOW, nullptr, 0, 0, 0);
    EndEffect();
}

// ---- 0x5608D0: the wobble (being hit), with the tunnel vision texture
void __cdecl Wobble(float speed, float strength) {
    void* fx = ms_effects[FX_WOBBLE];
    BlurredStart();
    SetFloat(fx, Param(fx, "gWobbleSpeed"), float(double(speed) / 600.0));
    SetFloat(fx, Param(fx, "gWobbleStrength"), float(double(strength) / 1.5));
    SetTexture(fx, Param(fx, "gBlurTexture"), D3DTextureOf(ms_halfTexture));
    PushCurrentTxd();
    if (ms_wobbleTxd == -1) {
        ms_wobbleTxd = FindTxdSlot("EffectTextures", 0);
    }
    SetCurrentTxd(ms_wobbleTxd);
    uint8_t* tunnel = FindTexture("TunnelVision", 0);
    SetTexture(fx, Param(fx, "gTunnelVisionTexture"), D3DTextureOfTexture(tunnel));
    FX_Draw(FX_WOBBLE, nullptr, 0x11, 10, 0);
    EndEffect();
    PopCurrentTxd();
}

// ---- 0x560A30: the monochrome filter in a colour (the camera's filters, mud)
void __cdecl Monochrome(const CRGBA* colour) {
    void* fx = ms_effects[FX_MONOCHROME];
    Renderer_EndTarget();
    BeginMainTarget();
    void* h = Param(fx, "gColor");
    float v[4];
    ColourVector(reinterpret_cast<const uint8_t*>(colour), v);
    SetVector(fx, h, v);
    FX_Draw(FX_MONOCHROME, nullptr, 0, 0, 0);
    EndEffect();
}

// ---- 0x560B20 (thiscall): a streak's constructor (its vectors zeroed)
Streak* __fastcall Streak_Construct(Streak* self, re::Edx) {
    float* f = reinterpret_cast<float*>(self);
    for (int i = 0; i <= 0x20; i++) {
        f[i] = 0.0f;
    }
    for (int i = 0x2D; i <= 0x32; i++) {
        f[i] = 0.0f;
    }
    return self;
}

// ---- 0x560BB0
void __cdecl RemoveStreak(int index) {
    if (ms_streaks[index].count) {
        ms_numStreaks--;
        ms_streaks[index].count = 0;
    }
}

// ---- 0x560BE0
void __cdecl ClearStreaks() {
    for (int i = 0; i < 10; i++) {
        ms_streaks[i].count = 0;
    }
    ms_numStreaks = 0;
}

// ---- 0x560C20
void __cdecl SetStreakSize(int index, float size) {
    ms_streaks[index].size = size;
}

// a line's step: (8..23)/23 of the size, at most maxLength
void NewLength(Streak& s, int line) {
    float length = float(double((GameRand() & 0xF) + 8) / 23.0 * double(s.size));
    s.lengths[line] = length;
    if (s.maxLength < length) {
        s.lengths[line] = s.maxLength;
    }
}

// ---- 0x560C40: a new streak (its index; -1: all 10 in use). (With the count off and
// no free slot, the game uses an uninitialised index.)
int32_t __cdecl AddStreak(const CVector* origin, const CVector* dir, float maxLength, float size) {
    if (ms_numStreaks >= 10) {
        return -1;
    }
    int32_t index = 0;
    for (int i = 0; i < 10; i++) {
        if (ms_streaks[i].count == 0) {
            index = i;
            break;
        }
    }
    Streak& s = ms_streaks[index];
    s.origin = *origin;
    s.size = size;
    s.dir = *dir;
    NormaliseSafe(&s.dir);
    s.spread.x = float((1.0 - double(std::fabs(dir->x))) * 2.0);
    s.spread.y = float((1.0 - double(std::fabs(dir->y))) * 2.0);
    s.unknownAC = 0x80;
    s.spread.z = float((1.0 - double(std::fabs(dir->z))) * 2.0);
    s.maxLength = maxLength;
    s.count = int(double(size) * 20.0);
    if (s.count > 10) {
        s.count = 10;
    } else if (s.count < 4) {
        s.count = 4;
    }
    for (int i = 0; i < s.count; i++) {
        s.points[i] = *origin;
        NewLength(s, i);
    }
    ms_numStreaks++;
    return index;
}

// ---- 0x560E00: the streaks on screen (4 at most) move on: each line steps along the
// direction, jittered sideways, shrinking; past maxLength (or too short) it starts again
void __cdecl UpdateStreaks() {
    float screen[3] = {0.0f, 0.0f, 0.0f};
    int updated = 0;
    int seen = 0;
    for (int n = 0; n < 10; n++) {
        Streak& s = ms_streaks[n];
        if (s.count == 0) {
            continue;
        }
        if (seen++ >= ms_numStreaks) {
            return;
        }
        if (updated == 4) {
            return;
        }
        if (!Camera_ProjectToScreen(TheCamera, &s.origin, screen, 0)) {
            continue;
        }
        ms_streakTexture = ms_masks[0];
        for (int i = 0; i < s.count; i++) {
            float step = s.lengths[i];
            CVector v = {step * s.dir.x, s.dir.y * step, step * s.dir.z};
            CVector sum;
            s.points[i] = *VectorAdd(&sum, &s.points[i], &v);
            float jitterScale = float(double(s.size) * 0.25 * double(s.maxLength));
            float jitter = float((double(GameRand() & 0x3F) / 63.0 - 0.5) * double(jitterScale));
            CVector w = {s.spread.x * jitter, s.spread.y * jitter, jitter * s.spread.z};
            s.points[i].x = w.x + s.points[i].x;
            s.points[i].y = w.y + s.points[i].y;
            s.points[i].z = s.points[i].z + w.z;
            s.lengths[i] = float(double(s.lengths[i]) * 0.949999988079071);
            CVector d;
            VectorSub(&d, &s.points[i], &s.origin);
            float distance =
                float(std::sqrt(double(std::fabs(float(double(d.x) * d.x + double(d.y) * d.y + double(d.z) * d.z)))));
            if (s.maxLength < distance || 0.01f > s.lengths[i]) {
                s.points[i] = s.origin;
                NewLength(s, i);
            }
        }
        updated++;
    }
}

} // namespace

#define SCREENFX_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(screenfx_original_, __LINE__);                                        \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(screenfx_original_, __LINE__), \
             re::Mode::Original)

SCREENFX_REVERSED("CScreenFX::Reset", 0x0055DFE0, &Reset);
SCREENFX_REVERSED("CScreenFX::Shutdown", 0x0055E0D0, &Shutdown);
SCREENFX_REVERSED("CScreenFX::LoadAreaSettings", 0x0055E100, &LoadAreaSettings);
SCREENFX_REVERSED("CScreenFX::Initialise", 0x0055E210, &Initialise);
SCREENFX_REVERSED("CScreenFX::Render", 0x0055E2C0, &Render);
SCREENFX_REVERSED("CScreenFX::ResetMotionBlur", 0x0055E850, &ResetMotionBlur);
SCREENFX_REVERSED("CScreenFX::ShaderInclude::Close", 0x0055E860, &ShaderInclude_Close);
SCREENFX_REVERSED("CScreenFX::GetEffect", 0x0055E880, &GetEffect);
SCREENFX_REVERSED("CScreenFX::CreateRasters", 0x0055E8F0, &CreateRasters);
SCREENFX_REVERSED("CScreenFX::MakeGaussianKernel", 0x0055E970, &MakeGaussianKernel);
SCREENFX_REVERSED("CScreenFX::CreateTargets", 0x0055EB70, &CreateTargets);
SCREENFX_REVERSED("CScreenFX::GetMissingEffects", 0x0055EF60, &GetMissingEffects);
SCREENFX_REVERSED("CScreenFX::CreatePhotoTarget", 0x0055EF80, &CreatePhotoTarget);
SCREENFX_REVERSED("CScreenFX::ReloadEffects", 0x0055F100, &ReloadEffects);
SCREENFX_REVERSED("CScreenFX::RecreateTargets", 0x0055F130, &RecreateTargets);
SCREENFX_REVERSED("CScreenFX::DrawEffect", 0x0055F5B0, &DrawEffect);
SCREENFX_REVERSED("CScreenFX::ReleaseTargets", 0x0055F790, &ReleaseTargets);
SCREENFX_REVERSED("CScreenFX::Modulate", 0x0055F910, &Modulate);
SCREENFX_REVERSED("CScreenFX::HeatHaze", 0x0055F9C0, &HeatHaze);
SCREENFX_REVERSED("CScreenFX::ShaderInclude::Open", 0x0055FAD0, &ShaderInclude_Open);
SCREENFX_REVERSED("CScreenFX::CompileEffect_55FBB0", 0x0055FBB0, &CompileEffect);
SCREENFX_REVERSED("CScreenFX::LoadEffect", 0x0055FC60, &LoadEffect);
SCREENFX_REVERSED("CScreenFX::RebuildEffects", 0x0055FD90, &RebuildEffects);
SCREENFX_REVERSED("CScreenFX::GaussianBlur", 0x0055FDE0, &GaussianBlur);
SCREENFX_REVERSED("CScreenFX::PassThru", 0x0055FED0, &PassThru);
SCREENFX_REVERSED("CScreenFX::MotionBlur", 0x0055FFE0, &MotionBlur);
SCREENFX_REVERSED("CScreenFX::DepthOfField", 0x005601F0, &DepthOfField);
SCREENFX_REVERSED("CScreenFX::OutOfFocus", 0x00560380, &OutOfFocus);
SCREENFX_REVERSED("CScreenFX::Bloom", 0x00560480, &Bloom);
SCREENFX_REVERSED("CScreenFX::Wobble", 0x005608D0, &Wobble);
SCREENFX_REVERSED("CScreenFX::Monochrome", 0x00560A30, &Monochrome);
SCREENFX_REVERSED("CScreenFX::Streak::Streak", 0x00560B20, &Streak_Construct);
SCREENFX_REVERSED("CScreenFX::RemoveStreak", 0x00560BB0, &RemoveStreak);
SCREENFX_REVERSED("CScreenFX::ClearStreaks", 0x00560BE0, &ClearStreaks);
SCREENFX_REVERSED("CScreenFX::SetStreakSize", 0x00560C20, &SetStreakSize);
SCREENFX_REVERSED("CScreenFX::AddStreak", 0x00560C40, &AddStreak);
SCREENFX_REVERSED("CScreenFX::UpdateStreaks", 0x00560E00, &UpdateStreaks);
