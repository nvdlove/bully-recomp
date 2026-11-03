// The effect system's particle systems (0x66E440..0x6712C0): the value-over-time field
// (2 bytes) in the definitions and its state in the instances, the emitters (angle
// emitter, source points, source emitter), the particle system primitive (definition, its
// loader, the instance: spawning, updating its movement and render infos, the batches that
// expire), the particle pool, and the camera wrap (particles kept in a box around the
// camera, as rain and snow). Shares the shapes of EffectSystemCore.cpp /
// EffectPrimitives.cpp: an instance's +0 link, +4 timer, +8 definition, +0xC effect, +0x10
// flags (1 dead); a primitive definition's +0 type, +4 link, +8 start time.
//
// A value over time (in a definition, 2 bytes): bit 0 an interpolator (bits 3.. its
// definition's index), else constant values (bits 3.. the first one's place in the key
// value array at 0x1B838CC); bits 1..2 its number of components. Its state (in an
// instance, 2 bytes): bit 0 an interpolator instance (bits 1.. its index in pool 0; all
// ones: none), else a copy of the definition's value.
//
// A particle (0x1C, the pool at *0xB6F1E4): +0 position, +0xC velocity, +0x18 next.
// (A dead one sits at 7654321.)

#include <cmath>
#include <cstdint>

#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_VAR(uint8_t*, effectManager_B6F1D4, 0x00B6F1D4);
GAME_VAR(uint8_t*, valueStore_B6F1D8, 0x00B6F1D8);      // the key values' store
GAME_VAR(uint8_t*, definitionStore_B6F1DC, 0x00B6F1DC);
GAME_VAR(uint8_t*, effectPools_B6F1E0, 0x00B6F1E0);
GAME_VAR(uint8_t*, particlePool_B6F1E4, 0x00B6F1E4);
GAME_VAR(uint8_t*, effectTextures_B6F1F0, 0x00B6F1F0);
GAME_VAR(float*, keyValues_1B838CC, 0x01B838CC);
GAME_VAR(float, twoPi_B8DB88, 0x00B8DB88);
GAME_VAR(uint8_t, worldCoordinates_1B890B8, 0x01B890B8);   // the last stepped system's
GAME_VAR(uint32_t, noValue_93D7FC, 0x0093D7FC);            // (all ones)
inline void* const random_CF356C = reinterpret_cast<void*>(0x00CF356C);

// the archive's names
GAME_VAR(const char*, nameDefinition, 0x0093D56C);
GAME_VAR(const char*, nameUsingInterpolator, 0x0093D574);
GAME_VAR(const char*, nameIndexValues, 0x0093D64C);
GAME_VAR(const char*, nameFloatValue, 0x0093D578);
GAME_VAR(const char*, nameTarget, 0x0093D598);
GAME_VAR(const char*, nameMaxAngle, 0x0093D59C);
GAME_VAR(const char*, nameMinAngle, 0x0093D5A0);
GAME_VAR(const char*, nameVelocity, 0x0093D5A4);
GAME_VAR(const char*, nameVelocityInaccuracy, 0x0093D5A8);
GAME_VAR(const char*, namePositionInaccuracy, 0x0093D5AC);
GAME_VAR(const char*, namePositionCenterBias, 0x0093D638);
GAME_VAR(const char*, nameUseParticleSourceEmitter, 0x0093D634);
GAME_VAR(const char*, nameLowDetailParticleRateMultiply, 0x0093D610);
GAME_VAR(const char*, nameHightDetailSpawnCountDown, 0x0093D614);
GAME_VAR(const char*, nameLowDetailSpawnCountDownDifference, 0x0093D618);
GAME_VAR(const char*, nameUseWorldCoordinates, 0x0093D558);
GAME_VAR(const char*, namePseudoBillboard, 0x0093D55C);
GAME_VAR(const char*, nameFaceEffect, 0x0093D648);
GAME_VAR(const char*, nameBool, 0x0093D644);
GAME_VAR(const char*, nameInitialParticleCount, 0x0093D560);
GAME_VAR(const char*, nameExpirationTime, 0x0093D564);
GAME_VAR(const char*, nameTextureName, 0x0093D570);
GAME_VAR(const char*, nameBlendMode, 0x0093D5E0);
GAME_VAR(const char*, nameUpdatesAtStart, 0x0093D628);
GAME_VAR(const char*, nameDistanceSizeMultiply, 0x0093D63C);
GAME_VAR(const char*, nameIndexSource, 0x0093D62C);
GAME_VAR(const char*, nameUseVelocitis, 0x0093D630);

constexpr uint32_t kVtValueOverTimeDefinitionLoader = 0x0093D7B8;
constexpr uint32_t kVtInterpolatorDefinitionLoader = 0x0093D7A8;
constexpr uint32_t kVtAngleEmitterDefinitionLoader = 0x0093D7CC;
constexpr uint32_t kVtSourceEmitterDefinitionLoader = 0x0093D7DC;
constexpr uint32_t kVtEmitterDefinitionLoader = 0x0093D808;
constexpr uint32_t kVtEffectPrimitiveDefinitionLoader = 0x0093D728;
constexpr uint32_t kVtMovementInfo = 0x0093D7EC;

constexpr float kDeadZ = 7654321.0f;
constexpr double kThirtieth = 0.03333333507180214;   // (double)(1/30.f)
constexpr double kPi = 3.1415927410125732;           // (double)(float)pi

GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
GAME_FN(void, __thiscall, Empty_49A650, 0x0049A650, void* self);          // (does nothing)
GAME_FN(void, __thiscall, ValueStore_Read_8A20A0, 0x008A20A0, uint8_t* store, float value);   // (empty)
GAME_FN(uint8_t, __thiscall, ValueStore_IsPreset, 0x0066B5C0, uint8_t* store);
GAME_FN(uint32_t, __thiscall, ValueStore_ValueCount, 0x00856850, uint8_t* store);
GAME_FN(void, __thiscall, ValueStore_AddValue, 0x00668230, uint8_t* store);
GAME_FN(uint8_t, __thiscall, Archive_ReadSection, 0x005B0720, void* archive, void* reader, const char* name);
GAME_FN(uint8_t, __thiscall, Definition_GetKind, 0x00665FE0, const uint8_t* definition);
GAME_FN(void*, __thiscall, DefinitionBaseLoader_Construct, 0x00665FF0, void* self, void* definition);
GAME_FN(uint8_t*, __thiscall, DefinitionStore_Get, 0x00666150, uint8_t* self, int index, const uint8_t* type);
GAME_FN(uint32_t, __thiscall, DefinitionStore_IndexOf, 0x00666170, uint8_t* self, uint8_t* definition);
GAME_FN(uint8_t*, __thiscall, DefinitionStore_New, 0x00666B10, uint8_t* self, int type);
GAME_FN(int, __thiscall, EffectDefinition_NumPrimitives, 0x00666C30, uint8_t* self);
GAME_FN(uint8_t*, __thiscall, Effect_FindPrimitive, 0x00666EE0, uint8_t* self, int n);
GAME_FN(void, __cdecl, EffectDefinition_LoadPrimitives, 0x00667680, void* archive, void* list);
GAME_FN(void*, __thiscall, Effects_GetCamera, 0x00667760, void* self);
GAME_FN(void, __thiscall, List_Init, 0x00667DA0, void* list);
GAME_FN(void*, __thiscall, List_PopFront, 0x00667DB0, void* list);
GAME_FN(void, __thiscall, PrimDefinition_Construct, 0x00668970, uint8_t* self);
GAME_FN(void, __thiscall, EffectPrimitiveDefinitionLoader_Read, 0x00668990, void* self, void* archive);
GAME_FN(uint8_t*, __thiscall, PrimInstance_Construct, 0x006689E0, uint8_t* self);
GAME_FN(void, __thiscall, PrimInstance_Destruct, 0x00668A00, uint8_t* self);
GAME_FN(void, __thiscall, PrimInstance_Restart, 0x00668A20, uint8_t* self);
GAME_FN(void, __thiscall, PrimInstance_Kill, 0x00668A40, uint8_t* self);
GAME_FN(int, __thiscall, PrimInstance_Index, 0x00668AF0, uint8_t* self);
GAME_FN(void*, __thiscall, ListHead_Construct, 0x006693E0, void* self);
GAME_FN(void, __thiscall, ListHead_Destruct, 0x006693F0, void* self);
GAME_FN(uint8_t*, __thiscall, Effects_Alloc, 0x00669410, uint8_t* pools, int kind);
GAME_FN(uint8_t*, __thiscall, EffectPools_Get, 0x00669430, uint8_t* pools, int index, int kind);
GAME_FN(int, __thiscall, EffectPools_IndexOf, 0x00669720, uint8_t* pools, void* object, int kind);
GAME_FN(void, __thiscall, Effects_Free, 0x00669DC0, uint8_t* pools, void* object);
GAME_FN(void, __thiscall, ArchiveString_Init, 0x00669E20, void* string);
GAME_FN(void, __thiscall, ArchiveString_ToLower, 0x00669E60, void* string);
GAME_FN(void, __thiscall, Timer_Sub, 0x00669FE0, void* timer, float seconds);
GAME_FN(float, __thiscall, PrimTimer_Get, 0x0066A010, void* timer);
GAME_FN(uint8_t, __cdecl, VirtualList_Create_66A040, 0x0066A040, void* out, void* definitions);
GAME_FN(uint8_t, __cdecl, VirtualList_Create_66A100, 0x0066A100, void* out, void* definitions);
GAME_FN(void, __cdecl, VirtualList_Release_66A270, 0x0066A270, void* list);
GAME_FN(void, __cdecl, VirtualList_Release_66A2B0, 0x0066A2B0, void* list);
GAME_FN(void, __cdecl, BatchList_Release_66A330, 0x0066A330, void* list);
GAME_FN(uint8_t, __thiscall, ValueOverTimeInstance_InitBasic, 0x0066AA80, uint8_t* self, uint8_t* definition);
GAME_FN(void, __thiscall, ValueOverTimeInstance_Get1, 0x0066B4F0, uint8_t* self, float* out, float t);
GAME_FN(void, __thiscall, ValueOverTimeInstance_Construct, 0x0066BEA0, void* self);   // (empty)
GAME_FN(void, __thiscall, TextureRef_Construct, 0x005781C0, void* ref);
GAME_FN(uint8_t, __thiscall, EffectTextures_Has, 0x005781F0, void* self, uint32_t hash);
GAME_FN(uint8_t, __thiscall, EffectTextures_Add, 0x005782C0, void* self, const char* path, const char* name);
GAME_FN(void, __thiscall, TextureRef_Set, 0x005783F0, void* ref, uint32_t hash);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(const char*, __thiscall, String_Get_773DB0, 0x00773DB0, void* object);   // its +0
GAME_FN(uint8_t, __cdecl, Vector_NearlyEqual, 0x00578540, const float* a, const float* b, float tolerance);
GAME_FN(float, __thiscall, Vec_Normalise, 0x004294A0, float* v);
GAME_FN(float, __thiscall, Vector_Normalise_414E10, 0x00414E10, float* v);
GAME_FN(float*, __thiscall, NiTransform_Transform, 0x00450B10, const void* transform, float* out, const float* v);
GAME_FN(float*, __thiscall, NiMatrix3_MultiplyVector, 0x00768A70, const float* self, float* out, const float* v);
GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(void, __cdecl, NiTransform_Invert, 0x00522440, void* out, const void* in);
GAME_FN(void*, __cdecl, Camera_GetNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, Node_WorldTransform, 0x006C9A90, void* node);
GAME_FN(int, __thiscall, Wrap_GetMode, 0x008C3450, uint8_t* wrap);                // its +0x24
GAME_FN(void, __thiscall, Wrap_Set10, 0x0044A1C0, uint8_t* wrap, void* value);   // its +0x10

