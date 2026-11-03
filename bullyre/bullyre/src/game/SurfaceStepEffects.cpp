// The footstep effects by surface type (0x20C8BA0, from the "SurfaceStepEffect" sections
// of a data file): per surface a step effect, a slow speed multiplier, a winter (snow
// footprint) effect and a trail (leaving footprint) effect. 0x72E350..0x72E600.

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

constexpr int kSurfaceTypes = 0x43;

struct SurfaceStepEffects {
    int32_t step[kSurfaceTypes];       // +0x0 (an effect id, -1: none)
    float slowSpeed[kSurfaceTypes];    // +0x10C
    int32_t winter[kSurfaceTypes];     // +0x218
    int32_t trail[kSurfaceTypes];      // +0x324
};
static_assert(sizeof(SurfaceStepEffects) == 0x430);

GAME_VAR(SurfaceStepEffects, stepEffects, 0x020C8BA0);
GAME_VAR(void*, sseEffectLibrary, 0x00B6F1E8);
constexpr uint32_t kVtStepEffectLoader = 0x0094D0C8;

// a string the reader fills (up to `capacity`)
struct SseString {
    re::Ptr32<char> text;          // +0x0
    int32_t capacity;    // +0x4
    char buffer[0x68];   // +0x8
};
static_assert(sizeof(SseString) == 0x70);

GAME_FN(void*, __cdecl, SseArchive_Open, 0x005B07E0, const char* path, int32_t* out, int flag);
GAME_FN(uint8_t, __thiscall, SseArchive_ReadSection, 0x005B0720, void* archive, void* loader, const char* name);
GAME_FN(void, __cdecl, SseArchive_Close, 0x005B0860, void* archive);
GAME_FN(int, __cdecl, SseSurfaceFromName, 0x0056EB60, const char* name);
GAME_FN(uint16_t, __thiscall, SseEffectLibrary_Find, 0x00667B60, void* library, const char* name);

} // namespace

namespace sse {

// 0x72E350 (thiscall): the constructor (no effects, speed 1)
SurfaceStepEffects* __fastcall Construct(SurfaceStepEffects* self, re::Edx) {
    for (int i = 0; i < kSurfaceTypes; i++) {
        self->slowSpeed[i] = 1.0f;
        self->step[i] = -1;
        self->winter[i] = -1;
        self->trail[i] = -1;
    }
    return self;
}

// 0x72E390: the "SurfaceStepEffect" sections of a file read
void __fastcall Load(void*, re::Edx, const char* path) {
    int32_t out = 0;
    void* archive = SseArchive_Open(path, &out, 1);
    uint32_t loader;   // (the loader is just its vtable)
    do {
        loader = kVtStepEffectLoader;
    } while (SseArchive_ReadSection(archive, &loader, "SurfaceStepEffect"));
    SseArchive_Close(archive);
}

// 0x72E3E0 (SurfaceTypeStepEffectLoader vf1, thiscall): one section: the surface's
// effects by name (an empty name leaves the effect as it was)
void __fastcall Loader_Read(void*, re::Edx, void* reader) {
    SseString surface, step, winter, trail;
    surface.text = surface.buffer;
    surface.capacity = 0x65;
    surface.buffer[0] = 0;
    vc::Call<void>(reader, 0x24, &surface, 0x65, "SurfaceName");
    step.text = step.buffer;
    step.capacity = 0x65;
    step.buffer[0] = 0;
    vc::Call<void>(reader, 0x24, &step, 0x65, "StepEffectName");
    winter.text = winter.buffer;
    winter.capacity = 0x65;
    winter.buffer[0] = 0;
    vc::Call<void>(reader, 0x24, &winter, 0x65, "SnowFootprintEffectName");
    trail.text = trail.buffer;
    trail.capacity = 0x65;
    trail.buffer[0] = 0;
    vc::Call<void>(reader, 0x24, &trail, 0x65, "LeavingFootprintEffectName");
    float slow = 1.0f;
    vc::Call<void>(reader, 0x28, &slow, "SlowSpeedMultiply");
    int type = SseSurfaceFromName(surface.text);
    stepEffects.slowSpeed[type] = slow;
    if (step.text[0] != '\0') {
        stepEffects.step[type] = SseEffectLibrary_Find(sseEffectLibrary, step.text);
    }
    if (winter.text[0] != '\0') {
        stepEffects.winter[type] = SseEffectLibrary_Find(sseEffectLibrary, winter.text);
    }
    if (trail.text[0] != '\0') {
        stepEffects.trail[type] = SseEffectLibrary_Find(sseEffectLibrary, trail.text);
    }
}

// 0x72E580..0x72E600 (thiscall): a surface's effects (ids are 16-bit; "Has": not -1)
uint8_t __fastcall HasStep(SurfaceStepEffects* self, re::Edx, int type) {
    return self->step[type] != -1;
}
uint16_t __fastcall Step(SurfaceStepEffects* self, re::Edx, int type) {
    return uint16_t(self->step[type]);
}
float __fastcall SlowSpeed(SurfaceStepEffects* self, re::Edx, int type) {
    return self->slowSpeed[type];
}
uint8_t __fastcall HasWinter(SurfaceStepEffects* self, re::Edx, int type) {
    return self->winter[type] != -1;
}
uint16_t __fastcall Winter(SurfaceStepEffects* self, re::Edx, int type) {
    return uint16_t(self->winter[type]);
}
uint8_t __fastcall HasTrail(SurfaceStepEffects* self, re::Edx, int type) {
    return self->trail[type] != -1;
}
uint16_t __fastcall Trail(SurfaceStepEffects* self, re::Edx, int type) {
    return uint16_t(self->trail[type]);
}

} // namespace sse

#define SSE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

SSE_REVERSED("SurfaceStepEffects::Construct", 0x0072E350, &sse::Construct);
SSE_REVERSED("SurfaceStepEffects::Load", 0x0072E390, &sse::Load);
SSE_REVERSED("SurfaceTypeStepEffectLoader::Read", 0x0072E3E0, &sse::Loader_Read);
SSE_REVERSED("SurfaceStepEffects::HasStep", 0x0072E580, &sse::HasStep);
SSE_REVERSED("SurfaceStepEffects::Step", 0x0072E590, &sse::Step);
SSE_REVERSED("SurfaceStepEffects::SlowSpeed", 0x0072E5A0, &sse::SlowSpeed);
SSE_REVERSED("SurfaceStepEffects::HasWinter", 0x0072E5B0, &sse::HasWinter);
SSE_REVERSED("SurfaceStepEffects::Winter", 0x0072E5D0, &sse::Winter);
SSE_REVERSED("SurfaceStepEffects::HasTrail", 0x0072E5E0, &sse::HasTrail);
SSE_REVERSED("SurfaceStepEffects::Trail", 0x0072E600, &sse::Trail);
