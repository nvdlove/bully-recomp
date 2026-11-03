// The effect system's core (0x665FD0..0x66A03B): the definition loaders' constructors
// and the loader switch, the definition store (19 typed arrays whose counts come from
// DEFINITIONS_COUNTS), effect instances (a list of primitive instances, fading,
// importance countdown), the effect definition's loader, the definition / light /
// effect managers, and the effect proxies (EffectProxy, EffectProxyAttach,
// EffectProxySetLocation, SpawnEffectPrimitive's proxy).
//
// An archive (TextFileArchive.cpp) reads through its virtuals: 0x24 a string, 0x28 a
// float, 0x38 an int, 0x3C a bool (each (out, name)); writes with 0x60 an int (value, name).
// Definitions start with their type byte (the store's array index; its element size in
// the table at 0xB31CE8).

#include <cstdint>
#include <cstring>

#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

// a singly linked list: its head, each item's link +0 the next one
struct Link {
    re::Ptr32<Link> next;
};

constexpr int kDefinitionTypes = 19;
GAME_ARRAY(uint32_t, definitionSizes, 0x00B31CE8);   // [19] element sizes
GAME_VAR(const char*, definitionsCountsName, 0x00B31D34);   // "DEFINITIONS_COUNTS"
GAME_VAR(uint8_t*, definitionCounts_B6F1D8, 0x00B6F1D8);
GAME_VAR(uint8_t*, definitionStore, 0x00B6F1DC);
GAME_VAR(uint8_t*, effectManager_B6F1E0, 0x00B6F1E0);
GAME_VAR(const char*, countName_93D640, 0x0093D640);
GAME_ARRAY(uint8_t, definitionKinds, 0x00B6F214);   // a definition type's kind

constexpr uint32_t kVtDefinitionBaseLoader = 0x0093CEEC;
constexpr uint32_t kVtParticleSystemDefinitionLoader = 0x0093CF10;
constexpr uint32_t kVtColorAndSizeRenderInfoDefinitionLoader = 0x0093CF20;
constexpr uint32_t kVtRectangle2dRenderInfoDefinitionLoader = 0x0093CF30;
constexpr uint32_t kVtRotationRenderInfoDefinitionLoader = 0x0093CF40;
constexpr uint32_t kVtAttractorMovementInfoDefinitionLoader = 0x0093CF50;
constexpr uint32_t kVtFrictionMovementInfoDefinitionLoader = 0x0093CF60;
constexpr uint32_t kVtJitterMovementInfoDefinitionLoader = 0x0093CF70;
constexpr uint32_t kVtGravityMovementInfoDefinitionLoader = 0x0093CF80;
constexpr uint32_t kVtUVRenderInfoDefinitionLoader = 0x0093CF90;
constexpr uint32_t kVtLightPrimitiveDefinitionLoader = 0x0093CFA0;
constexpr uint32_t kVtSpawnEffectPrimitiveDefinitionLoader = 0x0093CFB0;
constexpr uint32_t kVtPlaySoundPrimitiveDefinitionLoader = 0x0093CFC0;
constexpr uint32_t kVtHeatHazePrimitiveDefinitionLoader = 0x0093CFD0;
constexpr uint32_t kVtMotionBlurPrimitiveDefinitionLoader = 0x0093CFE0;
constexpr uint32_t kVtDefinitionCountLoader = 0x0093CFF0;
constexpr uint32_t kVtEffectDefinitionLoader = 0x0093D6B4;
constexpr uint32_t kVtEffectDefinitionManager = 0x0093D6C4;
constexpr uint32_t kVtEffectLightManager = 0x0093D6CC;
constexpr uint32_t kVtEffectManager = 0x0093D6D4;
GAME_VAR(uint8_t*, effectCounter_B6F1D4, 0x00B6F1D4);   // the effect manager (+8 its count)
GAME_VAR(uint8_t, fireCreate_BF3808, 0x00BF3808);
GAME_VAR(void*, niCamera, 0x00C3CCF4);
GAME_VAR(uint8_t*, effectLibrary_B6F1E8, 0x00B6F1E8);   // the effect definition manager
GAME_VAR(uint8_t*, effectLights_B6F1F4, 0x00B6F1F4);    // the effect light manager
GAME_VAR(uint8_t*, particleRenderer_B6F1EC, 0x00B6F1EC);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);               // CTimer::m_snTimeInMilliseconds
inline void* const trails_1B850E0 = reinterpret_cast<void*>(0x01B850E0);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(uint8_t, cameraMode_C936F8, 0x00C936F8);   // 0xFF: none
GAME_VAR(uint8_t, renderingMirror_C936FA, 0x00C936FA);
constexpr uint32_t kVtEffectProxy = 0x0093D6DC;
GAME_VAR(const char*, startTimeName_93D550, 0x0093D550);
GAME_VAR(const char*, durationName_93D554, 0x0093D554);
GAME_VAR(const char*, descriptionName_93D5E8, 0x0093D5E8);
constexpr uint32_t kVtEffectProxyAttach = 0x0093D6F0;
constexpr uint32_t kVtEffectProxySetLocation = 0x0093D704;
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);
constexpr uint32_t kVtEffectSystem = 0x0093D718;
inline void* const nameRef_CF356C = reinterpret_cast<void*>(0x00CF356C);
GAME_VAR(const char*, effectNameKey_93D544, 0x0093D544);
GAME_ARRAY(const char*, effectFlagNames_93D5EC, 0x0093D5EC);   // 5 names: bits 1, 0, 2, 3, importance
GAME_ARRAY(const char*, effectDistanceNames_93D600, 0x0093D600);   // 4 names
//@@TOP@@

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
inline void __cdecl Exit(int code) { crt::Exit(code); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, int count, void* ctor);
GAME_FN(uint8_t, __thiscall, Archive_ReadSection, 0x005B0720, void* archive, void* reader, const char* name);
GAME_FN(void*, __cdecl, Archive_Open, 0x005B07E0, const char* path, const int32_t* mode, int);
GAME_FN(void, __cdecl, Archive_Close, 0x005B0860, void* archive);

// this file's
GAME_FN(uint8_t, __thiscall, Definition_GetType, 0x00665FD0, const uint8_t* definition);
GAME_FN(void*, __thiscall, DefinitionBaseLoader_Construct, 0x00665FF0, void* self, void* definition);
GAME_FN(void*, __thiscall, ParticleSystemDefinitionLoader_Construct, 0x00666010, void* self, void* definition);
GAME_FN(void*, __thiscall, AttractorMovementInfoDefinitionLoader_Construct, 0x00666030, void* self, void* definition);
GAME_FN(void*, __thiscall, FrictionMovementInfoDefinitionLoader_Construct, 0x00666050, void* self, void* definition);
GAME_FN(void*, __thiscall, JitterMovementInfoDefinitionLoader_Construct, 0x00666070, void* self, void* definition);
GAME_FN(void*, __thiscall, GravityMovementInfoDefinitionLoader_Construct, 0x00666090, void* self, void* definition);
GAME_FN(void*, __thiscall, LightPrimitiveDefinitionLoader_Construct, 0x006660B0, void* self, void* definition);
GAME_FN(void*, __thiscall, SpawnEffectPrimitiveDefinitionLoader_Construct, 0x006660D0, void* self, void* definition);
GAME_FN(void*, __thiscall, PlaySoundPrimitiveDefinitionLoader_Construct, 0x006660F0, void* self, void* definition);
GAME_FN(void*, __thiscall, HeatHazePrimitiveDefinitionLoader_Construct, 0x00666110, void* self, void* definition);
GAME_FN(void*, __thiscall, MotionBlurPrimitiveDefinitionLoader_Construct, 0x00666130, void* self, void* definition);
GAME_FN(void, __thiscall, DefinitionStore_Allocate, 0x00666510, uint8_t* self);
GAME_FN(void, __thiscall, DefinitionStore_Release, 0x00666B60, uint8_t* self, void* definition);
GAME_FN(float, __thiscall, EffectDefinition_MaxRadius, 0x00666C40, uint8_t* self);
GAME_FN(void, __thiscall, Effect_KillPrimitives, 0x00666CA0, uint8_t* self);
GAME_FN(void, __thiscall, Effect_RemoveDead, 0x00666E60, uint8_t* self);
GAME_FN(void, __thiscall, List_Init, 0x00667DA0, void* list);
GAME_FN(Link*, __thiscall, List_PopFront, 0x00667DB0, void* list);
GAME_FN(int, __thiscall, List_Count, 0x00667DD0, void* list);
GAME_FN(void, __thiscall, PrimInstance_Kill, 0x00668A40, void* prim);
GAME_FN(void, __thiscall, PrimInstance_Update, 0x00668A50, void* prim, void* effectState);
GAME_FN(void, __thiscall, PrimInstance_SetVisible, 0x00668AD0, void* prim, int visible);
GAME_FN(void*, __thiscall, PrimInstanceList_Construct, 0x006693E0, void* self);
GAME_FN(uint8_t, __thiscall, PrimInstance_GetKind, 0x00669400, void* prim);
GAME_FN(void, __thiscall, Effects_FreePrimInstance, 0x00669DC0, void* manager, void* prim);
GAME_FN(float, __thiscall, PrimTimer_Get, 0x0066A010, void* timer);
GAME_FN(void, __cdecl, PrimInstance_Stop, 0x0066A1C0, void* prim);
GAME_FN(void, __cdecl, PrimDefList_Release, 0x0066A230, void* list);
GAME_FN(void, __cdecl, PrimInstanceList_Release, 0x0066A2F0, void* list);
GAME_FN(uint8_t, __cdecl, DefinitionKind, 0x0066A370, int type);
GAME_FN(uint8_t, __cdecl, PrimInstanceList_Create, 0x0066A380, void* list, void* definitions);
GAME_FN(void, __thiscall, ParticleSystem_SetScale, 0x00670200, void* prim, float x, float y, int z);
GAME_FN(void, __thiscall, ParticleSystem_Set_670370, 0x00670370, void* prim, float value);
GAME_FN(void, __thiscall, ParticleSystem_Set_6702F0, 0x006702F0, void* prim, float value);
GAME_FN(void, __thiscall, ParticleSystem_Set_670330, 0x00670330, void* prim, float value);
GAME_FN(void, __thiscall, ParticleSystem_Set_6703B0, 0x006703B0, void* prim, uintptr_t value);
GAME_FN(uint8_t*, __thiscall, DefinitionStore_NewFn, 0x00666B10, uint8_t* self, int type);
GAME_FN(void, __cdecl, Definition_ReadFn, 0x006661A0, uint8_t* definition, void* archive, const char* name);
GAME_FN(void, __thiscall, List_PushBack, 0x00667DF0, void* list, void* item);
GAME_FN(uint8_t, __thiscall, Effect_InitFn, 0x006672B0, uint8_t* self, uint8_t* definition);
GAME_FN(void, __thiscall, Effect_ReleasePrimitivesFn, 0x00666C90, uint8_t* self);
GAME_FN(void, __thiscall, Effect_ClearFadeFn, 0x00666F60, uint8_t* self);
GAME_FN(void, __thiscall, EffectDefinition_ReleaseFn, 0x00666C20, uint8_t* self);
GAME_FN(uint8_t*, __thiscall, Effects_Alloc, 0x00669410, uint8_t* manager, int kind);
GAME_FN(void, __thiscall, EffectCount_Add, 0x00668230, uint8_t* manager);
GAME_FN(void, __thiscall, EffectCount_Remove, 0x00668240, uint8_t* manager);
GAME_FN(void, __thiscall, SuspendedEffect_Save, 0x00668900, uint8_t* state, uint8_t* effectState);
GAME_FN(void, __thiscall, SuspendedEffect_Restore, 0x00668930, uint8_t* state, uint8_t* effectState);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(uint16_t, __thiscall, EffectDefinitionManager_FindHashFn, 0x006679A0, uint8_t* self, uint32_t hash);
GAME_FN(const char*, __cdecl, DefinitionType_NameFn, 0x00667330, uint32_t type);
GAME_FN(void, __thiscall, ArchiveString_Init, 0x00669E20, void* string);
inline char* __cdecl Strncpy(char* dst, const char* src, uint32_t n) { return crt::Strncpy(dst, src, (size_t)n); }
GAME_FN(void, __thiscall, EffectLight_Update, 0x0066BA50, uint8_t* light);
GAME_FN(void, __thiscall, Timer_Init, 0x00669FD0, void* timer);
GAME_FN(void, __thiscall, Timer_Reset, 0x00669FF0, void* timer);
GAME_FN(void, __thiscall, Timer_Sub, 0x00669FE0, void* timer, float seconds);
GAME_FN(void, __thiscall, Effect_UpdateFn, 0x00667100, uint8_t* self);
GAME_FN(void, __thiscall, EffectHandle_StopFn, 0x006677E0, uint8_t* handle);
GAME_FN(void, __thiscall, EffectHandle_KillFn, 0x00667950, uint8_t* handle);
GAME_FN(uint8_t, __thiscall, EffectHandle_StartFn, 0x006677A0, uint8_t* handle, uint8_t* definition);
GAME_FN(void, __thiscall, EffectProxy_Attach, 0x00668B60, uint8_t* proxy, uint8_t* handle);
GAME_FN(void, __thiscall, EffectProxy_Detach, 0x00668B80, uint8_t* proxy, uint8_t* handle);
GAME_FN(void, __thiscall, EffectProxy_Failed, 0x005FD140, uint8_t* proxy);
GAME_FN(uint8_t*, __thiscall, EffectDefinitionManager_GetFn, 0x006679F0, uint8_t* self, int index);
GAME_FN(void, __thiscall, EffectLightManager_UpdateFn, 0x00667BF0, uint8_t* self);
GAME_FN(void, __thiscall, Trails_Update, 0x0066C0F0, void* self);
GAME_FN(void, __thiscall, ParticleRenderer_Begin, 0x005775B0, uint8_t* renderer);
GAME_FN(void, __thiscall, ParticleRenderer_Add, 0x0057A190, uint8_t* renderer, void* prim);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);   // (+0x24: the translation)
GAME_FN(int, __thiscall, Camera_IsSphereVisible, 0x004F13D0, void* camera, const float* centre, float radius);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const float* v);
GAME_FN(void, __thiscall, EffectHandle_SuspendFn, 0x00667840, uint8_t* handle);
GAME_FN(void, __thiscall, EffectHandle_ResumeFn, 0x006678B0, uint8_t* handle);
GAME_FN(void, __thiscall, EffectManager_UpdateLod, 0x00668360, uint8_t* self, uint8_t* effect);
GAME_FN(void, __thiscall, EffectManager_UpdateSuspension, 0x00668610, uint8_t* self, uint8_t* handle);
GAME_FN(void, __thiscall, Trails_Render, 0x0066CA90, void* self);
GAME_FN(uint8_t, __cdecl, Render_GetFlag_DA7519, 0x005EFAD0);
GAME_FN(void, __cdecl, Render_SetFlag_DA7519, 0x005EFAC0, int value);
GAME_FN(void, __thiscall, Quaternion_FromMatrix_776040, 0x00776040, void* self, const void* transform);
GAME_FN(void, __thiscall, Quaternion_ToMatrix_413C50, 0x00413C50, const void* self, void* transform);
GAME_FN(int, __thiscall, EffectDefinition_NumPrimitivesFn, 0x00666C30, uint8_t* self);
GAME_FN(Link*, __thiscall, Effect_FindPrimitiveFn, 0x00666EE0, uint8_t* self, int n);
GAME_FN(void, __thiscall, EffectManager_ForgetProxyFn, 0x00668100, uint8_t* self, uint8_t* proxy);
GAME_FN(void, __thiscall, EffectManager_RemoveFn, 0x006681B0, uint8_t* self, uint8_t* handle);
GAME_FN(void, __thiscall, Effect_SetBoundedFn, 0x00666F50, uint8_t* self);
GAME_FN(void, __thiscall, Effect_KillAfterFn, 0x00666E10, uint8_t* self, float time);
GAME_FN(void, __thiscall, Effect_SetScaleFn, 0x00666CD0, uint8_t* self, float x, float y, int z);
GAME_FN(uint8_t*, __thiscall, EffectProxy_Construct, 0x00668B30, uint8_t* self);
GAME_FN(void, __thiscall, EffectProxy_Destruct, 0x00668B40, uint8_t* self);
GAME_FN(uint8_t*, __thiscall, EffectProxy_GetHandle, 0x0066B5D0, uint8_t* self);
GAME_FN(void, __thiscall, AnimManager_Detach, 0x006C2200, void* manager, void* clump, void* node);
GAME_FN(void, __thiscall, Matrix3_MakeIdentity, 0x00770920, void* matrix);
GAME_FN(float*, __cdecl, CombineTranslation, 0x00415F50, void* out, const float* translation, int mode);
GAME_FN(float*, __thiscall, NiTransform_Transform, 0x00450B10, const void* transform, float* out, const float* v);
GAME_FN(void, __cdecl, MatrixInvert, 0x00522440, void* out, const void* in);
GAME_FN(void*, __cdecl, GetCameraWorldTransform, 0x006C81D0, void* camera);
GAME_FN(void, __cdecl, ScreenToWorld, 0x00668F80, float* v);
GAME_FN(uint8_t, __cdecl, Vector_NearlyEqual, 0x00578540, const float* a, const float* b, float tolerance);
GAME_FN(float, __thiscall, Vec_Normalise, 0x004294A0, float* v);
GAME_FN(void, __cdecl, Matrix_OrientTo, 0x00669170, float* matrix, const float* direction);
GAME_FN(uint8_t, __thiscall, EffectPools_KindOf, 0x006696E0, uint8_t* self, const void* p);
GAME_FN(void, __thiscall, EffectPools_InitFn, 0x00669450, uint8_t* self);
inline int __cdecl ToLower(int c) { return crt::Tolower(c); }
GAME_FN(void, __thiscall, EffectManager_ResetTimersFn, 0x00668130, uint8_t* self);
GAME_FN(void, __thiscall, StringRef_Assign, 0x005772F0, void* field, uint32_t value);
GAME_FN(void, __thiscall, EffectManager_Shutdown_668350, 0x00668350, uint8_t* self);
GAME_FN(void, __thiscall, EffectDefinitionManager_ClearFn, 0x00667A00, uint8_t* self);
GAME_FN(void, __thiscall, ParticleRenderer_Shutdown_5781D0, 0x005781D0, uint8_t* self);
GAME_FN(void, __thiscall, Definitions_Shutdown_66B800, 0x0066B800, uint8_t* self);
GAME_FN(void, __cdecl, EffectDefinition_LoadPrimitivesFn, 0x00667680, void* archive, void* list);
//@@DECL@@

