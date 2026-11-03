// CPOIGroup: a group of peds gathered at a point of interest (in its own pool of 0x110
// byte objects, 0x20C7C60), with a spot around the point for each (0x489460 ..
// 0x48A180, with two of the ped groups' own).
//
// +0 the vtable (0x90FAC4), +4 the members (10 registered peds), +0x2C their count, +0x30
// the spots wanted, +0x38 a default ped type, +0x40 the point of interest (+0x70 its
// kind), +0x80 the centre, +0x8C a heading, +0x90 a "full" flag, +0x94 the spots (10 of
// 12 bytes) and +0x10C how many are made.

#include <cmath>
#include <cstdint>

#include "game/Pool.h"
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

GAME_VAR(CPool<uint8_t>*, PGS_pool, 0x020C7C60);
inline re::Ptr32<void>* const PGS_vtable = reinterpret_cast<re::Ptr32<void>*>(0x0090FAC4);

GAME_FN(void, __thiscall, PGS_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(V3*, __cdecl, PGS_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PGS_VectorAdd, 0x004129A0, V3* out, const V3* a, const V3* b);
GAME_FN(float, __thiscall, PGS_Normalise, 0x00414E10, V3* v);
GAME_FN(V3*, __cdecl, PGS_DirectionFromAngle, 0x00432E30, V3* out, float angle);
GAME_FN(float, __cdecl, PGS_Angle4333A0, 0x004333A0, float angle);
inline int __cdecl PGS_Rand() { return crt::Rand(); }
inline void* __cdecl PGS_Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(V3*, __thiscall, PGS_GetForward, 0x004154A0, void* entity, V3* out);
GAME_FN(uint8_t, __thiscall, PGS_Ped4A5000_x64, 0x004A5000, void* self, float, float, float, int flags, float, float, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t PGS_Ped4A5000(void* a0, V3 a1, int a2, float a3, float a4, int a5, int a6) {
    return PGS_Ped4A5000_x64(a0, a1.x, a1.y, a1.z, a2, a3, a4, a5, a6);
}
GAME_FN(void, __thiscall, PGS_GroupRemove, 0x00489770, void* group, void* ped);
GAME_FN(void, __thiscall, PGS_PedGroupConstruct, 0x00489520, void* self, int, int, int, int);
// (this file's, through the game's address)
GAME_FN(void*, __thiscall, PGS_SpotsReset, 0x00489460, void* spots);
GAME_FN(void, __thiscall, PGS_ClearMembers, 0x004897E0, uint8_t* self);
GAME_FN(uint8_t, __thiscall, PGS_GetSpot, 0x004898B0, uint8_t* self, void* ped, V3* spot, V3* toCentre);
GAME_FN(void, __thiscall, PGS_MakeSpots, 0x00489A80, uint8_t* self, float radius);
GAME_FN(uint8_t*, __thiscall, PGS_PoolNew, 0x00489E80, CPool<uint8_t>* pool);

// The member slot holding `ped` (-1: none); the game looks it up through a registered
// copy of the pointer (an argument taken by value)
int MemberIndex(uint8_t* self, void* ped) {
    void* copy = nullptr;
    PGS_RefSet(&copy, ped);
    int index = -1;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        if (At<void*>(self, 4 + i * 4) == copy) {
            index = static_cast<int>(i);
            break;
        }
    }
    PGS_RefSet(&copy, nullptr);
    return index;
}

} // namespace

namespace c07pgs {

// 0x489460: the spots cleared
uint8_t* __fastcall SpotsReset(uint8_t* self, re::Edx) {
    for (uint32_t offset = 0; offset < 0x78; offset += 4) {
        At<float>(self, offset) = 0.0f;
    }
    At<int32_t>(self, 0x78) = 0;
    return self;
}

// 0x4894D0 (a ped group's settings, 0x30): the defaults
uint8_t* __fastcall GroupSettingsReset(uint8_t* self, re::Edx) {
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x24) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    PGS_Memset(self, 0, 0x30);
    At<uint32_t>(self, 0xC) &= 0xFFFFFFFC;
    At<uint8_t>(self, 0x1C) &= 0xFE;
    At<uint8_t>(self, 8) = static_cast<uint8_t>((At<uint8_t>(self, 8) & 5) | 0xAA);
    uint8_t flagsA = static_cast<uint8_t>((At<uint8_t>(self, 0xA) & 0xFA) | 2);
    At<int32_t>(self, 0x10) = 1000;
    At<uint8_t>(self, 9) = 0xF8;
    At<uint8_t>(self, 0xA) = flagsA;
    At<int32_t>(self, 0x2C) = -1;
    return self;
}

