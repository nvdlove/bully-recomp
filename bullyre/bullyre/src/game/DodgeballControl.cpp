// The dodgeball minigame's global entry points (0x6839F0..0x683C00): the game object at
// *0x1B89150 made and destroyed, the script natives' and the other systems' calls passed
// to it, the HUD and camera switched with it; and the three-plane intersection its camera
// uses (0x683C40).

#include <cstdint>

#include "game/VCall.h"
#include "reversed.h"

namespace {

using vc::At;

GAME_VAR(uint8_t*, dbcGame, 0x01B89150);
GAME_VAR(uint8_t*, dbcHudDodgeball, 0x00C674F8);
GAME_VAR(uint8_t*, dbcHudWeapons, 0x00C674F0);
GAME_VAR(int32_t, dbcCamera_C3CE0C, 0x00C3CE0C);   // TheCamera+0x1A4
GAME_VAR(int32_t, dbcCamera_C3CED8, 0x00C3CED8);
inline void* const dbcTheCamera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(void, __thiscall, Game_Start, 0x0067D7A0, uint8_t* game, const char* name, int flag);
GAME_FN(void, __thiscall, Game_SetPed, 0x0067AD60, uint8_t* game, void* setup);
GAME_FN(uint8_t, __thiscall, Game_IsActive, 0x0067B250, uint8_t* game);
GAME_FN(void, __thiscall, Game_Update, 0x0067D880, uint8_t* game);
GAME_FN(int, __thiscall, Game_GetWinner, 0x0067B1E0, uint8_t* game);
GAME_FN(uint8_t*, __thiscall, Game_Construct, 0x0067CD10, void* self);
GAME_FN(void, __thiscall, Game_Destruct, 0x0067C0A0, uint8_t* game);
GAME_FN(void, __thiscall, Game_Pause, 0x0067B5F0, uint8_t* game, int paused);
GAME_FN(void, __thiscall, Game_AimThrow, 0x0067B800, uint8_t* game, const float* from, float* target, uint8_t* entity);
GAME_FN(void, __thiscall, Game_End, 0x0067DEB0, uint8_t* game);
GAME_FN(uint8_t, __stdcall, Dodgeball_IsDodging, 0x00680CC0, uint8_t* entity);
GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int shot, int a, int instant, int b, int c);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
// ours, through the game's addresses
GAME_FN(void, __cdecl, Dodgeball_EndGame, 0x00683B90);

// a HUD component's SetVisible (vf5)
inline void SetVisible(uint8_t* component, int visible) {
    vc::Call<void>(component, 0x14, visible);
}

// 0x6839F0: a game started (the name of its setup, a flag)
void __cdecl Dodgeball_StartGameImpl(const char* name, int flag) {
    Game_Start(dbcGame, name, flag);
}

// 0x683A10: a player's setup (DodgeballSetPed)
void __cdecl Dodgeball_SetPedImpl(void* setup) {
    Game_SetPed(dbcGame, setup);
}

// 0x683A30: a game is on
uint8_t __cdecl Dodgeball_IsActiveImpl() {
    if (!dbcGame) {
        return 0;
    }
    return Game_IsActive(dbcGame);
}

// 0x683A50: the game's update, when there is one
void __cdecl Dodgeball_UpdateImpl() {
    if (dbcGame) {
        Game_Update(dbcGame);
    }
}

// 0x683A60: the winner (no game: the game reads through null)
int __cdecl Dodgeball_GetWinnerImpl() {
    return Game_GetWinner(dbcGame);
}

// 0x683A70: the game object made (0x1898 bytes)
void __cdecl Dodgeball_InitImpl() {
    void* p = Malloc(0x1898);
    if (p) {
        dbcGame = Game_Construct(p);
        return;
    }
    dbcGame = nullptr;
}

// 0x683AA0: the dodgeball HUD shown or hidden, the weapon HUD the other way. (The game
// pushes sete's dl with edx's other bytes left from the vtable read: a byte flag.)
void __cdecl Dodgeball_EnableHudImpl(int on) {
    SetVisible(dbcHudDodgeball, on);
    SetVisible(dbcHudWeapons, uint8_t(on) == 0);
}

// 0x683AD0: the dodgeball camera shot (0xB) or the normal one (1)
void __cdecl Dodgeball_EnableCameraImpl(int on) {
    dbcCamera_C3CE0C = 1;
    dbcCamera_C3CED8 = 0;
    if (re::Flag(on)) {
        Camera_SetShot(dbcTheCamera, 0xB, 0, 1, 0, 0);
        dbcCamera_C3CE0C = 0;
        dbcCamera_C3CED8 = 0;
        return;
    }
    Camera_SetShot(dbcTheCamera, 1, 0, 1, 0, 0);
}

// 0x683B20: paused or not
void __cdecl Dodgeball_PauseImpl(int paused) {
    Game_Pause(dbcGame, paused);
}

