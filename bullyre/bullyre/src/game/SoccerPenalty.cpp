// The soccer penalty minigame (SoccerP, game mode 0xC, vtable 0x949E00; 0x713E70..0x716036):
// kicking balls at targets in a goal; the aim (a point bouncing about the goal area, steered
// with the stick), two meters (the kick's power along the goal, its height), the goalkeeper
// ped, the score pop-ups, and its script natives.
//
// SoccerP (0xAD0): the Minigame base; +0xC4..+0xD8 the health bars' positions; +0xE0 the
// balls to kick; +0xE4 kicked; +0xE8 [2] the meters stopped, +0xEC [2] their speeds, +0xF4
// [2] (2.0), +0xFC [2]; +0x104 / +0x2EC the power and height meters' shapes, +0x4D4 /
// +0x6BC two more (0x1E8 each: HudTaggingMeter.cpp's shape; +0x1C4 its value 0..1);
// +0x8A4 [2] the kick's flash; +0x8B0 the targets (+0x8B4 positions, +0x944 sizes, +0x9A4
// points; 12); +0x9D4 the goal area set, +0x9D8 / +0x9E4 its corners, +0x9F0 its height,
// +0x9F4 its width; +0x9F8 the ball projectile, +0x9FC the goalkeeper (registered
// references); +0xA00 the stick used; +0xA04 five score pop-ups (0x24); +0xAB8 the stick,
// +0xAC0 the aim's velocity, +0xAC8 its offset in the goal (from the middle).
// The x87 runs at 24-bit precision here (the main thread): float, each step rounded.

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct SpVec {
    float x, y, z;
};
static_assert(sizeof(SpVec) == 0xC);

struct SpRgba {
    uint8_t r, g, b, a;
};

// a meter's shape (HudTaggingMeter.cpp's; only what is set here)
struct SpShape {
    float progress;          // +0x0
    int32_t steps;           // +0x4
    float corners[8];        // +0x8 four (x, y)
    float angleFrom;         // +0x28
    float angleTo;           // +0x2C
    int32_t segments;        // +0x30
    uint8_t segmentData[0x190];
    float value;             // +0x1C4 0..1
    float speed;             // +0x1C8
    SpRgba fill;             // +0x1CC
    SpRgba fillFaded;        // +0x1D0
    float outlineWidth;      // +0x1D4
    SpRgba outline;          // +0x1D8
    uint8_t unknown1DC[8];
    int32_t mode;            // +0x1E4 (2: a ribbon)
};
static_assert(sizeof(SpShape) == 0x1E8);
static_assert(offsetof(SpShape, value) == 0x1C4);

// a score pop-up
struct SpMessage {
    int32_t kind;      // +0x0
    float timer;       // +0x4 seconds left (2)
    float unknown8;    // +0x8
    uint8_t unknownC[8];
    SpVec pos;         // +0x14
    uint8_t active;    // +0x20
    uint8_t pad21[3];
};
static_assert(sizeof(SpMessage) == 0x24);

struct SoccerP {
    re::Ptr32<re::Ptr32<void>> vtable;               // +0x0
    uint8_t active;              // +0x4
    uint8_t hud;                 // +0x5
    uint8_t success;             // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<re::Ptr32<void>> textures;             // +0x1C
    uint8_t unknown20[0x18];
    int32_t kind38;              // +0x38 (4)
    int32_t mode;                // +0x3C (0xC)
    uint8_t unknown40[0x10];
    int32_t popupKind;           // +0x50 (the popup's start)
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;        // +0xA0
    uint8_t unknownA1[0x23];
    float healthBar[4];          // +0xC4
    float pedsHealthBar[2];      // +0xD4
    float unknownDC;             // +0xDC
    int32_t balls;               // +0xE0
    int32_t kicked;              // +0xE4
    uint8_t meterStopped[2];     // +0xE8
    uint8_t padEA[2];
    float meterSpeed[2];         // +0xEC
    float unknownF4[2];          // +0xF4 (2.0)
    float unknownFC[2];          // +0xFC
    SpShape power;               // +0x104
    SpShape height;              // +0x2EC
    SpShape bars[2];             // +0x4D4
    float flash[2];              // +0x8A4
    int32_t unknown8AC;          // +0x8AC
    int32_t targetCount;         // +0x8B0
    SpVec targets[12];           // +0x8B4
    float targetSize[12][2];     // +0x944 width, height
    int32_t targetPoints[12];    // +0x9A4
    uint8_t goalSet;             // +0x9D4
    uint8_t pad9D5[3];
    SpVec goalA;                 // +0x9D8
    SpVec goalB;                 // +0x9E4
    float goalHeight;            // +0x9F0
    float goalWidth;             // +0x9F4
    re::Ptr32<uint8_t> ball;               // +0x9F8 (a registered reference)
    re::Ptr32<uint8_t> keeper;             // +0x9FC (a registered reference)
    uint8_t stickUsed;           // +0xA00
    uint8_t padA01[3];
    SpMessage messages[5];       // +0xA04
    float stick[2];              // +0xAB8
    float velocity[2];           // +0xAC0
    float aim[2];                // +0xAC8
};
static_assert(offsetof(SoccerP, healthBar) == 0xC4);
static_assert(offsetof(SoccerP, power) == 0x104);
static_assert(offsetof(SoccerP, bars) == 0x4D4);
static_assert(offsetof(SoccerP, targetCount) == 0x8B0);
static_assert(offsetof(SoccerP, goalSet) == 0x9D4);
static_assert(offsetof(SoccerP, keeper) == 0x9FC);
static_assert(offsetof(SoccerP, messages) == 0xA04);
static_assert(offsetof(SoccerP, aim) == 0xAC8);
static_assert(sizeof(SoccerP) == 0xAD0);

