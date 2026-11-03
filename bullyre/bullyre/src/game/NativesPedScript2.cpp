// Script natives 0x5C7AF0..0x5C8FE0 for peds: wandering, following, fleeing on bikes,
// moving to points and entities, anchors and positions, area and facing tests, being
// hit, vehicles, health. Each is `int __cdecl Native_X(lua_State* L)`, returning how
// many results it pushed. Objectives go to the ped's objective manager (+0x5F0).

#include <cstdint>

#include "game/NativesMPP.h"

namespace {

GAME_FN(void*, __cdecl, P2_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(uint8_t, __thiscall, P2_Objectives_Add, 0x00471390, void* manager, void* objective);
GAME_FN(uint8_t, __thiscall, Objectives_Prepend, 0x00471470, void* manager, void* objective);
GAME_FN(void, __thiscall, P2_Objectives_Clear, 0x00470350, void* manager);
GAME_FN(void, __thiscall, P2_Objectives_SetDefault, 0x00470300, void* manager, void* objective);
GAME_FN(float, __cdecl, P2_GetSpeedArgOpt, 0x005C62E0, lua_State* L, int i, float def);
GAME_FN(void*, __thiscall, WanderObjective_Construct, 0x0046FC80, void* self, void* ped, float speed);
GAME_FN(void, __thiscall, Scanner_ClearFocus, 0x0048C4C0, void* scanner, void* ped);
GAME_FN(void*, __thiscall, FollowFocusObjective_Construct, 0x00471C10, void* self, void* ped, void* focus);
GAME_FN(uint8_t*, __thiscall, Scanner_GetThreat, 0x0048C070, void* scanner);
GAME_FN(uint8_t*, __thiscall, FleeOnPathOnBikeObjective_Construct, 0x00470FC0, void* self, void* ped, void* from);
GAME_FN(void*, __thiscall, FleeOnRoadOnBikeObjective_Construct, 0x00471080, void* self, void* ped, void* from);
GAME_VAR(void*, p2_patrolPathPool, 0x00D13CB4);
GAME_FN(void*, __thiscall, PatrolPath_Get, 0x005ED3A0, void* pool, int id);
GAME_FN(float, __cdecl, P2_FindGroundZ2D, 0x004634A0, float x, float y, int);
GAME_FN(float, __cdecl, FindGroundZFor3DCoord, 0x004635A0, float x, float y, float z, uint8_t* found, int flag);
GAME_FN(uint8_t*, __thiscall, MoveToPosObjective_Construct, 0x004705B0, void* self, void* ped, const float* target,
        float speed, float radius, int stopWhenThere, int flag18);
GAME_FN(uint8_t*, __thiscall, MoveToEntityObjective_Construct, 0x004708F0, void* self, void* ped, void* target,
        float speed, float radius, int stopWhenThere, int flag18);
GAME_FN(void*, __thiscall, FollowEntityObjective_Construct, 0x00470AA0, void* self, void* ped, void* target, float value);
GAME_FN(void, __cdecl, P2_StoreCallbackOpt, 0x005DA060, lua_State* L, int i, void* callback);
GAME_FN(uint8_t*, __cdecl, P2_GetAreaEntity, 0x005C28D0, int handle, int kind, int noVehicle, int cutscene);
GAME_FN(float*, __cdecl, P2_GetAreaEntityPos, 0x005C2970, float* out, int handle, int kind);
GAME_FN(uint8_t, __cdecl, P2_IsEntityInArea, 0x005C2A70, int is3D, int handle, int kind, float x, float y, float z,
        float rx, float ry, float rz, int a10, void* rotation, int a12, int a13, int a14, int a15, int a16);
GAME_FN(uint8_t*, __thiscall, Ped_GetMatrix, 0x004734D0, void* ped);
GAME_FN(void, __thiscall, Matrix_SetTranslate, 0x004127C0, void* m, const float* v);
GAME_FN(void, __thiscall, P2_Matrix_SetRotateZ, 0x00412C20, void* m, float angle);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, void* m);
inline void* const p2_routes = reinterpret_cast<void*>(0x00BC5E7C);
GAME_FN(void*, __thiscall, P2_Routes_Get, 0x006D6450, void* routes, int index);
GAME_FN(float*, __thiscall, P2_Route_First, 0x006D59B0, void* route);
GAME_FN(float*, __thiscall, P2_Route_Next, 0x006D59F0, void* route);
GAME_FN(void, __cdecl, P2_WarpPed, 0x005C7600, void* ped, const float* pos);
GAME_FN(void, __thiscall, P2_Ped_SetHeading, 0x00475730, void* ped, float heading);
GAME_FN(uint8_t*, __cdecl, P2_GetVehicle, 0x0044A580, int handle);
GAME_FN(uint8_t, __cdecl, P2_IsPointInFront, 0x005C65C0, void* entity, float fov, float x, float y, float z);
GAME_FN(uint8_t*, __cdecl, FindVehicleTarget, 0x004D33F0, void* ped, float range, int, int, int);
GAME_FN(void, __cdecl, EntityRef_Add, 0x004657D0, void* entity, void* ref);
GAME_FN(void*, __thiscall, EnterVehicleObjective_Construct, 0x0046FBA0, void* self, void* ped, void* vehicle);
GAME_FN(void*, __thiscall, ExitVehicleObjective_Construct, 0x0046FC40, void* self, void* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsInVehicle, 0x004CE690, void* ped);
GAME_FN(int, __cdecl, GetVehicleRef, 0x0044CAC0, void* vehicle);
GAME_FN(void, __thiscall, Ped_SetHealth, 0x004782E0, void* ped, float health);

uint8_t* P2_Ped(lua_State* L, int i) {
    return M_GetPedFromHandle(Script::GetInt(L, i), 2);
}

// (ped, [speed]): wanders (its default objective)
int __cdecl Native_PedWander(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped != nullptr) {
        float speed = P2_GetSpeedArgOpt(L, 1, 0.30000001f);
        void* memory = P2_Malloc(0x14);
        void* objective = memory != nullptr ? WanderObjective_Construct(memory, ped, speed) : nullptr;
        P2_Objectives_SetDefault(ped + 0x5F0, objective);
    }
    return 0;
}

