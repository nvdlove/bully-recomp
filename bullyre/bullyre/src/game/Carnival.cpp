// The carnival minigames (0x6D7CB0..0x6D9D2D): the baseball toss (balls left), the dunk
// tank (aiming a crosshair with the sticks), the shooting gallery (score numbers
// floating up from the targets) and the striker (a power meter: tapping fills it, it
// drains); their script natives; and MGClassArt's settings setters (0x6D9D30..
// 0x6DA006). The Minigame base (0x7059D0 constructor, 0x705A50 destructor; +4 running,
// +5 HUD shown, +6 result, +0x1C the textures, +0x38 / +0x3C kinds) is another file's.
//
// Baseball toss: +0xC4 the balls. Dunk tank: +0xC4 the stick input (2D), +0xCC the
// crosshair's velocity, +0xD4 its position, +0xE4 the balls, +0xE8 / +0xEC the
// baseball weapon's saved effect ids. Shooting: 5 numbers of 0x24 at +0xC4 {value,
// timer, screen x / y, ..., world position (+0x14), active (+0x20)}. Striker: +0xC4
// stopped, +0xC8 the level (0..1), +0xCC the drain rate, +0xD0 the fill rate, +0xD4 /
// +0xD8 the tap interval range, +0xDC the hold time, +0xE0 the last tap, +0xE4 the
// hold left, +0xE8 / +0x2D0 two meter shapes, +0x4B8 / +0x4BC flash timers.

#include <cmath>
#include <cstdint>

#include "game/ScriptApi.h"
#include "reversed.h"

