// The effect system's primitives (0x66A040..0x66E3B0): creating a list's instances from
// their definitions, the value-over-time keys, the heat haze, light, motion blur, sound
// and spawned-effect primitives (definitions, their loaders, instances), the interpolator
// element store and the definition store's loaders, the effect light object. Shares the
// shapes of EffectSystemCore.cpp: an instance's +0 link, +4 timer, +8 definition, +0xC
// effect, +0x10 flags (1 dead, 2/4 hidden); a primitive definition's +0 type, +4 link, +8
// start time, +0xC duration.

#include <cstdint>

#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct Link {
    re::Ptr32<Link> next;
};

GAME_VAR(uint8_t*, effectCounter_B6F1D4, 0x00B6F1D4);   // the effect manager
GAME_VAR(uint8_t*, valueStore_B6F1D8, 0x00B6F1D8);      // the value-over-time keys' store
GAME_VAR(uint8_t*, definitionStore, 0x00B6F1DC);
GAME_VAR(uint8_t*, effectPools, 0x00B6F1E0);
GAME_ARRAY(uint8_t, definitionKinds, 0x00B6F214);
GAME_VAR(float, randomAzimuthRange_B6F228, 0x00B6F228);
GAME_VAR(float*, keyTimes, 0x01B838D0);
GAME_VAR(float*, keyValues, 0x01B838CC);
inline void* const random_CF356C = reinterpret_cast<void*>(0x00CF356C);
constexpr uint32_t kVtEffectPrimitiveDefinitionLoader = 0x0093D728;
GAME_VAR(const char*, name_93D56C, 0x0093D56C);
GAME_VAR(const char*, name_93D578, 0x0093D578);
GAME_VAR(const char*, name_93D644, 0x0093D644);
GAME_VAR(const char*, keyCountName_93D580, 0x0093D580);
GAME_VAR(const char*, name_93D650, 0x0093D650);
GAME_VAR(const char*, name_93D64C, 0x0093D64C);
GAME_VAR(const char*, name_93D57C, 0x0093D57C);
GAME_VAR(const char*, name_93D584, 0x0093D584);
GAME_VAR(const char*, name_93D588, 0x0093D588);
GAME_VAR(const char*, name_93D58C, 0x0093D58C);
GAME_VAR(const char*, name_93D590, 0x0093D590);
GAME_VAR(const char*, name_93D594, 0x0093D594);
constexpr uint32_t kVtInterpolatorElementStore = 0x0093D738;
constexpr uint32_t kVtStoreLoader = 0x0093D740;
GAME_VAR(const char*, countName_93D548, 0x0093D548);
GAME_VAR(const char*, storeName_93D654, 0x0093D654);
GAME_VAR(uint8_t*, effectLights_B6F1F4, 0x00B6F1F4);
GAME_VAR(uint8_t*, trailTextures_B6F1F0, 0x00B6F1F0);   // +0x5C: a texture per kind, 12 bytes apart
GAME_VAR(uint32_t, trailBufferGuards_1B850D8, 0x01B850D8);   // the static buffers' guards
GAME_VAR(int32_t, trailLastFrame_1B850DC, 0x01B850DC);
GAME_ARRAY(float, trailPoints_1B844D8, 0x01B844D8);     // point pairs (6 floats), smoothed
GAME_ARRAY(float, trailControl_1B838D8, 0x01B838D8);    // the pairs before smoothing
GAME_VAR(int32_t, im3dVertexCount, 0x00CA2AE8);
GAME_VAR(int32_t, im3dIndexCount, 0x00CA2AEC);
GAME_ARRAY(uint8_t, im3dVertices, 0x00BD7800);          // 0x24 bytes each
GAME_ARRAY(uint16_t, im3dIndices, 0x00BF0B2C);
GAME_VAR(uint8_t*, particleRenderer_B6F1EC, 0x00B6F1EC);  // +0: the camera's position
inline void* const trails_1B850E0 = reinterpret_cast<void*>(0x01B850E0);
GAME_VAR(const char*, textureName_93D570, 0x0093D570);
GAME_VAR(const char*, countName_93D640, 0x0093D640);
constexpr uint32_t kVtEffectSpawnProxy = 0x0093D794;
GAME_VAR(const char*, soundName_93D658, 0x0093D658);
GAME_VAR(const char*, soundBank_93D65C, 0x0093D65C);
GAME_VAR(const char*, effectName_93D544, 0x0093D544);
GAME_VAR(uint8_t*, effectLibrary_B6F1E8, 0x00B6F1E8);
//@@TOP@@

GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, void* key);
GAME_FN(uint8_t, __thiscall, Archive_ReadSection, 0x005B0720, void* archive, void* reader, const char* name);
GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
GAME_FN(uint8_t, __thiscall, Definition_GetKind, 0x00665FE0, const uint8_t* definition);
GAME_FN(void*, __thiscall, DefinitionBaseLoader_Construct, 0x00665FF0, void* self, void* definition);
GAME_FN(void, __thiscall, DefinitionStore_Release, 0x00666B60, uint8_t* self, void* definition);
GAME_FN(Link*, __thiscall, List_PopFront, 0x00667DB0, void* list);
GAME_FN(void, __thiscall, EffectCount_Add, 0x00668230, uint8_t* manager);
GAME_FN(void, __thiscall, PrimDefinition_Construct, 0x00668970, uint8_t* self);
GAME_FN(void, __thiscall, EffectPrimitiveDefinitionLoader_Read, 0x00668990, void* self, void* archive);
GAME_FN(uint8_t*, __thiscall, PrimInstance_Construct, 0x006689E0, uint8_t* self);
GAME_FN(void, __thiscall, PrimInstance_Destruct, 0x00668A00, uint8_t* self);
GAME_FN(void, __thiscall, PrimInstance_Restart, 0x00668A20, uint8_t* self);
GAME_FN(void, __thiscall, PrimInstance_Kill, 0x00668A40, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PrimInstance_GetKind, 0x00669400, void* prim);
GAME_FN(uint8_t*, __thiscall, Effects_Alloc, 0x00669410, uint8_t* pools, int kind);
GAME_FN(void, __thiscall, Effects_Free, 0x00669DC0, uint8_t* pools, void* object);
GAME_FN(float, __thiscall, PrimTimer_Get, 0x0066A010, void* timer);
GAME_FN(void, __cdecl, PrimInstance_Stop, 0x0066A1C0, void* prim);
GAME_FN(uint32_t, __thiscall, ValueStore_TimeCount, 0x0066B5D0, uint8_t* store);
GAME_FN(void, __thiscall, ValueStore_AddTime, 0x0066B5E0, uint8_t* store);
GAME_FN(uint32_t, __thiscall, ValueStore_ValueCount, 0x00856850, uint8_t* store);
GAME_FN(void, __thiscall, ValueOverTime_Construct, 0x0066E460, void* self);
GAME_FN(void, __thiscall, ValueOverTime_SetComponents, 0x0066E470, void* self, int n);
GAME_FN(void*, __thiscall, ValueOverTimeDefinitionLoader_Construct, 0x0066E440, void* self, void* definition);
GAME_FN(uint8_t, __thiscall, ValueOverTimeInstance_Init, 0x0066E560, void* self, void* definition);
GAME_FN(void, __thiscall, ValueOverTimeInstance_Stop, 0x0066E620, void* self);
GAME_FN(void, __thiscall, ValueOverTimeInstance_Get, 0x0066E6B0, void* self, float* out, float time);
GAME_FN(void, __thiscall, ValueOverTimeInstance_Construct, 0x0066BEA0, void* self);
GAME_FN(void, __cdecl, HeatHaze_Remove, 0x00560BB0, int32_t id);
GAME_FN(void, __cdecl, HeatHaze_SetStrength, 0x00560C20, int32_t id, float strength);
GAME_FN(int32_t, __cdecl, HeatHaze_Add, 0x00560C40, const float* position, const float* direction, float size, float strength);
GAME_FN(void, __thiscall, ValueStore_Read_8A20A0, 0x008A20A0, uint8_t* store, float value);   // (empty)
GAME_FN(uint8_t, __thiscall, ValueStore_IsPreset, 0x0066B5C0, uint8_t* store);
GAME_FN(void, __thiscall, ValueOverTime_AddKeyFn, 0x0066A8E0, uint8_t* self, const float* values, float time);
GAME_FN(float, __thiscall, ValueOverTimeInstance_LoopTimeFn, 0x0066AAA0, uint8_t* self, float t);
GAME_FN(void, __thiscall, ValueOverTimeInstance_ZeroFn, 0x0066ABA0, uint8_t* self, float* out);
GAME_FN(void, __thiscall, ValueOverTimeInstance_KeyValuesFn, 0x0066ABF0, uint8_t* self, float* out, int index);
GAME_FN(void, __thiscall, ValueOverTimeInstance_InterpolateFn, 0x0066ACC0, uint8_t* self, float* out, float t);
GAME_FN(void, __thiscall, ValueOverTimeInstance_ComputeSlopeFn, 0x0066AF80, uint8_t* self);
GAME_FN(void, __thiscall, ValueOverTimeInstance_UpdateConstantFn, 0x0066AFE0, uint8_t* self);
GAME_FN(void, __thiscall, ValueOverTimeInstance_EvaluateFn, 0x0066B250, uint8_t* self, float* out, float t);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __cdecl, Archive_Open, 0x005B07E0, const char* path, const int32_t* mode, int);
GAME_FN(void, __cdecl, Archive_Close, 0x005B0860, void* archive);
GAME_FN(void, __thiscall, ValueStore_Allocate, 0x0066B570, uint8_t* self);
GAME_FN(void, __thiscall, EffectLightManager_Add, 0x00667C80, uint8_t* self, uint8_t* light);
GAME_FN(void, __thiscall, EffectLightManager_Remove, 0x00667C20, uint8_t* self, uint8_t* light);
// CPointLights::AddLight(type, position, direction, radius, r, g, b, fog, shadows)
GAME_FN(void, __cdecl, PointLights_AddLight, 0x00522DB0, int type, float x, float y, float z, float dx, float dy,
        float dz, float radius, float r, float g, float b, int fog, int shadows);
GAME_FN(uint8_t*, __thiscall, Trails_Find, 0x0066BD10, uint8_t* self, int32_t id);
GAME_FN(void, __thiscall, Trails_Fade, 0x0066BDF0, uint8_t* self, uint8_t* trail);
GAME_FN(float*, __cdecl, VectorSub, 0x00439F60, float* out, const float* a, const float* b);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, int count, void* ctor);
GAME_FN(float*, __cdecl, CatmullRom, 0x00412400, float* out, const float* p0, const float* p1, const float* p2,
        const float* p3, float t);
GAME_FN(void, __thiscall, Trails_BuildVerticesFn, 0x0066C150, uint8_t* self, uint8_t* trail);
GAME_FN(void, __thiscall, Trails_AddPointsFn, 0x0066BF90, void* self, int32_t id, float ax, float ay, float az,
        float aw, float bx, float by, float bz, float bw);
GAME_FN(uint32_t, __cdecl, GetRendererFrame, 0x00402600);
GAME_FN(void, __thiscall, ArchiveString_Init, 0x00669E20, void* string);
inline char* __cdecl Strncpy(char* dst, const char* src, uint32_t n) { return crt::Strncpy(dst, src, (size_t)n); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t, __thiscall, EffectTextures_Has, 0x005781F0, void* self, uint32_t hash);
GAME_FN(uint8_t, __thiscall, EffectTextures_Add, 0x005782C0, void* self, const char* path, const char* name);
GAME_FN(int, __thiscall, EffectTextures_FindIndex, 0x00578350, void* self, uint32_t hash);
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void*, __cdecl, Im3D_Transform, 0x005EFAF0, void* vertices, int count, void* transform, uint32_t flags);
GAME_FN(uint8_t, __cdecl, Im3D_RenderIndexedPrimitive, 0x005F0170, int type, uint16_t* indices, int count, int);
GAME_FN(uint8_t, __cdecl, Im3D_End, 0x005EFB20);
GAME_FN(uint8_t, __cdecl, Vector_NearlyEqual, 0x00578540, const float* a, const float* b, float tolerance);
GAME_FN(float, __thiscall, Vec_Normalise, 0x004294A0, float* v);
GAME_FN(float*, __thiscall, NiTransform_Transform, 0x00450B10, const void* transform, float* out, const float* v);
GAME_FN(void, __thiscall, TextureRef_Construct, 0x005781C0, void* ref);
GAME_FN(void, __thiscall, TextureRef_Set, 0x005783F0, void* ref, uint32_t hash);
GAME_FN(const char*, __thiscall, String_Get_773DB0, 0x00773DB0, void* string);
GAME_FN(void, __thiscall, ArchiveString_ToLower, 0x00669E60, void* string);
GAME_FN(void, __thiscall, Timer_Init, 0x00669FD0, void* timer);
GAME_FN(void, __thiscall, Timer_Reset, 0x00669FF0, void* timer);
GAME_FN(int32_t, __thiscall, Trails_StartFn, 0x0066BB00, void* self, int32_t capacity, float fade, uint32_t colour,
        int32_t priority);
GAME_FN(void, __thiscall, Trails_ReleaseFn, 0x0066BF40, void* self, int32_t id);
GAME_FN(void, __thiscall, Trails_FreeFn, 0x0066BF70, void* self, int32_t id);
GAME_FN(void, __thiscall, Trails_SetTextureFn, 0x0066BEF0, void* self, int32_t id, int kind);
GAME_FN(void, __thiscall, Trails_Set314Fn, 0x0066BF20, void* self, int32_t id, uint32_t value);
GAME_FN(void, __thiscall, Trails_Set20Fn, 0x0066BED0, void* self, int32_t id, uint32_t value);
GAME_FN(void, __thiscall, Trails_Set310Fn, 0x0066BEB0, void* self, int32_t id, float value);
GAME_FN(void, __thiscall, Trails_Set32CFn, 0x0066BDC0, void* self, int32_t id, float value);
GAME_FN(void, __thiscall, Trails_AddPositionFn, 0x0066CD10, void* self, int32_t id, float x, float y, float z,
        float unused, float half);
GAME_FN(void, __thiscall, MotionBlur_UpdateFadeFn, 0x0066D650, uint8_t* self, uint8_t* transform);
GAME_FN(void, __thiscall, MotionBlur_AddPointFn, 0x0066D750, uint8_t* self, float x, float y, float z, float unused,
        float half);
