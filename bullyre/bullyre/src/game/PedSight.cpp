// What a ped sees (CPed +0xA64, 0x48CE00 .. 0x48D470), and two tests of the space
// around (0x48C990, 0x48C9F0).
//
// The sight: +0 the ped, +4 its field of view (degrees... used halved through cos), +8
// its range, +0xC how far up or down it sees the player, +0x10 16 remembered answers of
// 12 bytes (+0 the ped, +4 seen, +8 frames left), +0xD0 / +0xD4 an eye position that
// overrides the head's.

#include <cfloat>
#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "game/World.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct V3 {
    float x, y, z;
};

using re::At;

V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

GAME_VAR(CPool<uint8_t>*, PSI_pedPool, 0x00C0F5F0);
GAME_VAR(uint8_t*, PSI_playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, PSI_noHeadModel, 0x00BF42DC);

GAME_FN(void, __cdecl, PSI_FindObjectsInRange, 0x0045E3C0, const V3* pos, float range, int twoD, int16_t* count, int maxCount,
        re::Ptr32<void>* out, int lists);
GAME_FN(V3*, __thiscall, PSI_GetBoundCentre, 0x00417C00, void* entity, V3* out);
GAME_FN(V3*, __cdecl, PSI_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PSI_VectorAdd, 0x004129A0, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PSI_VectorScale, 0x00412350, V3* out, const V3* v, float scale);
GAME_FN(V3*, __cdecl, PSI_CrossProduct, 0x00414730, V3* out, const V3* a, const V3* b);
GAME_FN(float, __thiscall, PSI_Normalise, 0x00414E10, V3* v);
GAME_FN(float, __thiscall, PSI_Magnitude, 0x0040DFB0, const V3* v);
GAME_FN(float, __thiscall, PSI_Magnitude2D, 0x004246B0, const V3* v);
GAME_FN(uint8_t, __thiscall, PSI_RectIsPointInside, 0x00435AA0, const col::CRect* rect, const float* point);
GAME_FN(V3*, __thiscall, PSI_GetForward, 0x004154A0, void* entity, V3* out);
GAME_FN(float, __cdecl, PSI_Max, 0x00414490, float a, float b);
GAME_FN(uint8_t, __cdecl, PSI_ProcessLineOfSight, 0x00461EA0, const V3* from, const V3* to, CColPoint* colPoint, re::Ptr32<void>* entity,
        int flags, int, int);
GAME_FN(void, __thiscall, PSI_ColPointDestruct, 0x00415500, CColPoint* colPoint);
GAME_FN(void, __cdecl, PSI_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void*, __thiscall, PSI_Ped497020, 0x00497020, void* ped);
GAME_FN(float, __thiscall, PSI_Ped496990, 0x00496990, void* ped);
GAME_FN(float, __cdecl, PSI_DistanceSquaredLess, 0x00487E50, const V3* a, const V3* b, float r);
inline int __cdecl PSI_Rand() { return crt::Rand(); }
// (this file's, through the game's address)
GAME_FN(V3*, __thiscall, PSI_GetLookDir, 0x0048CEE0, uint8_t* self, V3* out);
GAME_FN(V3*, __thiscall, PSI_GetEyePos, 0x0048D020, uint8_t* self, V3* out);
GAME_FN(uint8_t, __thiscall, PSI_CanSee, 0x0048D0A0, uint8_t* self, V3 pos);
GAME_FN(uint8_t, __thiscall, PSI_CanSeeClearly, 0x0048D230, uint8_t* self, uint8_t* target, int throughGlass);

// (both of the game's branches do the same: it compares the entity with 0xC2A8E8 first)
void ReleaseColPoint(CColPoint* colPoint) {
    if (colPoint->entity) {
        PSI_CleanUpOldReference(colPoint->entity, &colPoint->entity);
    }
}

} // namespace

