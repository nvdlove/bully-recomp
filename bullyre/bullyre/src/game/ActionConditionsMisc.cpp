// The action tree's conditions (continued): getting on a vehicle by its seats, weapons
// and dictionaries, the balance beam, vehicle hits, props and points of interest clear
// of peds, allies, nodes playing on others, the mirror, fights, objectives and
// stimuli, tandem helpers, the grappling hook, proxy props: each one's Evaluate(owner)
// (vf1, thiscall), and the scanner's focus setter. 0x6056C0..0x607300.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "reversed.h"

using namespace actc;

namespace {

GAME_ARRAY(float, upVector, 0x009E33DC);                 // (0, 0, 1)
GAME_VAR(uint32_t, nodeSeatLeft_1B65550, 0x01B65550);    // the bikes' seat nodes (name hashes)
GAME_VAR(uint32_t, nodeSeatRight_1B65554, 0x01B65554);
GAME_VAR(CPool<uint8_t>*, txdPool, 0x01B64654);          // CTxdStore::ms_pTxdPool
GAME_VAR(uint8_t, txdFlag_20C8AC1, 0x020C8AC1);
GAME_VAR(uint8_t, mirrorState, 0x00C936F8);              // the reflection manager's (0xFF: none)
GAME_VAR(float, mirrorFacing, 0x00C92D90);
GAME_ARRAY(float, mirrorBoxHigh, 0x00C92D70);            // (the eyes must be below these)
GAME_ARRAY(float, mirrorBoxLow, 0x00C92D80);             // (and above these)
GAME_VAR(float, grappleAnchorUp_1B64D64, 0x01B64D64);
GAME_VAR(int32_t, tandemExcludedA, 0x00BF42D4);          // models that never help
GAME_VAR(int32_t, tandemExcludedB, 0x00BF42D0);
GAME_VAR(int32_t, tandemExcludedC, 0x00BF42CC);
GAME_VAR(int32_t, tandemExcludedD, 0x00BF42DC);

// CMatrix (0x48 bytes)
struct Matrix {
    float right[4];
    float forward[4];
    float up[4];
    float pos[4];
    uint32_t attached;
    uint32_t owns;
};
static_assert(sizeof(Matrix) == 0x48);
// NiTransform
struct Transform {
    float rot[9];  // row major
    float translate[3];
    float scale;
};
static_assert(sizeof(Transform) == 0x34);

GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(uint8_t*, __thiscall, Entity_GetMatrix, 0x008B6120, uint8_t* entity);   // (reads +0x14)
GAME_FN(uint8_t*, __thiscall, Objective_GetTarget, 0x008B6120, uint8_t* objective);   // (the same: its +0x14)
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, Matrix* self, const uint8_t* other);
GAME_FN(uint8_t*, __thiscall, Seats_GetInfo, 0x004CE4C0, uint8_t* seats, int seat);
GAME_FN(V3*, __cdecl, Matrix_Multiply3x3, 0x0041C490, V3* out, const Matrix* m, const float* v);
GAME_FN(uint8_t, __cdecl, GetNodeTransform, 0x00467D80, uint8_t* entity, uint32_t node, Transform* out, int flag);
GAME_FN(uint8_t, __cdecl, ProcessSphereCast, 0x004616A0, const float* start, const float* end, float radius,
        ColPoint* colPoint, re::Ptr32<uint8_t>* entity, int flags, int lists, int);
GAME_FN(uint8_t, __thiscall, Combat_HasRangedWeapon, 0x00486C30, uint8_t* combat);
GAME_FN(int32_t, __cdecl, TxdStore_FindSeasonal, 0x005F2450, uint32_t name, int);
GAME_FN(V3*, __cdecl, Vector_FromAngle, 0x00432DF0, V3* out, float angle);   // (-sin, cos, 0)
GAME_FN(V3*, __cdecl, CrossProduct, 0x00414730, V3* out, const V3* a, const V3* b);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const V3* a, const V3* b);
GAME_FN(uint8_t*, __thiscall, Ped_GetLastHitWeapon, 0x00474BE0, uint8_t* ped);   // +8 where it hit
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(float, __cdecl, LimitAngle, 0x00432EE0, float angle);
GAME_FN(float, __cdecl, HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(float, __cdecl, Distance, 0x0041A950, const float* a, const float* b);
GAME_FN(float, __cdecl, Distance2D, 0x00470110, const float* a, const float* b);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const float* start, const float* end, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(uint8_t, __cdecl, IsAreaClear, 0x0048C990, const float* pos, float radius, int, re::Ptr32<uint8_t>* blocker);
GAME_FN(uint8_t, __thiscall, Ped_EveryNthFrame, 0x00465A60, uint8_t* ped, uint32_t n);
GAME_FN(uint8_t, __thiscall, Ally_IsFriend, 0x00485590, uint8_t* ally, uint8_t* ped);
GAME_FN(uint8_t*, __thiscall, ObjectiveQueue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Scanner_Alert, 0x0048C480, uint8_t* scanner, uint8_t* ped);
GAME_FN(void, __thiscall, Ped_SetTargetEntity, 0x00498B10, uint8_t* ped, uint8_t* target);
GAME_FN(void, __thiscall, Targeting_SetTarget, 0x004D79E0, uint8_t* targeting, uint8_t* target);
GAME_FN(uint8_t, __thiscall, Scanner_IsCooldownOver, 0x0048BFF0, uint8_t* scanner, const uint32_t* node);
GAME_FN(void, __thiscall, Scanner_SetCooldown, 0x0048C1C0, uint8_t* scanner, const uint32_t* node, int32_t ms);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlayingNode, 0x005F48D0, uint8_t* controller, uint8_t* node, int ancestors,
        int nested);