// RwIm3DVertex
struct SpVertex {
    float x, y, z;
    float nx, ny, nz;
    uint32_t colour;
    float u, v;
};
static_assert(sizeof(SpVertex) == 0x24);

GAME_VAR(SoccerP*, currentSoccer, 0x020C8704);
GAME_VAR(float, timeStepSp, 0x00C1A9A4);
GAME_VAR(float, popupRate, 0x00BC7154);           // the pop-ups' fade rate (0.02)
GAME_VAR(float, screenScaleXSp, 0x00A957A8);
GAME_VAR(float, screenScaleYSp, 0x00A957AC);
GAME_VAR(int32_t, userStartResponseSp, 0x020C87AC);
GAME_VAR(void*, musicChannelSp, 0x00CF4288);
GAME_VAR(CPool<uint8_t>*, pedPoolSp, 0x00C0F5F0);
inline void* const missionTimerSp = reinterpret_cast<void*>(0x00BF4508);
inline const uint8_t* const ballColour = reinterpret_cast<const uint8_t*>(0x020C8838);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(SpVec*, __cdecl, VectorAdd, 0x004129A0, SpVec* out, const void* a, const void* b);
GAME_FN(SpVec*, __cdecl, VectorSub, 0x00414B70, SpVec* out, const void* a, const void* b);
GAME_FN(SpVec*, __cdecl, CrossProduct, 0x00414730, SpVec* out, const SpVec* a, const SpVec* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, SpVec* v);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const float* v);
GAME_FN(void, __thiscall, Vector2D_Normalise, 0x00443730, float* v);
GAME_FN(float*, __cdecl, Vector2D_Divide, 0x006D8020, float* out, const float* v, float k);
GAME_FN(uint8_t*, __thiscall, RGBA_Set, 0x00510DE0, SpRgba* rgba, int r, int g, int b, int a);
GAME_FN(void, __thiscall, Shape_Construct, 0x005593A0, SpShape* shape);
GAME_FN(void, __thiscall, Shape_SetSegment, 0x00557FB0, SpShape* shape, int index, float length, float x, float y,
        float width, float width2, const SpRgba* colour);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void*, __cdecl, GetButtonTexture, 0x00514EC0, int button);
GAME_FN(int, __cdecl, GetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, Im3D_Transform, 0x005EFAF0, void* vertices, int count, void* transform, uint32_t flags);
GAME_FN(void, __cdecl, Im3D_RenderIndexedPrimitive, 0x005F0170, int type, uint16_t* indices, int count, int);
GAME_FN(uint8_t, __cdecl, Im3D_End, 0x005EFB20);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(int32_t, __thiscall, Channel_IsPlaying, 0x005A6D60, void* channel);
GAME_FN(void, __thiscall, Channel_Pause, 0x005A6E40, void* channel);
GAME_FN(void, __thiscall, MissionTimer_SetPaused, 0x0043F6B0, void* timer, int paused);
GAME_FN(int, __thiscall, Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, Popup_Show, 0x00705220, void* popup, int kind, const char* title, const char* buttons, int,
        int);
GAME_FN(void, __thiscall, Popup_Draw, 0x00706000, void* popup);
GAME_FN(void, __thiscall, Minigame_Construct, 0x007059D0, SoccerP* self);
GAME_FN(void, __thiscall, Minigame_Destruct, 0x00705A50, SoccerP* self);
GAME_FN(void, __thiscall, Minigame_UnloadTextures, 0x00704DE0, SoccerP* self);
struct SpTextureEntry {
    int32_t id;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};
static_assert(sizeof(SpTextureEntry) == 0xC);
GAME_FN(void, __thiscall, Minigame_LoadTextures, 0x00704D30, SoccerP* self, int count, const SpTextureEntry* entries,
        const char* txd);
GAME_FN(uint8_t*, __cdecl, GetProjectile, 0x004D3DD0, int index);
GAME_FN(int32_t, __cdecl, PedHandleSlot, 0x0046DF10, int32_t handle);