// ---- the definition loaders -------------------------------------------------------------------

// 0x665FD0 (thiscall): a definition's type (its first byte)
[[maybe_unused]] uint8_t __fastcall Definition_GetTypeImpl(const uint8_t* self, re::Edx) {
    return *self;
}
RE_UNHOOKED_FN("Definition::GetType", 0x00665FD0, &Definition_GetTypeImpl);

// 0x665FE0 (thiscall): the kind of a definition's type
uint32_t __fastcall Definition_GetKind(const uint8_t* self, re::Edx) {
    return DefinitionKind(*self);
}

// 0x665FF0 (thiscall): DefinitionBaseLoader's constructor (the definition it fills)
void* __fastcall DefinitionBaseLoader_Ctor(uint8_t* self, re::Edx, void* definition) {
    At<uint32_t>(self, 0) = kVtDefinitionBaseLoader;
    At<void*>(self, 4) = definition;
    return self;
}

// 0x666010..0x666130 (thiscall): the typed loaders' constructors: the base's, then their
// own vtable
template <uint32_t Vt>
void* __fastcall TypedLoader_Ctor(uint8_t* self, re::Edx, void* definition) {
    DefinitionBaseLoader_Construct(self, definition);
    At<uint32_t>(self, 0) = Vt;
    return self;
}

// 0x6661A0 (cdecl): a definition read from an archive section by its type's loader
void __cdecl Definition_Read(uint8_t* definition, void* archive, const char* name) {
    uint8_t loader[8];
    switch (Definition_GetType(definition)) {
    case 5:
        DefinitionBaseLoader_Construct(loader, definition);
        At<uint32_t>(loader, 0) = kVtColorAndSizeRenderInfoDefinitionLoader;
        break;
    case 6:
        DefinitionBaseLoader_Construct(loader, definition);
        At<uint32_t>(loader, 0) = kVtRectangle2dRenderInfoDefinitionLoader;
        break;
    case 7:
        DefinitionBaseLoader_Construct(loader, definition);
        At<uint32_t>(loader, 0) = kVtRotationRenderInfoDefinitionLoader;
        break;
    case 0xC:
        DefinitionBaseLoader_Construct(loader, definition);
        At<uint32_t>(loader, 0) = kVtUVRenderInfoDefinitionLoader;
        break;
    case 8:
        AttractorMovementInfoDefinitionLoader_Construct(loader, definition);
        break;
    case 9:
        FrictionMovementInfoDefinitionLoader_Construct(loader, definition);
        break;
    case 0xA:
        JitterMovementInfoDefinitionLoader_Construct(loader, definition);
        break;
    case 0xB:
        GravityMovementInfoDefinitionLoader_Construct(loader, definition);
        break;
    case 2:
        ParticleSystemDefinitionLoader_Construct(loader, definition);
        break;
    case 0xD:
        LightPrimitiveDefinitionLoader_Construct(loader, definition);
        break;
    case 0xF:
        SpawnEffectPrimitiveDefinitionLoader_Construct(loader, definition);
        break;
    case 0x10:
        HeatHazePrimitiveDefinitionLoader_Construct(loader, definition);
        break;
    case 0x11:
        MotionBlurPrimitiveDefinitionLoader_Construct(loader, definition);
        break;
    case 0x12:
        PlaySoundPrimitiveDefinitionLoader_Construct(loader, definition);
        break;
    default:
        return;
    }
    Archive_ReadSection(archive, loader, name);
}

// ---- the definition store ---------------------------------------------------------------------
// +0xC counts[19], +0x58 used[19], +0xA4 arrays[19]

// the store's counts: +4, +8 and the store's +0xC..+0x54 (its own at 0xB6F1D8 and 0xB6F1DC)
// 0x666390 (thiscall): DefinitionCountLoader's vf2, written
void __fastcall DefinitionCountLoader_Write(uint8_t*, re::Edx, void* archive) {
    vc::Call<void>(archive, 0x60, At<int32_t>(definitionCounts_B6F1D8, 4), countName_93D640);
    vc::Call<void>(archive, 0x60, At<int32_t>(definitionCounts_B6F1D8, 8), countName_93D640);
    for (int at = 0xC; at < 0x58; at += 4) {
        vc::Call<void>(archive, 0x60, At<int32_t>(definitionStore, at), countName_93D640);
    }
}
// 0x666400 (thiscall): DefinitionCountLoader's vf1, read
void __fastcall DefinitionCountLoader_Read(uint8_t*, re::Edx, void* archive) {
    vc::Call<void>(archive, 0x38, &At<int32_t>(definitionCounts_B6F1D8, 4), countName_93D640);
    vc::Call<void>(archive, 0x38, &At<int32_t>(definitionCounts_B6F1D8, 8), countName_93D640);
    for (int at = 0xC; at < 0x58; at += 4) {
        vc::Call<void>(archive, 0x38, &At<int32_t>(definitionStore, at), countName_93D640);
    }
}

// 0x666150 (thiscall): a type's element by index
uint8_t* __fastcall DefinitionStore_Get(uint8_t* self, re::Edx, int index, const uint8_t* type) {
    return At<uint8_t*>(self, 0xA4 + *type * 4) + definitionSizes[*type] * index;
}

// 0x666170 (thiscall): an element's index in its type's array
uint32_t __fastcall DefinitionStore_IndexOf(uint8_t* self, re::Edx, uint8_t* definition) {
    uint8_t type = Definition_GetType(definition);
    return uint32_t(definition - At<uint8_t*>(self, 0xA4 + type * 4)) / definitionSizes[type];
}

// 0x666470 (thiscall): the store's constructor (+0, +4, the counts and used counts zeroed)
uint8_t* __fastcall DefinitionStore_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0;
    At<uint32_t>(self, 4) = 0;
    for (int i = 0; i < kDefinitionTypes; i++) {
        At<uint32_t>(self, 0x58 + i * 4) = 0;
    }
    for (int i = 0; i < kDefinitionTypes; i++) {
        At<uint32_t>(self, 0xC + i * 4) = 0;
    }
    return self;
}

// new T[count] the compiler's way: the count before the elements, each built by `ctor`
// (sizes saturate on overflow)
uint8_t* NewArray(uint32_t count, uint32_t size, uintptr_t ctor) {
    uint64_t bytes = uint64_t(count) * size;
    uint32_t total = (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
    total = total > 0xFFFFFFFBu ? 0xFFFFFFFFu : total + 4;
    uint32_t* memory = static_cast<uint32_t*>(Malloc(total));
    if (!memory) {
        return nullptr;
    }
    *memory = count;
    VectorConstruct(memory + 1, size, int(count), reinterpret_cast<void*>(ctor));
    return reinterpret_cast<uint8_t*>(memory + 1);
}

// 0x666510 (thiscall): the arrays allocated for the counts read (their elements' constructors)
void __fastcall DefinitionStore_AllocateImpl(uint8_t* self, re::Edx) {
    static constexpr uintptr_t ctors[kDefinitionTypes] = {
        0x00666BE0, 0x0066A8A0, 0x0066F8F0, 0x0066F1D0, 0x0066E8B0, 0x00671D70, 0x006720A0,
        0x006722D0, 0x00671590, 0x00671870, 0x00671870, 0x00671870, 0x00672550, 0x0066B840,
        0x006708E0, 0x0066E2A0, 0x0066A5B0, 0x0066D1F0, 0x0066DEB0,
    };
    for (int i = 0; i < kDefinitionTypes; i++) {
        int32_t count = At<int32_t>(self, 0xC + i * 4);
        if (count > 0) {
            At<uint8_t*>(self, 0xA4 + i * 4) = NewArray(uint32_t(count), definitionSizes[i], ctors[i]);
        }
    }
}

// 0x666B10 (thiscall): the next free element of a type, its type byte set (out of room:
// the game exits)
uint8_t* __fastcall DefinitionStore_New(uint8_t* self, re::Edx, int type) {
    PushMemoryTag(0x22);
    uint32_t t = uint8_t(type);
    uint32_t used = At<uint32_t>(self, 0x58 + t * 4);
    uint8_t* element = At<uint8_t*>(self, 0xA4 + t * 4) + definitionSizes[t] * used;
    used++;
    At<uint32_t>(self, 0x58 + t * 4) = used;
    if (used > At<uint32_t>(self, 0xC + t * 4)) {
        Exit(1);
    }
    PopMemoryTag();
    *element = uint8_t(type);
    return element;
}

// 0x666B60 (thiscall): an element given back (nothing: the arrays aren't reused)
void __fastcall DefinitionStore_ReleaseImpl(uint8_t*, re::Edx, void*) {
    PushMemoryTag(0x22);
    PopMemoryTag();
}

// 0x666B80 (thiscall): the counts read from the archive's DEFINITIONS_COUNTS section, one
// more of type 0, and the arrays allocated
void __fastcall DefinitionStore_LoadCounts(uint8_t* self, re::Edx, const char* path) {
    int32_t mode = 0;
    void* archive = Archive_Open(path, &mode, 1);
    uint32_t loader = kVtDefinitionCountLoader;
    Archive_ReadSection(archive, &loader, definitionsCountsName);
    Archive_Close(archive);
    At<int32_t>(self, 0xC) += 1;
    DefinitionStore_Allocate(self);
}

// ---- effect definitions -----------------------------------------------------------------------
// +4 high detail distance, +8 low detail distance, +0xC max radius, +0x10 fade distance,
// +0x14 its primitives (the list's links at their +4), +0x18 the name's hash, +0x1C flags
// (1, 2 ignore in mirrors, 4 update when faded, 8, 0x70 importance)

// 0x666BE0 (thiscall): the constructor (the type byte 0xFF, +4/+0xC and +8/+0x10 the two
// constants at 0x900D38 / 0x900490, +0x14 its empty list of primitives, +0x1C's flags)
uint8_t* __fastcall EffectDefinition_Construct(uint8_t* self, re::Edx) {
    float a = *reinterpret_cast<const float*>(0x00900D38);
    float b = *reinterpret_cast<const float*>(0x00900490);
    At<float>(self, 4) = a;
    self[0] = 0xFF;
    At<float>(self, 8) = b;
    At<float>(self, 0x10) = b;
    At<float>(self, 0xC) = a;
    At<uint32_t>(self, 0x14) = 0;
    self[0x1C] = uint8_t((self[0x1C] & 0xA4) | 0x24);
    return self;
}

// 0x666C20 (thiscall): its primitives given back
void __fastcall EffectDefinition_Release(uint8_t* self, re::Edx) {
    PrimDefList_Release(self + 0x14);
}

// 0x666C30 (thiscall): its number of primitives
int __fastcall EffectDefinition_NumPrimitives(uint8_t* self, re::Edx) {
    return List_Count(self + 0x14);
}

// 0x666C40 (thiscall): the largest diameter of its primitives (their +8 doubled)
float __fastcall EffectDefinition_MaxRadiusImpl(uint8_t* self, re::Edx) {
    float largest = 0.0f;
    for (Link* link = At<Link*>(self, 0x14); link; link = link->next) {
        uint8_t* prim = reinterpret_cast<uint8_t*>(link) - 4;
        float diameter = At<float>(prim, 0xC) + At<float>(prim, 0xC);
        if (largest < diameter) {
            largest = diameter;
        }
    }
    return largest;
}

// ---- effect instances -------------------------------------------------------------------------
// +4 its primitive instances (a list: +4 a timer, +8 the definition, +0xC the effect, +0x10
// flags: 1 dead), +8 its state for the primitives, +0x2C position, +0x38, +0x3C last
// position, +0x48 fade, +0x4C, +0x50 its definition, +0x54 flags: the low byte a countdown
// to being killed, 0x100 killed, 0x200 faded out, 0x400 fading out, 0x800 updated, 0x1000
// bounded

// 0x666C90 (thiscall): its primitive instances given back
void __fastcall Effect_ReleasePrimitives(uint8_t* self, re::Edx) {
    PrimInstanceList_Release(self + 4);
}

// 0x666CA0 (thiscall): every primitive killed (once)
void __fastcall Effect_KillPrimitivesImpl(uint8_t* self, re::Edx) {
    if (self[0x55] & 1) {
        return;
    }
    for (Link* prim = At<Link*>(self, 4); prim; prim = prim->next) {
        PrimInstance_Kill(prim);
    }
    At<uint16_t>(self, 0x54) |= 0x100;
}

// 0x666CD0 (thiscall): its particle systems' (kind 2) scale
void __fastcall Effect_SetScale(uint8_t* self, re::Edx, float x, float y, int z) {
    for (Link* prim = At<Link*>(self, 4); prim; prim = prim->next) {
        if (PrimInstance_GetKind(prim) == 2) {
            ParticleSystem_SetScale(prim, x, y, z);
        }
    }
}

// 0x666D10 / 0x666D50 / 0x666D90 / 0x666DD0 (thiscall): a value passed to its particle
// systems, unless killed
template <typename T, uintptr_t Fn>
void __fastcall Effect_ForParticleSystems(uint8_t* self, re::Edx, T value) {
    if (self[0x55] & 1) {
        return;
    }
    for (Link* prim = At<Link*>(self, 4); prim; prim = prim->next) {
        if (PrimInstance_GetKind(prim) == 2) {
            reinterpret_cast<void(__thiscall*)(void*, T)>(Fn)(prim, value);
        }
    }
}

// 0x666E10 (thiscall): killed after the time given (its countdown in 1/30 s steps: t / 8.5
// * 255, 1..255; a sooner one wins)
void __fastcall Effect_KillAfter(uint8_t* self, re::Edx, float time) {
    float scaled = time / 8.5f;
    scaled = scaled * 255.0f;
    int steps = int(scaled);   // (_ftol)
    if (steps < 1) {
        steps = 1;
    } else if (steps > 0xFF) {
        steps = 0xFF;
    }
    uint8_t countdown = uint8_t(At<uint16_t>(self, 0x54));
    if (steps < int(countdown) || countdown == 0) {
        self[0x54] = uint8_t(steps);
    }
}

// 0x666E60 (thiscall): its dead primitives removed and given back (all gone: killed)
void __fastcall Effect_RemoveDeadImpl(uint8_t* self, re::Edx) {
    Link* prim = At<Link*>(self, 4);
    Link* prev = nullptr;
    bool allDead = true;
    if (prim) {
        do {
            Link* next = prim->next;
            if (!(At<uint8_t>(prim, 0x10) & 1)) {
                allDead = false;
                prev = prim;
            } else {
                PrimInstance_Stop(prim);
                if (prev) {
                    prev->next = prim->next;
                } else {
                    List_PopFront(self + 4);
                }
                Effects_FreePrimInstance(effectManager_B6F1E0, prim);
            }
            prim = next;
        } while (prim);
        if (!allDead) {
            return;
        }
    }
    At<uint16_t>(self, 0x54) |= 0x100;
}

// 0x666EE0 (thiscall): the instance of its definition's n-th primitive
Link* __fastcall Effect_FindPrimitive(uint8_t* self, re::Edx, int n) {
    Link* def = At<Link*>(At<uint8_t*>(self, 0x50), 0x14);
    Link* found = nullptr;
    for (; n > 0; n--) {
        def = def->next;
    }
    uint8_t* definition = def ? reinterpret_cast<uint8_t*>(def) - 4 : nullptr;
    for (Link* prim = At<Link*>(self, 4); prim; prim = prim->next) {
        if (At<uint8_t*>(prim, 8) == definition) {
            found = prim;
        }
    }
    return found;
}

// 0x666F20 (thiscall): its primitives shown or hidden, unless killed
void __fastcall Effect_SetVisible(uint8_t* self, re::Edx, int visible) {
    if (self[0x55] & 1) {
        return;
    }
    for (Link* prim = At<Link*>(self, 4); prim; prim = prim->next) {
        PrimInstance_SetVisible(prim, visible);
    }
}

// 0x666F50 (thiscall)
void __fastcall Effect_SetBounded(uint8_t* self, re::Edx) {
    At<uint16_t>(self, 0x54) |= 0x1000;
}

// 0x666F60 (thiscall): not fading any more, the fade at 0
void __fastcall Effect_ClearFade(uint8_t* self, re::Edx) {
    At<uint16_t>(self, 0x54) &= 0xF9FF;
    At<float>(self, 0x48) = 0.0f;
}

// 0x6670D0 (thiscall): an effect instance's constructor
uint8_t* __fastcall Effect_Ctor(uint8_t* self, re::Edx) {
    PrimInstanceList_Construct(self);
    List_Init(self + 4);
    At<float>(self, 0x38) = 1.0f;
    self[0x54] = 0;
    At<float>(self, 0x48) = 1.0f;
    At<uint16_t>(self, 0x54) &= 0xE0FF;
    At<float>(self, 0x4C) = 1.0f;
    At<uint32_t>(self, 0x50) = 0;
    return self;
}

// 0x667100 (thiscall): the countdown, fading (0.1 a frame), its primitives updated (those
// past their end killed, the dead removed), the position kept as the last one
void __fastcall Effect_Update(uint8_t* self, re::Edx) {
    if (self[0x54] != 0) {
        self[0x54] -= 1;
        uint16_t flags = At<uint16_t>(self, 0x54);
        if (uint8_t(flags) == 0 && !(flags & 0x100)) {
            for (Link* prim = At<Link*>(self, 4); prim; prim = prim->next) {
                PrimInstance_Kill(prim);
            }
            At<uint16_t>(self, 0x54) |= 0x100;
        }
        if (uint8_t(At<uint16_t>(self, 0x54)) < 10) {
            // (the game then clears 0x200 unless fading: never, as it just set 0x400)
            At<uint16_t>(self, 0x54) |= 0x400;
        }
    }
    uint16_t flags = At<uint16_t>(self, 0x54);
    float& fade = At<float>(self, 0x48);
    if ((flags >> 10) & 1) {
        if (!(fade == 0.0f)) {
            float faded = fade - 0.1f;
            fade = faded;
            if (faded <= 0.0f) {
                fade = 0.0f;
                At<uint16_t>(self, 0x54) = uint16_t(flags | 0x200);
                if (At<uint8_t>(At<uint8_t*>(self, 0x50), 0x1C) & 8) {
                    Effect_KillPrimitives(self);
                }
            }
        }
    } else if (!(fade == 1.0f)) {
        float faded = fade + 0.1f;
        fade = faded;
        if (faded >= 1.0f) {
            fade = 1.0f;
        }
    }
    Link* prim = At<Link*>(self, 4);
    bool anyDead = false;
    if (prim) {
        do {
            Link* next = prim->next;
            if (At<uint8_t>(prim, 0x10) & 1) {
                anyDead = true;
            } else {
                uint8_t* definition = At<uint8_t*>(prim, 8);
                float elapsed = PrimTimer_Get(reinterpret_cast<uint8_t*>(prim) + 4) - At<float>(definition, 8);
                if (elapsed >= 0.0f) {
                    elapsed = PrimTimer_Get(reinterpret_cast<uint8_t*>(prim) + 4) - At<float>(definition, 8);
                    if (At<float>(definition, 0xC) <= elapsed) {
                        anyDead = true;
                        PrimInstance_Kill(prim);
                    } else {
                        PrimInstance_Update(prim, self + 8);
                    }
                }
            }
            prim = next;
        } while (prim);
        if (anyDead) {
            Effect_RemoveDead(self);
        }
    }
    At<uint16_t>(self, 0x54) |= 0x800;
    At<float>(self, 0x3C) = At<float>(self, 0x2C);
    At<float>(self, 0x40) = At<float>(self, 0x30);
    At<float>(self, 0x44) = At<float>(self, 0x34);
}

// 0x6672B0 (thiscall): started from a definition: its primitive instances made (each told
// its effect); bounded when their radius is under 1e6
uint8_t __fastcall Effect_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint16_t>(self, 0x54) &= 0xE0FF;
    At<float>(self, 0x48) = 1.0f;
    At<float>(self, 0x4C) = 1.0f;
    At<uint8_t*>(self, 0x50) = definition;
    self[0x54] = 0;
    uint8_t made = PrimInstanceList_Create(self + 4, definition + 0x14);
    if (!made) {
        return made;
    }
    for (Link* prim = At<Link*>(self, 4); prim; prim = prim->next) {
        At<uint8_t*>(prim, 0xC) = self;
    }
    if (EffectDefinition_MaxRadius(At<uint8_t*>(self, 0x50)) < 1000000.0f) {
        At<uint16_t>(self, 0x54) |= 0x1000;
    }
    return made;
}

