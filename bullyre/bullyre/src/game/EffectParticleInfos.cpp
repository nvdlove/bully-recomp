// The particle systems' infos (0x671350..0x672A00): the batches (a system's particles
// spawned together, expiring together), the movement infos (attractor, friction, gravity,
// jitter: vf0(particles, count, time) moves count particles and returns the next one,
// vf1(time, transform) updates their values, vf2(definition) starts, vf3 stops) and the
// render infos (colour and size, 2d rectangle, rotation, UV: vf0(time) updates their
// values, vf1(definition) starts, vf2 stops), their definitions and loaders. See
// EffectParticleSystems.cpp for the value-over-time fields (2 bytes) and their states.
//
// An info: +0 vtable, +4 its link in its system's (or batch's) list, its fields from +8.
// Its definition: +0 type, +4 link, its fields from +8 (or +0xC).

#include <cmath>
#include <cstdint>

#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

GAME_VAR(uint8_t*, effectPools_B6F1E0, 0x00B6F1E0);
GAME_VAR(uint8_t, worldCoordinates_1B890B8, 0x01B890B8);   // the stepping system's
GAME_VAR(float, twoPi_B9D1D4, 0x00B9D1D4);
inline void* const random_CF356C = reinterpret_cast<void*>(0x00CF356C);

GAME_VAR(const char*, nameFloatValue, 0x0093D578);
GAME_VAR(const char*, nameColor, 0x0093D5B0);
GAME_VAR(const char*, nameAlpha, 0x0093D5B4);
GAME_VAR(const char*, nameSize, 0x0093D5B8);
GAME_VAR(const char*, nameAttractorPoint, 0x0093D5C8);
GAME_VAR(const char*, nameAttractorStrength, 0x0093D5CC);
GAME_VAR(const char*, nameTopLeft, 0x0093D5D0);
GAME_VAR(const char*, nameBottomLeft, 0x0093D5D4);
GAME_VAR(const char*, nameBottomRight, 0x0093D5D8);
GAME_VAR(const char*, nameTopRight, 0x0093D5DC);
GAME_VAR(const char*, nameRotationAngleDelta, 0x0093D5E4);
GAME_VAR(const char*, nameAnimationFrameCount, 0x0093D61C);
GAME_VAR(const char*, nameUVAnimationLoopingType, 0x0093D620);
GAME_VAR(const char*, nameRotationRandomAngleStart, 0x0093D624);

constexpr uint32_t kVtRenderInfo = 0x0093D83C;
constexpr uint32_t kVtAttractor = 0x0093D850;
constexpr uint32_t kVtFriction = 0x0093D86C;
constexpr uint32_t kVtGravity = 0x0093D884;
constexpr uint32_t kVtJitter = 0x0093D89C;
constexpr uint32_t kVtColorAndSize = 0x0093D8B0;
constexpr uint32_t kVtRectangle2d = 0x0093D8C0;
constexpr uint32_t kVtRotation = 0x0093D8D4;
constexpr uint32_t kVtUV = 0x0093D8EC;

constexpr double kStrengthScale = 0.0022239999379962683;
constexpr double kRotationScale = 0.06669999659061432;

GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
GAME_FN(void, __thiscall, Empty_49A650, 0x0049A650, void* self);   // (does nothing)
GAME_FN(uint8_t, __thiscall, Archive_ReadSection, 0x005B0720, void* archive, void* reader, const char* name);
GAME_FN(void, __thiscall, List_Init, 0x00667DA0, void* list);
GAME_FN(void*, __thiscall, List_PopFront, 0x00667DB0, void* list);
GAME_FN(void*, __thiscall, ListHead_Construct, 0x006693E0, void* self);
GAME_FN(void, __thiscall, ListHead_Destruct, 0x006693F0, void* self);
GAME_FN(void, __thiscall, Effects_Free, 0x00669DC0, uint8_t* pools, void* object);
GAME_FN(uint8_t, __cdecl, VirtualList_Create_66A040, 0x0066A040, void* out, void* definitions);
GAME_FN(uint8_t, __cdecl, VirtualList_Create_66A100, 0x0066A100, void* out, void* definitions);
GAME_FN(void, __cdecl, RandomDirection, 0x0066A500, float* out);
GAME_FN(void, __thiscall, ValueOverTimeInstance_Construct, 0x0066BEA0, void* self);   // (empty)
GAME_FN(void*, __thiscall, ValueOverTimeDefinitionLoader_Construct, 0x0066E440, void* self, void* definition);
GAME_FN(void, __thiscall, ValueOverTime_Construct, 0x0066E460, void* self);
GAME_FN(void, __thiscall, ValueOverTime_SetComponents, 0x0066E470, void* self, int n);
GAME_FN(uint8_t, __thiscall, ValueOverTimeState_Init, 0x0066E560, void* self, const void* definition);
GAME_FN(void, __thiscall, ValueOverTimeState_Stop, 0x0066E620, void* self);
GAME_FN(void, __thiscall, ValueOverTimeState_Get, 0x0066E6B0, void* self, float* out, float time);
GAME_FN(uint8_t*, __thiscall, ListDefinition_Construct, 0x0066F840, uint8_t* self);
GAME_FN(uint8_t*, __thiscall, MovementInfo_Construct, 0x0066F860, uint8_t* self);
GAME_FN(void, __thiscall, MovementInfo_Destruct, 0x0066F880, uint8_t* self);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, float* v);
GAME_FN(float*, __thiscall, NiTransform_Transform, 0x00450B10, const void* transform, float* out, const float* v);

// ours, through the game's addresses
GAME_FN(uint8_t*, __thiscall, RenderInfo_ConstructFn, 0x00671370, uint8_t* self);
GAME_FN(void, __thiscall, RenderInfo_DestructFn, 0x00671390, uint8_t* self);
GAME_FN(void, __thiscall, Batch_StopFn, 0x006714A0, uint8_t* self);
GAME_FN(void, __thiscall, UV_RandomFrameFn, 0x006724D0, uint8_t* self);
GAME_FN(void, __thiscall, UV_SetFrameFn, 0x00672790, uint8_t* self, int frame);

