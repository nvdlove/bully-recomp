// CObject's processing and upkeep: ProcessControl / ProcessCollision for thrown
// objects, damage and breaking, the ambient sounds from the object data, the
// constructors, and the functions over the whole object pool (GTA's
// DeleteAllTempObjects and friends).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "game/Object.h"
#include "game/Pools.h"
#include "reversed.h"
#include "runflags.h"

#include "game/Crt.h"
namespace c11object {

using veh::CVector;

// 0x38 bytes an entry (CObjectData, 0xC35200); what the object code reads
//   +0x0 uint16 (low 15 bits: the id kept in +0x220), +0x2 bit 0 (only some ped hurts it),
//   +0x4 uint16[2] effects on breaking (+0x8 int16[2] their heights, 1/256),
//   +0xC int16 the effect when hit, +0xE int16 the sound bank, +0x10 int32 the bank's range,
//   +0x14 the ambient sound, +0x18 the sound on breaking, +0x1C the sound when hit,
//   +0x20 low nibble (signed) the ambient sound's kind, high nibble the others',
//   +0x21 bit 0 an ambience emitter (the rest: its volume, percent), +0x22 bit 0,
//   +0x2C something made on breaking (+0x30 int16 its height, 1/256), +0x34 an extra model

// vf12 0x4CF4A0: thrown objects (kind 7) slowed by their projectile's drag; then
// CPhysical's (not at all while +0xB0 is set)
void __fastcall ProcessControl(CObject* self, re::Edx edx);
// 0x4CF620: its ambient sound and its sound bank by the player's distance
void __fastcall UpdateAmbientSound(CObject* self, re::Edx edx);
// vf13 0x4CF9C0: thrown objects moved by their speed (CPhysical's for the rest)
void __fastcall ProcessCollision(CObject* self, re::Edx edx, int arg);
// 0x4CFAC0: the prop anim of the model within 2 m plays the named action
void __fastcall PlayNearbyPropAnim(CObject* self, re::Edx edx, int model, const char* action);
// 0x4CFDF0 / 0x4CFE70: every temporary (type 3) / mission (type 2) object deleted
void __cdecl DeleteAllTempObjects();
void __cdecl DeleteAllMissionObjects();
// 0x4CFEF0: the temporary ones within the radius
void __cdecl DeleteAllTempObjectsInArea(float x, float y, float z, float radius);
// 0x4D0040: its object data's extra model requested (when the player is within 5 m)
void __fastcall RequestExtraModel(CObject* self, re::Edx edx);
// 0x4D0110: every object's and projectile's sound and extra model
void __cdecl UpdateAllAmbientSounds();
// 0x4D01F0: every object's and projectile's sound bank let go; the prop anims' 0x4EA310
void __cdecl StopAllAmbientSounds();
// 0x4D03A0: the pointer is within the object or the projectile pool (one slot past
// the end passes)
uint8_t __cdecl IsInObjectPools(CObject* object);
// 0x4D0450: broken: its sound emitter off, the prop anim's "NotUseable" action for some
// models, static, collision off, out of the visibility list
void __fastcall Break(CObject* self, re::Edx edx, int keepEffects);
// 0x4D0580: the constructor (with its Gamebryo object made when `create`)
CObject* __fastcall Construct(CObject* self, re::Edx edx, int model, int create);
// 0x4D05E0: the constructor from a dummy (its object and matrix taken over)
CObject* __fastcall ConstructFromDummy(CObject* self, re::Edx edx, CEntity* dummy);
// vf15 0x4D06C0: damage (100 ms apart): its effect and sound; broken at 0 health
uint8_t __fastcall InflictDamage(CObject* self, re::Edx edx, float damage, int arg2, CEntity* attacker, int weaponType,
                                 const CVector* hitPos, int, int, int, int, int, int, int, int, int, int, int, int, int);
// vf14 0x4D0B30: moved (out of the world and back)
void __fastcall Teleport(CObject* self, re::Edx edx, float x, float y, float z);
// 0x4D0B80: a live object (in the pools, and in the world)
uint8_t __cdecl IsObjectPointerValid(CObject* object);
// 0x4D0BB0: a sound size by name (1 small .. 9 supersize; 0 "generic", else 1)
uint8_t __cdecl GetSoundSizeFromName(void* name);
// 0x4D0CD0: the model has object data
uint8_t __cdecl HasObjectInfo(int model);
// 0x4D0D10: every model's object data index (+0x12) from the table and the overrides
void __cdecl SetModelObjectInfoIndices();

} // namespace c11object

