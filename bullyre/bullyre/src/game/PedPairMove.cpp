// CPed: the move checked against a vehicle's collision (0x483690), the deleting
// destructor, and vf13: the four ground probes around the ped and the partner moved
// along with it (0x483B10).

#include <cfloat>
#include <cmath>
#include <cstdint>

#include "game/Collision.h"
#include "game/Pool.h"
#include "game/World.h"
#include "reversed.h"

namespace {

struct V3 {
    float x, y, z;
};

using re::At;

// CPlaceable::GetPosition: the matrix's (+0x14, +0x30) if there is one, else +4
V3* EntityPos(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? reinterpret_cast<V3*>(matrix + 0x30) : &At<V3>(entity, 4);
}

template <typename R, typename... A>
R VCall(void* self, uint32_t offset, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(self);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[offset / 4]))(self, args...);
}

GAME_VAR(CPool<uint8_t>*, PPM_pedPool, 0x00C0F5F0);        // CPools::ms_pPedPool
GAME_ARRAY(uint8_t*, PPM_modelInfoPtrs, 0x00C67738);      // CModelInfo::ms_modelInfoPtrs
GAME_VAR(int32_t, PPM_debugMoveAnim, 0x00C2B1C8);          // (the ped's own anim moved too)
GAME_VAR(int32_t, PPM_flattenPartnerMove, 0x00A339D0);

GAME_FN(uint8_t, __thiscall, PPM_GetIsTouchingMoving, 0x00465D10, void* entity, V3* pos, float radius);
GAME_FN(V3*, __cdecl, PPM_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PPM_VectorAdd, 0x004129A0, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, PPM_VectorScale, 0x00412350, V3* out, const V3* v, float scale);
GAME_FN(float, __thiscall, PPM_Normalise, 0x00414E10, V3* v);
// the rotation to identity (the position and attached matrix left alone)
GAME_FN(void, __thiscall, PPM_MatrixSetUnityRotation, 0x004127A0, col::CMatrix* m);
// the swept test of a collision model from one matrix to another
GAME_FN(uint8_t, __cdecl, PPM_ProcessSweep, 0x0056C8C0, col::CMatrix* to, col::CMatrix* from, void* colModel, void* matrixA,
        void* matrixB, uintptr_t arg6, CColPoint* colPoint, int, int, int, int, int, int lists, uintptr_t, int);
GAME_FN(int*, __thiscall, PPM_GetVehicleClass, 0x004CAF30, void* vehicle, int* out);
GAME_FN(void, __thiscall, PPM_Ped482070, 0x00482070, void* ped, int a, int b);
GAME_FN(void, __thiscall, PPM_ColPointSetEntity, 0x0045F1F0, CColPoint* colPoint, void* entity);
GAME_FN(void, __thiscall, PPM_Vehicle4C4830, 0x004C4830, void* vehicle, void* pedPart, CColPoint* colPoint, int);
GAME_FN(void, __cdecl, PPM_CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, PPM_PedDestruct, 0x00480CC0, void* ped);
GAME_FN(uint8_t, __thiscall, PPM_Physical41C540, 0x0041C540, void* ped);
GAME_FN(void, __thiscall, PPM_RemoveAndAdd, 0x00469470, void* entity);
GAME_FN(V3*, __thiscall, PPM_GetForward, 0x004154A0, void* entity, V3* out);
GAME_FN(V3*, __thiscall, PPM_GetRight, 0x004736F0, void* entity, V3* out);
// CWorld::ProcessVerticalLine (from the point down to z)
GAME_FN(uint8_t, __cdecl, PPM_ProcessVerticalLine, 0x004629C0, const V3* point, float z, CColPoint* colPoint, re::Ptr32<void>* entity,
        int lists, int);
GAME_FN(void, __thiscall, PPM_Physical41EE20, 0x0041EE20, void* entity, const V3* pos);
// the ped's anim (+0x114) moved by an offset
GAME_FN(void, __thiscall, PPM_Anim6BA090_x64, 0x006BA090, void* anim, float, float, float, int, float);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void PPM_Anim6BA090(void* a0, V3 a1, int a2, float a3) {
    return PPM_Anim6BA090_x64(a0, a1.x, a1.y, a1.z, a2, a3);
}

// (both of the game's branches do the same: it compares the entity with 0xC2A8E8 first)
void ReleaseColPointEntity(CColPoint* colPoint) {
    if (colPoint->entity) {
        PPM_CleanUpOldReference(colPoint->entity, &colPoint->entity);
    }
}

