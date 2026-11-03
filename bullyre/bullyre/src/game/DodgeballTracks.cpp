// The dodgeball minigame's action tree tracks and conditions (0x672A30..0x678C80): their
// virtuals (factories and destructors in DodgeballFactories.cpp). They read and set the
// game's state (*0x1B89150, DodgeballGame.cpp): its flags (+0x17DC), its players (9
// records of 0x28C from +0x28: +0 the ped, +4 flags (1 in play, 8 out, 0x80 holding a
// ball, 0x1000 / 0x1080 may pick one up), +8 a ducking float, +0x22C / +0x230, +0x238 last
// catch attempt, +0x23C / +0x248 path points), the two teams' current players, the ball.
//
// A track: +0 vtable, +0x18.. its fields; a condition: +0 vtable, +4.. its fields; the
// owner (the ped's action context) +8 the ped.

#include <bit>
#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/DodgeballCourt.h"
#include "game/Pool.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

using actc::At;
using actc::Pos;
using actc::V3;

GAME_VAR(uint8_t*, dodgeballGame, 0x01B89150);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);          // CTimer::m_snTimeInMilliseconds
GAME_VAR(uint8_t*, dodgeballCamera_C3CEC0, 0x00C3CEC0);   // +0x34 its view flags
GAME_VAR(uint8_t*, hudDodgeball_C674F8, 0x00C674F8);
GAME_ARRAY(float, zeroVector, 0x00BCC564);

// the game's accessors (DodgeballGame.cpp)
GAME_FN(float, __thiscall, Dodgeball_CurrentPlayerFloat, 0x0067AF00, uint8_t* game, int team);
GAME_FN(int, __thiscall, Dodgeball_PlayerIndex, 0x0067AF20, uint8_t* game, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Dodgeball_PlayerPed, 0x0067B040, uint8_t* game, int i);
GAME_FN(int, __thiscall, Dodgeball_TeamCurrent, 0x0067B060, uint8_t* game, int i);
GAME_FN(int, __thiscall, Dodgeball_OtherTeamCurrent, 0x0067B080, uint8_t* game, int i);
GAME_FN(uint32_t*, __thiscall, Dodgeball_GetPlayerFlags, 0x0067B0A0, uint8_t* game, uint32_t* out, int i);
GAME_FN(void, __thiscall, Dodgeball_SetPlayerFlags, 0x0067B0C0, uint8_t* game, int i, uint32_t flags);
GAME_FN(uint32_t*, __thiscall, Dodgeball_GetFlags, 0x0067B0E0, uint8_t* game, uint32_t* out);
GAME_FN(void, __thiscall, Dodgeball_SetFlags, 0x0067B0F0, uint8_t* game, uint32_t flags);
GAME_FN(uint8_t, __thiscall, Dodgeball_SameTeam, 0x0067B100, uint8_t* game, uint8_t* a, uint8_t* b);
GAME_FN(uint8_t*, __thiscall, Dodgeball_Player, 0x0067B1F0, uint8_t* game, int i);
GAME_FN(uint8_t*, __thiscall, Dodgeball_PlayerOf, 0x0067B210, uint8_t* game, uint8_t* ped);
GAME_FN(void, __thiscall, Dodgeball_SetMessage, 0x0067B2A0, uint8_t* game, const char* text);
GAME_FN(uint32_t, __thiscall, Dodgeball_TeamCode, 0x0067B500, uint8_t* game, uint8_t* ped);   // (al; eax's rest garbage)
GAME_FN(void, __thiscall, Dodgeball_Reset_67D1C0, 0x0067D1C0, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_Reset_67D450, 0x0067D450, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_FindBall, 0x0067D560, uint8_t* game);
GAME_FN(uint8_t*, __thiscall, Dodgeball_GetBall, 0x0067D780, uint8_t* game);
GAME_FN(uint8_t, __thiscall, Dodgeball_BallInArea, 0x0067DC80, uint8_t* game, float x0, float x1, float y0, float y1);
GAME_FN(float*, __thiscall, Court_Point, 0x0067AB50, uint8_t* court, float* out, float u, float v, float w);
GAME_FN(void, __thiscall, HudDodgeball_SetText, 0x0067DF30, uint8_t* hud, const wchar_t* text);

GAME_FN(uint8_t, __cdecl, CompareOp, 0x005FB8B0, float value, float reference, int op);
GAME_FN(float*, __cdecl, VectorAdd, 0x004129A0, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, VectorSub, 0x00414B70, float* out, const float* a, const float* b);
GAME_FN(float*, __cdecl, CrossProduct, 0x00414730, float* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, float* v);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, void* targeting);
GAME_FN(void, __thiscall, Targeting_SetTarget, 0x004D79E0, void* targeting, uint8_t* target);
GAME_FN(void, __cdecl, Marker_Place_x64, 0x004DB580, int id, float, float, float, float, float, float, float a, float b);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Marker_Place(int a0, V3 a1, V3 a2, float a3, float a4) {
    return Marker_Place_x64(a0, a1.x, a1.y, a1.z, a2.x, a2.y, a2.z, a3, a4);
}
GAME_FN(void, __cdecl, Marker_Clear, 0x004DB600, int id);
GAME_FN(void, __cdecl, Marker_SetColour, 0x004DB620, int id, int r, int g, int b, int a, int);
GAME_FN(void, __cdecl, Projectile_Activate, 0x004D3700, int index);
GAME_FN(uint8_t*, __cdecl, Projectile_Info, 0x004D34B0, int index);   // +4 its thrower
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(void, __thiscall, Weapon_4D9410, 0x004D9410, void* weapon);
GAME_FN(void, __thiscall, Weapon_SetWeapon, 0x004DA840, void* weapon, int id, int value);
GAME_FN(int, __cdecl, Prop_FindBone, 0x00655620, void* entity, int id);
GAME_FN(void, __thiscall, Weapon_4D9B60, 0x004D9B60, void* weapon, void* matrix, int a, int b);
GAME_FN(void, __thiscall, Weapon_Attach, 0x004D9460, void* weapon);
GAME_VAR(int32_t, boneId_1B65308, 0x01B65308);
// a ped's move towards a target (ped+0x654): (target, flags, a, b, 0, team code)
GAME_FN(uint8_t, __thiscall, PedMove_To_x64, 0x004A5000, void* self, float, float, float, int flags, float a, float b, uintptr_t, uint32_t team);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t PedMove_To(void* a0, V3 a1, int a2, float a3, float a4, int a5, uint32_t a6) {
    return PedMove_To_x64(a0, a1.x, a1.y, a1.z, a2, a3, a4, a5, a6);
}
GAME_FN(float, __thiscall, Weapon_InfoFloatC, 0x00465E10, uint8_t* object);   // its weapon info's +0xC
GAME_FN(uint8_t, __thiscall, NavRef_Valid, 0x005E7AF0, void* ref);
GAME_FN(uint8_t, __cdecl, Nav_FindNear, 0x005EA870, const float* position, float radius, void* ref, uintptr_t, float height);
GAME_FN(void, __thiscall, Nav_Position, 0x005E3110, void* nav, float* out);
GAME_FN(void, __thiscall, Intercept_Setup_x64, 0x0065BA70, void* self, uint8_t* ped, int flags, float, float, float, float a, float b, float c, float d);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Intercept_Setup(void* a0, uint8_t* a1, int a2, V3 a3, float a4, float a5, float a6, float a7) {
    return Intercept_Setup_x64(a0, a1, a2, a3.x, a3.y, a3.z, a4, a5, a6, a7);
}
GAME_FN(uint8_t, __thiscall, Intercept_Test_x64, 0x0065CAC0, void* self, float, float, float, float, float, float, float speed, float e, uint32_t flags);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t Intercept_Test(void* a0, V3 a1, V3 a2, float a3, float a4, uint32_t a5) {
    return Intercept_Test_x64(a0, a1.x, a1.y, a1.z, a2.x, a2.y, a2.z, a3, a4, a5);
}
inline int __cdecl CrtRand() { return crt::Rand(); }
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(void, __cdecl, AsciiToWide, 0x00690090, const char* text, wchar_t* out);

// ours, through the game's addresses
GAME_FN(int, __cdecl, PlayersIn_Count, 0x00673040, int team);
GAME_FN(float*, __cdecl, TargetPositionFn, 0x00675EE0, float* out, uint8_t* ped, int mode);

inline uint8_t* OwnerPed(uint8_t* owner) {
    return At<uint8_t*>(owner, 8);
}
inline bool IsPed(uint8_t* e) {
    return (e[0x108] & 7) == 3;
}
inline void Copy3(float* dst, const float* src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}
// a nav mesh reference (5 words: +0 the mesh, +4 its polygon, then 3 floats)
struct NavRef {
    re::Ptr32<uint8_t> mesh;
    int32_t polygon;
    float extra[3];
};
static_assert(sizeof(NavRef) == 0x14);
// the team code a polygon belongs to (its +6 of 8 bytes in the mesh's +0x38 +0x10)
inline uint8_t PolygonTeam(const NavRef& ref) {
    return At<uint8_t*>(At<uint8_t*>(ref.mesh, 0x38), 0x10)[ref.polygon * 8 + 6];
}

// ---- small virtuals ------------------------------------------------------------------------

// 0x672A30 (thiscall): CDodgeballSetPedFlagTrack's vf3: (without a game: done) its flag
// set again each update when so defined
uint8_t __fastcall SetPedFlag_Update(uint8_t* self, re::Edx, float) {
    if (!dodgeballGame) {
        return 0;
    }
    if (self[0x1D]) {
        vc::Call<uint8_t>(self, 8, At<uint8_t*>(self, 0x20));
    }
    return 1;
}

// 0x672A60 (thiscall): CDodgeballTargetSetPedFlagTrack's vf3: the same, its start's result
uint8_t __fastcall TargetSetPedFlag_Update(uint8_t* self, re::Edx, float) {
    if (!dodgeballGame) {
        return 0;
    }
    if (self[0x1D]) {
        return vc::Call<uint8_t>(self, 8, At<uint8_t*>(self, 0x20));
    }
    return 1;
}

// 0x672A90 (thiscall): CDodgeballGameFlagCond's vf1: a game flag (bit +4) is as expected
// (+8)
uint8_t __fastcall GameFlagCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    uint32_t flags;
    Dodgeball_GetFlags(dodgeballGame, &flags);
    uint8_t set = (flags & (1u << (At<uint32_t>(self, 4) & 31))) != 0;
    return set == self[8];
}