namespace {

using namespace c11object;

GAME_FN(void, __thiscall, Object_UpdateAmbientSound, 0x004CF620, CObject* o);
GAME_FN(void, __thiscall, Object_PlayNearbyPropAnim, 0x004CFAC0, CObject* o, int model, const char* action);
GAME_FN(void, __thiscall, Object_RequestExtraModel, 0x004D0040, CObject* o);
GAME_FN(uint8_t, __cdecl, Object_IsInObjectPools, 0x004D03A0, CObject* o);
GAME_FN(void, __thiscall, Object_Break, 0x004D0450, CObject* o, int keepEffects);
GAME_FN(uint8_t, __cdecl, Object_HasObjectInfo, 0x004D0CD0, int model);
GAME_FN(void, __thiscall, Object_Init, 0x004CFCE0, CObject* o);

GAME_FN(uint8_t*, __cdecl, GetObjectInfo, 0x004D0CA0, int model);
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(uint8_t, __cdecl, Dodgeball_IsActive, 0x00683A30);
GAME_FN(void, __thiscall, CopyInt, 0x005FA880, uint32_t* dst, const void* src);
GAME_FN(void, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, PathKey_Make, 0x005FA8C0, void* self, int count, const uint32_t* names);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, void* path, void* names);
GAME_FN(void, __stdcall, DestructArray, 0x004017C0, void* array, size_t size, int count, void* destructor);
GAME_FN(void, __stdcall, ConstructArray, 0x00401240, void* array, size_t size, int count, void* constructor);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_FN(void, __thiscall, Physical_ProcessControl, 0x00469B70, CObject* o);
GAME_FN(void, __thiscall, Physical_ProcessCollision, 0x0046CB90, CObject* o, int arg);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Ambience_PlayEmitter, 0x005870D0, void* ambience, int sound, float distSq, const CVector* pos,
        float volume, int kind, int, int, int, int);
GAME_FN(void, __thiscall, Ambience_RemoveEmitter, 0x005875F0, void* owner, CObject* object, int);
GAME_FN(uint8_t, __cdecl, IsPlayerInFastVehicle, 0x004469A0);
GAME_FN(void, __thiscall, SoundBanks_Load, 0x0059A3E0, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(uint8_t, __thiscall, SoundBanks_IsLoaded, 0x005995F0, void* banks, int bank);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int, uintptr_t, int, int, int,
        int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(void, __thiscall, Sound_Construct, 0x0042D6D0, uint32_t* sound, void* handle);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, uint32_t* sound, uintptr_t id);
GAME_FN(void*, __thiscall, Placeable_GetMatrix, 0x0046DD20, void* placeable);
GAME_FN(void, __thiscall, Matrix_CopyConstruct, 0x00412A10, void* self, const void* other);
GAME_FN(uint8_t, __cdecl, Fn_683B40, 0x00683B40, CObject* o);
GAME_FN(void, __thiscall, Matrix_Reorthogonalise, 0x00413A80, void* m);
GAME_FN(void, __thiscall, Entity_443AA0, 0x00443AA0, void* entity);
GAME_FN(void, __thiscall, Physical_RemoveAndAdd, 0x00469470, void* entity);
GAME_FN(CEntity*, __cdecl, FindNearestObjectOfTypeInRange, 0x004607A0, int model, const CVector* pos, float range, int lists);
GAME_FN(void*, __cdecl, CatStore_GetUnknown, 0x0052D810, int cat);
GAME_FN(void*, __cdecl, ActTree_FindNode, 0x005F5B10, const char* name, void* node);
GAME_FN(void, __thiscall, PropAnim_4EE010, 0x004EE010, CEntity* prop, void* node);
GAME_FN(void, __cdecl, World_Remove, 0x0045DD70, void* entity);
GAME_FN(void, __cdecl, World_Add, 0x0045D6B0, void* entity);
GAME_FN(int, __thiscall, Pool_GetSizeChained, 0x00435BB0, void* pool);
GAME_FN(void*, __thiscall, Pool_GetAtChained, 0x00435BD0, void* pool, int slot, int onlyUsed);
GAME_FN(int, __thiscall, Pool_GetIndexChained, 0x004CF3E0, void* pool, void* object);
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(float, __cdecl, DistanceSquared, 0x00472D00, const CVector* a, const CVector* b);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __thiscall, WeaponInfo_7282A0, 0x007282A0, uint8_t* info);
GAME_FN(uint8_t, __cdecl, IsModelRequested, 0x004151C0, int id);
GAME_FN(void, __thiscall, PropAnim_RequestExtraModel, 0x004EA310, void* prop);
GAME_FN(void, __thiscall, Physical_RemoveFromMovingList, 0x00469840, void* entity);
GAME_FN(void, __thiscall, VisibilityList_Remove, 0x005E6990, void* list, CObject* entity);
GAME_FN(void, __cdecl, Fn_5B3690, 0x005B3690, CObject* o);
GAME_FN(void, __thiscall, Physical_Construct, 0x00468E90, CObject* o);
GAME_FN(void, __thiscall, Entity_SetModelIndex, 0x00468660, CObject* o, int model, int arg);
GAME_FN(void, __thiscall, Entity_SetModelIndexNoCreate, 0x00465E20, CObject* o, int model);
GAME_FN(void, __thiscall, Entity_AttachToRwObject, 0x004666F0, CObject* o, void* rwObject);
GAME_FN(void, __thiscall, Entity_DetachFromRwObject, 0x00466740, void* entity);
GAME_FN(void, __thiscall, Placeable_SetMatrix, 0x0046DE90, CObject* o, const void* m, int forceMatrix);
GAME_FN(void, __thiscall, Matrix_CopyOnlyMatrix, 0x00412720, void* dst, const void* src);
GAME_FN(uint8_t, __thiscall, Physical_41C540, 0x0041C540, CObject* o);
GAME_FN(void, __cdecl, Fn_5201E0, 0x005201E0, uintptr_t thing, const CVector* pos);
GAME_FN(int16_t, __thiscall, Collectibles_FindByHash, 0x004263C0, void* mgr, uint32_t hash);
GAME_FN(uint8_t, __thiscall, Collectibles_IsType0, 0x00426360, void* mgr, int index);
GAME_FN(uint8_t, __thiscall, Collectibles_IsType4, 0x00426380, void* mgr, int index);
GAME_FN(uint8_t, __thiscall, Collectibles_IsType5, 0x004263A0, void* mgr, int index);
GAME_FN(void, __thiscall, Collectibles_SetCollected, 0x00428A10, void* mgr, int index, int collected);
GAME_FN(void, __thiscall, Collectibles_ShowCollectedMessage, 0x00428B00, void* mgr, int index);
GAME_FN(void, __thiscall, AE_Manager_Start, 0x00668290, void* manager, const void* effectType, void* proxy, int);
GAME_FN(void, __thiscall, AE_Proxy_SetPosition, 0x00669040, void* proxy, const CVector* pos, int);
GAME_FN(void, __thiscall, AE_Proxy_Construct, 0x00668E90, void* proxy);
GAME_FN(void, __thiscall, AE_Proxy_Destruct, 0x00668EB0, void* proxy);
GAME_FN(CVector*, __thiscall, Entity_GetPosition, 0x00415460, void* entity);
GAME_FN(void, __thiscall, Stimulus_Add, 0x004A6670, void* manager, int kind, CEntity* from, void* to, const CVector* pos,
        float a, float b);