// ours, through the game's addresses
GAME_FN(void, __thiscall, ValueOverTime_AllocValuesFn, 0x0066E490, uint16_t* self);
GAME_FN(void, __thiscall, ValueOverTime_SetValuesFn, 0x0066E510, uint16_t* self, const float* values);
GAME_FN(uint8_t, __thiscall, ValueOverTimeState_InitFn, 0x0066E560, uint16_t* self, const uint16_t* definition);
GAME_FN(void, __thiscall, ValueOverTimeState_StopFn, 0x0066E620, uint16_t* self);
GAME_FN(void, __thiscall, ValueOverTimeState_GetRestFn, 0x0066E660, uint16_t* self, float* out);
GAME_FN(void, __thiscall, ValueOverTimeState_GetFn, 0x0066E6B0, uint16_t* self, float* out, float time);
GAME_FN(void, __thiscall, ValueOverTime_ConstructFn, 0x0066E460, void* self);
GAME_FN(void, __thiscall, ValueOverTime_SetComponentsFn, 0x0066E470, void* self, int n);
GAME_FN(void*, __thiscall, ValueOverTimeDefinitionLoader_ConstructFn, 0x0066E440, void* self, void* definition);
GAME_FN(uint8_t, __thiscall, AngleEmitter_InitFn, 0x0066EAD0, uint8_t* self, uint8_t* definition);
GAME_FN(void, __thiscall, AngleEmitter_StopFn, 0x0066EB60, uint8_t* self);
GAME_FN(void, __cdecl, RandomOnSphereFn, 0x0066EBA0, float* out, const float* diameter, const float* base);
GAME_FN(void, __thiscall, RandomInConeFn, 0x0066ECE0, void* self, float* out, const float* direction,
        const float* minAngle, const float* maxAngle);
GAME_FN(void, __thiscall, AngleEmitter_EmitFn, 0x0066EF80, uint8_t* self, float* particle, void* transform,
        int local, float time);
GAME_FN(void, __thiscall, EmitterDefinition_ConstructFn, 0x0066F1D0, void* self);
GAME_FN(void, __thiscall, Emitter_ConstructFn, 0x0066F280, void* self);
GAME_FN(uint8_t, __thiscall, Emitter_InitFn, 0x0066F290, re::Ptr32<uint8_t>* self, uint8_t* definition);
GAME_FN(void, __thiscall, Emitter_StopFn, 0x0066F350, re::Ptr32<uint8_t>* self);
GAME_FN(void, __thiscall, Emitter_SpreadAlongMoveFn, 0x0066F3C0, re::Ptr32<uint8_t>* self, float* particle, uint8_t* system);
GAME_FN(void, __thiscall, Emitter_EmitFn, 0x0066F4C0, re::Ptr32<uint8_t>* self, float* particle, uint8_t* system);
GAME_FN(void, __thiscall, ParticleSystem_UpdateInfosFn, 0x0066FA10, uint8_t* self, void* state);
GAME_FN(void, __thiscall, ParticleSystem_RemoveExpiredFn, 0x0066FAB0, uint8_t* self);
GAME_FN(int, __thiscall, ParticleSystem_SpawnCountFn, 0x0066FCA0, uint8_t* self);
GAME_FN(void, __thiscall, ParticleSystem_AddParticleFn, 0x0066FDA0, uint8_t* self, float* particle, void* unused);
GAME_FN(uint8_t, __thiscall, ParticleSystem_NewBatchFn, 0x0066FDE0, uint8_t* self);
GAME_FN(void, __thiscall, ParticleSystem_SpawnFn, 0x0066FE70, uint8_t* self, void* state);
GAME_FN(float, __thiscall, ParticleSystem_VelocityScaleFn, 0x006703C0, uint8_t* self);
GAME_FN(float*, __thiscall, ParticleList_FindDeadFn, 0x00670400, uint8_t* system);
GAME_FN(float*, __thiscall, ParticleSystem_RandomParticleFn, 0x006701A0, uint8_t* self);
GAME_FN(void, __thiscall, Particle_KillFn, 0x00670430, void* self, float* particle);
GAME_FN(void, __thiscall, ParticleSystem_UpdateFn, 0x00670730, uint8_t* self, void* state);
GAME_FN(void, __thiscall, ParticleSystem_StepFn, 0x006707E0, uint8_t* self, void* state);
GAME_FN(uint8_t, __thiscall, SourceEmitter_InitFn, 0x00670970, uint8_t* self, uint8_t* definition);
GAME_FN(uint8_t, __thiscall, SourceEmitter_EmitFn, 0x00670980, uint8_t* self, float* position, float* velocity,
        uint8_t* effect);
GAME_FN(float, __cdecl, Wrap_FoldFn, 0x006709F0, float a, float b, float c);
GAME_FN(void, __thiscall, Wrap_ClearFn, 0x00670A70, uint8_t* self);
GAME_FN(void, __thiscall, Wrap_CopyToMirror10Fn, 0x00670A90, uint8_t* self, float* particle);
GAME_FN(void, __thiscall, Wrap_CopyToMirror14Fn, 0x00670AE0, uint8_t* self, float* particle);
GAME_FN(void, __thiscall, Wrap_SetMode1Fn, 0x00670B30, uint8_t* self, void* system);
GAME_FN(void, __thiscall, Wrap_Set14Fn, 0x00670B50, uint8_t* self, void* system);
GAME_FN(void, __thiscall, Wrap_SetMode2Fn, 0x00670B60, uint8_t* self, void* system);
GAME_FN(void, __thiscall, Wrap_SetMode3Fn, 0x00670B80, uint8_t* self, void* system);
GAME_FN(void, __thiscall, Wrap_Set20Fn, 0x00670BA0, uint8_t* self, void* system);
GAME_FN(void, __thiscall, Wrap_Mode2ReleaseFn, 0x00670BB0, uint8_t* self, float* particle);
GAME_FN(void, __thiscall, Wrap_CameraTransformsFn, 0x00670C00, uint8_t* self, void* inverse, void* transform);
GAME_FN(void, __thiscall, Wrap_FoldParticleFn, 0x00670D10, uint8_t* self, float* particle);
GAME_FN(void, __thiscall, Wrap_ProcessFn, 0x00670F00, uint8_t* self, uint8_t* system);
GAME_FN(void, __thiscall, Wrap_Mode3ReleaseFn, 0x00671250, uint8_t* self, float* particle);
GAME_FN(void, __thiscall, SourcePoints_ClearFn, 0x006712B0, uint8_t* self);
GAME_FN(void, __thiscall, SourcePoints_RandomFn, 0x006712C0, uint8_t* self, float* out);
GAME_FN(void, __thiscall, SourceEmitter_StopFn, 0x005FD140, uint8_t* self);   // its +4 cleared
GAME_FN(void, __thiscall, Batch_StopFn, 0x006714A0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, Batch_InitFn, 0x00671540, uint8_t* self, void* render, void* movement, float time);
GAME_FN(void, __thiscall, Batch_UpdateFn, 0x00671440, uint8_t* self, float time, void* state);
GAME_FN(float*, __thiscall, Batch_ProcessFn, 0x006713F0, uint8_t* self, float* particle, float time);

inline void Copy3(float* dst, const float* src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}
inline float Random() {
    return Random_GetFloat(random_CF356C);
}
// a random value in [lo, hi): the game's r * (hi - lo) + lo, stored as a float
inline float RandomRange(float lo, float hi) {
    return Random() * (hi - lo) + lo;
}
// the particle pool: +0 the number in use, +4 1500 particles, +0xA414 the free list
inline float* Pool_Take() {
    uint8_t* pool = particlePool_B6F1E4;
    float* p = At<float*>(pool, 0xA414);
    if (p) {
        At<int32_t>(pool, 0) += 1;
        At<float*>(pool, 0xA414) = At<float*>(p, 0x18);
    }
    return p;
}
inline void Pool_Give(float* p) {
    uint8_t* pool = particlePool_B6F1E4;
    At<float*>(p, 0x18) = At<float*>(pool, 0xA414);
    At<int32_t>(pool, 0) -= 1;
    At<float*>(pool, 0xA414) = p;
}
// a particle appended to a system's list (+0x16 its count, +0x18 head, +0x1C tail)
inline void System_Append(uint8_t* system, float* p) {
    system[0x16] += 1;
    if (!At<float*>(system, 0x18)) {
        At<float*>(system, 0x18) = p;
    } else {
        At<float*>(At<float*>(system, 0x1C), 0x18) = p;
    }
    At<float*>(system, 0x1C) = p;
    At<float*>(p, 0x18) = nullptr;
}

// ---- the value over time ---------------------------------------------------------------------

// 0x66E440 (thiscall): its loader (the definition's field)
void* __fastcall ValueOverTimeDefinitionLoader_Ctor(uint32_t* self, re::Edx, void* definition) {
    self[0] = kVtValueOverTimeDefinitionLoader;
    self[1] = reinterpret_cast<uint32_t>(definition);
    return self;
}

// 0x66E460 (thiscall)
uint16_t* __fastcall ValueOverTime_Ctor(uint16_t* self, re::Edx) {
    *self = 0;
    return self;
}

// 0x66E470 (thiscall): its number of components (the store's empty 0x49A650 after)
void __fastcall ValueOverTime_SetComponents(uint16_t* self, re::Edx, int n) {
    uint32_t bits = (uint32_t(n) * 2) ^ *reinterpret_cast<uint8_t*>(self);
    *self ^= uint16_t(bits & 6);
    Empty_49A650(valueStore_B6F1D8);
}

inline int Components(const uint16_t* self) {
    return (*reinterpret_cast<const uint8_t*>(self) >> 1) & 3;
}

// 0x66E490 (thiscall): room for its constant values taken from the store (zeroed)
void __fastcall ValueOverTime_AllocValues(uint16_t* self, re::Edx) {
    uint32_t first = ValueStore_ValueCount(valueStore_B6F1D8);
    *self = uint16_t((*reinterpret_cast<uint8_t*>(self) & 7) | (first * 8));
    if (*reinterpret_cast<uint8_t*>(self) & 6) {
        int i = 0;
        do {
            ValueStore_AddValue(valueStore_B6F1D8);
            i++;
        } while (i < Components(self));
    }
    if (*reinterpret_cast<uint8_t*>(self) & 6) {
        int i = 0;
        do {
            keyValues_1B838CC[(*self >> 3) + i] = 0.0f;
            i++;
        } while (i < Components(self));
    }
}

// 0x66E510 (thiscall): its constant values set
void __fastcall ValueOverTime_SetValues(uint16_t* self, re::Edx, const float* values) {
    for (int i = 0; i < ((*self >> 1) & 3); i++) {
        keyValues_1B838CC[(*self >> 3) + i] = values[i];
    }
}

// 0x66E560 (thiscall): the state started from its definition (an interpolator's instance
// taken from pool 0)
uint8_t __fastcall ValueOverTimeState_Init(uint16_t* self, re::Edx, const uint16_t* definition) {
    if (!(*definition & 1)) {
        *self = uint16_t(*definition & 0xFFFE);
        return 1;
    }
    *self = 0xFFFF;
    uint8_t* instance = Effects_Alloc(effectPools_B6F1E0, 0);
    if (!instance) {
        return 0;
    }
    // (the type is read through a pointer to the argument's slot, its low byte set to 1)
    uint8_t type = 1;
    uint8_t* interpolator = DefinitionStore_Get(definitionStore_B6F1DC, *definition >> 3, &type);
    uint8_t ok = ValueOverTimeInstance_InitBasic(instance, interpolator);
    if (!ok) {
        Effects_Free(effectPools_B6F1E0, instance);
        return ok;
    }
    int index = EffectPools_IndexOf(effectPools_B6F1E0, instance, 0);
    *self = uint16_t((*reinterpret_cast<uint8_t*>(self) & 1) | (index * 2));
    return ok;
}

// 0x66E620 (thiscall): stopped (its interpolator instance given back)
void __fastcall ValueOverTimeState_Stop(uint16_t* self, re::Edx) {
    uint16_t v = *self;
    if (!(v & 1)) {
        return;
    }
    if ((v & 0xFFFE) != 0xFFFE) {
        Effects_Free(effectPools_B6F1E0, EffectPools_Get(effectPools_B6F1E0, v >> 1, 0));
    }
    *self |= 0xFFFE;
}