namespace c07psi {

// 0x48C990 (cdecl): no vehicle or ped (lists 6) within `radius` (-1: 0.75) of the point
// in 2D, of at most `maxCount` (-1: 2) found
uint8_t __cdecl IsAreaClear(const V3* pos, float radius, int maxCount, re::Ptr32<void>* out) {
    if (radius == -1.0) {
        radius = 0.75f;
    }
    int count = maxCount;
    if (maxCount == -1) {
        count = 2;
    }
    int16_t found;
    PSI_FindObjectsInRange(pos, radius, 1, &found, count, out, 6);
    return found == 0 ? 1 : 0;
}

// 0x48C9F0 (cdecl): the way between two entities (neither with flags 0x60 of +0x14) is
// less crowded than half their +0x16 counts: the peds but the player within 1.5 of both
// heights, inside the box around the way (2 to each side), within 2 of the line between
uint8_t __cdecl IsWayClear(uint8_t* a, uint8_t* b) {
    if (!a || !b) {
        return 1;
    }
    if ((At<uint8_t>(a, 0x14) & 0x60) || (At<uint8_t>(b, 0x14) & 0x60)) {
        return 0;
    }
    int both = (At<uint8_t>(a, 0x16) & 0xF) + (At<uint8_t>(b, 0x16) & 0xF);
    int limit = static_cast<int>(both * 0.5);
    if (limit == 0) {
        return 0;
    }
    CPool<uint8_t>* pool = PSI_pedPool;
    int slot = pool->size;
    int crowd = 0;
    V3 posA, posB;
    PSI_GetBoundCentre(a, &posA);
    PSI_GetBoundCentre(b, &posB);
    V3 way;
    PSI_VectorSub(&way, &posB, &posA);
    way.z = 0.0f;
    PSI_Normalise(&way);
    V3 up = {0.0f, 0.0f, 1.0f};
    V3 side;
    PSI_CrossProduct(&side, &way, &up);
    PSI_Normalise(&side);
    side.x = static_cast<float>(side.x * 2.0);
    side.y = static_cast<float>(side.y * 2.0);
    side.z = static_cast<float>(side.z * 2.0);
    V3 p1, p2;
    PSI_VectorAdd(&p1, &posB, &side);
    PSI_VectorSub(&p2, &posA, &side);
    float minX = p1.x;
    float maxX = p2.x;
    if (p1.x > p2.x) {
        minX = p2.x;
        maxX = p1.x;
    }
    float minY = p1.y;
    float maxY = p2.y;
    if (p1.y > p2.y) {
        minY = p2.y;
        maxY = p1.y;
    }
    col::CRect box;
    box.left = minX;
    box.bottom = minY;
    box.right = maxX;
    box.top = maxY;
    while (slot != 0) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (!ped || At<int32_t>(ped, 0x1310) == 13) {
            continue;
        }
        V3 q = *EntityPos(ped);
        float dzA = std::fabs(q.z - posA.z);
        if (1.5f < dzA || dzA != dzA) {
            continue;
        }
        float dzB = std::fabs(q.z - posB.z);
        if (1.5f < dzB || dzB != dzB) {
            continue;
        }
        float point[2] = {q.x, q.y};
        if (!PSI_RectIsPointInside(&box, point)) {
            continue;
        }
        V3 seg;
        float length = PSI_Magnitude(PSI_VectorSub(&seg, &posB, &posA));
        double sy = static_cast<double>(posB.y) - posA.y;
        double sx = static_cast<double>(posB.x) - posA.x;
        double sz = static_cast<double>(posB.z) - posA.z;
        double dot = ((q.x - static_cast<double>(posA.x)) * sx + (q.y - static_cast<double>(posA.y)) * sy) +
                     (q.z - static_cast<double>(posA.z)) * sz;
        float u = static_cast<float>(dot / (static_cast<double>(length) * length));
        if (u < 0.0f || 1.0f < u) {
            continue;
        }
        V3 nearest = {static_cast<float>(posA.x + u * sx), static_cast<float>(posA.y + u * sy),
                      static_cast<float>(u * sz + posA.z)};
        V3 off;
        float away = std::fabs(PSI_Magnitude2D(PSI_VectorSub(&off, &q, &nearest)));
        if (away < 2.0f || away == 2.0f) {
            crowd++;
        }
    }
    return crowd < limit ? 1 : 0;
}

// 0x48CE00: the eye position overridden
void __fastcall SetEyeOverride(uint8_t* self, re::Edx, const V3* pos) {
    At<uint8_t>(self, 0xD0) = 1;
    At<V3>(self, 0xD4) = *pos;
}

// 0x48CE30: the eye override dropped
void __fastcall ClearEyeOverride(uint8_t* self, re::Edx) {
    At<uint8_t>(self, 0xD0) = 0;
}

// 0x48CE40: the remembered answers a frame older (gone at 0)
void __fastcall Tick(uint8_t* self, re::Edx) {
    for (int i = 0; i < 16; i++) {
        uint8_t* entry = self + 0x10 + i * 0xC;
        if (At<void*>(entry, 0) && --At<int32_t>(entry, 8) <= 0) {
            At<void*>(entry, 0) = nullptr;
            At<uint8_t>(entry, 4) = 0;
            At<int32_t>(entry, 8) = 0;
        }
    }
}