inline float Random() {
    return Random_GetFloat(random_CF356C);
}
// the objects of a list linked at +4 (null link: none)
inline uint8_t* InfoOf(uint8_t* link) {
    return link ? link - 4 : nullptr;
}
// a value-over-time field's loader (8 bytes on the stack)
struct Loader {
    uint32_t words[2];
    Loader(void* field) {
        ValueOverTimeDefinitionLoader_Construct(words, field);
    }
};

// ---- the batch (pool 1, 0x14): +0 next, +4 its render infos, +8 its movement infos, +0xC
// when it started, +0x10 its number of particles -------------------------------------------

// 0x671350 (thiscall): source points' constructor (see EffectParticleSystems.cpp)
uint8_t* __fastcall SourcePoints_Ctor(uint8_t* self, re::Edx) {
    ListHead_Construct(self);
    self[0xF4] = 0;
    return self;
}

// 0x671370 (thiscall)
uint8_t* __fastcall RenderInfo_Ctor(uint8_t* self, re::Edx) {
    ListHead_Construct(self + 4);
    At<uint32_t>(self, 0) = kVtRenderInfo;
    return self;
}

// 0x671390 (thiscall)
void __fastcall RenderInfo_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtRenderInfo;
    ListHead_Destruct(self + 4);
}

// 0x6713A0 (thiscall)
uint8_t* __fastcall Batch_Ctor(uint8_t* self, re::Edx) {
    ListHead_Construct(self);
    List_Init(self + 4);
    List_Init(self + 8);
    At<float>(self, 0xC) = 0.0f;
    self[0x10] = 0;
    return self;
}

// 0x6713D0 (thiscall)
void __fastcall Batch_Dtor(uint8_t* self, re::Edx) {
    Empty_49A650(self + 8);
    Empty_49A650(self + 4);
    ListHead_Destruct(self);
}

// 0x6713F0 (thiscall): its particles (from `particle`) moved by its movement infos at its
// age; the last one's result (the particle after them; none: null)
uint8_t* __fastcall Batch_Process(uint8_t* self, re::Edx, uint8_t* particle, float time) {
    float age = time - At<float>(self, 0xC);
    uint8_t* result = nullptr;
    for (uint8_t* link = At<uint8_t*>(self, 8); link; link = At<uint8_t*>(link, 0)) {
        result = vc::Call<uint8_t*>(link - 4, 0, particle, int(self[0x10]), age);
    }
    return result;
}

// 0x671440 (thiscall): its movement infos' and render infos' values updated at its age
void __fastcall Batch_Update(uint8_t* self, re::Edx, float time, void* transform) {
    float age = time - At<float>(self, 0xC);
    for (uint8_t* link = At<uint8_t*>(self, 8); link; link = At<uint8_t*>(link, 0)) {
        vc::Call<void>(link - 4, 4, age, transform);
    }
    for (uint8_t* link = At<uint8_t*>(self, 4); link; link = At<uint8_t*>(link, 0)) {
        vc::Call<void>(link - 4, 0, age);
    }
}

// 0x6714A0 (thiscall): its infos stopped and given back
void __fastcall Batch_Stop(uint8_t* self, re::Edx) {
    for (uint8_t* info = InfoOf(At<uint8_t*>(self, 4)); info; info = InfoOf(At<uint8_t*>(self, 4))) {
        vc::Call<void>(info, 8);
        List_PopFront(self + 4);
        Effects_Free(effectPools_B6F1E0, info + 4);
    }
    for (uint8_t* info = InfoOf(At<uint8_t*>(self, 8)); info; info = InfoOf(At<uint8_t*>(self, 8))) {
        vc::Call<void>(info, 0xC);
        List_PopFront(self + 8);
        Effects_Free(effectPools_B6F1E0, info + 4);
    }
    self[0x10] = 0;
}

// 0x671540 (thiscall): started at `time` with its infos made from the definitions (failing:
// stopped)
uint8_t __fastcall Batch_Init(uint8_t* self, re::Edx, void* render, void* movement, float time) {
    At<float>(self, 0xC) = time;
    self[0x10] = 0;
    if (VirtualList_Create_66A040(self + 4, render) && VirtualList_Create_66A100(self + 8, movement)) {
        return 1;
    }
    Batch_StopFn(self);
    return 0;
}

// ---- movement infos --------------------------------------------------------------------------

// 0x671590 (thiscall): the attractor's definition (0xC): +8 its point (3), +0xA its strength
uint8_t* __fastcall AttractorDefinition_Ctor(uint8_t* self, re::Edx) {
    ListDefinition_Construct(self);
    ValueOverTime_Construct(self + 8);
    ValueOverTime_Construct(self + 0xA);
    ValueOverTime_SetComponents(self + 8, 3);
    ValueOverTime_SetComponents(self + 0xA, 1);
    return self;
}

// 0x6715D0 (thiscall): AttractorMovementInfoDefinitionLoader's vf1
void __fastcall AttractorDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    Loader point(definition + 8);
    Loader strength(definition + 0xA);
    Archive_ReadSection(archive, &point, nameAttractorPoint);
    Archive_ReadSection(archive, &strength, nameAttractorStrength);
}

// The attractor (0x1C): +8 its point, +0x14 its strength, +0x18 / +0x1A their states

// 0x671630 (thiscall)
void __fastcall Attractor_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtAttractor;
    Empty_49A650(self + 0x1A);
    Empty_49A650(self + 0x18);
    MovementInfo_Destruct(self);
}

// 0x671660 (thiscall): vf2, started (failing: stopped)
uint8_t __fastcall Attractor_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    if (ValueOverTimeState_Init(self + 0x18, definition + 8) && ValueOverTimeState_Init(self + 0x1A, definition + 0xA)) {
        At<float>(self, 8) = 0.0f;
        At<float>(self, 0xC) = 0.0f;
        At<float>(self, 0x10) = 0.0f;
        At<float>(self, 0x14) = 0.0f;
        return 1;
    }
    vc::Call<void>(self, 0xC);
    return 0;
}

// 0x6716C0 (thiscall): vf3, stopped
void __fastcall Attractor_Stop(uint8_t* self, re::Edx) {
    ValueOverTimeState_Stop(self + 0x18);
    ValueOverTimeState_Stop(self + 0x1A);
}

