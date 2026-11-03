// The action tree's conditions of 0x65C270..0x65D210: targeting a projectile or a prop
// (a pickup, a projectile, an object near a bone, the prop finder's), the target's prop,
// and catching a projectile: the catch zone (0x65BA70 builds it) tested against a point
// and against a projectile's path this frame.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/PropTracks.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;

namespace {

// the catch zone (0x24 bytes; WeaponConditions2.cpp)
struct CatchZone {
    V3 pos;       // +0x0
    V3 fwd;       // +0xC
    float reach;  // +0x18
    float yaw;    // +0x1C
    float pitch;  // +0x20
};
static_assert(sizeof(CatchZone) == 0x24);

// a pickup (0x48 bytes, at 0xC0A100)
struct Pickup {
    uint8_t pad0[0x1C];
    re::Ptr32<uint8_t> entity;    // +0x1C
    uint8_t pad20[0x10];
    int32_t model;      // +0x30
    int32_t objective;  // +0x34 the objective it's for
    uint8_t pad38[0xE];
    uint8_t flags;      // +0x46 (4: not to be taken)
    uint8_t pad47;
};
static_assert(sizeof(Pickup) == 0x48);

GAME_FN(void, __cdecl, CModelInfo_GetModelInfoInRange, 0x0051C260, const char* name, int* id, int first, int last);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, uint8_t* ped);
GAME_FN(int, __cdecl, Projectile_FindNearest, 0x004D4140, float x, float y, float z, float a4, float a5, int model,
        uint8_t* source, int armedOnly);
GAME_FN(uint8_t*, __cdecl, GetProjectile, 0x004D3DD0, int index);
GAME_FN(int32_t, __cdecl, GetProjectileModel, 0x004D3650, int index);
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(void, __thiscall, NamedRef_Set, 0x00471300, uint8_t* ref, uint8_t* entity, int value);
GAME_FN(void, __thiscall, Scanner_SetFocus, 0x00606CA0, uint8_t* scanner, uint8_t* entity);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(uint8_t*, __thiscall, Queue_GetCurrent, 0x00470340, uint8_t* queue);
GAME_FN(void, __thiscall, Queue_RemoveFirst, 0x004702C0, uint8_t* queue);
GAME_FN(void, __cdecl, Pickups_FindInRange, 0x004440F0, const V3* pos, float radius, int sort);
GAME_FN(uint8_t*, __cdecl, GetWeaponInfo, 0x0051B450, int weapon);
GAME_FN(int, __cdecl, GetWeaponIndexByHash, 0x0051AF00, uint32_t hash);
GAME_FN(uint8_t, __thiscall, Ped_HasAttackResponse, 0x00478090, uint8_t* ped, int weapon);
GAME_FN(int, __cdecl, BoneTable_Count, 0x005191C0);
GAME_FN(int, __cdecl, BoneTable_Get, 0x00520380, int index);
GAME_FN(uint8_t*, __cdecl, FindNearestObjectOfTypeInRange, 0x004607A0, int model, const float* pos, float range,
        uint32_t lists);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(uint8_t, __cdecl, IsOnWalkable2, 0x005EB850, const float* unused, const V3* pos, void* ref);
GAME_FN(void, __thiscall, PropFinder_Setup, 0x00499330, uint8_t* finder, float range, int flag, float height, int);
GAME_FN(uint8_t, __thiscall, PropFinder_Find, 0x00497B00, uint8_t* finder, int, float range, int, int, int, int);
GAME_FN(void, __cdecl, RegisterReference, 0x004657D0, uint8_t* entity, re::Ptr32<uint8_t>* ref);
GAME_FN(float, __cdecl, GetAngleOfVector2D, 0x00433190, const V3* v);
GAME_FN(float, __cdecl, GetATanOfXY, 0x00432FE0, float x, float y);
GAME_FN(float, __cdecl, AngleDifference, 0x00432980, float a, float b);
GAME_FN(void, __thiscall, ColSphere_Set, 0x0056EBA0, float* sphere, float radius, const V3* center, int surface, int piece);
GAME_FN(uint8_t, __thiscall, ColSphere_IntersectRay, 0x0056EBD0, float* sphere, const V3* origin, const V3* dir, V3* entry,
        V3* exit);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(float, __thiscall, Entity_GetBoundRadius, 0x00465E10, uint8_t* entity);
GAME_FN(uint8_t, __thiscall, CatchZone_Contains, 0x0065C940, CatchZone* self, float x, float y, float z);
GAME_FN(uint8_t, __thiscall, CatchZone_Sweep, 0x0065CAC0, CatchZone* self, float x, float y, float z, float vx, float vy,
        float vz, float extra, float maxSpeed, int toward);