GAME_FN(uint8_t*, __thiscall, ValueOverTimeInstance_Definition, 0x0066E890, void* self);
GAME_FN(uint8_t, __thiscall, ValueOverTime_NumKeysFn, 0x0066A8D0, uint8_t* self);
GAME_FN(void, __thiscall, ValueOverTime_GetKeyFn, 0x0066AA00, uint8_t* self, float* values, float* time, int index);
GAME_FN(void, __thiscall, Archive_WriteSection, 0x005B0730, void* archive, void* writer, const char* name);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(void, __thiscall, ArchiveString33_Init, 0x00669E40, void* string);
GAME_FN(void, __thiscall, EffectProxySetLocation_Construct, 0x00668E90, void* self);
GAME_FN(void, __thiscall, EffectProxySetLocation_Destruct, 0x00668EB0, void* self);
GAME_FN(void, __thiscall, EffectProxy_KillAfter, 0x00668D00, void* self, float time);
GAME_FN(uint8_t*, __thiscall, EffectProxy_GetHandle, 0x0066B5D0, void* self);
GAME_FN(void, __thiscall, Matrix3_MakeIdentity, 0x00770920, void* matrix);
GAME_FN(void, __cdecl, Matrix_OrientTo, 0x00669170, float* matrix, const float* direction);
GAME_FN(uint8_t*, __thiscall, NiTransform_Multiply, 0x00406830, const void* self, void* out, const void* other);
GAME_FN(uint8_t, __cdecl, String_NotEqual_4479F0, 0x004479F0, const void* string, const char* text);
GAME_FN(uint16_t, __thiscall, EffectDefinitionManager_FindString, 0x00667A70, uint8_t* self, const void* name);
GAME_FN(void, __thiscall, EffectManager_Start, 0x00668290, uint8_t* self, const int32_t* index, void* proxy, int flag);
GAME_FN(void, __thiscall, SpawnEffect_ScheduleFn, 0x0066E020, uint8_t* self);
GAME_FN(void, __thiscall, SpawnEffect_PlaceFn, 0x0066E300, uint8_t* self);
GAME_FN(void, __thiscall, SpawnProxy_SetTransformFn, 0x0066E200, void* self, const float* position,
        const float* direction, const void* parent);
//@@DECL@@

// ---- a list's instances ---------------------------------------------------------------------

// 0x66A040 / 0x66A100 (cdecl): a list of objects made from a list of definitions (each of
// its definition's kind, taken from the pools; its vtable just before the link), by their
// virtual at Create (the one at Destroy undoing it). One failing: every one made undone.
// (The objects' vtables sit 4 bytes before the pool's links.)
template <int Create, int Destroy>
uint8_t __cdecl VirtualList_Create(re::Ptr32<Link>* out, re::Ptr32<Link>* definitions) {
    uint8_t ok = 1;
    Link* def = *definitions;
    if (!def) {
        return ok;
    }
    do {
        uint8_t* object = Effects_Alloc(effectPools, Definition_GetKind(reinterpret_cast<uint8_t*>(def) - 4));
        if (!object) {
            ok = 0;
        } else {
            uint8_t* self = object - 4;
            if (vc::Call<uint8_t>(self, Create, def ? reinterpret_cast<uint8_t*>(def) - 4 : nullptr)) {
                At<Link*>(object, 0) = *out;
                *out = reinterpret_cast<Link*>(object);
            } else {
                vc::Call<void>(self, Destroy);
                Effects_Free(effectPools, object);
                ok = 0;
            }
        }
        def = def->next;
    } while (def);
    if (ok) {
        return ok;
    }
    for (Link* object = *out; object; object = *out) {
        vc::Call<void>(reinterpret_cast<uint8_t*>(object) - 4, Destroy);
        List_PopFront(out);
        Effects_Free(effectPools, object);
    }
    return ok;
}

// 0x66A1C0 (cdecl): a primitive instance stopped (by its kind)
void __cdecl PrimInstance_StopImpl(uint8_t* prim) {
    uintptr_t fn;
    if (PrimInstance_GetKind(prim) == 2) {
        fn = 0x006700E0;
    } else if (PrimInstance_GetKind(prim) == 0xF) {
        fn = 0x0066BA20;
    } else if (PrimInstance_GetKind(prim) == 0x12) {
        fn = 0x0066E0F0;
    } else if (PrimInstance_GetKind(prim) == 0x13) {
        fn = 0x0066A6F0;
    } else if (PrimInstance_GetKind(prim) == 0x14) {
        fn = 0x0066D3B0;
    } else if (PrimInstance_GetKind(prim) == 0x15) {
        fn = 0x0066DCF0;
    } else {
        return;
    }
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(fn)(prim);
}

// 0x66A230 (cdecl): a list of definitions given back to the store
void __cdecl PrimDefList_Release(re::Ptr32<Link>* list) {
    for (Link* link = *list; link; link = *list) {
        List_PopFront(list);
        DefinitionStore_Release(definitionStore, link ? reinterpret_cast<uint8_t*>(link) - 4 : nullptr);
    }
}

// 0x66A270 / 0x66A2B0 (cdecl): a list of objects destroyed (their virtual at Destroy) and
// given back to the pools
template <int Destroy>
void __cdecl VirtualList_Release(re::Ptr32<Link>* list) {
    for (Link* object = *list; object; object = *list) {
        vc::Call<void>(reinterpret_cast<uint8_t*>(object) - 4, Destroy);
        List_PopFront(list);
        Effects_Free(effectPools, object);
    }
}

// 0x66A2F0 (cdecl): a list of primitive instances stopped and given back
void __cdecl PrimInstanceList_Release(re::Ptr32<Link>* list) {
    for (Link* prim = *list; prim; prim = *list) {
        PrimInstance_Stop(prim);
        List_PopFront(list);
        Effects_Free(effectPools, prim);
    }
}

// 0x66A330 (cdecl): the same for particle emitters (0x6714A0)
void __cdecl EmitterList_Release(re::Ptr32<Link>* list) {
    for (Link* object = *list; object; object = *list) {
        reinterpret_cast<void(__thiscall*)(Link*)>(0x006714A0)(object);
        List_PopFront(list);
        Effects_Free(effectPools, object);
    }
}

// 0x66A370 (cdecl): a definition type's kind of instance
uint8_t __cdecl DefinitionKindImpl(int type) {
    return definitionKinds[type];
}

// 0x66A380 (cdecl): an effect's primitive instances made from its definitions (by kind:
// particle system, light, spawned effect, heat haze, motion blur, sound); one failing:
// every one made stopped and given back
uint8_t __cdecl PrimInstanceList_Create(re::Ptr32<Link>* out, re::Ptr32<Link>* definitions) {
    uint8_t ok = 1;
    Link* def = *definitions;
    if (!def) {
        return ok;
    }
    do {
        uint8_t kind = Definition_GetKind(reinterpret_cast<uint8_t*>(def) - 4);
        uint8_t* prim = Effects_Alloc(effectPools, kind);
        if (!prim) {
            ok = 0;
        } else {
            uintptr_t init = 0;
            switch (kind) {
            case 2: init = 0x0066FC00; break;
            case 0xF: init = 0x0066B9E0; break;
            case 0x12: init = 0x0066E090; break;
            case 0x13: init = 0x0066A6B0; break;
            case 0x14: init = 0x0066D2D0; break;
            case 0x15: init = 0x0066DC80; break;
            }
            uint8_t made = 0;
            if (init) {
                made = reinterpret_cast<uint8_t(__thiscall*)(uint8_t*, uint8_t*)>(init)(
                    prim, def ? reinterpret_cast<uint8_t*>(def) - 4 : nullptr);
            }
            if (made) {
                At<Link*>(prim, 0) = *out;
                *out = reinterpret_cast<Link*>(prim);
            } else {
                PrimInstance_Stop(prim);
                Effects_Free(effectPools, prim);
                ok = 0;
            }
        }
        def = def->next;
    } while (def);
    if (ok) {
        return ok;
    }
    for (Link* prim = *out; prim; prim = *out) {
        PrimInstance_Stop(prim);
        List_PopFront(out);
        Effects_Free(effectPools, prim);
    }
    return ok;
}

// 0x66A500 (cdecl): a random direction (elevation -pi..pi, azimuth over the range at
// 0xB6F228)
void __cdecl RandomDirection(float* out) {
    float elevation = Random_GetFloat(random_CF356C) * 6.2831855f - 3.1415927f;
    float range = randomAzimuthRange_B6F228;
    float r = Random_GetFloat(random_CF356C);
    float azimuth = 0.0f + (range - 0.0f) * r;
    float cosElevation = trk::CrtCos(elevation);
    float sinAzimuth = trk::CrtSin(azimuth);
    out[0] = -cosElevation * sinAzimuth;
    float cosAzimuth = trk::CrtCos(azimuth);
    out[1] = cosAzimuth * cosElevation;
    out[2] = trk::CrtSin(elevation);
}

// ---- heat haze primitives --------------------------------------------------------------------
// The definition: +0x10 its strength over time, +0x14 its size, +0x18 the detail it needs,
// +0x1C along the effect's up axis (else straight up). The instance: +0x14 its strength's
// state, +0x18 the haze (-1: none).

// 0x66A5B0 (thiscall)
uint8_t* __fastcall HeatHazeDefinition_Ctor(uint8_t* self, re::Edx) {
    PrimDefinition_Construct(self);
    ValueOverTime_Construct(self + 0x10);
    At<float>(self, 0x14) = 1.0f;
    At<float>(self, 0x18) = 0.0f;
    self[0x1C] = 0;
    ValueOverTime_SetComponents(self + 0x10, 1);
    return self;
}

// 0x66A5E0 (thiscall): HeatHazePrimitiveDefinitionLoader's vf1
void __fastcall HeatHazePrimitiveDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t base[8];
    DefinitionBaseLoader_Construct(base, definition);
    At<uint32_t>(base, 0) = kVtEffectPrimitiveDefinitionLoader;
    EffectPrimitiveDefinitionLoader_Read(base, archive);
    uint8_t strength[8];
    ValueOverTimeDefinitionLoader_Construct(strength, definition + 0x10);
    Archive_ReadSection(archive, strength, name_93D56C);
    vc::Call<void>(archive, 0x28, definition + 0x14, name_93D578);
    vc::Call<void>(archive, 0x28, definition + 0x18, name_93D578);
    vc::Call<void>(archive, 0x3C, definition + 0x1C, name_93D644);
}

// 0x66A670 (thiscall)
uint8_t* __fastcall HeatHaze_Ctor(uint8_t* self, re::Edx) {
    PrimInstance_Construct(self);
    ValueOverTimeInstance_Construct(self + 0x14);
    At<int32_t>(self, 0x18) = -1;
    return self;
}

// 0x66A690 (thiscall): its destructor
void __fastcall HeatHaze_Dtor(uint8_t* self, re::Edx) {
    NameKey_Destruct(self + 0x14);
    PrimInstance_Destruct(self);
}

// 0x66A6B0 (thiscall): started from its definition
uint8_t __fastcall HeatHaze_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint8_t*>(self, 8) = definition;
    At<int32_t>(self, 0x18) = -1;
    uint8_t made = ValueOverTimeInstance_Init(self + 0x14, definition + 0x10) != 0;
    PrimInstance_Restart(self);
    return made;
}

// 0x66A6F0 (thiscall): stopped (its haze removed)
void __fastcall HeatHaze_Stop(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x18) >= 0) {
        HeatHaze_Remove(At<int32_t>(self, 0x18));
        At<int32_t>(self, 0x18) = -1;
    }
    if (!(self[0x10] & 1)) {
        PrimInstance_Kill(self);
    }
    ValueOverTimeInstance_Stop(self + 0x14);
}

// 0x66A720 (thiscall): its haze made, strengthened or removed (while the effect's detail
// is at least the definition's)
void __fastcall HeatHaze_Update(uint8_t* self, re::Edx, uint8_t* transform) {
    uint8_t* definition = At<uint8_t*>(self, 8);
    if (!(At<float>(definition, 0x18) < At<float>(At<uint8_t*>(self, 0xC), 0x4C) ||
          At<float>(definition, 0x18) == At<float>(At<uint8_t*>(self, 0xC), 0x4C))) {
        if (At<int32_t>(self, 0x18) >= 0) {
            HeatHaze_Remove(At<int32_t>(self, 0x18));
            At<int32_t>(self, 0x18) = -1;
        }
        return;
    }
    float time = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    float strength;
    ValueOverTimeInstance_Get(self + 0x14, &strength, time);
    int32_t id = At<int32_t>(self, 0x18);
    if (id >= 0) {
        if (strength < 0.0f || strength == 0.0f || strength != strength) {
            HeatHaze_Remove(id);
            At<int32_t>(self, 0x18) = -1;
            return;
        }
        HeatHaze_SetStrength(id, strength);
        return;
    }
    float size = At<float>(At<uint8_t*>(self, 8), 0x14);
    if (strength < 0.0f || strength == 0.0f || strength != strength) {
        return;
    }
    float position[3] = {At<float>(transform, 0x24), At<float>(transform, 0x28), At<float>(transform, 0x2C)};
    float direction[3];
    if (At<uint8_t>(At<uint8_t*>(self, 8), 0x1C)) {
        direction[0] = At<float>(transform, 8);
        direction[1] = At<float>(transform, 0x14);
        direction[2] = At<float>(transform, 0x20);
    } else {
        direction[0] = 0.0f;
        direction[1] = 0.0f;
        direction[2] = 1.0f;
    }
    id = HeatHaze_Add(position, direction, size, strength);
    At<int32_t>(self, 0x18) = id;
    HeatHaze_SetStrength(id, strength);
}

// ---- value over time: +1 bits 3..5 its number of components, +4 its number of keys, +6
// its first time and +8 its first value in the store's arrays ---------------------------------

// 0x66A8A0 (thiscall): the constructor (the type byte 0xFF, no components, no keys)
uint8_t* __fastcall ValueOverTime_Construct66A8A0(uint8_t* self, re::Edx) {
    self[1] &= 0xC0;
    self[2] &= 0x80;
    self[3] &= 0x80;
    self[0] = 0xFF;
    self[4] = 0;
    self[5] = 0;
    At<uint16_t>(self, 6) = 0;
    At<uint16_t>(self, 8) = 0;
    return self;
}

// 0x66A8D0 (thiscall)
uint32_t __fastcall ValueOverTime_NumKeys(uint8_t* self, re::Edx) {
    return self[4];
}

// 0x66A8E0 (thiscall): a key added (its time and components appended to the store's
// arrays; the first one's places kept)
void __fastcall ValueOverTime_AddKey(uint8_t* self, re::Edx, const float* values, float time) {
    NameKey_Destruct(valueStore_B6F1D8);
    if (self[4] != 0) {
        uint32_t index = ValueStore_TimeCount(valueStore_B6F1D8);
        ValueStore_AddTime(valueStore_B6F1D8);
        keyTimes[index] = time;
        uint32_t first = ValueStore_ValueCount(valueStore_B6F1D8);
        for (int i = 0; i < ((self[1] >> 3) & 7); i++) {
            EffectCount_Add(valueStore_B6F1D8);
            keyValues[first + i] = values[i];
        }
    } else {
        At<uint16_t>(self, 6) = uint16_t(ValueStore_TimeCount(valueStore_B6F1D8));
        ValueStore_AddTime(valueStore_B6F1D8);
        keyTimes[At<uint16_t>(self, 6)] = time;
        At<uint16_t>(self, 8) = uint16_t(ValueStore_ValueCount(valueStore_B6F1D8));
        for (int i = 0; i < ((self[1] >> 3) & 7); i++) {
            EffectCount_Add(valueStore_B6F1D8);
            keyValues[At<uint16_t>(self, 8) + i] = values[i];
        }
    }
    self[4] += 1;
}