// 0x489730 (a ped group): the n-th member there is (nullptr: none)
void* __fastcall GetMember(uint8_t* self, re::Edx, int n) {
    int found = 0;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        if (At<void*>(self, 4 + i * 4)) {
            if (found == n) {
                return At<void*>(self, 4 + i * 4);
            }
            found++;
        }
    }
    return nullptr;
}

// 0x4897E0: the members let go (their group, CPed +0x1D20, cleared)
void __fastcall ClearMembers(uint8_t* self, re::Edx) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        uint8_t* ped = At<uint8_t*>(self, 4 + i * 4);
        if (ped) {
            At<void*>(ped, 0x1D20) = nullptr;
            PGS_RefSet(self + 4 + i * 4, nullptr);
        }
    }
}

// 0x489820 (vf2): the first member's ped type, else the default
int* __fastcall GetPedType(uint8_t* self, re::Edx, int* out) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        uint8_t* ped = At<uint8_t*>(self, 4 + i * 4);
        if (ped) {
            *out = At<int32_t>(ped, 0x1310);
            return out;
        }
    }
    *out = At<int32_t>(self, 0x38);
    return out;   // (returns the output pointer, as the game's eax does: 2026-10-30)
}

// 0x489870: a member has the model
uint8_t __fastcall HasModel(uint8_t* self, re::Edx, int model) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        uint8_t* ped = At<uint8_t*>(self, 4 + i * 4);
        if (ped && At<int16_t>(ped, 0x10E) == model) {
            return 1;
        }
    }
    return 0;
}

// 0x4898B0: a member's spot and the way from it to the centre (normalised). (A ped that
// isn't a member reads the spot before the first, as the game does.)
uint8_t __fastcall GetSpot(uint8_t* self, re::Edx, void* ped, V3* spot, V3* toCentre) {
    if (!ped) {
        return 0;
    }
    int index = MemberIndex(self, ped);
    *spot = At<V3>(self, 0x94 + index * 0xC);
    V3 d;
    *toCentre = *PGS_VectorSub(&d, &At<V3>(self, 0x80), spot);
    PGS_Normalise(toCentre);
    return 1;
}

// 0x489970: the first free member slot's spot and the way from it to the centre
uint8_t __fastcall GetFreeSpot(uint8_t* self, re::Edx, V3* spot, V3* toCentre) {
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        if (At<void*>(self, 4 + i * 4) == nullptr) {
            *spot = At<V3>(self, 0x94 + i * 0xC);
            V3 d;
            *toCentre = *PGS_VectorSub(&d, &At<V3>(self, 0x80), spot);
            return 1;
        }
    }
    return 0;
}

// 0x489A00: a member's spot moved
void __fastcall SetSpot(uint8_t* self, re::Edx, void* ped, const V3* spot) {
    if (!ped) {
        return;
    }
    int index = MemberIndex(self, ped);
    if (index >= 0) {
        At<V3>(self, 0x94 + index * 0xC) = *spot;
    }
}

// 0x489A80: the spots made in a ring around the centre (`radius` 0: by how many), from the
// heading (random when 0, the point's own for kind 1) in steps of 2pi / n radians,
// truncated to whole radians
void __fastcall MakeSpots(uint8_t* self, re::Edx, float radius) {
    int count = At<int32_t>(self, 0x30);
    if (count == 0) {
        return;
    }
    float angle = At<float>(self, 0x8C);
    float countF = static_cast<float>(count);
    const float twoPi = 6.28318548f;
    int step = static_cast<int>(twoPi * (1.0 / countF));
    if (radius == 0.0f) {
        radius = static_cast<float>(countF * 0.5 * 0.6499999761581421);
    }
    if (At<float>(self, 0x8C) == 0.0f) {
        angle = static_cast<float>(PGS_Rand() / 32767.0 * (twoPi - 0.0f) + 0.0f);
    }
    uint8_t* poi = At<uint8_t*>(self, 0x40);
    if (poi && At<int32_t>(poi, 0x70) == 1) {
        angle = PGS_Angle4333A0(At<float>(self, 0x8C));
    }
    float stepF = static_cast<float>(step);
    for (int i = 0; i < At<int32_t>(self, 0x30); i++) {
        V3 tmp;
        V3 dir = *PGS_DirectionFromAngle(&tmp, angle);
        dir.z = 0.0f;
        PGS_Normalise(&dir);
        V3 offset = {radius * dir.x, dir.y * radius, radius * dir.z};
        V3 sum;
        V3* pos = PGS_VectorAdd(&sum, &At<V3>(self, 0x80), &offset);
        int& made = At<int32_t>(self, 0x10C);
        angle = stepF + angle;
        At<V3>(self, 0x94 + made * 0xC) = *pos;
        made++;
    }
}

