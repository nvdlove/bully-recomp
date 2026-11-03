// The action tree's prop tracks of 0x657420..0x658870: starting a prop's animation (by
// name, or the model's own "<model>\<suffix>" one), firing a projectile from a prop's
// bone, playing a node on a prop, handing a prop over, the locker save, entity flags at
// the end, and turning a prop to face the ped (the player's: towards a clear direction).

#include <cstdint>
#include <cstring>

#include "game/ActionFactories.h"
#include "game/PropTracks.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
using namespace actc;
using proptr::NodeTransform;
using proptr::UsePoint_GetEntity;

namespace {

GAME_FN(void, __thiscall, AreaTransition_SetTarget, 0x004152B0, void* transition, void* target);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(uint8_t*, __thiscall, AnimDict_Find, 0x006B1D60, uint8_t* dict, uint32_t name, uintptr_t, int32_t* index);
GAME_FN(uint8_t*, __thiscall, Anim_GetSlot, 0x006B9380, void* self, int32_t type);
GAME_FN(uint8_t*, __thiscall, Entries2C_GetAt, 0x00607480, uint8_t* dict, uint32_t i);
GAME_FN(uint8_t*, __thiscall, Entries2C_Find, 0x006074A0, uint8_t* dict, const char* name, uint32_t* index);
GAME_FN(int32_t, __thiscall, Ped_GetStat, 0x00477060, uint8_t* ped, int32_t stat);
GAME_FN(int, __thiscall, AnimManager_Blend, 0x006C0D40, void* manager, uint8_t* blend, uintptr_t index, uint8_t* slot,
        int flags, float speed, float blendIn);
GAME_FN(void, __thiscall, DebugPrint_44C070, 0x0044C070, void* self, char* buffer, int size, int);
GAME_FN(void, __thiscall, AnimManager_Play, 0x006C0E90, void* manager, uint8_t* blend, uint8_t* anim, int a, float time,
        int b, int c);
GAME_FN(void, __thiscall, NameKey_FromString, 0x005FA890, uint32_t* key, const char* name);
GAME_FN(uint8_t, __thiscall, AnimationParametricTrack_Start, 0x00613520, uint8_t* self, uint8_t* owner);
GAME_FN(uint32_t*, __thiscall, NameKey_Copy, 0x005FA880, uint32_t* key, const uint32_t* from);
GAME_FN(void, __thiscall, HashPath_Set, 0x005FA8C0, int32_t* path, int32_t count, uint32_t* hashes);
GAME_FN(uint8_t*, __cdecl, FindBoneInfo, 0x0072AEA0, int32_t* path, void* names);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void*, __thiscall, ElementTable_FindById, 0x00465580, void* table, int id, int* outIndex);
GAME_FN(uint8_t*, __thiscall, ElementTable_GetElement, 0x00464EA0, void* table, int index);
GAME_FN(int, __thiscall, AnimManager_GetNodePosition, 0x006C1190, void* manager, uint8_t* anim, int bone, V3* out, uintptr_t);
GAME_FN(int, __thiscall, AnimManager_GetNodeTransform, 0x006BF570, void* manager, uint8_t* anim, int bone,
        NodeTransform* out, void* node);
