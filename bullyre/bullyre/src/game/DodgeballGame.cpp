// The dodgeball minigame's game object (0x67A1E0..0x67DEB0; the instance at *0x1B89150):
// its court (+0x17D4), players, turns and ball. The layout, as used here:
//   +0x00 / +0x14 the two teams (0x14 each): +0 the peds of its 4 players, +0x10 its current
//                 player (0..3)
//   +0x28         9 players (0x28C each; the 9th the referee): +0 the ped, +4 flags (1 in
//                 play, 2 current, 8 out, 0x10, 0x20 / 0x40 changing, 0x80 its turn / has a
//                 ball, 0x1000 / 0x1080 may pick one up; its place << 17), +8 a ducking
//                 float, +0xC.. names (+0xC its act file, +0x8C its action tree, +0xAC its
//                 AI's act file, +0x12C its AI tree, +0x14C, +0x1CC, +0x1EC / +0x20C its
//                 personality / difficulty profile, +0x258), +0x22C / +0x230 those profiles,
//                 +0x234 its old weapon slot, +0x238 its last catch, +0x23C / +0x248 its path
//                 points, +0x254 its old value of ped +0x18C4
//   +0x1714 the court (0xC0 bytes, DodgeballCourt.h), +0x17D4 the court used, +0x17DC the game
//   flags (1 the start's flag, 4 / 8 fading in, 2 the second team's turn, 0x10 paused, 0x20
//   over, 0x40, 0x80 someone out after 3 s, 0x100 started, 0x200 the first team won, 0x400
//   forced pause), +0x17E0 the ball, +0x17E4 the game's tree (controller, context, root),
//   +0x1804 / +0x1810 the tracks' and conditions' registrations (DodgeballTracks.cpp),
//   +0x181C the message, +0x1880 its time, +0x1884 the pause's time, +0x1888, +0x188C the
//   winner (-1 none), +0x1890 the animation group, +0x1894 the dodgeball's weapon index.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/DodgeballCourt.h"
#include "game/Pool.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using dbcourt::At;
using dbcourt::Copy3;

GAME_VAR(uint8_t*, dodgeballGame, 0x01B89150);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, dodgeballTuning_20C8898, 0x020C8898);
GAME_VAR(CPool<uint8_t>*, objectPool_C0F618, 0x00C0F618);
inline uint8_t* const theCamera = reinterpret_cast<uint8_t*>(0x00C3CC68);
inline void* const trackRegistry_1B6565C = reinterpret_cast<void*>(0x01B6565C);
inline void* const conditionRegistry_1B64A94 = reinterpret_cast<void*>(0x01B64A94);
inline void* const playerControl_A45A30 = reinterpret_cast<void*>(0x00A45A30);
inline const char* const dodgeballName = reinterpret_cast<const char*>(0x00904C68);   // "dodgeball"

constexpr int kPlayers = 0x28;
constexpr int kPlayerSize = 0x28C;

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, Empty_49A650, 0x0049A650, void* self);
GAME_FN(void, __thiscall, Registry_Remove, 0x0061A740, void* registry, void* entry);
GAME_FN(void, __thiscall, Camera_SetController, 0x004F11C0, void* camera, int index, void* controller);
GAME_FN(void, __thiscall, Weapon_Reset, 0x004D9410, void* weapon);
GAME_FN(void, __thiscall, Ped_SetHealth, 0x004782E0, uint8_t* ped, float health);
GAME_FN(void, __thiscall, Ped_Set474350, 0x00474350, uint8_t* ped, int value);
GAME_FN(void, __thiscall, Mover_Reset_5F3A40, 0x005F3A40, void* mover);
GAME_FN(void, __thiscall, ActionController_Reset_5F40A0, 0x005F40A0, void* controller);
GAME_FN(float, __cdecl, HeadingFromTo, 0x00433350, const float* from, const float* to);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float heading);
GAME_FN(void, __thiscall, Ped_SetHeading, 0x004734C0, uint8_t* ped, float heading);
GAME_FN(void, __thiscall, Ped_Face_499A60, 0x00499A60, void* self, const float* point, float blend);
GAME_FN(void, __thiscall, PlayerControl_SetEnabled, 0x0050DFA0, void* self, uint32_t enabled);
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(int, __thiscall, Pool_ChainSize, 0x00435BB0, void* next);
GAME_FN(uint8_t*, __thiscall, Pool_ChainAt, 0x00435BD0, void* next, int slot, int);

// ours, through the game's addresses
GAME_FN(void, __thiscall, Dodgeball_SetCurrentFn, 0x0067AFA0, uint8_t* game, int player);
GAME_FN(void, __thiscall, Dodgeball_PlaceEntityFn, 0x0067B2C0, uint8_t* game, uint8_t* entity, const float* position,
        const float* direction);
GAME_FN(int, __thiscall, Dodgeball_FirstInPlayFn, 0x0067B180, uint8_t* game, int team);
GAME_FN(uint8_t*, __thiscall, Dodgeball_PlayerOfFn, 0x0067B210, uint8_t* game, uint8_t* ped);
GAME_FN(void, __thiscall, Dodgeball_UnregisterTracksFn, 0x0067BFB0, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_ResetHealthFn, 0x0067AC90, uint8_t* game, int player);
GAME_FN(float*, __thiscall, Court_InitFn, 0x0067A1E0, void* court);
GAME_FN(float*, __thiscall, Court_PointFn, 0x0067AB50, uint8_t* court, float* out, float across, float along,
        float up);
GAME_FN(void, __thiscall, Dodgeball_TakeBallsFn, 0x0067AC50, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_HealthFractionsFn, 0x0067AE70, uint8_t* game, float* out, int count);
GAME_FN(void, __thiscall, Dodgeball_PlaceAllFn, 0x0067B360, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_ForcePauseFn, 0x0067B620, uint8_t* game, uint32_t paused);
GAME_FN(uint8_t*, __thiscall, Dodgeball_FindBallObjectFn, 0x0067C100, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_StartGameFn, 0x0067C2A0, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_EndGameFn, 0x0067C7F0, uint8_t* game);
GAME_FN(void, __thiscall, DodgeballTree_SetupFn, 0x0067CBF0, uint8_t* tree);
GAME_FN(void, __thiscall, Dodgeball_SpawnBallFn, 0x0067D000, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_ResetFullFn, 0x0067D1C0, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_FindBallFn, 0x0067D560, uint8_t* game);
GAME_FN(void, __thiscall, DodgeballTree_FreeFn, 0x0067DC00, uint8_t* tree);
GAME_FN(void, __thiscall, Dodgeball_RemoveBallsFn, 0x0067DE50, uint8_t* game);
GAME_FN(void, __thiscall, Dodgeball_RegisterTracksFn, 0x00676510, uint8_t* game);
GAME_FN(void*, __thiscall, DodgeballCamera_Construct, 0x00678C90, void* self);

GAME_VAR(uint8_t*, dodgeballSetup_20C88B0, 0x020C88B0);   // +0x18 / +0x1C / +0x20 route names
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, controlledPed_C2B120, 0x00C2B120);
GAME_VAR(float, gameTreeRate_B9D9BC, 0x00B9D9BC);   // 0.02
GAME_VAR(float, timeStep, 0x00C1A9A4);              // CTimer::ms_fTimeStep
GAME_ARRAY(float, zeroVector, 0x00BCC564);
inline void* const animManager_20C4910 = reinterpret_cast<void*>(0x020C4910);
inline void* const personalityRoot_1B6536C = reinterpret_cast<void*>(0x01B6536C);
inline void* const difficultyRoot_1B65370 = reinterpret_cast<void*>(0x01B65370);
inline const uint32_t* const dodgeballHash_1B65364 = reinterpret_cast<const uint32_t*>(0x01B65364);
inline const uint32_t* const boneName_1B6531C = reinterpret_cast<const uint32_t*>(0x01B6531C);
inline const uint32_t* const boneName_1B65394 = reinterpret_cast<const uint32_t*>(0x01B65394);
inline const uint32_t* const boneName_1B65368 = reinterpret_cast<const uint32_t*>(0x01B65368);
inline void* const boneRoot_1B65398 = reinterpret_cast<void*>(0x01B65398);
inline const char* const dodgeballAiName = reinterpret_cast<const char*>(0x0093E630);         // "DodgeballAI"
inline const char* const personalityProfilesName = reinterpret_cast<const char*>(0x0093E61C); // "PersonalityProfiles"
inline const char* const difficultyProfilesName = reinterpret_cast<const char*>(0x0093E608);  // "DifficultyProfiles"
inline const char* const globalPathFormat = reinterpret_cast<const char*>(0x0092DD40);        // "/Global/%s"
inline const char* const aiRootFormat = reinterpret_cast<const char*>(0x0093E5E0);    // "/Global/%s/AIRoot"
inline const char* const dodgeballActFormat = reinterpret_cast<const char*>(0x0093E5F4);   // "Act/Dodgeball/%s"
inline const char* const ambientAnimName = reinterpret_cast<const char*>(0x0093E63C);   // "C_PLAYER\AMBIENT_1"
inline const char* const dbGameActFile = reinterpret_cast<const char*>(0x0093E664);   // "Act/Dodgeball/DB_Game.act"
inline const char* const dbGameRootPath = reinterpret_cast<const char*>(0x0093E650);  // "/Global/DBGame/Root"
inline const char* const dbGamePath = reinterpret_cast<const char*>(0x0093E680);      // "/Global/DBGame"
inline const char* const dodgeballGroupName = reinterpret_cast<const char*>(0x0093E5D4);   // "DodgeBall"

GAME_FN(float*, __cdecl, Route_GetStart, 0x005BFE10, float* out, const char* name);
GAME_FN(void, __cdecl, Fn_45B800, 0x0045B800);
GAME_FN(void, __cdecl, GenerateAllBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __thiscall, Ped_474110, 0x00474110, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_474090, 0x00474090, uint8_t* ped);
GAME_FN(void, __thiscall, Seats_RemoveDriver, 0x004CE960, void* seats);
GAME_FN(void, __thiscall, Ped_SetSpecialRate, 0x00474340, uint8_t* ped, float rate);
GAME_FN(void, __thiscall, Ped_SetStat, 0x004770F0, uint8_t* ped, int stat, int value);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, uint8_t* ped);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(uint32_t*, __thiscall, NameKey_FromHash, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(int32_t*, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, int32_t* path, void* root);
GAME_FN(uint8_t*, __cdecl, ActionTree_Find, 0x005F5B10, const char* path, uint8_t* from);
GAME_FN(uint8_t*, __cdecl, ActFile_Load, 0x005F6DB0, const char* path, uint8_t* parent, void* names);
GAME_FN(void, __cdecl, RemovePath, 0x005F6500, const char* path);
GAME_FN(void, __cdecl, ResolveAll, 0x005F51B0);
GAME_FN(void, __cdecl, CacheNodes, 0x005F37D0);
GAME_FN(void, __thiscall, Owner_SetNode, 0x005F3830, uint8_t* owner, uint8_t* node);
GAME_FN(void*, __thiscall, Ctl_Construct, 0x005F3E50, void* self);
GAME_FN(void, __thiscall, Ctl_Clear, 0x005F4070, void* controller);
GAME_FN(void, __thiscall, Ctl_DropTracks, 0x005F40F0, void* controller);
GAME_FN(void, __thiscall, Ctl_PlayNode, 0x005F45F0, void* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_Update, 0x005F4B00, void* controller, float dt);
GAME_FN(uint8_t*, __thiscall, AnimDict_FindByName, 0x006B1D60, void* dict, uint32_t hash, const char* name,
        int32_t* index);
GAME_FN(uint8_t*, __thiscall, Blend_GetAnim, 0x006B1F50, uint8_t* blend, uint32_t name);
GAME_FN(void, __thiscall, AnimManager_Start, 0x006C0D40, void* manager, uint8_t* blend, uintptr_t index, uint8_t* anim,
        int, float, float);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0E90, void* manager, uint8_t* blend, uint8_t* anim, int,
        float time, int, int);