GAME_FN(CatchZone*, __thiscall, CatchZone_Compute, 0x0065BA70, CatchZone* self, uint8_t* entity, int bone, float ox,
        float oy, float oz, float angle, float reach, float yaw, float pitch);
GAME_FN(uint8_t, __thiscall, Ball_IsTimeUp, 0x004711D0, uint8_t* objective);
GAME_FN(uint8_t, __thiscall, Ball_IsTimeUpF, 0x004711F0, uint8_t* objective);

GAME_VAR(uint32_t, pickupsFoundCount, 0x00C0A0A8);
GAME_ARRAY(int32_t, pickupsFound, 0x00C0A0B0);
GAME_ARRAY(Pickup, pickups, 0x00C0A100);
GAME_VAR(int32_t, walkableModel_BF40EC, 0x00BF40EC);
GAME_ARRAY(uint8_t, projectileActive, 0x00C3A04C);   // per projectile (0x34 apart, 32 of them)

float CrtSqrt(float x) {
    return crt::Sqrtf(x);
}
inline float Length(const V3& v) {
    return CrtSqrt(std::fabs(v.x * v.x + v.y * v.y + v.z * v.z));
}

// 0x65C270 (thiscall): a projectile (of the model named at +8; the ped's target's with
// +0xC) near the ped's middle (+0x10 the radius, else 2) taken as the ped's prop and its
// scanner's focus
uint8_t __fastcall TargetProjectile_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (ped[0x354] == 0) {
        return 0;
    }
    float half = float(double(vc::Call<float>(ped, 0x6C)) * 0.5);
    V3 up{0.0f, 0.0f, half};
    V3 middle;
    VectorAdd(&middle, Pos(ped), &up.x);
    int model = -1;
    if (At<const char*>(self, 8)) {
        CModelInfo_GetModelInfoInRange(At<const char*>(self, 8), &model, 0, 400);
    }
    uint8_t* target = nullptr;
    if (self[0xC] != 0) {
        target = Ped_GetTarget(ped);
        if (!target) {
            return 0;
        }
    }
    float radius = 0.0f < At<float>(self, 0x10) ? At<float>(self, 0x10) : 2.0f;
    int index = Projectile_FindNearest(middle.x, middle.y, middle.z, At<float>(self, 4), radius, model, target, 0);
    if (index == -1) {
        return 0;
    }
    NamedRef_Set(ped + 0x438, GetProjectile(index), 0);
    Scanner_SetFocus(ped + 0x3BC, GetProjectile(index));
    return 1;
}

