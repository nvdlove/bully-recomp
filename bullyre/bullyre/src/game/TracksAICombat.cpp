// The action tree's AI tracks of 0x61A8C0..0x61C760: their field initialisers,
// destructors and small virtuals (AI update, controller, wrestle, warn target, wander /
// ride around nodes, flee, hang out, POIs, the player's physical state, weapons, path,
// dock, greet / teacher / objectives, combat and pursuit locomotion, attack, action
// requests, block, projectile cover, rat locomotion, target player, evading), and the
// path, combat locomotion, projectile cover and rat locomotion helpers.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

constexpr uint32_t kVtAIUpdateTrack = 0x009333CC;
constexpr uint32_t kVtControllerTrack = 0x009333E4;
constexpr uint32_t kVtWrestleTrack = 0x009333FC;
constexpr uint32_t kVtFleeTrack = 0x00933414;
constexpr uint32_t kVtGreetTrack = 0x00933430;
constexpr uint32_t kVtTeacherTrack = 0x00933448;
constexpr uint32_t kVtMoveObjectiveTrack = 0x00933460;
constexpr uint32_t kVtFaceObjectiveTrack = 0x00933478;
constexpr uint32_t kVtPursuitLocoTrack = 0x009334AC;
constexpr uint32_t kVtAttackTrack = 0x009334C4;
constexpr uint32_t kVtActionNodeRequestTrack = 0x009334DC;

GAME_VAR(int32_t, controllerLockA, 0x00C2B120);
GAME_VAR(int32_t, controllerLockB, 0x00C2B124);
GAME_VAR(uint8_t, playerPhysicalState, 0x00C1AF4C);   // (the player info's +0xA4)
inline void* const playerInfo = reinterpret_cast<void*>(0x00C1AEA8);
inline void* const pathNodes = reinterpret_cast<void*>(0x00BF6228);
inline void* const coverPoints = reinterpret_cast<void*>(0x00C2B210);
GAME_VAR(void*, patrolPaths, 0x00D13CB4);
GAME_VAR(uint8_t*, buttonHistory, 0x00C674F4);
GAME_VAR(uint8_t*, stimuli, 0x00C2C1A0);
GAME_VAR(float, combatStepRate, 0x00B0C0B0);
GAME_VAR(float, combatMaxAngle, 0x00B0C0A0);
GAME_VAR(float, ratTurnScale, 0x00B0C214);
GAME_VAR(float, ratWobble, 0x00B0C210);
GAME_VAR(float, ratFoodRange, 0x00B0C218);
GAME_VAR(uint32_t, pickupsInRangeCount, 0x00C0A0A8);
GAME_ARRAY(int32_t, pickupsInRange, 0x00C0A0B0);
inline uint8_t* const pickups = reinterpret_cast<uint8_t*>(0x00C0A100);   // 0x48 bytes each

GAME_FN(int32_t, __cdecl, Pad_GetActive, 0x00738070);
GAME_FN(uint8_t, __thiscall, PathNode_IsValid, 0x0043F8C0, const uint8_t* node);
GAME_FN(V3*, __thiscall, PathNodes_LaneOffset, 0x00441790, void* nodes, V3* out, const uint8_t* a, const uint8_t* b);
GAME_FN(void, __thiscall, PlayerInfo_SetPhysicalState, 0x00445E30, void* info, int state);
GAME_FN(void, __thiscall, PlayerInfo_RefreshPhysicalState, 0x00446B40, void* info);
GAME_FN(uint8_t*, __thiscall, PatrolPath_Get, 0x005ED3A0, void* pool, int32_t id);
GAME_FN(int32_t, __thiscall, Path_GetNumNodes, 0x005EC670, uint8_t* path);
GAME_FN(float*, __thiscall, Path_GetNode, 0x005EC8C0, uint8_t* path, int32_t node);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, int n);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const void* a, const void* b);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const void* a, const void* b);
GAME_FN(V3*, __cdecl, FlatDirection, 0x0041A520, V3* out, const V3* v, float length);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(void, __thiscall, Ped_SetMoveTarget, 0x00474A00, uint8_t* ped, const float* pos);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(V3*, __cdecl, HeadingToVector, 0x00432DF0, V3* out, float heading);
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(uint8_t, __thiscall, Ped_IsInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const float* a, const float* b, float range);
GAME_FN(void, __thiscall, ButtonHistory_SetScriptedInput, 0x0053B490, uint8_t* history, int32_t a, int32_t b);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint8_t, __thiscall, CoverPoints_Has, 0x004883E0, void* cover, uint8_t* point, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, CoverPoints_Choose, 0x00488550, void* cover, uint8_t* ped);
GAME_FN(void, __thiscall, Timer_Reset, 0x0045BD10, uint8_t* timer);
GAME_FN(void, __thiscall, Timer_Set, 0x0045BD70, uint8_t* timer, float seconds);
GAME_FN(uint8_t, __thiscall, Timer_IsRunning, 0x0045BDB0, uint8_t* timer);
GAME_FN(uint8_t, __thiscall, Timer_IsDone, 0x0045BDD0, uint8_t* timer);
GAME_FN(void, __thiscall, Timer_Start, 0x0045BDF0, uint8_t* timer, float least, float most);
GAME_FN(void, __thiscall, EntityRef_Set, 0x005D7F10, re::Ptr32<uint8_t>* ref, uint8_t* entity);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, uint8_t* inputs, int button, int kind, float time);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(void, __cdecl, Pickups_FindInRange, 0x004440F0, const float* pos, float range, int);
GAME_FN(uint8_t*, __thiscall, Stimuli_Find, 0x004A6150, uint8_t* stimuli, uint8_t* ped, int32_t type, int32_t);
// the path track's own (by address, so their modes apply)
GAME_FN(uint8_t, __thiscall, PathTrack_Step, 0x0061AE20, uint8_t* self, int32_t* index);
GAME_FN(uint8_t, __thiscall, PathTrack_IsStop, 0x0061AEC0, uint8_t* self, int32_t index);