GAME_FN(void, __thiscall, AnimManager_SetSpeed, 0x006C0E60, void* manager, uint8_t* blend, uint8_t* anim, int,
        float speed);
GAME_FN(void, __thiscall, AnimManager_AddRef, 0x006BD010, void* manager, uint8_t* table, uint32_t group);
GAME_FN(void, __thiscall, AnimManager_Release, 0x006BD030, void* manager, uint8_t* table, uint32_t group, int);
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(void, __cdecl, Entity_CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* reference);
GAME_FN(void, __cdecl, Entity_RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* reference);
GAME_FN(void, __cdecl, ClearDodgeballsAndProjectiles, 0x005C14B0);
GAME_FN(int, __cdecl, GetModelIndexFromHash, 0x0051AF00, uint32_t hash);
GAME_FN(uint8_t*, __cdecl, AddProjectile_x64, 0x004D4D90, uint8_t* creator, int32_t model, re::Float3, re::Float3, uint8_t* info, float, float, uintptr_t, uintptr_t, int32_t, re::Float3, re::Float3, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* AddProjectile(uint8_t* a0, int32_t a1, float a2, float a3, float a4, float a5, float a6, float a7, uint8_t* a8, float a9, float a10, int32_t a11, uintptr_t a12, int32_t a13, uint32_t a14, uint32_t a15, uint32_t a16, uint32_t a17, uint32_t a18, uint32_t a19, int a20, int a21) {
    return AddProjectile_x64(a0, a1, re::Float3{a2, a3, a4}, re::Float3{a5, a6, a7}, a8, a9, a10, a11, a12, a13, re::Float3{re::FloatOf(uint32_t(a14)), re::FloatOf(uint32_t(a15)), re::FloatOf(uint32_t(a16))}, re::Float3{re::FloatOf(uint32_t(a17)), re::FloatOf(uint32_t(a18)), re::FloatOf(uint32_t(a19))}, a20, a21);
}
GAME_FN(void, __cdecl, Projectile_Destroy, 0x004D3F10, int32_t index, int);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, uint8_t* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, uint8_t* entity);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, uint8_t* entity);
GAME_FN(void, __thiscall, Physical_RemoveAndAdd, 0x00469470, uint8_t* physical);
GAME_FN(int, __thiscall, Camera_GetFadeState, 0x004F12F0, void* camera);
GAME_FN(uint8_t, __cdecl, Pad_738900, 0x00738900, int button, int);
GAME_FN(uint8_t, __cdecl, Game_4D9790, 0x004D9790, int32_t model, int);
GAME_FN(int, __cdecl, FindAgrId, 0x0052DAD0, const char* name);
GAME_FN(uint8_t*, __cdecl, GetAgrGroup, 0x0052DB50, int id, uint32_t* index);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, LoadAllRequestedModels, 0x00533EE0, int);

inline uint8_t* Player(uint8_t* game, int i) {
    return game + kPlayers + i * kPlayerSize;
}
inline uint8_t* PlayerPed(uint8_t* game, int i) {
    return At<uint8_t*>(Player(game, i), 0);
}
inline uint32_t& PlayerFlags(uint8_t* game, int i) {
    return At<uint32_t>(Player(game, i), 4);
}
inline float* Pos(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}

// 0x67AB50 (thiscall, the court): a point of the court: its centre plus the across axis
// times `across` of its half extent and the along axis times `along`, `up` higher
float* __fastcall Court_Point(uint8_t* court, re::Edx, float* out, float across, float along, float up) {
    float w[3];
    dbcourt::VectorSub(w, &At<float>(court, 0xC), reinterpret_cast<float*>(court));
    float* u = &At<float>(court, 0xA8);
    float* v = &At<float>(court, 0xB4);
    float dv = (v[0] * w[0] + v[1] * w[1]) + v[2] * w[2];
    float sv = dv * along;
    float a[3] = {v[0] * sv, v[1] * sv, sv * v[2]};
    float du = (w[0] * u[0] + w[1] * u[1]) + w[2] * u[2];
    float su = -du * across;
    float b[3] = {u[0] * su, u[1] * su, su * u[2]};
    float tmp[3];
    dbcourt::VectorAdd(out, dbcourt::VectorAdd(tmp, &At<float>(court, 0x9C), b), a);
    out[2] = up + out[2];
    return out;
}

// 0x67AC50 (thiscall): every player's ball taken away (flag 0x10 cleared)
void __fastcall Dodgeball_TakeBalls(uint8_t* game, re::Edx) {
    for (int i = 0; i < 9; i++) {
        uint8_t* weapon = At<uint8_t*>(PlayerPed(game, i), 0x1D0);
        if (At<int16_t>(weapon, 0x10E) != -1) {
            Weapon_Reset(weapon);
            PlayerFlags(game, i) &= ~0x10u;
        }
    }
}

// 0x67AC90 (thiscall): a player's health set to its share of its maximum (the tuning's
// +0x38.. by its place in the team); none left: out of play
void __fastcall Dodgeball_ResetHealth(uint8_t* game, re::Edx, int i) {
    float share = 1.0f;
    int team = i >= 4;
    uint32_t local = uint32_t(i - team * 4);
    if (local <= 3) {
        share = At<float>(dodgeballTuning_20C8898, 0x38 + int(local) * 4);
    }
    uint8_t* player = Player(game, i);
    float health = At<float>(At<uint8_t*>(player, 0), 0x1CA4) * share;
    Ped_SetHealth(At<uint8_t*>(player, 0), health);
    if (!(0.0f < health)) {
        At<uint32_t>(player, 4) &= ~1u;
    }
}

// 0x67AD60 (thiscall): a player set up from a description (team, index, ped, 9 names)
void __fastcall Dodgeball_SetupPlayer(uint8_t* game, re::Edx, const uint32_t* description) {
    int team = int(description[0]);
    uint8_t* player = Player(game, int(description[1]) + team * 4);
    At<uint32_t>(player, 0) = description[2];
    static constexpr int kNames[] = {0x8C, 0xC, 0xAC, 0x12C, 0x14C, 0x1CC, 0x1EC, 0x20C, 0x258};
    static constexpr int kSources[] = {4, 3, 5, 6, 7, 8, 9, 10, 11};
    for (int n = 0; n < 9; n++) {
        std::strcpy(reinterpret_cast<char*>(player + kNames[n]), reinterpret_cast<const char*>(description[kSources[n]]));
    }
    if (team < 2) {
        At<uint32_t>(game, (team * 5 + int(description[1])) * 4) = description[2];
    }
}

// 0x67AE70 (thiscall): the players' health above the tuning's knock-out share, as
// fractions of what's left above it (at least 0)
void __fastcall Dodgeball_HealthFractions(uint8_t* game, re::Edx, float* out, int count) {
    float share = At<float>(dodgeballTuning_20C8898, 0x1C);
    for (int i = 0; i < count; i++) {
        out[i] = 0.0f;
        uint8_t* ped = PlayerPed(game, i);
        if (ped) {
            float knockOut = At<float>(ped, 0x1CA4) * share;
            float rest = At<float>(ped, 0x1CA4) - knockOut;
            out[i] = (At<float>(ped, 0x1CB8) - rest) / knockOut;
        }
        if (0.0f > out[i]) {
            out[i] = 0.0f;
        }
    }
}

// 0x67AF00 (thiscall): a team's current player's ped's +0x18C0
float __fastcall Dodgeball_CurrentPlayerFloat(uint8_t* game, re::Edx, int team) {
    int current = At<int32_t>(game, team * 0x14 + 0x10);
    return At<float>(At<uint8_t*>(game, (current + team * 5) * 4), 0x18C0);
}

// 0x67AF20 (thiscall): a ped's player index (none: -1)
int __fastcall Dodgeball_PlayerIndex(uint8_t* game, re::Edx, uint8_t* ped) {
    for (int i = 0; i < 9; i++) {
        if (PlayerPed(game, i) == ped) {
            return i;
        }
    }
    return -1;
}

// 0x67AF50 (thiscall): a team's player made the one changed to (0x20; the flag argument
// unused); its flags' address
uint32_t* __fastcall Dodgeball_ChangePlayer(uint8_t* game, re::Edx, int from, int to, int) {
    int first = (from >= 4) * 4;
    for (int i = first; i < first + 4; i++) {
        PlayerFlags(game, i) &= ~0x20u;
    }
    PlayerFlags(game, to) |= 0x20;
    return &PlayerFlags(game, to);
}

// 0x67AFA0 (thiscall): a player made its team's current one (taking over the old one's
// ped +0x18C0 as +0x474350's value, the old one's reset to 100; flag 2 and the turn moved)
void __fastcall Dodgeball_SetCurrent(uint8_t* game, re::Edx, int i) {
    int team = i >= 4;
    int32_t* current = &At<int32_t>(game, team * 0x14 + 0x10);
    int base = team * 4;
    uint8_t* old = Player(game, *current + base);
    uint8_t* now = Player(game, i);
    int value = int(At<float>(At<uint8_t*>(old, 0), 0x18C0));
    Ped_Set474350(At<uint8_t*>(now, 0), value);
    Ped_Set474350(At<uint8_t*>(old, 0), 100);
    At<uint32_t>(old, 4) &= ~2u;
    Mover_Reset_5F3A40(At<void*>(At<uint8_t*>(old, 0), 0x2E0));
    At<uint32_t>(now, 4) |= 2;
    uint32_t flags = At<uint32_t>(now, 4);
    At<uint32_t>(now, 4) = ((At<uint32_t>(old, 4) ^ flags) & 0x80) ^ flags;
    *current = i - base;
}

// 0x67B040 (thiscall)
uint8_t* __fastcall Dodgeball_PlayerPed(uint8_t* game, re::Edx, int i) {
    return PlayerPed(game, i);
}

// 0x67B060 / 0x67B080 (thiscall): the current player (as a player index) of a player's team
// / of the other team
int __fastcall Dodgeball_TeamCurrent(uint8_t* game, re::Edx, int i) {
    int team = i >= 4;
    return At<int32_t>(game, team * 0x14 + 0x10) + team * 4;
}
int __fastcall Dodgeball_OtherTeamCurrent(uint8_t* game, re::Edx, int i) {
    int team = 1 - (i >= 4);
    return At<int32_t>(game, team * 0x14 + 0x10) + team * 4;
}

