#include "game/GameLogic.h"

#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

using namespace CGameLogicData;

namespace {

inline void* const TheMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const TheLurStore = reinterpret_cast<void*>(0x00D02850);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheZones = reinterpret_cast<void*>(0x020C7E98);   // the zone names
GAME_VAR(CPool<uint8_t>*, ms_pZonePool, 0x020C7C48);
GAME_VAR(uint8_t*, FindPlayerPed_ptr, 0x00C1AEA8);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, ms_clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, ms_bInFrontEnd_20C5BE6, 0x020C5BE6);
GAME_VAR(uint8_t, ms_bPlayerDown_20C87A8, 0x020C87A8);   // set by OnPlayerDown's DEAD/SLEEP message
GAME_VAR(uint8_t, ms_bFlag_20C870A, 0x020C870A);
GAME_VAR(uint8_t, ms_playerFlags_C1AF6C, 0x00C1AF6C);   // (bit 3: no "failed" message)
GAME_VAR(col::CVector, ms_hospitalPos_20C440C, 0x020C440C);
GAME_VAR(col::CVector, ms_restartLookAt_BCC564, 0x00BCC564);
GAME_ARRAY(uint8_t, ms_fadeColour, 0x00C3CD2C);          // TheCamera's fade colour (r, g, b, a)
// the respawn's hand-over to the frame (CGame::ShouldMovePlayer's)
GAME_VAR(uint8_t, ms_bRespawnForced, 0x00BD0FDC);
GAME_VAR(int32_t, ms_moveToPoint, 0x00BD0FD4);
GAME_VAR(float, ms_moveToHeading, 0x00BD0FD8);
GAME_VAR(col::CVector, ms_moveToPos, 0x00BD0FE0);
GAME_VAR(col::CVector, ms_moveToLookAt, 0x00BD0FEC);
GAME_VAR(int32_t, ms_moveToArea, 0x009F4760);
GAME_VAR(float, ms_float_CF1080, 0x00CF1080);
GAME_VAR(int32_t, ms_int_CF107C, 0x00CF107C);
GAME_VAR(uint8_t, ms_bRestarted_CF4434, 0x00CF4434);
GAME_VAR(col::CVector, ms_restartLookAt_CF4438, 0x00CF4438);
GAME_VAR(uint32_t, ms_restartTime_CF4444, 0x00CF4444);
GAME_VAR(uint32_t, ms_timeInMs, 0x00C1A9B4);             // CTimer's
GAME_VAR(uint8_t, ms_bFlag_A45A50, 0x00A45A50);

// ours, through the game
GAME_FN(void, __thiscall, GL_Init, 0x00430640, CGameLogic*);
GAME_FN(uint8_t, __stdcall, GL_FindRestartPoint, 0x00431770, const CRestartPoint* points, int* index);
GAME_FN(void, __thiscall, GL_GetRestartPoint, 0x00431C90, CGameLogic*, int, col::CVector*, float*, int*, col::CVector*);
GAME_FN(void, __thiscall, GL_RestorePlayer, 0x00432320, CGameLogic*, int, col::CVector*);
GAME_FN(uint8_t, __cdecl, VectorsDiffer, 0x004305F0, const col::CVector* a, const col::CVector* b);
GAME_FN(void, __cdecl, DrasticTidyUpMemory, 0x004304E0, int);
GAME_FN(void, __cdecl, SetMovePlayer, 0x0042E710);
GAME_FN(void, __cdecl, SetGameClock, 0x00426070, int hours, int minutes);
GAME_FN(void, __cdecl, LoadingScreen, 0x0043C4F0, const char* title, const char* message, const char*);

// the zones
GAME_FN(int, __thiscall, Zones_FindByName, 0x006D7A10, void* zones, const char* name);
GAME_FN(uint8_t, __thiscall, Zone_IsPointInside, 0x006D6EB0, void* zone, const float* xy);