// 0x66E660 (thiscall): the constant components after the first
void __fastcall ValueOverTimeState_GetRest(uint16_t* self, re::Edx, float* out) {
    uint32_t v = *self;
    int i = 1;
    if (int8_t((v >> 1) & 3) <= 1) {
        return;
    }
    do {
        out[i] = keyValues_1B838CC[(v >> 3) + i];
        i++;
        v = *self;
    } while (i < int((v >> 1) & 3));
}

// 0x66E6B0 (thiscall): its components at a time (constants: all of them; an interpolator:
// its first)
void __fastcall ValueOverTimeState_Get(uint16_t* self, re::Edx, float* out, float time) {
    uint32_t v = *self;
    if (!(v & 1)) {
        out[0] = keyValues_1B838CC[v >> 3];
        if ((*reinterpret_cast<uint8_t*>(self) & 6) > 2) {
            ValueOverTimeState_GetRestFn(self, out);
        }
        return;
    }
    uint8_t* instance = EffectPools_Get(effectPools_B6F1E0, int(v >> 1), 0);
    ValueOverTimeInstance_Get1(instance, out, time);
}

// 0x66E710 (thiscall): ValueOverTimeDefinitionLoader's vf1: an interpolator (its definition
// made and read) or constant values (read into the store, or, with a preset store, the
// place of values already there)
void __fastcall ValueOverTimeDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint16_t* value = At<uint16_t*>(self, 4);
    uint8_t usingInterpolator = 0;
    vc::Call<uint8_t>(archive, 0x3C, &usingInterpolator, nameUsingInterpolator);
    *value = uint16_t((usingInterpolator & 1) | (*value & 0xFFFE));
    if (usingInterpolator) {
        uint8_t* definition = DefinitionStore_New(definitionStore_B6F1DC, 1);
        uint32_t index = DefinitionStore_IndexOf(definitionStore_B6F1DC, definition);
        uint32_t v = (*reinterpret_cast<uint8_t*>(value) & 7) | (index * 8);
        *value = uint16_t(v);
        definition[1] = uint8_t((((uint8_t(v) >> 1) & 3) << 3) | (definition[1] & 0xC7));
        uint32_t loader[2];
        DefinitionBaseLoader_Construct(loader, definition);
        loader[0] = kVtInterpolatorDefinitionLoader;
        Archive_ReadSection(archive, loader, nameDefinition);
        return;
    }
    int32_t index = -1;   // (in the archive argument's slot)
    vc::Call<uint8_t>(archive, 0x38, &index, nameIndexValues);
    float values[3];      // (only the components read are set)
    int i = 0;
    if (*reinterpret_cast<uint8_t*>(value) & 6) {
        do {
            vc::Call<uint8_t>(archive, 0x28, &values[i], nameFloatValue);
            ValueStore_Read_8A20A0(valueStore_B6F1D8, values[i]);
            ValueStore_IsPreset(valueStore_B6F1D8);
            i++;
        } while (i < Components(value));
    }
    Empty_49A650(valueStore_B6F1D8);
    if (ValueStore_IsPreset(valueStore_B6F1D8)) {
        *value = uint16_t((*reinterpret_cast<uint8_t*>(value) & 7) | (index * 8));
        return;
    }
    ValueOverTime_AllocValuesFn(value);
    ValueOverTime_SetValuesFn(value, values);
}

// 0x66E890 (thiscall): an interpolator state's definition (none: null)
uint8_t* __fastcall ValueOverTimeState_Definition(uint16_t* self, re::Edx) {
    uint32_t v = *self;
    if (!(v & 1)) {
        return nullptr;
    }
    return At<uint8_t*>(EffectPools_Get(effectPools_B6F1E0, int(v >> 1), 0), 4);
}

// ---- the angle emitter: a definition (0x10): +2 its target direction (3), +4 its max
// angle, +6 its min angle, +8 its velocity, +0xA the velocity's inaccuracy, +0xC the
// position's, +0xE bit 0 biased to the centre. Its instance (0x14): +4 the definition,
// +8.. those values' states ------------------------------------------------------------------

// 0x66E8B0 (thiscall)
uint8_t* __fastcall AngleEmitterDefinition_Ctor(uint8_t* self, re::Edx) {
    self[0] = 0xFF;
    for (int at = 2; at <= 0xC; at += 2) {
        ValueOverTime_ConstructFn(self + at);
    }
    self[0xE] |= 1;
    ValueOverTime_SetComponentsFn(self + 2, 3);
    for (int at = 4; at <= 0xC; at += 2) {
        ValueOverTime_SetComponentsFn(self + at, 1);
    }
    return self;
}

// 0x66E940 (thiscall): AngleEmitterDefinitionLoader's vf1
void __fastcall AngleEmitterDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint32_t loaders[6][2];
    for (int i = 0; i < 6; i++) {
        ValueOverTimeDefinitionLoader_ConstructFn(loaders[i], definition + 2 + i * 2);
    }
    Archive_ReadSection(archive, loaders[0], nameTarget);
    Archive_ReadSection(archive, loaders[1], nameMaxAngle);
    Archive_ReadSection(archive, loaders[2], nameMinAngle);
    Archive_ReadSection(archive, loaders[3], nameVelocity);
    Archive_ReadSection(archive, loaders[4], nameVelocityInaccuracy);
    Archive_ReadSection(archive, loaders[5], namePositionInaccuracy);
    uint8_t bias = 1;
    vc::Call<uint8_t>(archive, 0x3C, &bias, namePositionCenterBias);
    definition[0xE] ^= (definition[0xE] ^ bias) & 1;
}

// 0x66EA40 (thiscall)
uint8_t* __fastcall AngleEmitter_Ctor(uint8_t* self, re::Edx) {
    ListHead_Construct(self);
    At<uint32_t>(self, 4) = 0;
    for (int at = 8; at <= 0x12; at += 2) {
        ValueOverTimeInstance_Construct(self + at);
    }
    return self;
}

// 0x66EA90 (thiscall): its destructor
void __fastcall AngleEmitter_Dtor(uint8_t* self, re::Edx) {
    for (int at = 0x12; at >= 8; at -= 2) {
        Empty_49A650(self + at);
    }
    ListHead_Destruct(self);
}

// 0x66EAD0 (thiscall): started from its definition
uint8_t __fastcall AngleEmitter_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint8_t*>(self, 4) = definition;
    for (int i = 0; i < 6; i++) {
        if (!ValueOverTimeState_InitFn(reinterpret_cast<uint16_t*>(self + 8 + i * 2),
                                       reinterpret_cast<uint16_t*>(At<uint8_t*>(self, 4) + 2 + i * 2))) {
            return 0;
        }
    }
    return 1;
}

// 0x66EB60 (thiscall)
void __fastcall AngleEmitter_Stop(uint8_t* self, re::Edx) {
    for (int at = 8; at <= 0x12; at += 2) {
        ValueOverTimeState_StopFn(reinterpret_cast<uint16_t*>(self + at));
    }
}

// 0x66EBA0 (cdecl): a random point on a sphere `diameter` across around `base`
void __cdecl RandomOnSphere(float* out, const float* diameter, const float* base) {
    float r = Random();
    out[2] = (r + r) - 1.0f;
    float azimuth = RandomRange(0.0f, twoPi_B8DB88);
    float s = crt::Sqrtf(std::fabs(1.0f - out[2] * out[2]));   // (the CRT's x87 sqrt)
    float x = trk::CrtCos(azimuth) * s;
    out[0] = x;
    float y = trk::CrtSin(azimuth) * s;
    out[1] = y;
    float d = *diameter;
    float t0 = x * d;
    float t1 = y * d;
    float t2 = d * out[2];
    float tmp[3] = {t0 * 0.5f, t1 * 0.5f, 0.5f * t2};
    Copy3(out, tmp);
    Copy3(out, VectorAdd(tmp, out, base));
}

// 0x66ECE0 (thiscall, this unused): a random direction within a cone around `direction`
// (between the two angles from it, radians): out's 4th word is the game's uninitialised
// stack
void __fastcall RandomInCone(void*, re::Edx, float* out, const float* direction, const float* minAngle,
                             const float* maxAngle) {
    float d[3] = {direction[0], direction[1], direction[2]};
    float a[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    float b[3] = {0.0f, 0.0f, -1.0f};
    if (Vector_NearlyEqual(d, a, 0.1f) || Vector_NearlyEqual(d, b, 0.1f)) {
        a[0] = 1.0f;
        a[1] = 0.0f;
        a[2] = 0.0f;
    }
    // c = d x a, e = d x c: the cone's frame (e, c, d) as a transform's columns
    float c[3];
    c[0] = a[2] * d[1] - d[2] * a[1];
    c[1] = d[2] * a[0] - a[2] * d[0];
    c[2] = a[1] * d[0] - d[1] * a[0];
    float e[3];
    e[0] = c[2] * d[1] - c[1] * d[2];
    e[1] = d[2] * c[0] - c[2] * d[0];
    e[2] = c[1] * d[0] - d[1] * c[0];
    float transform[13] = {e[0], c[0], d[0], e[1], c[1], d[1], e[2], c[2], d[2], 0.0f, 0.0f, 0.0f, 1.0f};
    float azimuth = RandomRange(0.0f, twoPi_B8DB88);
    float lo = *minAngle;
    float hi = *maxAngle;
    float elevation = Random() * (hi - lo) + lo;
    float s = trk::CrtSin(elevation);
    float v[3];
    v[0] = trk::CrtCos(azimuth) * s;
    v[1] = trk::CrtSin(azimuth) * s;
    v[2] = trk::CrtCos(elevation);
    float tmp[3];
    float w[4];
    Copy3(w, NiTransform_Transform(transform, tmp, v));
    w[3] = 0.0f;   // (uninitialised in the game)
    Vec_Normalise(w);
    out[0] = w[0];
    out[1] = w[1];
    out[2] = w[2];
    out[3] = w[3];
}

// 0x66EF80 (thiscall): a particle placed and launched: its position moved to a random point
// (the inaccuracy, biased to the centre or its edge), its velocity along a random direction
// within the angles from the target direction (arguments 2 and 3 unused)
void __fastcall AngleEmitter_Emit(uint8_t* self, re::Edx, float* particle, void*, int, float time) {
    float inaccuracy;
    ValueOverTimeState_GetFn(reinterpret_cast<uint16_t*>(self + 0x12), &inaccuracy, time);
    float u = RandomRange(0.0f, 1.0f);
    float k = (At<uint8_t*>(self, 4)[0xE] & 1) ? u : 1.0f - u * u;
    float diameter = k * inaccuracy;
    RandomOnSphereFn(particle, &diameter, particle);
    float target[3];
    ValueOverTimeState_GetFn(reinterpret_cast<uint16_t*>(self + 8), target, time);
    float maxAngle;
    ValueOverTimeState_GetFn(reinterpret_cast<uint16_t*>(self + 0xA), &maxAngle, time);
    float minAngle;
    ValueOverTimeState_GetFn(reinterpret_cast<uint16_t*>(self + 0xC), &minAngle, time);
    float lo = (maxAngle < minAngle || maxAngle == minAngle) ? maxAngle : minAngle;
    float velocity;
    ValueOverTimeState_GetFn(reinterpret_cast<uint16_t*>(self + 0xE), &velocity, time);
    velocity = float(double(velocity) * kThirtieth);
    float velocityInaccuracy;
    ValueOverTimeState_GetFn(reinterpret_cast<uint16_t*>(self + 0x10), &velocityInaccuracy, time);
    velocityInaccuracy = float(double(velocityInaccuracy) * kThirtieth);
    float direction[4] = {target[0], target[1], target[2], 0.0f};
    Vec_Normalise(direction);
    float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (Vector_NearlyEqual(zero, direction, 0.1f)) {
        direction[0] = 1.0f;
        direction[1] = 0.0f;
        direction[2] = 0.0f;
        direction[3] = zero[3];
    }
    float hi = float(double(maxAngle) * kPi) / 180.0f;
    float loRadians = float(kPi * double(lo)) / 180.0f;
    float cone[4];
    RandomInConeFn(self, cone, direction, &loRadians, &hi);
    float speed = RandomRange(0.0f, velocityInaccuracy);
    speed = speed + velocity;
    particle[3] = cone[0] * speed;
    particle[4] = cone[1] * speed;
    particle[5] = speed * cone[2];
}

// ---- the emitter: a definition (0xC): +4 its angle emitter's definition, +8 its source
// emitter's. Its instance (0xC): +0 the angle emitter, +4 source points, +8 the source
// emitter ---------------------------------------------------------------------------------

// 0x66F1D0 (thiscall)
uint8_t* __fastcall EmitterDefinition_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    self[0] = 3;
    return self;
}