// 0x6716E0 (thiscall)
uint8_t* __fastcall Attractor_Ctor(uint8_t* self, re::Edx) {
    MovementInfo_Construct(self);
    At<uint32_t>(self, 0) = kVtAttractor;
    At<float>(self, 8) = 0.0f;
    At<float>(self, 0xC) = 0.0f;
    At<float>(self, 0x10) = 0.0f;
    At<float>(self, 0x14) = 0.0f;
    ValueOverTimeInstance_Construct(self + 0x18);
    ValueOverTimeInstance_Construct(self + 0x1A);
    At<float>(self, 8) = 0.0f;
    At<float>(self, 0xC) = 0.0f;
    At<float>(self, 0x10) = 0.0f;
    return self;
}

// 0x671720 (thiscall): vf0, each particle's velocity pulled towards the point (by its
// direction from it, times the strength)
uint8_t* __fastcall Attractor_Move(uint8_t* self, re::Edx, uint8_t* particle, int count, float) {
    if (count <= 0) {
        return particle;
    }
    do {
        float direction[3];
        VectorSub(direction, reinterpret_cast<float*>(particle), &At<float>(self, 8));
        Vector_Normalise(direction);
        count--;
        float strength = At<float>(self, 0x14);
        float pull[3] = {direction[0] * strength, direction[1] * strength, strength * direction[2]};
        At<float>(particle, 0xC) = At<float>(particle, 0xC) - pull[0];
        At<float>(particle, 0x10) = At<float>(particle, 0x10) - pull[1];
        At<float>(particle, 0x14) = At<float>(particle, 0x14) - pull[2];
        particle = At<uint8_t*>(particle, 0x18);
    } while (count);
    return particle;
}

// 0x6717B0 (thiscall): vf1, its strength (scaled) and point (into the world for a system in
// world coordinates) at a time
void __fastcall Attractor_Update(uint8_t* self, re::Edx, float time, void* transform) {
    ValueOverTimeState_Get(self + 0x1A, &At<float>(self, 0x14), time);
    At<float>(self, 0x14) = float(double(At<float>(self, 0x14)) * kStrengthScale);
    float point[3];
    ValueOverTimeState_Get(self + 0x18, point, time);
    if (!worldCoordinates_1B890B8) {
        At<float>(self, 8) = point[0];
        At<float>(self, 0xC) = point[1];
        At<float>(self, 0x10) = point[2];
        return;
    }
    float v[3] = {point[0], point[1], point[2]};
    float out[3];
    float* world = NiTransform_Transform(transform, out, v);
    v[0] = world[0];
    v[1] = world[1];
    v[2] = world[2];
    At<float>(self, 8) = world[0];
    At<float>(self, 0xC) = world[1];
    At<float>(self, 0x10) = world[2];
}

// 0x671870 (thiscall): a one-value definition (0xC: +8 its value), friction's, gravity's
// and jitter's
uint8_t* __fastcall SingleValueDefinition_Ctor(uint8_t* self, re::Edx) {
    ListDefinition_Construct(self);
    ValueOverTime_Construct(self + 8);
    ValueOverTime_SetComponents(self + 8, 1);
    return self;
}

// 0x6718A0 / 0x671A40 / 0x671B70 (thiscall): Friction / Gravity / Jitter
// MovementInfoDefinitionLoader's vf1: its value
template <uintptr_t Name>
void __fastcall SingleValueDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    Loader value(At<uint8_t*>(self, 4) + 8);
    Archive_ReadSection(archive, &value, *reinterpret_cast<const re::Ptr32<const char>*>(Name));
}

// A one-value info (0x10): +8 its value, +0xC its state

// 0x6718D0 / 0x671A70 / 0x671BA0 (thiscall)
template <uint32_t Vt>
uint8_t* __fastcall SingleValue_Ctor(uint8_t* self, re::Edx) {
    MovementInfo_Construct(self);
    At<float>(self, 8) = 0.0f;
    At<uint32_t>(self, 0) = Vt;
    ValueOverTimeInstance_Construct(self + 0xC);
    return self;
}

// 0x6718F0 / 0x671A90 / 0x671BC0 (thiscall)
template <uint32_t Vt>
void __fastcall SingleValue_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = Vt;
    Empty_49A650(self + 0xC);
    MovementInfo_Destruct(self);
}

// 0x671910 (thiscall): vf1 of friction and jitter, its value at a time
void __fastcall SingleValue_Update(uint8_t* self, re::Edx, float time, void*) {
    ValueOverTimeState_Get(self + 0xC, &At<float>(self, 8), time);
}

// 0x671930 (thiscall): friction's vf0, each particle's velocity multiplied by it (the game
// unrolled 4 times)
uint8_t* __fastcall Friction_Move(uint8_t* self, re::Edx, uint8_t* particle, int count, float) {
    for (int i = 0; i < count; i++) {
        float f = At<float>(self, 8);
        At<float>(particle, 0xC) = At<float>(particle, 0xC) * f;
        At<float>(particle, 0x10) = At<float>(particle, 0x10) * f;
        At<float>(particle, 0x14) = f * At<float>(particle, 0x14);
        particle = At<uint8_t*>(particle, 0x18);
    }
    return particle;
}

// 0x671AB0 (thiscall): gravity's vf1, its strength at a time (scaled)
void __fastcall Gravity_Update(uint8_t* self, re::Edx, float time, void*) {
    ValueOverTimeState_Get(self + 0xC, &At<float>(self, 8), time);
    At<float>(self, 8) = float(double(At<float>(self, 8)) * kStrengthScale);
}

// 0x671AE0 (thiscall): gravity's vf0, taken off each particle's upward velocity (unrolled)
uint8_t* __fastcall Gravity_Move(uint8_t* self, re::Edx, uint8_t* particle, int count, float) {
    for (int i = 0; i < count; i++) {
        At<float>(particle, 0x14) = At<float>(particle, 0x14) - At<float>(self, 8);
        particle = At<uint8_t*>(particle, 0x18);
    }
    return particle;
}