namespace {

using re::At;

struct Rgba {
    uint8_t r, g, b, a;
};

GAME_VAR(uint8_t*, cv_minigame, 0x020C8704);
GAME_VAR(float, cv_screenScaleX, 0x00A957A8);
GAME_VAR(float, cv_screenScaleY, 0x00A957AC);
GAME_VAR(float, cv_screenScale2, 0x00A957B0);
GAME_VAR(float, cv_timeStep, 0x00C1A9A4);
GAME_VAR(int32_t, cv_timeMs, 0x00C1A9B4);
GAME_VAR(float, cv_dunkScale, 0x00BC5F80);       // (114.58)
GAME_VAR(float, cv_dunkWidth, 0x00BC5F84);       // (140)
GAME_VAR(float, cv_dunkHeight, 0x00BC5F88);      // (220)
GAME_VAR(float, cv_numberSpeed, 0x00BC6004);     // (0.02)
GAME_VAR(float, cv_strikerDecay, 0x00BC601C);
inline const Rgba* const cv_ballColour = reinterpret_cast<const Rgba*>(0x020C8214);
inline const Rgba* const cv_crosshairColour = reinterpret_cast<const Rgba*>(0x020C8218);
inline const Rgba* const cv_dunkBallColour = reinterpret_cast<const Rgba*>(0x020C821C);
GAME_VAR(Rgba, cv_numberShadow, 0x020C822C);
GAME_VAR(Rgba, cv_numberColour, 0x020C8230);
GAME_VAR(Rgba, cv_numberPositive, 0x020C8234);
GAME_VAR(Rgba, cv_numberNegative, 0x020C8238);
GAME_VAR(Rgba, cv_fontColour, 0x0208B764);
GAME_VAR(uint16_t, cv_fontWord784, 0x0208B784);
GAME_VAR(uint16_t, cv_fontWord786, 0x0208B786);
inline void* const cv_camera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
GAME_VAR(void*, cv_effectLibrary, 0x00B6F1E8);
GAME_ARRAY(uint8_t*, cv_modelInfoPtrs, 0x00C67738);
constexpr uintptr_t VT_BaseballToss = 0x00943AD8;
constexpr uintptr_t VT_DunkTank = 0x00943C7C;
constexpr uintptr_t VT_Shooting = 0x00943D7C;
constexpr uintptr_t VT_Striker = 0x00943F18;

// a texture set entry the Minigame loads: (button, texture, mask)
struct TextureEntry {
    int32_t button;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};

GAME_FN(void, __thiscall, CV_Minigame_Construct, 0x007059D0, void* minigame);
GAME_FN(void, __thiscall, CV_Minigame_Destruct, 0x00705A50, void* minigame);
GAME_FN(void, __thiscall, CV_Minigame_LoadTextures, 0x00704D30, void* minigame, int count, const TextureEntry* entries,
        const char* txd);
GAME_FN(void, __thiscall, CV_Minigame_UnloadTextures, 0x00704DE0, void* minigame);
GAME_FN(void, __cdecl, CV_Font_SetButton, 0x00687D60, int button, void* texture);
GAME_FN(void, __cdecl, CV_Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, CV_DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture, const Rgba* colour,
        const float* uv1, const float* uv2);
GAME_FN(float*, __cdecl, CV_ScreenPosition, 0x0052D4F0, float* out, float x, float y, int hAlign, int vAlign);
GAME_FN(Rgba*, __thiscall, CV_Rgba_Set, 0x00510DE0, Rgba* rgba, int r, int g, int b, int a);
GAME_FN(uint8_t, __cdecl, CV_Pad_JustPressed, 0x00738900, int control, int pad);
GAME_FN(float, __cdecl, CV_Pad_GetAxis, 0x00738840, int control, int pad);
GAME_FN(float, __thiscall, CV_Vector2_Magnitude, 0x004246B0, const float* v);
GAME_FN(void, __thiscall, CV_Vector2_Normalise, 0x00443730, float* v);
GAME_FN(int32_t, __cdecl, CV_HashName, 0x00576ED0, const char* name);
GAME_FN(int32_t, __cdecl, CV_GetModelIndexFromHash, 0x0051AF00, int32_t hash);
GAME_FN(void, __thiscall, CV_WeaponInfo_Fn7282A0, 0x007282A0, void* info);
GAME_FN(void, __thiscall, CV_WeaponEffects_Fn728430, 0x00728430, void* effects);
GAME_FN(uint16_t, __thiscall, CV_EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(int32_t, __thiscall, CV_Camera_ProjectToScreen, 0x004F2240, void* camera, const float* point, float* out, int);
GAME_FN(void, __cdecl, CV_Font_Begin, 0x00688E60);
GAME_FN(void, __cdecl, CV_Font_SetStyle, 0x00688050, int style);
GAME_FN(void, __cdecl, CV_Font_AddNumber, 0x006880D0, float number);
GAME_FN(const uint16_t*, __thiscall, CV_Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __cdecl, CV_Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* format, int size);
GAME_FN(void, __cdecl, CV_Font_Print, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(float, __cdecl, CV_GetTimeSeconds, 0x00425710);
GAME_FN(float, __cdecl, CV_Clamp, 0x0041A470, float value, float low, float high);
GAME_FN(void*, __cdecl, CV_Hud_GetTexture, 0x00514EC0, int which);
GAME_FN(void, __thiscall, CV_Shape_SetSegment, 0x00557FB0, void* shape, int index, float length, float x, float y,
        float width, float width2, const Rgba* colour);
GAME_FN(void, __thiscall, CV_Shape_Construct, 0x005593A0, void* shape);
// (this file's)
GAME_FN(float*, __thiscall, CV_DunkTank_GetTargetPos, 0x006D7FF0, void* minigame, float* out);
GAME_FN(float*, __cdecl, CV_Vector2_Divide, 0x006D8020, float* out, const float* v, float k);
GAME_FN(void, __thiscall, CV_DunkTank_Destruct, 0x006D83D0, void* minigame);
GAME_FN(void, __thiscall, CV_Shooting_AddNumber, 0x006D8880, void* minigame, int value, float x, float y, float z);
GAME_FN(void, __thiscall, CV_Shooting_DrawNumbers, 0x006D8920, void* minigame);

// an unsigned millisecond count in seconds (fild, +2^32 when negative, * 0.001)
float MsToSeconds(int32_t ms) {
    double value = double(ms);
    if (ms < 0) {
        value = double(float(value + 4294967296.0));
    }
    return float(value * 0.0010000000474974513);
}

// a channel of a lerp between two colours (truncated)
uint8_t LerpChannel(uint8_t from, uint8_t to, float t) {
    return uint8_t(int32_t(float(int32_t(to) - int32_t(from)) * t + float(int32_t(from))));
}
Rgba LerpColour(const Rgba& from, const Rgba& to, float t) {
    return {LerpChannel(from.r, to.r, t), LerpChannel(from.g, to.g, t), LerpChannel(from.b, to.b, t),
            uint8_t(int32_t(t * float(int32_t(to.a) - int32_t(from.a)) + float(int32_t(from.a))))};
}

// the flash curve of a fading pulse: up over the first 0.2, then down
float Pulse(float p) {
    if (0.2f > p) {
        float q = p / 0.2f;
        return q * q;
    }
    float q = (p - 0.2f) / 0.8f;
    return 1.0f - q * q;
}

void* Texture(uint8_t* minigame, int index) {
    re::Ptr32<void>* textures = At<re::Ptr32<void>*>(minigame, 0x1C);
    return textures != nullptr ? textures[index] : nullptr;
}

} // namespace

#define CARNIVAL_REVERSED(name, address, fn) REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// 0x6D7CB0 (thiscall): a loaded texture (null with none)
void* __fastcall Minigame_GetTexture(uint8_t* self, re::Edx, int index) {
    return Texture(self, index);
}
CARNIVAL_REVERSED("Minigame::GetTexture", 0x006D7CB0, &Minigame_GetTexture);

// ---------------------------------------------------------------------------------------
// the baseball toss

// 0x6D7CD0 (thiscall): the constructor (its textures, three of them buttons)
uint8_t* __fastcall BaseballToss_Construct(uint8_t* self, re::Edx) {
    CV_Minigame_Construct(self);
    TextureEntry entries[4] = {
        {0, "HUDIcon_baseball", "HUDIcon_baseball_x"},
        {1, "BT_Catcher", "BT_Catcher_x"},
        {2, reinterpret_cast<const char*>(0x00943A84), reinterpret_cast<const char*>(0x00943A78)},
        {3, reinterpret_cast<const char*>(0x00943A6C), reinterpret_cast<const char*>(0x00943A60)},
    };
    At<uint32_t>(self, 0) = VT_BaseballToss;
    At<int32_t>(self, 0x38) = 2;
    At<int32_t>(self, 0x3C) = 8;
    CV_Minigame_LoadTextures(self, 4, entries, "MG_ballt");
    CV_Font_SetButton(0, Texture(self, 1));
    CV_Font_SetButton(1, Texture(self, 2));
    CV_Font_SetButton(2, Texture(self, 3));
    return self;
}
CARNIVAL_REVERSED("CarnivalBaseballToss::CarnivalBaseballToss", 0x006D7CD0, &BaseballToss_Construct);

// 0x6D7DD0 (thiscall, slot 4): starts (no balls)
void __fastcall BaseballToss_Start(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 4) = 1;
    At<uint8_t>(self, 5) = 0;
    At<int32_t>(self, 0xC4) = 0;
}
CARNIVAL_REVERSED("CarnivalBaseballToss::Start", 0x006D7DD0, &BaseballToss_Start);

// 0x6D7DE0 (thiscall, Minigame's slot 6): the camera offset: none
float* __fastcall Minigame_GetCameraOffset(void*, re::Edx, float* out, int) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    return out;
}
CARNIVAL_REVERSED("Minigame::GetCameraOffset", 0x006D7DE0, &Minigame_GetCameraOffset);

// 0x6D7E00 (thiscall): the deleting destructor
uint8_t* __fastcall BaseballToss_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_BaseballToss;
    CV_Minigame_UnloadTextures(self);
    CV_Minigame_Destruct(self);
    if ((flags & 1) != 0) {
        CV_Free(self);
    }
    return self;
}
CARNIVAL_REVERSED("CarnivalBaseballToss::DeletingDestructor", 0x006D7E00, &BaseballToss_DeletingDestructor);