// the archive's string (vf 0x24 fills it, up to `cap`)
template <int N>
struct ArchiveString {
    re::Ptr32<char> text;
    int32_t cap;
    char data[N];
};

// 0x667330 (cdecl): a definition type's name ("Effect", "Interpolator", ...; another
// type: the argument itself, as the game returns it)
const char* __cdecl DefinitionType_Name(uint32_t type) {
    switch (uint8_t(type)) {
    case 0: return reinterpret_cast<const char*>(0x0093D0EC);    // Effect
    case 1: return reinterpret_cast<const char*>(0x0093D0DC);    // Interpolator
    case 2: return reinterpret_cast<const char*>(0x0093D0CC);    // ParticleSystem
    case 3: return reinterpret_cast<const char*>(0x0093D0C4);    // Emitter
    case 4: return reinterpret_cast<const char*>(0x0093D0B4);    // Angle_Emitter
    case 5: return reinterpret_cast<const char*>(0x0093D0A8);    // ColorSize
    case 8: return reinterpret_cast<const char*>(0x0093D090);    // Attractor
    case 9: return reinterpret_cast<const char*>(0x0093D084);    // Friction
    case 0xA: return reinterpret_cast<const char*>(0x0093D07C);  // Jitter
    case 0xB: return reinterpret_cast<const char*>(0x0093B704);  // Gravity
    case 6: return reinterpret_cast<const char*>(0x0093D09C);    // Rectangle2d
    case 7: return reinterpret_cast<const char*>(0x008FFF9C);    // Rotation
    case 0xC: return reinterpret_cast<const char*>(0x0093D078);  // UV
    case 0xD: return reinterpret_cast<const char*>(0x0093D070);  // Light
    case 0xE: return reinterpret_cast<const char*>(0x0093D054);  // ParticleSystemSourceEmitter
    case 0xF: return reinterpret_cast<const char*>(0x0093D03C);  // SpawnEffectPrimitive
    case 0x10: return reinterpret_cast<const char*>(0x0093D028); // HeatHazePrimitive
    case 0x11: return reinterpret_cast<const char*>(0x0093D014); // MotionBlurPrimitive
    case 0x12: return reinterpret_cast<const char*>(0x0093D000); // PlaySoundPrimitive
    default: return reinterpret_cast<const char*>(uintptr_t(type));
    }
}

// 0x667400 (cdecl): a definition type by its name
uint8_t __cdecl DefinitionType_FromName(const char* name) {
    static constexpr uintptr_t names[] = {
        0x0093D0EC, 0x0093D0DC, 0x0093D0CC, 0x0093D0C4, 0x0093D0B4, 0x0093D0A8, 0x0093D09C,
        0x008FFF9C, 0x0093D090, 0x0093D084, 0x0093D07C, 0x0093B704, 0x0093D078, 0x0093D070,
        0x0093D054, 0x0093D03C, 0x0093D028, 0x0093D014, 0x0093D000,
    };
    for (uint8_t type = 0; type < 19; type++) {
        if (std::strcmp(name, reinterpret_cast<const char*>(names[type])) == 0) {
            return type;
        }
    }
    // (unknown: the game returns a byte of uninitialised stack)
    return 0;
}

// 0x667680 (cdecl): an effect's primitive definitions read (LIST_COUNT of them, each
// made by its DefinitionTypeName, then read) and appended to the list (their links at +4)
void __cdecl EffectDefinition_LoadPrimitives(void* archive, void* list) {
    int32_t count = 0;
    vc::Call<void>(archive, 0x38, &count, reinterpret_cast<const char*>(0x0093D52C));
    for (int i = 0; i < count; i++) {
        ArchiveString<0x41> typeName;
        typeName.text = typeName.data;
        typeName.cap = 0x41;
        typeName.data[0] = 0;
        vc::Call<void>(archive, 0x24, &typeName, 0x41, reinterpret_cast<const char*>(0x0093D4A4));
        uint8_t type = DefinitionType_FromName(typeName.text);
        uint8_t* definition = DefinitionStore_NewFn(definitionStore, type);
        Definition_ReadFn(definition, archive, reinterpret_cast<const char*>(0x0093D498));
        List_PushBack(list, definition ? definition + 4 : nullptr);
    }
}

// 0x667740 / 0x667750 (stdcall, no object): the two words of a table at 0xB41448 (8 bytes
// an entry)
uint32_t __fastcall Table_B41448_A(void*, re::Edx, int index) {
    return *reinterpret_cast<uint32_t*>(0x00B4144C + uint8_t(index) * 8);
}
uint32_t __fastcall Table_B41448_B(void*, re::Edx, int index) {
    return *reinterpret_cast<uint32_t*>(0x00B41450 + uint8_t(index) * 8);
}

// 0x667760 (cdecl): the camera's Gamebryo camera
void* __cdecl Effects_GetCamera() {
    return niCamera;
}

// ---- effect handles ---------------------------------------------------------------------------
// +0 (a list), +4 the effect instance (kind 0x16), +8 its suspended state (kind 0x17: +0x20
// the definition), +0xC, +0x10

// 0x667770 (thiscall)
uint8_t* __fastcall EffectHandle_Ctor(uint8_t* self, re::Edx) {
    PrimInstanceList_Construct(self);
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xC) = 0;
    self[0x10] = 0;
    return self;
}

// 0x6677A0 (thiscall): an instance of the definition started
uint8_t __fastcall EffectHandle_Start(uint8_t* self, re::Edx, uint8_t* definition) {
    uint8_t started = 0;
    uint8_t* effect = Effects_Alloc(effectManager_B6F1E0, 0x16);
    At<uint8_t*>(self, 4) = effect;
    if (effect) {
        started = Effect_InitFn(effect, definition);
        EffectCount_Add(effectCounter_B6F1D4);
    }
    return started;
}

// 0x6677E0 (thiscall): the instance (or its suspended state) given back
void __fastcall EffectHandle_Stop(uint8_t* self, re::Edx) {
    if (At<uint8_t*>(self, 4)) {
        Effect_ReleasePrimitivesFn(At<uint8_t*>(self, 4));
        Effects_FreePrimInstance(effectManager_B6F1E0, At<uint8_t*>(self, 4));
        At<uint8_t*>(self, 4) = nullptr;
        EffectCount_Remove(effectCounter_B6F1D4);
        return;
    }
    if (At<uint8_t*>(self, 8)) {
        NameKey_Destruct(At<uint8_t*>(self, 8));
        Effects_FreePrimInstance(effectManager_B6F1E0, At<uint8_t*>(self, 8));
        At<uint8_t*>(self, 8) = nullptr;
    }
}

// 0x667840 (thiscall): suspended (unless killed): its state kept, the instance given back
void __fastcall EffectHandle_Suspend(uint8_t* self, re::Edx) {
    if (At<uint8_t>(At<uint8_t*>(self, 4), 0x55) & 1) {
        return;
    }
    uint8_t* state = Effects_Alloc(effectManager_B6F1E0, 0x17);
    At<uint8_t*>(self, 8) = state;
    if (!state) {
        return;
    }
    EffectCount_Remove(effectCounter_B6F1D4);
    SuspendedEffect_Save(At<uint8_t*>(self, 8), At<uint8_t*>(self, 4) + 8);
    At<uint8_t*>(At<uint8_t*>(self, 8), 0x20) = At<uint8_t*>(At<uint8_t*>(self, 4), 0x50);
    Effect_ReleasePrimitivesFn(At<uint8_t*>(self, 4));
    Effects_FreePrimInstance(effectManager_B6F1E0, At<uint8_t*>(self, 4));
    At<uint8_t*>(self, 4) = nullptr;
}

// 0x6678B0 (thiscall): resumed from its suspended state (an instance of the same definition)
void __fastcall EffectHandle_Resume(uint8_t* self, re::Edx) {
    if (!At<uint8_t*>(At<uint8_t*>(self, 8), 0x20)) {
        return;
    }
    uint8_t* effect = Effects_Alloc(effectManager_B6F1E0, 0x16);
    At<uint8_t*>(self, 4) = effect;
    if (!effect) {
        return;
    }
    if (Effect_InitFn(effect, At<uint8_t*>(At<uint8_t*>(self, 8), 0x20))) {
        SuspendedEffect_Restore(At<uint8_t*>(self, 8), At<uint8_t*>(self, 4) + 8);
        NameKey_Destruct(At<uint8_t*>(self, 8));
        Effects_FreePrimInstance(effectManager_B6F1E0, At<uint8_t*>(self, 8));
        At<uint8_t*>(self, 8) = nullptr;
        EffectCount_Add(effectCounter_B6F1D4);
        Effect_ClearFadeFn(At<uint8_t*>(self, 4));
        return;
    }
    Effect_ReleasePrimitivesFn(At<uint8_t*>(self, 4));
    Effects_FreePrimInstance(effectManager_B6F1E0, At<uint8_t*>(self, 4));
    At<uint8_t*>(self, 4) = nullptr;
}

// 0x667950 (thiscall): killed (a suspended one forgets its definition)
void __fastcall EffectHandle_Kill(uint8_t* self, re::Edx) {
    if (At<uint8_t*>(self, 4)) {
        Effect_KillPrimitives(At<uint8_t*>(self, 4));
        return;
    }
    if (At<uint8_t*>(self, 8)) {
        At<uint32_t>(At<uint8_t*>(self, 8), 0x20) = 0;
    }
}

// ---- the effect definition manager (the effect library, 0xB6F1E8) ----------------------------
// +0 vtable, +4 count, +8 its effect definitions [400]

// 0x667970 (thiscall)
uint8_t* __fastcall EffectDefinitionManager_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectDefinitionManager;
    At<uint32_t>(self, 4) = 0;
    for (int i = 0; i < 400; i++) {
        At<uint32_t>(self, 8 + i * 4) = 0;
    }
    return self;
}

// 0x667990 (thiscall): its destructor
void __fastcall EffectDefinitionManager_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectDefinitionManager;
}

// 0x6679A0 (thiscall): a definition's index by its name's hash (0xFFFF: none; none either
// while creating fires, the high half of the result the caller's)
uint16_t __fastcall EffectDefinitionManager_FindHash(uint8_t* self, re::Edx, uint32_t hash) {
    if (fireCreate_BF3808) {
        return 0;
    }
    int32_t count = At<int32_t>(self, 4);
    for (int i = 0; i < count; i++) {
        if (At<uint32_t>(At<uint8_t*>(self, 8 + i * 4), 0x18) == hash) {
            return uint16_t(i);
        }
    }
    return 0xFFFF;
}