// 0x671B60 (thiscall): vf3 of the one-value infos, stopped
void __fastcall SingleValue_Stop(uint8_t* self, re::Edx) {
    ValueOverTimeState_Stop(self + 0xC);
}

// 0x671BE0 (thiscall): vf2 of the one-value infos, started (its value at 0; failing:
// stopped)
uint8_t __fastcall SingleValue_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    if (!ValueOverTimeState_Init(self + 0xC, definition + 8)) {
        vc::Call<void>(self, 0xC);
        return 0;
    }
    ValueOverTimeState_Get(self + 0xC, &At<float>(self, 8), 0.0f);
    return 1;
}

// 0x671C30 (thiscall): jitter's vf0 (with a strength above 0): each particle's velocity
// turned towards a random direction (by the strength), its speed kept
uint8_t* __fastcall Jitter_Move(uint8_t* self, re::Edx, uint8_t* particle, int count, float) {
    if (!(0.0f < At<float>(self, 8))) {
        return particle;
    }
    for (; count > 0; count--) {
        float vx = At<float>(particle, 0xC);
        float vy = At<float>(particle, 0x10);
        float vz = At<float>(particle, 0x14);
        float sum = (vy * vy + vx * vx) + vz * vz;
        float speed = crt::Sqrtf(std::fabs(sum));   // (the CRT x87 sqrt)
        float v[3] = {0.0f, 0.0f, 0.0f};
        RandomDirection(v);
        float j = speed * At<float>(self, 8);
        v[0] = j * v[0];
        v[1] = v[1] * j;
        v[2] = j * v[2];
        v[0] = At<float>(particle, 0xC) + v[0];
        v[1] = At<float>(particle, 0x10) + v[1];
        v[2] = At<float>(particle, 0x14) + v[2];
        Vector_Normalise(v);
        At<float>(particle, 0xC) = v[0] * speed;
        At<float>(particle, 0x10) = v[1] * speed;
        At<float>(particle, 0x14) = speed * v[2];
        particle = At<uint8_t*>(particle, 0x18);
    }
    return particle;
}

// ---- render infos ----------------------------------------------------------------------------

// 0x671D70 (thiscall): colour and size's definition (0x18): +0xC its colour (3), +0xE alpha,
// +0x10 size, +0x14 how dark it may randomly start (1: never)
uint8_t* __fastcall ColorAndSizeDefinition_Ctor(uint8_t* self, re::Edx) {
    ListDefinition_Construct(self);
    ValueOverTime_Construct(self + 0xC);
    ValueOverTime_Construct(self + 0xE);
    ValueOverTime_Construct(self + 0x10);
    At<float>(self, 0x14) = 1.0f;
    ValueOverTime_SetComponents(self + 0xC, 3);
    ValueOverTime_SetComponents(self + 0xE, 1);
    ValueOverTime_SetComponents(self + 0x10, 1);
    return self;
}

// 0x671DC0 (thiscall): ColorAndSizeRenderInfoDefinitionLoader's vf1
void __fastcall ColorAndSizeDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    Loader color(definition + 0xC);
    Loader alpha(definition + 0xE);
    Loader size(definition + 0x10);
    Archive_ReadSection(archive, &color, nameColor);
    Archive_ReadSection(archive, &alpha, nameAlpha);
    Archive_ReadSection(archive, &size, nameSize);
    vc::Call<uint8_t>(archive, 0x28, definition + 0x14, nameFloatValue);
}

// Colour and size (0x1C): +8 RGBA, +0xC size, +0x10 its brightness, +0x14.. the states

// 0x671E50 (thiscall)
uint8_t* __fastcall ColorAndSize_Ctor(uint8_t* self, re::Edx) {
    RenderInfo_ConstructFn(self);
    At<float>(self, 0xC) = 0.0f;
    At<uint32_t>(self, 0) = kVtColorAndSize;
    At<float>(self, 0x10) = 1.0f;
    ValueOverTimeInstance_Construct(self + 0x14);
    ValueOverTimeInstance_Construct(self + 0x16);
    ValueOverTimeInstance_Construct(self + 0x18);
    self[8] = 0;
    self[9] = 0;
    self[0xA] = 0;
    self[0xB] = 0;
    return self;
}

// 0x671EA0 (thiscall)
void __fastcall ColorAndSize_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtColorAndSize;
    Empty_49A650(self + 0x18);
    Empty_49A650(self + 0x16);
    Empty_49A650(self + 0x14);
    RenderInfo_DestructFn(self);
}

// 0x671ED0 (thiscall): vf0, its size and colour (times its brightness, as bytes) at a time
void __fastcall ColorAndSize_Update(uint8_t* self, re::Edx, float time) {
    ValueOverTimeState_Get(self + 0x18, &At<float>(self, 0xC), time);
    float alpha;
    ValueOverTimeState_Get(self + 0x16, &alpha, time);
    float color[3];
    ValueOverTimeState_Get(self + 0x14, color, time);
    color[0] = color[0] * At<float>(self, 0x10);
    color[1] = color[1] * At<float>(self, 0x10);
    color[2] = color[2] * At<float>(self, 0x10);
    self[8] = uint8_t(int(color[0] * 255.0f));
    self[9] = uint8_t(int(color[1] * 255.0f));
    self[0xA] = uint8_t(int(color[2] * 255.0f));
    self[0xB] = uint8_t(int(255.0f * alpha));
}

// 0x671FF0 (thiscall): vf1, started: a random brightness from the definition's up to 1
// (failing: stopped)
uint8_t __fastcall ColorAndSize_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    if (!ValueOverTimeState_Init(self + 0x14, definition + 0xC) || !ValueOverTimeState_Init(self + 0x16, definition + 0xE) ||
        !ValueOverTimeState_Init(self + 0x18, definition + 0x10)) {
        vc::Call<void>(self, 8);
        return 0;
    }
    float least = At<float>(definition, 0x14);
    At<float>(self, 0x10) = Random() * (1.0f - least) + least;
    vc::Call<void>(self, 0, 0.0f);
    return 1;
}

// 0x672080 (thiscall): vf2, stopped
void __fastcall ColorAndSize_Stop(uint8_t* self, re::Edx) {
    ValueOverTimeState_Stop(self + 0x14);
    ValueOverTimeState_Stop(self + 0x16);
    ValueOverTimeState_Stop(self + 0x18);
}