// the missions and the scripts' callback
GAME_FN(void*, __thiscall, MissionMgr_Fn_6AA790, 0x006AA790, void*);
GAME_FN(uint8_t, __thiscall, Mission_Fn_6AA3F0, 0x006AA3F0, void*);
GAME_FN(void*, __thiscall, MissionMgr_Fn_6AA690, 0x006AA690, void*);
GAME_FN(void*, __thiscall, MissionMgr_Fn_6AA680, 0x006AA680, void*);
GAME_FN(uint8_t, __thiscall, Mission_Fn_6AA300, 0x006AA300, void*, int, int, int, int, int, int, int);
GAME_FN(int, __thiscall, Mission_GetState, 0x00856850, void*);
GAME_FN(void, __cdecl, ShowMessage_7074E0, 0x007074E0, const char* name, int, float, uintptr_t, uintptr_t, int);
GAME_FN(int, __thiscall, ScriptRef_GetFunction, 0x005D80D0, uint32_t* ref);
GAME_FN(int, __thiscall, ScriptRef_GetScript, 0x005D80B0, uint32_t* ref);
GAME_FN(int, __thiscall, LurStore_IsScriptRunning, 0x005DB8F0, void* store, int script);
GAME_FN(void*, __thiscall, ScriptRef_GetState, 0x005D9C20, uint32_t* ref);
GAME_FN(void, __thiscall, ScriptRef_Clear, 0x005D9C50, uint32_t* ref);
GAME_FN(void, __cdecl, Lua_Fn_5D8270, 0x005D8270, void* state);
GAME_FN(void, __cdecl, Lua_Fn_5D8670, 0x005D8670, void* state, int function);
GAME_FN(void, __cdecl, Lua_PushBool, 0x005D85B0, void* state, int value);
GAME_FN(void, __cdecl, Lua_Call, 0x005D82A0, void* state, int args, int results);
GAME_FN(void, __thiscall, Fn_5913D0, 0x005913D0, void*, int, int);
GAME_FN(void, __thiscall, Camera_Fade, 0x004F1AD0, void* camera, float seconds, int direction, int);
GAME_FN(int, __thiscall, Camera_Fn_4F12F0, 0x004F12F0, void* camera);
GAME_FN(void, __thiscall, Camera_Init, 0x004F3720, void* camera);
GAME_FN(uint8_t, __cdecl, Fn_705500, 0x00705500);
GAME_FN(void, __cdecl, Fn_45EC50, 0x0045EC50, const col::CVector* pos, float radius, int);

// the respawn's
GAME_FN(float, __cdecl, FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int);
GAME_FN(float, __thiscall, Ped_GetDistanceFromCentreOfMassToBaseOfModel, 0x00466110, void* ped);
GAME_FN(void, __cdecl, Fn_45B810, 0x0045B810);
GAME_FN(void, __cdecl, Fn_5140C0, 0x005140C0);
GAME_FN(void, __cdecl, Streaming_Fn_5331B0, 0x005331B0);
GAME_FN(void, __cdecl, Streaming_Fn_52F140, 0x0052F140, const col::CVector* pos);
GAME_FN(void, __cdecl, Streaming_RemoveUnusedModelsInLoadedList, 0x00530B50);
GAME_FN(void, __thiscall, Fn_454240, 0x00454240, void*, int area);
GAME_FN(void, __thiscall, Fn_561040, 0x00561040, void*);
GAME_FN(void, __thiscall, Fn_41A6E0, 0x0041A6E0, void*);
GAME_FN(void, __thiscall, Fn_49CD50, 0x0049CD50, void*);
GAME_FN(void, __cdecl, Streaming_Fn_532330, 0x00532330, int area);
GAME_FN(void, __cdecl, Fn_738740, 0x00738740, int);
GAME_FN(void, __thiscall, Ped_Fn_49A840, 0x0049A840, void* ped);
GAME_FN(void, __thiscall, Ped_Fn_49A980, 0x0049A980, void* ped);
GAME_FN(void, __thiscall, Ped_Fn_47D2F0, 0x0047D2F0, void* ped);
GAME_FN(void, __thiscall, Fn_4D9410, 0x004D9410, void*);
GAME_FN(void, __thiscall, Ped_SetHealth, 0x004782E0, void* ped, float health);
GAME_FN(void, __thiscall, Ped_Fn_476180, 0x00476180, void* ped, int);
GAME_FN(void, __thiscall, WorldPlayers_Fn_445DA0, 0x00445DA0, void* players, int);
GAME_FN(void, __thiscall, Ped_Fn_49AB60, 0x0049AB60, void* ped);
GAME_FN(void, __thiscall, Fn_5F40A0, 0x005F40A0, void*);
GAME_FN(void, __thiscall, Fn_4CE960, 0x004CE960, void*);
GAME_FN(void, __thiscall, Ped_PlayAction, 0x004740F0, void* ped, const char* node, const char* file);
GAME_FN(void, __cdecl, Fn_4252D0, 0x004252D0);
GAME_FN(void, __cdecl, Fn_45B820, 0x0045B820);