// the soccer minigame's own (at the game's addresses, so modes apply)
GAME_FN(void, __thiscall, Soccer_AddMessage, 0x00713E70, SoccerP* self, int kind, float x, float y, float z);
GAME_FN(void, __thiscall, Soccer_SetTarget, 0x00713EF0, SoccerP* self, int index, float x, float y, float z,
        float width, float height, int points);
GAME_FN(SpVec*, __thiscall, Soccer_GetHitPos, 0x00714040, SoccerP* self, SpVec* out);
GAME_FN(void, __thiscall, Soccer_UpdateMessages, 0x007142D0, SoccerP* self);
GAME_FN(void, __thiscall, Soccer_SetGoalArea, 0x00714440, SoccerP* self, float ax, float ay, float az, float bx,
        float by, float bz, float height);
GAME_FN(void, __thiscall, Soccer_SetKeeper, 0x00715F50, SoccerP* self, uint8_t* ped);

inline float Fb(uint32_t bits) {
    return std::bit_cast<float>(bits);
}
inline float CrtSqrt(float x) {
    return crt::Sqrtf(x);
}
// an entity reference set (the old one released, the new one registered)
inline void SetReference(re::Ptr32<uint8_t>& ref, uint8_t* entity) {
    if (entity != ref) {
        if (ref != nullptr) {
            CleanUpOldReference(ref, &ref);
        }
        ref = entity;
        if (entity != nullptr) {
            RegisterReference(entity, &ref);
        }
    }
}

#define SOCCER45_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// (thiscall) a score pop-up (2 s) in the first free slot (none when all five show)
void __fastcall AddMessage(SoccerP* self, re::Edx, int kind, float x, float y, float z) {
    for (int i = 0; i < 5; i++) {
        SpMessage& m = self->messages[i];
        if (m.timer == 0.0f) {
            m.timer = 2.0f;
            m.unknown8 = 0.0f;
            m.pos.x = x;
            m.kind = kind;
            m.pos.y = y;
            m.pos.z = z;
            m.active = 1;
            return;
        }
    }
}
SOCCER45_REVERSED("SoccerP::AddMessage", 0x00713E70, &AddMessage);

// (thiscall) a target (0..12: past 12 the 12th; 12 itself runs into the sizes): its
// position (projected onto the goal area's plane once that is set), size and points
void __fastcall SetTarget(SoccerP* self, re::Edx, int index, float x, float y, float z, float width, float height,
                          int points) {
    if (index > 12) {
        index = 11;
    } else if (index < 0) {
        index = 0;
    }
    if (self->goalSet == 0) {
        self->targets[index] = {x, y, z};
    } else {
        SpVec dir;
        VectorSub(&dir, &self->goalB, &self->goalA);
        Vector_Normalise(&dir);
        SpVec up = {0.0f, 0.0f, 1.0f};
        SpVec n;
        CrossProduct(&n, &dir, &up);
        SpVec point = {x, y, z};
        SpVec tmp;
        SpVec* v = VectorSub(&tmp, &self->goalA, &point);
        float s = v->y * n.y + v->x * n.x;
        float t = s + v->z * n.z;
        SpVec w = {n.x * t, n.y * t, t * n.z};
        SpVec out;
        self->targets[index] = *VectorAdd(&out, &point, &w);
    }
    self->targetSize[index][0] = width;
    self->targetSize[index][1] = height;
    self->targetPoints[index] = points;
}
SOCCER45_REVERSED("SoccerP::SetTarget", 0x00713EF0, &SetTarget);

// (thiscall) where the kick hits: along the goal from its first corner by the power meter
// (its whole width at 1), up by the height meter (its height at 0)
SpVec* __fastcall GetHitPos(SoccerP* self, re::Edx, SpVec* out) {
    float power = self->power.value;
    float lift = self->height.value;
    SpVec d;
    VectorSub(&d, &self->goalB, &self->goalA);
    float xy = d.x * d.x + d.y * d.y;
    float length2 = std::fabs(xy + d.z * d.z);
    Vector_Normalise(&d);
    float up = (1.0f - lift) * self->goalHeight;
    float side = up * 0.0f;
    SpVec rise = {side, side, up};
    float k = CrtSqrt(length2) * power;
    SpVec along = {k * d.x, k * d.y, k * d.z};
    SpVec tmp;
    VectorAdd(out, VectorAdd(&tmp, &self->goalA, &along), &rise);
    return out;
}
SOCCER45_REVERSED("SoccerP::GetHitPos", 0x00714040, &GetHitPos);