// 0x6679F0 (thiscall)
uint8_t* __fastcall EffectDefinitionManager_Get(uint8_t* self, re::Edx, int index) {
    return At<uint8_t*>(self, 8 + uint16_t(index) * 4);
}

// 0x667A00 (thiscall): every definition given back
void __fastcall EffectDefinitionManager_Clear(uint8_t* self, re::Edx) {
    for (int i = 0; i < At<int32_t>(self, 4); i++) {
        EffectDefinition_ReleaseFn(At<uint8_t*>(self, 8 + i * 4));
        DefinitionStore_Release(definitionStore, At<uint8_t*>(self, 8 + i * 4));
        At<uint32_t>(self, 8 + i * 4) = 0;
    }
    At<uint32_t>(self, 4) = 0;
}

// 0x667A50 (thiscall): its deleting destructor
uint8_t* __fastcall EffectDefinitionManager_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectDefinitionManager;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x667A70 (thiscall): by a name held in a string
uint16_t __fastcall EffectDefinitionManager_FindString(uint8_t* self, re::Edx, const re::Ptr32<const char>* name) {
    if (fireCreate_BF3808) {
        return 0;
    }
    return EffectDefinitionManager_FindHashFn(self, HashName(*name));
}

// 0x667AA0 (thiscall): the effects of an archive read (an "Effect" section each, until one
// isn't there)
void __fastcall EffectDefinitionManager_Load(uint8_t* self, re::Edx, const char* path) {
    int32_t mode = 0;
    void* archive = Archive_Open(path, &mode, 1);
    bool done = false;
    do {
        uint16_t index = At<uint16_t>(self, 4);
        At<uint8_t*>(self, 8 + At<int32_t>(self, 4) * 4) = DefinitionStore_NewFn(definitionStore, 0);
        At<int32_t>(self, 4) += 1;
        uint8_t* definition = At<uint8_t*>(self, 8 + index * 4);
        uint8_t loader[8];
        DefinitionBaseLoader_Construct(loader, definition);
        At<uint32_t>(loader, 0) = kVtEffectDefinitionLoader;
        if (!Archive_ReadSection(archive, loader, DefinitionType_NameFn(0))) {
            EffectDefinition_ReleaseFn(definition);
            DefinitionStore_Release(definitionStore, definition);
            At<int32_t>(self, 4) -= 1;
            done = true;
        }
        At<uint32_t>(loader, 0) = 0x00900CDC;   // (its base's destructor)
    } while (!done);
    Archive_Close(archive);
}

// 0x667B60 (thiscall): a definition's index by its name (copied to a 0x21 string first)
uint16_t __fastcall EffectDefinitionManager_Find(uint8_t* self, re::Edx, const char* name) {
    if (fireCreate_BF3808) {
        return 0;
    }
    ArchiveString<0x24> text;
    ArchiveString_Init(&text);
    Strncpy(text.text, name, uint32_t(text.cap));
    if (fireCreate_BF3808) {
        return 0;
    }
    return EffectDefinitionManager_FindHashFn(self, HashName(text.text));
}

// ---- the effect light manager: the 5 lights shown -------------------------------------------
// +0 vtable, +4 lights[5], +0x18 count. A light: +0xC its effect, +0x1C its slot (low 7
// bits), +0x20 flags (1: shown)

// 0x667BD0 (thiscall)
uint8_t* __fastcall EffectLightManager_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectLightManager;
    At<uint32_t>(self, 0x18) = 0;
    return self;
}

// 0x667BE0 (thiscall): its destructor
void __fastcall EffectLightManager_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectLightManager;
}

// 0x667BF0 (thiscall): the lights shown updated
void __fastcall EffectLightManager_Update(uint8_t* self, re::Edx) {
    for (int i = 0; i < At<int32_t>(self, 0x18); i++) {
        EffectLight_Update(At<uint8_t*>(self, 4 + i * 4));
    }
}

inline void SetLightSlot(uint8_t* light, uint32_t slot) {
    At<uint32_t>(light, 0x1C) ^= (At<uint32_t>(light, 0x1C) ^ slot) & 0x7F;
}

// 0x667C20 (thiscall): a light no longer shown (the last one moved to its slot)
void __fastcall EffectLightManager_Remove(uint8_t* self, re::Edx, uint8_t* light) {
    if (!(light[0x20] & 1)) {
        return;
    }
    uint32_t slot = At<uint32_t>(light, 0x1C) & 0x7F;
    uint8_t* last = At<uint8_t*>(self, At<int32_t>(self, 0x18) * 4);
    At<uint8_t*>(self, 4 + slot * 4)[0x20] &= 0xFE;
    At<uint8_t*>(self, 4 + slot * 4) = last;
    SetLightSlot(last, slot);
    At<int32_t>(self, 0x18) -= 1;
}

// 0x667C60 (thiscall): its deleting destructor
uint8_t* __fastcall EffectLightManager_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectLightManager;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

inline uint32_t Importance(uint8_t* effect) {
    return (At<uint8_t>(At<uint8_t*>(effect, 0x50), 0x1C) >> 4) & 7;
}

// 0x667C80 (thiscall): a light shown; with 5 already, in place of the least important one
// when it's more important (by its effect's importance, then +0x4C). (The lights are
// weighed against the first one's importance and +0x4C throughout, not the least so far's.)
void __fastcall EffectLightManager_Add(uint8_t* self, re::Edx, uint8_t* light) {
    if (light[0x20] & 1) {
        return;
    }
    int32_t count = At<int32_t>(self, 0x18);
    if (count < 5) {
        At<uint8_t*>(self, 4 + count * 4) = light;
        light[0x20] |= 1;
        SetLightSlot(light, uint32_t(At<int32_t>(self, 0x18)));
        At<int32_t>(self, 0x18) += 1;
        return;
    }
    uint8_t* first = At<uint8_t*>(At<uint8_t*>(self, 4), 0xC);
    uint32_t importance = Importance(first);
    float weight = At<float>(first, 0x4C);
    int weakest = 0;
    for (int i = 1; i < At<int32_t>(self, 0x18); i++) {
        uint8_t* effect = At<uint8_t*>(At<uint8_t*>(self, 4 + i * 4), 0xC);
        uint32_t other = Importance(effect);
        if (other > importance) {
            continue;
        }
        if (other < importance || At<float>(effect, 0x4C) < weight) {
            weakest = i;
        }
    }
    count = At<int32_t>(self, 0x18);
    uint8_t* effect = At<uint8_t*>(light, 0xC);
    uint32_t mine = Importance(effect);
    if (importance > mine) {
        return;
    }
    if (importance == mine && !(At<float>(effect, 0x4C) > weight)) {
        return;
    }
    uint8_t* last = At<uint8_t*>(self, count * 4);
    At<uint8_t*>(self, 4 + weakest * 4)[0x20] &= 0xFE;
    At<uint8_t*>(self, 4 + weakest * 4) = last;
    SetLightSlot(last, uint32_t(weakest));
    At<int32_t>(self, 0x18) -= 1;
    At<uint8_t*>(self, 4 + At<int32_t>(self, 0x18) * 4) = light;
    uint32_t slot = uint32_t(At<int32_t>(self, 0x18));
    light[0x20] |= 1;
    SetLightSlot(light, slot);
    At<int32_t>(self, 0x18) += 1;
}

// ---- lists ------------------------------------------------------------------------------------

// 0x667DA0 (thiscall): empty
re::Ptr32<Link>* __fastcall List_InitImpl(re::Ptr32<Link>* self, re::Edx) {
    *self = nullptr;
    return self;
}

// 0x667DB0 (thiscall): the first item taken off (its link cleared)
Link* __fastcall List_PopFrontImpl(re::Ptr32<Link>* self, re::Edx) {
    Link* first = *self;
    if (first) {
        *self = first->next;
        first->next = nullptr;
    }
    return first;
}
// 0x667DD0 (thiscall)
int __fastcall List_CountImpl(re::Ptr32<Link>* self, re::Edx) {
    int n = 0;
    for (Link* link = *self; link; link = link->next) {
        n++;
    }
    return n;
}
// 0x667DF0 (thiscall): an item appended
void __fastcall List_PushBackImpl(re::Ptr32<Link>* self, re::Edx, Link* item) {
    Link* link = *self;
    if (!link) {
        *self = item;
        return;
    }
    while (link->next) {
        link = link->next;
    }
    link->next = item;
}

// ---- the effect manager (0xB6F1D4) ------------------------------------------------------------

// 0x667E20 (thiscall)
uint8_t* __fastcall EffectManager_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectManager;
    self[4] = 0;
    self[5] = 1;
    At<uint32_t>(self, 8) = 0;
    self[0xC] = 1;
    Timer_Init(self + 0x10);
    List_Init(self + 0x14);
    At<uint32_t>(self, 0x18) = 0;
    Timer_Init(self + 0x1C);
    List_Init(self + 0x20);
    return self;
}

// 0x667E70 (thiscall): its destructor (its members' are empty)
void __fastcall EffectManager_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectManager;
    NameKey_Destruct(self + 0x20);
    NameKey_Destruct(self + 0x1C);
    NameKey_Destruct(self + 0x14);
    NameKey_Destruct(self + 0x10);
}

// an effect handle taken off the manager's list (its proxy detached) and given back
inline void RemoveHandle(uint8_t* self, Link* handle, Link* prev) {
    if (At<uint8_t*>(handle, 0xC)) {
        EffectProxy_Detach(At<uint8_t*>(handle, 0xC), reinterpret_cast<uint8_t*>(handle));
    }
    EffectHandle_StopFn(reinterpret_cast<uint8_t*>(handle));
    At<int32_t>(self, 0x18) -= 1;
    if (At<Link*>(self, 0x14) == handle) {
        List_PopFront(self + 0x14);
    } else {
        prev->next = handle->next;
    }
    Effects_FreePrimInstance(effectManager_B6F1E0, handle);
}

// 0x667EA0 (thiscall): the trails and lights updated, then the effects at 30 steps a second (2 at
// most a frame, the time left over kept): each effect updated (a faded out one only when
// it updates when faded), the killed ones (and suspended ones forgotten) removed
void __fastcall EffectManager_Update(uint8_t* self, re::Edx) {
    Trails_Update(trails_1B850E0);
    EffectLightManager_UpdateFn(effectLights_B6F1F4);
    uint8_t* timer = self + 0x10;
    self[4] = 1;
    if (!(PrimTimer_Get(timer) >= 0.033333335f)) {
        self[4] = 0;
        return;
    }
    int steps = int(PrimTimer_Get(timer) / 0.033333335f);   // (_ftol)
    float left = PrimTimer_Get(timer) - float(steps) * 0.033333335f;
    if (steps > 2) {
        steps = 2;
        left = 0.0f;
    }
    Timer_Reset(timer);
    Timer_Sub(timer, left);
    if (steps <= 0) {
        self[4] = 0;
        return;
    }
    for (int n = steps; n != 0; n--) {
        Link* prev = nullptr;
        for (Link* handle = At<Link*>(self, 0x14); handle;) {
            Link* next = handle->next;
            uint8_t* effect = At<uint8_t*>(handle, 4);
            if (effect) {
                if (!((At<uint16_t>(effect, 0x54) >> 9) & 1) || (At<uint8_t>(At<uint8_t*>(effect, 0x50), 0x1C) & 4)) {
                    Effect_UpdateFn(effect);
                }
                if (At<uint8_t>(At<uint8_t*>(handle, 4), 0x55) & 1) {
                    RemoveHandle(self, handle, prev);
                } else {
                    prev = handle;
                }
            } else if (At<uint32_t>(At<uint8_t*>(handle, 8), 0x20) == 0) {
                RemoveHandle(self, handle, prev);
            } else {
                prev = handle;
            }
            handle = next;
        }
    }
    self[4] = 0;
}

// 0x668050 (thiscall): the particle systems (kind 2, not flagged 2 or 4) of the live
// effects (not killed or faded out) given to the particle renderer
void __fastcall EffectManager_RenderParticles(uint8_t* self, re::Edx) {
    uint8_t* renderer = particleRenderer_B6F1EC;
    ParticleRenderer_Begin(renderer);
    for (Link* handle = At<Link*>(self, 0x14); handle;) {
        Link* next = handle->next;
        uint8_t* effect = At<uint8_t*>(handle, 4);
        if (effect) {
            uint32_t flags = At<uint16_t>(effect, 0x54);
            if (!(flags & 0x100) && !((flags >> 9) & 1)) {
                for (Link* prim = At<Link*>(effect, 4); prim; prim = prim->next) {
                    if (PrimInstance_GetKind(prim) == 2 && !(At<uint8_t>(prim, 0x10) & 6)) {
                        ParticleRenderer_Add(renderer, prim);
                    }
                }
            }
        }
        handle = next;
    }
}

// 0x6680C0 (thiscall): the handle of one proxy moved to another (the first's cleared)
void __fastcall EffectManager_ReplaceProxy(uint8_t* self, re::Edx, uint8_t* from, uint8_t* to) {
    for (Link* handle = At<Link*>(self, 0x14); handle; handle = handle->next) {
        if (At<uint8_t*>(handle, 0xC) == from) {
            At<uint32_t>(At<uint8_t*>(handle, 0xC), 4) = 0;
            At<uint8_t*>(handle, 0xC) = to;
            EffectProxy_Attach(to, reinterpret_cast<uint8_t*>(handle));
            return;
        }
    }
}

// 0x668100 (thiscall): a proxy let go of (its handles' and its own links cleared)
void __fastcall EffectManager_ForgetProxy(uint8_t* self, re::Edx, uint8_t* proxy) {
    for (Link* handle = At<Link*>(self, 0x14); handle; handle = handle->next) {
        uint8_t* own = At<uint8_t*>(handle, 0xC);
        if (own == proxy) {
            At<uint32_t>(own, 4) = 0;
            At<uint32_t>(handle, 0xC) = 0;
        }
    }
}

// 0x668130 (thiscall): its timers restarted
void __fastcall EffectManager_ResetTimers(uint8_t* self, re::Edx) {
    Timer_Reset(self + 0x10);
    Timer_Reset(self + 0x1C);
}

// 0x668150 (thiscall): its timers restarted, every effect removed
void __fastcall EffectManager_Clear(uint8_t* self, re::Edx) {
    Timer_Reset(self + 0x10);
    Timer_Reset(self + 0x1C);
    for (Link* handle = At<Link*>(self, 0x14); handle; handle = At<Link*>(self, 0x14)) {
        if (At<uint8_t*>(handle, 0xC)) {
            EffectProxy_Detach(At<uint8_t*>(handle, 0xC), reinterpret_cast<uint8_t*>(handle));
        }
        EffectHandle_StopFn(reinterpret_cast<uint8_t*>(handle));
        List_PopFront(self + 0x14);
        Effects_FreePrimInstance(effectManager_B6F1E0, handle);
    }
    At<uint32_t>(self, 0x18) = 0;
}

// 0x6681B0 (thiscall): an effect removed (during an update only killed)
void __fastcall EffectManager_Remove(uint8_t* self, re::Edx, Link* which) {
    if (self[4]) {
        EffectHandle_KillFn(reinterpret_cast<uint8_t*>(which));
        return;
    }
    self[4] = 1;
    Link* prev = nullptr;
    for (Link* handle = At<Link*>(self, 0x14); handle;) {
        Link* next = handle->next;
        if (handle == which) {
            EffectProxy_Detach(At<uint8_t*>(handle, 0xC), reinterpret_cast<uint8_t*>(handle));
            EffectHandle_StopFn(reinterpret_cast<uint8_t*>(handle));
            At<int32_t>(self, 0x18) -= 1;
            if (!prev) {
                List_PopFront(self + 0x14);
            } else {
                prev->next = handle->next;
            }
            Effects_FreePrimInstance(effectManager_B6F1E0, handle);
        }
        prev = handle;   // (the removed one too)
        handle = next;
    }
    self[4] = 0;
}

// 0x668230 / 0x668240 (thiscall): the number of effects running
void __fastcall EffectManager_AddCount(uint8_t* self, re::Edx) {
    At<int32_t>(self, 8) += 1;
}
void __fastcall EffectManager_RemoveCount(uint8_t* self, re::Edx) {
    At<int32_t>(self, 8) -= 1;
}
RE_UNHOOKED_FN("EffectManager::AddCount", 0x00668230, &EffectManager_AddCount);
RE_UNHOOKED_FN("EffectManager::RemoveCount", 0x00668240, &EffectManager_RemoveCount);

