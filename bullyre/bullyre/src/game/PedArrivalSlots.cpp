// Peds held back to be placed later (0x48ADD0 .. 0x48B0C0): five slots of 0x20 (+0 a
// flag, +4 the ped (registered), +8 where to put it, +0x14 its heading, +0x18, +0x1C -1)
// and +0xA0 a flag; placing one walks it 3 ahead onto the ground. And the placing of a
// ped near the area's entry point (a random free spot within 3 of it).

#include <cstdint>

#include "game/Pool.h"
#include "reversed.h"
#include "runflags.h"

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

GAME_VAR(V3, PAS_zero, 0x00BCC564);
GAME_VAR(uint8_t*, PAS_playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, PAS_areaTransitions, 0x009E4F4C);   // (+0x8C4 the entry point)
inline void* const PAS_obj_C3CC68 = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(void*, PAS_obj_C2C108, 0x00C2C108);   // (a pointer: the ped spawn manager?)
inline void* const PAS_obj_C2AB70 = reinterpret_cast<void*>(0x00C2AB70);
inline void* const PAS_obj_20C3CA0 = reinterpret_cast<void*>(0x020C3CA0);
GAME_VAR(int32_t, PAS_currArea, 0x00BD1008);   // CGame::currArea
GAME_VAR(int32_t, PAS_keepModel1, 0x00BF42D4);
GAME_VAR(int32_t, PAS_keepModel2, 0x00BF42D0);
GAME_VAR(int32_t, PAS_keepModel3, 0x00BF42CC);
GAME_FN(uint8_t, __thiscall, PAS_Obj6AA840, 0x006AA840, void* self);
GAME_FN(uint8_t, __thiscall, PAS_AllyFollowsPlayer, 0x00485530, void* ally);
GAME_FN(uint8_t, __thiscall, PAS_Ped44E400, 0x0044E400, void* ped);
GAME_FN(uint8_t*, __thiscall, PAS_PedGetModelInfo, 0x00477DC0, void* ped);
GAME_FN(void, __thiscall, PAS_Ped473E60, 0x00473E60, void* ped);
GAME_FN(void, __thiscall, PAS_QueueClear, 0x00470350, void* queue);
GAME_FN(int16_t, __thiscall, PAS_ModelInfo5228D0, 0x005228D0, void* info);
GAME_FN(void, __thiscall, PAS_Spawner49EE40, 0x0049EE40, void* self, int model);
GAME_FN(void, __thiscall, PAS_Spawner4A0B60, 0x004A0B60, void* self, int);
GAME_FN(void, __thiscall, PAS_Spawner49FE90, 0x0049FE90, void* self);
GAME_FN(V3*, __thiscall, PAS_TransitionsGetVec8D0, 0x0048ADD0, void* self, V3* out);
GAME_FN(void, __thiscall, PAS_Hold, 0x0048B960, uint8_t* self, uint8_t* ped, float distance);

