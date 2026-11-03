// The race minigame (CRace, game mode 0xE, vtable 0x949A50; 0x70CB60..0x713697): bike,
// go-kart and on-foot races along a patrol path, with shortcut and jump paths, the
// highlighted (waypoint) nodes, the racers' rubber-banding speeds, the arcade (go-kart
// track) extras: electric barriers, boost pads, the kart explosion and the transition
// screens. The racer list keeps the finished racers first (+0x124 of them), the rest
// sorted by how far along the race they are. The script natives are MinigameRaceNatives.cpp.
//
// A racer (0x6C): +0 the ped (a registered reference), +4 the node it heads for, +8 its
// lap, +0xC the shortcut / jump path it is on (nullptr: the race's path), +0x10 the race,
// +0x14 its finish time, +0x24..+0x54 the stats RaceSetRacerStats sets, +0x58 the last
// warp, +0x5C the last bike re-mount, +0x60 the last boost, +0x64 its start slot,
// +0x68 its lane (-1 left, 0, 1 right of the path).
// The x87 runs at 24-bit precision here (the main thread): float, each step rounded.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/Pool.h"
#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// ---- the layout -------------------------------------------------------------------------------

struct RaceVec {
    float x, y, z;
};
static_assert(sizeof(RaceVec) == 0xC);

// the game's growable array: data, capacity, count
template <typename T>
struct RaceArray {
    re::Ptr32<re::Field<T>> data;
    uint32_t capacity;
    uint32_t count;
};
static_assert(sizeof(RaceArray<int32_t>) == 0xC);

// a shortcut or jump path: its nodes [first..last] leave / rejoin the race path's
struct RacePathRange {
    int32_t path;    // +0 a patrol path id
    int32_t first;   // +4 the race path's node it leaves from
    int32_t last;    // +8 the race path's node it rejoins at
};
static_assert(sizeof(RacePathRange) == 0xC);

// a highlighted node's record: who passed it first and when (for the split times)
struct RaceRecord {
    re::Ptr32<uint8_t> ped;   // +0 a registered reference
    uint32_t time;  // +4
    int32_t lap;    // +8
};
static_assert(sizeof(RaceRecord) == 0xC);

struct RaceJumpNode {
    int32_t path;
    int32_t node;
};
static_assert(sizeof(RaceJumpNode) == 8);

struct CRace;

struct RaceRacer {
    re::Ptr32<uint8_t> ped;              // +0x0 (a registered reference)
    int32_t node;              // +0x4
    int32_t lap;               // +0x8
    re::Ptr32<RacePathRange> shortcut;   // +0xC
    re::Ptr32<CRace> race;               // +0x10
    uint32_t finishTime;       // +0x14
    uint32_t jumpTime;         // +0x18 the jump button pressed (0: none)
    float boostTimer;          // +0x1C counts up; > 0 a boost may start
    uint8_t boosting;          // +0x20
    uint8_t pad21[3];
    float boostSpeed;          // +0x24 (1.3)
    float speed;               // +0x28 (1.0)
    float aheadDistance;       // +0x2C (10) slows past this far ahead of the player
    float aheadSpeed;          // +0x30 (1.3) hmm: the speed it eases to (ahead)
    float behindDistance;      // +0x34 (10) speeds up past this far behind
    float behindSpeed;         // +0x38 (0.7)
    int32_t shortcutChance;    // +0x3C percent
    int32_t unknown40;         // +0x40
    int32_t unknown44;         // +0x44
    int32_t boostInterval;     // +0x48
    int32_t boostDuration;     // +0x4C
    int32_t boostChance;       // +0x50 (not set by the constructor)
    float aggression;          // +0x54 (0.5)
    uint32_t warpTime;         // +0x58
    uint32_t mountTime;        // +0x5C
    uint32_t bumpTime;         // +0x60 the last boost pad hit
    int32_t startSlot;         // +0x64
    int32_t lane;              // +0x68
};
static_assert(sizeof(RaceRacer) == 0x6C);
static_assert(offsetof(RaceRacer, boostSpeed) == 0x24);
static_assert(offsetof(RaceRacer, aggression) == 0x54);
static_assert(offsetof(RaceRacer, lane) == 0x68);

// an effect proxy (vtable, effect)
struct RaceFx {
    re::Ptr32<void> vtable;
    re::Ptr32<void> effect;
};
static_assert(sizeof(RaceFx) == 8);

struct RaceBarrier {
    RaceVec a, b;
};
static_assert(sizeof(RaceBarrier) == 0x18);

struct CRace {
    re::Ptr32<re::Ptr32<void>> vtable;                       // +0x0
    uint8_t active;                      // +0x4
    uint8_t unknown5[0x33];
    int32_t kind38;                      // +0x38 (4)
    int32_t mode;                        // +0x3C (0xE)
    uint8_t unknown40[0x10];
    int32_t popupKind;                   // +0x50 the popup (1 the exit question)
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;                // +0xA0
    uint8_t unknownA1[0x23];
    RaceArray<RaceRacer> racers;         // +0xC4
    RaceArray<RacePathRange> shortcuts;  // +0xD0
    RaceArray<RacePathRange> jumpPaths;  // +0xDC
    RaceArray<int32_t> highlight;        // +0xE8 the waypoint nodes (sorted)
    RaceArray<RaceJumpNode> jumpNodes;   // +0xF4
    RaceArray<RaceRecord> records;       // +0x100 one per node
    RaceArray<int32_t> bottlenecks;      // +0x10C (sorted)
    int32_t path;                        // +0x118 the race's patrol path
    int32_t laps;                        // +0x11C
    int32_t racerCount;                  // +0x120 the script's racers and the player
    int32_t finishedCount;               // +0x124
    uint8_t started;                     // +0x128
    uint8_t pad129[3];
    uint32_t startTime;                  // +0x12C
    uint32_t lapTime;                    // +0x130
    uint32_t wrongWayTime;               // +0x134
    uint32_t sortTime;                   // +0x138
    int32_t wrongWayNode;                // +0x13C
    int32_t blip;                        // +0x140
    RaceFx waypoint;                     // +0x144
    RaceFx beam;                         // +0x14C
    uint8_t showWaypoint;                // +0x154
    uint8_t finishOnly;                  // +0x155
    uint8_t pad156[2];
    RaceBarrier barriers[10];            // +0x158
    re::Ptr32<void> barrierSounds[10];             // +0x248
    RaceFx barrierFx[10][4];             // +0x270
    int32_t barrierCount;                // +0x3B0
    uint32_t barrierTime;                // +0x3B4
    int32_t barrierState;                // +0x3B8 0 off, 1 charging, 2 on
    RaceVec boosts[16];                  // +0x3BC x, y, heading (degrees)
    RaceFx boostFx[16];                  // +0x47C
    int32_t boostCount;                  // +0x4FC
    int32_t transition;                  // +0x500
    re::Ptr32<void> transitionSprite;              // +0x504 (a CSprite2d)
    int32_t transitionTxd;               // +0x508
    uint8_t arcade;                      // +0x50C
    uint8_t pad50D[3];
};
static_assert(offsetof(CRace, racers) == 0xC4);
static_assert(offsetof(CRace, path) == 0x118);
static_assert(offsetof(CRace, waypoint) == 0x144);
static_assert(offsetof(CRace, barriers) == 0x158);
static_assert(offsetof(CRace, barrierFx) == 0x270);
static_assert(offsetof(CRace, barrierCount) == 0x3B0);
static_assert(offsetof(CRace, boosts) == 0x3BC);
static_assert(offsetof(CRace, boostFx) == 0x47C);
static_assert(offsetof(CRace, boostCount) == 0x4FC);
static_assert(offsetof(CRace, arcade) == 0x50C);
static_assert(sizeof(CRace) == 0x510);

// CColPoint (0x28; +0xC a registered entity reference)
struct RaceColPoint {
    RaceVec pos;      // +0x0
    re::Ptr32<uint8_t> entity;  // +0xC
    RaceVec normal;   // +0x10
    uint8_t pad1C[8];
    float depth;      // +0x24
};
static_assert(sizeof(RaceColPoint) == 0x28);
// CColLine and CColSphere
struct RaceColLine {
    float start[4];
    float end[4];
};
static_assert(sizeof(RaceColLine) == 0x20);
struct RaceColSphere {
    float center[3];
    float radius;
    uint8_t surface, piece, pad[2];
};
static_assert(sizeof(RaceColSphere) == 0x14);
// an effect's transform: 3x3 (column vectors), position, scale
struct RaceFxTransform {
    float m[9];
    RaceVec pos;
    float scale;
};
static_assert(sizeof(RaceFxTransform) == 0x34);
// CRect (left, bottom, right, top; bottom the larger y)
struct RaceRect {
    float left, bottom, right, top;
};

// ---- the game ---------------------------------------------------------------------------------

GAME_VAR(CRace*, currentRace, 0x020C8704);      // the running minigame (a CRace here)
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);
GAME_VAR(float, timeStep, 0x00C1A9A4);
GAME_VAR(uint8_t, transitionDisplayed, 0x020C8824);
GAME_VAR(void*, pathStore, 0x00D13CB4);
GAME_VAR(void*, effectManager, 0x00B6F1D4);
GAME_VAR(void*, effectLibrary, 0x00B6F1E8);
GAME_VAR(uint8_t*, alwaysValidEntity, 0x00C2A8E8);
GAME_VAR(int32_t, userStartResponse, 0x020C87AC);
GAME_VAR(void*, camera, 0x00C3CCF4);
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);
GAME_VAR(void*, raceHudPtr, 0x00C674C8);           // the race HUD (nullptr without one)
GAME_VAR(uint8_t*, aiTuningRace, 0x020C8900);      // the AI's tuning (+0x58 the racers' brake factor)
GAME_VAR(float, raceBoostRate, 0x00BC7034);        // the boost timer's rate (0.02)
GAME_VAR(int32_t, cameraMode, 0x00C3CCF8);
GAME_VAR(uint32_t, previousTimeInMs, 0x00C1A9AC);
inline void* const cameraShake = reinterpret_cast<void*>(0x00C3CE20);   // the camera's view offset
inline const float* const upVector = reinterpret_cast<const float*>(0x009E33DC);
inline const float* const zeroVector = reinterpret_cast<const float*>(0x00BCC564);

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, uint32_t size, uint32_t count, void* ctor);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(void, __thiscall, EntityPtr_Assign, 0x00478760, re::Ptr32<uint8_t>* ref, const re::Ptr32<uint8_t>* from);
// vectors
GAME_FN(RaceVec*, __cdecl, VectorAdd, 0x004129A0, RaceVec* out, const void* a, const void* b);
GAME_FN(RaceVec*, __cdecl, VectorSub, 0x00414B70, RaceVec* out, const void* a, const void* b);
GAME_FN(RaceVec*, __cdecl, ScaleVector, 0x00412320, RaceVec* out, float s, const RaceVec* v);
GAME_FN(RaceVec*, __cdecl, VectorScale, 0x00412350, RaceVec* out, const RaceVec* v, float s);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, RaceVec* v);
GAME_FN(float, __thiscall, Vector_NormaliseSafe, 0x00415130, RaceVec* v);
GAME_FN(void, __thiscall, Vector_AddTo, 0x00415200, RaceVec* v, const RaceVec* add);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const RaceVec* v);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const RaceVec* v);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const RaceVec* v);
GAME_FN(void, __thiscall, Vector_Scale, 0x0088A480, RaceVec* v, float s);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const void* a, const void* b);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const void* a, const void* b);
GAME_FN(float, __cdecl, DistanceSquared, 0x00472D00, const void* a, const void* b);
GAME_FN(float, __cdecl, DistanceBeyond, 0x0041A4D0, const void* a, const void* b, float range);
GAME_FN(float, __cdecl, MaxF, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, MinF, 0x00414470, float a, float b);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const RaceVec* v);
GAME_FN(uint8_t, __thiscall, Rect_IsPointInside, 0x00435AA0, const RaceRect* rect, const float* point);
// entities
GAME_FN(RaceVec*, __thiscall, Placeable_GetPosition, 0x00415460, uint8_t* placeable);
GAME_FN(RaceVec*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, RaceVec* out);
GAME_FN(RaceVec*, __thiscall, Placeable_GetRight, 0x00473600, uint8_t* placeable, RaceVec* out);
GAME_FN(RaceVec*, __thiscall, Entity_GetRight, 0x004736F0, uint8_t* entity, RaceVec* out);
GAME_FN(void, __thiscall, Matrix_SetRotateZOnly, 0x00412C20, void* matrix, float angle);
GAME_FN(void, __thiscall, Physical_SetMoveSpeed, 0x0041A440, uint8_t* physical, const float* speed);
GAME_FN(void, __thiscall, Physical_SetTurnSpeed, 0x004D3480, uint8_t* physical, const float* speed);
GAME_FN(void, __thiscall, Physical_SetPosition, 0x0041EE20, uint8_t* entity, const RaceVec* pos);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0045D3E0, uint8_t* entity, const RaceVec* force);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(void, __thiscall, Ped_MountBike, 0x0047D0D0, uint8_t* ped, uint8_t* bike);
GAME_FN(void, __thiscall, Ped_WarpIntoFreeSeat, 0x0049A580, uint8_t* ped, uint8_t* vehicle, int front);
GAME_FN(int32_t*, __thiscall, Vehicle_GetClass, 0x004CAF30, uint8_t* vehicle, int32_t* out);
GAME_FN(uint8_t*, __thiscall, Seats_GetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(void, __thiscall, Kart_Explode, 0x004AA530, uint8_t* vehicle);
GAME_FN(void, __thiscall, Kart_UpdateStatus, 0x004AA700, uint8_t* vehicle);
GAME_FN(void, __thiscall, Physical_SetTurnSpeedXYZ, 0x004A6E00, uint8_t* vehicle, float, float, float);
GAME_FN(uint8_t*, __cdecl, FindVehicle, 0x004D33F0, uint8_t* ped, float range, int, uint32_t, int);
GAME_FN(uint8_t*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(float*, __cdecl, FindPlayerCoors, 0x00446900, float* out);
// the world
GAME_FN(uint8_t, __cdecl, ProcessVerticalLine, 0x004629C0, const RaceVec* point, float zStop, RaceColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int buildings, int vehicles);
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightClear, 0x004617D0, const void* a, const void* b, int flags);
GAME_FN(void, __thiscall, ColPoint_Construct, 0x00415FA0, RaceColPoint* colPoint);
GAME_FN(void, __thiscall, ColPoint_Destruct, 0x00415500, RaceColPoint* colPoint);
GAME_FN(void, __thiscall, ColLine_Set, 0x00564950, RaceColLine* line, const void* start, const void* end);
GAME_FN(void, __thiscall, ColSphere_Set, 0x0056EBA0, RaceColSphere* sphere, float radius, const void* center,
        int surface, int piece);
GAME_FN(uint8_t, __cdecl, TestLineSphere, 0x00568FC0, const RaceColLine* line, const RaceColSphere* sphere);
// patrol paths
GAME_FN(uint8_t*, __thiscall, PathStore_Get, 0x005ED3A0, void* store, int32_t id);
GAME_FN(uint32_t, __thiscall, Path_GetNumNodes, 0x005EC670, uint8_t* path);
GAME_FN(float*, __thiscall, Path_GetNode, 0x005EC8C0, uint8_t* path, int32_t node);
// effects
GAME_FN(uint16_t, __thiscall, EffectLibrary_Find, 0x00667B60, void* library, const char* name);
GAME_FN(void, __thiscall, Fx_Start, 0x00668290, void* manager, const uint32_t* type, RaceFx* proxy, int);
GAME_FN(void, __thiscall, Fx_Kill, 0x00668BA0, RaceFx* proxy);
GAME_FN(void, __thiscall, Fx_Construct, 0x00668E90, RaceFx* proxy);
GAME_FN(void, __thiscall, Fx_Destruct, 0x00668EB0, RaceFx* proxy);
GAME_FN(void, __thiscall, Fx_SetTransform, 0x00668EC0, RaceFx* proxy, const RaceFxTransform* transform);
GAME_FN(void, __thiscall, Fx_SetPosition, 0x00669040, RaceFx* proxy, const void* pos, int);
// sounds
GAME_FN(int32_t, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void, __cdecl, Sounds_PlayLoop, 0x00597400, re::Ptr32<void>* slot, int32_t id, int pan, int, int volume, int, int);
GAME_FN(void, __thiscall, Sounds_PlayAt, 0x0059BAB0, void* sounds, int32_t id, int volume, int pan, int);
GAME_FN(int32_t, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, int32_t id, const float* pos, int, uintptr_t, int, int,
        int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Sounds_PlayFrontEnd, 0x0059C150, void* sounds, int sound);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, re::Ptr32<void>* sound, uintptr_t id);
GAME_FN(int, __cdecl, Sfx_DistanceVolume, 0x0059E3E0, float distance, int curve);
GAME_FN(float, __cdecl, Sfx_Curve, 0x0059E4F0, float value, int curve);
// the HUD, the radar, the camera, input
GAME_FN(void, __cdecl, BigMessage_Show, 0x005142A0, const char* key, float time);
GAME_FN(void, __thiscall, RaceHud_SetSplit, 0x00546970, void* hud, int ms);
GAME_FN(void, __thiscall, RaceHud_SetLap, 0x00546990, void* hud, int ms);
GAME_FN(int32_t, __thiscall, Radar_AddBlip, 0x0054A410, void* radar, int type, float x, float y, float z,
        uint32_t colour, int, int, uint8_t);
GAME_FN(void, __thiscall, Radar_SetBlipSize, 0x00548BF0, void* radar, int32_t blip, int size);
GAME_FN(void, __thiscall, Radar_RemoveBlip, 0x0054A940, void* radar, int32_t blip);
GAME_FN(void, __thiscall, CameraShake_Enable, 0x004F3DA0, void* shake, int on);
GAME_FN(void, __thiscall, CameraShake_SetOffset, 0x004F3D10, void* shake, float x, float y, float z);
GAME_FN(void, __thiscall, CameraShake_SetRotation, 0x004F3D30, void* shake, float x, float y, float z);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(float*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);   // (+0x24 the translation)
GAME_FN(uint8_t, __cdecl, Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(void, __thiscall, ActionInputs_SetRequest, 0x005F3A90, void* inputs, int button, int kind, float time);
// txds, sprites, fonts
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1610, void* dictionary, const char* name);
GAME_FN(void, __cdecl, Txd_RemoveTxd, 0x005F1830, int slot);
GAME_FN(uint8_t, __cdecl, Txd_LoadTxd, 0x005F2A80, int slot, const char* filename);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void, __cdecl, Txd_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __thiscall, Sprite_SetTexture, 0x0052C230, re::Ptr32<void>* sprite, const char* name);
GAME_FN(void, __thiscall, Sprite_Delete, 0x0052C6D0, re::Ptr32<void>* sprite);
GAME_FN(void, __thiscall, Pointer_Construct, 0x00667DA0, re::Ptr32<void>* p);
GAME_FN(void, __cdecl, FontStyle_Load, 0x00687BC0, int style);
GAME_FN(void, __cdecl, FontStyle_Unload, 0x00687D10, int style);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
// the Minigame base
GAME_FN(void, __thiscall, Minigame_Construct, 0x007059D0, CRace* self);
GAME_FN(void, __thiscall, Minigame_Destruct, 0x00705A50, CRace* self);
GAME_FN(int, __thiscall, Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, Popup_Show, 0x00705220, void* popup, int kind, const char* title, const char* buttons, int,
        int);