// (ped, focus ped): follows it (its default objective; its scanner's focus cleared)
int __cdecl Native_PedFollowFocus(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    uint8_t* focus = P2_Ped(L, 1);
    if (ped != nullptr && focus != nullptr) {
        Scanner_ClearFocus(ped + 0x3BC, focus);
        void* memory = P2_Malloc(0x14);
        void* objective = memory != nullptr ? FollowFocusObjective_Construct(memory, ped, focus) : nullptr;
        P2_Objectives_SetDefault(ped + 0x5F0, objective);
    }
    return 0;
}

// (ped, path, point, [a = 0]): flees its threat along the path. (A failed allocation is
// written through anyway)
int __cdecl Native_PedFleeOnPathOnBike(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int path = Script::GetInt(L, 1);
    int point = Script::GetInt(L, 2);
    int a = Script::GetIntOpt(L, 3, 0);
    P2_Objectives_Clear(ped + 0x5F0);
    uint8_t* threat = Scanner_GetThreat(ped + 0x3BC);
    void* memory = P2_Malloc(0x40);
    uint8_t* objective = memory != nullptr ? FleeOnPathOnBikeObjective_Construct(memory, ped, threat) : nullptr;
    P2_Objectives_Add(ped + 0x5F0, objective);
    At<int32_t>(objective, 0x10) = path;
    At<int32_t>(objective, 0x14) = a;
    if (PatrolPath_Get(p2_patrolPathPool, path) != nullptr) {
        At<int32_t>(objective, 0x24) = point;
    }
    return 0;
}

// (ped, [from ped = its threat])
int __cdecl Native_PedFleeOnRoadOnBike(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    int fromHandle = Script::GetIntOpt(L, 1, -1);
    uint8_t* ped = M_GetPedFromHandle(handle, 2);
    if (ped == nullptr) {
        return 0;
    }
    P2_Objectives_Clear(ped + 0x5F0);
    uint8_t* from = nullptr;
    if (fromHandle != -1) {
        from = M_GetPedFromHandle(fromHandle, 2);
    }
    if (from == nullptr) {
        from = Scanner_GetThreat(ped + 0x3BC);
    }
    void* memory = P2_Malloc(0x14);
    void* objective = memory != nullptr ? FleeOnRoadOnBikeObjective_Construct(memory, ped, from) : nullptr;
    P2_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, speed, x, y, [z: the ground], [radius = 0.3], [stop], [flag])
