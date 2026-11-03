// A ped's tether (CPed +0x358, 0x499590 .. 0x499CC0): where the ped may go (a
// position, an entity, or a trigger area, with a radius) and an anti-tether where it
// may not. The script natives PedSetTether* / PedSetAntiTether* / PedClearTether set it.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace tether {

struct CVector {
    float x, y, z;
};
static_assert(sizeof(CVector) == 0xC);

// The kinds (+4)
enum : int32_t {
    TETHER_NONE = 0,
    TETHER_POSITION = 1,
    TETHER_ENTITY = 2,   // the entity's position plus an offset in its space
    TETHER_TRIGGER = 3,  // a trigger area (the script's triggers, 0x20C7E98)
};

// One tether (the ped's, or the anti-tether)
struct TetherTarget {
    re::Ptr32<void> owner;         // +0x0 the ped (CEntity)
    int32_t type;        // +0x4
    float radius;        // +0x8
    CVector pos;         // +0xC (kept up to date for an entity by Update)
    re::Ptr32<void> entity;        // +0x18 (registered: 0x5D7F10)
    CVector offset;      // +0x1C in the entity's space
    int32_t trigger;     // +0x28 -1: none

    void* GetEntity();
    CVector* GetPosition(CVector* out);
    void* GetPed();
    uint8_t IsInside(CVector* pos, float margin);
    uint8_t ClampInside(CVector* pos, float margin);
    void Update();
};
static_assert(offsetof(TetherTarget, type) == 0x4);
static_assert(offsetof(TetherTarget, pos) == 0xC);
static_assert(offsetof(TetherTarget, entity) == 0x18);
static_assert(offsetof(TetherTarget, offset) == 0x1C);
static_assert(offsetof(TetherTarget, trigger) == 0x28);
static_assert(sizeof(TetherTarget) == 0x2C);

struct PedTether {
    re::Ptr32<void> owner;           // +0x0 the ped
    TetherTarget tether;   // +0x4
    TetherTarget anti;     // +0x30
    uint8_t unknown5C;     // +0x5C (1 whenever the tether is set or cleared)
    uint8_t unknown5D;     // +0x5D (0 then)
    uint8_t pad5E[2];
    float speed;           // +0x60 (PedSetTetherSpeed; 1.5 at first)

    uint8_t IsPosAllowed(CVector* pos, float margin);
    uint8_t IsEntityAllowed(void* entity, float margin);
    uint8_t IsOwnerAllowed(float margin);
    uint8_t Clamp(CVector* pos, float margin);
    void Update();
    void Clear();
    void SetToPosition(const CVector* pos, float radius);
    void SetToEntity(void* entity, float radius, const CVector* offset);
    void SetToTrigger(int trigger);
    void ClearAnti();
    void SetAntiToPosition(const CVector* pos, float radius);
    void SetAntiToEntity(void* entity, float radius, const CVector* offset);
    void SetAntiToTrigger(int trigger);
    void Init();
};
static_assert(offsetof(PedTether, tether) == 0x4);
static_assert(offsetof(PedTether, anti) == 0x30);
static_assert(offsetof(PedTether, unknown5C) == 0x5C);
static_assert(offsetof(PedTether, speed) == 0x60);
static_assert(sizeof(PedTether) == 0x64);

} // namespace tether

using tether::CVector;
using tether::PedTether;
using tether::TetherTarget;