GAME_FN(void, __thiscall, PAS_PedSetFlags1E24, 0x00474C10, void* ped, int set, int second);
GAME_FN(void, __thiscall, PAS_RefSet, 0x005D7F10, void* ref, void* entity);
GAME_FN(void, __thiscall, PAS_PedSetHeading, 0x00475730, void* ped, float heading);
GAME_FN(V3*, __cdecl, PAS_DirectionFromHeading, 0x00432DF0, V3* out, float heading);
GAME_FN(float, __thiscall, PAS_Normalise, 0x00414E10, V3* v);
GAME_FN(float, __cdecl, PAS_FindGroundZ, 0x004636B0, float x, float y, float z, uint8_t* found, int);
GAME_FN(void*, __cdecl, PAS_Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void*, __thiscall, PAS_MoveToPosConstruct, 0x004705B0, void* self, void* ped, const V3* pos, float, float, int, int);
GAME_FN(uint8_t, __thiscall, PAS_QueuePrepend, 0x00471470, void* queue, void* objective);
GAME_FN(int, __thiscall, PAS_Obj4F12F0, 0x004F12F0, void* self);
GAME_FN(void, __thiscall, PAS_Ped4732A0, 0x004732A0, void* ped, int, int);
GAME_FN(uint8_t*, __thiscall, PAS_AllyGetTop, 0x00485560, void* ally);
GAME_FN(V3*, __thiscall, PAS_GetRight, 0x004736F0, void* ped, V3* out);
GAME_FN(V3*, __cdecl, PAS_VectorAdd, 0x004129A0, V3* out, const V3* a, const V3* b);
GAME_FN(uint8_t, __thiscall, PAS_Obj49E2C0, 0x0049E2C0, void* self, const V3* pos, void* ped);
GAME_FN(uint8_t, __thiscall, PAS_Obj46EFB0, 0x0046EFB0, void* self, void* ped, const V3* pos, int);
GAME_FN(uint8_t, __cdecl, PAS_IsPosClear5EB7D0, 0x005EB7D0, const V3* pos, int, uintptr_t);
GAME_FN(uint8_t, __thiscall, PAS_Obj49D860, 0x0049D860, void* self, const V3* from, V3* out, int, int, float, float);
GAME_FN(float, __thiscall, PAS_Ped466110, 0x00466110, void* ped);
inline int __cdecl PAS_Rand() { return crt::Rand(); }
GAME_VAR(CPool<uint8_t>*, PAS_pedPool, 0x00C0F5F0);
GAME_VAR(uint32_t, PAS_timeInMs, 0x00C1A9B4);
GAME_VAR(uint8_t*, PAS_ptrNodes, 0x00C0F788);            // CPools::ms_pPtrNodes
GAME_ARRAY(uint8_t*, PAS_modelInfoPtrs, 0x00C67738);
GAME_VAR(int32_t, PAS_skipModel1, 0x00BF3E78);
GAME_VAR(int32_t, PAS_skipModel2, 0x00BF3E74);
inline uint8_t* const PAS_sectors = reinterpret_cast<uint8_t*>(0x00C1B17C);   // 36 x 36, 0x14 each: +0 a list
inline void* const PAS_obj_BCC580 = reinterpret_cast<void*>(0x00BCC580);
GAME_FN(uint8_t, __thiscall, PAS_Ped4765C0, 0x004765C0, void* ped);
GAME_FN(void, __thiscall, PAS_Vehicle4B9D40, 0x004B9D40, void* vehicle);
GAME_FN(void, __thiscall, PAS_Obj4CE960, 0x004CE960, void* self);
GAME_FN(void, __thiscall, PAS_ControllerStop, 0x005F40A0, void* controller);
GAME_FN(uint8_t*, __cdecl, PAS_GetEntityInPool, 0x0044A3E0, int pool, int slot);
GAME_FN(uint8_t, __thiscall, PAS_ModelInfo520440, 0x00520440, void* info);
GAME_FN(float, __cdecl, PAS_Distance, 0x0041A950, const V3* a, const V3* b);
GAME_FN(V3*, __thiscall, PAS_GetForward, 0x004154A0, void* entity, V3* out);
GAME_FN(V3*, __cdecl, PAS_VectorSub, 0x00414B70, V3* out, const V3* a, const V3* b);
GAME_FN(float, __cdecl, PAS_HeadingFromVector, 0x00433310, const V3* v);
GAME_FN(uint8_t, __thiscall, PAS_Obj415380, 0x00415380, void* self);
GAME_FN(uint8_t, __cdecl, PAS_IsNear48C990, 0x0048C990, const V3* pos, float radius, int, uintptr_t);
GAME_FN(uint8_t*, __thiscall, PAS_PedGetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, PAS_QueueGetCurrent, 0x00470340, void* queue);
GAME_FN(void, __thiscall, PAS_Ped497FB0, 0x00497FB0, void* ped, void* target, int);
GAME_FN(void, __thiscall, PAS_PedClearPartner, 0x00477470, void* ped);
// (this file's, through the game's address)
GAME_FN(uint8_t, __cdecl, PAS_FindExitSpot, 0x0048B510, V3* pos, float* heading, void* ped);
GAME_FN(void, __thiscall, PAS_PlaceNearEntry, 0x0048B0C0, void* self, uint8_t* ped);
GAME_FN(void, __thiscall, PAS_Place, 0x0048AF20, uint8_t* self, uint32_t index);
GAME_FN(void, __thiscall, PAS_Release, 0x0048AE80, uint8_t* self, uint32_t index);