GAME_FN(uint8_t, __cdecl, IsModelInList25, 0x0043F420, int model);
GAME_FN(uint8_t, __cdecl, IsModelInList30, 0x0043F520, int model);
GAME_FN(void, __cdecl, IncrementStat, 0x00458480, int stat, int by);
GAME_FN(void, __thiscall, Entity_SetPosition, 0x0041C4E0, void* entity, float x, float y, float z);
GAME_FN(void, __thiscall, Entity_UpdateRwFrame, 0x00465760, void* entity);
GAME_FN(void, __thiscall, FixedString_ToChars, 0x0061A3B0, void* self, uintptr_t value, char* buffer, int size, int,
        int);
inline int __cdecl Stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }

GAME_VAR(CEntity*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t*, defaultDragInfo, 0x020C888C);
GAME_VAR(uint32_t, boneNameKey, 0x01B65374);
inline void* const boneNames = reinterpret_cast<void*>(0x01B65378);
inline void* const emptyFunction = reinterpret_cast<void*>(0x0049A650);
inline void* const proxyConstruct = reinterpret_cast<void*>(0x00668E90);
inline void* const proxyDestruct = reinterpret_cast<void*>(0x00668EB0);
GAME_VAR(CVector, vecUnset, 0x00BCC564);
GAME_VAR(CVector, cameraForward, 0x00C1AF7C);
inline void* const ambience = reinterpret_cast<void*>(0x00AE5F60);
GAME_VAR(void*, ambienceEmitters, 0x00CF4278);
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const sounds = reinterpret_cast<void*>(0x00CF3DA0);
inline const uint8_t* const soundKinds = reinterpret_cast<const uint8_t*>(0x00AE6CDE);   // 0xB8 each: +0 uint16 range
GAME_VAR(CPool<CObject>*, objectPool, 0x00C0F614);
GAME_VAR(CPool<CObject>*, projectilePool, 0x00C0F618);
GAME_VAR(CPool<uint8_t>*, propAnimPool, 0x00C0F608);
GAME_VAR(uint32_t, soundUpdateCounter, 0x00C34E4C);
GAME_ARRAY(uint8_t*, modelInfoPtrs, 0x00C67738);
GAME_VAR(uint32_t, timeMs, 0x00A14A50);
GAME_VAR(void*, effectManager, 0x00B6F1D4);
inline void* const collectibles = reinterpret_cast<void*>(0x00BCEB50);
GAME_VAR(void*, stimuli, 0x00C2C1A0);
GAME_VAR(int32_t, explosionWeaponType, 0x00BF442C);
inline void* const visibilityList = reinterpret_cast<void*>(0x00D12AF8);
inline char* const soundSizeBuffer = reinterpret_cast<char*>(0x00C34E50);
inline uint8_t* const objectInfos = reinterpret_cast<uint8_t*>(0x00C35200);
inline int32_t* const objectInfoModels = reinterpret_cast<int32_t*>(0x00C35228);        // +0x28 of each entry
inline int16_t* const objectInfoOverrides = reinterpret_cast<int16_t*>(0x00C34F58);    // 170 (model, index) pairs
// the prop anim models "NotUseable" is played on, by the broken model
GAME_VAR(int32_t, modelBF4478, 0x00BF4478);
GAME_VAR(int32_t, modelBF4474, 0x00BF4474);
GAME_VAR(int32_t, modelBF4144, 0x00BF4144);
GAME_VAR(int32_t, modelBF40FC, 0x00BF40FC);
GAME_VAR(int32_t, propBF4470, 0x00BF4470);
GAME_VAR(int32_t, propBF4460, 0x00BF4460);
GAME_VAR(int32_t, propBF4458, 0x00BF4458);
// the models whose breaking counts a stat
GAME_VAR(int32_t, modelBF410C, 0x00BF410C);
GAME_VAR(int32_t, modelBF3E70, 0x00BF3E70);
GAME_VAR(int32_t, modelBF3E6C, 0x00BF3E6C);
GAME_VAR(int32_t, modelBF3E68, 0x00BF3E68);
GAME_VAR(int32_t, modelBF3E64, 0x00BF3E64);

