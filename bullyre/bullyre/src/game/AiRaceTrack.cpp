// The action tree's RaceTrack (a racer in a race minigame, game mode 0xE): its Update,
// 0x6236D0.
// The track: +0x1C the owner. The race (the game mode object): +4 on, +0x3C the mode,
// +0x128 started, +0x12C the start time (ms). A racer (0x70D390): +0x24 / +0x28 its
// ramming / cruising gas (an int8 on the vehicle's +0x2E4), +0x2C / +0x30 the distance
// ahead of the player beyond which (and the scale) it eases off, +0x34 / +0x38 behind
// it (and the scale it catches up by; +0x34 also the ramming limit), +0x54 its
// aggression.
// The vehicle: +0x294 its forward speed, +0x2D9 / +0x2DA the driving mission and
// style, +0x2DB stopped, +0x2E4 the gas, +0x2EC flags (0x10 racing), +0x46C its kind (0 a
// car). Carts fire rockets (the "brocket" weapon) at a player just ahead.
// Register-hazard check (tools/regcheck/regcheck2.py, chunk 31): no caller keeps ecx/edx.

#include <cstdint>
#include <cstring>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;

namespace {

struct FxProxy {
    re::Ptr32<void> vtable;
    re::Ptr32<void> effect;
};
static_assert(sizeof(FxProxy) == 8);
struct BonePath {
    int32_t count;
    re::Ptr32<uint32_t> names;
};

GAME_VAR(uint8_t*, gameModeObject, 0x020C8704);
GAME_VAR(void*, effectsManager, 0x00B6F1D4);
GAME_VAR(uint16_t, rocketEffect, 0x020C8FD8);
inline const uint32_t* const rocketBoneName0 = reinterpret_cast<const uint32_t*>(0x01B6531C);
inline const uint32_t* const rocketBoneName1 = reinterpret_cast<const uint32_t*>(0x01B65394);
inline const uint32_t* const rocketBoneName2 = reinterpret_cast<const uint32_t*>(0x01B65440);
inline void* const rocketBoneRoot = reinterpret_cast<void*>(0x01B65398);

GAME_FN(uint8_t, __thiscall, Steering_GoTo, 0x004A5000, uint8_t* steering, float x, float y, float z, uint32_t flags,
        float speed, float radius, uint8_t* entity, int flag);
GAME_FN(uint8_t, __thiscall, Race_IsRacing, 0x00712CA0, uint8_t* race, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, Race_GetRacer, 0x0070D390, uint8_t* race, uint8_t* ped);
GAME_FN(void, __thiscall, Race_GetTarget, 0x0070DA90, uint8_t* race, V3* out, uint8_t* ped, float lookAhead);
GAME_FN(float, __thiscall, Race_GetSpeed, 0x00711EC0, uint8_t* race, uint8_t* ped);
GAME_FN(int32_t, __thiscall, Race_GetPosition, 0x0070D420, uint8_t* race, uint8_t* ped);
GAME_FN(float, __thiscall, Race_711B80, 0x00711B80, uint8_t* race, uint8_t* ped);
GAME_FN(void, __thiscall, Physical_SetMoveSpeed, 0x0041A440, uint8_t* physical, const float* speed);
GAME_FN(void, __thiscall, Physical_SetTurnSpeed, 0x004D3480, uint8_t* physical, const float* speed);
GAME_FN(float, __cdecl, GetCarToGoToCoors, 0x0041A170, uint8_t* vehicle, const V3* target);
GAME_FN(V3*, __cdecl, FindPlayerCentreOfWorld, 0x004469E0, uintptr_t player);
GAME_FN(V3*, __cdecl, FindPlayerSpeed, 0x00445D60);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const V3* v);
GAME_FN(uint8_t*, __thiscall, Ped_46ECE0, 0x0046ECE0, uint8_t* ped);
GAME_FN(uint8_t, __cdecl, Fn42E430, 0x0042E430);
GAME_FN(int32_t, __cdecl, GetGameMode, 0x004A84A0);
GAME_FN(int32_t*, __thiscall, Vehicle_GetClass, 0x004CAF30, uint8_t* vehicle, int32_t* out);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);
GAME_FN(V3*, __thiscall, Entity_GetRight, 0x004736F0, uint8_t* entity, V3* out);
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(uint32_t*, __thiscall, NameKey_FromHash, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, BonePath* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, BonePath* path, void* root);
GAME_FN(void, __thiscall, AE_Proxy_Construct, 0x00668E90, FxProxy* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Destruct, 0x00668EB0, FxProxy* proxy);
GAME_FN(void, __thiscall, AE_Manager_Start, 0x00668290, void* manager, const uint32_t* type, FxProxy* proxy, int flag);
GAME_FN(uint8_t*, __cdecl, Projectile_Fire_x64, 0x004D4D90, uint8_t* shooter, int weapon, re::Float3, re::Float3, uint8_t* bone, float, float, FxProxy* effect, uintptr_t, int, re::Float3, re::Float3, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Projectile_Fire(uint8_t* a0, int a1, float a2, float a3, float a4, float a5, float a6, float a7, uint8_t* a8, float a9, float a10, FxProxy* a11, uintptr_t a12, int a13, float a14, float a15, float a16, float a17, float a18, float a19, int a20, int a21) {
    return Projectile_Fire_x64(a0, a1, re::Float3{a2, a3, a4}, re::Float3{a5, a6, a7}, a8, a9, a10, a11, a12, a13, re::Float3{a14, a15, a16}, re::Float3{a17, a18, a19}, a20, a21);
}
GAME_FN(void, __stdcall, VectorDestruct, 0x004017C0, void* array, size_t size, int count, void* dtor);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(V3*, __cdecl, VectorScale, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(V3*, __cdecl, ScaleVector, 0x00412320, V3* out, float s, const V3* v);
GAME_FN(V3*, __cdecl, Normalised, 0x00472D40, V3* out, const V3* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const V3* a, const V3* b);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(float, __cdecl, DistanceSquared, 0x00472D00, const float* a, const float* b);
GAME_FN(float, __cdecl, MaxF, 0x00414490, float a, float b);
GAME_FN(float, __cdecl, MinF, 0x00414470, float a, float b);
GAME_FN(uint8_t, __cdecl, GetIsLineOfSightClear, 0x004617D0, const float* a, const float* b, int flags);

V3 PosOf(uint8_t* e) {
    const float* p = Pos(e);
    return {p[0], p[1], p[2]};
}
// __ftol2's low byte
uint8_t Gas(float x) {
    return uint8_t(int32_t(int64_t(x)));
}
// the racer's aggression by its place (0..1)
float Aggression(uint8_t* race, uint8_t* ped, uint8_t* racer) {
    float a = float(Race_GetPosition(race, ped)) * 0.05f + At<float>(racer, 0x54);
    return MinF(1.0f, MaxF(0.0f, a));
}
// the collision model's bounding radius (+0x20) of an entity's model
float ModelRadius(uint8_t* e) {
    uint8_t* info = modelInfoPtrs[At<int16_t>(e, 0x10E)];
    return At<float>(At<uint8_t*>(info, 0xC), 0x20);
}
// `length` from `from` toward `to` clear of the world
bool LeadIsClear(const V3& from, const V3& to, float length) {
    V3 d, n, lead, end;
    VectorSub(&d, &to.x, &from.x);
    VectorScale(&lead, Normalised(&n, &d), length);
    return GetIsLineOfSightClear(&from.x, &VectorAdd(&end, &from.x, &lead.x)->x, 1) != 0;
}
// from the ped to the target and to the player, both normalised: their dot
float TargetAlongPlayer(const V3& p, const V3& t, const V3& player) {
    V3 u1, n1, u2, n2;
    VectorSub(&u1, &player.x, &p.x);
    Normalised(&n1, &u1);
    VectorSub(&u2, &t.x, &p.x);
    Normalised(&n2, &u2);
    return DotProduct(&n2, &n1);
}
uint8_t GoTo(uint8_t* ped, const V3& t, uint32_t flags, float speed) {
    return Steering_GoTo(ped + 0x654, t.x, t.y, t.z, flags, speed, 0.3f, nullptr, 0);
}
void CruiseToTarget(uint8_t* vehicle, uint8_t* racer, const V3& t) {
    vehicle[0x2E4] = Gas(At<float>(racer, 0x28));
    vehicle[0x2EC] |= 0x10;
    vehicle[0x2D9] = 2;
    vehicle[0x2DA] = 8;
    GetCarToGoToCoors(vehicle, &t);
}

// a rocket from the cart's launcher bone, ahead of it (carrying its speed when it
// goes forward)
void FireRocket(uint8_t* ped, uint8_t* vehicle, float aggression) {
    int weapon = GetWeaponIndex("brocket");
    uint32_t names[3];
    NameKey_FromHash(&names[0], rocketBoneName0);
    NameKey_FromHash(&names[1], rocketBoneName1);
    NameKey_FromHash(&names[2], rocketBoneName2);
    BonePath path;
    HashPath_Set(&path, 3, names);
    uint8_t* bone = FindBoneInfo(&path, rocketBoneRoot);
    if (bone) {
        uint32_t type;   // (a word; the upper half the aggression's, as the stack had it)
        std::memcpy(&type, &aggression, 4);
        type = (type & 0xFFFF0000u) | rocketEffect;
        FxProxy proxy;
        AE_Proxy_Construct(&proxy);
        AE_Manager_Start(effectsManager, &type, &proxy, 0);
        V3 zeroA = {0.0f, 0.0f, 0.0f};
        V3 zeroB = {0.0f, 0.0f, 0.0f};
        V3 lift = {0.0f, 0.0f, 0.11f};
        const float* at = Pos(vehicle);
        V3 fwd1, aim, fwd2, ahead, from;
        VectorAdd(&aim, &Placeable_GetForward(vehicle, &fwd1)->x, &lift.x);
        ScaleVector(&ahead, 2.0f, Placeable_GetForward(vehicle, &fwd2));
        VectorAdd(&from, at, &ahead.x);
        uint8_t* rocket = Projectile_Fire(ped, weapon, from.x, from.y, from.z, aim.x, aim.y, aim.z, bone, 0.0f, 0.0f,
                                          &proxy, 0, 0, zeroB.x, zeroB.y, zeroB.z, zeroA.x, zeroA.y, zeroA.z, 0, 0);
        AE_Proxy_Destruct(&proxy);
        if (rocket) {
            V3 fwd3;
            if (DotProduct(reinterpret_cast<V3*>(vehicle + 0x124), Placeable_GetForward(vehicle, &fwd3)) > 0.0f) {
                At<float>(rocket, 0x124) = At<float>(vehicle, 0x124) + At<float>(rocket, 0x124);
                At<float>(rocket, 0x128) = At<float>(vehicle, 0x128) + At<float>(rocket, 0x128);
            }
        }
    }
    VectorDestruct(names, 4, 3, reinterpret_cast<void*>(0x0049A650));
}

// a car racer: rams the player just ahead of it (mission 3, style 5; or 3 / 3 when
// behind it), else cruises to its target; eases off ahead of the player and catches up
// behind; a cart fires a rocket at it
uint8_t RaceCar(uint8_t* race, uint8_t* ped, uint8_t* vehicle, uint8_t* racer, const V3& p, const V3& t) {
    uint8_t* player = playerPed;
    if (!player) {
        CruiseToTarget(vehicle, racer, t);
        return 1;
    }
    V3 pc = *FindPlayerCentreOfWorld(0);
    float dsq = DistanceSquared(&p.x, &pc.x);
    float a = Aggression(race, ped, racer);
    bool ramming = false;
    if (0.7f < a && dsq < 100.0f && Vector_MagnitudeSqr(FindPlayerSpeed()) > 0.017f) {
        V3 sn, u, tn;
        Normalised(&sn, FindPlayerSpeed());
        VectorSub(&u, &t.x, &p.x);
        if (1.0f - a < DotProduct(Normalised(&tn, &u), &sn) && 1.0f - (a + a) < TargetAlongPlayer(p, t, pc) &&
            TargetAlongPlayer(p, t, pc) < 0.707f) {
            vehicle[0x2D9] = 3;
            vehicle[0x2DA] = 5;
            vehicle[0x2E4] = Gas(At<float>(racer, 0x24));
            ramming = true;
        }
    }
    if (!ramming && a > 0.45f && dsq < 100.0f && Vector_MagnitudeSqr(FindPlayerSpeed()) > 0.017f) {
        int32_t playerPlace = Race_GetPosition(race, player);
        if (Race_GetPosition(race, ped) > playerPlace && 1.0f - a < TargetAlongPlayer(p, t, pc) &&
            TargetAlongPlayer(p, t, pc) < 0.707f) {
            vehicle[0x2D9] = 3;
            vehicle[0x2DA] = 3;
            vehicle[0x2E4] = Gas(At<float>(racer, 0x24));
            ramming = true;
        }
    }
    if (!ramming) {
        CruiseToTarget(vehicle, racer, t);
    }
    int32_t playerPlace = Race_GetPosition(race, player);
    int32_t place = Race_GetPosition(race, ped);
    float ahead = At<float>(racer, 0x2C);
    if (place > playerPlace && dsq > ahead * ahead) {
        vehicle[0x2E4] = Gas(float(int8_t(vehicle[0x2E4])) * At<float>(racer, 0x30));
    } else {
        playerPlace = Race_GetPosition(race, player);
        place = Race_GetPosition(race, ped);
        float behind = At<float>(racer, 0x34);
        if (place < playerPlace && dsq > behind * behind) {
            vehicle[0x2E4] = Gas(float(int8_t(vehicle[0x2E4])) * At<float>(racer, 0x38));
        }
    }
    int32_t vehicleClass;
    if (Fn42E430() && GetGameMode() == 0xE && *Vehicle_GetClass(vehicle, &vehicleClass) == 10 &&
        (vehicle[0x108] & 0xF8) != 0x28 && Ped_EveryNthFrame(ped, 0x10) && dsq < 200.0f &&
        1.0f - a * 0.3f < TargetAlongPlayer(p, t, pc) && gameModeObject[0x128] == 1 &&
        At<uint32_t>(gameModeObject, 0x12C) + 6000 < timeInMs) {
        FireRocket(ped, vehicle, a);
    }
    return 1;
}

// 0x6236D0 (thiscall): RaceTrack::Update: none outside a race (mode 0xE); before the start
// the ped (and its vehicle) held still (1); none unless racing as a racer. The race's
// target ahead; a car racer by RaceCar; else (a bike or on foot) steered to it at the
// race's speed: a bike's racer rams a rival (0x46ECE0, else the player) close ahead
// (flags 0x1C408, the gas past the rival's lead or its ramming gas) or with it closing
// alongside within 8 swerves aside (style 0x19; else 8); flags 0xC000 when reversing,
// else 0xC010. The steering's result.
uint8_t __fastcall RaceTrack_UpdateImpl(uint8_t* self, re::Edx, float) {
    uint8_t* race = gameModeObject;
    if (!race || !race[4] || At<int32_t>(race, 0x3C) != 0xE || !race) {
        return 0;
    }
    uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x1C));
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    V3 p = PosOf(ped);
    if (!race[0x128]) {
        At<float*>(ped, 0x2E0)[7] = 0.0f;
        float* move = At<float*>(ped, 0x2E0);
        move[0] = zeroVector[0];
        move[1] = zeroVector[1];
        move[2] = zeroVector[2];
        if (vehicle) {
            Physical_SetMoveSpeed(vehicle, zeroVector);
            Physical_SetTurnSpeed(vehicle, zeroVector);
            if (!At<int32_t>(vehicle, 0x46C)) {
                vehicle[0x2DB] = 1;
            }
        }
        return 1;
    }
    if (!Race_IsRacing(race, ped)) {
        return 0;
    }
    uint8_t* racer = Race_GetRacer(race, ped);
    if (!racer) {
        return 0;
    }
    V3 t = {0.0f, 0.0f, 0.0f};
    Race_GetTarget(race, &t, ped, At<float>(ped, 0x524));
    float speed = Race_GetSpeed(race, ped);
    if (vehicle && !At<int32_t>(vehicle, 0x46C)) {
        return RaceCar(race, ped, vehicle, racer, p, t);
    }
    if (!vehicle) {
        return GoTo(ped, t, 0xC010, speed);
    }
    bool swerved = false;
    uint8_t* rival = Ped_46ECE0(ped);
    if (!rival) {
        rival = playerPed;
    }
    if (rival) {
        V3 e = PosOf(rival);
        uint8_t* rivalVehicle = At<uint8_t*>(rival, 0x1554);
        V3 v;
        float rivalRight[4] = {};
        if (rivalVehicle) {
            v = {At<float>(rivalVehicle, 0x124), At<float>(rivalVehicle, 0x128), At<float>(rivalVehicle, 0x12C)};
            const float* m = reinterpret_cast<const float*>(Entity_GetMatrix(rivalVehicle));
            rivalRight[0] = m[0];
            rivalRight[1] = m[1];
            rivalRight[2] = m[2];
            rivalRight[3] = m[3];
        } else {
            v = {At<float>(rival, 0x124), At<float>(rival, 0x128), At<float>(rival, 0x12C)};
        }
        float dsq = DistanceSquared(&p.x, &e.x);
        float a = Aggression(race, ped, racer);
        V3 g = t;
        V3 vp = PosOf(vehicle);
        V3 d, dn;
        VectorSub(&d, &t.x, &p.x);
        Normalised(&dn, &d);
        if (rivalVehicle) {
            if (0.6f < a && dsq < 100.0f && Vector_MagnitudeSqr(&v) > 0.005f) {
                float within = 1.0f - a * 1.5f;
                V3 vn, u, un;
                if (DotProduct(&dn, Normalised(&vn, &v)) > within) {
                    VectorSub(&u, &e.x, &p.x);
                    if (DotProduct(&dn, Normalised(&un, &u)) > within) {
                        // ram: at the rival 30 steps ahead, beside it by both bounds
                        V3 lead, ahead;
                        VectorScale(&lead, reinterpret_cast<V3*>(rivalVehicle + 0x124), 30.0f);
                        VectorAdd(&ahead, Pos(rivalVehicle), &lead.x);
                        t = ahead;
                        float off = ModelRadius(vehicle) + ModelRadius(rivalVehicle) + 0.2f;
                        const float* vm = reinterpret_cast<const float*>(Entity_GetMatrix(vehicle));
                        V3 right = {vm[0], vm[1], vm[2]};
                        V3 rel;
                        VectorSub(&rel, Pos(rivalVehicle), &vp.x);
                        if (DotProduct(&right, &rel) > 0.0f) {
                            g.x = t.x - rivalRight[0] * off;
                            g.y = t.y - rivalRight[1] * off;
                        } else {
                            g.x = rivalRight[0] * off + t.x;
                            g.y = off * rivalRight[1] + t.y;
                        }
                        V3 gp;
                        VectorSub(&gp, &g.x, &p.x);
                        if (DotProduct(&d, &gp) > 0.85f && LeadIsClear(vp, g, 2.0f)) {
                            t = g;
                        }
                        float lag = -Race_711B80(race, ped);
                        vehicle[0x2DA] = 3;
                        if (At<float>(racer, 0x34) < lag) {
                            vehicle[0x2E4] = Gas(speed);
                            return Steering_GoTo(ped + 0x654, t.x, t.y, t.z, 0x1C408, speed, 0.3f, nullptr, 0);
                        }
                        vehicle[0x2E4] = Gas(At<float>(racer, 0x24));
                        return Steering_GoTo(ped + 0x654, t.x, t.y, t.z, 0x1C408,
                                             MaxF(speed, At<float>(racer, 0x24)), 0.3f, nullptr, 0);
                    }
                }
            }
            V3 u, un;
            if (0.5f > a && 8.0f > dsq && Vector_MagnitudeSqr(&v) > 0.005f && DotProduct(&v, &d) > 0.0f &&
                DotProduct(&v, VectorSub(&u, &e.x, &p.x)) < 0.0f &&
                DotProduct(&dn, Normalised(&un, VectorSub(&u, &e.x, &p.x))) > -0.8f &&
                DotProduct(&dn, Normalised(&un, VectorSub(&u, &e.x, &p.x))) < 0.8f) {
                // swerve aside from the rival closing alongside
                float dist = Distance(&t.x, &p.x);
                const float* vm = reinterpret_cast<const float*>(Entity_GetMatrix(vehicle));
                V3 right = {vm[0], vm[1], vm[2]};
                V3 rel;
                VectorSub(&rel, Pos(rivalVehicle), &vp.x);
                bool done = false;
                V3 r, s, end;
                if (DotProduct(&right, &rel) > 0.0f) {
                    VectorScale(&s, Entity_GetRight(vehicle, &r), 2.0f);
                    if (GetIsLineOfSightClear(&vp.x, &VectorSub(&end, &vp.x, &s.x)->x, 1)) {
                        swerved = true;
                        g.x = t.x - rivalRight[0] * dist * 0.3f;
                        g.y = t.y - rivalRight[1] * dist * 0.3f;
                        done = true;
                    }
                }
                if (!done) {
                    VectorScale(&s, Entity_GetRight(vehicle, &r), 2.0f);
                    if (GetIsLineOfSightClear(&vp.x, &VectorAdd(&end, &vp.x, &s.x)->x, 1)) {
                        swerved = true;
                        g.x = rivalRight[0] * dist * 0.3f + t.x;
                        g.y = rivalRight[1] * dist * 0.3f + t.y;
                    }
                }
                if (dist > 6.0f) {
                    V3 gp;
                    if (DotProduct(&d, VectorSub(&gp, &g.x, &p.x)) > 0.92f && LeadIsClear(vp, g, 3.0f)) {
                        t = g;
                    }
                }
            }
        }
    }
    vehicle[0x2DA] = swerved ? 0x19 : 8;
    vehicle[0x2E4] = Gas(speed);
    uint32_t flags = -0.001f > At<float>(vehicle, 0x294) ? 0xC000 : 0xC010;
    return GoTo(ped, t, flags, speed);
}

}  // namespace

#define AIRACETRK_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

AIRACETRK_REVERSED("RaceTrack::Update", 0x006236D0, &RaceTrack_UpdateImpl);