// 0x66AA00 (thiscall): a key's values and time
void __fastcall ValueOverTime_GetKey(uint8_t* self, re::Edx, float* values, float* time, int index) {
    *time = keyTimes[At<uint16_t>(self, 6) + index];
    int n = (self[1] >> 3) & 7;
    for (int i = 0; i < n; i++) {
        values[i] = keyValues[At<uint16_t>(self, 8) + n * index + i];
        n = (self[1] >> 3) & 7;
    }
}

// ---- a value over time's state: +4 its definition, +0xC the slope of its first component,
// +0x10 its key (low 7 bits; bit 7: a single constant component) ------------------------------

// 0x66AA60 (thiscall)
uint8_t* __fastcall ValueOverTimeInstance_Ctor(uint8_t* self, re::Edx) {
    reinterpret_cast<void(__thiscall*)(uint8_t*)>(0x006693E0)(self);
    At<float>(self, 8) = 0.0f;
    At<uint32_t>(self, 4) = 0;
    At<float>(self, 0xC) = 0.0f;
    self[0x10] = 0;
    return self;
}

// 0x66AA80 (thiscall)
uint8_t __fastcall ValueOverTimeInstance_InitBasic(uint8_t* self, re::Edx, uint8_t* definition) {
    At<float>(self, 8) = 0.0f;
    At<uint8_t*>(self, 4) = definition;
    At<float>(self, 0xC) = 0.0f;
    self[0x10] = 0;
    return 1;
}

// 0x66AAA0 (thiscall): a time within its loop (+2: the loop's first key, +3 its length in
// keys, +5 how many times it loops; +1 bit 2 forever): past the loops, the time after them
float __fastcall ValueOverTimeInstance_LoopTime(uint8_t* self, re::Edx, float t) {
    uint8_t* def = At<uint8_t*>(self, 4);
    uint32_t start = def[2] & 0x7F;
    uint32_t first = At<uint16_t>(def, 6);
    float loopStart = keyTimes[start + first];
    float loopEnd = keyTimes[(def[3] & 0x7F) + start + first];
    int32_t loops = def[5];
    float length = loopEnd - loopStart;
    float end = float(loops) * length;
    end = end + loopStart;
    if (def[1] & 4) {
        end = 500000.0f;
    }
    if (t >= loopStart && end > t) {
        float q = (t - loopStart) / length;
        float fraction = q - float(int(q));   // (_ftol)
        float into = length * fraction;
        return loopStart + into;
    }
    if (end > t || end != end || t != t) {
        return t;
    }
    return t - end;
}

// 0x66ABA0 (thiscall): its components zeroed
void __fastcall ValueOverTimeInstance_Zero(uint8_t* self, re::Edx, float* out) {
    float n = float(int((At<uint8_t*>(self, 4)[1] >> 3) & 7));
    if (n > 0.0f) {
        int i = 0;
        do {
            out[i] = 0.0f;
            i++;
        } while (float(i) < n);
    }
}

// 0x66ABF0 (thiscall): a key's values
void __fastcall ValueOverTimeInstance_KeyValues(uint8_t* self, re::Edx, float* out, int index) {
    uint8_t* def = At<uint8_t*>(self, 4);
    int n = (def[1] >> 3) & 7;
    for (int i = 0; i < n; i++) {
        out[i] = keyValues[At<uint16_t>(def, 8) + n * index + i];
    }
}

// 0x66ACC0 (thiscall): its components at a time between its key and the next (the first
// by its kept slope)
void __fastcall ValueOverTimeInstance_Interpolate(uint8_t* self, re::Edx, float* out, float t) {
    uint8_t* def = At<uint8_t*>(self, 4);
    uint32_t key = self[0x10] & 0x7F;
    int n = (def[1] >> 3) & 7;
    float d = t - keyTimes[At<uint16_t>(def, 6) + key];
    d = d * At<float>(self, 0xC);
    out[0] = d + keyValues[key * n + At<uint16_t>(def, 8)];
    if (n <= 1) {
        return;
    }
    const float* times = &keyTimes[key + At<uint16_t>(At<uint8_t*>(self, 4), 6)];
    float span = times[1] - times[0];
    for (int i = 1; i < n; i++) {
        uint32_t k = self[0x10] & 0x7F;
        uint8_t* definition = At<uint8_t*>(self, 4);
        const float* v0 = &keyValues[k * n + At<uint16_t>(definition, 8) + i];
        float since = t - keyTimes[At<uint16_t>(definition, 6) + k];
        float slope = (keyValues[(k + 1) * n + At<uint16_t>(definition, 8) + i] - *v0) / span;
        out[i] = since * slope + *v0;
    }
}

// 0x66AF80 (thiscall): the slope of its first component to the next key
void __fastcall ValueOverTimeInstance_ComputeSlope(uint8_t* self, re::Edx) {
    uint8_t* def = At<uint8_t*>(self, 4);
    int n = (def[1] >> 3) & 7;
    uint32_t key = self[0x10] & 0x7F;
    const float* times = &keyTimes[At<uint16_t>(def, 6) + key];
    float rise = keyValues[(key + 1) * n + At<uint16_t>(def, 8)] - keyValues[key * n + At<uint16_t>(def, 8)];
    float run = times[1] - times[0];
    At<float>(self, 0xC) = rise / run;
}

// 0x66AFE0 (thiscall): bit 7 of +0x10 set for a single component with no interpolation
// flags (+1 bits 0..1)
void __fastcall ValueOverTimeInstance_UpdateConstant(uint8_t* self, re::Edx) {
    uint8_t flags = At<uint8_t*>(self, 4)[1];
    uint8_t constant = (!(flags & 3) && (flags & 0x38) == 8) ? 1 : 0;
    self[0x10] = uint8_t((self[0x10] & 0x7F) | (constant << 7));
}

// 0x66B020 (thiscall): InterpolatorDefinitionLoader's vf1: a value over time read (its
// KeyCount keys, a time and its components each; with a preset store their places come
// from the file), its loop count, forever flag, loop start and length, and whether only
// the keys' values are used. (The loop start, length and last flag are read into
// uninitialised locals.)
void __fastcall InterpolatorDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* def = At<uint8_t*>(self, 4);
    NameKey_Destruct(valueStore_B6F1D8);
    int32_t keyCount = 0;
    vc::Call<void>(archive, 0x38, &keyCount, keyCountName_93D580);
    int32_t timeStart;
    int32_t valueStart;
    vc::Call<void>(archive, 0x38, &timeStart, name_93D650);
    vc::Call<void>(archive, 0x38, &valueStart, name_93D64C);
    for (int key = 0; key < keyCount; key++) {
        float time;
        vc::Call<void>(archive, 0x28, &time, name_93D57C);
        ValueStore_Read_8A20A0(valueStore_B6F1D8, time);
        float values[8];
        for (int i = 0; i < ((def[1] >> 3) & 7); i++) {
            vc::Call<void>(archive, 0x28, &values[i], name_93D578);
            ValueStore_Read_8A20A0(valueStore_B6F1D8, values[i]);
            ValueStore_IsPreset(valueStore_B6F1D8);
        }
        if (!ValueStore_IsPreset(valueStore_B6F1D8)) {
            ValueOverTime_AddKeyFn(def, values, time);
        }
    }
    if (ValueStore_IsPreset(valueStore_B6F1D8)) {
        At<uint16_t>(def, 6) = uint16_t(timeStart);
        At<uint16_t>(def, 8) = uint16_t(valueStart);
        def[4] = uint8_t(keyCount);
    }
    NameKey_Destruct(valueStore_B6F1D8);
    NameKey_Destruct(valueStore_B6F1D8);
    int32_t loops;
    vc::Call<void>(archive, 0x38, &loops, name_93D584);
    def[5] = uint8_t(loops);
    uint8_t forever = 0;
    vc::Call<void>(archive, 0x3C, &forever, name_93D588);
    def[1] ^= uint8_t((forever * 4) ^ def[1]) & 4;
    uint8_t flags = def[1];
    if (loops > 0 || (flags & 4)) {
        flags |= 2;
    } else {
        flags &= 0xFD;
    }
    def[1] = flags;
    int32_t loopStart;
    vc::Call<void>(archive, 0x38, &loopStart, name_93D58C);
    def[2] ^= uint8_t(def[2] ^ uint8_t(loopStart)) & 0x7F;
    int32_t loopLength;
    vc::Call<void>(archive, 0x38, &loopLength, name_93D590);
    def[3] ^= uint8_t(def[3] ^ uint8_t(loopLength)) & 0x7F;
    uint8_t keysOnly;
    vc::Call<void>(archive, 0x3C, &keysOnly, name_93D594);
    def[1] ^= uint8_t(def[1] ^ keysOnly) & 1;
    NameKey_Destruct(valueStore_B6F1D8);
}

// 0x66B250 (thiscall): its components at a time (looped when it loops): the first key's
// before it, the last key's after it (zero when only the keys' values count), else
// interpolated from the key it's at (+8: when the next key starts). (Stepping to the next
// key only happens from a key after the first: from the first one it searches.)
void __fastcall ValueOverTimeInstance_Evaluate(uint8_t* self, re::Edx, float* out, float t) {
    uint8_t* def = At<uint8_t*>(self, 4);
    if (def[1] & 2) {
        t = ValueOverTimeInstance_LoopTimeFn(self, t);
    }
    const float* times = keyTimes;
    uint32_t first = At<uint16_t>(def, 6);
    if (!(times[first] < t || times[first] != times[first] || t != t)) {
        self[0x10] = 0;
        if (!(def[1] & 1)) {
            ValueOverTimeInstance_KeyValuesFn(self, out, 0);
        } else {
            ValueOverTimeInstance_ZeroFn(self, out);
        }
        At<float>(self, 0xC) = 0.0f;
        At<float>(self, 8) = keyTimes[At<uint16_t>(At<uint8_t*>(self, 4), 6)];
        return;
    }
    int keys = def[4];
    float last = times[keys + first - 1];
    if (last < t || last == t) {
        self[0x10] &= 0x7F;
        uint8_t key = self[0x10];
        int lastKey = def[4] - 1;
        if (!(def[1] & 1)) {
            ValueOverTimeInstance_KeyValuesFn(self, out, lastKey);
        } else if (int(key & 0x7F) != lastKey) {
            ValueOverTimeInstance_KeyValuesFn(self, out, lastKey);
        } else {
            ValueOverTimeInstance_ZeroFn(self, out);
        }
        At<float>(self, 0xC) = 0.0f;
        self[0x10] ^= uint8_t((At<uint8_t*>(self, 4)[4] - 1) ^ self[0x10]) & 0x7F;
        return;
    }
    float nextStart = At<float>(self, 8);
    if (nextStart < t || nextStart == t) {
        uint8_t key = self[0x10];
        float next = times[(key & 0x7F) + first + 1];
        if (next > t) {
            if (key & 0x7F) {
                self[0x10] = uint8_t((((key + 1) ^ key) & 0x7F) ^ key);
            }
            ValueOverTimeInstance_ComputeSlopeFn(self);
            At<float>(self, 8) = keyTimes[(self[0x10] & 0x7F) + At<uint16_t>(At<uint8_t*>(self, 4), 6) + 1];
            ValueOverTimeInstance_InterpolateFn(self, out, t);
            ValueOverTimeInstance_UpdateConstantFn(self);
            return;
        }
    }
    if (nextStart > t) {
        uint8_t key = self[0x10];
        float current = times[(key & 0x7F) + first];
        if (current < t || current == t) {
            self[0x10] = key & 0x7F;
            if (!(def[1] & 1)) {
                ValueOverTimeInstance_InterpolateFn(self, out, t);
            } else {
                ValueOverTimeInstance_ZeroFn(self, out);
            }
            return;
        }
    }
    // search for the key it's at
    bool found = false;
    if (keys > 0) {
        for (int i = 0; i < def[4]; i++) {
            if (found) {
                continue;
            }
            const float* at = &times[At<uint16_t>(def, 6) + i];
            if (!(*at < t || *at == t)) {
                continue;
            }
            if (i < def[4] && !(at[1] > t)) {
                continue;
            }
            self[0x10] ^= uint8_t(self[0x10] ^ uint8_t(i)) & 0x7F;
            times = keyTimes;
            found = true;
        }
    }
    ValueOverTimeInstance_ComputeSlopeFn(self);
    At<float>(self, 8) = keyTimes[(self[0x10] & 0x7F) + At<uint16_t>(At<uint8_t*>(self, 4), 6) + 1];
    ValueOverTimeInstance_InterpolateFn(self, out, t);
    uint8_t flags = At<uint8_t*>(self, 4)[1];
    uint8_t constant = (!(flags & 3) && (flags & 0x38) == 8) ? 1 : 0;
    self[0x10] = uint8_t((constant << 7) | (self[0x10] & 0x7F));
}

// 0x66B4F0 (thiscall): its first component at a time (by the kept slope while a constant
// one is before its next key, else evaluated)
void __fastcall ValueOverTimeInstance_Get1(uint8_t* self, re::Edx, float* out, float t) {
    *out = 0.0f;
    if ((self[0x10] & 0x80) && At<float>(self, 8) > t) {
        uint8_t* def = At<uint8_t*>(self, 4);
        uint32_t key = self[0x10] & 0x7F;
        float v = t - keyTimes[At<uint16_t>(def, 6) + key];
        v = v * At<float>(self, 0xC);
        *out = v + keyValues[At<uint16_t>(def, 8) + key];
        return;
    }
    ValueOverTimeInstance_EvaluateFn(self, out, t);
}

// ---- the value store (0xB6F1D8, InterpolatorElementStore): +4 the number of key times,
// +8 of key values, +0xC preset (their places read from the file); the arrays at 0x1B838D0
// (times) and 0x1B838CC (values) ------------------------------------------------------------

// 0x66B560 (thiscall): its destructor
void __fastcall ValueStore_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtInterpolatorElementStore;
}

inline uint32_t SaturatedBytes(uint32_t count) {
    uint64_t bytes = uint64_t(count) * 4;
    return (bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes);
}

// 0x66B570 (thiscall): the arrays allocated for its counts
void __fastcall ValueStore_AllocateImpl(uint8_t* self, re::Edx) {
    uint32_t values = At<uint32_t>(self, 8);
    keyTimes = static_cast<float*>(Malloc(SaturatedBytes(At<uint32_t>(self, 4))));
    keyValues = static_cast<float*>(Malloc(SaturatedBytes(values)));
}

