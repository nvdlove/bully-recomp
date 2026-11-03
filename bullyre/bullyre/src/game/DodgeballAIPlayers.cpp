// The dodgeball minigame's AI players' tracks (0x67EA50..0x6839D0, beside the ball's and
// the natives' 0x6800A0..0x681750 in DodgeballBall.cpp): the lead (the team's current
// player: throws, catches, dodges; a state machine picking actions by weight), the one
// fetching a ball, the sideline players (out: along the sides), the supports, and the
// small face-lead / catch / goto / idle tracks. Their factories and destructors are in
// DodgeballFactories.cpp, the other dodgeball tracks in DodgeballTracks.cpp.
//
// The game's state (*0x1B89150): +0x28 nine player records of 0x28C (+0 the ped, +4 flags:
// 1 in play, 0x80 holding a ball, 0x100 / 0x200 turn bits, bits 14..15 a sideline spot,
// 0x10000; bits 17..18 a support spot), +0x17D4 the court (DodgeballArenaButes: +0 / +0xC
// the inner rectangle's corners, +0x18.. the outer one's, +0x9C the centre, +0xA8 the
// long axis, +0xB4 the cross axis).
//
// A base AI track (DodgeballBaseAITrack): +0x1C the owner, +0x20 its AI butes
// (DodgeballAIButes), +0x24 the ramp butes, +0x28 the player's index (0..3 the first
// team, 4..7 the second).

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

using actc::V3;
using vc::At;

GAME_VAR(uint8_t*, dbGame, 0x01B89150);
GAME_VAR(uint32_t, dbTimeMs, 0x00C1A9B4);   // CTimer::m_snTimeInMilliseconds
GAME_ARRAY(float, dbZeroVector, 0x00BCC564);
// the AI's court margins
GAME_VAR(float, dbSideMargin, 0x00B9DA84);     // from the inner rectangle's sides
GAME_VAR(float, dbOuterMargin, 0x00B9DA88);    // inside the outer rectangle
GAME_VAR(float, dbCentreMargin, 0x00B9DA8C);   // from the centre line
GAME_VAR(float, dbSidelineScale, 0x00B9DA90);
GAME_VAR(float, dbSidelineMargin, 0x00B9DA94);
GAME_VAR(float, dbSupportBack, 0x00B9DA98);
GAME_VAR(float, dbSupportBackFirst, 0x00B9DA9C);
GAME_VAR(float, dbSupportMargin, 0x00B9DAA0);
GAME_VAR(float, dbSupportScale, 0x00B9DAA4);
// the action names the lead checks (/Global/... hashes)
inline const uint32_t* const dbThrowNode = reinterpret_cast<const uint32_t*>(0x01B65284);
inline const uint32_t* const dbCatchNode = reinterpret_cast<const uint32_t*>(0x01B6529C);

// the game's accessors
GAME_FN(float, __thiscall, Dodgeball_CurrentPlayerFloat, 0x0067AF00, uint8_t* game, int team);
GAME_FN(uint8_t*, __thiscall, Dodgeball_PlayerPed, 0x0067B040, uint8_t* game, int i);
GAME_FN(int, __thiscall, Dodgeball_TeamCurrent, 0x0067B060, uint8_t* game, int i);
GAME_FN(int, __thiscall, Dodgeball_OtherTeamCurrent, 0x0067B080, uint8_t* game, int i);
GAME_FN(uint32_t*, __thiscall, Dodgeball_GetPlayerFlags, 0x0067B0A0, uint8_t* game, uint32_t* out, int i);
GAME_FN(void, __thiscall, Dodgeball_SetPlayerFlags, 0x0067B0C0, uint8_t* game, int i, uint32_t flags);
GAME_FN(uint8_t, __thiscall, Dodgeball_IsTeamCurrent, 0x0067B1C0, uint8_t* game, int i);
GAME_FN(uint8_t*, __thiscall, Dodgeball_Player, 0x0067B1F0, uint8_t* game, int i);
GAME_FN(uint32_t, __thiscall, Dodgeball_TeamCode, 0x0067B500, uint8_t* game, uint8_t* ped);   // (al)
GAME_FN(uint8_t*, __thiscall, Dodgeball_GetBall, 0x0067D780, uint8_t* game);
GAME_FN(int, __cdecl, PlayersIn_Count, 0x00673040, int team);
// the base AI track's
GAME_FN(uint8_t, __thiscall, AITrack_Start, 0x00674FE0, uint8_t* self, uint8_t* owner);
GAME_FN(uint8_t*, __thiscall, AITrack_OpponentPed, 0x00672B90, uint8_t* self);
GAME_FN(void, __thiscall, AITrack_RunTo_x64, 0x00675040, uint8_t* self, V3 target, uint32_t, uint32_t, uint32_t, float speed);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void AITrack_RunTo(uint8_t* a0, V3 a1, V3 a2, float a3) {
    return AITrack_RunTo_x64(a0, a1, re::BitsOf(a2.x), re::BitsOf(a2.y), re::BitsOf(a2.z), a3);
}
GAME_FN(void, __thiscall, TrackBase_Construct, 0x0061C320, uint8_t* self);
GAME_FN(void, __thiscall, AITrack_Destruct, 0x00672B80, uint8_t* self);
// ours, through the game's addresses
GAME_FN(V3*, __thiscall, Lead_CourtTarget, 0x0067EB50, uint8_t* self, V3* out, float along, int mirrored);
GAME_FN(int, __thiscall, Lead_PickAttack, 0x0067EAA0, uint8_t* self);
GAME_FN(int, __thiscall, Lead_PickDefence, 0x0067EF10, uint8_t* self);
GAME_FN(void, __thiscall, Lead_Shadow, 0x0067F120, uint8_t* self);
GAME_FN(void, __thiscall, Lead_RunAtOpponent, 0x0067F320, uint8_t* self);
GAME_FN(void, __thiscall, Lead_Wander, 0x0067F3C0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, Lead_ThrowTimer, 0x0067F4B0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, Lead_FakeTimer, 0x0067F530, uint8_t* self);
GAME_FN(uint8_t, __thiscall, Lead_Facing, 0x0067F5B0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, Lead_AtTarget, 0x0067F720, uint8_t* self);
GAME_FN(uint8_t, __thiscall, Lead_PassTimer, 0x0067F7B0, uint8_t* self);
GAME_FN(V3*, __thiscall, Lead_Sideways, 0x0067F840, uint8_t* self, V3* out);
GAME_FN(void, __thiscall, GetBall_RunToBall, 0x0067FF30, uint8_t* self);
GAME_FN(int, __thiscall, SidelineLead_PickAction, 0x00681770, uint8_t* self);
GAME_FN(void, __thiscall, Sideline_Clamp, 0x00681980, uint8_t* self, V3* position, int);
GAME_FN(uint8_t, __thiscall, Sideline_Spot, 0x00682280, uint8_t* self, int player, V3* out);
GAME_FN(uint8_t, __thiscall, Support_Spot, 0x00682A90, uint8_t* self, int player, V3* out);

GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(void, __thiscall, ActionInputs_Clear, 0x005F3A40, uint8_t* inputs);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlaying, 0x005F4800, uint8_t* controller, const uint32_t* name, int ancestors, int nested);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(uint8_t, __thiscall, PedMove_To_x64, 0x004A5000, void* self, float, float, float, int flags, float a, float b, uintptr_t, uint32_t team);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t PedMove_To(void* a0, V3 a1, int a2, float a3, float a4, int a5, uint32_t a6) {
    return PedMove_To_x64(a0, a1.x, a1.y, a1.z, a2, a3, a4, a5, a6);
}
inline int __cdecl CrtRand() { return crt::Rand(); }
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const void* a, const void* b);
GAME_FN(V3*, __cdecl, VectorSub, 0x00414B70, V3* out, const void* a, const void* b);
GAME_FN(V3*, __cdecl, CrossProduct, 0x00414730, V3* out, const void* a, const void* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, V3* v);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(float, __cdecl, VecHeading, 0x00433190, const V3* v);
GAME_FN(float, __cdecl, AngleDiff, 0x00432980, float a, float b);