void SetPosition(void* entity, V3 pos) {
    reinterpret_cast<void(__thiscall*)(void*, float, float, float)>(
        static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(entity))[0x38 / 4]))(entity, pos.x, pos.y, pos.z);   // (Teleport)
}

// The spot is free for the ped: 0xC2C108's 0x49E2C0, not 0xC2AB70's 0x46EFB0, and clear
bool SpotFree(const V3* pos, void* ped, void* player) {
    return PAS_Obj49E2C0(PAS_obj_C2C108, pos, player) && !PAS_Obj46EFB0(PAS_obj_C2AB70, ped, pos, 1) &&
           PAS_IsPosClear5EB7D0(pos, 1, 0);
}

float RandomOffset() {
    return static_cast<float>(PAS_Rand() / 32767.0 * 6.0 - 3.0);
}

} // namespace

namespace c07pas {

// 0x48ADD0 (the area transitions, 0xBCC580): +0x8D0
V3* __fastcall PedGetVec8D0(uint8_t* self, re::Edx, V3* out) {
    out->x = At<float>(self, 0x8D0);
    out->y = At<float>(self, 0x8D4);
    out->z = At<float>(self, 0x8D8);
    return out;
}

// 0x48AE00: the held peds deleted, the slots cleared
void __fastcall DeleteAll(uint8_t* self, re::Edx) {
    for (int i = 0; i < 5; i++) {
        uint8_t* slot = self + i * 0x20;
        void* ped = At<void*>(slot, 4);
        if (!ped) {
            continue;
        }
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0]))(ped, 1);
        At<uint8_t>(slot, 0) = 0;
        At<V3>(slot, 8) = PAS_zero;
        At<float>(slot, 0x14) = 0.0f;
        At<int32_t>(slot, 0x18) = 0;
        At<int32_t>(slot, 0x1C) = -1;
    }
    At<uint8_t>(self, 0xA0) = 1;
}

// 0x48AE80: a slot's ped let go (its +0x1358 and flag 1 of +0x1E24 cleared)
void __fastcall Release(uint8_t* self, re::Edx, uint32_t index) {
    if (index > 4) {
        return;
    }
    uint8_t* slot = self + index * 0x20;
    uint8_t* ped = At<uint8_t*>(slot, 4);
    if (!ped) {
        return;
    }
    At<int32_t>(ped, 0x1358) = 0;
    PAS_PedSetFlags1E24(At<void*>(slot, 4), 1, 0);
    PAS_RefSet(slot + 4, nullptr);
    At<uint8_t>(slot, 0) = 0;
    At<V3>(slot, 8) = PAS_zero;
    At<float>(slot, 0x14) = 0.0f;
    At<int32_t>(slot, 0x18) = 0;
    At<int32_t>(slot, 0x1C) = -1;
}

