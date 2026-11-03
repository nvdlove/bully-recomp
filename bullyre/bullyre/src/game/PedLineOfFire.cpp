// A ped's checks of the space it shoots and throws through (0x48A1B0 .. 0x48AD90): the
// projectiles in flight (CProjectileInfo's 32 records at 0xC3A03C, 0x34 each: +0 the
// thrower, +0x10 in use, +0x2C the projectile's weapon info), whether a line of fire is
// clear of other peds and of the world. `self` is a ped part whose +0 is the ped.

#include <cfloat>
#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "game/World.h"
#include "reversed.h"

namespace {

struct V3 {
    float x, y, z;
};

using re::At;

V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

inline uint8_t* const PLF_projectiles = reinterpret_cast<uint8_t*>(0x00C3A03C);
constexpr int kProjectileCount = 32;
GAME_ARRAY(uint8_t*, PLF_modelInfoPtrs, 0x00C67738);
// the aim offsets above a ped (made on first use): standing, crouched (+0x131C)
GAME_VAR(uint32_t, PLF_offsetsMade, 0x00C2B2FC);
GAME_VAR(V3, PLF_aimStanding, 0x00C2B2F0);
GAME_VAR(V3, PLF_aimCrouched, 0x00C2B2E4);

GAME_FN(uint8_t*, __cdecl, PLF_GetProjectile, 0x004D3DD0, int index);
GAME_FN(uint8_t*, __thiscall, PLF_GetColSpheres, 0x00473F80, void* entity);
GAME_FN(V3*, __thiscall, PLF_GetPosition, 0x00415460, void* entity);
GAME_FN(V3*, __cdecl, PLF_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PLF_VectorAdd, 0x004129A0, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PLF_VectorScale, 0x00412350, V3* out, const V3* v, float scale);
GAME_FN(V3*, __cdecl, PLF_SetLength, 0x0041A520, V3* out, const V3* v, float length);
GAME_FN(float, __cdecl, PLF_DotProduct, 0x00414640, const V3* a, const V3* b);
GAME_FN(float, __thiscall, PLF_Normalise, 0x00414E10, V3* v);
GAME_FN(float, __cdecl, PLF_DistanceSquared, 0x00472D00, const V3* a, const V3* b);
GAME_FN(float, __cdecl, PLF_Distance, 0x0041A950, const V3* a, const V3* b);
GAME_FN(float, __cdecl, PLF_InRange41A4D0, 0x0041A4D0, const V3* a, const V3* b, float range);
GAME_FN(uint8_t, __thiscall, PLF_Sees48D0A0, 0x0048D0A0, void* self, V3 pos);
GAME_FN(uint8_t, __thiscall, PLF_Weapon4D96D0, 0x004D96D0, void* weapon);
GAME_FN(void, __thiscall, PLF_WeaponInfo7282A0, 0x007282A0, void* info);
GAME_FN(V3*, __thiscall, PLF_Entity466C70, 0x00466C70, void* entity, V3* out);
GAME_FN(uint8_t, __thiscall, PLF_Ped496F30, 0x00496F30, void* ped, void* other);
GAME_FN(float, __thiscall, PLF_LengthSquared2DLess, 0x0048A180, const V3* v, float r);
GAME_FN(float, __cdecl, PLF_DistanceSquaredLess, 0x00487E50, const V3* a, const V3* b, float r);
// CWorld::ProcessLineOfSight
GAME_FN(uint8_t, __cdecl, PLF_ProcessLineOfSight, 0x00461EA0, const V3* from, const V3* to, CColPoint* colPoint, re::Ptr32<void>* entity,
        int flags, int, int);
GAME_FN(void, __cdecl, PLF_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
// (this file's, through the game's address)
GAME_FN(uint8_t, __stdcall, PLF_TouchesSpheres, 0x0048A1B0, const V3* pos, float radius, void* entity);
GAME_FN(uint8_t, __thiscall, PLF_IsLineOfFireClear, 0x0048A700, void* self, const V3* from, uint8_t* target, int flags);

uint8_t* Projectile(int i) {
    return PLF_projectiles + i * 0x34;
}

// (both of the game's branches do the same: it compares the entity with 0xC2A8E8 first)
void ReleaseColPoint(CColPoint* colPoint) {
    if (colPoint->entity) {
        PLF_CleanUpOldReference(colPoint->entity, &colPoint->entity);
    }
}

} // namespace

namespace c07plf {

// 0x48A1B0 (stdcall): `pos` within `radius` of one of the entity's collision spheres
// (their centres offset from its position, unrotated)
uint8_t __stdcall TouchesSpheres(const V3* pos, float radius, void* entity) {
    uint8_t* info = PLF_GetColSpheres(entity);
    for (int i = 0; i < At<int8_t>(At<uint8_t*>(info, 0x38), 0x1E); i++) {
        uint8_t* sphere = At<uint8_t*>(At<uint8_t*>(info, 0x38), 0) + i * 0x14;
        V3 centre, d;
        PLF_VectorAdd(&centre, PLF_GetPosition(entity), reinterpret_cast<V3*>(sphere));
        PLF_VectorSub(&d, &centre, pos);
        float reach = At<float>(sphere, 0xC) + radius;
        float distanceSq = d.x * d.x + d.y * d.y + d.z * d.z;
        float reachSq = reach * reach;
        if (distanceSq < reachSq) {
            return 1;
        }
    }
    return 0;
}

// 0x48A280: the nearest (squared, within 100) live projectile with a blast (+0x94) to
// run from: its position and blast radius. One of the ped's own it's already out of the
// blast of (+0.5) says true at once.
uint8_t __fastcall FindProjectileThreat(uint8_t* self, re::Edx, V3* pos, float* blast) {
    if (At<int32_t>(At<uint8_t*>(self, 0), 0x1360) != 0) {
        return 0;
    }
    float best = 100.0f;
    uint8_t found = 0;
    for (int i = 0; i < kProjectileCount; i++) {
        uint8_t* record = Projectile(i);
        uint8_t* projectile = PLF_GetProjectile(i);
        if (!projectile || !At<uint8_t>(record, 0x10)) {
            continue;
        }
        uint8_t* info = At<uint8_t*>(record, 0x2C);
        if (!info) {
            continue;
        }
        V3 d;
        V3* diff = PLF_VectorSub(&d, EntityPos(projectile), EntityPos(At<uint8_t*>(self, 0)));
        float distanceSq = diff->x * diff->x + diff->y * diff->y + diff->z * diff->z;
        if (At<uint8_t*>(record, 0) == At<uint8_t*>(self, 0)) {
            if (!At<uint8_t*>(record, 0x2C)) {
                continue;
            }
            float range = At<float>(info, 0x94);
            if (range > 0.0f) {
                float outside = range + 0.5f;
                float outsideSq = outside * outside;
                if (outsideSq < distanceSq) {
                    return 1;
                }
            }
        }
        uint8_t* live = At<uint8_t*>(record, 0x2C);
        if (live && !At<uint8_t>(live, 0xE4)) {
            continue;
        }
        if (!(0.0f < At<float>(info, 0x94) || std::isnan(At<float>(info, 0x94)))) {
            continue;
        }
        if (!(0.0f < At<float>(info, 0xA0) || std::isnan(At<float>(info, 0xA0)))) {
            continue;
        }
        if (!At<uint8_t>(info, 0x8C) && !At<uint8_t>(info, 0x8D) && At<int32_t>(info, 0x48) <= 0) {
            continue;
        }
        if (best < distanceSq) {
            continue;
        }
        best = distanceSq;
        found = 1;
        *pos = *EntityPos(projectile);
        *blast = At<float>(info, 0x94);
    }
    return found;
}

// 0x48A450: the nearest (squared, within 1000) moving projectile of someone else's that
// the ped can see and that comes its way within 3: its position, direction and the
// point it passes nearest the ped
uint8_t __fastcall FindIncomingProjectile(uint8_t* self, re::Edx, V3* pos, V3* direction, V3* nearest) {
    if (At<int32_t>(At<uint8_t*>(self, 0), 0x1360) != 0) {
        return 0;
    }
    float best = 1000.0f;
    uint8_t found = 0;
    for (int i = 0; i < kProjectileCount; i++) {
        uint8_t* record = Projectile(i);
        if (!At<uint8_t>(record, 0x10)) {
            continue;
        }
        uint8_t* projectile = PLF_GetProjectile(i);
        if (!projectile || At<uint8_t*>(record, 0) == At<uint8_t*>(self, 0)) {
            continue;
        }
        uint8_t* live = At<uint8_t*>(record, 0x2C);
        if (live && !At<uint8_t>(live, 0xE4)) {
            continue;
        }
        V3* velocity = &At<V3>(projectile, 0x124);
        float speedSq = velocity->y * velocity->y + velocity->x * velocity->x + velocity->z * velocity->z;
        if (static_cast<float>(speedSq - 0.00039999998211860677) < 0.0f) {
            continue;
        }
        if (!PLF_Sees48D0A0(At<uint8_t*>(self, 0) + 0xA64, *EntityPos(projectile))) {
            continue;
        }
        float distanceSq = PLF_DistanceSquared(EntityPos(projectile), EntityPos(At<uint8_t*>(self, 0)));
        if (best < distanceSq) {
            continue;
        }
        V3* at = EntityPos(projectile);
        V3 toPed;
        float closing = PLF_DotProduct(PLF_VectorSub(&toPed, EntityPos(At<uint8_t*>(self, 0)), at), velocity);
        if (closing < 0.0f || closing == 0.0f) {
            continue;
        }
        V3 dir;
        PLF_SetLength(&dir, velocity, 1.0f);
        V3 d2, along;
        float t = PLF_DotProduct(PLF_VectorSub(&d2, EntityPos(At<uint8_t*>(self, 0)), at), &dir);
        V3 point;
        PLF_VectorAdd(&point, at, PLF_VectorScale(&along, &dir, t));
        if (PLF_InRange41A4D0(EntityPos(At<uint8_t*>(self, 0)), &point, 3.0f) > 0.0f) {
            continue;
        }
        best = distanceSq;
        *pos = *at;
        *direction = dir;
        *nearest = point;
        found = 1;
    }
    return found;
}

// 0x48A700: the line from `from` to `target` is clear to fire along: not too close (0.1);
// with flag 4 the thrown weapon's blast (+0x94) touching neither the thrower nor its
// friends near the target's aim point; no other ped within 1 of the line (flag 2: where
// they're going, flag 1: not ones moving faster than 0.2); and nothing in the world
// (but vehicles near the aim point) in the way from 1.5 above `from`
uint8_t __fastcall IsLineOfFireClear(uint8_t* self, re::Edx, const V3* from, uint8_t* target, int flags) {
    if (!target) {
        return 0;
    }
    if (!(PLF_offsetsMade & 1)) {
        PLF_aimStanding.x = 0.0f;
        PLF_aimStanding.y = 0.0f;
        PLF_offsetsMade |= 1;
        PLF_aimStanding.z = 1.5f;
    }
    if (!(PLF_offsetsMade & 2)) {
        PLF_aimCrouched.x = 0.0f;
        PLF_aimCrouched.y = 0.0f;
        PLF_offsetsMade |= 2;
        PLF_aimCrouched.z = 0.75f;
    }
    uint8_t* targetPed = (At<uint8_t>(target, 0x108) & 7) == 3 ? target : nullptr;
    V3 targetPos = *EntityPos(target);
    float distance = PLF_Distance(from, &targetPos);
    if (distance < 0.10000000149011612) {
        return 0;
    }
    V3 d;
    V3 dir = *PLF_VectorSub(&d, &targetPos, from);
    PLF_Normalise(&dir);
    V3 aim = {0.0f, 0.0f, 0.0f};
    bool checkBlast = false;
    float blast = 0.0f;
    uint8_t* ped = At<uint8_t*>(self, 0);
    if ((flags & 4) && At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) != -1 &&
        PLF_Weapon4D96D0(At<void*>(At<uint8_t*>(self, 0), 0x1D0))) {
        uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(self, 0), 0x1D0);
        uint8_t* weaponInfo = At<uint8_t*>(PLF_modelInfoPtrs[At<int32_t>(weapon, 0x12C)], 0x18);
        if (weaponInfo) {
            PLF_WeaponInfo7282A0(weaponInfo);
            if (At<void*>(weaponInfo, 0x80)) {
                PLF_WeaponInfo7282A0(weaponInfo);
                uint8_t* projectileInfo = At<uint8_t*>(weaponInfo, 0x80);
                if (0.0f < At<float>(projectileInfo, 0x94)) {
                    blast = At<float>(projectileInfo, 0x94);
                    checkBlast = true;
                    V3 tmp;
                    V3* point;
                    if (!targetPed) {
                        point = PLF_Entity466C70(target, &tmp);
                    } else {
                        point = PLF_VectorAdd(&tmp, &targetPos, At<int32_t>(targetPed, 0x131C) ? &PLF_aimCrouched : &PLF_aimStanding);
                    }
                    aim = *point;
                    if (PLF_TouchesSpheres(&aim, blast, At<void*>(self, 0))) {
                        return 0;
                    }
                }
            }
        }
    }
    uint8_t* nearby = At<uint8_t*>(self, 0) + 0x3BC;
    for (int i = 0; i < At<int32_t>(nearby, 4); i++) {
        uint8_t* other = At<uint8_t*>(nearby, 8 + i * 4);
        if (!other) {
            continue;
        }
        V3 otherPos = *EntityPos(other);
        if (checkBlast && !At<uint8_t>(other, 0x1C98) && PLF_Ped496F30(At<void*>(self, 0), other) &&
            PLF_TouchesSpheres(&aim, blast, other)) {
            return 0;
        }
        if (other == target) {
            continue;
        }
        if ((flags & 2) && At<uint8_t>(other, 0x1E14)) {
            otherPos = At<V3>(other, 0x1E18);
        } else if ((flags & 1) && At<float>(At<uint8_t*>(other, 0x2E0), 0x1C) > 0.20000000298023224) {
            continue;
        }
        V3 v;
        PLF_VectorSub(&v, &otherPos, from);
        float along = v.y * dir.y + v.x * dir.x + v.z * dir.z;
        if (along < 0.0f) {
            continue;
        }
        if (distance < along) {
            continue;
        }
        V3 step = {dir.x * along, dir.y * along, dir.z * along};
        V3 nearestPoint, off;
        PLF_VectorAdd(&nearestPoint, from, &step);
        PLF_VectorSub(&off, &otherPos, &nearestPoint);
        float height = std::fabs(off.z);
        if (height > 3.0f) {
            continue;
        }
        float sideSq = PLF_LengthSquared2DLess(&off, 1.0f);
        if (!(0.0f < sideSq)) {
            return 0;
        }
    }
    CColPoint colPoint;
    colPoint.point = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = FLT_MAX;
    re::Ptr32<void> hit = nullptr;
    V3 start;
    PLF_VectorAdd(&start, from, &PLF_aimStanding);
    V3 end = *EntityPos(target);
    if (targetPed) {
        const V3& offset = At<int32_t>(targetPed, 0x131C) ? PLF_aimCrouched : PLF_aimStanding;
        end.x = end.x + offset.x;
        end.y = end.y + offset.y;
        end.z = end.z + offset.z;
    }
    if (PLF_ProcessLineOfSight(&start, &end, &colPoint, &hit, 0x11B, 0, 0) && hit && hit != target) {
        if ((At<uint8_t>(hit, 0x108) & 7) != 6) {
            ReleaseColPoint(&colPoint);
            return 0;
        }
        if (PLF_DistanceSquaredLess(reinterpret_cast<V3*>(&colPoint.point), &end, 3.0f) < 0.0f) {
            ReleaseColPoint(&colPoint);
            return 0;
        }
    }
    ReleaseColPoint(&colPoint);
    return 1;
}