GAME_FN(uint8_t*, __thiscall, Targeting_GetTarget, 0x004D5AD0, uint8_t* targeting);
GAME_FN(uint8_t, __cdecl, CompareInts, 0x005FBCB0, int32_t a, int32_t b, int op);
GAME_FN(V3*, __thiscall, Ped_GetEyePosition, 0x0048D020, uint8_t* social, V3* out);   // (on the ped's +0xA64)
GAME_FN(uint8_t, __thiscall, Ped_IsElderly, 0x0044E160, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Ped_IsRegisteredAsGirlfriend, 0x00473F10, uint8_t* ped);
GAME_FN(uint8_t, __thiscall, Combat_IsInFight, 0x004864E0, uint8_t* combat);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(uint8_t, __thiscall, Scanner_IsIgnored, 0x0048C080, uint8_t* scanner, uint8_t* entity);
GAME_FN(void, __thiscall, Scanner_Ignore, 0x0048C500, uint8_t* scanner, uint8_t* entity);
GAME_FN(void, __thiscall, Scanner_SetFocusGame, 0x00606CA0, uint8_t* scanner, uint8_t* entity);
GAME_FN(uint8_t*, __thiscall, Objective_GetEntity, 0x0077E8E0, uint8_t* objective);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, uint8_t* ped);
GAME_FN(V3*, __thiscall, Placeable_GetPosition, 0x00415460, uint8_t* placeable);
GAME_FN(uint8_t*, __thiscall, Ped_GetGrappleAnchor, 0x00479AC0, uint8_t* ped, int a, int b, int c, int d);
GAME_FN(void, __thiscall, Ped_AnchorGrapple, 0x00479B00, uint8_t* ped, int a, int b, int c, int d);
GAME_FN(float*, __thiscall, Ped_GetMatrix, 0x004734D0, uint8_t* ped);
GAME_FN(float, __thiscall, Vec_Normalise, 0x004294A0, float* v);
GAME_FN(uint8_t, __thiscall, ModelInfo_IsPropAnim, 0x00520430, uint8_t* mi);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(void, __thiscall, PropFinder_Setup, 0x00499330, uint8_t* finder, float range, int flag, float height, int);
GAME_FN(uint8_t, __thiscall, PropFinder_Find, 0x00497B00, uint8_t* finder, int, float range, int, int, int, int);

ColPoint EmptyColPoint() {
    ColPoint cp = {};
    cp.depth = 3.4028235e38f;
    return cp;
}
uint8_t* AsPed(uint8_t* entity) {
    return entity && IsPedEntity(entity) ? entity : nullptr;
}
// a registered reference field set to entity
void SetReference(re::Ptr32<uint8_t>* field, uint8_t* entity) {
    if (entity != *field) {
        if (*field) {
            Entity_CleanUpOldReference(*field, field);
        }
        *field = entity;
        if (entity) {
            RegisterReference(entity, field);
        }
    }
}

// 0x6056C0 / 0x605950: room for a quarter-unit sphere from the ped (one up) to the
// vehicle's seat Seat (a car's seat offset, the second one when the first is unset; a
// bike's seat node)
template <int Seat>
uint8_t __fastcall CanGetOnVehicleSide_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x430);
    if (!vehicle) {
        return 1;
    }
    V3 from;
    VectorAdd(&from, Pos(ped), upVector);
    float* p = Pos(vehicle);
    float to[3] = {p[0], p[1], p[2]};
    Matrix m;
    Transform t;
    if (At<int32_t>(vehicle, 0x46C) == 0) {
        Matrix_CopyConstruct(&m, Entity_GetMatrix(vehicle));
        uint8_t* seats = vehicle + 0x1C4;
        uint8_t* seat = Seats_GetInfo(seats, Seat);
        float offset[3] = {At<float>(seat, 0x24), At<float>(seat, 0x28), At<float>(seat, 0x2C)};
        if (zeroVector[0] == offset[0] && zeroVector[1] == offset[1] && zeroVector[2] == offset[2]) {
            seat = Seats_GetInfo(seats, Seat);
            offset[0] = At<float>(seat, 0x18);
            offset[1] = At<float>(seat, 0x1C);
            offset[2] = At<float>(seat, 0x20);
        }
        V3 rotated;
        V3* r = Matrix_Multiply3x3(&rotated, &m, offset);
        offset[0] = r->x;
        offset[1] = r->y;
        offset[2] = r->z;
        m.pos[0] = m.pos[0] + offset[0];
        m.pos[1] = m.pos[1] + offset[1];
        m.pos[2] = m.pos[2] + offset[2];
        V3 sum;
        V3* s = VectorAdd(&sum, m.pos, upVector);
        to[0] = s->x;
        to[1] = s->y;
        to[2] = s->z;
    } else {
        t.scale = 1.0f;
        GetNodeTransform(vehicle, Seat == 0 ? nodeSeatLeft_1B65550 : nodeSeatRight_1B65554, &t, 1);
        V3 sum;
        V3* s = VectorAdd(&sum, t.translate, upVector);
        to[0] = s->x;
        to[1] = s->y;
        to[2] = s->z;
    }
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit;
    uint8_t blocked = ProcessSphereCast(&from.x, to, 0.25f, &cp, &hit, 9, 0xF, 1);
    uint8_t result = blocked == 0 ? 1 : 0;
    ReleaseColPoint(cp);
    return result;
}