// (thiscall) the target the kick hits (its index; the keeper within 0.4 m (2D) and 1.2 m
// in height saves it: -100; none: -1)
int __fastcall GetHitTarget(SoccerP* self, re::Edx) {
    SpVec hit;
    Soccer_GetHitPos(self, &hit);
    uint8_t* keeper = self->keeper;
    if (keeper != nullptr) {
        uint8_t* m = *reinterpret_cast<re::Ptr32<uint8_t>*>(keeper + 0x14);
        const float* pos = m != nullptr ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(keeper + 4);
        SpVec d;
        VectorSub(&d, pos, &hit);
        if (std::fabs(d.z) < 1.2f) {
            float xy = d.x * d.x + d.y * d.y;
            float flat = CrtSqrt(std::fabs(xy + 0.0f * 0.0f));
            if (flat < 0.4f) {
                return -100;
            }
        }
    }
    for (int i = 0; i < self->targetCount; i++) {
        SpVec d;
        VectorSub(&d, &hit, &self->targets[i]);
        float dz = std::fabs(d.z);
        if (self->targetSize[i][1] * 0.5f >= dz) {
            d.z = 0.0f;
            float xy = d.x * d.x + d.y * d.y;
            float flat = CrtSqrt(std::fabs(xy + 0.0f * 0.0f));
            if (self->targetSize[i][0] * 0.5f >= flat) {
                return i;
            }
        }
    }
    return -1;
}
SOCCER45_REVERSED("SoccerP::GetHitTarget", 0x00714160, &GetHitTarget);
// (0x714207 is a label inside GetHitTarget, the target loop, not a function)
RE_UNHOOKED("SoccerP::GetHitTarget+0xA7", 0x00714207);

// (thiscall) the pop-ups' timers run down (each ending clears +4: the game's)
void __fastcall UpdateMessages(SoccerP* self, re::Edx) {
    for (int i = 0; i < 5; i++) {
        float& timer = self->messages[i].timer;
        if (timer != 0.0f) {
            float step = popupRate * timeStepSp;
            float t = timer - step;
            timer = t;
            if (t < 0.0f || t == 0.0f) {
                timer = 0.0f;
                self->active = 0;
            }
        }
    }
}
SOCCER45_REVERSED("SoccerP::UpdateMessages", 0x007142D0, &UpdateMessages);

// (thiscall) the goal area: its two lower corners and height; the targets re-placed on it
// (their points reset)
void __fastcall SetGoalArea(SoccerP* self, re::Edx, float ax, float ay, float az, float bx, float by, float bz,
                            float height) {
    self->goalA = {ax, ay, az};
    self->goalB = {bx, by, bz};
    SpVec d;
    VectorSub(&d, &self->goalB, &self->goalA);
    float xy = d.x * d.x + d.y * d.y;
    self->goalWidth = CrtSqrt(std::fabs(xy + d.z * d.z));
    self->goalSet = 1;
    self->goalHeight = height;
    for (int i = 0; i < self->targetCount; i++) {
        SpVec t = self->targets[i];
        Soccer_SetTarget(self, i, t.x, t.y, t.z, self->targetSize[i][0], self->targetSize[i][1], 0);
    }
}
SOCCER45_REVERSED("SoccerP::SetGoalArea", 0x00714440, &SetGoalArea);

// (thiscall) the minigame constructed: its four shapes, the goalkeeper and ball references
// cleared, its HUD icon texture loaded
SoccerP* __fastcall Construct(SoccerP* self, re::Edx) {
    Minigame_Construct(self);
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00949E00);
    Shape_Construct(&self->power);
    Shape_Construct(&self->height);
    Shape_Construct(&self->bars[0]);
    Shape_Construct(&self->bars[1]);
    for (int i = 0; i < 12; i++) {
        self->targets[i] = {0.0f, 0.0f, 0.0f};
    }
    self->goalA = {0.0f, 0.0f, 0.0f};
    self->ball = nullptr;
    self->goalB = {0.0f, 0.0f, 0.0f};
    self->keeper = nullptr;
    for (int i = 0; i < 5; i++) {
        self->messages[i].pos = {0.0f, 0.0f, 0.0f};
    }
    self->kind38 = 4;
    self->mode = 0xC;
    if (self->ball != nullptr) {
        CleanUpOldReference(self->ball, &self->ball);
        self->ball = nullptr;
    }
    if (self->keeper != nullptr) {
        CleanUpOldReference(self->keeper, &self->keeper);
        self->keeper = nullptr;
    }
    self->unknownDC = 0.0f;
    self->goalSet = 0;
    self->balls = 5;
    SpTextureEntry icon = {0, "HUDIcon_soccer", "HUDIcon_soccer_x"};
    Minigame_LoadTextures(self, 1, &icon, "soccer");
    return self;
}
SOCCER45_REVERSED("SoccerP::SoccerP", 0x00714530, &Construct);

// (thiscall, vtable +0x18) the camera's target: the hit position
SpVec* __fastcall GetCameraTarget(SoccerP* self, re::Edx, SpVec* out, int) {
    Soccer_GetHitPos(self, out);
    return out;
}
SOCCER45_REVERSED("SoccerP::GetCameraTarget", 0x007146C0, &GetCameraTarget);

