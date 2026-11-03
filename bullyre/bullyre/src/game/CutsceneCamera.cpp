#include "game/Camera.h"

#include <cmath>
#include <cstring>

#include "game/Crt.h"
#include "reversed.h"
// The cutscene camera (CutsceneCameraController, vtable 0x9144D4): key-framed tracks
// read from the cutscene's camera file (position, target, fov, roll, fades), played
// against the cutscene audio's clock, or the view taken from an animated camera clump.
// Also the key reservers and track readers it loads with.

#define CUTSCENECAM_REVERSED(...) REVERSED(__VA_ARGS__)

namespace {

// A key array: {data, capacity, count}
struct CutsceneTrack {
    re::Ptr32<void> data;        // +0
    uint32_t capacity; // +4
    uint32_t count;    // +8
};
static_assert(sizeof(CutsceneTrack) == 0xC);

// A 0x18-byte key (fov and roll): a cubic Hermite curve between keys
struct CutsceneKey18 {
    float time;       // +0 seconds
    float value;      // +4
    float inTangent;  // +8
    float outTangent; // +0xC
    float easeIn;     // +0x10
    float easeOut;    // +0x14
};
static_assert(sizeof(CutsceneKey18) == 0x18);

// A fade key
struct CutsceneFadeKey {
    float time;     // +0
    float fadeIn;   // +4 0: fades out (to 1), else in
    float duration; // +8 seconds
};
static_assert(sizeof(CutsceneFadeKey) == 0xC);

struct CutsceneCameraController {
    re::Ptr32<void> vtable;              // +0 (0x9144D4)
    CutsceneTrack position;    // +4 0x30-byte keys (time first)
    CutsceneTrack target;      // +0x10 0x30-byte keys
    CutsceneTrack fov;         // +0x1C CutsceneKey18
    CutsceneTrack roll;        // +0x28 CutsceneKey18 (degrees)
    CutsceneTrack fades;       // +0x34 CutsceneFadeKey
    int32_t positionKey;       // +0x40
    int32_t targetKey;         // +0x44
    int32_t fovKey;            // +0x48
    int32_t rollKey;           // +0x4C
    int32_t fadeKey;           // +0x50
    int32_t lastFadeKey;       // +0x54
    int32_t done;              // +0x58
    float time;                // +0x5C seconds
    float fadeTime;            // +0x60
    float endTime;             // +0x64 the last position key's time
    cam::CVector unknown68;    // +0x68
    cam::NiTransform view;     // +0x74 what goes on the camera
    cam::CVector lookAt;       // +0xA8
    float fovValue;            // +0xB4
    re::Ptr32<void> clump;               // +0xB8 an animated camera to follow instead (0: the tracks)
};
static_assert(offsetof(CutsceneCameraController, fades) == 0x34);
static_assert(offsetof(CutsceneCameraController, positionKey) == 0x40);
static_assert(offsetof(CutsceneCameraController, unknown68) == 0x68);
static_assert(offsetof(CutsceneCameraController, view) == 0x74);
static_assert(offsetof(CutsceneCameraController, lookAt) == 0xA8);
static_assert(offsetof(CutsceneCameraController, clump) == 0xB8);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* constructor);
GAME_FN(cam::CVector*, __cdecl, VectorSub, 0x00414B70, cam::CVector* out, const cam::CVector* a, const cam::CVector* b);
GAME_FN(cam::CVector*, __cdecl, CrossProduct, 0x00414730, cam::CVector* out, const cam::CVector* a, const cam::CVector* b);
GAME_FN(float, __thiscall, NormaliseSafe, 0x00415130, cam::CVector* v);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(cam::NiTransform*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
template <typename... A>
inline int __cdecl Sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
GAME_FN(uint8_t, __cdecl, ReadLine, 0x0042D330, void* file, char* buffer, int size);
// (the same from a memory stream)
GAME_FN(uint8_t, __cdecl, ReadLineStream, 0x004F7A40, void* stream, char* buffer, int size);
GAME_FN(int32_t, __thiscall, Clock_GetMilliseconds, 0x00595B30, void* clock);
GAME_FN(void*, __thiscall, Clump_GetAnim, 0x006B1F50, void* clump, int index);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0E90, void* manager, void* clump, void* anim, int a, float time, int b, int c);
GAME_FN(void, __thiscall, AnimManager_Update, 0x006C1910, void* manager, void* clump, float a, int b, uintptr_t c);
GAME_FN(float, __cdecl, EaseKey, 0x004F74B0, float t, float easeOut, float easeIn);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, CCamera* camera, float seconds, int direction, int unused);
// the key arrays' reservers ({data, capacity, count}; the flag sets count = capacity)
GAME_FN(void, __thiscall, Track18_Reserve, 0x004F78F0, CutsceneTrack* track, uint32_t count, int setCount);
GAME_FN(void, __thiscall, Track30_ReserveGame, 0x004F7F90, CutsceneTrack* track, uint32_t count, int setCount);
GAME_FN(void, __thiscall, TrackFade_Reserve, 0x004F7990, CutsceneTrack* track, uint32_t count, int setCount);
GAME_FN(void, __thiscall, FreeTracks, 0x004F7AC0, CutsceneCameraController* self);
GAME_FN(void, __thiscall, SetView, 0x004F7B30, CutsceneCameraController* self, const cam::CVector* position, const cam::CVector* target, float roll);