// 0x605BE0: the ped has a ranged weapon
uint8_t __fastcall HasRangedWeapon_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    return Combat_HasRangedWeapon(OwnerPed(owner) + 0x668) != 0 ? 1 : 0;
}

// 0x605C00: the dictionary +4 (seasonal) is loaded (+8: the flag at 0x20C8AC1 is clear)
uint8_t __fastcall HasTXDLoaded_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    if (self[8] != 0) {
        return txdFlag_20C8AC1 == 0 ? 1 : 0;
    }
    int32_t slot = TxdStore_FindSeasonal(At<uint32_t>(self, 4), -1);
    CPool<uint8_t>* pool = txdPool;
    // (a free slot reads its dictionary through null, as the game does)
    uint8_t* def = (pool->flags[slot] & 0x80) ? nullptr : pool->objects + pool->elemSize * slot;
    volatile int32_t* dictionary = reinterpret_cast<volatile int32_t*>(def);
    return *dictionary != 0 ? 1 : 0;
}

// 0x605C50: the ped's sphere hit (+0x2E0: +0x408 set, +0x464 which) is +4 (1-based)
uint8_t __fastcall PedSphereHit_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* actions = At<uint8_t*>(ped, 0x2E0);
    return actions[0x408] != 0 && At<int32_t>(self, 4) == int32_t(int8_t(actions[0x464])) + 1 ? 1 : 0;
}

// the balance beam's sideways offset: the player's (+0x1EA0 - +0x1EB0) along the beam's
// side (the beam's heading +0x1EAC crossed with up)
float BeamLateral(uint8_t* player) {
    V3 dir;
    Vector_FromAngle(&dir, At<float>(player, 0x1EAC));
    V3 up = {0.0f, 0.0f, 1.0f};
    V3 side;
    CrossProduct(&side, &dir, &up);
    V3 d;
    VectorSub(&d, reinterpret_cast<float*>(player + 0x1EA0), reinterpret_cast<float*>(player + 0x1EB0));
    d.z = 0.0f;
    return DotProduct(&side, &d);
}

// 0x605C90: the player on the beam (+0x1548; +0x14F8: always) is off its middle by more
// or less (+8) than |+4| (+4 made absolute)
uint8_t __fastcall BeamLateralDeviation_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* player = playerPed;
    uint8_t* ped = OwnerPed(owner);
    if (!ped || !player || ped != player) {
        return 0;
    }
    if (At<int32_t>(player, 0x14F8) != 0) {
        return 1;
    }
    if (At<int32_t>(player, 0x1548) == 0) {
        return 0;
    }
    float deviation = std::fabs(BeamLateral(player));
    float limit = std::fabs(At<float>(self, 4));
    At<float>(self, 4) = limit;
    return CompareOp(deviation, limit, At<int32_t>(self, 8));
}

// 0x605D90: the player on the beam is on its left (+4 = 0) / right (+4 = 1) side
uint8_t __fastcall BeamLateralPosition_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* player = playerPed;
    uint8_t* ped = OwnerPed(owner);
    if (!ped || !player || ped != player) {
        return 0;
    }
    if (At<int32_t>(player, 0x14F8) != 0) {
        return 1;
    }
    if (At<int32_t>(player, 0x1548) == 0) {
        return 0;
    }
    float lateral = BeamLateral(player);
    switch (At<int32_t>(self, 4)) {
    case 0:
        return !(0.0f < lateral) && !std::isnan(lateral) ? 1 : 0;
    case 1:
        return 0.0f < lateral ? 1 : 0;
    default:
        return 0;
    }
}