// ---- field initialisers (thiscall) ----
// 0x61A930 AIUpdateTrack
void __fastcall InitFields_61A930(uint8_t* self, re::Edx) {
    At<float>(self, 0x20) = 1.0f;
    self[0x19] = 0;
    At<float>(self, 0x24) = 1.0f;
    self[0x18] = 0;
    At<int32_t>(self, 0x1C) = 3;
    At<float>(self, 0x28) = 0.0f;
    self[8] = 0;
    At<float>(self, 0x2C) = -3.14159274f;
}
// 0x61A990 ControllerTrack (its pad the active one)
void __fastcall InitFields_61A990(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x18) = Pad_GetActive();
    At<float>(self, 0x28) = 0.0f;
    At<float>(self, 0x2C) = 0.0f;
    self[0x1C] = 1;
    At<float>(self, 0x38) = 0.0f;
    self[0x24] = 0;
    At<float>(self, 0x40) = 0.0f;
}
// 0x61AA20 WrestleTrack
void __fastcall InitFields_61AA20(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 1.0f;
    self[0x3C] = 0;
    At<float>(self, 0x1C) = 2.0f;
    At<float>(self, 0x34) = 2.0f;
    At<float>(self, 0x20) = 1.0f;
    At<float>(self, 0x24) = 1.0f;
    At<float>(self, 0x28) = 1.0f;
    At<float>(self, 0x2C) = 1.0f;
    At<float>(self, 0x30) = 1.0f;
    At<float>(self, 0x38) = 1.0f;
}
void __fastcall InitFields_61AA80(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 0;
    self[0x1C] = 1;
    self[8] = 1;
}
void __fastcall InitFields_61AA90(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x1C) = 1.0f;
    self[0x18] = 0;
}
void __fastcall InitFields_61AAB0(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 0;
    self[8] = 1;
    ClearName(self, 0x1C);
}
void __fastcall InitFields_61ABD0(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 5.0f;
    self[8] = 1;
    self[0x20] = 0;
    At<float>(self, 0x1C) = 30.0f;
    self[0x21] = 1;
}
void __fastcall InitFields_61AC40(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 10.0f;
    At<float>(self, 0x1C) = 0.0f;
}
void __fastcall InitFields_61AC60(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 10.0f;
}
void __fastcall InitFields_61ACF0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 10.0f;
    At<float>(self, 0x1C) = 20.0f;
}
void __fastcall InitFields_61AD10(uint8_t* self, re::Edx) {
    At<float>(self, 0x28) = 0.0f;
    self[8] = 1;
    At<float>(self, 0x2C) = 20.0f;
    self[0x1C] = 0;
    At<int32_t>(self, 0x20) = 0;
    self[0x24] = 0;
    self[0x30] = 1;
}
void __fastcall InitFields_61AD40(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x1C) = 0;
    self[0x20] = 0;
}
void __fastcall InitFields_61AD60(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 8.0f;
    self[0x20] = 1;
    self[8] = 1;
    At<float>(self, 0x1C) = 0.5f;
}
void __fastcall InitFields_61AD80(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 2.0f;
    At<int32_t>(self, 0x28) = 0;
    self[8] = 1;
    At<float>(self, 0x20) = 0.5f;
    At<float>(self, 0x24) = 5000.0f;
}
void __fastcall InitFields_61ADB0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 10.0f;
    At<float>(self, 0x1C) = 0.5f;
}
void __fastcall InitFields_61ADD0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 5.0f;
    At<float>(self, 0x1C) = 0.5f;
}
void __fastcall InitFields_61ADF0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 8.0f;
    At<float>(self, 0x1C) = 2.0f;
    At<float>(self, 0x20) = 0.5f;
    At<float>(self, 0x24) = 1.0f;
}
void __fastcall InitFields_61B390(uint8_t* self, re::Edx) {
    self[0x60] = 0;
    self[8] = 1;
}
void __fastcall InitFields_61B3A0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 2.0f;
    At<float>(self, 0x1C) = 1.0f;
}
void __fastcall InitFields_61B3C0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 10.0f;
    At<float>(self, 0x1C) = 5.0f;
    At<float>(self, 0x20) = 1.0f;
}
void __fastcall InitFields_61B500(uint8_t* self, re::Edx) {
    self[8] = 1;
    self[0x18] = 0;
    self[0x19] = 0;
}
void __fastcall InitFields_61BAD0(uint8_t* self, re::Edx) {
    self[0x18] = 0;
    At<float>(self, 0x1C) = 1.1f;
    self[8] = 1;
    At<float>(self, 0x20) = 0.6f;
}
void __fastcall InitFields_61BB20(uint8_t* self, re::Edx) {
    self[0x20] = 0;
    At<float>(self, 0x18) = 0.5f;
    At<int32_t>(self, 0x24) = 4000;
    self[8] = 1;
    At<float>(self, 0x1C) = 1.25f;
}
void __fastcall InitFields_61BBB0(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 0;
    At<int32_t>(self, 0x1C) = 1;
    self[0x20] = 0;
    self[8] = 1;
}
void __fastcall InitFields_61BBF0(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x1C) = 1;
    At<int32_t>(self, 0x20) = 7;
    At<int32_t>(self, 0x24) = 1;
    self[8] = 1;
    self[0x18] = 0;
}
// 0x61BC40 / 0x61BCB0: the action node request tracks (+0x18 its node's name)
void __fastcall InitFields_61BC40(uint8_t* self, re::Edx) {
    NameKey_FromString(&At<uint32_t>(self, 0x18), emptyString);
    self[0x1C] = 0;
    self[0x1D] = 0;
    self[0x1E] = 0;
    self[8] = 1;
}
void __fastcall InitFields_61BCB0(uint8_t* self, re::Edx) {
    NameKey_FromString(&At<uint32_t>(self, 0x18), emptyString);
    self[0x1C] = 0;
    self[0x1D] = 0;
    self[0x1E] = 0;
    self[8] = 1;
    At<int32_t>(self, 0x28) = 0;
}
void __fastcall InitFields_61BD20(uint8_t* self, re::Edx) {
    self[0x18] = 0;
    At<float>(self, 0x1C) = 0.0f;
    self[8] = 1;
}
void __fastcall InitFields_61BD60(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = -1.0f;
}
void __fastcall InitFields_61BD70(uint8_t* self, re::Edx) {
    self[0x18] = 0;
    self[0x19] = 0;
    self[0x1B] = 0;
    self[0x1C] = 1;
    self[8] = 1;
    self[0x1A] = 0;
}
void __fastcall InitFields_61BD90(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 0.0f;
    At<float>(self, 0x1C) = 0.0f;
    At<int32_t>(self, 0x20) = 1;
    At<float>(self, 0x24) = 0.0f;
    self[0x28] = 0;
    self[0x29] = 0;
    self[0x2A] = 0;
    self[8] = 1;
    At<int32_t>(self, 0x2C) = 8;
}
void __fastcall InitFields_61BDC0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 0.5f;
}
void __fastcall InitFields_61BDD0(uint8_t* self, re::Edx) {
    ClearName(self, 0x18);
    self[8] = 1;
}
void __fastcall InitFields_61BE00(uint8_t* self, re::Edx) {
    ClearName(self, 0x20);
    At<float>(self, 0x18) = 10.0f;
    self[8] = 1;
    At<float>(self, 0x1C) = 0.0f;
}
void __fastcall InitFields_61BE40(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x18) = 1;
    self[0x1C] = 0;
}
void __fastcall InitFields_61C0F0(uint8_t* self, re::Edx) {
    self[0x18] = 0;
    self[8] = 1;
}
void __fastcall InitFields_61C100(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 0.0f;
    self[0x18] = 0;
    self[0x19] = 0;
    self[0x1A] = 0;
    self[8] = 1;
}
void __fastcall InitFields_61C120(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 1.0f;
    self[8] = 1;
    At<float>(self, 0x1C) = 1.0f;
    self[0x20] = 1;
}
void __fastcall InitFields_61C140(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 1.0f;
    At<float>(self, 0x1C) = 7.0f;
    At<float>(self, 0x20) = 0.6f;
    At<float>(self, 0x24) = 1.0f;
    At<float>(self, 0x28) = 0.15f;
    At<float>(self, 0x2C) = 1.25f;
}
void __fastcall InitFields_61C180(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 3.0f;
    At<float>(self, 0x1C) = 6.0f;
}
// 0x61C1A0: RatLocoTrack
void __fastcall InitFields_61C1A0(uint8_t* self, re::Edx) {
    At<float>(self, 0x1C) = 0.0f;
    self[8] = 1;
    At<float>(self, 0x20) = 0.0f;
    self[0x18] = 1;
    self[0x2C] = 1;
    At<float>(self, 0x24) = -1.0f;
    self[0x2E] = 0;
    At<float>(self, 0x28) = 1.0f;
    self[0x2D] = 0;
    self[0x2F] = 0;
    At<float>(self, 0x30) = 0.05f;
    self[0x38] = 0;
    At<float>(self, 0x34) = 1.0f;
    for (int at = 0x44; at <= 0xBC; at += 4) {
        At<uint32_t>(self, at) = 0;
    }
    At<float>(self, 0xC0) = zeroVector[0];
    At<float>(self, 0xC4) = zeroVector[1];
    At<float>(self, 0xC8) = zeroVector[2];
}
void __fastcall InitFields_61C290(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 5.0f;
    self[0x20] = 1;
    self[8] = 1;
    At<float>(self, 0x1C) = 1.5f;
}
void __fastcall InitFields_61C2B0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 2.0f;
    At<float>(self, 0x1C) = 8.0f;
    At<float>(self, 0x20) = 0.5f;
    At<float>(self, 0x24) = 1.0f;
}
void __fastcall InitFields_61C310(uint8_t* self, re::Edx) {
    self[0x19] = 0;
    self[0x18] = 0;
    self[8] = 1;
}
void __fastcall InitFields_61C320(uint8_t* self, re::Edx) {
    self[8] = 1;
}
void __fastcall InitFields_61C330(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x18) = 0;
    self[0x1C] = 1;
}
void __fastcall InitFields_61C340(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<float>(self, 0x18) = 0.5f;
    At<float>(self, 0x1C) = 1.0f;
    At<float>(self, 0x20) = 2.0f;
    At<float>(self, 0x24) = 5.0f;
}
void __fastcall InitFields_61C5E0(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x18) = 3;
}
void __fastcall InitFields_61C5F0(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x18] = 0;
    At<int32_t>(self, 0x1C) = 0x56;
    EntityRef_Set(&At<uint8_t*>(self, 0x24), nullptr);
}
void __fastcall InitFields_61C610(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<int32_t>(self, 0x18) = 3;
}
void __fastcall InitFields_61C620(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x18] = 0;
    At<int32_t>(self, 0x1C) = 0x19;
    self[0x20] = 0;
}
void __fastcall InitFields_61C640(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 2000;
}
void __fastcall InitFields_61C650(uint8_t* self, re::Edx) {
    self[8] = 0;
    At<float>(self, 0x1C) = 100.0f;
}
void __fastcall InitFields_61C6C0(uint8_t* self, re::Edx) {
    self[8] = 1;
    self[0x18] = 1;
}
void __fastcall InitFields_61C710(uint8_t* self, re::Edx) {
    self[8] = 0;
    self[0x18] = 1;
    self[0x19] = 1;
    self[0x1A] = 1;
}
void __fastcall InitFields_61C720(uint8_t* self, re::Edx) {
    self[8] = 1;
    At<int32_t>(self, 0x18) = 0;
    At<int32_t>(self, 0x1C) = 0;
    At<int32_t>(self, 0x20) = 2;
}
void __fastcall InitFields_61C740(uint8_t* self, re::Edx) {
    At<float>(self, 0x18) = 15.0f;
}
void __fastcall InitFields_61C750(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x18) = 1;
}