GAME_VAR(void*, cutsceneCameraController, 0x00C3CFC4); // the one there is
GAME_VAR(uint8_t, fovLocked, 0x00C3CF82);               // TheCamera+0x31A: nothing touches the fov
GAME_VAR(uint8_t, haveCutsceneName, 0x020C5BE6);
inline char* const cutsceneName = reinterpret_cast<char*>(0x020C5B20);
inline void* const soundClock = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);

constexpr uintptr_t kCutsceneCameraControllerVtable = 0x009144D4;
constexpr uintptr_t kCameraControllerVtable = 0x00914160;

cam::NiTransform* CameraTransform(void* camera) {
    return GetLocalTransform(GetCameraNode(camera));
}

// A clump's frame: its node's world translate (+0x8C) and the point it looks at (+0x58)
void* ClumpNode(void* clump) {
    return *reinterpret_cast<re::Ptr32<void>*>(*reinterpret_cast<re::Ptr32<uint8_t>*>(static_cast<uint8_t*>(clump) + 0x14) + 8);
}
cam::CVector NodeVector(void* node, int offset) {
    return *reinterpret_cast<cam::CVector*>(static_cast<uint8_t*>(node) + offset);
}

// ---- the register-argument helpers, called at the game's address

// 0x4F75A0 / 0x4F7D80 / 0x4F75E0: (edx) key index, (esi) track, t
__declspec(noinline) void CallAdvance(uintptr_t fn, int32_t* index, CutsceneTrack* track, float t) {
#if defined(_M_IX86)
    __asm {
        mov edx, index
        mov esi, track
        push t
        call fn
        add esp, 4
    }
#else
    re::Ours<void (*)(int32_t*, CutsceneTrack*, float)>(fn)(index, track, t);
#endif
}

// 0x4F7620 (a vector) / 0x4F7DC0 (a float): (eax) key index, (edx) track, (edi) out, t
__declspec(noinline) void CallEvaluate(uintptr_t fn, int32_t index, CutsceneTrack* track, void* result, float t) {
#if defined(_M_IX86)
    __asm {
        mov eax, index
        mov edx, track
        mov edi, result
        push t
        call fn
        add esp, 4
    }
#else
    re::Ours<void (*)(int32_t, CutsceneTrack*, void*, float)>(fn)(index, track, result, t);
#endif
}

// 0x4F8350 / 0x4F8450 / 0x4F8570 / 0x4F8690: (ebx) file or stream, (edi) track, flag
__declspec(noinline) uint8_t CallReadTrack(uintptr_t fn, void* file, CutsceneTrack* track, int flag) {
#if defined(_M_IX86)
    uint8_t result;
    __asm {
        mov ebx, file
        mov edi, track
        push flag
        call fn
        add esp, 4
        mov result, al
    }
    return result;
#else
    return uint8_t(re::Ours<uint32_t (*)(void*, CutsceneTrack*, int)>(fn)(file, track, flag));
#endif
}

} // namespace