// 0x605EA0: the angle of the ped's last hit (its position) around the vehicle's centre
// (a bike's "CenterOMass" node; else half its height up) is between +4 and +8 (the
// other way round when +4 >= +8); +0xC: the angle in the vehicle's frame (+0xD: the
// bike's node's axes), else the heading difference to the ped's forward
uint8_t __fastcall VehicleHitOrientation_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
    uint8_t* record = Ped_GetLastHitWeapon(ped);
    if (!vehicle) {
        return 0;
    }
    Transform t;
    t.scale = 1.0f;
    float center[3] = {0.0f, 0.0f, 0.0f};
    uint8_t* m = Entity_GetMatrix(ped);
    V3 forward = {At<float>(m, 0x10), At<float>(m, 0x14), At<float>(m, 0x18)};
    uint8_t* bike = nullptr;
    if ((vehicle[0x108] & 7) == 2 && At<int32_t>(vehicle, 0x46C) == 1) {
        uint32_t key;
        NameKey_Construct(&key, "CenterOMass");
        bike = vehicle;
        GetNodeTransform(vehicle, key, &t, 1);
        center[0] = t.translate[0];
        center[1] = t.translate[1];
        center[2] = t.translate[2];
        NameKey_Destruct(&key);
    } else {
        float* p = Pos(vehicle);
        center[0] = p[0];
        center[1] = p[1];
        center[2] = p[2];
        center[2] = vc::Call<float>(vehicle, 0x6C) * 0.5f + center[2];
    }
    V3 up = {0.0f, 0.0f, 1.0f};
    V3 toHit;
    VectorSub(&toHit, reinterpret_cast<float*>(record + 8), center);
    float angle;
    if (self[0xC] != 0) {
        if (self[0xD] != 0 && bike) {
            forward = {t.rot[1], t.rot[4], t.rot[7]};
            up = {t.rot[2], t.rot[5], t.rot[8]};
        } else {
            forward.z = 0.0f;
            Vector_Normalise(&forward);
        }
        float alongUp = toHit.y * up.y + toHit.x * up.x + toHit.z * up.z;
        float alongForward = toHit.y * forward.y + toHit.x * forward.x + toHit.z * forward.z;
        angle = LimitAngle(float(std::atan2(double(alongUp), double(alongForward))));
    } else if (bike) {
        float alongRight = t.rot[3] * toHit.y + t.rot[0] * toHit.x + t.rot[6] * toHit.z;
        float alongForward = t.rot[4] * toHit.y + t.rot[1] * toHit.x + t.rot[7] * toHit.z;
        angle = LimitAngle(float(std::atan2(double(alongRight), double(alongForward))));
    } else {
        double heading = HeadingFromVector(&forward);
        angle = float(heading - HeadingFromVector(&toHit));
    }
    float low;
    float high;
    uint8_t ordered;
    if (At<float>(self, 8) > At<float>(self, 4)) {
        low = At<float>(self, 4);
        high = At<float>(self, 8);
        ordered = 1;
    } else {
        low = At<float>(self, 8);
        high = At<float>(self, 4);
        ordered = 0;
    }
    if (low < angle && high > angle) {
        return ordered;
    }
    return ordered == 0 ? 1 : 0;
}

// 0x6061F0: no nearby ped within +8 of the ped's prop's (+0x438) node +4 (else the ped's
// +0x43C)
uint8_t __fastcall TargetPropClearOfPeds_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* prop = At<uint8_t*>(OwnerPed(owner), 0x438);
    if (!prop) {
        return 0;
    }
    uint32_t node = At<uint32_t>(self, 4);
    if (node == 0) {
        node = At<uint32_t>(OwnerPed(owner), 0x43C);
        if (node == 0) {
            return 0;
        }
    }
    Transform t;
    t.scale = 1.0f;
    GetNodeTransform(prop, node, &t, 1);
    for (int i = 0; i < At<int32_t>(OwnerPed(owner), 0x3C0); i++) {
        uint8_t* other = At<uint8_t*>(OwnerPed(owner), 0x3C4 + i * 4);
        if (other) {
            float at[3] = {t.translate[0], t.translate[1], t.translate[2]};
            float range = At<float>(self, 8);
            if (range > Distance(Pos(other), at)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x6062D0: the ped's point of interest (+0x1D24) beyond +4 (2D), or seen from the
// ped's feet (z 1) and clear of others within a quarter unit
uint8_t __fastcall POIClearOfPeds_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* poi = At<uint8_t*>(ped, 0x1D24);
    if (!poi) {
        return 1;
    }
    float d = Distance2D(Pos(ped), Pos(poi));
    float range = At<float>(self, 4);
    if (range < d || range == d) {
        return 1;
    }
    float* p = Pos(OwnerPed(owner));
    float from[3] = {p[0], p[1], 1.0f};
    ColPoint cp = EmptyColPoint();
    re::Ptr32<uint8_t> hit = nullptr;
    uint8_t result = 0;
    if (!ProcessLineOfSight(from, Pos(poi), &cp, &hit, 4, 0, 0) || hit == OwnerPed(owner)) {
        if (IsAreaClear(Pos(poi), 0.25f, 1, &hit) || hit == OwnerPed(owner)) {
            result = 1;
        }
    }
    ReleaseColPoint(cp);
    return result;
}

// 0x6064C0: every 4th frame, the ped leading allies (+0x2F4, +0x304) finds a nearby
// ped (not +0x1C98) whose target is a friend of the ped's and who is fighting it
// (objective 7 or 16): it's alerted to, targeted and aimed at
uint8_t __fastcall AllyIsInCombat_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!Ped_EveryNthFrame(ped, 4) || !ped || !At<uint8_t*>(ped, 0x2F4) || ped[0x304] == 0) {
        return 0;
    }
    for (int i = 0; i < At<int32_t>(ped, 0x3C0); i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (!other || other[0x1C98] != 0) {
            continue;
        }
        uint8_t* target = AsPed(Ped_GetTarget(other));
        if (!Ally_IsFriend(ped + 0x2F0, target)) {
            continue;
        }
        uint8_t* objective = ObjectiveQueue_GetCurrent(other + 0x5F0);
        if (objective && (At<int32_t>(objective, 8) == 7 || At<int32_t>(objective, 8) == 16)) {
            Scanner_Alert(ped + 0x3BC, other);
            Ped_SetTargetEntity(ped, other);
            Targeting_SetTarget(At<uint8_t*>(ped, 0x1C8), other);
            return 1;
        }
    }
    return 0;
}

// 0x6065B0 / 0x6065D0: the ally state (+0x318) / morale (+0x31C) is +4
template <int Offset>
uint8_t __fastcall PedDwordIs_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return At<int32_t>(OwnerPed(owner), Offset) == At<int32_t>(self, 4) ? 1 : 0;
}

// 0x6065F0: the ped grapples someone (+0x155C)
uint8_t __fastcall HasGrappleTarget_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    return ped && At<uint8_t*>(ped, 0x155C) ? 1 : 0;
}

