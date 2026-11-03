// CPed damage (0x47B7A0 .. 0x47C812): taking a hit (vf15, the whole reaction: who hit it
// and from where, the damage scaled by weapon kind and stats, the hit reaction's action
// nodes, rumble and events) and taking damage (0x47B800).

#include <cmath>

#include "game/Ped.h"
#include "game/Pool.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

inline uint8_t* Bytes(void* p) {
    return reinterpret_cast<uint8_t*>(p);
}
using re::At;
const ped::CVector* PosOf(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<ped::CVector*>(matrix + 0x30) : reinterpret_cast<ped::CVector*>(Bytes(entity) + 4);
}

constexpr int PED_TYPE_PLAYER = 0xD;

GAME_VAR(CPed*, PV_player, 0x00C1AEA8);               // FindPlayerPed
GAME_VAR(uint32_t, PV_timeInMs, 0x00C1A9B4);
GAME_VAR(CPool<CPed>*, PV_pedPool, 0x00C0F5F0);
GAME_VAR(int32_t, PV_C3CCF8, 0x00C3CCF8);
GAME_VAR(float, PV_f_2p32, 0x00900AC8);
GAME_VAR(double, PV_d_100, 0x00900130);
GAME_VAR(double, PV_d_0_15, 0x009051E0);
GAME_VAR(float, PV_f_100, 0x00900490);
GAME_VAR(double, PV_d_quarterPi, 0x0090CFA8);
GAME_VAR(double, PV_d_threeQuarterPi, 0x0090CFA0);
GAME_VAR(double, PV_d_halfPi, 0x00900960);
GAME_VAR(float, PV_f_minusOne, 0x009009AC);
GAME_VAR(ped::CVector, PV_zeroVector, 0x00BCC564);
GAME_VAR(int32_t, PV_1B6553C, 0x01B6553C);            // an attack kind that doesn't count twice within 100 ms
GAME_VAR(int32_t, PV_1B65484, 0x01B65484);
GAME_VAR(uint8_t, PV_difficulty, 0x00C1AF4C);
GAME_VAR(uint8_t*, PV_tuning, 0x020C88B8);            // +0xAC .. +0xB4: the player's damage by difficulty
GAME_VAR(int32_t, PV_id43E8, 0x00BF43E8);
GAME_VAR(int32_t, PV_id4344, 0x00BF4344);
GAME_VAR(float, PV_f_0_0001, 0x0090CF18);
GAME_VAR(double, PV_d_150, 0x0090CF98);
GAME_VAR(float, PV_deadHealth, 0x00906520);           // -100
GAME_VAR(void*, PV_C2C1A0, 0x00C2C1A0);
inline void* const PV_camera = reinterpret_cast<void*>(0x00C3CC68);

GAME_FN(uint8_t, __thiscall, PV_IsNodePlaying, 0x005F4800, void* controller, void* node, int a, int b);
GAME_FN(uint8_t, __thiscall, PV_Fn485590, 0x00485590, void* self, CPed* attacker);
GAME_FN(CPed*, __thiscall, PV_TargetingGetTarget, 0x004D5AD0, void* targeting);
GAME_FN(void, __thiscall, PV_ClearBlends, 0x00473AC0, CPed* ped);
GAME_FN(void, __cdecl, PV_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, PV_Fn497520, 0x00497520, CPed* ped, CPed* attacker);
GAME_FN(void, __thiscall, PV_TakeHealth, 0x004783F0, CPed* ped, float amount);
GAME_FN(int*, __thiscall, PV_GetAttitude, 0x00475380, CPed* ped, int* out, int pedType);
GAME_FN(void, __thiscall, PV_PropFn4EE060, 0x004EE060, void* prop, CPed* ped);
GAME_FN(ped::CVector*, __thiscall, PV_GetForward, 0x004154A0, void* entity, ped::CVector* out);
GAME_FN(ped::CVector*, __cdecl, PV_VectorSub, 0x00414B70, ped::CVector* out, const ped::CVector* a,
        const ped::CVector* b);