// ---- the race's own (called at the game's addresses, so modes apply) --------------------------

GAME_FN(void, __cdecl, Race_VolumePan, 0x0070CB60, const float* pos, int curve, int* volume, int* pan);
GAME_FN(void, __thiscall, Racer_CopyConstruct, 0x0070D2D0, RaceRacer* self, const RaceRacer* from);
GAME_FN(RaceRacer*, __thiscall, Race_GetRacer, 0x0070D390, CRace* self, uint8_t* ped);
GAME_FN(int32_t, __thiscall, Race_GetPosition, 0x0070D420, CRace* self, uint8_t* ped);
GAME_FN(float, __thiscall, Race_GetDuration, 0x0070D480, CRace* self);
GAME_FN(uint8_t, __thiscall, Race_HasFinished, 0x0070D500, CRace* self, uint8_t* ped);
GAME_FN(RacePathRange*, __thiscall, Race_FindRange, 0x0070D550, CRace* self, int node, RaceArray<RacePathRange>* list,
        int inclusive);
GAME_FN(RacePathRange*, __thiscall, Race_FindBranch, 0x0070D5B0, CRace* self, int node, int inclusive);
GAME_FN(uint8_t, __thiscall, Racer_TryBranch, 0x0070D620, RaceRacer* self);
GAME_FN(uint8_t, __thiscall, Race_IsJumpNode, 0x0070D9A0, CRace* self, int path, int node);
GAME_FN(void, __thiscall, IntArray_Insert, 0x0070E380, RaceArray<int32_t>* self, uint32_t index, const int32_t* value);
GAME_FN(void, __thiscall, Race_UpdateBarriers, 0x0070CCB0, CRace* self);
GAME_FN(void, __thiscall, Race_CheckBarriers, 0x0070E3E0, CRace* self, uint8_t* ped);
GAME_FN(void, __thiscall, Race_UpdateBoostPads, 0x0070E540, CRace* self);
GAME_FN(void, __thiscall, Race_CheckBoostPads, 0x0070E8D0, CRace* self, uint8_t* ped);
GAME_FN(void, __thiscall, Race_PlaceWaypoint, 0x0070EB10, CRace* self, const RaceVec* pos, const RaceVec* dir, int final);
GAME_FN(void, __thiscall, Racer_Assign, 0x0070EE70, RaceRacer* self, const RaceRacer* from);
GAME_FN(float, __thiscall, Race_GetRaceTime, 0x0070EF50, CRace* self, uint8_t* ped);
GAME_FN(void, __thiscall, Racer_Advance, 0x0070F070, RaceRacer* self, int32_t* progress, int checkGround);
GAME_FN(void, __thiscall, Racer_SkipToHighlight, 0x0070F1E0, RaceRacer* self);
GAME_FN(void, __thiscall, Racer_CheckJump, 0x0070F240, RaceRacer* self);
GAME_FN(void*, __thiscall, Race_LoadTransition, 0x0070F860, CRace* self, const char* txd, const char* texture);
GAME_FN(void, __thiscall, RacerArray_Delete, 0x0070FB50, RaceArray<RaceRacer>* self);
GAME_FN(void, __thiscall, RacerArray_RemoveAt, 0x0070FBA0, RaceArray<RaceRacer>* self, uint32_t index);
GAME_FN(void, __thiscall, RacerArray_Clear, 0x0070FBE0, RaceArray<RaceRacer>* self);
GAME_FN(void, __thiscall, RecordArray_Delete, 0x0070FC40, RaceArray<RaceRecord>* self);
GAME_FN(void, __thiscall, RacerArray_Reserve, 0x0070FC90, RaceArray<RaceRacer>* self, uint32_t capacity, int setCount);
GAME_FN(void, __thiscall, RecordArray_Reserve, 0x0070FD70, RaceArray<RaceRecord>* self, uint32_t capacity,
        int setCount);
GAME_FN(void, __thiscall, Race_Destruct, 0x0070FEA0, CRace* self);
GAME_FN(void, __thiscall, Race_CheckKart, 0x007101F0, CRace* self, uint8_t* ped);
GAME_FN(void, __thiscall, Race_MarkFinished, 0x00710610, CRace* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Race_GetNodeAhead, 0x00710710, CRace* self, RaceVec* out, uint8_t* ped, int ahead,
        int highlighted);
GAME_FN(void, __thiscall, Racer_NextHighlight, 0x00710810, RaceRacer* self, int32_t* progress);
GAME_FN(void, __thiscall, Racer_NextNode, 0x007108B0, RaceRacer* self, int tryBranch, int checkGround);
GAME_FN(void, __thiscall, Race_ShowNextWaypoint, 0x00710960, CRace* self, uint8_t* ped);
GAME_FN(void, __thiscall, Race_AddRacer, 0x00710B60, CRace* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Race_IsAhead, 0x00710C50, CRace* self, const RaceRacer* a, const RaceRacer* b);
GAME_FN(float, __thiscall, Race_DistanceAhead, 0x00710D80, CRace* self, const RaceRacer* a, const RaceRacer* b);
GAME_FN(void, __thiscall, Race_Sort, 0x00711510, CRace* self);
GAME_FN(uint8_t, __thiscall, Race_UpdateRacerNode, 0x00711610, CRace* self, RaceRacer* racer);
GAME_FN(float, __thiscall, Race_DistanceAheadOfPlayer, 0x00711B80, CRace* self, uint8_t* ped);
GAME_FN(void, __thiscall, Race_GetTarget, 0x0070DA90, CRace* self, RaceVec* out, uint8_t* ped, float lookAhead);
// the chunk before's (another pass's)
GAME_FN(void, __thiscall, Race_DestroyTransition, 0x0070B830, CRace* self);
GAME_FN(uint8_t, __thiscall, Race_ProgressLess, 0x0070B5C0, const int32_t* a, const int32_t* b);
GAME_FN(RaceVec*, __thiscall, Racer_GetNextNodePos, 0x0070B680, RaceRacer* self, RaceVec* out);
GAME_FN(RaceVec*, __thiscall, Racer_GetPrevNodePos, 0x0070B710, RaceRacer* self, RaceVec* out);
GAME_FN(uint8_t, __thiscall, Race_IsBottleneck, 0x0070B980, CRace* self, int node);
GAME_FN(RaceVec*, __thiscall, RaceProgress_GetPos, 0x0070B9C0, const int32_t* progress, RaceVec* out);
GAME_FN(void, __thiscall, Racer_Construct, 0x0070B900, RaceRacer* self);
GAME_FN(void, __thiscall, Array8_Reserve, 0x0070B870, void* array, uint32_t capacity, int setCount);
GAME_FN(void, __thiscall, Array4_Reserve, 0x006DB480, void* array, uint32_t capacity, int setCount);
GAME_FN(void, __thiscall, Array12_Reserve, 0x004F7990, void* array, uint32_t capacity, int setCount);

// ---- helpers ----------------------------------------------------------------------------------

using re::At;
// a placeable's position (its matrix's, or the one in the placeable)
inline float* EntityPos(uint8_t* e) {
    uint8_t* m = At<uint8_t*>(e, 0x14);
    return m != nullptr ? reinterpret_cast<float*>(m + 0x30) : reinterpret_cast<float*>(e + 4);
}
inline uint8_t* PedVehicle(uint8_t* ped) {
    return At<uint8_t*>(ped, 0x1554);
}
inline RaceVec* MoveSpeed(uint8_t* physical) {
    return &At<RaceVec>(physical, 0x124);
}
// the move speed's squared length, the game's order
inline float SpeedSqr(uint8_t* physical) {
    const RaceVec& v = *MoveSpeed(physical);
    float xy = v.x * v.x + v.y * v.y;
    return xy + v.z * v.z;
}
inline uint8_t* GetPath(int32_t id) {
    return PathStore_Get(pathStore, id);
}
// the path a racer's progress (node, lap, shortcut) runs on: the shortcut's or the race's
inline int32_t ProgressPathId(const RacePathRange* shortcut) {
    return shortcut != nullptr ? shortcut->path : currentRace->path;
}
inline void CopyVec(RaceVec& out, const float* in) {
    out.x = in[0];
    out.y = in[1];
    out.z = in[2];
}
// the game's constants are floats (its doubles here are floats widened): float math
constexpr float kRacePi = 3.14159274f;
constexpr float kRaceTwoPi = 6.28318548f;

// a 64-bit-safe array size (the game's operator new[] overflow guard)
inline uint32_t ArrayBytes(uint32_t count, uint32_t size) {
    uint64_t n = uint64_t(count) * size;
    return (n >> 32) != 0 ? 0xFFFFFFFFu : uint32_t(n);
}
// an int array grown in place (the inlined reserve: a new block, the elements copied)
inline void IntArrayGrowInline(RaceArray<int32_t>& a, uint32_t capacity) {
    a.capacity = capacity;
    int32_t* data = nullptr;
    if (capacity != 0) {
        data = static_cast<int32_t*>(Malloc(ArrayBytes(capacity, 4)));
    }
    if (a.data != nullptr) {
        for (uint32_t i = 0; i < a.count; i++) {
            data[i] = a.data[i];
        }
        Free(a.data);
    }
    a.data = data;
}
inline int32_t IntArrayFind(const RaceArray<int32_t>& a, int32_t value) {
    for (uint32_t i = 0; i < a.count; i++) {
        if (a.data[i] == value) {
            return int32_t(i);
        }
    }
    return -1;
}
// a colpoint's entity reference released (the CColPoint destructor's inlined body)
inline void ReleaseColPointEntity(re::Ptr32<uint8_t>& entity) {
    if (entity != nullptr) {
        CleanUpOldReference(entity, &entity);
    }
}
inline uint32_t EffectType(const char* name) {
    return EffectLibrary_Find(effectLibrary, name);
}
// the unsigned ms count as the x87 holds it (fild is exact; the 2^32 fix-up is an fadd,
// rounded to 24 bits); a difference of two is then rounded once, as float(a - b)
inline double MsX87(uint32_t ms) {
    double d = double(int32_t(ms));
    if (int32_t(ms) < 0) {
        d = double(float(d + 4294967296.0));
    }
    return d;
}

#define RACE45_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

// ---- 0x70CB60: a sound's volume and pan from the player's vehicle -----------------------------

// (cdecl) the volume by the 2D distance to the player's vehicle on `curve` (0..1024 through
// the curve 4), the pan by the direction's x (-90..90); full volume, centred without one
void __cdecl VolumePan(const float* pos, int curve, int* volume, int* pan) {
    uint8_t* player = playerPed;
    if (player != nullptr && PedVehicle(player) != nullptr) {
        RaceVec d;
        VectorSub(&d, EntityPos(PedVehicle(player)), pos);
        float sum = d.x * d.x + d.y * d.y;
        float distance = crt::Sqrtf(std::fabs(sum));
        int v = Sfx_DistanceVolume(distance, curve);
        *volume = v;
        float scaled = float(v) * 0.0009765625f;
        *volume = int(Sfx_Curve(scaled, 4) * 1024.0f);   // (the game's _ftol: truncation)
        VectorSub(&d, pos, EntityPos(PedVehicle(playerPed)));
        Vector_Normalise(&d);
        *pan = int(d.x * 90.0f);
        return;
    }
    *pan = 0;
    *volume = int(Sfx_Curve(1.0f, 4) * 1024.0f);
}
RACE45_REVERSED("CRace::VolumePan", 0x0070CB60, &VolumePan);


// ---- 0x70CCB0: the barriers' effects ------------------------------------------------------------

// (thiscall) on (state 2, the player on a vehicle): each barrier's sparks loop at its middle
// (at the vehicle's height) and its four effects: a far glow at each end, two arcs between;
// charging (1): the end glows growing over 1.8 s (0.1..1), the arcs killed; else all off
void __fastcall UpdateBarriers(CRace* self, re::Edx) {
    uint8_t* player = playerPed;
    if (self->barrierState == 2 && player != nullptr && PedVehicle(player) != nullptr) {
        uint32_t far = EffectType("ElectrocuteLRG_Far");
        uint32_t arc = EffectType("ElectrocuteArc");
        for (int i = 0; i < self->barrierCount; i++) {
            RaceBarrier& b = self->barriers[i];
            RaceFx* fx = self->barrierFx[i];
            RaceVec sum;
            RaceVec* t = VectorAdd(&sum, &b.a, &b.b);
            RaceVec pos;
            pos.x = t->x * 0.5f;
            pos.y = t->y * 0.5f;
            pos.z = t->z * 0.5f;
            pos.z = EntityPos(playerPed)[2];
            uint8_t* vehicle = PedVehicle(playerPed);
            if (vehicle != nullptr) {
                pos.z = EntityPos(vehicle)[2];
            }
            int volume, pan;
            Race_VolumePan(&pos.x, 5, &volume, &pan);
            Sounds_PlayLoop(&self->barrierSounds[i], Sounds_GetId(sounds, "PlasmaSparks"), pan, 1, volume, 0, 0);
            if (fx[0].effect == nullptr) {
                Fx_Start(effectManager, &far, &fx[0], 0);
                Fx_SetPosition(&fx[0], &b.a, 0);
            }
            if (fx[1].effect == nullptr) {
                Fx_Start(effectManager, &far, &fx[1], 0);
                Fx_SetPosition(&fx[1], &b.b, 0);
            }
            if (fx[2].effect == nullptr) {
                Fx_Start(effectManager, &arc, &fx[2], 0);
                RaceFxTransform m;
                m.scale = 1.0f;
                RaceVec tmp;
                RaceVec d = *VectorSub(&tmp, &b.a, &b.b);
                Vector_Normalise(&d);
                m.m[0] = d.x;
                m.m[1] = 0.0f;
                m.m[2] = d.y;
                m.m[3] = d.y;
                m.m[4] = 0.0f;
                m.m[5] = -d.x;
                m.m[6] = 0.0f;
                m.m[7] = 1.0f;
                m.m[8] = 0.0f;
                RaceVec* c = VectorAdd(&tmp, &b.a, &b.b);
                m.pos.x = c->x * 0.5f;
                m.pos.y = c->y * 0.5f;
                m.pos.z = c->z * 0.5f;
                Fx_SetTransform(&fx[2], &m);
            }
            if (fx[3].effect == nullptr) {
                Fx_Start(effectManager, &arc, &fx[3], 0);
                RaceFxTransform m;
                m.scale = 1.0f;
                RaceVec tmp;
                RaceVec d = *VectorSub(&tmp, &b.a, &b.b);
                Vector_Normalise(&d);
                m.m[0] = d.x;
                m.m[1] = d.y;
                m.m[2] = 0.0f;
                m.m[3] = d.y;
                m.m[4] = -d.x;
                m.m[5] = 0.0f;
                m.m[6] = 0.0f;
                m.m[7] = 0.0f;
                m.m[8] = 1.0f;
                RaceVec* c = VectorAdd(&tmp, &b.a, &b.b);
                m.pos.x = c->x * 0.5f;
                m.pos.y = c->y * 0.5f;
                m.pos.z = c->z * 0.5f;
                Fx_SetTransform(&fx[3], &m);
            }
        }
    } else if (self->barrierState == 1 && player != nullptr && PedVehicle(player) != nullptr) {
        uint32_t far = EffectType("ElectrocuteLRG_Far");
        for (int i = 0; i < self->barrierCount; i++) {
            RaceBarrier& b = self->barriers[i];
            RaceFx* fx = self->barrierFx[i];
            RaceFxTransform m;
            m.scale = 1.0f;
            float elapsed = float(MsX87(timeInMs) - MsX87(self->barrierTime));
            float e = elapsed / 1800.0f;
            e = e * 0.9f;
            e = e + 0.1f;
            m.m[0] = e;
            m.m[1] = 0.0f;
            m.m[2] = 0.0f;
            m.m[3] = 0.0f;
            m.m[4] = 0.0f;
            m.m[5] = -e;
            m.m[6] = 0.0f;
            m.m[7] = e;
            m.m[8] = 0.0f;
            if (fx[0].effect == nullptr) {
                Fx_Start(effectManager, &far, &fx[0], 0);
            }
            RaceVec up = {0.0f, 0.0f, 1.0f};
            RaceVec tmp;
            m.pos = *VectorAdd(&tmp, &b.a, &up);
            Fx_SetTransform(&fx[0], &m);
            if (fx[1].effect == nullptr) {
                Fx_Start(effectManager, &far, &fx[1], 0);
            }
            RaceVec up2 = {0.0f, 0.0f, 1.0f};
            m.pos = *VectorAdd(&tmp, &b.b, &up2);
            Fx_SetTransform(&fx[1], &m);
            Fx_Kill(&fx[2]);
            Fx_Kill(&fx[3]);
        }
    } else {
        for (int i = 0; i < self->barrierCount; i++) {
            Fx_Kill(&self->barrierFx[i][0]);
            Fx_Kill(&self->barrierFx[i][1]);
            Fx_Kill(&self->barrierFx[i][2]);
            Fx_Kill(&self->barrierFx[i][3]);
        }
    }
}
RACE45_REVERSED("CRace::UpdateBarriers", 0x0070CCB0, &UpdateBarriers);

// ---- 0x70D2D0..0x70D5B0: the racer list ---------------------------------------------------------

// (thiscall) a racer copied into a new one (the ped's reference registered)
RaceRacer* __fastcall RacerCopyConstruct(RaceRacer* self, re::Edx, const RaceRacer* from) {
    uint8_t* ped = from->ped;
    self->ped = ped;
    if (ped != nullptr) {
        RegisterReference(ped, &self->ped);
    }
    const uint32_t* src = reinterpret_cast<const uint32_t*>(from);
    uint32_t* dst = reinterpret_cast<uint32_t*>(self);
    for (int i = 1; i < 8; i++) {
        dst[i] = src[i];
    }
    self->boosting = from->boosting;
    for (int i = 9; i < 0x1B; i++) {
        dst[i] = src[i];
    }
    return self;
}
RACE45_REVERSED("RaceRacer::CopyConstruct", 0x0070D2D0, &RacerCopyConstruct);

// the racer's index (searching from the end), -1 for none
inline int32_t FindRacer(const CRace* self, const uint8_t* ped) {
    if (ped != nullptr) {
        for (int32_t i = int32_t(self->racers.count) - 1; i >= 0; i--) {
            if (self->racers.data[i].ped == ped) {
                return i;
            }
        }
    }
    return -1;
}