GAME_FN(void, __thiscall, Matrix_FromNode, 0x00412A70, uint8_t* matrix, NodeTransform* node);
GAME_FN(V3*, __cdecl, VectorScale, 0x00412350, V3* out, const V3* v, float s);
GAME_FN(uint8_t*, __cdecl, GetModelInfo, 0x0051C190, const char* name, int* id);
GAME_FN(uint8_t*, __cdecl, Projectile_Fire_x64, 0x004D4D90, uint8_t* shooter, int weapon, re::Float3, re::Float3, uint8_t* info, float speed, float spread, void* effect, uintptr_t, int, re::Float3, re::Float3, uintptr_t, int);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* Projectile_Fire(uint8_t* a0, int a1, float a2, float a3, float a4, float a5, float a6, float a7, uint8_t* a8, float a9, float a10, void* a11, uintptr_t a12, int a13, float a14, float a15, float a16, float a17, float a18, float a19, int a20, int a21) {
    return Projectile_Fire_x64(a0, a1, re::Float3{a2, a3, a4}, re::Float3{a5, a6, a7}, a8, a9, a10, a11, a12, a13, re::Float3{a14, a15, a16}, re::Float3{a17, a18, a19}, a20, a21);
}
GAME_FN(uint8_t*, __cdecl, GetProjectileInstance, 0x004D34B0, int index);
GAME_FN(void, __thiscall, ProjectileInfo_UpdateTrail, 0x004D3DE0, uint8_t* self, uint8_t* projectile, float alpha);
GAME_FN(void, __cdecl, Stats_Increment, 0x00458480, int stat, int by);
GAME_FN(void, __thiscall, PropAnim_4ED940, 0x004ED940, uint8_t* prop);
GAME_FN(void, __thiscall, Ctl_Play, 0x005F4A50, uint8_t* controller, uint8_t* node);
GAME_FN(void, __thiscall, Ctl_StartTracks, 0x005F4110, uint8_t* controller, float dt, int all);
GAME_FN(uint8_t, __thiscall, Ctl_IsPlaying, 0x005F4800, uint8_t* controller, const void* name, int ancestors, int nested);
GAME_FN(uint8_t*, __thiscall, Node_Get0C, 0x005F4F90, uint8_t* reference, int32_t key);
GAME_FN(uint8_t*, __thiscall, ModelInfo_GetAnimInfo, 0x005201D0, uint8_t* modelInfo);
GAME_FN(void, __thiscall, Gangs_72C0A0, 0x0072C0A0, void* self, int32_t id, int32_t value);
GAME_FN(void, __thiscall, CameraCapsule_Set, 0x004F8AE0, uint8_t* capsule, const V3* a, const V3* b, float radius);
GAME_FN(void, __thiscall, CapsuleQuery_Setup, 0x00562D80, uint8_t* query, uint8_t* capsule, int lists, ColPoint* points,
        int maxPoints);
GAME_FN(int32_t, __thiscall, CapsuleQuery_Scan, 0x00563E80, uint8_t* query);
GAME_FN(void, __thiscall, PedUsePoint_Set, 0x00478850, uint8_t* usePoint, uint8_t* prop);
GAME_FN(void, __thiscall, Prop_ReleasePed, 0x004EE060, uint8_t* prop, uint8_t* ped);
GAME_FN(void, __thiscall, PropAnim_AttachPed, 0x004ED830, uint8_t* prop, uint8_t* ped);
GAME_FN(void, __thiscall, PropAnim_MarkUsed, 0x004EA6A0, uint8_t* prop);
GAME_FN(uint8_t, __thiscall, PAnimModelInfo_IsKind2, 0x00520460, uint8_t* modelInfo);
GAME_FN(void, __thiscall, Prop_CrushVehicles, 0x004EE690, uint8_t* prop);
GAME_FN(uint8_t, __thiscall, ModelInfo_IsPropAnim, 0x00520430, uint8_t* modelInfo);
GAME_FN(float, __thiscall, Entity_GetHeading, 0x00415470, uint8_t* entity);
GAME_FN(float*, __thiscall, Camera_GetPosition, 0x0041AAC0, void* camera);
GAME_FN(V3*, __thiscall, Camera_GetViewDirection, 0x0041AA70, void* camera, V3* out);
GAME_FN(void, __thiscall, ColPoint_Construct, 0x00415FA0, ColPoint* colPoint);
GAME_FN(void, __cdecl, ClearExtraEntities, 0x0045D5D0);
GAME_FN(void, __cdecl, AddExtraEntity, 0x0045D5E0, uint8_t* entity);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const float* a, const float* b);
GAME_FN(uint8_t, __cdecl, AreaIsClear, 0x00657BC0, const V3* a, const V3* b, float radius, int lists);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const V3* start, const V3* end, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(float, __cdecl, AngleBetweenPoints, 0x00432D10, float x1, float y1, float x2, float y2);
GAME_FN(float, __cdecl, LimitRadianAngle, 0x00432EE0, float angle);
GAME_FN(void, __cdecl, Entity_SetHeadingInPlace, 0x006572B0, uint8_t* entity, float heading);