// 0x6720A0 (thiscall): the 2d rectangle's definition (0x14): +0xC its top left (2), +0xE
// bottom right (2), +0x10 how small it may randomly start (1: never)
uint8_t* __fastcall Rectangle2dDefinition_Ctor(uint8_t* self, re::Edx) {
    ListDefinition_Construct(self);
    ValueOverTime_Construct(self + 0xC);
    ValueOverTime_Construct(self + 0xE);
    ValueOverTime_SetComponents(self + 0xC, 2);
    ValueOverTime_SetComponents(self + 0xE, 2);
    At<float>(self, 0x10) = 1.0f;
    return self;
}

// 0x6720E0 (thiscall): Rectangle2dRenderInfoDefinitionLoader's vf1
void __fastcall Rectangle2dDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    Loader topLeft(definition + 0xC);
    Loader bottomRight(definition + 0xE);
    Archive_ReadSection(archive, &topLeft, nameTopLeft);
    Archive_ReadSection(archive, &bottomRight, nameBottomRight);
    vc::Call<uint8_t>(archive, 0x28, definition + 0x10, nameFloatValue);
}

// The 2d rectangle (0x20): +8 top left, +0x10 bottom right, +0x18 its scale, +0x1C / +0x1E
// the states

// 0x672150 (thiscall)
uint8_t* __fastcall Rectangle2d_Ctor(uint8_t* self, re::Edx) {
    RenderInfo_ConstructFn(self);
    At<float>(self, 8) = 1.0f;
    At<uint32_t>(self, 0) = kVtRectangle2d;
    At<float>(self, 0xC) = -1.0f;
    At<float>(self, 0x10) = -1.0f;
    At<float>(self, 0x14) = 1.0f;
    At<float>(self, 0x18) = 1.0f;
    ValueOverTimeInstance_Construct(self + 0x1C);
    ValueOverTimeInstance_Construct(self + 0x1E);
    return self;
}

// 0x672190 (thiscall)
void __fastcall Rectangle2d_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtRectangle2d;
    Empty_49A650(self + 0x1E);
    Empty_49A650(self + 0x1C);
    RenderInfo_DestructFn(self);
}

// 0x6721C0 (thiscall): vf0, its corners at a time, scaled
void __fastcall Rectangle2d_Update(uint8_t* self, re::Edx, float time) {
    float topLeft[2];
    ValueOverTimeState_Get(self + 0x1C, topLeft, time);
    At<float>(self, 8) = topLeft[0];
    At<float>(self, 0xC) = topLeft[1];
    float bottomRight[2];
    ValueOverTimeState_Get(self + 0x1E, bottomRight, time);
    At<float>(self, 8) = At<float>(self, 0x18) * At<float>(self, 8);
    At<float>(self, 0xC) = At<float>(self, 0xC) * At<float>(self, 0x18);
    At<float>(self, 0x10) = At<float>(self, 0x18) * bottomRight[0];
    At<float>(self, 0x14) = At<float>(self, 0x18) * bottomRight[1];
}

// 0x672230 (thiscall): vf1, started: a random scale from the definition's up to 1
// (failing: stopped)
uint8_t __fastcall Rectangle2d_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    if (!ValueOverTimeState_Init(self + 0x1C, definition + 0xC) || !ValueOverTimeState_Init(self + 0x1E, definition + 0xE)) {
        vc::Call<void>(self, 8);
        return 0;
    }
    float least = At<float>(definition, 0x10);
    At<float>(self, 0x18) = Random() * (1.0f - least) + least;
    vc::Call<void>(self, 0, 0.0f);
    return 1;
}

// 0x6722B0 (thiscall): vf2, stopped
void __fastcall Rectangle2d_Stop(uint8_t* self, re::Edx) {
    ValueOverTimeState_Stop(self + 0x1C);
    ValueOverTimeState_Stop(self + 0x1E);
}

// 0x6722D0 (thiscall): the rotation's definition (0x10): +0xC its angle's change, +0xE bit
// 0 a random start
uint8_t* __fastcall RotationDefinition_Ctor(uint8_t* self, re::Edx) {
    ListDefinition_Construct(self);
    ValueOverTime_Construct(self + 0xC);
    self[0xE] &= 0xFE;
    ValueOverTime_SetComponents(self + 0xC, 1);
    return self;
}

// 0x672300 (thiscall): RotationRenderInfoDefinitionLoader's vf1
void __fastcall RotationDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    Loader delta(definition + 0xC);
    Archive_ReadSection(archive, &delta, nameRotationAngleDelta);
    uint8_t random = 0;
    vc::Call<uint8_t>(archive, 0x3C, &random, nameRotationRandomAngleStart);
    definition[0xE] ^= (definition[0xE] ^ random) & 1;
}

// The rotation (0x1C): +8 its angle, +0xC, +0x10 its sine, +0x14 cosine, +0x18 the state

// 0x672360 (thiscall)
uint8_t* __fastcall Rotation_Ctor(uint8_t* self, re::Edx) {
    RenderInfo_ConstructFn(self);
    At<float>(self, 8) = 0.0f;
    At<float>(self, 0xC) = 0.0f;
    At<uint32_t>(self, 0) = kVtRotation;
    At<float>(self, 0x10) = 0.0f;
    At<float>(self, 0x14) = 1.0f;
    ValueOverTimeInstance_Construct(self + 0x18);
    return self;
}

// 0x672390 (thiscall)
void __fastcall Rotation_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtRotation;
    Empty_49A650(self + 0x18);
    RenderInfo_DestructFn(self);
}

// 0x6723B0 (thiscall): vf2, stopped
void __fastcall Rotation_Stop(uint8_t* self, re::Edx) {
    ValueOverTimeState_Stop(self + 0x18);
}

// 0x6723C0 (thiscall): vf0, turned by its change at a time (degrees, scaled)
void __fastcall Rotation_Update(uint8_t* self, re::Edx, float time) {
    float delta;   // (the game reuses the argument's slot)
    ValueOverTimeState_Get(self + 0x18, &delta, time);
    float radians = delta * 3.14159274f / 180.0f;
    float angle = float(double(radians) * kRotationScale) + At<float>(self, 8);
    At<float>(self, 8) = angle;
    At<float>(self, 0x10) = trk::CrtSin(angle);
    At<float>(self, 0x14) = trk::CrtCos(angle);
}