// 0x672AC0 (thiscall): CDodgeballSetGameFlagTrack's vf2: a game flag (bit +0x18) set or
// cleared (+0x1C)
uint8_t __fastcall SetGameFlag_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint32_t flags;
    Dodgeball_GetFlags(dodgeballGame, &flags);
    uint32_t bit = 1u << (At<uint32_t>(self, 0x18) & 31);
    flags = self[0x1C] ? (bit | flags) : (~bit & flags);
    Dodgeball_SetFlags(dodgeballGame, flags);
    return 1;
}

// 0x672B30 (thiscall): CDodgeballPedMarkerTrack's vf2: started (updated at once)
uint8_t __fastcall PedMarker_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x34) = owner;
    vc::Call<uint8_t>(self, 0xC, 0.0f);
    return 1;
}

// 0x672B50 (thiscall): CDodgeballPedMarkerTrack's vf4: its marker (+0x18) cleared
uint8_t __fastcall PedMarker_End(uint8_t* self, re::Edx) {
    Marker_Clear(At<int32_t>(self, 0x18));
    return 1;
}

// 0x672B60 / 0x672B70 / 0x673030 / 0x6730D0 (thiscall): vf2, the owner kept (at +0x20:
// done at once; +0x2C, +0x30, +0x44: running)
template <int Offset, uint8_t Result>
uint8_t __fastcall KeepOwner_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, Offset) = owner;
    return Result;
}

// 0x672B90 (thiscall): the ped of the other team's current player (from this one's
// player +0x28)
uint8_t* __fastcall AITrack_OpponentPed(uint8_t* self, re::Edx) {
    int other = (1 - (At<int32_t>(self, 0x28) >= 4)) * 4;
    return Dodgeball_PlayerPed(dodgeballGame, Dodgeball_TeamCurrent(dodgeballGame, other));
}

// 0x672BC0 (thiscall): DodgeballGameTimerTrack's vf4: its text cleared (when shown)
uint8_t __fastcall GameTimer_End(uint8_t* self, re::Edx) {
    if (self[0x22]) {
        HudDodgeball_SetText(hudDodgeball_C674F8, nullptr);
    }
    return 1;
}

// 0x672BE0 (thiscall): DodgeballSetTurnTrack's vf2: whose turn it is (game flags 2 / 4,
// each current player's 0x80) by its mode: 0 swapped, 1 the first team's, 2 the second's,
// 3 nobody's, 4 both
uint8_t __fastcall SetTurn_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint32_t flags;
    if (At<int32_t>(self, 0x18) == 0) {
        Dodgeball_GetFlags(dodgeballGame, &flags);
        uint32_t second = ~(flags >> 1) & 1;
        flags = ((second & 1) * 2) | (flags & 0xFFFFFFF9u);
        Dodgeball_SetFlags(dodgeballGame, flags);
        int a = Dodgeball_TeamCurrent(dodgeballGame, 0);
        int b = Dodgeball_TeamCurrent(dodgeballGame, 4);
        uint32_t* fa = reinterpret_cast<uint32_t*>(Dodgeball_Player(dodgeballGame, a) + 4);
        *fa ^= ((uint32_t(second == 0) << 7) ^ *fa) & 0x80;
        uint32_t* fb = reinterpret_cast<uint32_t*>(Dodgeball_Player(dodgeballGame, b) + 4);
        *fb ^= ((uint32_t(second == 1) << 7) ^ *fb) & 0x80;
    }
    struct Mode {
        int mode;
        uint32_t clear, set;
        bool aOn, bOn;
    };
    static constexpr Mode kModes[] = {
        {1, 6, 0, true, false}, {2, 4, 2, false, true}, {4, 0, 4, true, true}, {3, 0, 4, false, false}};
    for (const Mode& m : kModes) {
        if (At<int32_t>(self, 0x18) != m.mode) {
            continue;
        }
        Dodgeball_GetFlags(dodgeballGame, &flags);
        flags = (flags & ~m.clear) | m.set;
        Dodgeball_SetFlags(dodgeballGame, flags);
        int a = Dodgeball_TeamCurrent(dodgeballGame, 0);
        int b = Dodgeball_TeamCurrent(dodgeballGame, 4);
        uint32_t& fa = At<uint32_t>(Dodgeball_Player(dodgeballGame, a), 4);
        fa = m.aOn ? (fa | 0x80) : (fa & ~0x80u);
        uint32_t& fb = At<uint32_t>(Dodgeball_Player(dodgeballGame, b), 4);
        fb = m.bOn ? (fb | 0x80) : (fb & ~0x80u);
    }
    return 1;
}

// 0x672E80 (thiscall): DodgeballResetTrack's vf2: the game reset (+0x18: fully)
uint8_t __fastcall Reset_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    if (self[0x18]) {
        Dodgeball_Reset_67D1C0(dodgeballGame);
    } else {
        Dodgeball_Reset_67D450(dodgeballGame);
    }
    return 1;
}

// 0x672EB0 (thiscall): DodgeballBallLocationCond's vf1: the ball in a part of the court
// (+4 / +0xC across, +8 / +0x10 along) as expected (+0x14)
uint8_t __fastcall BallLocationCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    uint8_t in = Dodgeball_BallInArea(dodgeballGame, At<float>(self, 4), At<float>(self, 0xC), At<float>(self, 8),
                                      At<float>(self, 0x10));
    return in == self[0x14];
}

// 0x672F10 (thiscall): DodgeballGameOverCond's vf1: game flag 0x20
uint8_t __fastcall GameOverCond_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    uint32_t flags;
    Dodgeball_GetFlags(dodgeballGame, &flags);
    return uint8_t((flags >> 5) & 1);
}

// 0x672F30 (thiscall): DodgeballBallMessageCond's vf1: the game's message (+0x181C) is its
// name (+4)
uint8_t __fastcall BallMessageCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    const uint8_t* a = dodgeballGame + 0x181C;
    const uint8_t* b = At<const uint8_t*>(self, 4);
    int result = 0;
    for (;; a++, b++) {
        if (*a != *b) {
            result = *a < *b ? -1 : 1;
            break;
        }
        if (!*a) {
            break;
        }
    }
    return result == 0;
}

// 0x672F80 (thiscall): DodgeballCameraViewTrack's vf2: the camera's view flags (+0x34) set
// from its 8 bytes (+0x18..), the old ones kept
uint8_t __fastcall CameraView_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* camera = dodgeballCamera_C3CEC0;
    At<uint32_t>(self, 0x28) = At<uint32_t>(camera, 0x34);
    uint32_t flags = 0;
    for (int i = 0; i < 8; i++) {
        if (self[0x18 + i]) {
            flags |= 1u << i;
        }
    }
    At<uint32_t>(camera, 0x34) = flags;
    return 1;
}

// 0x673010 (thiscall): DodgeballCameraViewTrack's vf4: the old flags back (when +0x20)
uint8_t __fastcall CameraView_End(uint8_t* self, re::Edx) {
    if (self[0x20]) {
        At<uint32_t>(dodgeballCamera_C3CEC0, 0x34) = At<uint32_t>(self, 0x28);
    }
    return 1;
}

// 0x673040 (cdecl): how many of a team's (0 / 1) players are in play (flag 1)
int __cdecl PlayersIn(int team) {
    int count = 0;
    for (int i = team * 4; i < team * 4 + 4; i++) {
        if (Dodgeball_Player(dodgeballGame, i)[4] & 1) {
            count++;
        }
    }
    return count;
}

// 0x673090 (thiscall): DodgeballPlayersInCond's vf1: a team's (+8) players in play compared
// (+0xC) with a count (+4)
uint8_t __fastcall PlayersInCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int count = PlayersIn_Count(At<int32_t>(self, 8));
    return CompareOp(float(count), float(At<int32_t>(self, 4)), At<int32_t>(self, 0xC));
}

// 0x6730E0 (cdecl): a ped given the dodgeball as its weapon (the ball projectile
// activated, the weapon attached to its hand); the ball found again
void __cdecl GiveBall(uint8_t* ped) {
    Projectile_Activate(At<int32_t>(Dodgeball_GetBall(dodgeballGame), 0x238));
    int weapon = GetWeaponIndex(reinterpret_cast<const char*>(0x00904C68));   // "dodgeball"
    uint8_t* slot = At<uint8_t*>(ped, 0x1D0);
    Weapon_4D9410(slot);
    Weapon_SetWeapon(At<uint8_t*>(ped, 0x1D0), weapon, 0);
    int a = Prop_FindBone(ped, boneId_1B65308);
    int b = Prop_FindBone(At<uint8_t*>(ped, 0x1D0), boneId_1B65308);
    Weapon_4D9B60(At<uint8_t*>(ped, 0x1D0), At<void*>(ped, 0x114), a, b);
    Weapon_Attach(At<uint8_t*>(ped, 0x1D0));
    Dodgeball_FindBall(dodgeballGame);
}

// ---- player flags ----------------------------------------------------------------------------

// 0x6745E0 (thiscall): CDodgeballPedFlagCond's vf1: the owner's player flag (bit +4) as
// expected (+8)
uint8_t __fastcall PedFlagCond_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(owner));
    uint32_t flags;
    Dodgeball_GetPlayerFlags(dodgeballGame, &flags, index);
    uint8_t set = (flags & (1u << (At<uint32_t>(self, 4) & 31))) != 0;
    return set == self[8];
}

// a player's flag (bit +0x18) set (or cleared)
inline void SetPlayerFlag(uint8_t* self, uint8_t* ped, bool set) {
    int index = Dodgeball_PlayerIndex(dodgeballGame, ped);
    uint32_t flags;
    Dodgeball_GetPlayerFlags(dodgeballGame, &flags, index);
    uint32_t bit = 1u << (At<uint32_t>(self, 0x18) & 31);
    flags = set ? (bit | flags) : (~bit & flags);
    Dodgeball_SetPlayerFlags(dodgeballGame, index, flags);
}

// 0x674630 (thiscall): CDodgeballSetPedFlagTrack's vf2: the owner's flag set (+0x1C) or
// cleared
uint8_t __fastcall SetPedFlag_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (dodgeballGame) {
        SetPlayerFlag(self, OwnerPed(owner), self[0x1C] != 0);
    }
    return 1;
}

// 0x6746A0 (thiscall): its vf4: undone at the end (when +0x1E)
uint8_t __fastcall SetPedFlag_End(uint8_t* self, re::Edx) {
    if (dodgeballGame && self[0x1E]) {
        SetPlayerFlag(self, OwnerPed(At<uint8_t*>(self, 0x20)), self[0x1C] == 0);
    }
    return 1;
}