GAME_VAR(uint8_t*, missionPool, 0x020C7C48);   // a pool: +0 its objects, +4 their flags (0x80: free)
GAME_VAR(uint8_t*, ignoreEntity, 0x00C1AE70);  // CWorld::pIgnoreEntity
GAME_VAR(uint8_t, losFlag_C1AE82, 0x00C1AE82);
GAME_VAR(float, cameraHeading_C3CD08, 0x00C3CD08);
GAME_VAR(int32_t, playerWeaponStatModel, 0x00BF4358);
GAME_VAR(int32_t, lockerAnimGroup, 0x01B65518);
GAME_VAR(int32_t, playOnPropAnimGroup, 0x01B65504);
// the models the player's clear-direction tests look through (8; the line of sight
// is given 7 of them as its extra entities)
GAME_ARRAY(int32_t, seeThroughModels, 0x00BF40C0);
inline uint8_t* SeeThrough(int i) {
    return reinterpret_cast<uint8_t*>(uintptr_t(uint32_t(seeThroughModels[i])));
}
inline char* const animNameBuffer = reinterpret_cast<char*>(0x00C221A8);
inline void* const areaTransition = reinterpret_cast<void*>(0x00BCC580);
inline void* const gangs = reinterpret_cast<void*>(0x020C8A00);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
inline const char* const openSuffix = reinterpret_cast<const char*>(0x0093BDFC);   // "OPEN"
inline const char* const pathSeparator = reinterpret_cast<const char*>(0x00901AE4);   // "\\"

// the pool's first object (null when its slot is free)
uint8_t* FirstPoolObject() {
    return (*At<uint8_t*>(missionPool, 4) & 0x80) ? nullptr : At<uint8_t*>(missionPool, 0);
}

// "<the prop's model's name>\<suffix>" in the game's name buffer
void PropAnimName(uint8_t* owner, const char* suffix) {
    uint8_t* prop = UsePoint_GetEntity(owner);
    const char* model = reinterpret_cast<const char*>(modelInfoPtrs[At<int16_t>(prop, 0x10E)] + 0x58);
    std::strcpy(animNameBuffer, model);
    std::memcpy(animNameBuffer + std::strlen(animNameBuffer), pathSeparator, 2);
    std::strcat(animNameBuffer, suffix);
}