// 0x66F1E0 (thiscall): EmitterDefinitionLoader's vf1
void __fastcall EmitterDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint8_t* angle = DefinitionStore_New(definitionStore_B6F1DC, 4);
    At<uint8_t*>(definition, 4) = angle;
    uint32_t loader[2];
    DefinitionBaseLoader_Construct(loader, angle);
    loader[0] = kVtAngleEmitterDefinitionLoader;
    Archive_ReadSection(archive, loader, nameDefinition);
    uint8_t useSource = 0;
    vc::Call<uint8_t>(archive, 0x3C, &useSource, nameUseParticleSourceEmitter);
    if (!useSource) {
        return;
    }
    uint8_t* source = DefinitionStore_New(definitionStore_B6F1DC, 0xE);
    At<uint8_t*>(definition, 8) = source;
    uint32_t sourceLoader[2];
    DefinitionBaseLoader_Construct(sourceLoader, source);
    sourceLoader[0] = kVtSourceEmitterDefinitionLoader;
    Archive_ReadSection(archive, sourceLoader, nameDefinition);
}

// 0x66F280 (thiscall)
uint8_t* __fastcall Emitter_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = 0;
    At<uint32_t>(self, 4) = 0;
    At<uint32_t>(self, 8) = 0;
    return self;
}

// 0x66F290 (thiscall): its parts made from the definition's; one failing: both given back
uint8_t __fastcall Emitter_Init(re::Ptr32<uint8_t>* self, re::Edx, uint8_t* definition) {
    uint8_t ok = 1;
    if (At<uint8_t*>(definition, 4)) {
        self[0] = Effects_Alloc(effectPools_B6F1E0, Definition_GetKind(At<uint8_t*>(definition, 4)));
        if (self[0]) {
            ok = AngleEmitter_InitFn(self[0], At<uint8_t*>(definition, 4)) != 0;
        } else {
            ok = 0;
        }
    }
    if (At<uint8_t*>(definition, 8)) {
        self[2] = Effects_Alloc(effectPools_B6F1E0, Definition_GetKind(At<uint8_t*>(definition, 8)));
        if (self[2] && ok && SourceEmitter_InitFn(self[2], At<uint8_t*>(definition, 8))) {
            return 1;
        }
        ok = 0;
    } else if (ok) {
        return ok;
    }
    if (self[0]) {
        Effects_Free(effectPools_B6F1E0, self[0]);
        self[0] = nullptr;
    }
    if (self[2]) {
        Effects_Free(effectPools_B6F1E0, self[2]);
        self[2] = nullptr;
    }
    return ok;
}

// 0x66F350 (thiscall): its parts stopped and given back
void __fastcall Emitter_Stop(re::Ptr32<uint8_t>* self, re::Edx) {
    if (self[0]) {
        AngleEmitter_StopFn(self[0]);
        Effects_Free(effectPools_B6F1E0, self[0]);
        self[0] = nullptr;
    }
    if (self[2]) {
        SourceEmitter_StopFn(self[2]);
        Effects_Free(effectPools_B6F1E0, self[2]);
        self[2] = nullptr;
    }
    if (self[1]) {
        SourcePoints_ClearFn(self[1]);
        Effects_Free(effectPools_B6F1E0, self[1]);
        self[1] = nullptr;
    }
}

// 0x66F3C0 (thiscall): with the effect's flag 0x800 (+0x54), a particle moved to a random
// point along the effect's move this frame (its last position +0x2C, the new one +0x3C)
void __fastcall Emitter_SpreadAlongMove(re::Ptr32<uint8_t>*, re::Edx, float* particle, uint8_t* system) {
    uint8_t* effect = At<uint8_t*>(system, 0xC);
    if (!((At<uint16_t>(effect, 0x54) >> 0xB) & 1)) {
        return;
    }
    float u = RandomRange(0.0f, 1.0f);
    float a[3] = {At<float>(effect, 0x3C) * u, At<float>(effect, 0x40) * u, At<float>(effect, 0x44) * u};
    float w = 1.0f - u;
    float b[3] = {At<float>(effect, 0x2C) * w, At<float>(effect, 0x30) * w, w * At<float>(effect, 0x34)};
    float c[3] = {b[0] + a[0], b[1] + a[1], b[2] + a[2]};
    float d[3] = {c[0] - At<float>(effect, 0x2C), c[1] - At<float>(effect, 0x30), c[2] - At<float>(effect, 0x34)};
    particle[0] = particle[0] + d[0];
    particle[1] = particle[1] + d[1];
    particle[2] = particle[2] + d[2];
}

// 0x66F4C0 (thiscall): a new particle of a system placed and launched by its emitter's
// parts (in the effect's space, then moved into the world, for a system in world
// coordinates), its velocity scaled by the system's; a wrapped system's (modes 2 and 3)
// particles start dead
void __fastcall Emitter_Emit(re::Ptr32<uint8_t>* self, re::Edx, float* particle, uint8_t* system) {
    uint8_t local = At<uint8_t*>(system, 8)[0x1C] & 1;
    uint8_t* transform = At<uint8_t*>(system, 0xC) + 8;
    float time = PrimTimer_Get(system + 4) - At<float>(At<uint8_t*>(system, 8), 8);
    float* velocity = particle + 3;
    for (int i = 0; i < 6; i++) {
        particle[i] = 0.0f;
    }
    if (self[0]) {
        // (the flag goes as the argument's slot with its low byte replaced)
        AngleEmitter_EmitFn(self[0], particle, transform, local, time);
    }
    float tmp[3];
    if (local) {
        Copy3(particle, NiTransform_Transform(transform, tmp, particle));
        Copy3(velocity, NiTransform_Transform(transform, tmp, velocity));
        float zero[3] = {0.0f, 0.0f, 0.0f};
        float origin[3];
        Copy3(origin, NiTransform_Transform(transform, tmp, zero));
        Copy3(velocity, VectorSub(tmp, velocity, origin));
        if (self[1]) {
            float t[3] = {At<float>(transform, 0x24), At<float>(transform, 0x28), At<float>(transform, 0x2C)};
            particle[0] = particle[0] - t[0];
            particle[1] = particle[1] - t[1];
            particle[2] = particle[2] - t[2];
            SourcePoints_RandomFn(self[1], t);
            particle[0] = particle[0] + t[0];
            particle[1] = particle[1] + t[1];
            particle[2] = t[2] + particle[2];
        }
        if (self[2]) {
            float dv[3] = {0.0f, 0.0f, 0.0f};
            float position[3];
            if (SourceEmitter_EmitFn(self[2], position, dv, At<uint8_t*>(system, 0xC))) {
                float t[3] = {At<float>(transform, 0x24), At<float>(transform, 0x28), At<float>(transform, 0x2C)};
                float x = particle[0] - t[0];
                particle[0] = x + position[0];
                float y = particle[1] - t[1];
                particle[1] = y + position[1];
                float z = particle[2] - t[2];
                particle[2] = z + position[2];
                velocity[0] = velocity[0] + dv[0];
                velocity[1] = velocity[1] + dv[1];
                velocity[2] = velocity[2] + dv[2];
            }
        }
        if (At<uint8_t*>(system, 8)[0x1C] & 8) {
            Emitter_SpreadAlongMoveFn(self, particle, system);
        }
    } else {
        if (self[1]) {
            float t[3] = {0.0f, 0.0f, 0.0f};
            SourcePoints_RandomFn(self[1], t);
            particle[0] = particle[0] + t[0];
            particle[1] = particle[1] + t[1];
            particle[2] = t[2] + particle[2];
        }
        if (self[2]) {
            float dv[3] = {0.0f, 0.0f, 0.0f};
            float position[3];
            if (SourceEmitter_EmitFn(self[2], position, dv, At<uint8_t*>(system, 0xC))) {
                particle[0] = particle[0] + position[0];
                particle[1] = particle[1] + position[1];
                particle[2] = position[2] + particle[2];
                velocity[0] = velocity[0] + dv[0];
                velocity[1] = velocity[1] + dv[1];
                velocity[2] = velocity[2] + dv[2];
            }
        }
    }
    float scale = ParticleSystem_VelocityScaleFn(system);
    velocity[0] = velocity[0] * scale;
    velocity[1] = velocity[1] * scale;
    velocity[2] = velocity[2] * scale;
    uint8_t* wrap = At<uint8_t*>(system, 0x40);
    if (wrap && (Wrap_GetMode(wrap) == 2 || Wrap_GetMode(wrap) == 3)) {
        Particle_KillFn(system, particle);
    }
}

// ---- movement infos' base (a vtable and a list head) and a list-owning definition --------

// 0x66F840 (thiscall): a definition with a list (+4) and no type yet
uint8_t* __fastcall ListDefinition_Ctor(uint8_t* self, re::Edx) {
    self[0] = 0xFF;
    List_Init(self + 4);
    return self;
}

// 0x66F860 (thiscall)
uint8_t* __fastcall MovementInfo_Ctor(uint8_t* self, re::Edx) {
    ListHead_Construct(self + 4);
    At<uint32_t>(self, 0) = kVtMovementInfo;
    return self;
}

// 0x66F880 (thiscall)
void __fastcall MovementInfo_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtMovementInfo;
    ListHead_Destruct(self + 4);
}

// 0x66F890 (thiscall): the particle pool (all 1500 free, zeroed, linked in order)
uint8_t* __fastcall ParticlePool_Ctor(uint8_t* self, re::Edx) {
    for (int i = 0; i < 1500; i++) {
        float* p = reinterpret_cast<float*>(self + 4 + i * 0x1C);
        for (int j = 0; j < 6; j++) {
            p[j] = 0.0f;
        }
    }
    At<uint32_t>(self, 0) = 0;
    At<uint8_t*>(self, 0xA414) = self + 4;
    for (int i = 0; i < 1499; i++) {
        At<uint8_t*>(self, 4 + i * 0x1C + 0x18) = self + 4 + (i + 1) * 0x1C;
    }
    At<uint8_t*>(self, 0xA410) = nullptr;
    return self;
}

// ---- the particle system primitive: a definition (0x44): +0x10 the low detail rate
// multiplier, +0x14 the distance size multiplier, +0x18 the batches' expiration time, +0x1C
// bit 0 world coordinates, 1 pseudo billboard, 2 facing the effect, 3 spread along the
// effect's move, bits 4..7 the high detail spawn countdown, 8..11 the low detail
// difference, 12..15 the blend mode, +0x1E the initial particle count, +0x20 updates at
// start, +0x22 the spawn rate over time, +0x24 its render infos, +0x28 the batches'
// render infos, +0x2C its movement infos, +0x30 the batches' movement infos, +0x34 its
// texture, +0x38 its emitter. The instance (0x48): +0x14 bits 0..5 its scale (/63), +0x15
// bits 0..4 the spawn countdown, 5..6 the update phase, 7 started, +0x16 its number of
// particles, +0x17 bits 0..3 its detail (/15), 4..7 its velocity scale (/15), +0x18 / +0x1C
// its particles' head and tail, +0x20 / +0x24 its render and movement infos, +0x28 / +0x2C
// its batches' head and tail, +0x30 its emitter, +0x3C its rate's state, +0x40 its camera
// wrap, +0x44 --------------------------------------------------------------------------------

// 0x66F8F0 (thiscall)
uint8_t* __fastcall ParticleSystemDefinition_Ctor(uint8_t* self, re::Edx) {
    PrimDefinition_Construct(self);
    At<float>(self, 0x10) = 0.2f;
    At<float>(self, 0x14) = 1.0f;
    At<uint16_t>(self, 0x1C) = 0xE71;
    At<float>(self, 0x18) = 0.0f;
    self[0x1E] = 0;
    At<uint16_t>(self, 0x20) = 0;
    ValueOverTime_ConstructFn(self + 0x22);
    List_Init(self + 0x24);
    List_Init(self + 0x28);
    List_Init(self + 0x2C);
    List_Init(self + 0x30);
    TextureRef_Construct(self + 0x34);
    EmitterDefinition_ConstructFn(self + 0x38);
    ValueOverTime_SetComponentsFn(self + 0x22, 1);
    return self;
}