// 0x4F7B30 (thiscall): the view from a position, a target and a roll (radians, about
// the look direction: pi/2 is upright)
void __fastcall CutsceneCameraController_SetView(CutsceneCameraController* self, re::Edx, const cam::CVector* position,
                                                 const cam::CVector* target, float roll) {
    cam::CVector up;
    up.x = float(std::cos(double(roll)));
    up.y = 0.0f;
    up.z = float(std::sin(double(roll)));
    cam::CVector tmp;
    cam::CVector forward = *VectorSub(&tmp, target, position);
    NormaliseSafe(&forward);
    cam::CVector right = *CrossProduct(&tmp, &up, &forward);
    NormaliseSafe(&right);
    cam::CVector up2 = *CrossProduct(&tmp, &forward, &right);
    NormaliseSafe(&up2);
    // (columns: right, up, forward)
    self->view.rotate[1] = up2.x;
    self->view.rotate[4] = up2.y;
    self->view.rotate[7] = up2.z;
    self->view.rotate[2] = forward.x;
    self->view.rotate[5] = forward.y;
    self->view.rotate[8] = forward.z;
    self->view.rotate[0] = right.x;
    self->view.rotate[3] = right.y;
    self->view.rotate[6] = right.z;
    self->view.translate = *position;
    if (fovLocked == 0) {
        TheCamera.fovOverridden = 0;
        TheCamera.fov = self->fovValue;
    }
    *CameraTransform(TheCamera.camera) = self->view;
}
CUTSCENECAM_REVERSED("CutsceneCameraController::SetView", 0x004F7B30, RE_BODY(&CutsceneCameraController_SetView), nullptr, nullptr, re::Mode::Original);

// 0x4F7D10 (cdecl): the view from an animated camera clump's frame
void __cdecl CutsceneCameraController_ViewFromClump(void* clump, CutsceneCameraController* self) {
    cam::CVector target = NodeVector(ClumpNode(clump), 0x58);
    cam::CVector position = NodeVector(ClumpNode(clump), 0x8C);
    SetView(self, &position, &target, 0.0f);
}
CUTSCENECAM_REVERSED("CutsceneCameraController::ViewFromClump", 0x004F7D10, &CutsceneCameraController_ViewFromClump, nullptr, nullptr, re::Mode::Original);

// 0x4F7D80 ((edx) index, (esi) track, t): moves the key index on to the last 0x18-byte
// key at or before t
void AdvanceKey18(int32_t* index, CutsceneTrack* track, float t) {
    const CutsceneKey18* keys = static_cast<const CutsceneKey18*>(track->data);
    while (uint32_t(*index + 1) < track->count && keys[*index + 1].time <= t) {
        (*index)++;
    }
}
RE_UNHOOKED_FN("CutsceneCameraController::AdvanceKey18", 0x004F7D80, &AdvanceKey18);

// 0x4F7DC0 ((eax) index, (edx) track, (edi) out, t): a 0x18-byte track's value at t
// (nothing written for an empty track). Cutscene "1-01" plays keys closer together.
void EvaluateKey18(int32_t index, const CutsceneTrack* track, float* out, float t) {
    if (track->count == 0) {
        return;
    }
    const CutsceneKey18* key = static_cast<const CutsceneKey18*>(track->data) + index;
    if (uint32_t(index + 1) < track->count) {
        const CutsceneKey18* next = key + 1;
        const char* name = haveCutsceneName ? cutsceneName : nullptr;
        double span;
        bool curve = false;
        if (name && Stricmp(name, "1-01") == 0) {
            if (key->time < t) {
                span = double(next->time) - double(key->time);
                // (the span compared as a float, divided by at full precision)
                float spanF = float(span);
                if (spanF >= 0.06666667f) {
                    curve = true;
                } else if (spanF >= 0.016666668f) {
                    float u = float((double(t) - key->time) / span);
                    float off = float(double(u) - 1.0);
                    off = std::fabs(off);
                    if (off <= 1e-05f) {
                        *out = next->value;
                    } else {
                        *out = float((1.0 - u) * key->value + double(u) * next->value);
                    }
                    return;
                }
            }
        } else if (key->time < t) {
            span = double(next->time) - double(key->time);
            if (0.4000000059604645 < span) {
                curve = true;
            }
        }
        if (curve) {
            float s = EaseKey(float((double(t) - key->time) / span), key->easeOut, next->easeIn);
            float s2 = float(double(s) * s);
            float s3 = float(double(s2) * s);
            float h00 = float(2.0 * s3 - 3.0 * s2 + 1.0);
            float h01 = float(3.0 * s2 - 2.0 * s3);
            float h10 = float(double(s3) - 2.0 * s2 + s);
            float h11 = float(double(s3) - s2);
            *out = float(double(h00) * key->value + double(h01) * next->value + double(h10) * key->outTangent +
                         double(h11) * next->inTangent);
            return;
        }
    }
    *out = key->value;
}
RE_UNHOOKED_FN("CutsceneCameraController::EvaluateKey18", 0x004F7DC0, &EvaluateKey18);