inline uint8_t* Court() {
    return At<uint8_t*>(dbGame, 0x17D4);
}
inline float& F(uint8_t* p, int offset) {
    return At<float>(p, offset);
}
inline uint8_t* Ped(uint8_t* self) {
    return At<uint8_t*>(At<uint8_t*>(self, 0x1C), 8);
}
inline uint8_t* Butes(uint8_t* self) {
    return At<uint8_t*>(self, 0x20);
}
inline int Player(uint8_t* self) {
    return At<int32_t>(self, 0x28);
}
inline float* PosOf(uint8_t* e) {
    return actc::Pos(e);
}
inline V3 Copy(const float* p) {
    return V3{p[0], p[1], p[2]};
}
inline V3 Scaled(const float* v, float k) {   // (v * k per component)
    return V3{v[0] * k, v[1] * k, v[2] * k};
}
// a ped holds a ball (its weapon slot's model)
inline bool HasBall(uint8_t* ped) {
    return At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) != -1;
}
inline uint8_t* Inputs(uint8_t* ped) {
    return At<uint8_t*>(ped, 0x2E0);
}
inline void Press(uint8_t* inputs, int button) {
    ActionInputs_SetRequest(inputs, button, 1, 0.0f);
}
// rand() / 32767 (the game's x87 at single precision)
inline float Random01() {
    return float(CrtRand()) / 32767.0f;
}

// A choice by weights (the game's inline loop): a random share of the sum, the first
// weight whose running share reaches it; n when none does
int WeightedPick(const float* weights, int n) {
    float total = weights[0] + 0.0f;
    for (int i = 1; i < n; i++) {
        total = total + weights[i];
    }
    float r = Random01() * (1.0f - 0.0f) + 0.0f;
    float sum = 0.0f;
    int i = 0;
    for (; i < n; i++) {
        sum = weights[i] / total + sum;
        if (!(sum < r)) {
            break;
        }
    }
    return i;
}

// The team's lead-player's count (butes +0x68) under the team's current player's float:
// 1.0 or 0.0 (the x87 compare of the int)
inline float BelowCurrent(uint8_t* self) {
    int32_t count = At<int32_t>(Butes(self), 0x68);
    float current = Dodgeball_CurrentPlayerFloat(dbGame, Player(self) >= 4);
    return double(count) < double(current) ? 1.0f : 0.0f;
}

// A ms time scaled by the ped's stat 20 (percent), truncated (the game converts the
// unsigned difference exactly and rounds the product once)
inline int ScaledElapsed(uint8_t* ped, uint32_t since) {
    float scale = float(Ped_GetStat(ped, 20)) / 100.0f;
    return int(float(double(scale) * double(uint32_t(dbTimeMs - since))));
}

// ---- the lead ---------------------------------------------------------------------------

// 0x67EA50 (thiscall, vf2): Start: the base's, then idle: waiting (state 4), no action,
// the target at the origin, no timers
uint8_t __fastcall Lead_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    AITrack_Start(self, owner);
    At<int32_t>(self, 0x2C) = 0;   // the action being played
    At<int32_t>(self, 0x30) = 0;   // the next one
    At<int32_t>(self, 0x38) = 0;   // the action's start time
    At<int32_t>(self, 0x34) = 4;   // the state
    F(self, 0x3C) = dbZeroVector[0];
    F(self, 0x40) = dbZeroVector[1];
    At<int32_t>(self, 0x48) = 0;   // the last wander target's time
    At<int32_t>(self, 0x4C) = 0;   // the throw deadline
    At<int32_t>(self, 0x50) = 0;   // the fake / pass deadline
    F(self, 0x44) = dbZeroVector[2];
    return 1;
}

// 0x67EAA0 (thiscall): an attack (9..12, or 13) by the butes' weights +0x44..+0x50
int __fastcall Lead_PickAttackImpl(uint8_t* self, re::Edx) {
    uint8_t* butes = Butes(self);
    float weights[4] = {F(butes, 0x44), F(butes, 0x48), F(butes, 0x4C), F(butes, 0x50)};
    return WeightedPick(weights, 4) + 9;
}

// 0x67EB50 (thiscall): a point on this team's half of the court: `along` (-1..1, the
// cross axis share, with a random spread of the butes' +0x18 degrees, clamped to 90) and a
// random depth (butes +0x1C / +0x20 / +0x24), mirrored for the other side
V3* __fastcall Lead_CourtTargetImpl(uint8_t* self, re::Edx, V3* out, float along, int mirrored) {
    uint8_t* butes = Butes(self);
    uint8_t* court = Court();
    const float halfPi = 1.5707963705062866f;

    float spread = F(butes, 0x18) * 3.1415927410125732f / 180.0f;
    float negSpread = -spread;
    float r = Random01();
    float jitter = negSpread + r * (spread - negSpread);
    float angle = jitter + (halfPi - spread) * (along + along);
    if (angle < -halfPi) {
        angle = -halfPi;
    } else if (angle > halfPi) {
        angle = halfPi;
    }
    float c = trk::CrtCos(angle);
    float x = c * F(butes, 0x1C);
    float s = trk::CrtSin(angle);
    float y = s * F(butes, 0x20);
    float depthRange = F(butes, 0x24);
    float depth = Random01() * (depthRange - 0.0f) + 0.0f;
    float near = 1.0f - depth;
    if (re::Flag(mirrored)) {
        x = -x;
    }

    V3 d;
    VectorSub(&d, court + 0xC, court);
    float lenX = (F(court, 0xAC) * d.y + d.x * F(court, 0xA8)) + F(court, 0xB0) * d.z;
    float lenY = (F(court, 0xB4) * d.x + F(court, 0xB8) * d.y) + d.z * F(court, 0xBC);
    float side = Player(self) < 0 ? 1.0f : -1.0f;
    float half = lenX * 0.5f * side;
    float depthShare = re::Flag(mirrored) ? 1.100000023841858f - F(butes, 0x28) : F(butes, 0x28);
    float w = depthShare * half;

    const float* axisX = &F(court, 0xA8);
    const float* axisY = &F(court, 0xB4);
    V3 base = Copy(&F(court, 0x9C));
    base.x = axisX[0] * w + base.x;
    base.y = base.y + axisX[1] * w;
    base.z = base.z + w * axisX[2];
    // (each step a vector times a scalar, in the game's order)
    V3 alongX = Scaled(axisX, x);
    alongX = Scaled(&alongX.x, lenX);
    alongX = Scaled(&alongX.x, 0.25f);
    alongX = Scaled(&alongX.x, near);
    alongX = Scaled(&alongX.x, side);
    V3 alongY = Scaled(axisY, y);
    alongY = Scaled(&alongY.x, lenY);
    alongY = Scaled(&alongY.x, 0.5f);
    alongY = Scaled(&alongY.x, near);
    V3 sum;
    VectorAdd(&sum, &base, &alongX);
    VectorAdd(out, &sum, &alongY);
    return out;
}

// The player's share along the long axis from the centre: (its offset from the centre on
// that axis) / (the court's length), signed for its side
float LeadDepthShare(uint8_t* self, uint8_t* ped) {
    uint8_t* court = Court();
    V3 d;
    VectorSub(&d, court + 0xC, court);
    float lenX = (F(court, 0xAC) * d.y + d.x * F(court, 0xA8)) + F(court, 0xB0) * d.z;
    float side = Player(self) < 0 ? 1.0f : -1.0f;
    V3 e;
    VectorSub(&e, PosOf(ped), court + 0x9C);
    float proj = (e.y * F(court, 0xAC) + e.x * F(court, 0xA8)) + e.z * F(court, 0xB0);
    return proj / lenX * side;
}

// 0x67EF10 (thiscall): a defence (1..7): 6 (step back) when the player is near its half's
// front (butes +0xA0), else by the butes' weights +0x6C..+0x84 (the last only while the
// team's count allows)
int __fastcall Lead_PickDefenceImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = Dodgeball_PlayerPed(dbGame, Player(self));
    if (LeadDepthShare(self, ped) < F(Butes(self), 0xA0)) {
        return 6;
    }
    float below = BelowCurrent(self);
    uint8_t* butes = Butes(self);
    float weights[7] = {F(butes, 0x6C), F(butes, 0x70), F(butes, 0x74), F(butes, 0x78),
                        F(butes, 0x7C), F(butes, 0x80), F(butes, 0x84) * below};
    return WeightedPick(weights, 7) + 1;
}