// Start (vf2, thiscall) of the prop animation tracks: the anim (+0x18 by name, or with a
// suffix (+SuffixAt) the prop model's own; "OPEN" on the player's door sends the area
// transition on) blended in on the prop's blend and placed at its start (+0x28); +0x20 2
// loops, +0x24 the slot, +0x1C its group, +0x30 the speed (times the ped's stat 0x14 %),
// +0x34 the blend in; +SyncedAt: at once. The prop's flag +0xE4 kept (+0x50) and set with
// +0x3C. BoneAt: the prop's bone index (+0x68) of the bone id there first. Always false
template <int SuffixAt, int SyncedAt, int BoneAt>
uint8_t __fastcall PropAnimation_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    At<int32_t>(self, 0x48) = -1;
    if constexpr (BoneAt != 0) {
        int index = 0;
        int bone = -1;
        uint8_t* anim = At<uint8_t*>(At<uint8_t*>(owner, 0xC), 0x114);
        if (anim && At<void*>(anim, 0x94) && ElementTable_FindById(At<void*>(anim, 0x94), At<int32_t>(self, BoneAt), &index) &&
            index != -1) {
            bone = index;
        }
        At<int32_t>(self, 0x68) = bone;
    }
    vc::Call<void>(self, acttree::kTrackResolve);
    uint8_t* blend = At<uint8_t*>(self, 0x4C);
    if (!blend || !At<void*>(blend, 0x94)) {
        return 0;
    }
    uint8_t* dict = At<uint8_t*>(At<uint8_t*>(blend, 0x94), 0x10);
    if (!dict) {
        return 0;
    }
    int32_t index;
    const char* suffix = At<const char*>(self, SuffixAt);
    if (suffix) {
        uint8_t* prop = At<uint8_t*>(At<uint8_t*>(self, 0x40), 0xC);
        if (prop && prop == At<uint8_t*>(playerPed, 0x438) && std::strcmp(suffix, openSuffix) == 0) {
            uint8_t* target = At<uint8_t*>(prop, 0x23C);
            if (target != FirstPoolObject() && At<void*>(prop, 0x194)) {
                AreaTransition_SetTarget(areaTransition, target);
            }
        }
        PropAnimName(At<uint8_t*>(self, 0x40), At<const char*>(self, SuffixAt));
        if (!AnimDict_Find(dict, HashName(animNameBuffer), 0, &index)) {
            At<int32_t>(self, 0x48) = -1;
            return 0;
        }
    } else if (!AnimDict_Find(dict, At<uint32_t>(self, 0x18), 0, &index)) {
        At<int32_t>(self, 0x48) = -1;
        return 0;
    }
    At<int32_t>(self, 0x48) = index;
    int flags = At<int32_t>(self, 0x20) == 2 ? 2 : 0;
    int32_t slotType = At<int32_t>(self, 0x24);
    uint8_t* slot = Anim_GetSlot(nullptr, slotType);
    if (slotType > 0) {
        uint8_t* entry = nullptr;
        const char* group = At<const char*>(self, 0x1C);
        if (group) {
            entry = Entries2C_Find(dict, group, nullptr);
        }
        if (entry || (entry = Entries2C_GetAt(dict, 0)) != nullptr) {
            At<uint8_t*>(slot, 8) = entry;
        }
    }
    float blendIn = At<float>(self, 0x34);
    if (blendIn == 0.0f) {
        flags |= 1;
    } else if (blendIn < 0.0f) {
        blendIn = -1.0f;
    }
    if (self[SyncedAt] != 0) {
        blendIn = 0.0f;
        flags |= 1;
    }
    self[0x44] = 1;
    float statScale = 1.0f;
    if (OwnerPed(At<uint8_t*>(self, 0x40))) {
        statScale = float(double(Ped_GetStat(OwnerPed(At<uint8_t*>(self, 0x40)), 0x14)) / 100.0);
    }
    float speed = At<float>(self, 0x30) * statScale;
    if (AnimManager_Blend(animManager, At<uint8_t*>(self, 0x4C), At<int32_t>(self, 0x48), slot, flags, speed, blendIn)) {
        void* debug = At<void*>(At<uint8_t*>(At<uint8_t*>(self, 0x40), 0), 0x10);
        if (debug) {
            char text[0x100];   // (a debug print compiled out: the buffer never written)
            DebugPrint_44C070(debug, text, 0x100, -1);
        }
    }
    AnimManager_Play(animManager, At<uint8_t*>(self, 0x4C), slot, 0, At<float>(self, 0x28), 0, 0);
    uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x40));
    if (prop) {
        self[0x50] = At<int32_t>(prop, 0xE4) != 0 ? 1 : 0;
        At<int32_t>(prop, 0xE4) = (self[0x3C] != 0 || At<int32_t>(prop, 0xE4) != 0) ? 1 : 0;
    }
    return 0;
}

// 0x657740 (thiscall): PropAnimationParametricTrack::Start: the prop model's own anim
// (+0x60 its suffix) named into +0x18, then the base's start
uint8_t __fastcall PropAnimationParametric_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x40) = owner;
    if (UsePoint_GetEntity(owner) && At<const char*>(self, 0x60)) {
        PropAnimName(owner, At<const char*>(self, 0x60));
        NameKey_FromString(&At<uint32_t>(self, 0x18), animNameBuffer);
    }
    return AnimationParametricTrack_Start(self, At<uint8_t*>(self, 0x40));
}