// 0x67B0A0 / 0x67B0C0 (thiscall): a player's flags
uint32_t* __fastcall Dodgeball_GetPlayerFlags(uint8_t* game, re::Edx, uint32_t* out, int i) {
    *out = PlayerFlags(game, i);
    return out;
}
void __fastcall Dodgeball_SetPlayerFlags(uint8_t* game, re::Edx, int i, uint32_t flags) {
    PlayerFlags(game, i) = flags;
}

// 0x67B0E0 / 0x67B0F0 (thiscall): the game flags
uint32_t* __fastcall Dodgeball_GetFlags(uint8_t* game, re::Edx, uint32_t* out) {
    *out = At<uint32_t>(game, 0x17DC);
    return out;
}
void __fastcall Dodgeball_SetFlags(uint8_t* game, re::Edx, uint32_t flags) {
    At<uint32_t>(game, 0x17DC) = flags;
}

// 0x67B100 (thiscall): two peds on the same team (either not a player: false)
uint8_t __fastcall Dodgeball_SameTeam(uint8_t* game, re::Edx, uint8_t* a, uint8_t* b) {
    int ia = -1;
    int ib = -1;
    for (int i = 0; i < 9; i++) {
        if (PlayerPed(game, i) == a) {
            ia = i;
            break;
        }
    }
    for (int i = 0; i < 9; i++) {
        if (PlayerPed(game, i) == b) {
            ib = i;
            break;
        }
    }
    if (ia == -1 || ib == -1) {
        return 0;
    }
    return (ia >= 4) == (ib >= 4);
}

// 0x67B180 (thiscall): a player's team's first player in play (-1: none)
int __fastcall Dodgeball_FirstInPlay(uint8_t* game, re::Edx, int i) {
    int team = i >= 4;
    for (int local = 0; local < 4; local++) {
        if (PlayerFlags(game, team * 4 + local) & 1) {
            return local + team * 4;
        }
    }
    return -1;
}

// 0x67B1C0 (thiscall): a player is its team's first in play
uint8_t __fastcall Dodgeball_IsFirstInPlay(uint8_t* game, re::Edx, int i) {
    return Dodgeball_FirstInPlayFn(game, i) == i;
}

// 0x67B1E0 (thiscall)
uint32_t __fastcall Dodgeball_Get188C(uint8_t* game, re::Edx) {
    return At<uint32_t>(game, 0x188C);
}

// 0x67B1F0 (thiscall): a player's record
uint8_t* __fastcall Dodgeball_Player(uint8_t* game, re::Edx, int i) {
    return Player(game, i);
}

// 0x67B210 (thiscall): a ped's player record (not a player: the one before the first)
uint8_t* __fastcall Dodgeball_PlayerOf(uint8_t* game, re::Edx, uint8_t* ped) {
    int index = -1;
    for (int i = 0; i < 9; i++) {
        if (PlayerPed(game, i) == ped) {
            index = i;
            break;
        }
    }
    return Player(game, index);
}

// 0x67B250 (thiscall): game flag 0x100 (in al; eax's rest the flags' shifted bits)
uint32_t __fastcall Dodgeball_Flag100(uint8_t* game, re::Edx) {
    uint32_t v = At<uint32_t>(game, 0x17DC) >> 8;
    return (v & 0xFFFFFF00u) | (v & 1);
}

// 0x67B260 (thiscall): a ped's player's +8 (not a player: 0)
float __fastcall Dodgeball_PlayerDuck(uint8_t* game, re::Edx, uint8_t* ped) {
    for (int i = 0; i < 9; i++) {
        if (PlayerPed(game, i) == ped) {
            return At<float>(Dodgeball_PlayerOfFn(game, ped), 8);
        }
    }
    return 0.0f;
}

// 0x67B2A0 (thiscall): the message set
void __fastcall Dodgeball_SetMessage(uint8_t* game, re::Edx, const char* text) {
    std::strcpy(reinterpret_cast<char*>(game + 0x181C), text);
}

// 0x67B2C0 (thiscall, the game unused): an entity placed at a point facing a direction
// (its action controllers reset)
void __fastcall Dodgeball_PlaceEntity(uint8_t*, re::Edx, uint8_t* entity, const float* position, const float* direction) {
    float ahead[3];
    dbcourt::VectorAdd(ahead, position, direction);
    vc::Call<void>(entity, 0x38, position[0], position[1], position[2]);
    float heading = HeadingFromTo(position, ahead);
    if (At<uint8_t*>(entity, 0x14)) {
        Matrix_SetRotateZOnly(At<uint8_t*>(entity, 0x14), heading);
    }
    At<float>(entity, 0x10) = heading;
    Ped_SetHeading(entity, heading);
    ActionController_Reset_5F40A0(At<uint8_t*>(entity, 0x2E0) + 0x290);
    ActionController_Reset_5F40A0(At<uint8_t*>(entity, 0x2E0) + 0x258);
}

// 0x67B360 (thiscall): everyone placed at the court's start points (the first team facing
// across, the second back, the referee along, turned to face the court)
void __fastcall Dodgeball_PlaceAll(uint8_t* game, re::Edx) {
    for (int i = 0; i < 4; i++) {
        uint8_t* court = At<uint8_t*>(game, 0x17D4);
        Dodgeball_PlaceEntityFn(game, At<uint8_t*>(game, i * 4), &At<float>(court, 0x30 + i * 0xC), &At<float>(court, 0xA8));
    }
    for (int i = 0; i < 4; i++) {
        uint8_t* court = At<uint8_t*>(game, 0x17D4);
        float back[3] = {-At<float>(court, 0xA8), -At<float>(court, 0xAC), -At<float>(court, 0xB0)};
        Dodgeball_PlaceEntityFn(game, At<uint8_t*>(game, 0x14 + i * 4), &At<float>(court, 0x60 + i * 0xC), back);
    }
    uint8_t* court = At<uint8_t*>(game, 0x17D4);
    Dodgeball_PlaceEntityFn(game, At<uint8_t*>(game, 0x1488), &At<float>(court, 0x90), &At<float>(court, 0xB4));
    uint8_t* referee = At<uint8_t*>(game, 0x1488);
    if (!referee) {
        return;
    }
    Ped_Face_499A60(referee + 0x358, &At<float>(At<uint8_t*>(game, 0x17D4), 0x90), 1.0f);
    Copy3(&At<float>(At<uint8_t*>(At<uint8_t*>(game, 0x1488), 0x2E0), 0xC), &At<float>(At<uint8_t*>(game, 0x17D4), 0xB4));
}