// a meter shape's ribbon: 0x7F steps, its stamp corners, its angle range, two segments
inline void SetUpShape(SpShape& s, float angleFrom, float angleTo) {
    s.corners[0] = -10.0f;
    s.corners[1] = -15.0f;
    s.corners[2] = 10.0f;
    s.corners[3] = -15.0f;
    s.corners[4] = -10.0f;
    s.corners[5] = 0.0f;
    s.corners[6] = 10.0f;
    s.corners[7] = 0.0f;
    s.steps = 0x7F;
    s.angleFrom = angleFrom;
    s.angleTo = angleTo;
    s.segments = 2;
}
// a bar along x (240..400 at y 380, 50 wide) or up (y 340..420 at x 332, 1 wide)
inline void SetSegments(SpShape& s, bool across) {
    SpRgba colour;
    if (across) {
        Shape_SetSegment(&s, 0, 0.0f, 240.0f, 380.0f, 50.0f, 1.5f,
                         reinterpret_cast<SpRgba*>(RGBA_Set(&colour, 0x10, 0x10, 0x10, 0xA0)));
        Shape_SetSegment(&s, 1, 1.0f, 400.0f, 380.0f, 50.0f, 1.5f,
                         reinterpret_cast<SpRgba*>(RGBA_Set(&colour, 0x10, 0x10, 0x10, 0xA0)));
    } else {
        Shape_SetSegment(&s, 0, 0.0f, 332.0f, 340.0f, 1.0f, 1.5f,
                         reinterpret_cast<SpRgba*>(RGBA_Set(&colour, 0x10, 0x10, 0x10, 0xA0)));
        Shape_SetSegment(&s, 1, 1.0f, 332.0f, 420.0f, 1.0f, 1.5f,
                         reinterpret_cast<SpRgba*>(RGBA_Set(&colour, 0x10, 0x10, 0x10, 0xA0)));
    }
}
// a meter's fill (yellow) and outline
inline void SetMeterColours(SpShape& s) {
    s.value = 0.5f;
    s.speed = 0.1f;
    s.mode = 2;
    SpRgba a, b;
    const SpRgba* faded = reinterpret_cast<SpRgba*>(RGBA_Set(&a, 0xEB, 0xEB, 0, 0));
    const SpRgba* fill = reinterpret_cast<SpRgba*>(RGBA_Set(&b, 0xEB, 0xEB, 0, 100));
    s.fill = *fill;
    s.fillFaded = *faded;
    s.outlineWidth = 1.5f;
    SpRgba c;
    s.outline = *reinterpret_cast<SpRgba*>(RGBA_Set(&c, 0x10, 0x10, 0x10, 0xFF));
}

// (thiscall, vtable +0x10) started: the pop-ups cleared, the bars and meters set up, the
// goal area and its six targets (two rows of three) placed
void __fastcall Start(SoccerP* self, re::Edx) {
    self->active = 1;
    self->success = 0;
    self->hud = 0;
    for (int i = 0; i < 5; i++) {
        self->messages[i].timer = 0.0f;
    }
    SetUpShape(self->bars[0], -0.0f, 0.0f);
    SetSegments(self->bars[0], true);
    self->bars[0].mode = 2;
    SetUpShape(self->bars[1], -0.5f, -0.5f);
    SetSegments(self->bars[1], false);
    self->bars[1].mode = 2;
    SetUpShape(self->power, -0.0f, 0.0f);
    SetSegments(self->power, true);
    SetMeterColours(self->power);
    SetUpShape(self->height, -0.5f, -0.5f);
    SetSegments(self->height, false);
    SetMeterColours(self->height);
    self->kicked = 0;
    for (int i = 0; i < 2; i++) {
        self->meterStopped[i] = 0;
        self->meterSpeed[i] = 0.04f;
        self->unknownF4[i] = 2.0f;
        self->unknownFC[i] = 0.0f;
        self->flash[i] = 0.0f;
        (i == 0 ? self->power : self->height).value = 0.5f;
    }
    self->goalSet = 0;
    self->targetCount = 0;
    self->unknown8AC = 0;
    self->stick[0] = 0.0f;
    self->stick[1] = 0.0f;
    self->velocity[0] = 0.0f;
    self->velocity[1] = 0.0f;
    self->aim[0] = 0.0f;
    self->aim[1] = 0.0f;
    self->stickUsed = 0;
    Soccer_SetGoalArea(self, Fb(0x428BA5D6), Fb(0xC2ACBCEE), Fb(0x40A50721), Fb(0x428BAD84), Fb(0xC2B8D3F8),
                       Fb(0x40A50721), Fb(0x4038F5C3));
    self->targetCount = 6;
    float w = Fb(0x3FAE147B);
    float h = Fb(0x3F570A3D);
    Soccer_SetTarget(self, 0, Fb(0x428B967A), Fb(0xC2AE1BA6), Fb(0x40F4115E), w, h, 0);
    Soccer_SetTarget(self, 1, Fb(0x428BA347), Fb(0xC2B2C72B), Fb(0x40F4115E), w, h, 0);
    Soccer_SetTarget(self, 2, Fb(0x428BB532), Fb(0xC2B7A5E3), Fb(0x40F4115E), w, h, 0);
    Soccer_SetTarget(self, 3, Fb(0x428B967A), Fb(0xC2AE1BA6), Fb(0x40B5D3ED), w, h, 0);
    Soccer_SetTarget(self, 4, Fb(0x428BA347), Fb(0xC2B2C72B), Fb(0x40B5D3ED), w, h, 0);
    Soccer_SetTarget(self, 5, Fb(0x428BB532), Fb(0xC2B7A5E3), Fb(0x40B5D3ED), w, h, 0);
}
SOCCER45_REVERSED("SoccerP::Start", 0x007146E0, &Start);