// 0x489C40 (vf3): a member sent to its spot, facing the centre (for kind 1 the point's
// heading, at the point); whether it faces that way already goes to +0x1378 / +0x13D4
uint8_t __fastcall SendToSpot(uint8_t* self, re::Edx, uint8_t* ped) {
    V3 spot = {0.0f, 0.0f, 0.0f};
    V3 dir = {0.0f, 0.0f, 0.0f};
    uint8_t ok = PGS_GetSpot(self, ped, &spot, &dir);
    if (!ok) {
        return ok;
    }
    int flags = 0x14;
    V3 d;
    V3* diff = PGS_VectorSub(&d, EntityPos(ped), &spot);
    float lengthSq = static_cast<float>((diff->x * diff->x + diff->y * diff->y + diff->z * diff->z) - 0.0025000000745058065);
    if (lengthSq > 0.5f) {
        flags = 0x1C;
    }
    uint8_t* poi = At<uint8_t*>(self, 0x40);
    if (poi && At<int32_t>(poi, 0x70) == 1) {
        float angle = static_cast<float>(At<float>(self, 0x8C) + 3.1415927410125732);
        V3 tmp;
        dir = *PGS_DirectionFromAngle(&tmp, angle);
        PGS_Normalise(&dir);
        spot = *EntityPos(At<uint8_t*>(self, 0x40));
    }
    At<int32_t>(ped, 0x1514) = 1;
    uint8_t failed = PGS_Ped4A5000(ped + 0x654, spot, static_cast<uint16_t>(flags), 0.3f, 0.3f, 0, 0);
    V3 forward;
    V3* f = PGS_GetForward(ped, &forward);
    float facing = f->y * dir.y + f->x * dir.x + f->z * dir.z;
    bool below = facing < 0.8999999761581421;
    int facesIt = (!failed && !below) ? 1 : 0;
    poi = At<uint8_t*>(self, 0x40);
    if (!poi) {
        At<int32_t>(ped, 0x1378) = facesIt;
    } else if (At<int32_t>(poi, 0x70) == 1) {
        At<int32_t>(ped, 0x13D4) = facesIt;
    } else if (At<int32_t>(poi, 0x70) == 2) {
        At<int32_t>(ped, 0x1378) = facesIt;
    }
    if (!failed) {
        uint8_t* intelligence = At<uint8_t*>(ped, 0x2E0);
        At<V3>(intelligence, 0xC) = dir;
        At<V3>(At<uint8_t*>(ped, 0x2E0), 0) = dir;
        At<float>(At<uint8_t*>(ped, 0x2E0), 0x1C) = 0.0f;
    }
    return 1;
}

// 0x489E80 (the group pool): a free slot taken and cleared (nullptr: full)
uint8_t* __fastcall PoolNew(CPool<uint8_t>* self, re::Edx) {
    bool wrapped = false;
    while (!(self->flags[self->lastUsed] & 0x80)) {
        if (++self->lastUsed >= self->size) {
            if (wrapped) {
                return nullptr;
            }
            self->lastUsed = 0;
            wrapped = true;
        }
    }
    self->flags[self->lastUsed] &= 0x7F;
    uint8_t& flag = self->flags[self->lastUsed];
    flag = static_cast<uint8_t>(flag ^ ((static_cast<uint8_t>(flag + 1) ^ flag) & 0x7F));
    uint8_t* object = self->objects + self->elemSize * self->lastUsed;
    PGS_Memset(object, 0, 0x110);
    return object;
}

