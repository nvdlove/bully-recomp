// CCarAI::UpdateCarAI (0x418A50): GTA's, Bully's version: a vehicle's mission
// switched by distance (to the player, its target coordinates or target entity),
// the temporary actions for stuck, upside-down or rammed vehicles, and the cruise
// speed multiplier eased towards the driving mode's.
// (x87 extended precision in the game; what it stores as floats is kept as floats,
// what it stores as doubles as doubles.)

#include <cmath>
#include <cstdint>

#include "reversed.h"

namespace c01carai {

struct CVector {
    float x, y, z;
};
using re::At;

// The vehicle's AutoPilot fields (by offset):
constexpr uint32_t MOVE_SPEED = 0x124;       // CVector
constexpr uint32_t LAST_COLLISION = 0x120;   // time
constexpr uint32_t AI_KIND = 0x290;          // byte (1, 2: special handling when stuck)
constexpr uint32_t TEMP_STEER = 0x298;       // float
constexpr uint32_t TIME_STUCK = 0x2CC;       // time last moving (or bumped)
constexpr uint32_t TIME_MOVING = 0x2D0;      // time last moving
constexpr uint32_t DRIVING_STYLE = 0x2D9;    // byte
constexpr uint32_t MISSION = 0x2DA;          // byte, GTA's eCarMission
constexpr uint32_t TEMP_ACTION = 0x2DB;      // byte
constexpr uint32_t TEMP_ACTION_TIME = 0x2DC; // time it ends
constexpr uint32_t CRUISE_SPEED = 0x2E4;     // byte
constexpr uint32_t SPEED_MODE = 0x2E5;       // int8
constexpr uint32_t SPEED_MULT = 0x2E8;       // float
constexpr uint32_t AP_FLAGS = 0x2EC;         // byte (0x10: ignore pathfinding)
constexpr uint32_t SWITCH_DIST = 0x2ED;      // byte
constexpr uint32_t AP_FLAGS2 = 0x2EE;        // byte
constexpr uint32_t TARGET_COORS = 0x2F0;     // CVector
constexpr uint32_t TARGET_RADIUS = 0x320;    // float
constexpr uint32_t TARGET_ENTITY = 0x324;
constexpr uint32_t TARGET_ENTITY2 = 0x328;
constexpr uint32_t FLAGS_33B = 0x33B;
constexpr uint32_t FIELD_368 = 0x368;
constexpr uint32_t SIREN_ON = 0x36D;

GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint32_t, ms_frameCounter, 0x00C1A99C);
GAME_VAR(float, ms_fTimeStep, 0x00C1A9A4);
GAME_VAR(uint8_t*, PED_player, 0x00C1AEA8);

GAME_FN(CVector*, __cdecl, FindPlayerCoors, 0x00446900, CVector* out);
GAME_FN(uint8_t*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t);
GAME_FN(CVector*, __cdecl, FindPlayerSpeed, 0x00445D60);
GAME_FN(float, __cdecl, Sqrtf, 0x0040DF90, float value);   // sqrt(|value|)
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const CVector* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __cdecl, Max, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, FindSwitchDistanceFar, 0x00418A20, void* vehicle);   // switch distance + 5
GAME_FN(uint8_t, __thiscall, HasSirenModel, 0x004CB1F0, void* vehicle);
GAME_FN(uint8_t, __thiscall, GetHasCollidedWith, 0x00468D40, void* vehicle, void* other);
GAME_FN(uint8_t, __cdecl, JoinCarWithRoadSystemGotoCoors_41E6C0, 0x0041E6C0, void* vehicle, CVector target, int);
GAME_FN(void, __cdecl, JoinCarWithRoadSystem_477DD0, 0x00477DD0, void* vehicle);
GAME_FN(void, __cdecl, Fn_41E370, 0x0041E370, void* vehicle, uintptr_t);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);