// the owner's target when it's a ped
inline uint8_t* PedTarget(uint8_t* owner) {
    uint8_t* target = Targeting_GetTarget(At<uint8_t*>(OwnerPed(owner), 0x1C8));
    return target && IsPed(target) ? target : nullptr;
}

// 0x674710 (thiscall): CDodgeballTargetSetPedFlagTrack's vf2: the same for the owner's
// target (none: done)
uint8_t __fastcall TargetSetPedFlag_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    if (!dodgeballGame) {
        return 0;
    }
    uint8_t* target = PedTarget(owner);
    if (!target) {
        return 0;
    }
    SetPlayerFlag(self, target, self[0x1C] != 0);
    return 1;
}

// 0x6747A0 (thiscall): its vf4
uint8_t __fastcall TargetSetPedFlag_End(uint8_t* self, re::Edx) {
    if (dodgeballGame && self[0x1E]) {
        uint8_t* target = PedTarget(At<uint8_t*>(self, 0x20));
        if (target) {
            SetPlayerFlag(self, target, self[0x1C] == 0);
        }
    }
    return 1;
}

// 0x674830 (thiscall): DodgeballPedSetDuckTrack's vf2: the owner's player's +8 set
uint8_t __fastcall PedSetDuck_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<float>(Dodgeball_PlayerOf(dodgeballGame, OwnerPed(owner)), 8) = At<float>(self, 0x18);
    return 1;
}

GAME_FN(void, __thiscall, Dodgeball_ChangePlayer, 0x0067AF50, uint8_t* game, int from, int to, int flag);
GAME_FN(void, __thiscall, Dodgeball_SwapPlayers, 0x0067B560, uint8_t* game, int from, int to, int flag);

// 0x674860 (thiscall): the change of player (team +0x20: from +0x24 to +0x28): made current
// (the owner targeting the new one) or swapped; the target cleared unless +0x18
void __fastcall ChangePlayer_Apply(uint8_t* self, re::Edx, int swap) {
    int base = At<int32_t>(self, 0x20) * 4;
    int to = At<int32_t>(self, 0x28) + base;
    int from = At<int32_t>(self, 0x24) + base;
    uint8_t* ped = Dodgeball_PlayerPed(dodgeballGame, to);
    if (!re::Flag(swap)) {
        Dodgeball_ChangePlayer(dodgeballGame, from, to, self[0x19]);
        Targeting_SetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x1C8), ped);
    } else {
        Dodgeball_SwapPlayers(dodgeballGame, from, to, self[0x19]);
    }
    if (!self[0x18]) {
        Targeting_SetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x1C8), nullptr);
    }
}

// 0x6748E0 (thiscall): CDodgeballPedMarkerTrack's vf3: while the owner is the first team's
// current player, its marker (+0x18) over it (+0x30 up; +0x28 / +0x2C, colour +0x1C..)
uint8_t __fastcall PedMarker_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x34));
    int index = Dodgeball_PlayerIndex(dodgeballGame, ped);
    if (index != Dodgeball_TeamCurrent(dodgeballGame, 0)) {
        return 0;
    }
    float* p = Pos(OwnerPed(At<uint8_t*>(self, 0x34)));
    V3 position = {p[0], p[1], At<float>(self, 0x30) + p[2]};
    V3 up = {0.0f, 0.0f, 1.0f};
    Marker_Place(At<int32_t>(self, 0x18), position, up, At<float>(self, 0x28), At<float>(self, 0x2C));
    Marker_SetColour(At<int32_t>(self, 0x18), self[0x1C], self[0x20], self[0x24], 0xFF, 0xFF);
    return 1;
}

// 0x6749D0 (thiscall): CDodgeballTargetTrack's vf3: the owner targets the ball (mode 0),
// its team's current player (1) or the other team's (2); done unless +0x1E
uint8_t __fastcall Target_Update(uint8_t* self, re::Edx, float) {
    uint8_t* target = Dodgeball_GetBall(dodgeballGame);
    int mode = At<int32_t>(self, 0x18);
    if (mode == 1) {
        int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x20)));
        target = Dodgeball_PlayerPed(dodgeballGame, Dodgeball_TeamCurrent(dodgeballGame, index));
    } else if (mode == 2) {
        int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x20)));
        target = Dodgeball_PlayerPed(dodgeballGame, Dodgeball_OtherTeamCurrent(dodgeballGame, index));
    }
    if (Targeting_GetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1C8)) != target) {
        Targeting_SetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1C8), target);
    }
    return self[0x1E] == 0;
}

// 0x674A70 (thiscall): its vf4: the target cleared unless +0x1D
uint8_t __fastcall Target_End(uint8_t* self, re::Edx) {
    if (!self[0x1D]) {
        Targeting_SetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x20)), 0x1C8), nullptr);
    }
    return 1;
}

// 0x674A90 (thiscall): CDodgeballPedInAreaCond's vf1: the owner inside the court (mode 2),
// its own half or the other's (by mode and team; the halves stop 0.5 short of the middle)
// as expected (+8). The court (game +0x17D4): x from +0 to +0xC, y from +4 to +0x10, at +8.
// (Inside: on the inner side of its four edges' vertical planes.)
uint8_t __fastcall PedInAreaCond_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    float* court = At<float*>(dodgeballGame, 0x17D4);
    float* p = Pos(ped);
    float position[3] = {p[0], p[1], p[2]};
    int index = Dodgeball_PlayerIndex(dodgeballGame, ped);
    int team = index >= 4;
    float c[4][3] = {{court[0], court[1], court[2]},
                     {court[3], court[1], court[2]},
                     {court[3], court[4], court[2]},
                     {court[0], court[4], court[2]}};
    int mode = At<int32_t>(self, 4);
    if (mode != 2) {
        float tmp[3];
        if (((mode == 0) + team) % 2 == 0) {
            float* m = VectorAdd(tmp, c[1], c[2]);
            float half[3] = {m[0] * 0.5f, m[1] * 0.5f, 0.5f * m[2]};
            Copy3(c[2], half);
            m = VectorAdd(tmp, c[3], c[0]);
            float half2[3] = {m[0] * 0.5f, m[1] * 0.5f, m[2] * 0.5f};
            Copy3(c[3], half2);
            c[2][1] = c[2][1] - 0.5f;
            c[3][1] = half2[1] - 0.5f;
        } else {
            float* m = VectorAdd(tmp, c[3], c[0]);
            float half[3] = {m[0] * 0.5f, m[1] * 0.5f, 0.5f * m[2]};
            Copy3(c[0], half);
            m = VectorAdd(tmp, c[1], c[2]);
            float half2[3] = {m[0] * 0.5f, m[1] * 0.5f, m[2] * 0.5f};
            Copy3(c[1], half2);
            c[0][1] = c[0][1] + 0.5f;
            c[1][1] = 0.5f + half2[1];
        }
    }
    float planes[4][4] = {};
    for (int i = 0; i < 4; i++) {
        float a[3] = {c[i][0], c[i][1], c[i][2]};
        int j = (i + 1) % 4;
        float b[3] = {c[j][0], c[j][1], c[j][2]};
        float edge[3];
        VectorSub(edge, b, a);
        float up[3] = {0.0f, 0.0f, 1.0f};
        float n[3];
        CrossProduct(n, edge, up);
        Vector_Normalise(n);
        float d = (n[1] * a[1] + n[0] * a[0]) + n[2] * a[2];
        planes[i][0] = n[0];
        planes[i][1] = n[1];
        planes[i][2] = n[2];
        planes[i][3] = float(double(d) - 0.0);
    }
    uint8_t inside = 1;
    for (int i = 0; i < 4; i++) {
        float dot = (planes[i][1] * position[1] + planes[i][0] * position[0]) + planes[i][2] * position[2];
        float distance = dot - planes[i][3];
        if (-distance < 0.0f) {
            inside = 0;
        }
    }
    return inside == self[8];
}

// 0x674FE0 (thiscall): DodgeballBaseAITrack's vf2: the owner's player index (+0x28) and
// its player's +0x22C / +0x230 kept (+0x20 / +0x24)
uint8_t __fastcall BaseAI_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    At<int32_t>(self, 0x28) = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(owner));
    At<uint32_t>(self, 0x20) = At<uint32_t>(Dodgeball_PlayerOf(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x1C))), 0x22C);
    At<uint32_t>(self, 0x24) = At<uint32_t>(Dodgeball_PlayerOf(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x1C))), 0x230);
    return 1;
}

// 0x675040 (thiscall): the owner moved towards a point, faster the further it is (a third
// of the distance times 0.7, within 0..0.7); arguments 4..6 unused
uint8_t __fastcall CatchBall_MoveTo(uint8_t* self, re::Edx, V3 target, int, int, int, float b) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    float d[3];
    VectorSub(d, &target.x, Pos(ped));
    float sum = (d[1] * d[1] + d[0] * d[0]) + d[2] * d[2];
    float distance = crt::Sqrtf(std::fabs(sum));   // (the CRT x87 sqrt)
    float third = float(double(distance) * 0.3333333333333333);
    float k = float(double(third) * 0.699999988079071);
    if (k < 0.0f) {
        k = 0.0f;
    } else if (0.7f < k) {
        k = 0.7f;
    }
    uint32_t team = Dodgeball_TeamCode(dodgeballGame, ped);
    return PedMove_To(ped + 0x654, target, 0x18, k, b, 0, team);
}

// 0x675140 (thiscall): DodgeballGameTimerTrack's vf2: started now (not paused), its text
// cleared when shown
uint8_t __fastcall GameTimer_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<int32_t>(self, 0x2C) = -1;
    At<uint8_t*>(self, 0x24) = owner;
    At<uint32_t>(self, 0x28) = timeInMs;
    if (self[0x22]) {
        HudDodgeball_SetText(hudDodgeball_C674F8, nullptr);
    }
    return 1;
}

// an unsigned as the x87 loads it (fild, then 2^32 added and rounded when negative)
inline double UnsignedX87(uint32_t v) {
    return int32_t(v) >= 0 ? double(v) : double(float(double(v)));
}