// 0x48AEF0: the ped is held
uint8_t __fastcall IsHeld(uint8_t* self, re::Edx, void* ped) {
    if (ped) {
        for (int i = 0; i < 5; i++) {
            if (At<void*>(self + i * 0x20, 4) == ped) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x48AF20: a held ped placed: at its spot and heading, told to walk 3 ahead (onto the
// ground), and let go
void __fastcall Place(uint8_t* self, re::Edx, uint32_t index) {
    uint8_t* slot = self + index * 0x20;
    uint8_t* ped = At<uint8_t*>(slot, 4);
    if (!ped) {
        return;
    }
    SetPosition(ped, At<V3>(slot, 8));
    PAS_PedSetHeading(At<void*>(slot, 4), At<float>(slot, 0x14));
    V3 target = At<V3>(slot, 8);
    V3 dir;
    PAS_DirectionFromHeading(&dir, At<float>(slot, 0x14));
    dir.z = 0.0f;
    PAS_Normalise(&dir);
    uint8_t found = 0;
    V3 step = {static_cast<float>(dir.x * 3.0), static_cast<float>(dir.y * 3.0), static_cast<float>(dir.z * 3.0)};
    target.x = step.x + target.x;
    target.y = step.y + target.y;
    target.z = step.z + target.z;
    float above = static_cast<float>(target.z + 1.0);
    float ground = PAS_FindGroundZ(target.x, target.y, above, &found, 1);
    if (found) {
        target.z = ground;
    }
    void* memory = PAS_Malloc(0x34);
    void* objective = memory ? PAS_MoveToPosConstruct(memory, At<void*>(slot, 4), &target, 0.3f, 0.3f, 0, 0) : nullptr;
    PAS_QueuePrepend(At<uint8_t*>(slot, 4) + 0x5F0, objective);
    if (PAS_Obj4F12F0(PAS_obj_C3CC68) == 1) {
        PAS_Ped4732A0(At<void*>(slot, 4), 0xFF, 0);
    }
    PAS_Release(self, index);
}

// 0x48B0C0: a ped placed near the area's entry point: a random free spot within 3 of it
// (200 tries; the spot beside the player, for its follower, is worked out and not used:
// the game's count check fails for it), else one 0xC2C108 finds, else the point itself,
// on the ground
void __fastcall PlaceNearEntry(void*, re::Edx, uint8_t* ped) {
    if (!ped) {
        return;
    }
    uint8_t* player = PAS_playerPed;
    V3 pos = PAS_zero;
    V3 entry = At<V3>(PAS_areaTransitions, 0x8C4);
    pos = entry;
    bool besidePlayer = false;
    uint8_t found;
    if (At<int32_t>(ped, 0x153C) != 0 && player == PAS_AllyGetTop(ped + 0x2F0)) {
        V3 right;
        V3* r = PAS_GetRight(player, &right);
        V3 offset;
        float y = static_cast<float>(r->y * 1.5);
        float z = static_cast<float>(r->z * 1.5);
        offset.x = static_cast<float>(1.5 * r->x);
        offset.y = y;
        offset.z = z;
        V3 sum;
        pos = *PAS_VectorAdd(&sum, EntityPos(player), &offset);
        found = 0;
        float ground = PAS_FindGroundZ(pos.x, pos.y, static_cast<float>(pos.z + 1.0), &found, 1);
        if (found) {
            pos.z = ground;
        }
        if (SpotFree(&pos, ped, player)) {
            besidePlayer = true;
        }
    }
    int tries = 0;
    bool placed = false;
    do {
        tries++;
        pos.x = RandomOffset() + entry.x;
        float dy = RandomOffset();
        found = 0;
        pos.y = dy + entry.y;
        float ground = PAS_FindGroundZ(pos.x, pos.y, static_cast<float>(pos.z + 1.0), &found, 1);
        if (found) {
            pos.z = ground;
            if (SpotFree(&pos, ped, player)) {
                placed = true;
                break;
            }
        }
    } while (tries < 200);
    if (!placed) {
        tries++;
    }
    if ((placed || besidePlayer) && tries < 200) {
        SetPosition(ped, pos);
        return;
    }
    if (!PAS_Obj49D860(PAS_obj_C2C108, &entry, &pos, 1, 0, 0.0f, 0.0f)) {
        pos = entry;
    }
    found = 0;
    float ground = PAS_FindGroundZ(pos.x, pos.y, static_cast<float>(pos.z + 1.0), &found, 1);
    if (found) {
        pos.z = ground;
    } else {
        pos.z = PAS_Ped466110(ped) + pos.z;
    }
    SetPosition(ped, pos);
}

// 0x48B430 (static): the peds but the player that 0x4765C0 accepts deleted
void __cdecl DeleteAmbientPeds() {
    CPool<uint8_t>* pool = PAS_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (!ped || At<int32_t>(ped, 0x1310) == 13 || !PAS_Ped4765C0(ped)) {
            continue;
        }
        reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0]))(ped, 1);
    }
}