// 0x606620: the owner's node (+0x34) is off cooldown for the ped's scanner; restarts
// the cooldown (+4 seconds)
uint8_t __fastcall HasNodeCooldown_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* node = At<uint8_t*>(owner, 0x34);
    if (!ped || !node) {
        return 0;
    }
    uint8_t* scanner = ped + 0x3BC;
    const uint32_t* name = vc::Call<const uint32_t*>(node, acttree::kNodeGetName);
    uint8_t over = Scanner_IsCooldownOver(scanner, name);
    if (over) {
        Scanner_SetCooldown(scanner, name, int32_t(int64_t(double(At<float>(self, 4)) * 1000.0)));
    }
    return over;
}

// the other peds of the ped's faction within +4 (2D) playing the owner's node (+0x34)
template <uint8_t Found>
uint8_t __fastcall ThisNodeIsPlaying_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    CPool<uint8_t>* pool = pedPool;
    uint8_t* node = At<uint8_t*>(owner, 0x34);
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * i;
        uint8_t* ped = OwnerPed(owner);
        if (!other || other == ped || At<int32_t>(ped, 0x1310) != At<int32_t>(other, 0x1310)) {
            continue;
        }
        V3 d;
        VectorSub(&d, Pos(other), Pos(ped));
        d.z = 0.0f;
        float dist = SqrtAbs(d.x * d.x + d.y * d.y + d.z * d.z);
        if (At<float>(self, 4) < dist) {
            continue;
        }
        if (Ctl_IsPlayingNode(At<uint8_t*>(other, 0x2E0) + 0x258, node, 1, 1)) {
            return Found;
        }
    }
    return Found ? 0 : 1;
}

// 0x6068F0: the player targets a ped, or its targeting system has a target
uint8_t __fastcall PlayerHasTarget_Evaluate(uint8_t*, re::Edx, uint8_t*) {
    if (!playerPed) {
        return 0;
    }
    uint8_t* target = AsPed(Ped_GetTarget(playerPed));
    uint8_t* aimed = Targeting_GetTarget(At<uint8_t*>(playerPed, 0x1C8));
    return target || aimed ? 1 : 0;
}

// 0x606950: the number of authority peds (factions 0, 7, 8) targeting the ped compared
// (+8) with +4
uint8_t __fastcall NumAuthorityInPursuit_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    CPool<uint8_t>* pool = pedPool;
    if (!OwnerPed(owner)) {
        return 0;
    }
    int32_t count = 0;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * i;
        if (!other) {
            continue;
        }
        uint8_t* target = Ped_GetTarget(other);
        if (!target || !IsPedEntity(target)) {
            continue;
        }
        int32_t faction = At<int32_t>(other, 0x1310);
        if ((faction == 0 || faction == 8 || faction == 7) && target == OwnerPed(owner)) {
            count++;
        }
    }
    return CompareInts(count, At<int32_t>(self, 4), At<int32_t>(self, 8));
}

// 0x6069F0: the player's eyes inside the mirror's box, the mirror within +4 degrees of
// facing
uint8_t __fastcall IsPlayerFacingMirror_Evaluate(uint8_t* self, re::Edx, uint8_t*) {
    if (mirrorState == 0xFF) {
        return 0;
    }
    double facing = mirrorFacing;
    float c = float(std::cos(double(float(double(At<float>(self, 4)) * double(3.14159274f) / 180.0))));
    if (!(-c > facing)) {
        return 0;
    }
    float low[3] = {mirrorBoxLow[0], mirrorBoxLow[1], mirrorBoxLow[2]};
    float high[3] = {mirrorBoxHigh[0], mirrorBoxHigh[1], mirrorBoxHigh[2]};
    V3 eyes;
    Ped_GetEyePosition(playerPed + 0xA64, &eyes);
    if (!(eyes.x > low[0]) || !(high[0] > eyes.x)) {
        return 0;
    }
    if (!(eyes.y > low[1]) || !(high[1] > eyes.y)) {
        return 0;
    }
    if (!(eyes.z > low[2]) || !(high[2] > eyes.z)) {
        return 0;
    }
    return 1;
}

// 0x606B00 / 0x606B20: queries of the ped's
uint8_t __fastcall IsElderly_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    return ped ? Ped_IsElderly(ped) : 0;
}
uint8_t __fastcall IsRegisteredAsGirlfriend_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    return ped ? Ped_IsRegisteredAsGirlfriend(ped) : 0;
}

// 0x606B40: fighting (or +0x9A4)
uint8_t __fastcall IsInFight_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!Combat_IsInFight(ped + 0x668) && ped[0x9A4] == 0) {
        return 0;
    }
    return 1;
}

// 0x606B70: avoiding any of the kinds +4..+8 asks for (+0x1251 bits 1..4, 6)
uint8_t __fastcall IsAvoiding_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t flags = OwnerPed(owner)[0x1251];
    if ((self[4] != 0 && (flags & 2)) || (self[5] != 0 && (flags & 4)) || (self[6] != 0 && (flags & 8)) ||
        (self[7] != 0 && (flags & 0x10)) || (self[8] != 0 && (flags & 0x40))) {
        return 1;
    }
    return 0;
}