// 0x6D7E30 (thiscall, slot 2): the balls left, a row of icons from the right
void __fastcall BaseballToss_Draw(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 5) == 0) {
        return;
    }
    float sx = cv_screenScaleX;
    float sy = cv_screenScaleY;
    void* texture = Texture(self, 0);
    if (At<int32_t>(self, 0xC4) <= 0) {
        return;
    }
    float half = sy * 16.0f;
    float y2 = half + 330.0f;
    float y1 = 330.0f - half;
    float step = sx * -30.0f;
    float start = sx * -48.0f + 640.0f;
    float width = sx * 16.0f;
    for (int i = 0; i < At<int32_t>(self, 0xC4); i++) {
        float uvA[2] = {1.0f, 1.0f};
        float uvB[2] = {0.0f, 0.0f};
        float x = float(i) * step + start;
        CV_DrawRectUV(x - width, y1, x + width, y2, texture, cv_ballColour, uvB, uvA);
    }
}
CARNIVAL_REVERSED("CarnivalBaseballToss::Draw", 0x006D7E30, &BaseballToss_Draw);

int __cdecl Native_CarnivalBallTossSetCount(lua_State* L) {
    uint8_t* minigame = cv_minigame;
    int count = Script::GetInt(L, 0);
    if (count >= 0) {
        At<int32_t>(minigame, 0xC4) = count;
    }
    return 0;
}

int __cdecl Native_CarnivalBallTossIncCount(lua_State*) {
    At<int32_t>(cv_minigame, 0xC4) += 1;
    return 0;
}

int __cdecl Native_CarnivalBallTossDecCount(lua_State*) {
    uint8_t* minigame = cv_minigame;
    int32_t count = At<int32_t>(minigame, 0xC4);
    if (count != 0) {
        At<int32_t>(minigame, 0xC4) = count - 1;
    }
    return 0;
}

int __cdecl Native_CarnivalBallTossGetCount(lua_State* L) {
    Script::PushInt(L, At<int32_t>(cv_minigame, 0xC4));
    return 1;
}

// 0x6D7FE0 (thiscall, slot 5): stops
void __fastcall Minigame_Stop(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 4) = 0;
    At<uint8_t>(self, 5) = 0;
}
CARNIVAL_REVERSED("Minigame::Stop_6D7FE0", 0x006D7FE0, &Minigame_Stop);

// ---------------------------------------------------------------------------------------
// the dunk tank

// 0x6D7FF0 (thiscall): the crosshair as a target (scaled, y up)
float* __fastcall DunkTank_GetTargetPos(uint8_t* self, re::Edx, float* out) {
    float scale = cv_dunkScale;
    out[0] = -At<float>(self, 0xD4) / scale;
    out[1] = -At<float>(self, 0xD8) / scale;
    out[2] = 0.0f;
    return out;
}
CARNIVAL_REVERSED("CarnivalDunkTank::GetTargetPos", 0x006D7FF0, &DunkTank_GetTargetPos);

// 0x6D8020 (cdecl): a 2D vector divided by k (times 1/k)
float* __cdecl Vector2_Divide(float* out, const float* v, float k) {
    float inverse = 1.0f / k;
    out[0] = v[0] * inverse;
    out[1] = inverse * v[1];
    return out;
}
CARNIVAL_REVERSED("CVector2D::Divide_6D8020", 0x006D8020, &Vector2_Divide);

namespace {
// the crosshair's reset: still, at the centre
void DunkTank_ResetAim(uint8_t* self) {
    At<float>(self, 0xC4) = 0.0f;
    At<float>(self, 0xC8) = 0.0f;
    At<float>(self, 0xCC) = 2.0f;
    At<float>(self, 0xD0) = 0.0f;
    float y = cv_dunkHeight * 0.5f;
    At<float>(self, 0xD4) = 0.0f;
    At<float>(self, 0xD8) = y;
}
} // namespace

// 0x6D8050 (thiscall, slot 4): starts
void __fastcall DunkTank_Start(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 4) = 1;
    At<uint8_t>(self, 5) = 0;
    At<int32_t>(self, 0xE4) = 0;
    DunkTank_ResetAim(self);
}
CARNIVAL_REVERSED("CarnivalDunkTank::Start", 0x006D8050, &DunkTank_Start);

// 0x6D80B0 (thiscall, slot 2): the balls left and the crosshair
void __fastcall DunkTank_Draw(uint8_t* self, re::Edx) {
    void* texture = Texture(self, 0);
    float scale = cv_screenScale2;
    float position[2];
    CV_ScreenPosition(position, -2.0f, -3.0f, 1, 1);
    if (At<int32_t>(self, 0xE4) > 0) {
        float half = scale * 16.0f;
        float y2 = half + 42.0f;
        float y1 = 42.0f - half;
        float start = 60.0f / scale;
        for (int i = 0; i < At<int32_t>(self, 0xE4); i++) {
            float uvA[2] = {1.0f, 1.0f};
            float uvB[2] = {0.0f, 0.0f};
            float x = float(i) * 30.0f + start;
            CV_DrawRectUV(x - 16.0f, y1, x + 16.0f, y2, texture, cv_dunkBallColour, uvB, uvA);
        }
    }
    if (At<uint8_t>(self, 5) == 0) {
        return;
    }
    float uvA[2] = {1.0f, 1.0f};
    float uvB[2] = {0.0f, 0.0f};
    float y = At<float>(self, 0xD8) * scale + position[1];
    float half = scale * 16.0f;
    float x = At<float>(self, 0xD4) + position[0];
    CV_DrawRectUV(x - 16.0f, y - half, x + 16.0f, y + half, texture, cv_crosshairColour, uvB, uvA);
}
CARNIVAL_REVERSED("CarnivalDunkTank::Draw", 0x006D80B0, &DunkTank_Draw);

// 0x6D8290 (thiscall): starts aiming
void __fastcall DunkTank_StartAiming(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 4) = 1;
    At<uint8_t>(self, 6) = 0;
    DunkTank_ResetAim(self);
}
CARNIVAL_REVERSED("CarnivalDunkTank::StartAiming", 0x006D8290, &DunkTank_StartAiming);