// 0x675170 (thiscall): its vf3: paused with game flag 0x10 (the time so far kept, +0x2C);
// else the seconds since it started (or left of +0x1C, counting down) formatted with its
// format (+0x18: as a float +1, or with +0x21 as the whole second) into the HUD text while
// its whole seconds are within +0x1C; running while above 0
uint8_t __fastcall GameTimer_Update(uint8_t* self, re::Edx, float) {
    uint32_t flags;
    Dodgeball_GetFlags(dodgeballGame, &flags);
    uint32_t now = timeInMs;
    if (flags & 0x10) {
        if (float(At<int32_t>(self, 0x2C)) < 0.0f) {
            At<uint32_t>(self, 0x2C) = now - At<uint32_t>(self, 0x28);
        }
        return 1;
    }
    int32_t paused = At<int32_t>(self, 0x2C);
    if (!(float(paused) < 0.0f || float(paused) == 0.0f)) {
        At<uint32_t>(self, 0x28) = now - uint32_t(paused);
        At<int32_t>(self, 0x2C) = -1;
    }
    float t = float(UnsignedX87(now) - UnsignedX87(At<uint32_t>(self, 0x28))) / 1000.0f;
    if (self[0x20]) {
        t = At<float>(self, 0x1C) - t;
    }
    int whole = int(t);
    int next = whole + 1;
    if (At<float>(self, 0x1C) < float(next)) {
        return 1;
    }
    char text[0x64];
    const char* format = At<const char*>(self, 0x18);
    if (format && format[0]) {
        if (!self[0x21]) {
            Sprintf(text, format, double(t + 1.0f));
        } else {
            Sprintf(text, format, next);
        }
    } else {
        Sprintf(text, reinterpret_cast<const char*>(0x008FEFBB));   // ""
    }
    wchar_t* wide = reinterpret_cast<wchar_t*>(self + 0x30);
    AsciiToWide(text, wide);
    HudDodgeball_SetText(hudDodgeball_C674F8, wide);
    if (0.0f < t) {
        return 1;
    }
    return 0;
}

// 0x6752F0 (thiscall): DodgeballBallStateCond's vf1: the ball in a state (+4) returns the
// expected byte (+8), else whether that's 0. Loose: 0 still (horizontal speed^2 below
// 0.0001 and vertical^2 below 0.0009), 1 moving (speed^2 above 0.0001), 4 its +0x22C, 5
// its flag 0x800 (+0x1BD bit 3), 6 game flag 0x40. Held (by a ped): 2 by the first team, 3
// the second, 7 the owner's team, 8 the other.
uint8_t __fastcall BallStateCond_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ball = Dodgeball_GetBall(dodgeballGame);
    if (!ball) {
        return 0;
    }
    int state = At<int32_t>(self, 4);
    bool holds = false;
    if (!IsPed(ball)) {
        if (state == 0) {
            float x = At<float>(ball, 0x124);
            float y = At<float>(ball, 0x128);
            float z = At<float>(ball, 0x12C);
            float s = (y * y + x * x) + 0.0f * 0.0f;
            holds = double(s) < 9.999999747378752e-05 && double(z * z) < 0.0008999999845400453;
        } else if (state == 1) {
            float x = At<float>(ball, 0x124);
            float y = At<float>(ball, 0x128);
            float z = At<float>(ball, 0x12C);
            float s = (y * y + x * x) + z * z;
            holds = double(s) > 9.999999747378752e-05;
        } else if (state == 4) {
            holds = ball[0x22C] != 0;
        } else if (state == 5) {
            holds = (ball[0x1BD] & 8) != 0;
        } else if (state == 6) {
            uint32_t flags;
            holds = (*Dodgeball_GetFlags(dodgeballGame, &flags) & 0x40) != 0;
        }
    } else if (state == 2) {
        holds = Dodgeball_PlayerIndex(dodgeballGame, ball) < 4;
    } else if (state == 3) {
        holds = Dodgeball_PlayerIndex(dodgeballGame, ball) >= 4;
    } else if (state == 7 || state == 8) {
        int holder = Dodgeball_PlayerIndex(dodgeballGame, ball);
        int mine = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(owner));
        bool same = (holder >= 4) == (mine >= 4);
        holds = state == 7 ? same : !same;
    }
    if (holds) {
        return self[8];
    }
    return self[8] == 0;
}

// 0x675550 (thiscall): DodgeballPedLocationCond's vf1: the owner's place on the court as
// fractions along its two axes (the court's origin +0x9C, axes +0xA8 / +0xB4, scaled by its
// extent) within +4..+0xC and +8..+0x10 returns the expected byte (+0x14), else whether
// that's 0 (above 20 or below -0.3 of the court: nowhere)
uint8_t __fastcall PedLocationCond_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* court = At<uint8_t*>(dodgeballGame, 0x17D4);
    float* u = &At<float>(court, 0xA8);
    float* v = &At<float>(court, 0xB4);
    float w[3];
    VectorSub(w, &At<float>(court, 0xC), reinterpret_cast<float*>(court));
    float du = (u[1] * w[1] + u[0] * w[0]) + u[2] * w[2];
    float negDu = -du;
    float dv = (w[0] * v[0] + w[1] * v[1]) + w[2] * v[2];
    float* p = Pos(OwnerPed(owner));
    float position[3] = {p[0], p[1], p[2]};
    float r[3];
    VectorSub(r, position, &At<float>(court, 0x9C));
    float su = (u[0] * r[0] + u[1] * r[1]) + u[2] * r[2];
    float fx = su / negDu;
    float sv = (r[0] * v[0] + r[1] * v[1]) + v[2] * r[2];
    float fy = sv / dv;
    if (20.0f < r[2] || r[2] < -0.3f) {
        fx = -99999997952.0f;
        fy = -99999997952.0f;
    }
    uint8_t expected = self[0x14];
    uint8_t no = expected == 0;
    if (At<float>(self, 4) > fx) {
        return no;
    }
    if (At<float>(self, 0xC) < fx) {
        return no;
    }
    if (At<float>(self, 8) > fy) {
        return no;
    }
    if (At<float>(self, 0x10) < fy) {
        return no;
    }
    return expected;
}

// 0x675710 (thiscall): DodgeballSetMessageTrack's vf2: the game's message set (its name,
// +0x18) and its time (+0x1880)
uint8_t __fastcall SetMessage_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    Dodgeball_SetMessage(dodgeballGame, At<const char*>(self, 0x18));
    At<uint32_t>(dodgeballGame, 0x1880) = timeInMs;
    return 1;
}

// 0x675740 (thiscall): DodgeballAttackerDistCond's vf1: the distance to the other team's
// current player compared (+8) with +4
uint8_t __fastcall AttackerDistCond_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(owner));
    uint8_t* other = Dodgeball_PlayerPed(dodgeballGame, Dodgeball_OtherTeamCurrent(dodgeballGame, index));
    float* mine = Pos(OwnerPed(owner));
    float d[3];
    VectorSub(d, Pos(other), mine);
    float sum = (d[1] * d[1] + d[0] * d[0]) + d[2] * d[2];
    float distance = crt::Sqrtf(std::fabs(sum));   // (the CRT x87 sqrt)
    return CompareOp(distance, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x675810 (thiscall): DodgeballAttackTimeCond's vf1: the milliseconds since the last
// message compared (+8) with +4
uint8_t __fastcall AttackTimeCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    float ms = float(UnsignedX87(timeInMs - At<uint32_t>(dodgeballGame, 0x1880)));
    return CompareOp(ms, At<float>(self, 4), At<int32_t>(self, 8));
}

// 0x675870 (thiscall): DodgeballCanPickupCond's vf1: every player's 0x80 set when holding a
// ball (and 0x1000); with the ball loose, each team's player nearest a point 0.3 short of
// it (level; on their side of the court: the players standing on the other side's polygons
// keep 0x1000 only with a ball) flagged 0x1080 when within +4 (below 0: any distance)
uint8_t __fastcall CanPickupCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    for (int i = 0; i < 8; i++) {
        uint8_t* player = Dodgeball_Player(dodgeballGame, i);
        uint32_t has = At<int16_t>(At<uint8_t*>(At<uint8_t*>(player, 0), 0x1D0), 0x10E) != -1;
        At<uint32_t>(player, 4) = (((has & 1) | 0x20) << 7) | (At<uint32_t>(player, 4) & 0xFFFFFF7Fu);
    }
    int best0 = -1;
    int best1 = -1;
    float distance0 = -1.0f;
    float distance1 = -1.0f;
    uint8_t* ball = Dodgeball_GetBall(dodgeballGame);
    if (!ball || IsPed(ball)) {
        return 0;
    }
    float* bp = Pos(ball);
    float ballPos[3] = {bp[0], bp[1], bp[2]};
    for (int i = 0; i < 8; i++) {
        uint8_t* player = Dodgeball_Player(dodgeballGame, i);
        float tmp[3];
        float* d = VectorSub(tmp, Pos(At<uint8_t*>(player, 0)), ballPos);
        float dir[3] = {d[0], d[1], 0.0f};
        Vector_Normalise(dir);
        constexpr double k = 0.30000001192092896;
        float offset[3] = {float(double(dir[0]) * k), float(double(dir[1]) * k), float(k * double(dir[2]))};
        float point[3];
        Copy3(point, VectorAdd(tmp, ballPos, offset));
        uint8_t team = uint8_t(Dodgeball_TeamCode(dodgeballGame, At<uint8_t*>(player, 0)));
        NavRef* own = reinterpret_cast<NavRef*>(At<uint8_t*>(player, 0) + 0x1268);
        NavRef ref = *own;
        if (NavRef_Valid(own)) {
            if (PolygonTeam(ref) != team) {
                uint32_t hasBall = At<int16_t>(At<uint8_t*>(At<uint8_t*>(player, 0), 0x1D0), 0x10E) != -1;
                At<uint32_t>(player, 4) ^= ((hasBall << 0xC) ^ At<uint32_t>(player, 4)) & 0x1000;
            } else if (Nav_FindNear(point, 2.0f, &ref, 0, 2.0f) && PolygonTeam(ref) != team) {
                continue;
            }
        }
        uint8_t* ped = At<uint8_t*>(player, 0);
        float e[3];
        VectorSub(e, point, Pos(ped));
        e[2] = 0.0f;
        float sum = (e[0] * e[0] + e[1] * e[1]) + 0.0f * 0.0f;
        float distance = crt::Sqrtf(std::fabs(sum));
        if (i < 4) {
            if (distance0 > distance || 0.0f > distance0) {
                distance0 = distance;
                best0 = i;
            }
        } else if (distance1 > distance || 0.0f > distance1) {
            distance1 = distance;
            best1 = i;
        }
    }
    float range = At<float>(self, 4);
    uint8_t found = 0;
    if (best0 >= 0 && (0.0f > range || !(range < distance0 || range != range || distance0 != distance0))) {
        At<uint32_t>(Dodgeball_Player(dodgeballGame, best0), 4) |= 0x1080;
        found = 1;
    }
    if (best1 >= 0 && !(range < distance1 || range != range || distance1 != distance1)) {
        // (the other team's check compares against the first team's distance too)
        if (0.0f > range || !(range < distance0 || range != range || distance0 != distance0)) {
            At<uint32_t>(Dodgeball_Player(dodgeballGame, best1), 4) |= 0x1080;
            found = 1;
        }
    }
    return found;
}