// 0x657800 (thiscall): PropWeaponFireTrack::Start: the weapon's butes (+0x30, by the
// name +0x20) found; a projectile of the model (+0x28) fired from the prop's bone (+0x24)
// along the bone's up at the butes' +0x64, its speed random between the butes' +0x4C and
// +0x50 (times the ped's stat 0x1F %)
uint8_t __fastcall PropWeaponFire_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    At<uint8_t*>(self, 0x30) = nullptr;
    if (!At<const char*>(self, 0x20)) {
        return 0;
    }
    uint32_t keys[3];
    NameKey_Copy(&keys[0], reinterpret_cast<uint32_t*>(0x01B6531C));
    NameKey_Copy(&keys[1], reinterpret_cast<uint32_t*>(0x01B65394));
    NameKey_Construct(&keys[2], At<const char*>(self, 0x20));
    int32_t path[3];
    HashPath_Set(path, 3, keys);
    At<uint8_t*>(self, 0x30) = FindBoneInfo(path, reinterpret_cast<void*>(0x01B65398));
    for (int i = 2; i >= 0; i--) {
        NameKey_Destruct(&keys[i]);
    }
    float frac = float(double(Rand()) / 32767.0);
    frac = 0.0f + (1.0f - 0.0f) * frac;
    uint8_t* butes = At<uint8_t*>(self, 0x30);
    float speed = (At<float>(butes, 0x50) - At<float>(butes, 0x4C)) * frac + At<float>(butes, 0x4C);
    int32_t stat = Ped_GetStat(OwnerPed(At<uint8_t*>(self, 0x2C)), 0x1F);
    uint8_t* anim = At<uint8_t*>(UsePoint_GetEntity(At<uint8_t*>(self, 0x2C)), 0x114);
    V3 pos{0.0f, 0.0f, 0.0f};
    NodeTransform transform;
    transform.scale = 1.0f;
    void* table = At<void*>(anim, 0x94);
    if (!table) {
        return 0;
    }
    int bone;
    if (!ElementTable_FindById(table, int(HashName(At<const char*>(self, 0x24))), &bone)) {
        return 0;
    }
    if (AnimManager_GetNodePosition(animManager, anim, bone, &pos, 0) != 0) {
        return 0;
    }
    if (AnimManager_GetNodeTransform(animManager, anim, bone, &transform, nullptr) != 0) {
        return 0;
    }
    // a CMatrix attached to the transform (its +0x44 left unset)
    alignas(4) uint8_t matrix[0x48];
    At<NodeTransform*>(matrix, 0x40) = &transform;
    Matrix_FromNode(matrix, &transform);
    V3 velocity;
    VectorScale(&velocity, reinterpret_cast<V3*>(matrix + 0x10), At<float>(At<uint8_t*>(self, 0x30), 0x64));
    V3 vel = velocity;
    int model;
    if (!GetModelInfo(At<const char*>(self, 0x28), &model)) {
        return 0;
    }
    float fireSpeed = float(double(stat) / 100.0) * speed;
    uint8_t* projectile = Projectile_Fire(OwnerPed(At<uint8_t*>(self, 0x2C)), model, pos.x, pos.y, pos.z, vel.x, vel.y,
                                          vel.z, At<uint8_t*>(self, 0x30), fireSpeed, frac, nullptr, 0, 0, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 0.0f, 0, 0);
    uint8_t* info = GetProjectileInstance(At<int32_t>(projectile, 0x238));
    ProjectileInfo_UpdateTrail(info, projectile, 1.0f);
    if (OwnerPed(At<uint8_t*>(self, 0x2C)) == playerPed && model == playerWeaponStatModel) {
        Stats_Increment(0x6B, 1);
    }
    return 0;
}