// (thiscall) the ped's racer, nullptr for none
RaceRacer* __fastcall GetRacer(CRace* self, re::Edx, uint8_t* ped) {
    int32_t i = FindRacer(self, ped);
    if (i == -1) {
        return nullptr;
    }
    return &self->racers.data[i];
}
RACE45_REVERSED("CRace::GetRacer", 0x0070D390, &GetRacer);

// (thiscall) the ped races
uint8_t __fastcall HasRacer(CRace* self, re::Edx, uint8_t* ped) {
    return FindRacer(self, ped) != -1 ? 1 : 0;
}
RACE45_REVERSED("CRace::HasRacer", 0x0070D3E0, &HasRacer);

// (thiscall) the ped's place, 1-based (0 for none)
int32_t __fastcall GetPosition(CRace* self, re::Edx, uint8_t* ped) {
    return FindRacer(self, ped) + 1;
}
// (register hazard: the game's UpdateRacer 0x712CA0 calls it twice in a row relying on ecx
// surviving it: turn on together with 0x712CA0)
RACE45_REVERSED("CRace::GetPosition", 0x0070D420, &GetPosition);

// (thiscall) the ped in a place (0-based; no bounds check)
uint8_t* __fastcall GetRacerInPosition(CRace* self, re::Edx, int32_t index) {
    return self->racers.data[index].ped;
}
RACE45_REVERSED("CRace::GetRacerInPosition", 0x0070D460, &GetRacerInPosition);

// (thiscall) seconds since the start: to the last racer's finish once all have finished;
// 0 before the start
float __fastcall GetDuration(CRace* self, re::Edx) {
    if (self->started == 0) {
        return 0.0f;
    }
    int32_t n = self->racerCount;
    if (n == self->finishedCount && n >= 1) {
        int32_t ms = int32_t(self->racers.data[n - 1].finishTime - self->startTime);
        return float(double(ms) / 1000.0);
    }
    uint32_t elapsed = timeInMs - self->startTime;
    return float(MsX87(elapsed) / 1000.0);
}
RACE45_REVERSED("CRace::GetDuration", 0x0070D480, &GetDuration);

// (thiscall) the ped has finished: among the first finishedCount (also true for a ped that
// isn't racing: its index -1)
uint8_t __fastcall HasFinished(CRace* self, re::Edx, uint8_t* ped) {
    return FindRacer(self, ped) < self->finishedCount ? 1 : 0;
}
// (register hazard: the game's GetRaceTime 0x70EF50 and Update 0x7123C0 call GetRacer right
// after it relying on ecx surviving it: turn on together with them)
RACE45_REVERSED("CRace::HasFinished", 0x0070D500, &HasFinished);

// (thiscall) the path range of `list` the race path's node is inside (strictly, or inclusive)
RacePathRange* __fastcall FindRange(CRace* self, re::Edx, int node, RaceArray<RacePathRange>* list, int inclusive) {
    GetPath(self->path);   // (looked up, unused)
    for (uint32_t i = 0; i < list->count; i++) {
        RacePathRange* r = &list->data[i];
        if (re::Flag(inclusive)) {
            if (r->first <= node && node <= r->last) {
                return r;
            }
        } else if (r->first < node && node < r->last) {
            return r;
        }
    }
    return nullptr;
}
RACE45_REVERSED("CRace::FindRange", 0x0070D550, &FindRange);

// (thiscall) the shortcut, else the jump path, the race path's node is on (none for the
// first and last nodes)
RacePathRange* __fastcall FindBranch(CRace* self, re::Edx, int node, int inclusive) {
    uint8_t* path = GetPath(self->path);
    if (node != 0 && node != int(Path_GetNumNodes(path) - 1)) {
        RacePathRange* r = Race_FindRange(self, node, &self->shortcuts, inclusive);
        if (r == nullptr) {
            r = Race_FindRange(self, node, &self->jumpPaths, inclusive);
        }
        return r;
    }
    return nullptr;
}
RACE45_REVERSED("CRace::FindBranch", 0x0070D5B0, &FindBranch);

// a dot product, the game's order here: (a.y*b.y + a.x*b.x) + a.z*b.z
inline float DotYXZ(const RaceVec& a, const RaceVec& b) {
    float s = a.y * b.y + a.x * b.x;
    return s + a.z * b.z;
}

// one candidate branch for TryBranch: by the racer's chance, when it starts at the racer's
// node and the racer heads into it (both the direction to it and the ped's forward within
// `limit` of the branch's start)
inline bool BranchFits(RaceRacer* self, RacePathRange* r, float limit) {
    if (r->first != self->node) {
        return false;
    }
    if ((Rand() * 99) / 0x7FFF >= self->shortcutChance) {
        return false;
    }
    RaceVec start;
    CopyVec(start, Path_GetNode(GetPath(r->path), 0));
    RaceVec prev;
    Racer_GetPrevNodePos(self, &prev);
    RaceVec along, to;
    VectorSub(&along, &start, &prev);
    VectorSub(&to, &start, EntityPos(self->ped));
    Vector_Normalise(&along);
    Vector_Normalise(&to);
    float dot = DotYXZ(along, to);
    if (!(dot > limit)) {
        return false;
    }
    RaceVec forward;
    RaceVec* f = Placeable_GetForward(self->ped, &forward);
    float s = f->y * along.y + f->x * along.x;
    float facing = s + f->z * along.z;
    return facing > limit;
}

// (thiscall) a racer not on a branch, at its node and nearly stopped on its vehicle (or on
// foot), no other vehicle within 3 m but the player's or its own: takes a shortcut, else
// a jump path, starting there (its node 0)
uint8_t __fastcall TryBranch(RaceRacer* self, re::Edx) {
    if (self->shortcut != nullptr) {
        return 0;
    }
    if (self->ped != nullptr && Ped_IsSeatedInVehicle(self->ped) != 0) {
        float speed = SpeedSqr(PedVehicle(self->ped));
        if (speed > 0.15f) {
            return 0;
        }
    }
    uint8_t* near = FindVehicle(self->ped, 3.0f, -1, 0, 1);
    if (near != nullptr && near != FindPlayerVehicle(0) && near != PedVehicle(self->ped)) {
        return 0;
    }
    for (uint32_t i = 0; i < self->race->shortcuts.count; i++) {
        RacePathRange* r = &self->race->shortcuts.data[i];
        if (r == nullptr) {
            break;
        }
        if (BranchFits(self, r, 0.8f)) {
            self->shortcut = r;
            self->node = 0;
            return 1;
        }
    }
    for (uint32_t i = 0; i < self->race->jumpPaths.count; i++) {
        RacePathRange* r = &self->race->jumpPaths.data[i];
        if (r == nullptr) {
            return 0;
        }
        if (BranchFits(self, r, 0.85f)) {
            self->shortcut = r;
            self->node = 0;
            return 1;
        }
    }
    return 0;
}
RACE45_REVERSED("RaceRacer::TryBranch", 0x0070D620, &TryBranch);

// (thiscall) the path's node is a jump node
uint8_t __fastcall IsJumpNode(CRace* self, re::Edx, int path, int node) {
    for (uint32_t i = 0; i < self->jumpNodes.count; i++) {
        if (self->jumpNodes.data[i].path == path && self->jumpNodes.data[i].node == node) {
            return 1;
        }
    }
    return 0;
}
RACE45_REVERSED("CRace::IsJumpNode", 0x0070D9A0, &IsJumpNode);

// (thiscall) a racer stands in the rectangle (its 2D position)
uint8_t __fastcall AnyRacerInRect(CRace* self, re::Edx, const RaceRect* rect) {
    for (uint32_t i = 0; i < self->racers.count; i++) {
        uint8_t* ped = self->racers.data[i].ped;
        if (ped == nullptr) {
            continue;
        }
        float* p = EntityPos(ped);
        float x = p[0];
        float y = p[1];
        if (!(rect->left > x) && !(rect->right < x) && !(rect->top > y) && rect->bottom >= y) {
            return 1;
        }
    }
    return 0;
}
RACE45_REVERSED("CRace::AnyRacerInRect", 0x0070D9F0, &AnyRacerInRect);

// ---- 0x70DA90: where a racer steers ------------------------------------------------------------

// N += R*k, P += R*k, Nx += Q*k: the lane offset of the path's segment (k = lane * scale)
inline void OffsetLane(RaceVec& node, RaceVec& prev, RaceVec& next, const RaceVec& side, const RaceVec& q,
                       const RaceRacer* racer, float scale) {
    RaceVec tmp;
    float k = float(racer->lane) * scale;
    Vector_AddTo(&node, ScaleVector(&tmp, k, &side));
    k = float(racer->lane) * scale;
    Vector_AddTo(&prev, ScaleVector(&tmp, k, &side));
    k = float(racer->lane) * scale;
    Vector_AddTo(&next, ScaleVector(&tmp, k, &q));
}

// (thiscall) the racer's target: its node; moving forwards, its lane offset to the side of
// the path (a bottleneck node: 0.1 m a lane; else 2.5 m on a bike, 1.25 m otherwise when the
// lane's spot is in sight, 0.1 m when not); heading for the node: a bike on the ground
// with all its wheels down (states 1 or 6) aims past the node by how much closer the
// previous node is; else, past 5 m from the node and 1 m from the previous, a blend of
// its heading and the direction to the node (by how far along the segment it is), then of
// that and the node direction by `lookAhead` (x0.4 on a car), 5 m ahead of the vehicle
void __fastcall GetTarget(CRace* self, re::Edx, RaceVec* out, uint8_t* ped, float lookAhead) {
    RaceRacer* racer = Race_GetRacer(self, ped);
    uint8_t* path = GetPath(ProgressPathId(racer->shortcut));
    CopyVec(*out, Path_GetNode(path, racer->node));
    uint8_t* vehicle = PedVehicle(ped);
    if (vehicle == nullptr) {
        return;
    }
    float speed = SpeedSqr(vehicle);
    if (speed < 0.001f) {
        return;
    }
    RaceVec forward;
    RaceVec* f = Placeable_GetForward(vehicle, &forward);
    const RaceVec& v = *MoveSpeed(vehicle);
    float s = v.y * f->y + v.x * f->x;
    float along = s + v.z * f->z;
    if (along < 0.0f) {
        return;
    }
    RaceVec node, prev, next;
    CopyVec(node, Path_GetNode(path, racer->node));
    Racer_GetPrevNodePos(racer, &prev);
    Racer_GetNextNodePos(racer, &next);
    RaceVec d;
    VectorSub(&d, &next, &node);
    Vector_Normalise(&d);
    RaceVec q = {d.y, -d.x, d.z};
    if (racer->lane != 0) {
        RaceVec side;
        VectorSub(&side, &node, &prev);
        Vector_Normalise(&side);
        float sx = side.x;
        side.x = side.y;
        side.y = -sx;
        if (Race_IsBottleneck(self, racer->node) != 0) {
            OffsetLane(node, prev, next, side, q, racer, 0.1f);
        } else {
            int32_t kind;
            int32_t* cls = Vehicle_GetClass(PedVehicle(ped), &kind);
            float* from = EntityPos(PedVehicle(ped));
            RaceVec tmp, spot, high;
            if (*cls == 7) {
                float k = float(racer->lane) * 3.3f;
                VectorAdd(&spot, &node, ScaleVector(&tmp, k, &side));
                VectorAdd(&high, &spot, upVector);
                if (GetIsLineOfSightClear(from, &high, 1) != 0) {
                    OffsetLane(node, prev, next, side, q, racer, 2.5f);
                } else {
                    OffsetLane(node, prev, next, side, q, racer, 0.1f);
                }
            } else {
                float k = float(racer->lane) * 1.5f;
                VectorAdd(&spot, &node, ScaleVector(&tmp, k, &side));
                VectorAdd(&high, &spot, upVector);
                if (GetIsLineOfSightClear(from, &high, 1) != 0) {
                    OffsetLane(node, prev, next, side, q, racer, 1.25f);
                } else {
                    OffsetLane(node, prev, next, side, q, racer, 0.1f);
                }
            }
        }
    }
    RaceVec heading = *MoveSpeed(PedVehicle(ped));
    Vector_Normalise(&heading);
    RaceVec toNode;
    VectorSub(&toNode, &node, EntityPos(PedVehicle(ped)));
    Vector_Normalise(&toNode);
    float t = toNode.x * heading.x + heading.y * toNode.y;
    float ahead = t + heading.z * toNode.z;
    if (!(ahead > -0.01f)) {
        return;
    }
    bool airborneBike = false;
    uint8_t* car = PedVehicle(ped);
    if (At<int32_t>(car, 0x46C) == 0) {
        int32_t kind;
        if (*Vehicle_GetClass(car, &kind) == 7) {
            uint8_t* veh = PedVehicle(ped);
            float* from = EntityPos(veh);
            RaceVec high;
            VectorAdd(&high, &next, upVector);
            if (GetIsLineOfSightClear(from, &high, 1) != 0) {
                airborneBike = true;
                for (uint8_t w = 0; w < 4; w++) {
                    uint8_t state = At<uint8_t>(veh, 0x6E6 + w * 0x28);
                    if (state != 1 && state != 6) {
                        airborneBike = false;
                        break;
                    }
                }
            }
        }
    }
    float toNodeDist = Distance(EntityPos(ped), &node);
    float toPrevDist = Distance(EntityPos(ped), &prev);
    if (toNodeDist > toPrevDist && toPrevDist > 1.0f && toNodeDist > 5.0f) {
        if (airborneBike) {
            return;
        }
        float span = Distance(&node, &prev) * 0.4f;
        float r = toPrevDist / span;
        float u = MinF(1.0f, MaxF(0.0f, r));
        RaceVec a, b;
        VectorScale(&a, &toNode, u);
        VectorScale(&b, &heading, 1.0f - u);
        RaceVec sum;
        heading = *VectorAdd(&sum, &b, &a);
        if (At<int32_t>(PedVehicle(ped), 0x46C) != 1) {
            lookAhead = lookAhead * 0.4f;
        }
        VectorScale(&a, &toNode, 1.0f - lookAhead);
        VectorScale(&b, &heading, lookAhead);
        heading = *VectorAdd(&sum, &b, &a);
        Vector_Normalise(&heading);
        RaceVec step;
        step.x = heading.x * 5.0f;
        step.y = heading.y * 5.0f;
        step.z = heading.z * 5.0f;
        RaceVec result;
        *out = *VectorAdd(&result, EntityPos(PedVehicle(ped)), &step);
        return;
    }
    if (airborneBike) {
        float k = toPrevDist - toNodeDist;
        RaceVec step;
        step.x = k * d.x;
        step.y = k * d.y;
        step.z = k * d.z;
        RaceVec result;
        *out = *VectorAdd(&result, &node, &step);
    }
}
RACE45_REVERSED("CRace::GetTarget", 0x0070DA90, &GetTarget);

// ---- 0x70E380..0x70F010 -------------------------------------------------------------------------

// (thiscall) an int inserted at `index` (grown x2, from 10, when full)
void __fastcall IntArrayInsert(RaceArray<int32_t>* self, re::Edx, uint32_t index, const int32_t* value) {
    if (self->count == self->capacity) {
        Array4_Reserve(self, self->capacity != 0 ? self->capacity * 2 : 10, 0);
    }
    for (uint32_t i = self->count; index < i; i--) {
        self->data[i] = self->data[i - 1];
    }
    self->data[index] = *value;
    self->count++;
}
RACE45_REVERSED("RaceIntArray::Insert", 0x0070E380, &IntArrayInsert);

// (thiscall) a go-kart (class 10, not wrecked) driven into a barrier's beam (a line between
// its ends at the kart's height, against a 2 m sphere) 5 s after its last respawn blows up;
// only while the barriers are on
void __fastcall CheckBarriers(CRace* self, re::Edx, uint8_t* ped) {
    if (self->barrierState != 2 || ped == nullptr || PedVehicle(ped) == nullptr) {
        return;
    }
    if (Ped_IsSeatedInVehicle(ped) == 0) {
        return;
    }
    int32_t kind;
    if (*Vehicle_GetClass(PedVehicle(ped), &kind) != 10) {
        return;
    }
    uint8_t* kart = PedVehicle(ped);
    if ((At<uint8_t>(kart, 0x108) & 0xF8) == 0x28) {
        return;
    }
    if (!(At<uint32_t>(kart, 0x348) + 5000 < timeInMs)) {
        return;
    }
    for (int i = 0; i < self->barrierCount; i++) {
        RaceBarrier& b = self->barriers[i];
        RaceColLine line;
        ColLine_Set(&line, &b.a, &b.b);
        uint8_t* vehicle = PedVehicle(ped);
        line.start[2] = EntityPos(vehicle)[2];
        line.end[2] = EntityPos(vehicle)[2];
        RaceColSphere sphere;
        sphere.center[0] = 0.0f;
        sphere.center[1] = 0.0f;
        sphere.center[2] = 0.0f;
        ColSphere_Set(&sphere, 2.0f, EntityPos(vehicle), 0, 0);
        if (TestLineSphere(&line, &sphere) != 0) {
            Kart_Explode(PedVehicle(ped));
            return;
        }
    }
}
RACE45_REVERSED("CRace::CheckBarriers", 0x0070E3E0, &CheckBarriers);

// the boost pad's / waypoint's transform: its normal n and facing f give r = f x n, u = r x n
// (the columns r, n, u)
inline void FacingTransform(RaceFxTransform& m, const RaceVec& f, const RaceVec& n) {
    float rx = f.y * n.z - f.z * n.y;
    float ry = n.x * f.z - f.x * n.z;
    float rz = n.y * f.x - n.x * f.y;
    float ux = ry * n.z - rz * n.y;
    float uy = rz * n.x - rx * n.z;
    float uz = rx * n.y - ry * n.x;
    m.m[0] = rx;
    m.m[1] = n.x;
    m.m[2] = ux;
    m.m[3] = ry;
    m.m[4] = n.y;
    m.m[5] = uy;
    m.m[6] = rz;
    m.m[7] = n.z;
    m.m[8] = uz;
}

