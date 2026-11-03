// The pickups (GTA's CPickups): 300 of 0x48 at 0xC0A100 (+0x46 flags: bit 3 in use),
// handles (the slot in the low word, a generation (+0x40) in the high), the collected
// pickups' table (100 keys and values at 0xC09980) and the collected bits (250 keys at
// 0xC09CC0, bits at 0xC09CA0); a pickup's key is +0xC * 10000 + (+0x30 << 16) + +8.
// Also the ActionTree ids some butes recognise, and a few small neighbours (CVector2D's
// Normalise, the entity's LTM and RW matrix update).

#include <cstdint>

#include "game/Collision.h"
#include "reversed.h"

namespace {

GAME_ARRAY(uint8_t, ms_pickups, 0x00C0A100);          // 300 of 0x48
GAME_ARRAY(int32_t, ms_collected, 0x00C09980);        // 100 of {key, value}
GAME_VAR(uint32_t, ms_collectedNext, 0x00C0997C);     // (the next slot to reuse)
GAME_ARRAY(int32_t, ms_collectedKeys, 0x00C09CC0);    // 250
GAME_ARRAY(uint32_t, ms_collectedBits, 0x00C09CA0);
GAME_VAR(int32_t, ms_numCollectedKeys, 0x00C09978);
GAME_ARRAY(int32_t, ms_list_C09960, 0x00C09960);
GAME_VAR(int32_t, ms_listCount_C09974, 0x00C09974);
// the ActionTree reference ids (made on first use from the counter)
GAME_VAR(uint16_t, ms_actionTreeIdCounter, 0x00BF3830);
GAME_VAR(uint16_t, ms_idA, 0x00C0F5B8);
GAME_VAR(uint32_t, ms_idAGuard, 0x00C0F5BC);
GAME_VAR(uint16_t, ms_idB, 0x00C0F5B0);
GAME_VAR(uint32_t, ms_idBGuard, 0x00C0F5B4);
GAME_VAR(uint16_t, ms_idC, 0x00C0F5C0);
GAME_VAR(uint32_t, ms_idCGuard, 0x00C0F5C4);

GAME_FN(float, __cdecl, SafeDivideBySqrt, 0x00414C20, float value, float lengthSq);
GAME_FN(void*, __cdecl, Rw_GetLTM, 0x006C9A90, void* frame);
GAME_FN(void*, __thiscall, Entity_GetMatrix, 0x008B6120, void* entity);
GAME_FN(void, __thiscall, Matrix_UpdateRW, 0x00413A70, void* matrix);
GAME_FN(void, __thiscall, Placement_SetFromLTM, 0x0046D8F0, void* placement, void* ltm);
GAME_FN(void*, __thiscall, Entity_GetLTM, 0x00443790, void* entity);
GAME_FN(uint8_t, __cdecl, IsActionTreeIdAB, 0x004437B0, int id);
GAME_FN(void, __thiscall, PickupButes_Destruct, 0x0072A900, void* self);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

using re::At;

int32_t PickupKey(uint8_t* p) {
    return At<int32_t>(p, 0xC) * 10000 + (At<int32_t>(p, 0x30) << 16) + At<int32_t>(p, 8);
}

uint16_t ActionTreeId(uint16_t& id, uint32_t& guard) {
    if (!(guard & 1)) {
        guard |= 1;
        id = ms_actionTreeIdCounter;
        ms_actionTreeIdCounter = uint16_t(ms_actionTreeIdCounter + 1);
    }
    return id;
}

} // namespace