// 0x668250 (thiscall): its deleting destructor
uint8_t* __fastcall EffectManager_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectManager;
    NameKey_Destruct(self + 0x20);
    NameKey_Destruct(self + 0x1C);
    NameKey_Destruct(self + 0x14);
    NameKey_Destruct(self + 0x10);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x668290 (thiscall): an effect started for a proxy (none when -1; not when more are
// running than its importance allows: (importance + 1) * 55 / 5); the proxy told when not
void __fastcall EffectManager_Start(uint8_t* self, re::Edx, const int16_t* index, uint8_t* proxy, int flag) {
    if (*index == -1) {
        return;
    }
    uint8_t* handle = Effects_Alloc(effectManager_B6F1E0, 0xC);
    uint8_t* definition = EffectDefinitionManager_GetFn(effectLibrary_B6F1E8, uint16_t(*index));
    if (handle) {
        int allowed = int(((At<uint8_t>(definition, 0x1C) >> 4) & 7) + 1) * 0x37 / 5;
        if (allowed >= At<int32_t>(self, 8)) {
            handle[0x10] = uint8_t(flag);
            if (EffectHandle_StartFn(handle, definition)) {
                At<uint8_t*>(handle, 0xC) = proxy;
                EffectProxy_Attach(proxy, handle);
                At<int32_t>(self, 0x18) += 1;
                At<uint8_t*>(handle, 0) = At<uint8_t*>(self, 0x14);
                At<uint8_t*>(self, 0x14) = handle;
                return;
            }
        }
        EffectHandle_StopFn(handle);
        Effects_FreePrimInstance(effectManager_B6F1E0, handle);
    }
    EffectProxy_Failed(proxy);
}

// the camera's position
inline const float* CameraPosition() {
    return reinterpret_cast<const float*>(GetLocalTransform(GetCameraNode(niCamera)) + 0x24);
}
// how near an effect has to be to come back: 95% of its fade distance, or 5 m less when
// that's nearer
inline float ResumeDistance(uint8_t* definition) {
    float fade = At<float>(definition, 0x10);
    float resume = fade * 0.95f;
    if (5.0f < fade - resume) {
        resume = fade - 5.0f;
    }
    return resume;
}
inline int AllowedEffects(uint8_t* definition) {
    return int(((At<uint8_t>(definition, 0x1C) >> 4) & 7) + 1) * 0x37 / 5;
}

// 0x668360 (thiscall): an effect's detail (+0x4C: 1 near, down to 0 from its high to its
// low detail distance) and fading (beyond its fade distance or out of view); 0 when more
// effects run than its importance allows. With a camera mode the detail only grows.
void __fastcall EffectManager_UpdateLodImpl(uint8_t* self, re::Edx, uint8_t* effect) {
    float pos[3] = {At<float>(effect, 0x2C), At<float>(effect, 0x30), At<float>(effect, 0x34)};
    const float* camera = CameraPosition();
    float d[3] = {camera[0] - pos[0], camera[1] - pos[1], camera[2] - pos[2]};
    uint8_t* definition = At<uint8_t*>(effect, 0x50);
    float dist2 = d[1] * d[1] + d[0] * d[0];
    dist2 = dist2 + d[2] * d[2];
    float fade = At<float>(definition, 0x10);
    float resume = ResumeDistance(definition);
    float detail = 0.0f;
    bool noCameraMode = cameraMode_C936F8 == 0xFF;
    uint8_t fading = 1;
    float fade2 = fade * fade;
    if (fade2 > dist2 && Camera_IsSphereVisible(theCamera, pos, At<float>(definition, 0xC)) != 0) {
        bool near = true;
        if ((At<uint16_t>(effect, 0x54) >> 10) & 1) {
            float resume2 = resume * resume;
            near = resume2 > dist2;
        }
        if (near) {
            float high = At<float>(definition, 8);
            fading = 0;
            float low = At<float>(definition, 4);
            float high2 = high * high;
            float low2 = low * low;
            if (high2 < dist2 || high2 == dist2) {
                detail = 0.0f;
            } else if (low2 >= dist2) {
                detail = 1.0f;
            } else {
                float range = high - low;
                if (!(0.0f < range)) {
                    detail = 1.0f;
                } else {
                    float past = Vector_Magnitude(d) - low;
                    detail = 1.0f - past / range;
                }
            }
        }
    } else if (noCameraMode) {
        At<uint16_t>(effect, 0x54) |= 0x600;
        detail = 0.0f;
        At<float>(effect, 0x48) = 0.0f;
        if (At<uint8_t>(At<uint8_t*>(effect, 0x50), 0x1C) & 8) {
            Effect_KillPrimitives(effect);
        }
    }
    if (AllowedEffects(definition) < At<int32_t>(self, 8)) {
        detail = 0.0f;
    }
    uint32_t flags;
    if (noCameraMode) {
        At<float>(effect, 0x4C) = detail;
        flags = uint32_t(fading & 1) << 10 | (At<uint16_t>(effect, 0x54) & 0xFBFF);
        At<uint16_t>(effect, 0x54) = uint16_t(flags);
    } else if (!(detail > At<float>(effect, 0x4C))) {
        At<uint16_t>(effect, 0x54) &= 0xFBFF;
        flags = At<uint16_t>(effect, 0x54);
    } else {
        At<uint16_t>(effect, 0x54) &= 0xFBFF;
        At<float>(effect, 0x4C) = detail;
        flags = At<uint16_t>(effect, 0x54);
    }
    if (!(flags & 0x400) && (flags & 0x200)) {
        At<uint16_t>(effect, 0x54) = uint16_t(flags & 0xFDFF);
    }
}

// 0x668610 (thiscall): a faded out effect suspended (unless bounded); a suspended one
// resumed when back in range and in view
void __fastcall EffectManager_UpdateSuspensionImpl(uint8_t* self, re::Edx, uint8_t* handle) {
    (void)self;
    uint8_t* effect = At<uint8_t*>(handle, 4);
    if (effect) {
        uint32_t flags = At<uint16_t>(effect, 0x54);
        if (((flags >> 9) & 1) && !((flags >> 12) & 1)) {
            EffectHandle_SuspendFn(handle);
        }
        return;
    }
    uint8_t* state = At<uint8_t*>(handle, 8);
    uint8_t* definition = At<uint8_t*>(state, 0x20);
    if (!definition) {
        return;
    }
    float resume = ResumeDistance(definition);
    float pos[3] = {At<float>(state, 0x14), At<float>(state, 0x18), At<float>(state, 0x1C)};
    const float* camera = CameraPosition();
    float c[3] = {camera[0], camera[1], camera[2]};
    float d[3] = {c[0] - pos[0], c[1] - pos[1], c[2] - pos[2]};
    float dist2 = d[1] * d[1] + d[0] * d[0];
    dist2 = dist2 + d[2] * d[2];
    float resume2 = resume * resume;
    if (resume2 > dist2 && Camera_IsSphereVisible(theCamera, pos, At<float>(definition, 0xC)) != 0) {
        EffectHandle_ResumeFn(handle);
    }
}

// 0x668780 (thiscall): the effects of one pass (flag: those drawn after the scene, the
// handle's +0x10) drawn (the trails first): their particle systems given to the particle renderer when in
// view (not in mirrors when so flagged); every 0.2 s their detail and suspension updated
void __fastcall EffectManager_Render(uint8_t* self, re::Edx, int late) {
    uint8_t* renderer = particleRenderer_B6F1EC;
    Trails_Render(trails_1B850E0);
    ParticleRenderer_Begin(renderer);
    bool lod = false;
    if (self[0xC] == 0 && self[5] == 1 && PrimTimer_Get(self + 0x1C) > 0.2f) {
        lod = true;
        Timer_Reset(self + 0x1C);
    }
    uint8_t mirror = renderingMirror_C936FA;
    uint8_t old = Render_GetFlag_DA7519();
    Render_SetFlag_DA7519(late);
    for (Link* handle = At<Link*>(self, 0x14); handle;) {
        Link* next = handle->next;
        bool mine = re::Flag(late) ? At<uint8_t>(handle, 0x10) != 0 : At<uint8_t>(handle, 0x10) == 0;
        if (mine) {
            uint8_t* effect = At<uint8_t*>(handle, 4);
            if (effect) {
                if (lod) {
                    EffectManager_UpdateLod(self, effect);
                }
                uint32_t flags;
                if ((!mirror || !(At<uint8_t>(At<uint8_t*>(effect, 0x50), 0x1C) & 2)) &&
                    !((flags = At<uint16_t>(effect, 0x54)) & 0x100) && !((flags >> 9) & 1) &&
                    Camera_IsSphereVisible(theCamera, &At<float>(effect, 0x2C),
                                           At<float>(At<uint8_t*>(effect, 0x50), 0xC)) == 1) {
                    for (Link* prim = At<Link*>(effect, 4); prim; prim = prim->next) {
                        if (PrimInstance_GetKind(prim) == 2 && !(At<uint8_t>(prim, 0x10) & 6)) {
                            ParticleRenderer_Add(renderer, prim);
                        }
                    }
                }
            }
            if (lod) {
                EffectManager_UpdateSuspension(self, reinterpret_cast<uint8_t*>(handle));
            }
        }
        handle = next;
    }
    Render_SetFlag_DA7519(old);
    self[0xC] = 0;
}

// ---- an effect's suspended state (kind 0x17) --------------------------------------------------
// +0 (a list), +4 its rotation (from the effect's transform), +0x14 its position, +0x20 the
// definition

// 0x6688E0 (thiscall)
uint8_t* __fastcall SuspendedEffect_Ctor(uint8_t* self, re::Edx) {
    PrimInstanceList_Construct(self);
    At<float>(self, 0x14) = 0.0f;
    At<uint32_t>(self, 0x20) = 0;
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    return self;
}

// 0x668900 (thiscall): the effect's transform (+8 of it: its rotation, +0x24 position) kept
void __fastcall SuspendedEffect_SaveImpl(uint8_t* self, re::Edx, uint8_t* transform) {
    At<uint32_t>(self, 0x14) = At<uint32_t>(transform, 0x24);
    At<uint32_t>(self, 0x18) = At<uint32_t>(transform, 0x28);
    At<uint32_t>(self, 0x1C) = At<uint32_t>(transform, 0x2C);
    Quaternion_FromMatrix_776040(self + 4, transform);
}

// 0x668930 (thiscall): the transform given back (its scale 1)
void __fastcall SuspendedEffect_RestoreImpl(uint8_t* self, re::Edx, uint8_t* transform) {
    Quaternion_ToMatrix_413C50(self + 4, transform);
    At<float>(transform, 0x30) = 1.0f;
    At<uint32_t>(transform, 0x24) = At<uint32_t>(self, 0x14);
    At<uint32_t>(transform, 0x28) = At<uint32_t>(self, 0x18);
    At<uint32_t>(transform, 0x2C) = At<uint32_t>(self, 0x1C);
}

// ---- primitive definitions and instances ------------------------------------------------------
// A primitive definition: +0 its type (0xFF: none), +4 its link, +8 start time, +0xC
// duration. An instance: +0 its link, +4 its timer, +8 its definition, +0xC its effect,
// +0x10 flags (1 dead, 2 and 4 hidden)

// 0x668970 (thiscall)
uint8_t* __fastcall PrimDefinition_Ctor(uint8_t* self, re::Edx) {
    self[0] = 0xFF;
    List_Init(self + 4);
    At<float>(self, 8) = 0.0f;
    At<float>(self, 0xC) = 0.0f;
    return self;
}

// 0x668990 (thiscall): EffectPrimitiveDefinitionLoader's vf1: StartTime, Duration and the
// description (read and dropped)
void __fastcall EffectPrimitiveDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    vc::Call<void>(archive, 0x28, definition + 8, startTimeName_93D550);
    vc::Call<void>(archive, 0x28, definition + 0xC, durationName_93D554);
    vc::Call<void>(archive, 0x24, static_cast<void*>(nullptr), 0, descriptionName_93D5E8);
}

// 0x6689E0 (thiscall)
uint8_t* __fastcall PrimInstance_Ctor(uint8_t* self, re::Edx) {
    PrimInstanceList_Construct(self);
    Timer_Init(self + 4);
    self[0x10] &= 0xF8;
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xC) = 0;
    return self;
}

// 0x668A00 (thiscall): its destructor (its members' are empty)
void __fastcall PrimInstance_Dtor(uint8_t* self, re::Edx) {
    NameKey_Destruct(self + 4);
    NameKey_Destruct(self);
}

// 0x668A20 (thiscall): restarted
void __fastcall PrimInstance_Restart(uint8_t* self, re::Edx) {
    Timer_Reset(self + 4);
    self[0x10] &= 0xF8;
}

// 0x668A40 (thiscall)
void __fastcall PrimInstance_KillImpl(uint8_t* self, re::Edx) {
    self[0x10] |= 1;
}

// 0x668A50 (thiscall): updated by its kind
void __fastcall PrimInstance_UpdateImpl(uint8_t* self, re::Edx, void* effectState) {
    uintptr_t fn;
    switch (PrimInstance_GetKind(self)) {
    case 2: fn = 0x00670870; break;     // particle system
    case 0xF: fn = 0x0066B940; break;   // light
    case 0x12: fn = 0x0066E3B0; break;  // spawned effect
    case 0x13: fn = 0x0066A720; break;  // heat haze
    case 0x14: fn = 0x0066D820; break;  // motion blur
    case 0x15: fn = 0x0066DDB0; break;  // sound
    default: return;
    }
    reinterpret_cast<void(__thiscall*)(uint8_t*, void*)>(fn)(self, effectState);
}

// 0x668AD0 (thiscall): hidden or shown (flags 2 and 4)
void __fastcall PrimInstance_SetVisibleImpl(uint8_t* self, re::Edx, int hidden) {
    self[0x10] ^= uint8_t((uint8_t(hidden) * 2) ^ self[0x10]) & 6;
}

// 0x668AF0 (thiscall): its index in its effect (the last match; -1: none)
int __fastcall PrimInstance_Index(uint8_t* self, re::Edx) {
    int index = -1;
    int count = EffectDefinition_NumPrimitivesFn(At<uint8_t*>(At<uint8_t*>(self, 0xC), 0x50));
    for (int i = 0; i < count; i++) {
        if (reinterpret_cast<uint8_t*>(Effect_FindPrimitiveFn(At<uint8_t*>(self, 0xC), i)) == self) {
            index = i;
        }
    }
    return index;
}

// ---- effect proxies: +0 vtable, +4 the effect handle -------------------------------------------

// 0x668B30 (thiscall)
uint8_t* __fastcall EffectProxy_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectProxy;
    At<uint32_t>(self, 4) = 0;
    return self;
}

// 0x668B40 (thiscall): its destructor (its effect let go of)
void __fastcall EffectProxy_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectProxy;
    if (At<uint32_t>(self, 4)) {
        EffectManager_ForgetProxyFn(effectCounter_B6F1D4, self);
    }
}

// 0x668B60 (thiscall): its effect handle set, then told (vf1)
void __fastcall EffectProxy_AttachImpl(uint8_t* self, re::Edx, uint8_t* handle) {
    At<uint8_t*>(self, 4) = handle;
    vc::Call<void>(self, 4);
}

// 0x668B80 (thiscall): told (vf2) its effect is gone, then cleared
void __fastcall EffectProxy_DetachImpl(uint8_t* self, re::Edx, uint8_t*) {
    if (At<uint32_t>(self, 4)) {
        vc::Call<void>(self, 8);
        At<uint32_t>(self, 4) = 0;
    }
}

// 0x668BA0 (thiscall): its effect killed and removed
void __fastcall EffectProxy_Kill(uint8_t* self, re::Edx) {
    if (!At<uint8_t*>(self, 4)) {
        return;
    }
    if (At<uint8_t*>(At<uint8_t*>(self, 4), 4)) {
        Effect_KillPrimitives(At<uint8_t*>(At<uint8_t*>(self, 4), 4));
    }
    EffectManager_RemoveFn(effectCounter_B6F1D4, At<uint8_t*>(self, 4));
    At<uint32_t>(self, 4) = 0;
}

// 0x668BD0 (thiscall): its effect's scale (the effect then bounded)
void __fastcall EffectProxy_SetScale(uint8_t* self, re::Edx, float x, float y, int z) {
    if (At<uint8_t*>(self, 4) && At<uint8_t*>(At<uint8_t*>(self, 4), 4)) {
        Effect_SetBoundedFn(At<uint8_t*>(At<uint8_t*>(self, 4), 4));
        Effect_SetScaleFn(At<uint8_t*>(At<uint8_t*>(self, 4), 4), x, y, z);
    }
}