// (thiscall, vtable +8) drawn: the pop-ups' timers; with the HUD, the balls left (icons at
// the bottom right), the meters set from the aim, the aim's reticle (the button 7 texture,
// 0.3 m wide, 0.3 m high) at the hit position, the popup
void __fastcall Render(SoccerP* self, re::Edx) {
    Soccer_UpdateMessages(self);
    if (self->hud == 0) {
        return;
    }
    float sx = screenScaleXSp;
    float sy = screenScaleYSp;
    void* icon = nullptr;
    if (self->textures != nullptr && (icon = self->textures[0]) != nullptr && self->balls > 0) {
        float ry = sy * 16.0f;
        float y2 = ry + 385.0f;
        float y1 = 385.0f - ry;
        float step = sx * -34.0f;
        float left = sx * -80.0f + 640.0f;
        float half = sx * 16.0f;
        for (int i = 0; i < self->balls; i++) {
            float uv2[2] = {1.0f, 1.0f};
            float uv1[2] = {0.0f, 0.0f};
            float x = float(i) * step + left;
            float x2 = half + x;
            float x1 = x - half;
            DrawRectUV(x1, y1, x2, y2, icon, ballColour, uv1, uv2);
        }
    }
    self->power.value = (self->aim[0] / (self->goalWidth * 0.5f) + 1.0f) * 0.5f;
    self->height.value = (self->aim[1] / (self->goalHeight * 0.5f) + 1.0f) * 0.5f;
    SetRenderState(8, 0);
    SetRenderState(6, 0);
    SetRenderState(0xC, 1);
    SetRenderState(10, 5);
    SetRenderState(0xB, 6);
    SetRenderState(0x14, 1);
    SpVertex v[4] = {};
    for (SpVertex& p : v) {
        p.nz = 1.0f;
    }
    SpVec dir;
    VectorSub(&dir, &self->goalB, &self->goalA);
    Vector_Normalise(&dir);
    SpVec hit;
    Soccer_GetHitPos(self, &hit);
    SpVec half = {(dir.x * 0.3f) * 0.5f, (dir.y * 0.3f) * 0.5f, (dir.z * 0.3f) * 0.5f};
    SpVec rise = {0.0f, 0.0f, 0.15f};
    SpVec t, c;
    c = *VectorSub(&t, VectorAdd(&t, &hit, &rise), &half);
    v[0].x = c.x, v[0].y = c.y, v[0].z = c.z;
    c = *VectorAdd(&t, VectorAdd(&t, &hit, &rise), &half);
    v[1].x = c.x, v[1].y = c.y, v[1].z = c.z;
    c = *VectorAdd(&t, VectorSub(&t, &hit, &rise), &half);
    v[2].x = c.x, v[2].y = c.y, v[2].z = c.z;
    c = *VectorSub(&t, VectorSub(&t, &hit, &rise), &half);
    v[3].x = c.x, v[3].y = c.y, v[3].z = c.z;
    v[0].colour = v[1].colour = v[2].colour = v[3].colour = 0xFFFFFFFFu;
    v[0].u = 0.0f, v[0].v = 0.0f;
    v[1].u = 1.0f, v[1].v = 0.0f;
    v[2].u = 1.0f, v[2].v = 1.0f;
    v[3].u = 0.0f, v[3].v = 1.0f;
    uint16_t indices[6] = {2, 1, 0, 0, 2, 3};
    SetRenderState(1, GetRaster(GetButtonTexture(7)));
    Im3D_Transform(v, 4, nullptr, 0x19);
    Im3D_RenderIndexedPrimitive(3, indices, 6, 0);
    Im3D_End();
    if (self->popupShowing != 0) {
        Popup_Draw(&self->popupKind);
    }
}
SOCCER45_REVERSED("SoccerP::Render", 0x00715060, &Render);

// (thiscall, vtable +0) the deleting destructor
SoccerP* __fastcall DeletingDestruct(SoccerP* self, re::Edx, int flags) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00949E00);
    Minigame_UnloadTextures(self);
    if (self->keeper != nullptr) {
        CleanUpOldReference(self->keeper, &self->keeper);
    }
    if (self->ball != nullptr) {
        CleanUpOldReference(self->ball, &self->ball);
    }
    Minigame_Destruct(self);
    if ((flags & 1) != 0) {
        Free(self);
    }
    return self;
}
SOCCER45_REVERSED("SoccerP::DeletingDestruct", 0x00715740, &DeletingDestruct);