// 0x66B5C0 / 0x66B5D0 (thiscall): preset / the number of times (too short to hook)
[[maybe_unused]] uint8_t __fastcall ValueStore_IsPresetImpl(uint8_t* self, re::Edx) {
    return self[0xC];
}
[[maybe_unused]] uint32_t __fastcall ValueStore_TimeCountImpl(uint8_t* self, re::Edx) {
    return At<uint32_t>(self, 4);
}
RE_UNHOOKED_FN("ValueStore::IsPreset", 0x0066B5C0, &ValueStore_IsPresetImpl);
// 0x0066B5D0: [8b 41 04 c3 cc cc cc cc] too short to hook: the 64-bit dispatch's body
static uint32_t __fastcall Tiny_0066B5D0(uint8_t* self, re::Edx) {
    return *reinterpret_cast<const uint32_t*>(self + 0x4);
}
RE_UNHOOKED_FN("ValueStore::TimeCount", 0x0066B5D0, &Tiny_0066B5D0);

// 0x66B5E0 (thiscall)
void __fastcall ValueStore_AddTimeImpl(uint8_t* self, re::Edx) {
    At<int32_t>(self, 4) += 1;
}

// 0x66B5F0 (thiscall): StoreLoader's vf1: every key time and value read (the counts in
// the file read and dropped: the store's own are used)
void __fastcall StoreLoader_Read(uint8_t*, re::Edx, void* archive) {
    int32_t count;
    vc::Call<void>(archive, 0x38, &count, countName_93D548);
    for (int i = 0; i < At<int32_t>(valueStore_B6F1D8, 4); i++) {
        vc::Call<void>(archive, 0x28, &keyTimes[i], name_93D57C);
    }
    vc::Call<void>(archive, 0x38, &count, countName_93D548);
    for (int i = 0; i < At<int32_t>(valueStore_B6F1D8, 8); i++) {
        vc::Call<void>(archive, 0x28, &keyValues[i], name_93D578);
    }
}

// 0x66B6A0 (thiscall): StoreLoader's vf2: the counts and every key time and value written
void __fastcall StoreLoader_Write(uint8_t*, re::Edx, void* archive) {
    vc::Call<void>(archive, 0x60, At<int32_t>(valueStore_B6F1D8, 4), countName_93D548);
    for (int i = 0; i < At<int32_t>(valueStore_B6F1D8, 4); i++) {
        vc::Call<void>(archive, 0x50, keyTimes[i], name_93D57C);
    }
    vc::Call<void>(archive, 0x60, At<int32_t>(valueStore_B6F1D8, 8), countName_93D548);
    for (int i = 0; i < At<int32_t>(valueStore_B6F1D8, 8); i++) {
        vc::Call<void>(archive, 0x50, keyValues[i], name_93D578);
    }
}

// 0x66B750 (thiscall): a preset store's keys loaded from an archive
void __fastcall ValueStore_Load(uint8_t* self, re::Edx, const char* path) {
    if (!self[0xC]) {
        return;
    }
    ValueStore_Allocate(self);
    int32_t mode = 0;
    void* archive = Archive_Open(path, &mode, 1);
    uint32_t loader = kVtStoreLoader;
    Archive_ReadSection(archive, &loader, storeName_93D654);
    Archive_Close(archive);
}

// 0x66B7B0 (thiscall)
uint8_t* __fastcall ValueStore_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtInterpolatorElementStore;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    keyTimes = nullptr;
    keyValues = nullptr;
    self[0xC] = 1;
    return self;
}

// 0x66B7E0 (thiscall): its deleting destructor
uint8_t* __fastcall ValueStore_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtInterpolatorElementStore;
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x66B800 (thiscall): the arrays freed
void __fastcall ValueStore_FreeArrays(uint8_t*, re::Edx) {
    if (keyTimes) {
        Free(keyTimes);
        keyTimes = nullptr;
    }
    if (keyValues) {
        Free(keyValues);
        keyValues = nullptr;
    }
}

// ---- light primitives -----------------------------------------------------------------------
// The definition: +0x10 its colour over time (3 components), +0x14 its radius, +0x18 the
// detail it needs, +0x1C bit 0 an intensity only (no light). The instance: +0x14 its
// colour's state, +0x18 intensity (0..255), +0x1C its light slot, +0x20 shown.

// 0x66B840 (thiscall)
uint8_t* __fastcall LightDefinition_Ctor(uint8_t* self, re::Edx) {
    PrimDefinition_Construct(self);
    ValueOverTime_Construct(self + 0x10);
    self[0x1C] &= 0xFE;
    At<float>(self, 0x14) = 1.0f;
    At<float>(self, 0x18) = 0.0f;
    ValueOverTime_SetComponents(self + 0x10, 3);
    return self;
}

// 0x66B870 (thiscall): LightPrimitiveDefinitionLoader's vf1
void __fastcall LightPrimitiveDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t base[8];
    DefinitionBaseLoader_Construct(base, definition);
    At<uint32_t>(base, 0) = kVtEffectPrimitiveDefinitionLoader;
    EffectPrimitiveDefinitionLoader_Read(base, archive);
    uint8_t colour[8];
    ValueOverTimeDefinitionLoader_Construct(colour, definition + 0x10);
    Archive_ReadSection(archive, colour, name_93D56C);
    vc::Call<void>(archive, 0x28, definition + 0x14, name_93D578);
    vc::Call<void>(archive, 0x28, definition + 0x18, name_93D578);
    uint8_t intensityOnly = 0;
    vc::Call<void>(archive, 0x3C, &intensityOnly, name_93D644);
    definition[0x1C] ^= uint8_t(definition[0x1C] ^ intensityOnly) & 1;
}

// 0x66B910 (thiscall)
uint8_t* __fastcall Light_Ctor(uint8_t* self, re::Edx) {
    PrimInstance_Construct(self);
    ValueOverTimeInstance_Construct(self + 0x14);
    At<uint32_t>(self, 0x1C) &= 0xFFFFFF80;
    self[0x20] &= 0xFE;
    At<int32_t>(self, 0x18) = 0;
    return self;
}

// 0x66B940 (thiscall): shown (or its intensity kept) while the effect's detail is at
// least the definition's
void __fastcall Light_Update(uint8_t* self, re::Edx, uint8_t*) {
    uint8_t* definition = At<uint8_t*>(self, 8);
    float needed = At<float>(definition, 0x18);
    float detail = At<float>(At<uint8_t*>(self, 0xC), 0x4C);
    if (needed < detail || needed == detail) {
        if (definition[0x1C] & 1) {
            float time = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
            float colour[3];
            ValueOverTimeInstance_Get(self + 0x14, colour, time);
            At<int32_t>(self, 0x18) = int(colour[0] * 255.0f);   // (_ftol)
            return;
        }
        EffectLightManager_Add(effectLights_B6F1F4, self);
        return;
    }
    if (definition[0x1C] & 1) {
        At<int32_t>(self, 0x18) = 0;
        return;
    }
    EffectLightManager_Remove(effectLights_B6F1F4, self);
}

// 0x66B9E0 (thiscall): started from its definition
uint8_t __fastcall Light_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint8_t*>(self, 8) = definition;
    At<int32_t>(self, 0x18) = 0;
    uint8_t made = ValueOverTimeInstance_Init(self + 0x14, definition + 0x10) != 0;
    PrimInstance_Restart(self);
    return made;
}

// 0x66BA20 (thiscall): stopped (no longer shown)
void __fastcall Light_Stop(uint8_t* self, re::Edx) {
    if (!(At<uint8_t*>(self, 8)[0x1C] & 1)) {
        EffectLightManager_Remove(effectLights_B6F1F4, self);
    }
    if (!(self[0x10] & 1)) {
        PrimInstance_Kill(self);
    }
    ValueOverTimeInstance_Stop(self + 0x14);
}

// 0x66BA50 (thiscall): its point light added at the effect's position (straight up, the
// definition's radius, its colour now)
void __fastcall Light_AddPointLight(uint8_t* self, re::Edx) {
    uint8_t* effect = At<uint8_t*>(self, 0xC);
    float y = At<float>(effect, 0x30);
    float z = At<float>(effect, 0x34);
    float x = At<float>(effect, 0x2C);
    float time = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    float colour[3];
    ValueOverTimeInstance_Get(self + 0x14, colour, time);
    PointLights_AddLight(0, x, y, z, 0.0f, 0.0f, 1.0f, At<float>(At<uint8_t*>(self, 8), 0x14), colour[0], colour[1],
                         colour[2], 0, 0);
}

// ---- trails (0x1B850E0): 20 ribbons of up to 31 point pairs ---------------------------------
// A trail (0x330 bytes): +0 its id, +4 points, +8 the oldest (once full), +0xC capacity,
// +0x10 how much of the last point each keeps a frame (the fade), +0x14 free, +0x18 frames
// left once released (-1: held), +0x1C age in frames, +0x20, +0x24 the point pairs (0x18
// each), +0x30C its texture, +0x310, +0x314, +0x318 its priority, +0x31C bit 0 in world
// space (else relative to +0x320), +0x32C. The manager: +0x3FC0 the last id, +0x3FC4 any
// updated, +0x3FC8 the default texture, +0x3FCC the last point added.
constexpr int kTrails = 20;
constexpr int kTrailSize = 0x330;

// 0x66BB00 (thiscall): a trail started (a free one, else the oldest one whose priority
// doesn't exceed this one's... the replaced one's priority is what's compared); its id
inline uint8_t* TrailAt(uint8_t* self, int i) {
    return self + i * kTrailSize;
}
int32_t __fastcall Trails_Start(uint8_t* self, re::Edx, int32_t capacity, float fade, uint32_t param, int32_t priority) {
    At<int32_t>(self, 0x3FC0) += 1;
    int index = -1;
    for (int i = 0; i < kTrails; i++) {
        if (TrailAt(self, i)[0x14] == 1) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        int best = 0;
        for (int i = 0; i < kTrails; i++) {
            if (At<uint32_t>(TrailAt(self, i), 0x1C) > At<uint32_t>(TrailAt(self, best), 0x1C) &&
                At<int32_t>(TrailAt(self, best), 0x318) <= priority) {
                best = i;
            }
        }
        index = best;
    }
    uint8_t* trail = TrailAt(self, index);
    At<float>(trail, 0x10) = 1.0f;
    At<float>(trail, 0x310) = 0.0f;
    At<float>(trail, 0x32C) = 1.0f;
    constexpr float kFadeScale = 1.0f - 0.97f;
    float keep = fade * kFadeScale;
    keep = keep + 0.97f;
    trail[0x31C] |= 1;
    At<int32_t>(trail, 0) = 0;
    At<int32_t>(trail, 4) = 0;
    At<int32_t>(trail, 8) = 0;
    At<uint32_t>(trail, 0x1C) = 0;
    At<uint32_t>(trail, 0x30C) = 0;
    At<uint32_t>(trail, 0x314) = 0;
    trail[0x14] = 1;
    At<int32_t>(trail, 0x18) = -1;
    At<int32_t>(trail, 0xC) = 0x1F;
    At<int32_t>(trail, 0x318) = 2;
    At<int32_t>(trail, 0x318) = priority;
    trail[0x14] = 0;
    At<int32_t>(trail, 0) = At<int32_t>(self, 0x3FC0);
    At<float>(trail, 0x10) = keep;
    At<uint32_t>(trail, 0x20) = param;
    At<int32_t>(trail, 0xC) = capacity;
    self[0x3FC4] = 1;
    trail[0x31C] |= 1;
    At<float>(trail, 0x310) = 0.0f;
    At<uint32_t>(trail, 0x314) = 0;
    At<uint32_t>(trail, 0x30C) = At<uint32_t>(self, 0x3FC8);
    return At<int32_t>(self, 0x3FC0);
}

// 0x66BD10 (thiscall): a trail by id (the last one with it; null: none)
uint8_t* __fastcall Trails_FindImpl(uint8_t* self, re::Edx, int32_t id) {
    uint8_t* found = nullptr;
    for (int i = 0; i < kTrails; i++) {
        if (At<int32_t>(TrailAt(self, i), 0) == id) {
            found = TrailAt(self, i);
        }
    }
    return found;
}

// 0x66BD80 (thiscall): a trail's points made relative to a position
void __fastcall Trails_SetOrigin(uint8_t* self, re::Edx, int32_t id, const float* origin) {
    uint8_t* trail = Trails_Find(self, id);
    if (!trail) {
        return;
    }
    trail[0x31C] &= 0xFE;
    At<float>(trail, 0x320) = origin[0];
    At<float>(trail, 0x324) = origin[1];
    At<float>(trail, 0x328) = origin[2];
}

// 0x66BDC0 (thiscall)
void __fastcall Trails_Set32C(uint8_t* self, re::Edx, int32_t id, float value) {
    if (Trails_Find(self, id)) {
        At<float>(Trails_Find(self, id), 0x32C) = value;
    }
}

// 0x66BDF0 (thiscall, the manager unused): a trail's points drawn together by its fade
// (each pair's first towards its second, then the second towards the new first)
void __fastcall Trails_FadeImpl(uint8_t*, re::Edx, uint8_t* trail) {
    if (1.0f == At<float>(trail, 0x10)) {
        return;
    }
    for (int i = 0; i < At<int32_t>(trail, 4); i++) {
        float* a = &At<float>(trail, 0x24 + i * 0x18);
        float* b = a + 3;
        for (int k = 0; k < 3; k++) {
            float keep = At<float>(trail, 0x10);
            float moved = (1.0f - keep) * b[k];
            a[k] = moved + a[k] * keep;
        }
        for (int k = 0; k < 3; k++) {
            float keep = At<float>(trail, 0x10);
            float moved = (1.0f - keep) * a[k];
            b[k] = moved + b[k] * keep;
        }
    }
}

// 0x66BEA0 (thiscall): a point pair's constructor (nothing: `mov eax, ecx; ret`, the object back).
// Too short to hook in the 32-bit game; the 64-bit dispatch's body
static void* __fastcall PointConstruct(void* self, re::Edx) {
    return self;
}
RE_UNHOOKED_FN("Trails::PointConstruct", 0x0066BEA0, &PointConstruct);

// 0x66BEB0 / 0x66BED0 / 0x66BF20 (thiscall): a trail's field set (no check that it exists)
void __fastcall Trails_Set310(uint8_t* self, re::Edx, int32_t id, float value) {
    At<float>(Trails_Find(self, id), 0x310) = value;
}
void __fastcall Trails_Set20(uint8_t* self, re::Edx, int32_t id, uint32_t value) {
    At<uint32_t>(Trails_Find(self, id), 0x20) = value;
}
void __fastcall Trails_Set314(uint8_t* self, re::Edx, int32_t id, uint32_t value) {
    At<uint32_t>(Trails_Find(self, id), 0x314) = value;
}