// 0x672430 (thiscall): vf1, started (at a random angle when so defined; failing: stopped)
uint8_t __fastcall Rotation_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    if (!ValueOverTimeState_Init(self + 0x18, definition + 0xC)) {
        vc::Call<void>(self, 8);
        return 0;
    }
    At<float>(self, 8) = 0.0f;
    if (definition[0xE] & 1) {
        float range = twoPi_B9D1D4;
        At<float>(self, 8) = Random() * (range - 0.0f) + 0.0f;
    }
    At<float>(self, 0x10) = trk::CrtSin(At<float>(self, 8));
    At<float>(self, 0x14) = trk::CrtCos(At<float>(self, 8));
    vc::Call<void>(self, 0, 0.0f);
    return 1;
}

// The UV (0x38): +8 top left, +0x10 bottom left, +0x18 bottom right, +0x20 top right (u, v
// each), +0x28.. their states, +0x30 its definition, +0x34 bits 0..3 its frame. Its
// definition (0x18): +0xC.. the corners over time (2 each), +0x14 bits 0..2 the looping type
// (0 corners over time, 1 loop, 2 once, 3 a random frame, 4 a random frame each update),
// bits 3..6 the frame count

// 0x6724D0 (thiscall): a random frame
void __fastcall UV_RandomFrame(uint8_t* self, re::Edx) {
    int count = (At<uint32_t>(At<uint8_t*>(self, 0x30), 0x14) >> 3) & 0xF;
    float u = Random() * (float(count) - 0.0f) + 0.0f;
    uint32_t frame = uint32_t(int64_t(u));
    At<uint32_t>(self, 0x34) ^= (frame ^ At<uint32_t>(self, 0x34)) & 0xF;
}

// 0x672550 (thiscall)
uint8_t* __fastcall UVDefinition_Ctor(uint8_t* self, re::Edx) {
    ListDefinition_Construct(self);
    ValueOverTime_Construct(self + 0xC);
    ValueOverTime_Construct(self + 0xE);
    ValueOverTime_Construct(self + 0x10);
    ValueOverTime_Construct(self + 0x12);
    At<uint32_t>(self, 0x14) &= 0xFFFFFF80u;
    for (int at = 0xC; at <= 0x12; at += 2) {
        ValueOverTime_SetComponents(self + at, 2);
    }
    return self;
}

// 0x6725C0 (thiscall): UVRenderInfoDefinitionLoader's vf1: the looping type, then the
// corners (type 0) or the frame count
void __fastcall UVDefinitionLoader_Read(uint8_t* self, re::Edx, void* archive) {
    uint8_t* definition = At<uint8_t*>(self, 4);
    int32_t type = 0;
    vc::Call<uint8_t>(archive, 0x38, &type, nameUVAnimationLoopingType);
    At<uint32_t>(definition, 0x14) ^= (At<uint32_t>(definition, 0x14) ^ uint32_t(type)) & 7;
    if (!(definition[0x14] & 7)) {
        Loader topLeft(definition + 0xC);
        Loader bottomLeft(definition + 0xE);
        Loader bottomRight(definition + 0x10);
        Loader topRight(definition + 0x12);
        Archive_ReadSection(archive, &topLeft, nameTopLeft);
        Archive_ReadSection(archive, &bottomLeft, nameBottomLeft);
        Archive_ReadSection(archive, &bottomRight, nameBottomRight);
        Archive_ReadSection(archive, &topRight, nameTopRight);
        return;
    }
    int32_t frames = 0;   // (in the archive argument's slot)
    vc::Call<uint8_t>(archive, 0x38, &frames, nameAnimationFrameCount);
    At<uint32_t>(definition, 0x14) ^= ((uint32_t(frames) * 8) ^ At<uint32_t>(definition, 0x14)) & 0x78;
}

// 0x6726C0 (thiscall)
void __fastcall UV_Dtor(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = kVtUV;
    Empty_49A650(self + 0x2E);
    Empty_49A650(self + 0x2C);
    Empty_49A650(self + 0x2A);
    Empty_49A650(self + 0x28);
    RenderInfo_DestructFn(self);
}

// 0x672700 (thiscall): vf2, stopped
void __fastcall UV_Stop(uint8_t* self, re::Edx) {
    for (int at = 0x28; at <= 0x2E; at += 2) {
        ValueOverTimeState_Stop(self + at);
    }
}

// 0x672730 (thiscall)
uint8_t* __fastcall UV_Ctor(uint8_t* self, re::Edx) {
    RenderInfo_ConstructFn(self);
    At<uint32_t>(self, 0) = kVtUV;
    for (int at = 0x28; at <= 0x2E; at += 2) {
        ValueOverTimeInstance_Construct(self + at);
    }
    At<uint32_t>(self, 0x34) &= 0xFFFFFFF0u;
    At<float>(self, 8) = 0.0f;
    At<uint32_t>(self, 0x30) = 0;
    At<float>(self, 0xC) = 1.0f;
    At<float>(self, 0x18) = 1.0f;
    At<float>(self, 0x20) = 1.0f;
    At<float>(self, 0x24) = 1.0f;
    At<float>(self, 0x10) = 0.0f;
    At<float>(self, 0x14) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    return self;
}

// 0x672790 (thiscall): its corners set to a frame's cell (2 by 2 cells, 3 by 3 above 4
// frames)
void __fastcall UV_SetFrame(uint8_t* self, re::Edx, int frame) {
    int cells = (At<uint8_t*>(self, 0x30)[0x14] & 0x78) > 0x20 ? 3 : 2;
    float n = float(cells);
    int row = frame / cells;
    int column = frame % cells;
    float u0 = float(column) / n;
    At<float>(self, 8) = u0;
    float v0 = float(row) / n;
    float step = 1.0f / n;
    float u1 = u0 + step;
    At<float>(self, 0x18) = u1;
    float v1 = v0 + step;
    At<float>(self, 0xC) = v1;
    At<float>(self, 0x10) = u0;
    At<float>(self, 0x1C) = v0;
    At<float>(self, 0x14) = v0;
    At<float>(self, 0x20) = u1;
    At<float>(self, 0x24) = v1;
}