// 0x6D82E0 (thiscall): the constructor (the baseball's hit effect swapped for HitSweat,
// the old ids kept)
uint8_t* __fastcall DunkTank_Construct(uint8_t* self, re::Edx) {
    CV_Minigame_Construct(self);
    TextureEntry entries[1] = {{0, "HUDIcon_baseball", "HUDIcon_baseball_x"}};
    At<uint32_t>(self, 0) = VT_DunkTank;
    At<int32_t>(self, 0x38) = 2;
    At<int32_t>(self, 0x3C) = 0xD;
    CV_Minigame_LoadTextures(self, 1, entries, "MG_dunkt");
    At<uint32_t>(self, 0xE8) = 0xFFFF;
    At<uint32_t>(self, 0xEC) = 0xFFFF;
    uint8_t* info = At<uint8_t*>(cv_modelInfoPtrs[CV_GetModelIndexFromHash(CV_HashName("baseball"))], 0x18);
    if (info == nullptr) {
        return self;
    }
    CV_WeaponInfo_Fn7282A0(info);
    uint8_t* effects = At<uint8_t*>(info, 0x80);
    if (effects == nullptr) {
        return self;
    }
    CV_WeaponEffects_Fn728430(effects);
    At<uint32_t>(self, 0xE8) = At<uint16_t>(effects, 0x150);
    CV_WeaponEffects_Fn728430(effects);
    At<uint32_t>(self, 0xEC) = At<uint16_t>(effects, 0x152);
    uint16_t sweat = CV_EffectLibrary_Find(cv_effectLibrary, "HitSweat");
    At<uint16_t>(effects, 0x150) = 0xFFFF;
    At<uint16_t>(effects, 0x152) = sweat;
    return self;
}
CARNIVAL_REVERSED("CarnivalDunkTank::CarnivalDunkTank", 0x006D82E0, &DunkTank_Construct);

// 0x6D83D0 (thiscall): the destructor (the baseball's effects put back)
void __fastcall DunkTank_Destruct(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = VT_DunkTank;
    uint8_t* info = At<uint8_t*>(cv_modelInfoPtrs[CV_GetModelIndexFromHash(CV_HashName("baseball"))], 0x18);
    if (info != nullptr) {
        CV_WeaponInfo_Fn7282A0(info);
        uint8_t* effects = At<uint8_t*>(info, 0x80);
        if (effects != nullptr) {
            At<uint16_t>(effects, 0x150) = At<uint16_t>(self, 0xE8);
            At<uint16_t>(effects, 0x152) = At<uint16_t>(self, 0xEC);
            At<uint32_t>(self, 0xE8) = 0xFFFF;
            At<uint32_t>(self, 0xEC) = 0xFFFF;
        }
    }
    CV_Minigame_UnloadTextures(self);
    CV_Minigame_Destruct(self);
}
CARNIVAL_REVERSED("CarnivalDunkTank::~CarnivalDunkTank", 0x006D83D0, &DunkTank_Destruct);

// 0x6D8450 (thiscall): the deleting destructor
uint8_t* __fastcall DunkTank_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    CV_DunkTank_Destruct(self);
    if ((flags & 1) != 0) {
        CV_Free(self);
    }
    return self;
}
CARNIVAL_REVERSED("CarnivalDunkTank::DeletingDestructor", 0x006D8450, &DunkTank_DeletingDestructor);

// 0x6D8470 (thiscall, slot 1): aiming: the sticks push the crosshair (drifting on with
// its velocity when let go), its speed kept between 2 and 6, bounced off the box edges
void __fastcall DunkTank_Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 4) == 0) {
        return;
    }
    if (CV_Pad_JustPressed(7, 0) != 0 || CV_Pad_JustPressed(0xC, 0) != 0) {
        At<uint8_t>(self, 4) = 0;
        At<uint8_t>(self, 6) = 1;
        return;
    }
    if (CV_Pad_JustPressed(8, 0) != 0) {
        At<uint8_t>(self, 4) = 0;
        At<uint8_t>(self, 6) = 0;
        return;
    }
    float* input = &At<float>(self, 0xC4);
    input[0] = -CV_Pad_GetAxis(0x10, 0) * 0.5f;
    input[1] = -CV_Pad_GetAxis(0x11, 0) * 0.5f;
    if (CV_Vector2_Magnitude(input) == 0.0f) {
        input[0] = -CV_Pad_GetAxis(0x12, 0) * 0.5f;
        input[1] = -CV_Pad_GetAxis(0x13, 0) * 0.5f;
    }
    if (CV_Vector2_Magnitude(input) == 0.0f) {
        float velocity[2] = {At<float>(self, 0xCC), At<float>(self, 0xD0)};
        CV_Vector2_Normalise(velocity);
        float x = velocity[0] * 0.05f;
        float y = 0.05f * velocity[1];
        input[0] = x;
        input[1] = y;
    }
    float dt = cv_timeStep;
    float ax = input[0] * dt;
    float ay = dt * input[1];
    float* velocity = &At<float>(self, 0xCC);
    velocity[0] = velocity[0] + ax;
    velocity[1] = ay + velocity[1];
    float speed2 = velocity[1] * velocity[1] + velocity[0] * velocity[0];
    if (speed2 > 36.0f) {
        CV_Vector2_Normalise(velocity);
        velocity[0] = velocity[0] * 6.0f;
        velocity[1] = 6.0f * velocity[1];
    }
    speed2 = velocity[1] * velocity[1] + velocity[0] * velocity[0];
    if (speed2 < 4.0f) {
        float direction[2];
        float* d = CV_Vector2_Divide(direction, velocity, CV_Vector2_Magnitude(velocity));
        float x = d[0] * 2.0f;
        float y = 2.0f * d[1];
        velocity[0] = x;
        velocity[1] = y;
    }
    dt = cv_timeStep;
    float mx = velocity[0] * dt;
    float my = dt * velocity[1];
    At<float>(self, 0xD4) = mx + At<float>(self, 0xD4);
    At<float>(self, 0xD8) = At<float>(self, 0xD8) + my;
    float width = cv_dunkWidth;
    float limit = width * 0.5f;
    if (limit < At<float>(self, 0xD4)) {
        At<float>(self, 0xD4) = limit;
        velocity[0] = velocity[0] * -1.0f;
    }
    limit = -width * 0.5f;
    if (limit > At<float>(self, 0xD4)) {
        At<float>(self, 0xD4) = limit;
        velocity[0] = velocity[0] * -1.0f;
    }
    float height = cv_dunkHeight;
    limit = height * 0.5f;
    if (limit < At<float>(self, 0xD8)) {
        At<float>(self, 0xD8) = limit;
        velocity[1] = velocity[1] * -1.0f;
    }
    limit = -height * 0.5f;
    if (limit > At<float>(self, 0xD8)) {
        At<float>(self, 0xD8) = limit;
        velocity[1] = -1.0f * velocity[1];
    }
}
CARNIVAL_REVERSED("CarnivalDunkTank::Update", 0x006D8470, &DunkTank_Update);