// (the game's CPool::GetAt, inlined: nullptr for a free slot)
uint8_t* ZoneAt(int index) {
    CPool<uint8_t>* pool = ms_pZonePool;
    if (pool->flags[index] & 0x80) {
        return nullptr;
    }
    return pool->objects + pool->elemSize * index;
}

void* FindZone(const char* name) {
    int index = Zones_FindByName(TheZones, name);
    return index == -1 ? nullptr : ZoneAt(index);
}

bool Same(const col::CVector& a, const col::CVector& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool InHours(int hour, int from, int to) {
    if (from < to) {
        return from <= hour && hour < to;
    }
    return hour >= from || hour < to;
}

// (fld, fmul qword pi, fdiv qword 180: x87, rounded to float at the store)
float ToRadians(float degrees) {
    return float(double(degrees) * 3.1415927410125732 / 180.0);
}

const col::CVector* PedPosition(uint8_t* ped) {
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14);
    return reinterpret_cast<const col::CVector*>(matrix ? matrix + 0x30 : ped + 4);
}

void SetPoint(CRestartPoint& p, const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
              const col::CVector* altPos, float altHeading, int altArea) {
    p.pos = *pos;
    p.heading = heading;
    p.area = area;
    p.hourFrom = hourFrom;
    p.hourTo = hourTo;
    p.altPos = *altPos;
    p.altHeading = altHeading;
    p.altArea = altArea;
    p.lookAt = ms_unusedPoint;
}

// the first unused point of a list (nullptr: full)
CRestartPoint* FreePoint(CRestartPoint* list) {
    for (int i = 0; i < 60; i++) {
        if (Same(list[i].pos, ms_unusedPoint)) {
            return &list[i];
        }
    }
    return nullptr;
}

CRestartPoint* PointAt(CRestartPoint* list, const col::CVector* pos) {
    for (int i = 0; i < 60; i++) {
        if (Same(list[i].pos, *pos)) {
            return &list[i];
        }
    }
    return nullptr;
}

void ClearPoint(CRestartPoint& p) {
    p.pos = ms_unusedPoint;
    p.heading = 0.0f;
    p.area = -1;
    p.hourFrom = 0;
    p.hourTo = 24;
    p.altPos = ms_unusedPoint;
    p.altHeading = 0.0f;
    p.altArea = -1;
}

void SetLookAt(CRestartPoint* list, const col::CVector* pos, const col::CVector* lookAt) {
    for (int i = 0; i < 60; i++) {
        CRestartPoint& p = list[i];
        if ((p.altArea == -1 && Same(p.pos, *pos)) || Same(p.altPos, *pos)) {
            p.lookAt = *lookAt;
        }
    }
}

// the script's callback for the restart, called with `value`
void RunScriptCallback(uint32_t* ref, int value) {
    int function = ScriptRef_GetFunction(ref);
    if (function == -1) {
        return;
    }
    if (LurStore_IsScriptRunning(TheLurStore, ScriptRef_GetScript(ref))) {
        void* state = ScriptRef_GetState(ref);
        Lua_Fn_5D8270(state);
        Lua_Fn_5D8670(state, function);
        Lua_PushBool(state, value);
        Lua_Call(state, 1, 0);
    } else {
        ScriptRef_Clear(ref);
    }
}

} // namespace

CGameLogic* CGameLogic::Construct() {
    for (int list = 0; list < 3; list++) {
        for (CRestartPoint& p : points[list]) {
            p.pos = {0.0f, 0.0f, 0.0f};
            p.altPos = {0.0f, 0.0f, 0.0f};
            p.lookAt = {0.0f, 0.0f, 0.0f};
        }
    }
    for (CRestartPoint& p : defaults) {
        p.pos = {0.0f, 0.0f, 0.0f};
        p.altPos = {0.0f, 0.0f, 0.0f};
        p.lookAt = {0.0f, 0.0f, 0.0f};
    }
    scriptCallback = 0;
    override1.pos = {0.0f, 0.0f, 0.0f};
    override0.pos = {0.0f, 0.0f, 0.0f};
    GL_Init(this);
    return this;
}