// 0x668C10 / 0x668C40 / 0x668C70 / 0x668CA0 / 0x668CD0 (thiscall): a value passed to its
// effect (then bounded) when it has a live one
template <typename T, uintptr_t Fn>
void __fastcall EffectProxy_ForEffect(uint8_t* self, re::Edx, T value) {
    if (At<uint8_t*>(self, 4) && At<uint8_t*>(At<uint8_t*>(self, 4), 4)) {
        Effect_SetBoundedFn(At<uint8_t*>(At<uint8_t*>(self, 4), 4));
        reinterpret_cast<void(__thiscall*)(uint8_t*, T)>(Fn)(At<uint8_t*>(At<uint8_t*>(self, 4), 4), value);
    }
}

// 0x668D00 (thiscall): its effect killed after a time (a suspended one removed at once)
void __fastcall EffectProxy_KillAfter(uint8_t* self, re::Edx, float time) {
    uint8_t* handle = At<uint8_t*>(self, 4);
    if (!handle) {
        return;
    }
    if (At<uint8_t*>(handle, 4)) {
        Effect_SetBoundedFn(At<uint8_t*>(handle, 4));
        Effect_KillAfterFn(At<uint8_t*>(At<uint8_t*>(self, 4), 4), time);
        return;
    }
    if (At<uint8_t*>(handle, 4)) {   // (never: checked just above)
        Effect_KillPrimitives(At<uint8_t*>(handle, 4));
    }
    EffectManager_RemoveFn(effectCounter_B6F1D4, At<uint8_t*>(self, 4));
    At<uint32_t>(self, 4) = 0;
}

// 0x668D60 (thiscall): its deleting destructor
uint8_t* __fastcall EffectProxy_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectProxy;
    if (At<uint32_t>(self, 4)) {
        EffectManager_ForgetProxyFn(effectCounter_B6F1D4, self);
    }
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// ---- EffectProxyAttach: +8 the node it's attached to, +0xC the clump ---------------------------

// 0x668DA0 (thiscall)
uint8_t* __fastcall EffectProxyAttach_Ctor(uint8_t* self, re::Edx) {
    EffectProxy_Construct(self);
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xC) = 0;
    At<uint32_t>(self, 0) = kVtEffectProxyAttach;
    return self;
}

// 0x668DC0 (thiscall): its destructor
void __fastcall EffectProxyAttach_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectProxyAttach;
    EffectProxy_Destruct(self);
}

// 0x668DD0 (thiscall): vf1 (attached): its effect bounded
void __fastcall EffectProxyAttach_Attached(uint8_t* self, re::Edx) {
    Effect_SetBoundedFn(At<uint8_t*>(EffectProxy_GetHandle(self), 4));
}

// 0x668DE0 (thiscall)
void __fastcall EffectProxyAttach_Set(uint8_t* self, re::Edx, uint8_t* clump, void* node) {
    At<void*>(self, 8) = node;
    At<uint8_t*>(self, 0xC) = clump;
}

// 0x668E00 (thiscall): its effect's transform
uint8_t* __fastcall EffectProxy_GetTransform(uint8_t* self, re::Edx) {
    return At<uint8_t*>(EffectProxy_GetHandle(self), 4) + 8;
}

// 0x668E10 (thiscall): its deleting destructor
uint8_t* __fastcall EffectProxyAttach_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectProxyAttach;
    EffectProxy_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x668E40 (thiscall): vf2 (detached): the node taken off the clump's animation
void __fastcall EffectProxyAttach_Detached(uint8_t* self, re::Edx) {
    void* node = At<void*>(self, 8);
    uint8_t* clump = At<uint8_t*>(self, 0xC);
    if (node && clump && At<uint32_t>(clump, 0xC4) && *At<uint32_t*>(clump, 0xB0)) {
        AnimManager_Detach(animManager, clump, node);
    }
    At<uint32_t>(self, 8) = 0;
    At<uint32_t>(self, 0xC) = 0;
}

// 0x668E90 (thiscall)
uint8_t* __fastcall EffectProxySetLocation_Ctor(uint8_t* self, re::Edx) {
    EffectProxy_Construct(self);
    At<uint32_t>(self, 0) = kVtEffectProxySetLocation;
    return self;
}

// 0x668EB0 (thiscall): EffectProxySetLocation's destructor
void __fastcall EffectProxySetLocation_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtEffectProxySetLocation;
    EffectProxy_Destruct(self);
}

// the effect's detail updated after it moved; faded out at once when fading
inline void AfterMove(uint8_t* self) {
    EffectManager_UpdateLod(effectCounter_B6F1D4, At<uint8_t*>(EffectProxy_GetHandle(self), 4));
    if ((At<uint16_t>(At<uint8_t*>(EffectProxy_GetHandle(self), 4), 0x54) >> 10) & 1) {
        uint8_t* effect = At<uint8_t*>(EffectProxy_GetHandle(self), 4);
        At<float>(effect, 0x48) = 0.0f;
        At<uint16_t>(effect, 0x54) |= 0x600;
        if (At<uint8_t>(At<uint8_t*>(effect, 0x50), 0x1C) & 8) {
            Effect_KillPrimitives(effect);
        }
    }
}

// 0x668EC0 (thiscall): its effect's transform set (a suspended one's kept)
void __fastcall EffectProxy_SetTransform(uint8_t* self, re::Edx, const uint8_t* transform) {
    if (!At<uint32_t>(self, 4)) {
        return;
    }
    if (!At<uint8_t*>(EffectProxy_GetHandle(self), 4)) {
        SuspendedEffect_Save(At<uint8_t*>(EffectProxy_GetHandle(self), 8), const_cast<uint8_t*>(transform));
    } else {
        uint8_t* effect = At<uint8_t*>(EffectProxy_GetHandle(self), 4);
        for (int i = 0; i < 0xD; i++) {
            At<uint32_t>(effect, 8 + i * 4) = *reinterpret_cast<const uint32_t*>(transform + i * 4);
        }
        AfterMove(self);
    }
    EffectManager_UpdateSuspension(effectCounter_B6F1D4, EffectProxy_GetHandle(self));
}

// 0x668F80 (cdecl): a screen position (640x480, +8 the depth) to the world, through the
// camera's inverse transform
void __cdecl ScreenToWorldImpl(float* v) {
    v[0] = (v[2] / 640.0f) * v[0];
    v[1] = (v[2] / 480.0f) * v[1];
    uint8_t inverse[0x34];
    At<float>(inverse, 0x30) = 1.0f;
    MatrixInvert(inverse, GetCameraWorldTransform(niCamera));
    float in[3] = {v[0], v[1], v[2]};
    float out[3];
    NiTransform_Transform(inverse, out, in);
    v[0] = out[0];
    v[1] = out[1];
    v[2] = out[2];
}