int __cdecl Native_PedMoveToXYZ(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    float x = Script::GetFloat(L, 2);
    float y = Script::GetFloat(L, 3);
    float speed = P2_GetSpeedArgOpt(L, 1, 0.69999999f);
    float z;
    if (Script::GetArgCount(L) > 4 && Script::IsNumber(L, 4) != 0) {
        z = Script::GetFloat(L, 4);
    } else {
        z = P2_FindGroundZ2D(x, y, 1);
    }
    float target[3] = {x, y, z};
    float radius = Script::GetFloatOpt(L, 5, 0.30000001f);
    uint8_t stop = Script::GetBoolOpt(L, 6, 0);
    uint8_t flag = Script::GetBoolOpt(L, 7, 0);
    void* memory = P2_Malloc(0x34);
    if (memory == nullptr) {
        P2_Objectives_Add(ped + 0x5F0, nullptr);
        return 0;
    }
    P2_Objectives_Add(ped + 0x5F0, MoveToPosObjective_Construct(memory, ped, target, speed, radius, stop, flag));
    return 0;
}

// (ped, speed, route, [callback], [point = 1], [radius = 0.3], [stop], [flag]): to the
// route's point (on the ground below it when there is some)
int __cdecl Native_PedMoveToPoint(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int n = Script::GetIntOpt(L, 3, 1);
    void* route = P2_Routes_Get(p2_routes, uint16_t(Script::GetInt(L, 2)));
    float* p = P2_Route_First(route);
    for (int i = 1; i < n;) {
        float* next = P2_Route_Next(route);
        if (next == nullptr) {
            break;
        }
        i++;
        p = next;
    }
    float pos[3] = {p[0], p[1], p[2]};
    float speed;
    switch (Script::GetIntOpt(L, 1, -1)) {
    case 0:
        speed = 0.30000001f;
        break;
    case 2:
        speed = 1.1f;
        break;
    case 3:
        speed = 1.5f;
        break;
    case 4:
        speed = -1.0f;
        break;
    default:
        speed = 0.69999999f;
        break;
    }
    uint8_t found = 0;
    float z = FindGroundZFor3DCoord(pos[0], pos[1], pos[2], &found, 1);
    if (found == 0) {
        z = pos[2];
    }
    float target[3] = {pos[0], pos[1], z};
    float radius = Script::GetFloatOpt(L, 5, 0.30000001f);
    uint8_t stop = Script::GetBoolOpt(L, 6, 0);
    uint8_t flag = Script::GetBoolOpt(L, 7, 0);
    void* memory = P2_Malloc(0x34);
    uint8_t* objective = memory != nullptr ? MoveToPosObjective_Construct(memory, ped, target, speed, radius, stop, flag)
                                           : nullptr;
    At<float>(objective, 0x10) = speed;
    P2_Objectives_Add(ped + 0x5F0, objective);
    P2_StoreCallbackOpt(L, 4, objective + 0x1C);
    return 0;
}

// (ped, handle, kind, speed, [callback], [radius = 0.3], [stop], [flag])
int __cdecl Native_PedMoveToObject(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int handle = Script::GetInt(L, 1);
    int kind = Script::GetInt(L, 2);
    float speed = P2_GetSpeedArgOpt(L, 3, 0.69999999f);
    uint8_t* target = P2_GetAreaEntity(handle, kind, 0, 0);
    float radius = Script::GetFloatOpt(L, 5, 0.30000001f);
    uint8_t stop = Script::GetBoolOpt(L, 6, 0);
    uint8_t flag = Script::GetBoolOpt(L, 7, 0);
    void* memory = P2_Malloc(0x2C);
    uint8_t* objective = memory != nullptr ? MoveToEntityObjective_Construct(memory, ped, target, speed, radius, stop,
                                                                              flag)
                                           : nullptr;
    At<float>(objective, 0x10) = speed;
    P2_Objectives_Add(ped + 0x5F0, objective);
    P2_StoreCallbackOpt(L, 4, objective + 0x1C);
    return 0;
}