// 0x4F7F90 (thiscall): room for `count` 0x30-byte keys (the old ones copied over; the
// flag: that many in use)
void __fastcall Track30_Reserve(CutsceneTrack* self, re::Edx, uint32_t count, int setCount) {
    void* data = nullptr;
    self->capacity = count;
    if (count != 0) {
        // (the size saturates on overflow)
        uint64_t size = uint64_t(count) * 0x30;
        data = Malloc(size > 0xFFFFFFFFull ? 0xFFFFFFFFu : size_t(size));
        if (data) {
            ConstructArray(data, 0x30, int(count), reinterpret_cast<void*>(0x004F7A20));
        } else {
            data = nullptr;
        }
    }
    if (self->data) {
        for (uint32_t i = 0; i < self->count; i++) {
            std::memcpy(static_cast<uint8_t*>(data) + i * 0x30, static_cast<uint8_t*>(self->data) + i * 0x30, 0x30);
        }
        Free(self->data);
    }
    self->data = data;
    if (uint8_t(setCount) != 0) {
        self->count = self->capacity;
    }
}
CUTSCENECAM_REVERSED("CutsceneTrack::Reserve30", 0x004F7F90, RE_BODY(&Track30_Reserve), nullptr, nullptr, re::Mode::Original);

// 0x4F8030 (thiscall)
void __fastcall CutsceneCameraController_Destruct(CutsceneCameraController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kCutsceneCameraControllerVtable);
    FreeTracks(self);
    cutsceneCameraController = nullptr;
    CutsceneTrack* tracks[] = {&self->fades, &self->roll, &self->fov, &self->target, &self->position};
    for (CutsceneTrack* track : tracks) {
        if (track->data) {
            Free(track->data);
            track->data = nullptr;
        }
    }
    self->vtable = reinterpret_cast<void*>(kCameraControllerVtable);
}
CUTSCENECAM_REVERSED("CutsceneCameraController::~CutsceneCameraController", 0x004F8030, RE_BODY(&CutsceneCameraController_Destruct), nullptr, nullptr, re::Mode::Original);