CVector* PositionOf(void* placeable) {
    uint8_t* matrix = At<uint8_t*>(placeable, 0x14);
    return matrix ? reinterpret_cast<CVector*>(matrix + 0x30) : reinterpret_cast<CVector*>(static_cast<uint8_t*>(placeable) + 4);
}
// the flat distance, its sum stored as a float before the square root
float Dist2D(double dx, double dy) {
    return Sqrtf(static_cast<float>(dx * dx + dy * dy));
}
float DistToPlayer(void* vehicle) {
    CVector player;
    const CVector* p = FindPlayerCoors(&player);
    const CVector* v = PositionOf(vehicle);
    double dy = static_cast<double>(p->y - v->y);
    double dx = static_cast<double>(p->x - v->x);
    return Dist2D(dx, dy);
}
float DistToCoors(void* vehicle) {
    const CVector* v = PositionOf(vehicle);
    const CVector& t = At<CVector>(vehicle, TARGET_COORS);
    return Dist2D(static_cast<double>(t.x - v->x), static_cast<double>(t.y - v->y));
}
float DistToEntity(void* vehicle, void* target) {
    const CVector* v = PositionOf(vehicle);
    const CVector* t = PositionOf(target);
    return Dist2D(static_cast<double>(t->x - v->x), static_cast<double>(t->y - v->y));
}
bool IgnoresPathfinding(void* vehicle) {
    return (At<uint8_t>(vehicle, AP_FLAGS) & 0x10) != 0;
}
bool JoinTowards(void* vehicle, const CVector& target) {
    return JoinCarWithRoadSystemGotoCoors_41E6C0(vehicle, target, 1) != 0;
}
void SirenIfAny(void* vehicle) {
    if (HasSirenModel(vehicle)) {
        At<uint8_t>(vehicle, SIREN_ON) = 1;
    }
}
void ClearSiren(void* vehicle) {
    At<uint8_t>(vehicle, SIREN_ON) = 0;
    At<int32_t>(vehicle, FIELD_368) = 0;
}
// the arrived-at-coordinates reset (GOTOCOORDS_STRAIGHT and _ACCURATE)
void Arrived(void* vehicle) {
    At<uint8_t>(vehicle, MISSION) = 0;
    At<uint8_t>(vehicle, TEMP_ACTION) = 0;
    if (At<uint8_t>(vehicle, FLAGS_33B) & 0x80) {
        JoinCarWithRoadSystem_477DD0(vehicle);
        At<uint8_t>(vehicle, FLAGS_33B) &= 0x7F;
    }
}
// a far/straight coordinates mission re-chosen every 8 frames when far off
void RechooseStraight(void* vehicle, float dist, uint8_t base) {
    if (static_cast<double>(At<uint8_t>(vehicle, SWITCH_DIST)) + 5.0 < dist && !IgnoresPathfinding(vehicle) &&
        (ms_frameCounter & 7) == 0) {
        At<uint8_t>(vehicle, TEMP_ACTION) = 0;
        At<uint8_t>(vehicle, MISSION) = static_cast<uint8_t>(base + (JoinTowards(vehicle, At<CVector>(vehicle, TARGET_COORS)) ? 1 : 0));
    }
}
// the chase of an entity (0x1A, 0x1B): the cruise speed matched to its speed along us
bool MatchTargetSpeed(void* vehicle, uint8_t* target) {
    CVector diff;
    const CVector* d = VectorSub(&diff, PositionOf(vehicle), PositionOf(target));
    float dist = Vector_Magnitude2D(d);
    if (!(At<float>(vehicle, TARGET_RADIUS) + 2.0 > dist)) {
        return false;
    }
    CVector forward;
    float along = DotProduct(reinterpret_cast<CVector*>(target + MOVE_SPEED), Placeable_GetForward(vehicle, &forward));
    At<uint8_t>(vehicle, CRUISE_SPEED) = static_cast<uint8_t>(static_cast<int>(along * 50.0));
    return true;
}