int __cdecl Native_CarnivalDunkTankGetTargetPos(lua_State* L) {
    float target[3];
    CV_DunkTank_GetTargetPos(cv_minigame, target);
    Script::PushFloat(L, target[0]);
    Script::PushFloat(L, target[1]);
    Script::PushFloat(L, target[2]);
    return 3;
}

// ---------------------------------------------------------------------------------------
// the shooting gallery

// 0x6D8880 (thiscall): a score number at a world point (the first free of 5)
void __fastcall Shooting_AddNumber(uint8_t* self, re::Edx, int value, float x, float y, float z) {
    int i = 0;
    for (; i < 5; i++) {
        if (0.0f == At<float>(self, 0xC8 + i * 0x24)) {
            break;
        }
    }
    if (i == 5) {
        return;
    }
    uint8_t* slot = self + 0xC4 + i * 0x24;
    At<float>(slot, 4) = 2.0f;
    At<float>(slot, 8) = 0.0f;
    At<float>(slot, 0x14) = x;
    At<float>(slot, 0x18) = y;
    At<int32_t>(slot, 0) = value;
    At<float>(slot, 0x1C) = z;
    At<uint8_t>(slot, 0x20) = 1;
}
CARNIVAL_REVERSED("CarnivalShooting::AddNumber", 0x006D8880, &Shooting_AddNumber);

// 0x6D88F0 (thiscall): the deleting destructor
uint8_t* __fastcall Shooting_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_Shooting;
    CV_Minigame_UnloadTextures(self);
    CV_Minigame_Destruct(self);
    if ((flags & 1) != 0) {
        CV_Free(self);
    }
    return self;
}
CARNIVAL_REVERSED("CarnivalShooting::DeletingDestructor", 0x006D88F0, &Shooting_DeletingDestructor);

// 0x6D8920 (thiscall): the numbers: rising over their 2 seconds, fading in and out,
// outlined in the shadow colour; positive / negative / zero coloured
void __fastcall Shooting_DrawNumbers(uint8_t* self, re::Edx) {
    for (int i = 0; i < 5; i++) {
        uint8_t* slot = self + 0xC4 + i * 0x24;
        float& timer = At<float>(slot, 4);
        if (0.0f == timer) {
            continue;
        }
        float progress = 1.0f - timer * 0.5f;
        float step = cv_numberSpeed * cv_timeStep;
        timer = timer - step;
        if (!(timer > 0.0f) && timer == timer) {
            timer = 0.0f;
        }
        if (At<uint8_t>(slot, 0x20) != 0) {
            float screen[3] = {0.0f, 0.0f, 0.0f};
            if (CV_Camera_ProjectToScreen(cv_camera, &At<float>(slot, 0x14), screen, 0) == 0) {
                continue;
            }
            At<float>(slot, 8) = screen[0];
            At<float>(slot, 0xC) = screen[1];
        }
        float x = At<float>(slot, 8);
        float y = progress * -50.0f * progress - 10.0f + At<float>(slot, 0xC);
        float alpha = Pulse(progress);
        CV_Font_Begin();
        cv_fontWord784 = 0;
        CV_Font_SetStyle(1);
        cv_fontWord786 = 3;
        CV_Font_AddNumber(std::fabs(float(At<int32_t>(slot, 0))));
        Rgba colour = cv_numberColour;
        uint16_t text[0x20];
        int32_t value = At<int32_t>(slot, 0);
        if (value > 0) {
            CV_Font_Format(text, CV_Text_Get(TheText, "MGSG_POSNUM"), 0x20);
            Rgba chosen = cv_numberPositive;
            colour.r = chosen.r;
            colour.g = chosen.g;
            colour.b = chosen.b;
            colour.a = chosen.a;
        } else if (value < 0) {
            CV_Font_Format(text, CV_Text_Get(TheText, "MGSG_NEGNUM"), 0x20);
            Rgba chosen = cv_numberNegative;
            colour.r = chosen.r;
            colour.g = chosen.g;
            colour.b = chosen.b;
            colour.a = chosen.a;
        } else {
            CV_Font_Format(text, CV_Text_Get(TheText, "MGSG_NUMBER"), 0x20);
        }
        Rgba shadow = cv_numberShadow;
        cv_fontColour.g = shadow.g;
        cv_fontColour.r = shadow.r;
        cv_fontColour.b = shadow.b;
        cv_fontColour.a = uint8_t(int32_t(float(int32_t(shadow.a)) * alpha));
        CV_Font_Print(x + 2.0f, y, text, 1);
        CV_Font_Print(x, y + 2.0f, text, 1);
        CV_Font_Print(x - 2.0f, y, text, 1);
        CV_Font_Print(x, y - 2.0f, text, 1);
        cv_fontColour.g = colour.g;
        cv_fontColour.r = colour.r;
        cv_fontColour.b = colour.b;
        cv_fontColour.a = uint8_t(int32_t(float(int32_t(colour.a)) * alpha));
        CV_Font_Print(x, y, text, 1);
    }
}
CARNIVAL_REVERSED("CarnivalShooting::DrawNumbers", 0x006D8920, &Shooting_DrawNumbers);

// 0x6D8CA0 (thiscall): the constructor
uint8_t* __fastcall Shooting_Construct(uint8_t* self, re::Edx) {
    CV_Minigame_Construct(self);
    At<uint32_t>(self, 0) = VT_Shooting;
    for (int i = 0; i < 5; i++) {
        At<float>(self, 0xD8 + i * 0x24) = 0.0f;
        At<float>(self, 0xDC + i * 0x24) = 0.0f;
        At<float>(self, 0xE0 + i * 0x24) = 0.0f;
    }
    TextureEntry entries[3] = {
        {0, "SG_Bottle", "SG_Bottle_x"},
        {1, "SG_Damsel", "SG_Damsel_x"},
        {2, "SG_Cowboy", "SG_Cowboy_x"},
    };
    At<int32_t>(self, 0x38) = 2;
    At<int32_t>(self, 0x3C) = 0x10;
    CV_Minigame_LoadTextures(self, 3, entries, "MG_shgal");
    CV_Font_SetButton(0, Texture(self, 0));
    CV_Font_SetButton(1, Texture(self, 1));
    CV_Font_SetButton(2, Texture(self, 2));
    return self;
}
CARNIVAL_REVERSED("CarnivalShooting::CarnivalShooting", 0x006D8CA0, &Shooting_Construct);