// 0x67F120 (thiscall): the lead without the ball keeps across from the opponent's current
// player (the court target at its cross-axis share, mirrored when the other team isn't
// holding back), moving the target only when it changed by more than 1.5
void __fastcall Lead_ShadowImpl(uint8_t* self, re::Edx) {
    uint8_t* court = Court();
    V3 d;
    VectorSub(&d, court + 0xC, court);
    float lenY = (F(court, 0xB8) * d.y + F(court, 0xB4) * d.x) + F(court, 0xBC) * d.z;
    V3 opponent = Copy(PosOf(AITrack_OpponentPed(self)));
    V3 e;
    VectorSub(&e, &opponent, court + 0x9C);
    float proj = (e.y * F(court, 0xB8) + e.x * F(court, 0xB4)) + e.z * F(court, 0xBC);
    float along = proj / lenY;
    uint32_t flags;
    uint8_t mirrored = uint8_t(~*Dodgeball_GetPlayerFlags(dbGame, &flags,
                                   Dodgeball_TeamCurrent(dbGame, (Player(self) >= 4) * 4)) & 1);
    V3 target;
    Lead_CourtTarget(self, &target, along, mirrored);
    V3* current = &At<V3>(self, 0x3C);
    V3 change;
    VectorSub(&change, &target, current);
    float len = std::sqrt(std::fabs((change.x * change.x + change.y * change.y) + change.z * change.z));
    if (len > 1.5f) {
        *current = target;
    }
    V3 goal = *current;
    V3 dir;
    VectorSub(&dir, &goal, PosOf(Ped(self)));
    AITrack_RunTo(self, goal, dir, 0.75f);
}

// 0x67F320 (thiscall): run at the opponent's current player
void __fastcall Lead_RunAtOpponentImpl(uint8_t* self, re::Edx) {
    V3 goal = Copy(PosOf(AITrack_OpponentPed(self)));
    V3 dir;
    VectorSub(&dir, &goal, PosOf(Ped(self)));
    AITrack_RunTo(self, goal, dir, 0.75f);
}

// 0x67F3C0 (thiscall): wander about the team's half: a new random court target every
// 100 ms or more (the time kept at +0x48)
void __fastcall Lead_WanderImpl(uint8_t* self, re::Edx) {
    if (dbTimeMs - At<uint32_t>(self, 0x48) > 100) {
        float along = Random01() * 0.20000000298023224f - 0.10000000149011612f;
        V3 target;
        At<V3>(self, 0x3C) = *Lead_CourtTarget(self, &target, along, 0);
    }
    V3 goal = At<V3>(self, 0x3C);
    At<uint32_t>(self, 0x48) = dbTimeMs;
    V3 dir;
    VectorSub(&dir, &goal, PosOf(Ped(self)));
    AITrack_RunTo(self, goal, dir, 0.2f);
}

// A random deadline: now + base +- 75% (the butes' int at ButesOffset), set when the
// field is clear (false); once passed, cleared (both deadlines, or just this one) and
// true. 0x67F4B0 the throw (+0x2C -> +0x4C), 0x67F530 the fake (+0x30 -> +0x50), 0x67F7B0
// the pass (+0xAC -> +0x50)
template <int ButesOffset, int Field, bool ClearBoth>
uint8_t __fastcall Lead_Deadline(uint8_t* self, re::Edx) {
    uint32_t deadline = At<uint32_t>(self, Field);
    if (deadline == 0) {
        int32_t base = At<int32_t>(Butes(self), ButesOffset);
        int32_t spread = int32_t(float(double(base) * 0.75));
        uint32_t now = dbTimeMs;
        int32_t low = base - spread;
        int32_t high = base + spread;
        // (RandomInt inlined: low + rand * (high - low - 1) / 32767)
        int32_t r = CrtRand();
        At<uint32_t>(self, Field) = uint32_t(r * (high - low - 1) / 32767 + low) + now;
        return 0;
    }
    if (dbTimeMs > deadline) {
        if (ClearBoth) {
            At<uint32_t>(self, 0x4C) = 0;
        }
        At<uint32_t>(self, 0x50) = 0;
        return 1;
    }
    return 0;
}

// 0x67F5B0 (thiscall): the player is close enough to its half's front (butes +0xA4) and
// faces the opponent's current player within 30 degrees
uint8_t __fastcall Lead_FacingImpl(uint8_t* self, re::Edx) {
    uint8_t* ped = Dodgeball_PlayerPed(dbGame, Player(self));
    if (!(F(Butes(self), 0xA4) > LeadDepthShare(self, ped))) {
        return 0;
    }
    float* own = PosOf(ped);
    V3 dir;
    VectorSub(&dir, PosOf(AITrack_OpponentPed(self)), own);
    Vector_Normalise(&dir);
    V3 forward;
    Placeable_GetForward(ped, &forward);
    float facing = VecHeading(&forward);
    float wanted = VecHeading(&dir);
    float diff = std::fabs(AngleDiff(wanted, facing));
    return diff < 0.5235987901687622f;
}

// 0x67F720 (thiscall): the player is within 0.2 of the target (+0x3C)
uint8_t __fastcall Lead_AtTargetImpl(uint8_t* self, re::Edx) {
    V3 d;
    VectorSub(&d, self + 0x3C, PosOf(Dodgeball_PlayerPed(dbGame, Player(self))));
    float len = std::sqrt(std::fabs((d.x * d.x + d.y * d.y) + d.z * d.z));
    return len < 0.2f;
}

// 0x67F840 (thiscall): the ped's sideways direction (forward x up)
V3* __fastcall Lead_SidewaysImpl(uint8_t* self, re::Edx, V3* out) {
    V3 forward;
    Placeable_GetForward(Ped(self), &forward);
    V3 up{0.0f, 0.0f, 1.0f};
    CrossProduct(out, &forward, &up);
    return out;
}