// 0x65C3B0 (thiscall): the ped's prop target found (+0x29: made its scanner's focus):
// +0x28 the prop held already (of the model +0x10); +0xC a pickup in range (+4, +8 up) it
// wants, else a projectile; +0x14 a bone: the nearest object of the model with the bone
// within +4 on walkable ground; else the prop finder's (+0x1C the kind)
uint8_t __fastcall TargetProp_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    int model = -1;
    uint8_t* found = nullptr;
    if (self[0x28] != 0) {
        uint8_t* prop = At<uint8_t*>(ped, 0x438);
        if (prop) {
            if (!At<uint32_t>(self, 0x10)) {
                if (self[0x29] != 0) {
                    Scanner_SetFocus(ped + 0x3BC, prop);
                }
                return 1;
            }
            model = GetModelIndexByHash(At<uint32_t>(self, 0x10), 0, 0x2FA7);
            if (model == At<int16_t>(prop, 0x10E)) {
                if (self[0x29] != 0) {
                    Scanner_SetFocus(ped + 0x3BC, prop);
                }
                return 1;
            }
        }
        found = nullptr;
    }
    if (At<uint32_t>(self, 0x10)) {
        model = GetModelIndexByHash(At<uint32_t>(self, 0x10), 0, 0x2FA7);
        if (model == -1) {
            return 0;
        }
    }
    if (self[0xC] != 0 && At<void*>(ped, 0x149C)) {
        uint8_t* objective = Queue_GetCurrent(ped + 0x5F0);
        if (objective && At<int32_t>(objective, 8) != 0x17) {
            objective = nullptr;
        }
        V3 pos = *reinterpret_cast<V3*>(Pos(ped));
        pos.z = At<float>(self, 8) + pos.z;
        Pickups_FindInRange(&pos, At<float>(self, 4), 1);
        for (uint32_t i = 0; i < pickupsFoundCount; i++) {
            Pickup& pickup = pickups[pickupsFound[i]];
            uint8_t* info = GetWeaponInfo(pickup.model);
            if ((pickup.flags & 4) || !info || info[0x24] == 0 || !At<void*>(ped, 0x149C)) {
                continue;
            }
            if (self[0xD] != 0) {
                int weapon = GetWeaponIndexByHash(At<uint32_t>(modelInfoPtrs[pickup.model], 4));
                bool wanted = Ped_HasAttackResponse(ped, weapon) != 0;
                if (At<int32_t>(ped, 0x1310) == 0xD) {
                    wanted = true;
                }
                if (objective && pickup.objective == At<int32_t>(objective, 0x10)) {
                    wanted = true;
                }
                if (pickup.entity == At<uint8_t*>(ped, 0x438)) {
                    wanted = true;
                }
                if (weapon != 0 && weapon != -1) {
                    wanted = true;
                }
                if (objective && pickup.objective != At<int32_t>(objective, 0x10)) {
                    continue;
                }
                if (!wanted) {
                    continue;
                }
            } else if (objective && pickup.objective != At<int32_t>(objective, 0x10)) {
                continue;
            }
            // (a pickup without its entity ends the search: on to the projectiles)
            found = pickup.entity;
            if (found) {
                Scanner_SetFocus(ped + 0x3BC, found);
                return 1;
            }
            break;
        }
        int index = Projectile_FindNearest(pos.x, pos.y, pos.z, At<float>(self, 4), At<float>(self, 0x18), -1, nullptr, 1);
        if (index == -1 || ped[0x354] == 0) {
            return 0;
        }
        uint8_t* info = At<uint8_t*>(modelInfoPtrs[GetProjectileModel(index)], 0x18);
        uint8_t* instance = GetProjectileInstance(index);
        if (!info || info[0x24] == 0) {
            return 0;
        }
        if (instance[0x17] != 0 && instance[0x18] != 0) {
            return 0;
        }
        found = GetProjectile(index);
        if (!found) {
            return 0;
        }
        Scanner_SetFocus(ped + 0x3BC, found);
        return 1;
    }
    bool inTable = false;
    for (int i = 0; i < BoneTable_Count(); i++) {
        if (At<int32_t>(self, 0x14) == BoneTable_Get(i)) {
            inTable = true;
            break;
        }
    }
    if (!inTable && At<int32_t>(self, 0x14) != 0) {
        if (model == -1) {
            return 0;
        }
        uint8_t* object = FindNearestObjectOfTypeInRange(model, Pos(ped), 10.0f, 8);
        if (!object) {
            return 0;
        }
        proptr::NodeTransform transform;
        transform.scale = 1.0f;
        if (!proptr::GetNodeTransform(object, At<int32_t>(self, 0x14), &transform, 1)) {
            return 0;
        }
        struct {
            uint32_t a, b;
        } navRef = {At<uint32_t>(ped, 0x18A8), At<uint32_t>(ped, 0x18AC)};
        V3 pedPos = *reinterpret_cast<V3*>(Pos(ped));
        V3 d{transform.pos.x - pedPos.x, transform.pos.y - pedPos.y, transform.pos.z - pedPos.z};
        if (!(At<float>(self, 4) > Vector_Magnitude(&d))) {
            return 0;
        }
        V3 at = transform.pos;
        if (!IsOnWalkable2(Pos(ped), &at, &navRef) && model != walkableModel_BF40EC) {
            return 0;
        }
        NamedRef_Set(ped + 0x438, object, At<int32_t>(self, 0x14));
        if (self[0x29] != 0) {
            Scanner_SetFocus(ped + 0x3BC, object);
        }
        return 1;
    }
    if (At<int32_t>(ped, 0x1310) != 0xD) {
        PropFinder_Setup(ped + 0x34C, At<float>(self, 4), self[0x20], At<float>(self, 0x24), 0);
    }
    uint8_t result = PropFinder_Find(ped + 0x34C, At<int32_t>(self, 0x1C), At<float>(self, 4), model, 1, 0, 0);
    if (result && self[0x29] != 0) {
        uint8_t* entity = At<uint8_t*>(ped, 0x444);
        re::Ptr32<uint8_t>* ref = &At<uint8_t*>(ped, 0x544);
        if (entity != *ref) {
            if (*ref) {
                Entity_CleanUpOldReference(*ref, ref);
            }
            *ref = entity;
            if (entity) {
                RegisterReference(entity, ref);
            }
        }
    }
    return result;
}