// 0x4F80B0 (vtable[7], process): the tracks (or the clump) at the cutscene audio's time
void __fastcall CutsceneCameraController_Process(CutsceneCameraController* self, re::Edx) {
    if (self->done != 0) {
        return;
    }
    cam::CVector target = {0.0f, 0.0f, 0.0f};
    cam::CVector position = {0.0f, 0.0f, 0.0f};
    int32_t ms = Clock_GetMilliseconds(soundClock);
    // (the fov is read into the slot the clock's value was in: an empty fov track
    // leaves the milliseconds' bits there as the fov)
    union {
        int32_t i;
        float f;
    } fovSlot;
    fovSlot.i = ms;
    float time = float(double(uint32_t(ms)) / 1000.0);
    int32_t oldPositionKey = self->positionKey;
    self->time = time;
    self->fadeTime = time;
    CallAdvance(0x004F75A0, &self->positionKey, &self->position, time);
    CallAdvance(0x004F75A0, &self->targetKey, &self->target, self->time);
    CallAdvance(0x004F7D80, &self->rollKey, &self->roll, self->time);
    CallAdvance(0x004F7D80, &self->fovKey, &self->fov, self->time);
    if (self->fades.count != 0) {
        if (oldPositionKey != self->positionKey) {
            // (a new position key: fades are timed from it)
            self->fadeTime = *reinterpret_cast<float*>(static_cast<uint8_t*>(self->position.data) + self->positionKey * 0x30);
        }
        CallAdvance(0x004F75E0, &self->fadeKey, &self->fades, self->fadeTime);
    }
    if (self->clump) {
        void* anim = Clump_GetAnim(self->clump, 0);
        AnimManager_Play(animManager, self->clump, anim, 0, self->time, 0, 0);
        AnimManager_Update(animManager, self->clump, 0.0f, 1, 0);
        target = NodeVector(ClumpNode(self->clump), 0x58);
        position = NodeVector(ClumpNode(self->clump), 0x8C);
    } else {
        CallEvaluate(0x004F7620, self->positionKey, &self->position, &position, self->time);
        CallEvaluate(0x004F7620, self->targetKey, &self->target, &target, self->time);
    }
    float roll = 0.0f;
    CallEvaluate(0x004F7DC0, self->rollKey, &self->roll, &roll, self->time);
    CallEvaluate(0x004F7DC0, self->fovKey, &self->fov, &fovSlot.f, self->time);
    if (self->fades.count != 0 && self->lastFadeKey != self->fadeKey) {
        self->lastFadeKey = self->fadeKey;
        const CutsceneFadeKey& fade = static_cast<const CutsceneFadeKey*>(self->fades.data)[self->fadeKey];
        Camera_Fade(&TheCamera, fade.duration, fade.fadeIn == 0.0f ? 1 : 0, 0);
    }
    self->lookAt = target;
    // (degrees to radians, a quarter turn on: 0 is upright)
    float radians = float((double(roll) + 90.0) * 3.1415927410125732 / 180.0);
    self->fovValue = fovSlot.f;
    SetView(self, &position, &target, radians);
    if (self->endTime <= self->time && self->positionKey == int32_t(self->position.count) - 1) {
        self->done = 1;
    }
}
CUTSCENECAM_REVERSED("CutsceneCameraController::Process", 0x004F80B0, RE_BODY(&CutsceneCameraController_Process), nullptr, nullptr, re::Mode::Original);

namespace {

// 0x4F8350 / 0x4F8450 / 0x4F8570 / 0x4F8690 ((ebx) file or stream, (edi) track, flag):
// a track: "count," then a line per key ("%f," x6 for 0x18-byte keys, x12 for 0x30-byte
// ones) and a line starting ';'. An empty track with the flag set ends at the count.
// (a key line not matching returns the scanned count with its low byte cleared)
template <int kFloats>
uint32_t ReadTrack(uint8_t(__cdecl* readLine)(void*, char*, int), void* file, CutsceneTrack* track, int flag) {
    char line[0xC0];
    readLine(file, line, 0xC0);
    uint32_t count = 0;
    Sscanf(line, "%d,", &count);
    if (count > track->capacity) {
        if (kFloats == 6) {
            Track18_Reserve(track, count, 1);
        } else {
            Track30_ReserveGame(track, count, 1);
        }
    }
    if (count == 0 && uint8_t(flag) != 0) {
        return 1;
    }
    for (int i = 0; i < int(count); i++) {
        readLine(file, line, 0xC0);
        float* k = reinterpret_cast<float*>(static_cast<uint8_t*>(track->data) + i * kFloats * 4);
        int read;
        if (kFloats == 6) {
            read = Sscanf(line, "%f,%f,%f,%f,%f,%f,", k, k + 1, k + 2, k + 3, k + 4, k + 5);
        } else {
            read = Sscanf(line, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,", k, k + 1, k + 2, k + 3, k + 4, k + 5, k + 6,
                          k + 7, k + 8, k + 9, k + 10, k + 11);
        }
        if (read != kFloats) {
            return uint32_t(read) & 0xFFFFFF00u;
        }
    }
    if (readLine(file, line, 0xC0) && line[0] == ';') {
        return 1;
    }
    return 0;
}

} // namespace

uint32_t ReadTrack18File(void* file, CutsceneTrack* track, int flag) {
    return ReadTrack<6>(ReadLine, file, track, flag);
}
RE_UNHOOKED_FN("CutsceneCameraController::ReadTrack18File", 0x004F8350, &ReadTrack18File);

uint32_t ReadTrack18Stream(void* stream, CutsceneTrack* track, int flag) {
    return ReadTrack<6>(ReadLineStream, stream, track, flag);
}
RE_UNHOOKED_FN("CutsceneCameraController::ReadTrack18Stream", 0x004F8450, &ReadTrack18Stream);