inline void* const objectVtable = reinterpret_cast<void*>(0x009134F4);

using re::At;

template <typename R, typename... A>
R CallVirtual(void* object, int slot, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[slot]))(object, args...);
}

CVector& Position(void* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return matrix ? *reinterpret_cast<CVector*>(matrix + 0x30) : At<CVector>(entity, 4);
}

// the object's vf30 (a byte in eax)
bool Vf30(CObject* o) {
    return CallVirtual<uint8_t>(o, 30) != 0;
}

void StopSound(CObject* o) {
    CallVirtual<void>(reinterpret_cast<void*>(o->sound), 13);
    Sound_Set(&o->sound, 0);
}

// (GetPool's loop over a chained pool: the flags are read by the chained index too)
template <typename F>
void ForEachChained(CPool<CObject>* pool, F&& f) {
    int count = pool->size;
    if (pool->unknown10) {
        count += Pool_GetSizeChained(reinterpret_cast<void*>(uintptr_t(pool->unknown10)));
    }
    for (int i = 0; i < count; i++) {
        CObject* o = nullptr;
        if (i < pool->size) {
            o = reinterpret_cast<CObject*>(re::PtrCast<uint8_t*>(pool->objects) + pool->elemSize * i);
        } else if (pool->unknown10) {
            o = static_cast<CObject*>(
                Pool_GetAtChained(reinterpret_cast<void*>(uintptr_t(pool->unknown10)), i - pool->size, 1));
        }
        if (!(pool->flags[i] & 0x80) && o) {
            f(o);
        }
    }
}

void DeleteObject(CObject* o) {
    World_Remove(o);
    CallVirtual<void>(o, 0, 1);
}

} // namespace