namespace {
GAME_FN(void, __cdecl, PT_TransformPoint, 0x0046D850, CVector* out, const void* transform, const CVector* v);
// (0x0041A9A0 stores 16 bytes through `out`: called through re::CallVec16)
static inline CVector* PT_MultiplyPoint(CVector* out, const void* matrix, const CVector* v) { return re::CallVec16<0x0041A9A0, CVector*>(out, matrix, v); }
// the distance past the sphere's edge (<= 0: inside)
GAME_FN(float, __cdecl, PT_DistanceOutside, 0x00487E50, const CVector* pos, const CVector* center, float radius);
GAME_FN(CVector*, __cdecl, PT_VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, PT_VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, PT_Normalise, 0x00414E10, CVector* v);
// the script's triggers (0x20C7E98): one by index (nullptr: none)
GAME_FN(void*, __thiscall, PT_GetTrigger, 0x00415FD0, void* triggers, int index);
GAME_FN(uint8_t, __thiscall, PT_TriggerContains, 0x006D7020, void* trigger, CVector* pos, float margin);
GAME_FN(uint8_t, __thiscall, PT_TriggerClamp, 0x006D68D0, void* trigger, CVector* pos, float margin);
GAME_FN(void, __thiscall, PT_RefSet, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
// (ours below, called where modes apply)
GAME_FN(CVector*, __thiscall, PT_EntityTransformOffset, 0x00499630, void* entity, CVector* out, const CVector* offset);
GAME_FN(uint8_t, __thiscall, PT_IsInside, 0x00499680, TetherTarget* self, CVector* pos, float margin);
GAME_FN(uint8_t, __thiscall, PT_ClampInside, 0x00499710, TetherTarget* self, CVector* pos, float margin);
GAME_FN(void, __thiscall, PT_Update, 0x00499990, TetherTarget* self);

inline void* const scriptTriggers = reinterpret_cast<void*>(0x020C7E98);

// (inlined) an entity's position: its matrix's, or its placement's
CVector* EntityPosition(void* entity) {
    uint8_t* e = static_cast<uint8_t*>(entity);
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(e + 0x14);
    return reinterpret_cast<CVector*>(matrix ? matrix + 0x30 : e + 4);
}
} // namespace

namespace tether {

// 0x4995A0 (thiscall)
void* TetherTarget::GetEntity() {
    if (type != TETHER_ENTITY) {
        return nullptr;
    }
    return entity;
}

// 0x4995B0 (thiscall): where it is (nothing set: the owner's position)
CVector* TetherTarget::GetPosition(CVector* out) {
    if (type != TETHER_NONE) {
        *out = pos;
        return out;
    }
    *out = *EntityPosition(owner);
    return out;
}

// 0x499600 (thiscall): the entity if it's a ped
void* TetherTarget::GetPed() {
    if (type == TETHER_ENTITY && entity) {
        return (*(static_cast<uint8_t*>(entity) + 0x108) & 7) == 3 ? entity : nullptr;
    }
    return nullptr;
}

// 0x499680 (thiscall): the position is within the tether (nothing set, or a trigger
// that's gone: yes)
uint8_t TetherTarget::IsInside(CVector* p, float margin) {
    if (type > 0) {
        if (type <= 2) {
            return PT_DistanceOutside(p, &pos, radius + margin) <= 0.0f ? 1 : 0;
        }
        if (type == TETHER_TRIGGER && trigger != -1) {
            void* t = PT_GetTrigger(scriptTriggers, trigger);
            if (t && !PT_TriggerContains(t, p, margin)) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x499710 (thiscall): the position moved back inside when it's out (0: couldn't)
uint8_t TetherTarget::ClampInside(CVector* p, float margin) {
    if (PT_IsInside(this, p, margin)) {
        return 1;
    }
    if (type > 0) {
        if (type <= 2) {
            CVector tmp;
            CVector dir = *PT_VectorSub(&tmp, p, &pos);
            PT_Normalise(&dir);
            float distance = radius + margin;
            if (distance < 0.0f) {
                distance = 0.0f;
            }
            CVector scaled = {dir.x * distance, dir.y * distance, distance * dir.z};
            *p = *PT_VectorAdd(&tmp, &pos, &scaled);
            return 1;
        }
        if (type == TETHER_TRIGGER && trigger != -1) {
            void* t = PT_GetTrigger(scriptTriggers, trigger);
            if (!t) {
                return 1;
            }
            if (PT_TriggerClamp(t, p, margin)) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x499990 (thiscall): its position again (an entity's gone: none; a trigger: the
// owner's position, moved inside)
void TetherTarget::Update() {
    if (type == TETHER_ENTITY) {
        if (entity) {
            CVector tmp;
            pos = *PT_EntityTransformOffset(entity, &tmp, &offset);
            return;
        }
        type = TETHER_NONE;
    } else if (type == TETHER_TRIGGER) {
        pos = *EntityPosition(owner);
        PT_ClampInside(this, &pos, -0.3f);
    }
}

// 0x499840 (thiscall): not in the anti-tether, and in the tether
uint8_t PedTether::IsPosAllowed(CVector* pos, float margin) {
    if (anti.type != TETHER_NONE && PT_IsInside(&anti, pos, margin)) {
        return 0;
    }
    return PT_IsInside(&tether, pos, margin);
}

// 0x499880 (thiscall): the same for an entity's position (nullptr: no)
uint8_t PedTether::IsEntityAllowed(void* entity, float margin) {
    if (anti.type != TETHER_NONE) {
        if (!entity || PT_IsInside(&anti, EntityPosition(entity), margin)) {
            return 0;
        }
    }
    if (!entity) {
        return 0;
    }
    return PT_IsInside(&tether, EntityPosition(entity), margin);
}

// 0x4998F0 (thiscall): the same for the owner (each tether's own owner field)
uint8_t PedTether::IsOwnerAllowed(float margin) {
    if (anti.type != TETHER_NONE && anti.owner && PT_IsInside(&anti, EntityPosition(anti.owner), margin)) {
        return 0;
    }
    if (!tether.owner) {
        return 0;
    }
    return PT_IsInside(&tether, EntityPosition(tether.owner), margin);
}

// 0x499970 (thiscall)
uint8_t PedTether::Clamp(CVector* pos, float margin) {
    return PT_ClampInside(&tether, pos, margin);
}

// 0x499A20 (thiscall)
void PedTether::Update() {
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499A40 (thiscall)
void PedTether::Clear() {
    tether.type = TETHER_NONE;
    unknown5C = 1;
    unknown5D = 0;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499A60 (thiscall)
void PedTether::SetToPosition(const CVector* pos, float radius) {
    tether.type = TETHER_POSITION;
    tether.pos = *pos;
    tether.radius = radius;
    unknown5C = 1;
    unknown5D = 0;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499AB0 (thiscall)
void PedTether::SetToEntity(void* entity, float radius, const CVector* offset) {
    tether.type = TETHER_ENTITY;
    PT_RefSet(&tether.entity, entity);
    tether.pos = *EntityPosition(tether.entity);
    tether.radius = radius;
    tether.offset = *offset;
    unknown5C = 1;
    unknown5D = 0;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499B30 (thiscall)
void PedTether::SetToTrigger(int trigger) {
    tether.radius = 0.0f;
    tether.type = TETHER_TRIGGER;
    tether.trigger = trigger;
    unknown5C = 1;
    unknown5D = 0;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499B70 (thiscall)
void PedTether::ClearAnti() {
    anti.type = TETHER_NONE;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499B90 (thiscall)
void PedTether::SetAntiToPosition(const CVector* pos, float radius) {
    anti.type = TETHER_POSITION;
    anti.pos = *pos;
    anti.radius = radius;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499BD0 (thiscall)
void PedTether::SetAntiToEntity(void* entity, float radius, const CVector* offset) {
    anti.type = TETHER_ENTITY;
    PT_RefSet(&anti.entity, entity);
    anti.pos = *EntityPosition(anti.entity);
    anti.radius = radius;
    anti.offset = *offset;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499C40 (thiscall)
void PedTether::SetAntiToTrigger(int trigger) {
    anti.radius = 0.0f;
    anti.type = TETHER_TRIGGER;
    anti.trigger = trigger;
    PT_Update(&tether);
    PT_Update(&anti);
}

// 0x499C70 (thiscall): both cleared, for the owner already in +0
void PedTether::Init() {
    tether.owner = owner;
    anti.owner = owner;
    tether.type = TETHER_NONE;
    unknown5C = 1;
    unknown5D = 0;
    PT_Update(&tether);
    PT_Update(&anti);
    anti.type = TETHER_NONE;
    PT_Update(&tether);
    PT_Update(&anti);
    speed = 1.5f;
}

} // namespace tether

namespace {
// 0x499630 (thiscall on the entity): a point in its space to the world (a copy of
// CEntity::TransformFromObjectSpace, 0x466200)
CVector* __fastcall EntityTransformOffset(uint8_t* entity, re::Edx, CVector* out, const CVector* offset) {
    // (0x41A9A0 writes 16 bytes: a CVector and the pad after it)
    struct { CVector v; float pad; } tmp;
    void* matrix = *reinterpret_cast<re::Ptr32<void>*>(entity + 0x14);
    if (!matrix) {
        PT_TransformPoint(&tmp.v, entity + 4, offset);
    } else {
        PT_MultiplyPoint(&tmp.v, matrix, offset);
    }
    *out = tmp.v;
    return out;
}
} // namespace

#define C08P4_REVERSED(name, address, fn) \
    REVERSED(name, address, re::MemberAddress(fn), nullptr, nullptr, re::Mode::Original)
#define C08P4_REVERSED_STATIC(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

C08P4_REVERSED("TetherTarget::GetEntity", 0x004995A0, &TetherTarget::GetEntity);
C08P4_REVERSED("TetherTarget::GetPosition", 0x004995B0, &TetherTarget::GetPosition);
C08P4_REVERSED("TetherTarget::GetPed", 0x00499600, &TetherTarget::GetPed);
C08P4_REVERSED_STATIC("CEntity::TransformOffset", 0x00499630, &EntityTransformOffset);
C08P4_REVERSED("TetherTarget::IsInside", 0x00499680, &TetherTarget::IsInside);
C08P4_REVERSED("TetherTarget::ClampInside", 0x00499710, &TetherTarget::ClampInside);
C08P4_REVERSED("PedTether::IsPosAllowed", 0x00499840, &PedTether::IsPosAllowed);
C08P4_REVERSED("PedTether::IsEntityAllowed", 0x00499880, &PedTether::IsEntityAllowed);
C08P4_REVERSED("PedTether::IsOwnerAllowed", 0x004998F0, &PedTether::IsOwnerAllowed);
C08P4_REVERSED("PedTether::Clamp", 0x00499970, &PedTether::Clamp);
C08P4_REVERSED("TetherTarget::Update", 0x00499990, &TetherTarget::Update);
C08P4_REVERSED("PedTether::Update", 0x00499A20, &PedTether::Update);
C08P4_REVERSED("PedTether::Clear", 0x00499A40, &PedTether::Clear);
C08P4_REVERSED("PedTether::SetToPosition", 0x00499A60, &PedTether::SetToPosition);
C08P4_REVERSED("PedTether::SetToEntity", 0x00499AB0, &PedTether::SetToEntity);
C08P4_REVERSED("PedTether::SetToTrigger", 0x00499B30, &PedTether::SetToTrigger);
C08P4_REVERSED("PedTether::ClearAnti", 0x00499B70, &PedTether::ClearAnti);
C08P4_REVERSED("PedTether::SetAntiToPosition", 0x00499B90, &PedTether::SetAntiToPosition);
C08P4_REVERSED("PedTether::SetAntiToEntity", 0x00499BD0, &PedTether::SetAntiToEntity);
C08P4_REVERSED("PedTether::SetAntiToTrigger", 0x00499C40, &PedTether::SetAntiToTrigger);
C08P4_REVERSED("PedTether::Init", 0x00499C70, &PedTether::Init);
