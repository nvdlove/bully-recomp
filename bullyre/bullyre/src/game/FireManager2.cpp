// CFireManager, first part: its constructor, a fire's reset, its setters by index,
// its effect kept on the bone it burns on, whether a line passes through it, its
// strength, and starting one ("BarrelFire" unless named). 0x42D6D0..0x42DCE6.
// (The rest, and the fire's layout as understood there: FireManager.cpp.)

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

#define FIREMGR2_REVERSED(name, address, fn)                                                 \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

namespace {

struct Fm2Vector {
    float x, y, z;
};
static_assert(sizeof(Fm2Vector) == 0xC);

// A fire (see FireManager.cpp)
struct CFire2 {
    int32_t targetHandle;   // +0x0 what it was started on (a handle: 0x4EA3C0)
    float damageInterval;   // +0x4
    float damageArg;        // +0x8
    float damageScale;      // +0xC
    float strength;         // +0x10 > 0: burning
    float maxStrength;      // +0x14
    float lastDamageTime;   // +0x18
    float effectScale;      // +0x1C (the effect's size at full strength)
    uint8_t flags;          // +0x20 1: new, 2: on a target, 8: named effect
    uint8_t unknown21[3];
    Fm2Vector pos;          // +0x24
    re::Ptr32<void> owner;            // +0x30 (a registered reference)
    re::Ptr32<void> target;           // +0x34 (a registered reference)
    uint8_t effect[4];      // +0x38 its effect's attachment (0x669040)
    re::Ptr32<void> effectObject;     // +0x3C
    uint8_t sound[4];       // +0x40 its sound (0x59D110)
};
static_assert(offsetof(CFire2, strength) == 0x10);
static_assert(offsetof(CFire2, effectScale) == 0x1C);
static_assert(offsetof(CFire2, flags) == 0x20);
static_assert(offsetof(CFire2, pos) == 0x24);
static_assert(offsetof(CFire2, owner) == 0x30);
static_assert(offsetof(CFire2, target) == 0x34);
static_assert(offsetof(CFire2, effectObject) == 0x3C);
static_assert(offsetof(CFire2, sound) == 0x40);
static_assert(sizeof(CFire2) == 0x44);

constexpr int FM2_NUM_FIRES = 10;

// A Gamebryo transform (a 3x3 rotation, the translation, the scale)
struct Fm2Transform {
    float rotate[9];      // +0
    Fm2Vector translate;  // +0x24
    float scale;          // +0x30
};
static_assert(sizeof(Fm2Transform) == 0x34);

GAME_VAR(int32_t, fm2_fireBoneName, 0x01B65304);
inline void* const fm2_animManager = reinterpret_cast<void*>(0x020C4910);
inline const char* const fm2_barrelFire = reinterpret_cast<const char*>(0x00901BC0);  // "BarrelFire"
GAME_VAR(void*, fm2_effectNames, 0x00B6F1E8);
GAME_VAR(void*, fm2_effects, 0x00B6F1D4);

GAME_FN(void, __thiscall, FM2_Sound_Set, 0x0059D110, void* sound, uintptr_t id);
GAME_FN(void, __cdecl, FM2_CleanUpOldReference, 0x004657E0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __cdecl, FM2_RegisterReference, 0x004657D0, void* entity, re::Ptr32<void>* ref);
GAME_FN(void, __thiscall, FM2_Effect_Stop, 0x00668BA0, void* effect);
GAME_FN(void, __thiscall, FM2_Effect_Construct, 0x00668E90, void* effect);
GAME_FN(void, __thiscall, FM2_Effect_SetPosition, 0x00669040, void* effect, const Fm2Vector* pos, int arg);
GAME_FN(void, __thiscall, FM2_Effect_SetDirection, 0x00669360, void* effect, const Fm2Vector* dir);
GAME_FN(void, __thiscall, FM2_Effect_Fn668C10_x64, 0x00668C10, void* effect, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void FM2_Effect_Fn668C10(void* a0, float a1) {
    return FM2_Effect_Fn668C10_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, FM2_Effect_Fn668C70_x64, 0x00668C70, void* effect, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void FM2_Effect_Fn668C70(void* a0, float a1) {
    return FM2_Effect_Fn668C70_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, FM2_Effect_Fn668C40_x64, 0x00668C40, void* effect, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void FM2_Effect_Fn668C40(void* a0, float a1) {
    return FM2_Effect_Fn668C40_x64(a0, re::BitsOf(a1));
}
GAME_FN(int32_t, __cdecl, FM2_FindBone, 0x00655620, void* entity, int32_t name);
GAME_FN(void, __thiscall, FM2_Anim_GetBonePosition, 0x006C1190, void* manager, void* animObject, int32_t bone, Fm2Vector* out, uintptr_t arg);
GAME_FN(void, __thiscall, FM2_Anim_GetBoneTransform, 0x006BF570, void* manager, void* animObject, int32_t bone, Fm2Transform* out, uintptr_t arg);
GAME_FN(Fm2Vector*, __cdecl, FM2_Sub, 0x00414B70, Fm2Vector* out, const Fm2Vector* a, const Fm2Vector* b);
GAME_FN(Fm2Vector*, __cdecl, FM2_Add, 0x004129A0, Fm2Vector* out, const Fm2Vector* a, const Fm2Vector* b);
GAME_FN(uint16_t, __thiscall, FM2_EffectNames_Find, 0x00667B60, void* names, const char* name);
GAME_FN(void, __thiscall, FM2_Effects_Start, 0x00668290, void* effects, uint32_t* id, void* attachment, int arg);
GAME_FN(void*, __cdecl, FM2_EntityFromHandle, 0x004EA3C0, uintptr_t handle);
GAME_FN(void, __thiscall, FM2_Fire_Reset, 0x0042D6F0, CFire2* fire);
GAME_FN(void, __thiscall, FM2_Fire_UpdateOnBone, 0x0042D8A0, CFire2* fire);

Fm2Vector& EntityPos(void* entity) {
    uint8_t* e = static_cast<uint8_t*>(entity);
    uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(e + 0x14);
    return *reinterpret_cast<Fm2Vector*>(matrix ? matrix + 0x30 : e + 4);
}

} // namespace

namespace CFireManager {

// 0x42D6D0 (thiscall): a fire sound's constructor (its 0x59D110 with the id)
void* __fastcall FireSound_Construct(void* self, re::Edx, int id) {
    FM2_Sound_Set(self, id);
    return self;
}

// 0x42D6F0 (thiscall): the fire out at once: no strength, no owner or target, its
// effect stopped, its sound off
void __fastcall Fire_Reset(CFire2* self, re::Edx) {
    self->strength = 0.0f;
    if (self->owner) {
        FM2_CleanUpOldReference(self->owner, &self->owner);
    }
    if (self->target) {
        FM2_CleanUpOldReference(self->target, &self->target);
    }
    self->target = nullptr;
    self->owner = nullptr;
    if (self->effectObject) {
        FM2_Effect_Stop(self->effect);
    }
    FM2_Sound_Set(self->sound, 0);
}

// 0x42D750 (thiscall): the manager's constructor: its 10 fires out
CFire2* __fastcall Construct(CFire2* self, re::Edx) {
    for (int i = 0; i < FM2_NUM_FIRES; i++) {
        CFire2* fire = &self[i];
        fire->pos = {0.0f, 0.0f, 0.0f};
        FM2_Effect_Construct(fire->effect);
        *reinterpret_cast<uint32_t*>(fire->sound) = 0;
        fire->damageInterval = 1.0f;
        fire->flags |= 1;
        fire->damageArg = 10.0f;
        fire->targetHandle = 0;
        fire->strength = 0.0f;
        fire->maxStrength = 100.0f;
        fire->pos = {0.0f, 0.0f, 0.0f};
        fire->flags &= 9;
        fire->lastDamageTime = 0.0f;
        fire->owner = nullptr;
        fire->target = nullptr;
        fire->effectScale = 1.0f;
        fire->damageScale = 2.0f;
        FM2_Sound_Set(fire->sound, 0);
    }
    return self;
}

// 0x42D7E0 (thiscall)
void __fastcall SetDamageScale(CFire2* self, re::Edx, int index, float value) {
    self[index].damageScale = value;
}

// 0x42D800 (thiscall)
void __fastcall SetEffectScale(CFire2* self, re::Edx, int index, float value) {
    self[index].effectScale = value;
}

// 0x42D820 (thiscall)
void __fastcall SetDamage(CFire2* self, re::Edx, int index, float interval, float arg) {
    CFire2* fire = &self[index];
    fire->damageInterval = interval;
    fire->damageArg = arg;
}

// 0x42D840 (thiscall)
float __fastcall GetStrength(CFire2* self, re::Edx, int index) {
    return self[index].strength;
}

// 0x42D860 (thiscall)
void __fastcall SetStrengthAndMax(CFire2* self, re::Edx, int index, float strength, float maxStrength) {
    CFire2* fire = &self[index];
    fire->strength = strength;
    fire->maxStrength = maxStrength;
}

// 0x42D880 (thiscall): a fire out at once
void __fastcall ResetFire(CFire2* self, re::Edx, int index) {
    FM2_Fire_Reset(&self[index]);
}

// 0x42D8A0 (thiscall): a fire on a target: its effect on the target's fire bone (its
// position, its direction the bone's y), or on the target when it has no such bone
void __fastcall Fire_UpdateOnBone(CFire2* self, re::Edx) {
    if (!self->target) {
        return;
    }
    int32_t bone = FM2_FindBone(self->target, fm2_fireBoneName);
    if (bone == -1) {
        if (self->effectObject) {
            FM2_Effect_SetPosition(self->effect, &EntityPos(self->target), 0);
        }
        return;
    }
    Fm2Transform transform;
    transform.scale = 1.0f;
    void* animObject = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(self->target) + 0x114);
    FM2_Anim_GetBonePosition(fm2_animManager, animObject, bone, &self->pos, 0);
    animObject = *reinterpret_cast<re::Ptr32<void>*>(static_cast<uint8_t*>(self->target) + 0x114);
    FM2_Anim_GetBoneTransform(fm2_animManager, animObject, bone, &transform, 0);
    Fm2Vector dir = {transform.rotate[1], transform.rotate[4], transform.rotate[7]};
    if (self->effectObject) {
        FM2_Effect_SetPosition(self->effect, &self->pos, 0);
        FM2_Effect_SetDirection(self->effect, &dir);
    }
}

// 0x42D980 (thiscall): the line (start, a unit direction, a length) passes within 1.3
// of the fire (0.4 above its position), ahead of the start and not past its end + 1.3
uint8_t __fastcall Fire_IsHitBy(CFire2* self, re::Edx, Fm2Vector start, Fm2Vector dir, float length) {
    Fm2Vector centre = {self->pos.x, self->pos.y, float(self->pos.z + 0.4000000059604645)};
    Fm2Vector d;
    FM2_Sub(&d, &centre, &start);
    float along = float(double(dir.y) * d.y + double(d.x) * dir.x + double(dir.z) * d.z);
    if (along < 0.0f) {
        return 0;
    }
    if (length + 1.2999999523162842 < along) {
        return 0;
    }
    Fm2Vector scaled = {dir.x * along, dir.y * along, along * dir.z};
    Fm2Vector nearest;
    FM2_Add(&nearest, &start, &scaled);
    Fm2Vector off;
    Fm2Vector* diff = FM2_Sub(&off, &nearest, &centre);
    float sq = diff->x * diff->x + diff->y * diff->y + diff->z * diff->z;
    // (the game's square root takes the fabs first)
    float distance = std::sqrt(std::fabs(sq));
    return distance > 1.3f ? 0 : 1;
}

// 0x42DAC0 (thiscall): its strength (0..max; out at 0), the effect sized by it
void __fastcall Fire_SetStrength(CFire2* self, re::Edx, float strength) {
    if (self->maxStrength < strength) {
        strength = self->maxStrength;
    }
    if (strength < 0.0f) {
        strength = 0.0f;
    }
    self->strength = strength;
    if (!(strength > 0.0f)) {
        FM2_Fire_Reset(self);
        return;
    }
    if (!self->effectObject) {
        return;
    }
    float size = float(strength / double(self->maxStrength) * self->effectScale);
    FM2_Effect_Fn668C10(self->effect, size);
    FM2_Effect_Fn668C70(self->effect, size);
    FM2_Effect_Fn668C40(self->effect, 1.0f);
}

// 0x42DB60 (thiscall): a fire started in the first free slot (none free: the game
// writes through a null fire): at full strength (100), its effect by name
// ("BarrelFire" if none), on the target the handle names if there is one (at its
// position). Its index
int __fastcall StartFire(CFire2* self, re::Edx, float x, float y, float z, int32_t targetHandle, void* owner, const char* effectName) {
    CFire2* fire = nullptr;
    for (int i = 0; i < FM2_NUM_FIRES; i++) {
        if (!(0.0f < self[i].strength)) {
            fire = &self[i];
            break;
        }
    }
    fire->damageInterval = 1.0f;
    fire->damageArg = 10.0f;
    fire->pos.x = x;
    fire->strength = 100.0f;
    fire->maxStrength = 100.0f;
    fire->targetHandle = targetHandle;
    fire->pos.y = y;
    fire->lastDamageTime = 0.0f;
    fire->pos.z = z;
    fire->effectScale = 1.0f;
    fire->target = nullptr;
    fire->owner = owner;
    fire->flags = uint8_t((fire->flags & 8) | 1);
    if (owner) {
        FM2_RegisterReference(owner, &fire->owner);
    }
    uint16_t id = FM2_EffectNames_Find(fm2_effectNames, effectName ? effectName : fm2_barrelFire);
    fire->flags ^= (uint8_t((effectName != nullptr) << 3) ^ fire->flags) & 8;
    uint32_t effectId = id;
    FM2_Effects_Start(fm2_effects, &effectId, fire->effect, 0);
    if (targetHandle) {
        void* target = FM2_EntityFromHandle(targetHandle);
        if (target) {
            fire->flags |= 2;
            fire->target = target;
            fire->pos = EntityPos(target);
            FM2_RegisterReference(target, &fire->target);
        }
    }
    FM2_Effect_SetPosition(fire->effect, &fire->pos, 0);
    Fm2Vector up = {0.0f, 1.0f, 0.0f};
    FM2_Effect_SetDirection(fire->effect, &up);
    FM2_Fire_UpdateOnBone(fire);
    return int((reinterpret_cast<intptr_t>(fire) - reinterpret_cast<intptr_t>(self)) / int(sizeof(CFire2)));
}

FIREMGR2_REVERSED("CFireSound::Construct", 0x0042D6D0, &FireSound_Construct);
FIREMGR2_REVERSED("CFire::Reset", 0x0042D6F0, &Fire_Reset);
FIREMGR2_REVERSED("CFireManager::Construct", 0x0042D750, &Construct);
FIREMGR2_REVERSED("CFireManager::SetDamageScale", 0x0042D7E0, &SetDamageScale);
FIREMGR2_REVERSED("CFireManager::SetEffectScale", 0x0042D800, &SetEffectScale);
FIREMGR2_REVERSED("CFireManager::SetDamage", 0x0042D820, &SetDamage);
FIREMGR2_REVERSED("CFireManager::GetStrength", 0x0042D840, &GetStrength);
FIREMGR2_REVERSED("CFireManager::SetStrengthAndMax", 0x0042D860, &SetStrengthAndMax);
FIREMGR2_REVERSED("CFireManager::ResetFire", 0x0042D880, &ResetFire);
FIREMGR2_REVERSED("CFire::UpdateOnBone", 0x0042D8A0, &Fire_UpdateOnBone);
FIREMGR2_REVERSED("CFire::IsHitBy", 0x0042D980, &Fire_IsHitBy);
FIREMGR2_REVERSED("CFire::SetStrength", 0x0042DAC0, &Fire_SetStrength);
FIREMGR2_REVERSED("CFireManager::StartFire", 0x0042DB60, &StartFire);

} // namespace CFireManager