// 0x675C70 (thiscall): CDodgeballPickupBallTrack's vf3: with the ball in flight and the
// owner on its side, whether it can intercept it (its reach set from +0x18.. in degrees);
// if so, by chance (+0x38 for a teammate's throw, +0x3C an opponent's, else 1) its player's
// catch time (+0x238) set now. Done when on the other side's polygon.
uint8_t __fastcall PickupBall_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ball = Dodgeball_GetBall(dodgeballGame);
    if (!ball || !At<uint32_t>(ball, 0xC4)) {
        return 1;
    }
    float* bp = Pos(ball);
    V3 ballPos = {bp[0], bp[1], bp[2]};
    V3 velocity = {At<float>(ball, 0x124), At<float>(ball, 0x128), At<float>(ball, 0x12C)};
    uint8_t team = uint8_t(Dodgeball_TeamCode(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x44))));
    NavRef* own = reinterpret_cast<NavRef*>(OwnerPed(At<uint8_t*>(self, 0x44)) + 0x1268);
    NavRef ref = *own;
    if (NavRef_Valid(own) && PolygonTeam(ref) != team) {
        return 0;
    }
    V3 offset = {0.0f, At<float>(self, 0x1C), At<float>(self, 0x20)};
    float r28 = At<float>(self, 0x28) * 3.14159274f / 180.0f;
    float r24 = At<float>(self, 0x24) * 3.14159274f / 180.0f;
    float r30 = 3.14159274f * At<float>(self, 0x30) / 180.0f;
    alignas(4) uint8_t intercept[0x40];
    Intercept_Setup(intercept, OwnerPed(At<uint8_t*>(self, 0x44)), At<int32_t>(self, 0x18), offset, r30,
                    At<float>(self, 0x2C), r24, r28);
    float e = At<float>(self, 0x34);
    // (the flag goes as a word whose other bytes are the last angle's)
    uint32_t flag = (std::bit_cast<uint32_t>(r30) & 0xFFFFFF00u) | self[0x40];
    float speed = Weapon_InfoFloatC(ball);
    uint8_t can = Intercept_Test(intercept, ballPos, velocity, speed, e, flag);
    float chance = 1.0f;
    float u = float(CrtRand()) / 32767.0f * (1.0f - 0.0f) + 0.0f;
    uint8_t* thrower = At<uint8_t*>(Projectile_Info(At<int32_t>(ball, 0x238)), 4);
    if (thrower) {
        chance = Dodgeball_SameTeam(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x44)), thrower) ? At<float>(self, 0x38)
                                                                                                 : At<float>(self, 0x3C);
    }
    if (chance < u) {
        return 1;
    }
    if (can) {
        At<uint32_t>(Dodgeball_PlayerOf(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x44))), 0x238) = timeInMs;
    }
    return 1;
}

// 0x675EE0 (cdecl): an AI's target point by mode: 0 its own position, 1 / 2 points on the
// court (-0.2 / 0.6 across for the first team, 0.2 / -0.6 for the second; 0.2 up), 3 the
// other team's current player's, 4 the ball's (none: its own)
float* __cdecl TargetPosition(float* out, uint8_t* ped, int mode) {
    Copy3(out, Pos(ped));
    int index = Dodgeball_PlayerIndex(dodgeballGame, ped);
    int team = index >= 4;
    switch (uint32_t(mode)) {
    case 0:
        Copy3(out, Pos(ped));
        break;
    case 1:
    case 2: {
        float across = mode == 1 ? (team ? 0.2f : -0.2f) : (team ? -0.6f : 0.6f);
        float tmp[3];
        Copy3(out, Court_Point(At<uint8_t*>(dodgeballGame, 0x17D4), tmp, across, 0.0f, 0.0f));
        out[2] = float(double(out[2]) + 0.20000000298023224);
        break;
    }
    case 3:
        Copy3(out, Pos(Dodgeball_PlayerPed(dodgeballGame, Dodgeball_OtherTeamCurrent(dodgeballGame, index))));
        break;
    case 4:
        if (Dodgeball_GetBall(dodgeballGame)) {
            Copy3(out, Pos(Dodgeball_GetBall(dodgeballGame)));
        }
        break;
    default:
        break;
    }
    return out;
}

// 0x676060 (thiscall): DodgeballSetPathTrack's vf3: its player's path points (+0x23C,
// +0x248) set to the targets of modes +0x18 and +0x1C
uint8_t __fastcall SetPath_Update(uint8_t* self, re::Edx, float) {
    float first[3];
    TargetPositionFn(first, OwnerPed(At<uint8_t*>(self, 0x20)), At<int32_t>(self, 0x18));
    float second[3];
    TargetPositionFn(second, OwnerPed(At<uint8_t*>(self, 0x20)), At<int32_t>(self, 0x1C));
    Copy3(&At<float>(Dodgeball_PlayerOf(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x20))), 0x23C), first);
    Copy3(&At<float>(Dodgeball_PlayerOf(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x20))), 0x248), second);
    return 1;
}

// 0x676100 (thiscall): DodgeballCanPathToCond's vf1: true for the first team's current
// player and players flagged 8; else whether the nav polygon near where it stands is on its
// side (none near: true)
uint8_t __fastcall CanPathToCond_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    if (!OwnerPed(owner)) {
        return 0;
    }
    int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(owner));
    if (index == Dodgeball_TeamCurrent(dodgeballGame, 0)) {
        return 1;
    }
    if (Dodgeball_Player(dodgeballGame, index)[4] & 8) {
        return 1;
    }
    float position[3] = {0.0f, 0.0f, 0.0f};
    Nav_Position(OwnerPed(owner) + 0xDAC, position);
    uint8_t team = uint8_t(Dodgeball_TeamCode(dodgeballGame, OwnerPed(owner)));
    NavRef ref = {nullptr, -1, {0.0f, 0.0f, 0.0f}};
    if (!Nav_FindNear(position, 2.0f, &ref, 0, 2.0f)) {
        return 1;
    }
    return PolygonTeam(ref) == team;
}

// 0x676210 (thiscall): DodgeballFollowPathTrack's vf3: the owner moved towards its path's
// second point (+0x18 its b, a 1.1)
uint8_t __fastcall FollowPath_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x20));
    float* point = &At<float>(Dodgeball_PlayerOf(dodgeballGame, ped), 0x248);
    V3 target = {point[0], point[1], point[2]};
    float unused[3];
    VectorSub(unused, &target.x, Pos(ped));
    float b = At<float>(self, 0x18);
    uint8_t* mover = OwnerPed(At<uint8_t*>(self, 0x20));
    uint32_t team = Dodgeball_TeamCode(dodgeballGame, ped);
    return PedMove_To(mover + 0x654, target, 0x1C, 1.1f, b, 0, team);
}

// ---- the registration of the tracks and conditions --------------------------------------------

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, NameKey_Empty, 0x0066BEA0, void* key);
GAME_FN(void, __thiscall, NameKey_SetName, 0x005FA890, void* key, const char* name);
GAME_FN(void, __thiscall, Registry_Add, 0x0061A700, void* registry, void* entry);
inline void* const trackRegistry_1B6565C = reinterpret_cast<void*>(0x01B6565C);
inline void* const conditionRegistry_1B64A94 = reinterpret_cast<void*>(0x01B64A94);

struct RegistryEntry {
    uint32_t name;       // a string's address
    uint32_t create;     // New
    uint32_t createAt;   // CreateAt
    uint32_t typeKey;    // the class's name key
};
constexpr RegistryEntry kTracks[] = {
    {0x0093E104, 0x006762D0, 0x00673180, 0x01B89164},  // DodgeballLeadAI
    {0x0093E0F0, 0x00676310, 0x006731B0, 0x01B89184},  // DodgeballSupportAI
    {0x0093E0D8, 0x00676370, 0x006731D0, 0x01B89180},  // DodgeballSidelineLeadAI
    {0x0093E0C4, 0x006763A0, 0x006731F0, 0x01B8917C},  // DodgeballSidelineAI
    {0x0093E0B0, 0x006763E0, 0x00673230, 0x01B89188},  // DodgeballFaceLead
    {0x0093E09C, 0x00676420, 0x00673270, 0x01B8918C},  // DodgeballCatchBall
    {0x0093E088, 0x00676460, 0x00673290, 0x01B89168},  // DodgeballGetBallAI
    {0x0093E078, 0x00676490, 0x006732D0, 0x01B89190},  // DodgeballGotoAI
    {0x0093E068, 0x006764D0, 0x00673310, 0x01B89194},  // DodgeballIdleAI
    {0x0093E054, 0x00673330, 0x006733B0, 0x01B89140},  // DodgeballSetPath
    {0x0093E040, 0x006733D0, 0x00673440, 0x01B89148},  // DodgeballFollowPath
    {0x0093E028, 0x00673460, 0x006734E0, 0x01B890E4},  // DodgeballChangePlayer
    {0x0093E014, 0x00673500, 0x00673580, 0x01B890D0},  // DodgeballSetPedFlag
    {0x0093DFF8, 0x006735A0, 0x00673620, 0x01B890D4},  // DodgeballTargetSetPedFlag
    {0x0093DFE8, 0x00673640, 0x006736B0, 0x01B890EC},  // DodgeballTarget
    {0x0093DFD0, 0x006736D0, 0x00673740, 0x01B890E0},  // DodgeballSetGameFlag
    {0x0093DFBC, 0x00673760, 0x006737D0, 0x01B890FC},  // DodgeballGameIdle
    {0x0093DFA8, 0x006737F0, 0x00673890, 0x01B89100},  // DodgeballGameTimer
    {0x0093DF98, 0x006738C0, 0x00673930, 0x01B89108},  // DodgeballReset
    {0x0093DF84, 0x00673950, 0x006739C0, 0x01B89104},  // DodgeballSetTurn
    {0x0093DF70, 0x006739E0, 0x00673A50, 0x01B890D8},  // DodgeballPedSetDuck
    {0x0093DF54, 0x00673A70, 0x00673AF0, 0x01B890F0},  // DodgeballLocationCollision
    {0x0093DF40, 0x00673B10, 0x00673BA0, 0x01B8911C},  // DodgeballSetMessage
    {0x0093DF2C, 0x00673BD0, 0x00673C50, 0x01B8912C},  // DodgeballCameraView
    {0x0093DF0C, 0x00673C70, 0x00673CF0, 0x01B89130},  // DodgeballBallLocationCollision
    {0x0093DEF8, 0x00673D10, 0x00673DB0, 0x01B8913C},  // DodgeballPickupBall
    {0x0093DEE4, 0x00673DD0, 0x00673E60, 0x01B890E8},  // DodgeballPedMarker
};
constexpr RegistryEntry kConditions[] = {
    {0x0093DED0, 0x00673E80, 0x00673EE0, 0x01B890CC},  // DodgeballPedFlag
    {0x0093DEBC, 0x00673F00, 0x00673F60, 0x01B890DC},  // DodgeballGameFlag
    {0x0093DEA8, 0x00673F80, 0x00673FE0, 0x01B890F4},  // DodgeballPedInArea
    {0x0093DE94, 0x00674000, 0x00674060, 0x01B8910C},  // DodgeballBallState
    {0x0093DE80, 0x00674080, 0x006740D0, 0x01B89118},  // DodgeballGameOver
    {0x0093DE68, 0x006740F0, 0x00674160, 0x01B89110},  // DodgeballBallLocation
    {0x0093DE50, 0x00674180, 0x006741F0, 0x01B89114},  // DodgeballPedLocation
    {0x0093DE3C, 0x00674210, 0x006742A0, 0x01B89120},  // DodgeballMessage
    {0x0093DE24, 0x006742D0, 0x00674340, 0x01B89124},  // DodgeballAttackerDist
    {0x0093DE10, 0x00674360, 0x006743D0, 0x01B89134},  // DodgeballPlayersIn
    {0x0093DDFC, 0x006743F0, 0x00674450, 0x01B89138},  // DodgeballCanPickup
    {0x0093DDE8, 0x00674470, 0x006744E0, 0x01B89128},  // DodgeballAttackTime
    {0x0093DDD4, 0x00674500, 0x00674550, 0x01B89144},  // DodgeballCanPathTo
    {0x0093DDC0, 0x00674570, 0x006745C0, 0x01B8914C},  // DodgeballBallCount
};