// 0x67B480 (thiscall): the times since the pause and since +0x1888 (0 when unset); the
// second argument unused
uint32_t* __fastcall Dodgeball_Times(uint8_t* game, re::Edx, uint32_t* out, int) {
    out[0] = timeInMs - At<uint32_t>(game, 0x1884);
    uint32_t since = At<uint32_t>(game, 0x1888);
    out[1] = since ? timeInMs - since : since;
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// 0x67B4B0 (thiscall): the first players' positions
void __fastcall Dodgeball_PlayerPositions(uint8_t* game, re::Edx, float* out, int count) {
    for (int i = 0; i < count; i++) {
        Copy3(out + i * 3, Pos(PlayerPed(game, i)));
    }
}

// 0x67B500 (thiscall): a ped's team code (in al: first team 3 in play, 1 not; second team 2
// in play, 4 not; eax's rest from the index arithmetic, as the game's)
uint32_t __fastcall Dodgeball_TeamCode(uint8_t* game, re::Edx, uint8_t* ped) {
    if (!ped) {
        return reinterpret_cast<uint32_t>(ped) & 0xFFFFFF00u;
    }
    int index = -1;
    for (int i = 0; i < 9; i++) {
        if (PlayerPed(game, i) == ped) {
            index = i;
            break;
        }
    }
    uint32_t offset = uint32_t(index * kPlayerSize);
    uint8_t flags = *(game + kPlayers + 4 + int32_t(offset));
    if (index < 4) {
        return (offset & 0xFFFFFF00u) | uint32_t(((flags & 1) * 2) | 1);
    }
    uint32_t eax = (offset & 0xFFFFFF00u) | uint32_t((flags & 1) == 0);
    return eax * 2 + 2;
}

// 0x67B560 (thiscall): a change of player done: the team's changing flags cleared; made
// current (flag 2 moved) or marked swapped in (0x40)
void __fastcall Dodgeball_SwapPlayers(uint8_t* game, re::Edx, int from, int to, int swap) {
    int first = (from >= 4) * 4;
    for (int i = first; i < first + 4; i++) {
        PlayerFlags(game, i) &= ~0x60u;
    }
    if (re::Flag(swap)) {
        PlayerFlags(game, to) |= 0x40;
        return;
    }
    PlayerFlags(game, from) &= ~2u;
    PlayerFlags(game, to) |= 2;
    Dodgeball_SetCurrentFn(game, to);
}

// 0x67B5F0 (thiscall): paused (flag 0x10) or not (the pause's time kept when not)
void __fastcall Dodgeball_SetPaused(uint8_t* game, re::Edx, int paused) {
    uint8_t p = uint8_t(paused);
    At<uint32_t>(game, 0x17DC) ^= ((uint32_t(p) << 4) ^ At<uint32_t>(game, 0x17DC)) & 0x10;
    if (!p) {
        At<uint32_t>(game, 0x1884) = timeInMs;
    }
}

// 0x67B620 (thiscall): paused (flags 0x10 and 0x400) or not, the player's control and the
// players' peds' +0x1358 / +0x1344 with it
void __fastcall Dodgeball_ForcePause(uint8_t* game, re::Edx, uint32_t paused) {
    uint32_t bit = paused & 1;
    At<uint32_t>(game, 0x17DC) = ((bit << 0xA) | (At<uint32_t>(game, 0x17DC) & 0xFFFFFBFFu)) & 0xFFFFFFEFu | (bit << 4);
    if (!uint8_t(paused)) {
        At<uint32_t>(game, 0x1884) = timeInMs;
    }
    // (the byte goes as the argument with its low byte replaced)
    PlayerControl_SetEnabled(playerControl_A45A30, (paused & 0xFFFFFF00u) | uint32_t(uint8_t(paused) == 0));
    uint32_t value = uint8_t(paused) ? 1 : 0;
    for (int i = 0; i < 8; i++) {
        At<uint32_t>(PlayerPed(game, i), 0x1358) = value;
        At<uint32_t>(PlayerPed(game, i), 0x1344) = value;
    }
}

// 0x67B800 (thiscall): a point kept on the court (pushed back through each edge it's
// past); arguments 1 and 3 unused
void __fastcall Dodgeball_ClampToCourt(uint8_t*, re::Edx, int, float* position, int) {
    dbcourt::Frame frame = dbcourt::MakeFrame(At<uint8_t*>(dodgeballGame, 0x17D4));
    float corners[4][3];
    dbcourt::Corner(frame, -1.0f, -1.0f, corners[0]);
    dbcourt::Corner(frame, 1.0f, -1.0f, corners[1]);
    dbcourt::Corner(frame, 1.0f, 1.0f, corners[2]);
    dbcourt::Corner(frame, -1.0f, 1.0f, corners[3]);
    float planes[4][4] = {};
    for (int i = 0; i < 4; i++) {
        dbcourt::EdgePlane(corners[i], corners[(i + 1) % 4], false, planes[i]);
    }
    float q[3] = {position[0], position[1], position[2]};
    dbcourt::PushInside(planes, q);
    Copy3(position, q);
}

// 0x67BFB0 (thiscall): the tracks and conditions unregistered and freed
void __fastcall Dodgeball_UnregisterTracks(uint8_t* game, re::Edx) {
    const struct {
        int array;
        void* registry;
    } kArrays[] = {{0x1804, trackRegistry_1B6565C}, {0x1810, conditionRegistry_1B64A94}};
    for (const auto& a : kArrays) {
        for (uint32_t i = 0; i < At<uint32_t>(game, a.array + 8); i++) {
            Registry_Remove(a.registry, At<re::Ptr32<uint8_t>*>(game, a.array)[i]);
            uint8_t* entry = At<re::Ptr32<uint8_t>*>(game, a.array)[i];
            if (entry) {
                Empty_49A650(entry);
                Free(entry);
            }
        }
        if (At<void*>(game, a.array)) {
            Free(At<void*>(game, a.array));
            At<void*>(game, a.array) = nullptr;
        }
        if (a.array == 0x1804) {
            At<uint32_t>(game, a.array + 4) = 0;
            At<uint32_t>(game, a.array + 8) = 0;
        } else {
            At<uint32_t>(game, a.array + 8) = 0;
            At<uint32_t>(game, a.array + 4) = 0;
        }
    }
}

// 0x67C0A0 (thiscall): shut down: unregistered, the camera controller removed
void __fastcall Dodgeball_Shutdown(uint8_t* game, re::Edx) {
    Dodgeball_UnregisterTracksFn(game);
    Camera_SetController(theCamera, 0xB, nullptr);
    if (At<void*>(game, 0x1810)) {
        Free(At<void*>(game, 0x1810));
        At<void*>(game, 0x1810) = nullptr;
    }
    if (At<void*>(game, 0x1804)) {
        Free(At<void*>(game, 0x1804));
        At<void*>(game, 0x1804) = nullptr;
    }
}

// 0x67C100 (thiscall): the ball: a player's held one (kept as the ball), else the dodgeball
// object in play nearest the court's centre (none when more than one)
uint8_t* __fastcall Dodgeball_FindBallObject(uint8_t* game, re::Edx) {
    for (int i = 0; i < 9; i++) {
        if (At<int16_t>(At<uint8_t*>(PlayerPed(game, i), 0x1D0), 0x10E) != -1) {
            At<uint8_t*>(game, 0x17E0) = PlayerPed(game, i);
            return PlayerPed(game, i);
        }
    }
    At<int32_t>(game, 0x1894) = GetWeaponIndex(dodgeballName);
    CPool<uint8_t>* pool = objectPool_C0F618;
    int total = pool->size;
    if (pool->unknown10) {
        total += Pool_ChainSize(reinterpret_cast<void*>(pool->unknown10));
    }
    float* c = &At<float>(At<uint8_t*>(game, 0x17D4), 0x9C);
    float centre[3] = {c[0], c[1], c[2]};
    float best = 1.0000000200408773e+21f;
    uint8_t* found = nullptr;
    int count = 0;
    for (int slot = total; slot;) {
        slot--;
        uint8_t* object = nullptr;
        if (slot >= pool->size) {
            if (pool->unknown10) {
                object = Pool_ChainAt(reinterpret_cast<void*>(pool->unknown10), slot - pool->size, 1);
            }
        } else {
            object = re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * slot;
        }
        if ((pool->flags[slot] & 0x80) || !object || !At<uint32_t>(object, 0xC4) ||
            At<int32_t>(game, 0x1894) != At<int16_t>(object, 0x10E)) {
            continue;
        }
        count++;
        float d[3];
        dbcourt::VectorSub(d, Pos(object), centre);
        float sum = (d[1] * d[1] + d[0] * d[0]) + d[2] * d[2];
        if (best > sum) {
            best = sum;
            found = object;
        }
    }
    if (total && count > 1) {
        return nullptr;
    }
    return found;
}

// ---- the court (0xC0 bytes at game +0x1714)

inline float* Scale3(float* out, const float* v, float k) {
    out[0] = v[0] * k;
    out[1] = v[1] * k;
    out[2] = v[2] * k;
    return out;
}

// 0x67A1E0 (thiscall, the court): built from the dodgeball setup's three routes' first
// points (+0x18 A and +0x1C B along one side line, +0x20 C across): the along axis A - B
// (normalised), the across axis -(along x up); corners A pushed twice across, B, and those
// two pushed by (across + along) times the setup's +0x24 (the first forward, the second
// back); the centre half way between A and B pushed across; the referee off the side by
// the setup's +0x24 plus the half width plus 2; the corners' heights the centre's; the
// players' start points about the centre (0.1 up) by the setup's +0x28 / +0x2C / +0x30
float* __fastcall Court_Init(float* court, re::Edx) {
    for (int i = 0; i < 0x30; i++) {
        court[i] = 0.0f;
    }
    float* centre = court + 0x9C / 4;
    float* u = court + 0xA8 / 4;
    float* v = court + 0xB4 / 4;
    uint8_t* setup = dodgeballSetup_20C88B0;
    float a[3];
    float b[3];
    float c[3];
    Route_GetStart(a, At<const char*>(setup, 0x18));
    Route_GetStart(b, At<const char*>(setup, 0x1C));
    Route_GetStart(c, At<const char*>(setup, 0x20));
    float tmp[3];
    Copy3(v, dbcourt::VectorSub(tmp, a, b));
    dbcourt::Vector_Normalise(v);
    float up[3] = {0.0f, 0.0f, 1.0f};
    dbcourt::CrossProduct(tmp, v, up);
    float negated[3] = {-tmp[0], -tmp[1], -tmp[2]};
    Copy3(u, negated);
    float d[3];
    dbcourt::VectorSub(d, c, b);
    float du = (u[0] * d[0] + u[1] * d[1]) + u[2] * d[2];
    float dv = (d[0] * v[0] + d[1] * v[1]) + d[2] * v[2];
    float across[3];
    Scale3(across, u, du);
    float twice[3];
    Scale3(twice, across, 2.0f);
    Copy3(court, dbcourt::VectorAdd(tmp, a, twice));
    Copy3(court + 3, b);
    float side = At<float>(setup, 0x24);
    float diagonal[3];
    float step[3];
    Scale3(step, dbcourt::VectorAdd(diagonal, u, v), side);
    Copy3(court + 6, dbcourt::VectorAdd(tmp, court, step));
    side = At<float>(setup, 0x24);
    Scale3(step, dbcourt::VectorAdd(diagonal, u, v), side);
    Copy3(court + 9, dbcourt::VectorSub(tmp, court + 3, step));
    float half[3];
    Scale3(half, dbcourt::VectorAdd(tmp, a, b), 0.5f);
    Copy3(centre, dbcourt::VectorAdd(tmp, half, across));
    float out = (At<float>(setup, 0x24) + dv) + 2.0f;
    float off[3];
    Scale3(off, v, out);
    float lift[3] = {0.0f, 0.0f, 0.1f};
    float raised[3];
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x90 / 4, dbcourt::VectorSub(tmp, raised, off));
    court[2] = court[0xA4 / 4];
    court[5] = court[0xA4 / 4];
    court[8] = court[0xA4 / 4];
    court[11] = court[0xA4 / 4];

    // the start points: the raised centre plus / minus the across and along offsets
    float ua[3];
    float va[3];
    float r1[3];
    float r2[3];
    float t28 = At<float>(setup, 0x28);
    Scale3(ua, Scale3(r1, u, t28), 1.5f);
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x30 / 4, dbcourt::VectorSub(tmp, raised, ua));

    Scale3(va, v, At<float>(setup, 0x30));
    Scale3(ua, u, At<float>(setup, 0x2C) + At<float>(setup, 0x28));
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x3C / 4, dbcourt::VectorAdd(tmp, dbcourt::VectorSub(r1, raised, ua), va));

    Scale3(va, v, At<float>(setup, 0x30));
    Scale3(ua, u, At<float>(setup, 0x2C) + At<float>(setup, 0x28));
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x48 / 4, dbcourt::VectorSub(tmp, dbcourt::VectorSub(r1, raised, ua), va));

    Scale3(ua, u, At<float>(setup, 0x28) * 3.0f + At<float>(setup, 0x2C));
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x54 / 4, dbcourt::VectorAdd(tmp, raised, ua));

    Scale3(ua, u, At<float>(setup, 0x28));
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x60 / 4, dbcourt::VectorAdd(tmp, raised, ua));

    Scale3(va, v, At<float>(setup, 0x30));
    Scale3(ua, u, At<float>(setup, 0x2C) + At<float>(setup, 0x28));
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x6C / 4, dbcourt::VectorAdd(tmp, dbcourt::VectorAdd(r2, raised, ua), va));

    Scale3(va, v, At<float>(setup, 0x30));
    Scale3(ua, u, At<float>(setup, 0x2C) + At<float>(setup, 0x28));
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x78 / 4, dbcourt::VectorSub(tmp, dbcourt::VectorAdd(r2, raised, ua), va));

    Scale3(ua, u, At<float>(setup, 0x28) * 3.0f + At<float>(setup, 0x2C));
    dbcourt::VectorAdd(raised, lift, centre);
    Copy3(court + 0x84 / 4, dbcourt::VectorSub(tmp, raised, ua));
    return court;
}

// ---- starting and ending