// 0x672840 (thiscall): vf0, its corners by its looping type
void __fastcall UV_Update(uint8_t* self, re::Edx, float time) {
    uint32_t type = At<uint32_t>(At<uint8_t*>(self, 0x30), 0x14) & 7;
    if (type == 1) {
        UV_SetFrameFn(self, At<uint32_t>(self, 0x34) & 0xF);
        uint32_t f = At<uint32_t>(self, 0x34);
        uint32_t next = ((f + 1) ^ f) & 0xF ^ f;
        At<uint32_t>(self, 0x34) = next;
        uint8_t count = uint8_t((At<uint32_t>(At<uint8_t*>(self, 0x30), 0x14) >> 3) & 0xF);
        if (uint8_t(next & 0xF) < count) {
            return;
        }
        At<uint32_t>(self, 0x34) = next & 0xFFFFFFF0u;
        return;
    }
    if (type == 0) {
        ValueOverTimeState_Get(self + 0x28, &At<float>(self, 8), time);
        ValueOverTimeState_Get(self + 0x2A, &At<float>(self, 0x10), time);
        ValueOverTimeState_Get(self + 0x2C, &At<float>(self, 0x18), time);
        ValueOverTimeState_Get(self + 0x2E, &At<float>(self, 0x20), time);
        return;
    }
    if (type == 4) {
        UV_RandomFrameFn(self);
        UV_SetFrameFn(self, At<uint32_t>(self, 0x34) & 0xF);
        return;
    }
    if (type != 2) {
        return;
    }
    UV_SetFrameFn(self, At<uint32_t>(self, 0x34) & 0xF);
    uint32_t f = At<uint32_t>(self, 0x34);
    uint32_t next = ((f + 1) ^ f) & 0xF ^ f;
    At<uint32_t>(self, 0x34) = next;
    uint32_t frames = At<uint32_t>(At<uint8_t*>(self, 0x30), 0x14) >> 3;
    if (uint8_t(next & 0xF) < uint8_t(frames & 0xF)) {
        return;
    }
    At<uint32_t>(self, 0x34) = ((frames - 1) ^ next) & 0xF ^ next;   // held on the last
}

// 0x672960 (thiscall): vf1, started (type 0: its corners' states; 3 and 4: a random frame)
uint8_t __fastcall UV_Init(uint8_t* self, re::Edx, uint8_t* definition) {
    At<uint32_t>(self, 0x34) &= 0xFFFFFFF0u;
    At<uint8_t*>(self, 0x30) = definition;
    uint32_t type = At<uint32_t>(definition, 0x14) & 7;
    if (type == 0) {
        for (int i = 0; i < 4; i++) {
            if (!ValueOverTimeState_Init(self + 0x28 + i * 2, definition + 0xC + i * 2)) {
                vc::Call<void>(self, 8);
                return 0;
            }
        }
    } else if (type == 3 || type == 4) {
        UV_RandomFrameFn(self);
        UV_SetFrameFn(self, At<uint32_t>(self, 0x34) & 0xF);
    }
    vc::Call<void>(self, 0, 0.0f);
    return 1;
}

} // namespace

#define EFFINFO35_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