// a registration (0xC: its name key, New, CreateAt) made, the class's key named, and kept
// in one of the game's arrays (data, capacity, size; doubled when full)
inline void AddRegistration(uint8_t* game, int array, const RegistryEntry& e) {
    uint8_t* entry = static_cast<uint8_t*>(Malloc(0xC));
    if (entry) {
        NameKey_Empty(entry);
    } else {
        entry = nullptr;
    }
    const char* name = reinterpret_cast<const char*>(e.name);
    NameKey_SetName(entry, name);
    At<uint32_t>(entry, 8) = e.createAt;
    At<uint32_t>(entry, 4) = e.create;
    NameKey_SetName(reinterpret_cast<void*>(e.typeKey), name);
    uint32_t& capacity = At<uint32_t>(game, array + 4);
    uint32_t& size = At<uint32_t>(game, array + 8);
    if (size == capacity) {
        uint32_t grown = capacity ? capacity * 2 : 1;
        re::Ptr32<uint8_t>* data = nullptr;
        capacity = grown;
        if (grown) {
            uint64_t bytes = uint64_t(grown) * 4;
            data = static_cast<re::Ptr32<uint8_t>*>(Malloc((bytes >> 32) ? 0xFFFFFFFFu : uint32_t(bytes)));
        }
        re::Ptr32<uint8_t>* old = At<re::Ptr32<uint8_t>*>(game, array);
        if (old) {
            for (uint32_t i = 0; i < size; i++) {
                data[i] = At<re::Ptr32<uint8_t>*>(game, array)[i];
            }
            Free(At<re::Ptr32<uint8_t>*>(game, array));
        }
        At<re::Ptr32<uint8_t>*>(game, array) = data;
    }
    At<re::Ptr32<uint8_t>*>(game, array)[size] = entry;
    size += 1;
}

// 0x676510 (thiscall): the dodgeball tracks and conditions registered (their registrations
// kept in the game's arrays +0x1804 / +0x1810, then added to the track and condition
// registries)
void __fastcall Dodgeball_RegisterTracks(uint8_t* game, re::Edx) {
    for (const RegistryEntry& e : kTracks) {
        AddRegistration(game, 0x1804, e);
    }
    for (const RegistryEntry& e : kConditions) {
        AddRegistration(game, 0x1810, e);
    }
    for (uint32_t i = 0; i < At<uint32_t>(game, 0x180C); i++) {
        Registry_Add(trackRegistry_1B6565C, At<re::Ptr32<uint8_t>*>(game, 0x1804)[i]);
    }
    for (uint32_t i = 0; i < At<uint32_t>(game, 0x1818); i++) {
        Registry_Add(conditionRegistry_1B64A94, At<re::Ptr32<uint8_t>*>(game, 0x1810)[i]);
    }
}

// ---- changing players --------------------------------------------------------------------------

GAME_FN(void, __thiscall, Dodgeball_SetCurrent_67AFA0, 0x0067AFA0, uint8_t* game, int player);
GAME_FN(void, __thiscall, Dodgeball_PlayerPositions, 0x0067B4B0, uint8_t* game, float* out, int count);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, int count, void* ctor);
GAME_FN(void, __thiscall, ChangePlayer_ApplyFn, 0x00674860, uint8_t* self, int swap);

// CDodgeballChangePlayerTrack (0x2C): +0x18 keep the target, +0x19 passed on, +0x1A at
// once, +0x1B stay, +0x1C the owner, +0x20 its team, +0x24 / +0x28 from / to (in the team)

// 0x676DE0 (thiscall): its vf2: the next player (the next in play unless +0x19; the same
// with +0x1B); at once (+0x1A) made current, else the change applied now: none: done
uint8_t __fastcall ChangePlayer_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(owner));
    int team = index >= 4;
    At<int32_t>(self, 0x28) = index;
    At<int32_t>(self, 0x24) = index;
    At<int32_t>(self, 0x20) = team;
    if (index >= 4) {
        At<int32_t>(self, 0x28) = index - 4;
        At<int32_t>(self, 0x24) = index - 4;
    }
    bool check = false;
    if (self[0x19]) {
        if (!self[0x1B]) {
            At<int32_t>(self, 0x28) += 1;
            if (At<int32_t>(self, 0x28) >= 4) {
                At<int32_t>(self, 0x28) = 0;
            }
        }
    } else if (!self[0x1B]) {
        do {
            At<int32_t>(self, 0x28) += 1;
            if (At<int32_t>(self, 0x28) >= 4) {
                At<int32_t>(self, 0x28) = 0;
            }
            if (At<int32_t>(self, 0x28) == At<int32_t>(self, 0x24)) {
                break;
            }
            uint32_t flags;
            if (*Dodgeball_GetPlayerFlags(dodgeballGame, &flags, At<int32_t>(self, 0x28) + At<int32_t>(self, 0x20) * 4) & 1) {
                break;
            }
        } while (!self[0x1B]);
        check = true;
    }
    if (check && !self[0x1B] && At<int32_t>(self, 0x28) == At<int32_t>(self, 0x24)) {
        return 0;
    }
    if (self[0x1A]) {
        Dodgeball_SetCurrent_67AFA0(dodgeballGame, At<int32_t>(self, 0x28) + At<int32_t>(self, 0x20) * 4);
        return 0;
    }
    int base = At<int32_t>(self, 0x20) * 4;
    int to = At<int32_t>(self, 0x28) + base;
    int from = At<int32_t>(self, 0x24) + base;
    uint8_t* ped = Dodgeball_PlayerPed(dodgeballGame, to);
    Dodgeball_ChangePlayer(dodgeballGame, from, to, self[0x19]);
    Targeting_SetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x1C8), ped);
    if (!self[0x18]) {
        Targeting_SetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x1C8), nullptr);
    }
    return 1;
}

// 0x676F20 (thiscall): its vf3: while the owner is moving (its mover +0x2E0: direction,
// +0x1C speed above 0.1), the team mate (in play unless +0x19) nearest the point 10 x the
// move ahead becomes the change's target (applied when it changes)
uint8_t __fastcall ChangePlayer_Update(uint8_t* self, re::Edx, float) {
    if (!self[0x1B] && At<int32_t>(self, 0x28) == At<int32_t>(self, 0x24)) {
        return 0;
    }
    if (self[0x1A]) {
        return 0;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    float* mover = At<float*>(ped, 0x2E0);
    if (!(double(mover[7]) > 0.10000000149011612)) {
        return 1;
    }
    float direction[3] = {mover[0], mover[1], mover[2]};
    float speed = mover[7];
    float move[3] = {direction[0] * speed, direction[1] * speed, speed * direction[2]};
    float ahead[3] = {float(double(move[0]) * 10.0), float(double(move[1]) * 10.0), float(10.0 * double(move[2]))};
    float point[3];
    VectorAdd(point, Pos(ped), ahead);
    int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(At<uint8_t*>(self, 0x1C)));
    int first = (index >= 4) * 4;
    float positions[8][3];
    VectorConstruct(positions, 0xC, 8, reinterpret_cast<void*>(0x00508410));
    Dodgeball_PlayerPositions(dodgeballGame, &positions[0][0], 8);
    float best = 1.0000000200408773e+21f;
    int found = -1;
    int j = 0;
    for (int i = first; i < first + 4; i++, j++) {
        if (i == index) {
            continue;
        }
        if (!self[0x19]) {
            uint32_t flags;
            if (!(*Dodgeball_GetPlayerFlags(dodgeballGame, &flags, i) & 1)) {
                continue;
            }
        }
        float d[3];
        VectorSub(d, positions[i], point);
        float sum = (d[1] * d[1] + d[0] * d[0]) + d[2] * d[2];
        if (best > sum) {
            best = sum;
            found = j;
        }
    }
    if (found != -1 && found != At<int32_t>(self, 0x28)) {
        At<int32_t>(self, 0x28) = found;
        ChangePlayer_ApplyFn(self, 0);
    }
    return 1;
}