// 0x66F970 (thiscall)
uint8_t* __fastcall ParticleSystem_Ctor(uint8_t* self, re::Edx) {
    PrimInstance_Construct(self);
    self[0x14] |= 0x3F;
    self[0x15] = 0;
    self[0x16] = 0;
    self[0x17] = 0xFF;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    List_Init(self + 0x20);
    List_Init(self + 0x24);
    List_Init(self + 0x28);
    At<uint32_t>(self, 0x2C) = 0;
    Emitter_ConstructFn(self + 0x30);
    ValueOverTimeInstance_Construct(self + 0x3C);
    At<uint32_t>(self, 0x44) = noValue_93D7FC;
    At<uint32_t>(self, 0x40) = 0;
    return self;
}

// 0x66F9D0 (thiscall): its destructor
void __fastcall ParticleSystem_Dtor(uint8_t* self, re::Edx) {
    Empty_49A650(self + 0x3C);
    Empty_49A650(self + 0x30);
    Empty_49A650(self + 0x28);
    Empty_49A650(self + 0x24);
    Empty_49A650(self + 0x20);
    PrimInstance_Destruct(self);
}

// 0x66FA10 (thiscall): its render infos (vf0(time)) and movement infos (vf1(time, state),
// then vf0(particles, count, time)) updated; their links at +4
void __fastcall ParticleSystem_UpdateInfos(uint8_t* self, re::Edx, void* state) {
    float time = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    uint8_t* link = At<uint8_t*>(self, 0x20);
    for (uint8_t* info = link ? link - 4 : nullptr; info;) {
        vc::Call<void>(info, 0, time);
        link = At<uint8_t*>(info, 4);
        info = link ? link - 4 : nullptr;
    }
    link = At<uint8_t*>(self, 0x24);
    if (!link) {
        return;
    }
    uint8_t* head = At<uint8_t*>(self, 0x18);
    for (uint8_t* info = link - 4; info;) {
        vc::Call<void>(info, 4, time, state);
        vc::Call<void>(info, 0, head, int(self[0x16]), time);
        link = At<uint8_t*>(info, 4);
        info = link ? link - 4 : nullptr;
    }
}

// 0x66FAB0 (thiscall): its batches older than the expiration time given back (oldest
// first), with as many of its oldest particles (a wrap in modes 2 / 3 told first)
void __fastcall ParticleSystem_RemoveExpired(uint8_t* self, re::Edx) {
    float time = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    re::Ptr32<uint8_t>* head = &At<uint8_t*>(self, 0x28);
    int count = 0;
    uint8_t done = 0;
    for (uint8_t* batch = *head; batch && !done; batch = *head) {
        float age = time - At<float>(batch, 0xC);
        if (!(At<float>(At<uint8_t*>(self, 8), 0x18) < age)) {
            done = 1;
            continue;
        }
        count += batch[0x10];
        Batch_StopFn(batch);
        List_PopFront(head);
        Effects_Free(effectPools_B6F1E0, batch);
    }
    if (!*head) {
        At<uint32_t>(self, 0x2C) = 0;
    }
    if (count <= 0) {
        return;
    }
    float* p = At<float*>(self, 0x18);
    float* next = p;
    if (!At<uint8_t*>(self, 0x40)) {
        for (int i = count; i > 0; i--) {
            next = At<float*>(p, 0x18);
            Pool_Give(p);
            p = next;
        }
    } else {
        for (int i = count; i > 0; i--) {
            next = At<float*>(p, 0x18);
            if (Wrap_GetMode(At<uint8_t*>(self, 0x40)) == 2) {
                Wrap_Mode2ReleaseFn(At<uint8_t*>(self, 0x40), p);
            }
            if (Wrap_GetMode(At<uint8_t*>(self, 0x40)) == 3) {
                Wrap_Mode3ReleaseFn(At<uint8_t*>(self, 0x40), p);
            }
            Pool_Give(p);
            p = next;
        }
    }
    At<float*>(self, 0x18) = next;
    if (!next) {
        At<float*>(self, 0x1C) = next;
    }
    self[0x16] = uint8_t(self[0x16] - count);
}

// 0x66FC00 (thiscall): started from its definition
uint8_t __fastcall ParticleSystem_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    self[0x14] |= 0x3F;
    At<uint8_t*>(self, 8) = definition;
    At<uint32_t>(self, 0x44) = noValue_93D7FC;
    self[0x16] = 0;
    At<uint32_t>(self, 0x18) = 0;
    At<uint32_t>(self, 0x1C) = 0;
    self[0x15] = 0;
    At<uint32_t>(self, 0x40) = 0;
    At<uint32_t>(self, 0x2C) = 0;
    self[0x17] = 0xFF;
    uint8_t ok = 0;
    if (Emitter_InitFn(reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x30), definition + 0x38) &&
        ValueOverTimeState_InitFn(reinterpret_cast<uint16_t*>(self + 0x3C),
                                  reinterpret_cast<uint16_t*>(At<uint8_t*>(self, 8) + 0x22)) &&
        VirtualList_Create_66A040(self + 0x20, At<uint8_t*>(self, 8) + 0x24) &&
        VirtualList_Create_66A100(self + 0x24, At<uint8_t*>(self, 8) + 0x2C)) {
        ok = 1;
    }
    PrimInstance_Restart(self);
    return ok;
}

// 0x66FCA0 (thiscall): how many particles to spawn now: its rate (per second, at 30
// updates a second) scaled by the detail (the low detail multiplier towards 1 by the
// effect's detail +0x4C, then its own detail /15), the fraction by chance; plus, before
// it's started, the initial count (at least 1 when there is one)
int __fastcall ParticleSystem_SpawnCount(uint8_t* self, re::Edx) {
    uint8_t* definition = At<uint8_t*>(self, 8);
    float low = 1.0f - At<float>(definition, 0x10);
    float scale = low * At<float>(At<uint8_t*>(self, 0xC), 0x4C) + At<float>(definition, 0x10);
    uint8_t detail = self[0x17] & 0xF;
    if (detail != 0xF) {
        scale = float(detail) * scale / 15.0f;
    }
    float time = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    float rate;
    ValueOverTimeState_GetFn(reinterpret_cast<uint16_t*>(self + 0x3C), &rate, time);
    float amount = float(double(rate) * kThirtieth) * scale;
    int count = int(amount);
    float fraction = amount - float(count);
    if (!(fraction < Random())) {
        count++;
    }
    if (!(self[0x15] & 0x80)) {
        uint16_t initial = At<uint16_t>(At<uint8_t*>(self, 8), 0x1E) & 0xFF;
        int k = int(float(initial) * scale);
        if (k < 1 && initial) {
            k = 1;
        }
        count += k;
    }
    return count;
}

// 0x66FDA0 (thiscall): a particle added and launched (the second argument is replaced by
// the system for the emitter, the game jumping into it)
void __fastcall ParticleSystem_AddParticle(uint8_t* self, re::Edx, float* particle, void*) {
    System_Append(self, particle);
    Emitter_EmitFn(reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x30), particle, self);
}

// 0x66FDE0 (thiscall): a new batch (pool 1) started now with the batches' render and
// movement infos and appended (a failed start: given back)
uint8_t __fastcall ParticleSystem_NewBatch(uint8_t* self, re::Edx) {
    uint8_t* batch = Effects_Alloc(effectPools_B6F1E0, 1);
    if (!batch) {
        return 0;
    }
    uint8_t* definition = At<uint8_t*>(self, 8);
    float time = PrimTimer_Get(self + 4) - At<float>(definition, 8);
    uint8_t ok = Batch_InitFn(batch, definition + 0x28, definition + 0x30, time);
    if (ok != 1) {
        Effects_Free(effectPools_B6F1E0, batch);
        return ok;
    }
    if (!At<uint8_t*>(self, 0x28)) {
        At<uint8_t*>(batch, 0) = At<uint8_t*>(self, 0x28);
        At<uint8_t*>(self, 0x28) = batch;
    } else {
        At<uint8_t*>(At<uint8_t*>(self, 0x2C), 0) = batch;
    }
    At<uint8_t*>(self, 0x2C) = batch;
    return ok;
}

// 0x66FE70 (thiscall): this update's particles spawned (up to 255 in the system): without
// batch infos straight into the system; with them into the current batch while its
// countdown runs, else into a new batch (the countdown restarted from the definition's,
// longer at low detail; a batch that got none removed again)
void __fastcall ParticleSystem_Spawn(uint8_t* self, re::Edx, void* state) {
    int n = ParticleSystem_SpawnCountFn(self);
    int count = self[0x16];
    if (count + n > 0xFF) {
        n = 0xFF - count;
    }
    uint8_t* definition = At<uint8_t*>(self, 8);
    if (!At<uint8_t*>(definition, 0x28) && !At<uint8_t*>(definition, 0x30)) {
        if (n <= 0) {
            return;
        }
        do {
            float* p = Pool_Take();
            if (p) {
                System_Append(self, p);
                Emitter_EmitFn(reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x30), p, self);
            }
        } while (--n);
        return;
    }
    uint8_t b = self[0x15];
    if (b & 0x1F) {
        self[0x15] = uint8_t(((uint8_t(b - 1) ^ b) & 0x1F) ^ b);
    }
    if (n <= 0) {
        return;
    }
    if ((self[0x15] & 0x1F) && At<uint8_t*>(self, 0x28)) {
        do {
            float* p = Pool_Take();
            if (p) {
                ParticleSystem_AddParticleFn(self, p, state);
                At<uint8_t*>(self, 0x2C)[0x10] += 1;
            }
        } while (--n);
        return;
    }
    uint8_t* lastTail = At<uint8_t*>(self, 0x2C);
    uint8_t made = ParticleSystem_NewBatchFn(self);
    uint16_t bits = At<uint16_t>(At<uint8_t*>(self, 8), 0x1C);
    uint8_t high = (uint8_t(bits) >> 4) & 0xF;
    float lowDetail = 1.0f - At<float>(At<uint8_t*>(self, 0xC), 0x4C);
    int difference = (bits >> 8) & 0xF;
    int extra = int(float(difference) * lowDetail);
    uint8_t countdown = uint8_t(uint8_t(extra) + high);
    self[0x15] = uint8_t(((countdown ^ self[0x15]) & 0x1F) ^ self[0x15]);
    if (!made) {
        self[0x15] &= 0xE0;
        return;
    }
    uint8_t added = 0;
    do {
        float* p = Pool_Take();
        if (p) {
            System_Append(self, p);
            Emitter_EmitFn(reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x30), p, self);
            added = 1;
            At<uint8_t*>(self, 0x2C)[0x10] += added;
        }
    } while (--n);
    if (added) {
        return;
    }
    uint8_t* batch = At<uint8_t*>(self, 0x2C);
    Batch_StopFn(batch);
    At<uint8_t*>(batch, 0) = nullptr;
    if (lastTail) {
        At<uint8_t*>(self, 0x2C) = lastTail;
        At<uint8_t*>(lastTail, 0) = nullptr;
        Effects_Free(effectPools_B6F1E0, batch);
        return;
    }
    List_PopFront(self + 0x28);
    At<uint8_t*>(self, 0x2C) = nullptr;
    Effects_Free(effectPools_B6F1E0, batch);
}

// 0x6700E0 (thiscall): stopped: its parts given back, its particles to the pool (the list
// left as it was)
void __fastcall ParticleSystem_Stop(uint8_t* self, re::Edx) {
    if (!(self[0x10] & 1)) {
        PrimInstance_Kill(self);
    }
    ValueOverTimeState_StopFn(reinterpret_cast<uint16_t*>(self + 0x3C));
    Emitter_StopFn(reinterpret_cast<re::Ptr32<uint8_t>*>(self + 0x30));
    if (At<uint8_t*>(self, 0x40)) {
        Wrap_ClearFn(At<uint8_t*>(self, 0x40));
        Effects_Free(effectPools_B6F1E0, At<uint8_t*>(self, 0x40));
        At<uint32_t>(self, 0x40) = 0;
    }
    VirtualList_Release_66A270(self + 0x20);
    VirtualList_Release_66A2B0(self + 0x24);
    BatchList_Release_66A330(self + 0x28);
    for (float* p = At<float*>(self, 0x18); p;) {
        float* next = At<float*>(p, 0x18);
        Pool_Give(p);
        p = next;
    }
}