// ---- small helpers and virtuals ----
// 0x61A8C0 (cdecl): value within range of centre
int __cdecl WithinRange(float value, float centre, float range) {
    if (!(centre + range >= value)) {
        return 0;
    }
    return centre - range <= value ? 1 : 0;
}
// 0x61A900 (thiscall): the ped's +0x1DC8 position
void __fastcall Ped_Set1DC8(uint8_t* ped, re::Edx, const float* pos) {
    At<float>(ped, 0x1DC8) = pos[0];
    At<float>(ped, 0x1DCC) = pos[1];
    At<float>(ped, 0x1DD0) = pos[2];
}
// 0x61A9F0 (thiscall): ControllerTrack::End: its pad's lock (+0x1C) released
uint8_t __fastcall ControllerTrack_End(uint8_t* self, re::Edx) {
    if (self[0x1C] != 0) {
        int32_t pad = At<int32_t>(self, 0x18);
        if (pad == 0) {
            controllerLockA = 0;
        } else if (pad == 1) {
            controllerLockB = 0;
        }
    }
    return 1;
}
// Start virtuals that keep the owner at an offset (and clear what follows)
template <int OwnerAt>
uint8_t __fastcall KeepOwner_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, OwnerAt) = owner;
    return 1;
}
template <int OwnerAt, int FlagAt>
uint8_t __fastcall KeepOwnerClearByte_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, OwnerAt) = owner;
    self[FlagAt] = 0;
    return 1;
}
template <int OwnerAt, int WordAt>
uint8_t __fastcall KeepOwnerClearWord_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, OwnerAt) = owner;
    At<uint32_t>(self, WordAt) = 0;
    return 1;
}
// 0x61AAF0 (thiscall): WanderTrack's next node (+0x1C the last)
void __fastcall WanderTrack_SetNode(uint8_t* self, re::Edx, uint32_t node) {
    At<uint32_t>(self, 0x1C) = At<uint32_t>(self, 0x20);
    At<uint32_t>(self, 0x20) = node;
}
// 0x61ABB0 (thiscall): RideAroundTrack's next node (+0x1C, +0x2C the ones before)
void __fastcall RideAroundTrack_SetNode(uint8_t* self, re::Edx, uint32_t node) {
    At<uint32_t>(self, 0x2C) = At<uint32_t>(self, 0x1C);
    At<uint32_t>(self, 0x1C) = At<uint32_t>(self, 0x20);
    At<uint32_t>(self, 0x20) = node;
}
// 0x61AB00 (thiscall): RideAroundTrack's position at a node (its coordinates are
// eighths), in the lane towards the node before it
V3* __fastcall RideAroundTrack_NodePosition(uint8_t* self, re::Edx, V3* out, uint8_t* node) {
    uint8_t* other = nullptr;
    if (node == At<uint8_t*>(self, 0x20)) {
        other = At<uint8_t*>(self, 0x1C);
    } else if (node == At<uint8_t*>(self, 0x1C)) {
        other = At<uint8_t*>(self, 0x2C);
    }
    out->x = float(At<int16_t>(node, 8)) * 0.125f;
    out->y = float(At<int16_t>(node, 0xA)) * 0.125f;
    out->z = float(At<int16_t>(node, 0xC)) * 0.125f;
    if (other && PathNode_IsValid(node) && PathNode_IsValid(other)) {
        V3 lane;
        V3* offset = PathNodes_LaneOffset(pathNodes, &lane, node, other);
        out->x = offset->x + out->x;
        out->y = offset->y + out->y;
        out->z = offset->z + out->z;
    }
    return out;
}
// 0x61ABF0 (thiscall): FleeTrack's deleting destructor (its +0x28 reference released)
uint8_t* __fastcall FleeTrack_DeletingDestruct(uint8_t* self, re::Edx, uint32_t flags) {
    At<uint32_t>(self, 0) = kVtFleeTrack;
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(self, 0x28);
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
    }
    TrackBase_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}