// 0x67F890 (thiscall, vf3): Update: the state (+0x34: 0 defending with the ball, 1
// waiting for the action, 2 attacking without it, 3 stepping back, 4 choosing) picks the
// action (+0x2C, next +0x30); the action then presses the ped's buttons (its time from
// +0x38 scaled by the ped's stat 20). (The flags' 0x200 cleared on starting an action is
// the local copy's only: never stored.)
uint8_t __fastcall Lead_Update(uint8_t* self, re::Edx, float) {
    if (!Ped(self)) {
        return 0;
    }
    uint32_t flags;
    Dodgeball_GetPlayerFlags(dbGame, &flags, Player(self));
    int other = Dodgeball_TeamCurrent(dbGame, (1 - (Player(self) >= 4)) * 4);
    uint32_t otherFlags;
    Dodgeball_GetPlayerFlags(dbGame, &otherFlags, other);
    uint8_t* myPed = Dodgeball_PlayerPed(dbGame, Player(self));
    Dodgeball_PlayerPed(dbGame, other);
    uint8_t* ped = Ped(self);
    bool hasBall = HasBall(ped);
    uint8_t* inputs = Inputs(ped);
    int32_t& action = At<int32_t>(self, 0x2C);
    int32_t& next = At<int32_t>(self, 0x30);
    int32_t& state = At<int32_t>(self, 0x34);
    uint32_t& start = At<uint32_t>(self, 0x38);

    switch (state) {
    case 0:
        if (!(flags & 0x80) || !hasBall) {
            state = 4;
            break;
        }
        if (next == 6) {
            Lead_RunAtOpponent(self);
            if (Lead_Facing(self)) {
                state = 3;
            }
        } else if (Lead_PassTimer(self)) {
            flags &= ~0x200u;
            action = 5;
        } else {
            Lead_RunAtOpponent(self);
            if (Lead_Facing(self) && action == 0) {
                flags &= ~0x200u;
                action = next;
            } else {
                Lead_Wander(self);
            }
        }
        break;
    case 2:
        Lead_Shadow(self);
        if ((flags & 0x80) || hasBall) {
            state = 4;
            break;
        }
        if (action != 0) {
            break;
        }
        if (!(otherFlags & 0x100)) {
            if (Lead_ThrowTimer(self)) {
                CrtRand();
                flags &= ~0x200u;
                action = 10;
            } else if (Lead_FakeTimer(self)) {
                flags &= ~0x200u;
                action = 0x11;
            }
        } else {
            uint8_t* ball = Dodgeball_GetBall(dbGame);
            if (ball && At<int32_t>(ball, 0xC4)) {
                float reach = 4.0f;
                if (next == 9) {
                    reach = F(Butes(self), 0x38);
                } else if (next == 10 || next == 11) {
                    reach = F(Butes(self), 0x3C);
                } else if (next == 12) {
                    reach = F(Butes(self), 0x40);
                }
                V3 d;
                VectorSub(&d, PosOf(ball), PosOf(myPed));
                float distance = Vector_Magnitude(&d);
                if (reach > distance) {
                    flags &= ~0x200u;
                    action = next != 13 ? next : 0;
                    otherFlags &= ~0x100u;
                    Dodgeball_SetPlayerFlags(dbGame, other, otherFlags);
                }
            }
        }
        break;
    case 3:
        Lead_Wander(self);
        if (Lead_AtTarget(self)) {
            state = 0;
            next = Lead_PickDefence(self);
        }
        [[fallthrough]];
    case 1:
        if (action == 0) {
            state = 4;
        }
        break;
    case 4:
        F(inputs, 0x1C) = 0.0f;
        if (!(flags & 0x80)) {
            state = 2;
            next = Lead_PickAttack(self);
        } else if (hasBall) {
            state = 0;
            next = Lead_PickDefence(self);
        }
        break;
    default:
        break;
    }

    int elapsed = ScaledElapsed(ped, start);
    // the actions' common ends: done once the ball's gone; done when the turn bit was set
    auto doneWithoutBall = [&]() -> uint8_t {
        if (!hasBall) {
            action = 0;
        }
        return 1;
    };
    auto doneOnTurn = [&]() -> uint8_t {
        if (flags & 0x200) {
            action = 0;
        }
        return 1;
    };
    switch (action) {
    case 1:
        Press(inputs, 1);
        return doneWithoutBall();
    case 2:
        if (start == 0) {
            start = dbTimeMs;
            elapsed = 0;
            Press(inputs, 4);
        } else if (elapsed < 2000) {
            Press(inputs, 4);
        }
        if (elapsed > At<int32_t>(Butes(self), 0x64)) {
            Press(inputs, 1);
        }
        if (!hasBall) {
            action = 0;
            start = 0;
        }
        return 1;
    case 3: {
        F(inputs, 0x1C) = 0.0f;
        if (start == 0) {
            start = dbTimeMs;
            elapsed = 0;
        }
        uint8_t playing = Ctl_IsPlaying(Inputs(myPed) + 0x258, dbThrowNode, 1, 1);
        Press(inputs, 4);
        if (!playing) {
            start = dbTimeMs;
        } else if (elapsed > At<int32_t>(Butes(self), 0x60)) {
            Press(inputs, 1);
        }
        if (!hasBall) {
            action = 0;
            start = 0;
        }
        return 1;
    }
    case 4:
    case 16:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 1);
        return doneWithoutBall();
    case 5:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 2);
        return doneWithoutBall();
    case 7:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 1);
        Press(inputs, 6);
        return doneWithoutBall();
    case 8:
    case 14:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 10);
        return doneOnTurn();
    case 9:
        F(inputs, 0x1C) = 0.0f;
        if (!Ctl_IsPlaying(Inputs(myPed) + 0x258, dbCatchNode, 1, 1)) {
            Press(inputs, 4);
        }
        return doneOnTurn();
    case 10:
    case 11: {
        if (start == 0) {
            start = dbTimeMs;
            elapsed = 0;
        }
        F(inputs, 0x1C) = 0.699999988079071f;
        V3 side;
        At<V3>(inputs, 0) = *Lead_Sideways(self, &side);
        if (action == 10) {
            V3& v = At<V3>(inputs, 0);
            v = V3{-v.x, -v.y, -v.z};
        }
        At<V3>(inputs, 0xC) = At<V3>(inputs, 0);
        if (elapsed > 1) {
            Press(inputs, 4);
        }
        return doneOnTurn();
    }
    case 12:
        if ((flags & 0x200) || hasBall) {
            action = 0;
        }
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 1);
        return 1;
    case 15:
        if (!hasBall) {
            action = 0;
            return 1;
        }
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 4);
        return doneOnTurn();
    case 17:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 2);
        if ((flags & 2) && !Dodgeball_IsTeamCurrent(dbGame, Player(self))) {
            return 1;
        }
        action = 0;
        return 1;
    case 18:
    case 19:
        action = 0;
        return 1;
    default:   // (0, 6, 13, and past 19)
        return 1;
    }
}

// ---- fetching a ball --------------------------------------------------------------------

// 0x67FF00 (thiscall, vf2): Start: the base's; the ped's +0x1340 cleared
uint8_t __fastcall GetBall_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    AITrack_Start(self, owner);
    At<int32_t>(Ped(self), 0x1340) = 0;
    return 1;
}

// 0x67FF30 (thiscall): run to the ball
void __fastcall GetBall_RunToBallImpl(uint8_t* self, re::Edx) {
    V3 goal = Copy(PosOf(Dodgeball_GetBall(dbGame)));
    V3 dir;
    VectorSub(&dir, &goal, PosOf(Ped(self)));
    AITrack_RunTo(self, goal, dir, 0.75f);
}

// 0x67FFD0 (thiscall, vf3): Update: run to the ball; running while the player has the
// turn's ball (0x80) and the ped doesn't hold one yet
uint8_t __fastcall GetBall_Update(uint8_t* self, re::Edx, float) {
    if (!Ped(self)) {
        return 0;
    }
    uint32_t flags;
    Dodgeball_GetPlayerFlags(dbGame, &flags, Player(self));
    bool hasBall = HasBall(Ped(self));
    GetBall_RunToBall(self);
    if (!(flags & 0x80)) {
        return 0;
    }
    return !hasBall;
}

// 0x680040 (thiscall, vf4): End (the AI tracks'): the ped's +0x1340 cleared
uint8_t __fastcall AITrack_End(uint8_t* self, re::Edx) {
    At<int32_t>(Ped(self), 0x1340) = 0;
    return 1;
}

// ---- the sideline -----------------------------------------------------------------------

// 0x681770 (thiscall): the sideline lead's action (1..4) by the butes' weights
// +0x90..+0x9C (the last only while the team's count allows)
int __fastcall SidelineLead_PickActionImpl(uint8_t* self, re::Edx) {
    float below = BelowCurrent(self);
    uint8_t* butes = Butes(self);
    float weights[4] = {F(butes, 0x90), F(butes, 0x94), F(butes, 0x98), F(butes, 0x9C) * below};
    return WeightedPick(weights, 4) + 1;
}

// p moved back along n by k (p - k*n)
inline void PushBack(V3* p, const V3& n, float k) {
    V3 v{k * n.x, k * n.y, k * n.z};
    V3 moved;
    VectorSub(&moved, p, &v);
    *p = moved;
}
// the outward normal of the edge a..b of a rectangle on the ground ((a - b) x up, unit)
inline V3 EdgeNormal(const float* a, const float* b) {
    V3 e;
    VectorSub(&e, a, b);
    V3 up{0.0f, 0.0f, 1.0f};
    V3 n;
    e = *CrossProduct(&n, &e, &up);
    Vector_Normalise(&e);
    return e;
}
// the plane constant of the edge through c (n . c, minus 0 as the game does)
inline float EdgeConstant(const V3& n, const float* c) {
    float d = (c[1] * n.y + n.x * c[0]) + n.z * c[2];
    return d - 0.0f;
}