// 0x670180 (thiscall): its texture reference
uint8_t* __fastcall ParticleSystem_Texture(uint8_t* self, re::Edx) {
    return At<uint8_t*>(self, 8) + 0x34;
}

// 0x670190 (thiscall): its blend mode (in ax; eax's top half the definition pointer's)
uint32_t __fastcall ParticleSystem_BlendMode(uint8_t* self, re::Edx) {
    uint8_t* definition = At<uint8_t*>(self, 8);
    return (reinterpret_cast<uint32_t>(definition) & 0xFFFF0000u) | uint32_t(At<uint16_t>(definition, 0x1C) >> 0xC);
}

// 0x6701A0 (thiscall): one of its particles at random (the head when it has none)
float* __fastcall ParticleSystem_RandomParticle(uint8_t* self, re::Edx) {
    float* p = At<float*>(self, 0x18);
    if (self[0x16] > 0) {
        float u = RandomRange(0.0f, 1.0f);
        int k = int(float(self[0x16]) * u);
        for (; k > 0; k--) {
            p = At<float*>(p, 0x18);
        }
    }
    return p;
}

// 0x670200 (thiscall): its camera wrap set (made when missing) to a box x by y: with `z`
// the last z primitives of the effect are wrap partners: a system before them wraps its
// own particles (mode 1) mirrored into two of them, one among them (mode 2 / 3 by
// parity) feeds on a source system
void __fastcall ParticleSystem_SetWrap(uint8_t* self, re::Edx, float x, float y, int z) {
    if (!At<uint8_t*>(self, 0x40)) {
        At<uint8_t*>(self, 0x40) = Effects_Alloc(effectPools_B6F1E0, 0x11);
        if (!At<uint8_t*>(self, 0x40)) {
            return;
        }
    }
    if (z) {
        uint8_t* effect = At<uint8_t*>(self, 0xC);
        int total = EffectDefinition_NumPrimitives(At<uint8_t*>(effect, 0x50));
        int index = PrimInstance_Index(self);
        total -= z;
        if (index < total) {
            uint8_t* a = Effect_FindPrimitive(At<uint8_t*>(self, 0xC), total * 3 - index * 2 - 3);
            uint8_t* b = Effect_FindPrimitive(At<uint8_t*>(self, 0xC), total + (total - index) * 2 - 2);
            Wrap_SetMode1Fn(At<uint8_t*>(self, 0x40), self);
            Wrap_Set10(At<uint8_t*>(self, 0x40), a);
            Wrap_Set14Fn(At<uint8_t*>(self, 0x40), b);
        } else {
            int d = index - total;
            if (d % 2 == 0) {
                Wrap_SetMode2Fn(At<uint8_t*>(self, 0x40), self);
            } else {
                Wrap_SetMode3Fn(At<uint8_t*>(self, 0x40), self);
            }
            Wrap_Set20Fn(At<uint8_t*>(self, 0x40), Effect_FindPrimitive(At<uint8_t*>(self, 0xC), d / 2));
        }
    }
    uint8_t* wrap = At<uint8_t*>(self, 0x40);
    At<float>(wrap, 4) = x;
    At<float>(wrap, 8) = y;
}

// 0x6702F0 / 0x670330 / 0x670370 (thiscall): its detail (x15, bits 0..3 of +0x17), scale
// (x63, bits 0..5 of +0x14), velocity scale (x15, bits 4..7 of +0x17), truncated
void __fastcall ParticleSystem_SetDetail(uint8_t* self, re::Edx, float value) {
    uint8_t v = uint8_t(int(value * 15.0f));
    self[0x17] ^= (v ^ self[0x17]) & 0xF;
}
void __fastcall ParticleSystem_SetScale(uint8_t* self, re::Edx, float value) {
    uint8_t v = uint8_t(int(value * 63.0f));
    self[0x14] ^= (v ^ self[0x14]) & 0x3F;
}
void __fastcall ParticleSystem_SetVelocityScale(uint8_t* self, re::Edx, float value) {
    uint8_t low = self[0x17] & 0xF;
    uint8_t v = uint8_t(int(value * 15.0f));
    self[0x17] = uint8_t((v << 4) | low);
}

// 0x6703B0 (thiscall): +0x44 set
void __fastcall ParticleSystem_Set44(uint8_t* self, re::Edx, const uint32_t* value) {
    At<uint32_t>(self, 0x44) = *value;
}

// 0x6703C0 / 0x6703E0 (thiscall): its velocity scale and scale as floats
float __fastcall ParticleSystem_VelocityScale(uint8_t* self, re::Edx) {
    return float(self[0x17] >> 4) / 15.0f;
}
float __fastcall ParticleSystem_GetScale(uint8_t* self, re::Edx) {
    return float(self[0x14] & 0x3F) / 63.0f;
}

// 0x670400 (thiscall): meant to find a dead particle of a system, but its loop never runs
// (its condition inverted): always null
float* __fastcall ParticleList_FindDead(uint8_t* system, re::Edx) {
    (void)system;
    return nullptr;
}

// 0x670430 (thiscall, this unused): a particle killed (moved to 7654321, stopped)
void __fastcall Particle_Kill(void*, re::Edx, float* particle) {
    particle[0] = kDeadZ;
    particle[1] = kDeadZ;
    particle[2] = kDeadZ;
    particle[3] = 0.0f;
    particle[4] = 0.0f;
    particle[5] = 0.0f;
}

// 0x670450 (thiscall): ParticleSystemDefinitionLoader's vf1. (The two countdowns, the first
// three flags and the initial count are read into uninitialised locals: a missing one keeps
// stack garbage in the game, 0 here.)
void __fastcall ParticleSystemDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    uint32_t base[2];
    DefinitionBaseLoader_Construct(base, definition);
    base[0] = kVtEffectPrimitiveDefinitionLoader;
    EffectPrimitiveDefinitionLoader_Read(base, archive);
    vc::Call<uint8_t>(archive, 0x28, definition + 0x10, nameLowDetailParticleRateMultiply);
    int32_t high = 0;
    vc::Call<uint8_t>(archive, 0x38, &high, nameHightDetailSpawnCountDown);
    At<uint16_t>(definition, 0x1C) ^= uint16_t(((uint32_t(high) << 4) ^ definition[0x1C]) & 0xF0);
    int32_t difference = 0;
    vc::Call<uint8_t>(archive, 0x38, &difference, nameLowDetailSpawnCountDownDifference);
    At<uint16_t>(definition, 0x1C) ^= uint16_t(((uint16_t(uint8_t(difference)) << 8) ^ At<uint16_t>(definition, 0x1C)) & 0xF00);
    uint8_t world = 0;
    vc::Call<uint8_t>(archive, 0x3C, &world, nameUseWorldCoordinates);
    At<uint16_t>(definition, 0x1C) = uint16_t((world & 1) | (At<uint16_t>(definition, 0x1C) & 0xFFFE));
    uint8_t billboard = 0;
    vc::Call<uint8_t>(archive, 0x3C, &billboard, namePseudoBillboard);
    At<uint16_t>(definition, 0x1C) = uint16_t(((billboard & 1) * 2) | (At<uint16_t>(definition, 0x1C) & 0xFFFD));
    uint8_t face = 0;
    vc::Call<uint8_t>(archive, 0x3C, &face, nameFaceEffect);
    At<uint16_t>(definition, 0x1C) = uint16_t(((face & 1) * 4) | (At<uint16_t>(definition, 0x1C) & 0xFFFB));
    uint8_t spread = 0;
    vc::Call<uint8_t>(archive, 0x3C, &spread, nameBool);
    At<uint16_t>(definition, 0x1C) = uint16_t(((spread & 1) * 8) | (At<uint16_t>(definition, 0x1C) & 0xFFF7));
    int32_t initial = 0;
    vc::Call<uint8_t>(archive, 0x38, &initial, nameInitialParticleCount);
    definition[0x1E] = uint8_t(initial);
    uint32_t rate[2];
    ValueOverTimeDefinitionLoader_ConstructFn(rate, definition + 0x22);
    Archive_ReadSection(archive, rate, nameDefinition);
    vc::Call<uint8_t>(archive, 0x28, definition + 0x18, nameExpirationTime);
    uint32_t emitter[2];
    DefinitionBaseLoader_Construct(emitter, definition + 0x38);
    emitter[0] = kVtEmitterDefinitionLoader;
    Archive_ReadSection(archive, emitter, nameDefinition);
    EffectDefinition_LoadPrimitives(archive, definition + 0x24);
    EffectDefinition_LoadPrimitives(archive, definition + 0x28);
    EffectDefinition_LoadPrimitives(archive, definition + 0x2C);
    EffectDefinition_LoadPrimitives(archive, definition + 0x30);
    // the texture: by its lower-case name, loaded when not there yet
    uint8_t name[0x2C];   // (an archive string: +0 its buffer)
    ArchiveString_Init(name);
    vc::Call<uint8_t>(archive, 0x24, name, 0x21, nameTextureName);
    ArchiveString_ToLower(name);
    uint32_t hash = HashName(At<const char*>(name, 0));
    if (!EffectTextures_Has(effectTextures_B6F1F0, hash)) {
        EffectTextures_Add(effectTextures_B6F1F0, String_Get_773DB0(effectTextures_B6F1F0), At<const char*>(name, 0));
    }
    TextureRef_Set(definition + 0x34, hash);
    // (both read into the archive argument's slot: its old value kept only in the dword)
    int32_t blend = int32_t(reinterpret_cast<uintptr_t>(archive));
    uint8_t ok = vc::Call<uint8_t>(archive, 0x38, &blend, nameBlendMode);
    At<uint16_t>(definition, 0x1C) = uint16_t((At<uint16_t>(definition, 0x1C) & 0xFFF) | ((ok ? blend : 0) << 0xC));
    ok = vc::Call<uint8_t>(archive, 0x38, &blend, nameUpdatesAtStart);
    At<uint16_t>(definition, 0x20) = uint16_t(ok ? blend : 0);
    vc::Call<uint8_t>(archive, 0x28, definition + 0x14, nameDistanceSizeMultiply);
}

// 0x670730 (thiscall): its infos updated; with batch infos its expired batches removed and
// each batch updated (with batch movement infos, each batch's particles moved in turn)
void __fastcall ParticleSystem_Update(uint8_t* self, re::Edx, void* state) {
    float time = PrimTimer_Get(self + 4) - At<float>(At<uint8_t*>(self, 8), 8);
    ParticleSystem_UpdateInfosFn(self, state);
    uint8_t* definition = At<uint8_t*>(self, 8);
    if (!At<uint8_t*>(definition, 0x28) && !At<uint8_t*>(definition, 0x30)) {
        return;
    }
    ParticleSystem_RemoveExpiredFn(self);
    if (At<uint8_t*>(At<uint8_t*>(self, 8), 0x30)) {
        float* p = At<float*>(self, 0x18);
        for (uint8_t* batch = At<uint8_t*>(self, 0x28); batch; batch = At<uint8_t*>(batch, 0)) {
            Batch_UpdateFn(batch, time, state);
            p = Batch_ProcessFn(batch, p, time);
        }
        return;
    }
    for (uint8_t* batch = At<uint8_t*>(self, 0x28); batch; batch = At<uint8_t*>(batch, 0)) {
        Batch_UpdateFn(batch, time, state);
    }
}