uint32_t ReadTrack30File(void* file, CutsceneTrack* track, int flag) {
    return ReadTrack<12>(ReadLine, file, track, flag);
}
RE_UNHOOKED_FN("CutsceneCameraController::ReadTrack30File", 0x004F8570, &ReadTrack30File);

uint32_t ReadTrack30Stream(void* stream, CutsceneTrack* track, int flag) {
    return ReadTrack<12>(ReadLineStream, stream, track, flag);
}
RE_UNHOOKED_FN("CutsceneCameraController::ReadTrack30Stream", 0x004F8690, &ReadTrack30Stream);

// 0x4F87B0 (thiscall)
CutsceneCameraController* __fastcall CutsceneCameraController_Construct(CutsceneCameraController* self, re::Edx) {
    self->vtable = reinterpret_cast<void*>(kCutsceneCameraControllerVtable);
    CutsceneTrack* tracks[] = {&self->position, &self->target, &self->fov, &self->roll, &self->fades};
    for (CutsceneTrack* track : tracks) {
        track->data = nullptr;
        track->capacity = 0;
        track->count = 0;
    }
    self->unknown68 = {0.0f, 0.0f, 0.0f};
    self->view.scale = 1.0f;
    self->lookAt = {0.0f, 0.0f, 0.0f};
    cutsceneCameraController = self;
    self->clump = nullptr;
    return self;
}
CUTSCENECAM_REVERSED("CutsceneCameraController::CutsceneCameraController", 0x004F87B0, RE_BODY(&CutsceneCameraController_Construct), nullptr, nullptr, re::Mode::Original);

namespace {

// 0x4F8820 / 0x4F8980: the tracks (fov, roll, position, target, then the fades)
uint8_t LoadTracks(CutsceneCameraController* self, void* file, uintptr_t read18, uintptr_t read30,
                   uint8_t(__cdecl* readLine)(void*, char*, int)) {
    FreeTracks(self);
    if (!CallReadTrack(read18, file, &self->fov, 0) || !CallReadTrack(read18, file, &self->roll, 0) ||
        !CallReadTrack(read30, file, &self->position, 0) || !CallReadTrack(read30, file, &self->target, 0)) {
        return 0;
    }
    char line[0xC0];
    readLine(file, line, 0xC0);
    uint32_t count = 0;
    Sscanf(line, "%d,", &count);
    if (count > self->fades.capacity) {
        TrackFade_Reserve(&self->fades, count, 1);
    }
    if (count != 0) {
        for (int i = 0; i < int(count); i++) {
            readLine(file, line, 0xC0);
            CutsceneFadeKey* key = static_cast<CutsceneFadeKey*>(self->fades.data) + i;
            if (Sscanf(line, "%f,%f,%f", &key->time, &key->fadeIn, &key->duration) != 3) {
                return 0;
            }
        }
        if (!readLine(file, line, 0xC0) || line[0] != ';') {
            return 0;
        }
    }
    if (self->position.count != 0) {
        self->endTime = *reinterpret_cast<float*>(static_cast<uint8_t*>(self->position.data) + self->position.count * 0x30 - 0x30);
    }
    return 1;
}

} // namespace

// 0x4F8820 (thiscall): from a file
uint8_t __fastcall CutsceneCameraController_LoadFile(CutsceneCameraController* self, re::Edx, void* file) {
    return LoadTracks(self, file, 0x004F8350, 0x004F8570, ReadLine);
}
CUTSCENECAM_REVERSED("CutsceneCameraController::LoadFile", 0x004F8820, RE_BODY(&CutsceneCameraController_LoadFile), nullptr, nullptr, re::Mode::Original);

// 0x4F8980 (thiscall): from a memory stream
uint8_t __fastcall CutsceneCameraController_LoadStream(CutsceneCameraController* self, re::Edx, void* stream) {
    return LoadTracks(self, stream, 0x004F8450, 0x004F8690, ReadLineStream);
}
CUTSCENECAM_REVERSED("CutsceneCameraController::LoadStream", 0x004F8980, RE_BODY(&CutsceneCameraController_LoadStream), nullptr, nullptr, re::Mode::Original);