// 0x48AD90: from the ped's position
uint8_t __fastcall IsLineOfFireClearFromPed(uint8_t* self, re::Edx, uint8_t* target, int flags) {
    uint8_t* matrix = At<uint8_t*>(At<uint8_t*>(self, 0), 0x14);
    if (matrix) {
        return PLF_IsLineOfFireClear(self, reinterpret_cast<V3*>(matrix + 0x30), target, flags);
    }
    return PLF_IsLineOfFireClear(self, &At<V3>(At<uint8_t*>(self, 0), 4), target, flags);
}

} // namespace c07plf

#define PLF_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PLF_REVERSED("PedLineOfFire::TouchesSpheres", 0x0048A1B0, &c07plf::TouchesSpheres);
PLF_REVERSED("PedLineOfFire::FindProjectileThreat", 0x0048A280, &c07plf::FindProjectileThreat);
PLF_REVERSED("PedLineOfFire::FindIncomingProjectile", 0x0048A450, &c07plf::FindIncomingProjectile);
PLF_REVERSED("PedLineOfFire::IsLineOfFireClear", 0x0048A700, &c07plf::IsLineOfFireClear);
PLF_REVERSED("PedLineOfFire::IsLineOfFireClearFromPed", 0x0048AD90, &c07plf::IsLineOfFireClearFromPed);