// 0x6707E0 (thiscall): a step: its particles moved by their velocities; every other step
// its infos updated; then spawning
void __fastcall ParticleSystem_Step(uint8_t* self, re::Edx, void* state) {
    worldCoordinates_1B890B8 = At<uint8_t*>(self, 8)[0x1C] & 1;
    for (float* p = At<float*>(self, 0x18); p; p = At<float*>(p, 0x18)) {
        p[0] = p[3] + p[0];
        p[1] = p[4] + p[1];
        p[2] = p[5] + p[2];
    }
    uint8_t b = self[0x15];
    uint8_t phase = b >> 5;
    if (!(phase & 3)) {
        self[0x15] = uint8_t((b & 0xBF) | 0x20);
        ParticleSystem_UpdateFn(self, state);
        ParticleSystem_SpawnFn(self, state);
        return;
    }
    self[0x15] = uint8_t(((uint8_t((phase - 1) << 5) ^ b) & 0x60) ^ b);
    ParticleSystem_SpawnFn(self, state);
}

// 0x670870 (thiscall): the primitive's update: a step (before it has started, as many more
// as its updates at start, each 1/30 s earlier); then its camera wrap
void __fastcall ParticleSystem_PrimUpdate(uint8_t* self, re::Edx, void* state) {
    ParticleSystem_StepFn(self, state);
    if (!(self[0x15] & 0x80) && At<uint16_t>(At<uint8_t*>(self, 8), 0x20)) {
        for (int i = 0; i < At<uint16_t>(At<uint8_t*>(self, 8), 0x20); i++) {
            Timer_Sub(self + 4, 1.0f / 30.0f);
            ParticleSystem_StepFn(self, state);
        }
    }
    if (At<uint8_t*>(self, 0x40)) {
        Wrap_ProcessFn(At<uint8_t*>(self, 0x40), self);
    }
    self[0x15] |= 0x80;
}

// ---- the source emitter: a definition (0xC): +4 bits 0..2 the source system's index,
// +8 bit 0 its velocities too. Its instance (8): +4 the definition ------------------------

// 0x6708E0 (thiscall; not in the function list)
uint8_t* __fastcall SourceEmitterDefinition_Ctor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 4) &= 0xFFFFFFF8u;
    self[8] &= 0xFE;
    self[0] = 0xFF;
    return self;
}

// 0x6708F0 (thiscall): ParticleSystemSourceEmitterDefinitionLoader's vf1
void __fastcall SourceEmitterDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    int32_t index = 0;
    vc::Call<uint8_t>(archive, 0x38, &index, nameIndexSource);
    At<uint32_t>(definition, 4) ^= (At<uint32_t>(definition, 4) ^ uint32_t(index)) & 7;
    uint8_t velocities = 0;
    vc::Call<uint8_t>(archive, 0x3C, &velocities, nameUseVelocitis);
    definition[8] ^= (definition[8] ^ velocities) & 1;
}

// 0x670950 (thiscall)
uint8_t* __fastcall SourceEmitter_Ctor(uint8_t* self, re::Edx) {
    ListHead_Construct(self);
    At<uint32_t>(self, 4) = 0;
    return self;
}

// 0x670970 (thiscall)
uint8_t __fastcall SourceEmitter_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint8_t*>(self, 4) = definition;
    return 1;
}

// 0x670980 (thiscall): a random particle of the effect's source system: its position (and
// velocity when so defined); none: false
uint8_t __fastcall SourceEmitter_Emit(uint8_t* self, re::Edx, float* position, float* velocity, uint8_t* effect) {
    uint8_t* source = Effect_FindPrimitive(effect, At<uint32_t>(At<uint8_t*>(self, 4), 4) & 7);
    if (!source) {
        return 0;
    }
    float* p = ParticleSystem_RandomParticleFn(source);
    if (!p) {
        return 0;
    }
    Copy3(position, p);
    if (At<uint8_t*>(self, 4)[8] & 1) {
        Copy3(velocity, p + 3);
    }
    return 1;
}

// ---- the camera wrap (pool 0x11, 0x28 bytes): +4 / +8 the box's half width and height,
// +0xC its system (mode 1), +0x10 / +0x14 the mirror systems, +0x18 / +0x1C (modes 2 / 3),
// +0x20 the source system, +0x24 the mode -----------------------------------------------------

// 0x6709F0 (cdecl): a - b, folded into [0, c) (as a fraction of c) when above c
float __cdecl Wrap_Fold(float a, float b, float c) {
    float d = a - b;
    if (c < d) {
        float q = d / c;
        return q - float(int(q));
    }
    return d;
}

// 0x670A40 (thiscall)
uint8_t* __fastcall Wrap_Ctor(uint8_t* self, re::Edx) {
    ListHead_Construct(self);
    At<float>(self, 4) = 1.0f;
    At<float>(self, 8) = 1.0f;
    for (int at = 0xC; at <= 0x24; at += 4) {
        At<uint32_t>(self, at) = 0;
    }
    return self;
}

// 0x670A70 (thiscall)
void __fastcall Wrap_Clear(uint8_t* self, re::Edx) {
    for (int at = 0xC; at <= 0x24; at += 4) {
        At<uint32_t>(self, at) = 0;
    }
}

// 0x670A90 / 0x670AE0 (thiscall): a particle copied into a dead one of a mirror system
// (never found: see 0x670400), then killed
template <int Mirror>
void __fastcall Wrap_CopyToMirror(uint8_t* self, re::Edx, float* particle) {
    float* dead = ParticleList_FindDeadFn(At<uint8_t*>(self, Mirror));
    if (!dead) {
        return;
    }
    for (int i = 0; i < 6; i++) {
        dead[i] = particle[i];
    }
    Particle_KillFn(At<uint8_t*>(self, 0xC), particle);
}

// 0x670B30 / 0x670B60 / 0x670B80 (thiscall): its system in mode 1 / 2 / 3
template <int At_, int Mode>
void __fastcall Wrap_SetMode(uint8_t* self, re::Edx, void* system) {
    At<void*>(self, At_) = system;
    At<int32_t>(self, 0x24) = Mode;
}
// 0x670B50 / 0x670BA0 (thiscall)
template <int At_>
void __fastcall Wrap_Set(uint8_t* self, re::Edx, void* system) {
    At<void*>(self, At_) = system;
}

// 0x670BB0 / 0x671250 (thiscall): a live particle of a mode 2 / 3 system copied into a
// dead one of the source system, mode 3 folding it into the box. (Never found, see
// 0x670400: the game writes through null. Mode 2 / 3 particles start dead, see 0x66F4C0.)
template <bool Fold>
void __fastcall Wrap_ReleaseLive(uint8_t* self, re::Edx, float* particle) {
    if (particle[2] == kDeadZ) {
        return;
    }
    float* volatile dead = ParticleList_FindDeadFn(At<uint8_t*>(self, 0x20));
    for (int i = 0; i < 6; i++) {
        dead[i] = particle[i];
    }
    if constexpr (Fold) {
        Wrap_FoldParticleFn(self, dead);
    }
}

// 0x670C00 (thiscall): the camera's flattened transform (its right and forward axes laid
// level, up straight up, at the camera's position) and its inverse. (The transform's
// scale is left to the caller.)
void __fastcall Wrap_CameraTransforms(uint8_t*, re::Edx, uint8_t* inverse, float* transform) {
    float* m = Node_WorldTransform(Camera_GetNode(Effects_GetCamera(effectManager_B6F1D4 + 0x20)));
    float forward[3] = {m[2], m[5], m[8]};
    uint32_t tx = At<uint32_t>(m, 0x24);
    uint32_t ty = At<uint32_t>(m, 0x28);
    uint32_t tz = At<uint32_t>(m, 0x2C);
    float right[3] = {m[0], m[3], m[6]};
    forward[2] = 0.0f;
    Vector_Normalise_414E10(forward);
    right[2] = 0.0f;
    Vector_Normalise_414E10(right);
    At<uint32_t>(transform, 0x24) = tx;
    At<uint32_t>(transform, 0x28) = ty;
    At<uint32_t>(transform, 0x2C) = tz;
    transform[2] = forward[0];
    transform[5] = forward[1];
    transform[8] = forward[2];
    transform[1] = 0.0f;
    transform[4] = 0.0f;
    transform[7] = 1.0f;
    transform[0] = right[0];
    transform[3] = right[1];
    transform[6] = right[2];
    NiTransform_Invert(inverse, transform);
}

// a coordinate past +half folded back from -half (the game's fraction, not scaled back)
inline float FoldAbove(float x, float half) {
    float w = half + half;
    float d = x - half;
    if (w < d) {
        float q = d / w;
        return (q - float(int(q))) - half;
    }
    return d - half;
}

// 0x670D10 (thiscall): a particle's position folded into the box around the camera (x
// across [-half, half], z (up) across [0, height]); moved back only when x or z changed
void __fastcall Wrap_FoldParticle(uint8_t* self, re::Edx, float* particle) {
    float transform[13];
    float inverse[13];
    transform[12] = 1.0f;
    inverse[12] = 1.0f;
    Wrap_CameraTransformsFn(self, inverse, transform);
    float p[3] = {particle[0], particle[1], particle[2]};
    float tmp[3];
    Copy3(p, NiTransform_Transform(inverse, tmp, p));
    uint8_t moved = 0;
    float half = At<float>(self, 4);
    if (half < p[0]) {
        moved = 1;
        p[0] = FoldAbove(p[0], half);
    } else if (p[0] < -half) {
        moved = 1;
        float w = half + half;
        p[0] = float((Wrap_FoldFn(-p[0], half, w) - half) * -1.0);
    }
    float height = At<float>(self, 8);
    if (height < p[2]) {
        float d = p[2] - height;
        if (height < d) {
            float q = d / height;
            p[2] = q - float(int(q));
        } else {
            p[2] = d;
        }
    } else if (p[2] < 0.0f) {
        double top = At<float>(self, 8);
        p[2] = float(top - Wrap_FoldFn(-p[2], height, height));
    } else if (moved != 1) {
        return;
    }
    Copy3(particle, NiTransform_Transform(transform, tmp, p));
}

// 0x670F00 (thiscall): a mode 0 / 1 wrap's live particles folded into the box around the
// camera (as 0x670D10, by the camera's inverse applied by hand with its scale). Mode 0
// moves them back; mode 1 moves those folded across x (or below the floor) back and mirrors
// them into +0x10, and mirrors those folded over the top into +0x14. (Mode 0's transform
// back is written over the inverse's scale in the game's frame: from the second particle
// on, the scale used is the last particle's new x. Kept: `inverse` spans that slot.)
void __fastcall Wrap_Process(uint8_t* self, re::Edx, uint8_t* system) {
    int mode = At<int32_t>(self, 0x24);
    if (mode != 0 && mode != 1) {
        return;
    }
    float transform[13];
    float inverse[16];   // [12] its scale, then the room mode 0's result spills into
    transform[12] = 1.0f;
    inverse[12] = 1.0f;
    Wrap_CameraTransformsFn(self, inverse, transform);
    for (float* p = At<float*>(system, 0x18); p; p = At<float*>(p, 0x18)) {
        if (p[2] == kDeadZ) {
            continue;
        }
        float k = inverse[12];
        float v[3] = {p[0], p[1], p[2]};
        float r[3];
        float* rotated = NiMatrix3_MultiplyVector(inverse, r, v);
        uint8_t moved = 0;
        uint8_t movedX = 0;
        uint8_t movedTop = 0;
        float scaled[3] = {k * rotated[0], rotated[1] * k, k * rotated[2]};
        float q[3] = {inverse[9] + scaled[0], inverse[10] + scaled[1], inverse[11] + scaled[2]};
        float half = At<float>(self, 4);
        if (half < q[0]) {
            moved = 1;
            movedX = 1;
            q[0] = FoldAbove(q[0], half);
        } else if (q[0] < -half) {
            moved = 1;
            movedX = 1;
            float w = half + half;
            float d = -q[0] - half;
            float f = d;
            if (w < d) {
                float quotient = d / w;
                f = quotient - float(int(quotient));
            }
            q[0] = float((f - half) * -1.0);
        }
        float height = At<float>(self, 8);
        if (height < q[2]) {
            movedTop = 1;
            float d = q[2] - height;
            if (height < d) {
                float quotient = d / height;
                q[2] = quotient - float(int(quotient));
            } else {
                q[2] = d;
            }
        } else if (q[2] < 0.0f) {
            movedX = 1;   // (the game sets the x flag here)
            float d = -q[2] - height;
            float f = d;
            if (height < d) {
                float quotient = d / height;
                f = quotient - float(int(quotient));
            }
            q[2] = height - f;
        } else if (moved != 1) {
            continue;
        }
        if (At<int32_t>(self, 0x24) == 0) {
            Copy3(p, NiTransform_Transform(transform, &inverse[12], q));
            continue;
        }
        if (movedX) {
            float back[3];
            Copy3(q, NiTransform_Transform(transform, back, q));
            Copy3(p, q);
            Wrap_CopyToMirror10Fn(self, p);
        }
        if (movedTop) {
            Wrap_CopyToMirror14Fn(self, p);
        }
    }
}

