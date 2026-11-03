// More of CWeapon (the weapon object a ped holds): its effects (three slots of 0x1C
// bytes made on first use), its children attached to its bones, its scaling callback,
// throwing it as a projectile; the weapon models' streaming (a weapon's models: its
// own, the chain of models its info names by hash at +0x9C, and the one at +0x84); and
// CWeaponManager (0xC3AEF8): the weapon/ped pairs read from the data (WeaponPedPair:
// which ped model carries which weapon, on which sockets, how likely), given out as
// peds are made.

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "game/HeapCheck.h"
#include "game/Weapon.h"
#include "reversed.h"

namespace c12weapon {

using re::At;

struct CVector {
    float x, y, z;
};

// an effect slot of a weapon (0x1C bytes)
struct WeaponEffect {
    uint8_t proxy[0x10];   // +0 the effect proxy (+4 its effect: 0 none)
    int32_t endTime;       // +0x10 (-1)
    uint8_t unknown14;     // +0x14
    uint8_t unknown15;     // +0x15
    uint8_t unknown16[2];
    int32_t effectId;      // +0x18 (0xFF)
};
static_assert(offsetof(WeaponEffect, endTime) == 0x10);
static_assert(offsetof(WeaponEffect, effectId) == 0x18);
static_assert(sizeof(WeaponEffect) == 0x1C);

// a weapon/ped pair (0x14 bytes); the names are indices into the bone name table
struct WeaponPedPair {
    int32_t pedModel;          // +0 (-1: none yet)
    int32_t weaponModel;       // +4
    uint16_t pedName;          // +8
    uint16_t weaponName;       // +0xA
    uint16_t socketOnPed;      // +0xC
    uint16_t socketOnWeapon;   // +0xE
    uint8_t probability;       // +0x10 (percent)
    uint8_t flags;             // +0x11 1: winter only
    uint8_t unknown12[2];
};
static_assert(sizeof(WeaponPedPair) == 0x14);

struct CWeaponManager {
    WeaponPedPair pairs[100];   // +0
    int32_t numPairs;           // +0x7D0
};
static_assert(offsetof(CWeaponManager, numPairs) == 0x7D0);

// the game's short string (its data, its capacity, its buffer)
template <int N>
struct ShortString {   // (the game's layout: the reader writes through +0)
    re::Ptr32<char> data;
    int32_t capacity;
    char buffer[N];
};

inline void* const theAnimManager = reinterpret_cast<void*>(0x020C4910);
inline void* const theScriptStore = reinterpret_cast<void*>(0x00D02850);
inline void* const g_random = reinterpret_cast<void*>(0x00CF356C);
inline void* const boneNames = reinterpret_cast<void*>(0x00C3A978);
inline CWeaponManager* const TheWeaponManager = reinterpret_cast<CWeaponManager*>(0x00C3AEF8);
inline const char* const damageFxName = reinterpret_cast<const char*>(0x00913B3C);   // "damageFx"
constexpr uintptr_t vtblWeaponPedPairLoader = 0x00913B4C;
GAME_VAR(int32_t, gFirstWeaponModel, 0x00A136B0);
GAME_VAR(int32_t, gLastWeaponModel, 0x00A136B4);
GAME_VAR(int32_t, gFirstModelB8, 0x00A136B8);
GAME_VAR(int32_t, gLastModelBC, 0x00A136BC);
GAME_ARRAY(uint8_t*, ms_modelInfoPtrs, 0x00C67738);
GAME_VAR(uint8_t*, FindPlayerPed_ptr, 0x00C1AEA8);
GAME_VAR(void*, gEffectInfos, 0x00B6F1E8);
GAME_VAR(void*, gEffectManager, 0x00B6F1D4);
GAME_VAR(int32_t, gTimeMs, 0x00C1A9B4);
GAME_VAR(float, gThrowSpread, 0x00A45384);    // (0.07)
GAME_VAR(float, gThrowMinSpread, 0x00A45388); // (0.04)
GAME_ARRAY(float, gVectorBCC564, 0x00BCC564);
GAME_VAR(int32_t, currArea, 0x00BD1008);       // CGame::currArea
GAME_VAR(uint8_t, g_chapter, 0x00A147E0);
GAME_ARRAY(int32_t, g_chapterSeason, 0x00A147EC); // (2: winter)

GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __stdcall, VectorConstruct, 0x00401240, void* array, size_t size, int count, void* ctor);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(int32_t, __thiscall, Skeleton_GetBoneIndex, 0x00465580, void* skeleton, uint32_t hash, int32_t* index);
// streaming
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);   // (+4 its load state)
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, SetModelMissionRequired, 0x0052E720, int id);   // (flag 2, off the LRU list)
GAME_FN(void, __cdecl, SetMissionDoesntRequireModel, 0x0052FF40, int id);
GAME_FN(void, __cdecl, SetModelIsDeletable, 0x0052FE40, int id);
GAME_FN(void*, __thiscall, ScriptStore_GetCurrentScript, 0x005DB940, void* store);
GAME_FN(void, __thiscall, Script_AddModel, 0x005D9450, void* script, int id);
GAME_FN(uint8_t*, __cdecl, GetButes, 0x0051B450, int model);   // (the weapon info: +0x84, +0x9C)
GAME_FN(void, __thiscall, WeaponInfo_Request, 0x007282A0, uint8_t* info);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(int, __cdecl, GetModelIndex, 0x0051C330, const char* name);
GAME_FN(void, __thiscall, ModelInfo_AddRef, 0x0050EB90, uint8_t* info);
GAME_FN(void, __thiscall, ModelInfo_RemoveRef, 0x0050EBC0, uint8_t* info);
GAME_FN(void, __thiscall, ModelInfo_50EBF0, 0x0050EBF0, uint8_t* info);
GAME_FN(void, __cdecl, RegisterCallback, 0x0072B870, void* fn, int);
// rendering
GAME_FN(void, __cdecl, GetRenderState, 0x00519290, int state, int* value);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_FN(void, __thiscall, Entity_Render, 0x00466D40, void* entity);
// effects
GAME_FN(void, __thiscall, EffectProxy_Construct, 0x00668DA0, void* proxy);
GAME_FN(uint16_t, __thiscall, EffectInfos_FindId, 0x006679A0, void* infos, uint32_t name);
GAME_FN(void*, __thiscall, EffectInfos_Get, 0x006679F0, void* infos, int id);
GAME_FN(float, __thiscall, EffectInfo_GetDuration, 0x00666C40, void* info);
GAME_FN(void, __thiscall, EffectManager_Start, 0x00668290, void* manager, void* id, void* proxy, int);
GAME_FN(void, __thiscall, Effect_SetPosition, 0x00669040, void* proxy, const CVector* pos, int);
GAME_FN(void, __thiscall, Effect_SetDirection, 0x00669360, void* proxy, const CVector* dir);
GAME_FN(void, __thiscall, Effect_FadeOut, 0x00668D00, void* proxy, float time);
GAME_FN(void, __thiscall, Effect_Remove, 0x00668BA0, void* proxy);
GAME_FN(void, __thiscall, Effect_Stop, 0x00668E40, void* proxy);
GAME_FN(void*, __thiscall, Effect_GetNode, 0x00668E00, void* proxy);
GAME_FN(void, __thiscall, Effect_SetAttachment, 0x00668DE0, void* proxy, void* animData, uintptr_t handle);
// the anim manager: a bone's position and direction, attachments
GAME_FN(void, __thiscall, AnimManager_GetBonePosition, 0x006C1190, void* manager, void* animData, int, CVector* out, uintptr_t bone);
GAME_FN(void, __thiscall, AnimManager_GetBoneDirection, 0x006C11E0, void* manager, void* animData, int, CVector* out, uintptr_t bone);
GAME_FN(int, __thiscall, AnimManager_Attach, 0x006C2070, void* manager, void* animData, int bone, void* frame, int* handle,
        void* node, void* animData2, void* flag);