// 0x6D8DE0 (thiscall, slot 2): the draw (the numbers)
void __fastcall Shooting_Draw(uint8_t* self, re::Edx) {
    CV_Shooting_DrawNumbers(self);
}
CARNIVAL_REVERSED("CarnivalShooting::Draw", 0x006D8DE0, &Shooting_Draw);

int __cdecl Native_CarnivalShootAddNumber(lua_State* L) {
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    uint8_t* minigame = cv_minigame;
    CV_Shooting_AddNumber(minigame, Script::GetInt(L, 0), x, y, z);
    return 0;
}

// ---------------------------------------------------------------------------------------
// the striker

namespace {
// a meter shape's set up (its two segments: bottom and top colours)
void SetUpMeter(uint8_t* shape, const int bottom[4], const int top[4]) {
    At<float>(shape, 8) = -10.0f;
    At<float>(shape, 0xC) = -15.0f;
    At<float>(shape, 0x10) = 10.0f;
    At<float>(shape, 0x14) = -15.0f;
    At<float>(shape, 0x18) = -10.0f;
    At<float>(shape, 0x1C) = 0.0f;
    At<float>(shape, 0x20) = 10.0f;
    At<float>(shape, 0x24) = 0.0f;
    At<int32_t>(shape, 4) = 0x7F;
    At<float>(shape, 0x28) = -0.0f;
    At<float>(shape, 0x2C) = 0.0f;
    At<int32_t>(shape, 0x30) = 2;
    Rgba colour;
    CV_Shape_SetSegment(shape, 0, 0.0f, 240.0f, 380.0f, 50.0f, 1.5f,
                        CV_Rgba_Set(&colour, bottom[0], bottom[1], bottom[2], bottom[3]));
    CV_Shape_SetSegment(shape, 1, 1.0f, 400.0f, 380.0f, 50.0f, 1.5f, CV_Rgba_Set(&colour, top[0], top[1], top[2], top[3]));
}
} // namespace

// 0x6D8E60 (thiscall, slot 4): starts: the rates, the two meters (the backing and the
// fill), their colours
void __fastcall Striker_Start(uint8_t* self, re::Edx) {
    At<float>(self, 0xC8) = 0.0f;
    At<uint8_t>(self, 4) = 1;
    At<uint8_t>(self, 5) = 0;
    At<float>(self, 0xCC) = 1.8f;
    At<uint8_t>(self, 0xC4) = 1;
    At<float>(self, 0xD0) = 9.0f;
    At<float>(self, 0xD4) = 0.01f;
    At<float>(self, 0xD8) = 2.0f;
    At<float>(self, 0xDC) = 0.08f;
    At<float>(self, 0xE4) = 0.0f;
    At<float>(self, 0xE0) = MsToSeconds(cv_timeMs);
    At<float>(self, 0x4B8) = 0.0f;
    At<float>(self, 0x4BC) = 0.0f;
    const int grey[4] = {0x10, 0x10, 0x10, 0xA0};
    SetUpMeter(self + 0x2D0, grey, grey);
    At<int32_t>(self, 0x4B4) = 2;
    At<float>(self, 0x4A4) = 1.5f;
    Rgba colour;
    At<Rgba>(self, 0x4A8) = *CV_Rgba_Set(&colour, 0x10, 0x10, 0x10, 0xFF);
    const int green[4] = {0, 0xEB, 0, 0x64};
    const int red[4] = {0xEB, 0, 0, 0x64};
    SetUpMeter(self + 0xE8, green, red);
    At<float>(self, 0x2AC) = 0.8f;
    At<float>(self, 0x2B0) = 0.1f;
    At<int32_t>(self, 0x2CC) = 2;
    Rgba clear;
    CV_Rgba_Set(&clear, 0xEB, 0xEB, 0, 0);
    Rgba yellow;
    At<Rgba>(self, 0x2B4) = *CV_Rgba_Set(&yellow, 0xEB, 0xEB, 0, 0x64);
    At<Rgba>(self, 0x2B8) = clear;
    At<float>(self, 0x2BC) = 1.5f;
    At<Rgba>(self, 0x2C0) = *CV_Rgba_Set(&colour, 0x10, 0x10, 0x10, 0xFF);
}
CARNIVAL_REVERSED("CarnivalStriker::Start", 0x006D8E60, &Striker_Start);

// 0x6D9240 (thiscall, slot 1): tapping fills (faster for quick taps), the level holds a
// moment at the top, then drains
void __fastcall Striker_Update(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 4) == 0 || At<uint8_t>(self, 0xC4) != 0) {
        return;
    }
    float& level = At<float>(self, 0xC8);
    if (!(1.0f > level)) {
        At<uint8_t>(self, 6) = 1;
        return;
    }
    if (CV_Pad_JustPressed(7, 0) != 0) {
        float rate = At<float>(self, 0xD0) / 60.0f;
        float add = rate * cv_timeStep;
        if (At<float>(self, 0xD4) < At<float>(self, 0xD8)) {
            float since = CV_GetTimeSeconds() - At<float>(self, 0xE0);
            float t = (CV_Clamp(since, At<float>(self, 0xD4), At<float>(self, 0xD8)) - At<float>(self, 0xD4)) /
                      (At<float>(self, 0xD8) - At<float>(self, 0xD4));
            add = rate * 2.0999999f * (1.0f - t);
        }
        float next = add + level;
        level = next;
        if (!(1.0f > next) && next == next) {
            level = 1.0f;
        }
        At<float>(self, 0xE0) = CV_GetTimeSeconds();
    }
    if (CV_Pad_JustPressed(8, 0) != 0) {
        At<uint8_t>(self, 4) = 0;
    }
    float& hold = At<float>(self, 0xE4);
    if (0.0f != hold) {
        float step = cv_strikerDecay * cv_timeStep;
        float next = hold - step;
        hold = next;
        if (next > 0.0f || next != next) {
            return;
        }
        hold = 0.0f;
        level = level - At<float>(self, 0xCC) / 60.0f * cv_timeStep;
        return;
    }
    if (1.0f == level) {
        hold = At<float>(self, 0xDC);
        return;
    }
    float next = level - At<float>(self, 0xCC) / 60.0f * cv_timeStep;
    level = next;
    if (next < 0.0f || next == 0.0f) {
        level = 0.0f;
    }
}
CARNIVAL_REVERSED("CarnivalStriker::Update", 0x006D9240, &Striker_Update);