// 0x657AE0 (thiscall): PlayOnPropTrack::Start: the node (+0x18) played on the prop's
// controller (+0x128) at once; a prop of the gang anim group playing the tag node tells
// the gangs its owner's id with the ped's value (vf 0xAC)
uint8_t __fastcall PlayOnProp_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    uint8_t* node = Node_Get0C(self + 0x18, acttree::OwnerKey(owner));
    if (!node) {
        return 0;
    }
    uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x28));
    if (!prop) {
        return 1;
    }
    PropAnim_4ED940(prop);
    uint8_t* controller = prop + 0x128;
    Ctl_Play(controller, node);
    PropAnim_4ED940(prop);
    Ctl_StartTracks(controller, 0.0f, 0);
    uint8_t* info = ModelInfo_GetAnimInfo(modelInfoPtrs[At<int16_t>(prop, 0x10E)]);
    if (playOnPropAnimGroup == At<int32_t>(info, 0x80)) {
        PropAnim_4ED940(prop);
        if (Ctl_IsPlaying(controller, reinterpret_cast<void*>(0x01B652C8), 1, 1)) {
            uint8_t* ped = OwnerPed(At<uint8_t*>(self, 0x28));
            int32_t id = At<int32_t>(At<uint8_t*>(prop, 0x23C), 0x2C);
            int32_t value = vc::Call<int32_t>(ped, 0xAC);
            Gangs_72C0A0(gangs, id, value);
        }
    }
    return 1;
}

// 0x657BC0 (cdecl): no world geometry (but the see-through models) in the capsule from a
// to b
uint8_t __cdecl AreaIsClearImpl(const V3* a, const V3* b, float radius, int lists) {
    ColPoint points[16];
    for (ColPoint& p : points) {
        p.entity = nullptr;
        p.pos = V3{0.0f, 0.0f, 0.0f};
        p.normal = V3{0.0f, 0.0f, 0.0f};
        p.depth = 3.4028235e38f;
    }
    alignas(4) uint8_t capsule[0x38] = {};
    CameraCapsule_Set(capsule, a, b, radius);
    alignas(4) uint8_t query[0x64];
    CapsuleQuery_Setup(query, capsule, lists, points, 16);
    int32_t count = CapsuleQuery_Scan(query);
    if (count > 0) {
        int32_t n = count;
        for (int32_t i = 0; i < n; i++) {
            int32_t model = At<int16_t>(points[i].entity, 0x10E);
            for (int k = 0; k < 8; k++) {
                if (model == seeThroughModels[k]) {
                    count--;
                    break;
                }
            }
        }
    }
    NameKey_Destruct(query);   // (the query's destructor: nothing)
    for (int i = 15; i >= 0; i--) {
        ReleaseColPoint(points[i]);
    }
    return count == 0 ? 1 : 0;
}

// 0x657D70 (thiscall): PropAttachPropAnimationTrack::Resolve: the blend (+0x4C) of the
// child animation attached at the prop's bone (+0x68)
uint8_t* __fastcall PropAttachPropAnimation_Resolve(uint8_t* self, re::Edx) {
    uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x40));
    uint8_t* anim = At<uint8_t*>(prop, 0x114);
    At<uint8_t*>(self, 0x64) = anim;
    if (!anim || !At<void*>(anim, 0x94)) {
        return prop;
    }
    uint8_t* element = ElementTable_GetElement(At<void*>(anim, 0x94), At<int32_t>(self, 0x68));
    uint8_t* child = At<int32_t>(anim, 0xC4) != 0 ? *At<re::Ptr32<uint8_t>*>(anim, 0xB0) : nullptr;
    while (child) {
        if (At<uint8_t*>(child, 0xC) == element) {
            At<uint8_t*>(self, 0x4C) = At<uint8_t*>(child, 0x14);
            return element;
        }
        uint8_t* next = At<uint8_t*>(child, 0x10);
        if (!next) {
            break;
        }
        next = At<uint8_t*>(next, 8);
        if (!next) {
            break;
        }
        child = *reinterpret_cast<re::Ptr32<uint8_t>*>(next);
    }
    return element;
}

// 0x658160 (thiscall): PropTransferOwnershipTrack::Start: to the ped's partner (+0x155C):
// +0x18 only as its use point's prop, else the prop let go by the ped and taken by it
uint8_t __fastcall PropTransferOwnership_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 1;
    }
    uint8_t* other = At<uint8_t*>(ped, 0x155C);
    if (!other) {
        return 1;
    }
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (self[0x18] != 0) {
        PedUsePoint_Set(other + 0x3BC, prop);
        return 1;
    }
    Prop_ReleasePed(prop, ped);
    re::Ptr32<uint8_t>* ref = &At<uint8_t*>(ped, 0x548);
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
        *ref = nullptr;
    }
    ref = &At<uint8_t*>(ped, 0x438);
    if (*ref) {
        Entity_CleanUpOldReference(*ref, ref);
        *ref = nullptr;
    }
    At<uint32_t>(ped, 0x43C) = 0;
    PropAnim_AttachPed(prop, other);
    return 1;
}