void CGameLogic::Init() {
    for (int i = 0; i < 60; i++) {
        for (int list = 0; list < 3; list++) {
            CRestartPoint& p = points[list][i];
            p.fromArea = -1;
            ClearPoint(p);   // (the look-at left as it was)
        }
    }
    ClearPoint(defaults[0]);   // (fromArea and the look-at left as they were)
    defaults[2] = defaults[0];
    defaults[1] = defaults[0];
    ScriptRef_Clear(&scriptCallback);
    groundPending = 0;
    fading = 0;
    clockSet = 0;
}

void CGameLogic::AddPoint0(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                           const col::CVector* altPos, float altHeading, int altArea) {
    if (CRestartPoint* p = FreePoint(points[0])) {
        p->fromArea = -1;
        SetPoint(*p, pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
    }
}

void CGameLogic::AddPoint0InArea(int fromArea, const col::CVector* pos, float heading, int area, int hourFrom,
                                 int hourTo, const col::CVector* altPos, float altHeading, int altArea) {
    if (CRestartPoint* p = FreePoint(points[0])) {
        p->fromArea = fromArea;
        SetPoint(*p, pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
    }
}

void CGameLogic::RemovePoint0(const col::CVector* pos) {
    if (CRestartPoint* p = PointAt(points[0], pos)) {
        p->fromArea = -1;
        ClearPoint(*p);
        p->lookAt = ms_unusedPoint;
    }
}

void CGameLogic::AddPoint1(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                           const col::CVector* altPos, float altHeading, int altArea) {
    if (CRestartPoint* p = FreePoint(points[1])) {
        SetPoint(*p, pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
    }
}

void CGameLogic::SetLookAt0(const col::CVector* pos, const col::CVector* lookAt) {
    SetLookAt(points[0], pos, lookAt);
}

void CGameLogic::SetLookAt1(const col::CVector* pos, const col::CVector* lookAt) {
    SetLookAt(points[1], pos, lookAt);
}

void CGameLogic::SetLookAt2(const col::CVector* pos, const col::CVector* lookAt) {
    SetLookAt(points[2], pos, lookAt);
}

void CGameLogic::SetDefaultLookAt0(const col::CVector* lookAt) {
    defaults[0].lookAt = *lookAt;
}

void CGameLogic::SetDefaultLookAt1(const col::CVector* lookAt) {
    defaults[1].lookAt = *lookAt;
}

void CGameLogic::SetDefaultLookAt2(const col::CVector* lookAt) {
    defaults[2].lookAt = *lookAt;
}

void CGameLogic::AddPoint1InArea(int fromArea, const col::CVector* pos, float heading, int area, int hourFrom,
                                 int hourTo, const col::CVector* altPos, float altHeading, int altArea) {
    if (CRestartPoint* p = FreePoint(points[1])) {
        p->fromArea = fromArea;
        SetPoint(*p, pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
    }
}

void CGameLogic::RemovePoint1(const col::CVector* pos) {
    if (CRestartPoint* p = PointAt(points[1], pos)) {
        ClearPoint(*p);   // (fromArea left as it was)
        p->lookAt = ms_unusedPoint;
    }
}

void CGameLogic::AddPoint2InArea(int fromArea, const col::CVector* pos, float heading, int area, int hourFrom,
                                 int hourTo, const col::CVector* altPos, float altHeading, int altArea) {
    if (CRestartPoint* p = FreePoint(points[2])) {
        p->fromArea = fromArea;
        SetPoint(*p, pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
    }
}

void CGameLogic::SetDefault0(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                             const col::CVector* altPos, float altHeading, int altArea) {
    defaults[0].fromArea = -1;
    SetPoint(defaults[0], pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
}

void CGameLogic::SetDefault1(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                             const col::CVector* altPos, float altHeading, int altArea) {
    SetPoint(defaults[1], pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
}

void CGameLogic::SetDefault2(const col::CVector* pos, float heading, int area, int hourFrom, int hourTo,
                             const col::CVector* altPos, float altHeading, int altArea) {
    SetPoint(defaults[2], pos, heading, area, hourFrom, hourTo, altPos, altHeading, altArea);
}

void CGameLogic::OnPlayerDown(int knockedOut, int flag2, int asleep) {
    if (!re::Flag(knockedOut) && re::Flag(flag2) && !Mission_Fn_6AA3F0(MissionMgr_Fn_6AA790(TheMissionMgr))) {
        ShowMessage_7074E0(re::Flag(asleep) ? "SLEEP" : "DEAD", 0, 0.0f, 0, 0, 0);
        ms_bPlayerDown_20C87A8 = 1;
    }
    if (Mission_Fn_6AA3F0(MissionMgr_Fn_6AA790(TheMissionMgr))) {
        uint8_t first = Mission_Fn_6AA300(MissionMgr_Fn_6AA690(TheMissionMgr), 0, knockedOut, flag2, 1, 0, 0, asleep);
        int showFailed = ((ms_playerFlags_C1AF6C >> 3) & 1) ^ 1;
        uint8_t second =
            Mission_Fn_6AA300(MissionMgr_Fn_6AA680(TheMissionMgr), 0, knockedOut, flag2, 1, showFailed, 0, asleep);
        RunScriptCallback(&scriptCallback, knockedOut);
        if (first || second) {
            return;
        }
        if (Mission_GetState(MissionMgr_Fn_6AA690(TheMissionMgr)) != 0xC &&
            Mission_GetState(MissionMgr_Fn_6AA680(TheMissionMgr)) != 0xC) {
            return;
        }
    } else {
        ms_restartReason = uint8_t(re::Flag(knockedOut) ? 2 : 1);
    }
    fading = 1;
    ms_fadeColour[0] = 0;
    ms_fadeColour[1] = 0;
    ms_fadeColour[2] = 0;
    ms_fadeColour[3] = 0xFF;
    Camera_Fade(TheCamera, 1.0f, 1, 0);
}

void CGameLogic::SetOverride1(const col::CVector* pos, float heading, int area) {
    override1.active = 1;
    override1.pos = *pos;
    override1.heading = heading;
    override1.area = area;
}

void CGameLogic::SetOverride0(const col::CVector* pos, float heading, int area) {
    override0.active = 1;
    override0.pos = *pos;
    override0.heading = heading;
    override0.area = area;
}

void CGameLogic::ClearOverrides() {
    override1.active = 0;
    override0.active = 0;
}

uint8_t __stdcall FindRestartPoint(const CRestartPoint* points, int* index) {
    void* carnival = FindZone("ZoneCarnival");
    void* business = FindZone("ZoneBusiness");
    void* industrial = FindZone("ZoneIndustrial");
    void* poor = FindZone("ZonePoor");
    void* rich = FindZone("ZoneRich");
    void* school = FindZone("ZoneSchool");
    const col::CVector player = *PedPosition(FindPlayerPed_ptr);
    int hour = ms_clockHours;
    // a point for the area the player is in
    for (int i = 0; i < 60; i++) {
        const CRestartPoint& p = points[i];
        if (!Same(p.pos, ms_unusedPoint) && p.fromArea != -1 && InHours(hour, p.hourFrom, p.hourTo) &&
            p.fromArea == currArea) {
            *index = i;
            return 1;
        }
    }
    // else one in the zone he's in (the points' own z isn't looked at)
    for (int i = 0; i < 60; i++) {
        const CRestartPoint& p = points[i];
        if (Same(p.pos, ms_unusedPoint) || p.fromArea != -1 || !InHours(hour, p.hourFrom, p.hourTo)) {
            continue;
        }
        float at[2] = {player.x, player.y};
        float here[2] = {p.pos.x, p.pos.y};
        if (Zone_IsPointInside(carnival, at) && Zone_IsPointInside(carnival, here)) {
            *index = i;
            return 1;
        }
        if (Zone_IsPointInside(business, at) && Zone_IsPointInside(business, here)) {
            *index = i;
            return 1;
        }
        if (Zone_IsPointInside(industrial, at) && Zone_IsPointInside(industrial, here)) {
            *index = i;
            return 1;
        }
        if (Zone_IsPointInside(poor, at) && Zone_IsPointInside(poor, here)) {
            *index = i;
            return 1;
        }
        if (Zone_IsPointInside(rich, at) && !Zone_IsPointInside(carnival, at) && Zone_IsPointInside(rich, here) &&
            !Zone_IsPointInside(carnival, here)) {
            *index = i;
            return 1;
        }
        if (Zone_IsPointInside(school, at) && Zone_IsPointInside(school, here)) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

void __fastcall SetRestartReason(void*, re::Edx, int flag4, int flag5, int flag6) {
    if (re::Flag(flag4)) {
        ms_restartReason = 4;
    } else if (re::Flag(flag6)) {
        ms_restartReason = 6;
    } else {
        ms_restartReason = uint8_t(re::Flag(flag5) ? 5 : 3);
    }
}

void CGameLogic::GetRestartPoint(int forced, col::CVector* pos, float* heading, int* area, col::CVector* lookAt) {
    uint8_t* ped = FindPlayerPed_ptr;
    int reason = ms_restartReason;
    if (re::Flag(forced)) {
        reason = 3;
    }
    *area = currArea;
    *heading = *reinterpret_cast<float*>(ped + 0x1700);
    *pos = *PedPosition(ped);
    *lookAt = ms_unusedPoint;
    if (ped[0x16CC] || *reinterpret_cast<int32_t*>(ped + 0x13DC)) {
        const CRestartPoint& d = defaults[0];
        *area = d.area;
        *heading = ToRadians(d.heading);
        *pos = d.pos;
        *lookAt = d.lookAt;
        if (*reinterpret_cast<int32_t*>(ped + 0x13DC)) {
            reason = 1;
        }
    }
    int list;
    const CRestartOverride* override;
    switch (reason) {
    case 1: case 5: case 6:
        list = 0;
        override = &override0;
        break;
    case 2: case 4:
        list = 1;
        override = &override1;
        break;
    case 3:
        *area = currArea;
        *heading = *reinterpret_cast<float*>(ped + 0x1700);
        *pos = *PedPosition(ped);
        return;
    case 7:
        list = 2;
        override = nullptr;
        break;
    default:
        return;
    }
    if (override && override->active) {
        *pos = override->pos;
        *heading = override->heading;
        *area = override->area;
        return;
    }
    int index = 60;
    GL_FindRestartPoint(points[list], &index);
    if (index < 60) {
        const CRestartPoint& p = points[list][index];
        if (p.altArea == -1) {
            *area = p.area;
            *heading = ToRadians(p.heading);
            *pos = p.pos;
        } else {
            *area = p.altArea;
            *heading = ToRadians(p.altHeading);
            *pos = p.altPos;
        }
        *lookAt = p.lookAt;
        return;
    }
    const CRestartPoint& d = defaults[list];
    if (VectorsDiffer(&d.pos, &ms_unusedPoint)) {
        *area = d.area;
        *heading = ToRadians(d.heading);
        *pos = d.pos;
        *lookAt = d.lookAt;
        return;
    }
    if (list != 2) {
        return;
    }
    // (list 2 without a point: the hospital, or where he stands on the ground)
    if (ped[0x1E91] && *reinterpret_cast<int32_t*>(ped + 0x13DC) == 0) {
        *pos = ms_hospitalPos_20C440C;
        return;
    }
    if (ped[0x16CC] == 0 && *reinterpret_cast<int32_t*>(ped + 0x13DC) == 0) {
        groundPending = 1;
        float z = float(double(pos->z) + 1.0);
        uint8_t found = 0;
        float ground = FindGroundZ(pos->x, pos->y, z, &found, 1);
        if (found) {
            pos->z = ground;
        }
        pos->z = Ped_GetDistanceFromCentreOfMassToBaseOfModel(FindPlayerPed_ptr) + pos->z;
    }
}

void CGameLogic::RestorePlayer(int forced, col::CVector* lookAt) {
    Fn_45B810();
    Fn_5140C0();
    uint8_t* ped = FindPlayerPed_ptr;
    col::CVector pos = {0.0f, 0.0f, 0.0f};
    float heading;
    int area;
    GL_GetRestartPoint(this, forced, &pos, &heading, &area, lookAt);
    col::CVector at = pos;
    ms_bRespawnForced = uint8_t(forced);
    Streaming_Fn_5331B0();
    Streaming_Fn_52F140(&at);
    Streaming_RemoveUnusedModelsInLoadedList();
    LoadingScreen("SortOutStreamingAndMemory(const CVector posn)", "CGame::DrasticTidyUpMemory()", nullptr);
    DrasticTidyUpMemory(1);
    Fn_454240(reinterpret_cast<void*>(0x00C13FD8), area);
    Fn_561040(reinterpret_cast<void*>(0x00CF1F60));
    Fn_41A6E0(*reinterpret_cast<re::Ptr32<void>*>(0x00BCD080));
    Fn_49CD50(*reinterpret_cast<re::Ptr32<void>*>(0x00C2C108));
    Streaming_Fn_532330(currArea);
    void* vehicle = *reinterpret_cast<re::Ptr32<void>*>(ped + 0x1554);
    Fn_738740(0);
    ms_float_CF1080 = 0.0f;
    uint8_t reason = ms_restartReason;
    bool wakeUp = reason == 7;
    ms_int_CF107C = 0;
    bool died = reason == 2 || reason == 4;
    bool busted = reason == 1 || reason == 5 || reason == 6;
    if (reason != 3 && reason != 5 && reason != 6) {
        Ped_Fn_49A840(ped);
        reason = ms_restartReason;
    }
    if ((reason == 7 || reason == 6) && uint8_t(ms_clockHours - 2) <= 5) {
        SetGameClock(8, 0);
    }
    if (died) {
        Ped_Fn_49A980(ped);
    }
    float health = *reinterpret_cast<float*>(ped + 0x1CB8);
    ped[0x1E91] = 0;
    Ped_Fn_47D2F0(ped);
    if (ms_restartReason == 4) {
        Fn_4D9410(*reinterpret_cast<re::Ptr32<void>*>(ped + 0x1D0));
    }
    ms_restartReason = 0;
    Ped_SetHealth(ped, *reinterpret_cast<float*>(ped + 0x1CA4));
    if (health > 0.0f && !re::Flag(forced) && !busted && !died && !wakeUp) {
        Ped_SetHealth(ped, health);
    }
    *reinterpret_cast<int32_t*>(ped + 0x48) = 1;
    *reinterpret_cast<int32_t*>(ped + 0x18B8) = 0;
    Ped_Fn_476180(ped, 0);
    WorldPlayers_Fn_445DA0(reinterpret_cast<void*>(0x00C1AEA8), 0);
    *reinterpret_cast<int32_t*>(ped + 0x78) = 0;
    Ped_Fn_49AB60(ped);
    Fn_5F40A0(*reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x2E0) + 0x258);
    *reinterpret_cast<int32_t*>(ped + 0x13DC) = 0;
    reinterpret_cast<float*>(ped + 0x124)[0] = 0.0f;
    reinterpret_cast<float*>(ped + 0x124)[1] = 0.0f;
    reinterpret_cast<float*>(ped + 0x124)[2] = 0.0f;
    *reinterpret_cast<int32_t*>(ped + 0x14C4) = 0;
    if (!groundPending) {
        ms_moveToPos = pos;
        ms_moveToPoint = 1;
        ms_moveToLookAt = *lookAt;
        ms_moveToHeading = heading;
        ms_moveToArea = area;
        SetMovePlayer();
    } else {
        ms_moveToLookAt = ms_restartLookAt_BCC564;
    }
    groundPending = 0;
    if (vehicle) {
        Fn_4CE960(static_cast<uint8_t*>(vehicle) + 0x1C4);
        (*reinterpret_cast<re::Ptr32<re::Ptr32<void __thiscall(void*, int)>>*>(vehicle))[0](vehicle, 1);   // (deleting destructor)
    }
    if (wakeUp) {
        Ped_PlayAction(ped, "/Global/Ambient/MissionSpec/WakeUp/WakeUp_Anywhere", "Act/Anim/Ambient.act");
    }
    Fn_4252D0();
    ms_bRestarted_CF4434 = 1;
    ms_restartTime_CF4444 = ms_timeInMs;
    ms_restartLookAt_CF4438 = *lookAt;
    ms_bFlag_A45A50 = 1;
    Fn_45B820();
}

void CGameLogic::Process() {
    if (ms_bInFrontEnd_20C5BE6) {
        return;
    }
    if (fading == 1 && Camera_Fn_4F12F0(TheCamera) != 1) {
        return;
    }
    if (Fn_705500() && !ms_bPlayerDown_20C87A8 && !ms_bFlag_20C870A) {
        return;
    }
    bool restart = false;
    uint8_t reason = ms_restartReason;
    if (reason == 1 || reason == 7 || reason == 2) {
        int function = ScriptRef_GetFunction(&scriptCallback);
        Fn_5913D0(reinterpret_cast<void*>(0x00CF42D4), 0, 0);
        if (function != -1) {
            if (LurStore_IsScriptRunning(TheLurStore, ScriptRef_GetScript(&scriptCallback))) {
                void* state = ScriptRef_GetState(&scriptCallback);
                Lua_Fn_5D8270(state);
                Lua_Fn_5D8670(state, function);
                Lua_PushBool(state, reason == 2 ? 1 : 0);
                Lua_Call(state, 1, 0);
            } else {
                ScriptRef_Clear(&scriptCallback);
            }
        }
        restart = true;
    }
    if (!clockSet) {
        if (ms_restartReason != 0) {
            if (uint8_t(ms_clockHours - 2) <= 5) {
                SetGameClock(8, 0);
            }
            clockSet = 1;
        }
    } else if (ms_restartReason == 0) {
        clockSet = 0;
    }
    if (!restart) {
        return;
    }
    fading = 0;
    Fn_45EC50(&ms_restartLookAt_BCC564, 4000.0f, 1);
    col::CVector none = {0.0f, 0.0f, 0.0f};
    GL_RestorePlayer(this, 0, &none);
    Camera_Init(TheCamera);
}

// ---- registration ----------------------------------------------------------------------

#define GL_REVERSED(fn, address)                                                                  \
    static void* Original_##fn;                                                                   \
    REVERSED("CGameLogic::" #fn, address, re::MemberAddress(&CGameLogic::fn), nullptr, &Original_##fn, \
             re::Mode::Original)

GL_REVERSED(Construct, 0x004315C0);
GL_REVERSED(Init, 0x00430640);
GL_REVERSED(AddPoint0, 0x00430840);
GL_REVERSED(AddPoint0InArea, 0x00430920);
GL_REVERSED(RemovePoint0, 0x00430A00);
GL_REVERSED(AddPoint1, 0x00430AD0);
GL_REVERSED(SetLookAt0, 0x00430BE0);
GL_REVERSED(SetLookAt1, 0x00430C70);
GL_REVERSED(SetLookAt2, 0x00430D00);
GL_REVERSED(SetDefaultLookAt0, 0x00430D90);
GL_REVERSED(SetDefaultLookAt1, 0x00430DC0);
GL_REVERSED(SetDefaultLookAt2, 0x00430DF0);
GL_REVERSED(AddPoint1InArea, 0x00430E20);
GL_REVERSED(RemovePoint1, 0x00430F30);
GL_REVERSED(AddPoint2InArea, 0x00431030);
GL_REVERSED(SetDefault0, 0x00431140);
GL_REVERSED(SetDefault1, 0x004311F0);
GL_REVERSED(SetDefault2, 0x00431290);
GL_REVERSED(OnPlayerDown, 0x00431330);
GL_REVERSED(SetOverride1, 0x00431530);
GL_REVERSED(SetOverride0, 0x00431570);
GL_REVERSED(ClearOverrides, 0x004315B0);
GL_REVERSED(GetRestartPoint, 0x00431C90);
GL_REVERSED(RestorePlayer, 0x00432320);
GL_REVERSED(Process, 0x00432690);

static void* Original_FindRestartPoint;
REVERSED("CGameLogic::FindRestartPoint", 0x00431770, RE_BODY(&FindRestartPoint), nullptr,
         &Original_FindRestartPoint, re::Mode::Original);
static void* Original_SetRestartReason;
REVERSED("CGameLogic::SetRestartReason", 0x004314F0, RE_BODY(&SetRestartReason), nullptr,
         &Original_SetRestartReason, re::Mode::Original);