// 0x48B480 (static): every ped but the player stopped: seated ones (2) leave their
// vehicle's +0x2E0 at 0 (0x4B9D40 first for a +0x46C 1 one), others stop their vehicle's
// +0x1C4 and their main action
void __cdecl StopPeds() {
    CPool<uint8_t>* pool = PAS_pedPool;
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (!ped || At<int32_t>(ped, 0x1310) == 13) {
            continue;
        }
        uint8_t* vehicle = At<uint8_t*>(ped, 0x1554);
        if (At<uint8_t>(ped, 0x1570) == 2 && vehicle) {
            if (At<int32_t>(vehicle, 0x46C) == 1) {
                PAS_Vehicle4B9D40(vehicle);
            }
            At<float>(At<uint8_t*>(ped, 0x1554), 0x2E0) = 0.0f;
        } else {
            if (At<uint8_t*>(ped, 0x1554)) {
                PAS_Obj4CE960(At<uint8_t*>(ped, 0x1554) + 0x1C4);
            }
            PAS_ControllerStop(At<uint8_t*>(ped, 0x2E0) + 0x258);
        }
    }
}

// 0x48B510 (cdecl): the nearest door (an object whose model 0x520440 accepts, without
// +0x180, not the two excluded models) within 10 of the player in the player's sector:
// the spot half a unit off it on the player's side, on the ground, and the heading from
// there to the player
uint8_t __cdecl FindExitSpot(V3* pos, float* heading, void* ped) {
    uint8_t found = 0;
    V3 player = *EntityPos(PAS_playerPed);
    int sx = static_cast<int>(player.x / 50.0 + 18.0);
    if (sx < 0) {
        sx = 0;
    } else if (sx > 35) {
        sx = 35;
    }
    int sy = static_cast<int>(player.y / 50.0 + 18.0);
    if (sy < 0) {
        sy = 0;
    } else if (sy > 35) {
        sy = 35;
    }
    float best = 10.0f;
    uint32_t* node = At<uint32_t*>(PAS_sectors, (sx + sy * 36) * 0x14);
    if (!node) {
        return 0;
    }
    do {
        uint8_t* entity = PAS_GetEntityInPool(*node & 0xF, (*node >> 4) & 0x3FFF);
        uint32_t next = *node >> 18;
        node = next == 0x3FFF ? nullptr : reinterpret_cast<uint32_t*>(PAS_ptrNodes + next * 4);
        if ((At<uint8_t>(entity, 0x108) & 7) != 6 || !PAS_ModelInfo520440(PAS_modelInfoPtrs[At<int16_t>(entity, 0x10E)]) ||
            At<int32_t>(entity, 0x180) != 0) {
            continue;
        }
        int model = At<int16_t>(entity, 0x10E);
        if (model == PAS_skipModel1 || model == PAS_skipModel2) {
            continue;
        }
        V3 door = *EntityPos(entity);
        float distance = PAS_Distance(&door, &player);
        if (!(best > distance)) {
            continue;
        }
        best = distance;
        V3 forward;
        PAS_GetForward(entity, &forward);
        forward.z = 0.0f;
        PAS_Normalise(&forward);
        V3 d;
        V3 toPlayer = *PAS_VectorSub(&d, &player, &door);
        toPlayer.z = 0.0f;
        PAS_Normalise(&toPlayer);
        float facing = forward.x * toPlayer.x + forward.y * toPlayer.y + forward.z * toPlayer.z;
        V3 half = {static_cast<float>(forward.x * 0.5), static_cast<float>(forward.y * 0.5), static_cast<float>(forward.z * 0.5)};
        V3 tmp;
        V3* spot = (facing > 0.0f) ? PAS_VectorSub(&tmp, &door, &half) : PAS_VectorAdd(&tmp, &door, &half);
        *pos = *spot;
        if (PAS_Obj46EFB0(PAS_obj_C2AB70, ped, pos, 1)) {
            continue;
        }
        uint8_t onGround = 0;
        float above = static_cast<float>(pos->z + 2.0);
        float ground = PAS_FindGroundZ(pos->x, pos->y, above, &onGround, 1);
        if (onGround) {
            pos->z = ground;
        }
        V3 back;
        PAS_VectorSub(&back, &player, pos);
        back.z = 0.0f;
        PAS_Normalise(&back);
        *heading = PAS_HeadingFromVector(&back);
        found = 1;
    } while (node);
    return found;
}