// 0x61AC80 / 0x61ACA0 / 0x61ACC0 / 0x61ACE0 (thiscall): the player's physical state up
// two, down one, reset, refreshed (each once)
uint8_t __fastcall IncrPlayerPhysicalStateTrack_Update(uint8_t*, re::Edx, float) {
    PlayerInfo_SetPhysicalState(playerInfo, uint8_t(playerPhysicalState + 2));
    return 0;
}
uint8_t __fastcall DecrPlayerPhysicalStateTrack_Update(uint8_t*, re::Edx, float) {
    PlayerInfo_SetPhysicalState(playerInfo, uint8_t(playerPhysicalState - 1));
    return 0;
}
uint8_t __fastcall ResetPlayerPhysicalStateTrack_Update(uint8_t*, re::Edx, float) {
    PlayerInfo_SetPhysicalState(playerInfo, 0);
    return 0;
}
uint8_t __fastcall RefreshPlayerPhysicalStateTrack_Update(uint8_t*, re::Edx, float) {
    PlayerInfo_RefreshPhysicalState(playerInfo);
    return 0;
}

// ---- path track ----
// A path track: +0x18 owner, +0x20 the node, +0x28 the patrol path, +0x30 its loop mode
// (0 once, 1 around, 2 back and forth, 3 stay), +0x34 the direction, +0x44 the node
// headed for (+0x48 at, +0x54 the direction through it)
uint8_t* PatrolPath(uint8_t* self) {
    return PatrolPath_Get(patrolPaths, At<int32_t>(self, 0x28));
}
V3 NodePos(uint8_t* path, int32_t node) {
    const float* p = Path_GetNode(path, node);
    return {p[0], p[1], p[2]};
}

// 0x61AE20 (thiscall): the node after index (by the loop mode); 0 past the end
uint8_t __fastcall PathTrack_StepImpl(uint8_t* self, re::Edx, int32_t* index) {
    uint8_t* path = PatrolPath(self);
    if (!path) {
        return 0;
    }
    int32_t count = Path_GetNumNodes(path);
    if (!count) {
        return 0;
    }
    switch (At<int32_t>(self, 0x30)) {
    case 0:
        *index += 1;
        return *index < count ? 1 : 0;
    case 1:
        *index += 1;
        if (*index >= count) {
            *index = 0;
        }
        return 1;
    case 2:
        *index += At<int32_t>(self, 0x34);
        if (*index >= count) {
            At<int32_t>(self, 0x34) = -1;
            *index = count - 2;
            return 1;
        }
        if (*index < 0) {
            *index = 1;
            At<int32_t>(self, 0x34) = 1;
        }
        return 1;
    case 3:
        return 1;
    default:
        return 0;
    }
}
// 0x61AEC0 (thiscall): the node is a stop (it waits there, or the path ends there)
uint8_t __fastcall PathTrack_IsStopImpl(uint8_t* self, re::Edx, int32_t index) {
    uint8_t* path = PatrolPath(self);
    if (!path) {
        return 0;
    }
    int32_t count = Path_GetNumNodes(path);
    float* node = Path_GetNode(path, index);
    if (At<int32_t>(node, 0x20) != 0 || 0.0f < At<float>(node, 0x1C)) {
        return 1;
    }
    int32_t mode = At<int32_t>(self, 0x30);
    if (mode == 0 || mode == 3) {
        return index == count - 1 ? 1 : 0;
    }
    if (mode == 2) {
        return index == count - 1 || index == 0 ? 1 : 0;
    }
    return 0;
}
// 0x61AF40 (thiscall): a riding ped close to its node (within 3) already past it
// (heading more along the leg to the next node than the one before)
uint8_t __fastcall PathTrack_PastNode(uint8_t* self, re::Edx, uint8_t* ped, float distance) {
    if (At<uint8_t*>(ped, 0x1554) == nullptr) {
        return 0;
    }
    if (3.0f < distance) {
        return 0;
    }
    if (Ped_EveryNthFrame(ped, 2)) {
        return 0;
    }
    int32_t node = At<int32_t>(self, 0x20);
    int32_t next = node;
    uint8_t stepped = PathTrack_Step(self, &next);
    if (!stepped) {
        return stepped;
    }
    uint8_t* path = PatrolPath(self);
    if (!path) {
        return 0;
    }
    int32_t previous;
    switch (At<int32_t>(self, 0x30)) {
    case 0:
        previous = node - 1;
        if (previous < 0) {
            return 0;
        }
        break;
    case 1:
        previous = node - 1;
        if (previous < 0) {
            previous = Path_GetNumNodes(path) - 1;
        }
        break;
    case 2:
        previous = node - At<int32_t>(self, 0x34);
        if (!(uint32_t(previous) < uint32_t(Path_GetNumNodes(path)))) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    V3 at = NodePos(path, At<int32_t>(self, 0x20));
    V3 ahead = NodePos(path, next);
    V3 behind = NodePos(path, previous);
    V3 back;
    V3 forward;
    V3 toPed;
    VectorSub(&back, &behind.x, &at.x);
    VectorSub(&forward, &ahead.x, &at.x);
    VectorSub(&toPed, Pos(ped), &at.x);
    Vector_Normalise(&back);
    Vector_Normalise(&forward);
    Vector_Normalise(&toPed);
    float alongForward = DotProduct(&forward, &toPed);
    float alongBack = DotProduct(&back, &toPed);
    return alongBack > alongForward ? 1 : 0;
}
// 0x61B100 (thiscall): the node headed for (+0x44, -1 at a stop or the end), where it
// is (+0x48) and the direction through it (+0x54: along the legs' bisector's normal,
// from previous; the leg's own when there's none or they nearly cancel)
void __fastcall PathTrack_SetNode(uint8_t* self, re::Edx, int32_t previous, int32_t index) {
    if (PathTrack_IsStop(self, index)) {
        At<int32_t>(self, 0x44) = -1;
        return;
    }
    int32_t next = index;
    if (!PathTrack_Step(self, &next)) {
        At<int32_t>(self, 0x44) = -1;
        return;
    }
    uint8_t* path = PatrolPath(self);
    if (!path) {
        return;
    }
    V3 at = NodePos(path, index);
    V3 ahead = NodePos(path, next);
    V3 leg;
    V3 d;
    VectorSub(&leg, &ahead.x, &at.x);
    FlatDirection(&d, &leg, 1.0f);
    At<float>(self, 0x48) = at.x;
    At<float>(self, 0x4C) = at.y;
    At<float>(self, 0x50) = at.z;
    At<int32_t>(self, 0x44) = index;
    if (previous == -1) {
        At<float>(self, 0x54) = d.x;
        At<float>(self, 0x58) = d.y;
        At<float>(self, 0x5C) = d.z;
        return;
    }
    V3 before = NodePos(path, previous);
    V3 legBack;
    V3 e;
    VectorSub(&legBack, &before.x, &at.x);
    FlatDirection(&e, &legBack, 1.0f);
    V3 sum;
    VectorAdd(&sum, &e, &d);
    float sq = sum.y * sum.y + sum.x * sum.x + sum.z * sum.z;
    if (sq < 0.05f) {
        At<float>(self, 0x54) = d.x;
        At<float>(self, 0x58) = d.y;
        At<float>(self, 0x5C) = d.z;
        return;
    }
    V3 across = {-sum.y, sum.x, 0.0f};
    V3 normal;
    FlatDirection(&normal, &across, 1.0f);
    if (DotProduct(&d, &normal) < 0.0f) {
        At<float>(self, 0x54) = -normal.x;
        At<float>(self, 0x58) = -normal.y;
        At<float>(self, 0x5C) = -normal.z;
        return;
    }
    At<float>(self, 0x54) = normal.x;
    At<float>(self, 0x58) = normal.y;
    At<float>(self, 0x5C) = normal.z;
}
// 0x61B350 (thiscall): PathTrack::Start (eax is left 1)
uint8_t __fastcall PathTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    At<int32_t>(self, 0x28) = -1;
    At<int32_t>(self, 0x20) = 0;
    At<int32_t>(self, 0x24) = -1;
    At<int32_t>(self, 0x2C) = 0;
    At<int32_t>(self, 0x30) = 1;
    At<int32_t>(self, 0x34) = 1;
    At<int32_t>(self, 0x1C) = 0;
    At<int32_t>(self, 0x44) = -1;
    return 1;
}