// 0x681980 (thiscall, ret 8): a sideline position kept in bounds: off the centre line
// (this team's side), inside the outer rectangle (+0x18..+0x28), and outside the inner one
// (+0..+0x10: pushed out through its nearest side when inside); on the ground (the
// centre's height). (With no side found while inside, the game reads the "corner" before
// the array: the centre-line normal's y, z and its constant; kept.)
void __fastcall Sideline_ClampImpl(uint8_t* self, re::Edx, V3* p, int) {
    uint8_t* court = Court();
    const float* axisX = &F(court, 0xA8);
    // the stack layout the game indexes: the normal, its constant, the 4 corners
    float frame[16];
    V3& n = *reinterpret_cast<V3*>(&frame[0]);
    float& d0 = frame[3];
    if (Player(self) < 4) {
        n = Copy(axisX);
    } else {
        n = V3{-axisX[0], -axisX[1], -axisX[2]};
    }
    float d = (n.y * F(court, 0xA0) + n.x * F(court, 0x9C)) + F(court, 0xA4) * n.z;
    d0 = d - 0.0f;
    float s = (p->x * n.x + p->y * n.y) + n.z * p->z;
    s = s - d0;
    if (dbCentreMargin > s) {
        PushBack(p, n, s - dbCentreMargin);
    }

    auto corner = [&](int i) -> float* { return &frame[4 + i * 3]; };
    auto setCorners = [&](float x0, float y0, float x1, float y1) {
        float* c = corner(0);
        c[0] = x0, c[1] = y0, c[2] = 0.0f;
        c[3] = x1, c[4] = y0, c[5] = 0.0f;
        c[6] = x1, c[7] = y1, c[8] = 0.0f;
        c[9] = x0, c[10] = y1, c[11] = 0.0f;
    };
    setCorners(F(court, 0x18), F(court, 0x1C), F(court, 0x24), F(court, 0x28));
    for (int i = 0; i < 4; i++) {
        float* c = corner(i);
        V3 e = EdgeNormal(c, corner((i + 1) & 3));
        float edge = EdgeConstant(e, c);
        float dist = (p->y * e.y + p->x * e.x) + e.z * p->z;
        dist = dist - edge;
        if (dbOuterMargin > dist) {
            PushBack(p, e, dist - dbOuterMargin);
        }
    }

    int best = -1;
    uint8_t inside = 1;
    float bestDistance = 99999997952.0f;
    setCorners(F(court, 0x0), F(court, 0x4), F(court, 0xC), F(court, 0x10));
    for (int i = 0; i < 4; i++) {
        float* c = corner(i);
        float* cn = corner((i + 1) & 3);
        V3 sum;
        VectorAdd(&sum, c, cn);
        V3 mid{sum.x * 0.5f, sum.y * 0.5f, sum.z * 0.5f};
        float t = (mid.y * n.y + mid.x * n.x) + mid.z * n.z;
        t = t - d0;
        if (t < -2.0f) {
            continue;   // (the far side's edges)
        }
        V3 e = EdgeNormal(c, cn);
        float edge = EdgeConstant(e, c);
        float dist = (e.x * p->x + e.y * p->y) + e.z * p->z;
        dist = dist - edge;
        if (dbSideMargin > dist) {
            inside = 0;
        }
        if (dbSideMargin < dist && bestDistance > dist) {
            bestDistance = dist;
            best = i;
        }
    }
    if (inside) {
        float* c = corner(best);
        V3 e = EdgeNormal(c, corner((best + 1) & 3));
        float edge = EdgeConstant(e, c);
        float dist = (e.y * p->y + e.x * p->x) + e.z * p->z;
        dist = dist - edge;
        dist = dist - dbSideMargin;
        PushBack(p, e, dist);
    }
    p->z = F(court, 0xA4);
}

// 0x681880 (thiscall, vf2): the sideline player's Start: the base's; a random speed
// between +0x2C and +0x30; a free sideline spot (bits 14..15: the count of the team's
// players out, less one) taken once (0x10000)
uint8_t __fastcall Sideline_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    AITrack_Start(self, owner);
    At<int32_t>(Ped(self), 0x1340) = 0;
    float high = F(self, 0x30);
    float low = F(self, 0x2C);
    float r = Random01();
    At<int32_t>(self, 0x3C) = 0;
    F(self, 0x38) = low + r * (high - low);
    if (!(Dodgeball_Player(dbGame, Player(self))[6] & 1)) {
        int first = (Player(self) >= 4) * 4;
        int count = 0;
        for (int i = first; i < first + 4; i++) {
            if (!(Dodgeball_Player(dbGame, i)[4] & 1)) {
                count++;
            }
        }
        uint32_t& flags = At<uint32_t>(Dodgeball_Player(dbGame, Player(self)), 4);
        flags ^= ((uint32_t(count - 1) << 14) ^ flags) & 0xC000;
        At<uint32_t>(Dodgeball_Player(dbGame, Player(self)), 4) |= 0x10000;
    }
    return 1;
}

// 0x682000 (thiscall, vf2): the sideline lead's Start: the base's; a random aim (0.3..1)
uint8_t __fastcall SidelineLead_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    AITrack_Start(self, owner);
    At<int32_t>(Ped(self), 0x1340) = 0;
    float r = Random01();
    At<int32_t>(self, 0x34) = 0;
    At<int32_t>(self, 0x38) = 0;   // the action
    At<int32_t>(self, 0x3C) = 0;   // its start time
    F(self, 0x30) = r * 0.699999988079071f + 0.30000001192092896f;
    return 1;
}

// 0x682060 (thiscall, vf3): the sideline lead's Update: a new action (+0x38) when none,
// then its buttons; back to 5 (pass) when done
uint8_t __fastcall SidelineLead_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = Ped(self);
    if (!ped) {
        return 0;
    }
    At<int32_t>(ped, 0x1340) = 0;
    int32_t& action = At<int32_t>(self, 0x38);
    uint32_t& start = At<uint32_t>(self, 0x3C);
    int elapsed = ScaledElapsed(Ped(self), start);
    if (action == 0) {
        action = SidelineLead_PickAction(self);
        start = dbTimeMs;
    }
    ped = Ped(self);
    uint8_t* inputs = Inputs(ped);
    bool hasBall = HasBall(ped);
    uint32_t flags;
    Dodgeball_GetPlayerFlags(dbGame, &flags, Player(self));
    switch (action) {
    case 1:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 1);
        if (!hasBall) {
            action = 0;
        }
        break;
    case 2: {
        F(inputs, 0x1C) = 0.0f;
        if (start == 0) {
            start = dbTimeMs;
            elapsed = 0;
        }
        uint8_t playing = Ctl_IsPlaying(Inputs(Ped(self)) + 0x258, dbThrowNode, 1, 1);
        Press(inputs, 4);
        if (!playing) {
            start = dbTimeMs;
        } else if (elapsed > At<int32_t>(Butes(self), 0x60)) {
            Press(inputs, 1);
        }
        if (!hasBall) {
            action = 0;
            start = 0;
        }
        break;
    }
    case 3:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 2);
        if (!hasBall) {
            action = 0;
        }
        break;
    case 4:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 1);
        Press(inputs, 6);
        if (!hasBall) {
            action = 0;
        }
        break;
    case 5:
        F(inputs, 0x1C) = 0.0f;
        Press(inputs, 2);
        if (!(flags & 2) || Dodgeball_IsTeamCurrent(dbGame, Player(self))) {
            action = 0;
        }
        break;
    default:
        break;
    }
    if (action == 0) {
        action = 5;
        start = dbTimeMs;
    }
    return 1;
}