// 0x48B890: a step: while 0xBCC580's 0x415380 holds, the held peds' exit spots found;
// else each held ped waits its delay, then is placed (at its spot when that's clear,
// else near the entry point)
void __fastcall Update(uint8_t* self, re::Edx) {
    if (PAS_Obj415380(PAS_obj_BCC580)) {
        for (int i = 0; i < 5; i++) {
            uint8_t* slot = self + i * 0x20;
            void* ped = At<void*>(slot, 4);
            if (ped && !At<uint8_t>(slot, 0)) {
                At<uint8_t>(slot, 0) = PAS_FindExitSpot(&At<V3>(slot, 8), &At<float>(slot, 0x14), ped);
            }
        }
        return;
    }
    for (uint32_t i = 0; i < 5; i++) {
        uint8_t* slot = self + i * 0x20;
        uint8_t* ped = At<uint8_t*>(slot, 4);
        if (!ped) {
            continue;
        }
        uint32_t& due = At<uint32_t>(slot, 0x1C);
        if (due == 0xFFFFFFFF) {
            due = At<uint32_t>(slot, 0x18) + PAS_timeInMs;
            continue;
        }
        if (!(PAS_timeInMs > due) && At<uint8_t>(slot, 0)) {
            continue;
        }
        if (!At<uint8_t>(slot, 0)) {
            PAS_PlaceNearEntry(self, ped);
            PAS_Release(self, i);
        } else if (PAS_IsNear48C990(&At<V3>(slot, 8), 0.2f, -1, 0)) {
            PAS_Place(self, i);
        }
    }
}

// 0x48B960: a ped held in the first free slot (2 seconds apart), with an exit spot if
// there's one; its attack objective set to 3 and its partner dropped
void __fastcall Hold(uint8_t* self, re::Edx, uint8_t* ped, float) {
    float heading = 0.0f;
    V3 pos = PAS_zero;
    uint8_t found = 0;
    if (PAS_FindExitSpot(&pos, &heading, nullptr)) {
        found = 1;
    }
    int index = 0;
    for (; index < 5; index++) {
        if (At<void*>(self + index * 0x20, 4) == nullptr) {
            break;
        }
    }
    if (index >= 5) {
        return;
    }
    uint8_t* slot = self + index * 0x20;
    PAS_RefSet(slot + 4, ped);
    At<int32_t>(slot, 0x18) = index * 2000;
    At<int32_t>(slot, 0x1C) = -1;
    At<int32_t>(At<uint8_t*>(slot, 4), 0x1358) = 1;
    PAS_PedSetFlags1E24(At<void*>(slot, 4), 0, 0);
    At<float>(slot, 0x14) = heading;
    At<uint8_t>(slot, 0) = found;
    At<V3>(slot, 8) = pos;
    uint8_t* target = PAS_PedGetTarget(ped);
    if (target && (At<uint8_t>(target, 0x108) & 7) == 3 && At<int32_t>(PAS_QueueGetCurrent(ped + 0x5F0), 8) == 7) {
        uint8_t* current = PAS_QueueGetCurrent(ped + 0x5F0);
        if (current && At<int32_t>(current, 8) != 7) {
            current = nullptr;
        }
        At<int32_t>(current, 0x10) = 3;
        PAS_Ped497FB0(ped, target, 3);
    }
    uint8_t* partner = At<uint8_t*>(ped, 0x155C);
    if (partner) {
        if (At<uint8_t*>(partner, 0x155C) == ped) {
            PAS_PedClearPartner(partner);
        }
        PAS_PedClearPartner(ped);
    }
}