namespace c11object {

void __fastcall ProcessControl(CObject* self, re::Edx) {
    if (self->unknownB0 != 0) {
        return;
    }
    if (self->objectKind == 7 && !Dodgeball_IsActive()) {
        uint8_t* weaponInfo = At<uint8_t*>(GetProjectileInstance(At<int32_t>(self, 0x238)), 0x30);
        const char* name = At<const char*>(weaponInfo, 0x12C);
        uint8_t* drag = nullptr;
        if (name && *name) {
            uint32_t names[2];
            uint8_t path[8];
            CopyInt(&names[0], &boneNameKey);
            NameKey_Construct(&names[1], At<const char*>(weaponInfo, 0x12C));
            PathKey_Make(path, 2, names);
            drag = FindBoneInfo(path, boneNames);
            DestructArray(names, 4, 2, emptyFunction);
        }
        if (!drag) {
            drag = defaultDragInfo;
        }
        float speed = Vector_Magnitude(&self->moveSpeed);
        float factor;
        if (At<float>(drag, 0x1C) > speed) {
            factor = At<float>(drag, 0x24);
            if (At<float>(drag, 0x18) > speed) {   // too slow: stopped
                self->moveSpeed = vecUnset;
                self->turnSpeed = vecUnset;
            }
        } else {
            factor = At<float>(drag, 0x20);
        }
        self->moveSpeed.x = factor * self->moveSpeed.x;
        self->moveSpeed.y = self->moveSpeed.y * factor;
        self->moveSpeed.z = self->moveSpeed.z * factor;
        self->turnSpeed.x = self->turnSpeed.x * factor;
        self->turnSpeed.y = self->turnSpeed.y * factor;
        self->turnSpeed.z = factor * self->turnSpeed.z;
    }
    Physical_ProcessControl(self);
}

void __fastcall UpdateAmbientSound(CObject* self, re::Edx) {
    CEntity* player = playerPed;
    CVector d;
    CVector* diff = VectorSub(&d, &Position(player), &Position(self));
    float distSq = float((double(diff->x) * diff->x + double(diff->y) * diff->y) + double(diff->z) * diff->z);
    uint8_t* info = GetObjectInfo(self->modelIndex);
    if (!info) {
        return;
    }
    int kind = int8_t(uint8_t(info[0x20] << 4)) >> 4;
    if ((info[0x21] & 1) && !Vf30(self)) {
        float volume = float(int(uint8_t(info[0x21] >> 1)) / 100.0);
        Ambience_PlayEmitter(ambience, At<int32_t>(info, 0x14), distSq, &Position(self), volume, kind, 0, 200, 2000, 1);
    }
    if (At<int16_t>(info, 0xE) == -1) {
        return;
    }
    float bankRange = float(At<int32_t>(info, 0x10));
    float padded = float(double(bankRange) + (bankRange > 225.0 || std::isnan(bankRange) ? 100.0 : 25.0));
    float range = bankRange + padded;
    if (IsPlayerInFastVehicle()) {
        range = range + range;
    }
    bool bankLoaded = (self->objectFlags & 0x20) != 0;
    if (bankLoaded && range < distSq) {
        if (self->sound) {
            StopSound(self);
        }
        SoundBanks_Unload(soundBanks, At<int16_t>(info, 0xE), 1, 1);
        self->objectFlags &= ~0x20;
    } else if (!bankLoaded && !Vf30(self)) {
        bool check = true;
        double threshold = 0.0;
        if (IsPlayerInFastVehicle()) {
            // ahead of a fast player: the range stretched by how straight ahead it is
            CVector toObject;
            VectorSub(&toObject, &Position(self), &Position(player));
            CVector forward = cameraForward;
            Vector_Normalise(&toObject);
            float along = DotProduct(&toObject, &forward);
            if (0.949999988079071 <= along) {
                threshold = double(At<int32_t>(info, 0x10)) * 2.0 * (along - 0.949999988079071) / 0.05000000074505806;
            } else {
                check = false;
            }
        } else {
            threshold = At<int32_t>(info, 0x10);
        }
        if (check && distSq < threshold) {
            SoundBanks_Load(soundBanks, At<int16_t>(info, 0xE), 1, 1);
            self->objectFlags |= 0x20;
        }
    }

    if (info[0x21] & 1) {
        return;
    }
    float soundRange = float(*reinterpret_cast<const uint16_t*>(soundKinds + kind * 0xB8));
    bool near = distSq < double(soundRange) * soundRange;
    if (!self->sound) {
        if ((self->objectFlags & 0x20) && At<int32_t>(info, 0x14) &&
            SoundBanks_IsLoaded(soundBanks, At<int16_t>(info, 0xE)) && near && !Vf30(self)) {
            void* handle = Sounds_PlayAt(sounds, At<int32_t>(info, 0x14), &Position(self), kind, 0, 0, 0x78, 0, 0, 0,
                                         0, 6, 1, 1.0f, 0);
            Sound_Construct(&self->sound, handle);
            return;
        }
        if (!self->sound) {
            return;
        }
    }
    if (!near || Vf30(self)) {
        StopSound(self);
    }
}

void __fastcall ProcessCollision(CObject* self, re::Edx, int arg) {
    if (self->objectKind != 7) {
        Physical_ProcessCollision(self, arg);
        return;
    }
    At<float>(self, 0x19C) = 0.0f;
    if (self->unknownB0 != 0 || At<int32_t>(self, 0x1C) == 0) {
        return;
    }
    self->flags1BC &= 0x7F;
    At<uint8_t>(self, 0x1BD) &= ~4;
    uint8_t oldMatrix[0x48];
    Matrix_CopyConstruct(oldMatrix, Placeable_GetMatrix(self));
    if (!Fn_683B40(self)) {
        CallVirtual<void>(self, 37);   // ApplyMoveSpeed
        CallVirtual<void>(self, 38);
    }
    if (self->matrix) {
        Matrix_Reorthogonalise(self->matrix);
    }
    Entity_443AA0(self);
    CVector oldPos = *reinterpret_cast<CVector*>(oldMatrix + 0x30);
    At<int32_t>(self, 0x34) = 0;
    At<int32_t>(self, 0x38) = 1;
    CVector moved;
    At<float>(self, 0x19C) = Vector_Magnitude(VectorSub(&moved, &Position(self), &oldPos));   // how far it went
    Physical_RemoveAndAdd(self);
}

void __fastcall PlayNearbyPropAnim(CObject* self, re::Edx, int model, const char* action) {
    if (model == -1) {
        return;
    }
    CEntity* prop = FindNearestObjectOfTypeInRange(model, &Position(self), 2.0f, 8);
    if (prop) {
        void* node = ActTree_FindNode(action, CatStore_GetUnknown(At<int16_t>(prop, 0x22C)));
        PropAnim_4EE010(prop, node);
    }
}

void __cdecl DeleteAllTempObjects() {
    ForEachChained(objectPool, [](CObject* o) {
        if (o->objectType == 3) {
            DeleteObject(o);
        }
    });
}

void __cdecl DeleteAllMissionObjects() {
    ForEachChained(objectPool, [](CObject* o) {
        if (o->objectType == 2) {
            DeleteObject(o);
        }
    });
}

void __cdecl DeleteAllTempObjectsInArea(float x, float y, float z, float radius) {
    ForEachChained(objectPool, [&](CObject* o) {
        if (o->objectType != 3) {
            return;
        }
        CVector& p = Position(o);
        double dy = double(y) - p.y;
        double dx = double(x) - p.x;
        double dz = double(z) - p.z;
        float distSq = float((dy * dy + dx * dx) + dz * dz);
        if (distSq < double(radius) * radius) {
            DeleteObject(o);
        }
    });
}

void __fastcall RequestExtraModel(CObject* self, re::Edx) {
    uint8_t* info = GetObjectInfo(self->modelIndex);
    if (!info) {
        return;
    }
    int model = At<int32_t>(info, 0x34);
    if (model == -1) {
        return;
    }
    if (GetStreamingInfo(model)[4] != 1 && GetStreamingInfo(model)[4] != 2) {
        if (DistanceSquared(&Position(playerPed), &Position(self)) <= 25.0) {
            RequestModel(model, 0);
            return;
        }
    }
    uint8_t* modelInfo = modelInfoPtrs[model];
    if (modelInfo && At<uint8_t*>(modelInfo, 0x18)) {
        uint8_t* weapon = At<uint8_t*>(modelInfo, 0x18);
        WeaponInfo_7282A0(weapon);
        int extra = At<int32_t>(weapon, 0x84);
        if (extra != -1 && !IsModelRequested(extra)) {
            RequestModel(extra, 0);
        }
    }
}

void __cdecl UpdateAllAmbientSounds() {
    soundUpdateCounter++;
    auto update = [](CObject* o) {
        Object_UpdateAmbientSound(o);
        Object_RequestExtraModel(o);
    };
    ForEachChained(objectPool, update);
    ForEachChained(projectilePool, update);
}

void __cdecl StopAllAmbientSounds() {
    auto stop = [](CObject* o) {
        if (!(o->objectFlags & 0x20)) {
            return;
        }
        uint8_t* info = GetObjectInfo(o->modelIndex);
        if (info && At<int16_t>(info, 0xE) != -1) {
            SoundBanks_Unload(soundBanks, At<int16_t>(info, 0xE), 1, 1);
            o->objectFlags &= ~0x20;
        }
    };
    ForEachChained(objectPool, stop);
    ForEachChained(projectilePool, stop);
    ForEachChained(re::PtrCast<CPool<CObject>*>(propAnimPool), [](CObject* prop) { PropAnim_RequestExtraModel(prop); });
}

uint8_t __cdecl IsInObjectPools(CObject* object) {
    if (!object) {
        return 0;
    }
    CPool<CObject>* pools[2] = {objectPool, projectilePool};
    for (CPool<CObject>* pool : pools) {
        int slot = int((reinterpret_cast<uint8_t*>(object) - re::PtrCast<uint8_t*>(pool->objects)) / pool->elemSize);
        // (compared with the start plus the slot in bytes, not in slots)
        if (reinterpret_cast<uint8_t*>(object) != re::PtrCast<uint8_t*>(pool->objects) + slot) {
            slot = pool->size;
            if (pool->unknown10) {
                slot += Pool_GetIndexChained(reinterpret_cast<void*>(uintptr_t(pool->unknown10)), object);
            }
        }
        if (slot >= 0) {
            int count = pool->size;
            if (pool->unknown10) {
                count += Pool_GetSizeChained(reinterpret_cast<void*>(uintptr_t(pool->unknown10)));
            }
            if (slot <= count) {
                return 1;
            }
        }
    }
    return 0;
}

void __fastcall Break(CObject* self, re::Edx, int keepEffects) {
    if (self->objectFlags & 0x40) {
        return;
    }
    if (!Object_HasObjectInfo(self->modelIndex)) {
        return;
    }
    if (!Vf30(self)) {
        uint8_t* info = GetObjectInfo(self->modelIndex);
        if (info && (info[0x22] & 1)) {
            Ambience_RemoveEmitter(ambienceEmitters, self, 1);
        }
    }
    int model = self->modelIndex;
    if (model == modelBF4478 || model == modelBF4474) {
        Object_PlayNearbyPropAnim(self, propBF4470, "./NotUseable");
    } else if (model == modelBF4144) {
        Object_PlayNearbyPropAnim(self, propBF4460, "./NotUseable/Destroyed");
    } else if (model == modelBF40FC) {
        Object_PlayNearbyPropAnim(self, propBF4458, "./NotUseable");
    }
    self->unknown220 = 0;
    self->unknown48 = 0;
    self->unknown20 = 0;
    if (self->unknown28 == 0 && At<int32_t>(self, 0xAC) == 0) {
        Physical_RemoveFromMovingList(self);
    }
    self->moveSpeed = {0.0f, 0.0f, 0.0f};
    self->unknown28 = 1;
    self->turnSpeed.x = 0.0f;
    At<int32_t>(self, 0x44) = 1;
    self->turnSpeed.y = 0.0f;
    self->turnSpeed.z = 0.0f;
    VisibilityList_Remove(visibilityList, self);
    if (!uint8_t(keepEffects)) {
        Fn_5B3690(self);
    }
}

CObject* __fastcall Construct(CObject* self, re::Edx, int model, int create) {
    Physical_Construct(self);
    self->vtable = static_cast<re::Ptr32<void>*>(objectVtable);
    self->unknown204 = 0;
    self->sound = 0;
    if (uint8_t(create)) {
        Entity_SetModelIndex(self, model, 1);
    } else {
        Entity_SetModelIndexNoCreate(self, model);
    }
    Object_Init(self);
    return self;
}

CObject* __fastcall ConstructFromDummy(CObject* self, re::Edx, CEntity* dummy) {
    Physical_Construct(self);
    self->vtable = static_cast<re::Ptr32<void>*>(objectVtable);
    self->unknown204 = 0;
    self->sound = 0;
    Entity_SetModelIndexNoCreate(self, At<int16_t>(dummy, 0x10E));
    void* rwObject = At<void*>(dummy, 0x18);
    if (rwObject) {
        Entity_AttachToRwObject(self, rwObject);
    }
    rwObject = At<void*>(dummy, 0x18);
    if (!rwObject || At<int32_t>(rwObject, 4) == 0) {
        Placeable_SetMatrix(self, Placeable_GetMatrix(dummy), 0);
    }
    Matrix_CopyOnlyMatrix(reinterpret_cast<uint8_t*>(self) + 0x1C4, Placeable_GetMatrix(dummy));
    Entity_DetachFromRwObject(dummy);
    Object_Init(self);
    self->unknown111 = At<uint8_t>(dummy, 0x111);
    self->hashId = At<uint32_t>(dummy, 0x124);
    self->unknown224 = 0;
    return self;
}

uint8_t __fastcall InflictDamage(CObject* self, re::Edx, float damage, int, CEntity* attacker, int weaponType,
                                 const CVector* hitPos, int, int, int, int, int, int, int, int, int, int, int, int, int) {
    if ((self->objectFlags & 0x40) || !Physical_41C540(self) || damage <= 0.0f ||
        timeMs - uint32_t(self->unknown120) <= 100) {
        return 0;
    }
    self->unknown120 = int32_t(timeMs);
    uint8_t* info = GetObjectInfo(self->modelIndex);
    if (!info) {
        return 0;
    }
    // (a ped of kind 13 only hurts the ones marked so with the right weapon)
    bool hurt = true;
    if (attacker && At<int32_t>(attacker, 0x1310) == 0xD && (info[2] & 1)) {
        uint8_t* weapon = At<uint8_t*>(attacker, 0x1D0);
        hurt = weapon && At<uint8_t*>(weapon, 0x264) && At<uint8_t>(At<uint8_t*>(weapon, 0x264), 0x6A);
    }
    if (hurt && self->unknown220 < 30000) {
        self->unknown220 = int16_t(int(double(self->unknown220) - damage));
    }

    if (self->unknown220 > 0) {
        CVector pos = Position(self);
        if (hitPos) {
            pos = *hitPos;
        }
        if (At<int16_t>(info, 0xC) != -1) {
            uint8_t proxy[8];
            AE_Proxy_Construct(proxy);
            uint32_t effect = At<uint16_t>(info, 0xC);
            AE_Manager_Start(effectManager, &effect, proxy, 0);
            AE_Proxy_SetPosition(proxy, &pos, 0);
            AE_Proxy_Destruct(proxy);
        }
        if (At<int32_t>(info, 0x1C) && weaponType != explosionWeaponType) {
            Sounds_PlayAt(sounds, At<int32_t>(info, 0x1C), &pos, int8_t(info[0x20]) >> 4, 0, 0, 0x78, 0, 0, 0, 0, 6, 0,
                          1.0f, 0);
        }
        return 1;
    }

    // broken
    self->unknown220 = 0;
    if (At<int32_t>(info, 0x2C)) {
        float height = float(At<int16_t>(info, 0x30) * 0.00390625);
        CVector up = {0.0f, 0.0f, height};
        CVector at;
        Fn_5201E0(At<int32_t>(info, 0x2C), VectorAdd(&at, &Position(self), &up));
    }
    if (self->hashId) {
        int16_t collectible = Collectibles_FindByHash(collectibles, self->hashId);
        if (collectible >= 0 &&
            (Collectibles_IsType0(collectibles, collectible) || Collectibles_IsType4(collectibles, collectible) ||
             Collectibles_IsType5(collectibles, collectible))) {
            Collectibles_SetCollected(collectibles, collectible, 1);
            Collectibles_ShowCollectedMessage(collectibles, collectible);
        }
    }
    uint8_t proxies[2][8];
    ConstructArray(proxies, 8, 2, proxyConstruct);
    for (int i = 0; i < 2; i++) {
        uint16_t effect = At<uint16_t>(info, 4 + i * 2);
        if (effect == 0xFFFF) {
            continue;
        }
        uint32_t effectId = effect;
        AE_Manager_Start(effectManager, &effectId, proxies[i], 0);
        float height = float(At<int16_t>(info, 8 + i * 2) * 0.00390625);
        CVector up = {0.0f, 0.0f, height};
        CVector at;
        AE_Proxy_SetPosition(proxies[i], VectorAdd(&at, Entity_GetPosition(self), &up), 0);
    }
    if (attacker) {
        Stimulus_Add(stimuli, 2, attacker, self, nullptr, -1.0f, -1.0f);
        if (At<int32_t>(attacker, 0x1310) == 0xD) {
            int model = self->modelIndex;
            int stat = -1;
            if (IsModelInList25(model)) {
                stat = 0x15;
            } else if (IsModelInList30(model)) {
                stat = 0x14;
            } else if (model == modelBF410C) {
                stat = 0x1A;
            } else if (model == modelBF3E70 || model == modelBF3E6C || model == modelBF3E68 || model == modelBF3E64) {
                stat = 0x1B;
            }
            if (stat != -1) {
                IncrementStat(stat, 1);
            }
        }
        self->unknown224 = CallVirtual<int32_t>(attacker, 43);
    }
    if (At<int32_t>(info, 0x18)) {
        Sounds_PlayAt(sounds, At<int32_t>(info, 0x18), &Position(self), int8_t(info[0x20]) >> 4, 0, 0, 0x78, 0, 0, 0, 0,
                      6, 0, 1.0f, 0);
    }
    Object_Break(self, 0);
    DestructArray(proxies, 8, 2, proxyDestruct);
    return 1;
}

void __fastcall Teleport(CObject* self, re::Edx, float x, float y, float z) {
    World_Remove(self);
    Entity_SetPosition(self, x, y, z);
    Entity_443AA0(self);
    Entity_UpdateRwFrame(self);
    World_Add(self);
}

uint8_t __cdecl IsObjectPointerValid(CObject* object) {
    if (!Object_IsInObjectPools(object)) {
        return 0;
    }
    if (At<int32_t>(object, 0x58) == 0) {
        return At<int32_t>(object, 0x178) != 0 ? 1 : 0;
    }
    return 1;
}

uint8_t __cdecl GetSoundSizeFromName(void* name) {
    FixedString_ToChars(name, *static_cast<uint32_t*>(name), soundSizeBuffer, 0x100, 0, 1);
    if (Stricmp(soundSizeBuffer, "small") == 0) return 1;
    if (Stricmp(soundSizeBuffer, "medium") == 0) return 2;
    if (Stricmp(soundSizeBuffer, "large") == 0) return 3;
    if (Stricmp(soundSizeBuffer, "speech") == 0) return 4;
    if (Stricmp(soundSizeBuffer, "extralarge") == 0) return 5;
    if (Stricmp(soundSizeBuffer, "jumbo") == 0) return 8;
    if (Stricmp(soundSizeBuffer, "supersize") == 0) return 9;
    return Stricmp(soundSizeBuffer, "generic") != 0 ? 1 : 0;
}

uint8_t __cdecl HasObjectInfo(int model) {
    int16_t index = At<int16_t>(modelInfoPtrs[model], 0x12);
    if (index == -1) {
        return 0;
    }
    return (objectInfos + index * 0x38) != nullptr ? 1 : 0;
}

void __cdecl SetModelObjectInfoIndices() {
    int16_t index = 0;
    for (int32_t* model = objectInfoModels; reinterpret_cast<uintptr_t>(model) < 0x00C39B70; model += 0xE, index++) {
        if (*model != -1 && modelInfoPtrs[*model]) {
            At<int16_t>(modelInfoPtrs[*model], 0x12) = index;
        }
    }
    for (int i = 0; i < 170; i++) {
        int16_t model = objectInfoOverrides[i * 2];
        if (model != -1 && modelInfoPtrs[model]) {
            At<int16_t>(modelInfoPtrs[model], 0x12) = objectInfoOverrides[i * 2 + 1];
        }
    }
}

} // namespace c11object