// (ped, handle, kind, value)
int __cdecl Native_PedFollowObject(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int handle = Script::GetInt(L, 1);
    int kind = Script::GetInt(L, 2);
    float value = Script::GetFloat(L, 3);
    uint8_t* target = P2_GetAreaEntity(handle, kind, 0, 0);
    void* memory = P2_Malloc(0x18);
    void* objective = memory != nullptr ? FollowEntityObjective_Construct(memory, ped, target, value) : nullptr;
    P2_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped, x, y, z, heading): its matrix set directly
int __cdecl Native_PedSetWorldAnchor(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    uint8_t* matrix = Ped_GetMatrix(ped);
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float pos[3] = {x, y, z};
    Matrix_SetTranslate(matrix, pos);
    P2_Matrix_SetRotateZ(matrix, Script::GetFloat(L, 4));
    Matrix_UpdateRW(matrix);
    return 0;
}

// (ped, route, [point]): warped to the route's point, facing its heading - 90
int __cdecl Native_PedSetPosPoint(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int args = lua::gettop(L);
    int n = 0;   // (uninitialised in the game when not given; unused then)
    if (args == 3) {
        n = Script::GetInt(L, 2);
    }
    void* route = P2_Routes_Get(p2_routes, Script::GetInt(L, 1));
    if (route == nullptr) {
        return 0;
    }
    float* p = P2_Route_First(route);
    if (args == 3) {
        for (int i = 1; i < n;) {
            float* next = P2_Route_Next(route);
            if (next == nullptr) {
                break;
            }
            i++;
            p = next;
        }
    }
    if (p == nullptr) {
        return 0;
    }
    float heading = float((double(p[7]) - 90.0) * 3.1415927410125732 / 180.0);
    float pos[3] = {p[0], p[1], p[2]};
    P2_WarpPed(ped, pos);
    P2_Ped_SetHeading(ped, heading);
    return 0;
}

// (ped) -> x, y, z (its vehicle's when in one; 0s for none)
int __cdecl Native_PedGetPosXYZ(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        Script::PushFloat(L, 0.0f);
        Script::PushFloat(L, 0.0f);
        Script::PushFloat(L, 0.0f);
        return 3;
    }
    M_Scripts_GetCurrent(m_lurStore);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    float* p = EntityPos(vehicle != nullptr ? vehicle : ped);
    float y = p[1];
    float z = p[2];
    Script::PushFloat(L, p[0]);
    Script::PushFloat(L, y);
    Script::PushFloat(L, z);
    return 3;
}