// 0x418A50
void __cdecl UpdateCarAI(uint8_t* vehicle) {
    uint8_t status = vehicle[0x108] >> 3;
    if (status == 2 || status == 3) {
        switch (static_cast<int8_t>(At<uint8_t>(vehicle, MISSION))) {
        case 2:   // RAMPLAYER_FARAWAY
            if (DistToPlayer(vehicle) < static_cast<float>(At<uint8_t>(vehicle, SWITCH_DIST)) || IgnoresPathfinding(vehicle)) {
                At<uint8_t>(vehicle, MISSION) = 3;
                SirenIfAny(vehicle);
            }
            break;
        case 3: {   // RAMPLAYER_CLOSE
            float dist = DistToPlayer(vehicle);
            if (!(FindSwitchDistanceFar(vehicle) < dist) || IgnoresPathfinding(vehicle)) {
                if (FindPlayerVehicle(0) && GetHasCollidedWith(vehicle, FindPlayerVehicle(0))) {
                    uint8_t action = At<uint8_t>(vehicle, TEMP_ACTION);
                    if (action != 6 && action != 7) {
                        float speed = Vector_Magnitude(reinterpret_cast<CVector*>(FindPlayerVehicle(0) + MOVE_SPEED));
                        At<uint8_t>(vehicle, TEMP_ACTION) = 2;
                        if (speed < 0.05000000074505806) {
                            At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 800;
                        } else {
                            At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 50;
                        }
                    }
                }
            } else {
                CVector player;
                if (!JoinTowards(vehicle, *FindPlayerCoors(&player))) {
                    At<uint8_t>(vehicle, MISSION) = 2;
                    ClearSiren(vehicle);
                }
            }
            break;
        }
        case 4:   // BLOCKPLAYER_FARAWAY
            if (DistToPlayer(vehicle) < static_cast<float>(At<uint8_t>(vehicle, SWITCH_DIST)) || IgnoresPathfinding(vehicle)) {
                At<uint8_t>(vehicle, MISSION) = 5;
                SirenIfAny(vehicle);
            }
            break;
        case 5: {   // BLOCKPLAYER_CLOSE
            float dist = DistToPlayer(vehicle);
            if (FindSwitchDistanceFar(vehicle) < dist && !IgnoresPathfinding(vehicle)) {
                CVector player;
                if (!JoinTowards(vehicle, *FindPlayerCoors(&player))) {
                    At<uint8_t>(vehicle, MISSION) = 4;
                    ClearSiren(vehicle);
                }
            }
            break;
        }
        case 8:   // GOTOCOORDS
            if (DistToCoors(vehicle) < static_cast<float>(At<uint8_t>(vehicle, SWITCH_DIST)) || IgnoresPathfinding(vehicle)) {
                At<uint8_t>(vehicle, MISSION) = 9;
            }
            break;
        case 9: {   // GOTOCOORDS_STRAIGHT
            float dist = DistToCoors(vehicle);
            if (dist < 3.0f) {
                Arrived(vehicle);
            } else {
                RechooseStraight(vehicle, dist, 8);
            }
            break;
        }
        case 0xC:   // GOTOCOORDS_ACCURATE
            if (DistToCoors(vehicle) < static_cast<float>(At<uint8_t>(vehicle, SWITCH_DIST)) || IgnoresPathfinding(vehicle)) {
                At<uint8_t>(vehicle, MISSION) = 0xD;
            }
            break;
        case 0xD: {   // GOTO_COORDS_STRAIGHT_ACCURATE
            float dist = DistToCoors(vehicle);
            if (dist < 1.0f) {
                Arrived(vehicle);
            } else {
                RechooseStraight(vehicle, dist, 0xC);
            }
            break;
        }
        case 0xE: {   // RAMCAR_FARAWAY
            uint8_t* target = At<uint8_t*>(vehicle, TARGET_ENTITY);
            if (!target) {
                At<uint8_t>(vehicle, MISSION) = 0;
                break;
            }
            if (DistToEntity(vehicle, target) < static_cast<float>(At<uint8_t>(vehicle, SWITCH_DIST)) || IgnoresPathfinding(vehicle)) {
                At<uint8_t>(vehicle, MISSION) = 0xF;
            }
            break;
        }
        case 0xF: {   // RAMCAR_CLOSE
            uint8_t* target = At<uint8_t*>(vehicle, TARGET_ENTITY);
            if (!target) {
                At<uint8_t>(vehicle, MISSION) = 0;
                break;
            }
            float dist = DistToEntity(vehicle, target);
            if (!(dist > FindSwitchDistanceFar(vehicle)) || IgnoresPathfinding(vehicle)) {
                if (GetHasCollidedWith(vehicle, target) &&
                    Vector_Magnitude(reinterpret_cast<CVector*>(vehicle + MOVE_SPEED)) < 0.03999999910593033f) {
                    At<uint8_t>(vehicle, TEMP_ACTION) = 2;
                    At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 800;
                }
            } else {
                At<uint8_t>(vehicle, MISSION) = 0xE;
                Fn_41E370(vehicle, 0);
            }
            break;
        }
        case 0x10: {   // BLOCKCAR_FARAWAY
            uint8_t* target = At<uint8_t*>(vehicle, TARGET_ENTITY);
            if (!target) {
                At<uint8_t>(vehicle, MISSION) = 0;
                break;
            }
            if (DistToEntity(vehicle, target) < static_cast<float>(At<uint8_t>(vehicle, SWITCH_DIST)) || IgnoresPathfinding(vehicle)) {
                At<uint8_t>(vehicle, MISSION) = 0x11;
                SirenIfAny(vehicle);
            }
            break;
        }
        case 0x11: {   // BLOCKCAR_CLOSE
            uint8_t* target = At<uint8_t*>(vehicle, TARGET_ENTITY);
            if (!target) {
                At<uint8_t>(vehicle, MISSION) = 0;
                break;
            }
            float dist = DistToEntity(vehicle, target);
            if (FindSwitchDistanceFar(vehicle) < dist && !IgnoresPathfinding(vehicle)) {
                At<uint8_t>(vehicle, MISSION) = 0x10;
                ClearSiren(vehicle);
                Fn_41E370(vehicle, 0);
            }
            break;
        }
        case 0x13:
            if (DistToCoors(vehicle) < 1.5) {
                At<uint8_t>(vehicle, MISSION) = 0x14;
            }
            break;
        case 0x14:
            if (DistToPlayer(vehicle) < 13.0) {
                JoinCarWithRoadSystem_477DD0(vehicle);
                At<uint8_t>(vehicle, CRUISE_SPEED) = 0;
                At<uint8_t>(vehicle, MISSION) = 0xB;
            }
            break;
        case 0x15: {
            CVector player;
            const CVector* p = FindPlayerCoors(&player);
            float dx = p->x - PositionOf(vehicle)->x;
            p = FindPlayerCoors(&player);
            float dy = p->y - PositionOf(vehicle)->y;
            float dist = Max(0.0010000000474974513f, Sqrtf(static_cast<float>(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy)));
            if (FindPlayerVehicle(0)) {
                double along = static_cast<double>(static_cast<float>(dy / dist)) * FindPlayerSpeed()->y;
                if (static_cast<double>(static_cast<float>(dx / dist)) * FindPlayerSpeed()->x + along <= 0.05000000074505806) {
                    break;
                }
            }
            At<uint8_t>(vehicle, MISSION) = 5;
            break;
        }
        case 0x1A: {
            uint8_t* target = At<uint8_t*>(vehicle, TARGET_ENTITY2);
            if (!target) {
                At<uint8_t>(vehicle, MISSION) = 0xB;
                break;
            }
            uint8_t kind = target[0x108] & 7;
            if (kind == 3 || kind == 2) {
                MatchTargetSpeed(vehicle, target);
            }
            break;
        }
        case 0x1B: {
            uint8_t* target = At<uint8_t*>(vehicle, TARGET_ENTITY2);
            if (!target) {
                At<uint8_t>(vehicle, MISSION) = 0xB;
                break;
            }
            if (At<int32_t>(target, 0xF0) == 0 || !MatchTargetSpeed(vehicle, target)) {
                break;
            }
            if (At<uint8_t>(vehicle, AP_FLAGS2) & 1) {
                At<uint8_t>(vehicle, TEMP_ACTION) = 5;
                At<float>(vehicle, TEMP_STEER) = 1.0f;
                At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 500;
            }
            float speedSq = Vector_MagnitudeSqr(reinterpret_cast<CVector*>(vehicle + MOVE_SPEED));
            if (speedSq <= 0.5f) {
                At<uint8_t>(vehicle, MISSION) = 0xB;
                JoinCarWithRoadSystem_477DD0(vehicle);
            }
            break;
        }
        default:
            break;
        }
    } else if (status == 4 || status == 5) {
        At<uint8_t>(vehicle, CRUISE_SPEED) = 0;
        At<uint8_t>(vehicle, MISSION) = 0;
    }

    // stuck: not moving for 2 seconds while it should be driving
    const CVector& speed = At<CVector>(vehicle, MOVE_SPEED);
    float speedSq = speed.x * speed.x + speed.y * speed.y;
    const float stuckSpeedSq = 0.0003239999641664326f;
    if (speedSq > stuckSpeedSq) {
        At<uint32_t>(vehicle, TIME_MOVING) = m_snTimeInMilliseconds;
        At<uint32_t>(vehicle, TIME_STUCK) = m_snTimeInMilliseconds;
    }
    uint8_t mission = At<uint8_t>(vehicle, MISSION);
    uint8_t style = At<uint8_t>(vehicle, DRIVING_STYLE);
    uint8_t kind = At<uint8_t>(vehicle, AI_KIND);
    if ((vehicle[0x108] & 0xF8) == 0x18 && At<uint8_t>(vehicle, TEMP_ACTION) == 0 && mission != 0 && mission != 0xB &&
        mission != 6 && mission != 0x18 && mission != 0x1C && style != 5 && At<uint8_t>(vehicle, CRUISE_SPEED) != 0 &&
        !(kind == 1 && mission == 1) && (kind == 2 || (style != 0 && style != 4))) {
        if (m_snTimeInMilliseconds - At<uint32_t>(vehicle, LAST_COLLISION) > 500) {
            At<uint32_t>(vehicle, TIME_STUCK) = m_snTimeInMilliseconds;
        }
        if (speedSq < stuckSpeedSq && m_snTimeInMilliseconds - At<uint32_t>(vehicle, TIME_STUCK) > 2000) {
            At<uint8_t>(vehicle, TEMP_ACTION) = 2;
            if (mission == 1 && kind != 2) {
                At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 700;
            } else {
                At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 1500;
            }
            At<uint32_t>(vehicle, TIME_STUCK) = m_snTimeInMilliseconds;
            if (kind == 1) {
                At<uint8_t>(vehicle, DRIVING_STYLE) =
                    static_cast<uint8_t>(static_cast<int>(Max(static_cast<float>(static_cast<int8_t>(style)), 2.0f)));
            }
            reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>(At<re::Ptr32<void>*>(vehicle, 0)[0xDC / 4]))(vehicle);
        }
    }

    // upside down
    if (Entity_GetMatrix(vehicle)[10] < -0.699999988079071f) {
        At<uint8_t>(vehicle, TEMP_ACTION) = 1;
        At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 1000;
    }

    // the player's vehicle coming up fast behind, same way: out of its way
    if (At<uint8_t>(vehicle, TEMP_ACTION) == 0 && static_cast<uint8_t>(At<uint8_t>(vehicle, MISSION) - 2) <= 3 && FindPlayerVehicle(0)) {
        float playerSpeed = Vector_Magnitude(FindPlayerSpeed());
        if (playerSpeed > Vector_Magnitude(reinterpret_cast<CVector*>(vehicle + MOVE_SPEED)) &&
            Vector_Magnitude(FindPlayerSpeed()) > 0.10000000149011612) {
            CVector f1, f2, f3, f4;
            double y1 = Placeable_GetForward(FindPlayerVehicle(0), &f1)->y;
            double yy = Placeable_GetForward(vehicle, &f2)->y * y1;
            double x1 = Placeable_GetForward(FindPlayerVehicle(0), &f3)->x;
            if (Placeable_GetForward(vehicle, &f4)->x * x1 + yy > 0.0) {
                CVector player;
                double vx = PositionOf(vehicle)->x;
                float dx = static_cast<float>(vx - FindPlayerCoors(&player)->x);
                double vy = PositionOf(vehicle)->y;
                float dy = static_cast<float>(vy - FindPlayerCoors(&player)->y);
                float sx = FindPlayerSpeed()->x;
                float sy = FindPlayerSpeed()->y;
                double along = static_cast<double>(dx) * sx + static_cast<double>(dy) * sy;
                double halfDist = Sqrtf(static_cast<float>(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy)) * 0.5;
                double speed2D = Sqrtf(static_cast<float>(static_cast<double>(sx) * sx + static_cast<double>(sy) * sy));
                if (speed2D * halfDist < along) {
                    CVector diff;
                    const CVector* d = VectorSub(&diff, FindPlayerCoors(&player), PositionOf(vehicle));
                    if (Vector_Magnitude(d) > 12.0f) {
                        At<uint8_t>(vehicle, TEMP_ACTION) = 1;
                        At<uint32_t>(vehicle, TEMP_ACTION_TIME) = m_snTimeInMilliseconds + 500;
                    }
                }
            }
        }
    }

    // the cruise speed multiplier
    float wanted = 1.0f;
    if (At<uint8_t>(vehicle, MISSION) == 1) {
        int8_t mode = static_cast<int8_t>(At<uint8_t>(vehicle, SPEED_MODE));
        if (PED_player && At<int32_t>(PED_player, 0x16C8) != 0) {
            wanted = mode == 1 ? 2.0f : mode == 2 ? 2.5999999046325684f : mode == 3 ? 0.8999999761581421f : 1.5f;
        } else {
            wanted = mode == 1 ? 1.5f : mode == 2 ? 2.0f : mode == 3 ? 0.25f : 1.0f;
        }
    }
    float step = static_cast<float>(ms_fTimeStep * 0.09000000357627869);
    float current = At<float>(vehicle, SPEED_MULT);
    float diff = std::fabs(static_cast<float>(current - wanted));
    if (step > diff) {
        At<float>(vehicle, SPEED_MULT) = wanted;
    } else if (!(current > wanted)) {
        At<float>(vehicle, SPEED_MULT) = step + current;
    } else {
        At<float>(vehicle, SPEED_MULT) = current - step;
    }
}

} // namespace c01carai

// ---- registration ----------------------------------------------------------------------

#define CARAI_REVERSED_C01(name, address, fn)                                                \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace c01carai {
CARAI_REVERSED_C01("CCarAI::UpdateCarAI", 0x00418A50, &UpdateCarAI);
} // namespace c01carai