// 0x61B3E0 (thiscall): DockTrack's input: straight ahead (both vectors) at +0x24
void __fastcall DockTrack_SetInput(uint8_t* self, re::Edx, uint8_t* ped) {
    V3 f;
    Placeable_GetForward(ped, &f);
    float* in = At<float*>(ped, 0x2E0);
    in[0] = f.x;
    in[1] = f.y;
    in[2] = f.z;
    in = At<float*>(ped, 0x2E0);
    in[3] = f.x;
    in[4] = f.y;
    in[5] = f.z;
    At<float*>(ped, 0x2E0)[7] = At<float>(self, 0x24);
}

// 0x61B510: a point pulled to distance from centre (flat): its arguments in esi (the
// point, in / out), edi (the centre) and on the stack (the distance), so not hooked
void CombatLocoTrack_KeepAtDistance(V3* point, const V3* centre, float distance);
RE_UNHOOKED_FN("CombatLocoTrack::KeepAtDistance_61B510", 0x0061B510, &CombatLocoTrack_KeepAtDistance);
void CombatLocoTrack_KeepAtDistance(V3* point, const V3* centre, float distance) {
    V3 d;
    V3* r = VectorSub(&d, &point->x, &centre->x);
    V3 l = {r->x, r->y, 0.0f};
    Vector_Normalise(&l);
    l.x = l.x * distance;
    l.y = l.y * distance;
    l.z = distance * l.z;
    V3 m;
    VectorAdd(&m, centre, &l);
    *point = m;
}

// ---- combat locomotion ----
// A combat locomotion track: +0x20 the ped, +0x24, +0x28 its state, +0x2C where it goes
// (+0x38 from the centre), +0x44 ms in the state, +0x48, +0x58 its angle, +0x5C whom it
// circles, +0x60 the centre, +0x78 / +0x79 flags

// 0x61B590 (thiscall): where it goes (and the ped's move target)
void __fastcall CombatLocoTrack_SetGoal(uint8_t* self, re::Edx, const float* pos, int32_t kind) {
    At<float>(self, 0x2C) = pos[0];
    At<float>(self, 0x30) = pos[1];
    At<float>(self, 0x34) = pos[2];
    V3 d;
    V3* r = VectorSub(&d, pos, &At<float>(self, 0x60));
    At<float>(self, 0x38) = r->x;
    At<float>(self, 0x3C) = r->y;
    float z = r->z;
    At<int32_t>(self, 0x24) = kind;
    Ped_SetMoveTarget(At<uint8_t*>(self, 0x20), pos);
    At<float>(self, 0x40) = z;
}
// 0x61B5F0 (thiscall): the next point round the centre: turned from the one it faces
// by the ped's side offset, a step (0x1B0C0B0 a frame times the circled's +0x19C, 50
// each) away (less within sqrt 10 of the centre)
V3* __fastcall CombatLocoTrack_Circle(uint8_t* self, re::Edx, V3* out) {
    const float* pp = Pos(At<uint8_t*>(self, 0x20));
    V3 p = {pp[0], pp[1], pp[2]};
    V3 f;
    Placeable_GetForward(At<uint8_t*>(self, 0x5C), &f);
    const float* centre = &At<float>(self, 0x60);
    V3 g;
    VectorAdd(&g, centre, &f);
    V3 side = {f.y, -f.x, 0.0f};
    Vector_Normalise(&side);
    float heading = HeadingFromVector(&f);
    V3 v;
    VectorSub(&v, &p.x, &g.x);
    float offset = v.y * side.y + side.x * v.x + v.z * side.z;
    if (offset < -0.5f) {
        offset = -0.5f;
    } else if (0.5f < offset) {
        offset = 0.5f;
    }
    uint8_t* circled = At<uint8_t*>(self, 0x5C);
    float twice = offset + offset;
    heading = heading - twice * 0.9f;
    float step = combatStepRate * timeStep;
    step = step * At<float>(circled, 0x19C);
    step = step * 50.0f;
    V3 w;
    VectorSub(&w, &p.x, centre);
    float sq = w.y * w.y + w.x * w.x + w.z * w.z;
    float length = step;
    if (sq < 10.0f) {
        length = step * (sq / 10.0f);
    }
    V3 hv;
    V3* dir = HeadingToVector(&hv, heading);
    V3 m = {length * dir->x, dir->y * length, dir->z * length};
    VectorAdd(out, centre, &m);
    return out;
}
// 0x61B800 (thiscall): a random angle within +-0x1B0C0A0 (+0x58); state 3
void __fastcall CombatLocoTrack_PickAngle(uint8_t* self, re::Edx) {
    float most = combatMaxAngle;
    float least = -most;
    int r = GameRand();
    At<int32_t>(self, 0x28) = 3;
    self[0x78] = 1;
    float q = float(r) / 32767.0f;
    self[0x79] = 1;
    At<float>(self, 0x58) = (most - least) * q + least;
}
// 0x61B860 (thiscall): the state's time (+0x44); there (within 0.3): state 2; state 0
// after 10 s, or 1.5 s when the circled is in a vehicle it's reached
void __fastcall CombatLocoTrack_UpdateTimer(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0x44) += int32_t(int64_t(combatStepRate * timeStep * 1000.0f));
    V3 d;
    VectorSub(&d, Pos(At<uint8_t*>(self, 0x20)), &At<float>(self, 0x2C));
    float sq = d.x * d.x + d.y * d.y + 0.0f * 0.0f;
    if (sq - 0.09f < 0.0f) {
        At<int32_t>(self, 0x48) = 0;
        At<int32_t>(self, 0x28) = 2;
        self[0x79] = 1;
    }
    uint8_t* circled = At<uint8_t*>(self, 0x5C);
    if (circled && Ped_IsInVehicle(circled) && At<int32_t>(self, 0x44) >= 1500 &&
        DistanceBeyond(Pos(At<uint8_t*>(self, 0x20)), &At<float>(self, 0x2C), 1.6f) < 0.0f) {
        self[0x79] = 1;
        At<int32_t>(self, 0x28) = 0;
        return;
    }
    if (At<int32_t>(self, 0x44) >= 10000) {
        self[0x79] = 1;
        At<int32_t>(self, 0x28) = 0;
    }
}
float Clamp01(float v) {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (1.0f < v) {
        return 1.0f;
    }
    return v;
}
float DistanceSq(const V3& d) {
    return d.x * d.x + d.y * d.y + d.z * d.z;
}
// 0x61B980 (thiscall): the urge from the ped's distance to the centre beyond the AI's
// engage distance (0.5..1.5)
float __fastcall CombatLocoTrack_CentreUrge(uint8_t* self, re::Edx) {
    V3 d;
    VectorSub(&d, &At<float>(self, 0x60), Pos(At<uint8_t*>(self, 0x20)));
    float v = (DistanceSq(d) - At<float>(aiButes, 0x2C)) * 0.04f;
    v = 1.0f - Clamp01(v);
    v = trk::CrtPow(v, 8.0f);
    v = 1.0f - v;
    return v + 0.5f;
}
// 0x61BA40 (thiscall): the urge from the distance to the goal (0.2..1.5)
float __fastcall CombatLocoTrack_GoalUrge(uint8_t* self, re::Edx) {
    V3 d;
    VectorSub(&d, &At<float>(self, 0x2C), Pos(At<uint8_t*>(self, 0x20)));
    float v = Clamp01(DistanceSq(d) * 0.015625f);
    return v * 1.3f + 0.2f;
}