// (thiscall, vtable +4) the frame: the exit question (pausing the music and the mission
// timer); back asks it, the kick button kicks (the meters flash, a pop-up at the hit, the
// result set); else the stick steers the aim (with no input, after it was used, it keeps
// drifting at 0.005), its speed held under 0.1 and over 0.03, bouncing off the goal's edges
void __fastcall Update(SoccerP* self, re::Edx) {
    void* popup = &self->popupKind;
    if (self->popupShowing != 0 && self->popupKind == 1) {
        if (Channel_IsPlaying(musicChannelSp) != 0) {
            Channel_Pause(musicChannelSp);
        }
        MissionTimer_SetPaused(missionTimerSp, 1);
        if (Popup_Result(popup) == 2) {
            self->active = 0;
            self->success = 0;
        }
        return;
    }
    if (self->active == 0 || userStartResponseSp != 0) {
        return;
    }
    MissionTimer_SetPaused(missionTimerSp, 0);
    if (Pad_JustDown(8, 0) != 0) {
        Popup_Show(popup, 1, "EXIT_QUESTION", "EXIT_BUTTONS", 7, 8);
        return;
    }
    if (Pad_JustDown(7, 0) != 0) {
        self->flash[0] = 0.3f;
        self->flash[1] = 0.3f;
        self->meterStopped[0] = 1;
        self->kicked = 1;
        self->meterStopped[1] = 1;
        SpVec hit;
        SpVec* p = Soccer_GetHitPos(self, &hit);
        Soccer_AddMessage(self, 0, p->x, p->y, p->z);
        self->success = 1;
        return;
    }
    if (self->meterStopped[1] != 0) {
        return;
    }
    float* stick = self->stick;
    stick[0] = -Pad_GetValue(0x10, 0) * 0.01f;
    stick[1] = -Pad_GetValue(0x11, 0) * 0.01f;
    if (Vector_Magnitude2D(stick) == 0.0f) {
        if (self->stickUsed != 0) {
            float dir[2] = {self->velocity[0], self->velocity[1]};
            Vector2D_Normalise(dir);
            float x = dir[0] * 0.005f;
            float y = 0.005f * dir[1];
            stick[0] = x;
            stick[1] = y;
        }
    } else {
        self->stickUsed = 1;
    }
    float dt = timeStepSp;
    float dx = stick[0] * dt;
    float dy = dt * stick[1];
    float* vel = self->velocity;
    vel[0] = vel[0] + dx;
    vel[1] = vel[1] + dy;
    float m2 = vel[0] * vel[0] + vel[1] * vel[1];
    if (m2 > Fb(0x3C23D70B)) {   // (0.0100000007)
        Vector2D_Normalise(vel);
        vel[0] = vel[0] * 0.1f;
        vel[1] = 0.1f * vel[1];
    }
    m2 = vel[0] * vel[0] + vel[1] * vel[1];
    if (m2 < 0.0009f && self->stickUsed != 0) {
        float length = Vector_Magnitude2D(vel);
        float unit[2];
        float* u = Vector2D_Divide(unit, vel, length);
        float x = u[0] * 0.03f;
        float y = 0.03f * u[1];
        vel[0] = x;
        vel[1] = y;
    }
    dx = vel[0] * dt;
    dy = dt * vel[1];
    self->aim[0] = self->aim[0] + dx;
    self->aim[1] = self->aim[1] + dy;
    float hi = self->goalWidth * 0.5f;
    if (self->aim[0] > hi) {
        self->aim[0] = hi;
        vel[0] = vel[0] * -1.0f;
    }
    float lo = -self->goalWidth * 0.5f;
    if (self->aim[0] < lo) {
        self->aim[0] = lo;
        vel[0] = vel[0] * -1.0f;
    }
    hi = self->goalHeight * 0.5f;
    if (self->aim[1] > hi) {
        self->aim[1] = hi;
        vel[1] = vel[1] * -1.0f;
    }
    lo = -self->goalHeight * 0.5f;
    if (self->aim[1] < lo) {
        self->aim[1] = lo;
        vel[1] = -1.0f * vel[1];
    }
}
SOCCER45_REVERSED("SoccerP::Update", 0x007157A0, &Update);

// (thiscall) aiming started: the meters reset (0.5) and running
void __fastcall StartAiming(SoccerP* self, re::Edx) {
    self->active = 1;
    self->success = 0;
    for (int i = 0; i < 2; i++) {
        self->meterStopped[i] = 0;
        self->unknownFC[i] = 0.0f;
        self->flash[i] = 0.0f;
        (i == 0 ? self->power : self->height).value = 0.5f;
    }
    self->kicked = 0;
}
SOCCER45_REVERSED("SoccerP::StartAiming", 0x00715B40, &StartAiming);

// (thiscall) the goalkeeper ped
void __fastcall SetKeeper(SoccerP* self, re::Edx, uint8_t* ped) {
    SetReference(self->keeper, ped);
}
SOCCER45_REVERSED("SoccerP::SetKeeper", 0x00715F50, &SetKeeper);