// (thiscall) each boost pad within 100 m (2D) of the camera: its effect on the ground
// (0.25 m up), facing its heading; farther ones killed
void __fastcall UpdateBoostPads(CRace* self, re::Edx) {
    for (int i = 0; i < self->boostCount; i++) {
        RaceVec& pad = self->boosts[i];
        float* cam = GetLocalTransform(GetCameraNode(camera)) + 9;
        RaceVec d;
        VectorSub(&d, &pad, cam);
        d.z = 0.0f;
        float xy = d.y * d.y + d.x * d.x;
        float dist = xy + 0.0f;
        dist = dist - 10000.0f;
        if (!(dist < 0.0f)) {
            Fx_Kill(&self->boostFx[i]);
            continue;
        }
        RaceVec n = {0.0f, 0.0f, 1.0f};
        RaceVec pos = pad;   // (the z holds the heading until the ground is found)
        float angle = (pad.z / 180.0f) * kRacePi;
        float s = trk::CrtSin(angle);
        RaceVec f;
        f.x = -s;
        float c = trk::CrtCos(angle);
        f.y = -c;
        f.z = 0.0f;
        RaceColPoint col;
        col.pos.x = 0.0f;
        col.pos.y = 0.0f;
        col.pos.z = 0.0f;
        col.normal.x = 0.0f;
        col.normal.y = 0.0f;
        col.normal.z = 0.0f;
        col.depth = 3.4028235e38f;
        col.entity = nullptr;
        RaceVec from = {pad.x, pad.y, 1000.0f};
        re::Ptr32<uint8_t> hit = nullptr;
        if (ProcessVerticalLine(&from, -100.0f, &col, &hit, 1, 0) != 0) {
            pos.x = col.pos.x;
            pos.y = col.pos.y;
            pos.z = col.pos.z + 0.25f;
            n = col.normal;
        }
        RaceFxTransform m;
        FacingTransform(m, f, n);
        m.pos = pos;
        m.scale = 1.0f;
        RaceFx* fx = &self->boostFx[i];
        if (fx->effect == nullptr) {
            uint32_t type = EffectType("RaceBoost");
            Fx_Start(effectManager, &type, fx, 0);
        }
        Fx_SetTransform(fx, &m);
        ReleaseColPointEntity(col.entity);
    }
}
RACE45_REVERSED("CRace::UpdateBoostPads", 0x0070E540, &UpdateBoostPads);

// (thiscall) a moving vehicle (not wrecked) within 3 m of a boost pad: launched along the
// pad's heading at 1.25x its speed and turned that way (the bump sound at most once a
// second)
void __fastcall CheckBoostPads(CRace* self, re::Edx, uint8_t* ped) {
    if (ped == nullptr || Ped_IsSeatedInVehicle(ped) == 0) {
        return;
    }
    uint8_t* vehicle = PedVehicle(ped);
    if ((At<uint8_t>(vehicle, 0x108) & 0xF8) == 0x28) {
        return;
    }
    if (!(SpeedSqr(vehicle) > 0.0001f)) {
        return;
    }
    for (int i = 0; i < self->boostCount; i++) {
        RaceVec& pad = self->boosts[i];
        float beyond = DistanceBeyond(Placeable_GetPosition(ped), &pad, 3.0f);
        if (!(beyond < 0.0f)) {
            continue;
        }
        RaceRacer* racer = Race_GetRacer(self, ped);
        if (racer == nullptr) {
            continue;
        }
        if (timeInMs - racer->bumpTime > 1000) {
            int volume, pan;
            Race_VolumePan(EntityPos(PedVehicle(ped)), 5, &volume, &pan);
            Sounds_PlayAt(sounds, Sounds_GetId(sounds, "BUMP01"), volume, pan, 0);
        }
        float angle = (pad.z / 180.0f) * kRacePi;
        RaceVec dir;
        dir.x = trk::CrtSin(angle);
        dir.y = trk::CrtCos(angle);
        dir.z = 0.0f;
        uint8_t* veh = PedVehicle(ped);
        RaceVec* speed = MoveSpeed(veh);
        float m = Vector_Magnitude(speed) * 1.25f;
        speed->x = dir.x * m;
        speed->y = dir.y * m;
        speed->z = m * 0.0f;
        veh = PedVehicle(ped);
        float heading = HeadingFromVector(&dir);
        if (At<void*>(veh, 0x14) != nullptr) {
            Matrix_SetRotateZOnly(At<void*>(veh, 0x14), heading);
        }
        At<float>(veh, 0x10) = heading;
        racer->bumpTime = timeInMs;
    }
}
RACE45_REVERSED("CRace::CheckBoostPads", 0x0070E8D0, &CheckBoostPads);

// (thiscall) the waypoint effect (and its beam, 2.5 m up) on the ground under `pos`,
// facing `dir`; the final ones ("...Final") restart both
void __fastcall PlaceWaypoint(CRace* self, re::Edx, const RaceVec* pos, const RaceVec* dir, int final) {
    RaceVec n = {0.0f, 0.0f, 1.0f};
    RaceColPoint col;
    col.pos.x = 0.0f;
    col.pos.y = 0.0f;
    col.pos.z = 0.0f;
    col.normal.x = 0.0f;
    col.normal.y = 0.0f;
    col.normal.z = 0.0f;
    col.depth = 3.4028235e38f;
    col.entity = nullptr;
    RaceVec at = *pos;
    RaceVec from = {pos->x, pos->y, pos->z + 3.0f};
    re::Ptr32<uint8_t> hit = nullptr;
    if (ProcessVerticalLine(&from, -100.0f, &col, &hit, 1, 0) != 0) {
        n = col.normal;
        at.x = col.pos.x;
        at.y = col.pos.y;
        Vector_NormaliseSafe(&n);
        at.z = col.pos.z + 0.25f;
    }
    RaceVec f = *dir;
    Vector_NormaliseSafe(&f);
    RaceFxTransform m;
    FacingTransform(m, f, n);
    m.pos = at;
    m.scale = 1.0f;
    if (re::Flag(final)) {
        Fx_Kill(&self->beam);
        Fx_Kill(&self->waypoint);
    }
    if (self->beam.effect == nullptr) {
        uint32_t type = EffectType("RaceBeam");
        if (re::Flag(final)) {
            type = EffectType("RaceBeamFinal");
        }
        Fx_Start(effectManager, &type, &self->beam, 0);
    }
    RaceVec top = {at.x, at.y, at.z + 2.5f};
    Fx_SetPosition(&self->beam, &top, 0);
    if (self->waypoint.effect == nullptr) {
        uint32_t type = EffectType("RaceWaypoint");
        if (re::Flag(final)) {
            type = EffectType("RaceWaypointFinal");
        }
        Fx_Start(effectManager, &type, &self->waypoint, 0);
    }
    Fx_SetTransform(&self->waypoint, &m);
    ReleaseColPointEntity(col.entity);
}
RACE45_REVERSED("CRace::PlaceWaypoint", 0x0070EB10, &PlaceWaypoint);

// (thiscall) a racer assigned (the ped's reference moved over)
RaceRacer* __fastcall RacerAssign(RaceRacer* self, re::Edx, const RaceRacer* from) {
    uint8_t* ped = from->ped;
    if (ped != self->ped) {
        if (self->ped != nullptr) {
            CleanUpOldReference(self->ped, &self->ped);
        }
        self->ped = ped;
        if (ped != nullptr) {
            RegisterReference(ped, &self->ped);
        }
    }
    const uint32_t* src = reinterpret_cast<const uint32_t*>(from);
    uint32_t* dst = reinterpret_cast<uint32_t*>(self);
    for (int i = 1; i < 8; i++) {
        dst[i] = src[i];
    }
    self->boosting = from->boosting;
    for (int i = 9; i < 0x1B; i++) {
        dst[i] = src[i];
    }
    return self;
}
RACE45_REVERSED("RaceRacer::operator=", 0x0070EE70, &RacerAssign);

// (thiscall) the ped's race time in seconds: its finish time once finished, else the time
// so far
float __fastcall GetRaceTime(CRace* self, re::Edx, uint8_t* ped) {
    if (Race_HasFinished(self, ped) != 0) {
        RaceRacer* racer = Race_GetRacer(self, ped);
        if (racer != nullptr && racer->finishTime != 0xFFFFFFFFu) {
            int32_t ms = int32_t(racer->finishTime - self->startTime);
            return float(double(ms) / 1000.0);
        }
    }
    return Race_GetDuration(self);
}
RACE45_REVERSED("CRace::GetRaceTime", 0x0070EF50, &GetRaceTime);

// a path range appended (grown x2, from 1)
inline void AddRange(RaceArray<RacePathRange>& list, int32_t path, int32_t first, int32_t last) {
    if (list.count == list.capacity) {
        Array12_Reserve(&list, list.capacity != 0 ? list.capacity * 2 : 1, 0);
    }
    RacePathRange& r = list.data[list.count];
    r.path = path;
    r.first = first;
    r.last = last;
    list.count++;
}

// (thiscall) a shortcut path from the race path's node `first` back at `last`
void __fastcall AddShortcut(CRace* self, re::Edx, int32_t path, int32_t first, int32_t last) {
    AddRange(self->shortcuts, path, first, last);
}
RACE45_REVERSED("CRace::AddShortcut", 0x0070EFB0, &AddShortcut);

// (thiscall) a jump path the same way
void __fastcall AddJumpPath(CRace* self, re::Edx, int32_t path, int32_t first, int32_t last) {
    AddRange(self->jumpPaths, path, first, last);
}
RACE45_REVERSED("CRace::AddJumpPath", 0x0070F010, &AddJumpPath);

// ---- 0x70F070..0x70F7F0: progress along the path, the node lists ------------------------------

// (thiscall on the racer) a progress (node, lap, branch) one node on: off the end of a branch
// back to the race path's rejoining node, off the race path's end a lap on; with
// `checkGround` (not for the player) only when there is ground within 140 m below the next
// node (20 m above it)
void __fastcall ProgressAdvance(RaceRacer* self, re::Edx, int32_t* progress, int checkGround) {
    re::Ptr32<RacePathRange>& branch = *reinterpret_cast<re::Ptr32<RacePathRange>*>(&progress[2]);
    uint8_t* path = GetPath(branch != nullptr ? branch->path : self->race->path);
    if (re::Flag(checkGround) && At<int32_t>(self->ped, 0x1310) != 0xD) {
        RaceColPoint col;
        col.pos.x = 0.0f;
        col.pos.y = 0.0f;
        col.pos.z = 0.0f;
        col.entity = nullptr;
        col.normal.x = 0.0f;
        col.normal.y = 0.0f;
        col.normal.z = 0.0f;
        col.depth = 3.4028235e38f;
        RaceVec next;
        Racer_GetNextNodePos(self, &next);
        next.z = next.z + 20.0f;
        float bottom = next.z - 140.0f;
        re::Ptr32<uint8_t> hit = nullptr;
        uint8_t found = ProcessVerticalLine(&next, bottom, &col, &hit, 1, 0);
        ReleaseColPointEntity(col.entity);
        if (found == 0) {
            return;
        }
    }
    progress[0]++;
    if (uint32_t(progress[0]) >= Path_GetNumNodes(path)) {
        if (branch != nullptr) {
            progress[0] = branch->last;
            branch = nullptr;
            return;
        }
        progress[1]++;
        progress[0] = 0;
    }
}
RACE45_REVERSED("RaceRacer::Advance", 0x0070F070, &ProgressAdvance);

// (thiscall) the player's racer moved on to the next highlighted node of the race path
void __fastcall SkipToHighlight(RaceRacer* self, re::Edx) {
    if (At<int32_t>(self->ped, 0x1310) != 0xD) {
        return;
    }
    int32_t* progress = &self->node;
    for (;;) {
        if (self->shortcut == nullptr && IntArrayFind(self->race->highlight, self->node) != -1) {
            return;
        }
        Racer_Advance(self, progress, 1);
    }
}
RACE45_REVERSED("RaceRacer::SkipToHighlight", 0x0070F1E0, &SkipToHighlight);

// (thiscall) at a jump node, in front of the camera and heading along the path (both the
// direction to the next node and the ped's forward within 0.93): the jump button pressed
void __fastcall CheckJump(RaceRacer* self, re::Edx) {
    int32_t* progress = &self->node;
    if (Race_IsJumpNode(self->race, ProgressPathId(self->shortcut), self->node) == 0) {
        return;
    }
    float* m = GetLocalTransform(GetCameraNode(camera));
    RaceVec look = {m[2], m[5], m[8]};
    float* cam = GetLocalTransform(GetCameraNode(camera)) + 9;
    RaceVec tmp;
    RaceVec* d = VectorSub(&tmp, EntityPos(self->ped), cam);
    float s = d->y * look.y + d->x * look.x;
    float front = s + d->z * look.z;
    if (!(front > 0.0f)) {
        return;
    }
    RaceVec next, here;
    Racer_GetNextNodePos(self, &next);
    RaceProgress_GetPos(progress, &here);
    RaceVec along, to;
    VectorSub(&along, &next, &here);
    VectorSub(&to, &next, EntityPos(self->ped));
    Vector_Normalise(&along);
    Vector_Normalise(&to);
    float dot = DotYXZ(along, to);
    if (!(dot > 0.93f)) {
        return;
    }
    uint8_t* ped = self->ped;
    RaceVec forward;
    RaceVec* f = Placeable_GetForward(ped, &forward);
    float t = f->y * along.y + f->x * along.x;
    float facing = t + f->z * along.z;
    if (!(facing > 0.93f)) {
        return;
    }
    self->jumpTime = timeInMs;
    ActionInputs_SetRequest(At<void*>(ped, 0x2E0), 4, 2, 0.0f);
}
RACE45_REVERSED("RaceRacer::CheckJump", 0x0070F240, &CheckJump);

// an int appended to a sorted unique list, or inserted in order (the caller's `inserted`
// tells which); nothing when it is there
enum class SortedAdd { Present, Inserted, Appended };
inline SortedAdd SortedAddInline(RaceArray<int32_t>& list, int32_t* value, bool growInline) {
    if (IntArrayFind(list, *value) != -1) {
        return SortedAdd::Present;
    }
    for (uint32_t i = 0; i < list.count; i++) {
        if (*value < list.data[i]) {
            IntArray_Insert(&list, i, value);
            return SortedAdd::Inserted;
        }
    }
    if (list.count == list.capacity) {
        uint32_t capacity = list.capacity != 0 ? list.capacity * 2 : 1;
        if (growInline) {
            IntArrayGrowInline(list, capacity);
        } else {
            Array4_Reserve(&list, capacity, 0);
        }
    }
    list.data[list.count] = *value;
    list.count++;
    return SortedAdd::Appended;
}

// (thiscall) highlight: -1 every node (the waypoint hidden), -2 none shown (the waypoint
// hidden), else the nodes tagged `tag` (stopping after one inserted mid-list)
void __fastcall AddHighlight(CRace* self, re::Edx, int tag) {
    uint8_t* path = GetPath(self->path);
    int count = int(Path_GetNumNodes(path));
    if (!(tag < count)) {
        return;
    }
    if (tag == -1) {
        self->showWaypoint = 0;
        if (self->beam.effect != nullptr) {
            Fx_Kill(&self->beam);
        }
        if (self->waypoint.effect != nullptr) {
            Fx_Kill(&self->waypoint);
        }
        for (int32_t node = 0; node < count; node++) {
            int32_t value = node;
            SortedAddInline(self->highlight, &value, true);
        }
    } else if (tag == -2) {
        self->showWaypoint = 0;
        if (self->beam.effect != nullptr) {
            Fx_Kill(&self->beam);
        }
        if (self->waypoint.effect != nullptr) {
            Fx_Kill(&self->waypoint);
        }
    } else {
        for (int32_t node = 0; node < count; node++) {
            if (At<int32_t>(Path_GetNode(path, node), 0x18) != tag) {
                continue;
            }
            int32_t value = node;
            if (SortedAddInline(self->highlight, &value, false) == SortedAdd::Inserted) {
                return;
            }
        }
    }
}
RACE45_REVERSED("CRace::AddHighlight", 0x0070F400, &AddHighlight);

// (thiscall) the nodes tagged `tag` made bottlenecks (no lane offsets there; stopping after
// one inserted mid-list)
void __fastcall AddBottleneck(CRace* self, re::Edx, int tag) {
    uint8_t* path = GetPath(self->path);
    int count = int(Path_GetNumNodes(path));
    if (count <= tag || count < 1) {
        return;
    }
    for (int32_t node = 0; node < count; node++) {
        if (At<int32_t>(Path_GetNode(path, node), 0x18) != tag) {
            continue;
        }
        int32_t value = node;
        if (SortedAddInline(self->bottlenecks, &value, true) == SortedAdd::Inserted) {
            return;
        }
    }
}
RACE45_REVERSED("CRace::AddBottleneck", 0x0070F670, &AddBottleneck);

// (thiscall) a jump node (a path's node), once
void __fastcall AddJumpNode(CRace* self, re::Edx, int path, int node) {
    RaceArray<RaceJumpNode>& list = self->jumpNodes;
    for (uint32_t i = 0; i < list.count; i++) {
        if (list.data[i].path == path && list.data[i].node == node) {
            return;
        }
    }
    if (list.count == list.capacity) {
        Array8_Reserve(&list, list.capacity != 0 ? list.capacity * 2 : 1, 0);
    }
    list.data[list.count].path = path;
    list.data[list.count].node = node;
    list.count++;
}
RACE45_REVERSED("CRace::AddJumpNode", 0x0070F7F0, &AddJumpNode);

// ---- 0x70F860..0x70FD70: the transition screens, the arrays ------------------------------------

// CTxdStore::ms_pTxdPool's header (a CPool: the slots, their flags, +0xC the slot size)
struct RaceTxdPool {
    re::Ptr32<uint8_t> objects;
    re::Ptr32<uint8_t> flags;
    int32_t size;
    int32_t slotSize;
};
GAME_VAR(RaceTxdPool*, txdPoolRace, 0x01B64654);

// (thiscall) the transition sprite (+0x504) set to `texture` of "TXD\ARCADE\<txd>.nft",
// loaded into the "racescr" slot unless that dictionary already has the texture
re::Ptr32<void>* __fastcall LoadTransition(CRace* self, re::Edx, const char* txd, const char* texture) {
    PushMemoryTag(8);
    if (self->transitionTxd == -1) {
        self->transitionTxd = Txd_AddTxdSlot("racescr");
    }
    RaceTxdPool* pool = txdPoolRace;
    re::Ptr32<void>* entry = nullptr;   // (a free slot: the game reads through nullptr)
    if ((pool->flags[self->transitionTxd] & 0x80) == 0) {
        entry = reinterpret_cast<re::Ptr32<void>*>(pool->objects + pool->slotSize * self->transitionTxd);
    }
    void* dictionary = *entry;
    if (dictionary == nullptr || Txd_FindTexture(dictionary, texture) == nullptr) {
        char filename[128];
        Sprintf(filename, "TXD\\ARCADE\\%s.nft", txd);
        Sprite_Delete(&self->transitionSprite);
        if (dictionary != nullptr) {
            Txd_RemoveTxd(self->transitionTxd);
        }
        Txd_LoadTxd(self->transitionTxd, filename);
        Txd_AddRef(self->transitionTxd);
        Txd_PushCurrentTxd();
        Txd_SetCurrentTxd(self->transitionTxd);
        Sprite_SetTexture(&self->transitionSprite, texture);
        Txd_PopCurrentTxd();
    }
    PopMemoryTag();
    return &self->transitionSprite;
}
RACE45_REVERSED("CRace::LoadTransition", 0x0070F860, &LoadTransition);