// 0x6D9430 (thiscall, slot 2): the button prompt (pulsing while tapping), or, once
// stopped, the meter's flash
void __fastcall Striker_Draw(uint8_t* self, re::Edx) {
    if (At<uint8_t>(self, 5) == 0) {
        return;
    }
    float sx = cv_screenScaleX;
    float sy = cv_screenScaleY;
    uint8_t done = At<uint8_t>(self, 6);
    if (done == 0 && At<uint8_t>(self, 0xC4) == 0) {
        float& flash = At<float>(self, 0x4B8);
        if (!(0.0f < flash) && flash == flash) {
            flash = 0.3f;
        }
        Rgba a, b, c, d;
        CV_Rgba_Set(&a, 0xC0, 0xC0, 0xC0, 0x64);
        CV_Rgba_Set(&b, 0xEB, 0xEB, 0xEB, 0xFF);
        CV_Rgba_Set(&c, 0x10, 0x10, 0x10, 0x5A);
        CV_Rgba_Set(&d, 0x10, 0x10, 0x10, 0xC8);
        float alpha = 0.0f;
        if (0.0f < flash) {
            alpha = Pulse(1.0f - flash / 0.3f);
            float step = cv_strikerDecay * cv_timeStep;
            float next = flash - step;
            flash = next;
            if (!(next > 0.0f) && next == next) {
                flash = 0.0f;
            }
        }
        float scale = alpha * 0.20000005f + 1.0f;
        void* texture = CV_Hud_GetTexture(7);
        Rgba shade = LerpColour(c, d, alpha);
        float position[2];
        CV_ScreenPosition(position, 0.0f, 99.0f, 1, 1);
        float uvA[2] = {1.0f, 1.0f};
        float uvB[2] = {0.0f, 0.0f};
        float size = scale * 17.6f;
        float h = sy * size;
        float w = size * sx;
        CV_DrawRectUV(position[0] - w, position[1] - h, position[0] + w, position[1] + h, texture, &shade, uvB, uvA);
        Rgba tint = LerpColour(a, b, alpha);
        float uvC[2] = {1.0f, 1.0f};
        float uvD[2] = {0.0f, 0.0f};
        size = scale * 16.0f;
        h = sy * size;
        w = size * sx;
        CV_DrawRectUV(position[0] - w, position[1] - h, position[0] + w, position[1] + h, texture, &tint, uvD, uvC);
        return;
    }
    if (done == 0 && At<uint8_t>(self, 0xC4) == 0) {
        return;
    }
    float& flash = At<float>(self, 0x4BC);
    if (0.0f == flash) {
        flash = 0.4f;
    }
    Rgba f, g;
    CV_Rgba_Set(&f, 0x10, 0x10, 0x10, 0xFF);
    CV_Rgba_Set(&g, 0xEB, 0xEB, 0xEB, 0xFF);
    float alpha = 0.0f;
    if (0.0f < flash) {
        alpha = Pulse(1.0f - flash / 0.4f);
        float step = cv_strikerDecay * cv_timeStep;
        float next = flash - step;
        flash = next;
        if (!(next > 0.0f) && next == next) {
            flash = -1.0f;
        }
    }
    CV_Hud_GetTexture(7);
    Rgba colour = LerpColour(f, g, alpha);
    float width = alpha * 0.20000005f + 1.0f;
    At<float>(self, 0x2BC) = width * 1.5f;
    At<uint8_t>(self, 0x2C2) = colour.b;
    At<uint8_t>(self, 0x2C0) = colour.r;
    At<uint8_t>(self, 0x2C1) = colour.g;
    At<uint8_t>(self, 0x2C3) = colour.a;
}
CARNIVAL_REVERSED("CarnivalStriker::Draw", 0x006D9430, &Striker_Draw);

// 0x6D9B80 (thiscall): the constructor
uint8_t* __fastcall Striker_Construct(uint8_t* self, re::Edx) {
    CV_Minigame_Construct(self);
    At<uint32_t>(self, 0) = VT_Striker;
    CV_Shape_Construct(self + 0xE8);
    CV_Shape_Construct(self + 0x2D0);
    At<int32_t>(self, 0x38) = 2;
    At<int32_t>(self, 0x3C) = 7;
    return self;
}
CARNIVAL_REVERSED("CarnivalStriker::CarnivalStriker", 0x006D9B80, &Striker_Construct);

// 0x6D9BC0 (thiscall): the deleting destructor
uint8_t* __fastcall Striker_DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    At<uint32_t>(self, 0) = VT_Striker;
    CV_Minigame_Destruct(self);
    if ((flags & 1) != 0) {
        CV_Free(self);
    }
    return self;
}
CARNIVAL_REVERSED("CarnivalStriker::DeletingDestructor", 0x006D9BC0, &Striker_DeletingDestructor);

int __cdecl Native_CarnivalStrikerMeterStart(lua_State*) {
    uint8_t* minigame = cv_minigame;
    At<float>(minigame, 0xE4) = 0.0f;
    At<uint8_t>(minigame, 0xC4) = 0;
    At<float>(minigame, 0xC8) = 0.0f;
    return 0;
}

int __cdecl Native_CarnivalStrikerMeterIsRunning(lua_State* L) {
    uint8_t* minigame = cv_minigame;
    int32_t running = (At<uint8_t>(minigame, 0xC4) == 0 && At<uint8_t>(minigame, 4) != 0) ? 1 : 0;
    // (through a float compare with 0)
    Script::PushBool(L, float(running) == 0.0f ? 0 : 1);
    return 1;
}