// 0x66BEF0 (thiscall): a trail's texture, by kind
void __fastcall Trails_SetTexture(uint8_t* self, re::Edx, int32_t id, int kind) {
    uint8_t* trail = Trails_Find(self, id);
    At<uint32_t>(trail, 0x30C) = At<uint32_t>(trailTextures_B6F1F0, kind * 12 + 0x5C);
}

// 0x66BF40 (thiscall): let go of (it fades out over 2 frames)
void __fastcall Trails_Release(uint8_t* self, re::Edx, int32_t id) {
    uint8_t* trail = Trails_Find(self, id);
    if (trail && trail[0x14] == 0 && At<int32_t>(trail, 0x18) == -1) {
        At<int32_t>(trail, 0x18) = 2;
    }
}

// 0x66BF70 (thiscall): freed at once
void __fastcall Trails_Free(uint8_t* self, re::Edx, int32_t id) {
    uint8_t* trail = Trails_Find(self, id);
    if (trail) {
        trail[0x14] = 1;
    }
}

// 0x66BF90 (thiscall): a point pair added (both as 4-float vectors; relative to its origin
// unless in world space); once full, over the oldest
void __fastcall Trails_AddPoints(uint8_t* self, re::Edx, int32_t id, float ax, float ay, float az, float, float bx,
                                 float by, float bz, float) {
    At<float>(self, 0x3FCC) = ax;
    At<float>(self, 0x3FD0) = ay;
    At<float>(self, 0x3FD4) = az;
    uint8_t* trail = Trails_Find(self, id);
    if (!trail) {
        return;
    }
    float a[3] = {ax, ay, az};
    float b[3] = {bx, by, bz};
    int32_t count = At<int32_t>(trail, 4);
    int32_t capacity = At<int32_t>(trail, 0xC);
    int32_t slot;
    if (count < capacity) {
        slot = count;
    } else {
        At<int32_t>(trail, 8) += 1;
        if (At<int32_t>(trail, 8) >= capacity) {
            At<int32_t>(trail, 8) = 0;
        }
        slot = (capacity + At<int32_t>(trail, 8) - 1) % capacity;
    }
    float* pa = &At<float>(trail, 0x24 + slot * 0x18);
    float* pb = &At<float>(trail, 0x30 + slot * 0x18);
    if (trail[0x31C] & 1) {
        pa[0] = a[0];
        pa[1] = a[1];
        pa[2] = a[2];
        pb[0] = b[0];
        pb[1] = b[1];
        pb[2] = b[2];
    } else {
        VectorSub(pa, a, &At<float>(trail, 0x320));
        VectorSub(pb, b, &At<float>(trail, 0x320));
    }
    if (count < capacity) {
        At<int32_t>(trail, 4) += 1;
    }
}

// 0x66C0F0 (thiscall): every trail in use faded and aged; a released one freed when its
// frames run out
void __fastcall Trails_Update(uint8_t* self, re::Edx) {
    self[0x3FC4] = 0;
    for (int i = 0; i < kTrails; i++) {
        uint8_t* trail = TrailAt(self, i);
        if (trail[0x14] != 0) {
            continue;
        }
        Trails_Fade(self, trail);
        self[0x3FC4] = 1;
        int32_t left = At<int32_t>(trail, 0x18);
        At<uint32_t>(trail, 0x1C) += 1;
        if (left > 0) {
            At<int32_t>(trail, 0x18) = left - 1;
            if (left - 1 == 0) {
                trail[0x14] = 1;
            }
        }
    }
}

// 0x66C9F0 (thiscall): the trails' constructor (all free)
uint8_t* __fastcall Trails_Ctor(uint8_t* self, re::Edx) {
    for (int i = 0; i < kTrails; i++) {
        uint8_t* trail = TrailAt(self, i);
        VectorConstruct(trail + 0x24, 0x18, 0x1F, reinterpret_cast<void*>(0x0066BEA0));
        trail[0x31C] |= 1;
        At<float>(trail, 0x10) = 1.0f;
        At<int32_t>(trail, 0) = 0;
        At<float>(trail, 0x310) = 0.0f;
        At<int32_t>(trail, 4) = 0;
        At<int32_t>(trail, 8) = 0;
        trail[0x14] = 1;
        At<int32_t>(trail, 0x18) = -1;
        At<float>(trail, 0x32C) = 1.0f;
        At<uint32_t>(trail, 0x1C) = 0;
        At<int32_t>(trail, 0xC) = 0x1F;
        At<uint32_t>(trail, 0x30C) = 0;
        At<uint32_t>(trail, 0x314) = 0;
        At<int32_t>(trail, 0x318) = 2;
    }
    self[0x3FC4] = 0;
    At<uint32_t>(self, 0x3FC8) = 0;
    At<int32_t>(self, 0x3FC0) = 1;
    return self;
}

inline void CopyPair(float* dst, const float* src) {
    for (int i = 0; i < 6; i++) {
        reinterpret_cast<uint32_t*>(dst)[i] = reinterpret_cast<const uint32_t*>(src)[i];
    }
}
inline void PutVertex(uint8_t* v, const float* pos, uint32_t colour, float u, float w) {
    At<float>(v, 0) = pos[0];
    At<float>(v, 4) = pos[1];
    At<float>(v, 8) = pos[2];
    At<uint32_t>(v, 0x18) = colour;
    At<float>(v, 0x1C) = u;
    At<float>(v, 0x20) = w;
}

// 0x66C150 (thiscall, the manager unused): a trail (more than 2 points) turned into
// Im3D quads: its pairs in order (in the world), smoothed by Catmull-Rom into +0x314 steps
// a segment when set, then a quad per segment, fading in by steps of its colour's alpha
// (+0x23 * +0x32C over the segments; halved per frame left once released) and out to
// nothing at the end
void __fastcall Trails_BuildVertices(uint8_t*, re::Edx, uint8_t* trail) {
    if (At<int32_t>(trail, 4) <= 2) {
        return;
    }
    uint32_t guards = trailBufferGuards_1B850D8;
    if (!(guards & 1)) {
        guards |= 1;
        trailBufferGuards_1B850D8 = guards;
    }
    float* points = trailPoints_1B844D8;
    int32_t j = At<int32_t>(trail, 8);
    for (int i = 0; i < At<int32_t>(trail, 4); i++, j++) {
        if (j >= At<int32_t>(trail, 0xC)) {
            j = 0;
        }
        CopyPair(points + i * 6, &At<float>(trail, 0x24 + j * 0x18));
    }
    if (!(trail[0x31C] & 1)) {
        for (int i = 0; i < At<int32_t>(trail, 4); i++) {
            float* p = points + i * 6;
            p[0] = At<float>(trail, 0x320) + p[0];
            p[1] = p[1] + At<float>(trail, 0x324);
            p[2] = At<float>(trail, 0x328) + p[2];
            p[3] = p[3] + At<float>(trail, 0x320);
            p[4] = p[4] + At<float>(trail, 0x324);
            p[5] = p[5] + At<float>(trail, 0x328);
        }
    }
    int32_t count = At<int32_t>(trail, 4);
    int32_t steps = At<int32_t>(trail, 0x314);
    int32_t segments = count - 1;
    if (steps > 0) {
        if (!(guards & 2)) {
            guards |= 2;
            trailBufferGuards_1B850D8 = guards;
        }
        float* control = trailControl_1B838D8;
        for (int i = 0; i < At<int32_t>(trail, 4); i++) {
            CopyPair(control + i * 6, points + i * 6);
        }
        float stepT = 1.0f / float(steps);
        for (int seg = 0; seg < At<int32_t>(trail, 4) - 1; seg++) {
            float before[6];
            float after[6];
            float from[6];
            float to[6];
            if (seg == 0) {
                CopyPair(before, control);
                CopyPair(after, control + 2 * 6);
            } else if (seg == At<int32_t>(trail, 4) - 2) {
                CopyPair(before, control + (seg - 1) * 6);
                CopyPair(after, control + (seg + 1) * 6);
            } else {
                CopyPair(before, control + (seg - 1) * 6);
                CopyPair(after, control + (seg + 2) * 6);
            }
            CopyPair(from, control + seg * 6);
            CopyPair(to, control + (seg + 1) * 6);
            float t = 0.0f;
            for (int k = 0; k < steps; k++) {
                float* out = points + (steps * seg + k) * 6;
                float a[3];
                CatmullRom(a, before, from, to, after, t);
                out[0] = a[0];
                out[1] = a[1];
                out[2] = a[2];
                float b[3];
                CatmullRom(b, before + 3, from + 3, to + 3, after + 3, t);
                t = stepT + t;
                out[3] = b[0];
                out[4] = b[1];
                out[5] = b[2];
            }
        }
        int32_t last = At<int32_t>(trail, 4);
        segments = (last - 1) * steps;
        CopyPair(points + segments * 6, control + (last - 1) * 6);
    }
    uint32_t colourIn = At<uint32_t>(trail, 0x20);
    uint32_t r = colourIn & 0xFF;
    uint32_t g = (colourIn >> 8) & 0xFF;
    uint32_t b = (colourIn >> 16) & 0xFF;
    uint8_t alpha = 0;
    float s = At<float>(trail, 0x32C) * float(int32_t(trail[0x23]));
    s = s / float(segments);
    int32_t alphaStep = int32_t(s);   // (_ftol)
    if (At<int32_t>(trail, 0x18) != -1) {
        float f = float(At<int32_t>(trail, 0x18)) * 0.5f;
        f = f * float(alphaStep);
        alphaStep = int32_t(f);
    }
    int32_t vbase = im3dVertexCount;
    im3dVertexCount = vbase + segments * 4;
    uint8_t* v = im3dVertices + vbase * 0x24;
    int32_t ibase = im3dIndexCount;
    uint16_t* index = im3dIndices + ibase;
    im3dIndexCount = ibase + segments * 6;
    float u0, u1, u2, u3;
    if (0.0f == At<float>(trail, 0x310)) {
        u0 = 0.0f;
        u1 = 0.0f;
        u2 = 1.0f;
        u3 = 1.0f;
    } else {
        float shifted = At<float>(trail, 0x310) + 1.0f;
        u0 = shifted;
        u1 = shifted;
        u2 = At<float>(trail, 0x310);
        u3 = At<float>(trail, 0x310);
    }
    uint32_t rgb = (r << 16) | (g << 8) | b;
    int i = 1;
    for (; i < segments; i++) {
        const float* p0 = points + (i - 1) * 6;
        const float* p1 = points + i * 6;
        uint32_t c0 = (uint32_t(alpha) << 24) | rgb;
        uint32_t c1 = ((uint32_t(alpha) + uint8_t(alphaStep)) << 24) | rgb;
        PutVertex(v, p0, c0, u0, 1.0f);
        PutVertex(v + 0x24, p0 + 3, c0, u1, 0.0f);
        PutVertex(v + 0x48, p1 + 3, c1, u2, 0.0f);   // (the end pair the other way round)
        PutVertex(v + 0x6C, p1, c1, u3, 1.0f);
        alpha = uint8_t(alpha + uint8_t(alphaStep));
        int32_t quad = vbase + i * 4 - 4;
        index[0] = uint16_t(quad);
        index[1] = uint16_t(quad + 3);
        index[2] = uint16_t(quad + 1);
        index[3] = uint16_t(quad + 1);
        index[4] = uint16_t(quad + 3);
        index[5] = uint16_t(quad + 2);
        index += 6;
        v += 0x90;
    }
    // the last quad fades to nothing
    const float* p0 = points + (segments - 1) * 6;
    const float* p1 = points + segments * 6;
    uint32_t c0 = (uint32_t(alpha) << 24) | rgb;
    PutVertex(v, p0, c0, u0, 1.0f);
    PutVertex(v + 0x24, p0 + 3, c0, u1, 0.0f);
    PutVertex(v + 0x48, p1 + 3, rgb, u2, 0.0f);
    PutVertex(v + 0x6C, p1, rgb, u3, 1.0f);
    int32_t quad = vbase + i * 4 - 4;
    index[0] = uint16_t(quad);
    index[1] = uint16_t(quad + 3);
    index[2] = uint16_t(quad + 1);
    index[3] = uint16_t(quad + 1);
    index[4] = uint16_t(quad + 3);
    index[5] = uint16_t(quad + 2);
}

// the Im3D buffer drawn and emptied
inline void FlushTrails() {
    Im3D_Transform(im3dVertices, im3dVertexCount, nullptr, 0x19);
    Im3D_RenderIndexedPrimitive(3, im3dIndices, im3dIndexCount, 0);
    Im3D_End();
    im3dVertexCount = 0;
    im3dIndexCount = 0;
}
inline void SetTrailTexture(void* texture) {
    GetRaster(texture);
    SetRenderState(1, GetRaster(texture));
}

// 0x66CA90 (thiscall): the trails drawn (once a frame, when any is in use): additive
// blending, their textures ("blur" by default) switched between batches
void __fastcall Trails_RenderImpl(uint8_t* self, re::Edx) {
    if (trailLastFrame_1B850DC == int32_t(GetRendererFrame())) {
        return;
    }
    trailLastFrame_1B850DC = int32_t(GetRendererFrame());
    if (!self[0x3FC4]) {
        return;
    }
    void* texture = nullptr;
    if (At<uint32_t>(self, 0x3FC8) == 0) {
        uint8_t name[0x2C];
        ArchiveString_Init(name);
        Strncpy(At<char*>(name, 0), reinterpret_cast<const char*>(0x0093D760), At<uint32_t>(name, 4));   // "blur"
        uint32_t hash = HashName(At<char*>(name, 0));
        if (!EffectTextures_Has(trailTextures_B6F1F0, hash)) {
            EffectTextures_Add(trailTextures_B6F1F0, reinterpret_cast<const char*>(0x008FEFBB), At<char*>(name, 0));
        }
        int found = EffectTextures_FindIndex(trailTextures_B6F1F0, hash);
        texture = At<void*>(trailTextures_B6F1F0, found * 12 + 0x5C);
        At<void*>(self, 0x3FC8) = texture;
        SetTrailTexture(texture);
    }
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    SetRenderState(7, 2);
    SetRenderState(6, 1);
    SetRenderState(8, 0);
    SetRenderState(0xC, 1);
    SetRenderState(0xE, 0);
    SetRenderState(2, 1);
    SetRenderState(9, 2);
    SetRenderState(0x14, 1);
    im3dVertexCount = 0;
    im3dIndexCount = 0;
    for (int i = 0; i < kTrails; i++) {
        uint8_t* trail = TrailAt(self, i);
        if (trail[0x14] != 0) {
            continue;
        }
        void* own = At<void*>(trail, 0x30C);
        if (!own) {
            if (texture != At<void*>(self, 0x3FC8)) {
                if (im3dVertexCount > 0) {
                    FlushTrails();
                }
                SetTrailTexture(At<void*>(self, 0x3FC8));
                texture = At<void*>(self, 0x3FC8);
            }
        } else if (texture != own) {
            if (im3dVertexCount > 0) {
                FlushTrails();
            }
            SetTrailTexture(At<void*>(trail, 0x30C));
            texture = At<void*>(trail, 0x30C);
        }
        Trails_BuildVerticesFn(self, trail);
    }
    if (im3dVertexCount > 0) {
        FlushTrails();
    }
}