// 0x658210 (thiscall): PropSaveLockerStateTrack::Start: the player's locker marked used
uint8_t __fastcall PropSaveLockerState_Start(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* ped = OwnerPed(owner);
    if (ped && At<int32_t>(ped, 0x1310) == 0xD) {
        uint8_t* prop = At<uint8_t*>(ped, 0x438);
        if (prop && (prop[0x108] & 7) == 6) {
            uint8_t* info = ModelInfo_GetAnimInfo(modelInfoPtrs[At<int16_t>(prop, 0x10E)]);
            if (lockerAnimGroup == At<int32_t>(info, 0x80)) {
                PropAnim_MarkUsed(prop);
            }
        }
    }
    return 1;
}

// 0x658270 (thiscall): PropEntityFlagsTrack::End: the prop's flag (+0x1C) set to +0x18;
// flag 1 set on a crushing prop crushes the vehicles under it
uint8_t __fastcall PropEntityFlags_End(uint8_t* self, re::Edx) {
    if (!UsePoint_GetEntity(At<uint8_t*>(self, 0x20))) {
        return 0;
    }
    int32_t index = At<int32_t>(self, 0x1C);
    uint32_t value = self[0x18];
    At<uint32_t>(UsePoint_GetEntity(At<uint8_t*>(self, 0x20)), 0x1C + index * 4) = value;
    if (At<int32_t>(self, 0x1C) == 1 && self[0x18] == 1) {
        uint8_t* prop = UsePoint_GetEntity(At<uint8_t*>(self, 0x20));
        if (PAnimModelInfo_IsKind2(modelInfoPtrs[At<int16_t>(prop, 0x10E)])) {
            Prop_CrushVehicles(UsePoint_GetEntity(At<uint8_t*>(self, 0x20)));
        }
    }
    return 1;
}