int __cdecl Native_CarnivalStrikerMeterGetLevel(lua_State* L) {
    uint8_t* minigame = cv_minigame;
    float level = At<float>(minigame, 0xC8) * 100.0f;
    At<uint8_t>(minigame, 0xC4) = 1;
    Script::PushFloat(L, level);
    return 1;
}

int __cdecl Native_CarnivalStrikerMeterSetParams(lua_State* L) {
    At<float>(cv_minigame, 0xCC) = Script::GetFloat(L, 0);
    At<float>(cv_minigame, 0xD0) = Script::GetFloat(L, 1);
    At<float>(cv_minigame, 0xDC) = Script::GetFloat(L, 2);
    if (Script::GetArgCount(L) > 3) {
        At<float>(cv_minigame, 0xD8) = Script::GetFloat(L, 3);
        if (Script::GetArgCount(L) > 4) {
            At<float>(cv_minigame, 0xD4) = Script::GetFloat(L, 4);
        }
    }
    return 0;
}

#define CARNIVAL_NATIVE(name, address) \
    REVERSED("Native_" #name, address, RE_BODY(Native_##name), nullptr, nullptr, re::Mode::Original)

CARNIVAL_NATIVE(CarnivalBallTossSetCount, 0x006D7F60);
CARNIVAL_NATIVE(CarnivalBallTossIncCount, 0x006D7F90);
CARNIVAL_NATIVE(CarnivalBallTossDecCount, 0x006D7FA0);
CARNIVAL_NATIVE(CarnivalBallTossGetCount, 0x006D7FC0);
CARNIVAL_NATIVE(CarnivalDunkTankGetTargetPos, 0x006D8790);
CARNIVAL_NATIVE(CarnivalShootAddNumber, 0x006D8DF0);
CARNIVAL_NATIVE(CarnivalStrikerMeterStart, 0x006D9BF0);
CARNIVAL_NATIVE(CarnivalStrikerMeterIsRunning, 0x006D9C10);
CARNIVAL_NATIVE(CarnivalStrikerMeterGetLevel, 0x006D9C60);
CARNIVAL_NATIVE(CarnivalStrikerMeterSetParams, 0x006D9CA0);

// ---------------------------------------------------------------------------------------
// MGClassArt's settings (the MGCA_ natives' setters)

// 0x6D9D30 (thiscall): a localised string (its key looked up)
void __fastcall MGClassArt_SetString(uint8_t* self, re::Edx, int index, const char* key) {
    At<const uint16_t*>(self, 0xC4 + index * 4) = CV_Text_Get(TheText, key);
}
CARNIVAL_REVERSED("MGClassArt::SetString", 0x006D9D30, &MGClassArt_SetString);

namespace {
template <int offset>
void __fastcall SetInt(uint8_t* self, re::Edx, int32_t value) {
    At<int32_t>(self, offset) = value;
}
template <int offset>
void __fastcall SetFloat(uint8_t* self, re::Edx, float value) {
    At<float>(self, offset) = value;
}
template <int offset>
void __fastcall SetFloatAt(uint8_t* self, re::Edx, int index, float value) {
    At<float>(self, offset + index * 4) = value;
}
template <int offset>
void __fastcall SetIntAt(uint8_t* self, re::Edx, int index, int32_t value) {
    At<int32_t>(self, offset + index * 4) = value;
}
} // namespace

#define MGCA_SETTER(kind, offset, address) \
    CARNIVAL_REVERSED("MGClassArt::Set" #kind "_" #offset, address, &Set##kind<offset>)

MGCA_SETTER(Int, 0x10C, 0x006D9D60);
MGCA_SETTER(Float, 0x110, 0x006D9D70);
MGCA_SETTER(Float, 0x114, 0x006D9D80);
MGCA_SETTER(Float, 0x118, 0x006D9D90);
MGCA_SETTER(FloatAt, 0x11C, 0x006D9DA0);
MGCA_SETTER(FloatAt, 0x138, 0x006D9DC0);
MGCA_SETTER(FloatAt, 0x154, 0x006D9DE0);
MGCA_SETTER(IntAt, 0x170, 0x006D9E00);
MGCA_SETTER(IntAt, 0x18C, 0x006D9E20);
MGCA_SETTER(Int, 0x1A8, 0x006D9E40);
MGCA_SETTER(Float, 0x1AC, 0x006D9E50);
MGCA_SETTER(Int, 0x1B4, 0x006D9E60);
MGCA_SETTER(Float, 0x1B0, 0x006D9E70);
MGCA_SETTER(FloatAt, 0x1B8, 0x006D9E80);
MGCA_SETTER(FloatAt, 0x1C8, 0x006D9EA0);
MGCA_SETTER(FloatAt, 0x1D8, 0x006D9EC0);
MGCA_SETTER(IntAt, 0x1E8, 0x006D9EE0);
MGCA_SETTER(Int, 0x1F8, 0x006D9F00);
MGCA_SETTER(IntAt, 0x1FC, 0x006D9F10);
MGCA_SETTER(FloatAt, 0x20C, 0x006D9F30);
MGCA_SETTER(Float, 0x21C, 0x006D9F50);
MGCA_SETTER(Float, 0x220, 0x006D9F60);
MGCA_SETTER(Float, 0x224, 0x006D9F70);
MGCA_SETTER(Float, 0x228, 0x006D9F80);
MGCA_SETTER(Int, 0x22C, 0x006D9F90);
MGCA_SETTER(Int, 0x230, 0x006D9FA0);

// 0x6D9FB0 (thiscall): a 0x14-byte entry appended
void __fastcall MGClassArt_AddEntry(uint8_t* self, re::Edx, const uint32_t* entry) {
    int32_t& count = At<int32_t>(self, 0x244);
    uint32_t* slot = &At<uint32_t>(self, 0x248 + count * 0x14);
    for (int i = 0; i < 5; i++) {
        slot[i] = entry[i];
    }
    count++;
}
CARNIVAL_REVERSED("MGClassArt::AddEntry", 0x006D9FB0, &MGClassArt_AddEntry);

// 0x6DA000 (thiscall): +0x610
int32_t __fastcall MGClassArt_Get610(uint8_t* self, re::Edx) {
    return At<int32_t>(self, 0x610);
}
CARNIVAL_REVERSED("MGClassArt::Get610", 0x006DA000, &MGClassArt_Get610);