// 0x489F50: a ped added (off its old group) when there's room and it isn't a member
void __fastcall AddMember(uint8_t* self, re::Edx, uint8_t* ped) {
    if (reinterpret_cast<uint8_t(__thiscall*)(void*)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self))[1]))(self)) {
        return;
    }
    if (MemberIndex(self, ped) != -1) {
        return;
    }
    if (At<void*>(ped, 0x1D20)) {
        PGS_GroupRemove(At<void*>(ped, 0x1D20), ped);
    }
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        if (At<void*>(self, 4 + i * 4) == nullptr) {
            PGS_RefSet(self + 4 + i * 4, ped);
            At<uint8_t*>(ped, 0x1D20) = self;
            return;
        }
    }
}

// 0x48A080: the constructor (the ped group's, then the centre, heading and spots)
uint8_t* __fastcall Construct(uint8_t* self, re::Edx, const V3* centre, float heading, int a, int b, int c, float radius) {
    PGS_PedGroupConstruct(self, a, 1, b, c);
    At<re::Ptr32<void>*>(self, 0) = PGS_vtable;
    At<V3>(self, 0x80) = *centre;
    At<float>(self, 0x8C) = heading;
    PGS_SpotsReset(self + 0x94);
    PGS_ClearMembers(self);
    PGS_MakeSpots(self, radius);
    At<uint8_t>(self, 0x90) = 0;
    return self;
}

// 0x48A100 (cdecl): a group from the pool
uint8_t* __cdecl New() {
    return PGS_PoolNew(PGS_pool);
}

// 0x48A110 (vf1): full (all its spots taken, or flagged)
uint32_t __fastcall IsFull(uint8_t* self, re::Edx) {
    int members = 0;
    for (uint32_t i = 0; i < At<uint32_t>(self, 0x2C); i++) {
        if (At<void*>(self, 4 + i * 4)) {
            members++;
        }
    }
    if (members < At<int32_t>(self, 0x30) && !At<uint8_t>(self, 0x90)) {
        return 0;
    }
    return 1;
}

// 0x48A150 (cdecl): a group back to the pool
void __cdecl Delete(uint8_t* group) {
    CPool<uint8_t>* pool = PGS_pool;
    int slot = static_cast<int>((group - pool->objects) / pool->elemSize);
    pool->flags[slot] |= 0x80;
    if (slot < pool->lastUsed) {
        pool->lastUsed = slot;
    }
}

// 0x48A180 (a 2D vector): its squared length less r squared
float __fastcall LengthSquared2DLess(const float* self, re::Edx, float r) {
    float lengthSq = self[0] * self[0] + self[1] * self[1];
    return lengthSq - r * r;
}

} // namespace c07pgs

#define PGS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PGS_REVERSED("POISpots::Reset", 0x00489460, &c07pgs::SpotsReset);
PGS_REVERSED("PedGroupSettings::Reset", 0x004894D0, &c07pgs::GroupSettingsReset);
PGS_REVERSED("CPedGroup::GetMember", 0x00489730, &c07pgs::GetMember);
PGS_REVERSED("CPOIGroup::ClearMembers", 0x004897E0, &c07pgs::ClearMembers);
PGS_REVERSED("CPOIGroup::GetPedType", 0x00489820, &c07pgs::GetPedType);
PGS_REVERSED("CPOIGroup::HasModel", 0x00489870, &c07pgs::HasModel);
PGS_REVERSED("CPOIGroup::GetSpot", 0x004898B0, &c07pgs::GetSpot);
PGS_REVERSED("CPOIGroup::GetFreeSpot", 0x00489970, &c07pgs::GetFreeSpot);
PGS_REVERSED("CPOIGroup::SetSpot", 0x00489A00, &c07pgs::SetSpot);
PGS_REVERSED("CPOIGroup::MakeSpots", 0x00489A80, &c07pgs::MakeSpots);
PGS_REVERSED("CPOIGroup::SendToSpot", 0x00489C40, &c07pgs::SendToSpot);
PGS_REVERSED("CPOIGroupPool::New", 0x00489E80, &c07pgs::PoolNew);
PGS_REVERSED("CPOIGroup::AddMember", 0x00489F50, &c07pgs::AddMember);
PGS_REVERSED("CPOIGroup::Construct", 0x0048A080, &c07pgs::Construct);
PGS_REVERSED("CPOIGroup::New", 0x0048A100, &c07pgs::New);
PGS_REVERSED("CPOIGroup::IsFull", 0x0048A110, &c07pgs::IsFull);
PGS_REVERSED("CPOIGroup::Delete", 0x0048A150, &c07pgs::Delete);
PGS_REVERSED("CVector2D::LengthSquaredLess", 0x0048A180, &c07pgs::LengthSquared2DLess);