// 0x65C8E0 (thiscall): the ped's target (a ped) has a prop: made the ped's
uint8_t __fastcall TargetMyTargetsProp_Evaluate(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* target = Ped_GetTarget(ped);
    if (!target || (target[0x108] & 7) != 3) {
        return 0;
    }
    uint8_t* prop = At<uint8_t*>(target, 0x438);
    if (!prop) {
        return 0;
    }
    if (At<uint8_t*>(ped, 0x438) != prop) {
        NamedRef_Set(ped + 0x438, prop, 0);
    }
    return 1;
}

// 0x65C940 (thiscall, the point by value): the point is in the catch zone: within its
// reach, its yaw and pitch from the zone's facing within the limits
uint8_t __fastcall CatchZone_ContainsImpl(CatchZone* self, re::Edx, float x, float y, float z) {
    V3 point{x, y, z};
    V3 d;
    VectorSub(&d, &point.x, &self->pos.x);
    float dist = Length(d);
    if (self->reach < dist) {
        return 0;
    }
    float facingYaw = GetAngleOfVector2D(&self->fwd);
    float fx = self->fwd.x;
    float fy = self->fwd.y;
    float flat = CrtSqrt(std::fabs(fx * fx + fy * fy));
    float facingPitch = GetATanOfXY(flat, self->fwd.z);
    float yaw = GetAngleOfVector2D(&d);
    float pitch = GetATanOfXY(dist, d.z);
    float yawRatio = std::fabs(AngleDifference(yaw, facingYaw)) / self->yaw;
    float pitchOff = std::fabs(AngleDifference(pitch, facingPitch));
    if (1.0f < yawRatio) {
        return 0;
    }
    float pitchRatio = pitchOff / self->pitch;
    if (1.0f < pitchRatio) {
        return 0;
    }
    return 1;
}

// 0x65CAC0 (thiscall): a body at the point moving at the speed (no faster than maxSpeed;
// towards the zone with `toward`) comes into the catch zone this frame: its path (the
// zone's sphere widened by `extra`) sampled in steps of `extra`
uint8_t __fastcall CatchZone_SweepImpl(CatchZone* self, re::Edx, float x, float y, float z, float vx, float vy, float vz,
                                       float extra, float maxSpeed, int toward) {
    float ts = timeStep;
    float speed = CrtSqrt(std::fabs(vx * vx + vy * vy + vz * vz));
    if (maxSpeed < speed) {
        return 0;
    }
    if (speed < 0.0001f) {
        return CatchZone_Contains(self, x, y, z);
    }
    if (uint8_t(toward) != 0) {
        float dot = self->fwd.y * vy + vx * self->fwd.x + self->fwd.z * vz;
        if (!(dot < 0.0f)) {
            return 0;
        }
    }
    float sphere[5] = {0.0f, 0.0f, 0.0f};   // a CColSphere (its centre zeroed first)
    ColSphere_Set(sphere, self->reach, &self->pos, 0, 0);
    V3 dir{vx, vy, vz};
    Vector_Normalise(&dir);
    V3 entry{0.0f, 0.0f, 0.0f};
    V3 exit{0.0f, 0.0f, 0.0f};
    V3 start{x, y, z};
    if (!ColSphere_IntersectRay(sphere, &start, &dir, &exit, &entry)) {
        return 0;
    }
    V3 step{vx * ts, vy * ts, ts * vz};
    V3 end;
    VectorAdd(&end, &start.x, &step.x);
    float spacing = extra + 0.0001f;
    V3 velocity{vx, vy, vz};
    float travelled = Vector_Magnitude(&velocity) * ts;
    float total = (travelled + spacing) + 0.00001f;
    int count = int(total / spacing);
    float inverse = 1.0f / (float(count) - 1.0f);
    V3 seg;
    VectorSub(&seg, &end.x, &start.x);
    V3 delta{seg.x * inverse, seg.y * inverse, inverse * seg.z};
    if (count <= 0) {
        return 0;
    }
    for (int i = 0; i < count; i++) {
        float fi = float(i);
        V3 off{delta.x * fi, delta.y * fi, fi * delta.z};
        V3 point;
        VectorAdd(&point, &start.x, &off.x);
        if (CatchZone_Contains(self, point.x, point.y, point.z)) {
            return 1;
        }
    }
    return 0;
}