// 0x48BAC0: the area changed: every ped but the player dealt with: the player's
// followers come along (placed near the entry point, partners dropped, objectives
// cleared), a ped attacking the player near the way in (within 10, with +0x1C99) is held
// to follow in, and the rest that 0x4765C0 accepts (or that are in a group of kind 3..15
// but 3) deleted, the spawner told to make their model again where the area allows
void __fastcall AreaChanged(uint8_t* self, re::Edx) {
    CPool<uint8_t>* pool = PAS_pedPool;
    int slot = pool->size;
    uint8_t cutscene = PAS_Obj6AA840(PAS_obj_20C3CA0);
    int area = PAS_currArea;
    bool inA = area == 0 || area == 2 || area == 9 || area == 0xD;
    bool inB = area == 0x26 || area == 0x25 || area == 0x20 || area == 0x24 || area == 2 || area == 0xE || area == 0x23 ||
               area == 0x36;
    bool inC = area == 0x26 || area == 0x25 || area == 0x20 || area == 0x24 || area == 0x36 || area == 0x1E || area == 0x39 ||
               area == 0x3B || area == 0x3C || area == 0x3D;
    // (the remaster's seamless door, runflags.h: the followers come along as ever; the rest are left
    // where they are, the area left's by its door and the new one's already about, both peopled)
    bool keep = re::SeamlessKeepsPeds();
    while (slot != 0) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (!ped) {
            continue;
        }
        At<int32_t>(ped, 0x18A8) = 0;
        At<int32_t>(ped, 0x18AC) = -1;
        bool respawn = true;
        if (At<int32_t>(ped, 0x1310) == 13) {
            continue;
        }
        float distance = 0.0f;
        bool hold = false;
        if (!cutscene && PAS_AllyFollowsPlayer(ped + 0x2F0)) {
            if (inA || At<int32_t>(ped, 0x141C) != 0 || (!PAS_Ped44E400(ped) && !At<uint8_t>(PAS_PedGetModelInfo(ped), 0x50))) {
                PAS_PlaceNearEntry(self, ped);
                uint8_t* partner = At<uint8_t*>(ped, 0x155C);
                if (partner) {
                    if (At<uint8_t*>(partner, 0x155C) == ped) {
                        PAS_PedClearPartner(partner);
                    }
                    PAS_PedClearPartner(ped);
                }
                PAS_Ped473E60(ped);
                PAS_QueueClear(ped + 0x5F0);
                At<int32_t>(ped, 0x318) = 1;
            }
            continue;
        }
        if (keep) {
            continue;
        }
        if (At<uint8_t>(self, 0xA0)) {
            uint8_t* target = PAS_PedGetTarget(ped);
            if (target && (At<uint8_t>(target, 0x108) & 7) != 3) {
                target = nullptr;
            }
            int model = At<int16_t>(ped, 0x10E);
            if (At<uint8_t>(ped, 0x1C99) && model != PAS_keepModel1 && model != PAS_keepModel2 && model != PAS_keepModel3 &&
                target && At<int32_t>(target, 0x1310) == 13) {
                uint8_t* current = PAS_QueueGetCurrent(ped + 0x5F0);
                if (current && At<int32_t>(current, 8) == 7) {
                    V3 tmp;
                    distance = PAS_Distance(EntityPos(ped), PAS_TransitionsGetVec8D0(PAS_areaTransitions, &tmp));
                    if (!(10.0f < distance || distance != distance)) {
                        hold = true;
                    }
                }
            }
        }
        int type = At<int32_t>(ped, 0x1310);
        bool clear;
        if (type == 0 || type == 8 || type == 7) {
            clear = !inA;
        } else if (type == 9) {
            clear = inB;
        } else {
            clear = inC;
        }
        if (clear) {
            hold = false;
            respawn = false;
        }
        if (((type > 0 && type < 7) || type == 0xB || type == 0xD) && !At<uint8_t>(PAS_PedGetModelInfo(ped), 0x50) &&
            PAS_currArea == 0x23) {
            hold = false;
            respawn = false;
        }
        type = At<int32_t>(ped, 0x1310);
        if (((type > 0 && type < 7) || type == 0xB || type == 0xD) && At<uint8_t>(PAS_PedGetModelInfo(ped), 0x50) &&
            PAS_currArea == 0xE) {
            hold = false;
            respawn = false;
        }
        bool again;
        if (!inA && At<int32_t>(ped, 0x141C) == 0 && (PAS_Ped44E400(ped) || At<uint8_t>(PAS_PedGetModelInfo(ped), 0x50))) {
            again = false;
        } else if (hold) {
            PAS_Hold(self, ped, distance);
            PAS_Ped473E60(ped);
            continue;
        } else {
            again = respawn;
        }
        uint8_t* group = At<uint8_t*>(ped, 0x1D24);
        if (PAS_Ped4765C0(ped) ||
            (group && At<int32_t>(group, 0x70) != 3 && At<int32_t>(group, 0x70) > 2 && At<int32_t>(group, 0x70) < 0x10)) {
            int model = At<int16_t>(ped, 0x10E);
            reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0]))(ped, 1);
            if (PAS_ModelInfo5228D0(PAS_modelInfoPtrs[model]) != -1 && again) {
                PAS_Spawner49EE40(PAS_obj_C2C108, model);
                PAS_Spawner4A0B60(PAS_obj_C2C108, 1);
            }
        }
    }
    if (!keep) {
        PAS_Spawner49FE90(PAS_obj_C2C108);   // (those seen going in made again inside: kept, they're there)
    }
}

} // namespace c07pas