// 0x66CD10 (thiscall): a position added to a trail as a pair across it, `half` either
// side, facing the camera (at right angles to the way it moved from the last pair's
// middle); the last pair again when it hasn't moved; a first one as the position twice.
// (Its fifth argument isn't read; the pairs' fourth words, unused, are stack garbage in the game.)
void __fastcall Trails_AddPosition(uint8_t* self, re::Edx, int32_t id, float x, float y, float z, float, float half) {
    float p[3] = {x, y, z};
    float q[3] = {x, y, z};
    uint8_t* trail = Trails_Find(self, id);
    if (!trail) {
        return;
    }
    int32_t n = At<int32_t>(trail, 4);
    if (n != 0) {
        int32_t last = (At<int32_t>(trail, 8) + n) % At<int32_t>(trail, 0xC) - 1;
        if (last < 0) {
            last = n - 1;
        }
        const float* a = &At<float>(trail, 0x24 + last * 0x18);
        const float* b = &At<float>(trail, 0x30 + last * 0x18);
        float sum[3] = {b[0] + a[0], b[1] + a[1], b[2] + a[2]};
        float middle[3] = {sum[0] * 0.5f, sum[1] * 0.5f, 0.5f * sum[2]};
        float d[3] = {x - middle[0], y - middle[1], z - middle[2]};
        float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        bool still = Vector_NearlyEqual(d, zero, 0.01f) != 0;
        const float* camera = re::PtrCast<const float*>(particleRenderer_B6F1EC);
        float c[3] = {camera[0], camera[1], camera[2]};
        float toCamera[3] = {c[0] - x, c[1] - y, c[2] - z};
        float f[3] = {-d[0], -d[1], -d[2]};
        Vec_Normalise(f);
        toCamera[0] = -toCamera[0];
        toCamera[1] = -toCamera[1];
        toCamera[2] = -toCamera[2];
        float r[3];
        r[0] = f[1] * toCamera[2] - f[2] * toCamera[1];
        r[1] = f[2] * toCamera[0] - toCamera[2] * f[0];
        r[2] = f[0] * toCamera[1] - f[1] * toCamera[0];
        Vec_Normalise(r);
        float u[3];
        u[0] = r[1] * f[2] - r[2] * f[1];
        u[1] = r[2] * f[0] - f[2] * r[0];
        u[2] = r[0] * f[1] - r[1] * f[0];
        Vec_Normalise(u);
        // NiTransform: rows (r, f, u) by column, the position, scale 1
        float transform[13] = {r[0], f[0], u[0], r[1], f[1], u[1], r[2], f[2], u[2], x, y, z, 1.0f};
        float side[3] = {half, 0.0f, 0.0f};
        float other[3] = {-half, 0.0f, 0.0f};
        float out[3];
        NiTransform_Transform(transform, out, side);
        q[0] = out[0];
        q[1] = out[1];
        q[2] = out[2];
        NiTransform_Transform(transform, out, other);
        p[0] = out[0];
        p[1] = out[1];
        p[2] = out[2];
        if (still) {
            q[0] = a[0];
            q[1] = a[1];
            q[2] = a[2];
            p[0] = b[0];
            p[1] = b[1];
            p[2] = b[2];
        }
    }
    Trails_AddPointsFn(self, id, q[0], q[1], q[2], 0.0f, p[0], p[1], p[2], 0.0f);
}

// ---- motion blur primitives: a trail following the effect ------------------------------------
// The definition: +0x10 the trail's position over time (3), +0x12 its half width (1), +0x14
// its texture scroll speed (1), +0x16 its colour (3), +0x18 its alpha (1), +0x1C the fade,
// +0x20 how often a point is added (0: every update), +0x24 its steps a segment, +0x25 its
// length, +0x26 bit 0 a point per key (the trail freed at the start), +0x27 its texture.
// The instance: +0x14.. those values' states, +0x1E bit 1 running, +0x2C the point timer,
// +0x30 its texture offset, +0x34 its trail (-1: none).

// 0x66D1F0 (thiscall)
uint8_t* __fastcall MotionBlurDefinition_Ctor(uint8_t* self, re::Edx) {
    PrimDefinition_Construct(self);
    ValueOverTime_Construct(self + 0x10);
    ValueOverTime_Construct(self + 0x12);
    ValueOverTime_Construct(self + 0x14);
    ValueOverTime_Construct(self + 0x16);
    ValueOverTime_Construct(self + 0x18);
    self[0x26] &= 0xFE;
    At<float>(self, 0x1C) = 1.0f;
    At<float>(self, 0x20) = 0.03f;
    self[0x24] = 0;
    self[0x25] = 5;
    TextureRef_Construct(self + 0x27);
    ValueOverTime_SetComponents(self + 0x10, 3);
    ValueOverTime_SetComponents(self + 0x12, 1);
    ValueOverTime_SetComponents(self + 0x14, 1);
    ValueOverTime_SetComponents(self + 0x16, 3);
    ValueOverTime_SetComponents(self + 0x18, 1);
    return self;
}

// 0x66D290 (thiscall): the instance's destructor (its members' are empty)
void __fastcall MotionBlur_Dtor(uint8_t* self, re::Edx) {
    NameKey_Destruct(self + 0x2C);
    NameKey_Destruct(self + 0x1C);
    NameKey_Destruct(self + 0x1A);
    NameKey_Destruct(self + 0x18);
    NameKey_Destruct(self + 0x16);
    NameKey_Destruct(self + 0x14);
    PrimInstance_Destruct(self);
}

// 0x66D2D0 (thiscall): started from its definition (a random texture offset when it
// scrolls)
uint8_t __fastcall MotionBlur_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<float>(self, 0x30) = 0.0f;
    At<uint8_t*>(self, 8) = definition;
    self[0x1E] = uint8_t((self[0x1E] & 0xFE) | 2);
    At<int32_t>(self, 0x34) = -1;
    uint8_t made = 0;
    if (ValueOverTimeInstance_Init(self + 0x14, definition + 0x10) &&
        ValueOverTimeInstance_Init(self + 0x16, At<uint8_t*>(self, 8) + 0x12) &&
        ValueOverTimeInstance_Init(self + 0x18, At<uint8_t*>(self, 8) + 0x14) &&
        ValueOverTimeInstance_Init(self + 0x1A, At<uint8_t*>(self, 8) + 0x16) &&
        ValueOverTimeInstance_Init(self + 0x1C, At<uint8_t*>(self, 8) + 0x18)) {
        made = 1;
    }
    PrimInstance_Restart(self);
    float speed;
    ValueOverTimeInstance_Get(self + 0x18, &speed, 0.0f);
    if (!(0.0f == speed)) {
        float r = Random_GetFloat(random_CF356C);
        At<float>(self, 0x30) = 0.0f + (1.0f - 0.0f) * r;
    }
    Timer_Reset(self + 0x2C);
    return made;
}

// 0x66D3B0 (thiscall): stopped (its trail freed)
void __fastcall MotionBlur_Stop(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x34) >= 0) {
        Trails_FreeFn(trails_1B850E0, At<int32_t>(self, 0x34));
        At<int32_t>(self, 0x34) = -1;
    }
    if (!(self[0x10] & 1)) {
        PrimInstance_Kill(self);
    }
    ValueOverTimeInstance_Stop(self + 0x14);
    ValueOverTimeInstance_Stop(self + 0x16);
    ValueOverTimeInstance_Stop(self + 0x18);
    ValueOverTimeInstance_Stop(self + 0x1A);
    ValueOverTimeInstance_Stop(self + 0x1C);
    self[0x1E] = uint8_t((self[0x1E] & 0xFE) | 2);
}

// 0x66D410 (thiscall): MotionBlurPrimitiveDefinitionLoader's vf1 (its texture loaded by
// its lower-case name when not yet there)
void __fastcall MotionBlurPrimitiveDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t base[8];
    DefinitionBaseLoader_Construct(base, definition);
    At<uint32_t>(base, 0) = kVtEffectPrimitiveDefinitionLoader;
    EffectPrimitiveDefinitionLoader_Read(base, archive);
    for (int at = 0x10; at <= 0x18; at += 2) {
        uint8_t loader[8];
        ValueOverTimeDefinitionLoader_Construct(loader, definition + at);
        Archive_ReadSection(archive, loader, name_93D56C);
    }
    vc::Call<void>(archive, 0x28, definition + 0x1C, name_93D578);
    vc::Call<void>(archive, 0x28, definition + 0x20, name_93D578);
    int32_t steps = 0;
    vc::Call<void>(archive, 0x38, &steps, countName_93D640);
    definition[0x24] = uint8_t(steps);
    int32_t length = 0;
    vc::Call<void>(archive, 0x38, &length, countName_93D640);
    definition[0x25] = uint8_t(length);
    uint8_t perKey = 0;
    vc::Call<void>(archive, 0x3C, &perKey, name_93D644);
    definition[0x26] ^= uint8_t(definition[0x26] ^ perKey) & 1;
    uint8_t name[0x2C];
    ArchiveString_Init(name);
    vc::Call<void>(archive, 0x24, static_cast<void*>(name), 0x21, textureName_93D570);
    ArchiveString_ToLower(name);
    uint32_t hash = HashName(At<char*>(name, 0));
    if (!EffectTextures_Has(trailTextures_B6F1F0, hash)) {
        EffectTextures_Add(trailTextures_B6F1F0, String_Get_773DB0(trailTextures_B6F1F0), At<char*>(name, 0));
    }
    TextureRef_Set(definition + 0x27, hash);
}

// 0x66D5F0 (thiscall)
uint8_t* __fastcall MotionBlur_Ctor(uint8_t* self, re::Edx) {
    PrimInstance_Construct(self);
    ValueOverTimeInstance_Construct(self + 0x14);
    ValueOverTimeInstance_Construct(self + 0x16);
    ValueOverTimeInstance_Construct(self + 0x18);
    ValueOverTimeInstance_Construct(self + 0x1A);
    ValueOverTimeInstance_Construct(self + 0x1C);
    self[0x1E] = uint8_t((self[0x1E] & 0xFE) | 2);
    Timer_Init(self + 0x2C);
    At<float>(self, 0x30) = 0.0f;
    At<int32_t>(self, 0x34) = -1;
    return self;
}

// 0x66D650 (thiscall): its trail's +0x32C: 1, or with no detail left fading from the
// effect's low detail distance to its fade distance
void __fastcall MotionBlur_UpdateFade(uint8_t* self, re::Edx, uint8_t*) {
    float fade = 1.0f;
    uint8_t* effect = At<uint8_t*>(self, 0xC);
    if (0.0f == At<float>(effect, 0x4C)) {
        const float* camera = re::PtrCast<const float*>(particleRenderer_B6F1EC);
        float d[3] = {camera[0] - At<float>(effect, 0x2C), camera[1] - At<float>(effect, 0x30),
                      camera[2] - At<float>(effect, 0x34)};
        float dist2 = d[1] * d[1] + d[0] * d[0];
        dist2 = dist2 + d[2] * d[2];
        float dist = crt::Sqrtf(dist2);   // sqrt
        uint8_t* def = At<uint8_t*>(At<uint8_t*>(self, 0xC), 0x50);
        float low = At<float>(def, 8);
        float far = At<float>(def, 0x10);
        if (low < dist || low == dist) {
            if (far < dist || far == dist) {
                fade = 0.0f;
            } else {
                float into = (dist - low) / (far - low);
                float left = 1.0f - into;
                fade = left + 0.0f;
            }
        }
    }
    Trails_Set32CFn(trails_1B850E0, At<int32_t>(self, 0x34), fade);
}

// 0x66D750 (thiscall): a point added to its trail from a position in the effect's space:
// as a pair with the fixed point +0x20 (when +0x1E bit 0), else across it `half` wide
void __fastcall MotionBlur_AddPoint(uint8_t* self, re::Edx, float x, float y, float z, float, float half) {
    float local[3] = {x, y, z};
    float p[3];
    NiTransform_Transform(At<uint8_t*>(self, 0xC) + 8, p, local);
    if (self[0x1E] & 1) {
        Trails_AddPointsFn(trails_1B850E0, At<int32_t>(self, 0x34), p[0], p[1], p[2], 0.0f, At<float>(self, 0x20),
                           At<float>(self, 0x24), At<float>(self, 0x28), 0.0f);
        return;
    }
    Trails_AddPositionFn(trails_1B850E0, At<int32_t>(self, 0x34), p[0], p[1], p[2], 0.0f, half);
}