// 0x6582F0 (thiscall): PropRotateProxyTrack::Start: an animated prop turned: +0x1C clear
// of the ped at +0x18 from facing it (its first heading kept at +0x124); else the
// player's turned towards the first of three clear directions in a row (20 degree steps
// from the camera's view, 0.85 out at 1.2 up), others kept as they are
uint8_t __fastcall PropRotateProxy_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    uint8_t* ped = OwnerPed(owner);
    if (!ped) {
        return 0;
    }
    uint8_t* prop = UsePoint_GetEntity(owner);
    if (!prop || !ModelInfo_IsPropAnim(modelInfoPtrs[At<int16_t>(prop, 0x10E)])) {
        return 1;
    }
    V3 dir{0.0f, 0.0f, 0.0f};
    if (self[0x1C] == 0) {
        V3 d;
        VectorSub(&d, Pos(ped), Pos(prop));
        dir = d;
        dir.z = 0.0f;
        Vector_Normalise(&dir);
        float angle = AngleBetweenPoints(dir.x, dir.y, 0.0f, 0.0f);
        float heading = LimitRadianAngle(At<float>(self, 0x18) + angle);
        if (At<float>(prop, 0x124) == 99999997952.0f) {
            At<float>(prop, 0x124) = Entity_GetHeading(prop);
        }
        Entity_SetHeadingInPlace(prop, heading);
        return 1;
    }
    if (At<int32_t>(ped, 0x1310) != 0xD) {
        Entity_SetHeadingInPlace(prop, Entity_GetHeading(prop));
        return 1;
    }
    V3 propPos = *reinterpret_cast<V3*>(Pos(prop));
    V3 camPos = *reinterpret_cast<V3*>(Camera_GetPosition(theCamera));
    V3 view{0.0f, 0.0f, 0.0f};
    V3 t;
    VectorSub(&t, &camPos.x, &propPos.x);
    float distSq = t.x * t.x + t.y * t.y;
    if (distSq < 0.0025f) {
        V3 out;
        view = *Camera_GetViewDirection(theCamera, &out);
    } else {
        view = *VectorSub(&t, &propPos.x, &camPos.x);
    }
    view.z = 0.0f;
    Vector_Normalise(&view);
    propPos.z = propPos.z + 1.2f;
    ColPoint colPoint;
    ColPoint_Construct(&colPoint);
    re::Ptr32<uint8_t> hitEntity = nullptr;
    ignoreEntity = nullptr;
    ClearExtraEntities();
    AddExtraEntity(SeeThrough(7));
    AddExtraEntity(SeeThrough(6));
    AddExtraEntity(SeeThrough(4));
    AddExtraEntity(SeeThrough(3));
    AddExtraEntity(SeeThrough(2));
    AddExtraEntity(SeeThrough(1));
    AddExtraEntity(SeeThrough(5));
    // the straight line out first (its results unused)
    V3 offset{view.x * 0.85f, view.y * 0.85f, view.z * 0.85f};
    losFlag_C1AE82 = 1;
    V3 end;
    V3 endCopy = *VectorAdd(&end, &propPos.x, &offset.x);
    AreaIsClear(&propPos, &endCopy, 0.35f, 0x1B);
    ProcessLineOfSight(&propPos, &endCopy, &colPoint, &hitEntity, 0x1B, 0, 0);
    ClearExtraEntities();
    float baseHeading = cameraHeading_C3CD08;
    int run = 0;
    losFlag_C1AE82 = 0;
    int best = 0;
    float zOffset = 0.0f * 0.85f;
    bool found = false;
    int deg = 0;
    for (; deg < 400; deg += 20) {
        float a = float(deg) * 3.14159274f / 180.0f;
        float c = trk::CrtCos(a);
        float s = trk::CrtSin(a);
        float rx = view.x * c - view.y * s;
        float ry = view.y * c + view.x * s;
        V3 off{rx * 0.85f, 0.85f * ry, zOffset};
        V3 sum;
        V3 probe = *VectorAdd(&sum, &propPos.x, &off.x);
        if (AreaIsClear(&propPos, &probe, 0.35f, 0x1B)) {
            run++;
            if (run == 3) {
                found = true;
                break;
            }
            best = deg;
        } else {
            run = 0;
        }
    }
    int turn = found ? deg - 20 : best;
    float heading = (float(turn) + 180.0f) * 3.14159274f / 180.0f - baseHeading;
    Entity_SetHeadingInPlace(prop, heading);
    ReleaseColPoint(colPoint);
    return 1;
}

} // namespace

// (variadic: the template arguments' commas)
#define PROPTRK3_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

PROPTRK3_REVERSED("PropAnimationTrack::Start", 0x00657420, &PropAnimation_Start<0x58, 0x5C, 0>);
PROPTRK3_REVERSED("PropAnimationParametricTrack::Start", 0x00657740, &PropAnimationParametric_Start);
PROPTRK3_REVERSED("PropWeaponFireTrack::Start", 0x00657800, &PropWeaponFire_Start);
PROPTRK3_REVERSED("PlayOnPropTrack::Start", 0x00657AE0, &PlayOnProp_Start);
PROPTRK3_REVERSED("Prop::AreaIsClear_657BC0", 0x00657BC0, &AreaIsClearImpl);
PROPTRK3_REVERSED("PropAttachPropAnimationTrack::Resolve", 0x00657D70, &PropAttachPropAnimation_Resolve);
PROPTRK3_REVERSED("PropAttachPropAnimationTrack::Start", 0x00657DF0, &PropAnimation_Start<0x5C, 0x60, 0x58>);
PROPTRK3_REVERSED("PropTransferOwnershipTrack::Start", 0x00658160, &PropTransferOwnership_Start);
PROPTRK3_REVERSED("PropSaveLockerStateTrack::Start", 0x00658210, &PropSaveLockerState_Start);
PROPTRK3_REVERSED("PropEntityFlagsTrack::End", 0x00658270, &PropEntityFlags_End);
PROPTRK3_REVERSED("PropRotateProxyTrack::Start", 0x006582F0, &PropRotateProxy_Start);