#define PAS_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

PAS_REVERSED("AreaTransitions::GetVec8D0", 0x0048ADD0, &c07pas::PedGetVec8D0);
PAS_REVERSED("PedArrivals::DeleteAll", 0x0048AE00, &c07pas::DeleteAll);
PAS_REVERSED("PedArrivals::Release", 0x0048AE80, &c07pas::Release);
PAS_REVERSED("PedArrivals::IsHeld", 0x0048AEF0, &c07pas::IsHeld);
PAS_REVERSED("PedArrivals::Place", 0x0048AF20, &c07pas::Place);
PAS_REVERSED("PedArrivals::PlaceNearEntry", 0x0048B0C0, &c07pas::PlaceNearEntry);
PAS_REVERSED("PedArrivals::DeleteAmbientPeds", 0x0048B430, &c07pas::DeleteAmbientPeds);
PAS_REVERSED("PedArrivals::StopPeds", 0x0048B480, &c07pas::StopPeds);
PAS_REVERSED("PedArrivals::FindExitSpot", 0x0048B510, &c07pas::FindExitSpot);
PAS_REVERSED("PedArrivals::Update", 0x0048B890, &c07pas::Update);
PAS_REVERSED("PedArrivals::Hold", 0x0048B960, &c07pas::Hold);
PAS_REVERSED("PedArrivals::AreaChanged", 0x0048BAC0, &c07pas::AreaChanged);