// 0x48CEB0: reset
void __fastcall Reset(uint8_t* self, re::Edx) {
    At<int32_t>(self, 0xC) = 0;
    for (int i = 0; i < 16; i++) {
        uint8_t* entry = self + 0x10 + i * 0xC;
        At<void*>(entry, 0) = nullptr;
        At<uint8_t>(entry, 4) = 0;
        At<int32_t>(entry, 8) = 0;
    }
    At<uint8_t>(self, 0xD0) = 0;
}

// 0x48CEE0: where the ped looks: its forward, turned towards the head's axis by the
// head frame's weight (for a ped with a head frame and not the excluded model)
V3* __fastcall GetLookDir(uint8_t* self, re::Edx, V3* out) {
    uint8_t* ped = At<uint8_t*>(self, 0);
    V3 forward;
    PSI_GetForward(ped, &forward);
    uint8_t* anim = At<uint8_t*>(ped, 0x114);
    if (anim && At<uint8_t*>(anim, 0x14) && At<uint8_t*>(At<uint8_t*>(anim, 0x14), 8) && !((At<uint32_t>(anim, 0x98) >> 13) & 1) &&
        At<int16_t>(ped, 0x10E) != PSI_noHeadModel) {
        uint8_t* head = At<uint8_t*>(At<uint8_t*>(anim, 0x14), 8);
        V3 axis = {At<float>(head, 0x3B0), At<float>(head, 0x3BC), At<float>(head, 0x3C8)};
        float weight = PSI_Max(At<float>(head, 0x3C4), 0.0f);
        float rest = 1.0f - weight;
        V3 a, b, sum;
        V3* scaledForward = PSI_VectorScale(&a, &forward, rest);
        V3* scaledAxis = PSI_VectorScale(&b, &axis, weight);
        PSI_VectorAdd(&sum, scaledAxis, scaledForward);
        *out = sum;
        return out;
    }
    *out = forward;
    return out;
}

// 0x48D020: the eye position: the head frame's (+0x2FC) for a ped with one (no +0xA4, not
// the excluded model), else the ped's position
V3* __fastcall GetEyePos(uint8_t* self, re::Edx, V3* out) {
    uint8_t* ped = At<uint8_t*>(self, 0);
    if (At<int32_t>(ped, 0xA4) == 0 && At<int16_t>(ped, 0x10E) != PSI_noHeadModel) {
        uint8_t* anim = At<uint8_t*>(ped, 0x114);
        if (anim && At<uint8_t*>(anim, 0x14) && At<uint8_t*>(At<uint8_t*>(anim, 0x14), 8)) {
            *out = At<V3>(At<uint8_t*>(At<uint8_t*>(anim, 0x14), 8), 0x2FC);
            return out;
        }
    }
    *out = *EntityPos(ped);
    return out;
}

// 0x48D0A0: the point within range (2D) and inside the field of view
uint8_t __fastcall CanSee(uint8_t* self, re::Edx, V3 pos) {
    float range = At<float>(self, 8);
    V3 d;
    PSI_VectorSub(&d, &pos, EntityPos(At<uint8_t*>(self, 0)));
    float sum = d.x * d.x + d.y * d.y + 0.0f * 0.0f;
    float outside = sum - range * range;
    if (outside > 0.0f) {
        return 0;
    }
    V3 look;
    PSI_GetLookDir(self, &look);
    look.z = 0.0f;
    PSI_Normalise(&look);
    V3 e;
    PSI_VectorSub(&e, &pos, EntityPos(At<uint8_t*>(self, 0)));
    V3 to = {e.x, e.y, 0.0f};
    PSI_Normalise(&to);
    float facing = look.x * to.x + to.y * look.y + to.z * look.z;
    float half = static_cast<float>(At<float>(self, 4) * 0.5);
    float limit = static_cast<float>(std::cos(half));
    return facing > limit ? 1 : 0;
}

// 0x48D1F0: the entity's position seen
uint8_t __fastcall CanSeeEntity(uint8_t* self, re::Edx, uint8_t* entity) {
    return PSI_CanSee(self, *EntityPos(entity));
}

