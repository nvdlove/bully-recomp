// Lawn mowing (PLawnMowing, vtable 0x949438, minigame kind 11): the lawn (a rectangle the
// script sets) split into 60x60 cells found by probing the ground (only grass surfaces
// count); the mower cuts the cells it passes (flowers and weeds cost points, power-ups
// give them), its engine smokes as it takes damage, and the uncut grass is drawn as
// crossed quads with random jitter (0x7093D0..0x70B5B1, with the LawnMowing natives).
//
// The minigame: +0xC4 the grass blades' twelve template vertices (0x24 each), +0x274 the
// cells to cut (a percentage of the grass), +0x278 the grass cells, +0x27C cut, +0x280 the
// lawn (x0, y0, x1, y1), +0x290 the lawn is set, +0x294.. the texture indices, +0x2A8 /
// +0x2AC / +0x2B0 power-ups left, +0x2B4 the mower's health, +0x2B8 boosting (+0x2BC since
// when), +0x2B9 slowed, +0x2C0 the slow-down, +0x2C4 the cells (0x18 each: the ground's
// z, the state, a power-up flag, -1, an effect proxy), +0x15444.. the effects (the boost,
// three smokes, the flowers), +0x1549C.. HUD flashes and their times, +0x154AC.. three
// areas (+0x15584 which).
// Cell states: 0 none, 1 grass (2 cut), 3 a flower (4 cut), 5 a power-up (6 taken), 7 a
// weed (8 cut). The x87 runs at 24-bit precision here (the main thread): float throughout.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "game/SchoolClasses.h"
#include "game/ScriptApi.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using school::At;
using school::GameString;

struct LawnVec {
    float x, y, z;
};
static_assert(sizeof(LawnVec) == 0xC);

// a cell of the lawn (from +0x2C4)
struct LawnCell {
    float z;          // +0x0 the ground
    int32_t state;    // +0x4
    uint8_t bonus;    // +0x8 a power-up's icon here
    uint8_t unknown9[3];
    int32_t unknownC; // +0xC (-1)
    re::Ptr32<void> proxy;      // +0x10 the icon's effect
    int32_t live;     // +0x14
};
static_assert(sizeof(LawnCell) == 0x18);

constexpr int kLawnCells = 0x2C4;
constexpr int kLawnBoost = 0x15448;    // proxies (their effect ids 4 bytes before)
constexpr int kLawnSmoke1 = 0x15458;
constexpr int kLawnSmoke2 = 0x15468;
constexpr int kLawnSmoke3 = 0x15478;
constexpr int kLawnBonus = 0x15488;
constexpr int kLawnFlowers = 0x15494;

GAME_VAR(uint8_t*, lawnCurrentMinigame, 0x020C8704);
GAME_VAR(uint8_t*, lawnPlayerPed, 0x00C1AEA8);
GAME_VAR(uint32_t, lawnTimeInMs, 0x00C1A9B4);
GAME_VAR(void*, lawnEffectLibrary, 0x00B6F1E8);
GAME_VAR(void*, lawnEffectManager, 0x00B6F1D4);
GAME_VAR(uint16_t, lawnDefaultEffect, 0x020C8FF4);
GAME_VAR(void*, lawnCameraNode, 0x00C3CCF4);
GAME_VAR(int32_t, lawnBatchCount, 0x00CA2AE8);
GAME_VAR(int32_t, lawnBatchCA2AEC, 0x00CA2AEC);
inline uint8_t* const lawnBatch = reinterpret_cast<uint8_t*>(0x00BD7800);   // Im3D vertices
GAME_VAR(float, lawnScreenScaleX, 0x00A957A8);
GAME_VAR(uint32_t, lawnColourAhead, 0x020C881C);
GAME_VAR(uint32_t, lawnColourBehind, 0x020C8820);
GAME_ARRAY(uint8_t, lawnFontColour, 0x0208B764);
GAME_VAR(int16_t, lawnFontShadow, 0x0208B784);
GAME_VAR(int16_t, lawnFontEdge, 0x0208B786);
GAME_VAR(uint8_t, lawnFontFlags, 0x0208B78A);
inline void* const lawnSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const lawnTheText = reinterpret_cast<void*>(0x020C2FA4);