namespace Pickups {

// 0x443730 (thiscall): CVector2D::Normalise (a zero or negative length: (1, y))
static void __fastcall Vector2D_Normalise(float* self, re::Edx) {
    float lengthSq = float(double(self[0]) * self[0] + double(self[1]) * self[1]);
    if (!(lengthSq > 0.0f) && lengthSq == lengthSq) {
        self[0] = 1.0f;
        return;
    }
    float scale = SafeDivideBySqrt(1.0f, lengthSq);
    self[0] = self[0] * scale;
    self[1] = scale * self[1];
}

// 0x443790 (thiscall): the entity's frame's LTM (nullptr without a frame)
static void* __fastcall Entity_GetLTMImpl(uint8_t* self, re::Edx) {
    void* object = At<void*>(self, 0x18);
    if (object) {
        if (void* frame = At<void*>(object, 4)) {
            return Rw_GetLTM(frame);
        }
    }
    return nullptr;
}

// 0x4437B0: one of the two ActionTree ids (made on first use)
static uint8_t __cdecl IsActionTreeIdABImpl(int id) {
    ActionTreeId(ms_idA, ms_idAGuard);
    if (int16_t(id) == int16_t(ms_idA)) {
        return 1;
    }
    ActionTreeId(ms_idB, ms_idBGuard);
    return int16_t(id) == int16_t(ms_idB);
}

// VehicleCamButes vf0 (0x443820): its ActionTree id, or one of the two
static uint8_t __fastcall VehicleCamButes_IsId(void*, re::Edx, int id) {
    ActionTreeId(ms_idC, ms_idCGuard);
    if (int16_t(id) == int16_t(ms_idC)) {
        return 1;
    }
    return IsActionTreeIdAB(id);
}

// 0x443870: the pickup in use with that +0x1C (nullptr: none)
static uint8_t* __cdecl FindPickupBy1C(int32_t value) {
    for (int i = 0; i < 300; i++) {
        uint8_t* p = ms_pickups + i * 0x48;
        if ((p[0x46] & 8) && At<int32_t>(p, 0x1C) == value) {
            return p;
        }
    }
    return nullptr;
}

// 0x4438B0: the handle's slot (-1: none, not in use, or an old generation)
static int __cdecl GetActualPickupIndex(uint32_t handle) {
    uint32_t slot = handle & 0xFFFF;
    if (handle == 0xFFFFFFFF) {
        return -1;
    }
    uint8_t* p = ms_pickups + slot * 0x48;
    if (!(p[0x46] & 8) || int16_t(handle >> 16) != At<int16_t>(p, 0x40)) {
        return -1;
    }
    return int(slot);
}

// 0x4438F0: the pickup's collected value (+0x24) back from the table (+0x44 cleared)
static uint8_t __cdecl RestoreCollected(uint8_t* p) {
    int32_t key = PickupKey(p);
    for (uint32_t i = 0; i < 100; i++) {
        int32_t k = ms_collected[i * 2];
        if (k == key) {
            p[0x44] = 0;
            At<int32_t>(p, 0x24) = ms_collected[i * 2 + 1];
            return 1;
        }
        if (k == -1) {
            return 0;
        }
    }
    return 0;
}

// 0x443940: the pickup's value (+0x24) into the table (its key's slot, else the next one,
// round the 100)
static void __cdecl StoreCollected(uint8_t* p) {
    int32_t key = PickupKey(p);
    uint32_t slot = ms_collectedNext;
    bool fresh = true;
    for (uint32_t i = 0; i < 100; i++) {
        int32_t k = ms_collected[i * 2];
        if (k == key) {
            slot = i;
            fresh = false;
            break;
        }
        if (k == -1) {
            break;
        }
    }
    ms_collected[slot * 2] = key;
    ms_collected[slot * 2 + 1] = At<int32_t>(p, 0x24);
    if (fresh) {
        if (++ms_collectedNext >= 100) {
            ms_collectedNext = 0;
        }
    }
}

// 0x4439C0: the pickup's key added to the collected bits (its bit set)
static void __cdecl AddCollectedKey(uint8_t* p) {
    int32_t n = ms_numCollectedKeys;
    ms_collectedKeys[n] = PickupKey(p);
    ms_collectedBits[uint32_t(n) >> 5] |= 1u << (n & 0x1F);
    ms_numCollectedKeys = n + 1;
}

// 0x443A10: is the key (b[1] * 10000 + (a << 16) + b[0]) collected
static uint8_t __cdecl IsKeyCollected(int a, const int32_t* b) {
    int32_t key = b[1] * 10000 + (a << 16) + b[0];
    for (uint32_t i = 0; i < 250; i++) {
        if (ms_collectedKeys[i] == key) {
            return (ms_collectedBits[i >> 5] & (1u << (i & 0x1F))) != 0;
        }
    }
    return 0;
}

// 0x443A60: the value out of the list at 0xC09960 (the last one moved into its place;
// that one isn't checked again)
static void __cdecl RemoveFromList(int32_t value) {
    int count = ms_listCount_C09974;
    if (count <= 0) {
        return;
    }
    for (int i = 0; i < count; i++) {
        if (value == ms_list_C09960[i]) {
            ms_list_C09960[i] = ms_list_C09960[count - 1];
            count--;
        }
    }
    ms_listCount_C09974 = count;
}

// 0x443AA0 (thiscall): the entity's matrix (or placement) from its frame's LTM
static void __fastcall Entity_UpdateFromRW(uint8_t* self, re::Edx) {
    void* object = At<void*>(self, 0x18);
    if (!object) {
        return;
    }
    void* frame = At<void*>(object, 4);
    if (!frame) {
        return;
    }
    void* ltm = Rw_GetLTM(frame);
    if (!ltm) {
        return;
    }
    if (At<void*>(self, 0x14)) {
        if (At<void*>(Entity_GetMatrix(self), 0x40) == nullptr) {
            void* entityLtm = Entity_GetLTM(self);
            void* matrix = Entity_GetMatrix(self);
            At<void*>(matrix, 0x40) = entityLtm;
            Matrix_UpdateRW(matrix);
        } else {
            Matrix_UpdateRW(Entity_GetMatrix(self));
        }
        return;
    }
    Placement_SetFromLTM(self + 4, ltm);
}

// PickupButes vf1 (0x443B10): the destructor (flags 1: and freed)
static void* __fastcall PickupButes_Destroy(void* self, re::Edx, int flags) {
    PickupButes_Destruct(self);
    if (flags & 1) {
        Free(self);
    }
    return self;
}

} // namespace Pickups