// 0x67C2A0 (thiscall): the game started: every ped's vehicle given up (+0x2E4 cleared,
// +0x6B8 |= 2, its driver seat emptied, the ped's speeds zeroed); each player set up: its
// ped +0x16DC = 3, its flags (1 in play, 4 the first team, its place << 17, 0x1000; the
// bits 19.. from an uninitialised register, which held the players' address), its old
// ped +0x18C4 kept, health reset, its weapon's index kept (the weapon then reset), its AI
// profiles found (DodgeballAI/PersonalityProfiles/<+0x1EC> and DifficultyProfiles/
// <+0x20C>) and its ped's stats 0x14 / 10 from the difficulty's +0x18 / +0x1C (percent),
// 5 cleared, its ped flagged for the game; the first players' turns (0x80 / 2), the
// referee (9); every player's action trees loaded (when not yet) and started
void __fastcall Dodgeball_StartGame(uint8_t* game, re::Edx) {
    Fn_45B800();
    CPool<uint8_t>* peds = pedPool;
    int count = peds->size;
    for (int slot = 0; slot < count; slot++) {
        if (peds->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = re::PtrCast<uint8_t*>(peds->objects) + peds->elemSize * slot;
        if (!ped) {
            continue;
        }
        Ped_474110(ped);
        uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
        if (!vehicle) {
            continue;
        }
        At<uint8_t>(vehicle, 0x2E4) = 0;
        At<uint8_t>(At<uint8_t*>(ped, 0x1554), 0x6B8) |= 2;
        Seats_RemoveDriver(At<uint8_t*>(ped, 0x1554) + 0x1C4);
        for (int k = 0; k < 6; k++) {
            At<float>(ped, 0x124 + k * 4) = 0.0f;
        }
    }
    // (the flags' upper bits: the register's value, the players' address)
    uint32_t leftover = (uint32_t(reinterpret_cast<uintptr_t>(game + kPlayers)) & 0xFFF81005u) | 0x1005u;
    for (int i = 0; i < 9; i++) {
        uint8_t* player = Player(game, i);
        uint8_t* ped = At<uint8_t*>(player, 0);
        At<uint32_t>(ped, 0x16DC) = 3;
        At<uint32_t>(player, 4) = ((uint32_t(i < 4) | (uint32_t(i & 3) << 0xF)) * 4) | (leftover & 0xFFF9FFFBu) | 1;
        Ped_Set474350(At<uint8_t*>(player, 0), 0);
        At<float>(player, 0x254) = At<float>(At<uint8_t*>(player, 0), 0x18C4);
        Ped_SetSpecialRate(At<uint8_t*>(player, 0), 0.0f);
        Dodgeball_ResetHealthFn(game, i);
        At<float>(player, 8) = 0.0f;
        ped = At<uint8_t*>(player, 0);
        At<int32_t>(player, 0x234) = -1;
        At<int32_t>(player, 0x234) = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
        Weapon_Reset(At<uint8_t*>(ped, 0x1D0));
        uint32_t personality[3];
        NameKey_Construct(&personality[0], dodgeballAiName);
        NameKey_Construct(&personality[1], personalityProfilesName);
        NameKey_Construct(&personality[2], reinterpret_cast<const char*>(player + 0x1EC));
        int32_t personalityPath[3];
        HashPath_Set(personalityPath, 3, personality);
        uint32_t difficulty[3];
        NameKey_Construct(&difficulty[0], dodgeballAiName);
        NameKey_Construct(&difficulty[1], difficultyProfilesName);
        NameKey_Construct(&difficulty[2], reinterpret_cast<const char*>(player + 0x20C));
        int32_t difficultyPath[3];
        HashPath_Set(difficultyPath, 3, difficulty);
        At<uint8_t*>(player, 0x22C) = FindBoneInfo(personalityPath, personalityRoot_1B6536C);
        uint8_t* level = FindBoneInfo(difficultyPath, difficultyRoot_1B65370);
        At<uint8_t*>(player, 0x230) = level;
        float skill = At<float>(level, 0x18);
        Ped_SetStat(At<uint8_t*>(player, 0), 0x14, int(skill * 100.0f));
        skill = At<float>(At<uint8_t*>(player, 0x230), 0x1C);
        Ped_SetStat(At<uint8_t*>(player, 0), 10, int(skill * 100.0f));
        Ped_SetStat(At<uint8_t*>(player, 0), 5, 0);
        At<uint32_t>(player, 0x238) = 0;
        At<uint32_t>(At<uint8_t*>(player, 0), 0x13E4) = 1;
        At<uint32_t>(At<uint8_t*>(player, 0), 0x1328) = 1;
        At<uint32_t>(At<uint8_t*>(player, 0), 0xA0) = 0;
        At<uint32_t>(At<uint8_t*>(player, 0), 0xE4) = 1;
        At<uint32_t>(At<uint8_t*>(player, 0), 0x1310) = 0xD;
        At<uint32_t>(At<uint8_t*>(player, 0), 0x14D0) = 1;
        At<float>(At<uint8_t*>(player, 0), 0x1CA8) = 1.0f;
        for (int k = 2; k >= 0; k--) {
            Empty_49A650(&difficulty[k]);
        }
        for (int k = 2; k >= 0; k--) {
            Empty_49A650(&personality[k]);
        }
    }
    PlayerFlags(game, 0) |= 0x82;
    PlayerFlags(game, 4) |= 2;
    PlayerFlags(game, 8) |= 9;
    At<uint32_t>(PlayerPed(game, 8), 0x1328) = 1;
    At<uint32_t>(PlayerPed(game, 8), 0xA0) = 0;
    At<uint32_t>(PlayerPed(game, 8), 0xE4) = 1;
    At<uint32_t>(PlayerPed(game, 8), 0x14D0) = 1;
    At<float>(PlayerPed(game, 8), 0x1CA8) = 1.0f;
    char path[260];
    for (int i = 0; i < 9; i++) {
        uint8_t* player = Player(game, i);
        std::sprintf(path, globalPathFormat, player + 0x8C);
        if (!ActionTree_Find(path, nullptr)) {
            std::sprintf(path, dodgeballActFormat, player + 0xC);
            ActFile_Load(path, nullptr, nullptr);
        }
        std::sprintf(path, aiRootFormat, player + 0x12C);
        if (!ActionTree_Find(path, nullptr)) {
            std::sprintf(path, dodgeballActFormat, player + 0xAC);
            ActFile_Load(path, nullptr, nullptr);
        }
    }
    ResolveAll();
    for (int i = 0; i < 9; i++) {
        uint8_t* player = Player(game, i);
        std::sprintf(path, globalPathFormat, player + 0x8C);
        uint8_t* node = ActionTree_Find(path, nullptr);
        At<uint8_t*>(At<uint8_t*>(PlayerPed(game, i), 0x2E0), 0x4B0) = node;
        Owner_SetNode(At<uint8_t*>(PlayerPed(game, i), 0x2E0) + 0x300, node);
        std::sprintf(path, aiRootFormat, player + 0x12C);
        node = ActionTree_Find(path, nullptr);
        At<uint8_t*>(At<uint8_t*>(PlayerPed(game, i), 0x2E0), 0x4B4) = node;
        Owner_SetNode(At<uint8_t*>(PlayerPed(game, i), 0x2E0) + 0x358, node);
        ActionController_Reset_5F40A0(At<uint8_t*>(PlayerPed(game, i), 0x2E0) + 0x258);
        ActionController_Reset_5F40A0(At<uint8_t*>(PlayerPed(game, i), 0x2E0) + 0x290);
    }
    GenerateAllBoundingBoxes(0);
}

// a ped's model's own trees loaded when not yet (+0x64 its tree, +0x68 its file, +0x6C
// its AI tree, +0x70 that one's file)
void LoadModelTrees(uint8_t* ped) {
    uint8_t* info = Ped_GetModelInfo(ped);
    if (!info) {
        return;
    }
    const char* file = At<const char*>(info, 0x68);
    const char* aiTree = At<const char*>(info, 0x6C);
    const char* aiFile = At<const char*>(info, 0x70);
    if (!ActionTree_Find(At<const char*>(info, 0x64), nullptr)) {
        ActFile_Load(file, nullptr, nullptr);
    }
    if (!ActionTree_Find(aiTree, nullptr)) {
        ActFile_Load(aiFile, nullptr, nullptr);
    }
}

// 0x67C7F0 (thiscall): the game ended: each player's controllers reset, its ped's game
// flags cleared, health back to its maximum, its weapon slot (+0x2E0 +0x18) and stat 10
// (when the game had set +0x1310 to 0xD: 100) restored, its old +0x18C4 back; peds with an
// animation dictionary play C_PLAYER\AMBIENT_1 (from a random time 0..0.3, at a speed
// 0.9..1.1); the players' trees unloaded; the player's and every other ped's own trees
// loaded (when not yet), the nodes cached, the peds' 0x474090 called, the players' nodes
// back to their models' trees; the controlled ped the player's again
void __fastcall Dodgeball_EndGame(uint8_t* game, re::Edx) {
    Fn_45B800();
    CPool<uint8_t>* peds = pedPool;
    int count = peds->size;
    for (int i = 0; i < 9; i++) {
        uint8_t* player = Player(game, i);
        ActionController_Reset_5F40A0(At<uint8_t*>(At<uint8_t*>(player, 0), 0x2E0) + 0x258);
        ActionController_Reset_5F40A0(At<uint8_t*>(At<uint8_t*>(player, 0), 0x2E0) + 0x290);
        At<uint32_t>(At<uint8_t*>(player, 0), 0x13E4) = 0;
        At<uint32_t>(At<uint8_t*>(player, 0), 0x1328) = 0;
        At<uint32_t>(At<uint8_t*>(player, 0), 0xE4) = 0;
        float health = At<float>(At<uint8_t*>(player, 0), 0x1CA4);
        Ped_SetHealth(At<uint8_t*>(player, 0), health);
        At<float>(At<uint8_t*>(player, 0), 0x1CA8) = 0.0f;
        if (At<int32_t>(player, 0x234) != -1) {
            At<int32_t>(At<uint8_t*>(At<uint8_t*>(player, 0), 0x2E0), 0x18) = At<int32_t>(player, 0x234);
        }
        if (At<int32_t>(At<uint8_t*>(player, 0), 0x1310) == 0xD) {
            Ped_SetStat(At<uint8_t*>(player, 0), 10, 100);
        }
        Ped_SetSpecialRate(At<uint8_t*>(player, 0), At<float>(player, 0x254));
        uint8_t* clump = At<uint8_t*>(At<uint8_t*>(player, 0), 0x114);
        void* dict = At<void*>(At<uint8_t*>(clump, 0x94), 0x10);
        if (!dict) {
            continue;
        }
        int32_t index = -1;
        if (!AnimDict_FindByName(dict, 0, ambientAnimName, &index)) {
            continue;
        }
        if (index == -1) {
            index = 0;
        }
        uint8_t* anim = Blend_GetAnim(At<uint8_t*>(At<uint8_t*>(player, 0), 0x114), 0);
        AnimManager_Start(animManager_20C4910, At<uint8_t*>(At<uint8_t*>(player, 0), 0x114), index, anim, 3, 1.0f,
                          -1.0f);
        float r = float(GameRand()) / 32767.0f;
        float time = (float(0.30000001192092896) - 0.0f) * r + 0.0f;
        AnimManager_Play(animManager_20C4910, At<uint8_t*>(At<uint8_t*>(player, 0), 0x114), anim, 0, time, 0, 0);
        r = float(GameRand()) / 32767.0f;
        float speed = r * float(0.20000004768371582) + float(0.8999999761581421);
        AnimManager_SetSpeed(animManager_20C4910, At<uint8_t*>(At<uint8_t*>(player, 0), 0x114), anim, 0, speed);
    }
    At<uint32_t>(PlayerPed(game, 8), 0x1328) = 0;
    At<uint32_t>(PlayerPed(game, 8), 0xE4) = 0;
    At<float>(PlayerPed(game, 8), 0x1CA8) = 0.0f;
    char path[256];
    for (int i = 0; i < 9; i++) {
        uint8_t* player = Player(game, i);
        At<uint32_t>(At<uint8_t*>(player, 0), 0x16DC) = 0;
        std::sprintf(path, globalPathFormat, player + 0x8C);
        RemovePath(path);
        std::sprintf(path, globalPathFormat, player + 0x12C);
        RemovePath(path);
    }
    if (playerPed) {
        LoadModelTrees(playerPed);
    }
    for (int slot = 0; slot < count; slot++) {
        if (peds->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = re::PtrCast<uint8_t*>(peds->objects) + peds->elemSize * slot;
        if (!ped || ped == playerPed) {
            continue;
        }
        LoadModelTrees(ped);
    }
    CacheNodes();
    for (int slot = 0; slot < count; slot++) {
        if (peds->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = re::PtrCast<uint8_t*>(peds->objects) + peds->elemSize * slot;
        if (ped) {
            Ped_474090(ped);
        }
    }
    for (int i = 0; i < 9; i++) {
        const char* tree = At<const char*>(Ped_GetModelInfo(PlayerPed(game, i)), 0x64);
        uint8_t* owner = At<uint8_t*>(PlayerPed(game, i), 0x2E0);
        At<uint8_t*>(owner, 0x4B0) = ActionTree_Find(tree, nullptr);
        const char* aiTree = At<const char*>(Ped_GetModelInfo(PlayerPed(game, i)), 0x6C);
        owner = At<uint8_t*>(PlayerPed(game, i), 0x2E0);
        At<uint8_t*>(owner, 0x4B4) = ActionTree_Find(aiTree, nullptr);
    }
    controlledPed_C2B120 = playerPed;
    GenerateAllBoundingBoxes(0);
}

// ---- the game's own action tree (game +0x17E4: +0 its controller, +4 its context, +8
// its root node)

// 0x67CBF0 (thiscall, the tree): a controller and a context (0x58 bytes) made, the
// controller reset, DB_Game.act loaded and /Global/DBGame/Root found and played (when the
// controller has nothing playing), then updated for a 50th of the time step
void __fastcall DodgeballTree_Setup(uint8_t* tree, re::Edx) {
    void* memory = Malloc(0x38);
    At<void*>(tree, 0) = memory ? Ctl_Construct(memory) : nullptr;
    uint8_t* context = static_cast<uint8_t*>(Malloc(0x58));
    if (context) {
        static constexpr int kZeroes[] = {0, 4, 8, 0xC, 0x10, 0x14, 0x18, 0x20, 0x34, 0x38, 0x40, 0x44, 0x50, 0x54};
        for (int offset : kZeroes) {
            At<uint32_t>(context, offset) = 0;
        }
        At<uint8_t>(context, 0x1C) = 0;
        At<uint8_t>(context, 0x1D) = 0;
        At<uint8_t>(context, 0x3C) = 0;
        At<int32_t>(context, 0x48) = -1;
        for (int offset = 0x24; offset <= 0x30; offset += 4) {
            At<float>(context, offset) = 0.0f;
        }
    }
    At<uint8_t*>(tree, 4) = context;
    At<uint8_t*>(At<uint8_t*>(tree, 0), 0x18) = context;
    ActionController_Reset_5F40A0(At<void*>(tree, 0));
    At<void*>(At<uint8_t*>(tree, 4), 0) = At<void*>(tree, 0);
    At<uint8_t>(At<uint8_t*>(tree, 4), 0x1D) = 0;
    re::Ptr32<uint8_t>* entity = &At<uint8_t*>(At<uint8_t*>(tree, 4), 8);
    if (*entity) {
        Entity_CleanUpOldReference(*entity, entity);
        *entity = nullptr;
    }
    At<uint32_t>(At<uint8_t*>(tree, 4), 0x20) = 0;
    ActFile_Load(dbGameActFile, nullptr, nullptr);
    uint8_t* root = ActionTree_Find(dbGameRootPath, nullptr);
    At<uint8_t*>(tree, 8) = root;
    if (!At<uint32_t>(At<uint8_t*>(tree, 0), 0x10) && root) {
        uint8_t* node = vc::Call<uint8_t*>(root, 0x4C, At<void*>(tree, 4));
        Ctl_PlayNode(At<void*>(tree, 0), node);
    }
    float dt = gameTreeRate_B9D9BC * timeStep;
    Ctl_Update(At<void*>(tree, 0), dt);
}

// 0x67DC00 (thiscall, the tree): its controller cleared and freed, its context's entity
// references dropped and it freed, /Global/DBGame unloaded (the root node kept)
void __fastcall DodgeballTree_Free(uint8_t* tree, re::Edx) {
    Ctl_Clear(At<void*>(tree, 0));
    void* controller = At<void*>(tree, 0);
    if (controller) {
        Ctl_DropTracks(controller);
        Free(controller);
    }
    uint8_t* context = At<uint8_t*>(tree, 4);
    if (context) {
        static constexpr int kReferences[] = {0xC, 8};
        for (int offset : kReferences) {
            re::Ptr32<uint8_t>* entity = &At<uint8_t*>(context, offset);
            if (*entity) {
                Entity_CleanUpOldReference(*entity, entity);
            }
        }
        Free(context);
    }
    At<void*>(tree, 0) = nullptr;
    At<void*>(tree, 4) = nullptr;
    RemovePath(dbGamePath);
}

// ---- construction and the ball

// 0x67CD10 (thiscall): constructed: the teams and players cleared (flags' low bits: 0x1005
// kept from what was there), the court built, flags 4 | 8 then 8 cleared, the tree,
// registrations, ball, message and time cleared, +0x1890 -1; the tracks registered and the
// dodgeball camera made the camera's controller 0xB
uint8_t* __fastcall Dodgeball_Construct(uint8_t* game, re::Edx) {
    for (int team = 0; team < 2; team++) {
        uint8_t* t = game + team * 0x14;
        At<uint32_t>(t, 0) = 0;
        At<uint32_t>(t, 4) = 0;
        At<uint32_t>(t, 8) = 0;
        At<uint32_t>(t, 0xC) = 0;
        At<uint32_t>(t, 0x10) = 0;
    }
    for (int i = 0; i < 9; i++) {
        uint8_t* player = Player(game, i);
        At<uint32_t>(player, 4) = (At<uint32_t>(player, 4) & 0xFFF81005u) | 0x1005u;
        for (int k = 0; k < 6; k++) {
            At<float>(player, 0x23C + k * 4) = 0.0f;
        }
    }
    Court_InitFn(game + 0x1714);
    At<uint32_t>(game, 0x17DC) = (At<uint32_t>(game, 0x17DC) & 0xFFFFF80Cu) | 0xC;
    At<uint32_t>(game, 0x17E4) = 0;
    At<uint32_t>(game, 0x17E8) = 0;
    At<uint32_t>(game, 0x17EC) = 0;
    for (int offset = 0x1804; offset < 0x181C; offset += 4) {
        At<uint32_t>(game, offset) = 0;
    }
    At<uint32_t>(game, 0x17DC) &= ~8u;
    At<uint32_t>(game, 0x17E0) = 0;
    At<int32_t>(game, 0x1890) = -1;
    At<char>(game, 0x181C) = 0;
    At<uint32_t>(game, 0x1880) = 0;
    Dodgeball_RegisterTracksFn(game);
    void* memory = Malloc(0x38);
    void* camera = memory ? DodgeballCamera_Construct(memory) : nullptr;
    Camera_SetController(theCamera, 0xB, camera);
    return game;
}

// 0x67D000 (thiscall): the ball: the one found (kept when there was none), else the
// players' balls taken, the dodgeballs cleared and a new one thrown (no velocity) from
// the dodgeball's bone at a quarter above the court's centre; referenced as the ball
void __fastcall Dodgeball_SpawnBall(uint8_t* game, re::Edx) {
    uint8_t* found = Dodgeball_FindBallObjectFn(game);
    if (found) {
        if (!At<uint8_t*>(game, 0x17E0)) {
            At<uint8_t*>(game, 0x17E0) = found;
        }
        return;
    }
    ClearDodgeballsAndProjectiles();
    Dodgeball_TakeBallsFn(game);
    int model = GetModelIndexFromHash(*dodgeballHash_1B65364);
    uint32_t names[3];
    NameKey_FromHash(&names[0], boneName_1B6531C);
    NameKey_FromHash(&names[1], boneName_1B65394);
    NameKey_FromHash(&names[2], boneName_1B65368);
    int32_t path[3];
    HashPath_Set(path, 3, names);
    uint8_t* info = FindBoneInfo(path, boneRoot_1B65398);
    float lift[3] = {0.0f, 0.0f, 0.25f};
    float position[3];
    dbcourt::VectorAdd(position, &At<float>(At<uint8_t*>(game, 0x17D4), 0x9C), lift);
    uint8_t* ball = AddProjectile(nullptr, model, position[0], position[1], position[2], 0.0f, 0.0f, 1.0f, info,
                                  0.0f, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    At<uint8_t*>(game, 0x17E0) = ball;
    At<uint32_t>(ball, 0xA0) = 0;
    Entity_RegisterReference(At<uint8_t*>(game, 0x17E0), &At<uint8_t*>(game, 0x17E0));
    At<uint32_t>(ball, 0xE4) = 1;
    for (int k = 2; k >= 0; k--) {
        Empty_49A650(&names[k]);
    }
}

// the ball's speeds zeroed and it re-placed (0x443AA0, its frame, its vfuncs 3 and 2)
void SettleBall(uint8_t* game) {
    uint8_t* ball = At<uint8_t*>(game, 0x17E0);
    for (int k = 0; k < 6; k++) {
        At<float>(ball, 0x124 + k * 4) = zeroVector[k % 3];
    }
    Entity_443AA0(At<uint8_t*>(game, 0x17E0));
    Entity_UpdateRwFrame(At<uint8_t*>(game, 0x17E0));
    vc::Call<void>(At<uint8_t*>(game, 0x17E0), 0xC);
    vc::Call<void>(At<uint8_t*>(game, 0x17E0), 8);
}

// 0x67D1C0 (thiscall): everything reset: the players' balls taken, the dodgeballs cleared,
// the ball made (a new one when it isn't an object), put at the tuning's court point
// (+0x2C across, +0x30 along, +0x34 up) and settled; unless fading in (flag 8), the
// current players put by it: the first team's one across axis back facing across, the
// second's three quarters of it on facing back
void __fastcall Dodgeball_ResetFull(uint8_t* game, re::Edx) {
    Dodgeball_TakeBallsFn(game);
    ClearDodgeballsAndProjectiles();
    if (!At<uint8_t*>(game, 0x17E0)) {
        Dodgeball_SpawnBallFn(game);
    }
    uint8_t* ball = At<uint8_t*>(game, 0x17E0);
    if (!At<uint32_t>(ball, 0xC4)) {
        Weapon_Reset(At<void*>(ball, 0x1D0));
        Dodgeball_SpawnBallFn(game);
    }
    uint8_t* tuning = dodgeballTuning_20C8898;
    float point[3];
    Court_PointFn(At<uint8_t*>(game, 0x17D4), point, At<float>(tuning, 0x2C), At<float>(tuning, 0x30),
                  At<float>(tuning, 0x34));
    Entity_SetPosition(At<uint8_t*>(game, 0x17E0), point[0], point[1], point[2]);
    SettleBall(game);
    if ((At<uint32_t>(game, 0x17DC) & 8) && !Camera_GetFadeState(theCamera)) {
        return;
    }
    float position[3] = {zeroVector[0], zeroVector[1], zeroVector[2]};
    for (int i = 0; i < 8; i++) {
        uint32_t flags = PlayerFlags(game, i);
        if (!(flags & 1) || !(flags & 2)) {
            continue;
        }
        const float* direction;
        float back[3];
        if (i >= 4) {
            float* u = &At<float>(At<uint8_t*>(game, 0x17D4), 0xA8);
            float step[3] = {0.75f * u[0], u[1] * 0.75f, u[2] * 0.75f};
            float tmp[3];
            Copy3(position, dbcourt::VectorAdd(tmp, point, step));
            u = &At<float>(At<uint8_t*>(game, 0x17D4), 0xA8);
            back[0] = -u[0];
            back[1] = -u[1];
            back[2] = -u[2];
            direction = back;
        } else {
            float tmp[3];
            Copy3(position, dbcourt::VectorSub(tmp, point, &At<float>(At<uint8_t*>(game, 0x17D4), 0xA8)));
            direction = &At<float>(At<uint8_t*>(game, 0x17D4), 0xA8);
        }
        Dodgeball_PlaceEntityFn(game, PlayerPed(game, i), position, direction);
    }
}

// 0x67D450 (thiscall): the ball reset: made when missing (a new one when it isn't an
// object), put back where the old one was at the tuning's height (+0x34) and settled
void __fastcall Dodgeball_ResetBall(uint8_t* game, re::Edx) {
    if (!At<uint8_t*>(game, 0x17E0)) {
        Dodgeball_SpawnBallFn(game);
    }
    uint8_t* ball = At<uint8_t*>(game, 0x17E0);
    float position[3];
    Copy3(position, Pos(ball));
    if (!At<uint32_t>(ball, 0xC4)) {
        Weapon_Reset(At<void*>(ball, 0x1D0));
        Dodgeball_SpawnBallFn(game);
    }
    position[2] = At<float>(dodgeballTuning_20C8898, 0x34);
    Entity_SetPosition(At<uint8_t*>(game, 0x17E0), position[0], position[1], position[2]);
    SettleBall(game);
}

// 0x67D560 (thiscall): the players' holding flags (0x10) brought up to date; a ball that
// is a ped no longer holding it dropped (a ped not a player: the flags before the first
// player's read); none: found (an object, else a holding player's ped) and referenced,
// else everything reset; an object ball re-added to the world
void __fastcall Dodgeball_FindBall(uint8_t* game, re::Edx) {
    for (int i = 0; i < 8; i++) {
        uint32_t holds = At<int16_t>(At<uint8_t*>(PlayerPed(game, i), 0x1D0), 0x10E) != -1;
        PlayerFlags(game, i) ^= ((holds << 4) ^ PlayerFlags(game, i)) & 0x10;
    }
    re::Ptr32<uint8_t>* ballRef = &At<uint8_t*>(game, 0x17E0);
    uint8_t* ball = *ballRef;
    if (ball && (At<uint8_t>(ball, 0x108) & 7) == 3) {
        int index = -1;
        for (int i = 0; i < 9; i++) {
            if (PlayerPed(game, i) == ball) {
                index = i;
                break;
            }
        }
        if (!(At<uint8_t>(game, kPlayers + 4 + index * kPlayerSize) & 0x10)) {
            Entity_CleanUpOldReference(ball, ballRef);
            *ballRef = nullptr;
        }
    }
    if (!*ballRef) {
        *ballRef = Dodgeball_FindBallObjectFn(game);
        if (!*ballRef) {
            for (int i = 0; i < 8; i++) {
                if (PlayerFlags(game, i) & 0x10) {
                    *ballRef = PlayerPed(game, i);
                    break;
                }
            }
        }
        if (*ballRef) {
            Entity_RegisterReference(*ballRef, ballRef);
        } else {
            Dodgeball_ResetFullFn(game);
        }
    }
    ball = *ballRef;
    if (ball && At<uint32_t>(ball, 0xC4)) {
        Physical_RemoveAndAdd(ball);
    }
}

// 0x67D780 (thiscall): the ball (looked for when there is none)
uint8_t* __fastcall Dodgeball_GetBall(uint8_t* game, re::Edx) {
    if (!At<uint8_t*>(game, 0x17E0)) {
        Dodgeball_FindBallFn(game);
    }
    return At<uint8_t*>(game, 0x17E0);
}

// 0x67D7A0 (thiscall, ret 8): started (the first argument unused; the second's low bit as
// flag 1, 0x100 set): the court used, the tree set up, the second team's turn cleared,
// +0x1888 0 and the winner -1; the dodgeball's model and the DodgeBall animation group
// requested (the group's table added to the animation manager); everyone placed, the
// game started and reset and the ball found. (The second argument's slot is the group
// lookup's out value, as the game's.)
void __fastcall Dodgeball_Start(uint8_t* game, re::Edx, int, uint32_t flag) {
    At<uint32_t>(game, 0x17DC) = ((flag & 0xFF) & 1) | (At<uint32_t>(game, 0x17DC) & ~1u) | 0x100;
    At<uint8_t*>(game, 0x17D4) = game + 0x1714;
    DodgeballTree_SetupFn(game + 0x17E4);
    At<uint32_t>(game, 0x17DC) &= ~2u;
    At<uint32_t>(game, 0x1888) = 0;
    At<int32_t>(game, 0x188C) = -1;
    Game_4D9790(GetModelIndexFromHash(*dodgeballHash_1B65364), 0);
    int group = FindAgrId(dodgeballGroupName);
    At<int32_t>(game, 0x1890) = group;
    if (group >= 0) {
        uint8_t* table = GetAgrGroup(group, &flag);
        RequestModel(At<int32_t>(game, 0x1890), 4);
        LoadAllRequestedModels(0);
        AnimManager_AddRef(animManager_20C4910, table, flag);
    }
    Dodgeball_PlaceAllFn(game);
    Dodgeball_StartGameFn(game);
    Dodgeball_ResetFullFn(game);
    Dodgeball_FindBallFn(game);
}

// 0x67D880 (thiscall): a frame: pause handling (while fading in, flag 8: a forced pause
// (0x400) ends on button 8 or when the fade is done, else button 12 forces one; otherwise
// a forced pause, the game being over or a fade unpauses), the ball found; while not over:
// players whose health is spent put out of play (flag 0x80 after 3 s of play), a current
// one replaced by its team's next in play or, none left, the other team the winner; past
// the tuning's +0x24 seconds the team with more in play wins (level: the healthier, when
// no team has run out; level again: no winner yet); a winner: the players' controllers
// reset, over (0x20; 0x200 the first team won); then the game's tree played and updated,
// flag 0x40 cleared
void __fastcall Dodgeball_Update(uint8_t* game, re::Edx) {
    uint32_t flags = At<uint32_t>(game, 0x17DC);
    if ((flags & 8) && !(flags & 0x20)) {
        if (flags & 0x400) {
            if (!Pad_738900(8, 0) && !Camera_GetFadeState(theCamera)) {
                return;
            }
            Dodgeball_ForcePauseFn(game, 0);
            return;
        }
        if (Pad_738900(0xC, 0)) {
            Dodgeball_ForcePauseFn(game, 1);
        }
    } else if ((flags & 0x400) || (flags & 0x20) || Camera_GetFadeState(theCamera)) {
        Dodgeball_ForcePauseFn(game, 0);
    }
    if (At<uint8_t>(dodgeballTuning_20C8898, 0x22)) {
        // (a debug display's vectors, unused)
        float d[3];
        uint8_t* court = At<uint8_t*>(game, 0x17D4);
        dbcourt::VectorSub(d, &At<float>(court, 0x18), &At<float>(court, 0x9C));
        court = At<uint8_t*>(game, 0x17D4);
        dbcourt::VectorSub(d, &At<float>(court, 0x24), &At<float>(court, 0x9C));
    }
    Dodgeball_FindBallFn(game);
    if (!(At<uint32_t>(game, 0x17DC) & 0x20)) {
        float health[8];
        Dodgeball_HealthFractionsFn(game, health, 8);
        uint8_t teamOut = 0;
        for (int i = 0; i < 8; i++) {
            uint32_t& playerFlags = PlayerFlags(game, i);
            if (!(playerFlags & 1) || !(health[i] <= 0.0f)) {
                continue;
            }
            playerFlags &= ~1u;
            if (int32_t(timeInMs - At<uint32_t>(game, 0x1884)) > 3000) {
                At<uint32_t>(game, 0x17DC) |= 0x80;
            }
            if (playerFlags & 2) {
                int next = Dodgeball_FirstInPlayFn(game, i);
                if (next == -1) {
                    teamOut = 1;
                    At<uint32_t>(game, 0x188C) = i < 4;
                } else {
                    Dodgeball_SetCurrentFn(game, next);
                }
            }
        }
        int32_t elapsed = int32_t(timeInMs - At<uint32_t>(game, 0x1884));
        bool decided = true;
        if (elapsed > int(At<float>(dodgeballTuning_20C8898, 0x24)) * 1000) {
            int32_t inPlay[2] = {0, 0};
            for (int team = 0; team < 2; team++) {
                for (int k = 0; k < 4; k++) {
                    if (PlayerFlags(game, team * 4 + k) & 1) {
                        inPlay[team] += 1;
                    }
                }
            }
            if (inPlay[0] > inPlay[1]) {
                At<uint32_t>(game, 0x188C) = 0;
            } else if (inPlay[0] < inPlay[1]) {
                At<uint32_t>(game, 0x188C) = 1;
            } else if (!teamOut) {
                Dodgeball_HealthFractionsFn(game, health, 8);
                float sum[2] = {0.0f, 0.0f};
                for (int team = 0; team < 2; team++) {
                    float s = health[team * 4] + sum[team];
                    s = s + health[team * 4 + 1];
                    s = s + health[team * 4 + 2];
                    sum[team] = s + health[team * 4 + 3];
                }
                if (sum[1] < sum[0]) {
                    At<uint32_t>(game, 0x188C) = 0;
                } else if (sum[1] > sum[0]) {
                    At<uint32_t>(game, 0x188C) = 1;
                } else {
                    decided = false;
                }
            }
        } else if (!teamOut) {
            decided = false;
        }
        if (decided) {
            if (!(At<uint32_t>(game, 0x17DC) & 0x20)) {
                for (int i = 0; i < 9; i++) {
                    ActionController_Reset_5F40A0(At<uint8_t*>(PlayerPed(game, i), 0x2E0) + 0x290);
                }
            }
            At<uint32_t>(game, 0x17DC) |= 0x20;
            if (!At<uint32_t>(game, 0x188C)) {
                At<uint32_t>(game, 0x17DC) |= 0x200;
            } else {
                At<uint32_t>(game, 0x17DC) &= ~0x200u;
            }
        }
    }
    uint8_t* tree = game + 0x17E4;
    if (!At<uint32_t>(At<uint8_t*>(tree, 0), 0x10) && At<uint8_t*>(tree, 8)) {
        uint8_t* node = vc::Call<uint8_t*>(At<uint8_t*>(tree, 8), 0x4C, At<void*>(tree, 4));
        Ctl_PlayNode(At<void*>(tree, 0), node);
    }
    float dt = gameTreeRate_B9D9BC * timeStep;
    Ctl_Update(At<void*>(tree, 0), dt);
    At<uint32_t>(game, 0x17DC) &= ~0x40u;
}

// 0x67DC80 (thiscall, ret 0x10; the game the global one): the ball (looked for when
// missing) within fractions of the court: its across and along parts over the court's
// half extents (the across one against the negated extent) between the bounds; more than
// 20 above or 0.3 below the centre: outside
uint8_t __fastcall Dodgeball_BallInArea(uint8_t*, re::Edx, float minAcross, float maxAcross, float minAlong,
                                        float maxAlong) {
    uint8_t* game = dodgeballGame;
    if (!At<uint8_t*>(game, 0x17E0)) {
        Dodgeball_FindBallFn(game);
        game = dodgeballGame;
    }
    uint8_t* ball = At<uint8_t*>(game, 0x17E0);
    if (!ball) {
        return 0;
    }
    uint8_t* court = At<uint8_t*>(game, 0x17D4);
    float* u = &At<float>(court, 0xA8);
    float* v = &At<float>(court, 0xB4);
    float w[3];
    dbcourt::VectorSub(w, &At<float>(court, 0xC), reinterpret_cast<float*>(court));
    float du = (u[0] * w[0] + u[1] * w[1]) + u[2] * w[2];
    float negDu = -du;
    float dv = (w[0] * v[0] + w[1] * v[1]) + w[2] * v[2];
    float p[3];
    Copy3(p, Pos(ball));
    float d[3];
    dbcourt::VectorSub(d, p, &At<float>(court, 0x9C));
    float pu = (u[1] * d[1] + d[0] * u[0]) + u[2] * d[2];
    float across = pu / negDu;
    float pv = (d[0] * v[0] + d[1] * v[1]) + v[2] * d[2];
    float along = pv / dv;
    if (20.0f < d[2] || d[2] < -0.3f) {
        across = -99999997952.0f;
        along = -99999997952.0f;
    }
    if (minAcross > across || maxAcross < across || minAlong > along || maxAlong < along) {
        return 0;
    }
    return 1;
}

// 0x67DE50 (thiscall): up to three balls removed (each found; an object's projectile
// removed, a held one's weapon reset)
void __fastcall Dodgeball_RemoveBalls(uint8_t* game, re::Edx) {
    for (int n = 0; n < 3; n++) {
        Dodgeball_FindBallFn(game);
        uint8_t* ball = At<uint8_t*>(game, 0x17E0);
        if (!ball) {
            return;
        }
        if (!At<uint32_t>(ball, 0xC4)) {
            Weapon_Reset(At<void*>(ball, 0x1D0));
        }
        ball = At<uint8_t*>(game, 0x17E0);
        if (At<uint32_t>(ball, 0xC4)) {
            Projectile_Destroy(At<int32_t>(ball, 0x238), 0);
        }
    }
}

// 0x67DEB0 (thiscall): stopped: not fading in, the tree freed, the game ended, the balls
// removed, the animation group released (its lookup's out value an uninitialised local
// slot, passed on as the game's)
void __fastcall Dodgeball_Stop(uint8_t* game, re::Edx) {
    At<uint32_t>(game, 0x17DC) &= ~8u;
    DodgeballTree_FreeFn(game + 0x17E4);
    Dodgeball_EndGameFn(game);
    Dodgeball_RemoveBallsFn(game);
    int group = At<int32_t>(game, 0x1890);
    if (group < 0) {
        return;
    }
    // (the lookup's out value a local slot the entry's push of ecx made: the game)
    uint32_t slot = uint32_t(reinterpret_cast<uintptr_t>(game));
    uint8_t* table = GetAgrGroup(group, &slot);
    AnimManager_Release(animManager_20C4910, table, slot, 0);
    At<int32_t>(game, 0x1890) = 0;
}

} // namespace

#define DODGEGAME35_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

DODGEGAME35_REVERSED("DodgeballCourt::Init", 0x0067A1E0, &Court_Init);
DODGEGAME35_REVERSED("DodgeballCourt::Point", 0x0067AB50, &Court_Point);
DODGEGAME35_REVERSED("DodgeballGame::TakeBalls", 0x0067AC50, &Dodgeball_TakeBalls);
DODGEGAME35_REVERSED("DodgeballGame::ResetHealth", 0x0067AC90, &Dodgeball_ResetHealth);
DODGEGAME35_REVERSED("DodgeballGame::SetupPlayer", 0x0067AD60, &Dodgeball_SetupPlayer);
DODGEGAME35_REVERSED("DodgeballGame::HealthFractions", 0x0067AE70, &Dodgeball_HealthFractions);
DODGEGAME35_REVERSED("DodgeballGame::CurrentPlayerFloat", 0x0067AF00, &Dodgeball_CurrentPlayerFloat);
DODGEGAME35_REVERSED("DodgeballGame::PlayerIndex", 0x0067AF20, &Dodgeball_PlayerIndex);
DODGEGAME35_REVERSED("DodgeballGame::ChangePlayer", 0x0067AF50, &Dodgeball_ChangePlayer);
DODGEGAME35_REVERSED("DodgeballGame::SetCurrent", 0x0067AFA0, &Dodgeball_SetCurrent);
DODGEGAME35_REVERSED("DodgeballGame::PlayerPed", 0x0067B040, &Dodgeball_PlayerPed);
DODGEGAME35_REVERSED("DodgeballGame::TeamCurrent", 0x0067B060, &Dodgeball_TeamCurrent);
DODGEGAME35_REVERSED("DodgeballGame::OtherTeamCurrent", 0x0067B080, &Dodgeball_OtherTeamCurrent);
DODGEGAME35_REVERSED("DodgeballGame::GetPlayerFlags", 0x0067B0A0, &Dodgeball_GetPlayerFlags);
DODGEGAME35_REVERSED("DodgeballGame::SetPlayerFlags", 0x0067B0C0, &Dodgeball_SetPlayerFlags);
DODGEGAME35_REVERSED("DodgeballGame::GetFlags", 0x0067B0E0, &Dodgeball_GetFlags);
DODGEGAME35_REVERSED("DodgeballGame::SetFlags", 0x0067B0F0, &Dodgeball_SetFlags);
DODGEGAME35_REVERSED("DodgeballGame::SameTeam", 0x0067B100, &Dodgeball_SameTeam);
DODGEGAME35_REVERSED("DodgeballGame::FirstInPlay", 0x0067B180, &Dodgeball_FirstInPlay);
DODGEGAME35_REVERSED("DodgeballGame::IsFirstInPlay", 0x0067B1C0, &Dodgeball_IsFirstInPlay);
DODGEGAME35_REVERSED("DodgeballGame::GetWinner", 0x0067B1E0, &Dodgeball_Get188C);
DODGEGAME35_REVERSED("DodgeballGame::Player", 0x0067B1F0, &Dodgeball_Player);
DODGEGAME35_REVERSED("DodgeballGame::PlayerOf", 0x0067B210, &Dodgeball_PlayerOf);
DODGEGAME35_REVERSED("DodgeballGame::Flag100", 0x0067B250, &Dodgeball_Flag100);
DODGEGAME35_REVERSED("DodgeballGame::PlayerDuck", 0x0067B260, &Dodgeball_PlayerDuck);
DODGEGAME35_REVERSED("DodgeballGame::SetMessage", 0x0067B2A0, &Dodgeball_SetMessage);
DODGEGAME35_REVERSED("DodgeballGame::PlaceEntity", 0x0067B2C0, &Dodgeball_PlaceEntity);
DODGEGAME35_REVERSED("DodgeballGame::PlaceAll", 0x0067B360, &Dodgeball_PlaceAll);
DODGEGAME35_REVERSED("DodgeballGame::Times", 0x0067B480, &Dodgeball_Times);
DODGEGAME35_REVERSED("DodgeballGame::PlayerPositions", 0x0067B4B0, &Dodgeball_PlayerPositions);
DODGEGAME35_REVERSED("DodgeballGame::TeamCode", 0x0067B500, &Dodgeball_TeamCode);
DODGEGAME35_REVERSED("DodgeballGame::SwapPlayers", 0x0067B560, &Dodgeball_SwapPlayers);
DODGEGAME35_REVERSED("DodgeballGame::SetPaused", 0x0067B5F0, &Dodgeball_SetPaused);
DODGEGAME35_REVERSED("DodgeballGame::ForcePause", 0x0067B620, &Dodgeball_ForcePause);
DODGEGAME35_REVERSED("DodgeballGame::ClampToCourt", 0x0067B800, &Dodgeball_ClampToCourt);
DODGEGAME35_REVERSED("DodgeballGame::UnregisterTracks", 0x0067BFB0, &Dodgeball_UnregisterTracks);
DODGEGAME35_REVERSED("DodgeballGame::Shutdown", 0x0067C0A0, &Dodgeball_Shutdown);
DODGEGAME35_REVERSED("DodgeballGame::FindBallObject", 0x0067C100, &Dodgeball_FindBallObject);
DODGEGAME35_REVERSED("DodgeballGame::StartGame", 0x0067C2A0, &Dodgeball_StartGame);
DODGEGAME35_REVERSED("DodgeballGame::EndGame", 0x0067C7F0, &Dodgeball_EndGame);
DODGEGAME35_REVERSED("DodgeballGameTree::Setup", 0x0067CBF0, &DodgeballTree_Setup);
DODGEGAME35_REVERSED("DodgeballGame::DodgeballGame", 0x0067CD10, &Dodgeball_Construct);
DODGEGAME35_REVERSED("DodgeballGame::SpawnBall", 0x0067D000, &Dodgeball_SpawnBall);
DODGEGAME35_REVERSED("DodgeballGame::ResetFull", 0x0067D1C0, &Dodgeball_ResetFull);
DODGEGAME35_REVERSED("DodgeballGame::ResetBall", 0x0067D450, &Dodgeball_ResetBall);
DODGEGAME35_REVERSED("DodgeballGame::FindBall", 0x0067D560, &Dodgeball_FindBall);
DODGEGAME35_REVERSED("DodgeballGame::GetBall", 0x0067D780, &Dodgeball_GetBall);
DODGEGAME35_REVERSED("DodgeballGame::Start", 0x0067D7A0, &Dodgeball_Start);
DODGEGAME35_REVERSED("DodgeballGame::Update", 0x0067D880, &Dodgeball_Update);
DODGEGAME35_REVERSED("DodgeballGameTree::Free", 0x0067DC00, &DodgeballTree_Free);
DODGEGAME35_REVERSED("DodgeballGame::BallInArea", 0x0067DC80, &Dodgeball_BallInArea);
DODGEGAME35_REVERSED("DodgeballGame::RemoveBalls", 0x0067DE50, &Dodgeball_RemoveBalls);
DODGEGAME35_REVERSED("DodgeballGame::Stop", 0x0067DEB0, &Dodgeball_Stop);