// 0x66D820 (thiscall): its trail started (coloured, textured) and fed: a point per key of
// its position when so defined, else one every +0x20 seconds; its texture scrolled; the
// trail released once the effect has faded out
void __fastcall MotionBlur_Update(uint8_t* self, re::Edx, uint8_t* transform) {
    if ((At<uint16_t>(At<uint8_t*>(self, 0xC), 0x54) >> 9) & 1) {
        if (At<int32_t>(self, 0x34) >= 0) {
            Trails_ReleaseFn(trails_1B850E0, At<int32_t>(self, 0x34));
            At<int32_t>(self, 0x34) = -1;
        }
        return;
    }
    if (!(self[0x1E] & 2)) {
        return;
    }
    if ((At<uint8_t*>(self, 8)[0x26] & 1) && At<int32_t>(self, 0x34) >= 0) {
        Trails_FreeFn(trails_1B850E0, At<int32_t>(self, 0x34));
        At<int32_t>(self, 0x34) = -1;
    }
    float t = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    float alpha;
    ValueOverTimeInstance_Get(self + 0x1C, &alpha, t);
    float rgb[3];
    ValueOverTimeInstance_Get(self + 0x1A, rgb, t);
    uint8_t bytes[4];
    bytes[0] = uint8_t(int32_t(rgb[0] * 255.0f));
    bytes[1] = uint8_t(int32_t(rgb[1] * 255.0f));
    bytes[2] = uint8_t(int32_t(rgb[2] * 255.0f));
    bytes[3] = uint8_t(int32_t(255.0f * alpha));
    uint32_t colour = uint32_t(bytes[0]) | uint32_t(bytes[1]) << 8 | uint32_t(bytes[2]) << 16 | uint32_t(bytes[3]) << 24;
    if (At<int32_t>(self, 0x34) < 0) {
        uint8_t* def = At<uint8_t*>(self, 8);
        int32_t importance = (At<uint8_t>(At<uint8_t*>(At<uint8_t*>(self, 0xC), 0x50), 0x1C) >> 4) & 7;
        int32_t id = Trails_StartFn(trails_1B850E0, def[0x25], At<float>(def, 0x1C), colour, importance);
        At<int32_t>(self, 0x34) = id;
        if (id >= 0) {
            Trails_SetTextureFn(trails_1B850E0, id, At<uint8_t*>(self, 8)[0x27] >> 1);
            if (At<uint8_t*>(self, 8)[0x24] != 0) {
                Trails_Set314Fn(trails_1B850E0, At<int32_t>(self, 0x34), At<uint8_t*>(self, 8)[0x24]);
            }
        }
        if (At<int32_t>(self, 0x34) < 0) {
            return;
        }
    }
    MotionBlur_UpdateFadeFn(self, transform);
    Trails_Set20Fn(trails_1B850E0, At<int32_t>(self, 0x34), colour);
    float speed;
    ValueOverTimeInstance_Get(self + 0x18, &speed, t);
    Trails_Set310Fn(trails_1B850E0, At<int32_t>(self, 0x34), At<float>(self, 0x30));
    float offset = speed + At<float>(self, 0x30);
    At<float>(self, 0x30) = offset;
    if (1.0f < offset) {
        At<float>(self, 0x30) = offset - 1.0f;
    }
    uint8_t* def = At<uint8_t*>(self, 8);
    if (def[0x26] & 1) {
        uint8_t* positions = ValueOverTimeInstance_Definition(self + 0x14);
        if (!positions) {
            return;
        }
        for (int i = 0; i < ValueOverTime_NumKeysFn(positions); i++) {
            float position[3];
            float keyTime;
            ValueOverTime_GetKeyFn(positions, position, &keyTime, i);
            float half;
            uint8_t keys = ValueOverTime_NumKeysFn(ValueOverTimeInstance_Definition(self + 0x16));
            if (ValueOverTime_NumKeysFn(positions) == keys) {
                ValueOverTime_GetKeyFn(ValueOverTimeInstance_Definition(self + 0x16), &half, &keyTime, i);
            } else {
                ValueOverTimeInstance_Get(self + 0x16, &half, 0.0f);
            }
            MotionBlur_AddPointFn(self, position[0], position[1], position[2], 0.0f, half);
        }
        return;
    }
    float interval = At<float>(def, 0x20);
    if (0.0f < interval) {
        if (PrimTimer_Get(self + 0x2C) < interval) {
            return;
        }
        Timer_Reset(self + 0x2C);
    }
    float position[3];
    ValueOverTimeInstance_Get(self + 0x14, position, t);
    float half;
    ValueOverTimeInstance_Get(self + 0x16, &half, t);
    MotionBlur_AddPointFn(self, position[0], position[1], position[2], 0.0f, half);
}

// ---- sound primitives (their sound stripped: only a message is formatted) -------------------
// The definition: +0x10 the sound's name, +0x3C its bank, +0x78 how often it repeats (0:
// once; below 0: never), +0x7C. The instance: +0x14, +0x15 repeating, +0x16 never, +0x18
// the last phase, +0x1C its position.

// 0x66DBF0 (thiscall): PlaySoundPrimitiveDefinitionLoader's vf2, written (the base loader
// made but its fields not written)
void __fastcall PlaySoundPrimitiveDefinitionLoader_Write(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t base[8];
    DefinitionBaseLoader_Construct(base, definition);
    At<uint32_t>(base, 0) = kVtEffectPrimitiveDefinitionLoader;
    vc::Call<void>(archive, 0x4C, definition + 0x10, soundName_93D658);
    vc::Call<void>(archive, 0x4C, definition + 0x3C, soundBank_93D65C);
    vc::Call<void>(archive, 0x50, At<float>(definition, 0x78), name_93D578);
    vc::Call<void>(archive, 0x60, At<int32_t>(definition, 0x7C), countName_93D640);
}

// 0x66DC80 (thiscall): started
uint8_t __fastcall PlaySound_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint8_t*>(self, 8) = definition;
    PrimInstance_Restart(self);
    uint8_t* def = At<uint8_t*>(self, 8);
    self[0x14] = 0;
    self[0x15] = (0.0f < At<float>(def, 0x78)) ? 1 : 0;
    self[0x16] = (At<float>(def, 0x78) < 0.0f) ? 1 : 0;
    At<float>(self, 0x18) = 6666666.0f;
    return 1;
}

// 0x66DCF0 (thiscall): stopped
void __fastcall PlaySound_Stop(uint8_t* self, re::Edx) {
    if (!(self[0x10] & 1)) {
        PrimInstance_Kill(self);
    }
}

// 0x66DD00 (thiscall): PlaySoundPrimitiveDefinitionLoader's vf1
void __fastcall PlaySoundPrimitiveDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t base[8];
    DefinitionBaseLoader_Construct(base, definition);
    At<uint32_t>(base, 0) = kVtEffectPrimitiveDefinitionLoader;
    EffectPrimitiveDefinitionLoader_Read(base, archive);
    vc::Call<void>(archive, 0x24, definition + 0x10, 0x21, soundName_93D658);
    vc::Call<void>(archive, 0x24, definition + 0x3C, 0x33, soundBank_93D65C);
    vc::Call<void>(archive, 0x28, definition + 0x78, name_93D578);
    vc::Call<void>(archive, 0x38, definition + 0x7C, countName_93D640);
}

// 0x66DD90 (thiscall)
uint8_t* __fastcall PlaySound_Ctor(uint8_t* self, re::Edx) {
    PrimInstance_Construct(self);
    At<float>(self, 0x1C) = 0.0f;
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    return self;
}

// 0x66DDB0 (thiscall): its position followed; each time its phase wraps (or once) the
// "Playing" message formatted and dropped
void __fastcall PlaySound_Update(uint8_t* self, re::Edx, uint8_t* transform) {
    char message[0x400];
    At<uint32_t>(self, 0x1C) = At<uint32_t>(transform, 0x24);
    At<uint32_t>(self, 0x20) = At<uint32_t>(transform, 0x28);
    At<uint32_t>(self, 0x24) = At<uint32_t>(transform, 0x2C);
    uint8_t* def = At<uint8_t*>(self, 8);
    if (self[0x15]) {
        float t = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
        float span = At<float>(def, 0x78) - 0.0f;
        float q = (t - 0.0f) / span;
        float fraction = q - float(int(q));   // (_ftol)
        float into = span * fraction;
        float phase = 0.0f + into;
        if (At<float>(self, 0x18) > phase) {
            Sprintf(message, reinterpret_cast<const char*>(0x0093D774), At<const char*>(def, 0x3C),
                    At<const char*>(def, 0x10), At<int32_t>(def, 0x7C));
        }
        At<float>(self, 0x18) = phase;
        return;
    }
    if (!self[0x16] && !self[0x14]) {
        Sprintf(message, reinterpret_cast<const char*>(0x0093D774), At<const char*>(def, 0x3C),
                At<const char*>(def, 0x10), At<int32_t>(def, 0x7C));
    }
}

// 0x66DEB0 (thiscall): the definition's constructor
uint8_t* __fastcall PlaySoundDefinition_Ctor(uint8_t* self, re::Edx) {
    PrimDefinition_Construct(self);
    ArchiveString_Init(self + 0x10);
    ArchiveString33_Init(self + 0x3C);
    Strncpy(At<char*>(self, 0x10), reinterpret_cast<const char*>(0x008FEFBB), At<uint32_t>(self, 0x14));
    Strncpy(At<char*>(self, 0x3C), reinterpret_cast<const char*>(0x008FEFBB), At<uint32_t>(self, 0x40));
    At<float>(self, 0x78) = 0.0f;
    At<int32_t>(self, 0x7C) = 1;
    return self;
}

// ---- spawned-effect primitives: another effect placed along the effect ------------------------
// The definition: +0x10 its position over time (3), +0x12 its direction (3), +0x14 how
// long before it respawns at most (0: never), +0x18 the effect's name. The instance: +0x14,
// +0x16 their states, +0x18 frames until it's spawned, +0x1C its proxy (+0x20 the handle).

// 0x66DF10 (thiscall): SpawnEffectPrimitiveDefinitionLoader's vf2, written
void __fastcall SpawnEffectPrimitiveDefinitionLoader_Write(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t base[8];
    DefinitionBaseLoader_Construct(base, definition);
    At<uint32_t>(base, 0) = kVtEffectPrimitiveDefinitionLoader;
    uint8_t position[8];
    ValueOverTimeDefinitionLoader_Construct(position, definition + 0x10);
    Archive_WriteSection(archive, position, name_93D56C);
    uint8_t direction[8];
    ValueOverTimeDefinitionLoader_Construct(direction, definition + 0x12);
    Archive_WriteSection(archive, direction, name_93D56C);
    vc::Call<void>(archive, 0x50, At<float>(definition, 0x14), name_93D578);
    vc::Call<void>(archive, 0x4C, definition + 0x18, effectName_93D544);
}

// 0x66DFB0 (thiscall)
uint8_t* __fastcall SpawnEffect_Ctor(uint8_t* self, re::Edx) {
    PrimInstance_Construct(self);
    ValueOverTimeInstance_Construct(self + 0x14);
    ValueOverTimeInstance_Construct(self + 0x16);
    self[0x18] = 0;
    EffectProxySetLocation_Construct(self + 0x1C);
    At<uint32_t>(self, 0x1C) = kVtEffectSpawnProxy;
    return self;
}

// 0x66DFF0 (thiscall): its destructor
void __fastcall SpawnEffect_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0x1C) = kVtEffectSpawnProxy;
    EffectProxySetLocation_Destruct(self + 0x1C);
    NameKey_Destruct(self + 0x16);
    NameKey_Destruct(self + 0x14);
    PrimInstance_Destruct(self);
}

// 0x66E020 (thiscall): the respawn scheduled: a random time up to the definition's, in
// frames at 30 a second (1..255)
void __fastcall SpawnEffect_Schedule(uint8_t* self, re::Edx) {
    uint8_t* def = At<uint8_t*>(self, 8);
    if (0.0f == At<float>(def, 0x14)) {
        return;
    }
    float most = At<float>(def, 0x14);
    float r = Random_GetFloat(random_CF356C);
    float wait = 0.0f + (most - 0.0f) * r;
    float frames = wait / 0.033333335f;
    int32_t n = int32_t(frames);   // (_ftol)
    if (n <= 0) {
        self[0x18] = 1;
        return;
    }
    if (n > 0xFF) {
        n = 0xFF;
    }
    self[0x18] = uint8_t(n);
}

// 0x66E090 (thiscall): started (spawning next frame)
uint8_t __fastcall SpawnEffect_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint8_t*>(self, 8) = definition;
    uint8_t made = 0;
    if (ValueOverTimeInstance_Init(self + 0x14, definition + 0x10) &&
        ValueOverTimeInstance_Init(self + 0x16, At<uint8_t*>(self, 8) + 0x12)) {
        made = 1;
    }
    self[0x18] = 1;
    PrimInstance_Restart(self);
    return made;
}

// 0x66E0F0 (thiscall): stopped (the spawned effect killed over 0.1 s)
void __fastcall SpawnEffect_Stop(uint8_t* self, re::Edx) {
    if (!(self[0x10] & 1)) {
        PrimInstance_Kill(self);
    }
    ValueOverTimeInstance_Stop(self + 0x14);
    ValueOverTimeInstance_Stop(self + 0x16);
    if (At<uint32_t>(self, 0x20)) {
        EffectProxy_KillAfter(self + 0x1C, 0.1f);
    }
}

// 0x66E130 (thiscall): the spawn proxy's deleting destructor
uint8_t* __fastcall EffectSpawnProxy_DeletingDtor(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtEffectSpawnProxy;
    EffectProxySetLocation_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

// 0x66E160 (thiscall): SpawnEffectPrimitiveDefinitionLoader's vf1
void __fastcall SpawnEffectPrimitiveDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t base[8];
    DefinitionBaseLoader_Construct(base, definition);
    At<uint32_t>(base, 0) = kVtEffectPrimitiveDefinitionLoader;
    EffectPrimitiveDefinitionLoader_Read(base, archive);
    uint8_t position[8];
    ValueOverTimeDefinitionLoader_Construct(position, definition + 0x10);
    Archive_ReadSection(archive, position, name_93D56C);
    uint8_t direction[8];
    ValueOverTimeDefinitionLoader_Construct(direction, definition + 0x12);
    Archive_ReadSection(archive, direction, name_93D56C);
    vc::Call<void>(archive, 0x28, definition + 0x14, name_93D578);
    vc::Call<void>(archive, 0x24, definition + 0x18, 0x21, effectName_93D544);
}

// 0x66E200 (thiscall, on the spawn proxy): its effect placed at a position, facing a
// direction, within the parent transform
void __fastcall SpawnProxy_SetTransform(uint8_t* self, re::Edx, const float* position, const float* direction,
                                        const uint8_t* parent) {
    if (!At<uint32_t>(self, 4) || !At<uint8_t*>(EffectProxy_GetHandle(self), 4)) {
        return;
    }
    uint8_t* effect = At<uint8_t*>(EffectProxy_GetHandle(self), 4);
    uint8_t local[0x34];
    At<float>(local, 0x30) = 1.0f;
    Matrix3_MakeIdentity(local);
    Matrix_OrientTo(reinterpret_cast<float*>(local), direction);
    At<float>(local, 0x24) = position[0];
    At<float>(local, 0x28) = position[1];
    At<float>(local, 0x2C) = position[2];
    uint8_t combined[0x34];
    const uint8_t* result = NiTransform_Multiply(parent, combined, local);
    for (int i = 0; i < 0x34; i += 4) {
        At<uint32_t>(local, i) = *reinterpret_cast<const uint32_t*>(result + i);
    }
    for (int i = 0; i < 0x34; i += 4) {
        At<uint32_t>(effect, 8 + i) = At<uint32_t>(local, i);
    }
}

// 0x66E2A0 (thiscall): the definition's constructor
uint8_t* __fastcall SpawnEffectDefinition_Ctor(uint8_t* self, re::Edx) {
    PrimDefinition_Construct(self);
    ValueOverTime_Construct(self + 0x10);
    ValueOverTime_Construct(self + 0x12);
    At<float>(self, 0x14) = 0.0f;
    ArchiveString_Init(self + 0x18);
    ValueOverTime_SetComponents(self + 0x10, 3);
    ValueOverTime_SetComponents(self + 0x12, 3);
    Strncpy(At<char*>(self, 0x18), reinterpret_cast<const char*>(0x008FEFBB), At<uint32_t>(self, 0x1C));
    return self;
}

// 0x66E300 (thiscall): the spawned effect placed by the position and direction now
void __fastcall SpawnEffect_Place(uint8_t* self, re::Edx) {
    float t = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    float position[3];
    ValueOverTimeInstance_Get(self + 0x14, position, t);
    float p[3] = {position[0], position[1], position[2]};
    t = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    float direction[3];
    ValueOverTimeInstance_Get(self + 0x16, direction, t);
    float d[3] = {direction[0], direction[1], direction[2]};
    SpawnProxy_SetTransformFn(self + 0x1C, p, d, At<uint8_t*>(self, 0xC) + 8);
}