// A CMatrix built on the stack: its rotation to identity, at `pos`. (The game copies a
// fourth dword after the position from uninitialised stack; left alone here.)
void MatrixAt(col::CMatrix* m, const V3* pos) {
    m->attached = nullptr;
    PPM_MatrixSetUnityRotation(m);
    m->m[12] = pos->x;
    m->m[13] = pos->y;
    m->m[14] = pos->z;
}

} // namespace

namespace c07ped {

// 0x483690 (thiscall, 5 args): the ped's model swept from where it is towards the point
// (`matrix`'s position at the ped's height), at least twice its vf28 (0x70) far: false when
// it doesn't touch `radius` of it or the sweep hits nothing. On a hit (not against a
// vehicle of class 6 / 7) the point in front of the hit goes to +0x1564.
uint8_t __fastcall MoveAgainstVehicle(uint8_t* self, re::Edx, void* vehicle, int arg2, uint8_t* matrix, V3* move, float radius) {
    V3 target = *reinterpret_cast<V3*>(matrix + 0x30);
    if (!PPM_GetIsTouchingMoving(self, &target, radius)) {
        return 0;
    }
    float speed = static_cast<float>(std::sqrt(std::fabs(move->x * move->x + move->y * move->y + move->z * move->z)));
    float half = VCall<float>(self, 0x70);
    float twice = half + half;
    if (!(twice <= speed)) {
        speed = twice;
    }
    V3 pos = *EntityPos(self);
    col::CMatrix from;
    MatrixAt(&from, &pos);
    V3 flat = {target.x, target.y, pos.z};
    V3 dir;
    PPM_VectorSub(&dir, &pos, &flat);
    *move = dir;
    PPM_Normalise(&dir);
    move->x = speed * dir.x;
    move->y = dir.y * speed;
    move->z = speed * dir.z;
    V3 dest;
    PPM_VectorAdd(&dest, &pos, move);
    col::CMatrix to;
    MatrixAt(&to, &dest);
    uint8_t* inVehicle = At<uint8_t*>(self, 0x1554);
    CColPoint colPoint;
    colPoint.point = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = FLT_MAX;
    int lists = (VCall<uint8_t>(vehicle, 0x14) && !inVehicle) ? 10 : 15;
    void* colModel = At<void*>(PPM_modelInfoPtrs[At<int16_t>(self, 0x10E)], 0xC);
    if (!PPM_ProcessSweep(&to, &from, colModel, matrix, matrix, arg2, &colPoint, 0, 0, 0, 0, 1, lists, 0, 1)) {
        ReleaseColPointEntity(&colPoint);
        return 0;
    }
    int kind;
    if (!VCall<uint8_t>(vehicle, 0x14) || (*PPM_GetVehicleClass(vehicle, &kind) != 6 && *PPM_GetVehicleClass(vehicle, &kind) != 7)) {
        V3 away = *move;
        PPM_Normalise(&away);
        V3 point;
        PPM_VectorAdd(&point, reinterpret_cast<V3*>(&colPoint.point), &away);
        At<float>(self, 0x1564) = point.x;
        At<float>(self, 0x1568) = point.y;
        At<float>(self, 0x156C) = dest.z;
        PPM_Ped482070(self, 1, 0);
        if (inVehicle && At<int32_t>(inVehicle, 0x46C) == 1) {
            PPM_ColPointSetEntity(&colPoint, self);
            PPM_Vehicle4C4830(inVehicle, self + 0x124, &colPoint, 1);
        }
    }
    ReleaseColPointEntity(&colPoint);
    return 1;
}

// 0x483AD0 (vf0): the deleting destructor (the slot back to the ped pool)
uint8_t* __fastcall DeletingDestructor(uint8_t* self, re::Edx, int flags) {
    PPM_PedDestruct(self);
    if (flags & 1) {
        CPool<uint8_t>* pool = PPM_pedPool;
        int slot = static_cast<int>((self - pool->objects) / pool->elemSize);
        pool->flags[slot] |= 0x80;
        if (slot < pool->lastUsed) {
            pool->lastUsed = slot;
        }
    }
    return self;
}

// The ground probe at one corner: (pos + right * side) + forward * ahead, down to z
static void ProbeCorner(uint8_t* self, const V3* pos, float z, re::Ptr32<void>* entity, int lists, float ahead, float side, uint32_t colPoint) {
    V3 forward, scaledForward, right, scaledRight, beside, point;
    PPM_VectorScale(&scaledForward, PPM_GetForward(self, &forward), ahead);
    PPM_VectorAdd(&beside, pos, PPM_VectorScale(&scaledRight, PPM_GetRight(self, &right), side));
    PPM_VectorAdd(&point, &beside, &scaledForward);
    PPM_ProcessVerticalLine(&point, z, &At<CColPoint>(self, colPoint), entity, lists, 0);
}

// 0x483B10 (vf13): with a collision model, a matrix link (+0x1C) and flag 2 of +0x1E24:
// re-added to the sectors, the ground probed at four corners around +0x174C (when +0x1470
// or +0x1474 and +0x1758) and, with `movePartner`, the partner (+0x155C) moved by as much
// as this one moved. Otherwise the +0x1758 reference dropped (and vf37 without a link).
void __fastcall Vf13(uint8_t* self, re::Edx, int movePartner) {
    At<float>(self, 0x19C) = 0.0f;
    At<uint8_t>(self, 0x18A4) = 0;
    At<uint8_t>(self, 0x18A5) = 0;
    At<int32_t>(self, 0x1490) = 0;
    if (At<int32_t>(self, 0xB0) != 0 || !PPM_Physical41C540(self) || At<int32_t>(self, 0x1C) == 0 ||
        !(At<uint8_t>(self, 0x1E24) & 2)) {
        re::Ptr32<void>& ref = At<void*>(self, 0x1758);
        if (ref) {
            PPM_CleanUpOldReference(ref, &ref);
            ref = nullptr;
        }
        if (At<int32_t>(self, 0x1C) == 0) {
            VCall<void>(self, 0x94);
        }
        return;
    }
    V3 before = *EntityPos(self);
    PPM_Ped482070(self, 0, 1);
    PPM_RemoveAndAdd(self);
    if ((At<int32_t>(self, 0x1474) != 0 || At<int32_t>(self, 0x1470) != 0) && At<int32_t>(self, 0x1758) != 0) {
        V3 top = At<V3>(self, 0x174C);
        re::Ptr32<void> entity = nullptr;
        float bottom = top.z - 1.0f;
        top.z = top.z + 1.0f;
        int lists = At<int32_t>(self, 0x148C) != 0 ? 9 : 0xB;
        ProbeCorner(self, &top, bottom, &entity, lists, 0.1f, 0.1f, 0x1774);
        ProbeCorner(self, &top, bottom, &entity, lists, -0.1f, 0.1f, 0x179C);
        ProbeCorner(self, &top, bottom, &entity, lists, -0.1f, -0.1f, 0x17C4);
        ProbeCorner(self, &top, bottom, &entity, lists, 0.1f, -0.1f, 0x17EC);
    }
    if (!re::Flag(movePartner)) {
        return;
    }
    uint8_t* partner = At<uint8_t*>(self, 0x155C);
    if (!partner) {
        return;
    }
    V3 moved;
    PPM_VectorSub(&moved, EntityPos(self), &before);
    if (PPM_debugMoveAnim != 0) {
        PPM_Anim6BA090(At<void*>(self, 0x114), moved, 0, -1.0f);
    }
    if (PPM_flattenPartnerMove != 0) {
        moved.z = 0.0f;
    }
    V3 partnerBefore = *EntityPos(partner);
    V3 partnerPos = {partnerBefore.x + moved.x, moved.y + partnerBefore.y, partnerBefore.z + moved.z};
    PPM_Physical41EE20(partner, &partnerPos);
    PPM_Ped482070(partner, 0, 0);
    PPM_RemoveAndAdd(partner);
    V3 partnerAfter = *EntityPos(partner);
    V3 partnerMoved;
    PPM_VectorSub(&partnerMoved, &partnerAfter, &partnerBefore);
    PPM_Anim6BA090(At<void*>(partner, 0x114), partnerMoved, 0, -1.0f);
}

} // namespace c07ped

#define PPM_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PPM_REVERSED("CPed::MoveAgainstVehicle", 0x00483690, &c07ped::MoveAgainstVehicle);
PPM_REVERSED("CPed::DeletingDestructor", 0x00483AD0, &c07ped::DeletingDestructor);
PPM_REVERSED("CPed::Vf13", 0x00483B10, &c07ped::Vf13);