EFFINFO35_REVERSED("SourcePoints::Construct_671350", 0x00671350, &SourcePoints_Ctor);
EFFINFO35_REVERSED("RenderInfo::Construct_671370", 0x00671370, &RenderInfo_Ctor);
EFFINFO35_REVERSED("RenderInfo::Destruct_671390", 0x00671390, &RenderInfo_Dtor);
EFFINFO35_REVERSED("ParticleBatch::Construct_6713A0", 0x006713A0, &Batch_Ctor);
EFFINFO35_REVERSED("ParticleBatch::Destruct_6713D0", 0x006713D0, &Batch_Dtor);
EFFINFO35_REVERSED("ParticleBatch::Process_6713F0", 0x006713F0, &Batch_Process);
EFFINFO35_REVERSED("ParticleBatch::Update_671440", 0x00671440, &Batch_Update);
EFFINFO35_REVERSED("ParticleBatch::Stop_6714A0", 0x006714A0, &Batch_Stop);
EFFINFO35_REVERSED("ParticleBatch::Init_671540", 0x00671540, &Batch_Init);
EFFINFO35_REVERSED("AttractorMovementInfoDefinition::Construct_671590", 0x00671590, &AttractorDefinition_Ctor);
EFFINFO35_REVERSED("AttractorMovementInfoDefinitionLoader::Read_6715D0", 0x006715D0, &AttractorDefinitionLoader_Read);
EFFINFO35_REVERSED("AttractorMovementInfo::Destruct_671630", 0x00671630, &Attractor_Dtor);
EFFINFO35_REVERSED("AttractorMovementInfo::Init_671660", 0x00671660, &Attractor_Init);
EFFINFO35_REVERSED("AttractorMovementInfo::Stop_6716C0", 0x006716C0, &Attractor_Stop);
EFFINFO35_REVERSED("AttractorMovementInfo::Construct_6716E0", 0x006716E0, &Attractor_Ctor);
EFFINFO35_REVERSED("AttractorMovementInfo::Move_671720", 0x00671720, &Attractor_Move);
EFFINFO35_REVERSED("AttractorMovementInfo::Update_6717B0", 0x006717B0, &Attractor_Update);
EFFINFO35_REVERSED("SingleValueMovementInfoDefinition::Construct_671870", 0x00671870, &SingleValueDefinition_Ctor);
EFFINFO35_REVERSED("FrictionMovementInfoDefinitionLoader::Read_6718A0", 0x006718A0, &SingleValueDefinitionLoader_Read<0x0093D5BC>);
EFFINFO35_REVERSED("FrictionMovementInfo::Construct_6718D0", 0x006718D0, &SingleValue_Ctor<kVtFriction>);
EFFINFO35_REVERSED("FrictionMovementInfo::Destruct_6718F0", 0x006718F0, &SingleValue_Dtor<kVtFriction>);
EFFINFO35_REVERSED("SingleValueMovementInfo::Update_671910", 0x00671910, &SingleValue_Update);
EFFINFO35_REVERSED("FrictionMovementInfo::Move_671930", 0x00671930, &Friction_Move);
EFFINFO35_REVERSED("GravityMovementInfoDefinitionLoader::Read_671A40", 0x00671A40, &SingleValueDefinitionLoader_Read<0x0093D5C4>);
EFFINFO35_REVERSED("GravityMovementInfo::Construct_671A70", 0x00671A70, &SingleValue_Ctor<kVtGravity>);
EFFINFO35_REVERSED("GravityMovementInfo::Destruct_671A90", 0x00671A90, &SingleValue_Dtor<kVtGravity>);
EFFINFO35_REVERSED("GravityMovementInfo::Update_671AB0", 0x00671AB0, &Gravity_Update);
EFFINFO35_REVERSED("GravityMovementInfo::Move_671AE0", 0x00671AE0, &Gravity_Move);
EFFINFO35_REVERSED("SingleValueMovementInfo::Stop_671B60", 0x00671B60, &SingleValue_Stop);
EFFINFO35_REVERSED("JitterMovementInfoDefinitionLoader::Read_671B70", 0x00671B70, &SingleValueDefinitionLoader_Read<0x0093D5C0>);
EFFINFO35_REVERSED("JitterMovementInfo::Construct_671BA0", 0x00671BA0, &SingleValue_Ctor<kVtJitter>);
EFFINFO35_REVERSED("JitterMovementInfo::Destruct_671BC0", 0x00671BC0, &SingleValue_Dtor<kVtJitter>);
EFFINFO35_REVERSED("SingleValueMovementInfo::Init_671BE0", 0x00671BE0, &SingleValue_Init);
EFFINFO35_REVERSED("JitterMovementInfo::Move_671C30", 0x00671C30, &Jitter_Move);
EFFINFO35_REVERSED("ColorAndSizeRenderInfoDefinition::Construct_671D70", 0x00671D70, &ColorAndSizeDefinition_Ctor);
EFFINFO35_REVERSED("ColorAndSizeRenderInfoDefinitionLoader::Read_671DC0", 0x00671DC0, &ColorAndSizeDefinitionLoader_Read);
EFFINFO35_REVERSED("ColorAndSizeRenderInfo::Construct_671E50", 0x00671E50, &ColorAndSize_Ctor);
EFFINFO35_REVERSED("ColorAndSizeRenderInfo::Destruct_671EA0", 0x00671EA0, &ColorAndSize_Dtor);
EFFINFO35_REVERSED("ColorAndSizeRenderInfo::Update_671ED0", 0x00671ED0, &ColorAndSize_Update);
EFFINFO35_REVERSED("ColorAndSizeRenderInfo::Init_671FF0", 0x00671FF0, &ColorAndSize_Init);
EFFINFO35_REVERSED("ColorAndSizeRenderInfo::Stop_672080", 0x00672080, &ColorAndSize_Stop);
EFFINFO35_REVERSED("Rectangle2dRenderInfoDefinition::Construct_6720A0", 0x006720A0, &Rectangle2dDefinition_Ctor);
EFFINFO35_REVERSED("Rectangle2dRenderInfoDefinitionLoader::Read_6720E0", 0x006720E0, &Rectangle2dDefinitionLoader_Read);
EFFINFO35_REVERSED("Rectangle2dRenderInfo::Construct_672150", 0x00672150, &Rectangle2d_Ctor);
EFFINFO35_REVERSED("Rectangle2dRenderInfo::Destruct_672190", 0x00672190, &Rectangle2d_Dtor);
EFFINFO35_REVERSED("Rectangle2dRenderInfo::Update_6721C0", 0x006721C0, &Rectangle2d_Update);
EFFINFO35_REVERSED("Rectangle2dRenderInfo::Init_672230", 0x00672230, &Rectangle2d_Init);
EFFINFO35_REVERSED("Rectangle2dRenderInfo::Stop_6722B0", 0x006722B0, &Rectangle2d_Stop);
EFFINFO35_REVERSED("RotationRenderInfoDefinition::Construct_6722D0", 0x006722D0, &RotationDefinition_Ctor);
EFFINFO35_REVERSED("RotationRenderInfoDefinitionLoader::Read_672300", 0x00672300, &RotationDefinitionLoader_Read);
EFFINFO35_REVERSED("RotationRenderInfo::Construct_672360", 0x00672360, &Rotation_Ctor);
EFFINFO35_REVERSED("RotationRenderInfo::Destruct_672390", 0x00672390, &Rotation_Dtor);
EFFINFO35_REVERSED("RotationRenderInfo::Stop_6723B0", 0x006723B0, &Rotation_Stop);
EFFINFO35_REVERSED("RotationRenderInfo::Update_6723C0", 0x006723C0, &Rotation_Update);
EFFINFO35_REVERSED("RotationRenderInfo::Init_672430", 0x00672430, &Rotation_Init);
EFFINFO35_REVERSED("UVRenderInfo::RandomFrame_6724D0", 0x006724D0, &UV_RandomFrame);
EFFINFO35_REVERSED("UVRenderInfoDefinition::Construct_672550", 0x00672550, &UVDefinition_Ctor);
EFFINFO35_REVERSED("UVRenderInfoDefinitionLoader::Read_6725C0", 0x006725C0, &UVDefinitionLoader_Read);
EFFINFO35_REVERSED("UVRenderInfo::Destruct_6726C0", 0x006726C0, &UV_Dtor);
EFFINFO35_REVERSED("UVRenderInfo::Stop_672700", 0x00672700, &UV_Stop);
EFFINFO35_REVERSED("UVRenderInfo::Construct_672730", 0x00672730, &UV_Ctor);
EFFINFO35_REVERSED("UVRenderInfo::SetFrame_672790", 0x00672790, &UV_SetFrame);
EFFINFO35_REVERSED("UVRenderInfo::Update_672840", 0x00672840, &UV_Update);
EFFINFO35_REVERSED("UVRenderInfo::Init_672960", 0x00672960, &UV_Init);