// 0x61BB80 (thiscall): AttackTrack::Start
uint8_t __fastcall AttackTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    At<int32_t>(self, 0x2C) = 0;
    At<int32_t>(self, 0x34) = 0;
    At<int32_t>(self, 0x38) = 0;
    self[0x30] = 1;
    At<int32_t>(self, 0x3C) = 0x63;
    return 1;
}
// 0x61BC10 (thiscall): ActionRegisterButtonHistoryTrack::Start: the HUD's scripted
// button (+0x20, +0x24)
uint8_t __fastcall ActionRegisterButtonHistoryTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    ButtonHistory_SetScriptedInput(buttonHistory, At<int32_t>(self, 0x20), At<int32_t>(self, 0x24));
    self[0x30] = 0;
    At<int32_t>(self, 0x2C) = 0;
    At<uint8_t*>(self, 0x28) = owner;
    return 1;
}
// 0x61BD00 (thiscall): DialogActionNodeRequestTrack::Start
uint8_t __fastcall DialogActionNodeRequestTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    self[0x2C] = 0;
    self[0x24] = 0;
    At<uint8_t*>(self, 0x20) = owner;
    return 1;
}

// ---- projectile cover ----
// A projectile cover track: +0x18 its button, +0x24 the ped, +0x2C the cover's timings
// (+0x24.. of it), +0x30 the cover point, +0x34 its timer, +0x40 in cover, +0x44 its
// side, +0x48 its position and direction, +0x60, +0x68 the state, +0x6C its timer, +0x79

// 0x61BE50 (thiscall): a cover point kept or chosen (a new one every 5..10 s)
uint8_t __fastcall ProjectileCoverTrack_ChooseCover(uint8_t* self, re::Edx) {
    if (!CoverPoints_Has(coverPoints, At<uint8_t*>(self, 0x30), At<uint8_t*>(self, 0x24))) {
        At<uint8_t*>(self, 0x30) = nullptr;
    }
    if (!At<uint8_t*>(self, 0x30) || Timer_IsDone(self + 0x34)) {
        uint8_t* point = CoverPoints_Choose(coverPoints, At<uint8_t*>(self, 0x24));
        if (point) {
            At<uint8_t*>(self, 0x30) = point;
            Timer_Start(self + 0x34, 5.0f, 10.0f);
        }
    }
    uint8_t* point = At<uint8_t*>(self, 0x30);
    if (!point) {
        return 0;
    }
    At<uint8_t*>(self, 0x2C) = point + 0x24;
    EntityRef_Set(&At<uint8_t*>(point, 0x70), At<uint8_t*>(self, 0x24));
    Timer_Set(At<uint8_t*>(self, 0x30) + 0x74, 1.0f);
    point = At<uint8_t*>(self, 0x30);
    int32_t kind = At<int32_t>(point, 4);
    if (kind == 1) {
        At<int32_t>(self, 0x44) = 0;
    } else if (kind == 2) {
        At<int32_t>(self, 0x44) = 1;
    } else {
        return 0;
    }
    for (int i = 0; i < 6; i++) {
        At<uint32_t>(self, 0x48 + i * 4) = At<uint32_t>(point, 8 + i * 4);
    }
    return 1;
}
// 0x61BF30 (thiscall): the cover state (1 hide, 2 peek / move, 3 wait, 4 pop out, 5
// fire) and its timer; the fire button requested (+0x79) on popping out / firing
void __fastcall ProjectileCoverTrack_SetState(uint8_t* self, re::Edx, int32_t state) {
    uint8_t* timer = self + 0x6C;
    At<int32_t>(self, 0x68) = state;
    Timer_Reset(timer);
    int32_t button = At<int32_t>(self, 0x18);
    for (;;) {
        int32_t next;
        switch (At<int32_t>(self, 0x68)) {
        case 1: {
            if (At<int32_t>(self, 0x40) != 0 && self[0x60] != 0) {
                next = 2;
                break;
            }
            float least = At<float>(At<uint8_t*>(self, 0x2C), 0x24);
            float most = At<float>(At<uint8_t*>(self, 0x2C), 0x28);
            if (At<int32_t>(self, 0x40) == 0 || !(least < 0.0f || least == 0.0f) ||
                !(most < 0.0f || most == 0.0f)) {
                Timer_Start(timer, least, most);
                return;
            }
            next = 2;
            break;
        }
        case 2:
            if (At<int32_t>(self, 0x40) == 0) {
                next = 1;
            } else {
                if (At<int32_t>(self, 0x44) == 1) {
                    return;
                }
                next = 3;
            }
            break;
        case 3:
            Timer_Set(timer, At<float>(At<uint8_t*>(self, 0x2C), 0x2C));
            return;
        case 4:
            Timer_Start(timer, 0.5f, 1.5f);
            if (self[0x79] != 0) {
                ActionInputs_SetRequest(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0x2E0), button, 2, 0.0f);
            }
            return;
        case 5:
            Timer_Set(timer, At<float>(At<uint8_t*>(self, 0x2C), 0x30));
            if (self[0x79] != 0) {
                ActionInputs_SetRequest(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0x2E0), button, 3, 0.0f);
                return;
            }
            ActionInputs_SetRequest(At<uint8_t*>(At<uint8_t*>(self, 0x24), 0x2E0), button, 1, 0.0f);
            return;
        default:
            return;
        }
        At<int32_t>(self, 0x68) = next;
        Timer_Reset(timer);
        button = At<int32_t>(self, 0x18);
    }
}

// 0x61C2E0 (thiscall): OnBikeDashTrack::Start: its start time (ms, as a float)
uint8_t __fastcall OnBikeDashTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    At<float>(self, 0x2C) = float(timeInMs);
    return 1;
}