// (thiscall) a transition screen shown (0 end, 1 intro, 2..4 loading, 5 main, 6 game over,
// 7 controls, 8 continue; the "b" set for the second track); returns 0
int __fastcall DisplayTransition(CRace* self, re::Edx, int screen, int second) {
    static const char* const kTxd[9] = {"Arcade1", "Arcade2", "Arcade3", "Arcade4", "Arcade5",
                                        "Arcade6", "Arcade7", "Arcade8", "Arcade9"};
    static const char* const kTxdB[9] = {"Arcade1b", "Arcade2b", "Arcade3b", "Arcade4b", "Arcade5b",
                                         "Arcade6b", "Arcade7b", "Arcade8b", "Arcade9b"};
    static const char* const kTexture[9] = {"Arcade_end",   "Arcade_intro", "Arcade_load01",
                                            "Arcade_load02", "Arcade_load03", "Arcade_main",
                                            "Arcade_gameover", "Arcade_controls", "Arcade_contine"};
    transitionDisplayed = 1;
    self->transition = screen;
    if (uint32_t(screen) <= 8) {
        Race_LoadTransition(self, re::Flag(second) ? kTxdB[screen] : kTxd[screen], kTexture[screen]);
    }
    return 0;
}
RACE45_REVERSED("CRace::DisplayTransition", 0x0070F960, &DisplayTransition);

// (thiscall) the racers' array freed (new[]'s count before it; each ped's reference released)
void __fastcall RacerArrayDelete(RaceArray<RaceRacer>* self, re::Edx) {
    RaceRacer* data = self->data;
    if (data == nullptr) {
        return;
    }
    uint32_t* block = reinterpret_cast<uint32_t*>(data) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        if (data[i].ped != nullptr) {
            CleanUpOldReference(data[i].ped, &data[i].ped);
        }
    }
    Free(block);
    self->data = nullptr;
}
RACE45_REVERSED("RaceRacerArray::Delete", 0x0070FB50, &RacerArrayDelete);

// (thiscall) the racer at `index` removed (the ones after moved down; the last slot keeps
// its old copy)
void __fastcall RacerArrayRemoveAt(RaceArray<RaceRacer>* self, re::Edx, uint32_t index) {
    for (; index < self->count - 1; index++) {
        Racer_Assign(&self->data[index], &self->data[index + 1]);
    }
    self->count--;
}
RACE45_REVERSED("RaceRacerArray::RemoveAt", 0x0070FBA0, &RacerArrayRemoveAt);

// (thiscall) the racers freed and the array emptied
void __fastcall RacerArrayClear(RaceArray<RaceRacer>* self, re::Edx) {
    RaceRacer* data = self->data;
    if (data != nullptr) {
        uint32_t* block = reinterpret_cast<uint32_t*>(data) - 1;
        for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
            if (data[i].ped != nullptr) {
                CleanUpOldReference(data[i].ped, &data[i].ped);
            }
        }
        Free(block);
        self->data = nullptr;
    }
    self->capacity = 0;
    self->count = 0;
}
RACE45_REVERSED("RaceRacerArray::Clear", 0x0070FBE0, &RacerArrayClear);

// (thiscall) the node records freed (each ped's reference released)
void __fastcall RecordArrayDelete(RaceArray<RaceRecord>* self, re::Edx) {
    RaceRecord* data = self->data;
    if (data == nullptr) {
        return;
    }
    uint32_t* block = reinterpret_cast<uint32_t*>(data) - 1;
    for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
        if (data[i].ped != nullptr) {
            CleanUpOldReference(data[i].ped, &data[i].ped);
        }
    }
    Free(block);
    self->data = nullptr;
}
RACE45_REVERSED("RaceRecordArray::Delete", 0x0070FC40, &RecordArrayDelete);

// a new[] block of `count` elements (the count before them), each constructed
template <typename T>
T* NewArray(uint32_t count, void* ctor) {
    uint32_t bytes = ArrayBytes(count, sizeof(T));
    uint32_t* block = static_cast<uint32_t*>(Malloc(bytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : bytes + 4));
    if (block == nullptr) {
        return nullptr;
    }
    *block = count;
    T* data = reinterpret_cast<T*>(block + 1);
    VectorConstruct(data, sizeof(T), count, ctor);
    return data;
}

// (thiscall) the racers' array reallocated to `capacity` (copied over by assignment; the
// count set to it with `setCount`)
void __fastcall RacerArrayReserve(RaceArray<RaceRacer>* self, re::Edx, uint32_t capacity, int setCount) {
    RaceRacer* data = nullptr;
    self->capacity = capacity;
    if (capacity != 0) {
        data = NewArray<RaceRacer>(capacity, reinterpret_cast<void*>(0x0070B900));
    }
    if (self->data != nullptr) {
        for (uint32_t i = 0; i < self->count; i++) {
            Racer_Assign(&data[i], &self->data[i]);
        }
        RaceRacer* old = self->data;
        if (old != nullptr) {
            uint32_t* block = reinterpret_cast<uint32_t*>(old) - 1;
            for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
                if (old[i].ped != nullptr) {
                    CleanUpOldReference(old[i].ped, &old[i].ped);
                }
            }
            Free(block);
        }
    }
    self->data = data;
    if (re::Flag(setCount)) {
        self->count = self->capacity;
    }
}
RACE45_REVERSED("RaceRacerArray::Reserve", 0x0070FC90, &RacerArrayReserve);

// (thiscall) the node records reallocated the same way
void __fastcall RecordArrayReserve(RaceArray<RaceRecord>* self, re::Edx, uint32_t capacity, int setCount) {
    RaceRecord* data = nullptr;
    self->capacity = capacity;
    if (capacity != 0) {
        data = NewArray<RaceRecord>(capacity, reinterpret_cast<void*>(0x00667DA0));
    }
    if (self->data != nullptr) {
        for (uint32_t i = 0; i < self->count; i++) {
            RaceRecord& from = self->data[i];
            RaceRecord& to = data[i];
            uint8_t* ped = from.ped;
            if (ped != to.ped) {
                if (to.ped != nullptr) {
                    CleanUpOldReference(to.ped, &to.ped);
                }
                to.ped = ped;
                if (ped != nullptr) {
                    RegisterReference(ped, &to.ped);
                }
            }
            to.time = from.time;
            to.lap = from.lap;
        }
        RaceRecord* old = self->data;
        if (old != nullptr) {
            uint32_t* block = reinterpret_cast<uint32_t*>(old) - 1;
            for (int32_t i = int32_t(*block) - 1; i >= 0; i--) {
                if (old[i].ped != nullptr) {
                    CleanUpOldReference(old[i].ped, &old[i].ped);
                }
            }
            Free(block);
        }
    }
    self->data = data;
    if (re::Flag(setCount)) {
        self->count = self->capacity;
    }
}
RACE45_REVERSED("RaceRecordArray::Reserve", 0x0070FD70, &RecordArrayReserve);

// ---- 0x70FEA0..0x710960: the destructor, karts, the racers' progress ---------------------------

// (thiscall) the race destroyed: the transition released, its effects killed, the racers'
// peds let go (+0x1360 cleared), the lists freed, then the Minigame base
void __fastcall Destruct(CRace* self, re::Edx) {
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00949A50);
    Sprite_Delete(&self->transitionSprite);
    if (self->transitionTxd != -1) {
        Txd_RemoveTxdSlot(self->transitionTxd);
    }
    self->transitionTxd = -1;
    transitionDisplayed = 0;
    self->transition = -1;
    FontStyle_Unload(4);
    if (self->beam.effect != nullptr) {
        Fx_Kill(&self->beam);
    }
    if (self->waypoint.effect != nullptr) {
        Fx_Kill(&self->waypoint);
    }
    for (int i = 0; i < self->barrierCount; i++) {
        for (int k = 0; k < 4; k++) {
            if (self->barrierFx[i][k].effect != nullptr) {
                Fx_Kill(&self->barrierFx[i][k]);
            }
        }
    }
    for (int i = 0; i < self->boostCount; i++) {
        if (self->boostFx[i].effect != nullptr) {
            Fx_Kill(&self->boostFx[i]);
        }
    }
    for (uint32_t i = 0; i < self->racers.count; i++) {
        RaceRacer& r = self->racers.data[i];
        if (r.ped != nullptr) {
            At<int32_t>(r.ped, 0x1360) = 0;
            if (r.ped != nullptr) {
                CleanUpOldReference(r.ped, &r.ped);
            }
        }
    }
    for (uint32_t i = 0; i < self->racers.capacity; i++) {
        RaceRacer& r = self->racers.data[i];
        if (r.ped != nullptr) {
            CleanUpOldReference(r.ped, &r.ped);
            r.ped = nullptr;
        }
    }
    RacerArray_Clear(&self->racers);
    auto freeList = [](auto& list) {
        if (list.data != nullptr) {
            Free(list.data);
            list.data = nullptr;
        }
    };
    freeList(self->shortcuts);
    self->shortcuts.capacity = 0;
    self->shortcuts.count = 0;
    freeList(self->jumpPaths);
    self->jumpPaths.capacity = 0;
    self->jumpPaths.count = 0;
    freeList(self->highlight);
    self->highlight.capacity = 0;
    self->highlight.count = 0;
    freeList(self->bottlenecks);
    self->bottlenecks.capacity = 0;
    self->bottlenecks.count = 0;
    // the members' destructors
    Sprite_Delete(&self->transitionSprite);
    for (int i = 15; i >= 0; i--) {
        Fx_Destruct(&self->boostFx[i]);
    }
    for (int i = 39; i >= 0; i--) {
        Fx_Destruct(&self->barrierFx[0][0] + i);
    }
    for (int i = 9; i >= 0; i--) {
        Sound_Set(&self->barrierSounds[i], 0);
    }
    Fx_Destruct(&self->beam);
    Fx_Destruct(&self->waypoint);
    freeList(self->bottlenecks);
    RecordArray_Delete(&self->records);
    freeList(self->jumpNodes);
    freeList(self->highlight);
    freeList(self->jumpPaths);
    freeList(self->shortcuts);
    RacerArray_Delete(&self->racers);
    Minigame_Destruct(self);
}
RACE45_REVERSED("CRace::~CRace", 0x0070FEA0, &Destruct);

// (thiscall) the minigame's +4 set (its vtable's +0x10; also a generic setter other
// classes' tables share)
void __fastcall SetActive(CRace* self, re::Edx) {
    self->active = 1;
}
RACE45_REVERSED("CRace::SetActive", 0x007101E0, &SetActive);

// (thiscall) a go-kart racer's wreck (class 10, wrecked): an explosion every 16th frame;
// after 2.5 s the kart reset (the script's racers turned towards their node), before that
// (the script's racers) floated above the ground by their place
void __fastcall CheckKart(CRace* self, re::Edx, uint8_t* ped) {
    if (ped == nullptr || PedVehicle(ped) == nullptr || Ped_IsSeatedInVehicle(ped) == 0) {
        return;
    }
    int32_t kind;
    if (*Vehicle_GetClass(PedVehicle(ped), &kind) != 10) {
        return;
    }
    if ((At<uint8_t>(PedVehicle(ped), 0x108) & 0xF8) != 0x28) {
        return;
    }
    if (Ped_EveryNthFrame(ped, 0x10) != 0) {
        uint32_t type = EffectType("CarDestroyed");
        RaceFx fx;
        Fx_Construct(&fx);
        Fx_Start(effectManager, &type, &fx, 0);
        RaceFxTransform m;
        m.scale = 1.0f;
        RaceVec up;
        m.m[1] = 0.0f;
        m.m[7] = 0.0f;
        m.m[2] = 0.0f;
        m.m[5] = 0.0f;
        m.m[3] = 0.0f;
        m.m[6] = 0.0f;
        up.x = 0.0f;
        up.y = 0.0f;
        m.m[4] = 1.0f;
        m.m[8] = 1.0f;
        m.m[0] = 1.0f;
        up.z = 1.0f;
        RaceVec tmp;
        m.pos = *VectorAdd(&tmp, EntityPos(PedVehicle(ped)), &up);
        Fx_SetTransform(&fx, &m);
        int volume, pan;
        Race_VolumePan(EntityPos(PedVehicle(ped)), 5, &volume, &pan);
        Sounds_PlayAt(sounds, Sounds_GetId(sounds, "Explosion"), volume, pan, 0);
        Fx_Destruct(&fx);
    }
    uint8_t* kart = PedVehicle(ped);
    if (At<uint32_t>(kart, 0x348) + 0x9C4 < timeInMs) {
        Kart_UpdateStatus(kart);
        if (At<int32_t>(ped, 0x1310) == 0xD) {
            return;
        }
        RaceRacer* racer = Race_GetRacer(self, ped);
        RaceVec tmp, d;
        VectorSub(&d, RaceProgress_GetPos(&racer->node, &tmp), EntityPos(PedVehicle(ped)));
        float angle = AngleBetweenPoints(0.0f, 0.0f, d.x, d.y);
        angle = angle - kRacePi;
        float heading = LimitAngle(angle);
        uint8_t* vehicle = PedVehicle(ped);
        if (At<void*>(vehicle, 0x14) != nullptr) {
            Matrix_SetRotateZOnly(At<void*>(vehicle, 0x14), heading);
        }
        At<float>(vehicle, 0x10) = heading;
        Physical_SetTurnSpeedXYZ(PedVehicle(ped), 0.0f, 0.0f, 0.0f);
        return;
    }
    if (At<int32_t>(ped, 0x1310) == 0xD) {
        return;
    }
    Physical_SetTurnSpeedXYZ(kart, 0.0f, 0.0f, 0.2f);
    RaceColPoint col;
    ColPoint_Construct(&col);
    float* p = EntityPos(PedVehicle(ped));
    float x = p[0];
    float y = p[1];
    RaceVec from = {x, y, p[2] + 5.0f};
    re::Ptr32<uint8_t> hit = nullptr;
    if (ProcessVerticalLine(&from, -100.0f, &col, &hit, 1, 0) != 0) {
        int32_t place = Race_GetPosition(self, ped);
        float lift = float(place) * 0.1f;
        float z = col.pos.z + 9.0f;
        RaceVec to = {x, y, z + lift};
        Physical_SetPosition(PedVehicle(ped), &to);
    }
    ColPoint_Destruct(&col);
}
RACE45_REVERSED("CRace::CheckKart", 0x007101F0, &CheckKart);

// (thiscall) the ped's racer dropped (its +0x1360 cleared; no check that it races)
void __fastcall RemoveRacer(CRace* self, re::Edx, uint8_t* ped) {
    int32_t i = FindRacer(self, ped);
    RaceRacer* r = self->racers.data + i;
    if (r->ped == nullptr) {
        return;
    }
    At<int32_t>(r->ped, 0x1360) = 0;
    r = self->racers.data + i;
    if (r->ped != nullptr) {
        CleanUpOldReference(r->ped, &r->ped);
    }
    r = self->racers.data + i;
    if (r->ped != nullptr) {
        CleanUpOldReference(r->ped, &r->ped);
        r->ped = nullptr;
    }
    RacerArray_RemoveAt(&self->racers, uint32_t(i));
}
RACE45_REVERSED("CRace::RemoveRacer", 0x00710560, &RemoveRacer);

// (thiscall) the ped finished: moved up to the first unfinished place, its finish time set
void __fastcall MarkFinished(CRace* self, re::Edx, uint8_t* ped) {
    int32_t i = FindRacer(self, ped);
    if (i != self->finishedCount) {
        RaceRacer tmp;
        Racer_CopyConstruct(&tmp, self->racers.data + i);
        for (; i > self->finishedCount; i--) {
            Racer_Assign(self->racers.data + i, self->racers.data + i - 1);
        }
        Racer_Assign(self->racers.data + self->finishedCount, &tmp);
        if (tmp.ped != nullptr) {
            CleanUpOldReference(tmp.ped, &tmp.ped);
        }
    }
    self->racers.data[self->finishedCount].finishTime = timeInMs;
    self->finishedCount++;
}
RACE45_REVERSED("CRace::MarkFinished", 0x00710610, &MarkFinished);

// (thiscall) the position `ahead` nodes past the ped's (with `highlighted`, only counting
// highlighted race-path nodes; branch nodes never); false for no racer, a finished one or
// past the last lap
uint8_t __fastcall GetNodeAhead(CRace* self, re::Edx, RaceVec* out, uint8_t* ped, int ahead, int highlighted) {
    RaceRacer* racer = Race_GetRacer(self, ped);
    if (racer == nullptr || Race_HasFinished(self, ped) != 0) {
        return 0;
    }
    int32_t p[3] = {racer->node, racer->lap, reinterpret_cast<int32_t&>(racer->shortcut)};
    for (; ahead > 0; ahead--) {
        for (;;) {
            Racer_Advance(racer, p, 1);
            if (p[2] != 0) {
                continue;
            }
            if (!re::Flag(highlighted)) {
                break;
            }
            if (IntArrayFind(self->highlight, p[0]) != -1) {
                break;
            }
        }
        if (p[1] >= self->laps) {
            return 0;
        }
    }
    RaceVec tmp;
    *out = *RaceProgress_GetPos(p, &tmp);
    return 1;
}
RACE45_REVERSED("CRace::GetNodeAhead", 0x00710710, &GetNodeAhead);

// (thiscall on the racer) a progress moved on to a highlighted node of the race path (none
// when it is on one)
void __fastcall NextHighlight(RaceRacer* self, re::Edx, int32_t* progress) {
    for (;;) {
        re::Ptr32<RacePathRange>& branch = *reinterpret_cast<re::Ptr32<RacePathRange>*>(&progress[2]);
        int32_t pathId;
        if (branch == nullptr) {
            CRace* race = self->race;
            if (IntArrayFind(race->highlight, progress[0]) != -1) {
                return;
            }
            pathId = race->path;
        } else {
            pathId = branch->path;
        }
        uint8_t* path = GetPath(pathId);
        progress[0]++;
        if (uint32_t(progress[0]) >= Path_GetNumNodes(path)) {
            if (branch == nullptr) {
                progress[1]++;
                progress[0] = 0;
            } else {
                progress[0] = branch->last;
                branch = nullptr;
            }
        }
    }
}
RACE45_REVERSED("RaceRacer::NextHighlight", 0x00710810, &NextHighlight);

// (thiscall) the racer's next node: the script's racers check for a jump and maybe take a
// branch first; the player's goes on to the next highlighted race-path node
void __fastcall NextNode(RaceRacer* self, re::Edx, int tryBranch, int checkGround) {
    if (At<int32_t>(self->ped, 0x1310) != 0xD) {
        Racer_CheckJump(self);
        if (re::Flag(tryBranch) && Racer_TryBranch(self) != 0) {
            return;
        }
        Racer_Advance(self, &self->node, checkGround);
        return;
    }
    for (;;) {
        Racer_Advance(self, &self->node, checkGround);
        if (self->shortcut != nullptr) {
            continue;
        }
        if (IntArrayFind(self->race->highlight, self->node) != -1) {
            return;
        }
    }
}
RACE45_REVERSED("RaceRacer::NextNode", 0x007108B0, &NextNode);

// (thiscall, vtable +0) the deleting destructor
CRace* __fastcall DeletingDestruct(CRace* self, re::Edx, int flags) {
    Race_Destruct(self);
    if ((flags & 1) != 0) {
        Free(self);
    }
    return self;
}
RACE45_REVERSED("CRace::DeletingDestruct", 0x00710940, &DeletingDestruct);