// 0x677120 (thiscall): its vf4: the change swapped in at the end (unless none or at once);
// the target cleared unless +0x18
uint8_t __fastcall ChangePlayer_End(uint8_t* self, re::Edx) {
    if (!self[0x1B] && At<int32_t>(self, 0x28) == At<int32_t>(self, 0x24)) {
        return 0;
    }
    if (self[0x1A]) {
        return 0;
    }
    int base = At<int32_t>(self, 0x20) * 4;
    int to = At<int32_t>(self, 0x28) + base;
    int from = At<int32_t>(self, 0x24) + base;
    Dodgeball_PlayerPed(dodgeballGame, to);   // (unused)
    Dodgeball_SwapPlayers(dodgeballGame, from, to, self[0x19]);
    if (!self[0x18]) {
        Targeting_SetTarget(At<uint8_t*>(OwnerPed(At<uint8_t*>(self, 0x1C)), 0x1C8), nullptr);
    }
    return 1;
}

GAME_VAR(CPool<uint8_t>*, objectPool_C0F618, 0x00C0F618);
GAME_FN(int, __thiscall, Pool_ChainSize, 0x00435BB0, void* next);
GAME_FN(uint8_t*, __thiscall, Pool_ChainAt, 0x00435BD0, void* next, int slot, int);
GAME_FN(uint8_t, __cdecl, CompareInts, 0x005FBCB0, int32_t a, int32_t b, int op);

// 0x677190 (thiscall): DodgeballBallCountCond's vf1: the balls (players holding one and
// dodgeball objects in play) compared (+8) with +4. (The chained pools' slots are read
// with the first pool's flags.)
uint8_t __fastcall BallCountCond_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    int count = 0;
    for (int i = 0; i < 9; i++) {
        if (At<int16_t>(At<uint8_t*>(At<uint8_t*>(Dodgeball_Player(dodgeballGame, i), 0), 0x1D0), 0x10E) != -1) {
            count++;
        }
    }
    int weapon = GetWeaponIndex(reinterpret_cast<const char*>(0x00904C68));   // "dodgeball"
    CPool<uint8_t>* pool = objectPool_C0F618;
    int size = pool->size;
    int total = size;
    if (pool->unknown10) {
        total += Pool_ChainSize(reinterpret_cast<void*>(pool->unknown10));
    }
    for (int slot = total; slot;) {
        int chained = slot - size - 1;
        slot--;
        uint8_t* object = nullptr;
        if (slot >= size) {
            if (pool->unknown10) {
                object = Pool_ChainAt(reinterpret_cast<void*>(pool->unknown10), chained, 1);
            }
        } else {
            object = re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot;
        }
        if (!(pool->flags[slot] & 0x80) && object && At<uint32_t>(object, 0xC4) && At<int16_t>(object, 0x10E) == weapon) {
            count++;
        }
    }
    return CompareInts(count, At<int32_t>(self, 4), At<int32_t>(self, 8));
}

// ---- the court's collisions ---------------------------------------------------------------------

using dbcourt::PlaneDistance;
// the area's corners: (+0x18, +0x1C), (+0x20, +0x1C), (+0x20, +0x24), (+0x18, +0x24)
inline void Area_Corners(uint8_t* self, const dbcourt::Frame& f, float corners[4][3]) {
    dbcourt::Corner(f, At<float>(self, 0x18), At<float>(self, 0x1C), corners[0]);
    dbcourt::Corner(f, At<float>(self, 0x20), At<float>(self, 0x1C), corners[1]);
    dbcourt::Corner(f, At<float>(self, 0x20), At<float>(self, 0x24), corners[2]);
    dbcourt::Corner(f, At<float>(self, 0x18), At<float>(self, 0x24), corners[3]);
}
// the nearest plane among those the point is inside of all (its distance; none: -1)
inline int Planes_Nearest(float planes[4][4], const float* p, float* best) {
    int found = -1;
    uint8_t inside = 1;
    *best = 99999997952.0f;
    for (int i = 0; i < 4; i++) {
        float distance = PlaneDistance(planes[i], p);
        if (distance < *best) {
            found = i;
            *best = distance;
        }
        if (0.0f > distance) {
            inside = 0;
        }
    }
    return inside ? found : -1;
}
// the horizontal distance from an entity to a point
inline float LevelDistance(const float* point, uint8_t* entity) {
    float d[3];
    VectorSub(d, point, Pos(entity));
    float sum = (d[0] * d[0] + d[1] * d[1]) + 0.0f * 0.0f;
    return crt::Sqrtf(std::fabs(sum));
}
// a ped moved to a point (the matrix's +0x3C gets `w`)
inline void MovePed(uint8_t* ped, const float* p, uint32_t w) {
    float x = p[0];
    float y = p[1];
    float z = p[2];
    uint8_t* matrix = At<uint8_t*>(ped, 0x14);
    if (matrix) {
        At<float>(matrix, 0x30) = x;
        At<float>(matrix, 0x34) = y;
        At<float>(matrix, 0x38) = z;
        At<uint32_t>(matrix, 0x3C) = w;
    }
    At<float>(ped, 4) = x;
    At<float>(ped, 8) = y;
    At<float>(ped, 0xC) = z;
}

// 0x677280 (thiscall): CDodgeballLocationCollisionTrack's vf3: for the first team's current
// player, an area of the court (+0x18.., fractions of it): with +0x29 kept inside (pushed
// back through each edge it's past, in turn), else kept out (pushed out through the nearest
// edge when inside); the move applied when between 0.0001 and 1. Kept out, with +0x28 its
// mover's direction (+0x2E0) also turned along the edge when its point 0.2 ahead would be
// inside (its speed zeroed if nothing is left).
uint8_t __fastcall LocationCollision_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x2C);
    int index = Dodgeball_PlayerIndex(dodgeballGame, OwnerPed(owner));
    if (index != Dodgeball_TeamCurrent(dodgeballGame, 0)) {
        return 0;
    }
    dbcourt::Frame frame = dbcourt::MakeFrame(At<uint8_t*>(dodgeballGame, 0x17D4));
    float corners[4][3];
    Area_Corners(self, frame, corners);
    float planes[4][4] = {};
    if (self[0x29]) {
        for (int i = 0; i < 4; i++) {
            dbcourt::EdgePlane(corners[i], corners[(i + 1) % 4], false, planes[i]);
        }
        // (the matrix's +0x3C gets the frame's last plane distance, as the game's)
        uint32_t lastD = std::bit_cast<uint32_t>(planes[3][3]);
        uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
        float* p = Pos(ped);
        float q[3] = {p[0], p[1], p[2]};
        dbcourt::PushInside(planes, q);
        float moved = LevelDistance(q, OwnerPed(owner));
        if (double(moved) > 9.999999747378752e-05 && moved < 1.0f) {
            MovePed(OwnerPed(At<uint8_t*>(self, 0x2C)), q, lastD);
        }
        return 1;
    }
    for (int i = 0; i < 4; i++) {
        dbcourt::EdgePlane(corners[i], corners[(i + 1) % 4], true, planes[i]);
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x2C));
    float* p = Pos(ped);
    float q[3] = {p[0], p[1], p[2]};
    (void)static_cast<uint8_t>(*reinterpret_cast<volatile uint8_t*>(0x01B890BA));   // (read; changes nothing)
    float best;
    int nearest = Planes_Nearest(planes, q, &best);
    if (nearest != -1) {
        float c[3] = {planes[nearest][0] * best, planes[nearest][1] * best, best * planes[nearest][2]};
        q[0] = q[0] - c[0];
        q[1] = q[1] - c[1];
        q[2] = q[2] - c[2];
    }
    float moved = LevelDistance(q, OwnerPed(At<uint8_t*>(self, 0x2C)));
    if (double(moved) > 9.999999747378752e-05 && moved < 1.0f) {
        // (the matrix's +0x3C gets uninitialised stack in the game; 0 here)
        MovePed(OwnerPed(At<uint8_t*>(self, 0x2C)), q, 0);
    }
    if (!self[0x28]) {
        return 1;
    }
    float* mover = At<float*>(OwnerPed(At<uint8_t*>(self, 0x2C)), 0x2E0);
    float v[3] = {mover[0], mover[1], mover[2]};
    float sum = (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2];
    if (crt::Sqrtf(std::fabs(sum)) == 0.0f) {
        return 1;
    }
    Vector_Normalise(v);
    float step[3] = {v[0] * 0.2f, v[1] * 0.2f, 0.2f * v[2]};
    float ahead[3];
    Copy3(ahead, VectorAdd(ahead, Pos(OwnerPed(At<uint8_t*>(self, 0x2C))), step));
    nearest = Planes_Nearest(planes, ahead, &best);
    if (nearest == -1) {
        return 1;
    }
    float* plane = planes[nearest];
    float c[3] = {plane[0] * best, plane[1] * best, best * plane[2]};
    ahead[0] = ahead[0] - c[0];
    ahead[1] = ahead[1] - c[1];
    ahead[2] = ahead[2] - c[2];
    float* here = Pos(OwnerPed(At<uint8_t*>(self, 0x2C)));
    float h[3] = {here[0], here[1], here[2]};
    float e = ((plane[1] * h[1] + h[0] * plane[0]) + plane[2] * h[2]) - plane[3];
    float f[3] = {plane[0] * e, plane[1] * e, e * plane[2]};
    h[0] = h[0] - f[0];
    h[1] = h[1] - f[1];
    h[2] = h[2] - f[2];
    float* move = At<float*>(OwnerPed(At<uint8_t*>(self, 0x2C)), 0x2E0);
    float out[3];
    Copy3(move, VectorSub(out, ahead, h));
    move = At<float*>(OwnerPed(At<uint8_t*>(self, 0x2C)), 0x2E0);
    float m2 = (move[0] * move[0] + move[1] * move[1]) + move[2] * move[2];
    if (0.0f < crt::Sqrtf(std::fabs(m2))) {
        Vector_Normalise(move);
        return 1;
    }
    move[7] = 0.0f;
    return 1;
}

GAME_VAR(uint8_t*, dodgeballTuning_20C888C, 0x020C888C);   // +0x2C along, +0x30 back
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, uint8_t* entity, float x, float y, float z);