// (ped, x, y, z, radius, marker, [heading])
int __cdecl Native_PedIsInAreaXYZ(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    if (M_GetPedFromHandle(handle, 2) == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float radius = Script::GetFloat(L, 4);
    int marker = Script::GetInt(L, 5);
    float heading = 0.0f;
    if (Script::GetArgCount(L) >= 7) {
        heading = float(double(Script::GetFloat(L, 6)) * 3.1415927410125732 / 180.0);
    }
    float rotation[3] = {0.0f, 0.0f, heading};
    uint8_t in = P2_IsEntityInArea(1, handle, 2, x, y, z, radius, radius, radius, marker, rotation, -1, -1, 0, 0, 1);
    Script::PushBool(L, in);
    return 1;
}

// (ped, handle, kind, radius, marker, [a = -1], [flag], [heading])
int __cdecl Native_PedIsInAreaObject(lua_State* L) {
    int handle = Script::GetInt(L, 0);
    uint8_t result = 0;
    if (M_GetPedFromHandle(handle, 2) != nullptr) {
        int other = Script::GetInt(L, 1);
        int kind = Script::GetInt(L, 2);
        float radius = Script::GetFloat(L, 3);
        int marker = Script::GetInt(L, 4);
        int a = -1;
        uint8_t flag = 0;
        if (Script::GetArgCount(L) >= 6) {
            a = Script::GetInt(L, 5);
        }
        if (Script::GetArgCount(L) >= 7) {
            flag = Script::GetBool(L, 6);
        }
        float heading = 0.0f;
        if (Script::GetArgCount(L) >= 8) {
            heading = float(double(Script::GetFloat(L, 7)) * 3.1415927410125732 / 180.0);
        }
        if (P2_GetAreaEntity(other, kind, 0, 0) != nullptr) {
            float pos[3];
            P2_GetAreaEntityPos(pos, other, kind);
            float rotation[3] = {0.0f, 0.0f, heading};
            result = P2_IsEntityInArea(1, handle, 2, pos[0], pos[1], pos[2], radius, radius, radius, marker, rotation, a,
                                       -1, flag, 0, 1);
        }
    }
    Script::PushBool(L, result);
    return 1;
}

// (ped, vehicle) -> it's in that vehicle
int __cdecl Native_PedIsInVehicle(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    int vehicle = Script::GetInt(L, 1);
    uint8_t in = 0;
    if (At<void*>(ped, 0x1554) != nullptr) {
        in = At<uint8_t*>(ped, 0x1554) == P2_GetVehicle(vehicle);
    }
    Script::PushBool(L, in);
    return 1;
}

// (ped, x, y, z, fov)
int __cdecl Native_PedIsFacingXYZ(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        Script::PushBool(L, 0);
        return 1;
    }
    float x = Script::GetFloat(L, 1);
    float y = Script::GetFloat(L, 2);
    float z = Script::GetFloat(L, 3);
    float fov = Script::GetFloat(L, 4);
    Script::PushBool(L, P2_IsPointInFront(ped, fov, x, y, z));
    return 1;
}

// (ped, handle, kind, fov)
int __cdecl Native_PedIsFacingObject(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    uint8_t facing = 0;
    if (ped != nullptr) {
        int handle = Script::GetInt(L, 1);
        int kind = Script::GetInt(L, 2);
        float fov = Script::GetFloat(L, 3);
        if (P2_GetAreaEntity(handle, kind, 0, 0) != nullptr) {
            float pos[3];
            P2_GetAreaEntityPos(pos, handle, kind);
            facing = P2_IsPointInFront(ped, fov, pos[0], pos[1], pos[2]);
        }
    }
    Script::PushBool(L, facing);
    return 1;
}

// (ped, unused, ms) -> its last hit (the damage info's +0x2C, -1 for none) was within
// the time
int __cdecl Native_PedIsHit(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped != nullptr) {
        Script::GetInt(L, 1);
        uint16_t ms = uint16_t(Script::GetInt(L, 2));
        float limit = float(double(int(ms)) / 1000.0);
        uint8_t* info = VCall<uint8_t*>(ped, 0x40);
        float when = At<float>(At<uint8_t*>(info, 0x10), 0x2C);
        if (!(-1.0f == when) && !(limit < when)) {
            Script::PushBool(L, 1);
            return 1;
        }
    }
    Script::PushBool(L, 0);
    return 1;
}

// (ped, [vehicle = the nearest within 15])
int __cdecl Native_PedEnterVehicle(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    int handle = Script::GetIntOpt(L, 1, -1);
    uint8_t* vehicle;
    if (handle != -1) {
        vehicle = P2_GetVehicle(handle);
    } else {
        vehicle = FindVehicleTarget(ped, 15.0f, 1, 1, 0);
        if (vehicle == nullptr) {
            return 0;
        }
    }
    At<uint8_t*>(ped, 0x430) = vehicle;
    EntityRef_Add(vehicle, ped + 0x430);
    void* memory = P2_Malloc(0x14);
    void* objective = memory != nullptr ? EnterVehicleObjective_Construct(memory, ped, vehicle) : nullptr;
    P2_Objectives_Add(ped + 0x5F0, objective);
    return 0;
}

// (ped): put first
int __cdecl Native_PedExitVehicle(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped != nullptr) {
        void* memory = P2_Malloc(0x10);
        void* objective = memory != nullptr ? ExitVehicleObjective_Construct(memory, ped) : nullptr;
        Objectives_Prepend(ped + 0x5F0, objective);
    }
    return 0;
}