GAME_VAR(void*, raceRadar, 0x00C674CC);

// (thiscall) the waypoint at the ped's next highlighted node (the final one when it is the
// last), with a radar blip; with finishOnly, only the finish line (at the last node, facing
// from the first) once no node is ahead
void __fastcall ShowNextWaypoint(CRace* self, re::Edx, uint8_t* ped) {
    RaceVec ahead;
    if (self->finishOnly != 0 && Race_GetNodeAhead(self, &ahead, ped, 1, 0) == 0) {
        uint8_t* path = GetPath(currentRace->path);
        RaceVec last, first;
        CopyVec(last, Path_GetNode(path, int32_t(Path_GetNumNodes(path) - 1)));
        CopyVec(first, Path_GetNode(path, 0));
        RaceVec dir;
        VectorSub(&dir, &last, &first);
        Race_PlaceWaypoint(self, &last, &dir, 1);
        return;
    }
    if (self->showWaypoint == 0) {
        if (self->beam.effect != nullptr) {
            Fx_Kill(&self->beam);
        }
        if (self->waypoint.effect != nullptr) {
            Fx_Kill(&self->waypoint);
        }
        return;
    }
    void* radar = raceRadar;
    if (self->blip != -1) {
        Radar_RemoveBlip(radar, self->blip);
    }
    RaceVec at = {0.0f, 0.0f, 0.0f};
    if (Race_GetNodeAhead(self, &at, ped, 0, 1) == 0) {
        self->blip = -1;
        return;
    }
    RaceVec next = {0.0f, 0.0f, 0.0f};
    uint8_t last = Race_GetNodeAhead(self, &next, ped, 1, 0) == 0 ? 1 : 0;
    RaceVec dir;
    VectorSub(&dir, &at, &next);
    Race_PlaceWaypoint(self, &at, &dir, last);
    int32_t blip = Radar_AddBlip(radar, 4, at.x, at.y, at.z, 0xFF007F00u, 1, 0, 0);
    self->blip = blip;
    Radar_SetBlipSize(radar, blip, 5);
}
RACE45_REVERSED("CRace::ShowNextWaypoint", 0x00710960, &ShowNextWaypoint);

// (thiscall) a ped added as a racer (its +0x1360 and +0xE4 set; its reference registered
// twice, the copy's once released: the game's)
void __fastcall AddRacer(CRace* self, re::Edx, uint8_t* ped) {
    At<int32_t>(ped, 0x1360) = 1;
    At<int32_t>(ped, 0xE4) = 1;
    RaceRacer tmp;
    Racer_Construct(&tmp);
    if (ped != tmp.ped) {
        if (tmp.ped != nullptr) {
            CleanUpOldReference(tmp.ped, &tmp.ped);
        }
        tmp.ped = ped;
        RegisterReference(ped, &tmp.ped);
    }
    RegisterReference(tmp.ped, &tmp.ped);
    tmp.race = self;
    RaceArray<RaceRacer>& list = self->racers;
    if (list.count == list.capacity) {
        RacerArray_Reserve(&list, list.capacity != 0 ? list.capacity * 2 : 1, 0);
    }
    Racer_Assign(&list.data[list.count], &tmp);
    list.count++;
    if (tmp.ped != nullptr) {
        CleanUpOldReference(tmp.ped, &tmp.ped);
    }
}
RACE45_REVERSED("CRace::AddRacer", 0x00710B60, &AddRacer);

// a progress's 2D distance to its node's position from an entity
inline float FlatDistance(const int32_t* progress, uint8_t* entity) {
    RaceVec tmp, d;
    RaceVec* v = VectorSub(&d, RaceProgress_GetPos(progress, &tmp), EntityPos(entity));
    float sum = v->x * v->x + v->y * v->y;
    return crt::Sqrtf(std::fabs(sum));
}

// (thiscall) racer a is ahead of b: by lap, then node (each at its next highlighted node),
// then the one closer (2D) to that node; a without a ped never, b without one always
uint8_t __fastcall IsAhead(CRace*, re::Edx, const RaceRacer* a, const RaceRacer* b) {
    int32_t pa[3] = {a->node, a->lap, reinterpret_cast<const int32_t&>(a->shortcut)};
    int32_t pb[3] = {b->node, b->lap, reinterpret_cast<const int32_t&>(b->shortcut)};
    Racer_NextHighlight(const_cast<RaceRacer*>(a), pa);
    Racer_NextHighlight(const_cast<RaceRacer*>(b), pb);
    if (pa[1] != pb[1]) {
        return pa[1] > pb[1] ? 1 : 0;
    }
    if (pa[0] != pb[0]) {
        return pa[0] > pb[0] ? 1 : 0;
    }
    RaceVec target;
    RaceProgress_GetPos(pa, &target);
    if (a->ped == nullptr) {
        return 0;
    }
    if (b->ped == nullptr) {
        return 1;
    }
    RaceVec d;
    float da = Vector_Magnitude2D(VectorSub(&d, &target, EntityPos(a->ped)));
    float db = Vector_Magnitude2D(VectorSub(&d, &target, EntityPos(b->ped)));
    return db > da ? 1 : 0;
}
RACE45_REVERSED("CRace::IsAhead", 0x00710C50, &IsAhead);

// (thiscall) how far racer a is ahead of b along the path (2D, node to node; negative
// behind); over 200 m apart: the straight distance negated when a is ahead, else its square
float __fastcall DistanceAhead(CRace* self, re::Edx, const RaceRacer* a, const RaceRacer* b) {
    RaceVec d;
    RaceVec* v = VectorSub(&d, EntityPos(a->ped), EntityPos(b->ped));
    float xy = v->x * v->x + v->y * v->y;
    float d2 = xy + v->z * v->z;
    if (40000.0f < d2) {
        if (Race_IsAhead(self, a, b) != 0) {
            float s = crt::Sqrtf(std::fabs(d2));
            d2 = s * -1.0f;
        }
        return d2;
    }
    int32_t pa[3] = {a->node, a->lap, reinterpret_cast<const int32_t&>(a->shortcut)};
    int32_t pb[3] = {b->node, b->lap, reinterpret_cast<const int32_t&>(b->shortcut)};
    Racer_NextHighlight(const_cast<RaceRacer*>(a), pa);
    Racer_NextHighlight(const_cast<RaceRacer*>(b), pb);
    if (Race_ProgressLess(pb, pa) != 0) {
        return Race_DistanceAhead(self, b, a) * -1.0f;
    }
    float bDistance = FlatDistance(pb, b->ped);
    float distance = FlatDistance(pa, a->ped);
    while (pa[0] != pb[0] || pa[1] != pb[1] || pa[2] != pb[2]) {
        re::Ptr32<RacePathRange>& branch = *reinterpret_cast<re::Ptr32<RacePathRange>*>(&pa[2]);
        RaceVec from;
        CopyVec(from, Path_GetNode(GetPath(ProgressPathId(branch)), pa[0]));
        uint8_t* path = GetPath(branch != nullptr ? branch->path : a->race->path);
        int32_t node = pa[0] + 1;
        pa[0] = node;
        if (uint32_t(node) >= Path_GetNumNodes(path)) {
            if (branch != nullptr) {
                pa[0] = branch->last;
                branch = nullptr;
            } else {
                pa[1]++;
                pa[0] = 0;
            }
        }
        Racer_NextHighlight(const_cast<RaceRacer*>(a), pa);
        RaceVec to;
        CopyVec(to, Path_GetNode(GetPath(ProgressPathId(*reinterpret_cast<re::Ptr32<RacePathRange>*>(&pa[2]))), pa[0]));
        RaceVec seg;
        VectorSub(&seg, &to, &from);
        float sum = seg.x * seg.x + seg.y * seg.y;
        float length = crt::Sqrtf(std::fabs(sum));
        distance = length + distance;
    }
    return distance - bDistance;
}
RACE45_REVERSED("CRace::DistanceAhead", 0x00710D80, &DistanceAhead);

// ---- 0x7110C0..0x712CA0: set-up, the racers each frame -------------------------------------------

// (thiscall) the race set up on a patrol path: `laps` laps, `opponents` script racers and
// the player (added as one); the arcade (go-kart) font loaded; every list sized to the path
// (the last node highlighted: the finish)
void __fastcall SetupRace(CRace* self, re::Edx, int32_t path, int32_t laps, int32_t opponents, int arcade) {
    uint8_t isArcade = static_cast<uint8_t>(arcade);
    if (isArcade != 0) {
        FontStyle_Load(4);
    }
    self->laps = laps;
    self->path = path;
    self->arcade = isArcade;
    self->blip = -1;
    uint32_t count = uint32_t(opponents + 1);
    self->racerCount = int32_t(count);
    if (count > self->racers.capacity) {
        RacerArray_Reserve(&self->racers, count, 0);
    }
    self->sortTime = timeInMs;
    uint32_t nodes = Path_GetNumNodes(GetPath(path));
    if (nodes > self->highlight.capacity) {
        IntArrayGrowInline(self->highlight, nodes);
    }
    if (self->highlight.count == self->highlight.capacity) {
        IntArrayGrowInline(self->highlight, self->highlight.capacity != 0 ? self->highlight.capacity * 2 : 1);
    }
    self->highlight.data[self->highlight.count] = int32_t(nodes - 1);
    self->highlight.count++;
    if (nodes > self->bottlenecks.capacity) {
        IntArrayGrowInline(self->bottlenecks, nodes);
    }
    if (nodes > self->records.capacity) {
        RecordArray_Reserve(&self->records, nodes, 1);
    }
    for (uint32_t i = 0; i < self->records.count; i++) {
        RaceRecord& r = self->records.data[i];
        if (r.ped != nullptr) {
            CleanUpOldReference(r.ped, &r.ped);
            r.ped = nullptr;
        }
        r.time = 0;
        r.lap = 0;
    }
    Race_AddRacer(self, playerPed);
}
RACE45_REVERSED("CRace::SetupRace", 0x007110C0, &SetupRace);

// (thiscall) the race constructed (on the Minigame base: kinds 4 / 0xE)
CRace* __fastcall Construct(CRace* self, re::Edx) {
    Minigame_Construct(self);
    self->vtable = reinterpret_cast<re::Ptr32<void>*>(0x00949A50);
    self->racers = {};
    self->shortcuts = {};
    self->jumpPaths = {};
    self->highlight = {};
    self->jumpNodes = {};
    self->records = {};
    self->bottlenecks = {};
    self->finishedCount = 0;
    self->started = 0;
    self->wrongWayTime = 0;
    Fx_Construct(&self->waypoint);
    Fx_Construct(&self->beam);
    self->showWaypoint = 1;
    self->finishOnly = 0;
    for (int i = 0; i < 10; i++) {
        self->barriers[i].a = {0.0f, 0.0f, 0.0f};
        self->barriers[i].b = {0.0f, 0.0f, 0.0f};
    }
    for (int i = 0; i < 10; i++) {
        self->barrierSounds[i] = nullptr;
    }
    for (int i = 0; i < 40; i++) {
        Fx_Construct(&self->barrierFx[0][0] + i);
    }
    self->barrierCount = 0;
    self->barrierTime = 0;
    self->barrierState = 0;
    for (int i = 0; i < 16; i++) {
        self->boosts[i] = {0.0f, 0.0f, 0.0f};
    }
    for (int i = 0; i < 16; i++) {
        Fx_Construct(&self->boostFx[i]);
    }
    self->boostCount = 0;
    Pointer_Construct(&self->transitionSprite);
    self->transitionTxd = -1;
    self->kind38 = 4;
    self->mode = 0xE;
    transitionDisplayed = 0;
    self->transition = -1;
    return self;
}
RACE45_REVERSED("CRace::CRace", 0x00711340, &Construct);

// swaps racers i and j (through a copy)
inline void SwapRacers(CRace* self, int32_t i, int32_t j) {
    RaceRacer tmp;
    Racer_CopyConstruct(&tmp, self->racers.data + i);
    Racer_Assign(self->racers.data + i, self->racers.data + j);
    Racer_Assign(self->racers.data + j, &tmp);
    if (tmp.ped != nullptr) {
        CleanUpOldReference(tmp.ped, &tmp.ped);
    }
}

// (thiscall) the unfinished racers sorted by progress (the one ahead first)
void __fastcall Sort(CRace* self, re::Edx) {
    for (int32_t i = self->finishedCount; i < self->racerCount - 1; i++) {
        int32_t j = self->racerCount - 1;
        for (int32_t n = j - i; n != 0; n--, j--) {
            if (Race_IsAhead(self, self->racers.data + j, self->racers.data + i) != 0) {
                SwapRacers(self, i, j);
            }
        }
    }
    self->sortTime = timeInMs;
}
RACE45_REVERSED("CRace::Sort", 0x00711510, &Sort);

// (thiscall) a racer reaching its node: at the race path's last node, once past it (the
// player's lap time in the arcade); else within 6 m (1 m for the player with waypoints
// shown), or within 17.5 m and nearer the next segment than the last; a far player on a
// branch's range within 17.5 m of where it rejoins. Then the node's split record, the next
// node, the order, and the finish. Returns false when the node isn't reached
uint8_t __fastcall UpdateRacerNode(CRace* self, re::Edx, RaceRacer* racer) {
    uint8_t* ped = racer->ped;
    int32_t* progress = &racer->node;
    RaceVec node;
    CopyVec(node, Path_GetNode(GetPath(ProgressPathId(racer->shortcut)), racer->node));
    RaceVec d;
    RaceVec* v = VectorSub(&d, &node, EntityPos(ped));
    float near2 = v->x * v->x + v->y * v->y;
    bool reached = false;
    if (racer->shortcut == nullptr && uint32_t(racer->node) >= Path_GetNumNodes(GetPath(currentRace->path)) - 1) {
        RaceVec prev;
        Racer_GetPrevNodePos(racer, &prev);
        RaceVec here;
        CopyVec(here, Path_GetNode(GetPath(ProgressPathId(racer->shortcut)), racer->node));
        RaceVec along, past;
        VectorSub(&along, &here, &prev);
        VectorSub(&past, EntityPos(ped), &here);
        Vector_Normalise(&along);
        Vector_Normalise(&past);
        float dot = DotYXZ(along, past);
        if (dot < 0.0f) {
            return 0;
        }
        if (self->arcade != 0 && At<int32_t>(racer->ped, 0x1310) == 0xD) {
            void* hud = raceHudPtr;
            if (hud != nullptr) {
                RaceHud_SetLap(hud, int(timeInMs - self->lapTime));
            }
            self->lapTime = timeInMs;
        }
        reached = true;
    }
    if (!reached) {
        int32_t type = At<int32_t>(ped, 0x1310);
        bool check = false;
        if ((type != 0xD || self->showWaypoint == 0) && 36.0f < near2) {
            check = true;
        } else if (type == 0xD && 1.0f < near2) {
            check = true;
        }
        if (check) {
            if (!(near2 > 306.25f)) {
                RaceVec next, prev, here;
                Racer_GetNextNodePos(racer, &next);
                Racer_GetPrevNodePos(racer, &prev);
                RaceProgress_GetPos(progress, &here);
                RaceVec back, ahead, at;
                VectorSub(&back, &prev, &here);
                VectorSub(&ahead, &next, &here);
                VectorSub(&at, EntityPos(ped), &here);
                Vector_Normalise(&back);
                Vector_Normalise(&ahead);
                Vector_Normalise(&at);
                float s = ahead.y * at.y + at.x * ahead.x;
                float toNext = s + ahead.z * at.z;
                float t = at.x * back.x + at.y * back.y;
                float toPrev = t + at.z * back.z;
                if (toPrev > toNext) {
                    return 0;
                }
            } else {
                RacePathRange* r = Race_FindBranch(self, racer->node, 0);
                if (At<int32_t>(ped, 0x1310) != 0xD || r == nullptr) {
                    return 0;
                }
                uint8_t* branchPath = GetPath(r->path);
                RaceVec e;
                RaceVec* w = VectorSub(&e, Path_GetNode(branchPath, r->last), EntityPos(ped));
                float far2 = w->x * w->x + w->y * w->y;
                if (far2 > 306.25f) {
                    return 0;
                }
            }
        }
    }
    // reached: the split record of a race-path node (not on a branch's range)
    if (racer->shortcut == nullptr && Race_FindBranch(self, racer->node, 0) == nullptr) {
        RaceRecord* rec = &self->records.data[racer->node];
        if (At<int32_t>(racer->ped, 0x1310) == 0xD) {
            if (rec->ped != nullptr && rec->lap >= racer->lap) {
                void* hud = raceHudPtr;
                if (hud != nullptr) {
                    RaceHud_SetSplit(hud, int(timeInMs - rec->time));
                }
            } else if (rec->lap <= racer->lap) {
                EntityPtr_Assign(&rec->ped, &racer->ped);
                self->records.data[racer->node].time = timeInMs;
                self->records.data[racer->node].lap = racer->lap;
            }
        } else {
            bool done = false;
            if (rec->ped == playerPed) {
                int32_t behindPlayer = Race_GetPosition(self, playerPed) + 1;
                if (Race_GetPosition(self, racer->ped) == behindPlayer) {
                    void* hud = raceHudPtr;
                    if (hud != nullptr) {
                        RaceHud_SetSplit(hud, int(rec->time - timeInMs));
                    }
                    done = true;
                }
            }
            if (!done) {
                int32_t playerPlace = Race_GetPosition(self, playerPed);
                if (Race_GetPosition(self, racer->ped) < playerPlace) {
                    EntityPtr_Assign(&rec->ped, &racer->ped);
                    self->records.data[racer->node].time = timeInMs;
                    self->records.data[racer->node].lap = racer->lap;
                }
            }
        }
    }
    bool last = racer->shortcut == nullptr &&
                uint32_t(racer->node) >= Path_GetNumNodes(GetPath(currentRace->path)) - 1;
    Racer_NextNode(racer, 1, last ? 0 : 1);
    Race_Sort(self);
    if (racer->lap >= self->laps) {
        Race_MarkFinished(self, ped);
    }
    return 1;
}
RACE45_REVERSED("CRace::UpdateRacerNode", 0x00711610, &UpdateRacerNode);

// (thiscall) how far the ped is ahead of the player along the race (0 when either isn't
// racing)
float __fastcall DistanceAheadOfPlayer(CRace* self, re::Edx, uint8_t* ped) {
    RaceRacer* a = Race_GetRacer(self, ped);
    RaceRacer* b = Race_GetRacer(self, playerPed);
    if (a != nullptr && b != nullptr) {
        return Race_DistanceAhead(self, a, b);
    }
    return 0.0f;
}
RACE45_REVERSED("CRace::DistanceAheadOfPlayer", 0x00711B80, &DistanceAheadOfPlayer);