// 0x678450 (thiscall): CDodgeballBallLocationCollisionTrack's vf3: the flying ball kept in
// an area of the court: past an edge, put back on it and its velocity bounced (along it x
// the tuning's +0x2C, back x its +0x30); below the floor (+0xA4 + +0x28) put on it (+0.001)
// with an upward 0.01; applied when anything changed
uint8_t __fastcall BallLocationCollision_Update(uint8_t* self, re::Edx, float) {
    uint8_t* court = At<uint8_t*>(dodgeballGame, 0x17D4);
    dbcourt::Frame frame = dbcourt::MakeFrame(court);
    float corners[4][3];
    Area_Corners(self, frame, corners);
    uint8_t changed = 0;
    uint8_t* ball = Dodgeball_GetBall(dodgeballGame);
    if (!ball || !At<uint32_t>(ball, 0xC4)) {
        return 1;
    }
    float* bp = Pos(ball);
    float b[3] = {bp[0], bp[1], bp[2]};
    float v[3] = {At<float>(ball, 0x124), At<float>(ball, 0x128), At<float>(ball, 0x12C)};
    float planes[4][4];
    VectorConstruct(planes, 0x10, 4, reinterpret_cast<void*>(0x00508410));
    for (int i = 0; i < 4; i++) {
        float* plane = planes[i];
        float* a = corners[i];
        float edge[3];
        VectorSub(edge, corners[(i + 1) % 4], a);
        float up[3] = {0.0f, 0.0f, 1.0f};
        float n[3];
        CrossProduct(n, edge, up);
        Vector_Normalise(n);
        plane[0] = n[0];
        plane[1] = n[1];
        plane[2] = n[2];
        float d = (a[1] * n[1] + a[0] * n[0]) + a[2] * n[2];
        plane[3] = float(double(d) - 0.0);
        float dot = (b[0] * plane[0] + plane[1] * b[1]) + plane[2] * b[2];
        float k = -(dot - plane[3]);
        if (!(k < 0.0f)) {
            continue;
        }
        float c[3] = {plane[0] * k, plane[1] * k, k * plane[2]};
        b[0] = c[0] + b[0];
        b[1] = b[1] + c[1];
        b[2] = b[2] + c[2];
        float along = (plane[1] * v[1] + plane[0] * v[0]) + plane[2] * v[2];
        float r[3] = {plane[0] * along, plane[1] * along, plane[2] * along};
        float t[3];
        VectorSub(t, v, r);
        uint8_t* tuning = dodgeballTuning_20C888C;
        float keep = At<float>(tuning, 0x2C);
        float back = -At<float>(tuning, 0x30);
        float bounce[3] = {r[0] * back, r[1] * back, back * r[2]};
        float slide[3] = {t[0] * keep, t[1] * keep, keep * t[2]};
        float out[3];
        Copy3(v, VectorAdd(out, slide, bounce));
        changed = 1;
    }
    float floor = At<float>(court, 0xA4) + At<float>(self, 0x28);
    if (floor > b[2]) {
        b[2] = float(double(floor) + 0.0010000000474974513);
        v[2] = 0.01f;
    } else if (!changed) {
        return 1;
    }
    Entity_SetPosition(ball, b[0], b[1], b[2]);
    At<float>(ball, 0x124) = v[0];
    At<float>(ball, 0x128) = v[1];
    At<float>(ball, 0x12C) = v[2];
    return 1;
}

} // namespace

#define DODGETRK35_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

DODGETRK35_REVERSED("CDodgeballSetPedFlagTrack::Update_672A30", 0x00672A30, &SetPedFlag_Update);
DODGETRK35_REVERSED("CDodgeballTargetSetPedFlagTrack::Update_672A60", 0x00672A60, &TargetSetPedFlag_Update);
DODGETRK35_REVERSED("CDodgeballGameFlagCond::Evaluate_672A90", 0x00672A90, &GameFlagCond_Evaluate);
DODGETRK35_REVERSED("CDodgeballSetGameFlagTrack::Start_672AC0", 0x00672AC0, &SetGameFlag_Start);
DODGETRK35_REVERSED("CDodgeballPedMarkerTrack::Start_672B30", 0x00672B30, &PedMarker_Start);
DODGETRK35_REVERSED("CDodgeballPedMarkerTrack::End_672B50", 0x00672B50, &PedMarker_End);
DODGETRK35_REVERSED("CDodgeballTargetTrack::Start_672B60", 0x00672B60, &KeepOwner_Start<0x20, 0>);
DODGETRK35_REVERSED("CDodgeballLocationCollisionTrack::Start_672B70", 0x00672B70, &KeepOwner_Start<0x2C, 1>);
DODGETRK35_REVERSED("DodgeballBaseAITrack::OpponentPed_672B90", 0x00672B90, &AITrack_OpponentPed);
DODGETRK35_REVERSED("DodgeballGameTimerTrack::End_672BC0", 0x00672BC0, &GameTimer_End);
DODGETRK35_REVERSED("DodgeballSetTurnTrack::Start_672BE0", 0x00672BE0, &SetTurn_Start);
DODGETRK35_REVERSED("DodgeballResetTrack::Start_672E80", 0x00672E80, &Reset_Start);
DODGETRK35_REVERSED("DodgeballBallLocationCond::Evaluate_672EB0", 0x00672EB0, &BallLocationCond_Evaluate);
DODGETRK35_REVERSED("DodgeballGameOverCond::Evaluate_672F10", 0x00672F10, &GameOverCond_Evaluate);
DODGETRK35_REVERSED("DodgeballBallMessageCond::Evaluate_672F30", 0x00672F30, &BallMessageCond_Evaluate);
DODGETRK35_REVERSED("DodgeballCameraViewTrack::Start_672F80", 0x00672F80, &CameraView_Start);
DODGETRK35_REVERSED("DodgeballCameraViewTrack::End_673010", 0x00673010, &CameraView_End);
DODGETRK35_REVERSED("CDodgeballBallLocationCollisionTrack::Start_673030", 0x00673030, &KeepOwner_Start<0x30, 1>);
DODGETRK35_REVERSED("DodgeballPlayersInCond::Count_673040", 0x00673040, &PlayersIn);
DODGETRK35_REVERSED("DodgeballPlayersInCond::Evaluate_673090", 0x00673090, &PlayersInCond_Evaluate);
DODGETRK35_REVERSED("CDodgeballPickupBallTrack::Start_6730D0", 0x006730D0, &KeepOwner_Start<0x44, 1>);
DODGETRK35_REVERSED("Dodgeball::GiveBall_6730E0", 0x006730E0, &GiveBall);
DODGETRK35_REVERSED("CDodgeballPedFlagCond::Evaluate_6745E0", 0x006745E0, &PedFlagCond_Evaluate);
DODGETRK35_REVERSED("CDodgeballSetPedFlagTrack::Start_674630", 0x00674630, &SetPedFlag_Start);
DODGETRK35_REVERSED("CDodgeballSetPedFlagTrack::End_6746A0", 0x006746A0, &SetPedFlag_End);
DODGETRK35_REVERSED("CDodgeballTargetSetPedFlagTrack::Start_674710", 0x00674710, &TargetSetPedFlag_Start);
DODGETRK35_REVERSED("CDodgeballTargetSetPedFlagTrack::End_6747A0", 0x006747A0, &TargetSetPedFlag_End);
DODGETRK35_REVERSED("DodgeballPedSetDuckTrack::Start_674830", 0x00674830, &PedSetDuck_Start);
DODGETRK35_REVERSED("CDodgeballChangePlayerTrack::Apply_674860", 0x00674860, &ChangePlayer_Apply);
DODGETRK35_REVERSED("CDodgeballPedMarkerTrack::Update_6748E0", 0x006748E0, &PedMarker_Update);
DODGETRK35_REVERSED("CDodgeballTargetTrack::Update_6749D0", 0x006749D0, &Target_Update);
DODGETRK35_REVERSED("CDodgeballTargetTrack::End_674A70", 0x00674A70, &Target_End);
DODGETRK35_REVERSED("CDodgeballPedInAreaCond::Evaluate_674A90", 0x00674A90, &PedInAreaCond_Evaluate);
DODGETRK35_REVERSED("DodgeballBaseAITrack::Start_674FE0", 0x00674FE0, &BaseAI_Start);
DODGETRK35_REVERSED("DodgeballCatchBallTrack::MoveTo_675040", 0x00675040, &CatchBall_MoveTo);
DODGETRK35_REVERSED("DodgeballGameTimerTrack::Start_675140", 0x00675140, &GameTimer_Start);
DODGETRK35_REVERSED("DodgeballGameTimerTrack::Update_675170", 0x00675170, &GameTimer_Update);
DODGETRK35_REVERSED("DodgeballBallStateCond::Evaluate_6752F0", 0x006752F0, &BallStateCond_Evaluate);
DODGETRK35_REVERSED("DodgeballPedLocationCond::Evaluate_675550", 0x00675550, &PedLocationCond_Evaluate);
DODGETRK35_REVERSED("DodgeballSetMessageTrack::Start_675710", 0x00675710, &SetMessage_Start);
DODGETRK35_REVERSED("DodgeballAttackerDistCond::Evaluate_675740", 0x00675740, &AttackerDistCond_Evaluate);
DODGETRK35_REVERSED("DodgeballAttackTimeCond::Evaluate_675810", 0x00675810, &AttackTimeCond_Evaluate);
DODGETRK35_REVERSED("DodgeballCanPickupCond::Evaluate_675870", 0x00675870, &CanPickupCond_Evaluate);
DODGETRK35_REVERSED("CDodgeballPickupBallTrack::Update_675C70", 0x00675C70, &PickupBall_Update);
DODGETRK35_REVERSED("DodgeballAI::TargetPosition_675EE0", 0x00675EE0, &TargetPosition);
DODGETRK35_REVERSED("DodgeballSetPathTrack::Update_676060", 0x00676060, &SetPath_Update);
DODGETRK35_REVERSED("DodgeballCanPathToCond::Evaluate_676100", 0x00676100, &CanPathToCond_Evaluate);
DODGETRK35_REVERSED("DodgeballFollowPathTrack::Update_676210", 0x00676210, &FollowPath_Update);
DODGETRK35_REVERSED("DodgeballGame::RegisterTracks_676510", 0x00676510, &Dodgeball_RegisterTracks);
DODGETRK35_REVERSED("CDodgeballChangePlayerTrack::Start_676DE0", 0x00676DE0, &ChangePlayer_Start);
DODGETRK35_REVERSED("CDodgeballChangePlayerTrack::Update_676F20", 0x00676F20, &ChangePlayer_Update);
DODGETRK35_REVERSED("CDodgeballChangePlayerTrack::End_677120", 0x00677120, &ChangePlayer_End);
DODGETRK35_REVERSED("DodgeballBallCountCond::Evaluate_677190", 0x00677190, &BallCountCond_Evaluate);
DODGETRK35_REVERSED("CDodgeballLocationCollisionTrack::Update_677280", 0x00677280, &LocationCollision_Update);
DODGETRK35_REVERSED("CDodgeballBallLocationCollisionTrack::Update_678450", 0x00678450, &BallLocationCollision_Update);