// 0x6712B0 (thiscall): source points (pool 0xE, 0xF8 bytes: +4 up to 20 points, +0xF4
// their number) emptied
void __fastcall SourcePoints_Clear(uint8_t* self, re::Edx) {
    self[0xF4] = 0;
}

// 0x6712C0 (thiscall): one of its points at random (none: zero)
void __fastcall SourcePoints_Random(uint8_t* self, re::Edx, float* out) {
    uint32_t n = At<uint32_t>(self, 0xF4) & 0xFF;
    if (!n) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        return;
    }
    float last = float(double(n) - 0.5099999904632568);
    float u = RandomRange(0.0f, last);
    int i = int(u);
    Copy3(out, reinterpret_cast<float*>(self + 4 + i * 0xC));
}

} // namespace

#define EFFPART35_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

EFFPART35_REVERSED("ValueOverTimeDefinitionLoader::Construct_66E440", 0x0066E440, &ValueOverTimeDefinitionLoader_Ctor);
EFFPART35_REVERSED("ValueOverTime::Construct_66E460", 0x0066E460, &ValueOverTime_Ctor);
EFFPART35_REVERSED("ValueOverTime::SetComponents_66E470", 0x0066E470, &ValueOverTime_SetComponents);
EFFPART35_REVERSED("ValueOverTime::AllocValues_66E490", 0x0066E490, &ValueOverTime_AllocValues);
EFFPART35_REVERSED("ValueOverTime::SetValues_66E510", 0x0066E510, &ValueOverTime_SetValues);
EFFPART35_REVERSED("ValueOverTimeState::Init_66E560", 0x0066E560, &ValueOverTimeState_Init);
EFFPART35_REVERSED("ValueOverTimeState::Stop_66E620", 0x0066E620, &ValueOverTimeState_Stop);
EFFPART35_REVERSED("ValueOverTimeState::GetRest_66E660", 0x0066E660, &ValueOverTimeState_GetRest);
EFFPART35_REVERSED("ValueOverTimeState::Get_66E6B0", 0x0066E6B0, &ValueOverTimeState_Get);
EFFPART35_REVERSED("ValueOverTimeDefinitionLoader::Read_66E710", 0x0066E710, &ValueOverTimeDefinitionLoader_Read);
EFFPART35_REVERSED("ValueOverTimeState::Definition_66E890", 0x0066E890, &ValueOverTimeState_Definition);
EFFPART35_REVERSED("AngleEmitterDefinition::Construct_66E8B0", 0x0066E8B0, &AngleEmitterDefinition_Ctor);
EFFPART35_REVERSED("AngleEmitterDefinitionLoader::Read_66E940", 0x0066E940, &AngleEmitterDefinitionLoader_Read);
EFFPART35_REVERSED("AngleEmitter::Construct_66EA40", 0x0066EA40, &AngleEmitter_Ctor);
EFFPART35_REVERSED("AngleEmitter::Destruct_66EA90", 0x0066EA90, &AngleEmitter_Dtor);
EFFPART35_REVERSED("AngleEmitter::Init_66EAD0", 0x0066EAD0, &AngleEmitter_Init);
EFFPART35_REVERSED("AngleEmitter::Stop_66EB60", 0x0066EB60, &AngleEmitter_Stop);
EFFPART35_REVERSED("Effects::RandomOnSphere_66EBA0", 0x0066EBA0, &RandomOnSphere);
EFFPART35_REVERSED("Effects::RandomInCone_66ECE0", 0x0066ECE0, &RandomInCone);
EFFPART35_REVERSED("AngleEmitter::Emit_66EF80", 0x0066EF80, &AngleEmitter_Emit);
EFFPART35_REVERSED("EmitterDefinition::Construct_66F1D0", 0x0066F1D0, &EmitterDefinition_Ctor);
EFFPART35_REVERSED("EmitterDefinitionLoader::Read_66F1E0", 0x0066F1E0, &EmitterDefinitionLoader_Read);
EFFPART35_REVERSED("Emitter::Construct_66F280", 0x0066F280, &Emitter_Ctor);
EFFPART35_REVERSED("Emitter::Init_66F290", 0x0066F290, &Emitter_Init);
EFFPART35_REVERSED("Emitter::Stop_66F350", 0x0066F350, &Emitter_Stop);
EFFPART35_REVERSED("Emitter::SpreadAlongMove_66F3C0", 0x0066F3C0, &Emitter_SpreadAlongMove);
EFFPART35_REVERSED("Emitter::Emit_66F4C0", 0x0066F4C0, &Emitter_Emit);
EFFPART35_REVERSED("ListDefinition::Construct_66F840", 0x0066F840, &ListDefinition_Ctor);
EFFPART35_REVERSED("MovementInfo::Construct_66F860", 0x0066F860, &MovementInfo_Ctor);
EFFPART35_REVERSED("MovementInfo::Destruct_66F880", 0x0066F880, &MovementInfo_Dtor);
EFFPART35_REVERSED("ParticlePool::Construct_66F890", 0x0066F890, &ParticlePool_Ctor);
EFFPART35_REVERSED("ParticleSystemDefinition::Construct_66F8F0", 0x0066F8F0, &ParticleSystemDefinition_Ctor);
EFFPART35_REVERSED("ParticleSystem::Construct_66F970", 0x0066F970, &ParticleSystem_Ctor);
EFFPART35_REVERSED("ParticleSystem::Destruct_66F9D0", 0x0066F9D0, &ParticleSystem_Dtor);
EFFPART35_REVERSED("ParticleSystem::UpdateInfos_66FA10", 0x0066FA10, &ParticleSystem_UpdateInfos);
EFFPART35_REVERSED("ParticleSystem::RemoveExpired_66FAB0", 0x0066FAB0, &ParticleSystem_RemoveExpired);
EFFPART35_REVERSED("ParticleSystem::Init_66FC00", 0x0066FC00, &ParticleSystem_Init);
EFFPART35_REVERSED("ParticleSystem::SpawnCount_66FCA0", 0x0066FCA0, &ParticleSystem_SpawnCount);
EFFPART35_REVERSED("ParticleSystem::AddParticle_66FDA0", 0x0066FDA0, &ParticleSystem_AddParticle);
EFFPART35_REVERSED("ParticleSystem::NewBatch_66FDE0", 0x0066FDE0, &ParticleSystem_NewBatch);
EFFPART35_REVERSED("ParticleSystem::Spawn_66FE70", 0x0066FE70, &ParticleSystem_Spawn);
EFFPART35_REVERSED("ParticleSystem::Stop_6700E0", 0x006700E0, &ParticleSystem_Stop);
EFFPART35_REVERSED("ParticleSystem::Texture_670180", 0x00670180, &ParticleSystem_Texture);
EFFPART35_REVERSED("ParticleSystem::BlendMode_670190", 0x00670190, &ParticleSystem_BlendMode);
EFFPART35_REVERSED("ParticleSystem::RandomParticle_6701A0", 0x006701A0, &ParticleSystem_RandomParticle);
EFFPART35_REVERSED("ParticleSystem::SetWrap_670200", 0x00670200, &ParticleSystem_SetWrap);
EFFPART35_REVERSED("ParticleSystem::SetDetail_6702F0", 0x006702F0, &ParticleSystem_SetDetail);
EFFPART35_REVERSED("ParticleSystem::SetScale_670330", 0x00670330, &ParticleSystem_SetScale);
EFFPART35_REVERSED("ParticleSystem::SetVelocityScale_670370", 0x00670370, &ParticleSystem_SetVelocityScale);
EFFPART35_REVERSED("ParticleSystem::Set44_6703B0", 0x006703B0, &ParticleSystem_Set44);
EFFPART35_REVERSED("ParticleSystem::VelocityScale_6703C0", 0x006703C0, &ParticleSystem_VelocityScale);
EFFPART35_REVERSED("ParticleSystem::GetScale_6703E0", 0x006703E0, &ParticleSystem_GetScale);
EFFPART35_REVERSED("ParticleSystem::FindDead_670400", 0x00670400, &ParticleList_FindDead);
EFFPART35_REVERSED("Particle::Kill_670430", 0x00670430, &Particle_Kill);
EFFPART35_REVERSED("ParticleSystemDefinitionLoader::Read_670450", 0x00670450, &ParticleSystemDefinitionLoader_Read);
EFFPART35_REVERSED("ParticleSystem::Update_670730", 0x00670730, &ParticleSystem_Update);
EFFPART35_REVERSED("ParticleSystem::Step_6707E0", 0x006707E0, &ParticleSystem_Step);
EFFPART35_REVERSED("ParticleSystem::PrimUpdate_670870", 0x00670870, &ParticleSystem_PrimUpdate);
EFFPART35_REVERSED("SourceEmitterDefinition::Construct_6708E0", 0x006708E0, &SourceEmitterDefinition_Ctor);
EFFPART35_REVERSED("ParticleSystemSourceEmitterDefinitionLoader::Read_6708F0", 0x006708F0, &SourceEmitterDefinitionLoader_Read);
EFFPART35_REVERSED("SourceEmitter::Construct_670950", 0x00670950, &SourceEmitter_Ctor);
EFFPART35_REVERSED("SourceEmitter::Init_670970", 0x00670970, &SourceEmitter_Init);
EFFPART35_REVERSED("SourceEmitter::Emit_670980", 0x00670980, &SourceEmitter_Emit);
EFFPART35_REVERSED("ParticleWrap::Fold_6709F0", 0x006709F0, &Wrap_Fold);
EFFPART35_REVERSED("ParticleWrap::Construct_670A40", 0x00670A40, &Wrap_Ctor);
EFFPART35_REVERSED("ParticleWrap::Clear_670A70", 0x00670A70, &Wrap_Clear);
EFFPART35_REVERSED("ParticleWrap::CopyToMirror_670A90", 0x00670A90, &Wrap_CopyToMirror<0x10>);
EFFPART35_REVERSED("ParticleWrap::CopyToMirror_670AE0", 0x00670AE0, &Wrap_CopyToMirror<0x14>);
EFFPART35_REVERSED("ParticleWrap::SetMode1_670B30", 0x00670B30, &Wrap_SetMode<0xC, 1>);
EFFPART35_REVERSED("ParticleWrap::Set14_670B50", 0x00670B50, &Wrap_Set<0x14>);
EFFPART35_REVERSED("ParticleWrap::SetMode2_670B60", 0x00670B60, &Wrap_SetMode<0x18, 2>);
EFFPART35_REVERSED("ParticleWrap::SetMode3_670B80", 0x00670B80, &Wrap_SetMode<0x1C, 3>);
EFFPART35_REVERSED("ParticleWrap::Set20_670BA0", 0x00670BA0, &Wrap_Set<0x20>);
EFFPART35_REVERSED("ParticleWrap::Mode2Release_670BB0", 0x00670BB0, &Wrap_ReleaseLive<false>);
EFFPART35_REVERSED("ParticleWrap::CameraTransforms_670C00", 0x00670C00, &Wrap_CameraTransforms);
EFFPART35_REVERSED("ParticleWrap::FoldParticle_670D10", 0x00670D10, &Wrap_FoldParticle);
EFFPART35_REVERSED("ParticleWrap::Process_670F00", 0x00670F00, &Wrap_Process);
EFFPART35_REVERSED("ParticleWrap::Mode3Release_671250", 0x00671250, &Wrap_ReleaseLive<true>);
EFFPART35_REVERSED("SourcePoints::Clear_6712B0", 0x006712B0, &SourcePoints_Clear);
EFFPART35_REVERSED("SourcePoints::Random_6712C0", 0x006712C0, &SourcePoints_Random);