// 0x606BD0: the peds (not +0x1C98) attacking the ped (objective 7) compared (+8) with +4:
// true as soon as the count so far compares true
uint8_t __fastcall PedHasNumAttackers_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    CPool<uint8_t>* pool = pedPool;
    uint8_t* ped = OwnerPed(owner);
    int32_t count = 0;
    for (int i = pool->size; i != 0;) {
        i--;
        if (pool->flags[i] & 0x80) {
            continue;
        }
        uint8_t* other = pool->objects + pool->elemSize * i;
        if (!other || other[0x1C98] != 0) {
            continue;
        }
        uint8_t* objective = ObjectiveQueue_GetCurrent(other + 0x5F0);
        if (objective && At<int32_t>(objective, 8) == 7 && Objective_GetTarget(objective) == ped) {
            count++;
        }
        if (CompareInts(count, At<int32_t>(self, 4), At<int32_t>(self, 8))) {
            return 1;
        }
    }
    return 0;
}

// 0x606C60: the ped uses a door (+0x1DD4: an object whose +0x184 is set)
uint8_t __fastcall IsUsingDoor_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* door = At<uint8_t*>(ped, 0x1DD4);
    return door && (door[0x108] & 7) == 6 && At<int32_t>(door, 0x184) != 0 ? 1 : 0;
}

// 0x606CA0 (thiscall): the scanner's focus (+0x188, a registered reference)
void __fastcall Scanner_SetFocus(uint8_t* scanner, re::Edx, uint8_t* entity) {
    SetReference(&At<uint8_t*>(scanner, 0x188), entity);
}

// 0x606CE0: the owner's attack record (+0x10) is within +4 seconds: a non-player ped
// alerts to its attacker and focuses on it (unless it ignores it)
uint8_t __fastcall Attacked_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* record = At<uint8_t*>(owner, 0x10);
    uint8_t recent = 0;
    if (At<float>(record, 0x2C) != -1.0f && At<float>(record, 0x2C) <= At<float>(self, 4)) {
        recent = 1;
    }
    uint8_t* attacker = At<uint8_t*>(record, 4);
    if (recent && attacker && At<int32_t>(OwnerPed(owner), 0x1310) != 13) {
        uint8_t* scanner = OwnerPed(owner) + 0x3BC;
        if (!Scanner_IsIgnored(scanner, attacker)) {
            Scanner_Ignore(scanner, attacker);
            Scanner_Alert(scanner, attacker);
            Scanner_SetFocusGame(scanner, attacker);
        }
    }
    return recent;
}

// 0x606D60: the ped's current objective is of type +4; fights (7, 16) alert to their
// target, 8 focuses the scanner on its entity
uint8_t __fastcall Objective_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* objective = ObjectiveQueue_GetCurrent(OwnerPed(owner) + 0x5F0);
    if (!objective) {
        return 0;
    }
    int32_t type = At<int32_t>(objective, 8);
    uint8_t match = At<int32_t>(self, 4) == type ? 1 : 0;
    if (!match) {
        return match;
    }
    uint8_t* fight = type == 7 ? objective : nullptr;
    uint8_t* fight16 = type == 16 ? objective : nullptr;
    uint8_t* use = type == 8 ? objective : nullptr;
    if (fight || fight16) {
        uint8_t* target = Objective_GetTarget(fight ? fight : fight16);
        if (target) {
            Scanner_Alert(OwnerPed(owner) + 0x3BC, target);
        }
        return match;
    }
    if (use) {
        uint8_t* entity = Objective_GetEntity(use);
        if (entity) {
            Scanner_SetFocusGame(OwnerPed(owner) + 0x3BC, entity);
        }
    }
    return match;
}

// 0x606E20 / 0x606E50: the stimulus's (+0x1D30) subject (+4) / object (+8) becomes the
// scanner's focus
template <int Offset>
uint8_t __fastcall TargetStimulusEntity_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* stimulus = At<uint8_t*>(OwnerPed(owner), 0x1D30);
    if (!stimulus) {
        return 0;
    }
    uint8_t* entity = At<uint8_t*>(stimulus, Offset);
    if (!entity) {
        return 0;
    }
    Scanner_SetFocusGame(OwnerPed(owner) + 0x3BC, entity);
    return 1;
}

// 0x606E80: a helper for tandem moves (+0x9B8): the nearest (2D) nearby friendly ped
// with health, not of the excluded models, with the same target and fighting (7);
// the two reference each other (+0x9B4 / +0x9B8)
uint8_t __fastcall FindTandemHelper_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t* helper = At<uint8_t*>(ped, 0x9B8);
    if (helper && helper[0x1C98] == 0) {
        return 1;
    }
    uint8_t* best = nullptr;
    float bestDist = 0.0f;
    if (At<int32_t>(ped, 0x3C0) <= 0) {
        return 0;
    }
    for (int i = 0; i < At<int32_t>(ped, 0x3C0); i++) {
        uint8_t* other = At<uint8_t*>(ped, 0x3C4 + i * 4);
        if (!other) {
            continue;
        }
        bool friendly = At<int32_t>(other, 0x1310) == At<int32_t>(ped, 0x1310) || Ally_IsFriend(other + 0x2F0, ped);
        bool able = false;
        if (Ped_GetModelInfo(other)[0x50] == 0 && 0.0f < At<float>(other, 0x1CB8)) {
            int32_t model = At<int16_t>(other, 0x10E);
            able = model != tandemExcludedA && model != tandemExcludedB && model != tandemExcludedC &&
                   model != tandemExcludedD;
        }
        if (!friendly || !able) {
            continue;
        }
        if (Ped_GetTarget(other) != Ped_GetTarget(ped)) {
            continue;
        }
        uint8_t* objective = ObjectiveQueue_GetCurrent(other + 0x5F0);
        if (!objective || At<int32_t>(objective, 8) != 7) {
            continue;
        }
        V3 d;
        V3* v = VectorSub(&d, reinterpret_cast<float*>(Placeable_GetPosition(ped)), Pos(other));
        float dist = v->x * v->x + v->y * v->y;
        if (!best || bestDist > dist) {
            bestDist = dist;
            best = other;
        }
    }
    if (!best) {
        return 0;
    }
    SetReference(&At<uint8_t*>(best, 0x9B4), ped);
    re::Ptr32<uint8_t>* field = &At<uint8_t*>(ped, 0x9B8);
    if (best != *field) {
        if (*field) {
            Entity_CleanUpOldReference(*field, field);
        }
        *field = best;
        RegisterReference(best, field);
    }
    return 1;
}