GAME_FN(float, __thiscall, PV_VectorNormalise, 0x00414E10, ped::CVector* v);
GAME_FN(uint8_t, __thiscall, PV_SeatsFn4CEA70, 0x004CEA70, void* seats, CPed* ped);
GAME_FN(float, __cdecl, PV_HeadingOf, 0x00433310, const ped::CVector* v);
GAME_FN(float, __cdecl, PV_LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, PV_Fn433350, 0x00433350, const ped::CVector* a, const ped::CVector* b);
GAME_FN(int, __cdecl, PV_Fn576ED0, 0x00576ED0, uintptr_t a);
GAME_FN(re::Ptr32<CEntity>*, __thiscall, PV_EntityPtrSet, 0x00478550, re::Ptr32<CEntity>* self, CEntity* entity);
inline double __cdecl PV_Crt85DBF0(double x) { return crt::Ceil(x); }  // (CRT math)
inline double __cdecl PV_Crt85D570(double x) { return crt::Floor(x); }  // (CRT math)
GAME_FN(float, __cdecl, PV_Fn450BA0, 0x00450BA0, float x);
GAME_FN(void, __thiscall, PV_CountActionStat, 0x00474120, CPed* ped, int id, int started);
GAME_FN(void*, __thiscall, PV_SeatsGetOccupant, 0x004CE560, void* seats, int seat);
GAME_FN(int*, __thiscall, PV_VehicleFn4CAF30, 0x004CAF30, void* vehicle, int* out);
GAME_FN(void, __thiscall, PV_ActionsFn5F3F80, 0x005F3F80, void* controller, void* node);
GAME_FN(void, __thiscall, PV_ActionsSetTree, 0x005F45F0, void* controller, void* node);
GAME_FN(void, __thiscall, PV_ActionsFn5F4110, 0x005F4110, void* controller, float a, int b);
GAME_FN(void, __thiscall, PV_PlayNode, 0x005F4AC0, void* controller, void* node);
GAME_FN(void, __cdecl, PV_Rumble, 0x00738680, int pad, int duration, int strength, int a);
GAME_FN(void, __cdecl, PV_Fn5B22F0, 0x005B22F0, CPed* ped, CPed* attacker);
GAME_FN(uint8_t, __thiscall, PV_PedIsPlaying1B654DC, 0x00473FD0, CPed* ped);
GAME_FN(CPed*, __thiscall, PV_Fn497020, 0x00497020, CPed* ped);
GAME_FN(void, __thiscall, PV_Fn4A6670, 0x004A6670, void* self, int kind, CPed* a, CPed* b, uintptr_t c, float d, float e);
GAME_FN(void, __thiscall, PV_Fn485A80, 0x00485A80, void* self, CPed* attacker);
GAME_FN(void, __thiscall, PV_Fn485B00, 0x00485B00, void* self, CPed* ped);
GAME_FN(void, __thiscall, PV_Fn48BF50, 0x0048BF50, void* self, CPed* attacker);
GAME_FN(void, __thiscall, PV_Fn48D7F0, 0x0048D7F0, void* self);
GAME_FN(void, __thiscall, PV_CameraFn4F2C20, 0x004F2C20, void* camera, int a, int b, int c, int d, int e);
GAME_FN(uint8_t, __cdecl, PV_Fn425700, 0x00425700);
GAME_FN(void, __thiscall, PV_MoveTargetSetBit, 0x00472CC0, void* target, int bit);

// The node source's vf19: a node for the intelligence's +0x300
void* NodeFor(void* source, CPed* ped) {
    return reinterpret_cast<void*(__thiscall*)(void*, void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(source))[0x4C / 4]))(
        source, ped->intelligence + 0x300);
}

void ReleaseRef(void* self, uint32_t offset) {
    re::Ptr32<void>& ref = At<void*>(self, offset);
    if (ref) {
        PV_CleanUpOldReference(ref, &ref);
        ref = nullptr;
    }
}

using ApplyHitFn = uint8_t(__thiscall*)(CPed*, float, float, CPed*, int, ped::CVector*, ped::CVector*, void*, int, int,
                                        int, float, float, int, int, int, int, float, int);

// 0x47B7A0 (thiscall): one of the first 24 peds of the pool, in the world (or the player)
uint8_t __fastcall CPed_IsInFirst24(CPed* self, re::Edx) {
    int slot = PV_pedPool->GetIndex(self);
    int handle = PV_pedPool->flags[slot] + (slot << 8);
    if (static_cast<uint32_t>(handle >> 8) <= 0x17 && (At<void*>(self, 0x178) != nullptr || self == PV_player)) {
        return 1;
    }
    return 0;
}