// (thiscall) the race started: the clocks, the player on to its first highlighted node and
// its waypoint, the records cleared; the grid order (nearest first to a point 30 m to the
// player's left), the script racers' lanes from -1 by that order; in the arcade the
// player's kart's respawn clock cleared
void __fastcall Start(CRace* self, re::Edx) {
    self->started = 1;
    self->startTime = timeInMs;
    self->lapTime = timeInMs;
    self->wrongWayNode = 0;
    self->wrongWayTime = 0;
    uint8_t* player = playerPed;
    RaceRacer* mine = Race_GetRacer(self, player);
    if (mine != nullptr) {
        Racer_SkipToHighlight(mine);
    }
    Race_ShowNextWaypoint(self, player);
    for (uint32_t i = 0; i < self->records.count; i++) {
        RaceRecord& r = self->records.data[i];
        if (r.ped != nullptr) {
            CleanUpOldReference(r.ped, &r.ped);
            r.ped = nullptr;
        }
        r.time = 0;
        r.lap = 0;
    }
    RaceVec right;
    float* m = At<float*>(player, 0x14);
    if (m != nullptr) {
        right.x = m[0];
        right.y = m[1];
        right.z = m[2];
    } else {
        Placeable_GetRight(player + 4, &right);
    }
    RaceVec side;
    side.x = right.x * 30.0f;
    side.y = right.y * 30.0f;
    side.z = right.z * 30.0f;
    RaceVec grid;
    VectorSub(&grid, EntityPos(player), &side);
    for (int32_t i = self->finishedCount; i < self->racerCount - 1; i++) {
        int32_t j = self->racerCount - 1;
        for (int32_t n = j - i; n != 0; n--, j--) {
            RaceVec a, b;
            RaceVec* va = VectorSub(&a, &grid, EntityPos(self->racers.data[j].ped));
            float xy = va->x * va->x + va->y * va->y;
            float dj = xy + va->z * va->z;
            RaceVec* vb = VectorSub(&b, &grid, EntityPos(self->racers.data[i].ped));
            float xy2 = vb->x * vb->x + vb->y * vb->y;
            float di = xy2 + vb->z * vb->z;
            if (di > dj) {
                SwapRacers(self, i, j);
            }
        }
    }
    int32_t lane = -1;
    for (int32_t i = 0; i < self->racerCount; i++) {
        RaceRacer& r = self->racers.data[i];
        if (At<int32_t>(r.ped, 0x1310) != 0xD) {
            r.startSlot = lane;
            r.lane = lane;
            lane++;
        }
    }
    Race_Sort(self);
    if (self->arcade != 0 && PedVehicle(playerPed) != nullptr) {
        At<uint32_t>(PedVehicle(playerPed), 0x348) = 0;
    }
}
RACE45_REVERSED("CRace::Start", 0x00711BC0, &Start);

// the AI's braking: a bike too fast along its heading (past `limit`) pushed back by its
// speed, the AI tuning's brake factor (+0x58) and its mass
inline void BrakeBike(uint8_t* vehicle) {
    RaceVec back;
    back.x = -At<float>(vehicle, 0x124);
    back.y = -At<float>(vehicle, 0x128);
    back.z = -At<float>(vehicle, 0x12C);
    RaceVec a, b;
    float mass = At<float>(vehicle, 0x158);
    float factor = At<float>(aiTuningRace, 0x58);
    RaceVec* scaled = VectorScale(&a, &back, factor);
    Physical_ApplyMoveForce(vehicle, VectorScale(&b, scaled, mass));
}

// (thiscall) a racer's speed (a factor of its vehicle's): ahead of the player by more than
// its ahead distance it eases (by 0.2 a metre) towards its ahead speed, else it may boost
// (a chance each time its boost timer runs past 0; boosting, its boost speed until the
// duration); behind by more than its behind distance it eases (0.4 a metre) towards its
// behind speed. Eased by how straight it heads along the path (squared); a bike over 1.5x
// (1.1x behind) its handling's top speed braked
float __fastcall GetSpeed(CRace* self, re::Edx, uint8_t* ped) {
    RaceRacer* a = Race_GetRacer(self, ped);
    RaceRacer* b = Race_GetRacer(self, playerPed);
    float ahead = (a != nullptr && b != nullptr) ? Race_DistanceAhead(self, a, b) : 0.0f;
    RaceRacer* racer = Race_GetRacer(self, ped);
    float result = racer->speed;
    RaceVec prev;
    Racer_GetPrevNodePos(racer, &prev);
    RaceVec node;
    CopyVec(node, Path_GetNode(GetPath(ProgressPathId(racer->shortcut)), racer->node));
    RaceVec tmp;
    RaceVec dir = *VectorSub(&tmp, &node, &prev);
    Vector_Normalise(&dir);
    RaceVec forward;
    RaceVec* f = Placeable_GetForward(ped, &forward);
    float s = f->y * dir.y + f->x * dir.x;
    float facing = s + f->z * dir.z;
    float step = raceBoostRate * timeStep;
    float timer = step + racer->boostTimer;
    racer->boostTimer = timer;
    if (ahead > 0.0f) {
        if (racer->boosting != 0) {
            result = racer->boostSpeed;
            if (float(racer->boostDuration) < timer) {
                racer->boosting = 0;
                racer->boostTimer = float(-racer->boostInterval);
            }
            return result;
        }
        if (racer->aheadDistance < ahead) {
            float span = racer->aheadSpeed - racer->speed;
            float k = (ahead - racer->aheadDistance) * 0.2f;
            if (k < 0.0f) {
                k = 0.0f;
            } else if (1.0f < k) {
                k = 1.0f;
            }
            float eased = k * span;
            eased = eased * facing;
            eased = eased * facing;
            result = eased + racer->speed;
        } else {
            if (Ped_IsSeatedInVehicle(ped) != 0) {
                uint8_t* vehicle = PedVehicle(ped);
                if (vehicle != nullptr && At<int32_t>(vehicle, 0x46C) == 1) {
                    RaceVec fwd;
                    float along = DotProduct(MoveSpeed(vehicle), Placeable_GetForward(vehicle, &fwd));
                    float limit = At<float>(At<uint8_t*>(vehicle, 0x2A4), 0x84) * 1.5f;
                    if (limit < along) {
                        BrakeBike(vehicle);
                    }
                }
            }
        }
        if (0.0f < racer->boostTimer) {
            float q = float(Rand()) / 32767.0f;
            float roll = q * (100.0f - 0.0f) + 0.0f;
            if (float(racer->boostChance) > roll) {
                racer->boostTimer = 0.0f;
                racer->boosting = 1;
                return result;
            }
            racer->boostTimer = float(-racer->boostInterval);
        }
        return result;
    }
    racer->boosting = 0;
    float behind = ahead * -1.0f;
    if (racer->behindDistance < behind) {
        float span = racer->speed - racer->behindSpeed;
        float k = (behind - racer->behindDistance) * 0.4f;
        if (k < 0.0f) {
            k = 0.0f;
        } else if (1.0f < k) {
            k = 1.0f;
        }
        float eased = k * span;
        eased = eased * facing;
        eased = eased * facing;
        result = racer->speed - eased;
    }
    if (Ped_IsSeatedInVehicle(ped) != 0) {
        uint8_t* vehicle = PedVehicle(ped);
        if (vehicle != nullptr && At<int32_t>(vehicle, 0x46C) == 1) {
            RaceVec fwd;
            RaceVec* fv = Placeable_GetForward(vehicle, &fwd);
            const RaceVec& sp = *MoveSpeed(vehicle);
            float t = sp.y * fv->y + sp.x * fv->x;
            float along = t + sp.z * fv->z;
            float limit = At<float>(At<uint8_t*>(vehicle, 0x2A4), 0x84) * 1.1f;
            if (limit < along) {
                BrakeBike(vehicle);
            }
        }
    }
    return result;
}
RACE45_REVERSED("CRace::GetSpeed", 0x00711EC0, &GetSpeed);

// (thiscall, vtable +4) the race's frame: the exit question (and its answer); the arcade's
// back button (a transition screen's sound); before the start the player's vehicle held
// still; then the barriers' cycle (off 1 s, charging 1.8 s, on 1 s), the arcade's effects
// and checks for the player, its node, the bump shake, and the wrong-way warning (heading
// back along the race path's nearest visible segment for 2 s, not near a branch)
void __fastcall Update(CRace* self, re::Edx) {
    void* popup = &self->popupKind;
    if (self->popupShowing != 0 && self->popupKind == 1) {
        if (Popup_Result(popup) == 2) {
            Race_DestroyTransition(self);
            self->active = 0;
        }
        return;
    }
    if (userStartResponse != 0) {
        return;
    }
    if (self->arcade != 0 && Pad_JustDown(8, 0) != 0) {
        if (transitionDisplayed != 0) {
            if (self->transition == 5 || self->transition == 7) {
                Sounds_Play(sounds, Sounds_GetId(sounds, "BUMP02"), nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
            }
            if (transitionDisplayed != 0) {
                int32_t t = self->transition;
                if (t == 8 || t == 2 || t == 3 || t == 6 || t == 0 || t == 4) {
                    return;
                }
            }
        }
        Popup_Show(popup, 1, "EXIT_QUESTION", "EXIT_BUTTONS", 7, 8);
        return;
    }
    if (transitionDisplayed != 0) {
        return;
    }
    if (self->started == 0) {
        uint8_t* player = playerPed;
        if (player == nullptr || Ped_IsSeatedInVehicle(player) == 0) {
            return;
        }
        Physical_SetMoveSpeed(PedVehicle(player), zeroVector);
        Physical_SetTurnSpeed(PedVehicle(player), zeroVector);
        if (self->arcade != 0 && cameraMode == 0xE) {
            At<uint32_t>(PedVehicle(player), 0x348) = timeInMs - 0x9C5;
        }
        return;
    }
    if (self->racerCount == int32_t(self->racers.count)) {
        Race_Sort(self);
    }
    if (self->barrierState == 0) {
        if (self->barrierTime + 1000 < timeInMs) {
            self->barrierState = 1;
            self->barrierTime = timeInMs;
        }
    } else if (self->barrierState == 1) {
        if (self->barrierTime + 1800 < timeInMs) {
            self->barrierState = 2;
            self->barrierTime = timeInMs;
        }
    } else if (self->barrierTime + 1000 < timeInMs) {
        self->barrierState = 0;
        self->barrierTime = timeInMs;
    }
    uint8_t* player = playerPed;
    Race_UpdateBarriers(self);
    Race_UpdateBoostPads(self);
    Race_CheckBarriers(self, player);
    Race_CheckKart(self, player);
    Race_CheckBoostPads(self, player);
    if (Race_HasFinished(self, player) != 0) {
        CameraShake_Enable(cameraShake, 0);
        Fx_Kill(&self->beam);
        Fx_Kill(&self->waypoint);
        return;
    }
    RaceRacer* racer = Race_GetRacer(self, player);
    if (Race_UpdateRacerNode(self, racer) != 0) {
        if (self->showWaypoint != 0) {
            Sounds_PlayFrontEnd(sounds, 0x1C);
        }
        Race_ShowNextWaypoint(self, player);
    }
    if (self->arcade != 0 && racer->bumpTime + 1000 > timeInMs && cameraMode != 0xE) {
        CameraShake_Enable(cameraShake, 1);
        float x = float(MsX87(timeInMs - racer->bumpTime));
        float pulse = x / 500.0f;
        if (pulse >= 1.0f) {
            float over = pulse - 1.0f;
            pulse = 1.0f - over;
        }
        if (0.0f > pulse) {
            pulse = 0.0f;
        }
        RaceVec offset = {0.1f, 0.1f, 0.0f};
        float t = x * 0.001f;
        float angle = (t * 20.0f) * kRaceTwoPi;
        float wave = trk::CrtSin(angle);
        Vector_Scale(&offset, wave * pulse);
        CameraShake_SetOffset(cameraShake, offset.x, offset.y, offset.z);
        CameraShake_SetRotation(cameraShake, zeroVector[0], zeroVector[1], zeroVector[2]);
    } else {
        CameraShake_Enable(cameraShake, 0);
    }
    if (self->showWaypoint != 0 || player == nullptr || Ped_IsSeatedInVehicle(player) == 0 ||
        racer->shortcut != nullptr || timeInMs / 1000 == previousTimeInMs / 1000) {
        return;
    }
    // the race path's segment nearest the player's vehicle that it is past the start of and
    // can see the end of
    uint8_t* path = GetPath(self->path);
    float best = 1e10f;
    RaceVec bestSegment = {0.0f, 0.0f, 0.0f};
    int32_t bestNode = -1;
    int32_t bestNext = -1;
    RaceVec from = {0.0f, 0.0f, 0.0f};
    RaceVec to = {0.0f, 0.0f, 0.0f};
    for (uint32_t i = 0; i < Path_GetNumNodes(path); i++) {
        uint32_t node = uint32_t(self->wrongWayNode) + i;
        if (node >= Path_GetNumNodes(path)) {
            node -= Path_GetNumNodes(path);
        }
        CopyVec(from, Path_GetNode(path, int32_t(node)));
        uint32_t next = node + 1;
        if (next >= Path_GetNumNodes(path)) {
            next = 0;
        }
        CopyVec(to, Path_GetNode(path, int32_t(next)));
        RaceVec tmp;
        RaceVec segment = *VectorSub(&tmp, &to, &from);
        RaceVec offset = *VectorSub(&tmp, EntityPos(PedVehicle(player)), &from);
        if (!(DotProduct(&segment, &offset) > 0.0f)) {
            continue;
        }
        float d2 = Vector_MagnitudeSqr(&offset);
        if (!(best > d2)) {
            continue;
        }
        float* vehiclePos = EntityPos(PedVehicle(player));
        RaceVec high;
        VectorAdd(&high, &to, upVector);
        if (GetIsLineOfSightClear(vehiclePos, &high, 1) == 0) {
            continue;
        }
        best = d2;
        bestNode = int32_t(node);
        bestNext = int32_t(next);
        bestSegment = segment;
    }
    if (bestNode == -1) {
        self->wrongWayTime = 0;
        return;
    }
    self->wrongWayNode = bestNode;
    RaceVec forward;
    if (!(DotProduct(&bestSegment, Placeable_GetForward(PedVehicle(player), &forward)) < 0.0f)) {
        self->wrongWayTime = 0;
        return;
    }
    if (Race_FindBranch(self, bestNode, 1) != nullptr || Race_FindBranch(self, bestNext, 1) != nullptr) {
        self->wrongWayTime = 0;
        return;
    }
    // not when a branch's node is nearer
    for (uint32_t i = 0; i < self->shortcuts.count; i++) {
        uint8_t* branch = GetPath(self->shortcuts.data[i].path);
        for (uint32_t k = 0; k < Path_GetNumNodes(branch); k++) {
            RaceVec tmp;
            float d2 = Vector_MagnitudeSqr(VectorSub(&tmp, Path_GetNode(branch, int32_t(k)), EntityPos(PedVehicle(player))));
            if (best > d2) {
                self->wrongWayTime = 0;
                return;
            }
        }
    }
    for (uint32_t i = 0; i < self->jumpPaths.count; i++) {
        uint8_t* branch = GetPath(self->jumpPaths.data[i].path);
        for (uint32_t k = 0; k < Path_GetNumNodes(branch); k++) {
            RaceVec tmp;
            RaceVec* v = VectorSub(&tmp, Path_GetNode(branch, int32_t(k)), EntityPos(PedVehicle(player)));
            float xy = v->x * v->x + v->y * v->y;
            float d2 = xy + v->z * v->z;
            if (best > d2) {
                self->wrongWayTime = 0;
                return;
            }
        }
    }
    uint32_t since = self->wrongWayTime;
    if (since != 0) {
        if (since + 2000 < timeInMs) {
            BigMessage_Show("RACE_WRONG_WAY", 1.0f);
            return;
        }
        return;
    }
    self->wrongWayTime = timeInMs;
}
RACE45_REVERSED("CRace::Update", 0x007123C0, &Update);

// a teleport through the entity's virtual (+0x38, a CVector by value)
inline void Teleport(uint8_t* entity, const RaceVec& pos) {
    using Fn = void(__thiscall*)(uint8_t*, RaceVec);
    re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[0x38 / 4])(entity, pos);
}

// any racer but `self` (or its vehicle) stands within the rect
inline bool RacerInRect(CRace* self, uint8_t* ped, const RaceRect& rect) {
    uint32_t count = self->racers.count;
    RaceRacer* r = self->racers.data;
    for (uint32_t i = 0; i < count; i++, r++) {
        uint8_t* other = r->ped;
        if (other == nullptr || ped == other) {
            continue;
        }
        float* p = EntityPos(other);
        float point[2] = {p[0], p[1]};
        if (Rect_IsPointInside(&rect, point) != 0) {
            return true;
        }
        uint8_t* vehicle = PedVehicle(other);
        if (vehicle != nullptr) {
            float* q = EntityPos(vehicle);
            float vpoint[2] = {q[0], q[1]};
            if (Rect_IsPointInside(&rect, vpoint) != 0) {
                return true;
            }
        }
    }
    return false;
}

GAME_VAR(int32_t, currAreaRace, 0x00BD1008);
GAME_VAR(RaceTxdPool*, vehiclePoolRace, 0x00C0F5F4);   // CPools::ms_pVehiclePool (the same CPool header)