// ---- registration ---------------------------------------------------------------------

#define OBJMISC_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

OBJMISC_REVERSED("CObject::ProcessControl", 0x004CF4A0, &c11object::ProcessControl);
OBJMISC_REVERSED("CObject::UpdateAmbientSound", 0x004CF620, &c11object::UpdateAmbientSound);
OBJMISC_REVERSED("CObject::ProcessCollision", 0x004CF9C0, &c11object::ProcessCollision);
OBJMISC_REVERSED("CObject::PlayNearbyPropAnim", 0x004CFAC0, &c11object::PlayNearbyPropAnim);
OBJMISC_REVERSED("CObject::DeleteAllTempObjects", 0x004CFDF0, &c11object::DeleteAllTempObjects);
OBJMISC_REVERSED("CObject::DeleteAllMissionObjects", 0x004CFE70, &c11object::DeleteAllMissionObjects);
OBJMISC_REVERSED("CObject::DeleteAllTempObjectsInArea", 0x004CFEF0, &c11object::DeleteAllTempObjectsInArea);
OBJMISC_REVERSED("CObject::RequestExtraModel", 0x004D0040, &c11object::RequestExtraModel);
OBJMISC_REVERSED("CObject::UpdateAllAmbientSounds", 0x004D0110, &c11object::UpdateAllAmbientSounds);
OBJMISC_REVERSED("CObject::StopAllAmbientSounds", 0x004D01F0, &c11object::StopAllAmbientSounds);
OBJMISC_REVERSED("CObject::IsInObjectPools", 0x004D03A0, &c11object::IsInObjectPools);
OBJMISC_REVERSED("CObject::Break", 0x004D0450, &c11object::Break);
OBJMISC_REVERSED("CObject::ConstructWithModel", 0x004D0580, &c11object::Construct);
OBJMISC_REVERSED("CObject::ConstructFromDummy", 0x004D05E0, &c11object::ConstructFromDummy);
OBJMISC_REVERSED("CObject::InflictDamage", 0x004D06C0, &c11object::InflictDamage);
OBJMISC_REVERSED("CObject::Teleport", 0x004D0B30, &c11object::Teleport);
OBJMISC_REVERSED("CObject::IsObjectPointerValid", 0x004D0B80, &c11object::IsObjectPointerValid);
OBJMISC_REVERSED("CObject::GetSoundSizeFromName", 0x004D0BB0, &c11object::GetSoundSizeFromName);
OBJMISC_REVERSED("CObjectData::HasObjectInfo", 0x004D0CD0, &c11object::HasObjectInfo);
OBJMISC_REVERSED("CObjectData::SetModelObjectInfoIndices", 0x004D0D10, &c11object::SetModelObjectInfoIndices);