// 0x683B50: a throw's target adjusted by the game, when there is one
void __cdecl Dodgeball_AimThrowImpl(const float* from, float* target, uint8_t* entity) {
    if (dbcGame) {
        Game_AimThrow(dbcGame, from, target, entity);
    }
}

// 0x683B70: an entity dodges the ball (ducking low enough), in a game
uint8_t __cdecl Dodgeball_IsDodgingPedImpl(uint8_t* entity) {
    if (!dbcGame) {
        return 0;
    }
    return Dodgeball_IsDodging(entity);
}

// 0x683B90: the game ended: the HUDs back, the game's end, the normal camera
void __cdecl Dodgeball_EndGameImpl() {
    if (!dbcGame) {
        return;
    }
    SetVisible(dbcHudDodgeball, 0);
    SetVisible(dbcHudWeapons, 1);
    Game_End(dbcGame);
    dbcCamera_C3CE0C = 1;
    dbcCamera_C3CED8 = 0;
    Camera_SetShot(dbcTheCamera, 1, 0, 1, 0, 0);
}

// 0x683C00: the game ended and destroyed
void __cdecl Dodgeball_TermImpl() {
    if (!dbcGame) {
        return;
    }
    Dodgeball_EndGame();
    uint8_t* game = dbcGame;
    if (game) {
        Game_Destruct(game);
        Free(game);
    }
    dbcGame = nullptr;
}

// A plane: n . p = d (4 floats)
struct Plane4 {
    float n[3];
    float d;
};

// 0x683C40 (thiscall, ret 0xC): the point on this plane and two others (Cramer's rule, in
// the game's order); false when they don't meet in one point (a zero determinant)
uint8_t __fastcall Plane_Intersect3(const Plane4* a, re::Edx, const Plane4* b, const Plane4* c, float* out) {
    float a0 = a->n[0], a1 = a->n[1], a2 = a->n[2];
    float b0 = b->n[0], b1 = b->n[1], b2 = b->n[2];
    float c0 = c->n[0], c1 = c->n[1], c2 = c->n[2];
    float aw = a->d, bw = b->d, cw = c->d;
    float m00 = b1 * c2 - b2 * c1;
    float m10 = -(b0 * c2 - b2 * c0);
    float m20 = b0 * c1 - c0 * b1;
    float det = (m10 * a1 + m00 * a0) + m20 * a2;
    if (det == 0.0f) {
        return 0;
    }
    float m01 = -(c2 * a1 - c1 * a2);
    float m11 = a0 * c2 - c0 * a2;
    float m21 = -(c1 * a0 - c0 * a1);
    float m02 = b2 * a1 - b1 * a2;
    float m12 = -(b2 * a0 - a2 * b0);
    float m22 = b1 * a0 - b0 * a1;
    float inv = 1.0f / det;
    m00 = inv * m00;
    m01 = m01 * inv;
    m02 = m02 * inv;
    m10 = inv * m10;
    m11 = m11 * inv;
    m12 = m12 * inv;
    m20 = inv * m20;
    m21 = m21 * inv;
    m22 = inv * m22;
    out[0] = (m01 * bw + m00 * aw) + m02 * cw;
    out[1] = (m11 * bw + m10 * aw) + m12 * cw;
    out[2] = (m20 * aw + m21 * bw) + cw * m22;
    return 1;
}

#define DBCTL_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

DBCTL_REVERSED("Dodgeball::StartGame", 0x006839F0, &Dodgeball_StartGameImpl);
DBCTL_REVERSED("Dodgeball::SetPed", 0x00683A10, &Dodgeball_SetPedImpl);
DBCTL_REVERSED("Dodgeball::IsActive", 0x00683A30, &Dodgeball_IsActiveImpl);
DBCTL_REVERSED("Dodgeball::Update", 0x00683A50, &Dodgeball_UpdateImpl);
DBCTL_REVERSED("Dodgeball::GetWinner", 0x00683A60, &Dodgeball_GetWinnerImpl);
DBCTL_REVERSED("Dodgeball::Init", 0x00683A70, &Dodgeball_InitImpl);
DBCTL_REVERSED("Dodgeball::EnableHud", 0x00683AA0, &Dodgeball_EnableHudImpl);
DBCTL_REVERSED("Dodgeball::EnableCamera", 0x00683AD0, &Dodgeball_EnableCameraImpl);
DBCTL_REVERSED("Dodgeball::Pause", 0x00683B20, &Dodgeball_PauseImpl);
DBCTL_REVERSED("Dodgeball::AimThrow", 0x00683B50, &Dodgeball_AimThrowImpl);
DBCTL_REVERSED("Dodgeball::IsDodgingPed", 0x00683B70, &Dodgeball_IsDodgingPedImpl);
DBCTL_REVERSED("Dodgeball::EndGame", 0x00683B90, &Dodgeball_EndGameImpl);
DBCTL_REVERSED("Dodgeball::Term", 0x00683C00, &Dodgeball_TermImpl);
DBCTL_REVERSED("CPlane::Intersect3_683C40", 0x00683C40, &Plane_Intersect3);

} // namespace