// 0x607060: the ped helps someone (+0x9B4) who still has it as helper (else the
// reference is dropped)
uint8_t __fastcall IsTandemHelper_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    re::Ptr32<uint8_t>* field = &At<uint8_t*>(OwnerPed(owner), 0x9B4);
    uint8_t* helped = *field;
    if (helped && At<uint8_t*>(helped, 0x9B8) == nullptr && helped) {
        Entity_CleanUpOldReference(helped, field);
        *field = nullptr;
    }
    return helped != nullptr ? 1 : 0;
}

// 0x6070B0: the grappling hook's anchor (+0xC..+0xF pick it) faces up enough and its
// height above the ped compares (+8) with +4: anchors it and turns the ped upright
uint8_t __fastcall GrapplingHookAnchord_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    float z = Pos(OwnerPed(owner))[2];
    uint8_t* anchor = Ped_GetGrappleAnchor(OwnerPed(owner), self[0xC], self[0xD], self[0xE], self[0xF]);
    float height = At<float>(anchor, 0x38) - z;
    anchor = Ped_GetGrappleAnchor(OwnerPed(owner), self[0xC], self[0xD], self[0xE], self[0xF]);
    float n[3] = {At<float>(anchor, 0x20), At<float>(anchor, 0x24), At<float>(anchor, 0x28)};
    float upness = n[1] * 0.0f + 0.0f * n[0] + n[2];
    if (!(grappleAnchorUp_1B64D64 < upness)) {
        return 0;
    }
    if (!CompareOp(height, At<float>(self, 4), At<int32_t>(self, 8))) {
        return 0;
    }
    uint8_t* ped = OwnerPed(owner);
    Ped_AnchorGrapple(ped, self[0xC], self[0xD], self[0xE], self[0xF]);
    float* m = Ped_GetMatrix(ped);
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = 1.0f;
    // (the game also copies stack garbage into the padding m[11], and later m[7])
    float right[3] = {m[5] * m[10] - m[6] * m[9], m[6] * m[8] - m[10] * m[4], m[9] * m[4] - m[5] * m[8]};
    Vec_Normalise(right);
    m[0] = right[0];
    m[1] = right[1];
    m[2] = right[2];
    float forward[3] = {m[2] * m[9] - m[1] * m[10], m[0] * m[10] - m[2] * m[8], m[8] * m[1] - m[9] * m[0]};
    Vec_Normalise(forward);
    m[4] = forward[0];
    m[5] = forward[1];
    m[6] = forward[2];
    return 1;
}

// 0x6072E0: spotted by a nearby ped of kind +4
uint8_t __fastcall Spotted_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    return reinterpret_cast<uint8_t(__cdecl*)(uint8_t*, int32_t)>(0x00602F20)(OwnerPed(owner), At<int32_t>(self, 4));
}

// 0x607300: +8: the ped's prop (an animated one) nearer than +4 (+9: and below +0xC);
// else the prop finder finds one within +4
uint8_t __fastcall TargetProxyProp_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    if (self[8] != 0 && At<uint8_t*>(OwnerPed(owner), 0x438)) {
        uint8_t* prop = At<uint8_t*>(OwnerPed(owner), 0x438);
        if ((prop[0x108] & 7) != 6 || !ModelInfo_IsPropAnim(modelInfoPtrs[At<int16_t>(prop, 0x10E)])) {
            return 0;
        }
        uint8_t* ped = OwnerPed(owner);
        V3 d;
        VectorSub(&d, Pos(At<uint8_t*>(ped, 0x438)), Pos(ped));
        float dist = Vector_Magnitude(&d);
        if (!(At<float>(self, 4) > dist)) {
            return 0;
        }
        bool below = At<float>(self, 0xC) > d.z;
        if (self[9] != 0 && !below) {
            return 0;
        }
        return 1;
    }
    uint8_t* ped = OwnerPed(owner);
    PropFinder_Setup(ped + 0x34C, At<float>(self, 4), self[9], At<float>(self, 0xC), 0);
    return PropFinder_Find(OwnerPed(owner) + 0x34C, 0, At<float>(self, 4), -1, 0, 0, 0) != 0 ? 1 : 0;
}

} // namespace

#define ACTCONDMISC_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