// (ped) -> its last attacker (the damage info's +4); nothing without one
int __cdecl Native_PedGetWhoHitMeLast(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    if (At<void*>(At<uint8_t*>(VCall<uint8_t*>(ped, 0x40), 0x10), 4) == nullptr) {
        return 0;
    }
    void* attacker = At<void*>(At<uint8_t*>(VCall<uint8_t*>(ped, 0x40), 0x10), 4);
    if (attacker == nullptr) {
        Script::PushInt(L, 0);
        return 1;
    }
    Script::PushInt(L, VCall<int>(attacker, 0xAC));
    return 1;
}

// (ped) -> the vehicle its last attacker is in; nothing otherwise
int __cdecl Native_PedGetVehicleWhoHitMeLast(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped == nullptr) {
        return 0;
    }
    if (At<void*>(At<uint8_t*>(VCall<uint8_t*>(ped, 0x40), 0x10), 4) == nullptr) {
        return 0;
    }
    if (Ped_IsInVehicle(At<void*>(At<uint8_t*>(VCall<uint8_t*>(ped, 0x40), 0x10), 4)) == 0) {
        return 0;
    }
    uint8_t* attacker = At<uint8_t*>(At<uint8_t*>(VCall<uint8_t*>(ped, 0x40), 0x10), 4);
    Script::PushInt(L, GetVehicleRef(At<void*>(attacker, 0x1554)));
    return 1;
}

// (ped, health): its maximum (+0x1CA4) raised to it
int __cdecl Native_PedSetHealth(lua_State* L) {
    uint8_t* ped = P2_Ped(L, 0);
    if (ped != nullptr) {
        float health = Script::GetFloat(L, 1);
        Ped_SetHealth(ped, health);
        if (At<float>(ped, 0x1CA4) < health) {
            At<float>(ped, 0x1CA4) = health;
        }
    }
    return 0;
}

} // namespace

#define PEDSCRIPT2_NATIVE(name, address)                                               \
    static decltype(&Native_##name) Original_P2_##name;                                  \
    REVERSED("Native_" #name, address, RE_BODY(&Native_##name), nullptr, \
             reinterpret_cast<void**>(&Original_P2_##name), re::Mode::Original)

PEDSCRIPT2_NATIVE(PedWander, 0x005C7AF0);
PEDSCRIPT2_NATIVE(PedFollowFocus, 0x005C7B80);
PEDSCRIPT2_NATIVE(PedFleeOnPathOnBike, 0x005C7C10);
PEDSCRIPT2_NATIVE(PedFleeOnRoadOnBike, 0x005C7D40);
PEDSCRIPT2_NATIVE(PedMoveToXYZ, 0x005C7DF0);
PEDSCRIPT2_NATIVE(PedMoveToPoint, 0x005C7F50);
PEDSCRIPT2_NATIVE(PedMoveToObject, 0x005C8140);
PEDSCRIPT2_NATIVE(PedFollowObject, 0x005C8250);
PEDSCRIPT2_NATIVE(PedSetWorldAnchor, 0x005C8300);
PEDSCRIPT2_NATIVE(PedSetPosPoint, 0x005C8420);
PEDSCRIPT2_NATIVE(PedGetPosXYZ, 0x005C8530);
PEDSCRIPT2_NATIVE(PedIsInAreaXYZ, 0x005C8600);
PEDSCRIPT2_NATIVE(PedIsInAreaObject, 0x005C8730);
PEDSCRIPT2_NATIVE(PedIsInVehicle, 0x005C88B0);
PEDSCRIPT2_NATIVE(PedIsFacingXYZ, 0x005C8A20);
PEDSCRIPT2_NATIVE(PedIsFacingObject, 0x005C8AE0);
PEDSCRIPT2_NATIVE(PedIsHit, 0x005C8BA0);
PEDSCRIPT2_NATIVE(PedEnterVehicle, 0x005C8C50);
PEDSCRIPT2_NATIVE(PedExitVehicle, 0x005C8D10);
PEDSCRIPT2_NATIVE(PedGetWhoHitMeLast, 0x005C8DC0);
PEDSCRIPT2_NATIVE(PedGetVehicleWhoHitMeLast, 0x005C8E50);
PEDSCRIPT2_NATIVE(PedSetHealth, 0x005C8FE0);