// 0x669010 (thiscall): EffectProxySetLocation's deleting destructor
uint8_t* __fastcall EffectProxySetLocation_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectProxySetLocation;
    EffectProxy_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x669040 (thiscall): its effect moved to a position (a screen one first taken to the
// world); its rotation reset
void __fastcall EffectProxy_SetPosition(uint8_t* self, re::Edx, const float* position, int screen) {
    if (!At<uint32_t>(self, 4)) {
        return;
    }
    float p[3] = {position[0], position[1], position[2]};
    if (re::Flag(screen)) {
        ScreenToWorld(p);
    }
    if (At<uint8_t*>(EffectProxy_GetHandle(self), 4)) {
        Matrix3_MakeIdentity(At<uint8_t*>(EffectProxy_GetHandle(self), 4) + 8);
        CombineTranslation(At<uint8_t*>(EffectProxy_GetHandle(self), 4) + 8, p, 0);
        AfterMove(self);
    } else {
        uint8_t transform[0x34];
        At<float>(transform, 0x30) = 1.0f;
        SuspendedEffect_Restore(At<uint8_t*>(EffectProxy_GetHandle(self), 8), transform);
        At<float>(transform, 0x24) = p[0];
        At<float>(transform, 0x28) = p[1];
        At<float>(transform, 0x2C) = p[2];
        SuspendedEffect_Save(At<uint8_t*>(EffectProxy_GetHandle(self), 8), transform);
    }
    EffectManager_UpdateSuspension(effectCounter_B6F1D4, EffectProxy_GetHandle(self));
}

// 0x669170 (cdecl): a rotation turned to face a direction (its column 1), keeping its
// column 2 as the up (its column 1 when the direction is along it)
void __cdecl Matrix_OrientToImpl(float* m, const float* direction) {
    float a[3] = {m[2], m[5], m[8]};
    float b[3] = {m[1], m[4], m[7]};
    float f[3] = {direction[0], direction[1], direction[2]};
    float saved[4];
    saved[0] = b[0];
    saved[1] = b[1];
    saved[2] = b[2];
    Vec_Normalise(f);
    float up[4];
    up[0] = a[0];
    up[1] = a[1];
    up[2] = a[2];
    a[0] = -a[0];
    a[1] = -a[1];
    a[2] = -a[2];
    if (Vector_NearlyEqual(f, up, 0.01f) || Vector_NearlyEqual(f, a, 0.01f)) {
        up[0] = saved[0];
        up[1] = saved[1];
        up[2] = saved[2];
        // (the game also copies the uninitialised word after them)
    }
    float r[3];
    r[0] = up[1] * f[2] - up[2] * f[1];
    r[1] = up[2] * f[0] - f[2] * up[0];
    r[2] = f[1] * up[0] - up[1] * f[0];
    Vec_Normalise(r);
    float u[3];
    u[0] = r[2] * f[1] - r[1] * f[2];
    u[1] = f[2] * r[0] - r[2] * f[0];
    u[2] = f[0] * r[1] - r[0] * f[1];   // (the game's operand order: its z is negated against x and y)
    Vec_Normalise(u);
    m[1] = f[0];
    m[4] = f[1];
    m[7] = f[2];
    m[2] = u[0];
    m[5] = u[1];
    m[8] = u[2];
    m[0] = r[0];
    m[3] = r[1];
    m[6] = r[2];
}

// 0x669360 (thiscall): its effect turned to face a direction (a suspended one's kept)
void __fastcall EffectProxy_SetDirection(uint8_t* self, re::Edx, const float* direction) {
    if (!At<uint32_t>(self, 4)) {
        return;
    }
    uint8_t transform[0x34];
    At<float>(transform, 0x30) = 1.0f;
    uint8_t* target;
    if (At<uint8_t*>(EffectProxy_GetHandle(self), 4)) {
        target = At<uint8_t*>(EffectProxy_GetHandle(self), 4) + 8;
    } else {
        target = transform;
        SuspendedEffect_Restore(At<uint8_t*>(EffectProxy_GetHandle(self), 8), target);
    }
    Matrix_OrientTo(reinterpret_cast<float*>(target), direction);
    if (At<uint8_t*>(EffectProxy_GetHandle(self), 8)) {
        SuspendedEffect_Save(At<uint8_t*>(EffectProxy_GetHandle(self), 8), target);
    }
}

// 0x6693E0 (thiscall): a list's constructor
re::Ptr32<Link>* __fastcall ListHead_Ctor(re::Ptr32<Link>* self, re::Edx) {
    List_Init(self);
    return self;
}
// 0x6693F0: a jump to the empty destructor 0x49A650
RE_UNHOOKED("EffectList::Destruct_6693F0", 0x006693F0);
RE_UNHOOKED("EffectList::Destruct_667790", 0x00667790);

// ---- the effect pools (0xB6F1E0): 24 kinds of fixed-size objects in static arrays ------------
// +0 arrays[24], +0x60 sizes, +0x120 counts, +0x1E0 their ends, +0x240 free lists

// 0x669400 (thiscall): an object's kind (by the pool it's in)
uint8_t __fastcall PrimInstance_GetKindImpl(uint8_t* self, re::Edx) {
    return EffectPools_KindOf(effectManager_B6F1E0, self);
}

// 0x669410 (thiscall): a free object of a kind (null: none left)
uint8_t* __fastcall EffectPools_Alloc(uint8_t* self, re::Edx, int kind) {
    re::Ptr32<Link>* head = &At<Link*>(self, 0x240 + uint8_t(kind) * 4);
    Link* first = *head;
    if (first) {
        List_PopFront(head);
    }
    return reinterpret_cast<uint8_t*>(first);
}

// 0x669430 (thiscall): an object by kind and index
uint8_t* __fastcall EffectPools_Get(uint8_t* self, re::Edx, int index, int kind) {
    uint8_t k = uint8_t(kind);
    return At<uint8_t*>(self, k * 4) + At<int32_t>(self, 0x60 + k * 4) * index;
}

struct PoolInfo {
    uint32_t offset;   // in the storage at 0x1B65D50
    int32_t count;
    int32_t size;
    uintptr_t ctor;
};
constexpr PoolInfo kPools[24] = {
    {0x0, 600, 0x14, 0x0066AA60},     {0x2EE0, 250, 0x14, 0x006713A0},  {0x4268, 90, 0x48, 0x0066F970},
    {0x5BB8, 1, 4, 0x006693E0},       {0x5BBC, 90, 0x14, 0x0066EA40},   {0x62C4, 250, 0x1C, 0x00671E50},
    {0x7E1C, 140, 0x20, 0x00672150},  {0x8F9C, 190, 0x1C, 0x00672360},  {0xA464, 50, 0x1C, 0x006716E0},
    {0xA9DC, 100, 0x10, 0x006718D0},  {0xB01C, 70, 0x10, 0x00671BA0},   {0xB47C, 90, 0x10, 0x00671A70},
    {0xBA1C, 200, 0x14, 0x00667770},  {0xC9BC, 30, 0x38, 0x00672730},   {0xD04C, 30, 0xF8, 0x00671350},
    {0xED5C, 20, 0x24, 0x0066B910},   {0xF02C, 10, 8, 0x00670950},      {0xF07C, 20, 0x28, 0x00670A40},
    {0xF39C, 20, 0x24, 0x0066DFB0},   {0xF66C, 30, 0x1C, 0x0066A670},   {0xF9B4, 10, 0x38, 0x0066D5F0},
    {0xFBE4, 10, 0x28, 0x0066DD90},   {0xFD74, 55, 0x58, 0x006670D0},   {0x1105C, 150, 0x24, 0x006688E0},
};

// each pool's first link: its first object, past the vtable for the kinds with one (5..11, 13)
constexpr uint32_t kPoolLinks[24] = {
    0x01B65D50, 0x01B68C30, 0x01B69FB8, 0x01B6B908, 0x01B6B90C, 0x01B6C018, 0x01B6DB70, 0x01B6ECF0,
    0x01B701B8, 0x01B70730, 0x01B70D70, 0x01B711D0, 0x01B7176C, 0x01B72710, 0x01B72D9C, 0x01B74AAC,
    0x01B74D7C, 0x01B74DCC, 0x01B750EC, 0x01B753BC, 0x01B75704, 0x01B75934, 0x01B75AC4, 0x01B76DAC,
};

// 0x669450 (thiscall): the pools set up over the storage at 0x1B65D50, every object put on
// its kind's free list (the list's old head, not cleared here, under the first one)
void __fastcall EffectPools_Init(uint8_t* self, re::Edx) {
    for (int i = 0; i < 24; i++) {
        At<int32_t>(self, 0x120 + i * 4) = kPools[i].count;
        At<int32_t>(self, 0x60 + i * 4) = kPools[i].size;
        At<uint32_t>(self, i * 4) = kPoolLinks[i];
    }
    for (int i = 0; i < 24; i++) {
        At<uint32_t>(self, 0x1E0 + i * 4) =
            uint32_t(At<int32_t>(self, 0x120 + i * 4) * At<int32_t>(self, 0x60 + i * 4)) + At<uint32_t>(self, i * 4);
    }
    for (int kind = 0; kind < 24; kind++) {
        for (int i = 0; i < At<int32_t>(self, 0x120 + kind * 4); i++) {
            uint8_t* object = At<uint8_t*>(self, uint8_t(kind) * 4) + At<int32_t>(self, 0x60 + uint8_t(kind) * 4) * i;
            At<uint32_t>(object, 0) = At<uint32_t>(self, 0x240 + kind * 4);
            At<uint8_t*>(self, 0x240 + kind * 4) = object;
        }
    }
}

// 0x6696E0 (thiscall): the kind of pool an object is in (0xFF: none; kind 5 checked first)
uint8_t __fastcall EffectPools_KindOfImpl(uint8_t* self, re::Edx, uintptr_t p) {
    if (p >= At<uint32_t>(self, 0x14) && p < At<uint32_t>(self, 0x1F4)) {
        return 5;
    }
    for (int i = 0; i < 24; i++) {
        if (p >= At<uint32_t>(self, i * 4) && p < At<uint32_t>(self, 0x1E0 + i * 4)) {
            return uint8_t(i);
        }
    }
    return 0xFF;
}

// 0x669720 (thiscall): an object's index in its pool
int __fastcall EffectPools_IndexOf(uint8_t* self, re::Edx, int32_t p, int kind) {
    uint8_t k = uint8_t(kind);
    return (p - At<int32_t>(self, k * 4)) / At<int32_t>(self, 0x60 + k * 4);
}

// 0x669740 (thiscall): the pools' storage constructed (every object's constructor)
uint8_t* __fastcall EffectPoolStorage_Ctor(uint8_t* self, re::Edx) {
    for (const PoolInfo& pool : kPools) {
        for (int i = 0; i < pool.count; i++) {
            reinterpret_cast<void(__thiscall*)(uint8_t*)>(pool.ctor)(self + pool.offset + i * pool.size);
        }
    }
    return self;
}

struct PoolDtor {
    uint32_t end;   // the pool's end in the storage
    int32_t count;
    int32_t size;
    uintptr_t dtor;
};
// the pools' storage's destruction: last pool first, each from its last object
constexpr PoolDtor kPoolDtors[] = {
    {0x12574, 150, 0x24, 0x00667790}, {0x1105C, 55, 0x58, 0x00668A00}, {0xFD74, 10, 0x28, 0x0066DC70},
    {0xFBE4, 10, 0x38, 0x0066D290},   {0xF9B4, 30, 0x1C, 0x0066A690},  {0xF66C, 20, 0x24, 0x0066DFF0},
    {0xF39C, 20, 0x28, 0x00667790},   {0xF07C, 10, 8, 0x00667790},     {0xF02C, 20, 0x24, 0x0066A690},
    {0xED5C, 30, 0xF8, 0x00667790},   {0xD04C, 30, 0x38, 0x006726C0},  {0xC9BC, 200, 0x14, 0x00667790},
    {0xBA1C, 90, 0x10, 0x00671A90},   {0xB47C, 70, 0x10, 0x00671BC0},  {0xB01C, 100, 0x10, 0x006718F0},
    {0xA9DC, 50, 0x1C, 0x00671630},   {0xA464, 190, 0x1C, 0x00672390}, {0x8F9C, 140, 0x20, 0x00672190},
    {0x7E1C, 250, 0x1C, 0x00671EA0},  {0x62C4, 90, 0x14, 0x0066EA90},  {0x5BBC, 1, 4, 0x006693F0},
    {0x5BB8, 90, 0x48, 0x0066F9D0},   {0x4268, 250, 0x14, 0x006713D0}, {0x2EE0, 600, 0x14, 0x00667790},
};

// 0x669A30 (thiscall): the pools' storage destroyed
void __fastcall EffectPoolStorage_Dtor(uint8_t* self, re::Edx) {
    for (const PoolDtor& pool : kPoolDtors) {
        uint8_t* object = self + pool.end;
        for (int i = 0; i < pool.count; i++) {
            object -= pool.size;
            reinterpret_cast<void(__thiscall*)(uint8_t*)>(pool.dtor)(object);
        }
    }
}

// 0x669D30 (thiscall): the pools' constructor (every table cleared, then set up)
uint8_t* __fastcall EffectPools_Ctor(uint8_t* self, re::Edx) {
    for (int i = 0; i < 24; i++) {
        List_Init(self + 0x240 + i * 4);
    }
    for (int i = 0; i < 24; i++) {
        At<uint32_t>(self, i * 4) = 0;
        At<uint32_t>(self, 0x60 + i * 4) = 0;
        At<uint32_t>(self, 0x120 + i * 4) = 0;
        At<uint32_t>(self, 0xC0 + i * 4) = 0;
        At<uint32_t>(self, 0x180 + i * 4) = 0;
        At<uint32_t>(self, 0x1E0 + i * 4) = 0;
    }
    EffectPools_InitFn(self);
    return self;
}

// 0x669D90 (thiscall): the pools' destructor (their free lists', which are empty)
void __fastcall EffectPools_Dtor(uint8_t* self, re::Edx) {
    for (int i = 23; i >= 0; i--) {
        NameKey_Destruct(self + 0x240 + i * 4);
    }
}

// 0x669DC0 (thiscall): an object given back to its pool's free list (one in no pool goes
// to a list past the table, as the game does)
void __fastcall EffectPools_FreeImpl(uint8_t* self, re::Edx, Link* object) {
    uintptr_t p = reinterpret_cast<uintptr_t>(object);
    uint8_t kind = 0xFF;
    if (p >= At<uint32_t>(self, 0x14) && p < At<uint32_t>(self, 0x1F4)) {
        kind = 5;
    } else {
        for (int i = 0; i < 24; i++) {
            if (p >= At<uint32_t>(self, i * 4) && p < At<uint32_t>(self, 0x1E0 + i * 4)) {
                kind = uint8_t(i);
                break;
            }
        }
    }
    re::Ptr32<Link>* head = &At<Link*>(self, 0x240 + kind * 4);
    object->next = *head;
    *head = object;
}

// 0x669E20 / 0x669E40 (thiscall): a string over its own buffer (0x21 / 0x33 characters)
template <int Cap>
uint8_t* __fastcall ArchiveString_Ctor(uint8_t* self, re::Edx) {
    At<uint8_t*>(self, 0) = self + 8;
    At<int32_t>(self, 4) = Cap;
    self[8] = 0;
    return self;
}

// 0x669E60 (thiscall): a string made lower case (through a 32-byte buffer, unchecked)
void __fastcall ArchiveString_ToLower(uint8_t* self, re::Edx) {
    char lower[0x20] = {};
    const char* text = At<const char*>(self, 0);
    char* out = lower;
    for (; *text; text++) {
        *out++ = char(ToLower(*text));
    }
    *out = 0;
    Strncpy(At<char*>(self, 0), lower, At<uint32_t>(self, 4));
}

// ---- the effect system (0xB6F1D0): +4 the effect manager, +8 0xB6F1D8, +0xC the definition
// store, +0x10 the pools, +0x14, +0x18 the library, +0x1C the particle renderer, +0x20, +0x24
// the light manager

// 0x669EE0 (thiscall): its deleting destructor
uint8_t* __fastcall EffectSystem_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectSystem;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x669F00 (thiscall): its parts (static objects) set up
void __fastcall EffectSystem_Init(uint8_t* self, re::Edx) {
    PushMemoryTag(0x22);
    At<uint32_t>(self, 4) = 0x01B782C8;
    At<uint32_t>(self, 8) = 0x01B782EC;
    At<uint32_t>(self, 0xC) = 0x01B78318;
    At<uint32_t>(self, 0x10) = 0x01B78408;
    At<uint32_t>(self, 0x14) = 0x01B786A8;
    At<uint32_t>(self, 0x18) = 0x01B82AC0;
    At<uint32_t>(self, 0x1C) = 0x01B83108;
    At<uint32_t>(self, 0x20) = 0x01B83278;
    At<uint32_t>(self, 0x24) = 0x01B782FC;
    PopMemoryTag();
    NameKey_Destruct(At<uint8_t*>(self, 0x20));
    EffectManager_ResetTimersFn(At<uint8_t*>(self, 4));
    StringRef_Assign(nameRef_CF356C, 0);
}

// 0x669F70 (thiscall): shut down (its parts emptied and forgotten)
void __fastcall EffectSystem_Shutdown(uint8_t* self, re::Edx) {
    EffectManager_Shutdown_668350(At<uint8_t*>(self, 4));
    EffectDefinitionManager_ClearFn(At<uint8_t*>(self, 0x18));
    ParticleRenderer_Shutdown_5781D0(At<uint8_t*>(self, 0x20));
    Definitions_Shutdown_66B800(At<uint8_t*>(self, 8));
    PushMemoryTag(0x22);
    PopMemoryTag();
    for (int at = 4; at <= 0x24; at += 4) {
        At<uint32_t>(self, at) = 0;
    }
}

// ---- timers: the time a float was reset at, in seconds ---------------------------------------

// the game's clock in seconds (its milliseconds as unsigned)
inline float ClockSeconds() {
    int32_t ms = int32_t(timeInMs);
    double t = double(ms);
    if (ms < 0) {
        t = double(float(t + 4294967296.0));
    }
    return float(t * double(0.001f));
}

// 0x669FD0 (thiscall): not started (-1)
float* __fastcall Timer_InitImpl(float* self, re::Edx) {
    *self = -1.0f;
    return self;
}
// 0x669FE0 (thiscall): its start moved `seconds` earlier
void __fastcall Timer_SubImpl(float* self, re::Edx, float seconds) {
    *self = *self - seconds;
}
// 0x669FF0 (thiscall): restarted now
void __fastcall Timer_ResetImpl(float* self, re::Edx) {
    *self = ClockSeconds();
}
// 0x66A010 (thiscall): the seconds since it started
float __fastcall Timer_GetImpl(float* self, re::Edx) {
    float now = ClockSeconds();
    return now - *self;
}

// 0x666F70 (thiscall): EffectDefinitionLoader's vf1: an effect definition read (its name's
// hash, flags, importance, distances, primitives). (The first flag is read into the
// archive argument's slot, the next three into uninitialised bytes: when the archive
// doesn't write them they are the archive pointer's low byte and garbage; 0 here.)
void __fastcall EffectDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* def = At<uint8_t*>(self, 4);
    ArchiveString<0x21> name;
    name.text = name.data;
    name.cap = 0x21;
    name.data[0] = 0;
    vc::Call<void>(archive, 0x24, &name, 0x21, effectNameKey_93D544);
    At<uint32_t>(def, 0x18) = HashName(name.text);
    uint8_t flag0 = uint8_t(reinterpret_cast<uintptr_t>(archive));
    vc::Call<void>(archive, 0x3C, &flag0, effectFlagNames_93D5EC[1]);
    def[0x1C] ^= uint8_t(def[0x1C] ^ flag0) & 1;
    uint8_t flag1 = 0;
    vc::Call<void>(archive, 0x3C, &flag1, effectFlagNames_93D5EC[0]);
    def[0x1C] ^= uint8_t((flag1 * 2) ^ def[0x1C]) & 2;
    uint8_t flag2 = 0;
    vc::Call<void>(archive, 0x3C, &flag2, effectFlagNames_93D5EC[2]);
    def[0x1C] ^= uint8_t((flag2 * 4) ^ def[0x1C]) & 4;
    uint8_t flag3 = 0;
    vc::Call<void>(archive, 0x3C, &flag3, effectFlagNames_93D5EC[3]);
    def[0x1C] ^= uint8_t((flag3 * 8) ^ def[0x1C]) & 8;
    int32_t importance;
    vc::Call<void>(archive, 0x38, &importance, effectFlagNames_93D5EC[4]);
    def[0x1C] ^= uint8_t(uint8_t(importance << 4) ^ def[0x1C]) & 0x70;
    vc::Call<void>(archive, 0x28, def + 4, effectDistanceNames_93D600[0]);
    vc::Call<void>(archive, 0x28, def + 8, effectDistanceNames_93D600[1]);
    vc::Call<void>(archive, 0x28, def + 0xC, effectDistanceNames_93D600[2]);
    vc::Call<void>(archive, 0x28, def + 0x10, effectDistanceNames_93D600[3]);
    EffectDefinition_LoadPrimitivesFn(archive, def + 0x14);
}

//@@NEXT@@
} // namespace

#define EFFSYSCORE_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

EFFSYSCORE_REVERSED("Definition::GetKind_665FE0", 0x00665FE0, &Definition_GetKind);
EFFSYSCORE_REVERSED("DefinitionBaseLoader::Construct_665FF0", 0x00665FF0, &DefinitionBaseLoader_Ctor);
EFFSYSCORE_REVERSED("ParticleSystemDefinitionLoader::Construct_666010", 0x00666010,
                    &TypedLoader_Ctor<kVtParticleSystemDefinitionLoader>);
EFFSYSCORE_REVERSED("AttractorMovementInfoDefinitionLoader::Construct_666030", 0x00666030,
                    &TypedLoader_Ctor<kVtAttractorMovementInfoDefinitionLoader>);
EFFSYSCORE_REVERSED("FrictionMovementInfoDefinitionLoader::Construct_666050", 0x00666050,
                    &TypedLoader_Ctor<kVtFrictionMovementInfoDefinitionLoader>);
EFFSYSCORE_REVERSED("JitterMovementInfoDefinitionLoader::Construct_666070", 0x00666070,
                    &TypedLoader_Ctor<kVtJitterMovementInfoDefinitionLoader>);
EFFSYSCORE_REVERSED("GravityMovementInfoDefinitionLoader::Construct_666090", 0x00666090,
                    &TypedLoader_Ctor<kVtGravityMovementInfoDefinitionLoader>);
EFFSYSCORE_REVERSED("LightPrimitiveDefinitionLoader::Construct_6660B0", 0x006660B0,
                    &TypedLoader_Ctor<kVtLightPrimitiveDefinitionLoader>);
EFFSYSCORE_REVERSED("SpawnEffectPrimitiveDefinitionLoader::Construct_6660D0", 0x006660D0,
                    &TypedLoader_Ctor<kVtSpawnEffectPrimitiveDefinitionLoader>);
EFFSYSCORE_REVERSED("PlaySoundPrimitiveDefinitionLoader::Construct_6660F0", 0x006660F0,
                    &TypedLoader_Ctor<kVtPlaySoundPrimitiveDefinitionLoader>);
EFFSYSCORE_REVERSED("HeatHazePrimitiveDefinitionLoader::Construct_666110", 0x00666110,
                    &TypedLoader_Ctor<kVtHeatHazePrimitiveDefinitionLoader>);
EFFSYSCORE_REVERSED("MotionBlurPrimitiveDefinitionLoader::Construct_666130", 0x00666130,
                    &TypedLoader_Ctor<kVtMotionBlurPrimitiveDefinitionLoader>);