GAME_FN(void, __thiscall, AnimManager_Detach, 0x006C2200, void* manager, void* animData, uintptr_t handle);
GAME_FN(uint8_t*, __cdecl, GetNodeWorldMatrix, 0x006C9A90, void* node);   // (+0x24 its position)
GAME_FN(float, __thiscall, Random_GetFloat, 0x00577300, void* generator);
GAME_FN(int, __cdecl, AddProjectile_x64, 0x004D4D90, void* creator, int weaponId, re::Float3, re::Float3, uintptr_t, float, float, uintptr_t, uintptr_t, int, re::Float3, re::Float3, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline int AddProjectile(void* a0, int a1, float a2, float a3, float a4, float a5, float a6, float a7, int a8, float a9, float a10, int a11, uintptr_t a12, int a13, float a14, float a15, float a16, float a17, float a18, float a19, int a20, int a21) {
    return AddProjectile_x64(a0, a1, re::Float3{a2, a3, a4}, re::Float3{a5, a6, a7}, a8, a9, a10, a11, a12, a13, re::Float3{a14, a15, a16}, re::Float3{a17, a18, a19}, a20, a21);
}
// the data reader
GAME_FN(void*, __cdecl, DataReader_Open, 0x005B07E0, void* reader, int32_t* out, int);
GAME_FN(uint8_t, __thiscall, DataReader_Next, 0x005B0720, void* handle, void* loader, const char* name);
GAME_FN(void, __cdecl, DataReader_Close, 0x005B0860, void* handle);
GAME_FN(const char*, __thiscall, BoneNames_Get, 0x004A75B0, void* table, int index);
GAME_FN(int32_t, __thiscall, BoneNames_Add, 0x004A7BA0, void* table, const char* name);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
// the projectile code's (0x4D2760.. 0x4D4730)
GAME_FN(void, __cdecl, Fn_4D2760, 0x004D2760);
GAME_FN(void, __cdecl, Fn_4D3E90, 0x004D3E90);
GAME_FN(void, __cdecl, Fn_4D2BF0, 0x004D2BF0);
GAME_FN(void, __cdecl, Fn_4D4730, 0x004D4730);
GAME_FN(void, __thiscall, Nop_49A650, 0x0049A650, void* self);   // (an empty destructor)
// (their own, called where modes apply)
GAME_FN(void, __thiscall, Weapon_Attach, 0x004D9460, CWeapon* self);
GAME_FN(uint8_t, __thiscall, Weapon_StartEffect, 0x004D9EF0, CWeapon* self, int slot, int effectId, int bone, int a4, int a5);
GAME_FN(int, __thiscall, WeaponManager_GiveAccessory, 0x004DAE50, CWeaponManager* self, uint8_t* ped, int weaponModel,
        int socketOnPed, int socketOnWeapon);
inline void* const weaponEffectConstructor = reinterpret_cast<void*>(0x004D9220);
inline void* const weaponModelsCallback = reinterpret_cast<void*>(0x004DA420);

WeaponEffect* Effects(CWeapon* weapon) {
    return re::PtrCast<WeaponEffect*>(weapon->effects);
}

// ---- CWeapon ---------------------------------------------------------------------------

// 0x4D9220 (thiscall): an effect slot
WeaponEffect* __fastcall WeaponEffect_Construct(WeaponEffect* self, re::Edx) {
    EffectProxy_Construct(self);
    self->endTime = -1;
    self->effectId = 0xFF;
    return self;
}

// 0x4D9240: the anim callback SetScale installs: the first node's rotation scaled
// (its first row by x, the others by y), and every other node's too, their positions
// scaled about the first's
void __cdecl ScaleCallback(uint8_t* animObject, CWeapon* weapon) {
    uint8_t* nodes = At<uint8_t*>(animObject, 0x14);
    float sx = weapon->scale[0];
    float sy = weapon->scale[1];
    float sz = weapon->scale[2];
    float* t = At<float*>(nodes, 8);   // NiTransforms of 0x34 bytes
    CVector origin{t[9], t[10], t[11]};
    t[0] = sx * t[0];
    t[1] = sx * t[1];
    t[2] = t[2] * sx;
    for (int i = 3; i < 9; i++) {
        t[i] = t[i] * sy;
    }
    int count = At<int32_t>(nodes, 4);
    for (int n = 1; n < count; n++) {
        t += 13;
        for (int i = 0; i < 3; i++) {
            t[i] = t[i] * sx;
        }
        for (int i = 3; i < 9; i++) {
            t[i] = t[i] * sy;
        }
        CVector pos{t[9], t[10], t[11]};
        CVector offset;
        VectorSub(&offset, &pos, &origin);
        CVector scaled{offset.x * sx, offset.y * sy, offset.z * sz};
        CVector moved;
        VectorAdd(&moved, &origin, &scaled);
        t[9] = moved.x;
        t[10] = moved.y;
        t[11] = moved.z;
    }
}

// 0x4D9710 (the model in esi, the flags in ebx): loaded (1; with flag 2 the model is
// made mission-required and the current script's), else requested (not while it's
// on its way: states 2..4)
uint8_t RequestIfNotLoaded(int model, uint32_t flags) {
    if (GetStreamingInfo(model)[4] == 1) {
        if (flags & 2) {
            if (ScriptStore_GetCurrentScript(theScriptStore)) {
                Script_AddModel(ScriptStore_GetCurrentScript(theScriptStore), model);
            }
            SetModelMissionRequired(model);
        }
        return 1;
    }
    if (GetStreamingInfo(model)[4] != 2 && GetStreamingInfo(model)[4] != 3 && GetStreamingInfo(model)[4] != 4) {
        RequestModel(model, int(flags));
    }
    return 0;
}
RE_UNHOOKED_FN("CWeapon::RequestIfNotLoaded", 0x004D9710, &RequestIfNotLoaded);

// 0x4D9790: the weapon's models requested (flag 0x10 added); 1 when all are loaded.
// (The +0x84 model is the last chained model's, not the weapon's own)
uint8_t __cdecl RequestWeaponModels(int weaponId, int flags) {
    if (weaponId < gFirstWeaponModel || weaponId > gLastWeaponModel) {
        return 0;
    }
    uint32_t requestFlags = uint32_t(flags) | 0x10;
    uint8_t* info = GetButes(weaponId);
    uint8_t loaded = 1;
    for (uint32_t hash = At<uint32_t>(info, 0x9C); hash != 0; hash = At<uint32_t>(info, 0x9C)) {
        int model = GetModelIndexByHash(hash, gFirstWeaponModel, gLastWeaponModel);
        loaded &= RequestIfNotLoaded(model, requestFlags);
        info = GetButes(model);
    }
    loaded &= RequestIfNotLoaded(weaponId, requestFlags);
    WeaponInfo_Request(info);
    if (At<int32_t>(info, 0x84) != -1) {
        WeaponInfo_Request(info);
        loaded &= RequestIfNotLoaded(At<int32_t>(info, 0x84), requestFlags);
    }
    return loaded;
}

// 0x4D9850 / 0x4D98E0: the weapon's models let go (as the one, then its +0x84 model,
// then the chain)
void ReleaseWeaponModels(int weaponId, void(__cdecl* Release)(int)) {
    uint8_t* info = GetButes(weaponId);
    Release(weaponId);
    WeaponInfo_Request(info);
    if (At<int32_t>(info, 0x84) != -1) {
        WeaponInfo_Request(info);
        Release(At<int32_t>(info, 0x84));
    }
    for (uint32_t hash = At<uint32_t>(info, 0x9C); hash != 0;) {
        int model = GetModelIndexByHash(hash, gFirstWeaponModel, gLastWeaponModel);
        Release(model);
        hash = At<uint32_t>(GetButes(model), 0x9C);
    }
}

void __cdecl SetWeaponModelsNotMissionRequired(int weaponId) {
    ReleaseWeaponModels(weaponId, SetMissionDoesntRequireModel);
}

void __cdecl SetWeaponModelsDeletable(int weaponId) {
    ReleaseWeaponModels(weaponId, SetModelIsDeletable);
}

// 0x4D9C10 (thiscall): the children rendered, then (unless the player's +0x1E94 is
// set) the weapon itself with alpha blending on (states 8 6 7 0xC 0xA 0xB 0xE kept)
void __fastcall RenderWithChildren(CWeapon* self, re::Edx) {
    if (!self->rwObject) {
        return;
    }
    for (uint32_t i = 0; i < uint32_t(self->numChildren); i++) {
        if (self->children[i]) {
            using Fn = void(__thiscall*)(void*);
            re::PtrCast<Fn>(self->children[i]->vtable[18])(self->children[i]);
        }
    }
    int saved[7];
    GetRenderState(8, &saved[0]);
    GetRenderState(6, &saved[1]);
    GetRenderState(7, &saved[2]);
    GetRenderState(0xC, &saved[3]);
    GetRenderState(0xA, &saved[4]);
    GetRenderState(0xB, &saved[5]);
    GetRenderState(0xE, &saved[6]);
    SetRenderState(8, 1);
    SetRenderState(6, 1);
    SetRenderState(7, 2);
    SetRenderState(0xC, 0);
    SetRenderState(0xA, 5);
    SetRenderState(0xB, 6);
    SetRenderState(0xE, 0);
    if (!FindPlayerPed_ptr || At<int32_t>(FindPlayerPed_ptr, 0x1E94) == 0) {
        Entity_Render(self);
    }
    SetRenderState(8, saved[0]);
    SetRenderState(6, saved[1]);
    SetRenderState(7, saved[2]);
    SetRenderState(0xC, saved[3]);
    SetRenderState(0xA, saved[4]);
    SetRenderState(0xB, saved[5]);
    SetRenderState(0xE, saved[6]);
}

// 0x4D9DC0 (thiscall): the info's damage effect (+0xA0) started on the "damageFx"
// bone, fading out after its duration and half a second
void __fastcall StartDamageEffect(CWeapon* self, re::Edx) {
    int32_t id = EffectInfos_FindId(gEffectInfos, At<uint32_t>(self->info, 0xA0));
    if (uint16_t(id) == 0xFFFF) {
        return;
    }
    if (At<int32_t>(self, 0x134)) {
        Effect_Remove(self->effectProxy);
    }
    CVector position{0.0f, 0.0f, 0.0f};
    CVector direction{0.0f, 0.0f, 0.0f};
    // (the skeleton of its own anim data, +0x148 + 0x94)
    int bone = Skeleton_GetBoneIndex(At<void*>(self, 0x1DC), HashName(damageFxName), nullptr);
    AnimManager_GetBonePosition(theAnimManager, self->ownAnimData, 0, &position, bone);
    AnimManager_GetBoneDirection(theAnimManager, self->ownAnimData, 0, &direction, bone);
    EffectManager_Start(gEffectManager, &id, self->effectProxy, 0);
    Effect_SetPosition(self->effectProxy, &position, 0);
    CVector dir = direction;
    Effect_SetDirection(self->effectProxy, &dir);
    float duration = EffectInfo_GetDuration(EffectInfos_Get(gEffectInfos, id));
    Effect_FadeOut(self->effectProxy, float(double(duration) + 0.5));
}

// 0x4D9EF0 (thiscall): the effect started in the slot, attached to the bone; 0 when
// it isn't made or can't be attached (then stopped)
uint8_t __fastcall StartEffect(CWeapon* self, re::Edx, int slot, int effectId, int bone, int a4, int a5) {
    WeaponEffect* effect = &Effects(self)[slot];
    if (At<int32_t>(effect, 4)) {
        Effect_Stop(effect);
        Effect_Remove(&Effects(self)[slot]);
    }
    EffectManager_Start(gEffectManager, &effectId, &Effects(self)[slot], 0);
    effect = &Effects(self)[slot];
    if (!At<int32_t>(effect, 4)) {
        return 0;
    }
    void* node = Effect_GetNode(effect);
    if (!node) {
        return 0;
    }
    int handle = 0;
    if (AnimManager_Attach(theAnimManager, self->animData, bone, nullptr, &handle, nullptr, nullptr, node)) {
        Effect_Stop(&Effects(self)[slot]);
        Effect_Remove(&Effects(self)[slot]);
        return 0;
    }
    Effect_SetAttachment(&Effects(self)[slot], self->animData, handle);
    Effects(self)[slot].unknown14 = uint8_t(a4);
    Effects(self)[slot].endTime = gTimeMs + 10000000;
    Effects(self)[slot].effectId = uint16_t(effectId);
    Effects(self)[slot].unknown15 = uint8_t(a5);
    return 1;
}

// 0x4DA010 (thiscall): the child into its slot, attached to our anim data's bones
void __fastcall SetChild(CWeapon* self, re::Edx, CWeapon* child, int index, int boneA, int boneB) {
    heapcheck::CheckIndex(self, self->children, index, self->childCapacity, "CWeapon::SetChild");   // (testing)
    self->children[index] = child;
    CWeapon* weapon = self->children[index];
    uint8_t* animData = self->animData;
    if (weapon->holderAnimData && weapon->attachHandle) {
        AnimManager_Detach(theAnimManager, weapon->holderAnimData, weapon->attachHandle);
        weapon->attachHandle = 0;
    }
    weapon->holderAnimData = animData;
    weapon->boneA = boneA;
    weapon->boneB = boneB;
    Weapon_Attach(self->children[index]);
}

// 0x4DA190 (thiscall): thrown from where its node is, in a random direction (x and y
// within the spread, at least the minimum across; the length ignores z), as a
// projectile of the weapon; then let go of its attachment
int __fastcall Throw(CWeapon* self, re::Edx, int unitScale) {
    uint8_t* matrix = GetNodeWorldMatrix(At<void*>(self->animData, 0xC));
    CVector pos{At<float>(matrix, 0x24), At<float>(matrix, 0x28), At<float>(matrix, 0x2C)};
    float hi = gThrowSpread;
    float lo = -gThrowSpread;
    float dx = Random_GetFloat(g_random) * (hi - lo) + lo;
    hi = gThrowSpread;
    lo = -gThrowSpread;
    float dy = Random_GetFloat(g_random) * (hi - lo) + lo;
    hi = gThrowSpread;
    float dz = Random_GetFloat(g_random) * (hi - 0.0f) + 0.0f;
    float length = float(std::sqrt(double((dx * dx + dy * dy) + 0.0f * 0.0f)));
    if (length < gThrowMinSpread) {
        dx = gThrowMinSpread;
        dy = gThrowMinSpread;
    }
    float a[3] = {gVectorBCC564[0], gVectorBCC564[1], gVectorBCC564[2]};
    float b[3] = {gVectorBCC564[0], gVectorBCC564[1], gVectorBCC564[2]};
    if (re::Flag(unitScale)) {
        a[0] = a[1] = a[2] = 1.0f;
        b[0] = b[1] = b[2] = 1.0f;
    }
    int projectile = AddProjectile(nullptr, self->weaponId, pos.x, pos.y, pos.z, dx, dy, dz, 0, 0.0f, 0.0f, 0, 0, 0,
                                   a[0], a[1], a[2], b[0], b[1], b[2], 0, 0);
    if (self->holderAnimData && self->attachHandle) {
        AnimManager_Detach(theAnimManager, self->holderAnimData, self->attachHandle);
        self->attachHandle = 0;
    }
    return projectile;
}

// 0x4DA420: every weapon model's (and the 0xA136B8.. range's) info through 0x50EBF0
void __cdecl WeaponModelsCallback() {
    for (int model = gFirstWeaponModel; model <= gLastWeaponModel; model++) {
        ModelInfo_50EBF0(ms_modelInfoPtrs[model]);
    }
    for (int model = gFirstModelB8; model <= gLastModelBC; model++) {
        ModelInfo_50EBF0(ms_modelInfoPtrs[model]);
    }
}

// 0x4DA480
void __cdecl RegisterWeaponModelsCallback() {
    RegisterCallback(weaponModelsCallback, 0);
}

// 0x4DA520 (thiscall): an effect on the bone into a slot (the one with the same effect,
// else the first ending before slot 0's, else slot 0; the slots made on first use);
// the slot, -1 when not started
int __fastcall AddEffect(CWeapon* self, re::Edx, int effectId, int bone, int a3, int a4) {
    if (bone == -1 || !At<void*>(self->animData, 0x94)) {
        return -1;
    }
    int slot;
    if (!self->effects) {
        int32_t* memory = static_cast<int32_t*>(Malloc(0x58));
        if (memory) {
            memory[0] = 3;
            VectorConstruct(memory + 1, 0x1C, 3, weaponEffectConstructor);
            self->effects = reinterpret_cast<uint8_t*>(memory + 1);
        } else {
            self->effects = nullptr;
        }
        slot = 0;
    } else {
        WeaponEffect* effects = Effects(self);
        slot = -1;
        for (int i = 0; i < 3; i++) {
            if (effects[i].effectId == int32_t(uint16_t(effectId))) {
                slot = i;
                break;
            }
        }
        if (slot == -1) {
            slot = 0;
            int32_t first = effects[0].endTime;
            for (int i = 0; i < 3; i++) {
                if (effects[i].endTime < first) {
                    slot = i;
                    break;
                }
            }
        }
    }
    if (!Weapon_StartEffect(self, slot, effectId, bone, a3, a4)) {
        return -1;
    }
    return slot;
}

// 0x4DA680: a reference added to (or with `remove`, taken from) each of the weapon's
// models (removing only from loaded models that have references)
void ChangeRef(int model, bool remove) {
    uint8_t* modelInfo = ms_modelInfoPtrs[model];
    if (!remove) {
        ModelInfo_AddRef(modelInfo);
    } else if (modelInfo && At<int16_t>(modelInfo, 0x14) != 0) {
        ModelInfo_RemoveRef(modelInfo);
    }
}

void __cdecl AddWeaponModelRefs(int weaponId, int remove) {
    uint8_t* info = GetButes(weaponId);
    bool removing = re::Flag(remove);
    ChangeRef(weaponId, removing);
    WeaponInfo_Request(info);
    if (At<int32_t>(info, 0x84) != -1) {
        WeaponInfo_Request(info);
        ChangeRef(At<int32_t>(info, 0x84), removing);
    }
    for (uint32_t hash = At<uint32_t>(info, 0x9C); hash != 0;) {
        int model = GetModelIndexByHash(hash, gFirstWeaponModel, gLastWeaponModel);
        ChangeRef(model, removing);
        hash = At<uint32_t>(GetButes(model), 0x9C);
    }
}

// 0x4D93E0 / 0x4D93F0 / 0x4D9400: the projectile code's init and shutdown, and two
// empty calls
void __cdecl InitProjectiles() {
    Fn_4D2760();
    Fn_4D3E90();
}

void __cdecl Nothing4D93F0() {
    Nop_49A650(nullptr);
    Nop_49A650(nullptr);
}

void __cdecl ShutdownProjectiles() {
    Fn_4D2BF0();
    Fn_4D4730();
}

// ---- CWeaponManager -----------------------------------------------------------------

// 0x4DAD90 (thiscall): the pairs' ped and weapon models from their names
void __fastcall MatchLookupToIndex(CWeaponManager* self, re::Edx) {
    for (int i = 0; i < self->numPairs; i++) {
        WeaponPedPair& pair = self->pairs[i];
        pair.pedModel = GetModelIndex(BoneNames_Get(boneNames, pair.pedName));
        pair.weaponModel = GetModelIndex(BoneNames_Get(boneNames, pair.weaponName));
    }
}

// 0x4DADF0 (thiscall): the pairs read from the data (counted as they're read)
void __fastcall Load(CWeaponManager* self, re::Edx, void* reader) {
    int32_t local = 0;
    void* handle = DataReader_Open(reader, &local, 1);
    for (;;) {
        uintptr_t loader = vtblWeaponPedPairLoader;   // (a WeaponPedPairLoader: its vtable only)
        if (!DataReader_Next(handle, &loader, "WeaponPedPair")) {
            break;
        }
        self->numPairs++;
    }
    DataReader_Close(handle);
}

// 0x4DAF70: WeaponAccessoryPedPairLoader's vf1: the next pair read (Ped, Weapon,
// SocketOnPed, SocketOnWeapon: names kept in the bone name table; Probablility;
// WinterOnly). (The values the reader doesn't write are stack garbage in the game; 0
// here)
void __fastcall LoadPair(void*, re::Edx, void* reader) {
    using ReadString = void(__thiscall*)(void*, void*, int, const char*);
    using ReadInt = void(__thiscall*)(void*, int32_t*, const char*);
    using ReadBool = void(__thiscall*)(void*, uint8_t*, const char*);
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(reader);
    CWeaponManager* manager = TheWeaponManager;

    ShortString<0x11> ped = {nullptr, 0x11, {0}};
    ped.data = ped.buffer;
    re::PtrCast<ReadString>(vtable[9])(reader, &ped, 0x11, "Ped");
    manager->pairs[manager->numPairs].pedName = uint16_t(BoneNames_Add(boneNames, ped.data));
    ShortString<0x11> weapon = {nullptr, 0x11, {0}};
    weapon.data = weapon.buffer;
    re::PtrCast<ReadString>(vtable[9])(reader, &weapon, 0x11, "Weapon");
    manager->pairs[manager->numPairs].weaponName = uint16_t(BoneNames_Add(boneNames, weapon.data));
    ShortString<0x11> onPed = {nullptr, 0x11, {0}};
    onPed.data = onPed.buffer;
    re::PtrCast<ReadString>(vtable[9])(reader, &onPed, 0x11, "SocketOnPed");
    manager->pairs[manager->numPairs].socketOnPed = uint16_t(BoneNames_Add(boneNames, onPed.data));
    ShortString<0x11> onWeapon = {nullptr, 0x11, {0}};
    onWeapon.data = onWeapon.buffer;
    re::PtrCast<ReadString>(vtable[9])(reader, &onWeapon, 0x11, "SocketOnWeapon");
    manager->pairs[manager->numPairs].socketOnWeapon = uint16_t(BoneNames_Add(boneNames, onWeapon.data));
    int32_t probability = 0;
    re::PtrCast<ReadInt>(vtable[14])(reader, &probability, "Probablility");
    manager->pairs[manager->numPairs].probability = uint8_t(probability);
    uint8_t winterOnly = 0;
    re::PtrCast<ReadBool>(vtable[15])(reader, &winterOnly, "WinterOnly");
    uint8_t& flags = manager->pairs[manager->numPairs].flags;
    flags ^= (flags ^ winterOnly) & 1;
}

// 0x4DB100 (thiscall)
CWeaponManager* __fastcall Construct(CWeaponManager* self, re::Edx) {
    for (int i = 0; i < 100; i++) {
        WeaponPedPair& pair = self->pairs[i];
        pair.flags &= 0xFE;
        pair.pedModel = -1;
        pair.weaponModel = -1;
        pair.pedName = 0;
        pair.weaponName = 0;
        pair.socketOnPed = 0;
        pair.socketOnWeapon = 0;
        pair.probability = 0;
    }
    self->numPairs = 0;
    return self;
}

// 0x4DB1C0 (thiscall): a ped's weapons, by its model's pairs: the season right, the
// chance met
void __fastcall AddWeaponsFor(CWeaponManager* self, re::Edx, uint8_t* ped) {
    if ((At<uint8_t>(ped, 0x108) & 7) != 3) {
        return;
    }
    int32_t pedModel = At<int16_t>(ped, 0x10E);
    for (int i = 0; i < self->numPairs; i++) {
        WeaponPedPair& pair = self->pairs[i];
        if (pair.pedModel != pedModel) {
            continue;
        }
        uint8_t winter = IsAreaInterior(currArea) && g_chapterSeason[g_chapter] == 2 ? 1 : 0;
        if ((pair.flags & 1) != winter) {
            continue;
        }
        float chance = float(double(int32_t(pair.probability)) / 100.0);
        float roll = Random_GetFloat(g_random);
        if (!(chance >= roll)) {
            continue;
        }
        WeaponManager_GiveAccessory(self, ped, pair.weaponModel, pair.socketOnPed, pair.socketOnWeapon);
    }
}

} // namespace c12weapon