// 0x682280 (thiscall, ret 8): a sideline player's spot by its sideline slot (bits 14..15):
// 0 behind the opponent's current player's end (across from it, scaled), 1 / 2 beside the
// court on either side level with the opponent's current player; kept in bounds
uint8_t __fastcall Sideline_SpotImpl(uint8_t* self, re::Edx, int player, V3* out) {
    uint8_t* court = Court();
    const float* axisX = &F(court, 0xA8);
    const float* axisY = &F(court, 0xB4);
    const float* centre = &F(court, 0x9C);
    V3 d;
    VectorSub(&d, court + 0xC, court);
    float lenX = (F(court, 0xAC) * d.y + d.x * F(court, 0xA8)) + F(court, 0xB0) * d.z;
    float negLenX = -lenX;
    float lenY = (d.x * F(court, 0xB4) + F(court, 0xB8) * d.y) + F(court, 0xBC) * d.z;
    V3 opponent = Copy(PosOf(Dodgeball_PlayerPed(dbGame, Dodgeball_OtherTeamCurrent(dbGame, player))));
    V3 own = Copy(PosOf(Dodgeball_PlayerPed(dbGame, player)));
    uint32_t spot = (At<uint32_t>(Dodgeball_Player(dbGame, Player(self)), 4) >> 14) & 3;
    float side = Player(self) >= 4 ? -0.6800000071525574f : 1.0f;
    V3 e;
    V3 sum;
    V3 result;
    if (spot == 0) {
        VectorSub(&e, &opponent, centre);
        VectorSub(&e, &opponent, centre);
        float proj = (F(court, 0xB8) * e.y + F(court, 0xB4) * e.x) + F(court, 0xBC) * e.z;
        VectorSub(&e, &own, centre);
        float limit = -lenY * 0.5f - dbSidelineMargin;
        float negLimit = -limit;
        if (negLimit > proj) {
            proj = negLimit;
        } else if (proj > limit) {
            proj = limit;
        }
        float k = -proj * dbSidelineScale;
        V3 alongY = Scaled(axisY, k);
        float t = negLenX * 0.5f + 0.10000000149011612f;
        V3 alongX = Scaled(axisX, t);
        alongX = Scaled(&alongX.x, side);
        VectorAdd(&sum, centre, &alongX);
        VectorAdd(&result, &sum, &alongY);
    } else {
        VectorSub(&e, &opponent, centre);
        float proj = (F(court, 0xAC) * e.y + F(court, 0xA8) * e.x) + F(court, 0xB0) * e.z;
        VectorSub(&e, &opponent, centre);
        VectorSub(&e, &own, centre);
        float t = lenY * 0.5f + 0.10000000149011612f;
        V3 alongY = Scaled(axisY, t);
        V3 alongX = Scaled(axisX, proj);
        VectorAdd(&sum, centre, &alongX);
        if (spot == 1) {
            VectorAdd(&result, &sum, &alongY);
        } else {
            VectorSub(&result, &sum, &alongY);
        }
    }
    *out = result;
    Sideline_Clamp(self, out, 1);
    return 1;
}

// 0x682720 (thiscall, vf3): the sideline player's Update: walk to its spot (the speed by
// the distance, 0..0.7); with a ball, pass it after half a second
uint8_t __fastcall Sideline_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = Ped(self);
    if (!ped) {
        return 0;
    }
    At<int32_t>(ped, 0x1340) = 0;
    V3 target{0.0f, 0.0f, 0.0f};
    Sideline_Spot(self, Player(self), &target);
    V3 d;
    VectorSub(&d, &target, PosOf(ped));
    float distance = std::sqrt(std::fabs((d.x * d.x + d.y * d.y) + d.z * d.z));
    V3 lead = Copy(PosOf(Dodgeball_PlayerPed(dbGame, Dodgeball_TeamCurrent(dbGame, Player(self)))));
    V3 toLead;
    VectorSub(&toLead, &lead, PosOf(Ped(self)));
    bool hasBall = HasBall(Ped(self));
    Vector_Normalise(&toLead);
    float speed = distance * 0.25f * 0.699999988079071f;
    if (speed < 0.0f) {
        speed = 0.0f;
    } else if (0.699999988079071f < speed) {
        speed = 0.699999988079071f;
    }
    uint8_t team = uint8_t(Dodgeball_TeamCode(dbGame, ped));
    PedMove_To(ped + 0x654, target, 0x18, speed, F(self, 0x38), 0, team);
    uint32_t& since = At<uint32_t>(self, 0x3C);
    if (hasBall) {
        if (since == 0) {
            since = dbTimeMs;
        }
        if (since == 0) {
            return 1;
        }
        if (since + 500 > dbTimeMs) {
            Press(Inputs(ped), 2);
            return 1;
        }
    }
    since = 0;
    return 1;
}

// ---- the supports and the small tracks --------------------------------------------------

// the walking speed for a distance: a third of it times 0.7, clamped to 0..0.7 (the third
// a double constant: one rounding in the game's x87)
inline float WalkSpeed(float distance) {
    float speed = float(double(distance) * 0.3333333333333333) * 0.699999988079071f;
    if (speed < 0.0f) {
        speed = 0.0f;
    } else if (0.699999988079071f < speed) {
        speed = 0.699999988079071f;
    }
    return speed;
}

// 0x682A40 (thiscall, vf2): the support's Start: the base's; a random speed (0.75..2)
uint8_t __fastcall Support_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    AITrack_Start(self, owner);
    At<int32_t>(Ped(self), 0x1340) = 0;
    F(self, 0x30) = Random01() * 1.25f + 0.75f;
    return 1;
}

// 0x682A90 (thiscall, ret 8; `this` unused): a support player's spot: the team's spot
// points (the court's +0x30 / +0x60, four per team) relative to the first: its own spot
// (flags bits 17..18) when three or more are in and the current player is out, else the
// one left or right of the current player; mirrored across when the opponent's current
// player is out; around the current player (moved to the court's scaled cross position
// when three or more are in and it is out); kept inside the inner rectangle's margins and
// behind the player's line
uint8_t __fastcall Support_SpotImpl(uint8_t*, re::Edx, int player, V3* out) {
    uint8_t* court = Court();
    const float* axisX = &F(court, 0xA8);
    const float* axisY = &F(court, 0xB4);
    const float* centre = &F(court, 0x9C);
    int team = player >= 4;
    int current = Dodgeball_TeamCurrent(dbGame, player);
    uint32_t flags;
    uint8_t otherIn = *Dodgeball_GetPlayerFlags(dbGame, &flags, Dodgeball_OtherTeamCurrent(dbGame, player)) & 1;
    uint8_t currentIn = *Dodgeball_GetPlayerFlags(dbGame, &flags, current) & 1;
    int countIn = PlayersIn_Count(team);
    bool otherOut = otherIn == 0;
    V3 currentPos = Copy(PosOf(Dodgeball_PlayerPed(dbGame, current)));
    Dodgeball_PlayerPed(dbGame, player);
    V3 offset{0.0f, 0.0f, 0.0f};
    int local = player - team * 4;
    int currentLocal = current - team * 4;
    uint8_t front = 0;
    auto point = [&](int i) { return reinterpret_cast<const float*>(court + i * 12); };
    const float* teamBase = reinterpret_cast<const float*>(court + (team * 3 + 3) * 16);
    V3 tmp;
    if (countIn >= 3 && !currentIn) {
        uint32_t spot = (*Dodgeball_GetPlayerFlags(dbGame, &flags, player) >> 17) & 3;
        offset = *VectorSub(&tmp, point(team * 4 + 4 + spot), teamBase);
        front = spot == 0;
    } else {
        int k = 0;
        for (int step = 1; step < 4; step++) {
            k = (step + currentLocal) % 4;
            if (local == k) {
                continue;
            }
            uint32_t other;
            if (*Dodgeball_GetPlayerFlags(dbGame, &other, team * 4 + k) & 1) {
                break;
            }
        }
        offset = *VectorSub(&tmp, point(team * 4 + 5 + (local > k)), teamBase);
    }
    if (otherOut) {
        float pY = (F(court, 0xB8) * offset.y + offset.x * F(court, 0xB4)) + F(court, 0xBC) * offset.z;
        float pX = (offset.x * F(court, 0xA8) + F(court, 0xAC) * offset.y) + F(court, 0xB0) * offset.z;
        float negX = -pX;
        V3 alongY = Scaled(axisY, pY);
        V3 alongX = Scaled(axisX, negX);
        offset = *VectorAdd(&tmp, &alongX, &alongY);
    }
    V3 pos = currentPos;
    if (countIn > 2 && !currentIn) {
        V3 e;
        VectorSub(&e, &pos, centre);
        float pX = (e.y * F(court, 0xAC) + e.x * F(court, 0xA8)) + e.z * F(court, 0xB0);
        VectorSub(&e, &pos, centre);
        float pY = (e.y * F(court, 0xB8) + e.x * F(court, 0xB4)) + e.z * F(court, 0xBC);
        pY = pY * dbSupportScale;
        V3 alongY = Scaled(axisY, pY);
        V3 alongX = Scaled(axisX, pX);
        V3 sum;
        VectorAdd(&sum, centre, &alongX);
        pos = *VectorAdd(&currentPos, &sum, &alongY);
    }
    *out = *VectorAdd(&tmp, &pos, &offset);

    // inside the inner rectangle, off its sides
    float corners[12] = {F(court, 0x0), F(court, 0x4), 0.0f, F(court, 0xC), F(court, 0x4), 0.0f,
                         F(court, 0xC), F(court, 0x10), 0.0f, F(court, 0x0), F(court, 0x10), 0.0f};
    for (int i = 0; i < 4; i++) {
        const float* c = &corners[i * 3];
        V3 e = EdgeNormal(c, &corners[((i + 1) & 3) * 3]);
        float edge = EdgeConstant(e, c);
        float dist = (e.x * out->x + e.y * out->y) + out->z * e.z;
        dist = dist - edge;
        if (dbSupportMargin > dist) {
            PushBack(out, e, dist - dbSupportMargin);
        }
    }

    // behind the line some way back from the centre towards the team's end
    float back = front ? dbSupportBackFirst : dbSupportBack;
    V3 n = team ? Copy(axisX) : V3{-axisX[0], -axisX[1], -axisX[2]};
    V3 shift{n.x * back, n.y * back, n.z * back};
    V3 on;
    VectorAdd(&on, centre, &shift);
    float d = (on.y * n.y + on.x * n.x) + on.z * n.z;
    float d0 = d - 0.0f;
    float s = (n.y * out->y + n.x * out->x) + out->z * n.z;
    s = s - d0;
    if (s < 0.0f) {
        PushBack(out, n, s);
    }
    out->z = F(court, 0xA4);
    return 1;
}

