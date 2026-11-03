#pragma once

// CGameLogic (g_GameLogic at 0xBD1078, "g_GameLogic.Init()" on the loading screen): where
// the player comes back after being busted/knocked out/sleeping. Three lists of 60
// restart points (the scripts add them, 0x5BE790 on), each with a default point, a
// one-off override for two of them, and the respawn itself (GTA's CRestart and
// CGameLogic::RestorePlayerStuffDuringResurrection, in one class).
// The lists by restart reason (CWorld::Players' byte at 0xC1AF00):
//   1, 5, 6 -> list 0 (override +0x2DD8), 2, 4 -> list 1 (override +0x2DC0), 7 -> list 2;
//   3: where the player stands.

#include <cstddef>
#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

struct CRestartPoint {
    col::CVector pos;       // +0 (unused: CGameLogic::ms_unusedPoint)
    float heading;          // +0xC degrees
    int32_t area;           // +0x10
    int32_t hourFrom;       // +0x14 used from this hour...
    int32_t hourTo;         // +0x18 ...to before this one (wrapping past midnight)
    col::CVector altPos;    // +0x1C used instead when altArea isn't -1
    float altHeading;       // +0x28
    int32_t altArea;        // +0x2C
    int32_t fromArea;       // +0x30 -1: any; else only for a player in that area
    col::CVector lookAt;    // +0x34 (handed back with the point)
};
static_assert(offsetof(CRestartPoint, altPos) == 0x1C);
static_assert(offsetof(CRestartPoint, fromArea) == 0x30);
static_assert(sizeof(CRestartPoint) == 0x40);

// A point set for the next restart only (0x18 bytes)
struct CRestartOverride {
    uint8_t active;         // +0
    uint8_t pad1[3];
    col::CVector pos;       // +4
    float heading;          // +0x10 radians already
    int32_t area;           // +0x14
};
static_assert(sizeof(CRestartOverride) == 0x18);

struct CGameLogic {
    CRestartPoint points[3][60];    // +0
    CRestartPoint defaults[3];      // +0x2D00 used when no point of the list fits
    CRestartOverride override1;     // +0x2DC0 (list 1)
    CRestartOverride override0;     // +0x2DD8 (list 0)
    uint32_t scriptCallback;        // +0x2DF0 a script function to call on the restart
    uint8_t groundPending;          // +0x2DF4 the respawn is to find the ground itself
    uint8_t fading;                 // +0x2DF5 a restart is under way (the fade out)
    uint8_t clockSet;               // +0x2DF6 the clock was moved on for this restart
    uint8_t pad2DF7;

    // 0x4315C0: the points zeroed, then Init (the static constructor's)
    CGameLogic* Construct();
    // 0x430640
    void Init();
    // 0x430840 / 0x430920 (with the area the player must be in)
    void AddPoint0(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                   const col::CVector* altPos, float altHeading, int altArea);
    void AddPoint0InArea(int fromArea, const col::CVector* pos, float heading, int area, int hourFrom,
                         int hourTo, const col::CVector* altPos, float altHeading, int altArea);
    // 0x430A00 / 0x430F30: the point at `pos` freed
    void RemovePoint0(const col::CVector* pos);
    void RemovePoint1(const col::CVector* pos);
    // 0x430AD0 (fromArea left as it was) / 0x430E20 / 0x431030
    void AddPoint1(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                   const col::CVector* altPos, float altHeading, int altArea);
    void AddPoint1InArea(int fromArea, const col::CVector* pos, float heading, int area, int hourFrom,
                         int hourTo, const col::CVector* altPos, float altHeading, int altArea);
    void AddPoint2InArea(int fromArea, const col::CVector* pos, float heading, int area, int hourFrom,
                         int hourTo, const col::CVector* altPos, float altHeading, int altArea);
    // 0x430BE0 / 0x430C70 / 0x430D00: the look-at of the points at `pos`
    void SetLookAt0(const col::CVector* pos, const col::CVector* lookAt);
    void SetLookAt1(const col::CVector* pos, const col::CVector* lookAt);
    void SetLookAt2(const col::CVector* pos, const col::CVector* lookAt);
    // 0x430D90 / 0x430DC0 / 0x430DF0
    void SetDefaultLookAt0(const col::CVector* lookAt);
    void SetDefaultLookAt1(const col::CVector* lookAt);
    void SetDefaultLookAt2(const col::CVector* lookAt);
    // 0x431140 / 0x4311F0 / 0x431290 (only the first sets fromArea)
    void SetDefault0(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                     const col::CVector* altPos, float altHeading, int altArea);
    void SetDefault1(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                     const col::CVector* altPos, float altHeading, int altArea);
    void SetDefault2(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                     const col::CVector* altPos, float altHeading, int altArea);
    // 0x431330: the player was busted (flags as the scripts' arguments): the missions
    // told, the script callback run, the fade started
    void OnPlayerDown(int knockedOut, int flag2, int asleep);
    // 0x431530 / 0x431570 / 0x4315B0
    void SetOverride1(const col::CVector* pos, float heading, int area);
    void SetOverride0(const col::CVector* pos, float heading, int area);
    void ClearOverrides();
    // 0x431C90: where to put the player back (heading in radians)
    void GetRestartPoint(int forced, col::CVector* pos, float* heading, int* area, col::CVector* lookAt);
    // 0x432320: the player put back at the restart point
    void RestorePlayer(int forced, col::CVector* lookAt);
    // 0x432690: once the fade is done, the respawn
    void Process();
};
static_assert(offsetof(CGameLogic, defaults) == 0x2D00);
static_assert(offsetof(CGameLogic, override1) == 0x2DC0);
static_assert(offsetof(CGameLogic, override0) == 0x2DD8);
static_assert(offsetof(CGameLogic, scriptCallback) == 0x2DF0);
static_assert(sizeof(CGameLogic) == 0x2DF8);

namespace CGameLogicData {
GAME_VAR(CGameLogic, g_GameLogic, 0x00BD1078);
GAME_VAR(col::CVector, ms_unusedPoint, 0x009F477C);   // (9999, 9999, 9999)
GAME_VAR(uint8_t, ms_restartReason, 0x00C1AF00);      // CWorld::Players[0]'s
} // namespace CGameLogicData

// 0x431770 (stdcall): the first point of a list that fits the player: one for the area
// he's in, else one whose zone (carnival, business, industrial, poor, rich - but not
// the carnival within it -, school) is his. False: none.
uint8_t __stdcall FindRestartPoint(const CRestartPoint* points, int* index);

// 0x4314F0 (stdcall): the restart reason from the scripts' flags
void __fastcall SetRestartReason(void*, re::Edx, int flag4, int flag5, int flag6);   // (thiscall, `this` unused)