// 0x66E3B0 (thiscall): the spawned effect kept placed; else counted down to spawning it
// (by name, through the effect manager), and rescheduled when it's gone
void __fastcall SpawnEffect_Update(uint8_t* self, re::Edx, uint8_t*) {
    if (At<uint32_t>(self, 0x20)) {
        SpawnEffect_PlaceFn(self);
        return;
    }
    uint8_t frames = self[0x18];
    if (frames == 0) {
        SpawnEffect_ScheduleFn(self);
        return;
    }
    self[0x18] = uint8_t(frames - 1);
    if (uint8_t(frames - 1) != 0) {
        return;
    }
    uint8_t* def = At<uint8_t*>(self, 8);
    if (String_NotEqual_4479F0(def + 0x18, reinterpret_cast<const char*>(0x008FEFBB))) {
        int32_t index = EffectDefinitionManager_FindString(effectLibrary_B6F1E8, def + 0x18);
        EffectManager_Start(effectCounter_B6F1D4, &index, self + 0x1C, 0);
    }
    if (At<uint32_t>(self, 0x20)) {
        SpawnEffect_PlaceFn(self);
        return;
    }
    if (0.0f == At<float>(At<uint8_t*>(self, 8), 0x14)) {
        return;
    }
    self[0x18] = 0;
    SpawnEffect_ScheduleFn(self);
}

//@@NEXT@@
} // namespace

#define EFFPRIM34_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

EFFPRIM34_REVERSED("VirtualList::Create_66A040", 0x0066A040, &VirtualList_Create<4, 8>);
EFFPRIM34_REVERSED("VirtualList::Create_66A100", 0x0066A100, &VirtualList_Create<8, 0xC>);
EFFPRIM34_REVERSED("PrimInstance::Stop_66A1C0", 0x0066A1C0, &PrimInstance_StopImpl);
EFFPRIM34_REVERSED("PrimDefList::Release_66A230", 0x0066A230, &PrimDefList_Release);
EFFPRIM34_REVERSED("VirtualList::Release_66A270", 0x0066A270, &VirtualList_Release<8>);
EFFPRIM34_REVERSED("VirtualList::Release_66A2B0", 0x0066A2B0, &VirtualList_Release<0xC>);
EFFPRIM34_REVERSED("PrimInstanceList::Release_66A2F0", 0x0066A2F0, &PrimInstanceList_Release);
EFFPRIM34_REVERSED("EmitterList::Release_66A330", 0x0066A330, &EmitterList_Release);
EFFPRIM34_REVERSED("DefinitionType::Kind_66A370", 0x0066A370, &DefinitionKindImpl);
EFFPRIM34_REVERSED("PrimInstanceList::Create_66A380", 0x0066A380, &PrimInstanceList_Create);
EFFPRIM34_REVERSED("Effects::RandomDirection_66A500", 0x0066A500, &RandomDirection);
EFFPRIM34_REVERSED("HeatHazeDefinition::Construct_66A5B0", 0x0066A5B0, &HeatHazeDefinition_Ctor);
EFFPRIM34_REVERSED("HeatHazePrimitiveDefinitionLoader::Read_66A5E0", 0x0066A5E0, &HeatHazePrimitiveDefinitionLoader_Read);
EFFPRIM34_REVERSED("HeatHazePrimitive::Construct_66A670", 0x0066A670, &HeatHaze_Ctor);
EFFPRIM34_REVERSED("HeatHazePrimitive::Destruct_66A690", 0x0066A690, &HeatHaze_Dtor);
EFFPRIM34_REVERSED("HeatHazePrimitive::Init_66A6B0", 0x0066A6B0, &HeatHaze_Init);
EFFPRIM34_REVERSED("HeatHazePrimitive::Stop_66A6F0", 0x0066A6F0, &HeatHaze_Stop);
EFFPRIM34_REVERSED("HeatHazePrimitive::Update_66A720", 0x0066A720, &HeatHaze_Update);
EFFPRIM34_REVERSED("ValueOverTime::Construct_66A8A0", 0x0066A8A0, &ValueOverTime_Construct66A8A0);
EFFPRIM34_REVERSED("ValueOverTime::NumKeys_66A8D0", 0x0066A8D0, &ValueOverTime_NumKeys);
EFFPRIM34_REVERSED("ValueOverTime::AddKey_66A8E0", 0x0066A8E0, &ValueOverTime_AddKey);
EFFPRIM34_REVERSED("ValueOverTime::GetKey_66AA00", 0x0066AA00, &ValueOverTime_GetKey);
EFFPRIM34_REVERSED("ValueOverTimeInstance::Construct_66AA60", 0x0066AA60, &ValueOverTimeInstance_Ctor);
EFFPRIM34_REVERSED("ValueOverTimeInstance::InitBasic_66AA80", 0x0066AA80, &ValueOverTimeInstance_InitBasic);
EFFPRIM34_REVERSED("ValueOverTimeInstance::LoopTime_66AAA0", 0x0066AAA0, &ValueOverTimeInstance_LoopTime);
EFFPRIM34_REVERSED("ValueOverTimeInstance::Zero_66ABA0", 0x0066ABA0, &ValueOverTimeInstance_Zero);
EFFPRIM34_REVERSED("ValueOverTimeInstance::KeyValues_66ABF0", 0x0066ABF0, &ValueOverTimeInstance_KeyValues);
EFFPRIM34_REVERSED("ValueOverTimeInstance::Interpolate_66ACC0", 0x0066ACC0, &ValueOverTimeInstance_Interpolate);
EFFPRIM34_REVERSED("ValueOverTimeInstance::ComputeSlope_66AF80", 0x0066AF80, &ValueOverTimeInstance_ComputeSlope);
EFFPRIM34_REVERSED("ValueOverTimeInstance::UpdateConstant_66AFE0", 0x0066AFE0, &ValueOverTimeInstance_UpdateConstant);
EFFPRIM34_REVERSED("InterpolatorDefinitionLoader::Read_66B020", 0x0066B020, &InterpolatorDefinitionLoader_Read);
EFFPRIM34_REVERSED("ValueOverTimeInstance::Evaluate_66B250", 0x0066B250, &ValueOverTimeInstance_Evaluate);
EFFPRIM34_REVERSED("ValueOverTimeInstance::Get1_66B4F0", 0x0066B4F0, &ValueOverTimeInstance_Get1);
EFFPRIM34_REVERSED("InterpolatorElementStore::Destruct_66B560", 0x0066B560, &ValueStore_Dtor);
EFFPRIM34_REVERSED("InterpolatorElementStore::Allocate_66B570", 0x0066B570, &ValueStore_AllocateImpl);
EFFPRIM34_REVERSED("InterpolatorElementStore::AddTime_66B5E0", 0x0066B5E0, &ValueStore_AddTimeImpl);
EFFPRIM34_REVERSED("StoreLoader::Read_66B5F0", 0x0066B5F0, &StoreLoader_Read);
EFFPRIM34_REVERSED("StoreLoader::Write_66B6A0", 0x0066B6A0, &StoreLoader_Write);
EFFPRIM34_REVERSED("InterpolatorElementStore::Load_66B750", 0x0066B750, &ValueStore_Load);
EFFPRIM34_REVERSED("InterpolatorElementStore::Construct_66B7B0", 0x0066B7B0, &ValueStore_Ctor);
EFFPRIM34_REVERSED("InterpolatorElementStore::DeletingDestruct_66B7E0", 0x0066B7E0, &ValueStore_DeletingDtor);
EFFPRIM34_REVERSED("InterpolatorElementStore::FreeArrays_66B800", 0x0066B800, &ValueStore_FreeArrays);
EFFPRIM34_REVERSED("LightDefinition::Construct_66B840", 0x0066B840, &LightDefinition_Ctor);
EFFPRIM34_REVERSED("LightPrimitiveDefinitionLoader::Read_66B870", 0x0066B870, &LightPrimitiveDefinitionLoader_Read);
EFFPRIM34_REVERSED("LightPrimitive::Construct_66B910", 0x0066B910, &Light_Ctor);
EFFPRIM34_REVERSED("LightPrimitive::Update_66B940", 0x0066B940, &Light_Update);
EFFPRIM34_REVERSED("LightPrimitive::Init_66B9E0", 0x0066B9E0, &Light_Init);
EFFPRIM34_REVERSED("LightPrimitive::Stop_66BA20", 0x0066BA20, &Light_Stop);
EFFPRIM34_REVERSED("LightPrimitive::AddPointLight_66BA50", 0x0066BA50, &Light_AddPointLight);
EFFPRIM34_REVERSED("Trails::Start_66BB00", 0x0066BB00, &Trails_Start);
EFFPRIM34_REVERSED("Trails::Find_66BD10", 0x0066BD10, &Trails_FindImpl);
EFFPRIM34_REVERSED("Trails::SetOrigin_66BD80", 0x0066BD80, &Trails_SetOrigin);
EFFPRIM34_REVERSED("Trails::Set32C_66BDC0", 0x0066BDC0, &Trails_Set32C);
EFFPRIM34_REVERSED("Trails::Fade_66BDF0", 0x0066BDF0, &Trails_FadeImpl);
EFFPRIM34_REVERSED("Trails::Set310_66BEB0", 0x0066BEB0, &Trails_Set310);
EFFPRIM34_REVERSED("Trails::Set20_66BED0", 0x0066BED0, &Trails_Set20);
EFFPRIM34_REVERSED("Trails::SetTexture_66BEF0", 0x0066BEF0, &Trails_SetTexture);
EFFPRIM34_REVERSED("Trails::Set314_66BF20", 0x0066BF20, &Trails_Set314);
EFFPRIM34_REVERSED("Trails::Release_66BF40", 0x0066BF40, &Trails_Release);
EFFPRIM34_REVERSED("Trails::Free_66BF70", 0x0066BF70, &Trails_Free);
EFFPRIM34_REVERSED("Trails::AddPoints_66BF90", 0x0066BF90, &Trails_AddPoints);
EFFPRIM34_REVERSED("Trails::Update_66C0F0", 0x0066C0F0, &Trails_Update);
EFFPRIM34_REVERSED("Trails::Construct_66C9F0", 0x0066C9F0, &Trails_Ctor);
EFFPRIM34_REVERSED("Trails::BuildVertices_66C150", 0x0066C150, &Trails_BuildVertices);
EFFPRIM34_REVERSED("Trails::Render_66CA90", 0x0066CA90, &Trails_RenderImpl);
EFFPRIM34_REVERSED("Trails::AddPosition_66CD10", 0x0066CD10, &Trails_AddPosition);
EFFPRIM34_REVERSED("MotionBlurDefinition::Construct_66D1F0", 0x0066D1F0, &MotionBlurDefinition_Ctor);
EFFPRIM34_REVERSED("MotionBlurPrimitive::Destruct_66D290", 0x0066D290, &MotionBlur_Dtor);
EFFPRIM34_REVERSED("MotionBlurPrimitive::Init_66D2D0", 0x0066D2D0, &MotionBlur_Init);
EFFPRIM34_REVERSED("MotionBlurPrimitive::Stop_66D3B0", 0x0066D3B0, &MotionBlur_Stop);
EFFPRIM34_REVERSED("MotionBlurPrimitiveDefinitionLoader::Read_66D410", 0x0066D410, &MotionBlurPrimitiveDefinitionLoader_Read);
EFFPRIM34_REVERSED("MotionBlurPrimitive::Construct_66D5F0", 0x0066D5F0, &MotionBlur_Ctor);
EFFPRIM34_REVERSED("MotionBlurPrimitive::UpdateFade_66D650", 0x0066D650, &MotionBlur_UpdateFade);
EFFPRIM34_REVERSED("MotionBlurPrimitive::AddPoint_66D750", 0x0066D750, &MotionBlur_AddPoint);
EFFPRIM34_REVERSED("MotionBlurPrimitive::Update_66D820", 0x0066D820, &MotionBlur_Update);
EFFPRIM34_REVERSED("PlaySoundPrimitiveDefinitionLoader::Write_66DBF0", 0x0066DBF0, &PlaySoundPrimitiveDefinitionLoader_Write);
EFFPRIM34_REVERSED("PlaySoundPrimitive::Init_66DC80", 0x0066DC80, &PlaySound_Init);
EFFPRIM34_REVERSED("PlaySoundPrimitive::Stop_66DCF0", 0x0066DCF0, &PlaySound_Stop);
EFFPRIM34_REVERSED("PlaySoundPrimitiveDefinitionLoader::Read_66DD00", 0x0066DD00, &PlaySoundPrimitiveDefinitionLoader_Read);
EFFPRIM34_REVERSED("PlaySoundPrimitive::Construct_66DD90", 0x0066DD90, &PlaySound_Ctor);
EFFPRIM34_REVERSED("PlaySoundPrimitive::Update_66DDB0", 0x0066DDB0, &PlaySound_Update);
EFFPRIM34_REVERSED("PlaySoundDefinition::Construct_66DEB0", 0x0066DEB0, &PlaySoundDefinition_Ctor);
EFFPRIM34_REVERSED("SpawnEffectPrimitiveDefinitionLoader::Write_66DF10", 0x0066DF10, &SpawnEffectPrimitiveDefinitionLoader_Write);
EFFPRIM34_REVERSED("SpawnEffectPrimitive::Construct_66DFB0", 0x0066DFB0, &SpawnEffect_Ctor);
EFFPRIM34_REVERSED("SpawnEffectPrimitive::Destruct_66DFF0", 0x0066DFF0, &SpawnEffect_Dtor);
EFFPRIM34_REVERSED("SpawnEffectPrimitive::Schedule_66E020", 0x0066E020, &SpawnEffect_Schedule);
EFFPRIM34_REVERSED("SpawnEffectPrimitive::Init_66E090", 0x0066E090, &SpawnEffect_Init);
EFFPRIM34_REVERSED("SpawnEffectPrimitive::Stop_66E0F0", 0x0066E0F0, &SpawnEffect_Stop);
EFFPRIM34_REVERSED("EffectSpawnProxy::DeletingDestruct_66E130", 0x0066E130, &EffectSpawnProxy_DeletingDtor);
EFFPRIM34_REVERSED("SpawnEffectPrimitiveDefinitionLoader::Read_66E160", 0x0066E160, &SpawnEffectPrimitiveDefinitionLoader_Read);
EFFPRIM34_REVERSED("EffectSpawnProxy::SetTransform_66E200", 0x0066E200, &SpawnProxy_SetTransform);
EFFPRIM34_REVERSED("SpawnEffectDefinition::Construct_66E2A0", 0x0066E2A0, &SpawnEffectDefinition_Ctor);
EFFPRIM34_REVERSED("SpawnEffectPrimitive::Place_66E300", 0x0066E300, &SpawnEffect_Place);
EFFPRIM34_REVERSED("SpawnEffectPrimitive::Update_66E3B0", 0x0066E3B0, &SpawnEffect_Update);
//@@REG@@