GAME_FN(void, __cdecl, Lawn_Free, 0x005EEAB0, void* p);
inline int __cdecl Lawn_Rand() { return crt::Rand(); }
inline void __cdecl Lawn_Srand(uint32_t seed) { crt::Srand((unsigned int)seed); }
GAME_FN(uint32_t, __cdecl, Lawn_GetCurrentTime, 0x0045BCA0);
GAME_FN(void*, __cdecl, Lawn_FindPlayerVehicle, 0x00446970, void* player);
GAME_FN(float*, __cdecl, Lawn_FindPlayerCoors, 0x004469E0, void* player);
GAME_FN(int32_t*, __thiscall, Lawn_Vehicle_GetState, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(void*, __thiscall, Lawn_GetMatrix, 0x0046DD20, void* entity);
GAME_FN(void, __thiscall, Lawn_Matrix_Copy, 0x00412A10, void* m, const void* from);
GAME_FN(void, __thiscall, Lawn_Matrix_Assign, 0x00412720, void* m, const void* from);
GAME_FN(void, __thiscall, Lawn_Matrix_Fn412AC0, 0x00412AC0, void* m, void* out);
GAME_FN(LawnVec*, __cdecl, Lawn_TransformDirection, 0x0041C490, LawnVec* out, const void* m, const LawnVec* v);
GAME_FN(LawnVec*, __cdecl, Lawn_VectorAdd, 0x004129A0, LawnVec* out, const LawnVec* a, const LawnVec* b);
GAME_FN(LawnVec*, __cdecl, Lawn_VectorSub, 0x00414B70, LawnVec* out, const LawnVec* a, const LawnVec* b);
GAME_FN(LawnVec*, __cdecl, Lawn_VectorScale, 0x00412350, LawnVec* out, const LawnVec* v, float s);
GAME_FN(float, __thiscall, Lawn_Vector_Magnitude, 0x0040DFB0, const LawnVec* v);
GAME_FN(float, __cdecl, Lawn_DistanceSqr, 0x00472D00, const LawnVec* a, const LawnVec* b);
GAME_FN(void, __thiscall, Lawn_SetMoveSpeed, 0x0041A440, void* physical, const LawnVec* speed);
GAME_FN(float, __cdecl, Lawn_MaxF, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, Lawn_MinF, 0x00414470, float a, float b);
GAME_FN(float, __cdecl, Lawn_Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Lawn_ProcessVerticalLine, 0x004629C0, const LawnVec* pos, float zStop, void* colPoint,
        re::Ptr32<void>* entity, int buildings, int vehicles);
GAME_FN(void, __cdecl, Lawn_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(uint16_t, __thiscall, Lawn_EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(void, __thiscall, Lawn_EffectManager_Start, 0x00668290, void* manager, void* id, void* proxy, int);
GAME_FN(void, __thiscall, Lawn_Proxy_Construct, 0x00668E90, void* proxy);
GAME_FN(void, __thiscall, Lawn_Proxy_Destruct, 0x00668EB0, void* proxy);
GAME_FN(void, __thiscall, Lawn_Proxy_Kill, 0x00668BA0, void* proxy);
GAME_FN(void, __thiscall, Lawn_Proxy_SlowKill, 0x00668D00, void* proxy, float time);
GAME_FN(void, __thiscall, Lawn_Proxy_SetTransform, 0x00668EC0, void* proxy, const void* m);
GAME_FN(void, __thiscall, Lawn_Proxy_SetPosition, 0x00669040, void* proxy, const LawnVec* pos, int);
GAME_FN(void, __thiscall, Lawn_Proxy_SetParticlesA_x64, 0x00668C40, void* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Lawn_Proxy_SetParticlesA(void* a0, float a1) {
    return Lawn_Proxy_SetParticlesA_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Lawn_Proxy_SetParticlesB_x64, 0x00668C70, void* proxy, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Lawn_Proxy_SetParticlesB(void* a0, float a1) {
    return Lawn_Proxy_SetParticlesB_x64(a0, re::BitsOf(a1));
}
GAME_FN(void*, __cdecl, Lawn_Camera_Fn6C82F0, 0x006C82F0, void* node);
GAME_FN(uint8_t*, __cdecl, Lawn_GetTransform, 0x006C9A90, void* frame);
GAME_FN(void, __cdecl, Lawn_SetRenderState, 0x00519790, int state, int value);
GAME_FN(void*, __cdecl, Lawn_TextureGetRaster, 0x0051A120, void* texture);
GAME_FN(void*, __cdecl, Lawn_Im3DTransform, 0x005EFAF0, void* vertices, int count, void* matrix, int flags);
GAME_FN(void, __cdecl, Lawn_Im3DRenderPrimitive, 0x005F09D0, int primitive);
GAME_FN(void, __cdecl, Lawn_Im3DEnd, 0x005EFB20);
GAME_FN(uint8_t*, __thiscall, Lawn_RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Lawn_Font_Reset, 0x00688E60);
GAME_FN(void, __cdecl, Lawn_Font_SetSizePreset, 0x00688050, int preset);
GAME_FN(void, __cdecl, Lawn_Font_AddNumber, 0x006880D0, float number);
GAME_FN(void, __cdecl, Lawn_Font_Format, 0x00688ED0, uint16_t* out, const uint16_t* text, int size);
GAME_FN(void, __cdecl, Lawn_Font_PrintString, 0x0068D450, float x, float y, const uint16_t* text, int);
GAME_FN(const uint16_t*, __thiscall, Lawn_Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void*, __thiscall, Lawn_Sounds_Play, 0x0059B8A0, void* sounds, uint32_t id, const float* pos, int, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Lawn_Minigame_Construct, 0x007059D0, void* mg);
GAME_FN(void, __thiscall, Lawn_Minigame_Destruct, 0x00705A50, void* mg);
GAME_FN(void, __thiscall, Lawn_Minigame_UnloadTextures, 0x00704DE0, void* mg);
GAME_FN(void, __thiscall, Lawn_Minigame_LoadTextures, 0x00704D30, void* mg, int count,
        const school::ClassTextureEntry* entries, const char* txd);
GAME_FN(void, __thiscall, Lawn_Minigame_LoadSounds, 0x00704E90, void* mg, int count,
        const school::ClassSoundEntry* entries, const char* bank);
// (this file's)
GAME_FN(void, __thiscall, Lawn_BonusTaken, 0x007093D0, void* mg);
GAME_FN(void, __thiscall, Lawn_Destruct, 0x007093F0, void* mg);
GAME_FN(int32_t, __thiscall, Lawn_GetCut, 0x00709520, void* mg);
GAME_FN(int32_t, __thiscall, Lawn_GetGrass, 0x00709530, void* mg);
GAME_FN(void, __thiscall, Lawn_UpdateBoost, 0x00709C80, void* mg, void* ped);
GAME_FN(void, __thiscall, Lawn_UpdateSmoke, 0x00709E50, void* mg, void* ped);
GAME_FN(void, __thiscall, Lawn_UpdateCells, 0x0070A2D0, void* mg, void* ped);
GAME_FN(void, __thiscall, Lawn_SetLawnArea, 0x0070B2B0, void* mg, float x0, float y0, float x1, float y1);

inline LawnCell* Cells(uint8_t* mg) {
    return reinterpret_cast<LawnCell*>(mg + kLawnCells);
}
inline uint8_t* Proxy(uint8_t* mg, int offset) {
    return mg + offset;
}
inline bool ProxyLive(uint8_t* mg, int offset) {
    return At<int32_t>(mg, offset + 4) != 0;
}
inline void LawnPlay(uint8_t* mg, int sound) {
    Lawn_Sounds_Play(lawnSounds, At<uint32_t*>(mg, 0x4C)[sound], nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}
// a vehicle-attached effect: the vehicle's matrix moved by `offset` (in its own axes)
void PlaceOnVehicle(void* vehicle, uint8_t* proxy, const LawnVec& offset, bool assign) {
    alignas(16) uint8_t m[0x50];
    if (assign) {
        Lawn_Matrix_Assign(m, Lawn_GetMatrix(vehicle));
    } else {
        Lawn_Matrix_Copy(m, Lawn_GetMatrix(vehicle));
    }
    LawnVec dir;
    Lawn_TransformDirection(&dir, m, &offset);
    LawnVec& pos = *reinterpret_cast<LawnVec*>(m + 0x30);
    pos.x = pos.x + dir.x;
    pos.y = pos.y + dir.y;
    pos.z = pos.z + dir.z;
    Lawn_Proxy_SetTransform(proxy, m);
}
// a value clamped to 0.1..1 the way the game does (0.1's double is the float widened)
inline float Clamp01(float v) {
    if (v < 0.1f) {
        v = 0.1f;
    }
    if (v > 1.0f) {
        v = 1.0f;
    }
    return v;
}

// 0x7093D0: a power-up taken: the HUD's flash
void __fastcall LawnBonusTaken(uint8_t* mg, re::Edx) {
    mg[0x1549D] = 1;
    At<uint32_t>(mg, 0x154A4) = Lawn_GetCurrentTime() + 500;
}

// 0x7093F0: the destructor
void __fastcall LawnDestruct(uint8_t* mg, re::Edx) {
    At<uint32_t>(mg, 0) = 0x00949438;
    static const int kProxies[6] = {kLawnBoost, kLawnSmoke1, kLawnSmoke2, kLawnSmoke3, kLawnBonus, kLawnFlowers};
    for (int offset : kProxies) {
        if (ProxyLive(mg, offset)) {
            Lawn_Proxy_Kill(Proxy(mg, offset));
        }
    }
    LawnCell* cells = Cells(mg);
    for (int i = 0; i < 60 * 60; i++) {
        if (cells[i].live != 0) {
            Lawn_Proxy_Kill(&cells[i].proxy);
        }
    }
    Lawn_Minigame_UnloadTextures(mg);
    for (int k = 5; k >= 0; k--) {
        Lawn_Proxy_Destruct(Proxy(mg, kProxies[k]));
    }
    for (int i = 60 * 60 - 1; i >= 0; i--) {
        Lawn_Proxy_Destruct(&cells[i].proxy);
    }
    Lawn_Minigame_Destruct(mg);
}

// 0x709510 (virtual 4, shared with the shooting gallery): starts
void __fastcall MinigameStart709510(uint8_t* mg, re::Edx) {
    mg[4] = 1;
    mg[5] = 0;
}

// 0x709520 / 0x709530 / 0x709540: the cells cut, the grass cells, the cells to cut
int32_t __fastcall LawnGetCut(uint8_t* mg, re::Edx) {
    return At<int32_t>(mg, 0x27C);
}
int32_t __fastcall LawnGetGrass(uint8_t* mg, re::Edx) {
    return At<int32_t>(mg, 0x278);
}
void __fastcall LawnSetMinScore(uint8_t* mg, re::Edx, int32_t score) {
    At<int32_t>(mg, 0x274) = score;
}

// 0x709550: the constructor
uint8_t* __fastcall LawnConstruct(uint8_t* mg, re::Edx) {
    Lawn_Minigame_Construct(mg);
    At<uint32_t>(mg, 0) = 0x00949438;
    for (int i = 0; i < 12; i++) {
        uint8_t* v = mg + 0xC4 + i * 0x24;
        At<float>(v, 0) = 0.0f;
        At<float>(v, 4) = 0.0f;
        At<float>(v, 8) = 0.0f;
        At<float>(v, 0xC) = 0.0f;
        At<float>(v, 0x10) = 0.0f;
        At<float>(v, 0x14) = 1.0f;
        At<int32_t>(v, 0x18) = 0;
        At<float>(v, 0x1C) = 0.0f;
        At<float>(v, 0x20) = 0.0f;
    }
    LawnCell* cells = Cells(mg);
    for (int i = 0; i < 60 * 60; i++) {
        Lawn_Proxy_Construct(&cells[i].proxy);
    }
    static const int kProxies[6] = {kLawnBoost, kLawnSmoke1, kLawnSmoke2, kLawnSmoke3, kLawnBonus, kLawnFlowers};
    for (int offset : kProxies) {
        Lawn_Proxy_Construct(Proxy(mg, offset));
    }
    for (int offset = 0x154AC; offset <= 0x15580; offset += 4) {
        At<float>(mg, offset) = 0.0f;
    }
    At<int32_t>(mg, 0x38) = 3;
    At<int32_t>(mg, 0x3C) = 0xB;
    At<int32_t>(mg, 0x274) = 0xCA8;
    At<int32_t>(mg, 0x27C) = 0;
    At<int32_t>(mg, 0x278) = 0xE10;
    mg[0x290] = 0;
    for (int i = 0; i < 60 * 60; i++) {
        cells[i].state = 0;
        cells[i].unknownC = -1;
    }
    mg[7] = 0;
    At<float>(mg, 0x2C0) = 0.6f;
    At<int32_t>(mg, 0x2B4) = 1000;
    At<int32_t>(mg, 0x2A8) = 0;
    At<int32_t>(mg, 0x2AC) = 0;
    mg[0x1549C] = 1;
    mg[0x1549D] = 1;
    mg[0x1549E] = 1;
    mg[0x2B9] = 0;
    At<uint32_t>(mg, 0x154A0) = Lawn_GetCurrentTime() + 500;
    At<uint32_t>(mg, 0x154A4) = lawnTimeInMs + 500;
    uint32_t now = Lawn_GetCurrentTime();
    At<int32_t>(mg, 0x2A0) = 4;
    At<uint32_t>(mg, 0x154A8) = now + 500;
    const school::ClassTextureEntry textures[9] = {
            {0, GameString(0x00949584), GameString(0x0094957C)},   // grass
            {1, GameString(0x00949570), GameString(0x00949564)},   // flower_01
            {2, GameString(0x00949558), GameString(0x0094954C)},   // flower_02
            {3, GameString(0x00949540), GameString(0x00949534)},   // flower_03
            {4, GameString(0x00949528), GameString(0x0094951C)},   // flower_04
            {5, GameString(0x00949510), GameString(0x00949504)},   // flower_05
            {6, GameString(0x009494FC), GameString(0x009494F4)},   // weeds
            {7, GameString(0x009494EC), GameString(0x009494E0)},   // orchid
            {8, GameString(0x009494D4), GameString(0x009494C4)},   // mower_play1
    };
    At<int32_t>(mg, 0x294) = 1;
    At<int32_t>(mg, 0x298) = 2;
    At<int32_t>(mg, 0x29C) = 3;
    At<int32_t>(mg, 0x2A4) = 5;
    Lawn_Minigame_LoadTextures(mg, 9, textures, GameString(0x0094958C));   // PLawnM
    const school::ClassSoundEntry sounds[3] = {
            {0, GameString(0x009494B0)},   // TurboPower
            {1, GameString(0x009494A4)},   // PowerDown
            {2, GameString(0x00949498)},   // BonusSound
    };
    Lawn_Minigame_LoadSounds(mg, 3, sounds, GameString(0x009494BC));   // LwnMwr
    // the blades' template: green, their uvs
    static const int kOnes[] = {0xE0, 0xE4, 0x108, 0x170, 0x194, 0x198, 0x1B8, 0x1BC, 0x1E0, 0x248, 0x26C, 0x270};
    static const int kZeros[] = {0x104, 0x128, 0x12C, 0x14C, 0x150, 0x174, 0x1DC, 0x200, 0x204, 0x224, 0x228, 0x24C};
    for (int offset : kZeros) {
        At<float>(mg, offset) = 0.0f;
    }
    for (int i = 0; i < 12; i++) {
        At<uint32_t>(mg, 0xDC + i * 0x24) = 0xFF7DD27D;
    }
    for (int offset : kOnes) {
        At<float>(mg, offset) = 1.0f;
    }
    At<uint16_t>(mg, 0x15444) = lawnDefaultEffect;
    At<uint16_t>(mg, 0x15454) = Lawn_EffectLibrary_Find(lawnEffectLibrary, GameString(0x0094948C));   // MowerSmoke1
    At<uint16_t>(mg, 0x15464) = Lawn_EffectLibrary_Find(lawnEffectLibrary, GameString(0x00949480));   // MowerSmoke2
    At<uint16_t>(mg, 0x15474) = Lawn_EffectLibrary_Find(lawnEffectLibrary, GameString(0x00949474));   // MowerSmoke3
    At<uint16_t>(mg, 0x15484) = Lawn_EffectLibrary_Find(lawnEffectLibrary, GameString(0x00949468));   // MowerBoost
    At<uint16_t>(mg, 0x15490) = Lawn_EffectLibrary_Find(lawnEffectLibrary, GameString(0x00949458));   // MowerFlowers
    return mg;
}

// 0x709AF0 (virtual 0): the deleting destructor
uint8_t* __fastcall LawnDeletingDestructor(uint8_t* mg, re::Edx, int flags) {
    Lawn_Destruct(mg);
    if ((flags & 1) != 0) {
        Lawn_Free(mg);
    }
    return mg;
}

// 0x709B10 (virtual 2): the share cut, percent (coloured ahead of the target or not)
void __fastcall LawnDraw(uint8_t* mg, re::Edx) {
    if (mg[5] == 0) {
        return;
    }
    Lawn_Font_Reset();
    uint8_t black[4];
    lawnFontShadow = 0;
    uint8_t* c = Lawn_RGBA_Set(black, 0, 0, 0, 0xFF);
    lawnFontColour[0] = c[0];
    lawnFontColour[1] = c[1];
    lawnFontColour[2] = c[2];
    lawnFontColour[3] = c[3];
    Lawn_Font_SetSizePreset(3);
    int32_t cut = At<int32_t>(mg, 0x27C);
    int32_t grass = At<int32_t>(mg, 0x278);
    float x = float(double(lawnScreenScaleX) * 87.0);
    lawnFontEdge = 3;
    float share = float(cut) / float(grass);
    // (__ftol2: a 64-bit truncation, eax its low half: 0 for no grass's NaN / infinity)
    int percent = int32_t(uint32_t(int64_t(share * 100.0f)));
    Lawn_Font_AddNumber(float(percent));
    uint16_t text[0x40];
    Lawn_Font_Format(text, Lawn_Text_Get(lawnTheText, GameString(0x00949594)), 0x40);   // LMOW_SCORENUM
    uint32_t colour = At<int32_t>(mg, 0x27C) > At<int32_t>(mg, 0x274) ? lawnColourAhead : lawnColourBehind;
    lawnFontColour[0] = uint8_t(colour);
    lawnFontColour[2] = uint8_t(colour >> 16);
    lawnFontColour[3] = uint8_t(colour >> 24);
    lawnFontFlags |= 0x20;
    lawnFontColour[1] = uint8_t(colour >> 8);
    lawnFontShadow = 2;
    Lawn_Font_PrintString(x, 115.0f, text, 1);
    lawnFontFlags &= 0xDF;
}

// 0x709C80: the boost: buttons 6 / 9 give a slow mower a push; its effect at the mower's
// back while it lasts (3.5 seconds)
void __fastcall LawnUpdateBoost(uint8_t* mg, re::Edx, void* ped) {
    if (ped == nullptr) {
        return;
    }
    if (mg[0x2B8] == 0) {
        if (ProxyLive(mg, kLawnBonus)) {
            Lawn_Proxy_Kill(Proxy(mg, kLawnBonus));
        }
        return;
    }
    uint8_t* vehicle = static_cast<uint8_t*>(Lawn_FindPlayerVehicle(ped));
    if (vehicle == nullptr) {
        return;
    }
    if ((Lawn_Pad_GetValue(6, 0) != 0.0f || Lawn_Pad_GetValue(9, 0) != 0.0f) && vehicle[0x464] == 0) {
        LawnVec speed = At<LawnVec>(vehicle, 0x124);
        if (Lawn_Vector_Magnitude(&speed) < 0.18f) {
            LawnVec push;
            Lawn_SetMoveSpeed(vehicle, Lawn_VectorScale(&push, &At<LawnVec>(vehicle, 0x124), 1.2f));
        }
    }
    float until = At<float>(mg, 0x2BC) + 3500.0f;
    uint32_t now = Lawn_GetCurrentTime();
    // (the time read exactly unless the sign bit's set: then +2^32 rounded to float)
    double nowValue = int32_t(now) >= 0 ? double(now) : double(float(double(int32_t(now)) + 4294967296.0));
    if (nowValue > double(until)) {
        mg[0x2B8] = 0;
    }
    if (!ProxyLive(mg, kLawnBonus)) {
        Lawn_EffectManager_Start(lawnEffectManager, mg + 0x15484, Proxy(mg, kLawnBonus), 0);
        Lawn_Proxy_SetParticlesA(Proxy(mg, kLawnBonus), 1.0f);
    }
    PlaceOnVehicle(vehicle, Proxy(mg, kLawnBonus), LawnVec{0.0f, -0.9f, 0.005f}, false);
}

// 0x709E50: the mower's damage (bumping things while moving costs health: more while
// reversing) and its smoke, thicker as it fails; badly damaged it slows down
void __fastcall LawnUpdateSmoke(uint8_t* mg, re::Edx, void* ped) {
    if (ped == nullptr) {
        return;
    }
    uint8_t* vehicle = static_cast<uint8_t*>(Lawn_FindPlayerVehicle(ped));
    if (vehicle == nullptr) {
        return;
    }
    LawnVec v = At<LawnVec>(vehicle, 0x124);
    float sum = (v.x * v.x + v.y * v.y) + v.z * v.z;
    float speed = school::CrtSqrt(std::fabs(sum));
    if (speed > 0.04f) {
        float gear = At<float>(vehicle, 0x45C);
        int damage;
        if (5.0f < gear) {
            damage = 50;
        } else {
            damage = -5.0f > gear ? 100 : 50;
        }
        if (vehicle[0x464] != 0) {
            At<int32_t>(mg, 0x2B4) -= damage;
            vehicle[0x464] = 0;
        }
    }
    int32_t health = At<int32_t>(mg, 0x2B4);
    if (health >= 600) {
        return;
    }
    if (!ProxyLive(mg, kLawnSmoke1)) {
        Lawn_EffectManager_Start(lawnEffectManager, mg + 0x15454, Proxy(mg, kLawnSmoke1), 0);
        At<float>(mg, 0x15460) = 0.1f;
    }
    PlaceOnVehicle(vehicle, Proxy(mg, kLawnSmoke1), LawnVec{0.2f, 0.2f, 0.3f}, true);
    float s1 = At<float>(mg, 0x15460) + 0.25f;
    At<float>(mg, 0x15460) = s1;
    s1 = Clamp01(s1);
    At<float>(mg, 0x15460) = s1;
    Lawn_Proxy_SetParticlesA(Proxy(mg, kLawnSmoke1), s1);
    Lawn_Proxy_SetParticlesB(Proxy(mg, kLawnSmoke1), At<float>(mg, 0x15460));
    if (health < 0x14A && mg[0x2B9] == 0) {
        mg[0x2B9] = 1;
        At<float>(vehicle, 0x154) = At<float>(mg, 0x2C0) * At<float>(vehicle, 0x154);
    }
    if (health >= 0xFA) {
        return;
    }
    if (!ProxyLive(mg, kLawnSmoke2)) {
        Lawn_EffectManager_Start(lawnEffectManager, mg + 0x15464, Proxy(mg, kLawnSmoke2), 0);
        At<float>(mg, 0x15470) = 0.1f;
    }
    PlaceOnVehicle(vehicle, Proxy(mg, kLawnSmoke2), LawnVec{-0.33f, 0.3f, 0.3f}, true);
    At<float>(mg, 0x15470) = At<float>(mg, 0x15470) + 0.25f;
    // (clamped from the first smoke's value)
    float s2 = Clamp01(At<float>(mg, 0x15460));
    At<float>(mg, 0x15470) = s2;
    Lawn_Proxy_SetParticlesA(Proxy(mg, kLawnSmoke2), s2);
    Lawn_Proxy_SetParticlesB(Proxy(mg, kLawnSmoke2), At<float>(mg, 0x15470));
    if (health >= 0x32) {
        return;
    }
    if (!ProxyLive(mg, kLawnSmoke3)) {
        Lawn_EffectManager_Start(lawnEffectManager, mg + 0x15474, Proxy(mg, kLawnSmoke3), 0);
        At<float>(mg, 0x15480) = 0.1f;
    }
    PlaceOnVehicle(vehicle, Proxy(mg, kLawnSmoke3), LawnVec{0.02f, 0.82f, 0.3f}, true);
    float s3 = Lawn_MinF(1.0f, Lawn_MaxF(0.1f, At<float>(mg, 0x15460)));
    At<float>(mg, 0x15480) = s3;
    Lawn_Proxy_SetParticlesA(Proxy(mg, kLawnSmoke3), s3);
    Lawn_Proxy_SetParticlesB(Proxy(mg, kLawnSmoke3), At<float>(mg, 0x15480));
}

// 0x70A2D0: the cells the mower passes over: grass cut, flowers and weeds cost points,
// power-ups taken (their icons), the cutting effect's strength
void __fastcall LawnUpdateCells(uint8_t* mg, re::Edx, void* ped) {
    if (ped == nullptr) {
        return;
    }
    int32_t state = 0;
    if (mg[0x290] == 1 && Lawn_FindPlayerVehicle(ped) != nullptr &&
        *Lawn_Vehicle_GetState(Lawn_FindPlayerVehicle(ped), &state) == 6) {
        float* coors = Lawn_FindPlayerCoors(ped);
        LawnVec mower = {coors[0], coors[1], coors[2]};
        uint8_t* boost = Proxy(mg, kLawnBoost);
        if (!ProxyLive(mg, kLawnBoost)) {
            Lawn_EffectManager_Start(lawnEffectManager, mg + 0x15444, boost, 0);
            At<float>(mg, 0x15450) = 0.8f;
        }
        alignas(16) uint8_t m[0x50];
        Lawn_Matrix_Copy(m, Lawn_GetMatrix(Lawn_FindPlayerVehicle(ped)));
        LawnVec offset = {0.65f, -0.2f, -0.4f};
        LawnVec dir;
        Lawn_TransformDirection(&dir, m, &offset);
        LawnVec& pos = *reinterpret_cast<LawnVec*>(m + 0x30);
        pos.x = pos.x + dir.x;
        pos.y = pos.y + dir.y;
        pos.z = pos.z + dir.z;
        Lawn_Proxy_SetTransform(boost, m);
        At<float>(mg, 0x15450) = At<float>(mg, 0x15450) - 0.25f;
        alignas(16) uint8_t other[0x50];
        Lawn_Matrix_Fn412AC0(m, other);
        Lawn_Proxy_SetTransform(boost, other);
        At<float>(mg, 0x15450) = At<float>(mg, 0x15450) - 0.25f;
        float cellW = (At<float>(mg, 0x288) - At<float>(mg, 0x280)) / 60.0f;
        float cellH = (At<float>(mg, 0x28C) - At<float>(mg, 0x284)) / 60.0f;
        LawnVec p;
        p.x = At<float>(mg, 0x280);
        LawnCell* cell = Cells(mg);
        for (int row = 0; row < 60; row++) {
            p.y = At<float>(mg, 0x284);
            for (int col = 0; col < 60; col++, cell++) {
                LawnVec gone = {0.0f, 0.0f, 1000.0f};
                bool moveFlowers = false;
                if (cell->state == 1) {
                    p.z = cell->z;
                    LawnVec d;
                    Lawn_VectorSub(&d, &p, &mower);
                    float d2 = (d.y * d.y + d.x * d.x) + d.z * d.z;
                    if (d2 < 0.8f) {
                        cell->state = 2;
                        At<float>(mg, 0x15450) = At<float>(mg, 0x15450) + 0.5f;
                        At<int32_t>(mg, 0x27C) += 1;
                        moveFlowers = true;
                    }
                } else if (cell->state == 3) {
                    p.z = cell->z;
                    LawnVec d;
                    Lawn_VectorSub(&d, &p, &mower);
                    float d2 = (d.y * d.y + d.x * d.x) + d.z * d.z;
                    if (d2 < 0.8f) {
                        LawnPlay(mg, 1);
                        cell->state = 4;
                        At<int32_t>(mg, 0x27C) -= 2;
                        uint8_t* flowers = Proxy(mg, kLawnFlowers);
                        if (!ProxyLive(mg, kLawnFlowers)) {
                            Lawn_EffectManager_Start(lawnEffectManager, mg + 0x15490, flowers, 0);
                        }
                        Lawn_Proxy_SlowKill(flowers, 2.0f);
                        Lawn_Proxy_SetTransform(flowers, m);
                        Lawn_Proxy_SetParticlesA(flowers, 1.0f);
                    }
                } else if (cell->state == 5) {
                    p.z = cell->z;
                    if (Lawn_DistanceSqr(&p, &mower) < 0.8f) {
                        LawnPlay(mg, 2);
                        cell->state = 6;
                        At<float>(mg, 0x15450) = At<float>(mg, 0x15450) + 0.5f;
                        At<int32_t>(mg, 0x27C) += 4;
                        At<int32_t>(mg, 0x2A8) -= 1;
                        mg[0x1549C] = 1;
                        At<uint32_t>(mg, 0x154A0) = Lawn_GetCurrentTime() + 500;
                        moveFlowers = true;
                    }
                } else if (cell->state == 7) {
                    p.z = cell->z;
                    if (Lawn_DistanceSqr(&p, &mower) < 0.8f) {
                        LawnPlay(mg, 1);
                        cell->state = 8;
                        At<int32_t>(mg, 0x27C) -= 4;
                        At<int32_t>(mg, 0x2B0) -= 1;
                        At<float>(mg, 0x15450) = At<float>(mg, 0x15450) + 0.5f;
                        Lawn_BonusTaken(mg);
                        moveFlowers = true;
                    }
                }
                if (moveFlowers) {
                    Lawn_Proxy_SetPosition(Proxy(mg, kLawnFlowers), &gone, 0);
                    if (ProxyLive(mg, kLawnFlowers)) {
                        Lawn_Proxy_Kill(Proxy(mg, kLawnFlowers));
                    }
                }
                if (cell->bonus != 0) {
                    p.z = cell->z;
                    LawnVec d;
                    Lawn_VectorSub(&d, &p, &mower);
                    float d2 = (d.y * d.y + d.x * d.x) + d.z * d.z;
                    if (cell->live == 0) {
                        uint32_t id = Lawn_EffectLibrary_Find(lawnEffectLibrary, GameString(0x009495C8));   // mower_icon
                        Lawn_EffectManager_Start(lawnEffectManager, &id, &cell->proxy, 0);
                        LawnVec at = p;
                        int area = At<int32_t>(mg, 0x15584);
                        uint8_t* source = area == 1   ? mg + 0x154AC
                                          : area == 2 ? mg + 0x154DC
                                          : area == 3 ? mg + 0x1550C
                                                      : nullptr;
                        at.z = At<float>(source, 8);
                        at.x = cellW * 0.5f + at.x;
                        at.y = 0.5f * cellH + at.y;
                        Lawn_Proxy_SetPosition(&cell->proxy, &at, 0);
                    }
                    Lawn_Proxy_SetParticlesA(&cell->proxy, 1.0f);
                    if (2.0f > d2) {
                        mg[0x2B8] = 1;
                        uint32_t now = Lawn_GetCurrentTime();
                        At<float>(mg, 0x2BC) = float(double(now));
                        LawnPlay(mg, 0);
                        cell->bonus = 0;
                        At<int32_t>(mg, 0x2AC) -= 1;
                        mg[0x1549E] = 1;
                        At<uint32_t>(mg, 0x154A8) = Lawn_GetCurrentTime() + 500;
                        if (cell->live != 0) {
                            LawnVec away = {0.0f, 0.0f, 1000.0f};
                            Lawn_Proxy_SetPosition(&cell->proxy, &away, 0);
                            Lawn_Proxy_Kill(&cell->proxy);
                            cell->bonus = 0;
                        }
                    }
                }
                p.y = p.y + cellH;
            }
            p.x = p.x + cellW;
        }
        float s = Clamp01(At<float>(mg, 0x15450));
        At<float>(mg, 0x15450) = s;
        Lawn_Proxy_SetParticlesA(boost, s);
        Lawn_Proxy_SetParticlesB(boost, At<float>(mg, 0x15450));
    } else {
        if (ProxyLive(mg, kLawnBoost)) {
            Lawn_Proxy_SetParticlesA(Proxy(mg, kLawnBoost), 0.0f);
            Lawn_Proxy_SetParticlesB(Proxy(mg, kLawnBoost), 0.0f);
        }
        if (ProxyLive(mg, kLawnSmoke1)) {
            Lawn_Proxy_SetParticlesA(Proxy(mg, kLawnSmoke1), 0.0f);
            Lawn_Proxy_SetParticlesB(Proxy(mg, kLawnSmoke1), 0.0f);
        }
    }
    Lawn_Srand(lawnTimeInMs);
}

// a random jitter of the grass (r / 32767 * 0.6 - 0.3, the constants floats widened)
inline float Jitter() {
    return float(Lawn_Rand()) / 32767.0f * 0.6f - 0.3f;
}
inline float Spread() {
    return float(Lawn_Rand()) / 32767.0f - 0.5f;
}
inline LawnVec* Vertex(int index) {
    return reinterpret_cast<LawnVec*>(lawnBatch + index * 0x24);
}

// 0x70AAC0 (virtual 3): the uncut grass: two crossed blades per cell (every other cell
// only near the camera), randomly bent with a fixed seed, in batches
void __fastcall LawnDrawGrass(uint8_t* mg, re::Edx) {
    if (mg[0x290] == 1) {
        float cellW = (At<float>(mg, 0x288) - At<float>(mg, 0x280)) / 60.0f;
        float cellH = (At<float>(mg, 0x28C) - At<float>(mg, 0x284)) / 60.0f;
        LawnVec p = {At<float>(mg, 0x280), 0.0f, 0.0f};
        uint8_t* transform = Lawn_GetTransform(Lawn_Camera_Fn6C82F0(lawnCameraNode));
        LawnVec camera = At<LawnVec>(transform, 0x24);
        Lawn_GetTransform(Lawn_Camera_Fn6C82F0(lawnCameraNode));
        Lawn_GetTransform(Lawn_Camera_Fn6C82F0(lawnCameraNode));
        Lawn_GetTransform(Lawn_Camera_Fn6C82F0(lawnCameraNode));
        Lawn_Srand(0x6F);
        Lawn_SetRenderState(8, 0);
        Lawn_SetRenderState(0xC, 0);
        Lawn_SetRenderState(10, 5);
        Lawn_SetRenderState(11, 6);
        Lawn_SetRenderState(0x14, 1);
        Lawn_SetRenderState(6, 1);
        re::Ptr32<void>* textures = At<re::Ptr32<void>*>(mg, 0x1C);
        Lawn_SetRenderState(1, static_cast<int>(reinterpret_cast<uintptr_t>(
                                       Lawn_TextureGetRaster(textures != nullptr ? textures[0] : nullptr))));
        lawnBatchCount = 0;
        lawnBatchCA2AEC = 0;
        LawnCell* cell = Cells(mg);
        for (int row = 0; row < 60; row++) {
            p.y = At<float>(mg, 0x284);
            for (int col = 0; col < 60; col++, cell++) {
                int32_t s = cell->state;
                if (s < 1 || s > 8) {
                    p.y = p.y + cellH;
                    continue;
                }
                p.z = cell->z;
                LawnVec up = {0.0f, 0.0f, 0.6f};
                LawnVec top;
                Lawn_VectorAdd(&top, &p, &up);
                float j1 = Jitter();
                float j2 = Jitter();
                LawnVec a = {j2 + 1.0f, j1 + 0.0f, 0.0f};
                float j3 = Jitter();
                float j4 = Jitter();
                LawnVec b = {j4 + 0.0f, j3 + 1.0f, 0.0f};
                LawnVec q1;
                LawnVec q2;
                LawnVec q3;
                LawnVec q4;
                float r = Spread();
                LawnVec d1 = {Spread(), r, 0.0f};
                Lawn_VectorAdd(&q1, &top, &d1);
                r = Spread();
                LawnVec d2 = {Spread(), r, 0.0f};
                Lawn_VectorAdd(&q2, &p, &d2);
                r = Spread();
                LawnVec d3 = {Spread(), r, 0.0f};
                Lawn_VectorAdd(&q3, &top, &d3);
                r = Spread();
                LawnVec d4 = {Spread(), r, 0.0f};
                Lawn_VectorAdd(&q4, &p, &d4);
                s = cell->state;
                bool cut = s == 2 || s == 4 || s == 6 || s == 8;
                bool draw = !cut;
                if (draw && ((col ^ row) & 1) != 0) {
                    LawnVec d;
                    Lawn_VectorSub(&d, &p, &camera);
                    float d2c = (d.y * d.y + d.x * d.x) + d.z * d.z;
                    draw = !(d2c > 300.0f);
                }
                if (draw) {
                    int count = lawnBatchCount;
                    if (count + 12 >= 0xB32) {
                        if (Lawn_Im3DTransform(lawnBatch, count, nullptr, 1) != nullptr) {
                            Lawn_Im3DRenderPrimitive(3);
                            Lawn_Im3DEnd();
                        }
                        count = 0;
                        lawnBatchCA2AEC = 0;
                    }
                    lawnBatchCount = count + 12;
                    std::memcpy(lawnBatch + count * 0x24, mg + 0xC4, 0x6C * 4);
                    auto put = [](LawnVec* out, const LawnVec& q, const LawnVec& o, bool add) {
                        out->x = add ? q.x + o.x : q.x - o.x;
                        out->y = add ? q.y + o.y : q.y - o.y;
                        out->z = add ? q.z + o.z : q.z - o.z;
                    };
                    put(Vertex(count + 0), q4, a, false);
                    put(Vertex(count + 1), q4, a, true);
                    put(Vertex(count + 2), q3, a, true);
                    put(Vertex(count + 3), q3, a, true);
                    put(Vertex(count + 4), q3, a, false);
                    put(Vertex(count + 5), q4, a, false);
                    put(Vertex(count + 6), q2, b, false);
                    put(Vertex(count + 7), q2, b, true);
                    put(Vertex(count + 8), q1, b, true);
                    put(Vertex(count + 9), q1, b, true);
                    put(Vertex(count + 10), q1, b, false);
                    put(Vertex(count + 11), q2, b, false);
                }
                p.y = p.y + cellH;
            }
            p.x = p.x + cellW;
        }
        if (Lawn_Im3DTransform(lawnBatch, lawnBatchCount, nullptr, 1) != nullptr) {
            Lawn_Im3DRenderPrimitive(3);
            Lawn_Im3DEnd();
        }
        Lawn_Srand(lawnTimeInMs);
    }
    Lawn_SetRenderState(8, 1);
    Lawn_SetRenderState(10, 5);
    Lawn_SetRenderState(11, 6);
    Lawn_SetRenderState(6, 0);
    Lawn_SetRenderState(1, 0);
}

// 0x70B2B0: the lawn's rectangle: each cell's ground found by a line down from 1000 (grass
// only); the cells to cut become that share of the grass
void __fastcall LawnSetArea(uint8_t* mg, re::Edx, float x0, float y0, float x1, float y1) {
    At<float>(mg, 0x280) = x0;
    At<float>(mg, 0x284) = y0;
    mg[0x290] = 1;
    At<float>(mg, 0x288) = x1;
    At<int32_t>(mg, 0x278) = 0;
    At<float>(mg, 0x28C) = y1;
    float cellW = (x1 - x0) / 60.0f;
    float cellH = (y1 - y0) / 60.0f;
    LawnVec p = {x0, 0.0f, 1000.0f};
    // a CColPoint (0x24 bytes), its entity reference at +0xC
    alignas(16) uint8_t col[0x30] = {};
    At<float>(col, 0x24) = 3.4028235e+38f;
    re::Ptr32<void> entity = nullptr;
    LawnCell* cells = Cells(mg);
    for (int base = 0; base < 0xE10; base += 60) {
        p.y = y0;
        for (int j = 0; j < 60; j++) {
            if (Lawn_ProcessVerticalLine(&p, -100.0f, col, &entity, 1, 0) != 0 && col[0x22] == 3) {
                LawnCell& c = cells[j + base];
                c.z = At<float>(col, 8);
                c.bonus = 0;
                c.state = 1;
                At<int32_t>(&c, 0xC) = -1;
                At<int32_t>(mg, 0x278) += 1;
            }
            p.y = p.y + cellH;
        }
        p.x = p.x + cellW;
    }
    At<int32_t>(mg, 0x274) = At<int32_t>(mg, 0x278) * At<int32_t>(mg, 0x274) / 100;
    void* ref = At<void*>(col, 0xC);
    if (ref != nullptr) {
        Lawn_CleanUpOldReference(ref, col + 0xC);
    }
}

// 0x70B460 (virtual 1): the tick: done when all is cut, passed past the target; the
// smoke, the cells, the boost
void __fastcall LawnUpdate(uint8_t* mg, re::Edx) {
    int32_t cut = At<int32_t>(mg, 0x27C);
    if (cut == At<int32_t>(mg, 0x278)) {
        mg[6] = 1;
        mg[4] = 0;
    }
    if (cut >= At<int32_t>(mg, 0x274)) {
        mg[6] = 1;
    }
    Lawn_UpdateSmoke(mg, lawnPlayerPed);
    Lawn_UpdateCells(mg, lawnPlayerPed);
    Lawn_UpdateBoost(mg, lawnPlayerPed);
}

// LawnMowingSetLawnArea(x0, y0, x1, y1)
int __cdecl Native_LawnMowingSetLawnArea(lua_State* L) {
    float x0 = Script::GetFloat(L, 0);
    float y0 = Script::GetFloat(L, 1);
    float x1 = Script::GetFloat(L, 2);
    float y1 = Script::GetFloat(L, 3);
    Lawn_SetLawnArea(lawnCurrentMinigame, x0, y0, x1, y1);
    return 0;
}

// LawnMowingGetPercent(): the share cut, percent
int __cdecl Native_LawnMowingGetPercent(lua_State* L) {
    float cut = float(Lawn_GetCut(lawnCurrentMinigame));
    int32_t grass = Lawn_GetGrass(lawnCurrentMinigame);
    float share = float(double(cut) / double(grass));
    // (__ftol2: a 64-bit truncation, eax its low half: 0 for no grass's NaN / infinity)
    Script::PushInt(L, int32_t(uint32_t(int64_t(float(double(share) * 100.0)))));
    return 1;
}

} // namespace

#define LAWN44_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

LAWN44_REVERSED("PLawnMowing::BonusTaken_7093D0", 0x007093D0, &LawnBonusTaken);
LAWN44_REVERSED("PLawnMowing::~PLawnMowing_7093F0", 0x007093F0, &LawnDestruct);
LAWN44_REVERSED("Minigame::Start_709510", 0x00709510, &MinigameStart709510);
LAWN44_REVERSED("PLawnMowing::GetCut_709520", 0x00709520, &LawnGetCut);
LAWN44_REVERSED("PLawnMowing::GetGrass_709530", 0x00709530, &LawnGetGrass);
LAWN44_REVERSED("PLawnMowing::SetMinScore_709540", 0x00709540, &LawnSetMinScore);
LAWN44_REVERSED("PLawnMowing::PLawnMowing_709550", 0x00709550, &LawnConstruct);
LAWN44_REVERSED("PLawnMowing::DeletingDestructor_709AF0", 0x00709AF0, &LawnDeletingDestructor);
LAWN44_REVERSED("PLawnMowing::Draw_709B10", 0x00709B10, &LawnDraw);
LAWN44_REVERSED("PLawnMowing::UpdateBoost_709C80", 0x00709C80, &LawnUpdateBoost);
LAWN44_REVERSED("PLawnMowing::UpdateSmoke_709E50", 0x00709E50, &LawnUpdateSmoke);
LAWN44_REVERSED("PLawnMowing::UpdateCells_70A2D0", 0x0070A2D0, &LawnUpdateCells);
LAWN44_REVERSED("PLawnMowing::DrawGrass_70AAC0", 0x0070AAC0, &LawnDrawGrass);
LAWN44_REVERSED("PLawnMowing::SetLawnArea_70B2B0", 0x0070B2B0, &LawnSetArea);
LAWN44_REVERSED("PLawnMowing::Update_70B460", 0x0070B460, &LawnUpdate);
LAWN44_REVERSED("Native_LawnMowingSetLawnArea", 0x0070B4D0, &Native_LawnMowingSetLawnArea);
LAWN44_REVERSED("Native_LawnMowingGetPercent", 0x0070B560, &Native_LawnMowingGetPercent);