// 0x683210 (thiscall, vf3): the support's Update: walk to its spot
uint8_t __fastcall Support_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = Ped(self);
    if (!ped) {
        return 0;
    }
    At<int32_t>(ped, 0x1340) = 0;
    V3 target{0.0f, 0.0f, 0.0f};
    Support_Spot(self, Player(self), &target);
    V3 d;
    VectorSub(&d, &target, PosOf(ped));
    float speed = WalkSpeed(std::sqrt(std::fabs((d.x * d.x + d.y * d.y) + d.z * d.z)));
    uint32_t team = Dodgeball_TeamCode(dbGame, ped);
    PedMove_To(ped + 0x654, target, 0x18, speed, F(self, 0x30), 0, team);
    return 1;
}

// 0x682960 (thiscall, vf2): the face-lead track's Start: the base's; the team's current
// player kept (+0x30)
uint8_t __fastcall FaceLead_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    AITrack_Start(self, owner);
    At<int32_t>(self, 0x30) = Dodgeball_TeamCurrent(dbGame, Player(self));
    return 1;
}

// 0x683340 (thiscall, vf3): turn to face the team's current player (the stick at 0.3,
// released once within about 18 degrees; then done when +0x2C says so)
uint8_t __fastcall FaceLead_Update(uint8_t* self, re::Edx, float) {
    V3 lead = Copy(PosOf(Dodgeball_PlayerPed(dbGame, At<int32_t>(self, 0x30))));
    V3 forward;
    Placeable_GetForward(Ped(self), &forward);
    V3 dir;
    VectorSub(&dir, &lead, PosOf(Ped(self)));
    forward.z = 0.0f;
    Vector_Normalise(&forward);
    dir.z = 0.0f;
    Vector_Normalise(&dir);
    float dot = (forward.y * dir.y + forward.x * dir.x) + dir.z * forward.z;
    At<V3>(Inputs(Ped(self)), 0) = dir;
    At<V3>(Inputs(Ped(self)), 0xC) = dir;
    uint8_t* inputs = Inputs(Ped(self));
    if (dot > 0.949999988079071f) {
        F(inputs, 0x1C) = 0.0f;
        if (self[0x2C]) {
            return 0;
        }
        return 1;
    }
    F(inputs, 0x1C) = 0.30000001192092896f;
    return 1;
}

// 0x6829D0 (thiscall, vf2): the catch track's Start: the base's
uint8_t __fastcall CatchBall_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    AITrack_Start(self, owner);
    return 1;
}

// 0x683480 (thiscall, vf3): the catch track's Update: stand facing the ball; catch
// (button 1) when the ball in flight (+0xC4) is above 0.3, near (+0x30, now or next
// frame), coming head-on (within +0x34 degrees) and its speed passes the test (+0x38,
// +0x3C), unless the player's flag 0x10 is set. Done when the ball lands, if +0x2C says
// so. (It "runs" to its own position: RunTo turns it to the ball.)
uint8_t __fastcall CatchBall_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ball = Dodgeball_GetBall(dbGame);
    if (!ball) {
        return 1;
    }
    ActionInputs_Clear(Inputs(Ped(self)));
    V3 ballPos = Copy(PosOf(ball));
    V3 own = Copy(PosOf(Ped(self)));
    V3 d;
    VectorSub(&d, &ballPos, &own);
    AITrack_RunTo(self, own, d, 0.1f);
    V3 velocity = At<V3>(ball, 0x124);
    float step = actc::timeStep;
    V3 move{velocity.x * step, velocity.y * step, step * velocity.z};
    V3 next;
    VectorAdd(&next, &ballPos, &move);
    V3 forward;
    Placeable_GetForward(Ped(self), &forward);
    float speed = std::sqrt(std::fabs((velocity.y * velocity.y + velocity.x * velocity.x) + velocity.z * velocity.z));
    forward.z = 0.0f;
    velocity.z = 0.0f;
    Vector_Normalise(&forward);
    Vector_Normalise(&velocity);
    float ballHeading = VecHeading(&velocity);
    float facing = VecHeading(&forward);
    float away = std::fabs(AngleDiff(facing, ballHeading));
    float headOn = 3.1415927410125732f - away;
    uint8_t* court = Court();
    uint32_t flags;
    uint8_t busy = uint8_t((*Dodgeball_GetPlayerFlags(dbGame, &flags, Player(self)) >> 4) & 1);
    VectorSub(&d, &ballPos, &own);
    float distance = std::sqrt(std::fabs(d.x * d.x + d.y * d.y));
    uint8_t near;
    if (F(self, 0x30) > distance) {
        near = 1;
    } else {
        VectorSub(&d, &next, &own);
        distance = std::sqrt(std::fabs(d.x * d.x + d.y * d.y));
        near = F(self, 0x30) > distance;
    }
    uint8_t coming = F(self, 0x34) > headOn * 180.0f / 3.1415927410125732f;
    uint8_t fastEnough = actc::CompareOp(speed, F(self, 0x38), At<int32_t>(self, 0x3C));
    uint8_t high = At<int32_t>(ball, 0xC4) != 0 && ballPos.z - F(court, 0xA4) > 0.30000001192092896f;
    if (!busy && near && coming && high && fastEnough) {
        Press(Inputs(Ped(self)), 1);
    }
    if (At<int32_t>(ball, 0xC4) != 0) {
        return 1;
    }
    return self[0x2C] == 0;
}

// 0x6837F0 (thiscall, vf2): the goto track's Start: the base's; speed 0.75; the ped's
// buttons cleared
uint8_t __fastcall Goto_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    AITrack_Start(self, owner);
    At<int32_t>(Ped(self), 0x1340) = 0;
    F(self, 0x30) = 0.75f;
    ActionInputs_Clear(Inputs(Ped(self)));
    return 1;
}

// 0x683830 (thiscall, vf3): the goto track's Update: walk to the team's first spot point
// (the court's +0x30 / +0x60); running while the move does
uint8_t __fastcall Goto_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = Ped(self);
    if (!ped) {
        return 0;
    }
    At<int32_t>(ped, 0x1340) = 0;
    V3 target{0.0f, 0.0f, 0.0f};
    target = Copy(reinterpret_cast<const float*>(Court() + ((Player(self) >= 4) * 3 + 3) * 16));
    V3 d;
    VectorSub(&d, &target, PosOf(ped));
    float speed = WalkSpeed(std::sqrt(std::fabs((d.x * d.x + d.y * d.y) + d.z * d.z)));
    uint32_t team = Dodgeball_TeamCode(dbGame, ped);
    return PedMove_To(ped + 0x654, target, 0x18, speed, F(self, 0x30), 0, team) != 0;
}