// 0x47B800 (thiscall): damage taken from an attacker (the attacker and the ped's own
// records told first); not while +0x1344 or +0x138C, or when the player hits a +0x1364
// one; a player's hit kills with 0x425700
uint8_t __fastcall CPed_TakeDamage(CPed* self, re::Edx, float amount, CPed* attacker) {
    PV_Fn485A80(Bytes(self) + 0x2F0, attacker);
    if (attacker && At<int32_t>(attacker, 0x2F8) != 0) {
        PV_Fn485B00(Bytes(attacker) + 0x2F0, self);
    }
    // (the game tests +0x3BC for null)
    At<uint32_t>(self, 0x4CC) = PV_timeInMs;
    PV_Fn48BF50(self->unknown3BC, attacker);
    if (attacker && attacker->pedType == PED_TYPE_PLAYER) {
        if (At<int32_t>(self, 0x134C) == 0) {
            At<int32_t>(self, 0x134C) = 1;
        }
        if (At<int32_t>(self, 0x1420) != 0) {
            PV_Fn48D7F0(Bytes(self) + 0xB44);
        }
    }
    if (At<int32_t>(self, 0x1344) != 0 || At<int32_t>(self, 0x138C) != 0 ||
        (attacker && attacker->pedType == PED_TYPE_PLAYER && At<int32_t>(self, 0x1364) != 0)) {
        return 0;
    }
    if (self->pedType == PED_TYPE_PLAYER &&
        (PV_C3CCF8 == 2 || PV_C3CCF8 == PED_TYPE_PLAYER || PV_C3CCF8 == 0xA || PV_C3CCF8 == 4)) {
        PV_CameraFn4F2C20(PV_camera, 1, 0, 1, 0, 0);
    }
    if (PV_Fn425700() && self->pedType != PED_TYPE_PLAYER && attacker && attacker->pedType == PED_TYPE_PLAYER) {
        PV_TakeHealth(self, self->health);
    }
    PV_TakeHealth(self, amount);
    return 1;
}

void EmitEvent(int kind, CPed* attacker, CPed* ped) {
    PV_Fn4A6670(PV_C2C1A0, kind, attacker, ped, 0, PV_f_minusOne, PV_f_minusOne);
}