// ---- rat locomotion ----
// 0x61C370 (thiscall): RatLocoTrack's wander: its input turned by a wobble (+0x68, the
// angle turning 3 times the step rate)
void __fastcall RatLocoTrack_Wobble(uint8_t* self, re::Edx) {
    float step = combatStepRate * timeStep;
    float angle = step * ratTurnScale + At<float>(self, 0x68);
    At<float>(self, 0x68) = angle;
    angle = LimitAngle(angle);
    At<float>(self, 0x68) = angle;
    float* in = At<float*>(At<uint8_t*>(self, 0x4C), 0x2E0);
    V3 v = {in[0], in[1], in[2]};
    float negY = -v.y;
    float c = trk::CrtCos(angle) * ratWobble;
    float ax = c * negY;
    float ay = v.x * c;
    float az = c * 0.0f;
    v.x = v.x + ax;
    v.y = ay + v.y;
    v.z = v.z + az;
    Vector_Normalise(&v);
    in = At<float*>(At<uint8_t*>(self, 0x4C), 0x2E0);
    in[0] = v.x;
    in[1] = v.y;
    in[2] = v.z;
}
// 0x61C470 (thiscall): RatLocoTrack's food: the nearest pickup within range (or a food
// stimulus, 0x1D, nearer still) as the goal (+0x40); 0 while its timer runs
uint8_t __fastcall RatLocoTrack_FindFood(uint8_t* self, re::Edx) {
    if (Timer_IsRunning(self + 0x6C)) {
        return 0;
    }
    float range = ratFoodRange;
    const float* at = &At<float>(self, 0x50);
    uint8_t found = 0;
    float best = range * range;
    Pickups_FindInRange(at, range, 0);
    for (uint32_t i = 0; i < pickupsInRangeCount; i++) {
        const float* p = reinterpret_cast<const float*>(pickups + pickupsInRange[i] * 0x48 + 8);
        V3 d;
        VectorSub(&d, at, p);
        float sq = DistanceSq(d);
        if (best > sq) {
            best = sq;
            At<float>(self, 0x40) = p[0];
            At<float>(self, 0x44) = p[1];
            found = 1;
            At<float>(self, 0x48) = p[2];
        }
    }
    uint8_t* stimulus = Stimuli_Find(stimuli, At<uint8_t*>(self, 0x4C), 0x1D, -1);
    if (stimulus) {
        V3 s = {At<float>(stimulus, 0xC), At<float>(stimulus, 0x10), At<float>(stimulus, 0x14)};
        V3 d;
        VectorSub(&d, at, &s.x);
        float sq = DistanceSq(d);
        if (best > sq) {
            At<float>(self, 0x40) = At<float>(stimulus, 0xC);
            At<float>(self, 0x44) = At<float>(stimulus, 0x10);
            At<float>(self, 0x48) = At<float>(stimulus, 0x14);
            return 1;
        }
    }
    return found;
}

// 0x61C660 (thiscall): TargetPlayerTrack::Start: updated at once
uint8_t __fastcall TargetPlayerTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    vc::Call<uint8_t>(self, acttree::kTrackUpdate, 0.0f);
    return 1;
}
// 0x61C680 / 0x61C6D0 (thiscall): Evade{Grenade,Projectile}Track::Start: evading, after
// 0.5..2 s
template <int FlagAt, int DelayAt>
uint8_t __fastcall EvadeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    self[FlagAt] = 1;
    int r = GameRand();
    At<float>(self, DelayAt) = float(r) / 32767.0f * 1.5f + 0.5f;
    return 1;
}

} // namespace

#define TRAICOMBAT_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