// 0x65CDD0 (thiscall): CatchProjectileCond::Evaluate: the ped's own projectile in flight
// (or, with +0x21, any other flying at it: within its speed's 50 frames' reach) comes into
// its catch zone (at the bone +4, offset (0, +8, +0xC), tilted +0x1C degrees, reaching +0x18
// within +0x10 / +0x14 degrees; +0x20 only coming towards it); a ball objective timed out
// (or caught) is dropped
uint8_t __fastcall CatchProjectile_Evaluate(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    uint8_t caught = 0;
    uint8_t* projectile = At<uint8_t*>(ped, 0x544);
    uint8_t* own = (projectile && At<void*>(projectile, 0xC4)) ? projectile : nullptr;
    float pitchLimit = float(At<float>(self, 0x14) * 3.14159274f) / 180.0f;
    float yawLimit = float(At<float>(self, 0x10) * 3.14159274f) / 180.0f;
    float tilt = float(3.14159274f * At<float>(self, 0x1C)) / 180.0f;
    CatchZone zone;
    CatchZone_Compute(&zone, ped, At<int32_t>(self, 4), 0.0f, At<float>(self, 8), At<float>(self, 0xC), tilt,
                      At<float>(self, 0x18), yawLimit, pitchLimit);
    if (own) {
        V3 pos = *reinterpret_cast<V3*>(Pos(own));
        V3 velocity = At<V3>(own, 0x124);
        // (the flag goes on the stack as a dword whose upper bytes are stale)
        caught = CatchZone_Sweep(&zone, pos.x, pos.y, pos.z, velocity.x, velocity.y, velocity.z, Entity_GetBoundRadius(own),
                                 10000.0f, self[0x20]);
    }
    if (self[0x21] != 0 && !caught) {
        V3 at{0.0f, 0.0f, 0.0f};
        uint8_t* anim = At<uint8_t*>(ped, 0x114);
        if (anim) {
            uint8_t* matrices = At<uint8_t*>(At<uint8_t*>(anim, 0x14), 8);
            at = *reinterpret_cast<V3*>(matrices + At<int32_t>(self, 4) * 0x34 + 0x24);
        } else {
            at = *reinterpret_cast<V3*>(Pos(ped));
        }
        for (int i = 0; i < 32; i++) {
            uint8_t* other = GetProjectile(i);
            if (!other || projectileActive[i * 0x34] == 0 || other == own) {
                continue;
            }
            V3 pos = *reinterpret_cast<V3*>(Pos(other));
            V3 velocity = At<V3>(other, 0x124);
            V3 d;
            VectorSub(&d, &pos.x, &at.x);
            float speed = Length(velocity);
            double reach = double(float(speed * 50.0f) + At<float>(self, 0x18));
            float range = float(double(Entity_GetBoundRadius(other)) + reach);
            float dist = Length(d);
            if (!(range > dist)) {
                continue;
            }
            if (self[0x20] != 0) {
                float dot = d.y * velocity.y + d.x * velocity.x + d.z * velocity.z;
                if (!(dot < 0.0f)) {
                    continue;
                }
            }
            caught = CatchZone_Sweep(&zone, pos.x, pos.y, pos.z, velocity.x, velocity.y, velocity.z,
                                     Entity_GetBoundRadius(other), 10000.0f, self[0x20]);
            if (caught) {
                Scanner_SetFocus(ped + 0x3BC, other);
                break;
            }
        }
    }
    uint8_t* queue = OwnerPed(owner) + 0x5F0;
    uint8_t* objective = Queue_GetCurrent(queue);
    if (objective) {
        if (At<int32_t>(objective, 8) == 0x13 && Ball_IsTimeUp(objective)) {
            Queue_RemoveFirst(OwnerPed(owner) + 0x5F0);
        }
        if (At<int32_t>(objective, 8) == 0x15 && (Ball_IsTimeUpF(objective) || caught)) {
            Queue_RemoveFirst(OwnerPed(owner) + 0x5F0);
        }
    }
    return caught;
}

} // namespace

#define WPNCOND3_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

WPNCOND3_REVERSED("ConditionTargetProjectile::Evaluate", 0x0065C270, &TargetProjectile_Evaluate);
WPNCOND3_REVERSED("ConditionTargetProp::Evaluate", 0x0065C3B0, &TargetProp_Evaluate);
WPNCOND3_REVERSED("ConditionTargetMyTargetsProp::Evaluate", 0x0065C8E0, &TargetMyTargetsProp_Evaluate);
WPNCOND3_REVERSED("CatchZone::Contains_65C940", 0x0065C940, &CatchZone_ContainsImpl);
WPNCOND3_REVERSED("CatchZone::Sweep_65CAC0", 0x0065CAC0, &CatchZone_SweepImpl);
WPNCOND3_REVERSED("CatchProjectileCond::Evaluate", 0x0065CDD0, &CatchProjectile_Evaluate);