// vf15 0x47B960: hit (by an attacker, from a direction, with a weapon or attack id): the
// hit recorded in the intelligence (+0x408 ..), the damage scaled, the reaction's nodes
// played (or passed on to a partner holding it), events raised. `ignoreChecks` (a15)
// skips the checks that refuse the hit.
uint8_t __fastcall CPed_ApplyHit(CPed* self, re::Edx, float damage, float a2, CPed* attacker, int weapon,
                                 ped::CVector* hitPos, ped::CVector* hitDir, void* nodeSource, int a8, int a9, int a10,
                                 float a11, float a12, int a13, int a14, int ignoreChecks, int a16, float a17, int a18) {
    uint8_t fromTarget = static_cast<uint8_t>(a13);
    CPed* atk = attacker;
    if (atk) {
        if (PV_IsNodePlaying(atk->intelligence + 0x258, reinterpret_cast<void*>(0x01B65534), 1, 1) && atk->partner &&
            atk->partner != self) {
            atk = atk->partner;
        }
        if (atk) {
            bool dropped = false;
            if (At<int32_t>(atk->intelligence, 0x46C) == PV_1B6553C) {
                int32_t dt = static_cast<int32_t>(PV_timeInMs - At<uint32_t>(self, 0x4CC));
                double v = static_cast<double>(dt);
                if (dt < 0) {
                    v = v + PV_f_2p32;
                }
                float elapsed = std::fabs(static_cast<float>(v));
                if (elapsed < PV_d_100) {
                    atk = nullptr;
                    dropped = true;
                }
            }
            if (!dropped && atk->pedType != PED_TYPE_PLAYER) {
                uint8_t* kind = self->unknown1D24;
                if ((kind && At<int32_t>(kind, 0x70) == 3) || At<int32_t>(self, 0x14CC) != 0) {
                    damage = 0.0f;
                }
            }
        }
    }
    if (!static_cast<uint8_t>(ignoreChecks) && PV_Fn485590(Bytes(self) + 0x2F0, atk) && atk != self) {
        // (a null attacker is read through here, as the game does)
        if (atk->pedType != PED_TYPE_PLAYER) {
            return 0;
        }
        if (PV_TargetingGetTarget(atk->targeting) != self) {
            return 0;
        }
        fromTarget = 1;
    }
    At<int32_t>(self, 0x1358) = 0;
    PV_ClearBlends(self);
    At<uint8_t>(self, 0x16D4) = 0xE2;
    ReleaseRef(self, 0x16D8);
    uint32_t lastHit = At<uint32_t>(self, 0x4CC);
    if (static_cast<uint8_t>(a14)) {
        At<uint32_t>(self, 0x4CC) = PV_timeInMs;
        if (atk && atk->pedType == PED_TYPE_PLAYER) {
            if (At<int32_t>(self, 0x134C) == 0) {
                At<int32_t>(self, 0x134C) = 1;
            } else {
                At<int32_t>(self, 0x1350) = 1;
            }
        }
    }
    if (atk && static_cast<uint8_t>(a14)) {
        PV_Fn497520(self, atk);
        uint8_t* vehicle = re::PtrCast<uint8_t*>(self->vehicle);
        if (vehicle && At<int32_t>(vehicle, 0x46C) == 0 && atk != self && self->seatState == 1) {
            void* set = self->attitudeSet;
            if (set) {
                reinterpret_cast<void(__thiscall*)(void*, int, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(set))[1]))(set, atk->pedType,
                                                                                                           0);
            }
        }
    }
    if (At<int32_t>(self, 0x1344) == 0 && self->pedType == PED_TYPE_PLAYER && At<int32_t>(self, 0x1E94) != 0 &&
        At<int32_t>(self, 0x1454) != 0) {
        PV_TakeHealth(self, static_cast<float>(damage * PV_d_0_15));
        if (!(0.0f < self->health)) {
            At<int32_t>(self, 0x1E94) = 0;
            return re::PtrCast<ApplyHitFn>(self->vtable[0x3C / 4])(self, PV_f_100, 0.0f, nullptr, -1, nullptr,
                                                                         nullptr, nullptr, 0, 0, 0, 0.0f, 0.0f, 0, 1, 0,
                                                                         -1, 0.0f, 0);
        }
    }
    uint8_t* intel = self->intelligence;
    if (!static_cast<uint8_t>(ignoreChecks)) {
        if (At<int32_t>(self, 0x1344) != 0) {
            return 0;
        }
        if (At<int32_t>(self, 0x138C) != 0 && self->pedType != PED_TYPE_PLAYER) {
            return 0;
        }
        if (atk) {
            if (At<int32_t>(self, 0x1364) != 0 && atk->pedType == PED_TYPE_PLAYER) {
                return 0;
            }
            if (fromTarget == 0) {
                int attitude;
                if (*PV_GetAttitude(atk, &attitude, self->pedType) == 4 && weapon == -1) {
                    return 0;
                }
            }
        }
        if (self->pedType == PED_TYPE_PLAYER && At<int32_t>(self, 0x1E94) != 0) {
            return 0;
        }
        intel = self->intelligence;
        if (intel[0x408] != 0 && At<float>(intel, 0x434) == 0.0f && damage <= At<float>(intel, 0x440)) {
            if (!self->partner || self->partner != atk) {
                return 0;
            }
        }
    }
    if (atk && At<void*>(self, 0x548) && At<int32_t>(self, 0x13C4) == 0 && At<int32_t>(self, 0x1548) == 0 &&
        At<int32_t>(self, 0x1348) == 0) {
        PV_PropFn4EE060(At<void*>(self, 0x548), self);
        ReleaseRef(self, 0x548);
    }
    // where the hit came from
    ped::CVector forward;
    PV_GetForward(self, &forward);
    ped::CVector dir = {-forward.x, -forward.y, -forward.z};
    if (atk) {
        ped::CVector d;
        dir = *PV_VectorSub(&d, PosOf(self), PosOf(atk));
    } else if (hitDir) {
        dir = *hitDir;
    }
    PV_VectorNormalise(&dir);
    bool inVehicle = self->vehicle && PV_SeatsFn4CEA70(re::PtrCast<uint8_t*>(self->vehicle) + 0x1C4, self);
    float heading = self->desiredHeading;
    ped::CVector back = {-dir.x, -dir.y, -dir.z};
    float angle = PV_HeadingOf(&back);
    float rel = a17;
    if (a17 == 0.0f) {
        rel = PV_LimitAngle(heading - angle);
    }
    int side = 0;
    if (inVehicle) {
        if (hitPos) {
            PV_Fn433350(PosOf(self), hitPos);
        }
        float absRel = std::fabs(rel);
        if (absRel <= PV_d_quarterPi) {
            side = 1;
        } else if (absRel >= PV_d_threeQuarterPi) {
            side = 2;
        } else if (rel >= 0.0f) {
            side = 4;
        } else {
            side = 3;
        }
    }
    At<float>(self->intelligence, 0x45C) = PV_f_minusOne;
    At<float>(self->intelligence, 0x460) = 0.0f;
    self->intelligence[0x408] = 1;
    self->intelligence[0x43C] = static_cast<uint8_t>(a10);
    if (atk || hitDir) {
        At<ped::CVector>(self->intelligence, 0x428) = dir;
        At<ped::CVector>(self->intelligence, 0x41C) = hitDir ? *hitDir : dir;
    }
    At<ped::CVector>(self->intelligence, 0x410) = hitPos ? *hitPos : PV_zeroVector;
    At<int32_t>(self->intelligence, 0x46C) = a9 ? PV_Fn576ED0(a9) : 0;
    if (atk && atk != self && At<int32_t>(self->intelligence, 0x46C) != PV_1B65484) {
        At<uint8_t>(atk, 0x5D8) = 1;
    }
    // the attacker (+0x40C) and the hit's side bits (a MoveTarget at +0x408, see 0x472CC0)
    if (atk == self) {
        ReleaseRef(self->intelligence, 0x40C);
    } else {
        PV_EntityPtrSet(&At<CEntity*>(self->intelligence, 0x40C), reinterpret_cast<CEntity*>(atk));
    }
    if (inVehicle) {
        uint8_t* target = self->intelligence + 0x408;
        float& pending = At<float>(target, 0x2C);
        if (pending != 0.0f) {
            At<float>(target, 0x30) = pending;
            target[0x68] = 0;
            pending = 0.0f;
        }
        target[0x68] |= static_cast<uint8_t>(1 << side);
    } else {
        float absRel = std::fabs(rel);
        PV_MoveTargetSetBit(self->intelligence + 0x408, absRel <= PV_d_halfPi ? 1 : 2);
        PV_MoveTargetSetBit(self->intelligence + 0x408, 0.0f < rel ? 3 : 4);
    }
    At<int16_t>(self->intelligence, 0x466) = static_cast<int16_t>(weapon);
    At<int32_t>(self->intelligence, 0x468) = a18;
    At<int32_t>(self->intelligence, 0x368) = At<int32_t>(self->intelligence, 0x310);
    if (weapon == -1 && atk && (At<uint8_t>(atk, 0x108) & 7) == 3 && atk->pedType == PED_TYPE_PLAYER) {
        uint32_t offset = 0;
        if (PV_difficulty == 0) {
            offset = 0xAC;
        } else if (PV_difficulty == 1) {
            offset = 0xB0;
        } else if (PV_difficulty == 2) {
            offset = 0xB4;
        }
        if (offset) {
            damage = At<float>(PV_tuning, offset) * damage;
        }
    }
    // the damage by the kind of hit (a18) and the stats of both
    CPed* pedAttacker = (atk && (At<uint8_t>(atk, 0x108) & 7) == 3) ? atk : nullptr;
    if (static_cast<uint8_t>(a10) && a18 == 4) {
        if (pedAttacker) {
            float v = At<float>(pedAttacker, 0x1CD8) * damage;
            damage = static_cast<float>(PV_Crt85DBF0(v));
        }
        float v = At<float>(self, 0x1D00) * damage;
        damage = static_cast<float>(PV_Crt85D570(v));
    } else if (a18 == 2 || a18 == 3) {
        if (pedAttacker) {
            float v = At<float>(pedAttacker, 0x1CCC) * damage;
            damage = static_cast<float>(PV_Crt85DBF0(v));
        }
        float v = At<float>(self, 0x1CF4) * damage;
        damage = static_cast<float>(PV_Crt85D570(v));
    } else if (a18 == 1 || weapon == -1) {
        if (pedAttacker) {
            float v = At<float>(pedAttacker, 0x1CD4) * damage;
            damage = PV_Fn450BA0(v);
        }
        float v = At<float>(self, 0x1CFC) * damage;
        damage = static_cast<float>(PV_Crt85D570(v));
    }
    if (atk == PV_player) {
        PV_CountActionStat(self, weapon, a10);
    }
    intel = self->intelligence;
    At<float>(intel, 0x440) = damage;
    At<float>(self->intelligence, 0x444) = a2;
    At<int32_t>(self->intelligence, 0x448) = 0;
    At<float>(self->intelligence, 0x454) = a11;
    At<float>(self->intelligence, 0x458) = a12;
    self->intelligence[0x464] = static_cast<uint8_t>(a16);
    // the reaction's nodes (not for a driver of a car of kind other than 6 or 7)
    uint8_t* vehicle = re::PtrCast<uint8_t*>(self->vehicle);
    bool react = true;
    if (vehicle && At<int32_t>(vehicle, 0x46C) == 0 && PV_SeatsGetOccupant(vehicle + 0x1C4, 0) == self &&
        At<int32_t>(self, 0x100) == 0) {
        int kind;
        react = *PV_VehicleFn4CAF30(self->vehicle, &kind) == 7 || *PV_VehicleFn4CAF30(self->vehicle, &kind) == 6;
    }
    if (react) {
        if (nodeSource) {
            intel = self->intelligence;
            void* node = NodeFor(nodeSource, self);
            if (static_cast<uint8_t>(a8)) {
                PV_ActionsFn5F3F80(intel + 0x258, node);
            } else {
                PV_ActionsSetTree(intel + 0x258, node);
            }
            PV_ActionsFn5F4110(self->intelligence + 0x258, 0.0f, 0);
            At<float>(self->intelligence, 0x434) = PV_f_0_0001;
        } else if (At<void*>(self->intelligence, 0x4B8)) {
            intel = self->intelligence;
            CPed* p = self->partner;
            if (p && p->partner == self && At<int32_t>(p, 0x1348) == 0) {
                bool pass;
                if ((intel[0x43C] && weapon == PV_id43E8) || weapon == PV_id4344) {
                    uint8_t* other = p->intelligence;
                    pass = At<int16_t>(other, 0x466) != weapon || 0.0f < At<float>(other, 0x434);
                } else {
                    pass = p != atk;
                }
                if (pass) {
                    re::PtrCast<ApplyHitFn>(p->vtable[0x3C / 4])(p, 0.0f, a2, self, weapon, hitPos, hitDir, nullptr, 0,
                                                                     a9, a10, a11, a12, 1, a14, ignoreChecks, a16, a17,
                                                                     a18);
                }
            }
            intel = self->intelligence;
            if (At<int32_t>(intel, 0x268) == 0) {
                PV_ActionsSetTree(intel + 0x258, NodeFor(At<void*>(intel, 0x4B0), self));
            }
            void* node = NodeFor(At<void*>(self->intelligence, 0x4B8), self);
            if (node) {
                PV_PlayNode(self->intelligence + 0x258, node);
            }
        }
    }
    if (self->pedType == PED_TYPE_PLAYER) {
        int strength = static_cast<int>(damage * PV_d_150);
        int8_t low = static_cast<int8_t>(strength);
        int16_t scaled = static_cast<int16_t>(low * static_cast<int8_t>(0x19));
        PV_Rumble(0, 0x28, static_cast<uint16_t>(scaled), 1);
    }
    PV_Fn5B22F0(self, atk);
    if (atk && static_cast<uint8_t>(a14)) {
        uint8_t playing = PV_IsNodePlaying(self->intelligence + 0x290, reinterpret_cast<void*>(0x01B6528C), 1, 1);
        uint8_t busy = PV_PedIsPlaying1B654DC(self);
        if (weapon == -1) {
            if (playing || busy ||
                (self->pedType == PED_TYPE_PLAYER && At<CPed*>(atk->intelligence, 0x40C) == self)) {
                if (lastHit + 5000 > PV_timeInMs || PV_Fn497020(atk) == self) {
                    EmitEvent(5, atk, self);
                }
                if (!busy) {
                    EmitEvent(4, atk, self);
                }
            } else {
                EmitEvent(4, atk, self);
            }
        } else if (!busy) {
            EmitEvent(4, atk, self);
        }
    }
    At<float>(self->weapon, 0x128) = PV_deadHealth;
    return 1;
}

#define PEDDAMAGE_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PEDDAMAGE_REVERSED("CPed::IsInFirst24", 0x0047B7A0, &CPed_IsInFirst24);
PEDDAMAGE_REVERSED("CPed::TakeDamage", 0x0047B800, &CPed_TakeDamage);
PEDDAMAGE_REVERSED("CPed::ApplyHit", 0x0047B960, &CPed_ApplyHit);

} // namespace