TRAICOMBAT_REVERSED("AITracks::WithinRange_61A8C0", 0x0061A8C0, &WithinRange);
TRAICOMBAT_REVERSED("CPed::Set1DC8_61A900", 0x0061A900, &Ped_Set1DC8);
TRAICOMBAT_REVERSED("AIUpdateTrack::InitFields", 0x0061A930, &InitFields_61A930);
TRAICOMBAT_REVERSED("AIUpdateTrack::DeletingDestruct", 0x0061A960, &TrackDeletingDtor<kVtAIUpdateTrack>);
TRAICOMBAT_REVERSED("ControllerTrack::InitFields", 0x0061A990, &InitFields_61A990);
TRAICOMBAT_REVERSED("ControllerTrack::DeletingDestruct", 0x0061A9C0, &TrackDeletingDtor<kVtControllerTrack>);
TRAICOMBAT_REVERSED("ControllerTrack::End", 0x0061A9F0, &ControllerTrack_End);
TRAICOMBAT_REVERSED("WrestleTrack::InitFields", 0x0061AA20, &InitFields_61AA20);
TRAICOMBAT_REVERSED("WrestleTrack::DeletingDestruct", 0x0061AA50, &TrackDeletingDtor<kVtWrestleTrack>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AA80", 0x0061AA80, &InitFields_61AA80);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AA90", 0x0061AA90, &InitFields_61AA90);
TRAICOMBAT_REVERSED("WarnTargetTrack::Start", 0x0061AAA0, &KeepOwner_Start<0x20>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AAB0", 0x0061AAB0, &InitFields_61AAB0);
TRAICOMBAT_REVERSED("WanderTrack::SetNode", 0x0061AAF0, &WanderTrack_SetNode);
TRAICOMBAT_REVERSED("RideAroundTrack::NodePosition", 0x0061AB00, &RideAroundTrack_NodePosition);
TRAICOMBAT_REVERSED("RideAroundTrack::SetNode", 0x0061ABB0, &RideAroundTrack_SetNode);
TRAICOMBAT_REVERSED("AITrack::InitFields_61ABD0", 0x0061ABD0, &InitFields_61ABD0);
TRAICOMBAT_REVERSED("FleeTrack::DeletingDestruct", 0x0061ABF0, &FleeTrack_DeletingDestruct);
TRAICOMBAT_REVERSED("HangOutTrack::Start", 0x0061AC30, &KeepOwnerClearByte_Start<0x24, 0x20>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AC40", 0x0061AC40, &InitFields_61AC40);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AC60", 0x0061AC60, &InitFields_61AC60);
TRAICOMBAT_REVERSED("MoveToPOITrack::Start", 0x0061AC70, &KeepOwnerClearByte_Start<0x1C, 0x20>);
TRAICOMBAT_REVERSED("IncrPlayerPhysicalStateTrack::Update", 0x0061AC80, &IncrPlayerPhysicalStateTrack_Update);
TRAICOMBAT_REVERSED("DecrPlayerPhysicalStateTrack::Update", 0x0061ACA0, &DecrPlayerPhysicalStateTrack_Update);
TRAICOMBAT_REVERSED("ResetPlayerPhysicalStateTrack::Update", 0x0061ACC0, &ResetPlayerPhysicalStateTrack_Update);
TRAICOMBAT_REVERSED("RefreshPlayerPhysicalStateTrack::Update", 0x0061ACE0, &RefreshPlayerPhysicalStateTrack_Update);
TRAICOMBAT_REVERSED("AITrack::InitFields_61ACF0", 0x0061ACF0, &InitFields_61ACF0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AD10", 0x0061AD10, &InitFields_61AD10);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AD40", 0x0061AD40, &InitFields_61AD40);
TRAICOMBAT_REVERSED("WeaponIncrementChargeTrack::Start", 0x0061AD50, &KeepOwner_Start<0x18>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AD60", 0x0061AD60, &InitFields_61AD60);
TRAICOMBAT_REVERSED("AITrack::InitFields_61AD80", 0x0061AD80, &InitFields_61AD80);
TRAICOMBAT_REVERSED("AITrack::InitFields_61ADB0", 0x0061ADB0, &InitFields_61ADB0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61ADD0", 0x0061ADD0, &InitFields_61ADD0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61ADF0", 0x0061ADF0, &InitFields_61ADF0);
TRAICOMBAT_REVERSED("PathTrack::Step", 0x0061AE20, &PathTrack_StepImpl);
TRAICOMBAT_REVERSED("PathTrack::IsStop", 0x0061AEC0, &PathTrack_IsStopImpl);
TRAICOMBAT_REVERSED("PathTrack::PastNode", 0x0061AF40, &PathTrack_PastNode);
TRAICOMBAT_REVERSED("PathTrack::SetNode", 0x0061B100, &PathTrack_SetNode);
TRAICOMBAT_REVERSED("PathTrack::Start", 0x0061B350, &PathTrack_Start);
TRAICOMBAT_REVERSED("AITrack::InitFields_61B390", 0x0061B390, &InitFields_61B390);
TRAICOMBAT_REVERSED("AITrack::InitFields_61B3A0", 0x0061B3A0, &InitFields_61B3A0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61B3C0", 0x0061B3C0, &InitFields_61B3C0);
TRAICOMBAT_REVERSED("DockTrack::SetInput", 0x0061B3E0, &DockTrack_SetInput);
TRAICOMBAT_REVERSED("GreetTrack::DeletingDestruct", 0x0061B440, &TrackDeletingDtor<kVtGreetTrack>);
TRAICOMBAT_REVERSED("TeacherTrack::DeletingDestruct", 0x0061B470, &TrackDeletingDtor<kVtTeacherTrack>);
TRAICOMBAT_REVERSED("MoveObjectiveTrack::DeletingDestruct", 0x0061B4A0, &TrackDeletingDtor<kVtMoveObjectiveTrack>);
TRAICOMBAT_REVERSED("FaceObjectiveTrack::DeletingDestruct", 0x0061B4D0, &TrackDeletingDtor<kVtFaceObjectiveTrack>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61B500", 0x0061B500, &InitFields_61B500);
TRAICOMBAT_REVERSED("CombatLocoTrack::SetGoal", 0x0061B590, &CombatLocoTrack_SetGoal);
TRAICOMBAT_REVERSED("CombatLocoTrack::Circle", 0x0061B5F0, &CombatLocoTrack_Circle);
TRAICOMBAT_REVERSED("CombatLocoTrack::PickAngle", 0x0061B800, &CombatLocoTrack_PickAngle);
TRAICOMBAT_REVERSED("CombatLocoTrack::UpdateTimer", 0x0061B860, &CombatLocoTrack_UpdateTimer);
TRAICOMBAT_REVERSED("CombatLocoTrack::CentreUrge", 0x0061B980, &CombatLocoTrack_CentreUrge);
TRAICOMBAT_REVERSED("CombatLocoTrack::GoalUrge", 0x0061BA40, &CombatLocoTrack_GoalUrge);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BAD0", 0x0061BAD0, &InitFields_61BAD0);
TRAICOMBAT_REVERSED("PursuitLocoTrack::DeletingDestruct", 0x0061BAF0, &TrackDeletingDtor<kVtPursuitLocoTrack>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BB20", 0x0061BB20, &InitFields_61BB20);
TRAICOMBAT_REVERSED("AttackTrack::DeletingDestruct", 0x0061BB50, &TrackDeletingDtor<kVtAttackTrack>);
TRAICOMBAT_REVERSED("AttackTrack::Start", 0x0061BB80, &AttackTrack_Start);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BBB0", 0x0061BBB0, &InitFields_61BBB0);
TRAICOMBAT_REVERSED("ActionRequestTrack::Start", 0x0061BBD0, &KeepOwnerClearWord_Start<0x24, 0x28>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BBF0", 0x0061BBF0, &InitFields_61BBF0);
TRAICOMBAT_REVERSED("ActionRegisterButtonHistoryTrack::Start", 0x0061BC10, &ActionRegisterButtonHistoryTrack_Start);
TRAICOMBAT_REVERSED("ActionNodeRequestTrack::InitFields_61BC40", 0x0061BC40, &InitFields_61BC40);
TRAICOMBAT_REVERSED("ActionNodeRequestTrack::DeletingDestruct", 0x0061BC70,
                    &TrackDeletingDtor<kVtActionNodeRequestTrack, 0x18>);
TRAICOMBAT_REVERSED("ActionNodeRequestTrack::Start", 0x0061BCA0, &KeepOwnerClearByte_Start<0x20, 0x24>);
TRAICOMBAT_REVERSED("ActionNodeRequestTrack::InitFields_61BCB0", 0x0061BCB0, &InitFields_61BCB0);
TRAICOMBAT_REVERSED("DialogActionNodeRequestTrack::Destruct", 0x0061BCE0,
                    &TrackDtor<kVtActionNodeRequestTrack, 0x18>);
TRAICOMBAT_REVERSED("DialogActionNodeRequestTrack::Start", 0x0061BD00, &DialogActionNodeRequestTrack_Start);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BD20", 0x0061BD20, &InitFields_61BD20);
TRAICOMBAT_REVERSED("BlockTrack::Start", 0x0061BD30, &KeepOwnerClearWord_Start<0x20, 0x24>);
TRAICOMBAT_REVERSED("WeaponWheelOffTrack::Start", 0x0061BD50, &KeepOwner_Start<0x1C>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BD60", 0x0061BD60, &InitFields_61BD60);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BD70", 0x0061BD70, &InitFields_61BD70);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BD90", 0x0061BD90, &InitFields_61BD90);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BDC0", 0x0061BDC0, &InitFields_61BDC0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BDD0", 0x0061BDD0, &InitFields_61BDD0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BE00", 0x0061BE00, &InitFields_61BE00);
TRAICOMBAT_REVERSED("AITrack::InitFields_61BE40", 0x0061BE40, &InitFields_61BE40);
TRAICOMBAT_REVERSED("ProjectileCoverTrack::ChooseCover", 0x0061BE50, &ProjectileCoverTrack_ChooseCover);
TRAICOMBAT_REVERSED("ProjectileCoverTrack::SetState", 0x0061BF30, &ProjectileCoverTrack_SetState);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C0F0", 0x0061C0F0, &InitFields_61C0F0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C100", 0x0061C100, &InitFields_61C100);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C120", 0x0061C120, &InitFields_61C120);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C140", 0x0061C140, &InitFields_61C140);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C180", 0x0061C180, &InitFields_61C180);
TRAICOMBAT_REVERSED("RatLocoTrack::InitFields", 0x0061C1A0, &InitFields_61C1A0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C290", 0x0061C290, &InitFields_61C290);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C2B0", 0x0061C2B0, &InitFields_61C2B0);
TRAICOMBAT_REVERSED("OnBikeDashTrack::Start", 0x0061C2E0, &OnBikeDashTrack_Start);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C310", 0x0061C310, &InitFields_61C310);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C320", 0x0061C320, &InitFields_61C320);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C330", 0x0061C330, &InitFields_61C330);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C340", 0x0061C340, &InitFields_61C340);
TRAICOMBAT_REVERSED("RatLocoTrack::Wobble", 0x0061C370, &RatLocoTrack_Wobble);
TRAICOMBAT_REVERSED("RatLocoTrack::FindFood", 0x0061C470, &RatLocoTrack_FindFood);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C5E0", 0x0061C5E0, &InitFields_61C5E0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C5F0", 0x0061C5F0, &InitFields_61C5F0);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C610", 0x0061C610, &InitFields_61C610);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C620", 0x0061C620, &InitFields_61C620);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C640", 0x0061C640, &InitFields_61C640);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C650", 0x0061C650, &InitFields_61C650);
TRAICOMBAT_REVERSED("TargetPlayerTrack::Start", 0x0061C660, &TargetPlayerTrack_Start);
TRAICOMBAT_REVERSED("EvadeGrenadeTrack::Start", 0x0061C680, &EvadeTrack_Start<0x34, 0x44>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C6C0", 0x0061C6C0, &InitFields_61C6C0);
TRAICOMBAT_REVERSED("EvadeProjectileTrack::Start", 0x0061C6D0, &EvadeTrack_Start<0x48, 0x58>);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C710", 0x0061C710, &InitFields_61C710);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C720", 0x0061C720, &InitFields_61C720);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C740", 0x0061C740, &InitFields_61C740);
TRAICOMBAT_REVERSED("AITrack::InitFields_61C750", 0x0061C750, &InitFields_61C750);