// ---- the script natives (on the running minigame, a SoccerP) -----------------------------------

// (0..12)
int __cdecl Native_SoccerPSetTargetCount(lua_State* L) {
    SoccerP* self = currentSoccer;
    int n = Script::GetInt(L, 0);
    if (n > 12) {
        self->targetCount = 12;
        return 0;
    }
    if (n < 0) {
        n = 0;
    }
    self->targetCount = n;
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetTargetCount", 0x00715BC0, &Native_SoccerPSetTargetCount);

int __cdecl Native_SoccerPSetMeterSpeed(lua_State* L) {
    SoccerP* self = currentSoccer;
    float a = Script::GetFloat(L, 0);
    float b = Script::GetFloat(L, 1);
    self->meterSpeed[0] = a;
    self->meterSpeed[1] = b;
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetMeterSpeed", 0x00715C00, &Native_SoccerPSetMeterSpeed);

// -> x, y, z
int __cdecl Native_SoccerPGetHitPos(lua_State* L) {
    SpVec hit;
    Script::PushFloat(L, Soccer_GetHitPos(currentSoccer, &hit)->x);
    Script::PushFloat(L, Soccer_GetHitPos(currentSoccer, &hit)->y);
    Script::PushFloat(L, Soccer_GetHitPos(currentSoccer, &hit)->z);
    return 3;
}
SOCCER45_REVERSED("Native_SoccerPGetHitPos", 0x00715C50, &Native_SoccerPGetHitPos);

int __cdecl Native_SoccerPSetHealthBarPosition(lua_State* L) {
    SoccerP* self = currentSoccer;
    float a = Script::GetFloat(L, 0);
    float b = Script::GetFloat(L, 1);
    float c = Script::GetFloat(L, 2);
    float d = Script::GetFloat(L, 3);
    self->healthBar[0] = a;
    self->healthBar[1] = b;
    self->healthBar[2] = c;
    self->healthBar[3] = d;
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetHealthBarPosition", 0x00715D20, &Native_SoccerPSetHealthBarPosition);

int __cdecl Native_SoccerPSetPedsHealthBar(lua_State* L) {
    SoccerP* self = currentSoccer;
    float a = Script::GetFloat(L, 0);
    float b = Script::GetFloat(L, 1);
    self->pedsHealthBar[0] = a;
    self->pedsHealthBar[1] = b;
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetPedsHealthBar", 0x00715DA0, &Native_SoccerPSetPedsHealthBar);

// (corner a x, y, z, corner b x, y, z, height)
int __cdecl Native_SoccerPSetGoalArea(lua_State* L) {
    float bx = Script::GetFloat(L, 3);
    float by = Script::GetFloat(L, 4);
    float bz = Script::GetFloat(L, 5);
    float ax = Script::GetFloat(L, 0);
    float ay = Script::GetFloat(L, 1);
    float az = Script::GetFloat(L, 2);
    float height = Script::GetFloat(L, 6);
    Soccer_SetGoalArea(currentSoccer, ax, ay, az, bx, by, bz, height);
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetGoalArea", 0x00715DF0, &Native_SoccerPSetGoalArea);

// (1-based index, x, y, z, width, height[, points])
int __cdecl Native_SoccerPSetTarget(lua_State* L) {
    int points = 0;
    if (Script::GetArgCount(L) > 6) {
        points = Script::GetInt(L, 6);
    }
    float width = Script::GetFloat(L, 4);
    float height = Script::GetFloat(L, 5);
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    int index = Script::GetInt(L, 0);
    Soccer_SetTarget(currentSoccer, index - 1, x, y, z, width, height, points);
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetTarget", 0x00715EA0, &Native_SoccerPSetTarget);

// (a projectile's index)
int __cdecl Native_SoccerPSetProjectile(lua_State* L) {
    uint8_t* ball = GetProjectile(Script::GetInt(L, 0));
    SetReference(currentSoccer->ball, ball);
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetProjectile", 0x00715F90, &Native_SoccerPSetProjectile);

// (the goalkeeper's handle; checked against its pool slot)
int __cdecl Native_SoccerPSetPed(lua_State* L) {
    int32_t handle = Script::GetInt(L, 0);
    int32_t slot = PedHandleSlot(handle);
    CPool<uint8_t>* pool = pedPoolSp;
    if ((pool->flags[slot] & 0x80) != 0) {
        return 0;
    }
    uint8_t* ped = pool->objects + pool->elemSize * slot;
    if (ped != nullptr) {
        using HandleFn = int32_t(__thiscall*)(uint8_t*);
        int32_t own = re::PtrCast<HandleFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0xAC / 4])(ped);
        if (own == handle) {
            Soccer_SetKeeper(currentSoccer, ped);
        }
    }
    return 0;
}
SOCCER45_REVERSED("Native_SoccerPSetPed", 0x00715FE0, &Native_SoccerPSetPed);

} // namespace