// ---- registration ----------------------------------------------------------------------

#define PICKUPS_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                            \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace Pickups {
PICKUPS_REVERSED("CVector2D::Normalise", 0x00443730, &Vector2D_Normalise);
PICKUPS_REVERSED("CEntity::GetLTM", 0x00443790, &Entity_GetLTMImpl);
PICKUPS_REVERSED("ActionTree::IsIdAB", 0x004437B0, &IsActionTreeIdABImpl);
PICKUPS_REVERSED("VehicleCamButes::IsId", 0x00443820, &VehicleCamButes_IsId);
PICKUPS_REVERSED("CPickups::FindPickupBy1C", 0x00443870, &FindPickupBy1C);
PICKUPS_REVERSED("CPickups::GetActualPickupIndex", 0x004438B0, &GetActualPickupIndex);
PICKUPS_REVERSED("CPickups::RestoreCollected", 0x004438F0, &RestoreCollected);
PICKUPS_REVERSED("CPickups::StoreCollected", 0x00443940, &StoreCollected);
PICKUPS_REVERSED("CPickups::AddCollectedKey", 0x004439C0, &AddCollectedKey);
PICKUPS_REVERSED("CPickups::IsKeyCollected", 0x00443A10, &IsKeyCollected);
PICKUPS_REVERSED("CPickups::RemoveFromList", 0x00443A60, &RemoveFromList);
PICKUPS_REVERSED("CEntity::UpdateFromRW", 0x00443AA0, &Entity_UpdateFromRW);
PICKUPS_REVERSED("PickupButes::Destroy", 0x00443B10, &PickupButes_Destroy);
} // namespace Pickups