// (thiscall) a racer's frame (false once it has finished): after the start its barrier,
// kart and boost checks; a script racer more than 80 m from the player (not in area 0x2A or
// the arcade) caught up: over no ground, or (first) ahead of the player by 150 m, moved a
// node towards the player (up to 3 times, not onto another racer, not past the player)
// with its vehicle (or remounted on its bike); its node, the jump button released; its
// lane by the nearest vehicle within 15 m: the player's (to the side it is on, on a bike
// the other way while ahead), else a racer's ahead (moving over when sharing its lane)
uint8_t __fastcall UpdateRacer(CRace* self, re::Edx, uint8_t* ped) {
    if (Race_HasFinished(self, ped) != 0) {
        return 0;
    }
    if (currentRace->started == 0) {
        return 1;
    }
    Race_CheckBarriers(self, ped);
    Race_CheckKart(self, ped);
    Race_CheckBoostPads(self, ped);
    RaceRacer* racer = Race_GetRacer(self, ped);
    RaceRacer* mine = Race_GetRacer(self, playerPed);
    if (At<int32_t>(ped, 0x1310) != 0xD && currAreaRace != 0x2A && self->arcade == 0) {
        float coors[3];
        if (DistanceSquared(EntityPos(ped), FindPlayerCoors(coors)) > 6400.0f) {
            int attempts = 0;
            for (;;) {
                if (attempts >= 3) {
                    break;
                }
                RaceColPoint col;
                ColPoint_Construct(&col);
                RaceVec at = {0.0f, 0.0f, 0.0f};
                uint8_t* vehicle = PedVehicle(ped);
                const float* p = vehicle != nullptr ? EntityPos(vehicle) : &Placeable_GetPosition(ped)->x;
                at.x = p[0];
                at.y = p[1];
                at.z = p[2];
                float away = DistanceSquared(&at, FindPlayerCoors(coors));
                at.z = at.z + 20.0f;
                bool found = false;
                bool warp = false;
                if (attempts == 0 && Race_IsAhead(self, Race_GetRacer(self, playerPed), racer) != 0 &&
                    Race_DistanceAheadOfPlayer(self, ped) > 150.0f) {
                    warp = true;
                } else {
                    float bottom = at.z - 140.0f;
                    re::Ptr32<uint8_t> hit = nullptr;
                    if (ProcessVerticalLine(&at, bottom, &col, &hit, 1, 0) != 0) {
                        found = true;
                    } else {
                        warp = true;
                    }
                }
                if (warp) {
                    uint8_t* path = GetPath(self->path);
                    int32_t playerPlace = Race_GetPosition(self, playerPed);
                    bool abort = false;
                    if (Race_GetPosition(self, ped) > playerPlace) {
                        // behind the player: a node on
                        RaceVec next;
                        float nearer = DistanceSquared(Racer_GetNextNodePos(racer, &next), FindPlayerCoors(coors));
                        if (away < nearer || away == nearer) {
                            ColPoint_Destruct(&col);
                            break;
                        }
                        RaceRect rect;
                        rect.left = Racer_GetNextNodePos(racer, &next)->x - 2.5f;
                        rect.top = Racer_GetNextNodePos(racer, &next)->y - 2.5f;
                        rect.right = Racer_GetNextNodePos(racer, &next)->x + 2.5f;
                        rect.bottom = Racer_GetNextNodePos(racer, &next)->y + 2.5f;
                        if (RacerInRect(self, ped, rect)) {
                            ColPoint_Destruct(&col);
                            break;
                        }
                        if (racer->shortcut != nullptr) {
                            racer->node = racer->shortcut->last;
                            racer->shortcut = nullptr;
                        } else {
                            racer->node++;
                            if (uint32_t(racer->node) >= Path_GetNumNodes(path)) {
                                racer->lap++;
                                racer->node = 0;
                            }
                        }
                        uint32_t theirs = Path_GetNumNodes(path) * uint32_t(mine->lap) + uint32_t(mine->node);
                        uint32_t ours = Path_GetNumNodes(path) * uint32_t(racer->lap) + 1 + uint32_t(racer->node);
                        if (!(ours < theirs)) {
                            if (racer->node != 0) {
                                racer->node--;
                            } else {
                                racer->node = int32_t(Path_GetNumNodes(path) - 1);
                                racer->lap--;
                            }
                            abort = true;
                        }
                    } else {
                        // ahead of the player: a node back
                        RaceVec prev;
                        float nearer = DistanceSquared(Racer_GetPrevNodePos(racer, &prev), FindPlayerCoors(coors));
                        if (away < nearer || away == nearer) {
                            ColPoint_Destruct(&col);
                            break;
                        }
                        RaceRect rect;
                        rect.left = Racer_GetPrevNodePos(racer, &prev)->x - 2.5f;
                        rect.top = Racer_GetPrevNodePos(racer, &prev)->y - 2.5f;
                        rect.right = Racer_GetPrevNodePos(racer, &prev)->x + 2.5f;
                        rect.bottom = Racer_GetPrevNodePos(racer, &prev)->y + 2.5f;
                        if (RacerInRect(self, ped, rect)) {
                            ColPoint_Destruct(&col);
                            break;
                        }
                        if (racer->shortcut != nullptr) {
                            racer->node = racer->shortcut->first;
                            racer->shortcut = nullptr;
                        } else if (racer->node == 0) {
                            racer->node = int32_t(Path_GetNumNodes(path) - 1);
                            racer->lap--;
                        } else {
                            racer->node--;
                        }
                    }
                    if (abort) {
                        ColPoint_Destruct(&col);
                        break;
                    }
                    RaceVec target;
                    RaceVec tmp;
                    target = *RaceProgress_GetPos(&racer->node, &tmp);
                    target.z = target.z + 3.0f;
                    if ((Ped_IsSeatedInVehicle(ped) != 0 && At<int32_t>(PedVehicle(ped), 0x46C) == 1) ||
                        (Ped_IsSeatedInVehicle(ped) != 0 && At<int32_t>(PedVehicle(ped), 0x46C) == 0)) {
                        Teleport(PedVehicle(ped), target);
                    } else {
                        Teleport(ped, target);
                        uint8_t* bike = At<uint8_t*>(ped, 0x1560);
                        if (bike != nullptr && Seats_GetOccupant(bike + 0x1C4, 0) == nullptr) {
                            racer->mountTime = timeInMs;
                            uint8_t* own = At<uint8_t*>(ped, 0x1560);
                            int32_t kind = At<int32_t>(own, 0x46C);
                            if (kind == 1) {
                                Teleport(own, target);
                                Ped_MountBike(ped, At<uint8_t*>(ped, 0x1560));
                            } else if (kind == 0) {
                                Teleport(own, target);
                                Ped_WarpIntoFreeSeat(ped, At<uint8_t*>(ped, 0x1560), 1);
                            }
                        }
                    }
                    attempts++;
                    racer->warpTime = timeInMs;
                }
                ReleaseColPointEntity(col.entity);
                if (found) {
                    break;
                }
            }
        }
    }
    Race_UpdateRacerNode(self, racer);
    if (racer->jumpTime != 0 && timeInMs - racer->jumpTime > 200) {
        ActionInputs_SetRequest(At<void*>(ped, 0x2E0), 4, 3, 0.0f);
        racer->jumpTime = 0;
    }
    uint8_t* myVehicle = PedVehicle(ped);
    if (myVehicle == nullptr) {
        return 1;
    }
    // the nearest other vehicle within 15 m
    RaceTxdPool* pool = vehiclePoolRace;
    float best = 225.0f;
    uint8_t* nearest = nullptr;
    for (int32_t i = pool->size; i != 0;) {
        i--;
        if ((pool->flags[i] & 0x80) != 0) {
            continue;
        }
        uint8_t* v = pool->objects + pool->slotSize * i;
        if (v == nullptr || v == PedVehicle(ped)) {
            continue;
        }
        RaceVec tmp;
        RaceVec* d = VectorSub(&tmp, EntityPos(v), EntityPos(PedVehicle(ped)));
        float xy = d->x * d->x + d->y * d->y;
        float d2 = xy + d->z * d->z;
        if (best > d2) {
            best = d2;
            nearest = v;
        }
    }
    if (nearest == nullptr) {
        racer->lane = 0;
        return 1;
    }
    void* seats = nearest + 0x1C4;
    uint8_t* driver = Seats_GetOccupant(seats, 0);
    if (driver != nullptr && At<int32_t>(Seats_GetOccupant(seats, 0), 0x1310) == 0xD) {
        uint8_t* own = PedVehicle(ped);
        RaceVec tmp, right;
        RaceVec* toThem = VectorSub(&tmp, EntityPos(nearest), EntityPos(own));
        float side = DotProduct(Entity_GetRight(own, &right), toThem);
        int32_t kind;
        if (side > 0.0f) {
            if (*Vehicle_GetClass(PedVehicle(ped), &kind) == 7) {
                int32_t theirs = Race_GetPosition(self, Seats_GetOccupant(seats, 0));
                if (Race_GetPosition(self, ped) < theirs) {
                    racer->lane = 1;
                    return 1;
                }
            }
            racer->lane = -1;
            return 1;
        }
        if (*Vehicle_GetClass(PedVehicle(ped), &kind) == 7) {
            int32_t theirs = Race_GetPosition(self, Seats_GetOccupant(seats, 0));
            if (Race_GetPosition(self, ped) < theirs) {
                racer->lane = -1;
                return 1;
            }
        }
        racer->lane = 1;
        return 1;
    }
    RaceRacer* other = Race_GetRacer(self, Seats_GetOccupant(seats, 0));
    if (other != nullptr) {
        int32_t theirs = Race_GetPosition(self, Seats_GetOccupant(seats, 0));
        if (Race_GetPosition(self, ped) > theirs) {
            if (racer->lane != other->lane) {
                return 1;
            }
            if (other->lane == 0) {
                racer->lane = racer->startSlot >= other->startSlot ? 1 : -1;
                return 1;
            }
        }
    }
    racer->lane = 0;
    return 1;
}
RACE45_REVERSED("CRace::UpdateRacer", 0x00712CA0, &UpdateRacer);

// ---- 0x7136A0..0x713E20: the script natives -------------------------------------------------------
// (on the running minigame, a CRace; a ped handle of kind 2)

GAME_FN(void, __thiscall, Race_AddBarrier_x64, 0x0070B7A0, CRace* self, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Race_AddBarrier(CRace* a0, float a1, float a2, float a3, float a4, float a5, float a6) {
    return Race_AddBarrier_x64(a0, re::BitsOf(a1), re::BitsOf(a2), re::BitsOf(a3), re::BitsOf(a4), re::BitsOf(a5), re::BitsOf(a6));
}
GAME_FN(void, __thiscall, Race_AddBoostPoint_x64, 0x0070B800, CRace* self, uint32_t, uint32_t, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Race_AddBoostPoint(CRace* a0, float a1, float a2, float a3) {
    return Race_AddBoostPoint_x64(a0, re::BitsOf(a1), re::BitsOf(a2), re::BitsOf(a3));
}
GAME_FN(int32_t, __thiscall, Path_IndexOfId, 0x005EC940, uint8_t* path, int32_t id);
GAME_FN(void, __thiscall, Race_SetupRace, 0x007110C0, CRace* self, int32_t path, int32_t laps, int32_t opponents,
        int arcade);
GAME_FN(void, __thiscall, Race_RemoveRacer, 0x00710560, CRace* self, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Race_HasRacer, 0x0070D3E0, CRace* self, uint8_t* ped);
GAME_FN(void, __thiscall, Race_AddShortcut, 0x0070EFB0, CRace* self, int32_t path, int32_t first, int32_t last);
GAME_FN(void, __thiscall, Race_AddJumpPath, 0x0070F010, CRace* self, int32_t path, int32_t first, int32_t last);
GAME_FN(int, __thiscall, Race_DisplayTransition, 0x0070F960, CRace* self, int screen, int second);
GAME_FN(uint8_t*, __thiscall, Race_GetRacerInPosition, 0x0070D460, CRace* self, int32_t index);
GAME_VAR(CPool<uint8_t>*, pedPoolRace, 0x00C0F5F0);   // CPools::ms_pPedPool

inline uint8_t* RacePed(lua_State* L, int i) {
    return static_cast<uint8_t*>(ScriptGame::GetPedFromHandle(Script::GetInt(L, i), 2));
}

// (path, laps, opponents, arcade?)
int __cdecl Native_RaceSetupRace(lua_State* L) {
    int32_t path = Script::GetInt(L, 0);
    int32_t laps = Script::GetInt(L, 1);
    int32_t opponents = Script::GetInt(L, 2);
    uint8_t arcade = Script::GetBoolOpt(L, 3, 0);
    Race_SetupRace(currentRace, path, laps, opponents, arcade);
    return 0;
}
RACE45_REVERSED("Native_RaceSetupRace", 0x007136A0, &Native_RaceSetupRace);

int __cdecl Native_RaceAddRacer(lua_State* L) {
    uint8_t* ped = RacePed(L, 0);
    if (ped != nullptr) {
        Race_AddRacer(currentRace, ped);
    }
    return 0;
}
RACE45_REVERSED("Native_RaceAddRacer", 0x00713700, &Native_RaceAddRacer);

int __cdecl Native_RaceRemoveRacer(lua_State* L) {
    uint8_t* ped = RacePed(L, 0);
    if (ped != nullptr) {
        Race_RemoveRacer(currentRace, ped);
    }
    return 0;
}
RACE45_REVERSED("Native_RaceRemoveRacer", 0x00713740, &Native_RaceRemoveRacer);

// (ped, boost speed, speed, ahead distance, ahead speed, behind distance, behind speed,
// [shortcut chance, +0x40, +0x44, boost interval, boost duration, boost chance, aggression])
// (no check that the ped races)
int __cdecl Native_RaceSetRacerStats(lua_State* L) {
    uint8_t* ped = RacePed(L, 0);
    if (ped != nullptr) {
        RaceRacer* racer = Race_GetRacer(currentRace, ped);
        racer->boostSpeed = Script::GetFloat(L, 1);
        racer->speed = Script::GetFloat(L, 2);
        racer->aheadDistance = Script::GetFloat(L, 3);
        racer->aheadSpeed = Script::GetFloat(L, 4);
        racer->behindDistance = Script::GetFloat(L, 5);
        racer->behindSpeed = Script::GetFloat(L, 6);
        racer->shortcutChance = Script::GetIntOpt(L, 7, 0);
        racer->unknown40 = Script::GetIntOpt(L, 8, 0);
        racer->unknown44 = Script::GetIntOpt(L, 9, 0);
        racer->boostInterval = Script::GetIntOpt(L, 10, 0);
        racer->boostDuration = Script::GetIntOpt(L, 11, 0);
        racer->boostChance = int32_t(Script::GetFloatOpt(L, 12, 0.0f));   // (truncated)
        racer->aggression = Script::GetFloatOpt(L, 13, 0.5f);
    }
    return 0;
}
RACE45_REVERSED("Native_RaceSetRacerStats", 0x00713780, &Native_RaceSetRacerStats);

// (true without a race)
int __cdecl Native_RaceHasFinished(lua_State* L) {
    CRace* race = currentRace;
    uint8_t finished = 0;
    if (race == nullptr || race->racerCount == race->finishedCount) {
        finished = 1;
    }
    Script::PushBool(L, finished);
    return 1;
}
RACE45_REVERSED("Native_RaceHasFinished", 0x007138B0, &Native_RaceHasFinished);

int __cdecl Native_RaceHasRacerFinished(lua_State* L) {
    uint8_t* ped = RacePed(L, 0);
    if (ped != nullptr && Race_HasRacer(currentRace, ped) != 0) {
        Script::PushBool(L, Race_HasFinished(currentRace, ped));
        return 1;
    }
    Script::PushBool(L, 0);
    return 1;
}
RACE45_REVERSED("Native_RaceHasRacerFinished", 0x007138F0, &Native_RaceHasRacerFinished);

// (1-based; -1 for no ped; no check that it races)
int __cdecl Native_RaceGetRacerLapNum(lua_State* L) {
    uint8_t* ped = RacePed(L, 0);
    if (ped == nullptr) {
        Script::PushInt(L, -1);
        return 1;
    }
    Script::PushInt(L, Race_GetRacer(currentRace, ped)->lap + 1);
    return 1;
}
RACE45_REVERSED("Native_RaceGetRacerLapNum", 0x00713960, &Native_RaceGetRacerLapNum);

int __cdecl Native_RaceGetPositionInRaceOfRacer(lua_State* L) {
    uint8_t* ped = RacePed(L, 0);
    if (ped == nullptr) {
        Script::PushInt(L, -1);
        return 1;
    }
    Script::PushInt(L, Race_GetPosition(currentRace, ped));
    return 1;
}
RACE45_REVERSED("Native_RaceGetPositionInRaceOfRacer", 0x007139F0, &Native_RaceGetPositionInRaceOfRacer);

int __cdecl Native_RaceGetRacerRaceTime(lua_State* L) {
    uint8_t* ped = RacePed(L, 0);
    float time = ped == nullptr ? -1.0f : Race_GetRaceTime(currentRace, ped);
    Script::PushFloat(L, time);
    return 1;
}
RACE45_REVERSED("Native_RaceGetRacerRaceTime", 0x00713A50, &Native_RaceGetRacerRaceTime);

// (path, the race path's node ids it leaves from and rejoins at)
template <bool kJump>
int __cdecl Native_RaceAddBranchPath(lua_State* L) {
    CRace* race = currentRace;
    int32_t path = Script::GetInt(L, 0);
    uint8_t* racePath = GetPath(race->path);
    int32_t first = Path_IndexOfId(racePath, Script::GetInt(L, 1));
    int32_t last = Path_IndexOfId(racePath, Script::GetInt(L, 2));
    if (kJump) {
        Race_AddJumpPath(race, path, first, last);
    } else {
        Race_AddShortcut(race, path, first, last);
    }
    return 0;
}
RACE45_REVERSED("Native_RaceAddShortcutPath", 0x00713AB0, &Native_RaceAddBranchPath<false>);
RACE45_REVERSED("Native_RaceAddJumpPath", 0x00713B20, &Native_RaceAddBranchPath<true>);

// (two ends' x, y, z)
int __cdecl Native_RaceAddBarrier(lua_State* L) {
    ScriptVector a, b;
    Script::GetVector(&a, L, 0);
    Script::GetVector(&b, L, 3);
    Race_AddBarrier(currentRace, a.x, a.y, a.z, b.x, b.y, b.z);
    return 0;
}
RACE45_REVERSED("Native_RaceAddBarrier", 0x00713C60, &Native_RaceAddBarrier);

// (screen, second track?) -> 0
int __cdecl Native_RaceDisplayTransition(lua_State* L) {
    int screen = Script::GetInt(L, 0);
    uint8_t second = Script::GetBoolOpt(L, 1, 0);
    Script::PushInt(L, Race_DisplayTransition(currentRace, screen, second));
    return 1;
}
RACE45_REVERSED("Native_RaceDisplayTransition", 0x00713CD0, &Native_RaceDisplayTransition);

// (x, y, heading)
int __cdecl Native_RaceAddBoostPoint(lua_State* L) {
    ScriptVector v;
    Script::GetVector(&v, L, 0);
    Race_AddBoostPoint(currentRace, v.x, v.y, v.z);
    return 0;
}
RACE45_REVERSED("Native_RaceAddBoostPoint", 0x00713D50, &Native_RaceAddBoostPoint);

// the radar's player position overridden (its +0xE838)
int __cdecl Native_RaceOverridePlayerPos(lua_State* L) {
    float x = Script::GetFloat(L, 0);
    float y = Script::GetFloat(L, 1);
    float z = Script::GetFloat(L, 2);
    uint8_t* radar = static_cast<uint8_t*>(raceRadar);
    At<float>(radar, 0xE838) = x;
    At<float>(radar, 0xE83C) = y;
    At<float>(radar, 0xE840) = z;
    return 0;
}
RACE45_REVERSED("Native_RaceOverridePlayerPos", 0x00713D90, &Native_RaceOverridePlayerPos);

// (1-based place) -> the ped's handle (its pool slot * 256 + its flags byte), -1 for none
int __cdecl Native_RaceGetIdOfRacerInPosition(lua_State* L) {
    int place = Script::GetInt(L, 0);
    uint8_t* ped = Race_GetRacerInPosition(currentRace, place - 1);
    int32_t handle = -1;
    if (ped != nullptr) {
        CPool<uint8_t>* pool = pedPoolRace;
        int32_t slot = int32_t(ped - pool->objects) / pool->elemSize;
        handle = int32_t(pool->flags[slot]) + slot * 0x100;
    }
    Script::PushInt(L, handle);
    return 1;
}
RACE45_REVERSED("Native_RaceGetIdOfRacerInPosition", 0x00713E20, &Native_RaceGetIdOfRacerInPosition);

} // namespace