ACTCONDMISC_REVERSED("ConditionCanGetOnVehicleLeftSide::Evaluate", 0x006056C0, &CanGetOnVehicleSide_Evaluate<0>);
ACTCONDMISC_REVERSED("ConditionCanGetOnVehicleRightSide::Evaluate", 0x00605950, &CanGetOnVehicleSide_Evaluate<1>);
ACTCONDMISC_REVERSED("ConditionHasRangedWeapon::Evaluate", 0x00605BE0, &HasRangedWeapon_Evaluate);
ACTCONDMISC_REVERSED("ConditionHasTXDLoaded::Evaluate", 0x00605C00, &HasTXDLoaded_Evaluate);
ACTCONDMISC_REVERSED("ConditionPedSphereHit::Evaluate", 0x00605C50, &PedSphereHit_Evaluate);
ACTCONDMISC_REVERSED("ConditionBeamLateralDeviation::Evaluate", 0x00605C90, &BeamLateralDeviation_Evaluate);
ACTCONDMISC_REVERSED("ConditionBeamLateralPosition::Evaluate", 0x00605D90, &BeamLateralPosition_Evaluate);
ACTCONDMISC_REVERSED("ConditionVehicleHitOrientation::Evaluate", 0x00605EA0, &VehicleHitOrientation_Evaluate);
ACTCONDMISC_REVERSED("ConditionTargetPropClearOfPeds::Evaluate", 0x006061F0, &TargetPropClearOfPeds_Evaluate);
ACTCONDMISC_REVERSED("ConditionPOIClearOfPeds::Evaluate", 0x006062D0, &POIClearOfPeds_Evaluate);
ACTCONDMISC_REVERSED("ConditionAllyIsInCombat::Evaluate", 0x006064C0, &AllyIsInCombat_Evaluate);
ACTCONDMISC_REVERSED("ConditionAllyState::Evaluate", 0x006065B0, &PedDwordIs_Evaluate<0x318>);
ACTCONDMISC_REVERSED("ConditionAllyMorale::Evaluate", 0x006065D0, &PedDwordIs_Evaluate<0x31C>);
ACTCONDMISC_REVERSED("ConditionHasGrappleTarget::Evaluate", 0x006065F0, &HasGrappleTarget_Evaluate);
ACTCONDMISC_REVERSED("ConditionHasNodeCooldown::Evaluate", 0x00606620, &HasNodeCooldown_Evaluate);
ACTCONDMISC_REVERSED("ConditionThisNodeIsPlaying::Evaluate", 0x006066B0, &ThisNodeIsPlaying_Evaluate<1>);
ACTCONDMISC_REVERSED("ConditionThisNodeIsNotPlaying::Evaluate", 0x006067D0, &ThisNodeIsPlaying_Evaluate<0>);
ACTCONDMISC_REVERSED("ConditionPlayerHasTarget::Evaluate", 0x006068F0, &PlayerHasTarget_Evaluate);
ACTCONDMISC_REVERSED("ConditionNumAuthorityInPursuit::Evaluate", 0x00606950, &NumAuthorityInPursuit_Evaluate);
ACTCONDMISC_REVERSED("ConditionIsPlayerFacingMirror::Evaluate", 0x006069F0, &IsPlayerFacingMirror_Evaluate);
ACTCONDMISC_REVERSED("ConditionIsElderly::Evaluate", 0x00606B00, &IsElderly_Evaluate);
ACTCONDMISC_REVERSED("ConditionIsRegisteredAsGirlfriend::Evaluate", 0x00606B20, &IsRegisteredAsGirlfriend_Evaluate);
ACTCONDMISC_REVERSED("ConditionIsInFight::Evaluate", 0x00606B40, &IsInFight_Evaluate);
ACTCONDMISC_REVERSED("ConditionIsAvoiding::Evaluate", 0x00606B70, &IsAvoiding_Evaluate);
ACTCONDMISC_REVERSED("ConditionPedHasNumAttackers::Evaluate", 0x00606BD0, &PedHasNumAttackers_Evaluate);
ACTCONDMISC_REVERSED("ConditionIsUsingDoor::Evaluate", 0x00606C60, &IsUsingDoor_Evaluate);
ACTCONDMISC_REVERSED("PedScanner::SetFocus_606CA0", 0x00606CA0, &Scanner_SetFocus);
ACTCONDMISC_REVERSED("ConditionAttacked::Evaluate", 0x00606CE0, &Attacked_Evaluate);
ACTCONDMISC_REVERSED("ConditionObjective::Evaluate", 0x00606D60, &Objective_Evaluate);
ACTCONDMISC_REVERSED("ConditionTargetStimulusEntitySubject::Evaluate", 0x00606E20, &TargetStimulusEntity_Evaluate<4>);
ACTCONDMISC_REVERSED("ConditionTargetStimulusEntityObject::Evaluate", 0x00606E50, &TargetStimulusEntity_Evaluate<8>);
ACTCONDMISC_REVERSED("ConditionFindTandemHelper::Evaluate", 0x00606E80, &FindTandemHelper_Evaluate);
ACTCONDMISC_REVERSED("ConditionIsTandemHelper::Evaluate", 0x00607060, &IsTandemHelper_Evaluate);
ACTCONDMISC_REVERSED("ConditionGrapplingHookAnchord::Evaluate", 0x006070B0, &GrapplingHookAnchord_Evaluate);
ACTCONDMISC_REVERSED("ConditionSpotted::Evaluate", 0x006072E0, &Spotted_Evaluate);
ACTCONDMISC_REVERSED("ConditionTargetProxyProp::Evaluate", 0x00607300, &TargetProxyProp_Evaluate);