// 0x683990 (thiscall, vf2): the idle track's Start: the base's; the ped's buttons cleared
uint8_t __fastcall Idle_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    AITrack_Start(self, owner);
    At<int32_t>(Ped(self), 0x1340) = 0;
    ActionInputs_Clear(Inputs(Ped(self)));
    return 1;
}

// 0x6839D0 (thiscall, vf3): the idle track's Update: the ped's buttons cleared
uint8_t __fastcall Idle_Update(uint8_t* self, re::Edx, float) {
    ActionInputs_Clear(Inputs(Ped(self)));
    return 1;
}

// The constructors' tails (after the track base's constructor 0x61C320) and the
// destructors (the vtable, then the base AI track's destructor)
// 0x682940: the face-lead track (+0x2C done-when-facing off)
void __fastcall FaceLead_Construct(uint8_t* self, re::Edx) {
    TrackBase_Construct(self);
    self[0x2C] = 0;
}
// 0x682990: the catch track (reach 10, head-on 270 degrees, speed test 0.01 by op 5)
void __fastcall CatchBall_Construct(uint8_t* self, re::Edx) {
    TrackBase_Construct(self);
    F(self, 0x30) = 10.0f;
    self[0x2C] = 0;
    At<int32_t>(self, 0x3C) = 5;
    F(self, 0x34) = 270.0f;
    F(self, 0x38) = 0.009999999776482582f;
}
// 0x6829F0: the goto track (keeps alive, +0x2C set)
void __fastcall Goto_Construct(uint8_t* self, re::Edx) {
    TrackBase_Construct(self);
    self[8] = 1;
    self[0x2C] = 1;
}
// 0x681750: the sideline track (its speed range 0.3..1)
void __fastcall Sideline_Construct(uint8_t* self, re::Edx) {
    TrackBase_Construct(self);
    F(self, 0x2C) = 0.30000001192092896f;
    F(self, 0x30) = 1.0f;
}
// 0x682A20: the idle track (keeps alive)
void __fastcall Idle_Construct(uint8_t* self, re::Edx) {
    TrackBase_Construct(self);
    self[8] = 1;
}
// 0x682950 / 0x6829C0 / 0x682A10 / 0x682A30
template <uintptr_t Vtable>
void __fastcall AITrack_DestructAs(uint8_t* self, re::Edx) {
    At<uint32_t>(self, 0) = uint32_t(Vtable);
    AITrack_Destruct(self);
}

#define DBAI_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

DBAI_REVERSED("DodgeballLeadAITrack::Start", 0x0067EA50, &Lead_Start);
DBAI_REVERSED("DodgeballLeadAITrack::PickAttack", 0x0067EAA0, &Lead_PickAttackImpl);
DBAI_REVERSED("DodgeballLeadAITrack::CourtTarget", 0x0067EB50, &Lead_CourtTargetImpl);
DBAI_REVERSED("DodgeballLeadAITrack::PickDefence", 0x0067EF10, &Lead_PickDefenceImpl);
DBAI_REVERSED("DodgeballLeadAITrack::Shadow", 0x0067F120, &Lead_ShadowImpl);
DBAI_REVERSED("DodgeballLeadAITrack::RunAtOpponent", 0x0067F320, &Lead_RunAtOpponentImpl);
DBAI_REVERSED("DodgeballLeadAITrack::Wander", 0x0067F3C0, &Lead_WanderImpl);
DBAI_REVERSED("DodgeballLeadAITrack::ThrowTimer", 0x0067F4B0, (&Lead_Deadline<0x2C, 0x4C, true>));
DBAI_REVERSED("DodgeballLeadAITrack::FakeTimer", 0x0067F530, (&Lead_Deadline<0x30, 0x50, true>));
DBAI_REVERSED("DodgeballLeadAITrack::Facing", 0x0067F5B0, &Lead_FacingImpl);
DBAI_REVERSED("DodgeballLeadAITrack::AtTarget", 0x0067F720, &Lead_AtTargetImpl);
DBAI_REVERSED("DodgeballLeadAITrack::PassTimer", 0x0067F7B0, (&Lead_Deadline<0xAC, 0x50, false>));
DBAI_REVERSED("DodgeballLeadAITrack::Sideways", 0x0067F840, &Lead_SidewaysImpl);
DBAI_REVERSED("DodgeballLeadAITrack::Update", 0x0067F890, &Lead_Update);
DBAI_REVERSED("DodgeballGetBallAITrack::Start", 0x0067FF00, &GetBall_Start);
DBAI_REVERSED("DodgeballGetBallAITrack::RunToBall", 0x0067FF30, &GetBall_RunToBallImpl);
DBAI_REVERSED("DodgeballGetBallAITrack::Update", 0x0067FFD0, &GetBall_Update);
DBAI_REVERSED("DodgeballBaseAITrack::End_680040", 0x00680040, &AITrack_End);
DBAI_REVERSED("DodgeballSidelineLeadAITrack::PickAction", 0x00681770, &SidelineLead_PickActionImpl);
DBAI_REVERSED("DodgeballSidelineAITrack::Construct_681750", 0x00681750, &Sideline_Construct);
DBAI_REVERSED("DodgeballSidelineAITrack::Start", 0x00681880, &Sideline_Start);
DBAI_REVERSED("DodgeballSidelineAITrack::Clamp", 0x00681980, &Sideline_ClampImpl);
DBAI_REVERSED("DodgeballSidelineLeadAITrack::Start", 0x00682000, &SidelineLead_Start);
DBAI_REVERSED("DodgeballSidelineLeadAITrack::Update", 0x00682060, &SidelineLead_Update);
DBAI_REVERSED("DodgeballSidelineAITrack::Spot", 0x00682280, &Sideline_SpotImpl);
DBAI_REVERSED("DodgeballSidelineAITrack::Update", 0x00682720, &Sideline_Update);
DBAI_REVERSED("DodgeballFaceLeadTrack::Construct_682940", 0x00682940, &FaceLead_Construct);
DBAI_REVERSED("DodgeballFaceLeadTrack::Destruct_682950", 0x00682950, &AITrack_DestructAs<0x0093DD38>);
DBAI_REVERSED("DodgeballFaceLeadTrack::Start", 0x00682960, &FaceLead_Start);
DBAI_REVERSED("DodgeballCatchBallTrack::Construct_682990", 0x00682990, &CatchBall_Construct);
DBAI_REVERSED("DodgeballCatchBallTrack::Destruct_6829C0", 0x006829C0, &AITrack_DestructAs<0x0093DD50>);
DBAI_REVERSED("DodgeballCatchBallTrack::Start", 0x006829D0, &CatchBall_Start);
DBAI_REVERSED("DodgeballGotoAITrack::Construct_6829F0", 0x006829F0, &Goto_Construct);
DBAI_REVERSED("DodgeballGotoAITrack::Destruct_682A10", 0x00682A10, &AITrack_DestructAs<0x0093DD80>);
DBAI_REVERSED("DodgeballIdleAITrack::Construct_682A20", 0x00682A20, &Idle_Construct);
DBAI_REVERSED("DodgeballIdleAITrack::Destruct_682A30", 0x00682A30, &AITrack_DestructAs<0x0093DD98>);
DBAI_REVERSED("DodgeballSupportAITrack::Start", 0x00682A40, &Support_Start);
DBAI_REVERSED("DodgeballSupportAITrack::Spot", 0x00682A90, &Support_SpotImpl);
DBAI_REVERSED("DodgeballSupportAITrack::Update", 0x00683210, &Support_Update);
DBAI_REVERSED("DodgeballFaceLeadTrack::Update", 0x00683340, &FaceLead_Update);
DBAI_REVERSED("DodgeballCatchBallTrack::Update", 0x00683480, &CatchBall_Update);
DBAI_REVERSED("DodgeballGotoAITrack::Start", 0x006837F0, &Goto_Start);
DBAI_REVERSED("DodgeballGotoAITrack::Update", 0x00683830, &Goto_Update);
DBAI_REVERSED("DodgeballIdleAITrack::Start", 0x00683990, &Idle_Start);
DBAI_REVERSED("DodgeballIdleAITrack::Update", 0x006839D0, &Idle_Update);

} // namespace