// ---- registration ---------------------------------------------------------------------

#define WEAPONACC_C12D_REVERSED(name, address, fn)   \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

WEAPONACC_C12D_REVERSED("CWeaponEffect::CWeaponEffect", 0x004D9220, &c12weapon::WeaponEffect_Construct);
WEAPONACC_C12D_REVERSED("CWeapon::ScaleCallback", 0x004D9240, &c12weapon::ScaleCallback);
WEAPONACC_C12D_REVERSED("CWeapon::InitProjectiles", 0x004D93E0, &c12weapon::InitProjectiles);
WEAPONACC_C12D_REVERSED("CWeapon::Nothing4D93F0", 0x004D93F0, &c12weapon::Nothing4D93F0);
WEAPONACC_C12D_REVERSED("CWeapon::ShutdownProjectiles", 0x004D9400, &c12weapon::ShutdownProjectiles);
WEAPONACC_C12D_REVERSED("CWeapon::RequestWeaponModels", 0x004D9790, &c12weapon::RequestWeaponModels);
WEAPONACC_C12D_REVERSED("CWeapon::SetWeaponModelsNotMissionRequired", 0x004D9850, &c12weapon::SetWeaponModelsNotMissionRequired);
WEAPONACC_C12D_REVERSED("CWeapon::SetWeaponModelsDeletable", 0x004D98E0, &c12weapon::SetWeaponModelsDeletable);
WEAPONACC_C12D_REVERSED("CWeapon::RenderWithChildren", 0x004D9C10, &c12weapon::RenderWithChildren);
WEAPONACC_C12D_REVERSED("CWeapon::StartDamageEffect", 0x004D9DC0, &c12weapon::StartDamageEffect);
WEAPONACC_C12D_REVERSED("CWeapon::StartEffect", 0x004D9EF0, &c12weapon::StartEffect);
WEAPONACC_C12D_REVERSED("CWeapon::SetChild", 0x004DA010, &c12weapon::SetChild);
WEAPONACC_C12D_REVERSED("CWeapon::Throw", 0x004DA190, &c12weapon::Throw);
WEAPONACC_C12D_REVERSED("CWeapon::WeaponModelsCallback", 0x004DA420, &c12weapon::WeaponModelsCallback);
WEAPONACC_C12D_REVERSED("CWeapon::RegisterWeaponModelsCallback", 0x004DA480, &c12weapon::RegisterWeaponModelsCallback);
WEAPONACC_C12D_REVERSED("CWeapon::AddEffect", 0x004DA520, &c12weapon::AddEffect);
WEAPONACC_C12D_REVERSED("CWeapon::AddWeaponModelRefs", 0x004DA680, &c12weapon::AddWeaponModelRefs);
WEAPONACC_C12D_REVERSED("CWeaponManager::MatchLookupToIndex", 0x004DAD90, &c12weapon::MatchLookupToIndex);
WEAPONACC_C12D_REVERSED("CWeaponManager::Load", 0x004DADF0, &c12weapon::Load);
WEAPONACC_C12D_REVERSED("WeaponAccessoryPedPairLoader::LoadPair", 0x004DAF70, &c12weapon::LoadPair);
WEAPONACC_C12D_REVERSED("CWeaponManager::CWeaponManager", 0x004DB100, &c12weapon::Construct);
WEAPONACC_C12D_REVERSED("CWeaponManager::AddWeaponsFor", 0x004DB1C0, &c12weapon::AddWeaponsFor);