// 0x48D230: the target seen and nothing in the line from the eyes (to a ped's eye
// height; `throughGlass` adds flag 0x100); the player too far above or below (+0xC, with
// the ped's +0x1398) or hiding (+0x1E91) is not
uint8_t __fastcall CanSeeClearly(uint8_t* self, re::Edx, uint8_t* target, int throughGlass) {
    if (!PSI_CanSee(self, *EntityPos(target))) {
        return 0;
    }
    CColPoint colPoint;
    colPoint.point = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = FLT_MAX;
    V3 eye;
    PSI_GetEyePos(self, &eye);
    if (At<uint8_t>(self, 0xD0)) {
        eye = At<V3>(self, 0xD4);
    }
    V3 to = {0.0f, 0.0f, 0.0f};
    if ((At<uint8_t>(target, 0x108) & 7) == 3) {
        if (target == PSI_playerPed) {
            if (At<uint8_t>(PSI_playerPed, 0x1E91)) {
                return 0;
            }
            uint8_t* ped = At<uint8_t*>(self, 0);
            if (At<int32_t>(ped, 0x1398) != 0) {
                float dz = std::fabs(EntityPos(ped)->z - EntityPos(target)->z);
                if (At<float>(self, 0xC) < dz) {
                    PSI_ColPointDestruct(&colPoint);
                    return 0;
                }
            }
        }
        to = *EntityPos(target);
        V3 targetEye;
        to.z = PSI_GetEyePos(target + 0xA64, &targetEye)->z;
    } else {
        to = *EntityPos(target);
    }
    re::Ptr32<void> hit;
    if (PSI_ProcessLineOfSight(&eye, &to, &colPoint, &hit, re::Flag(throughGlass) ? 0x14B : 0x4B, 0, 0)) {
        ReleaseColPoint(&colPoint);
        return 0;
    }
    ReleaseColPoint(&colPoint);
    return 1;
}

// 0x48D470: the ped seen (itself always; the one it's focused on within 5 always; else
// the remembered answer, or asked and remembered for 4..8 frames)
uint8_t __fastcall Sees(uint8_t* self, re::Edx, uint8_t* ped) {
    if (!ped) {
        return 0;
    }
    uint8_t* owner = At<uint8_t*>(self, 0);
    if (ped == owner) {
        return 1;
    }
    if (PSI_Ped497020(owner) == ped && 5.0f > PSI_Ped496990(At<void*>(self, 0))) {
        if (PSI_DistanceSquaredLess(EntityPos(At<uint8_t*>(self, 0)), EntityPos(ped), 5.0f) < 0.0f) {
            return 1;
        }
    }
    for (int i = 0; i < 16; i++) {
        uint8_t* entry = self + 0x10 + i * 0xC;
        if (At<uint8_t*>(entry, 0) == ped) {
            return At<uint8_t>(entry, 4);
        }
    }
    uint8_t seen = PSI_CanSeeClearly(self, ped, 0);
    for (int i = 0; i < 16; i++) {
        uint8_t* entry = self + 0x10 + i * 0xC;
        if (At<uint8_t*>(entry, 0) == nullptr) {
            At<uint8_t*>(entry, 0) = ped;
            At<uint8_t>(entry, 4) = seen;
            At<int32_t>(entry, 8) = PSI_Rand() * 5 / 0x7FFF + 4;
            return seen;
        }
    }
    return seen;
}

} // namespace c07psi

#define PSI_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PSI_REVERSED("PedSpace::IsAreaClear", 0x0048C990, &c07psi::IsAreaClear);
PSI_REVERSED("PedSpace::IsWayClear", 0x0048C9F0, &c07psi::IsWayClear);
PSI_REVERSED("PedSight::SetEyeOverride", 0x0048CE00, &c07psi::SetEyeOverride);
PSI_REVERSED("PedSight::ClearEyeOverride", 0x0048CE30, &c07psi::ClearEyeOverride);
PSI_REVERSED("PedSight::Tick", 0x0048CE40, &c07psi::Tick);
PSI_REVERSED("PedSight::Reset", 0x0048CEB0, &c07psi::Reset);
PSI_REVERSED("PedSight::GetLookDir", 0x0048CEE0, &c07psi::GetLookDir);
PSI_REVERSED("PedSight::GetEyePos", 0x0048D020, &c07psi::GetEyePos);
PSI_REVERSED("PedSight::CanSee", 0x0048D0A0, &c07psi::CanSee);
PSI_REVERSED("PedSight::CanSeeEntity", 0x0048D1F0, &c07psi::CanSeeEntity);
PSI_REVERSED("PedSight::CanSeeClearly", 0x0048D230, &c07psi::CanSeeClearly);
PSI_REVERSED("PedSight::Sees", 0x0048D470, &c07psi::Sees);