EFFSYSCORE_REVERSED("DefinitionStore::Get_666150", 0x00666150, &DefinitionStore_Get);
EFFSYSCORE_REVERSED("DefinitionStore::IndexOf_666170", 0x00666170, &DefinitionStore_IndexOf);
EFFSYSCORE_REVERSED("Definition::Read_6661A0", 0x006661A0, &Definition_Read);
EFFSYSCORE_REVERSED("DefinitionCountLoader::Write_666390", 0x00666390, &DefinitionCountLoader_Write);
EFFSYSCORE_REVERSED("DefinitionCountLoader::Read_666400", 0x00666400, &DefinitionCountLoader_Read);
EFFSYSCORE_REVERSED("DefinitionStore::Construct_666470", 0x00666470, &DefinitionStore_Ctor);
EFFSYSCORE_REVERSED("DefinitionStore::Allocate_666510", 0x00666510, &DefinitionStore_AllocateImpl);
EFFSYSCORE_REVERSED("DefinitionStore::New_666B10", 0x00666B10, &DefinitionStore_New);
EFFSYSCORE_REVERSED("DefinitionStore::Release_666B60", 0x00666B60, &DefinitionStore_ReleaseImpl);
EFFSYSCORE_REVERSED("DefinitionStore::LoadCounts_666B80", 0x00666B80, &DefinitionStore_LoadCounts);
EFFSYSCORE_REVERSED("EffectDefinition::Construct_666BE0", 0x00666BE0, &EffectDefinition_Construct);
EFFSYSCORE_REVERSED("EffectDefinition::Release_666C20", 0x00666C20, &EffectDefinition_Release);
EFFSYSCORE_REVERSED("EffectDefinition::NumPrimitives_666C30", 0x00666C30, &EffectDefinition_NumPrimitives);
EFFSYSCORE_REVERSED("EffectDefinition::MaxRadius_666C40", 0x00666C40, &EffectDefinition_MaxRadiusImpl);
EFFSYSCORE_REVERSED("Effect::ReleasePrimitives_666C90", 0x00666C90, &Effect_ReleasePrimitives);
EFFSYSCORE_REVERSED("Effect::KillPrimitives_666CA0", 0x00666CA0, &Effect_KillPrimitivesImpl);
EFFSYSCORE_REVERSED("Effect::SetScale_666CD0", 0x00666CD0, &Effect_SetScale);
EFFSYSCORE_REVERSED("Effect::SetParticles_666D10", 0x00666D10, &Effect_ForParticleSystems<float, 0x00670370>);
EFFSYSCORE_REVERSED("Effect::SetParticles_666D50", 0x00666D50, &Effect_ForParticleSystems<float, 0x006702F0>);
EFFSYSCORE_REVERSED("Effect::SetParticles_666D90", 0x00666D90, &Effect_ForParticleSystems<float, 0x00670330>);
EFFSYSCORE_REVERSED("Effect::SetParticles_666DD0", 0x00666DD0, &Effect_ForParticleSystems<int, 0x006703B0>);
EFFSYSCORE_REVERSED("Effect::KillAfter_666E10", 0x00666E10, &Effect_KillAfter);
EFFSYSCORE_REVERSED("Effect::RemoveDead_666E60", 0x00666E60, &Effect_RemoveDeadImpl);
EFFSYSCORE_REVERSED("Effect::FindPrimitive_666EE0", 0x00666EE0, &Effect_FindPrimitive);
EFFSYSCORE_REVERSED("Effect::SetVisible_666F20", 0x00666F20, &Effect_SetVisible);
EFFSYSCORE_REVERSED("Effect::SetBounded_666F50", 0x00666F50, &Effect_SetBounded);
EFFSYSCORE_REVERSED("Effect::ClearFade_666F60", 0x00666F60, &Effect_ClearFade);
EFFSYSCORE_REVERSED("Effect::Construct_6670D0", 0x006670D0, &Effect_Ctor);
EFFSYSCORE_REVERSED("Effect::Update_667100", 0x00667100, &Effect_Update);
EFFSYSCORE_REVERSED("Effect::Init_6672B0", 0x006672B0, &Effect_Init);
EFFSYSCORE_REVERSED("DefinitionType::Name_667330", 0x00667330, &DefinitionType_Name);
EFFSYSCORE_REVERSED("DefinitionType::FromName_667400", 0x00667400, &DefinitionType_FromName);
EFFSYSCORE_REVERSED("EffectDefinition::LoadPrimitives_667680", 0x00667680, &EffectDefinition_LoadPrimitives);
EFFSYSCORE_REVERSED("Effects::TableA_667740", 0x00667740, &Table_B41448_A);
EFFSYSCORE_REVERSED("Effects::TableB_667750", 0x00667750, &Table_B41448_B);
EFFSYSCORE_REVERSED("Effects::GetCamera_667760", 0x00667760, &Effects_GetCamera);
EFFSYSCORE_REVERSED("EffectHandle::Construct_667770", 0x00667770, &EffectHandle_Ctor);
EFFSYSCORE_REVERSED("EffectHandle::Start_6677A0", 0x006677A0, &EffectHandle_Start);
EFFSYSCORE_REVERSED("EffectHandle::Stop_6677E0", 0x006677E0, &EffectHandle_Stop);
EFFSYSCORE_REVERSED("EffectHandle::Suspend_667840", 0x00667840, &EffectHandle_Suspend);
EFFSYSCORE_REVERSED("EffectHandle::Resume_6678B0", 0x006678B0, &EffectHandle_Resume);
EFFSYSCORE_REVERSED("EffectHandle::Kill_667950", 0x00667950, &EffectHandle_Kill);
EFFSYSCORE_REVERSED("EffectDefinitionManager::Construct_667970", 0x00667970, &EffectDefinitionManager_Ctor);
EFFSYSCORE_REVERSED("EffectDefinitionManager::Destruct_667990", 0x00667990, &EffectDefinitionManager_Dtor);
EFFSYSCORE_REVERSED("EffectDefinitionManager::FindHash_6679A0", 0x006679A0, &EffectDefinitionManager_FindHash);
EFFSYSCORE_REVERSED("EffectDefinitionManager::Get_6679F0", 0x006679F0, &EffectDefinitionManager_Get);
EFFSYSCORE_REVERSED("EffectDefinitionManager::Clear_667A00", 0x00667A00, &EffectDefinitionManager_Clear);
EFFSYSCORE_REVERSED("EffectDefinitionManager::DeletingDestruct_667A50", 0x00667A50, &EffectDefinitionManager_DeletingDtor);
EFFSYSCORE_REVERSED("EffectDefinitionManager::FindString_667A70", 0x00667A70, &EffectDefinitionManager_FindString);
EFFSYSCORE_REVERSED("EffectDefinitionManager::Load_667AA0", 0x00667AA0, &EffectDefinitionManager_Load);
EFFSYSCORE_REVERSED("EffectDefinitionManager::Find_667B60", 0x00667B60, &EffectDefinitionManager_Find);
EFFSYSCORE_REVERSED("EffectLightManager::Construct_667BD0", 0x00667BD0, &EffectLightManager_Ctor);
EFFSYSCORE_REVERSED("EffectLightManager::Destruct_667BE0", 0x00667BE0, &EffectLightManager_Dtor);
EFFSYSCORE_REVERSED("EffectLightManager::Update_667BF0", 0x00667BF0, &EffectLightManager_Update);
EFFSYSCORE_REVERSED("EffectLightManager::Remove_667C20", 0x00667C20, &EffectLightManager_Remove);
EFFSYSCORE_REVERSED("EffectLightManager::DeletingDestruct_667C60", 0x00667C60, &EffectLightManager_DeletingDtor);
EFFSYSCORE_REVERSED("EffectLightManager::Add_667C80", 0x00667C80, &EffectLightManager_Add);
EFFSYSCORE_REVERSED("EffectList::Init_667DA0", 0x00667DA0, &List_InitImpl);
EFFSYSCORE_REVERSED("EffectList::PopFront_667DB0", 0x00667DB0, &List_PopFrontImpl);
EFFSYSCORE_REVERSED("EffectList::Count_667DD0", 0x00667DD0, &List_CountImpl);
EFFSYSCORE_REVERSED("EffectList::PushBack_667DF0", 0x00667DF0, &List_PushBackImpl);
EFFSYSCORE_REVERSED("EffectManager::Construct_667E20", 0x00667E20, &EffectManager_Ctor);
EFFSYSCORE_REVERSED("EffectManager::Destruct_667E70", 0x00667E70, &EffectManager_Dtor);
EFFSYSCORE_REVERSED("EffectManager::Update_667EA0", 0x00667EA0, &EffectManager_Update);
EFFSYSCORE_REVERSED("EffectManager::RenderParticles_668050", 0x00668050, &EffectManager_RenderParticles);
EFFSYSCORE_REVERSED("EffectManager::ReplaceProxy_6680C0", 0x006680C0, &EffectManager_ReplaceProxy);
EFFSYSCORE_REVERSED("EffectManager::ForgetProxy_668100", 0x00668100, &EffectManager_ForgetProxy);
EFFSYSCORE_REVERSED("EffectManager::ResetTimers_668130", 0x00668130, &EffectManager_ResetTimers);
EFFSYSCORE_REVERSED("EffectManager::Clear_668150", 0x00668150, &EffectManager_Clear);
EFFSYSCORE_REVERSED("EffectManager::Remove_6681B0", 0x006681B0, &EffectManager_Remove);
EFFSYSCORE_REVERSED("EffectManager::DeletingDestruct_668250", 0x00668250, &EffectManager_DeletingDtor);
EFFSYSCORE_REVERSED("EffectManager::Start_668290", 0x00668290, &EffectManager_Start);
EFFSYSCORE_REVERSED("EffectManager::UpdateLod_668360", 0x00668360, &EffectManager_UpdateLodImpl);
EFFSYSCORE_REVERSED("EffectManager::UpdateSuspension_668610", 0x00668610, &EffectManager_UpdateSuspensionImpl);
EFFSYSCORE_REVERSED("EffectManager::Render_668780", 0x00668780, &EffectManager_Render);
EFFSYSCORE_REVERSED("SuspendedEffect::Construct_6688E0", 0x006688E0, &SuspendedEffect_Ctor);
EFFSYSCORE_REVERSED("SuspendedEffect::Save_668900", 0x00668900, &SuspendedEffect_SaveImpl);
EFFSYSCORE_REVERSED("SuspendedEffect::Restore_668930", 0x00668930, &SuspendedEffect_RestoreImpl);
EFFSYSCORE_REVERSED("PrimDefinition::Construct_668970", 0x00668970, &PrimDefinition_Ctor);
EFFSYSCORE_REVERSED("EffectPrimitiveDefinitionLoader::Read_668990", 0x00668990, &EffectPrimitiveDefinitionLoader_Read);
EFFSYSCORE_REVERSED("PrimInstance::Construct_6689E0", 0x006689E0, &PrimInstance_Ctor);
EFFSYSCORE_REVERSED("PrimInstance::Destruct_668A00", 0x00668A00, &PrimInstance_Dtor);
EFFSYSCORE_REVERSED("PrimInstance::Restart_668A20", 0x00668A20, &PrimInstance_Restart);
EFFSYSCORE_REVERSED("PrimInstance::Kill_668A40", 0x00668A40, &PrimInstance_KillImpl);
EFFSYSCORE_REVERSED("PrimInstance::Update_668A50", 0x00668A50, &PrimInstance_UpdateImpl);
EFFSYSCORE_REVERSED("PrimInstance::SetHidden_668AD0", 0x00668AD0, &PrimInstance_SetVisibleImpl);
EFFSYSCORE_REVERSED("PrimInstance::Index_668AF0", 0x00668AF0, &PrimInstance_Index);
EFFSYSCORE_REVERSED("EffectProxy::Construct_668B30", 0x00668B30, &EffectProxy_Ctor);
EFFSYSCORE_REVERSED("EffectProxy::Destruct_668B40", 0x00668B40, &EffectProxy_Dtor);
EFFSYSCORE_REVERSED("EffectProxy::Attach_668B60", 0x00668B60, &EffectProxy_AttachImpl);
EFFSYSCORE_REVERSED("EffectProxy::Detach_668B80", 0x00668B80, &EffectProxy_DetachImpl);
EFFSYSCORE_REVERSED("EffectProxy::Kill_668BA0", 0x00668BA0, &EffectProxy_Kill);
EFFSYSCORE_REVERSED("EffectProxy::SetScale_668BD0", 0x00668BD0, &EffectProxy_SetScale);
EFFSYSCORE_REVERSED("EffectProxy::SetParticles_668C10", 0x00668C10, &EffectProxy_ForEffect<float, 0x00666D10>);
EFFSYSCORE_REVERSED("EffectProxy::SetParticles_668C40", 0x00668C40, &EffectProxy_ForEffect<float, 0x00666D50>);
EFFSYSCORE_REVERSED("EffectProxy::SetParticles_668C70", 0x00668C70, &EffectProxy_ForEffect<float, 0x00666D90>);
EFFSYSCORE_REVERSED("EffectProxy::SetParticles_668CA0", 0x00668CA0, &EffectProxy_ForEffect<int, 0x00666DD0>);
EFFSYSCORE_REVERSED("EffectProxy::SetHidden_668CD0", 0x00668CD0, &EffectProxy_ForEffect<int, 0x00666F20>);
EFFSYSCORE_REVERSED("EffectProxy::KillAfter_668D00", 0x00668D00, &EffectProxy_KillAfter);
EFFSYSCORE_REVERSED("EffectProxy::DeletingDestruct_668D60", 0x00668D60, &EffectProxy_DeletingDtor);
EFFSYSCORE_REVERSED("EffectProxyAttach::Construct_668DA0", 0x00668DA0, &EffectProxyAttach_Ctor);
EFFSYSCORE_REVERSED("EffectProxyAttach::Destruct_668DC0", 0x00668DC0, &EffectProxyAttach_Dtor);
EFFSYSCORE_REVERSED("EffectProxyAttach::Attached_668DD0", 0x00668DD0, &EffectProxyAttach_Attached);
EFFSYSCORE_REVERSED("EffectProxyAttach::Set_668DE0", 0x00668DE0, &EffectProxyAttach_Set);
EFFSYSCORE_REVERSED("EffectProxy::GetTransform_668E00", 0x00668E00, &EffectProxy_GetTransform);
EFFSYSCORE_REVERSED("EffectProxyAttach::DeletingDestruct_668E10", 0x00668E10, &EffectProxyAttach_DeletingDtor);
EFFSYSCORE_REVERSED("EffectProxyAttach::Detached_668E40", 0x00668E40, &EffectProxyAttach_Detached);
EFFSYSCORE_REVERSED("EffectProxySetLocation::Construct_668E90", 0x00668E90, &EffectProxySetLocation_Ctor);
EFFSYSCORE_REVERSED("EffectProxySetLocation::Destruct_668EB0", 0x00668EB0, &EffectProxySetLocation_Dtor);
EFFSYSCORE_REVERSED("EffectProxy::SetTransform_668EC0", 0x00668EC0, &EffectProxy_SetTransform);
EFFSYSCORE_REVERSED("Effects::ScreenToWorld_668F80", 0x00668F80, &ScreenToWorldImpl);
EFFSYSCORE_REVERSED("EffectProxySetLocation::DeletingDestruct_669010", 0x00669010, &EffectProxySetLocation_DeletingDtor);
EFFSYSCORE_REVERSED("EffectProxy::SetPosition_669040", 0x00669040, &EffectProxy_SetPosition);
EFFSYSCORE_REVERSED("Effects::OrientTo_669170", 0x00669170, &Matrix_OrientToImpl);
EFFSYSCORE_REVERSED("EffectProxy::SetDirection_669360", 0x00669360, &EffectProxy_SetDirection);
EFFSYSCORE_REVERSED("EffectList::Construct_6693E0", 0x006693E0, &ListHead_Ctor);
EFFSYSCORE_REVERSED("EffectPools::KindOfObject_669400", 0x00669400, &PrimInstance_GetKindImpl);
EFFSYSCORE_REVERSED("EffectPools::Alloc_669410", 0x00669410, &EffectPools_Alloc);
EFFSYSCORE_REVERSED("EffectPools::Get_669430", 0x00669430, &EffectPools_Get);
EFFSYSCORE_REVERSED("EffectPools::Init_669450", 0x00669450, &EffectPools_Init);
EFFSYSCORE_REVERSED("EffectPools::KindOf_6696E0", 0x006696E0, &EffectPools_KindOfImpl);
EFFSYSCORE_REVERSED("EffectPools::IndexOf_669720", 0x00669720, &EffectPools_IndexOf);
EFFSYSCORE_REVERSED("EffectPoolStorage::Construct_669740", 0x00669740, &EffectPoolStorage_Ctor);
EFFSYSCORE_REVERSED("EffectPoolStorage::Destruct_669A30", 0x00669A30, &EffectPoolStorage_Dtor);
EFFSYSCORE_REVERSED("EffectPools::Construct_669D30", 0x00669D30, &EffectPools_Ctor);
EFFSYSCORE_REVERSED("EffectPools::Destruct_669D90", 0x00669D90, &EffectPools_Dtor);
EFFSYSCORE_REVERSED("EffectPools::Free_669DC0", 0x00669DC0, &EffectPools_FreeImpl);
EFFSYSCORE_REVERSED("ArchiveString::Construct21_669E20", 0x00669E20, &ArchiveString_Ctor<0x21>);
EFFSYSCORE_REVERSED("ArchiveString::Construct33_669E40", 0x00669E40, &ArchiveString_Ctor<0x33>);
EFFSYSCORE_REVERSED("ArchiveString::ToLower_669E60", 0x00669E60, &ArchiveString_ToLower);
EFFSYSCORE_REVERSED("EffectSystem::DeletingDestruct_669EE0", 0x00669EE0, &EffectSystem_DeletingDtor);
EFFSYSCORE_REVERSED("EffectSystem::Init_669F00", 0x00669F00, &EffectSystem_Init);
EFFSYSCORE_REVERSED("EffectSystem::Shutdown_669F70", 0x00669F70, &EffectSystem_Shutdown);
EFFSYSCORE_REVERSED("EffectTimer::Init_669FD0", 0x00669FD0, &Timer_InitImpl);
EFFSYSCORE_REVERSED("EffectTimer::Sub_669FE0", 0x00669FE0, &Timer_SubImpl);
EFFSYSCORE_REVERSED("EffectTimer::Reset_669FF0", 0x00669FF0, &Timer_ResetImpl);
EFFSYSCORE_REVERSED("EffectTimer::Get_66A010", 0x0066A010, &Timer_GetImpl);
EFFSYSCORE_REVERSED("EffectDefinitionLoader::Read_666F70", 0x00666F70, &EffectDefinitionLoader_Read);
//@@REG@@
